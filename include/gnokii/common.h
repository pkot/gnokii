/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the definitions, enums etc. that are used by all models of
  handset.

  $Log$
  Revision 1.83  2001-11-17 20:18:33  pkot
  Added dau9p connection type for 6210/7110

  Revision 1.82  2001/11/13 16:12:21  pkot
  Preparing libsms to get to work. 6210/7110 SMS and SMS Folder updates

  Revision 1.81  2001/11/08 16:34:20  pkot
  Updates to work with new libsms

  Revision 1.80  2001/08/20 23:36:27  pkot
  More cleanup in AT code (Manfred Jonsson)

  Revision 1.79  2001/07/27 00:02:22  pkot
  Generic AT support for the new structure (Manfred Jonsson)

  Revision 1.78  2001/06/28 00:28:45  pkot
  Small docs updates (Pawel Kot)


*/

#ifndef __gsm_common_h
#define __gsm_common_h

#include <stdlib.h>

#include "data/rlp-common.h"
#include "gsm-sms.h"

/* Type of connection. Now we support serial connection with FBUS cable and
   IR (only with 61x0 models) */

typedef enum {
	GCT_Serial,   /* Serial connection. */
	GCT_DAU9P,     /* Serial connection using DAU9P cable; use only with 6210/7110 if you want faster initialization */
	GCT_Infrared, /* Infrared connection. */
	GCT_Tekram,   /* Tekram Ir-Dongle */
	GCT_Irda
} GSM_ConnectionType;

/* Maximum length of device name for serial port */

#define GSM_MAX_DEVICE_NAME_LENGTH (100)

/* Define an enum for specifying memory types for retrieving phonebook
   entries, SMS messages etc. This type is not mobile specific - the model
   code should take care of translation to mobile specific numbers - see 6110
   code.
   01/07/99:  Two letter codes follow GSM 07.07 release 6.2.0
*/

typedef enum {
	GMT_ME, /* Internal memory of the mobile equipment */
	GMT_SM, /* SIM card memory */
	GMT_FD, /* Fixed dial numbers */
	GMT_ON, /* Own numbers */
	GMT_EN, /* Emergency numbers */
	GMT_DC, /* Dialled numbers */
	GMT_RC, /* Received numbers */
	GMT_MC, /* Missed numbers */
	GMT_LD, /* Last dialed */
	GMT_MT, /* combined ME and SIM phonebook */
	GMT_TA, /* for compatibility only: TA=computer memory */
	GMT_CB, /* Currently selected memory */
	GMT_XX = 0xff	/* Error code for unknown memory type (returned by fbus-xxxx functions). */
} GSM_MemoryType;

/* Power source types */

typedef enum {
	GPS_ACDC=1, /* AC/DC powered (charging) */
	GPS_BATTERY /* Internal battery */
} GSM_PowerSource;

/* Definition of security codes. */

typedef enum {
	GSCT_SecurityCode = 0x01, /* Security code. */
	GSCT_Pin,                 /* PIN. */
	GSCT_Pin2,                /* PIN 2. */
	GSCT_Puk,                 /* PUK. */
	GSCT_Puk2,                /* PUK 2. */
	GSCT_None                 /* Code not needed. */
} GSM_SecurityCodeType;

/* Security code definition. */

typedef struct {
	GSM_SecurityCodeType Type; /* Type of the code. */
	char Code[10];             /* Actual code. */
} GSM_SecurityCode;

/* This structure is used to get the current network status */

typedef struct {
	char NetworkCode[10]; /* GSM network code */
	char CellID[10];      /* CellID */
	char LAC[10];         /* LAC */
} GSM_NetworkInfo;

/* Limits for sizing of array in GSM_PhonebookEntry. Individual handsets may
   not support these lengths so they have their own limits set. */

#define GSM_MAX_PHONEBOOK_NAME_LENGTH   (50)   /* For 7110 */
#define GSM_MAX_PHONEBOOK_NUMBER_LENGTH (48)   /* For 7110 */
#define GSM_MAX_PHONEBOOK_TEXT_LENGTH   (60)   /* For 7110 */
#define GSM_MAX_PHONEBOOK_SUB_ENTRIES   (8)    /* For 7110 */
                                               /* 7110 is able to in one
						* entry 5 numbers and 2
						* texts [email,notice,postal] */

/* Here is a macro for models that do not support caller groups. */

#define GSM_GROUPS_NOT_SUPPORTED -1

/* This data type is used to report the number of used and free positions in
   memory (sim or internal). */

typedef struct {
	GSM_MemoryType MemoryType; /* Type of the memory */
	int Used;                  /* Number of used positions */
	int Free;                  /* Number of free positions */
} GSM_MemoryStatus;

/* Some phones (at the moment 6210/7110) supports extended phonebook
   with additional data.  Here we have structures for them */

typedef enum {
	GSM_General = 0x0a,
	GSM_Mobile  = 0x03,
	GSM_Work    = 0x06,
	GSM_Fax     = 0x04,
	GSM_Home    = 0x02
} GSM_Number_Type;

