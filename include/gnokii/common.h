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

#ifndef _gnokii_gsm_common_h
#define _gnokii_gsm_common_h

#include <stdlib.h>

#include "config.h"
#include "misc.h"
#include "data/rlp-common.h"

/* Type of connection. Now we support serial connection with FBUS cable and
   IR (only with 61x0 models) */
typedef enum {
	GN_CT_Serial,   /* Serial connection. */
	GN_CT_DAU9P,    /* Serial connection using DAU9P cable; use only with 6210/7110 if you want faster initialization */
	GN_CT_DLR3P,    /* Serial connection using DLR3P cable */
	GN_CT_Infrared, /* Infrared connection. */
	GN_CT_Irda,     /* Linux IrDA support */
	GN_CT_Tekram,   /* Tekram Ir-Dongle */
	GN_CT_TCP,      /* TCP network connection */
	GN_CT_M2BUS	/* Serial connection with M2BUS protocol */
} gn_connection_type;

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
	GN_MT_MT, /* combined ME and SIM phonebook */
	GN_MT_TA, /* for compatibility only: TA=computer memory */
	GN_MT_CB, /* Currently selected memory */
	GN_MT_IN, /* Inbox for folder aware phones */
	GN_MT_OU, /* Outbox  */
	GN_MT_AR, /* Archive */
	GN_MT_TE, /* Templates */
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
	GN_MT_XX = 0xff	/* Error code for unknown memory type (returned by fbus-xxxx functions). */
} gn_memory_type;

/* Power source types */
typedef enum {
	GN_PS_ACDC = 1, /* AC/DC powered (charging) */
	GN_PS_BATTERY   /* Internal battery */
} gn_power_source;

/* Definition of security codes. */
typedef enum {
	GN_SCT_SecurityCode = 0x01, /* Security code. */
	GN_SCT_Pin,                 /* PIN. */
	GN_SCT_Pin2,                /* PIN 2. */
	GN_SCT_Puk,                 /* PUK. */
	GN_SCT_Puk2,                /* PUK 2. */
	GN_SCT_None                 /* Code not needed. */
} gn_security_code_type;

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
#define GN_PHONEBOOK_NUMBER_MAX_LENGTH          48   /* For 7110 */
#define GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER      10   /* For 6510 */
						     /* 7110 is able to have in one
						      * entry 5 numbers and 4
						      * texts [email,notice,postal,url] */
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
	GN_PHONEBOOK_NUMBER_Home    = 0x02,
	GN_PHONEBOOK_NUMBER_Mobile  = 0x03,
	GN_PHONEBOOK_NUMBER_Fax     = 0x04,
	GN_PHONEBOOK_NUMBER_Work    = 0x06,
	GN_PHONEBOOK_NUMBER_General = 0x0a,
} gn_phonebook_number_type;

typedef enum {
	GN_PHONEBOOK_ENTRY_Name       = 0x07,
	GN_PHONEBOOK_ENTRY_Email      = 0x08,
	GN_PHONEBOOK_ENTRY_Postal     = 0x09,
	GN_PHONEBOOK_ENTRY_Note       = 0x0a,
	GN_PHONEBOOK_ENTRY_Number     = 0x0b,
	GN_PHONEBOOK_ENTRY_Ringtone   = 0x0c,
	GN_PHONEBOOK_ENTRY_Date       = 0x13,   /* Date is used for DC,RC,etc (last calls) */
	GN_PHONEBOOK_ENTRY_Pointer    = 0x1a,   /* Pointer to the other memory */
	GN_PHONEBOOK_ENTRY_Logo       = 0x1b,
	GN_PHONEBOOK_ENTRY_LogoSwitch = 0x1c,
	GN_PHONEBOOK_ENTRY_Group      = 0x1e,
	GN_PHONEBOOK_ENTRY_URL        = 0x2c,
} gn_phonebook_entry_type;

typedef struct {
	gn_phonebook_entry_type entry_type;
	gn_phonebook_number_type number_type;
	union {
		char number[GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1]; /* Number */
		gn_timestamp date;                               /* or the last calls list */
	} data;
	int id;
} gn_phonebook_subentry;

/* Define datatype for phonebook entry, used for getting/writing phonebook
   entries. */
typedef struct {
	bool empty;                                       /* Is this entry empty? */
	char name[GN_PHONEBOOK_NAME_MAX_LENGTH + 1];      /* Plus 1 for
							     nullterminator. */
	char number[GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1];  /* Number */
	gn_memory_type memory_type;                       /* Type of memory */
	int caller_group;                                 /* Caller group */
	int location;                                     /* Location */
	gn_timestamp date;                                /* The record date and time
							     of the number. */
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
} gn_calnote_type;

