/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the definitions, enums etc. that are used by all models of
  handset.

  Last modification: Wed Apr  5 18:11:51 CEST 2000
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
  GMT_CB, /* Currently selected memory */
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


/* Maximum length of SMS center name */

#define GSM_MAX_SMS_CENTER_NAME_LENGTH	(20)

/* Limits of SMS messages. */

#define GSM_MAX_SMS_CENTER_LENGTH  (40)
#define GSM_MAX_SENDER_LENGTH      (40)
#define GSM_MAX_DESTINATION_LENGTH (40)
#define GSM_MAX_SMS_LENGTH         (160)

/* The maximum length of an uncompressed concatenated short message is
   255 * 153 = 39015 default alphabet characters */
#define GSM_MAX_CONCATENATED_SMS_LENGTH	(39015)

/* FIXME: what value should be here? (Pawel Kot) */
#define GSM_MAX_USER_DATA_HEADER_LENGTH (10)

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

/* SMS Messages sent as... */

typedef enum {
  GSMF_Text = 0x00,   /* Plain text message. */
  GSMF_Fax = 0x22,    /* Fax message. */
  GSMF_Voice = 0x24,  /* Voice mail message */
  GSMF_ERMES = 0x25,  /* ERMES message */
  GSMF_Paging = 0x26, /* Paging. */
  GSMF_UCI = 0x2d,    /* Email message in 8110i */
  GSMF_Email = 0x32,  /* Email message. */
  GSMF_X400 = 0x31    /* X.400 message. */
} GSM_SMSMessageFormat;

/* Validity of SMS Messages. */

typedef enum {
  GSMV_1_Hour   = 0x0b,
  GSMV_6_Hours  = 0x47,
  GSMV_24_Hours = 0xa7,
  GSMV_72_Hours = 0xa9,
  GSMV_1_Week   = 0xad,
  GSMV_Max_Time = 0xff
} GSM_SMSMessageValidity;

/* Define datatype for SMS Message Center */

typedef struct {
  int No;        /* Number of the SMSC in the phone memory */
  char Name[GSM_MAX_SMS_CENTER_NAME_LENGTH]; /* Name of the SMSC */
  GSM_SMSMessageFormat Format; /* SMS is sent as text/fax/paging/email. */
  GSM_SMSMessageValidity Validity; /* Validity of SMS Message. */
  char Number[GSM_MAX_SMS_CENTER_LENGTH]; /* Number of the SMSC */
} GSM_MessageCenter;

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

/* types of User Data Header */
typedef enum {
  GSM_NoUDH,
  GSM_ConcatenatedMessages,
  GSM_OpLogo,
  GSM_CallerIDLogo,
  GSM_Ringtone
} GSM_UDH;

/* Define datatype for SMS messages, used for getting SMS messages from the
   phones memory. */

