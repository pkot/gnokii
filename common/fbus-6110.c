/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml. & Hugh Blemings.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions on the 6110 and similar
  phones. See README for more details on supported mobile phones.

  The various routines are called FB61 (whatever) as a concatenation of FBUS
  and 6110.

  Last modification: Mon Apr 26 22:54:51 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

/* "Turn on" prototypes in fbus-6110.h */
#define __fbus_6110_c 

/* System header files */
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <libintl.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/* Various header file */
#include "misc.h"
#include "gsm-common.h"
#include "fbus-6110.h"
#include "gsm-networks.h"

/* Global variables used by code in gsm-api.c to expose the functions
   supported by this model of phone. */

bool FB61_LinkOK;

GSM_Error FB61_DialVoice(char *Number);

GSM_Functions FB61_Functions = {
  FB61_Initialise,
  FB61_Terminate,
  FB61_GetPhonebookLocation,
  FB61_WritePhonebookLocation,
  FB61_GetSMSMessage,
  FB61_DeleteSMSMessage,
  FB61_SendSMSMessage,
  FB61_GetRFLevel,
  FB61_GetBatteryLevel,
  FB61_EnterPin,
  FB61_GetIMEIAndCode,
  FB61_GetDateTime,
  FB61_SetDateTime,
  FB61_GetAlarm,
  FB61_SetAlarm,
  FB61_DialVoice
};

/* Mobile phone information */
GSM_Information FB61_Information = {
  "6110|6150|6190|5110", /* Supported models */
  4, /* Max RF Level */
  0, /* Min RF Level */
  GRF_Arbitrary, /* RF level units */
  4, /* Max Battery Level */
  0, /* Min Battery Level */
  GBU_Arbitrary, /* Battery level units */
  GDT_DateTime, /* Have date/time support */
  GDT_TimeOnly,	/* Alarm supports time only ? */
  1 /* Only one alarm available. */
};

	/* Local variables */
int PortFD;
int BufferCount;
u8 MessageBuffer[FB61_MAX_RECEIVE_LENGTH];
unsigned char MessageLength, MessageType, MessageDestination, MessageSource, MessageUnknown;
char PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
u8 RequestSequenceNumber=0x00;
pthread_t Thread;
bool RequestTerminate;
bool DisableKeepalive=false;
struct termios OldTermios; /* To restore termio on close. */
bool EnableMonitoringOutput;

/* Local variables used by get/set phonebook entry code.  ...Buffer is used as
   a source or destination for phonebook data, ...Error is set to GE_None by
   calling function, set to GE_COMPLETE or an error code by handler routines
   as appropriate. */
		   	   	   
GSM_PhonebookEntry *CurrentPhonebookEntry;
GSM_Error          CurrentPhonebookError;

GSM_SMSMessage     *CurrentSMSMessage;
GSM_Error          CurrentSMSMessageError;
int                CurrentSMSPointer;

GSM_Error          PINError;

GSM_DateTime       *CurrentDateTime;
GSM_Error          CurrentDateTimeError;

GSM_DateTime       *CurrentAlarm;
GSM_Error          CurrentAlarmError;

#define FB61_FRAME_HEADER 0x00, 0x01, 0x00

/* Initialise variables and state machine. */

GSM_Error FB61_Initialise(char *port_device, bool enable_monitoring)
{

  int rtn;

  RequestTerminate = false;
  EnableMonitoringOutput = enable_monitoring;

  strncpy(PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

  /* Create and start main thread. */

  rtn = pthread_create(&Thread, NULL, (void *)FB61_ThreadLoop, (void *)NULL);

  if (rtn != 0)
    return (GE_INTERNALERROR);

  return (GE_NONE);
}

/* This function send the status request to the phone. */

GSM_Error FB61_TX_SendStatusRequest()
{

  /* The status request is of the type 0x04. It's subtype is 0x01. If you have
     another subtypes and it's meaning - just inform Pavel, please. */

  unsigned char request[] = {FB61_FRAME_HEADER, 0x01, 0x01};

  FB61_TX_SendMessage(0x05, 0x04, request);

  return (GE_NONE);
}

GSM_Error	FB61_GetStorageStatus(GSM_MemoryType memory_type)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x07, 0x00, 0x01};

  req[4]= (memory_type==GMT_INTERNAL)?FB61_MEMORY_PHONE:FB61_MEMORY_SIM;

  FB61_TX_SendMessage(0x06, 0x03, req);

  return (GE_NONE);
}

GSM_Error	FB61_GetCalendarNote(int location)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x66, 0x00, 0x01};

  req[4]=location;

  FB61_TX_SendMessage(0x06, 0x13, req);

  return (GE_NONE);
}

/* This is the main loop for the FB61 functions. When FB61_Initialise is
   called a thread is created to run this loop. This loop is exited when the
   application calls the FB61_Terminate function. */

