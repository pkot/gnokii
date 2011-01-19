/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia the phones.

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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001-2004 Pawel Kot, BORBELY Zoltan
  Copyright (C) 2001      Manfred Jonsson, Marian Jancar, Chris Kemp, Marcin Wiacek
  Copyright (C) 2001-2002 Pavel Machek
  Copyright (C) 2002      Markus Plail

  Header file for the definitions, enums etc. that are used by all models of
  handset.

*/

#ifndef _gnokii_common_h
#define _gnokii_common_h

#include <gnokii/rlp-common.h>

/* Type of connection. Now we support serial connection with FBUS cable and
   IR (only with 61x0 models) */
typedef enum {
	GN_CT_NONE = -1,/* no connection type (error) */
	GN_CT_Serial,   /* Serial connection. */
	GN_CT_DAU9P,    /* Serial connection using DAU9P cable; use only with 6210/7110 if you want faster initialization */
	GN_CT_DLR3P,    /* Serial connection using DLR3P cable */
	GN_CT_Infrared, /* Infrared connection. */
	GN_CT_Irda,     /* Linux IrDA support */
	GN_CT_Bluetooth,/* Linux Bluetooth support */
	GN_CT_Tekram,   /* Tekram Ir-Dongle */
	GN_CT_TCP,      /* TCP network connection */
	GN_CT_M2BUS,	/* Serial connection with M2BUS protocol */
	GN_CT_DKU2,	/* DKU2 usb connection using nokia_dku2 kernel driver */
	GN_CT_DKU2LIBUSB, /* DKU2 usb connection using libusb */
	GN_CT_PCSC,     /* PC/SC SIM Card reader using libpsclite */
	GN_CT_SOCKETPHONET /* Linux PHONET kernel driver */
} gn_connection_type;

GNOKII_API const char *gn_connection_type2str(gn_connection_type t);

/* Maximum length of device name for serial port */

#define GN_DEVICE_NAME_MAX_LENGTH (32)

/* Define an enum for specifying memory types for retrieving phonebook
   entries, SMS messages etc. This type is not mobile specific - the model
   code should take care of translation to mobile specific numbers - see 6110
   code.
   01/07/99:  Two letter codes follow GSM 07.07 release 6.2.0
*/
typedef enum {
	GN_MT_ME, /* Internal memory of the mobile equipment */
	GN_MT_SM, /* SIM card memory */
	GN_MT_FD, /* Fixed dial numbers */
	GN_MT_ON, /* Own numbers */
	GN_MT_EN, /* Emergency numbers */
	GN_MT_DC, /* Dialled numbers */
	GN_MT_RC, /* Received numbers */
	GN_MT_MC, /* Missed numbers */
	GN_MT_LD, /* Last dialed */
	GN_MT_BD, /* Barred Dialing Numbers */
	GN_MT_SD, /* Service Dialing Numbers */
	GN_MT_MT, /* combined ME and SIM phonebook */
	GN_MT_TA, /* for compatibility only: TA=computer memory */
	GN_MT_CB, /* Currently selected memory */
	GN_MT_IN, /* Inbox for folder aware phones */
	GN_MT_OU, /* Outbox, sent items */
	GN_MT_AR, /* Archive */
	GN_MT_TE, /* Templates */
	GN_MT_SR, /* Status reports */
	GN_MT_DR, /* Drafts */
	GN_MT_OUS, /* Outbox, items to be sent */
	GN_MT_F1, /* 1st CUSTOM FOLDER */
	GN_MT_F2,
	GN_MT_F3,
	GN_MT_F4,
	GN_MT_F5,
	GN_MT_F6,
	GN_MT_F7,
	GN_MT_F8,
	GN_MT_F9,
	GN_MT_F10,
	GN_MT_F11,
	GN_MT_F12,
	GN_MT_F13,
	GN_MT_F14,
	GN_MT_F15,
	GN_MT_F16,
	GN_MT_F17,
	GN_MT_F18,
	GN_MT_F19,
	GN_MT_F20, /* 20th CUSTOM FOLDER */
	GN_MT_BM, /* Cell Broadcast Messages */
	GN_MT_LAST = GN_MT_BM,
	GN_MT_XX = 0xff	/* Error code for unknown memory type (returned by fbus-xxxx functions). */
} gn_memory_type;

GNOKII_API gn_memory_type gn_str2memory_type(const char *s);
GNOKII_API const char *gn_memory_type2str(gn_memory_type mt);

