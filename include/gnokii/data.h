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
 
/* Define the structure used to hold pointers to the various API functions.
   This is in effect the master list of functions provided by the gnokii API.
   Modules containing the model specific code each contain one of these
   structures which is "filled in" with the corresponding function within the
   model specific code.  If a function is not supported or not implemented, a
   generic not implemented function is used to return a GE_NOTIMPLEMENTED
   error code. */

typedef struct {
	/* FIXME: comment this. */

	GSM_Error (*Initialise)( char *port_device, char *initlength,
				 GSM_ConnectionType connection,
				 void (*rlp_callback)(RLP_F96Frame *frame));

	void (*Terminate)(void);	

	GSM_Error (*GetMemoryLocation)( GSM_PhonebookEntry *entry );

	GSM_Error (*WritePhonebookLocation)( GSM_PhonebookEntry *entry );

	GSM_Error (*GetSpeedDial)( GSM_SpeedDial *entry);

	GSM_Error (*SetSpeedDial)( GSM_SpeedDial *entry);

	GSM_Error (*GetMemoryStatus)( GSM_MemoryStatus *Status);

	GSM_Error (*GetSMSStatus)( GSM_SMSMemoryStatus *Status);

	GSM_Error (*GetSMSCenter)( SMS_MessageCenter *MessageCenter );

	GSM_Error (*SetSMSCenter)( SMS_MessageCenter *MessageCenter );

	GSM_Error (*GetSMSMessage)( GSM_SMSMessage *Message );

	GSM_Error (*DeleteSMSMessage)( GSM_SMSMessage *Message );

	GSM_Error (*SendSMSMessage)( GSM_SMSMessage *Message, int size );

	GSM_Error (*SaveSMSMessage)( GSM_SMSMessage *Message );

	/* If units is set to a valid GSM_RFUnits value, the code
	   will return level in these units if it is able.  Otherwise
	   value will be returned as GRF_Arbitary.  If phone doesn't
	   support GetRFLevel, function returns GE_NOTSUPPORTED */
	GSM_Error (*GetRFLevel)( GSM_RFUnits *units, float *level );

	/* Works the same as GetRFLevel, except returns battery
	   level if known. */
	GSM_Error (*GetBatteryLevel)( GSM_BatteryUnits *units, float *level );

	GSM_Error (*GetPowerSource)( GSM_PowerSource *source);

	GSM_Error (*GetDisplayStatus)( int *Status);

	GSM_Error (*EnterSecurityCode)( GSM_SecurityCode Code);

	GSM_Error (*GetSecurityCodeStatus)( int *Status );

	GSM_Error (*GetIMEI)( char *imei );

	GSM_Error (*GetRevision)( char *revision );

	GSM_Error (*GetModel)( char *model );

	GSM_Error (*GetManufacturer)( char *manufacturer );

	GSM_Error (*GetDateTime)( GSM_DateTime *date_time);

	GSM_Error (*SetDateTime)( GSM_DateTime *date_time);

	GSM_Error (*GetAlarm)( int alarm_number, GSM_DateTime *date_time );

	GSM_Error (*SetAlarm)( int alarm_number, GSM_DateTime *date_time );

	GSM_Error (*DialVoice)( char *Number);

	GSM_Error (*DialData)( char *Number, char type, void (* callpassup)(char c));

	GSM_Error (*GetIncomingCallNr)( char *Number );

	GSM_Error (*GetNetworkInfo) ( GSM_NetworkInfo *NetworkInfo );

	GSM_Error (*GetCalendarNote) ( GSM_CalendarNote *CalendarNote);

	GSM_Error (*WriteCalendarNote) ( GSM_CalendarNote *CalendarNote);

	GSM_Error (*DeleteCalendarNote) ( GSM_CalendarNote *CalendarNote);

	GSM_Error (*NetMonitor) ( unsigned char mode, char *Screen );

	GSM_Error (*SendDTMF) ( char *String );

	GSM_Error (*GetBitmap) ( GSM_Bitmap *Bitmap );
  
	GSM_Error (*SetBitmap) ( GSM_Bitmap *Bitmap );

	GSM_Error (*SetRingtone) ( GSM_Ringtone *ringtone );

	GSM_Error (*SendRingtone) ( GSM_Ringtone *ringtone, char *dest );

	GSM_Error (*Reset) ( unsigned char type );

	GSM_Error (*GetProfile) ( GSM_Profile *Profile );

	GSM_Error (*SetProfile) ( GSM_Profile *Profile );
  
	bool      (*SendRLPFrame) ( RLP_F96Frame *frame, bool out_dtx );

	GSM_Error (*CancelCall) ();
  
	GSM_Error (*EnableDisplayOutput) ();
  
	GSM_Error (*DisableDisplayOutput) ();
 
	GSM_Error (*EnableCellBroadcast) ();

	GSM_Error (*DisableCellBroadcast) ();

	GSM_Error (*ReadCellBroadcast) ( GSM_CBMessage *Message );
  
	GSM_Error (*SetKey) (int c, int up);
  
	GSM_Error (*HandleString) (char *s);
	
	GSM_Error (*AnswerCall) (char s);
} GSM_Functions;

/* Undefined functions in fbus/mbus files */
extern GSM_Error Unimplemented(void);
#define UNIMPLEMENTED (void *) Unimplemented

extern GSM_MemoryType StrToMemoryType (const char *s);

inline void GSM_DataClear(GSM_Data *data);

#endif	/* __gsm_data_h */
