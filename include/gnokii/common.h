/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Header file for the definitions, enums etc. that are used by all models of
  handset.

*/

#ifndef __gsm_common_h
#define __gsm_common_h

#include <stdlib.h>

#include "misc.h"
#include "data/rlp-common.h"

/* Type of connection. Now we support serial connection with FBUS cable and
   IR (only with 61x0 models) */
typedef enum {
	GCT_Serial,   /* Serial connection. */
	GCT_DAU9P,     /* Serial connection using DAU9P cable; use only with 6210/7110 if you want faster initialization */
	GCT_Infrared, /* Infrared connection. */
	GCT_Tekram,   /* Tekram Ir-Dongle */
	GCT_Irda,
	GCT_TCP,      /* TCP network connection */
} GSM_ConnectionType;

/* Maximum length of device name for serial port */

#define GSM_MAX_DEVICE_NAME_LENGTH (32)

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
	GMT_IN, /* Inbox for folder aware phones */
	GMT_OU, /* Outbox  */
	GMT_AR, /* Archive */
	GMT_TE, /* Templates */
	GMT_F1, /* 1st CUSTOM FOLDER */
	GMT_F2,
	GMT_F3,
	GMT_F4,
	GMT_F5,
	GMT_F6,
	GMT_F7,
	GMT_F8,
	GMT_F9,
	GMT_F10,
	GMT_F11,
	GMT_F12,
	GMT_F13,
	GMT_F14,
	GMT_F15,
	GMT_F16,
	GMT_F17,
	GMT_F18,
	GMT_F19,
	GMT_F20, /* 20th CUSTOM FOLDER */
	GMT_XX = 0xff	/* Error code for unknown memory type (returned by fbus-xxxx functions). */
} GSM_MemoryType;

/* Power source types */

typedef enum {
	GPS_ACDC = 1, /* AC/DC powered (charging) */
	GPS_BATTERY   /* Internal battery */
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
	char NewCode[10];          /* New code. */
} GSM_SecurityCode;

/* This structure is used to get the current network status */

typedef struct {
	char NetworkCode[10]; /* GSM network code */
	unsigned char CellID[10];      /* CellID */
	unsigned char LAC[10];         /* LAC */
} GSM_NetworkInfo;

/* Limits for sizing of array in GSM_PhonebookEntry. Individual handsets may
   not support these lengths so they have their own limits set. */

#define GSM_MAX_PHONEBOOK_NAME_LENGTH   (50)	/* For 7110 */
#define GSM_MAX_PHONEBOOK_NUMBER_LENGTH (48)	/* For 7110 */
#define GSM_MAX_PHONEBOOK_SUB_ENTRIES   (8)	/* For 7110 */
						/* 7110 is able to in one
						 * entry 5 numbers and 2
						 * texts [email,notice,postal] */

/* Here is a macro for models that do not support caller groups. */

#define GSM_GROUPS_NOT_SUPPORTED -1
#define GSM_MAX_CALLER_GROUPS     5

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
		char Number[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1]; /* Number */
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

typedef enum {
	GCN_NEVER   = 0,
	GCN_DAILY   = 24,
	GCN_WEEKLY  = 168,
	GCN_2WEEKLY = 336,
	GCN_YEARLY  = 65535
} GSM_CalendarRecurrences;

/* Calendar note type */

typedef struct {
	int Location;			/* The number of the note in the phone memory */
	GSM_CalendarNoteType Type;		/* The type of the note */
	GSM_DateTime Time;		/* The time of the note */
	GSM_DateTime Alarm;		/* The alarm of the note */
	char Text[20];		/* The text of the note */
	char Phone[20];		/* For Call only: the phone number */
	GSM_CalendarRecurrences Recurrence;	/* Recurrence of the note */
} GSM_CalendarNote;

/* List of Calendar Notes in phone */
#define MAX_CALENDAR_NOTES (500) /* FIXME how many are possible? */

typedef struct {
	int Number;		/* The number of notes in phone */
	int Location[MAX_CALENDAR_NOTES];	/* Location of the nth note */
} GSM_CalendarNotesList;

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
#define GSM_MAX_REVISION_LENGTH (20)
#define GSM_MAX_MODEL_LENGTH    (20)

/* Data structures for the call divert */
typedef enum {
	GSM_CDV_Busy = 0x01,
	GSM_CDV_NoAnswer,
	GSM_CDV_OutOfReach,
	GSM_CDV_NotAvailable,
	GSM_CDV_AllTypes
} GSM_CDV_DivertTypes;