/* Power source types - see subclause 8.4 of ETSI TS 127 007 V8.6.0 (2009-01) */
typedef enum {
	GN_PS_UNKNOWN,		/* +CME ERROR: ... */
	GN_PS_ACDC = 1,		/* +CBC: 1,... AC/DC powered (charging) */
	GN_PS_BATTERY,		/* +CBC: 0,... Battery powered (not charging) */
	GN_PS_NOBATTERY,	/* +CBC: 2,... No battery */
	GN_PS_FAULT		/* +CBC: 3,... There is a power fault and calls are inhibited */
} gn_power_source;

GNOKII_API const char *gn_power_source2str(gn_power_source s);

/* Definition of security codes. */
typedef enum {
	GN_SCT_SecurityCode = 0x01, /* Security code. */
	GN_SCT_Pin,                 /* PIN. */
	GN_SCT_Pin2,                /* PIN 2. */
	GN_SCT_Puk,                 /* PUK. */
	GN_SCT_Puk2,                /* PUK 2. */
	GN_SCT_None                 /* Code not needed. */
} gn_security_code_type;

GNOKII_API const char *gn_security_code_type2str(gn_security_code_type t);

/* Security code definition. */
typedef struct {
	gn_security_code_type type; /* Type of the code. */
	char code[10];              /* Actual code. */
	char new_code[10];          /* New code. */
} gn_security_code;

/* This structure is used to get the current network status */
typedef struct {
	char network_code[10];     /* GSM network code */
	unsigned char cell_id[10]; /* CellID */
	unsigned char LAC[10];     /* LAC */
} gn_network_info;

/* Limits for sizing of array in gn_phonebook_entry. Individual handsets may
   not support these lengths so they have their own limits set. */
#define GN_PHONEBOOK_NAME_MAX_LENGTH            61   /* For 6510 */
#define GN_PHONEBOOK_NUMBER_MAX_LENGTH          49   /* For 6510 */
#define GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER      64   /* it was 10 for 6510, but modern phones supports more */
						     /* 7110 is able to have in one
						      * entry 5 numbers and 4
						      * texts [email,notice,postal,url].
						      * Dirk reported on gnokii-users that N70 has the following additional fields:
						      * - Company
						      * - Position
						      * - Aliasname
						      * - DTMF
						      * - User ID 
						      * - all postal fields:
						      *  - PO Box
						      *  - Addition
						      *  - Street
						      *  - Zip Code
						      *  - City
						      *  - State/Province
						      *  - Country
						      *  - Birthday
						      */
#define GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER    5
#define GN_PHONEBOOK_ENTRY_MAX_LENGTH 1024

/* This data type is used to report the number of used and free positions in
   memory (sim or internal). */
typedef struct {
	gn_memory_type memory_type; /* Type of the memory */
	int used;                   /* Number of used positions */
	int free;                   /* Number of free positions */
} gn_memory_status;

/* General date and time structure. It is used for the SMS, calendar, alarm
 * settings, clock etc. */
typedef struct {
	int year;           /* The complete year specification - e.g. 1999. Y2K :-) */
	int month;          /* January = 1 */
	int day;
	int hour;
	int minute;
	int second;
	int timezone;      /* The difference between local time and GMT.
			      Note that different SMSC software treat this field
			      in the different ways. */
} gn_timestamp;

/* Some phones (at the moment 7110/6510 series) supports extended phonebook
   with additional data.  Here we have structures for them */
typedef enum {
	GN_PHONEBOOK_NUMBER_None    = 0x00,
	GN_PHONEBOOK_NUMBER_Common  = 0x01,
	GN_PHONEBOOK_NUMBER_Home    = 0x02,
	GN_PHONEBOOK_NUMBER_Mobile  = 0x03,
	GN_PHONEBOOK_NUMBER_Fax     = 0x04,
	GN_PHONEBOOK_NUMBER_Work    = 0x06,
	GN_PHONEBOOK_NUMBER_General = 0x0a,
} gn_phonebook_number_type;

GNOKII_API const char *gn_phonebook_number_type2str(gn_phonebook_number_type t);

