/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file contains the main code for 3810 support.
	
  Last modification: Fri May 19 15:31:26 EST 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#define     __fbus_3810_c   /* "Turn on" prototypes in fbus-3810.h */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#ifdef WIN32

  #include <windows.h>
  #include "win32/winserial.h"

  #undef IN
  #undef OUT

  #define WRITEPHONE(a, b, c) WriteCommBlock(b, c)
  #define sleep(x) Sleep((x) * 1000)
  #define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
  extern HANDLE hPhone;

#else

  #define WRITEPHONE(a, b, c) device_write(b, c)
  #include    <unistd.h>
  #include    <fcntl.h>
  #include    <ctype.h>
  #include    <signal.h>
  #include    <sys/types.h>
  #include    <sys/time.h>
  #include    <sys/ioctl.h>
  #include    <pthread.h>
  #include    <errno.h>
  #include    "device.h"

#endif /* WIN32 */

#include    "misc.h"
#include    "gsm-common.h"
#include    "rlp-common.h"
#include    "fbus-3810.h"

    /* Global variables used by code in gsm-api.c to expose the
       functions supported by this model of phone.  */
bool                    FB38_LinkOK;

#ifdef WIN32

  void FB38_InitializeLink();

#endif

GSM_Functions           FB38_Functions = {
        FB38_Initialise,
        FB38_Terminate,
        FB38_GetMemoryLocation,
        FB38_WritePhonebookLocation,
        FB38_GetSpeedDial,
        FB38_SetSpeedDial,
        FB38_GetMemoryStatus,
        FB38_GetSMSStatus,
        FB38_GetSMSCenter,
        FB38_SetSMSCenter,
        FB38_GetSMSMessage,
        FB38_DeleteSMSMessage,
        FB38_SendSMSMessage,
        FB38_GetRFLevel,
        FB38_GetBatteryLevel,
        FB38_GetPowerSource,
        FB38_GetDisplayStatus,
        FB38_EnterSecurityCode,
        FB38_GetSecurityCodeStatus,
        FB38_GetIMEI,
        FB38_GetRevision,
        FB38_GetModel,
        FB38_GetDateTime,
        FB38_SetDateTime,
        FB38_GetAlarm,
        FB38_SetAlarm,
        FB38_DialVoice,
        FB38_DialData,
        FB38_GetIncomingCallNr,
        FB38_GetNetworkInfo,
        FB38_GetCalendarNote,
        FB38_WriteCalendarNote,
        FB38_DeleteCalendarNote,
        FB38_Netmonitor,
        FB38_SendDTMF,
        FB38_GetBitmap,
        FB38_SetBitmap,
        FB38_Reset,
        FB38_GetProfile,
        FB38_SetProfile,
		FB38_SendRLPFrame,
        FB38_CancelCall
};

GSM_Information         FB38_Information = {
        "3110|3810|8110|8110i", /* Models */
        4,                      /* Max RF Level */
        0,                      /* Min RF Level */
        GRF_Arbitrary,          /* RF level units */
        4,                      /* Max Battery Level */
        0,                      /* Min Battery Level */
        GBU_Arbitrary,          /* Battery level units */
        GDT_None,               /* No date/time support */
        GDT_None,               /* No alarm support */
        0                       /* Max alarms = 0 */
};



    /* Local variables */
enum FB38_RX_States     RX_State;
int                     MessageLength;
u8                      MessageBuffer[FB38_MAX_RECEIVE_LENGTH];
unsigned char           MessageCSum;
int                     BufferCount;
unsigned char           CalculatedCSum;
int                     PortFD;
char                    PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
u8                      RequestSequenceNumber;

    /* Local variables */
enum FB38_RX_States     RX_State;
int                     MessageLength;
u8                      MessageBuffer[FB38_MAX_RECEIVE_LENGTH];
unsigned char           MessageCSum;
int                     BufferCount;
unsigned char           CalculatedCSum;
int                     PortFD;
char                    PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
u8                      RequestSequenceNumber;
#ifndef WIN32
pthread_t               Thread;
#endif /* WIN32 */
bool                    RequestTerminate;
bool                    DisableKeepalive;
float                   CurrentRFLevel;
float                   CurrentBatteryLevel;
int                     ExploreMessage; /* for debugging/investigation only */
int                     InitLength;
long                    ChecksumFails;  /* For diagnostics */

    /* Pointer to callback function in user code to be called when
       RLP frames are received. */
void                    (*RLP_RXCallback)(RLP_F96Frame *frame);

#ifdef WIN32
/* called repeatedly from a separate thread */
void FB38_KeepAliveProc()
{
    if (!DisableKeepalive)
  	FB38_TX_Send0x4aMessage();	/* send status request */
    Sleep(2000);
}
#endif

    /* These three are the information returned by AT+CGSN, AT+CGMR and
       AT+CGMM commands respectively. */
char                    IMEI[FB38_MAX_IMEI_LENGTH];
bool                    IMEIValid;
char                    Revision[FB38_MAX_REVISION_LENGTH];
bool                    RevisionValid;
char                    Model[FB38_MAX_MODEL_LENGTH];
bool                    ModelValid;

    /* We have differing lengths for internal memory vs SIM memory.
       internal memory is index 0, SIM index 1. */
int                     MaxPhonebookNumberLength[2];
int                     MaxPhonebookNameLength[2];

    /* Local variables used by get/set phonebook entry code. 
       ...Buffer is used as a source or destination for phonebook
       data, ...Error is set to GE_None by calling function, set to 
       GE_COMPLETE or an error code by handler routines as appropriate. */

GSM_PhonebookEntry      *CurrentPhonebookEntry;
GSM_Error               CurrentPhonebookError;

    /* Local variables used by send/retrieve SMS message code. */
int                     CurrentSMSMessageBodyLength;
GSM_SMSMessage          *CurrentSMSMessage;
GSM_Error               CurrentSMSMessageError;
u8                      CurrentSMSSendResponse[2];  /* Set by 0x28/29 messages */
bool                    SMSBlockAckReceived;        /* Set when block ack'd */
bool                    SMSHeaderAckReceived;       /* Set when header ack'd */

GSM_MessageCenter       *CurrentMessageCenter;
GSM_Error               CurrentMessageCenterError;

GSM_SMSStatus			*CurrentSMSStatus;
GSM_Error				CurrentSMSStatusError;


	/* Error variable for use when sending DTMF */
GSM_Error				CurrentDTMFError;

    /* The following functions are made visible to gsm-api.c and friends. */

    /* Initialise variables and state machine. */