typedef enum {
	GSM_CDV_VoiceCalls = 0x01,
	GSM_CDV_FaxCalls,
	GSM_CDV_DataCalls,
	GSM_CDV_AllCalls
} GSM_CDV_CallTypes;

typedef enum {
	GSM_CDV_Disable  = 0x00,
	GSM_CDV_Enable   = 0x01,
	GSM_CDV_Query    = 0x02,
	GSM_CDV_Register = 0x03,
	GSM_CDV_Erasure  = 0x04
} GSM_CDV_Opers;

/* This data-type is used to specify the type of the number. See the official
   GSM specification 03.40, version 6.1.0, section 9.1.2.5, page 35-37. */
typedef enum {
	SMS_Unknown       = 0x81, /* Unknown number */
	SMS_International = 0x91, /* International number */
	SMS_National      = 0xa1, /* National number */
	SMS_Network       = 0xb1, /* Network specific number */
	SMS_Subscriber    = 0xc1, /* Subscriber number */
	SMS_Alphanumeric  = 0xd0, /* Alphanumeric number */
	SMS_Abbreviated   = 0xe1  /* Abbreviated number */
} SMS_NumberType;

#define MAX_BCD_STRING_LENGTH		40

typedef struct {
	SMS_NumberType Type;
	char Number[MAX_BCD_STRING_LENGTH];
} SMS_Number;

typedef struct {
	GSM_CDV_DivertTypes DType;
	GSM_CDV_CallTypes   CType;
	GSM_CDV_Opers       Operation;
	SMS_Number          Number;
	unsigned int        Timeout;
} GSM_CallDivert;

typedef struct {
	bool Full; /* indicates if we have full data read */
	unsigned int Length;
	unsigned char *Data;
} GSM_RawData;

typedef enum {
	GSM_Draw_ClearScreen,
	GSM_Draw_DisplayText,
	GSM_Draw_DisplayStatus
} GSM_DrawCommand;

#define	DRAW_MAX_SCREEN_WIDTH 27
#define	DRAW_MAX_SCREEN_HEIGHT 6

typedef struct {
	int x;
	int y;
	unsigned char text[DRAW_MAX_SCREEN_WIDTH + 1];
} GSM_DrawData_DisplayText;

typedef struct {
	GSM_DrawCommand Command;
	union {
		GSM_DrawData_DisplayText DisplayText;
		int DisplayStatus;
	} Data;
} GSM_DrawMessage;

typedef struct {
	void (*OutputFn)(GSM_DrawMessage *DrawMessage);
	int State;
	struct timeval Last;
} GSM_DisplayOutput;

typedef struct {
	int Field;
	char Screen[50];
} GSM_NetMonitor;

/* Data structure for make, answer and cancel a call */
typedef enum {
	GSM_CT_VoiceCall,		/* Voice call */
	GSM_CT_NonDigitalDataCall,	/* Data call on non digital line */
	GSM_CT_DigitalDataCall		/* Data call on digital line */
} GSM_CallType;

typedef enum {
	GSM_CSN_Never,			/* Never send my number */
	GSM_CSN_Always,			/* Always send my number */
	GSM_CSN_Default			/* Use the network default settings */
} GSM_CallSendNumber;

typedef enum {
	GSM_CS_IncomingCall,		/* Incoming call */
	GSM_CS_LocalHangup,		/* Local end terminated the call */
	GSM_CS_RemoteHangup,		/* Remote end terminated the call */
	GSM_CS_Established,		/* Remote end answered the call */
	GSM_CS_CallHeld,		/* Call placed on hold */
	GSM_CS_CallResumed		/* Held call resumed */
} GSM_CallStatus;

typedef struct {
	GSM_CallType Type;
	char Number[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1];
	char Name[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1];
	GSM_CallSendNumber SendNumber;
	int CallID;
} GSM_CallInfo;

typedef enum {
	GSM_KEY_NONE = 0x00,
	GSM_KEY_1 = 0x01,
	GSM_KEY_2,
	GSM_KEY_3,
	GSM_KEY_4,
	GSM_KEY_5,
	GSM_KEY_6,
	GSM_KEY_7,
	GSM_KEY_8,
	GSM_KEY_9,
	GSM_KEY_0,
	GSM_KEY_HASH,
	GSM_KEY_ASTERISK,
	GSM_KEY_POWER,
	GSM_KEY_GREEN,
	GSM_KEY_RED,
	GSM_KEY_INCREASEVOLUME,
	GSM_KEY_DECREASEVOLUME,
	GSM_KEY_UP = 0x17,
	GSM_KEY_DOWN,
	GSM_KEY_MENU,
	GSM_KEY_NAMES
} GSM_KeyCode;

#endif	/* __gsm_common_h */
