/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the definitions, enums etc. that are used by all models of
  handset.

  Last modification: Thu May  6 00:51:48 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __gsm_common_h
#define __gsm_common_h

/* Maximum length of device name for serial port */

#define GSM_MAX_DEVICE_NAME_LENGTH (100)

/* Define an enum for specifying memory types for retrieving phonebook
   entries, SMS messages etc. This type is not mobile specific - the model
   code should take care of translation to mobile specific numbers - see 6110
   code */

typedef enum {
  GMT_INTERNAL, /* Internal memory of the mobile equipment */
  GMT_SIM       /* SIM card memory */
} GSM_MemoryType;

/* Power source types */

typedef enum {
  GPS_ACDC=1,     /* AC/DC powered (charging) */
  GPS_BATTERY /* Internal battery */
} GSM_PowerSource;

/* Limits of SMS messages. */

#define GSM_MAX_SMS_CENTRE_LENGTH (40)
#define GSM_MAX_SENDER_LENGTH (40)
#define GSM_MAX_SMS_LENGTH (160)

/* Define datatype for SMS messages, used for getting SMS messages from the
   phones memory. */

typedef struct {
  int Year, Month, Day;	                    /* Date of reception of messages. */
  int Hour, Minute, Second;                 /* Time of reception of messages. */
  int Length;                               /* Length of the SMS message. */
  char MessageText[GSM_MAX_SMS_LENGTH + 1]; /* Room for null term. */
  char MessageCentre[GSM_MAX_SMS_CENTRE_LENGTH + 1]; /* SMS Centre. */
  char Sender[GSM_MAX_SENDER_LENGTH + 1];   /* Sender of the SMS message. */
  int MessageNumber;                        /* Location in the memory. */
  GSM_MemoryType MemoryType;                /* Type of memory message is stored in. */
} GSM_SMSMessage;

/* Limits for sizing of array in GSM_PhonebookEntry. Individual handsets may
   not support these lengths so they have their own limits set. */

#define GSM_MAX_PHONEBOOK_NAME_LENGTH (40)
#define GSM_MAX_PHONEBOOK_NUMBER_LENGTH (40)

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
  int Group;                                        /* Group */
} GSM_PhonebookEntry;

/* Define enum used to describe what sort of date/time support is
   available. */

typedef enum {
  GDT_None,     /* The mobile phone doesn't support time and date. */
  GDT_TimeOnly, /* The mobile phone supports only time. */
  GDT_DateOnly, /* The mobile phone supports only date. */
  GDT_DateTime  /* Wonderful phone - it supports date and time. */
} GSM_DateTimeSupport;

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
} GSM_DateTime;


/* Define enums for RF units. */

typedef enum {
  GRF_Arbitrary,
  GRF_dBm,
  GRF_mV,
  GRF_uV
} GSM_RFUnits;

/* Define enums for RF units. */

typedef enum {
  GBU_Arbitrary,
  GBU_Volts,
  GBU_Minutes
} GSM_BatteryUnits;

/* This structure is provided to allow common information about the particular
   model to be looked up in a model independant way. Some of the values here
   define minimum and maximum levels for values retrieved by the various Get
   functions for example battery level. They are not defined as constants to
   allow model specific code to set them during initialisation */

typedef struct {
	 	
  char *Models; /* Models covered by this type, pipe '|' delimited. */

/* Minimum and maximum levels for RF signal strength. Units are as per the
   setting of RFLevelUnits. */

  float MaxRFLevel;
  float MinRFLevel;
  GSM_RFUnits RFLevelUnits;

/* Minimum anx maximum levels for battery level. Again, units are as per the
   setting of GSM_BatteryLevelUnits. */

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

/* Define the structure used to hold pointers to the various API functions.
   This is in effect the master list of functions provided by the gnokii API.
   Modules containing the model specific code each contain one of these
   structures which is "filled in" with the corresponding function within the
   model specific code.  If a function is not supported or not implemented, a
   generic not implemented function is used to return a GE_NOTIMPLEMENTED
   error code. */

typedef struct {

  /* FIXME: comment this. */

  GSM_Error (*Initialise)( char *port_device, bool enable_monitoring );

  void (*Terminate)(void);	

  GSM_Error (*GetPhonebookLocation)( GSM_MemoryType memory_type,
				     int location, GSM_PhonebookEntry *entry );

  GSM_Error (*WritePhonebookLocation)( GSM_MemoryType memory_type,
				       int location, GSM_PhonebookEntry *entry );

  GSM_Error (*GetMemoryStatus)( GSM_MemoryStatus *Status);

  GSM_Error (*GetSMSStatus)( GSM_SMSStatus *Status);

  GSM_Error (*GetSMSMessage)( GSM_MemoryType memory_type, int location,
			      GSM_SMSMessage *message );

  GSM_Error (*DeleteSMSMessage)( GSM_MemoryType memory_type,
				int location, GSM_SMSMessage *message );

  GSM_Error (*SendSMSMessage)( char *message_centre, char *destination, char *text);

  GSM_Error (*GetRFLevel)( float *level );

  GSM_Error (*GetBatteryLevel)( float *level );

  GSM_Error (*GetPowerSource)( GSM_PowerSource *source);

  GSM_Error (*EnterPin)( char *pin );

  GSM_Error (*GetIMEIAndCode)( char *imei, char *code );

  GSM_Error (*GetDateTime)( GSM_DateTime *date_time);

  GSM_Error (*SetDateTime)( GSM_DateTime *date_time);

  GSM_Error (*GetAlarm)( int alarm_number, GSM_DateTime *date_time );

  GSM_Error (*SetAlarm)( int alarm_number, GSM_DateTime *date_time );

  GSM_Error (*DialVoice)( char *Number);

  GSM_Error (*GetIncomingCallNr)( char *Number );
} GSM_Functions;

#endif	/* __gsm_common_h */