typedef enum {
	GN_PHONEBOOK_ENTRY_Name            = 0x07,
	GN_PHONEBOOK_ENTRY_Email           = 0x08,
	GN_PHONEBOOK_ENTRY_Postal          = 0x09,
	GN_PHONEBOOK_ENTRY_Note            = 0x0a,
	GN_PHONEBOOK_ENTRY_Number          = 0x0b,
	GN_PHONEBOOK_ENTRY_Ringtone        = 0x0c, /* Ringtone */
	GN_PHONEBOOK_ENTRY_Date            = 0x13, /* Date is used for DC,RC,etc (last calls) */
	GN_PHONEBOOK_ENTRY_Pointer         = 0x1a, /* Pointer to the other memory */
	GN_PHONEBOOK_ENTRY_Logo            = 0x1b, /* Bitmap */
	GN_PHONEBOOK_ENTRY_LogoSwitch      = 0x1c,
	GN_PHONEBOOK_ENTRY_Group           = 0x1e, /* Octect */
	GN_PHONEBOOK_ENTRY_URL             = 0x2c,
	GN_PHONEBOOK_ENTRY_Location        = 0x2f, /* Octect */
	GN_PHONEBOOK_ENTRY_Image           = 0x33, /* File ID */
	GN_PHONEBOOK_ENTRY_RingtoneAdv     = 0x37, /* File ID or Ringtone */
	GN_PHONEBOOK_ENTRY_UserID          = 0x38,
	GN_PHONEBOOK_ENTRY_PTTAddress      = 0x3f,
	GN_PHONEBOOK_ENTRY_ExtGroup        = 0x43,
	GN_PHONEBOOK_ENTRY_Video           = 0x45, /* File ID */
	GN_PHONEBOOK_ENTRY_FirstName       = 0x46,
	GN_PHONEBOOK_ENTRY_LastName        = 0x47,
	GN_PHONEBOOK_ENTRY_PostalAddress   = 0x4a,
	GN_PHONEBOOK_ENTRY_ExtendedAddress = 0x4b,
	GN_PHONEBOOK_ENTRY_Street          = 0x4c,
	GN_PHONEBOOK_ENTRY_City            = 0x4d,
	GN_PHONEBOOK_ENTRY_StateProvince   = 0x4e,
	GN_PHONEBOOK_ENTRY_ZipCode         = 0x4f,
	GN_PHONEBOOK_ENTRY_Country         = 0x50,
	GN_PHONEBOOK_ENTRY_FormalName      = 0x52,
	GN_PHONEBOOK_ENTRY_JobTitle        = 0x54,
	GN_PHONEBOOK_ENTRY_Company         = 0x55,
	GN_PHONEBOOK_ENTRY_Nickname        = 0x56,
	GN_PHONEBOOK_ENTRY_Birthday        = 0x57, /* Date */
} gn_phonebook_entry_type;

GNOKII_API const char *gn_phonebook_entry_type2str(gn_phonebook_entry_type t);
GNOKII_API const char *gn_subentrytype2string(gn_phonebook_entry_type entry_type, gn_phonebook_number_type number_type);

typedef enum {
	GN_PHONEBOOK_GROUP_Family,
	GN_PHONEBOOK_GROUP_Vips,
	GN_PHONEBOOK_GROUP_Friends,
	GN_PHONEBOOK_GROUP_Work,
	GN_PHONEBOOK_GROUP_Others, 
	GN_PHONEBOOK_GROUP_None,
} gn_phonebook_group_type;

GNOKII_API const char *gn_phonebook_group_type2str(gn_phonebook_group_type t);

#define GN_PHONEBOOK_PERSON_MAX_LENGTH 64

typedef struct {
	int has_person;
	char family_name[GN_PHONEBOOK_PERSON_MAX_LENGTH + 1];		/* GN_PHONEBOOK_ENTRY_LastName */
	char given_name[GN_PHONEBOOK_PERSON_MAX_LENGTH + 1];            /* GN_PHONEBOOK_ENTRY_FirstName */
	char additional_names[GN_PHONEBOOK_PERSON_MAX_LENGTH + 1];
	char honorific_prefixes[GN_PHONEBOOK_PERSON_MAX_LENGTH + 1];    /* GN_PHONEBOOK_ENTRY_FormalName */
	char honorific_suffixes[GN_PHONEBOOK_PERSON_MAX_LENGTH + 1];
} gn_phonebook_person;

#define GN_PHONEBOOK_ADDRESS_MAX_LENGTH 64

typedef struct {
	int has_address;
	char post_office_box[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];	/* GN_PHONEBOOK_ENTRY_PostalAddress */
	char extended_address[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];	/* GN_PHONEBOOK_ENTRY_ExtendedAddress */
	char street[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];		/* GN_PHONEBOOK_ENTRY_Street */
	char city[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];			/* GN_PHONEBOOK_ENTRY_City */
	char state_province[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];	/* GN_PHONEBOOK_ENTRY_StateProvince */
	char zipcode[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];		/* GN_PHONEBOOK_ENTRY_ZipCode */
	char country[GN_PHONEBOOK_ADDRESS_MAX_LENGTH + 1];		/* GN_PHONEBOOK_ENTRY_Country */
} gn_phonebook_address;

