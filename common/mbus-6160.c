/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This code contains the main part of the 5160/6160 code.
	
  Last modification: Fri May 19 15:31:26 EST 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#ifndef WIN32

#define		__mbus_6160_c	/* "Turn on" prototypes in mbus-6160.h */

#include	<termios.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/ioctl.h>
#include	<string.h>
#include	<pthread.h>
#include	<errno.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"mbus-6160.h"
#include	"device.h"

#define WRITEPHONE(a, b, c) device_write(b, c)

//#define	DEBUG

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					MB61_LinkOK;

GSM_Functions			MB61_Functions = {
		MB61_Initialise,
		MB61_Terminate,
		MB61_GetMemoryLocation,
		MB61_WritePhonebookLocation,
		MB61_GetSpeedDial,
		MB61_SetSpeedDial,
		MB61_GetMemoryStatus,
		MB61_GetSMSStatus,
		MB61_GetSMSCenter,
		MB61_SetSMSCenter,
  		MB61_GetSMSMessage,
		MB61_DeleteSMSMessage,
		MB61_SendSMSMessage,
      MB61_SaveSMSMessage,
		MB61_GetRFLevel,
		MB61_GetBatteryLevel,
		MB61_GetPowerSource,
		MB61_GetDisplayStatus,
		MB61_EnterSecurityCode,
		MB61_GetSecurityCodeStatus,
		MB61_GetIMEI,
		MB61_GetRevision,
		MB61_GetModel,
		MB61_GetDateTime,
		MB61_SetDateTime,
		MB61_GetAlarm,
		MB61_SetAlarm,
		MB61_DialVoice,
		MB61_DialData,
		MB61_GetIncomingCallNr,
		MB61_GetNetworkInfo,
		MB61_GetCalendarNote,
		MB61_WriteCalendarNote,
		MB61_DeleteCalendarNote,
		MB61_Netmonitor,
		MB61_SendDTMF,
		MB61_GetBitmap,
		MB61_SetBitmap,
		MB61_SetRingTone,
		MB61_SendRingTone,
		MB61_Reset,
		MB61_GetProfile,
		MB61_SetProfile,
		MB61_SendRLPFrame,
        MB61_CancelCall,
		MB61_EnableDisplayOutput,
		MB61_DisableDisplayOutput,
		MB61_EnableCellBroadcast,
		MB61_DisableCellBroadcast,
		MB61_ReadCellBroadcast
};

	/* FIXME - these are guesses only... */
GSM_Information			MB61_Information = {
		"5160|6160|6185",		/* Models */
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
pthread_t				Thread;
bool					RequestTerminate;
bool					MB61_LinkOK;
char                    PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
u8						RequestSequenceNumber; /* 2-63 */
int						PortFD;
bool					GotInitResponse;
bool					GotIDResponse;

enum MB61_RX_States     RX_State;
enum MB61_Models		ModelIdentified;
enum MB61_Responses		ExpectedResponse;
enum MB61_Responses		LatestResponse;

GSM_PhonebookEntry		*CurrentPhonebookEntry;
GSM_Error				CurrentPhonebookError;

int                     MessageLength;
u8                      MessageDestination;
u8                      MessageSource;
u8                      MessageCommand;
u8                      MessageBuffer[MB61_MAX_RECEIVE_LENGTH];
u8						MessageCSum;
u8						MessageSequenceNumber;
int                     BufferCount;
u8                      CalculatedCSum;
int						ChecksumFails;


	/* The following functions are made visible to gsm-api.c and friends. */

	/* Initialise variables and state machine. */
GSM_Error   MB61_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame))
{

	int		rtn;

	RequestTerminate = false;
	MB61_LinkOK = false;
	ModelIdentified = MB61_ModelUnknown;
	ExpectedResponse = MB61_Response_Unknown;
	CurrentPhonebookEntry = NULL;
	CurrentPhonebookError = GE_NONE;


    strncpy (PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) MB61_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (GE_INTERNALERROR);
    }

	return (GE_NONE);

}

	/* Applications should call MB61_Terminate to shut down
	   the MB61 thread and close the serial port. */
