/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the definitions, enums etc. that are used by all models of
  handset.

  Last modification: Thu Jul  1 21:28:36 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __gsm_common_h
#define __gsm_common_h

#ifndef __rlp_common_h
    #include "rlp-common.h"
#endif

/* Type of connection. Now we support serial connection with FBUS cable and
   IR (only with 61x0 models) */

typedef enum {
  GCT_Serial,  /* Serial connection. */
  GCT_Infrared /* Infrared connection. */
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
  GMT_XX = 0xff	/* Error code for unknown memory type (returned by fbus-xxxx functions. */
} GSM_MemoryType;

/* Power source types */

typedef enum {
  GPS_ACDC=1, /* AC/DC powered (charging) */
  GPS_BATTERY /* Internal battery */
} GSM_PowerSource;

/* This data-type is used to specify the type of the number. See the official
   GSM specification 03.40, version 5.3.0, section 9.1.2.5, page 33. */

typedef enum {
  GNT_UNKNOWN=0x81,      /* Unknown number */
  GNT_INTERNATIONAL=0x91 /* International number */
} GSM_NumberType;

/* Limits of SMS messages. */

#define GSM_MAX_SMS_CENTER_LENGTH  (40)
#define GSM_MAX_SENDER_LENGTH      (40)
#define GSM_MAX_DESTINATION_LENGTH (40)
#define GSM_MAX_SMS_LENGTH         (160)

/* Define datatype for SMS Message Type */

typedef enum {
  GST_MO, /* Mobile Originated (MO) message - Outbox message */
  GST_MT, /* Mobile Terminated (MT) message - Inbox message */
  GST_DR, /* Delivery Report */
  GST_UN  /* Unknown */
} GSM_SMSMessageType;

/* Datatype for SMS status */
/* FIXME - This needs to be made clearer and or turned into a 
   bitfield to allow compound values (read | sent etc.) */

typedef enum {
  GSS_SENTREAD = true,    /* Sent or read message */
  GSS_NOTSENTREAD = false /* Not sent or not read message */
} GSM_SMSMessageStatus;

/* Define datatype for SMS Message Center */

typedef struct {
  int No;        /* Number of the SMSC in the phone memory */
  char Name[20]; /* Name of the SMSC */
  char Number[GSM_MAX_SMS_CENTER_LENGTH]; /* Number of the SMSC */
} GSM_MessageCenter;

/* Structure used for passing dates/times to date/time functions such as
   GSM_GetTime and GSM_GetAlarm etc. */

typedef struct {
  bool AlarmEnabled; /* Is the alarm set? */
  int Year;          /* The complete year specification - e.g. 1999. Y2K :-) */
  int Month;	     /* January = 1 */
  int Day;
  int Hour;
  int Minute;
  int Second;
  int Timezone;      /* The difference between local time and GMT */
} GSM_DateTime;

/* Define datatype for SMS messages, used for getting SMS messages from the
   phones memory. */

typedef struct {
  GSM_DateTime Time;	                    /* Date of reception/response of messages. */
  int Length;                               /* Length of the SMS message. */
  int Validity;                             /* Validity Period of the SMS message. */
  char MessageText[GSM_MAX_SMS_LENGTH + 1]; /* Room for null term. */
  GSM_MessageCenter MessageCenter;          /* SMS Center. */
  char Sender[GSM_MAX_SENDER_LENGTH + 1];   /* Sender of the SMS message. */
  char Destination[GSM_MAX_DESTINATION_LENGTH + 1]; /* Destination of the message. */
  int MessageNumber;                        /* Location in the memory. */
  GSM_MemoryType MemoryType;                /* Type of memory message is stored in. */
  GSM_SMSMessageType Type;                  /* Type of the SMS message */
  GSM_SMSMessageStatus Status;              /* Status of the SMS message */
  int Class;                                /* Class Message: 0, 1, 2, 3 or none; see GSM 03.38 */
  bool EightBit;                            /* Indicates whether SMS contains 8 bit data or not */
  bool Compression;                         /* Indicates whether SMS contains compressed data or not */
  int Location;                             /* Location in the memory. */
} GSM_SMSMessage;

/* This structure is used to get the current network status */

typedef struct {
  char NetworkCode[10]; /* GSM network code */
  char CellID[10];      /* CellID */
  char LAC[10];         /* LAC */
} GSM_NetworkInfo;

/* Limits for sizing of array in GSM_PhonebookEntry. Individual handsets may
   not support these lengths so they have their own limits set. */

#define GSM_MAX_PHONEBOOK_NAME_LENGTH   (40)
#define GSM_MAX_PHONEBOOK_NUMBER_LENGTH (40)

/* Here is a macro for models that do not support caller groups. */

#define GSM_GROUPS_NOT_SUPPORTED -1

/* This data type is used to report the number of used and free positions in
   memory (sim or internal). */

typedef struct {
  GSM_MemoryType MemoryType; /* Type of the memory */
  int Used;                  /* Number of used positions */
  int Free;                  /* Number of free positions */
} GSM_MemoryStatus;

/* This data type is used to hold the current SMS status. */

typedef struct {
  int UnRead; /* The number of unread messages */
  int Number; /* The number of messages */
} GSM_SMSStatus;

/* Define datatype for phonebook entry, used for getting/writing phonebook
   entries. */