typedef struct {
	gn_phonebook_entry_type entry_type;
	gn_phonebook_number_type number_type;
	union {
		char number[GN_PHONEBOOK_NAME_MAX_LENGTH + 1];   /* Number, Name, Address, eMail... */
		/* GN_PHONEBOOK_ENTRY_Date
		 * GN_PHONEBOOK_ENTRY_Birthday
		 */
		gn_timestamp date;                               /* or the last calls list */
		/* GN_PHONEBOOK_ENTRY_Image
		 * GN_PHONEBOOK_ENTRY_RingtoneAdv
		 */
		char fileid[6];                                  /* Bitmap or ringtone fileid */
		/* GN_PHONEBOOK_ENTRY_ExtGroup
		 */
		int id;                                          /* Any number, like ID */
	} data;
	int id;
} gn_phonebook_subentry;

/* Define datatype for phonebook entry, used for getting/writing phonebook
   entries. */
typedef struct {
	int empty;                                        /* Is this entry empty? */
	char name[GN_PHONEBOOK_NAME_MAX_LENGTH + 1];      /* Plus 1 for
							     nullterminator. */
	char number[GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1];  /* Number */
	gn_memory_type memory_type;                       /* Type of memory */
	gn_phonebook_group_type caller_group;             /* Caller group - gn_phonebook_group_type */
	int location;                                     /* Location */
	gn_timestamp date;                                /* The record date and time
							     of the number. */
	gn_phonebook_person person;                       /* Personal information */
	gn_phonebook_address address;                     /* Address information */
	gn_phonebook_subentry subentries[GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER];
	/* For phones with
	 * additional phonebook
	 * entries */
	int subentries_count;                             /* Should be set to 0, if extended
							     phonebook is not used */
} gn_phonebook_entry;

/* This define speed dialing entries. */
typedef struct {
	int number;                 /* Which number is used to dialing? */
	gn_memory_type memory_type; /* Memory type of the number. */
	int location;               /* Location of the number in MemoryType. */
} gn_speed_dial;

/* Define enum used to describe what sort of date/time support is
   available. */
typedef enum {
	GN_DT_None,     /* The mobile phone doesn't support time and date. */
	GN_DT_TimeOnly, /* The mobile phone supports only time. */
	GN_DT_DateOnly, /* The mobile phone supports only date. */
	GN_DT_DateTime  /* Wonderful phone - it supports date and time. */
} gn_datetime_support;

/* Define enums for RF units. GRF_CSQ asks for units in form used
   in AT+CSQ command as defined by GSM 07.07 */
typedef enum {
	GN_RF_Arbitrary,
	GN_RF_dBm,
	GN_RF_mV,
	GN_RF_uV,
	GN_RF_CSQ,
	GN_RF_Percentage
} gn_rf_unit;

/* Define enums for Battery units. */
typedef enum {
	GN_BU_Arbitrary,
	GN_BU_Volts,
	GN_BU_Minutes,
	GN_BU_Percentage
} gn_battery_unit;

/* Define enums for Calendar Note types */
typedef enum {
	GN_CALNOTE_MEETING  = 0x01, /* Meeting */
	GN_CALNOTE_CALL     = 0x02, /* Call */
	GN_CALNOTE_BIRTHDAY = 0x04, /* Birthday */
	GN_CALNOTE_REMINDER = 0x08, /* Reminder */
	GN_CALNOTE_MEMO     = 0x16, /* Memo */
} gn_calnote_type;

GNOKII_API const char *gn_calnote_type2str(gn_calnote_type t);

typedef enum {
	GN_CALNOTE_NEVER   = 0,
	GN_CALNOTE_DAILY   = 24,
	GN_CALNOTE_WEEKLY  = 168,
	GN_CALNOTE_2WEEKLY = 336,
	GN_CALNOTE_MONTHLY = 65534,
	GN_CALNOTE_YEARLY  = 65535
} gn_calnote_recurrence;

GNOKII_API const char *gn_calnote_recurrence2str(gn_calnote_recurrence r);

#define GN_CALNOTE_MAX_NUMBER       1024 /* FIXME how many are possible? */
#define GN_CALNOTE_MAX_LENGTH        258
#define GN_CALNOTE_NUMBER_MAX_LENGTH  49