typedef enum {
	GSM_Number  = 0x0b,
	GSM_Note    = 0x0a,
	GSM_Postal  = 0x09,
	GSM_Email   = 0x08,
	GSM_Name    = 0x07,
	GSM_Date    = 0x13   /* Date is used for DC,RC,etc (last calls) */
} GSM_EntryType;

typedef struct {
	bool AlarmEnabled; /* Is alarm set ? */
	int Year;          /* The complete year specification - e.g. 1999. Y2K :-) */
	int Month;	   /* January = 1 */
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Timezone;      /* The difference between local time and GMT */
} GSM_DateTime;

typedef struct {
	GSM_EntryType   EntryType;
	GSM_Number_Type NumberType;
	union {
		char Number[GSM_MAX_PHONEBOOK_TEXT_LENGTH + 1]; /* Number */
		GSM_DateTime Date;                         /* or the last calls list */
	} data;
	int             BlockNumber;
} GSM_SubPhonebookEntry;

/* Define datatype for phonebook entry, used for getting/writing phonebook
   entries. */

typedef struct {
	bool Empty;                                       /* Is this entry empty? */
	char Name[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1];     /* Plus 1 for
							     nullterminator. */
	char Number[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1]; /* Number */
	GSM_MemoryType MemoryType;                        /* Type of memory */
	int Group;                                        /* Group */
	int Location;                                     /* Location */
	GSM_DateTime Date;                                /* The record date and time
							     of the number. */
	GSM_SubPhonebookEntry SubEntries[GSM_MAX_PHONEBOOK_SUB_ENTRIES];
	/* For phones with
	 * additional phonebook
	 * entries */
	int SubEntriesCount;                              /* Should be 0, if extended
							     phonebook is not used */
} GSM_PhonebookEntry;

/* This define speed dialing entries. */

typedef struct {
	int Number;                /* Which number is used to dialing? */
	GSM_MemoryType MemoryType; /* Memory type of the number. */
	int Location;              /* Location of the number in MemoryType. */
} GSM_SpeedDial;

/* Define enum used to describe what sort of date/time support is
   available. */

typedef enum {
	GDT_None,     /* The mobile phone doesn't support time and date. */
	GDT_TimeOnly, /* The mobile phone supports only time. */
	GDT_DateOnly, /* The mobile phone supports only date. */
	GDT_DateTime  /* Wonderful phone - it supports date and time. */
} GSM_DateTimeSupport;

/* Define enums for RF units.  GRF_CSQ asks for units in form used
   in AT+CSQ command as defined by GSM 07.07 */

typedef enum {
	GRF_Arbitrary,
	GRF_dBm,
	GRF_mV,
	GRF_uV,
	GRF_CSQ,
	GRF_Percentage
} GSM_RFUnits;

/* Define enums for Battery units. */

typedef enum {
	GBU_Arbitrary,
	GBU_Volts,
	GBU_Minutes,
	GBU_Percentage
} GSM_BatteryUnits;

/* Define enums for Calendar Note types */

typedef enum {
	GCN_REMINDER=1, /* Reminder */
	GCN_CALL,       /* Call */
	GCN_MEETING,    /* Meeting */
	GCN_BIRTHDAY    /* Birthday */
} GSM_CalendarNoteType;

/* Calendar note type */

typedef struct {
	int Location;			/* The number of the note in the phone memory */
	GSM_CalendarNoteType Type;	/* The type of the note */
	GSM_DateTime Time;		/* The time of the note */
	GSM_DateTime Alarm;		/* The alarm of the note */
	char Text[20];		/* The text of the note */
	char Phone[20];		/* For Call only: the phone number */
	double Recurance;		/* Recurance of the note */
} GSM_CalendarNote;

/* This structure is provided to allow common information about the particular
   model to be looked up in a model independant way. Some of the values here
   define minimum and maximum levels for values retrieved by the various Get
   functions for example battery level. They are not defined as constants to
   allow model specific code to set them during initialisation */

typedef struct {
	 	
	char *Models; /* Models covered by this type, pipe '|' delimited. */

/* Minimum and maximum levels for RF signal strength. Units are as per the
   setting of RFLevelUnits.  The setting of RFLevelUnits indicates the 
   default or "native" units used.  In the case of the 3110 and 6110 series
   these are arbitrary, ranging from 0 to 4. */

	float MaxRFLevel;
	float MinRFLevel;
	GSM_RFUnits RFLevelUnits;

/* Minimum and maximum levels for battery level. Again, units are as per the
   setting of GSM_BatteryLevelUnits.  The value that BatteryLevelUnits is set
   to indicates the "native" or default value that the phone supports.  In the
   case of the 3110 and 6110 series these are arbitrary, ranging from 0 to 4. */

	float MaxBatteryLevel;
	float MinBatteryLevel;
	GSM_BatteryUnits BatteryLevelUnits;

/* FIXME: some very similar code is in common/misc.c */

/* Information about date, time and alarm support. In case of alarm
   information we provide value for the number of alarms supported. */

	GSM_DateTimeSupport DateTimeSupport;
	GSM_DateTimeSupport AlarmSupport;
	int MaximumAlarms;
	u8 StartupLogoH;   /* Logo Widths and Heights - if supported */
	u8 StartupLogoW;
	u8 OpLogoH;
	u8 OpLogoW;
	u8 CallerLogoH;
	u8 CallerLogoW;
} GSM_Information;