typedef struct {
  bool Empty;                                       /* Is this entry empty? */
  char Name[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1];     /* Plus 1 for null terminator. */
  char Number[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1]; /* Number */
  GSM_MemoryType MemoryType;                        /* Type of memory */
  int Group;                                        /* Group */
  int Location;                                     /* Location */
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
} GSM_RFUnits;

/* Define enums for Battery units. */

typedef enum {
  GBU_Arbitrary,
  GBU_Volts,
  GBU_Minutes
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
  int Location;              /* The number of the note in the phone memory */
  GSM_CalendarNoteType Type; /* The type of the note */
  GSM_DateTime Time;         /* The time of the note */
  GSM_DateTime Alarm;        /* The alarm of the note */
  char Text[20];             /* The text of the note */
  char Phone[20];            /* For Call only: the phone number */
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

/* Information about date, time and alarm support. In case of alarm
   information we provide value for the number of alarms supported. */

  GSM_DateTimeSupport DateTimeSupport;
  GSM_DateTimeSupport AlarmSupport;
  int MaximumAlarms;
} GSM_Information;

/* Define standard GSM error/return code values. These codes are also used for
   some internal functions such as SIM read/write in the model specific code. */

typedef enum {
  GE_NONE = 0,              /* No error. */
  GE_DEVICEOPENFAILED,	    /* Couldn't open specified serial device. */
  GE_UNKNOWNMODEL,          /* Model specified isn't known/supported. */
  GE_NOLINK,                /* Couldn't establish link with phone. */
  GE_TIMEOUT,               /* Command timed out. */
  GE_INVALIDPIN,            /* Invalid PIN */
  GE_NOTIMPLEMENTED,        /* Command called isn't implemented in model. */
  GE_INVALIDSMSLOCATION,    /* Invalid SMS location. */
  GE_INVALIDPHBOOKLOCATION, /* Invalid phonebook location. */
  GE_INVALIDMEMORYTYPE,     /* Invalid type of memory. */
  GE_INVALIDSPEEDDIALLOCATION, /* Invalid speed dial location. */
  GE_INVALIDCALNOTELOCATION,/* Invalid calendar note location. */
  GE_INVALIDDATETIME,       /* Invalid date, time or alarm specification. */
  GE_EMPTYSMSLOCATION,      /* SMS location is empty. */
  GE_PHBOOKNAMETOOLONG,     /* Phonebook name is too long. */
  GE_PHBOOKNUMBERTOOLONG,   /* Phonebook number is too long. */
  GE_PHBOOKWRITEFAILED,     /* Phonebook write failed. */
  GE_SMSSENDOK,             /* SMS was send correctly. */
  GE_SMSSENDFAILED,         /* SMS send fail. */
  GE_SMSWAITING,            /* Waiting for the next part of SMS. */
  GE_SMSTOOLONG,            /* SMS message too long. */
  GE_INTERNALERROR,         /* Problem occured internal to model specific code. */
  GE_BUSY,                  /* Command is still being executed. */

  /* The following are here in anticipation of data call requirements. */

  GE_LINEBUSY,              /* Outgoing call requested reported line busy */
  GE_NOCARRIER,             /* No Carrier error during data call setup ? */
} GSM_Error;


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
                           bool enable_monitoring,
                           void (*rlp_callback)(RLP_F96Frame *frame));

  void (*Terminate)(void);	

  GSM_Error (*GetMemoryLocation)( GSM_PhonebookEntry *entry );

  GSM_Error (*WritePhonebookLocation)( GSM_PhonebookEntry *entry );

  GSM_Error (*GetSpeedDial)( GSM_SpeedDial *entry);

  GSM_Error (*SetSpeedDial)( GSM_SpeedDial *entry);

  GSM_Error (*GetMemoryStatus)( GSM_MemoryStatus *Status);

  GSM_Error (*GetSMSStatus)( GSM_SMSStatus *Status);

  GSM_Error (*GetSMSCenter)( GSM_MessageCenter *MessageCenter );

  GSM_Error (*GetSMSMessage)( GSM_SMSMessage *Message );

  GSM_Error (*DeleteSMSMessage)( GSM_SMSMessage *Message );

  GSM_Error (*SendSMSMessage)( GSM_SMSMessage *Message );

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

  GSM_Error (*EnterPin)( char *pin );

  GSM_Error (*GetIMEI)( char *imei );

  GSM_Error (*GetRevision)( char *revision );

  GSM_Error (*GetModel)( char *model );

  GSM_Error (*GetDateTime)( GSM_DateTime *date_time);

  GSM_Error (*SetDateTime)( GSM_DateTime *date_time);

  GSM_Error (*GetAlarm)( int alarm_number, GSM_DateTime *date_time );

  GSM_Error (*SetAlarm)( int alarm_number, GSM_DateTime *date_time );

  GSM_Error (*DialVoice)( char *Number);

  GSM_Error (*DialData)( char *Number);

  GSM_Error (*GetIncomingCallNr)( char *Number );

  GSM_Error (*SendBitmap) ( char *NetworkCode, int width, int height, unsigned char *bitmap);

  GSM_Error (*GetNetworkInfo) ( GSM_NetworkInfo *NetworkInfo );

  GSM_Error (*GetCalendarNote) ( GSM_CalendarNote *CalendarNote);

  GSM_Error (*WriteCalendarNote) ( GSM_CalendarNote *CalendarNote);

  GSM_Error (*DeleteCalendarNote) ( GSM_CalendarNote *CalendarNote);

  GSM_Error (*NetMonitor) (unsigned char mode, char *Screen);

} GSM_Functions;

#endif	/* __gsm_common_h */