typedef struct {
	int enabled; /* Is alarm set? */
	int tone; /* Is alarm tone enabled? */
	gn_timestamp timestamp;
} gn_calnote_alarm;

/* Calendar note type */
typedef struct {
	int location;                                    /* The number of the note in the phone memory */
	gn_calnote_type type;                            /* The type of the note */
	gn_timestamp time;                               /* The time of the note */
	gn_timestamp end_time;                           /* The end time of the note */
	gn_calnote_alarm alarm;                          /* The alarm of the note */
	char text[GN_CALNOTE_MAX_LENGTH];                /* The text of the note */
	char phone_number[GN_CALNOTE_NUMBER_MAX_LENGTH]; /* For Call only: the phone number */
	char mlocation[GN_CALNOTE_MAX_LENGTH];           /* For Meeting only: the location field */
	gn_calnote_recurrence recurrence;                /* Recurrence of the note */
	int occurrences;                                 /* Number of recurrence events; 0 for infinity */
} gn_calnote;

/* List of Calendar Notes in phone */
typedef struct {
	unsigned int number;                          /* The number of notes in phone */
	unsigned int location[GN_CALNOTE_MAX_NUMBER]; /* Location of the nth note */
	unsigned int last;                            /* Index of the last allocated note */
} gn_calnote_list;

/* ToDo things. It is only supported by the newer phones. */
#define GN_TODO_MAX_LENGTH	256
#define GN_TODO_MAX_NUMBER	512

typedef enum {
	GN_TODO_LOW = 3,
	GN_TODO_MEDIUM = 2,
	GN_TODO_HIGH = 1
} gn_todo_priority;

GNOKII_API const char *gn_todo_priority2str(gn_todo_priority p);

typedef struct {
	int location;			/* The number of the note in the phone memory */
	char text[GN_TODO_MAX_LENGTH];		/* The text of the note */
	gn_todo_priority priority;
} gn_todo;

/* List of ToDo Notes in phone */
typedef struct {
	int number;                       /* The number of notes in phone */
	int location[GN_TODO_MAX_NUMBER]; /* Location of the nth note */
} gn_todo_list;


/* WAP */
#define WAP_URL_MAX_LENGTH              258
#define WAP_NAME_MAX_LENGTH              52
#define WAP_SETTING_USERNAME_MAX_LENGTH  34
#define WAP_SETTING_NAME_MAX_LENGTH      22
#define WAP_SETTING_HOME_MAX_LENGTH      95
#define WAP_SETTING_APN_MAX_LENGTH      102

/* bookmarks */
typedef struct {
	int location;
	char name[WAP_NAME_MAX_LENGTH];
	char URL[WAP_URL_MAX_LENGTH];
} gn_wap_bookmark;

/* settings */
typedef enum {
	GN_WAP_SESSION_TEMPORARY = 0,
	GN_WAP_SESSION_PERMANENT
} gn_wap_session;

GNOKII_API const char *gn_wap_session2str(gn_wap_session p);

typedef enum {
	GN_WAP_AUTH_NORMAL = 0,
	GN_WAP_AUTH_SECURE
} gn_wap_authentication;

GNOKII_API const char *gn_wap_authentication2str(gn_wap_authentication p);

typedef enum {
	GN_WAP_BEARER_GSMDATA = 1,
	GN_WAP_BEARER_GPRS    = 3,
	GN_WAP_BEARER_SMS     = 7,
	GN_WAP_BEARER_USSD    = 9 /* FIXME real value? */
} gn_wap_bearer;

GNOKII_API const char *gn_wap_bearer2str(gn_wap_bearer p);

typedef enum {
	GN_WAP_CALL_ANALOGUE,
	GN_WAP_CALL_ISDN
} gn_wap_call_type;

GNOKII_API const char *gn_wap_call_type2str(gn_wap_call_type p);

typedef enum {
	GN_WAP_CALL_AUTOMATIC,
	GN_WAP_CALL_9600,
	GN_WAP_CALL_14400
} gn_wap_call_speed;

GNOKII_API const char *gn_wap_call_speed2str(gn_wap_call_speed p);

typedef enum {
	GN_WAP_LOGIN_MANUAL,
	GN_WAP_LOGIN_AUTOLOG
} gn_wap_login;

GNOKII_API const char *gn_wap_login2str(gn_wap_login p);

typedef enum {
	GN_WAP_GPRS_ALWAYS,
	GN_WAP_GPRS_WHENNEEDED
} gn_wap_gprs;

GNOKII_API const char *gn_wap_gprs2str(gn_wap_gprs p);