void	FB61_ThreadLoop(void)
{

  unsigned char init_char[] = {0x55};
  unsigned char connect1[] = {FB61_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02, 0x01};
  unsigned char connect2[] = {FB61_FRAME_HEADER, 0x20, 0x02, 0x01};
  unsigned char connect3[] = {FB61_FRAME_HEADER, 0x0d, 0x01, 0x00, 0x02, 0x01};
  unsigned char connect4[] = {FB61_FRAME_HEADER, 0x10, 0x01};
  int count, idle_timer;

  CurrentPhonebookEntry = NULL;  

		/* Try to open serial port, if we fail we sit here and don't proceed
		   to the main loop. */
	if (FB61_OpenSerial() != true) {
		FB61_LinkOK = false;

			/* Fail so sit here till calling code works out there is a
			   problem. */		
		while (!RequestTerminate) {
			usleep (100000);
		}
		return;
	}
	
		/* Initialise link with phone or what have you */
		/* Send init string to phone, this is a bunch of 0x55
		   characters.  Timing is empirical. */
	for (count = 0; count < 250; count ++) {
		usleep(100);
		write(PortFD, init_char, 1);
	}

	FB61_TX_SendStatusRequest();

	usleep(100);

	FB61_TX_SendMessage(8, 0x02, connect1);

	usleep(100);

	FB61_TX_SendMessage(6, 0x02, connect2);

	usleep(100);

	FB61_TX_SendMessage(8, 0x02, connect3);

	usleep(100);

	FB61_TX_SendMessage(5, 0x64, connect4);

	usleep(100);

	/* This function sends the Alarm request to the phone.  Values
	   passed are ignored at present */

	// FB61_GetAlarm(0, &date_time);

	/* Get the primary SMS Center */

	/* It's very strange that if I send this request again (with different
       number) it fails. But if I send it alone it succeds. It seems that 6110
       is refusing to tell you all the SMS Centers information at once :-( */

	//	FB61_GetSMSCenter(1);

	// 	FB61_GetCalendarNote(1);

	//	FB61_SetDateTime(NULL);

	idle_timer=0;

	/* Now enter main loop */
	while (!RequestTerminate) {

		  if (idle_timer == 0) {
			/* Dont send keepalive and status packets when doing other transactions. */
			if (!DisableKeepalive) {
			  FB61_TX_SendStatusRequest();
			  FB61_GetSMSStatus();
			  FB61_GetStorageStatus(GMT_INTERNAL);
			  FB61_GetStorageStatus(GMT_SIM);
			}

			idle_timer = 20;
		  }
		  else
			idle_timer--;

		  usleep(100000);		/* Avoid becoming a "busy" loop. */
	 	}
}

	/* Applications should call FB61_Terminate to shut down
	   the FB61 thread and close the serial port. */
void		FB61_Terminate(void)
{
		/* Request termination of thread */
	RequestTerminate = true;

		/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

		/* Close serial port. */
	tcsetattr(PortFD, TCSANOW, &OldTermios);
	
	close (PortFD);
}

	/* These are the actual functions used by the higher level code, 
	   accessed by the GSM_Functions structure.  For now these are
	   all return GE_NOTIMPLEMENTED! */
GSM_Error	FB61_GetRFLevel(float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_GetBatteryLevel(float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error FB61_DialVoice(char *Number) {

  unsigned char req[64]={FB61_FRAME_HEADER, 0x01};
  unsigned char req_end[]={0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01, 0x01};
  int i=0;

  req[4]=strlen(Number);

  for(i=0; i < strlen(Number) ; i++)
   req[5+i]=Number[i];

  memcpy(req+5+strlen(Number), req_end, 10);

  FB61_TX_SendMessage(14+strlen(Number), 0x01, req);

  return(GE_NONE);
}

GSM_Error FB61_EnterPin(char *pin)
{

  unsigned char pin_req[15] = {FB61_FRAME_HEADER, 0x0a, 0x02};
  int i=0, timeout=20;

  PINError=GE_BUSY;

  for (i=0; i<strlen(pin);i++)
    pin_req[5+i]=pin[i];

  pin_req[5+strlen(pin)]=0x01;

  FB61_TX_SendMessage(6+strlen(pin), 0x08, pin_req);

  DisableKeepalive=true;

  /* Wait for timeout or other error. */
  while (timeout != 0 && PINError == GE_BUSY) {

    if (timeout-- == 0) {
      DisableKeepalive=false;
      return (GE_TIMEOUT);
    }    

    usleep (100000);
  }

  DisableKeepalive=false;

  return (PINError);
}

GSM_Error	FB61_GetDateTime(GSM_DateTime *date_time)
{
  unsigned char clock_req[] = {FB61_FRAME_HEADER, 0x62, 0x01};
  int timeout=5;

  CurrentDateTime=date_time;
  CurrentDateTimeError=GE_BUSY;

  FB61_TX_SendMessage(0x05, 0x11, clock_req);

  DisableKeepalive=true;

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentDateTimeError == GE_BUSY) {

    if (timeout-- == 0) {
      DisableKeepalive=false;
      return (GE_TIMEOUT);
    }    

    usleep (100000);
  }

  DisableKeepalive=false;

  return (CurrentDateTimeError);
}

GSM_Error	FB61_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
  unsigned char alarm_req[] = {FB61_FRAME_HEADER, 0x6d, 0x01};
  int timeout=5;

  CurrentAlarm=date_time;
  CurrentAlarmError=GE_BUSY;

  FB61_TX_SendMessage(0x05, 0x11, alarm_req);

  DisableKeepalive=true;

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentAlarmError == GE_BUSY) {

    if (timeout-- == 0) {
      DisableKeepalive=false;
      return (GE_TIMEOUT);
    }    

    usleep (100000);
  }

  DisableKeepalive=false;

  return (CurrentAlarmError);
}

	/* This function sends to the mobile phone a request for the SMS Center
	   of rank `priority' */

GSM_Error	FB61_GetSMSCenter(u8 priority)
{
	unsigned char smsc_req[] = {FB61_FRAME_HEADER, 0x33, 0x64, 0x01, 0x01};

	smsc_req[5]=priority;

	FB61_TX_SendMessage(0x07, 0x02, smsc_req);

	return (GE_NONE);
}

GSM_Error	FB61_GetSMSStatus()
{
	unsigned char smsstatus_req[] = {FB61_FRAME_HEADER, 0x36, 0x64, 0x01};

	FB61_TX_SendMessage(0x06, 0x14, smsstatus_req);

	return (GE_NONE);
}