/* This enum is used for display status. */

typedef enum {
	DS_Call_In_Progress, /* Call in progress. */
	DS_Unknown,          /* The meaning is unknown now :-( */
	DS_Unread_SMS,       /* There is Unread SMS. */
	DS_Voice_Call,       /* Voice call active. */
	DS_Fax_Call,         /* Fax call active. */
	DS_Data_Call,        /* Data call active. */
	DS_Keyboard_Lock,    /* Keyboard lock status. */
	DS_SMS_Storage_Full  /* Full SMS Memory. */
} DisplayStatusEntity;

/* Bitmap types. */

typedef enum {
	GSM_None=0,
	GSM_StartupLogo,
	GSM_OperatorLogo,
	GSM_CallerLogo,
	GSM_PictureImage,
	GSM_WelcomeNoteText,
	GSM_DealerNoteText
} GSM_Bitmap_Types;

#define GSM_MAX_BITMAP_SIZE 864

/* Structure to hold incoming/outgoing bitmaps (and welcome-notes). */

typedef struct {
	u8 height;               /* Bitmap height (pixels) */
	u8 width;                /* Bitmap width (pixels) */
	u16 size;                /* Bitmap size (bytes) */
	GSM_Bitmap_Types type;   /* Bitmap type */
	char netcode[7];         /* Network operator code */
	char text[256];          /* Text used for welcome-note or callergroup name */
	char dealertext[256];    /* Text used for dealer welcome-note */
	bool dealerset;          /* Is dealer welcome-note set now ? */
	unsigned char bitmap[GSM_MAX_BITMAP_SIZE]; /* Actual Bitmap */ 
	char number;             /* Caller group number */
	char ringtone;           /* Ringtone no sent with caller group */
} GSM_Bitmap;


/* NoteValue is encoded as octave(scale)*14 + note */
/* where for note: c=0, d=2, e=4 .... */
/* ie. c#=1 and 5 and 13 are invalid */
/* note=255 means a pause */

#define MAX_RINGTONE_NOTES 256

/* Structure to hold note of ringtone. */

typedef struct {
	u8 duration;
	u8 note;
} GSM_RingtoneNote;

/* Structure to hold ringtones. */

typedef struct {
	char name[20];
	u8 tempo;
	u8 NrNotes;
	GSM_RingtoneNote notes[MAX_RINGTONE_NOTES];
} GSM_Ringtone;
  
/* Structure to hold profile entries. */

typedef struct {
	int Number;          /* The number of the profile. */
	char Name[40];       /* The name of the profile. */
	int DefaultName;     /* 0-6, when default name is used, -1, when not. */
	int KeypadTone;      /* Volume level for keypad tones. */
	int Lights;          /* Lights on/off. */
	int CallAlert;       /* Incoming call alert. */
	int Ringtone;        /* Ringtone for incoming call alert. */
	int Volume;          /* Volume of the ringing. */
	int MessageTone;     /* The tone for message indication. */
	int WarningTone;     /* The tone for warning messages. */
	int Vibration;       /* Vibration? */
	int CallerGroups;    /* CallerGroups. */
	int AutomaticAnswer; /* Does the phone auto-answer incoming call? */
} GSM_Profile;


#define FO_SUBMIT       0x01
#define FO_RD           0x40
#define FO_VPF_NONE     0x00
#define FO_VPF_REL      0x10
#define FO_VPF_ABS      0x18
#define FO_VPF_ENH      0x08
#define FO_SRR          0x20
#define FO_UDHI         0x40
#define FO_RP           0x80
#define FO_DEFAULT      (FO_SUBMIT | FO_VPF_REL | FO_SRR)

#define PID_DEFAULT     0x00
#define PID_TYPE0       0x40
#define PID_REPLACE1    0x41
#define PID_REPLACE2    0x42
#define PID_REPLACE3    0x43
#define PID_REPLACE4    0x44
#define PID_REPLACE5    0x45
#define PID_REPLACE6    0x46
#define PID_REPLACE7    0x47
#define PID_RETURN_CALL 0x5f

#define DCS_DEFAULT     0x00
#define DCS_MSG_WAIT_VOICE_DISCARD      0xc8
#define DCS_MSG_WAIT_VOICE_OFF          0xc0
#define DCS_MSG_WAIT_VOICE_STORE        0xd8
#define DCS_DATA        0xf4
#define DCS_CLASS0      0xf0
#define DCS_CLASS1      0xf1
#define DCS_CLASS2      0xf2
#define DCS_CLASS3      0xf3

/* Limits for IMEI, Revision and Model string storage. */
#define GSM_MAX_IMEI_LENGTH     (20)
#define GSM_MAX_REVISION_LENGTH (6)
#define GSM_MAX_MODEL_LENGTH    (6)



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

struct _GSM_Statemachine{
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

#endif	/* __gsm_common_h */