typedef struct {
	int read_before_write;
	int location;
	int successors[4];
	char number             [WAP_SETTING_NAME_MAX_LENGTH];
	char gsm_data_ip        [WAP_SETTING_NAME_MAX_LENGTH];
	char gprs_ip            [WAP_SETTING_NAME_MAX_LENGTH];
	char name               [WAP_SETTING_NAME_MAX_LENGTH];
	char home               [WAP_SETTING_HOME_MAX_LENGTH];
	char gsm_data_username  [WAP_SETTING_USERNAME_MAX_LENGTH];
	char gsm_data_password  [WAP_SETTING_NAME_MAX_LENGTH];
	char gprs_username      [WAP_SETTING_USERNAME_MAX_LENGTH];
	char gprs_password      [WAP_SETTING_NAME_MAX_LENGTH];
	char access_point_name  [WAP_SETTING_APN_MAX_LENGTH];
	char sms_service_number [WAP_SETTING_NAME_MAX_LENGTH];
	char sms_server_number  [WAP_SETTING_NAME_MAX_LENGTH];
	gn_wap_session session;
	int security;
	gn_wap_bearer bearer;
	gn_wap_authentication gsm_data_authentication;
	gn_wap_authentication gprs_authentication;
	gn_wap_call_type call_type;
	gn_wap_call_speed call_speed;
	gn_wap_login gsm_data_login;
	gn_wap_login gprs_login;
	gn_wap_gprs gprs_connection;
} gn_wap_setting;

/* This structure is provided to allow common information about the particular
   model to be looked up in a model independant way. Some of the values here
   define minimum and maximum levels for values retrieved by the various Get
   functions for example battery level. They are not defined as constants to
   allow model specific code to set them during initialisation */
typedef struct {
	unsigned char *models; /* Models covered by this type, pipe '|' delimited. */

	/* Minimum and maximum levels for RF signal strength. Units are as per the
	   setting of RFLevelUnits.  The setting of RFLevelUnits indicates the
	   default or "native" units used.  In the case of the 3110 and 6110 series
	   these are arbitrary, ranging from 0 to 4. */
	float max_rf_level;
	float min_rf_level;
	gn_rf_unit rf_level_unit;

	/* Minimum and maximum levels for battery level. Again, units are as per the
	   setting of GSM_BatteryLevelUnits.  The value that BatteryLevelUnits is set
	   to indicates the "native" or default value that the phone supports.  In the
	   case of the 3110 and 6110 series these are arbitrary, ranging from 0 to 4. */
	float max_battery_level;
	float min_battery_level;
	gn_battery_unit battery_level_unit;

	/* Information about date, time and alarm support. In case of alarm
	   information we provide value for the number of alarms supported. */
	gn_datetime_support datetime_support;
	gn_datetime_support alarm_support;
	int maximum_alarms_number;
	
	unsigned int startup_logo_height;   /* Logo widths and heights - if supported */
	unsigned int startup_logo_width;
	unsigned int operator_logo_height;
	unsigned int operator_logo_width;
	unsigned int caller_logo_height;
	unsigned int caller_logo_width;
} gn_phone;

typedef enum {
	GN_PROFILE_MESSAGE_NoTone	= 0x00,
	GN_PROFILE_MESSAGE_Standard	= 0x01,
	GN_PROFILE_MESSAGE_Special	= 0x02,
	GN_PROFILE_MESSAGE_BeepOnce	= 0x03,
	GN_PROFILE_MESSAGE_Ascending	= 0x04
} gn_profile_message_type;

GNOKII_API const char *gn_profile_message_type2str(gn_profile_message_type p);

typedef enum {
	GN_PROFILE_WARNING_Off		= 0xff,
	GN_PROFILE_WARNING_On		= 0x04
} gn_profile_warning_type;

GNOKII_API const char *gn_profile_warning_type2str(gn_profile_warning_type p);

typedef enum {
	GN_PROFILE_VIBRATION_Off	= 0x00,
	GN_PROFILE_VIBRATION_On		= 0x01
} gn_profile_vibration_type;

GNOKII_API const char *gn_profile_vibration_type2str(gn_profile_vibration_type p);

typedef enum {
	GN_PROFILE_CALLALERT_Ringing		= 0x01,
	GN_PROFILE_CALLALERT_BeepOnce		= 0x02,
	GN_PROFILE_CALLALERT_Off		= 0x04,
	GN_PROFILE_CALLALERT_RingOnce		= 0x05,
	GN_PROFILE_CALLALERT_Ascending		= 0x06,
	GN_PROFILE_CALLALERT_CallerGroups	= 0x07
} gn_profile_callalert_type;

