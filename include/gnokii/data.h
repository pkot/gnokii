/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#ifndef __gsm_data_h
#define __gsm_data_h

#include "gsm-common.h"
#include "gsm-sms.h"
#include "gsm-error.h"

/* This is a generic holder for high level information - eg a GSM_Bitmap */
typedef struct {
	SMS_Folder *SMSFolder;
	SMS_FolderList *SMSFolderList;
	GSM_SMSMessage *SMSMessage;
	GSM_PhonebookEntry *PhonebookEntry;
	GSM_SpeedDial *SpeedDial;
	GSM_MemoryStatus *MemoryStatus;
	GSM_SMSMemoryStatus *SMSStatus;
	SMS_MessageCenter *MessageCenter;
	char *Imei;
	char *Revision;
	char *Model;
	char *Manufacturer;
	GSM_NetworkInfo *NetworkInfo;
	GSM_CalendarNote *CalendarNote;
	GSM_Bitmap *Bitmap;
	GSM_Ringtone *Ringtone;
	GSM_Profile *Profile;
	GSM_BatteryUnits *BatteryUnits;
	float *BatteryLevel;
	GSM_RFUnits *RFUnits;
	float *RFLevel;
	GSM_Error (*OutputFn)(char *Display, char *Indicators);
	char *IncomingCallNr;
	GSM_PowerSource *PowerSource;
	GSM_DateTime *DateTime;
	GSM_RawData *RawData;
	GSM_CallDivert *CallDivert;
	GSM_Error (*OnSMS)(GSM_SMSMessage *Message);
	int *DisplayStatus;
} GSM_Data;

/* Global structures intended to be independant of phone etc */
/* Obviously currently rather Nokia biased but I think most things */
/* (eg at commands) can be enumerated */

/* A structure to hold information about the particular link */
/* The link comes 'under' the phone */
typedef struct {
	char PortDevice[20];   /* The port device */
	int InitLength;        /* Number of chars sent to sync the serial port */
	GSM_ConnectionType ConnectionType;   /* Connection type, serial, ir etc */

	/* A regularly called loop function */
	/* timeout can be used to make the function block or not */
	GSM_Error (*Loop)(struct timeval *timeout);

	/* A pointer to the function used to send out a message */
	/* This is used by the phone specific code to send a message over the link */
	GSM_Error (*SendMessage)(u16 messagesize, u8 messagetype, void *message);
} GSM_Link;

/* Small structure used in GSM_Phone */
/* Messagetype is passed to the function in case it is a 'generic' one */
typedef struct {
	u8 MessageType;
	GSM_Error (*Functions)(int messagetype, unsigned char *buffer, int length, GSM_Data *data);
} GSM_IncomingFunctionType;

typedef enum {
	GOP_Init,
	GOP_GetModel,
	GOP_GetRevision,
	GOP_GetImei,
	GOP_GetManufacturer,
	GOP_Identify,
	GOP_GetBitmap,
	GOP_SetBitmap,
	GOP_GetBatteryLevel,
	GOP_GetRFLevel,
	GOP_DisplayOutput,
	GOP_GetMemoryStatus,
	GOP_ReadPhonebook,
	GOP_WritePhonebook,
	GOP_GetPowersource,
	GOP_GetAlarm,
	GOP_GetSMSStatus,
	GOP_GetIncomingCallNr,
	GOP_GetNetworkInfo,
	GOP_GetSMS,
	GOP_GetNextSMS,
	GOP_DeleteSMS,
	GOP_SendSMS,
	GOP_GetSpeedDial,
	GOP_GetSMSCenter,
	GOP_GetDateTime,
	GOP_GetCalendarNote,
	GOP_CallDivert,
	GOP_OnSMS,
	GOP_PollSMS,
	GOP_SetAlarm,
	GOP_SetDateTime,
	GOP_GetProfile,
	GOP_SetProfile,
	GOP_WriteCalendarNote,
	GOP_DeleteCalendarNote,
	GOP_SetSpeedDial,
	GOP_GetDisplayStatus,
	GOP_Max,	/* don't append anything after this entry */
} GSM_Operation;

/* This structure contains the 'callups' needed by the statemachine */
/* to deal with messages from the phone and other information */

typedef struct _GSM_Statemachine GSM_Statemachine; 

typedef struct {
	/* These make up a list of functions, one for each message type and NULL terminated */
	GSM_IncomingFunctionType *IncomingFunctions;
	GSM_Error (*DefaultFunction)(int messagetype, unsigned char *buffer, int length);
	GSM_Information Info;
	GSM_Error (*Functions)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
} GSM_Phone;


/* The states the statemachine can take */

typedef enum {
	Startup,            /* Not yet initialised */
	Initialised,        /* Ready! */
	MessageSent,        /* A command has been sent to the link(phone) */ 
	WaitingForResponse, /* We are waiting for a response from the link(phone) */
	ResponseReceived    /* A response has been received - waiting for the phone layer to collect it */
} GSM_State;

/* How many message types we can wait for at one */
#define SM_MAXWAITINGFOR 3

/* All properties of the state machine */

struct _GSM_Statemachine {
	GSM_State CurrentState;
	GSM_Link Link;
	GSM_Phone Phone;
	
	/* Store last message for resend purposes */
	u8 LastMsgType;
	u16 LastMsgSize;
	void *LastMsg;

	/* The responses we are waiting for */
	unsigned char NumWaitingFor;
	unsigned char NumReceived;
	unsigned char WaitingFor[SM_MAXWAITINGFOR];
	GSM_Error ResponseError[SM_MAXWAITINGFOR];
	/* Data structure to be filled in with the response */
	GSM_Data *Data[SM_MAXWAITINGFOR];
};

/* Undefined functions in fbus/mbus files */
extern GSM_Error Unimplemented(void);
#define UNIMPLEMENTED (void *) Unimplemented

extern GSM_MemoryType StrToMemoryType (const char *s);

inline void GSM_DataClear(GSM_Data *data);

#endif	/* __gsm_data_h */