typedef enum {
	GN_CALNOTE_NEVER   = 0,
	GN_CALNOTE_DAILY   = 24,
	GN_CALNOTE_WEEKLY  = 168,
	GN_CALNOTE_2WEEKLY = 336,
	GN_CALNOTE_YEARLY  = 65535
} gn_calnote_recurrence;

#define GN_CALNOTE_MAX_NUMBER        500 /* FIXME how many are possible? */
#define GN_CALNOTE_MAX_LENGTH        258
#define GN_CALNOTE_NUMBER_MAX_LENGTH  49

typedef struct {
	bool enabled; /* Is alarm set ? */
	gn_timestamp timestamp;
} gn_calnote_alarm;

/* Calendar note type */
typedef struct {
	int location;                                    /* The number of the note in the phone memory */
	gn_calnote_type type;                            /* The type of the note */
	gn_timestamp time;                               /* The time of the note */
	gn_calnote_alarm alarm;                          /* The alarm of the note */
	char text[GN_CALNOTE_MAX_LENGTH];                /* The text of the note */
	char phone_number[GN_CALNOTE_NUMBER_MAX_LENGTH]; /* For Call only: the phone number */
	gn_calnote_recurrence recurrence;                /* Recurrence of the note */
} gn_calnote;

/* List of Calendar Notes in phone */
typedef struct {
	int number;                          /* The number of notes in phone */
	int location[GN_CALNOTE_MAX_NUMBER]; /* Location of the nth note */
} gn_calnote_list;

/* ToDo things. It is only supported by the newer phones. */
#define GN_TODO_MAX_LENGTH	256
#define GN_TODO_MAX_NUMBER	512

typedef enum {
	GN_TODO_LOW = 3,
	GN_TODO_MEDIUM = 2,
	GN_TODO_HIGH = 1
} gn_todo_priority;

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

typedef enum {
	GN_WAP_AUTH_NORMAL = 0,
	GN_WAP_AUTH_SECURE
} gn_wap_authentication;

typedef enum {
	GN_WAP_BEARER_GSMDATA = 1,
	GN_WAP_BEARER_GPRS    = 3,
	GN_WAP_BEARER_SMS     = 7,
	GN_WAP_BEARER_USSD    = 9 /* FIXME real value? */
} gn_wap_bearer;

typedef enum {
	GN_WAP_CALL_ANALOGUE,
	GN_WAP_CALL_ISDN
} gn_wap_call_type;

typedef enum {
	GN_WAP_CALL_AUTOMATIC,
	GN_WAP_CALL_9600,
	GN_WAP_CALL_14400
} gn_wap_call_speed;

typedef enum {
	GN_WAP_LOGIN_MANUAL,
	GN_WAP_LOGIN_AUTOLOG
} gn_wap_login;

typedef enum {
	GN_WAP_GPRS_ALWAYS,
	GN_WAP_GPRS_WHENNEEDED
} gn_wap_gprs;

typedef struct {
	bool read_before_write;
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
	bool security;
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
	
	u8 startup_logo_height;   /* Logo widths and heights - if supported */
	u8 startup_logo_width;
	u8 operator_logo_height;
	u8 operator_logo_width;
	u8 caller_logo_height;
	u8 caller_logo_width;
} gn_phone;

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

/* Limits for IMEI, Revision and Model string storage. */
#define GN_IMEI_MAX_LENGTH     20
#define GN_REVISION_MAX_LENGTH 20
#define GN_MODEL_MAX_LENGTH    20

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
	GN_CDV_Busy = 0x01,
	GN_CDV_NoAnswer,
	GN_CDV_OutOfReach,
	GN_CDV_NotAvailable,
	GN_CDV_AllTypes
} gn_call_divert_type;

typedef enum {
	GN_CDV_VoiceCalls = 0x01,
	GN_CDV_FaxCalls,
	GN_CDV_DataCalls,
	GN_CDV_AllCalls
} gn_call_divert_call_type;

typedef enum {
	GN_CDV_Disable  = 0x00,
	GN_CDV_Enable   = 0x01,
	GN_CDV_Query    = 0x02,
	GN_CDV_Register = 0x03,
	GN_CDV_Erasure  = 0x04
} gn_call_divert_operation;

typedef struct {
	gn_call_divert_type           type;
	gn_call_divert_call_type     ctype;
	gn_call_divert_operation operation;
	gn_gsm_number               number;
	unsigned int               timeout;
} gn_call_divert;

typedef struct {
	bool full; /* indicates if we have full data read */
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
	bool  userlock;		/* TRUE = user lock, FALSE = factory lock */
	bool  closed;
	char  data[12];
	int   counter;
} gn_locks_info;

#endif	/* _gnokii_gsm_common_h */