GNOKII_API const char *gn_profile_callalert_type2str(gn_profile_callalert_type p);

typedef enum {
	GN_PROFILE_KEYVOL_Off		= 0xff,
	GN_PROFILE_KEYVOL_Level1	= 0x00,
	GN_PROFILE_KEYVOL_Level2	= 0x01,
	GN_PROFILE_KEYVOL_Level3	= 0x02
} gn_profile_keyvol_type;

GNOKII_API const char *gn_profile_keyvol_type2str(gn_profile_keyvol_type p);

typedef enum {
	GN_PROFILE_VOLUME_Level1	= 0x06,
	GN_PROFILE_VOLUME_Level2	= 0x07,
	GN_PROFILE_VOLUME_Level3	= 0x08,
	GN_PROFILE_VOLUME_Level4	= 0x09,
	GN_PROFILE_VOLUME_Level5	= 0x0a,
} gn_profile_volume_type;

GNOKII_API const char *gn_profile_volume_type2str(gn_profile_volume_type p);

/* Structure to hold profile entries. */
typedef struct {
	int number;           /* The number of the profile. */
	char name[40];        /* The name of the profile. */
	int default_name;     /* 0-6, when default name is used, -1, when not. */
	int keypad_tone;      /* Volume level for keypad tones. */
	int lights;           /* Lights on/off. */
	int call_alert;       /* Incoming call alert. */
	int ringtone;         /* Ringtone for incoming call alert. */
	int volume;           /* Volume of the ringing. */
	int message_tone;     /* The tone for message indication. */
	int warning_tone;     /* The tone for warning messages. */
	int vibration;        /* Vibration? */
	int caller_groups;    /* CallerGroups. */
	int automatic_answer; /* Does the phone auto-answer incoming call? */
} gn_profile;

/* Limits for IMEI, Revision, Model and Manufacturer string storage. */
#define GN_IMEI_MAX_LENGTH         20
#define GN_REVISION_MAX_LENGTH     20
#define GN_MODEL_MAX_LENGTH        32
#define GN_MANUFACTURER_MAX_LENGTH 32

#define GN_BCD_STRING_MAX_LENGTH 40

/* This data-type is used to specify the type of the number. See the official
   GSM specification 03.40, version 6.1.0, section 9.1.2.5, page 35-37. */
typedef enum {
	GN_GSM_NUMBER_Unknown       = 0x81, /* Unknown number */
	GN_GSM_NUMBER_International = 0x91, /* International number */
	GN_GSM_NUMBER_National      = 0xa1, /* National number */
	GN_GSM_NUMBER_Network       = 0xb1, /* Network specific number */
	GN_GSM_NUMBER_Subscriber    = 0xc1, /* Subscriber number */
	GN_GSM_NUMBER_Alphanumeric  = 0xd0, /* Alphanumeric number */
	GN_GSM_NUMBER_Abbreviated   = 0xe1  /* Abbreviated number */
} gn_gsm_number_type;

typedef struct {
	gn_gsm_number_type type;
	char number[GN_BCD_STRING_MAX_LENGTH];
} gn_gsm_number;

/* Data structures for the call divert */
typedef enum {
	GN_CDV_Unconditional = 0x00,
	GN_CDV_Busy,
	GN_CDV_NoAnswer,
	GN_CDV_OutOfReach,
	GN_CDV_NotAvailable,
	GN_CDV_AllTypes
} gn_call_divert_type;

GNOKII_API const char *gn_call_divert_type2str(gn_call_divert_type p);

typedef enum {
	GN_CDV_VoiceCalls = 0x01,
	GN_CDV_FaxCalls,
	GN_CDV_DataCalls,
	GN_CDV_AllCalls
} gn_call_divert_call_type;

GNOKII_API const char *gn_call_divert_call_type2str(gn_call_divert_call_type p);

typedef enum {
	GN_CDV_Disable  = 0x00,
	GN_CDV_Enable   = 0x01,
	GN_CDV_Query    = 0x02,
	GN_CDV_Register = 0x03,
	GN_CDV_Erasure  = 0x04
} gn_call_divert_operation;

GNOKII_API const char *gn_call_divert_operation2str(gn_call_divert_operation p);

typedef struct {
	gn_call_divert_type           type;
	gn_call_divert_call_type     ctype;
	gn_call_divert_operation operation;
	gn_gsm_number               number;
	unsigned int               timeout;
} gn_call_divert;

