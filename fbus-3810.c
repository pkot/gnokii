	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  fbus-3810.c  Version 0.2.5
	
	   Provides an API for accessing functions on the 3810 and
	   similar phones.  Most testing has been done on the 3810
       so far.  The code relies on the pthread library to allow
	   communications to the phone to be run independantly of 
	   mailine code.

	   The various routines are called FB38 (whatever) as a
	   concatenation of FBUS and 3810.
	  
	   These functions are only ever called through the GSM_Functions
	   structure defined in gsm-common.h and set up in gsm-api.c */

#define		__fbus_3810_c	/* "Turn on" prototypes in fbus-3810.h */

#include	<termios.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<string.h>
#include	<pthread.h>
#include	<errno.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"fbus-3810.h"

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					FB38_LinkOK;

GSM_Functions			FB38_Functions = {
		FB38_Initialise,
		FB38_Terminate,
		FB38_GetPhonebookLocation,
		FB38_WritePhonebookLocation,
		FB38_GetMemoryStatus,
		FB38_GetSMSStatus,
		FB38_GetSMSMessage,
		FB38_DeleteSMSMessage,
		FB38_SendSMSMessage,
		FB38_GetRFLevel,
		FB38_GetBatteryLevel,
		FB38_GetPowerSource,
		FB38_EnterPin,
		FB38_GetIMEI,
		FB38_GetRevision,
		FB38_GetModel,
		FB38_GetDateTime,
		FB38_SetDateTime,
		FB38_GetAlarm,
		FB38_SetAlarm,
		FB38_DialVoice,
		FB38_GetIncomingCallNr
};

GSM_Information			FB38_Information = {
		"3110|3810|8110|8110i",	/* Models */
		4, 						/* Max RF Level */
		0,						/* Min RF Level */
		GRF_Arbitrary,			/* RF level units */
		4, 						/* Max Battery Level */
		0,						/* Min Battery Level */
		GBU_Arbitrary,			/* Battery level units */
		GDT_None,				/* No date/time support */
		GDT_None,				/* No alarm support */
		0						/* Max alarms = 0 */
};

	/* Local variables */
enum FB38_RX_States		RX_State;
int						MessageLength;
u8						MessageBuffer[FB38_MAX_RECEIVE_LENGTH];
unsigned char			MessageCSum;
int						BufferCount;
unsigned char			CalculatedCSum;
int						PortFD;
char					PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
u8						RequestSequenceNumber;
pthread_t				Thread;
bool					RequestTerminate;
struct termios			OldTermios;	/* To restore termio on close. */
bool					EnableMonitoringOutput;
bool					DisableKeepalive;
float					CurrentRFLevel;
float					CurrentBatteryLevel;

	/* These three are the information returned by AT+CGSN, AT+CGMR and
	   AT+CGMM commands respectively. */
char					IMEI[FB38_MAX_IMEI_LENGTH];
bool					IMEIValid;
char					Revision[FB38_MAX_REVISION_LENGTH];
bool					RevisionValid;
char					Model[FB38_MAX_MODEL_LENGTH];
bool					ModelValid;

	/* We have differing lengths for internal memory vs SIM memory.
	   internal memory is index 0, SIM index 1. */
int						MaxPhonebookNumberLength[2];
int						MaxPhonebookNameLength[2];

	/* Local variables used by get/set phonebook entry code. 
	   ...Buffer is used as a source or destination for phonebook
	   data, ...Error is set to GE_None by calling function, set to 
	   GE_COMPLETE or an error code by handler routines as appropriate. */

GSM_PhonebookEntry		*CurrentPhonebookEntry;
GSM_Error				CurrentPhonebookError;

	/* Local variables used by send/retrieve SMS message code. */
int						CurrentSMSMessageBodyLength;
GSM_SMSMessage			*CurrentSMSMessage;
GSM_Error				CurrentSMSMessageError;
u8						CurrentSMSSendResponse[2];	/* Set by 0x28/29 messages */
bool					SMSBlockAckReceived;		/* Set when block ack'd */
bool					SMSHeaderAckReceived;		/* Set when header ack'd */

	/* The following functions are made visible to gsm-api.c and friends. */

	/* Initialise variables and state machine. */
GSM_Error	FB38_Initialise(char *port_device, bool enable_monitoring)
{
	int		rtn;

	RequestTerminate = false;
	FB38_LinkOK = false;
	EnableMonitoringOutput = enable_monitoring;
	DisableKeepalive = false;
	CurrentRFLevel = -1;
	CurrentBatteryLevel = -1;
	IMEIValid = false;
	RevisionValid = false;
	ModelValid = false;

	strncpy (PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) FB38_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (GE_INTERNALERROR);
    }

	return (GE_NONE);

}

	/* Applications should call FB38_Terminate to shut down
	   the FB38 thread and close the serial port. */