GSM_Error	FB61_GetIMEIAndCode(char *imei, char *code)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_SetDateTime(GSM_DateTime *date_time)
{
	unsigned char req[] = {FB61_FRAME_HEADER,
			       0x60, /* set-time subtype */
			       0x01, 0x01, 0x07, /* unknown */
			       0x07, 0xcf, /* Year (0x07cf = 1999) */
			       0x04, 0x16, /* Month Day */
			       0x17, 0x15, /* Hours Minutes */
			       0x00, /* Unknown, but not seconds - try 59 and wait 1 sec. */
			       0x01 };

	FB61_TX_SendMessage(0x0f, 0x11, req);
	return (GE_NONE);
}

GSM_Error	FB61_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	FB61_GetPhonebookLocation(GSM_MemoryType memory_type, int location, GSM_PhonebookEntry *entry) {

  unsigned char req[] = {FB61_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00, 0x01};
  int memory_area;
  int timeout;

		/* State machine code writes data to these variables when
		   it comes in. */
	CurrentPhonebookEntry = entry;
	CurrentPhonebookError = GE_BUSY;

	if (memory_type == GMT_INTERNAL) {
		memory_area = FB61_MEMORY_PHONE;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = FB61_MEMORY_SIM;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

	req[4] = memory_area;
	req[5] = location;

	timeout = 20; 	/* 2 seconds for command to complete */

		/* Return if no link has been established. */
	if (!FB61_LinkOK) {
	  printf("returning no link :-(\n");
	  return GE_NOLINK;
	}

		/* Send request */
	FB61_TX_SendMessage(0x08, 0x03, req);

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
GSM_Error	FB61_WritePhonebookLocation(GSM_MemoryType memory_type, int location, GSM_PhonebookEntry *entry)
{

  unsigned char req[128] = {FB61_FRAME_HEADER, 0x04, 0x00, 0x00};
  int i=0, current=0, memory_area;

	if (memory_type == GMT_INTERNAL) {
		memory_area = FB61_MEMORY_PHONE;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = FB61_MEMORY_SIM;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

  req[4] = memory_area;
  req[5] = location;

  req[6] = strlen(entry->Name);

  current=7;

  for (i=0; i<strlen(entry->Name); i++)
    req[current+i] = entry->Name[i];

  current+=strlen(entry->Name);

  req[current++]=strlen(entry->Number);

  for (i=0; i<strlen(entry->Number); i++)
    req[current+i] = entry->Number[i];

  current+=strlen(entry->Number);

  req[current++]=0xff;
  req[current++]=0x01;

  FB61_TX_SendMessage(current, 3, req);

  sleep(5);

  return (GE_NONE);
}

GSM_Error	FB61_GetSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message)
{

	unsigned char req[] = {FB61_FRAME_HEADER, 0x07, 0x00, 0x00, 0x01, 0x64, 0x01};
	int memory_area;
	int timeout;

		/* State machine code writes data to these variables when
		   it comes in. */
	CurrentSMSMessage = message;
	CurrentSMSMessageError = GE_BUSY;

	if (memory_type == GMT_INTERNAL) {
		memory_area = FB61_MEMORY_PHONE;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = FB61_MEMORY_SIM;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

	req[4] = memory_area;
	req[5] = location;

	timeout = 50; 	/* 5 seconds for command to complete */

		/* Return if no link has been established. */
	if (!FB61_LinkOK) {
	  printf("returning no link :-(\n");
	  return GE_NOLINK;
	}

		/* Send request */
	FB61_TX_SendMessage(0x09, 0x02, req);

		/* Wait for timeout or other error. */
	while (timeout != 0 && CurrentSMSMessageError != GE_NONE) {

		timeout --;
		if (timeout == 0) {
			return (GE_TIMEOUT);
		}
		usleep (100000);
	}

	return (CurrentSMSMessageError);
}

/* FIXME: This is not tested yet */
GSM_Error	FB61_DeleteSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message)
{

	unsigned char req[] = {FB61_FRAME_HEADER, 0x0a, 0x00, 0x00, 0x01};
	int memory_area;

	if (memory_type == GMT_INTERNAL) {
		memory_area = FB61_MEMORY_PHONE;
	}
	else {
		if (memory_type == GMT_SIM) {
			memory_area = FB61_MEMORY_SIM;
		}
		else {
			return (GE_INVALIDMEMORYTYPE);
		}
	}

	req[4] = memory_area;
	req[5] = location;

	FB61_TX_SendMessage(0x07, 0x14, req);

	return (GE_NONE);


}

GSM_Error	FB61_SendSMSMessage(char *message_centre, char *destination, char *text)
{
	return (GE_NOTIMPLEMENTED);
}

void FB61_DumpSerial()
{
	unsigned int Flags=0;

	ioctl(PortFD, TIOCMGET, &Flags);

	printf("Serial flags dump:\n");
	printf("DTR is %s.\n", Flags&TIOCM_DTR?"up":"down");
	printf("RTS is %s.\n", Flags&TIOCM_RTS?"up":"down");
	printf("CAR is %s.\n", Flags&TIOCM_CAR?"up":"down");
	printf("CTS is %s.\n", Flags&TIOCM_CTS?"up":"down");
	printf("\n");
}

/* This functions set up the Nokia DAU-9P MBus Cable NSE-3 which is probably
   dual MBUS/FBUS to FBUS communication. If we skip this step we can
   communicate with the phone, but the phone sends us some garbagge (0x18 0x00
   ...).

   This was brought to my attention by people with original cable. My cable is
   FBUS only so I borrow the original cable from my friend and solve
   it. Thanks to Cobus, Juan and people on the mailing list.

   Colin wrote:

   The data suite cable has some electronics built into the connector. This of
   course needs a power supply of some sorts to operate properly.

   In this case power is drawn off the handshaking lines of the PC. DTR has to
   be set and RTS have to be cleared, thus if you use a terminal program (that
   does not set the handshaking lines to these conditions) you will get weird
   results. It will not set them like this since if Request To Send (RTS) is
   not set the other party will not send any data (in hardware handshaking)
   and if DTS is not set (handshaking = none) the cable will not receive
   power. */

/* FIXME: In this function we use ioctls - can people with different OSes than
   my (Linux) tell me if it works for them? */

void FB61_SetFBUS()
{

  unsigned int Flags=0;

#ifdef DEBUG
  FB61_DumpSerial();  
  printf("Setting FBUS communication...\n");
#endif DEBUG

  /* clearing the RTS bit */
  Flags=TIOCM_RTS;
  ioctl(PortFD, TIOCMBIC, &Flags);

  /* setting the DTR bit */
  Flags=TIOCM_DTR;
  ioctl(PortFD, TIOCMBIS, &Flags);

#ifdef DEBUG
  FB61_DumpSerial();  
#endif DEBUG
}

/* Called by initialisation code to open comm port in asynchronous mode. */
bool		FB61_OpenSerial(void)
{
	struct termios			new_termios;
	struct sigaction		sig_io;

		/* Open device. */
	PortFD = open (PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (PortFD < 0) {
		perror(_("Couldn't open FB61 device: "));
		return (false);
	}

		/* Set up and install handler before enabling async IO on port. */
	sig_io.sa_handler = FB61_SigHandler;
	sig_io.sa_flags = 0;
	sig_io.sa_restorer = NULL;
	sigaction (SIGIO, &sig_io, NULL);

		/* Allow process/thread to receive SIGIO */
	fcntl(PortFD, F_SETOWN, getpid());

		/* Make filedescriptor asynchronous. */
	fcntl(PortFD, F_SETFL, FASYNC);

		/* Save current port attributes so they can be restored on exit. */
	tcgetattr(PortFD, &OldTermios);

		/* Set port settings for canonical input processing */
	new_termios.c_cflag = FB61_BAUDRATE | CS8 | CLOCAL | CREAD;
	new_termios.c_iflag = IGNPAR;
	new_termios.c_oflag = 0;
	new_termios.c_lflag = 0;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;

	tcflush(PortFD, TCIFLUSH);
	tcsetattr(PortFD, TCSANOW, &new_termios);

	FB61_SetFBUS(PortFD);

	return (true);
}

void	FB61_SigHandler(int status)
{
	unsigned char 	buffer[255];
	int				count,res;

	res = read(PortFD, buffer, 255);

	for (count = 0; count < res ; count ++) {
		FB61_RX_StateMachine(buffer[count]);
	}
}

char *FB61_GetBCDNumber(u8 *Number) {
  static char Buffer[20]="";
  /* This is the length of BCD coded number */
  int length=Number[0];
  int count;

  if (Number[1]==0x91)
	sprintf(Buffer, "+");
  else
        Buffer[0] = '\0';

  for (count=0; count <length-1; count++) {

    int Digit;

    Digit=Number[count+2]&0x0f;
    if (Digit<10)
      sprintf(Buffer, "%s%d", Buffer, Digit);
    
    Digit=Number[count+2]>>4;
    if (Digit<10)
      sprintf(Buffer, "%s%d", Buffer, Digit);
  }

  return Buffer;
}

char *FB61_GetPackedDateTime(u8 *Number) {
  static char Buffer[20]="";

  sprintf(Buffer, "%d%d-", Number[0]&0xf, Number[0]>>4);
  sprintf(Buffer, "%s%d%d-", Buffer, Number[1]&0xf, Number[1]>>4);
  sprintf(Buffer, "%s%d%d ", Buffer, Number[2]&0xf, Number[2]>>4);
  sprintf(Buffer, "%s%d%d:", Buffer, Number[3]&0xf, Number[3]>>4);
  sprintf(Buffer, "%s%d%d:", Buffer, Number[4]&0xf, Number[4]>>4);
  sprintf(Buffer, "%s%d%d",  Buffer, Number[5]&0xf, Number[5]>>4);
  
  return Buffer;

}

#define ByteMask ((1<<NumOfBytes)-1)

unsigned char GSM_Default_Alphabet[] = {

  /* FIXME: see ETSI GSM 03.38, 6.2.1 Default alphabet */

};

int fromoctet(int length, unsigned char *input, unsigned char *output)
{

  int NumOfBytes=7;
  unsigned char Rest=0x00;

  unsigned char *OUT=output, *IN=input;

  while ((IN-input) < length) {

    *OUT = ((*IN & ByteMask) << (7-NumOfBytes)) | Rest;

    Rest = *IN >> NumOfBytes;

    IN++;OUT++;

    if (NumOfBytes==1) {
      *OUT=Rest;
      OUT++;
      NumOfBytes=7;
      Rest=0x00;
    }
    else
      NumOfBytes--;
  }

  return OUT-output;
}

enum FB61_RX_States		FB61_RX_DispatchMessage(void)
{

  int i, tmp, count;
  unsigned char output[160];

#ifdef DEBUG
  FB61_RX_DisplayMessage();
#endif DEBUG

		/* Switch on the basis of the message type byte */
	switch (MessageType) {
	  
	  /* Call information */

	case 0x01:
	  
	  switch (MessageBuffer[3]) {

	    /* Unknown message - it has been seen after the 0x07 message (call
               answered). Probably it has similar meaning. If you can solve
               this - just mail me. Pavel Janík ml.

	       The message looks like this:

	       Msg Destination: PC
	       Msg Source: Phone
	       Msg Type: 01
	       Msg Unknown: 00
	       Msg Len: 0e

	       Phone: [01 ][08 ][00 ] is the header of the frame

	              [03 ] is the call message subtype

		      [05 ] is the call sequence number

		      [05 ] unknown 

		      [00 ][01 ][03 ][02 ][91][00] are unknown but has been
		      seen in the Incoming call message (just after the
		      caller's name from the phonebook). But never change
		      between phone calls :-(
	    */
	  case 0x03:
	    printf("Message: Call message, type 0x03:");
	    printf("   Sequence nr. of the call: %d\n", MessageBuffer[4]);
	    printf("   Exact meaning not known yet, sorry :-(\n");
	    break;

	    /* Remote end has gone away before you answer the call.  Probably
               your mother-in-law or banker (which is worse?) ... */
	  case 0x04:

	    printf("Message: Remote end hang up.\n");
	    printf("   Sequence nr. of the call: %d\n", MessageBuffer[4]);
	    break;

	    /* Incoming call alert */
	  case 0x05:

	    printf("Message: Incoming call alert:\n");

	    /* We can have more then one call ringing - we can distinguish
               between them */

	    printf("   Sequence nr. of the call: %d\n", MessageBuffer[4]);
	    printf("   Number: ");
	    count=MessageBuffer[6];
	  
	    for (tmp=0; tmp <count; tmp++)
	      printf("%c", MessageBuffer[7+tmp]);

	    printf("\n");

	    printf("   Name: ");

	    for (tmp=0; tmp <MessageBuffer[7+count]; tmp++)
	      printf("%c", MessageBuffer[8+count+tmp]);

	    printf("\n");
	    break;

	    /* Call answered. Probably your girlfriend...*/
	  case 0x07:

	    printf("Message: Call answered.\n");
	    printf("   Sequence nr. of the call: %d\n", MessageBuffer[4]);
	    break;

	    /* Call ended. Girlfriend is girlfriend, but time is money :-) */
	  case 0x09:

	    printf("Message: Call ended by your phone.\n");
	    printf("   Sequence nr. of the call: %d\n", MessageBuffer[4]);
	    break;

	    /* This message has been seen with the message of subtype 0x09
               after I hang the call.

	       Msg Destination: PC
	       Msg Source: Phone
	       Msg Type: 01
	       Msg Unknown: 00
	       Msg Len: 08
	       Phone: [01 ][08 ][00 ][0a ][04 ][87 ][01 ][42B][1a ][c2 ]

	       What is the meaning of 87? Can you spell some magic light into
	       this issue?

	    */
	  case 0x0a:

	    printf("Message: Call message, type 0x0a:");
	    printf("   Sequence nr. of the call: %d\n", MessageBuffer[4]);
	    printf("   Exact meaning not known yet, sorry :-(\n");
	    break;

	  default:
#ifdef DEBUG
	    printf("Message: Unknown message of type 0x01\n");
#endif DEBUG
	}

	break;

	/* General phone control */

	case 0x02:

	  switch (MessageBuffer[3]) {

	  case 0x10:
	  
	  	printf(_("Message: SMS Message Received\n"));
    	  	printf("   SMS center number: %s\n", FB61_GetBCDNumber(MessageBuffer+7));
		MessageBuffer[23] = MessageBuffer[23]/2+1;
		printf("   Remote number: %s\n", FB61_GetBCDNumber(MessageBuffer+23));
      		printf("   Date: %s\n", FB61_GetPackedDateTime(MessageBuffer+35));
		printf("   SMS: ");

		tmp=fromoctet(MessageBuffer[0x16], MessageBuffer+42, output);

		for (i=0; i<tmp;i++)
		  printf("%c", (output[i]==0x00)?'@':output[i]);

		printf("\n");

		break;

	  case 0x34:

		printf(_("Message: SMS Center received:\n"));
		printf("   %d. SMS Center name is %s\n", MessageBuffer[4], MessageBuffer+33);
		printf("   %d. SMS Center number is %s\n", MessageBuffer[4], FB61_GetBCDNumber(MessageBuffer+21));

		break;

	  case 0x35:

		/* Nokia 6110 has support for three SMS centers with numbers 1, 2 and
           3. Each request for SMS Center without one of these numbers fail. */

		printf(_("Message: SMS Center error received:\n"));
		printf(_("   The request for SMS Center failed.\n"));

		break;  

	  default:
	    	    FB61_RX_DisplayMessage();
	  }

	  break;

	  /* Phonebook handling */

	case 0x03:

	  switch (MessageBuffer[3]) {

	  case 0x02:

#ifdef DEBUG
	    printf(_("Message: Phonebook entry received:\n"));
	    printf("   Name: ");
#endif DEBUG

	    CurrentPhonebookEntry->Empty = true;

	    count=MessageBuffer[5];
	  
#ifdef DEBUG
	    for (tmp=0; tmp <count; tmp++)
	      printf("%c", MessageBuffer[6+tmp]);

	    printf("\n");
#endif DEBUG

	    memcpy(CurrentPhonebookEntry->Name, MessageBuffer + 6, count);
	    CurrentPhonebookEntry->Name[count] = 0x00;
	    CurrentPhonebookEntry->Empty = false;

	    i=7+count;
	    count=MessageBuffer[6+count];

#ifdef DEBUG
	    printf("   Number: ");

	    for (tmp=0; tmp <count; tmp++)
	      printf("%c", MessageBuffer[i+tmp]);

	    printf("\n");
#endif DEBUG

	    memcpy(CurrentPhonebookEntry->Number, MessageBuffer + i, count);
	    CurrentPhonebookEntry->Number[count] = 0x00;
	    CurrentPhonebookEntry->Group = MessageBuffer[i+count];

	    /* Signal no error to calling code. */
	    CurrentPhonebookError = GE_NONE;

	    break;

	  case 0x03:

#ifdef DEBUG
	    printf(_("Message: Phonebook read entry error received:\n"));
#endif DEBUG
	    break;

	  case 0x08:

#ifdef DEBUG
	    printf(_("Message: Memory status received:\n"));
	    printf("   Memory Type: %s\n", (MessageBuffer[4]==FB61_MEMORY_PHONE)?"phone":"sim");
	    printf("   Used: %d\n", MessageBuffer[6]);
	    printf("   Free: %d\n", MessageBuffer[5]);
#endif DEBUG

	    break;

	  default:

#ifdef DEBUG
	    printf("Message: Unknown message of type 0x03\n");
#endif DEBUG

	  }
	  break;

	  /* Phone status */
	  
	case 0x04:

	  switch (MessageBuffer[3]) {

	  case 0x02:

#ifdef DEBUG
	    printf(_("Message: Phone status received:\n"));
	    printf("   Mode: ");

	    switch (MessageBuffer[4]) {

	    case 0x01:
	      printf("registered within the network\n");
	      break;
	      
	      /* I was really amazing why is there a hole in the type of 0x02,
		 now I know ... */
	      
	    case 0x02:
	      printf("call in progress\n"); /* ringing or already answered call */
	      break;
	      
	    case 0x03:
	      printf("waiting for pin\n");
	      break;

	    case 0x04:
	    printf("powered off\n");
	    break;

	    default:
	      printf("unknown\n");
	    }

	    printf("   Power source: ");

	    switch (MessageBuffer[7]) {

	    case 0x01:
	      printf("AC/DC\n");
	      break;
		
	    case 0x02:
	      printf("battery\n");
	      break;

	    default:
	      printf("unknown\n");
	    }

	    printf("   Battery Level: %d\n", MessageBuffer[8]);
	    printf("   Signal strength: %d\n", MessageBuffer[5]);

#endif DEBUG

	    break;

	  default:

#ifdef DEBUG
	    printf("Message: Unknown message of type 0x04\n");
#endif DEBUG

	  }
	  break;

	  /* PIN requests */

	case 0x08:

#ifdef DEBUG
	  printf("Message: PIN:\n");
#endif DEBUG
	  if (MessageBuffer[3] != 0x0c) {
#ifdef DEBUG
	    printf("   Code Accepted\n");
#endif DEBUG
	    PINError = GE_NONE;
	  }
	  else {
#ifdef DEBUG
	    printf("   Code Error\n   You're not my big owner :-)\n");
#endif DEBUG
	  PINError = GE_INVALIDPIN;
	  }

	  break;

	  /* Network status */

	case 0x0a:

#ifdef DEBUG
	  printf("Message: Network registration:\n");

	  printf("   CellID: %02x%2x\n", MessageBuffer[10], MessageBuffer[11]);
	  printf("   LAC: %02x%02x\n", MessageBuffer[12], MessageBuffer[13]);

	  sprintf(output, "%x%x%x %x%x", MessageBuffer[14] & 0x0f, MessageBuffer[14] >>4, MessageBuffer[15] & 0x0f, MessageBuffer[16] & 0x0f, MessageBuffer[16] >>4);

	  printf("   Network code: %s\n", output);
	  printf("   Network name: %s\n", GSM_GetNetworkName(output));
#endif DEBUG

	  break;

	  /* Keyboard lock */

	  /* FIXME: there are at least two types of 0x0d messages (length 09
             and 0b(?)). The exact meaning of all bytes except keyboard
             locked/unlocked is not known yet. */

	case 0x0d:

#ifdef DEBUG
	  printf("Message: Keyboard lock\n");
	  printf("   Keyboard is %s\n", (MessageBuffer[6]==2)?"locked":"unlocked");
#endif DEBUG

	  break;

	  /* Phone Clock and Alarm */

	case 0x11:

	  switch (MessageBuffer[3]) {

	  case 0x61:
	    printf("Message: Date and time set correctly\n");

	    break;

	  case 0x63:

#ifdef DEBUG
	    printf("Message: Clock\n");
	    printf("   Time: %02d:%02d:%02d\n", MessageBuffer[12], MessageBuffer[13], MessageBuffer[14]);
	    printf("   Date: %4d/%02d/%02d\n", 256*MessageBuffer[8]+MessageBuffer[9], MessageBuffer[10], MessageBuffer[11]);
#endif DEBUG

	    CurrentDateTime->Year=256*MessageBuffer[8]+MessageBuffer[9];
	    CurrentDateTime->Month=MessageBuffer[10];
	    CurrentDateTime->Day=MessageBuffer[11];

	    CurrentDateTime->Hour=MessageBuffer[12];
	    CurrentDateTime->Minute=MessageBuffer[13];
	    CurrentDateTime->Second=MessageBuffer[14];

	    CurrentDateTimeError=GE_NONE;

	    break;

	  case 0x6e:

#ifdef DEBUG
	    printf("Message: Alarm\n");
	    printf("   Alarm: %02d:%02d\n", MessageBuffer[9], MessageBuffer[10]);
	    printf("   Alarm is %s\n", (MessageBuffer[8]==2) ? "on":"off");
#endif DEBUG

	    CurrentAlarm->Hour=MessageBuffer[9];
	    CurrentAlarm->Minute=MessageBuffer[10];
	    CurrentAlarm->Second=0;

	    CurrentAlarm->AlarmEnabled=(MessageBuffer[8]==2);

	    CurrentAlarmError=GE_NONE;

	    break;

	  default:
	    printf("Message: Unknown message of type 0x11\n");

	  }
	  break;

	  /* Calendar notes handling */

	case 0x13:

	  switch (MessageBuffer[3]) {

	  case 0x67:

	    printf("Message: Calendar note received.\n");
	    printf("   Date: %d-%02d-%02d\n", 256*MessageBuffer[9]+MessageBuffer[10], MessageBuffer[11], MessageBuffer[12]);
	    printf("   Time: %02d:%02d:%02d\n", MessageBuffer[13], MessageBuffer[14], MessageBuffer[15]);

	    /* Some messages do not have alarm set up */
	    if (256*MessageBuffer[16]+MessageBuffer[17] != 0) {
	      printf("   Alarm date: %d-%02d-%02d\n", 256*MessageBuffer[16]+MessageBuffer[17], MessageBuffer[18], MessageBuffer[19]);
	      printf("   Alarm time: %02d:%02d:%02d\n", MessageBuffer[20], MessageBuffer[21], MessageBuffer[22]);
	    }

	    printf("   Number: %d\n", MessageBuffer[8]);
	    printf("   Text: ");

	    tmp=MessageBuffer[23];

	    for(i=0;i<tmp;i++)
	      printf("%c", MessageBuffer[24+i]);

	    printf("\n");

	    break;

	  default:

#ifdef DEBUG
	    printf("Message: Unknown message of type 0x13\n");
#endif DEBUG
	  }

	  break;

	  /* SMS Messages frame received */

	case 0x14:

	  switch (MessageBuffer[3]) {

	  case 0x08:

		MessageBuffer[24] = MessageBuffer[24]/2+1;

#ifdef DEBUG
		printf("Message: SMS Received.\n");
		printf("   Number: %d\n", MessageBuffer[6]);

		/* FIXME: This is probably bad ...

		printf("   Memory Type: %s\n", (MessageBuffer[5]==FB61_MEMORY_PHONE)?"phone":"sim");
		*/

		printf("   Date: %s\n", FB61_GetPackedDateTime(MessageBuffer+36));
	  	printf("   SMS center number: %s\n", FB61_GetBCDNumber(MessageBuffer+8));
		printf("   Remote number: %s\n", FB61_GetBCDNumber(MessageBuffer+24));
#endif

		CurrentSMSMessage->Year=10*(MessageBuffer[36]&0x0f)+(MessageBuffer[36]>>4);
		CurrentSMSMessage->Month=10*(MessageBuffer[37]&0x0f)+(MessageBuffer[37]>>4);
		CurrentSMSMessage->Day=10*(MessageBuffer[38]&0x0f)+(MessageBuffer[38]>>4);
		CurrentSMSMessage->Hour=10*(MessageBuffer[39]&0x0f)+(MessageBuffer[39]>>4);
		CurrentSMSMessage->Minute=10*(MessageBuffer[40]&0x0f)+(MessageBuffer[40]>>4);
		CurrentSMSMessage->Second=10*(MessageBuffer[41]&0x0f)+(MessageBuffer[41]>>4);

		strcpy(CurrentSMSMessage->Sender, FB61_GetBCDNumber(MessageBuffer+24));

		strcpy(CurrentSMSMessage->MessageCentre, FB61_GetBCDNumber(MessageBuffer+8));

		tmp=fromoctet(MessageLength-43-2, MessageBuffer+43, output);

		for (i=0; i<tmp;i++) {

#ifdef DEBUG
		  printf("%c", (output[i]==0x00)?'@':output[i]);
#endif DEBUG

		  /* FIXME: GSM Alphabet should be here... */
		  CurrentSMSMessage->MessageText[i]=(output[i]==0x00)?'@':output[i];
		}

		CurrentSMSMessage->MessageText[tmp]=0;

		CurrentSMSPointer=tmp;

		CurrentSMSMessage->MemoryType = MessageBuffer[5];
		CurrentSMSMessage->MessageNumber = MessageBuffer[6];
 
		/* Signal no error to calling code. */
		
		if (MessageBuffer[MessageLength-2]==2)
		  CurrentSMSMessageError = GE_SMSWAITING;
		else
		  CurrentSMSMessageError = GE_NONE;

#ifdef DEBUG
		printf("\n");
#endif DEBUG

		break;

	  case 0x09:
		/* You have requested invalid or empty location. */

#ifdef DEBUG
		printf("Message: SMS reading failed.\n");
#endif DEBUG

		break;

	  case 0x37:

#ifdef DEBUG
		printf("Message: SMS Status Received\n");
		printf("   The number of messages: %d\n", MessageBuffer[10]);
		printf("   Unread messages: %d\n", MessageBuffer[11]);
#endif DEBUG

	  break;
	  
	  default:

#ifdef DEBUG
	    printf("Message: the rest of the SMS message received.\n");
#endif DEBUG

	    tmp=fromoctet(MessageLength-2, MessageBuffer, output);

		for (i=0; i<tmp;i++) {

#ifdef DEBUG
		  printf("%c", (output[i]==0x00)?'@':output[i]);
#endif DEBUG

		  CurrentSMSMessage->MessageText[CurrentSMSPointer+i]=(output[i]==0x00)?'@':output[i];
		}

		CurrentSMSMessage->MessageText[CurrentSMSPointer+tmp]=0;

		CurrentSMSMessageError = GE_NONE;

#ifdef DEBUG
		printf("\n");
#endif DEBUG

	  }
	  break;

	  /* Mobile phone identification */

	case 0x64:

#ifdef DEBUG

	  printf("Message: Mobile phone identification received:\n");
	  printf("   Serial No. %s\n", MessageBuffer+4);

	  /* What is this? My phone reports: NSE-3 */
	  printf("   %s\n", MessageBuffer+25);

	  /* What is this? My phone reports: 0501505 */
	  printf("   %s\n", MessageBuffer+31);

	  /* What is this? My phone reports: 4200. John Kougoulos reported
	     4300... */

	  printf("   %s\n", MessageBuffer+39);

	  printf("   Firmware: %s\n", MessageBuffer+44);

	  /* These bytes are probably the source of the "Accessory not connected"
             messages on the phone when trying to emulate NCDS... I hope... */
   	  printf("   Magic byte1: %02x\n", MessageBuffer[50]);
   	  printf("   Magic byte2: %02x\n", MessageBuffer[51]);
   	  printf("   Magic byte3: %02x\n", MessageBuffer[52]);
   	  printf("   Magic byte4: %02x\n", MessageBuffer[53]);

#endif DEBUG

	  break;

	  /* Acknowlegment */
	  
	case 0x7f:

#ifdef DEBUG
  printf(_("[Received Ack of type %02x, seq: %2x]\n"), MessageBuffer[0], MessageBuffer[1]);
#endif DEBUG

  FB61_LinkOK = true;

	  break;

	  /* Power on message */

	case 0xd0:

	  printf(_("Message: The phone is powered on - seq 1.\n"));
	  FB61_RX_DisplayMessage();
	  break;

	  /* Power on message */

	case 0xf4:

	  printf(_("Message: The phone is powered on - seq 2.\n"));
	  FB61_RX_DisplayMessage();
	  break;

	  /* Unknown message - if you think that you know the exact meaning of
         other messages - please let us know. */

	default:
	  FB61_RX_DisplayMessage();

}

	return FB61_RX_Sync;
}

enum FB61_RX_States         RX_State;

	/* RX_State machine for receive handling.  Called once for each
	   character received from the phone/phone. */
void	FB61_RX_StateMachine(char rx_byte)
{

	switch (RX_State) {
	
					/* Messages from the phone start with an 0x1e.  We
					   use this to "synchronise" with the incoming data
					   stream. */		
		case FB61_RX_Sync:
				if (rx_byte == FB61_FRAME_ID) {
					BufferCount = 0;
					RX_State = FB61_RX_GetDestination;
				}
				break;

		case FB61_RX_GetDestination:
		  MessageDestination=rx_byte;
		  RX_State = FB61_RX_GetSource;
		  break;

		case FB61_RX_GetSource:
		  MessageSource=rx_byte;
		  RX_State = FB61_RX_GetType;
		  break;

		case FB61_RX_GetType:
		  MessageType=rx_byte;
		  RX_State = FB61_RX_GetUnknown;
		  break;

		case FB61_RX_GetUnknown:
		  MessageUnknown=rx_byte;
		  RX_State = FB61_RX_GetLength;
		  break;
		
		case FB61_RX_GetLength:
				MessageLength = rx_byte;
				RX_State = FB61_RX_GetMessage;
				break;

		case FB61_RX_GetMessage:
				MessageBuffer[BufferCount] = rx_byte;
				BufferCount ++;

				/* If this is the last byte, it's the checksum */

				if (BufferCount == MessageLength+(MessageLength%2)+2) {

				  if (MessageType != FB61_FRTYPE_ACK)
					FB61_TX_SendAck(MessageType, MessageBuffer[MessageLength-1] & 0x0f);

		  FB61_RX_DispatchMessage();

		  RX_State = FB61_RX_Sync;
		}

					
				break;

		default:
	}
}

char *FB61_PrintDevice(int Device)
{
    switch (Device) {

	case FB61_DEVICE_PHONE:
	  return "Phone";

	case FB61_DEVICE_PC:
	  return "PC";

	default:
	  return "Unknown";
	}
}

/* FB61_RX_DisplayMessage is called when a message we don't know about is
   received so that the user can see what is going back and forth, and perhaps
   shed some more light/explain another message type! */
	
void	FB61_RX_DisplayMessage(void)
{

#ifdef DEBUG

  int		count;

  fprintf(stdout, _("Msg Destination: %s\n"), FB61_PrintDevice(MessageDestination));
  fprintf(stdout, _("Msg Source: %s\n"), FB61_PrintDevice(MessageSource));

  fprintf(stdout, _("Msg Type: %02x\n"), MessageType);

  fprintf(stdout, _("Msg Unknown: %02x\n"), MessageUnknown);
  fprintf(stdout, _("Msg Len: %02x\nPhone: "), MessageLength);

  for (count = 0; count < MessageLength+(MessageLength%2)+2; count ++)
    if (isprint(MessageBuffer[count]))
      fprintf(stdout, "[%02x%c]", MessageBuffer[count], MessageBuffer[count]);
    else
      fprintf(stdout, "[%02x ]", MessageBuffer[count]);

  fprintf(stdout, "\n");
  fflush(stdout);
#endif DEBUG
}

	/* Prepares the message header and sends it, prepends the
	   message start byte (0x1e) and other values according
	   the value specified when called.  Calculates checksum
	   and then sends the lot down the pipe... */
int		FB61_TX_SendMessage(u8 message_length, u8 message_type, u8 *buffer)
{
	u8			out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
	int			count, current=0;
	unsigned char			checksum;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */
	out_buffer[current++] = FB61_FRAME_ID;	/* Start of message indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC; /* Source */

	out_buffer[current++] = message_type; /* Type */

	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = message_length+1; /* Length + 1 for seq. nr*/

		/* Copy in data if any. */	
	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current+=message_length;
	}

	out_buffer[current++]=0x40+RequestSequenceNumber;

	RequestSequenceNumber=(RequestSequenceNumber+1) & 0x07;

	/* If the message length is odd we should add pad byte 0x00 */
	if ( (message_length+1) % 2)
	  out_buffer[current++]=0x00;

		/* Now calculate checksums over entire message 
		   and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

#ifdef DEBUG

	printf("PC: ");

	for (count = 0; count < current; count++)
	  printf("%02x:", out_buffer[count]);

	printf("\n");

#endif DEBUG

		/* Send it out... */
	if (write(PortFD, out_buffer, current) != current) {
		return (false);
	}
	return (true);
}

int FB61_TX_SendAck(u8 message_type, u8 message_seq) {

  unsigned char out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
  int count, current=0;
  unsigned char			checksum;

#ifdef DEBUG
  printf("[Sending Ack of type %02x, seq: %x]\n", message_type, message_seq);
#endif DEBUG

  /* Now construct the Ack header. */
  out_buffer[current++] = FB61_FRAME_ID;	/* Start of message indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC; /* Source */

	out_buffer[current++] = FB61_FRTYPE_ACK; /* Ack */

	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = 2; /* Ack is always of 2 bytes */

	out_buffer[current++] = message_type; /* Type */
	out_buffer[current++] = message_seq; /* Sequence number */

		/* Now calculate checksums over entire message 
		   and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

		/* Send it out... */
	if (write(PortFD, out_buffer, current) != current) {
		return (false);
	}

return true;  
}