typedef struct {
	int full; /* indicates if we have full data read */
	unsigned int length;
	unsigned char *data;
} gn_raw_data;

/* This enum is used for display status. */
typedef enum {
	GN_DISP_Call_In_Progress, /* Call in progress. */
	GN_DISP_Unknown,          /* The meaning is unknown now :-( */
	GN_DISP_Unread_SMS,       /* There is Unread SMS. */
	GN_DISP_Voice_Call,       /* Voice call active. */
	GN_DISP_Fax_Call,         /* Fax call active. */
	GN_DISP_Data_Call,        /* Data call active. */
	GN_DISP_Keyboard_Lock,    /* Keyboard lock status. */
	GN_DISP_SMS_Storage_Full  /* Full SMS Memory. */
} gn_display_status;

#define	GN_DRAW_SCREEN_MAX_WIDTH  27
#define	GN_DRAW_SCREEN_MAX_HEIGHT  6

typedef enum {
	GN_DISP_DRAW_Clear,
	GN_DISP_DRAW_Text,
	GN_DISP_DRAW_Status
} gn_display_draw_command;

typedef struct {
	int x;
	int y;
	unsigned char text[GN_DRAW_SCREEN_MAX_WIDTH + 1];
} gn_display_text;

typedef struct {
	gn_display_draw_command cmd;
	union {
		gn_display_text text;
		gn_display_status status;
	} data;
} gn_display_draw_msg;

typedef struct {
	void (*output_fn)(gn_display_draw_msg *draw);
	int state;
	struct timeval last;
} gn_display_output;

typedef enum {
	GN_KEY_NONE = 0x00,
	GN_KEY_1 = 0x01,
	GN_KEY_2,
	GN_KEY_3,
	GN_KEY_4,
	GN_KEY_5,
	GN_KEY_6,
	GN_KEY_7,
	GN_KEY_8,
	GN_KEY_9,
	GN_KEY_0,
	GN_KEY_HASH,
	GN_KEY_ASTERISK,
	GN_KEY_POWER,
	GN_KEY_GREEN,
	GN_KEY_RED,
	GN_KEY_INCREASEVOLUME,
	GN_KEY_DECREASEVOLUME,
	GN_KEY_UP = 0x17,
	GN_KEY_DOWN,
	GN_KEY_MENU,
	GN_KEY_NAMES
} gn_key_code;

typedef struct {
	int field;
	char screen[50];
} gn_netmonitor;

typedef struct {
	int  userlock;		/* TRUE = user lock, FALSE = factory lock */
	int  closed;
	char  data[12];
	int   counter;
} gn_locks_info;

typedef struct {
	int frequency;
	int volume;
} gn_tone;

#define GN_RINGTONE_MAX_NAME 20
#define GN_RINGTONE_MAX_COUNT 256

typedef struct {
	int location;
	char name[20];
	int user_defined;
	int readable;
	int writable;
} gn_ringtone_info;

typedef struct {
	int count;
	int userdef_location;
	int userdef_count;
	gn_ringtone_info ringtone[GN_RINGTONE_MAX_COUNT];
} gn_ringtone_list;

typedef enum {
	GN_LOG_T_NONE = 0,
	GN_LOG_T_STDERR = 1
} gn_log_target;


typedef enum {
	GN_FT_None = 0,
	GN_FT_NOL,
	GN_FT_NGG,
	GN_FT_NSL,
	GN_FT_NLM,
	GN_FT_BMP,
	GN_FT_OTA,
	GN_FT_XPMF,
	GN_FT_RTTTL,
	GN_FT_OTT,
	GN_FT_MIDI,
	GN_FT_NOKRAW_TONE,
	GN_FT_GIF,
	GN_FT_JPG,
	GN_FT_MID,
	GN_FT_NRT,
	GN_FT_PNG,
} gn_filetypes;

typedef struct {
	gn_filetypes filetype;   /* file type */
	unsigned char *id;	/* file id */
	char name[512];		/* file name */
	int year;		/* datetime of creation/modification */
	int month;
	int day;
	int hour;
	int minute;
	int second;
	int file_length;	/* size of the file */
	int togo;		/* amount of bytes to be sent yet */
	int just_sent;		/* ??? */
	int folderId;           /* folder id of the file */
	unsigned char *file;	/* file contents */
} gn_file;

typedef struct {
	char path[512];
	gn_file **files;
	int file_count;
	int size;
} gn_file_list;

#endif	/* _gnokii_common_h */