void		FB38_Terminate(void)
{
		/* Request termination of thread */
	RequestTerminate = true;

		/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

		/* Close serial port. */
	tcsetattr(PortFD, TCSANOW, &OldTermios);
	
	close (PortFD);
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	FB38_GetPhonebookLocation(GSM_MemoryType memory_type, int location, GSM_PhonebookEntry *entry)
{
	int		memory_area;
	int		timeout;

		/* State machine code writes data to these variables when
		   it comes in. */
	CurrentPhonebookEntry = entry;
	CurrentPhonebookError = GE_BUSY;

	if (memory_type == GMT_INTERNAL) {
		memory_area = 1;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = 2;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

	timeout = 20; 	/* 2 seconds for command to complete */

		/* Return if no link has been established. */
	if (!FB38_LinkOK) {
		return GE_NOLINK;
	}

		/* Send request */
 	FB38_TX_Send0x43_RequestMemoryLocation(memory_area, location);
	
		/* Wait for timeout or other error. */
	while (timeout != 0 && CurrentPhonebookError == GE_BUSY) {

		timeout --;
		if (timeout == 0) {
			return (GE_TIMEOUT);
		}
		usleep (100000);
	}

	return (CurrentPhonebookError);
}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	FB38_WritePhonebookLocation(int location, GSM_PhonebookEntry *entry)
{

	int		memory_area;
	int		timeout;
	int		index;

		/* Make sure neither name or number is too long.  We assume (as
		   has been reported that memory_area 1 is internal, 2 is the SIM */
	if (entry->MemoryType == GMT_INTERNAL) {
		memory_area = 1;
	}
	else {
		if (entry->MemoryType == GMT_SIM) {
			memory_area = 2;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

	index = 0;
	if (memory_area == 2) {
		index = 1;
	}

	if (strlen(entry->Name) > MaxPhonebookNameLength[index]) {
		return (GE_PHBOOKNAMETOOLONG);
	}

	if (strlen(entry->Number) > MaxPhonebookNumberLength[index]) {
		return (GE_PHBOOKNUMBERTOOLONG);
	}

		/* Set error flag to busy */
	CurrentPhonebookError = GE_BUSY;

	timeout = 40; 	/* 4 seconds - can be quite slow ! */

	if (!FB38_LinkOK) {
		return GE_NOLINK;
	}

		/* Send message. */
 	if (FB38_TX_Send0x42_WriteMemoryLocation(memory_area, location, entry->Name, entry->Number) != 0) {
		return (GE_INTERNALERROR);
	}

	while (timeout != 0 && CurrentPhonebookError == GE_BUSY) {

		timeout --;
		if (timeout == 0) {
			return (GE_TIMEOUT);
		}
		usleep (100000);
	}
	return (CurrentPhonebookError);
}

GSM_Error	FB38_GetSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message)
{
	int		timeout;
	int		memory_area;

	if (memory_type == GMT_INTERNAL) {
		memory_area = 1;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = 2;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

		/* State machine code writes data to these variables when
		   it comes in. */
	CurrentSMSMessage = message;
	CurrentSMSMessageError = GE_BUSY;

	timeout = 50; 	/* 5 seconds for command to complete */

		/* Return if no link has been established. */
	if (!FB38_LinkOK) {
		return GE_NOLINK;
	}

		/* Send request */
	FB38_TX_Send0x25_RequestSMSMemoryLocation(memory_area, location);

		/* Wait for timeout or other error. */
	while (timeout != 0 && CurrentSMSMessageError == GE_BUSY) {

		timeout --;
		if (timeout == 0) {
			return (GE_TIMEOUT);
		}
		usleep (100000);
	}

	return(CurrentSMSMessageError);
}

GSM_Error	FB38_DeleteSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message)
{
	int		timeout;
	int		memory_area;

	if (memory_type == GMT_INTERNAL) {
		memory_area = 1;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = 2;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

		/* State machine code writes data to these variables when
		   it comes in. */
	CurrentSMSMessage = message;
	CurrentSMSMessageError = GE_BUSY;

	timeout = 50; 	/* 5 seconds for command to complete */

		/* Return if no link has been established. */
	if (!FB38_LinkOK) {
		return GE_NOLINK;
	}

		/* Send request */
	FB38_TX_Send0x26_DeleteSMSMemoryLocation(memory_area, location);

		/* Wait for timeout or other error. */
	while (timeout != 0 && CurrentSMSMessageError == GE_BUSY) {

		timeout --;
		if (timeout == 0) {
			return (GE_TIMEOUT);
		}
		usleep (100000);
	}

	return(CurrentSMSMessageError);
}

GSM_Error	FB38_SendSMSMessage(GSM_SMSMessage *SMS)
{
	int		timeout;
	int		text_offset;
	int		text_length;
	int		text_remaining;
	u8		block_count;
	u8		block_length;
	int		retry_count;

		/* Return if no link has been established. */
	if (!FB38_LinkOK) {
		return GE_NOLINK;
	}

		/* Get and check total length, */
	text_length = strlen(SMS->MessageText);

	if (text_length > 160) {
		return GE_SMSTOOLONG;
	}

		/* We have a loop here as if the response from the phone is
		   0x65 0x26 the rule appears to be just to try sending the
		   message again.  We do this a maximum of FB38_SMS_SEND_RETRY_COUNT
		   times before giving up.  This value is empirical only! */
	retry_count = FB38_SMS_SEND_RETRY_COUNT;

	while (retry_count > 0) {

			/* State machine code writes data to these variables when
			   responses etc comes in. */
		CurrentSMSMessageError = GE_BUSY;
		SMSBlockAckReceived = false;
		SMSHeaderAckReceived = false;

			/* Don't send keepalive during SMS send. */
		DisableKeepalive = true;

			/* Send header */
		FB38_TX_Send0x23_SendSMSHeader(SMS->MessageCentre, SMS->Destination, text_length);

		timeout = 20; 	/* 2 seconds for command to complete */

			/* Wait for timeout or header ack. */
		while (timeout != 0 && SMSHeaderAckReceived == false) {
			timeout --;
			if (timeout == 0) {
				DisableKeepalive = false;
				return (GE_TIMEOUT);
			}
			usleep (100000);
		}

			/* Now send as many blocks of maximum 55 characters as required
			   to send complete message. */
		block_count = 1;
		text_offset = 0;
		text_remaining = text_length;

		while (text_remaining > 0) {
			block_length = text_remaining;

				/* Limit length */
			if (block_length > 55) {
				block_length = 55;
			}

				/* Clear acknowledge received flag and send message. */
			SMSBlockAckReceived = false;		
			FB38_TX_Send0x27_SendSMSMessageText(block_count, block_length, SMS->MessageText + text_offset);

				/* update remaining and offset values for next time. */
			text_remaining -= block_length;
			text_offset += block_length;
			block_count ++;
	
			timeout = 20;	/* 2 seconds. */
	
				/* Wait for block to be acknowledged. */
			while (timeout != 0 && SMSBlockAckReceived == false) {
				timeout --;
				if (timeout == 0) {
					DisableKeepalive = false;
					return (GE_TIMEOUT);
					}
				usleep (100000);
			}
		}

			/* Now wait for response from network which will see
			   CurrentSMSMessageError change from busy. */

		timeout = 1200;	/* 120 seconds network wait. */

		while (timeout != 0 && CurrentSMSMessageError == GE_BUSY) {

			timeout --;
			if (timeout == 0) {
				DisableKeepalive = false;
				return (GE_TIMEOUT);
			}
			usleep (100000);
		}

			/* New code here - commenting it in this manner as it's a bit
			   late and hence I may be missing something.  Think this
		 	   bug crept in between 1.5 and 1.6 - even if SMS was sent
			   OK would still retry! HAB 19990517 */
		if (CurrentSMSMessageError == GE_SMSSENDOK) {
			DisableKeepalive = false;
			return(CurrentSMSMessageError);
		}

			/* Indicate attempt failed and show attempt number and number
			   of attempts remaining. */
		fprintf(stderr, _("SMS Send attempt failed, trying again (%d of %d)\n"), 			(retry_count - FB38_SMS_SEND_RETRY_COUNT + 1), FB38_SMS_SEND_RETRY_COUNT);
			/* Got a retry response so try again! */
		retry_count --;

			/* After an empirically determined pause... */
			usleep(500000);	/* 0.5 seconds. */
	}

		/* Retries must have failed. */
	DisableKeepalive = false;
	return(CurrentSMSMessageError);
}

	/* FB38_GetRFLevel
	   FIXME (sort of...)
	   For now, GetRFLevel and GetBatteryLevel both rely
	   on data returned by the "keepalive" packets.  I suspect
	   that we don't actually need the keepalive at all but
	   will await the official doco before taking it out.  HAB19990511 */
GSM_Error	FB38_GetRFLevel(float *level)
{
	if (CurrentRFLevel == -1) {
		return (GE_INTERNALERROR);
	}
	else {
		*level = CurrentRFLevel;
		return (GE_NONE);
	}
}

	/* FB38_GetBatteryLevel
	   FIXME (see above...) */
GSM_Error	FB38_GetBatteryLevel(float *level)
{
	if (CurrentBatteryLevel == -1) {
		return (GE_INTERNALERROR);
	}
	else {
		*level = CurrentBatteryLevel;
		return (GE_NONE);
	}
}

GSM_Error	FB38_GetIMEI(char *imei)
{
	if (IMEIValid) {
		strncpy (imei, IMEI, FB38_MAX_IMEI_LENGTH);
		return (GE_NONE);
	}
	else {
		return (GE_INTERNALERROR);
	}
}

GSM_Error	FB38_GetRevision(char *revision)
{
	if (RevisionValid) {
		strncpy (revision, Revision, FB38_MAX_REVISION_LENGTH);
		return (GE_NONE);
	}
	else {
		return (GE_INTERNALERROR);
	}
}

GSM_Error	FB38_GetModel(char *model)
{
	if (ModelValid) {
		strncpy (model, Model, FB38_MAX_MODEL_LENGTH);
		return (GE_NONE);
	}
	else {
		return (GE_INTERNALERROR);
	}
}

	/* Our "Not implemented" functions */
GSM_Error	FB38_GetMemoryStatus(GSM_MemoryStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_GetSMSStatus(GSM_SMSStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}


GSM_Error	FB38_GetPowerSource(GSM_PowerSource *source)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_EnterPin(char *pin)
{
	return (GE_NOTIMPLEMENTED);
}


GSM_Error	FB38_GetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_DialVoice(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB38_GetIncomingCallNr(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Everything from here down is internal to 3810 code. */


	/* This is the main loop for the FB38 functions.  When FB38_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the FB38_Terminate function. */
void	FB38_ThreadLoop(void)
{
	unsigned char 		init_char[1] = {0x55};
	int					count, idle_timer;

		/* Initialise RX state machine. */
	BufferCount = 0;
	RX_State = FB38_RX_Sync;

	CurrentPhonebookEntry = NULL;

		/* Set max lengths to default - eventually we'll work out
		   how to interrogate the phone for this information! */
	MaxPhonebookNumberLength[0] = FB38_DEFAULT_INT_PHONEBOOK_NUMBER_LENGTH;
	MaxPhonebookNameLength[0] = FB38_DEFAULT_INT_PHONEBOOK_NAME_LENGTH;

	MaxPhonebookNumberLength[1] = FB38_DEFAULT_SIM_PHONEBOOK_NUMBER_LENGTH;
	MaxPhonebookNameLength[1] = FB38_DEFAULT_SIM_PHONEBOOK_NAME_LENGTH;

		/* Try to open serial port, if we fail we sit here and don't proceed
		   to the main loop. */
	if (FB38_OpenSerial() != true) {
		FB38_LinkOK = false;
		
		while (!RequestTerminate) {
			usleep (100000);
		}
		return;
	}

		/* Initialise sequence number used when sending messages
		   to phone. */
	RequestSequenceNumber = 0x10;

		/* Send init string to phone, this is a bunch of 0x55
		   characters.  Timing is empirical. */
	for (count = 0; count < 100; count ++) {
		usleep(1000);
		write(PortFD, init_char, 1);
	}

		/* Now send the 0x15 message, the exact purpose is not understood
		   but if not sent, link doesn't work. */
	FB38_TX_Send0x15Message(0x11);

		/* We've now finished initialising things so sit in the loop
		   until told to do otherwise.  Loop doesn't do much other
		   than send periodic keepalive messages to phone.  This
		   loop will become more involved once we start doing 
		   fax/data calls. */

		/* Send IMEI/Revision/Model request */

	idle_timer = 0;

	while (!RequestTerminate) {
		if (idle_timer == 0) {
				/* Dont send keepalive packets when doing other transactions. */
			if (!DisableKeepalive) {
				FB38_TX_Send0x4aMessage();
					/* FIXME - Don't need to do this over and over... */
				FB38_TX_Send0x4c_RequestIMEIRevisionModelData();
			}
			idle_timer = 20;
		}
		else {
			idle_timer --;
		}

		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}

}

	/* Called by initialisation code to open comm port in
	   asynchronous mode. */
bool		FB38_OpenSerial(void)
{
	struct termios			new_termios;
	struct sigaction		sig_io;

		/* Open device. */
	PortFD = open (PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (PortFD < 0) {
		perror(_("Couldn't open FB38 device: "));
		return (false);
	}

		/* Set up and install handler before enabling async IO on port. */
	sig_io.sa_handler = FB38_SigHandler;
	sig_io.sa_flags = 0;
	sigaction (SIGIO, &sig_io, NULL);

		/* Allow process/thread to receive SIGIO */
	fcntl(PortFD, F_SETOWN, getpid());

		/* Make filedescriptor asynchronous. */
	fcntl(PortFD, F_SETFL, FASYNC);

		/* Save current port attributes so they can be restored on exit. */
	tcgetattr(PortFD, &OldTermios);

		/* Set port settings for canonical input processing */
	new_termios.c_cflag = FB38_BAUDRATE | CS8 | CLOCAL | CREAD;
	new_termios.c_iflag = IGNPAR;
	new_termios.c_oflag = 0;
	new_termios.c_lflag = 0;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;

	tcflush(PortFD, TCIFLUSH);
	tcsetattr(PortFD, TCSANOW, &new_termios);

	return (true);
}

	/* Handler called when characters received from serial port. 
	   calls state machine code to process it. */
void	FB38_SigHandler(int status)
{
	unsigned char 	buffer[255];
	int				count,res;

	res = read(PortFD, buffer, 255);

	for (count = 0; count < res ; count ++) {
		FB38_RX_StateMachine(buffer[count]);
	}
}

	/* RX_State machine for receive handling.  Called once for each
	   character received from the phone/phone. */
void	FB38_RX_StateMachine(char rx_byte)
{

	switch (RX_State) {
	
					/* Phone is currently off.  Wait for 0x55 before
					   restarting */
		case FB38_RX_Off:
				if (rx_byte != 0x55)
					break;

				/* Seen 0x55, restart at 0x04 */
				if (EnableMonitoringOutput == true) {
					fprintf(stdout, _("restarting.\n"));
				}

				RX_State = FB38_RX_Sync;

				/*FALLTHROUGH*/

					/* Messages from the phone start with an 0x04.  We
					   use this to "synchronise" with the incoming data
					   stream. */		
		case FB38_RX_Sync:
				if (rx_byte == 0x04) {
					BufferCount = 0;
					CalculatedCSum = rx_byte;
					RX_State = FB38_RX_GetLength;
				}
				break;
		
					/* Next byte is the length of the message including
					   the message type byte but not including the checksum. */
		case FB38_RX_GetLength:
				MessageLength = rx_byte;
				CalculatedCSum ^= rx_byte;
				RX_State = FB38_RX_GetMessage;
				break;

					/* Get each byte of the message.  We deliberately
					   get one too many bytes so we get the checksum
					   here as well. */
		case FB38_RX_GetMessage:
				MessageBuffer[BufferCount] = rx_byte;
				BufferCount ++;

				if (BufferCount >= FB38_MAX_RECEIVE_LENGTH) {
					RX_State = FB38_RX_Sync;		/* Should be PANIC */
				}
					/* If this is the last byte, it's the checksum */
				if (BufferCount > MessageLength) {

					MessageCSum = rx_byte;
						/* Compare against calculated checksum. */
					if (MessageCSum == CalculatedCSum) {
						/* Got checksum, matches calculated one so 
						   now pass to dispatch handler. */
						RX_State = FB38_RX_DispatchMessage();
					}
						/* Checksum didn't match so ignore. */
					else {
						fprintf(stderr, _("CS Fail %02x != %02x"), MessageCSum, CalculatedCSum);
						FB38_RX_DisplayMessage();
						fflush(stderr);
						RX_State = FB38_RX_Sync;
					}
					
				}
				CalculatedCSum ^= rx_byte;
				break;

		default:
	}
}

	/* FB38_RX_DispatchMessage
	   Once we've received a message from the phone, the command/message
	   type byte is used to call an appropriate handler routine or
	   simply acknowledge the message as required.  The rather verbose
	   names given to the handler routines reflect that the names
	   of the purpose of the handler are best guesses only hence both
	   the byte (0x0b) and the purpose (IncomingCall) are given. */
enum FB38_RX_States		FB38_RX_DispatchMessage(void)
{
	/* Uncomment this if you want all messages in raw form. */
	/*FB38_RX_DisplayMessage();*/

		/* Switch on the basis of the message type byte */
	switch (MessageBuffer[0]) {
	
			/* 0x4a message is a response to our 0x4a request, assumed to
			   be a keepalive message of sorts.  No response required. */
		case 0x4a:	break;

			/* 0x4b messages are sent by phone in response (it seems)
			   to the keep alive packet.  We must acknowledge these it seems
			   by sending a response with the "sequence number" byte loaded
			   appropriately. */
		case 0x4b:	FB38_RX_Handle0x4b_Status();
					break;

			/* 0x0b messages are sent by phone when an incoming call occurs,
			   this message must be acknowledged. */
		case 0x0b:	FB38_RX_Handle0x0b_IncomingCall();
					break;

			/* Fairly self explanatory these two, though the outgoing 
			   call message has three (unexplained) data bytes. */
		case 0x0e:	FB38_RX_Handle0x0e_OutgoingCallAnswered();
					break;

		case 0x0d:	FB38_RX_Handle0x0d_IncomingCallAnswered();
					break;

			/* 0x10 messages are sent by the phone when an outgoing
			    call terminates. */
		case 0x10:	FB38_RX_Handle0x10_EndOfOutgoingCall();
					break;

			/* 0x11 messages are sent by the phone when an incoming call
			   terminates.  There is some other data in the message, 
			   purpose as yet undertermined. */
		case 0x11: 	FB38_RX_Handle0x11_EndOfIncomingCall();
					break;

			/* 0x12 messages are sent after the 0x10 message at the 
			   end of an outgoing call.  Significance of two messages
			   versus the one at the end of an incoming call  is as 
			   yet undertermined. */
		case 0x12: 	FB38_RX_Handle0x12_EndOfOutgoingCall();
					break;

			/* 0x13 messages are sent after the phone restarts. 
			   Re-initialise */
		case 0x13:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
				RequestSequenceNumber = 0x10;
				FB38_TX_Send0x15Message(0x11);
				break;
			
			/* 0x15 messages are sent by the phone in response to the
			   init sequence sent so we don't acknowledge them! */
		case 0x15:	if (EnableMonitoringOutput == true) {
						fprintf(stdout, _("0x15 Registration Response 0x%02x\n"), MessageBuffer[1]);
					}
					DisableKeepalive = false;
					break;

			/* 0x16 messages are sent by the phone during initialisation,
			   after the response to the 0x15 message.  Purpose is unclear, 
			   however the sequence bytes have been observed to change with 
			   differing software versions.
			   V06.61 (19/08/97) sends 0x10 0x02, V07.02 (17/03/98) sends 
			   0x30 0x02.  The actual data byte (0x02) is unchanged. 
			   Go figure :) */ 
		case 0x16:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					if (EnableMonitoringOutput == true) {
						fprintf(stdout, _("0x16 Registration Response 0x%02x 0x%02x\n"), MessageBuffer[1], MessageBuffer[2]);
					}
					break;

			/* We send 0x23 messages to phone as a header for outgoing SMS
			   messages.  So we don't acknowledge it. */
		case 0x23:  SMSHeaderAckReceived = true;
					break;


			/* We send 0x25 messages to phone to request an SMS message
			   be dumped.  Thus we don't acknowledge it. */
		case 0x25:	break;


			/* We send 0x26 messages to phone to delete an SMS message
			   so it's not acknowledged. */
		case 0x26:	break;


			/* 0x27 messages are a little different in that both ends of
			   the link send them.  The PC sends them with SMS message
			   text as does the phone. */
		case 0x27:	FB38_RX_Handle0x27_SMSMessageText();
					break;

			/* 0x28 messages are sent by the phone to acknowledge succesfull
			   sending of an SMS message.  The byte returned is a receipt
			   number of some form, not sure if it's from the network, sending
			   phone or receiving phone. */
		case 0x28:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					CurrentSMSSendResponse[0] = MessageBuffer[2];
					CurrentSMSSendResponse[1] = 0;
					CurrentSMSMessageError = GE_SMSSENDOK;
					break;

			/* 0x29 messages are sent by the phone to indicate an error in
			   sending an SMS message.  Observed values are 0x65 0x15 when
			   the phone originated SMS was disabled by the network for
			   the particular phone.  0x65 0x26 was observed too, whereupon
			   the message was retried. */
		case 0x29:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					CurrentSMSSendResponse[0] = MessageBuffer[2];
					CurrentSMSSendResponse[1] = MessageBuffer[3];
					CurrentSMSMessageError = GE_SMSSENDFAILED;
					break;

			/* 0x2c messages are generated by the phone when we request
			   an SMS message with an 0x25 message.  Fields seem to be
			   largely the same as the 0x30 notification.  Immediately
			   after the 0x2c nessage, the phone sends 0x27 message(s) */
		case 0x2c:	FB38_RX_Handle0x2c_SMSHeader();
					break;

			/* 0x2d messages are generated when an SMS message is requested
			   that does not exist or is empty. */
		case 0x2d:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					if (MessageBuffer[2] == 0x74) {
						CurrentSMSMessageError = GE_INVALIDSMSLOCATION;
					}
					else {
						CurrentSMSMessageError = GE_EMPTYSMSLOCATION;
					}
	
					break;

			/* 0x2e messages are generated when an SMS message is deleted
			   successfully. */
		case 0x2e:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					CurrentSMSMessageError = GE_NONE;
					break;

			/* 0x2f messages are generated when an SMS message is deleted
			   that does not exist.  Unlike responses to a getsms message
			   no error is returned when the entry is already empty */
		case 0x2f:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					if (MessageBuffer[2] == 0x74) {
						CurrentSMSMessageError = GE_INVALIDSMSLOCATION;
					}
						/* Note 0x74 is the only value that has been seen! */
					else {
						CurrentSMSMessageError = GE_EMPTYSMSLOCATION;
					}
	
					break;
	
			/* 0x30 messages are generated by the phone when an incoming
			   SMS message arrives.  Message contains date, time, message 
			   number in phones memory as well as other (presently undetermined
			   information...) */
		case 0x30:	FB38_RX_Handle0x30_IncomingSMSNotification();
					break;

			/* We send an 0x3f message to the phone to request a different
			   type of status dump - this one seemingly concerned with 
			   SMS message centre details.  Phone responds with an ack to
			   our 0x3f request then sends an 0x41 message that has the
			   actual data in it. */
		case 0x3f:	break;	/* Don't send ack. */

			/* 0x41 Messages are sent in response to an 0x3f request. */
		case 0x41: 	FB38_RX_Handle0x41_SMSMessageCentreData();
					break;

			/* We send 0x42 messages to write a SIM location. */
		case 0x42:	break;

			/* 0x43 is a message we send to request data from a memory
			   location. The phone returns and acknowledge for the 
			   0x43 command then sends an 0x46 command with the actual data. */
		case 0x43:	break;

			/* 0x44 is sent by phone to acknowledge that phonebook location	
			   was written correctly. */
		case 0x44:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					CurrentPhonebookError = GE_NONE;
					break;

			/* 0x45 is sent by phone if a write to a phonebook location
			   failed. */
		case 0x45:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					CurrentPhonebookError = GE_INVALIDPHBOOKLOCATION;
					break;
	
			/* 0x46 is sent after an 0x43 response with the requested data. */
		case 0x46: 	FB38_RX_Handle0x46_MemoryLocationData();
					break;

			/* 0x47 is sent if the location requested in an 0x43 message is
			   invalid or unavailable (such as immediately after the phone
			   is switched on. */
		case 0x47:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					CurrentPhonebookError = GE_INVALIDPHBOOKLOCATION;
					break;

			/* 0x48 is sent during power-on of the phone, after the 0x13
			   message is received and the PIN (if any) has been entered
			   correctly. */
		case 0x48:	FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					if (EnableMonitoringOutput == true) {
						fprintf(stdout, _("PIN [possibly] entered.\n"));
					}
					break;

			/* 0x49 is sent when the phone is switched off.  Disable
			   keepalives and wait for 0x55 from the phone.  */
		case 0x49:	DisableKeepalive = true;
					FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					if (EnableMonitoringOutput == true) {
						fprintf(stdout, _("Phone powering off..."));
						fflush(stdout);
					}

					return FB38_RX_Off;
			/* We send 0x4c to request IMEI, Revision and Model info. */
		case 0x4c:	break;

			/* 0x4d Message provides IMEI, Revision and Model information. */
		case 0x4d:	FB38_RX_Handle0x4d_IMEIRevisionModelData();
					break;

			/* Here we  attempt to acknowledge and display messages we don't
			   understand fully... The phone will send the same message
			   several (5-6) times before giving up if no ack is received.
			   Phone also appears to refuse to send any of that message type
			   again until an init sequence is done again. */
		default:	if (FB38_TX_SendStandardAcknowledge(MessageBuffer[0]) != true) {
						fprintf(stderr, _("Standard Ack write failed!"));
					}
						/* Now display unknown message to user. */
		 			FB38_RX_DisplayMessage();
					break;
	}

	return FB38_RX_Sync;
}

	/* FB38_RX_DisplayMessage
	   Called when a message we don't know about is received so that
	   the user can see what is going back and forth, and perhaps shed
	   some more light/explain another message type! */
	
void	FB38_RX_DisplayMessage(void)
{
	int		count;
	int		line_count; 

		/* If monitoring output is disabled, don't display anything. */
	if (EnableMonitoringOutput == false) {
		return;
	}
	
	line_count = 0;
	fprintf(stdout, _("Unknown: "));

	fprintf(stdout, _("Msg Type: %02x "), MessageBuffer[0]);
	fprintf(stdout, _("Msg Len: %02x "), MessageLength);
	fprintf(stdout, _("Sequence Number: %02x "), MessageBuffer[1]);
	fprintf(stdout, _("Checksum: %02x \n   "), MessageCSum);

	for (count = 2; count < MessageLength; count ++) {
		if (isprint(MessageBuffer[count])) {
			fprintf(stdout, "[%02x%c]", MessageBuffer[count], MessageBuffer[count]);
		}
		else {
			fprintf(stdout, "[%02x ]", MessageBuffer[count]);
		}
		line_count ++;

		if (line_count >= 8) {
			line_count = 0;
			fprintf(stdout, "\n   ");
		}
	}

	fprintf(stdout, "\n");
	fflush(stdout);

}

	/* FB38_TX_UpdateSequenceNumber
	   Any command we originate must have a unique SequenceNumber.
	   Observation to date suggests that these values startx at 0x10
	   and cycle up to 0x17 before repeating again.  Perhaps more
	   accurately, the numbers cycle 0,1,2,3..7 with bit 4 of the byte
	   premanently set. */
void	FB38_TX_UpdateSequenceNumber(void)
{
	RequestSequenceNumber ++;

	if (RequestSequenceNumber > 0x17 || RequestSequenceNumber < 0x10) {
		RequestSequenceNumber = 0x10;
	}
}

	/* Sends the "standard" acknowledge message back to the phone in
	   response to a message it sent automatically or in response to
	   a command sent to it.  The ack. algorithm isn't 100% understood
	   at this time. */
int		FB38_TX_SendStandardAcknowledge(u8 message_type)
{
		/* Standard acknowledge seems to be to return an empty message
		   with the sequence number set to equal the sequence number
		   sent minus 0x08. */
	return(FB38_TX_SendMessage(0, message_type, (MessageBuffer[1] & 0x1f) - 0x08, NULL));

}

	/* 0x4a messages appear to be a keepalive message and cause the phone
	   to send an 0x4a acknowledge and then an 0x4b message which has
	   status information in it. */
void 	FB38_TX_Send0x4aMessage(void) 
{
	FB38_TX_UpdateSequenceNumber();
	
	if (FB38_TX_SendMessage(0, 0x4a, RequestSequenceNumber, NULL) != true) {
		fprintf(stderr, _("0x4a Write failed!"));	
	}
}

	/* 0x3f messages request SMS information, returned as an 0x41 message. */
void 	FB38_TX_Send0x3fMessage(void) 
{
	FB38_TX_UpdateSequenceNumber();
	
	if (FB38_TX_SendMessage(0, 0x3f, RequestSequenceNumber, NULL) != true) {
		fprintf(stderr, _("0x3f Write failed!"));	
	}
}

	/* 0x42 messages are sent to write a phonebook memory location.
	   This function is designed to be called by the FB38_WriteMemoryLocation
	   routine above which performs length checking etc. */
int		FB38_TX_Send0x42_WriteMemoryLocation(u8 memory_area, u8 location, char *label, char *number)
{
	u8		message[256];
	int		label_length;
	int		number_length;
	int		message_length;

	label_length = strlen (label);
	number_length = strlen (number);

	message[0] = memory_area;
	message[1] = location;

		/* Now add the label/name details to message, starting with length. */
	message[2] = label_length;
	memcpy(message + 3, label, label_length);

		/* and similarly, the number, noting the munging to position the
		   data in the right part of the message (offset by the label info) */
	message[3 + label_length] = number_length;
	memcpy(message + 4 + label_length, number, number_length);


		/* Calculate message length. */
	message_length = 4 + label_length + number_length;
	
		/* Update sequence number and send to phone. */
	FB38_TX_UpdateSequenceNumber();
	if (FB38_TX_SendMessage(message_length, 0x42, RequestSequenceNumber, message) != true) {
		fprintf(stderr, _("Set Mem Loc Write failed!"));	
		return (-1);
	}

	return (0);
}

	/* 0x43 messages appears to request the contents of a memory location.
	   The phone acknowledges the 0x43 message then returns an
	   0x46 message with the actual memory data.  As the 3810 only
	   has SIM memory, the memory_area value is untested for values 
	   other than 0x02.  Presumably the 8110 uses a different value to 
	   specify internal memory. */
void 	FB38_TX_Send0x43_RequestMemoryLocation(u8 memory_area, u8 location) 
{
	u8		message[2];

	FB38_TX_UpdateSequenceNumber();

		/* Build and send message */
	message[0] = memory_area;
	message[1] = location;

	if (FB38_TX_SendMessage(2, 0x43, RequestSequenceNumber, message) != true) {
		fprintf(stderr, _("Request Mem Loc Write failed!"));	
	}

}

	/* 0x4c Messages are sent to request IMEI, Revision and Model 
	   information. */

void    FB38_TX_Send0x4c_RequestIMEIRevisionModelData(void)
{
	FB38_TX_UpdateSequenceNumber();

	if (FB38_TX_SendMessage(0, 0x4c, RequestSequenceNumber, NULL) != true) {
		fprintf(stderr, _("Request IMEI/Revision/Model Write failed!"));	
	}
}


void	FB38_TX_Send0x23_SendSMSHeader(char *message_centre, char *destination, u8 total_length)
{
	
	u8		message[255];
	u8		message_centre_length, destination_length;

		/* Update sequence number. */
	FB38_TX_UpdateSequenceNumber();

		/* Get length of message centre and destination numbers. */
	message_centre_length = strlen (message_centre);
	destination_length = strlen (destination);

		/* Build and send message.  No idea what first 10 bytes are at this
		   this stage so just duplicate what has been observed. */

	message[0] = 0x11;
	message[1] = 0x00;
	message[2] = 0x00;
	message[3] = 0xff;
	message[4] = 0x00;
	message[5] = 0x00;
	message[6] = 0x00;
	message[7] = 0x00;
	message[8] = 0x00;
	message[9] = 0x00;

		/* Add total length and message_centre number length fields. */
	message[10] = total_length;
	message[11] = message_centre_length;

		/* Copy in the actual message centre number. */
	memcpy (message + 12, message_centre, message_centre_length);

		/* Now add destination length and number. */
	message[12 + message_centre_length] = destination_length;
	memcpy (message + 13 + message_centre_length, destination, destination_length);

	if (FB38_TX_SendMessage(13 + message_centre_length + destination_length, 0x23, RequestSequenceNumber, message) != true) {
		fprintf(stderr, _("Send SMS header failed!"));	
	}


}


void	FB38_TX_Send0x27_SendSMSMessageText(u8 block_number, u8 block_length, char *text)
{
	u8		message[255];

		/* Update sequence number. */
	FB38_TX_UpdateSequenceNumber();


		/* If block length is over 55 (observed maximum) limit it. */
	if (block_length > 55) {
		block_length = 55;
	}

		/* Build and send message. */
	message[0] = block_number;

		/* Copy in the text. */
	memcpy (message + 1, text, block_length);

	if (FB38_TX_SendMessage(1 + block_length, 0x27, RequestSequenceNumber, message) != true) {
		fprintf(stderr, _("Send SMS block %d failed!"), block_number);	
	}


}

	/* 0x25 messages requests the contents of an SMS message
	   from the phone.  The first byte has only ever been 
	   observed to be 0x02 - could be selecting internal versus
	   external memory.  Specifying memory 0x00 may request the
	   first location?  Phone replies with 0x2c and 0x27 messages
	   for valid locations, 0x2d for empty ones. */
void 	FB38_TX_Send0x25_RequestSMSMemoryLocation(u8 memory_type, u8 location) 
{
	u8		message[2];

	FB38_TX_UpdateSequenceNumber();

		/* Build and send message */
	message[0] = memory_type;
	message[1] = location;

	if (FB38_TX_SendMessage(2, 0x25, RequestSequenceNumber, message) != true) {
		fprintf(stderr, _("Request SMS Mem Loc Write failed!"));	
	}

}

	/* 0x26 messages deletes an SMS message from the phone. 
	   The first byte has only ever been observed to be 0x02
	   but is assumed to be selecting internal versus
	   external memory.  Phone replies with 0x2e for valid locations,
	   0x2f for invalid ones.  If a location is empty but otherwise
	   valid 0x2e is still returned. */

void 	FB38_TX_Send0x26_DeleteSMSMemoryLocation(u8 memory_type, u8 location) 
{
	u8		message[2];

	FB38_TX_UpdateSequenceNumber();

		/* Build and send message */
	message[0] = memory_type;
	message[1] = location;

	if (FB38_TX_SendMessage(2, 0x26, RequestSequenceNumber, message) != true) {
		fprintf(stderr, _("Delete SMS Mem Loc write failed!"));	
	}

}


	/* 0x15 messages are sent by the PC during the initialisation phase
	   after sending lots of 0x55 characters [side note this may be some 
	   sort of autobaud sequence given the bit pattern generated
	   by sending 0x55].  Anyway, the contents of the message are not understood
	   so we simply send the same sequence observed between the W95 PC 
	   and the phone.  The init sequence may still be a bit flaky and is not
	   fully understood. */

void	FB38_TX_Send0x15Message(u8 sequence_number)
{
	u8		message[20] = {0x02, 0x01, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x55, 0x63, 0xa8, 0x00, 0x00, 0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};

	if (FB38_TX_SendMessage(20, 0x15, sequence_number, message) != true) {
		fprintf(stderr, _("0x15 Write failed!"));	
	}
}

	/* Prepares the message header and sends it, prepends the
	   message start byte (0x01) and other values according
	   the value specified when called.  Calculates checksum
	   and then sends the lot down the pipe... */
int		FB38_TX_SendMessage(u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer) 
{
	u8			out_buffer[FB38_MAX_TRANSMIT_LENGTH + 5];
	int			count;
	unsigned char			checksum;

		/* Check message isn't too long, once the necessary
		   header and trailer bytes are included. */
	if ((message_length + 5) > FB38_MAX_TRANSMIT_LENGTH) {

		return (false);
	}
		/* Now construct the message header. */
	out_buffer[0] = 0x01;	/* Start of message indicator */
	out_buffer[1] = message_length + 2;	/* Our message length refers to buffer, 
										   the value in the protcol includes 
										   type and sequence info. */
	out_buffer[2] = message_type;
	out_buffer[3] = sequence_byte;

		/* Copy in data if any. */	
	if (message_length != 0) {
		memcpy(out_buffer + 4, buffer, message_length);
	}
		/* Now calculate checksum over entire message 
		   and append to message. */
	checksum = 0;
	for (count = 0; count < message_length + 4; count ++) {
		checksum ^= out_buffer[count];
	}
	out_buffer[message_length + 4] = checksum;

		/* Send it out... */
	if (write(PortFD, out_buffer, message_length + 5) != message_length + 5) {
		return (false);
	}
	return (true);
}

void	FB38_RX_Handle0x0b_IncomingCall(void)
{
	int		count;
	char	buffer[256];

		/* First, acknowledge message. */
	if (FB38_TX_SendMessage(0, 0x0b, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("Write failed!"));
	}

		/* Get info out of message.  At present, first three bytes are unknown
		   (though third seems to correspond to length of number).  Remaining 
		   bytes are the phone number, ASCII encoded. */

	for (count = 0; count < MessageBuffer[4]; count ++) {
		buffer[count] = MessageBuffer[5 + count];
	}
	buffer[count] = 0x00;

	if (EnableMonitoringOutput == false) {
		return;
	}

		/* Now display incoming call message. */
	fprintf(stdout, _("Incoming call - status %02x %02x %02x, Number %s.\n"), MessageBuffer[2], MessageBuffer[3], MessageBuffer[4], buffer);

	fflush (stdout);
}

	/* 0x27 messages are a little unusual when sent by the phone in that
	   they can either be an acknowledgement of an 0x27 message we sent
	   to the phone with message text in it or they could
	   contain message text for a message we requested. */
void	FB38_RX_Handle0x27_SMSMessageText(void)
{
	static int 	length_received;
	int			count;
	
		/* First see if it was an acknowledge to one of our messages. 
		   if so set the flag so SMS send code knows it can send next
		   block. */
	if (MessageLength == 0x02) {
		SMSBlockAckReceived = true;
		return;
	}

		/* It wasn't so acknowledge it. */
	if (FB38_TX_SendStandardAcknowledge(0x27) != true) {
		fprintf(stderr, _("0x27 Write failed!"));	
	}

		/* If this is the first block, reset remaining_length. */
	if (MessageBuffer[2] == 1) {
		length_received = 0;
	}

		/* Copy into current SMS message as long as it's non-NULL */
	if (CurrentSMSMessage != NULL) {
		for (count = 0; count < MessageLength - 3; count ++) {
			if ((length_received) < FB38_MAX_SMS_LENGTH) {
				CurrentSMSMessage->MessageText[length_received] = MessageBuffer[count + 3];
			}	
			length_received ++;
		}
	}
	
	if (length_received == CurrentSMSMessageBodyLength) {
		CurrentSMSMessage->MessageText[length_received] = 0;
			/* Signal that the response is complete. */
		CurrentSMSMessageError = GE_NONE;
	}

}




	/* 0x4b is a general status message. */
void	FB38_RX_Handle0x4b_Status(void)
{
		/* Map from values returned in status packet to the
		   the values returned by the AT+CSQ command */
	float	csq_map[5] = {0, 8, 16, 24, 31};

		/* First, send acknowledge. */
	if (FB38_TX_SendStandardAcknowledge(0x4b) != true) {
		fprintf(stderr, _("0x4b Write failed!"));	
	}

		/* There are three data bytes in the status message, two have been
		   attributed to signal level, the third is presently unknown. 
		   Unknown byte has been observed to be 0x01 when connected to normal
		   network, 0x04 when no network available.   Steps through 0x02, 0x03
		   when incoming or outgoing calls occur...*/	
	FB38_LinkOK = true;

	if (MessageBuffer[3] <= 4) {
		CurrentRFLevel = csq_map[MessageBuffer[3]];
	}
	else {
		CurrentRFLevel = 99;
	}

	CurrentBatteryLevel = MessageBuffer[4];

	if (EnableMonitoringOutput == false) {
		return;
	}

		/* Only output connection status byte now as the RF and Battery
	 	   levels are displayed by the main gnokii code. */
	fprintf(stdout, _("Status: Connection Status %02x.\n"), MessageBuffer[2]);

}


void	FB38_RX_Handle0x10_EndOfOutgoingCall(void)
{
		/* As usual, acknowledge first. */
	if (FB38_TX_SendMessage(0, 0x10, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("0x10 Write failed!"));
	}

	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("Outgoing call terminated (0x10 message).\n"));
	fflush(stdout);

}

void	FB38_RX_Handle0x11_EndOfIncomingCall(void)
{
		/* As usual, acknowledge first. */
	if (FB38_TX_SendMessage(0, 0x11, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("Write failed!"));
	}

	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("Incoming call terminated.\n"));
	fflush(stdout);

}

void	FB38_RX_Handle0x12_EndOfOutgoingCall(void)
{
		/* As usual, acknowledge first. */
	if (FB38_TX_SendMessage(0, 0x12, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("Write failed!"));
	}

	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("Outgoing call terminated (0x12 message).\n"));
	fflush(stdout);

}

void	FB38_RX_Handle0x0d_IncomingCallAnswered(void)
{
		/* As usual, acknowledge first. */
	if (FB38_TX_SendMessage(0, 0x0d, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("Write failed!"));
	}

	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("Incoming call answered.\n"));
	fflush(stdout);

}

void	FB38_RX_Handle0x0e_OutgoingCallAnswered(void)
{
		/* As usual, acknowledge first. */
	if (FB38_TX_SendMessage(0, 0x0e, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("Write failed!"));
	}

	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("Outgoing call answered - status bytes %02x %02x %02x.\n"), MessageBuffer[2], MessageBuffer[3], MessageBuffer[4]);
	fflush(stdout);

}

	/* 0x2c message is sent in response to an 0x25 request.  Appears
	   to have the same fields as the 0x30 notification but with
	   one extra. */
void	FB38_RX_Handle0x2c_SMSHeader(void)
{
	u8		sender_length;
	u8		message_centre_length;

		/* Acknowlege. */
	if (FB38_TX_SendStandardAcknowledge(0x2c) != true) {
		fprintf(stderr, _("0x2c Write failed!"));
	}

		/* Set CurrentSMSMessageBodyLength for use by 0x27 code. */
	CurrentSMSMessageBodyLength = MessageBuffer[15];

		/* If CurrentSMSMessage is null don't bother decoding rest of
		   message. */
	if (CurrentSMSMessage == NULL) {
		return;
	}

		/* Extract data from message into CurrentSMSMessage. */
	CurrentSMSMessage->MemoryType = MessageBuffer[2];
	CurrentSMSMessage->MessageNumber = MessageBuffer[3];
	CurrentSMSMessage->Length = MessageBuffer[15];

		/* Extract date and time information which is packed in to 
		   nibbles of each byte in reverse order.  Thus day 28 would be
		   encoded as 0x82 */

	CurrentSMSMessage->Year = (MessageBuffer[8] >> 4) + (10 * (MessageBuffer[8] & 0x0f)); 
	CurrentSMSMessage->Month = (MessageBuffer[9] >> 4) + (10 * (MessageBuffer[9] & 0x0f)); 
	CurrentSMSMessage->Day = (MessageBuffer[10] >> 4) + (10 * (MessageBuffer[10] & 0x0f)); 
	CurrentSMSMessage->Hour = (MessageBuffer[11] >> 4) + (10 * (MessageBuffer[11] & 0x0f)); 
	CurrentSMSMessage->Minute = (MessageBuffer[12] >> 4) + (10 * (MessageBuffer[12] & 0x0f)); 
	CurrentSMSMessage->Second = (MessageBuffer[13] >> 4) + (10 * (MessageBuffer[13] & 0x0f)); 

		/* Now get sender and message centre information. */
	message_centre_length = MessageBuffer[16];
	sender_length = MessageBuffer[16 +  message_centre_length + 1];

		/* Check they're not too long. */
	if (sender_length > FB38_MAX_SENDER_LENGTH) {
		sender_length = FB38_MAX_SENDER_LENGTH;
	}
		
	if (message_centre_length > FB38_MAX_SMS_CENTRE_LENGTH) {
		message_centre_length = FB38_MAX_SMS_CENTRE_LENGTH;
	}

		/* Now copy to strings... Note they are in reverse order to
		   0x30 message*/
	memcpy(CurrentSMSMessage->MessageCentre, MessageBuffer + 17, message_centre_length);
	CurrentSMSMessage->MessageCentre[message_centre_length] = 0;	/* Ensure null terminated. */
	
	strncpy(CurrentSMSMessage->Sender, MessageBuffer + 18 + message_centre_length, sender_length);
	CurrentSMSMessage->Sender[sender_length] = 0;

}

void	FB38_RX_Handle0x30_IncomingSMSNotification(void)
{
	int		year, month, day;		/* Date of arrival */
	int		hour, minute, second;	/* Time of arrival */
	int		msg_number;				/* Message number in phone's memory */

	char	sender[255];			/* Sender details */
	u8		sender_length;

	char	message_centre[255];	/* And message centre number/ID */
	u8		message_centre_length;

	u8		message_body_length;	/* Length of actual SMS message itself. */

	u8		unk0, unk2, unk3, unk4;	/* Unknown bytes at start of message */
	u8		unk9;

	u8		unk_end;				/* Unknown byte at end of message */

	if (FB38_TX_SendStandardAcknowledge(0x30) != true) {
		fprintf(stderr, _("Write failed!"));
	}

		/* Extract data from message. */
	unk0 = MessageBuffer[2];
	msg_number = MessageBuffer[3];
	unk2 = MessageBuffer[4];
	unk3 = MessageBuffer[5];
	unk4 = MessageBuffer[6];
	unk9 = MessageBuffer[13];
	message_body_length = MessageBuffer[14];

		/* Extract date and time information which is packed in to 
		   nibbles of each byte in reverse order.  Thus day 28 would be
		   encoded as 0x82 */

	year = (MessageBuffer[7] >> 4) + (10 * (MessageBuffer[7] & 0x0f)); 
	month = (MessageBuffer[8] >> 4) + (10 * (MessageBuffer[8] & 0x0f)); 
	day = (MessageBuffer[9] >> 4) + (10 * (MessageBuffer[9] & 0x0f)); 
	hour = (MessageBuffer[10] >> 4) + (10 * (MessageBuffer[10] & 0x0f)); 
	minute = (MessageBuffer[11] >> 4) + (10 * (MessageBuffer[11] & 0x0f)); 
	second = (MessageBuffer[12] >> 4) + (10 * (MessageBuffer[12] & 0x0f)); 

		/* Now get sender and message centre information. */
	sender_length = MessageBuffer[15];
	message_centre_length = MessageBuffer[15 +  sender_length + 1];

		/* Now copy to strings... */
	strncpy(sender, MessageBuffer + 16, sender_length);
	sender[sender_length] = 0;	/* Ensure null terminated. */
	
	strncpy(message_centre, MessageBuffer + 17 + sender_length, message_centre_length);
	message_centre[message_centre_length] = 0;

		/* Get last byte, purpose unknown. */
	unk_end = MessageBuffer[17 + sender_length + message_centre_length];

		/* And output. */
	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("Incoming SMS %d/%d/%d %d:%02d:%02d Sender: %s Msg Centre: %s\n"),
			year, month, day, hour, minute, second, sender, message_centre);
	fprintf(stdout, _("   Msg Length %d, Msg number %d,  Unknown bytes: %02x %02x %02x %02x %02x %02x\n"), 
			message_body_length, msg_number, unk0, unk2, unk3, unk4, unk9, unk_end);
	fflush(stdout);
}




	/* Handle data contained in an 0x46 message which is sent by the phone
	   in reponse to a 0x43 message requesting memory contents. */
void	FB38_RX_Handle0x46_MemoryLocationData(void)
{
	u8		label_length;	/* Stored in first data byte in message. */
	u8		number_length;	/* Stored immediately after label data. */

		/* As usual, acknowledge first. */
	if (FB38_TX_SendMessage(0, 0x46, MessageBuffer[1] - 0x08, NULL) != true) {
		fprintf(stderr, _("Write failed!"));
	}

		/* Get/Calculate label and number length. */
	label_length = MessageBuffer[2];
	number_length = MessageBuffer[label_length + 3];

		/* Providing it's not a NULL pointer, copy entry into CurrentPhonebookEntry */
	if (CurrentPhonebookEntry == NULL) {
			/* Tell calling code an error occured. */
		CurrentPhonebookError = GE_INTERNALERROR;
		return;
	}

	CurrentPhonebookEntry->Empty = true;

	if (label_length == 0) {
		CurrentPhonebookEntry->Name[0] = 0x00;
	}
	else {
		if (label_length >= FB38_MAX_PHONEBOOK_NAME_LENGTH) {
			label_length = FB38_MAX_PHONEBOOK_NAME_LENGTH;
		}
		
		memcpy(CurrentPhonebookEntry->Name, MessageBuffer + 3, label_length);
		CurrentPhonebookEntry->Name[label_length] = 0x00;
		CurrentPhonebookEntry->Empty = false;
	}

	if (number_length == 0) {
		CurrentPhonebookEntry->Number[0] = 0x00;
	}
	else {
		if (number_length >= FB38_MAX_PHONEBOOK_NUMBER_LENGTH) {
			number_length = FB38_MAX_PHONEBOOK_NUMBER_LENGTH;
		}
		
		memcpy(CurrentPhonebookEntry->Number, MessageBuffer + label_length + 4, number_length);
		CurrentPhonebookEntry->Number[number_length] = 0x00;
		CurrentPhonebookEntry->Empty = false;
	}

	CurrentPhonebookEntry->Group=GSM_GROUPS_NOT_SUPPORTED;

		/* Signal no error to calling code. */
	CurrentPhonebookError = GE_NONE;

}

	/* Handle 0x4d message which is sent by phone in response to 
	   0x4c request.  Provides IMEI, Revision and Model information. */
void	FB38_RX_Handle0x4d_IMEIRevisionModelData(void)
{
	int		imei_length;
	int		rev_length;

		/* As usual, acknowledge first. */
	if (!FB38_TX_SendStandardAcknowledge(0x41)) {
		fprintf(stderr, _("Write failed!"));
	}

	imei_length = strlen(MessageBuffer + 2);
	rev_length = strlen(MessageBuffer + 3 + imei_length);

	strncpy(IMEI, MessageBuffer + 2, FB38_MAX_IMEI_LENGTH);
	IMEIValid = true;

	strncpy(Revision, MessageBuffer + 3 + imei_length, FB38_MAX_REVISION_LENGTH);
	RevisionValid = true;

	strncpy(Model, MessageBuffer + 4 + imei_length + rev_length, FB38_MAX_MODEL_LENGTH);
	ModelValid = true;

}


	/* Handle 0x41 message which is sent by phone in response to an
	   0x3f request.  Contains data about the Message Centre in use,
	   the only bit understood at this time is the phone number portion
	   at the end of the message.  This number appears to be null terminated
	   unlike others but we don't rely on it at this stage! */
void	FB38_RX_Handle0x41_SMSMessageCentreData(void)
{
	u8		centre_number_length;
	int		count;
	
		/* As usual, acknowledge first. */
	if (!FB38_TX_SendStandardAcknowledge(0x41)) {
		fprintf(stderr, _("Write failed!"));
	}

		/* Get Message Centre number length, which is byte 13 in message. */
	centre_number_length = MessageBuffer[13];

		/* First 12 bytes are status values, purpose unknown.  For now
		   simply display for user to mull over... */
	if (EnableMonitoringOutput == false) {
		return;
	}

	fprintf(stdout, _("SMS Message Centre Data status bytes ="));
	for (count = 0; count < 12; count ++) {
		fprintf(stdout, "0x%02x ", MessageBuffer[2 + count]);
	}

	if (centre_number_length == 0) {
		fprintf(stdout, _("Number field emtpy."));
	}
	else {
		fprintf(stdout, _("Number: "));
		for (count = 0; count < centre_number_length; count ++) {
			fprintf(stdout, "%c", MessageBuffer[14 + count]);
		}
	}

	fprintf(stdout, "\n");
	fflush(stdout);

}