void		MB61_Terminate(void)
{
		/* Request termination of thread */
	RequestTerminate = true;

		/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

		/* Close serial port. */
	
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	MB61_GetMemoryLocation(GSM_PhonebookEntry *entry)
{
    int     timeout;

    if (entry->MemoryType != GMT_ME) {
    	return (GE_INVALIDMEMORYTYPE);
    }

    timeout = 20;   /* 2 seconds for command to complete */

        /* Return if no link has been established. */
    if (!MB61_LinkOK) {
        return GE_NOLINK;
    }

        /* Process depending on model identified */
	switch (ModelIdentified) {

		case MB61_Model5160:
			if (entry->Location >= MAX_5160_PHONEBOOK_ENTRIES) {
				return (GE_INVALIDPHBOOKLOCATION);
			}
		 	CurrentPhonebookEntry = entry;
		 	CurrentPhonebookError = GE_BUSY;
			MB61_SetExpectedResponse(MB61_Response_0x40_PhonebookRead);
			MB61_TX_SendPhonebookReadRequest(entry->Location);
			break;

		case MB61_Model6160:
			if (entry->Location >= MAX_6160_PHONEBOOK_ENTRIES) {
				return (GE_INVALIDPHBOOKLOCATION);
			}
		 	CurrentPhonebookEntry = entry;
		 	CurrentPhonebookError = GE_BUSY;
			MB61_SetExpectedResponse(MB61_Response_0x40_PhonebookRead);
			MB61_TX_SendPhonebookReadRequest(entry->Location);
			break;

		case MB61_Model6185:
			if (entry->Location >= MAX_6185_PHONEBOOK_ENTRIES) {
				return (GE_INVALIDPHBOOKLOCATION);
			}
		 	CurrentPhonebookEntry = entry;
		 	CurrentPhonebookError = GE_BUSY;
			MB61_SetExpectedResponse(MB61_Response_0x40_LongPhonebookRead);
			MB61_TX_SendLongPhonebookReadRequest(entry->Location);
			break;

		default:
			return(GE_NOTIMPLEMENTED);
	}

		/* When response is received, data is copied into entry
           by handler code or if error has occured, CurrentPhonebookEntry
           is set accordingly. */
	if (MB61_WaitForExpectedResponse(2000) != true) {
		return (GE_INTERNALERROR);
	}
	return (CurrentPhonebookError);

}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	MB61_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{

    if (entry->MemoryType != GMT_ME) {
    	return (GE_INVALIDMEMORYTYPE);
    }

        /* Return if no link has been established. */
    if (!MB61_LinkOK) {
        return GE_NOLINK;
    }

        /* Process depending on model identified */
	switch (ModelIdentified) {

		case MB61_Model5160:
			if (entry->Location >= MAX_5160_PHONEBOOK_ENTRIES) {
				return (GE_INVALIDPHBOOKLOCATION);
			}
			if (strlen(entry->Name) > MAX_5160_PHONEBOOK_NAME_LENGTH) {
				return (GE_PHBOOKNAMETOOLONG);
			}
			if (strlen(entry->Name) > MAX_5160_PHONEBOOK_NUMBER_LENGTH) {
				return (GE_PHBOOKNAMETOOLONG);
			}
		 	CurrentPhonebookError = GE_BUSY;
			MB61_SetExpectedResponse(MB61_Response_0x40_WriteAcknowledge);
			MB61_TX_SendPhonebookWriteRequest(entry);
			break;

		case MB61_Model6160:
			if (entry->Location >= MAX_6160_PHONEBOOK_ENTRIES) {
				return (GE_INVALIDPHBOOKLOCATION);
			}
			if (strlen(entry->Name) > MAX_616X_PHONEBOOK_NAME_LENGTH) {
				return (GE_PHBOOKNAMETOOLONG);
			}
			if (strlen(entry->Name) > MAX_616X_PHONEBOOK_NUMBER_LENGTH) {
				return (GE_PHBOOKNAMETOOLONG);
			}
		 	CurrentPhonebookError = GE_BUSY;
			MB61_SetExpectedResponse(MB61_Response_0x40_WriteAcknowledge);
			MB61_TX_SendPhonebookWriteRequest(entry);
			break;

		case MB61_Model6185:
			return (GE_NOTIMPLEMENTED);
			break;

		default:
			return(GE_NOTIMPLEMENTED);
	}

		/* When response is received, data is copied into entry
           by handler code or if error has occured, CurrentPhonebookEntry
           is set accordingly. */
	if (MB61_WaitForExpectedResponse(2000) != true) {
		return (GE_INTERNALERROR);
	}
	return (CurrentPhonebookError);

}

GSM_Error	MB61_GetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DeleteSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SendSMSMessage(GSM_SMSMessage *SMS, int data_size)
{
	return (GE_NOTIMPLEMENTED);
}


GSM_Error   MB61_SaveSMSMessage(GSM_SMSMessage *SMS)
{
    return GE_NOTIMPLEMENTED;
}

	/* MB61_GetRFLevel
	   FIXME (sort of...)
	   For now, GetRFLevel and GetBatteryLevel both rely
	   on data returned by the "keepalive" packets.  I suspect
	   that we don't actually need the keepalive at all but
	   will await the official doco before taking it out.  HAB19990511 */
GSM_Error	MB61_GetRFLevel(GSM_RFUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB61_GetBatteryLevel
	   FIXME (see above...) */
GSM_Error	MB61_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetIMEI(char *imei)
{
	return (GE_NOTIMPLEMENTED);

}

GSM_Error	MB61_GetRevision(char *revision)
{
	return (GE_NOTIMPLEMENTED);

}

GSM_Error	MB61_GetModel(char *model)
{
	return (GE_NOTIMPLEMENTED);

}

/* This function sends to the mobile phone a request for the SMS Center */

GSM_Error	MB61_GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Our "Not implemented" functions */
GSM_Error	MB61_GetMemoryStatus(GSM_MemoryStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSMSStatus(GSM_SMSStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetPowerSource(GSM_PowerSource *source)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetDisplayStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_EnterSecurityCode(GSM_SecurityCode SecurityCode)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSecurityCodeStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DialVoice(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DialData(char *Number, char type, void (* callpassup)(char c))
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetIncomingCallNr(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetNetworkInfo (GSM_NetworkInfo *NetworkInfo)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_WriteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DeleteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_Netmonitor(unsigned char mode, char *Screen)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SendDTMF(char *String)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetBitmap(GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetBitmap(GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error       MB61_SetRingTone(GSM_Ringtone *ringtone)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error      MB61_SendRingTone(GSM_Ringtone *ringtone, char *dest)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_Reset(unsigned char type)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}


GSM_Error   MB61_EnableDisplayOutput()
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   MB61_DisableDisplayOutput()
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   MB61_EnableCellBroadcast()
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   MB61_DisableCellBroadcast()
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   MB61_ReadCellBroadcast(GSM_CBMessage *Message)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_CancelCall(void)
{
    return (GE_NOTIMPLEMENTED);
}

bool		MB61_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
    return (false);
}

	/* Everything from here down is internal to 6160 code. */


	/* This is the main loop for the MB61 functions.  When MB61_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB61_Terminate function. */
void	MB61_ThreadLoop(void)
{
    int                 idle_timer;

        /* Initialise RX state machine. */
    BufferCount = 0;
    RX_State = MB61_RX_Sync;
    idle_timer = 0;

        /* Try to open serial port, if we fail we sit here and don't proceed
           to the main loop. */
    if (MB61_OpenSerial() != true) {
        MB61_LinkOK = false;
        
        while (!RequestTerminate) {
            usleep (100000);
        }
        return;
    }

		/* Do initialisation sequence, sit here if it fails. */
	if (MB61_InitialiseLink() != true) {
         MB61_LinkOK = false;
        
        while (!RequestTerminate) {
            usleep (100000);
        }
        return;
    }

		/* Link is up OK so sit here twiddling our thumbs until
           told to terminate. */
    while (!RequestTerminate) {
        if (idle_timer == 0) {
            idle_timer = 20;
        }
        else {
            idle_timer --;
			/*fprintf(stdout, ".");
			fflush(stdout);*/
        }

        usleep(100000);     /* Avoid becoming a "busy" loop. */
    }

		/* Drop DTR and RTS lines before exiting */
    device_setdtrrts(0, 0);
}


bool	MB61_InitialiseLink(void)
{
    unsigned char       init_char[1] = {0x04};

    fprintf(stdout, "Sending init...\n");

		/* Need to "toggle" the dtr/rts lines in the right
           sequence it seems for the interface to work. 
		   Base time value is units of 50ms it seems */

#define	BASE_TIME		(50000)

		/* Default state */
    device_setdtrrts(0, 1);
	sleep(1);

		/* RTS low for 250ms */
    device_setdtrrts(0, 0);
	usleep(5 * BASE_TIME);

		/* RTS high, DTR high for 50ms */
    device_setdtrrts(1, 1);
	usleep(BASE_TIME);

		/* RTS low, DTR high for 50ms */
    device_setdtrrts(1, 0);
	usleep(BASE_TIME);

		/* RTS high, DTR high for 50ms */
    device_setdtrrts(1, 1);
	usleep(BASE_TIME);

		/* RTS low, DTR high for 50ms */
    device_setdtrrts(1, 0);
	usleep(BASE_TIME);

		/* RTS low, DTR low for 50ms */
    device_setdtrrts(0, 0);
	usleep(BASE_TIME);

		/* RTS low, DTR high for 50ms */
    device_setdtrrts(1, 0);
	usleep(BASE_TIME);

		/* RTS high, DTR high for 50ms */
    device_setdtrrts(1, 1);
	usleep(BASE_TIME);

		/* RTS low, DTR low for 50ms */
    device_setdtrrts(0, 0);
	usleep(BASE_TIME);

		/* leave RTS high, DTR low for duration of session. */
	usleep(BASE_TIME);
    device_setdtrrts(0, 1);

	sleep(1);


        /* Initialise sequence number used when sending messages
           to phone. */

        /* Send Initialisation message to phone. */
	MB61_SetExpectedResponse(MB61_Response_0xD0_Init);

    RequestSequenceNumber = 0x02;
	MB61_TX_SendMessage(MSG_ADDR_PHONE, MSG_ADDR_PC, 0xd0, RequestSequenceNumber, 1, init_char);

        /* We've now finished initialising things so sit in the loop
           until told to do otherwise.  Loop doesn't do much other
           than send periodic keepalive messages to phone.  This
           loop will become more involved once we start doing 
           fax/data calls. */

	fprintf(stdout, "Waiting for first response...\n");
	fflush(stdout);

	if(MB61_WaitForExpectedResponse(100) == false) {
		return false;
	}

	MB61_SetExpectedResponse(MB61_Response_0xD0_Init);

    RequestSequenceNumber ++;
	MB61_TX_SendMessage(MSG_ADDR_PHONE, MSG_ADDR_PC, 0xd0, RequestSequenceNumber, 1, init_char);


	fprintf(stdout, "Waiting for second response...\n");
	fflush(stdout);
	if(MB61_WaitForExpectedResponse(100) == false) {
		return false;
	}


	MB61_SetExpectedResponse(MB61_Response_0xD0_Init);
	MB61_TX_SendPhoneIDRequest();
	if(MB61_WaitForExpectedResponse(300) == false) {
		return false;
	}

	MB61_LinkOK = true;
	return (true);

}

	/* MB61_SetExpectedResponse
	   Used in combination with MB61_WaitForExpectedResponse, these
       functions allow the MB61 code to specify what message it is expecting
       a response to.  This becomes important as it appears that there is
       no standard or unique character to identify incoming messages from
       the phone.  A good example is the ID and Version responses which differ
       only in their length and one of the bytes in the data portion of the
       message.  It may be that once we understand more of the MBUS protocol
       we can confirm that messages are unique... */

void	MB61_SetExpectedResponse(enum MB61_Responses response)
{
	LatestResponse = MB61_Response_Unknown;
	ExpectedResponse = response;
}

	/* MB61_WaitForExpectedResponse
       Allows code to wait a specified number of msecs
       for the requested response before timing out.  Returns
       true if expected response has been recieved, false in
       case of timeout */
bool	MB61_WaitForExpectedResponse(int timeout)
{
	int		count;

	count = timeout;
	while ((count > 0) && (LatestResponse != ExpectedResponse)) {
		count --;
		usleep(1000);
	}

	if (LatestResponse == ExpectedResponse) {
		return (true);
	}

	return (false);
}
      

    /* MB61_RX_DispatchMessage
       Once we've received a message from the phone, the command/message
       type byte is used to call an appropriate handler routine or
       simply acknowledge the message as required. */
enum    MB61_RX_States MB61_RX_DispatchMessage(void)
{

				/* If the message is from ADDR_PC ignore and don't process further. */
			if (MessageSource == MSG_ADDR_PC) {
    			return MB61_RX_Sync;
			}
    			/* Leave this uncommented if you want all messages in raw form. */
			//MB61_RX_DisplayMessage(); 

				/* Switch on the basis of the message type byte */
			switch (MessageCommand) {

				case 0x40:
					if (MB61_TX_SendStandardAcknowledge(MessageSequenceNumber) != true) {
						fprintf(stderr, _("Standard Ack write (0x40) failed!"));
					}

					if (ExpectedResponse == MB61_Response_0x40_PhonebookRead) {
						MB61_RX_Handle0x40_PhonebookRead();
						LatestResponse = MB61_Response_0x40_PhonebookRead;
						break;
					}

					if (ExpectedResponse == MB61_Response_0x40_WriteAcknowledge) {
						LatestResponse = MB61_Response_0x40_WriteAcknowledge;
						CurrentPhonebookError = GE_NONE;
						break;
					}
					break;

					/* 0xd0 messages are the response to
					   initialisation requests. */
				case 0xd0:
					if (ExpectedResponse == MB61_Response_0xD0_Init) {
						LatestResponse = MB61_Response_0xD0_Init;
					}
					break;

				case 0xd2:
					if (MB61_TX_SendStandardAcknowledge(MessageSequenceNumber) != true) {
						fprintf(stderr, _("Standard Ack write (0xd2) failed!"));
					}
  					if (ExpectedResponse == MB61_Response_0xD2_ID) {
						MB61_RX_Handle0xD2_ID();
					LatestResponse = MB61_Response_0xD2_ID;
					}
					if (ExpectedResponse == MB61_Response_0xD2_Version) {
						MB61_RX_Handle0xD2_Version();
						LatestResponse = MB61_Response_0xD2_Version;
					}
					break;


					/* Incoming 0x7f's are acks for commands we've sent. */
				case 0x7f:  break;

					/* Here we attempt to acknowledge and display messages
					   we don't understand fully... The phone will send
					   the same message several (3-4) times before giving
					   up if no ack is received.  */
				default: 	MB61_RX_DisplayMessage(); 
					if (MB61_TX_SendStandardAcknowledge(MessageSequenceNumber) != true) {
						fprintf(stderr, _("Standard Ack write failed!"));
					}
					fprintf(stdout, "Sent standard Ack for unknown %02x\n", MessageCommand);
                   	break;
    }

    return MB61_RX_Sync;
}

	/* "Short" phonebook reads have 8 bytes of data (unknown/unstudied)
       then a null terminated string for the number and then a null
       terminated string which is the name. */
void	MB61_RX_Handle0x40_PhonebookRead(void)
{
	int		i, j;
	bool	got_null;

	if (CurrentPhonebookEntry == NULL) {
		CurrentPhonebookError = GE_INTERNALERROR;
		return;
	}

		/* First do number */
	i = 8;
	got_null = false;
	j = 0;

	while ((i < MessageLength) && (!got_null) && (j < GSM_MAX_PHONEBOOK_NUMBER_LENGTH)) {
		CurrentPhonebookEntry->Number[j] = MessageBuffer[i];
		i++;
		j++;
		if (MessageBuffer[i] == 0) {
			got_null = true;
		}
	}
	CurrentPhonebookEntry->Number[j] = 0;

		/* Now name */
	got_null = false;
	j = 0;
	i ++;

	while ((i < MessageLength) && (!got_null) && (j < GSM_MAX_PHONEBOOK_NAME_LENGTH)) {
		CurrentPhonebookEntry->Name[j] = MessageBuffer[i];
		i++;
		j++;
		if (MessageBuffer[i] == 0) {
			got_null = true;
		}
	}
	CurrentPhonebookEntry->Name[j] = 0;
	
	if ((strlen(CurrentPhonebookEntry->Number) != 0) ||
	    (strlen(CurrentPhonebookEntry->Name) != 0)) {
		CurrentPhonebookEntry->Empty = false;
	}
	else {
		CurrentPhonebookEntry->Empty = true;
	}
	CurrentPhonebookEntry->Group = GSM_GROUPS_NOT_SUPPORTED;   	

		/* Done */
	CurrentPhonebookError = GE_NONE;

}

void	MB61_RX_Handle0x40_LongPhonebookRead(void)
{


}



	/* When we get an ID response back, we use it to set
       model information for later and if in debug mode print it out. */
void	MB61_RX_Handle0xD2_ID(void) 
{
#ifdef DEBUG
	int		i;
#endif

	if (strstr(MessageBuffer + 4, "NSW-1") != NULL) {
		ModelIdentified = MB61_Model5160;
#ifdef DEBUG
		fprintf(stdout, "Identified as 5160\n");
#endif
	}
	else {
		if (strstr(MessageBuffer + 4, "NSW-3") != NULL) {
			ModelIdentified = MB61_Model6160;
#ifdef DEBUG
			fprintf(stdout, "Identified as 6160\n");
#endif
		}
		else {
			if (strstr(MessageBuffer + 4, "NSD-3") != NULL) {
				ModelIdentified = MB61_Model6185;
#ifdef DEBUG
				fprintf(stdout, "Identified as 6185\n");
#endif
			}
			else {
#ifdef DEBUG
				fprintf(stdout, "Unknown model - please report dump below to hugh@linuxcare.com\n");
#endif
			}
		}
	}
#ifdef DEBUG
	for (i = 4; i < MessageLength; i++) {
		if (isprint(MessageBuffer[i])) {
			fprintf(stdout, "[%02x%c]", MessageBuffer[i], MessageBuffer[i]);
		}
		else {
			fprintf(stdout, "[%02x ]", MessageBuffer[i]);
		}
		if (((i - 3) % 16) == 0) {
			fprintf(stdout, "\n");
		}
	}	
#endif

	
#ifdef DEBUG
	fflush(stdout);
#endif

}
void	MB61_RX_Handle0xD2_Version(void) 
{


}


void    MB61_RX_DisplayMessage(void)
{
	int		i;

	fprintf(stdout, "Dest:%02x Src:%02x Cmd:%02x Len:%d Seq:%02x Csum:%02x\n", 
			MessageDestination, MessageSource, MessageCommand, MessageLength,
			MessageSequenceNumber, MessageCSum);

	if (MessageLength == 0) {
		return;
	}
	else {
		fprintf(stdout, "Data: ");
	}
	for (i = 0; i < MessageLength; i++) {
		if (isprint(MessageBuffer[i])) {
			fprintf(stdout, "[%02x%c]", MessageBuffer[i], MessageBuffer[i]);
		}
		else {
			fprintf(stdout, "[%02x ]", MessageBuffer[i]);
		}
		if (((i + 1) % 8) == 0) {
			fprintf(stdout, "\n      ");
		}
	}
	fprintf(stdout, "\n");

}

	/* Higher level code does bounds checks for length of name/number
       as well as entry number. */
GSM_Error	MB61_TX_SendPhonebookWriteRequest(GSM_PhonebookEntry *entry)
{
		/* 7 - header and null terminators, 17 - number length, 
		   17 - name length. */
	u8		message[7 + 17 + 17];
	int		name_length;
	int 	number_length;
	int		message_length;

	name_length = strlen(entry->Name);
	number_length = strlen(entry->Number);

 		/* Header plus two terminating nulls plus name/number themselves */
	message_length = 7 + name_length + 1 + number_length + 1;

	message[0] = 0x00; /* Header bytes, purpose not investigated */
	message[1] = 0x01; 
	message[2] = 0x1f; 
	message[3] = 0x01; 
	message[4] = 0x04; 
	message[5] = 0x87; 	

	message[6] = entry->Location; 

	strncpy(message + 7, entry->Number, 16);

	strncpy(message + 8 + number_length, entry->Name, 16);

	MB61_UpdateSequenceNumber();
	MB61_TX_SendMessage(MSG_ADDR_PHONE, MSG_ADDR_PC, 0x40, RequestSequenceNumber, message_length, message);

	return (GE_NONE);

}

bool		MB61_TX_SendPhonebookReadRequest(u8 entry)
{
	u8		message[7] = {0x00, 0x01, 0x1f, 0x01, 0x04, 0x86, 0x01};

	message[6] = entry;
	
	MB61_UpdateSequenceNumber();
	MB61_TX_SendMessage(MSG_ADDR_PHONE, MSG_ADDR_PC, 0x40, RequestSequenceNumber, 7, message);

	ExpectedResponse = MB61_Response_0x40_PhonebookRead;

	return (true);
}

	/* 6185 requires a different phone book request apparently */
bool		MB61_TX_SendLongPhonebookReadRequest(u8 entry)
{
	u8		message[8] = {0x00, 0x00, 0x07, 0x11, 0x00, 0x10, 0x00, 0x00};

	message[7] = entry;
	
	MB61_UpdateSequenceNumber();
	MB61_TX_SendMessage(MSG_ADDR_PHONE, MSG_ADDR_PC, 0x40, RequestSequenceNumber, 8, message);

	ExpectedResponse = MB61_Response_0x40_PhonebookRead;

	return (true);
}


void		MB61_TX_SendPhoneIDRequest(void)
{
	u8		message[5] = {0x00, 0x01, 0x00, 0x03, 0x00};
	
	MB61_UpdateSequenceNumber();
	MB61_TX_SendMessage(MSG_ADDR_PHONE, MSG_ADDR_PC, 0xd1, RequestSequenceNumber, 5, message);

	ExpectedResponse = MB61_Response_0xD2_ID;

}

void		MB61_UpdateSequenceNumber(void)
{
	RequestSequenceNumber ++;
	if (RequestSequenceNumber > 63) {
		RequestSequenceNumber = 2;
	}
}

	/* Not totally happy with this but it works for now. - HAB 20000602 */
bool		MB61_TX_SendStandardAcknowledge(u8 sequence_number)
{
	u8		out_buffer[6];
	u8		checksum;
	int		count;

	out_buffer[0] = 0x1f;
	out_buffer[1] = MSG_ADDR_PHONE;
	out_buffer[2] = MSG_ADDR_PC;
	out_buffer[3] = 0x7f;
	out_buffer[4] = sequence_number;

        /* Now calculate checksum over entire message 
           and append to message. */
    checksum = 0;
    for (count = 0; count < 5; count ++) {
        checksum ^= out_buffer[count];
    }
    out_buffer[5] = checksum;

        /* Send it out... */
    if (WRITEPHONE(PortFD, out_buffer, 6) != 6) {
        perror(_("TX_SendMessage - write:"));
        return (false);
    }

#ifdef 	DEBUG
	fprintf(stdout, "(Ack %02x) ", sequence_number);
#endif
	
    return (true);
}	
	    /* RX_State machine for receive handling.  Called once for each
       character received from the phone/phone. */
void    MB61_RX_StateMachine(char rx_byte)
{

#ifdef 	DEBUG
	fprintf(stdout, "(%d)", RX_State);
#endif

    switch (RX_State) {
    
                    /* Messages on the MBUS start with 0x1f.  We use
					   this to "synchronise" with the incoming data
                       stream. */       
        case MB61_RX_Sync:
                if (rx_byte == 0x1f) {

                    BufferCount = 0;
                    CalculatedCSum = rx_byte;
                    RX_State = MB61_RX_GetDestination;
                }
                break;
        
                    /* Next byte is the destination of the message. */
        case MB61_RX_GetDestination:
                MessageDestination = rx_byte;
                CalculatedCSum ^= rx_byte;
                RX_State = MB61_RX_GetSource;
                break;

                    /* Next byte is the source of the message. */
        case MB61_RX_GetSource:
                MessageSource = rx_byte;
                CalculatedCSum ^= rx_byte;

					/* Sanity check these.  We make sure that the Source
                       and destination are either PC/PHONE or PHONE/PC */
				if (((MessageSource == MSG_ADDR_PC) && (MessageDestination == MSG_ADDR_PHONE)) ||
				   ((MessageSource == MSG_ADDR_PHONE) && (MessageDestination == MSG_ADDR_PC))) {
                	RX_State = MB61_RX_GetCommand;
				}
				else {
					RX_State = MB61_RX_Sync;
				}
				break;

                    /* Next byte is the command type. */
        case MB61_RX_GetCommand:
                MessageCommand = rx_byte;
                CalculatedCSum ^= rx_byte;
					/* Command type 0x7f is an ack and is handled
					   differently in that it's length is known a priori */
				if (MessageCommand != 0x7f) {
                	RX_State = MB61_RX_GetLengthMSB;
                	break;
				}
				else {
					MessageLength = 0;
					RX_State = MB61_RX_GetMessage;
					break;
				}

                    /* Next is the most significant byte of message length. */
        case MB61_RX_GetLengthMSB:
                MessageLength = rx_byte * 256;
                CalculatedCSum ^= rx_byte;
                RX_State = MB61_RX_GetLengthLSB;
                break;

                    /* Next is the most significant byte of message length. */
        case MB61_RX_GetLengthLSB:
                MessageLength += rx_byte;
                CalculatedCSum ^= rx_byte;
                RX_State = MB61_RX_GetMessage;
                break;

                    /* Get each byte of the message.  We deliberately
                       get one too many bytes so we get the sequence
                       byte here as well. */
        case MB61_RX_GetMessage:
                CalculatedCSum ^= rx_byte;
                MessageBuffer[BufferCount] = rx_byte;
                BufferCount ++;

                if (BufferCount >= MB61_MAX_RECEIVE_LENGTH) {
                    RX_State = MB61_RX_Sync;        /* Should be PANIC */
                }
                    /* If this is the last byte, it's the checksum */
                if (BufferCount > MessageLength) {
					MessageSequenceNumber = rx_byte;
					RX_State = MB61_RX_GetCSum;
				}
				break;

					/* Get checksum and if valid hand over to 
					   dispatch message function */	
		case MB61_RX_GetCSum:

                MessageCSum = rx_byte;

                    /* Compare against calculated checksum. */
                if (MessageCSum == CalculatedCSum) {
                    /* Got checksum, matches calculated one so 
                       now pass to appropriate dispatch handler. */
                    RX_State = MB61_RX_DispatchMessage();
                }
                    /* Checksum didn't match so ignore. */
                else {
                    ChecksumFails ++;
                    fprintf(stderr, _("CS Fail %02x != %02x"), MessageCSum, CalculatedCSum);
                    MB61_RX_DisplayMessage();
                    fflush(stderr);
                    RX_State = MB61_RX_Sync;
                }
                    
                CalculatedCSum ^= rx_byte;
                break;

        default:
    }
}

    /* Called by initialisation code to open comm port in
       asynchronous mode. */
bool        MB61_OpenSerial(void)
{
    int                     result;
    struct sigaction        sig_io;

    /* Set up and install handler before enabling async IO on port. */

    sig_io.sa_handler = MB61_SigHandler;
    sig_io.sa_flags = 0;
    sigaction (SIGIO, &sig_io, NULL);

        /* Open device MBUS uses 9600,O,1  */
    result = device_open(PortDevice, true);

    if (!result) {
        perror(_("Couldn't open MB61 device: "));
        return (false);
    }
	fprintf(stdout, "Opened MB61 device\n");

    device_changespeed(9600);
    return (true);
}

    /* Handler called when characters received from serial port. 
       calls state machine code to process it. */
void    MB61_SigHandler(int status)
{
    unsigned char   buffer[255];
    int             count,res;

    res = device_read(buffer, 255);

    for (count = 0; count < res ; count ++) {
        MB61_RX_StateMachine(buffer[count]);

#ifdef		DEBUG
		if (isprint(buffer[count])) {
			fprintf(stdout, "<%02x%c>", buffer[count], buffer[count]);
		}
		else {
			fprintf(stdout, "<%02x >", buffer[count]);
		}
#endif

    }
	fflush(stdout);
}


	/* Prepares the message header and sends it, prepends the
       message start byte (0x01) and other values according
       the value specified when called.  Calculates checksum
       and then sends the lot down the pipe... */
int     MB61_TX_SendMessage(u8 destination, u8 source, u8 command, u8 sequence_byte, int message_length, u8 *buffer) 
{
    u8                      out_buffer[MB61_MAX_TRANSMIT_LENGTH + 7];
    int                     count;
    unsigned char           checksum;

        /* Check message isn't too long, once the necessary
           header and trailer bytes are included. */
    if ((message_length + 7) > MB61_MAX_TRANSMIT_LENGTH) {
        fprintf(stderr, _("TX_SendMessage - message too long!\n"));

        return (false);
    }

    //RX_State = MB61_RX_Sync;  Hack.

        /* Now construct the message header. */
    out_buffer[0] = 0x1f;   /* Start of message indicator */
    out_buffer[1] = destination;
    out_buffer[2] = source;
    out_buffer[3] = command;
    out_buffer[4] = message_length >> 8;
	out_buffer[5] = message_length & 0xff;

        /* Copy in data if any. */  
    if (message_length != 0) {
        memcpy(out_buffer + 6, buffer, message_length);
    }
	    /* Copy in sequence number */
    out_buffer[message_length + 6] = sequence_byte;

        /* Now calculate checksum over entire message 
           and append to message. */
    checksum = 0;
    for (count = 0; count < message_length + 7; count ++) {
        checksum ^= out_buffer[count];
    }
    out_buffer[message_length + 7] = checksum;

        /* Send it out... */
    if (WRITEPHONE(PortFD, out_buffer, message_length + 8) != message_length + 8) {
        perror(_("TX_SendMessage - write:"));
        return (false);
    }

#ifdef DEBUG
	for (count = 0; count < message_length + 8; count++) {
		if (isprint(out_buffer[count])) {
			fprintf(stdout, "{%02x%c}", out_buffer[count], out_buffer[count]);
		}
		else {
			fprintf(stdout, "{%02x }", out_buffer[count]);
		}
		if (((count + 1) % 16) == 0) {
			fprintf(stdout, "\n");
		}
	}	
	fflush(stdout);
#endif
	
    return (true);
}



#endif