typedef struct {
  GSM_DateTime Time;	                    /* Date of reception/response of messages. */
  GSM_DateTime SMSCTime;	            /* Date of SMSC response if DeliveryReport messages. */
  int Length;                               /* Length of the SMS message. */
  int Validity;                             /* Validity Period of the SMS message. */
  GSM_UDH UDHType;                          /* If UDH is present - type of UDH */
  char UDH[GSM_MAX_USER_DATA_HEADER_LENGTH]; /* If UDH is present - content of UDH */
  char MessageText[GSM_MAX_SMS_LENGTH + 1]; /* Room for null term. */
  GSM_MessageCenter MessageCenter;          /* SMS Center. */
  char Sender[GSM_MAX_SENDER_LENGTH + 1];   /* Sender of the SMS message. */
  char Destination[GSM_MAX_DESTINATION_LENGTH + 1]; /* Destination of the message. */
  int MessageNumber;                        /* Location in the memory. */
  GSM_MemoryType MemoryType;                /* Type of memory message is stored in. */
  GSM_SMSMessageType Type;                  /* Type of the SMS message */
  GSM_SMSMessageStatus Status;              /* Status of the SMS message */
  int Class;                                /* Class Message: 0, 1, 2, 3 or none; see GSM 03.38 */
  bool EightBit;                            /* Indicates whether SMS contains 8 bit data */
  bool Compression;                         /* Indicates whether SMS contains compressed data */
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
  GSM_DateTime Date;                                /* The record date and time
                                                       of the number. */
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
  GE_TRYAGAIN,              /* Try again. */
  GE_INVALIDSECURITYCODE,   /* Invalid Security code. */
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
  GE_UNKNOWN,               /* Unknown error - well better than nothing!! */

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

/* Bitmap types. */

typedef enum {
  GSM_None=0,
  GSM_StartupLogo,
  GSM_OperatorLogo,
  GSM_CallerLogo
} GSM_Bitmap_Types;

/* Structure to hold incoming/outgoing bitmaps (and welcome-notes). */

typedef struct {
  u8 height;               /* Bitmap height (pixels) */
  u8 width;                /* Bitmap width (pixels) */
  u16 size;                /* Bitmap size (bytes) */
  GSM_Bitmap_Types type;   /* Bitmap type */
  char netcode[7];         /* Network operator code */
  char text[256];          /* Text used for welcome-note or callergroup name */
  char dealertext[256];    /* Text used for dealer welcome-note */
  unsigned char bitmap[504]; /* Actual Bitmap (84*48/8=504) */ 
  char number;             /* Caller group number */
  char ringtone;           /* Ringtone no sent with caller group */
} GSM_Bitmap;

/* Structure to hold profile entries. */

typedef struct {
  int Number;          /* The number of the profile. */
  char Name[40];       /* The name of the profile. */
  int KeypadTone;      /* Volumen level for keypad tones. */
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

  GSM_Error (*GetSMSStatus)( GSM_SMSStatus *Status);

  GSM_Error (*GetSMSCenter)( GSM_MessageCenter *MessageCenter );

  GSM_Error (*SetSMSCenter)( GSM_MessageCenter *MessageCenter );

  GSM_Error (*GetSMSMessage)( GSM_SMSMessage *Message );

  GSM_Error (*DeleteSMSMessage)( GSM_SMSMessage *Message );

  GSM_Error (*SendSMSMessage)( GSM_SMSMessage *Message, int size );

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

  GSM_Error (*GetDateTime)( GSM_DateTime *date_time);

  GSM_Error (*SetDateTime)( GSM_DateTime *date_time);

  GSM_Error (*GetAlarm)( int alarm_number, GSM_DateTime *date_time );

  GSM_Error (*SetAlarm)( int alarm_number, GSM_DateTime *date_time );

  GSM_Error (*DialVoice)( char *Number);

  GSM_Error (*DialData)( char *Number, char type);

  GSM_Error (*GetIncomingCallNr)( char *Number );

  GSM_Error (*GetNetworkInfo) ( GSM_NetworkInfo *NetworkInfo );

  GSM_Error (*GetCalendarNote) ( GSM_CalendarNote *CalendarNote);

  GSM_Error (*WriteCalendarNote) ( GSM_CalendarNote *CalendarNote);

  GSM_Error (*DeleteCalendarNote) ( GSM_CalendarNote *CalendarNote);

  GSM_Error (*NetMonitor) ( unsigned char mode, char *Screen );

  GSM_Error (*SendDTMF) ( char *String );

  GSM_Error (*GetBitmap) ( GSM_Bitmap *Bitmap );
  
  GSM_Error (*SetBitmap) ( GSM_Bitmap *Bitmap );

  GSM_Error (*Reset) ( unsigned char type );

  GSM_Error (*GetProfile) ( GSM_Profile *Profile );

  GSM_Error (*SetProfile) ( GSM_Profile *Profile );
  
  bool      (*SendRLPFrame) ( RLP_F96Frame *frame, bool out_dtx );

  GSM_Error (*CancelCall) ();

} GSM_Functions;

#endif	/* __gsm_common_h */