GSM_Error   FB38_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame))
{

    /* ConnectionType is ignored in 3810 code. */

    int     rtn;

    RequestTerminate = false;
    FB38_LinkOK = false;
    DisableKeepalive = false;
    CurrentRFLevel = -1;
    CurrentBatteryLevel = -1;
    IMEIValid = false;
    RevisionValid = false;
    ModelValid = false;
    ExploreMessage = -1;
    ChecksumFails = 0;
    RLP_RXCallback = rlp_callback;
    
    strncpy (PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

    InitLength = atoi(initlength);

    if ((strcmp(initlength, "default") == 0) || (InitLength == 0)) {
        InitLength = 100;   /* This is the usual value, lower may work. */
    }

        /* Create and start thread, */

#ifdef WIN32
  DisableKeepalive = true;
  rtn = ! OpenConnection(PortDevice, FB38_RX_StateMachine, FB38_KeepAliveProc);
  if (rtn == 0) {
      FB38_InitializeLink(); /* makes more sense to do this in 'this' thread */
      DisableKeepalive = false;
  }
#else /* !WIN32 */
    rtn = pthread_create(&Thread, NULL, (void *) FB38_ThreadLoop, (void *)NULL);
#endif /* WIN32 */

    if (rtn != 0)
    {
        return (GE_INTERNALERROR);
    }

    return (GE_NONE);

}

    /* Applications should call FB38_Terminate to shut down
       the FB38 thread and close the serial port. */
void        FB38_Terminate(void)
{
        /* Request termination of thread */
    RequestTerminate = true;

        /* Now wait for thread to terminate. */

#ifndef WIN32
    pthread_join(Thread, NULL);

        /* Close serial port. */
    device_close();

        /* If in monitor mode, show number of checksum failures. */ 
#ifdef DEBUG
    fprintf (stderr, "Checksum failures: %ld\n", ChecksumFails);
#endif

#else /* WIN32 */
    CloseConnection();
#endif
}

    /* Routine to get specifed phone book location.  Designed to 
       be called by application.  Will block until location is
       retrieved or a timeout/error occurs. */
GSM_Error   FB38_GetMemoryLocation(GSM_PhonebookEntry *entry)
{
    int     memory_area;
    int     timeout;

        /* State machine code writes data to these variables when
           it comes in. */
    CurrentPhonebookEntry = entry;
    CurrentPhonebookError = GE_BUSY;

    if (entry->MemoryType == GMT_ME) {
        memory_area = 1;	/* 3 in 8110, 1 is GMT_CB */
    }
    else {
        if (entry->MemoryType == GMT_SM) {
            memory_area = 2;
        }
        else {
            return (GE_INVALIDMEMORYTYPE);
        }
    }

    timeout = 20;   /* 2 seconds for command to complete */

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send request */
    FB38_TX_Send0x43_RequestMemoryLocation(memory_area, entry->Location);
    
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
GSM_Error   FB38_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{

    int     memory_area;
    int     timeout;
    int     index;

        /* Make sure neither name or number is too long.  We assume (as
           has been reported that memory_area 1 is internal, 2 is the SIM */
    if (entry->MemoryType == GMT_ME) {
        memory_area = 1;	/* 3 in 8110, 1 is GMT_CB */
    }
    else {
        if (entry->MemoryType == GMT_SM) {
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

    timeout = 40;   /* 4 seconds - can be quite slow ! */

    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send message. */
    if (FB38_TX_Send0x42_WriteMemoryLocation(memory_area, entry->Location, entry->Name, entry->Number) != 0) {
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

GSM_Error   FB38_GetSpeedDial(GSM_SpeedDial *entry)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SetSpeedDial(GSM_SpeedDial *entry)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetSMSMessage(GSM_SMSMessage *message)
{
    int     timeout;
    int     memory_area;

    if (message->MemoryType == GMT_ME) {
        memory_area = 1;	/* 3 in 8110, 1 is GMT_CB */
    }
    else {
        if (message->MemoryType == GMT_SM) {
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

    timeout = 50;   /* 5 seconds for command to complete */

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send request */
    FB38_TX_Send0x25_RequestSMSMemoryLocation(memory_area, message->Location);

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

GSM_Error   FB38_DeleteSMSMessage(GSM_SMSMessage *message)
{
    int     timeout;
    int     memory_area;

    if (message->MemoryType == GMT_ME) {
        memory_area = 1;	/* 3 in 8110, 1 is GMT_CB */
    }
    else {
        if (message->MemoryType == GMT_SM) {
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

    timeout = 50;   /* 5 seconds for command to complete */

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send request */
    FB38_TX_Send0x26_DeleteSMSMemoryLocation(memory_area, message->Location);

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

GSM_Error   FB38_SendSMSMessage(GSM_SMSMessage *SMS, int data_size)
{
    int     timeout;
    int     text_offset;
    int     text_length;
    int     text_remaining;
    u8      block_count;
    u8      block_length;
    int     retry_count;
    GSM_Error error;

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Get SMSC number */
    if (SMS->MessageCenter.No) {
        error = FB38_GetSMSCenter(&SMS->MessageCenter);
        if (error != GE_NONE)
            return error;
    }
    
    fprintf(stdout, _("Sending SMS to %s via message center %s\n"), SMS->Destination, SMS->MessageCenter.Number);

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
			/* Use FB38_TX_Send0x24_StoreSMSHeader() if you want save message to phone */
        FB38_TX_Send0x23_SendSMSHeader(SMS->MessageCenter.Number, SMS->Destination, text_length);

        timeout = 20;   /* 2 seconds for command to complete */

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
    
            timeout = 20;   /* 2 seconds. */
    
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

        timeout = 1200; /* 120 seconds network wait. */

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
        fprintf(stderr, _("SMS Send attempt failed, trying again (%d of %d)\n"),            ((FB38_SMS_SEND_RETRY_COUNT - retry_count) + 1), FB38_SMS_SEND_RETRY_COUNT);
            /* Got a retry response so try again! */
        retry_count --;

            /* After an empirically determined pause... */
            usleep(500000); /* 0.5 seconds. */
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
GSM_Error   FB38_GetRFLevel(GSM_RFUnits *units, float *level)
{
        /* Map from values returned in status packet to the
           the values returned by the AT+CSQ command */
    float   csq_map[5] = {0, 8, 16, 24, 31};
    int     rf_level;

        /* Take a copy in case it changes midway through... */
    rf_level = CurrentRFLevel;

    if (rf_level == -1) {
        return (GE_INTERNALERROR);
    }

        /* Arbitrary units. */
    if (*units == GRF_Arbitrary) {
        *level = rf_level;
        return (GE_NONE);
    }

        /* CSQ units. */
    if (*units == GRF_CSQ) {
        if (rf_level <=4) {
            *level = csq_map[rf_level];
        }
        else {
            *level = 99;    /* Unknown/undefined */
        }
        return (GE_NONE);
    }

        /* Unit type is one we don't handle so return error */
    return (GE_INTERNALERROR);
}

    /* FB38_GetBatteryLevel
       FIXME (see above...) */
GSM_Error   FB38_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
    float   batt_level; 

    batt_level = CurrentBatteryLevel;
    
    if (batt_level == -1) {
        return (GE_INTERNALERROR);
    }

        /* Only units we handle at present are GBU_Arbitrary */
    if (*units == GBU_Arbitrary) {
        *level = batt_level;
        return (GE_NONE);
    }

    return (GE_INTERNALERROR);
}

GSM_Error   FB38_GetIMEI(char *imei)
{
    int     timeout;

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send request if IMEI not valid */
    if (IMEIValid != true) {
        FB38_TX_Send0x4c_RequestIMEIRevisionModelData();
    }

    timeout = 50;   /* 5 seconds */

        /* Wait for timeout or other error. */
    while (timeout != 0 && IMEIValid != true) {

        timeout --;
        if (timeout == 0) {
            return (GE_TIMEOUT);
        }
        usleep (100000);
    }

    if (IMEIValid) {
        strncpy (imei, IMEI, FB38_MAX_IMEI_LENGTH);
        return (GE_NONE);
    }
    else {
        return (GE_INTERNALERROR);
    }
}

GSM_Error   FB38_GetRevision(char *revision)
{
    int     timeout;

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send request if Revision not valid */
    if (RevisionValid != true) {
        FB38_TX_Send0x4c_RequestIMEIRevisionModelData();
    }

    timeout = 50;   /* 5 seconds */

        /* Wait for timeout or other error. */
    while (timeout != 0 && RevisionValid != true) {

        timeout --;
        if (timeout == 0) {
            return (GE_TIMEOUT);
        }
        usleep (100000);
    }

    if (RevisionValid) {
        strncpy (revision, Revision, FB38_MAX_REVISION_LENGTH);
        return (GE_NONE);
    }
    else {
        return (GE_INTERNALERROR);
    }
}

GSM_Error   FB38_GetModel(char *model)
{
    int     timeout;

        /* Return if no link has been established. */
    if (!FB38_LinkOK) {
        return GE_NOLINK;
    }

        /* Send request if Model not valid */
    if (ModelValid != true) {
        FB38_TX_Send0x4c_RequestIMEIRevisionModelData();
    }

    timeout = 50;   /* 5 seconds */

        /* Wait for timeout or other error. */
    while (timeout != 0 && ModelValid != true) {

        timeout --;
        if (timeout == 0) {
            return (GE_TIMEOUT);
        }
        usleep (100000);
    }

    if (ModelValid) {
        strncpy (model, Model, FB38_MAX_MODEL_LENGTH);
        return (GE_NONE);
    }
    else {
        return (GE_INTERNALERROR);
    }
}

/* This function sends to the mobile phone a request for the SMS Center */

GSM_Error   FB38_GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
    int 			timeout = 10;
	GSM_SMSStatus 	tmp;
 
	CurrentSMSStatus = &tmp;
    CurrentMessageCenter = MessageCenter;
    CurrentMessageCenterError = GE_BUSY;

    FB38_TX_Send0x3fMessage();

        /* Wait for timeout or other error. */
    while (timeout != 0 && CurrentMessageCenterError == GE_BUSY ) {

        if (--timeout == 0) {
            return (GE_TIMEOUT);
        }
        usleep (100000);
    }

        /* If successfull, CurrentMessageCenterError will be set
           to GE_NONE, otherwise it will be set to the appropriate
           error code. */
    return (CurrentMessageCenterError);
}

GSM_Error   FB38_DialVoice(char *Number)
{
    return(FB38_TX_SendDialCommand(0x05, Number));
}

GSM_Error   FB38_DialData(char *Number, char type, void (* callpassup)(char c))
{
    return(FB38_TX_SendDialCommand(0x01, Number));
}

bool		FB38_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	return (FB38_TX_SendRLPFrame(frame, out_dtx));
}

GSM_Error   FB38_SetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	u8 msg[64];
	int timeout;
	int len = 0;

	CurrentMessageCenter = MessageCenter;
	CurrentMessageCenterError = GE_BUSY;

	/* Mask defines which fields are set.
	   Bit	Field
	   7	??
	   6	SRR	Status-Report-Request
	   5	RP	Reply-Path
	   4	SCA	Service-Centre-Address
	   3	UA	Unknown-Address
	   2	VP	Validity-Period
	   1	U	Unknown
	   0	PID	Protocol-Identifier
	*/
	msg[len++] = 0x7f;			/* Mask */
	msg[len++] = MessageCenter->Format;	/* PID */
	msg[len++] = 0x00;			/* U */
	msg[len++] = MessageCenter->Validity;	/* VP */
	msg[len++] = 0x01;			/* RP (1 = no, 2 = yes) */
	msg[len++] = 0x02;			/* SRR (1 = no, 2 = yes) */

	msg[len++] = 0x00;			/* Length of UA */
	if (msg[len - 1] != 0) {		/* UA */
		strcpy(msg + len, MessageCenter->Number);
		len += msg[len - 1];
	}

	msg[len++] = strlen(MessageCenter->Number);	/* SCA */
	if (msg[len - 1] != 0) {
		strcpy(msg + len, MessageCenter->Number);
		len += msg[len - 1];
	}

	/* Set address type of UnknownNumber to 0, so phone automatically
	   changes it to correct value */
	msg[len++] = 0x00;			/* TA */

	FB38_TX_UpdateSequenceNumber();
	if (FB38_TX_SendMessage(0x3c, len, RequestSequenceNumber, msg) != true) {
		return (GE_INTERNALERROR);
	}

	timeout = 10;
	while (timeout != 0 && CurrentMessageCenterError == GE_BUSY) {
		timeout--;
		if (timeout == 0)
			return (GE_TIMEOUT);
		usleep(100000);
	}

	return (CurrentMessageCenterError);
}

GSM_Error   FB38_GetSMSStatus(GSM_SMSStatus *Status)
{
	GSM_MessageCenter tmp;
	int timeout = 10;

	CurrentMessageCenter = &tmp;
	CurrentMessageCenterError = GE_BUSY;
	CurrentSMSStatus = Status;

	FB38_TX_Send0x3fMessage();

	/* Wait for timeout or other error. */
	while (timeout != 0 && CurrentMessageCenterError == GE_BUSY ) {
		if (--timeout == 0) {
			return (GE_TIMEOUT);
		}
		usleep (100000);
	}

	return (CurrentMessageCenterError);
}

    /* Our "Not implemented" functions */
GSM_Error   FB38_GetMemoryStatus(GSM_MemoryStatus *Status)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetPowerSource(GSM_PowerSource *source)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetDisplayStatus(int *Status)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_EnterSecurityCode(GSM_SecurityCode SecurityCode)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetSecurityCodeStatus(int *Status)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetDateTime(GSM_DateTime *date_time)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SetDateTime(GSM_DateTime *date_time)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetIncomingCallNr(char *Number)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_CancelCall(void)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SendBitmap (char *NetworkCode, int width, int height, unsigned char *bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetNetworkInfo (GSM_NetworkInfo *NetworkInfo)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetCalendarNote (GSM_CalendarNote *CalendarNote)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_WriteCalendarNote (GSM_CalendarNote *CalendarNote)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_DeleteCalendarNote (GSM_CalendarNote *CalendarNote)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_Netmonitor (unsigned char mode, char *Screen)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SendDTMF (char *String)
{
	u8 message[64];
	int timeout;

	message[0] = strlen(String);
	strncpy(message + 1, String, message[0]);

	FB38_TX_UpdateSequenceNumber();
	if (FB38_TX_SendMessage(message[0] + 1, 0x20, RequestSequenceNumber, message) != true) {
		return (GE_INTERNALERROR);
	}

	CurrentDTMFError = GE_BUSY;

	timeout = 20;
	/* This function have nothing to do with MessageCenter... */
	while (timeout != 0 && CurrentDTMFError == GE_BUSY) {
		timeout--;
		if (timeout == 0)
			return (GE_TIMEOUT);
		usleep(100000);
	}

	return CurrentDTMFError;
}

GSM_Error   FB38_GetBitmap (GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SetBitmap (GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_Reset (unsigned char type)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_GetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error   FB38_SetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}

#ifndef WIN32
    /* Everything from here down is internal to 3810 code. */


    /* This is the main loop for the FB38 functions.  When FB38_Initialise
       is called a thread is created to run this loop.  This loop is
       exited when the application calls the FB38_Terminate function. */
void    FB38_ThreadLoop(void)
{
    unsigned char       init_char[1] = {0x55};
    int                 count, idle_timer;

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
    for (count = 0; count < InitLength; count ++) {
        usleep(1000);
        WRITEPHONE(PortFD, init_char, 1);
    }

        /* Now send the 0x15 message, the exact purpose is not understood
           but if not sent, link doesn't work. */
    FB38_TX_Send0x15Message(0x11);

        /* We've now finished initialising things so sit in the loop
           until told to do otherwise.  Loop doesn't do much other
           than send periodic keepalive messages to phone.  This
           loop will become more involved once we start doing 
           fax/data calls. */

    idle_timer = 0;

    while (!RequestTerminate) {
        if (idle_timer == 0) {
                /* Dont send keepalive packets when doing other transactions. */
            if (!DisableKeepalive) {
                FB38_TX_Send0x4aMessage();
            }
            idle_timer = 20;
        }
        else {
            idle_timer --;
        }

        usleep(100000);     /* Avoid becoming a "busy" loop. */
    }

}


    /* Called by initialisation code to open comm port in
       asynchronous mode. */
bool        FB38_OpenSerial(void)
{
    int                     result;
    struct sigaction        sig_io;

    /* Set up and install handler before enabling async IO on port. */

    sig_io.sa_handler = FB38_SigHandler;
    sig_io.sa_flags = 0;
    sigaction (SIGIO, &sig_io, NULL);

        /* Open device. */
    result = device_open(PortDevice, false);

    if (!result) {
        perror(_("Couldn't open FB38 device: "));
        return (false);
    }

    device_changespeed(115200);

    return (true);
}

    /* Handler called when characters received from serial port. 
       calls state machine code to process it. */
void    FB38_SigHandler(int status)
{
    unsigned char   buffer[255];
    int             count,res;

    res = device_read(buffer, 255);

    for (count = 0; count < res ; count ++) {
        FB38_RX_StateMachine(buffer[count]);
    }
}

#endif /* !WIN32 */

#ifdef WIN32

void FB38_InitializeLink()
{
    unsigned char       init_char[1] = {0x55};
    int                 count, idle_timer;

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

        /* Initialise sequence number used when sending messages
           to phone. */
    RequestSequenceNumber = 0x10;

        /* Send init string to phone, this is a bunch of 0x55
           characters.  Timing is empirical. */
    for (count = 0; count < InitLength; count ++) {
        usleep(1000);
        WRITEPHONE(PortFD, init_char, 1);
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

}

#endif /* WIN32 */

    /* RX_State machine for receive handling.  Called once for each
       character received from the phone/phone. */
void    FB38_RX_StateMachine(char rx_byte)
{
    static u8   current_message_type;

    switch (RX_State) {
    
                    /* Phone is currently off.  Wait for 0x55 before
                       restarting */
        case FB38_RX_Off:
                if (rx_byte != 0x55) {
                    break;
				}

                /* Seen 0x55, restart at 0x04 */
#ifdef DEBUG
                    fprintf(stdout, _("restarting.\n"));
#endif

                RX_State = FB38_RX_Sync;

                /*FALLTHROUGH*/

                    /* Messages from the phone start with an 0x04 during
                       "normal" operation, 0x03 when in data/fax mode.  We
                       use this to "synchronise" with the incoming data
                       stream. */       
        case FB38_RX_Sync:
                if (rx_byte == 0x04 || rx_byte == 0x03) {
                    current_message_type = rx_byte;
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
                    RX_State = FB38_RX_Sync;        /* Should be PANIC */
                }
                    /* If this is the last byte, it's the checksum */
                if (BufferCount > MessageLength) {

                    MessageCSum = rx_byte;
                        /* Compare against calculated checksum. */
                    if (MessageCSum == CalculatedCSum) {
                        /* Got checksum, matches calculated one so 
                           now pass to appropriate dispatch handler.
                           on the basis of current_meesage_type  */
                        if (current_message_type == 0x04) {
                            RX_State = FB38_RX_DispatchMessage();
                        }
                        else {
                            if (current_message_type == 0x03) {
                                RX_State = FB38_RX_HandleRLPMessage();
                            }
                                /* Hmm, unknown message type! */
                            else {
                                fprintf(stderr, _("MT Fail %02x"), current_message_type);
                            }
                        }
                    }
                        /* Checksum didn't match so ignore. */
                    else {
                        ChecksumFails ++;
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

enum FB38_RX_States     FB38_RX_HandleRLPMessage(void)
{
    RLP_F96Frame    frame;
    int             count;
    
    if (RLP_RXCallback == NULL) {
        return (FB38_RX_Sync);
    }
    
    frame.Header[0] = MessageBuffer[2];
    frame.Header[1] = MessageBuffer[3];
    
    for (count = 0; count < 25; count ++) {
        frame.Data[count] = MessageBuffer[4 + count];
    }
    
    frame.FCS[0] = MessageBuffer[29];
    frame.FCS[1] = MessageBuffer[30];
    frame.FCS[2] = MessageBuffer[31];
    
    RLP_RXCallback(&frame);

    return (FB38_RX_Sync);

}

    /* FB38_RX_DispatchMessage
       Once we've received a message from the phone, the command/message
       type byte is used to call an appropriate handler routine or
       simply acknowledge the message as required.  The rather verbose
       names given to the handler routines reflect that the names
       of the purpose of the handler are best guesses only hence both
       the byte (0x0b) and the purpose (IncomingCall) are given. */
enum FB38_RX_States     FB38_RX_DispatchMessage(void)
{
    /* Uncomment this if you want all messages in raw form. */
    /*FB38_RX_DisplayMessage(); */

        /* If the explore message function is in use, ensures
           the message we generate isn't acknowleged. */
    if (MessageBuffer[0] == ExploreMessage) {
        return FB38_RX_Sync;
    }

        /* Switch on the basis of the message type byte */
    switch (MessageBuffer[0]) {

            /* We send 0x0a messages to make a call so don't ack. */
        case 0x0a:      break;

            /* 0x0b messages are sent by phone when an incoming call occurs,
               this message must be acknowledged. */
        case 0x0b:  FB38_RX_Handle0x0b_IncomingCall();
                    break;

            /* We send 0x0c message to answer to incoming call so don't ack */
        case 0x0c:  break;

        case 0x0d:  FB38_RX_Handle0x0d_IncomingCallAnswered();
                    break;

            /* We send 0x0f message to hang up so don't ack */
        case 0x0f:  break;

            /* Fairly self explanatory these two, though the outgoing 
               call message has three (unexplained) data bytes. */
        case 0x0e:  FB38_RX_Handle0x0e_CallEstablished();
                    break;
 

            /* 0x10 messages are sent by the phone when an outgoing
                call terminates. */
        case 0x10:  FB38_RX_Handle0x10_EndOfOutgoingCall();
                    break;

            /* 0x11 messages are sent by the phone when an incoming call
               terminates.  There is some other data in the message, 
               purpose as yet undertermined. */
        case 0x11:  FB38_RX_Handle0x11_EndOfIncomingCall();
                    break;

            /* 0x12 messages are sent after the 0x10 message at the 
               end of an outgoing call.  Significance of two messages
               versus the one at the end of an incoming call  is as 
               yet undertermined. */
        case 0x12:  FB38_RX_Handle0x12_EndOfOutgoingCall();
                    break;

            /* 0x13 messages are sent after the phone restarts. 
               Re-initialise */
        case 0x13:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
					RequestSequenceNumber = 0x10;
					FB38_TX_Send0x15Message(0x11);
					break;
            
            /* 0x15 messages are sent by the phone in response to the
               init sequence sent so we don't acknowledge them! */
        case 0x15:  DisableKeepalive = false;

#ifdef DEBUG
					fprintf(stdout, _("0x15 Registration Response 0x%02x\n"), MessageBuffer[1]);
#endif
                    break;

            /* 0x16 messages are sent by the phone during initialisation,
               to response to the 0x15 message.
               Sequence bytes have been observed to change with 
               differing software versions.
               V06.61 (19/08/97) sends 0x10 0x02, V07.02 (17/03/98) sends 
               0x30 0x02.
               The actual data byte is 0x02 when SIM memory is available,
               and 0x01 when not, e.g. when SIM card isn't inserted to phone or
               when it is waiting for PIN */
        case 0x16:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);

#ifdef DEBUG
                    fprintf(stdout, _("0x16 Registration Response 0x%02x\n"), MessageBuffer[1]);
                    fprintf(stdout, _("SIM access: %s.\n"), (MessageBuffer[2] == 0x02 ? "Yes" : "No") );
#endif 
                    break;

        case 0x17:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);

#ifdef DEBUG
                    fprintf(stderr, "0x17 Registration Response: Failure!\n");
#endif
                    break;

            /* We send 0x20 message to phone to send DTFM, so don't ack */
        case 0x20:      break;
						
			/* 0x21/0x20 are responses to DTMF commands. */
        case 0x21:      CurrentDTMFError = GE_NONE;
                        break;
        case 0x22:      CurrentDTMFError = GE_UNKNOWN;
                        break;

            /* We send 0x23 messages to phone as a header for outgoing SMS
               messages.  So we don't acknowledge it. */
        case 0x23:  SMSHeaderAckReceived = true;
                    break;

            /* We send 0x24 messages to phone as a header for storing SMS
               messages in memory. So we don't acknowledge it. :) */
        case 0x24:  SMSHeaderAckReceived = true;
                    break;

            /* We send 0x25 messages to phone to request an SMS message
               be dumped.  Thus we don't acknowledge it. */
        case 0x25:  break;


            /* We send 0x26 messages to phone to delete an SMS message
               so it's not acknowledged. */
        case 0x26:  break;


            /* 0x27 messages are a little different in that both ends of
               the link send them.  The PC sends them with SMS message
               text as does the phone. */
        case 0x27:  FB38_RX_Handle0x27_SMSMessageText();
                    break;

            /* 0x28 messages are sent by the phone to acknowledge succesfull
               sending of an SMS message.  The byte returned is a receipt
               number of some form, not sure if it's from the network, sending
               sending of an SMS message.  The byte returned is the TP-MR
               (TP-Message-Reference) from sending phone (Also sent to network).
               TP-MR is send from phone within 0x32 message. TP-MR is increased
               by phone after each sent SMS */
        case 0x28:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentSMSSendResponse[0] = MessageBuffer[2];
                    CurrentSMSSendResponse[1] = 0;
                    CurrentSMSMessageError = GE_SMSSENDOK;
                    break;

            /* 0x29 messages are sent by the phone to indicate an error in
               sending an SMS message.  Observed values are 0x65 0x15 when
               the phone originated SMS was disabled by the network for
               the particular phone.  0x65 0x26 was observed too, whereupon
               the message was retried. */
        case 0x29:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentSMSSendResponse[0] = MessageBuffer[2];
                    CurrentSMSSendResponse[1] = MessageBuffer[3];
                    CurrentSMSMessageError = GE_SMSSENDFAILED;
                    break;

            /* Responses to StoreSMSMessage */
        case 0x2a:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentSMSSendResponse[0] = MessageBuffer[2];
                    CurrentSMSSendResponse[1] = 0x00;
                    CurrentSMSMessageError = GE_SMSSENDOK;
#ifdef DEBUG
                    fprintf(stdout, _("SMS Stored into location 0x%02x\n"),
                            CurrentSMSSendResponse[0]);
                    fflush(stdout);
#endif
                    break;

        case 0x2b:  CurrentSMSSendResponse[0] = MessageBuffer[2];
                    CurrentSMSSendResponse[1] = 0x00;
                    CurrentSMSMessageError = GE_SMSSENDFAILED;
#ifdef DEBUG
                    fprintf(stdout, _("SMS Store failed: 0x%02x\n"),
                            CurrentSMSSendResponse[0]);
                    fflush(stdout);
#endif
                    break;

            /* 0x2c messages are generated by the phone when we request
               an SMS message with an 0x25 message.  Fields seem to be
               largely the same as the 0x30 notification.  Immediately
               after the 0x2c nessage, the phone sends 0x27 message(s) */
        case 0x2c:  FB38_RX_Handle0x2c_SMSHeader();
                    break;

            /* 0x2d messages are generated when an SMS message is requested
               that does not exist or is empty. */
        case 0x2d:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    if (MessageBuffer[2] == 0x74) {
                        CurrentSMSMessageError = GE_INVALIDSMSLOCATION;
                    }
                    else {
                        CurrentSMSMessageError = GE_EMPTYSMSLOCATION;
                    }
    
                    break;

            /* 0x2e messages are generated when an SMS message is deleted
               successfully. */
        case 0x2e:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentSMSMessageError = GE_NONE;
                    break;

            /* 0x2f messages are generated when an SMS message is deleted
               that does not exist.  Unlike responses to a getsms message
               no error is returned when the entry is already empty */
        case 0x2f:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
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
        case 0x30:  FB38_RX_Handle0x30_IncomingSMSNotification();
                    break;

            /* Delivery report from network */
        case 0x32:  FB38_RX_Handle0x32_SMSDelivered();
                    break;


            /* We send 0x3c to write message center data, so don't ack */
        case 0x3c:  break;

            /* Response to 0x3c: OK */
        case 0x3d:  CurrentMessageCenterError = GE_NONE;
                    break;

            /* Response to 0x3c: Error */
        case 0x3e:  CurrentMessageCenterError = GE_INTERNALERROR;
                    break;

            /* We send an 0x3f message to the phone to request a different
               type of status dump - this one seemingly concerned with 
               SMS message center details.  Phone responds with an ack to
               our 0x3f request then sends an 0x41 message that has the
               actual data in it. */
        case 0x3f:  break;  /* Don't send ack. */

            /* 0x40 Messages are sent to response to an 0x3f request.
               e.g. when phone is waiting for PIN */
        case 0x40:  CurrentMessageCenterError = GE_UNKNOWN;
                    break;

            /* 0x41 Messages are sent in response to an 0x3f request. */
        case 0x41:  FB38_RX_Handle0x41_SMSMessageCenterData();
                    break;

            /* We send 0x42 messages to write a SIM location. */
        case 0x42:  break;

            /* 0x43 is a message we send to request data from a memory
               location. The phone returns and acknowledge for the 
               0x43 command then sends an 0x46 command with the actual data. */
        case 0x43:  break;

            /* 0x44 is sent by phone to acknowledge that phonebook location 
               was written correctly. */
        case 0x44:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentPhonebookError = GE_NONE;
                    break;

            /* 0x45 is sent by phone if a write to a phonebook location
               failed. */
        case 0x45:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentPhonebookError = GE_INVALIDPHBOOKLOCATION;
                    break;
    
            /* 0x46 is sent after an 0x43 response with the requested data. */
        case 0x46:  FB38_RX_Handle0x46_MemoryLocationData();
                    break;

            /* 0x47 is sent if the location requested in an 0x43 message is
               invalid or unavailable (such as immediately after the phone
               is switched on. */
        case 0x47:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
                    CurrentPhonebookError = GE_INVALIDPHBOOKLOCATION;
                    break;

            /* 0x48 is sent during power-on of the phone, after the 0x13
               message is received and the PIN (if any) has been entered
               correctly. */
        case 0x48:  FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);

#ifdef DEBUG
                    fprintf(stdout, _("PIN [possibly] entered.\n"));
#endif
                    break;

            /* 0x49 is sent when the phone is switched off.  Disable
               keepalives and wait for 0x55 from the phone.  */
        case 0x49:  DisableKeepalive = true;
                    FB38_TX_SendStandardAcknowledge(MessageBuffer[0]);
#ifdef DEBUG
                    fprintf(stdout, _("Phone powering off..."));
                    fflush(stdout);
#endif
                    return FB38_RX_Off;
    
            /* 0x4a message is a response to our 0x4a request, assumed to
               be a keepalive message of sorts.  No response required. */
        case 0x4a:  break;

            /* 0x4b messages are sent by phone in response (it seems)
               to the keep alive packet.  We must acknowledge these it seems
               by sending a response with the "sequence number" byte loaded
               appropriately. */
        case 0x4b:  FB38_RX_Handle0x4b_Status();
                    break;

            /* We send 0x4c to request IMEI, Revision and Model info. */
        case 0x4c:  break;

            /* 0x4d Message provides IMEI, Revision and Model information. */
        case 0x4d:  FB38_RX_Handle0x4d_IMEIRevisionModelData();
                    break;

            /* Here we  attempt to acknowledge and display messages we don't
               understand fully... The phone will send the same message
               several (5-6) times before giving up if no ack is received.
               Phone also appears to refuse to send any of that message type
               again until an init sequence is done again. */
        default:    if (FB38_TX_SendStandardAcknowledge(MessageBuffer[0]) != true) {
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
    
void    FB38_RX_DisplayMessage(void)
{
    int     count;
    int     line_count; 

        /* If debugging is disabled, don't display anything. */
#ifndef DEBUG
    return;
#endif
    
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
void    FB38_TX_UpdateSequenceNumber(void)
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
int     FB38_TX_SendStandardAcknowledge(u8 message_type)
{
        /* Standard acknowledge seems to be to return an empty message
           with the sequence number set to equal the sequence number
           sent minus 0x08. */
    return(FB38_TX_SendMessage(0, message_type, (MessageBuffer[1] & 0x1f) - 0x08, NULL));

}

    /* This is still a bit of a hack/work in progress. 
       Known call types are 0x01 - data and 0x05 voice at present. */
GSM_Error   FB38_TX_SendDialCommand(u8 call_type, char *Number)
{
    u8      message[256];
    int     number_length;
    int     i;
    
    number_length = strlen(Number);
    
    if (number_length > 200) {
        return(GE_INTERNALERROR);
    }

    message[0] = call_type; /* Data call */
    message[1] = 0x01;  /* Address/number type ? (cf Harri's work) */

    message[2] = strlen(Number);    /* Length of phone number */
    
        /* Copy in phone number, ascii encoded. */
    for (i = 0; i < number_length; i++) {
        message[3 + i] = Number[i];
    }
        /* Dunno what these are but may be initial setup values for RLP
           timers, sequence numbers or such ?
     	   InitField1 is much like as in FB38_TX_Send0x15...
	       InitField1 isn't needed for voice calls */
    message[3 + number_length] = 0x07;	/* Length of "InitField1" */
    message[4 + number_length] = 0xa2;  /* InitField1 */
    message[5 + number_length] = 0x88;	/* . */
    message[6 + number_length] = 0x81;	/* . */
    message[7 + number_length] = 0x21;	/* . */
    message[8 + number_length] = 0x15;	/* . */
    message[9 + number_length] = 0x63;	/* . */
    message[10 + number_length] = 0xa8;	/* . */

    message[11 + number_length] = 0x00;	/* Length of "InitField2" */
    message[12 + number_length] = 0x00;	/* Length of "InitField3" */
 
        /* Update sequence number and send to phone. */
    FB38_TX_UpdateSequenceNumber();
    if (FB38_TX_SendMessage(21, 0x0a, RequestSequenceNumber, message) != true) {
        fprintf(stderr, _("Set Mem Loc Write failed!"));    
        return (GE_INTERNALERROR);
    }

    return (GE_NONE);
}


	/* Send RLP frame to phone.  The format of the RLP message to the phone
	   is similar to the other command messages as far as checksum calculation
	   etc. goes. */
bool 	FB38_TX_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	u8 		message[36];
  	u8  	checksum;
	int		i;
	
		/* Setup message header */
	message[0] = 0x02;	/* Start of message - RLP type */
	message[1] = 0x21;  /* Length */
	message[2] = 0x01;  /* No idea */

		/* Byte 4 is 0x01 for Discontinuous transmission (DTX).
		   0x00 otherwise.   See section 5.6 of GSM 04.22 version 0.7.1 */
	if (out_dtx) {
    	message[3] = 0x01;
  	}
  	else {
		message[3] = 0x00;
  	}
	message[4] = 0xd9;	/* No idea but is the same as the 5110/6110 */

		/* Copy frame into message */
	memcpy(message + 5, (u8 *) frame, 30);

        /* Now calculate checksum over entire message 
           and append to message. */
    checksum = 0;
    for (i = 0; i < 36; i ++) {
        checksum ^= message[i];
    }
	message[35] = checksum;

        /* Send it out... */
    if (WRITEPHONE(PortFD,message, 36) != 36) {
        perror(_("TX_SendRLPFrame - write:"));
        return (false);
    }
    return (true);
}


    /* 0x4a messages appear to be a keepalive message and cause the phone
       to send an 0x4a acknowledge and then an 0x4b message which has
       status information in it. */
void    FB38_TX_Send0x4aMessage(void) 
{
    FB38_TX_UpdateSequenceNumber();
    
    if (FB38_TX_SendMessage(0, 0x4a, RequestSequenceNumber, NULL) != true) {
        fprintf(stderr, _("0x4a Write failed!"));   
    }
}

    /* Send arbitrary command - used to try and map out some
       more of the protocol (hopefully Cell Info for one!) */
void    FB38_TX_SendExploreMessage(u8 message) 
{
    DisableKeepalive = true;

    FB38_TX_UpdateSequenceNumber();

    ExploreMessage = message;

    if (FB38_TX_SendMessage(0, message, RequestSequenceNumber, NULL) != true) {
        fprintf(stderr, _("Explore Write failed!"));    
    }
}

    /* 0x3f messages request SMS information, returned as an 0x41 message. */
void    FB38_TX_Send0x3fMessage(void) 
{
    FB38_TX_UpdateSequenceNumber();
    
    if (FB38_TX_SendMessage(0, 0x3f, RequestSequenceNumber, NULL) != true) {
        fprintf(stderr, _("0x3f Write failed!"));   
    }
}

    /* 0x42 messages are sent to write a phonebook memory location.
       This function is designed to be called by the FB38_WriteMemoryLocation
       routine above which performs length checking etc. */
int     FB38_TX_Send0x42_WriteMemoryLocation(u8 memory_area, u8 location, char *label, char *number)
{
    u8      message[256];
    int     label_length;
    int     number_length;
    int     message_length;

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
void    FB38_TX_Send0x43_RequestMemoryLocation(u8 memory_area, u8 location) 
{
    u8      message[2];

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


void    FB38_TX_Send0x23_SendSMSHeader(char *message_center, char *destination, u8 total_length)
{
    
    u8      message[255];
    u8      message_center_length, destination_length;

        /* Update sequence number. */
    FB38_TX_UpdateSequenceNumber();

        /* Get length of message center and destination numbers. */
    message_center_length = strlen (message_center);
    destination_length = strlen (destination);

        /* Build and send message. */

    message[0] = FO_DEFAULT;	/* TP-FO */
    message[1] = PID_DEFAULT;	/* TP-PID */
    message[2] = DCS_DEFAULT;	/* TP-DCS */
    message[3] = GSMV_Max_Time;	/* VP (Only this octet used when VPF == Relative */
    message[4] = 0x00;		/* VP */
    message[5] = 0x00;		/* VP */
    message[6] = 0x00;		/* VP */
    message[7] = 0x00;		/* VP */
    message[8] = 0x00;		/* VP */
    message[9] = 0x00;		/* VP */
 
        /* Add total length and message_center number length fields. */
    message[10] = total_length;
    message[11] = message_center_length;

        /* Copy in the actual message center number. */
    memcpy (message + 12, message_center, message_center_length);

        /* Now add destination length and number. */
    message[12 + message_center_length] = destination_length;
    memcpy (message + 13 + message_center_length, destination, destination_length);

    if (FB38_TX_SendMessage(13 + message_center_length + destination_length, 0x23, RequestSequenceNumber, message) != true) {
        fprintf(stderr, _("Send SMS header failed!"));  
    }


}

void    FB38_TX_Send0x24_StoreSMSHeader(char *message_center, char *destination, u8 total_length)
{
	u8			msg[255];
	u8 		    smsc_len, dest_len;
	struct tm   *s_tm;
	time_t      t_t;

        /* Update sequence number. */
	FB38_TX_UpdateSequenceNumber();

        /* Get length of message center and destination numbers. */
	smsc_len = strlen (message_center);
    dest_len = strlen (destination);

	msg[0] = 0x01;	/* Store to GMT_CB (in 8110i) */
 
	msg[1] = 0x03;	/* Set status to received+new */
	msg[2] = 0x04;	/* These two are same as in requested sms's (Stat1 and Stat2) */
 
	msg[3] = 0x00;	/* PID */
	msg[4] = 0x00;	/* DCS */

	/* Set SCTS to current time */
	(void)time(&t_t);
	s_tm = localtime(&t_t);
#define swp(a) ( (((a) % 10) << 4) + ((a) / 10) )
	msg[5] = swp(s_tm->tm_year % 100);
	msg[6] = swp(s_tm->tm_mon + 1);
	msg[7] = swp(s_tm->tm_mday);
	msg[8] = swp(s_tm->tm_hour);
	msg[9] = swp(s_tm->tm_min);
	msg[10]= swp(s_tm->tm_sec);
	msg[11]= swp(0x08);	/* timezone in Finland */
#undef swp

	msg[12] = total_length;	/* UDL */

	msg[13] = smsc_len;	/* SCA */
	if (smsc_len > 0) {
		memcpy(msg + 14, message_center, smsc_len);
	}

	msg[14 + smsc_len] = dest_len;	/* DA, actually, destination is stored
					   as originating address */
	if (dest_len > 0) {
		memcpy(msg + 15 + smsc_len, destination, dest_len);
	}

	msg[15 + smsc_len + dest_len] = 0x00;	/* Address type of originating address.
						   When set to 0x00, phone changes it to
						   correct value automatically */

	if (FB38_TX_SendMessage(16 + smsc_len + dest_len, 0x24, RequestSequenceNumber, msg) != true) {
		fprintf(stderr, _("Store SMS header failed!"));  
	}
}
 

void    FB38_TX_Send0x27_SendSMSMessageText(u8 block_number, u8 block_length, char *text)
{
    u8      message[255];

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
void    FB38_TX_Send0x25_RequestSMSMemoryLocation(u8 memory_type, u8 location) 
{
    u8      message[2];

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

void    FB38_TX_Send0x26_DeleteSMSMemoryLocation(u8 memory_type, u8 location) 
{
    u8      message[2];

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

void    FB38_TX_Send0x15Message(u8 sequence_number)
{
    u8      message[20] = {0x02, 0x01, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x55, 0x63, 0xa8, 0x00, 0x00, 0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};

    if (FB38_TX_SendMessage(20, 0x15, sequence_number, message) != true) {
        fprintf(stderr, _("0x15 Write failed!"));   
    }
}

    /* Prepares the message header and sends it, prepends the
       message start byte (0x01) and other values according
       the value specified when called.  Calculates checksum
       and then sends the lot down the pipe... */
int     FB38_TX_SendMessage(u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer) 
{
    u8          out_buffer[FB38_MAX_TRANSMIT_LENGTH + 5];
    int         count;
    unsigned char           checksum;

        /* Check message isn't too long, once the necessary
           header and trailer bytes are included. */
    if ((message_length + 5) > FB38_MAX_TRANSMIT_LENGTH) {
        fprintf(stderr, _("TX_SendMessage - message too long!\n"));

        return (false);
    }
        /* Now construct the message header. */
    out_buffer[0] = 0x01;   /* Start of message indicator */
    out_buffer[1] = message_length + 2; /* Our message length refers to buffer, 
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
    if (WRITEPHONE(PortFD, out_buffer, message_length + 5) != message_length + 5) {
        perror(_("TX_SendMessage - write:"));
        return (false);
    }
    return (true);
}

void    FB38_RX_Handle0x0b_IncomingCall(void)
{
    int     count;
    char    buffer[256];

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

#ifdef DEBUG
        /* Now display incoming call message. */
    fprintf(stdout, _("Incoming call - Type: %s. %02x, Number %s.\n"),
		(MessageBuffer[2] == 0x05 ? "Voice":"Data?"), MessageBuffer[3], buffer);

    fflush (stdout);
#endif

}

    /* 0x27 messages are a little unusual when sent by the phone in that
       they can either be an acknowledgement of an 0x27 message we sent
       to the phone with message text in it or they could
       contain message text for a message we requested. */
void    FB38_RX_Handle0x27_SMSMessageText(void)
{
    static int  length_received;
    int         count;
    
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
    if (CurrentSMSMessage == NULL) {
        CurrentSMSMessageError = GE_INTERNALERROR;
        return;
    }

    for (count = 0; count < MessageLength - 3; count ++) {
        if ((length_received) < FB38_MAX_SMS_LENGTH) {
            CurrentSMSMessage->MessageText[length_received] = MessageBuffer[count + 3];
        }   
        length_received ++;
    }
    
    if (length_received == CurrentSMSMessageBodyLength) {
        CurrentSMSMessage->MessageText[length_received] = 0;
            /* Signal that the response is complete. */
        CurrentSMSMessageError = GE_NONE;
    }

}




    /* 0x4b is a general status message. */
void    FB38_RX_Handle0x4b_Status(void)
{
		/* Strings for the status byte received from phone. */
#ifdef DEBUG
	char *StatusStr[] = {
		"Unknown",
		"Ready",
		"Interworking",
		"Call in progress",
		"No SIM access"
	};
#endif

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

        /* GetRFLevel function does conversion into required units. */
    CurrentRFLevel = MessageBuffer[3];

        /* GetBatteryLevel does conversion into required units. */
    CurrentBatteryLevel = MessageBuffer[4];

#ifdef DEBUG
        /* Only output connection status byte now as the RF and Battery
           levels are displayed by the main gnokii code. */
    fprintf(stdout, _("Status: %s. Batt %02x RF %02x.\n"),
		StatusStr[MessageBuffer[2]], MessageBuffer[4], MessageBuffer[3]);
#endif

}


void    FB38_RX_Handle0x10_EndOfOutgoingCall(void)
{
        /* As usual, acknowledge first. */
    if (FB38_TX_SendMessage(0, 0x10, MessageBuffer[1] - 0x08, NULL) != true) {
        fprintf(stderr, _("0x10 Write failed!"));
    }

#ifdef DEBUG
    fprintf(stdout, _("Call terminated from phone (0x10 message).\n"));
    fflush(stdout);
#endif

}

void    FB38_RX_Handle0x11_EndOfIncomingCall(void)
{
        /* As usual, acknowledge first. */
    if (FB38_TX_SendMessage(0, 0x11, MessageBuffer[1] - 0x08, NULL) != true) {
        fprintf(stderr, _("Write failed!"));
    }

#ifdef DEBUG
    fprintf(stdout, _("Call terminated from opposite end of line (or from network).\n"));
    fflush(stdout);
#endif

}

void    FB38_RX_Handle0x12_EndOfOutgoingCall(void)
{
        /* As usual, acknowledge first. */
    if (FB38_TX_SendMessage(0, 0x12, MessageBuffer[1] - 0x08, NULL) != true) {
        fprintf(stderr, _("Write failed!"));
    }

#ifdef DEBUG
    fprintf(stdout, _("Call terminated from phone (0x12 message).\n"));
    fflush(stdout);
#endif

}

void    FB38_RX_Handle0x0d_IncomingCallAnswered(void)
{
        /* As usual, acknowledge first. */
    if (FB38_TX_SendMessage(0, 0x0d, MessageBuffer[1] - 0x08, NULL) != true) {
        fprintf(stderr, _("Write failed!"));
    }

#ifdef DEBUG
    fprintf(stdout, _("Incoming call answered from phone.\n"));
    fflush(stdout);
#endif

}

void    FB38_RX_Handle0x0e_CallEstablished(void)
{
        /* As usual, acknowledge first. */
    if (FB38_TX_SendMessage(0, 0x0e, MessageBuffer[1] - 0x08, NULL) != true) {
        fprintf(stderr, _("Write failed!"));
    }

#ifdef DEBUG
    fprintf(stdout, _("%s call established - status bytes %02x %02x.\n"),
		(MessageBuffer[2] == 0x05 ? "voice":"data(?)"), MessageBuffer[3], MessageBuffer[4]);
    fflush(stdout);
#endif

}

    /* 0x2c message is sent in response to an 0x25 request.  Appears
       to have the same fields as the 0x30 notification but with
       one extra. */
void    FB38_RX_Handle0x2c_SMSHeader(void)
{
    u8      sender_length;
    u8      message_center_length;

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
        
        /* Starting with memory type. */
    if (MessageBuffer[2] == 1) {
        CurrentSMSMessage->MemoryType = GMT_ME;
    }
    else {
        if (MessageBuffer[2] == 2) {
            CurrentSMSMessage->MemoryType = GMT_SM;
        }
        else {
                /* Unknown memory type. */
            CurrentSMSMessage->MemoryType = GMT_XX;
        }
    }
        /* 3810 series has limited support for different SMS "mailboxes"
           to the extent that the only know differentiation is between
           received messages 0x01, 0x04 and written messages 0x07 0x01.
           No flag has been found (yet) that indicates whether the 
           message has been sent or not. */
    
        /* Default to unknown message type */
    CurrentSMSMessage->Type = GST_UN;

        /* Consider received messages "Inbox" (Mobile Terminated) */
    if (MessageBuffer[4] == 0x01  && MessageBuffer[5] == 0x04) {
        CurrentSMSMessage->Type = GST_MT;
    }

        /* Consider written messages "Outbox" (Mobile Originated) */
    if (MessageBuffer[4] == 0x07  && MessageBuffer[5] == 0x01) {
        CurrentSMSMessage->Type = GST_MO;
    }
    
        /* We don't know about read/unread or sent/unsent status.
           so assume has been sent or read */
    CurrentSMSMessage->Status = GSS_SENTREAD;

        /* Now do message number and length */
    CurrentSMSMessage->MessageNumber = MessageBuffer[3];
    CurrentSMSMessage->Length = MessageBuffer[15];

        /* Extract date and time information which is packed in to 
           nibbles of each byte in reverse order.  Thus day 28 would be
           encoded as 0x82 */

    CurrentSMSMessage->Time.Year = (MessageBuffer[8] >> 4) + (10 * (MessageBuffer[8] & 0x0f)); 
    CurrentSMSMessage->Time.Month = (MessageBuffer[9] >> 4) + (10 * (MessageBuffer[9] & 0x0f)); 
    CurrentSMSMessage->Time.Day = (MessageBuffer[10] >> 4) + (10 * (MessageBuffer[10] & 0x0f)); 
    CurrentSMSMessage->Time.Hour = (MessageBuffer[11] >> 4) + (10 * (MessageBuffer[11] & 0x0f)); 
    CurrentSMSMessage->Time.Minute = (MessageBuffer[12] >> 4) + (10 * (MessageBuffer[12] & 0x0f)); 
    CurrentSMSMessage->Time.Second = (MessageBuffer[13] >> 4) + (10 * (MessageBuffer[13] & 0x0f)); 
    CurrentSMSMessage->Time.Timezone = MessageBuffer[14];

        /* Now get sender and message center information. */
    message_center_length = MessageBuffer[16];
    sender_length = MessageBuffer[16 +  message_center_length + 1];

        /* Check they're not too long. */
    if (sender_length > FB38_MAX_SENDER_LENGTH) {
        sender_length = FB38_MAX_SENDER_LENGTH;
    }
        
    if (message_center_length > FB38_MAX_SMS_CENTER_LENGTH) {
        message_center_length = FB38_MAX_SMS_CENTER_LENGTH;
    }

        /* Now copy to strings... Note they are in reverse order to
           0x30 message*/
    memcpy(CurrentSMSMessage->MessageCenter.Number, MessageBuffer + 17, message_center_length);
    CurrentSMSMessage->MessageCenter.Number[message_center_length] = 0; /* Ensure null terminated. */
    
    strncpy(CurrentSMSMessage->Sender, MessageBuffer + 18 + message_center_length, sender_length);
    CurrentSMSMessage->Sender[sender_length] = 0;

    fprintf(stdout, _("PID:%02x DCS:%02x Timezone:%02x Stat1:%02x Stat2:%02x\n"), 
                    MessageBuffer[6], MessageBuffer[7], MessageBuffer[14],
                    MessageBuffer[4], MessageBuffer[5]);
    fflush(stdout);
}

void    FB38_RX_Handle0x30_IncomingSMSNotification(void)
{
    int     year, month, day;       /* Date of arrival */
    int     hour, minute, second;   /* Time of arrival */
    int     msg_number;             /* Message number in phone's memory */

    char    sender[255];            /* Sender details */
    u8      sender_length;

    char    message_center[255];    /* And message center number/ID */
    u8      message_center_length;

    u8      message_body_length;    /* Length of actual SMS message itself. */

    u8      Mem, unk2, PID, DCS; /* Unknown bytes at start of message */
    u8      tz;

    u8      TOA;                /* Type of Originating Address */

    if (FB38_TX_SendStandardAcknowledge(0x30) != true) {
        fprintf(stderr, _("Write failed!"));
    }

        /* Extract data from message. */
    Mem = MessageBuffer[2];
    msg_number = MessageBuffer[3];	/* location */
    unk2 = MessageBuffer[4];
    PID = MessageBuffer[5];
    DCS = MessageBuffer[6];		/* DCS */

        /* Extract date and time information which is packed in to 
           nibbles of each byte in reverse order.  Thus day 28 would be
           encoded as 0x82 */

    year = (MessageBuffer[7] >> 4) + (10 * (MessageBuffer[7] & 0x0f)); 
    month = (MessageBuffer[8] >> 4) + (10 * (MessageBuffer[8] & 0x0f)); 
    day = (MessageBuffer[9] >> 4) + (10 * (MessageBuffer[9] & 0x0f)); 
    hour = (MessageBuffer[10] >> 4) + (10 * (MessageBuffer[10] & 0x0f)); 
    minute = (MessageBuffer[11] >> 4) + (10 * (MessageBuffer[11] & 0x0f)); 
    second = (MessageBuffer[12] >> 4) + (10 * (MessageBuffer[12] & 0x0f)); 

    tz = MessageBuffer[13];		/* timezone */
    message_body_length = MessageBuffer[14];

        /* Now get sender and message center information. */
    sender_length = MessageBuffer[15];
    message_center_length = MessageBuffer[15 +  sender_length + 1];

        /* Now copy to strings... */
    strncpy(sender, MessageBuffer + 16, sender_length);
    sender[sender_length] = 0;  /* Ensure null terminated. */
    
    strncpy(message_center, MessageBuffer + 17 + sender_length, message_center_length);
    message_center[message_center_length] = 0;

        /* Get last byte, type of originating address (number). (0xa1 = international)*/
    TOA = MessageBuffer[17 + sender_length + message_center_length];

 
        /* And output. */
#ifdef DEBUG
    fprintf(stdout, _("Incoming SMS %d/%d/%d %d:%02d:%02d tz:0x%02x Sender: %s(Type %02x) Msg Center: %s\n"),
            year, month, day, hour, minute, second, tz, sender, TOA, message_center);
    fprintf(stdout, _("   Msg Length %d, Msg memory %d Msg number %d,  PID: %02x DCS: %02x Unknown: %02x\n"), 
            message_body_length, Mem, msg_number, PID, DCS, unk2);
    fflush(stdout);
#endif
}




    /* Handle data contained in an 0x46 message which is sent by the phone
       in reponse to a 0x43 message requesting memory contents. */
void    FB38_RX_Handle0x46_MemoryLocationData(void)
{
    u8      label_length;   /* Stored in first data byte in message. */
    u8      number_length;  /* Stored immediately after label data. */

        /* As usual, acknowledge first. */
    if (FB38_TX_SendMessage(0, 0x46, MessageBuffer[1] - 0x08, NULL) != true) {
        fprintf(stderr, _("Write failed!"));
    }

        /* Get/Calculate label and number length. */
    label_length = MessageBuffer[2];
    number_length = MessageBuffer[label_length + 3];

        /* Providing it's not a NULL pointer, copy entry into
           CurrentPhonebookEntry */
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
void    FB38_RX_Handle0x4d_IMEIRevisionModelData(void)
{
    int     imei_length;
    int     rev_length;

        /* As usual, acknowledge first. */
    if (!FB38_TX_SendStandardAcknowledge(0x4d)) {
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

#ifdef DEBUG
    fprintf(stdout, _("Mobile phone identification received:\n"));
    fprintf(stdout, _("   IMEI:     %s\n"), IMEI);

    fprintf(stdout, _("   Model:    %s\n"), Model);

    fprintf(stdout, _("   Revision: %s\n"), Revision);
#endif
}


    /* Handle 0x41 message which is sent by phone in response to an
       0x3f request.  Contains data about the Message Center in use */
void    FB38_RX_Handle0x41_SMSMessageCenterData(void)
{
    u8      center_number_length;
    u8      option_number_length;
    u8      opt_num[64];

    int     count;
    
        /* As usual, acknowledge first. */
    if (!FB38_TX_SendStandardAcknowledge(0x41)) {
        fprintf(stderr, _("Write failed!"));
        CurrentMessageCenterError = GE_INTERNALERROR;
        return;
    }
        /* Check the CurrentMessageCenter is non-null (as can occur
           if doing passive monitoring during development) */
    if (CurrentMessageCenter == NULL) {
        CurrentMessageCenterError = GE_INTERNALERROR;
        return;
    }

    if (CurrentSMSStatus == NULL) {
        CurrentMessageCenterError = GE_INTERNALERROR;
        return;
    }

	CurrentMessageCenter->Format = MessageBuffer[7];
	CurrentMessageCenter->Validity = MessageBuffer[9];

	option_number_length = MessageBuffer[12]; // Dont know meaning of
						  // this number string
	if (option_number_length != 0) {
	        for (count = 0; count < option_number_length; count++) {
				opt_num[count] = MessageBuffer[13 + count];
			}
	}

	center_number_length = MessageBuffer[13 + option_number_length];

	if (center_number_length == 0) {
		CurrentMessageCenter->Number[0] = 0x00; /* Null terminate */
	} else {
		memcpy(CurrentMessageCenter->Number,
			MessageBuffer + 14 + option_number_length,
			center_number_length);
		CurrentMessageCenter->Number[center_number_length] = '\0';
	}
 
        /* 3810 series doesn't support Name or multiple center numbers
           so put in null data for them . */
    CurrentMessageCenter->Name[0] = 0x00;
    CurrentMessageCenter->No = 0;

	CurrentSMSStatus->UnRead = MessageBuffer[4] + MessageBuffer[6];
	CurrentSMSStatus->Number = MessageBuffer[3] + MessageBuffer[5];

    CurrentMessageCenterError = GE_NONE;


#ifdef DEBUG

	fprintf(stdout, _("SMS Message Center Data:\n"));
	fprintf(stdout, _("Selected memory: 0x%02x\n"), MessageBuffer[2]);
	fprintf(stdout, _("Messages in Phone: 0x%02x Unread: 0x%02x\n"),
		MessageBuffer[3], MessageBuffer[4]);
	fprintf(stdout, _("Messages in SIM: 0x%02x Unread: 0x%02x\n"),
		MessageBuffer[5], MessageBuffer[6]);
	fprintf(stdout, _("Reply via own centre: 0x%02x (%s)\n"),
		MessageBuffer[10], (MessageBuffer[10] == 0x02 ? "Yes" : "No"));
	fprintf(stdout, _("Delivery reports: 0x%02x (%s)\n"),
		MessageBuffer[11], (MessageBuffer[11] == 0x02 ? "Yes" : "No"));
	fprintf(stdout, _("Messages sent as: 0x%02x\n"), MessageBuffer[7]);
	fprintf(stdout, _("Message validity: 0x%02x\n"), MessageBuffer[9]);
	fprintf(stdout, _("Unknown: 0x%02x\n"), MessageBuffer[8]);

	if (option_number_length == 0)
		fprintf(stdout, _("UnknownNumber field empty."));
	else {
		fprintf(stdout, _("UnknownNumber: "));
		for (count = 0; count < option_number_length; count ++)
			fprintf(stdout, "%c", MessageBuffer[13 + count]);
	}
	fprintf(stdout, "\n");

    if (center_number_length == 0) {
        fprintf(stdout, _("Number field empty."));
    }
    else {
        fprintf(stdout, _("Number: "));
        for (count = 0; count < center_number_length; count ++) {
            fprintf(stdout, "%c", MessageBuffer[14 + option_number_length + count]);
        }
    }
    fprintf(stdout, "\n");

    fflush(stdout);
#endif
}

int sso2i(u8 x)
{
	return (int)(((x & 0x0f) << 4) + ((x & 0xf0) >> 4));
}

void FB38_RX_Handle0x32_SMSDelivered(void)
{
	GSM_DateTime DT, SCTS;
	char smsc[30];
	char dest[30];
	int dest_len, smsc_len;
	u8 MR, TA, U1, U2;

	FB38_TX_SendStandardAcknowledge(0x32);

	U1 = MessageBuffer[2];

	DT.Year = 	sso2i(MessageBuffer[3]);
	DT.Month = 	sso2i(MessageBuffer[4]);
	DT.Day = 	sso2i(MessageBuffer[5]);
	DT.Hour = 	sso2i(MessageBuffer[6]);
	DT.Minute = 	sso2i(MessageBuffer[7]);
	DT.Second = 	sso2i(MessageBuffer[8]);
	DT.Timezone = 	sso2i(MessageBuffer[9]);

	SCTS.Year = 	sso2i(MessageBuffer[10]);
	SCTS.Month = 	sso2i(MessageBuffer[11]);
	SCTS.Day = 	sso2i(MessageBuffer[12]);
	SCTS.Hour = 	sso2i(MessageBuffer[13]);
	SCTS.Minute = 	sso2i(MessageBuffer[14]);
	SCTS.Second = 	sso2i(MessageBuffer[15]);
	SCTS.Timezone = sso2i(MessageBuffer[16]);

	U2 = MessageBuffer[17];

	MR = MessageBuffer[18];

	dest_len = (int)MessageBuffer[19];
	strncpy(dest, MessageBuffer + 20, dest_len);
	dest[dest_len] = '\0';
	smsc_len = (int)MessageBuffer[20 + dest_len];
	strncpy(smsc, MessageBuffer + 21 + dest_len, smsc_len);
	smsc[smsc_len] = '\0';

	TA = MessageBuffer[MessageLength - 1];

#ifdef DEBUG
	fprintf(stdout, "Message [%02x] Delivered!\n", MR);
	fprintf(stdout, "Destination: %s (TypeOfAddress:%02x)\n", dest, TA);
	fprintf(stdout, "Message Center: %s\n", smsc);
	fprintf(stdout, "Unknowns: %02x %02x\n", U1, U2);	
	fprintf(stdout, "Discharge Time: %02x.%02x.%02x %02x:%02x:%02x\n",
		DT.Day, DT.Month, DT.Year, DT.Hour, DT.Minute, DT.Second);
	fprintf(stdout, "SC Time Stamp:  %02x.%02x.%02x %02x:%02x:%02x\n",
		SCTS.Day, SCTS.Month, SCTS.Year, SCTS.Hour, SCTS.Minute, SCTS.Second);
	fflush(stdout);
#endif
}
 
