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

  Copyright (C) 2001-2002 Pawe³ Kot <pkot@linuxnews.pl>

  Include file for the SMS library.

*/

#ifndef __gnokii_sms_h_
#define __gnokii_sms_h_

#include "misc.h"
#include "gsm-error.h"
#include "gsm-common.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"
#include "gsm-encoding.h"

/* Maximum length of SMS center name */
#define GSM_MAX_SMS_CENTER_NAME_LENGTH	20

/* Limits of SMS messages. */
#define GSM_MAX_SMS_LENGTH             160
#define GSM_MAX_8BIT_SMS_LENGTH        140
#define GSM_MAX_LONG_LENGTH	     10240

#define SMS_MAX_PART_NUMBER              3

/* FIXME: what value should be here? (Pawel Kot) */
#define SMS_MAX_UDH_NUMBER              10

/* Maximal number of SMS folders */
#define MAX_SMS_FOLDERS 24
#define MAX_SMS_MESSAGES 190

#define MAX_DATETIME_LENGTH   7
#define MAX_SMSC_NAME_LEN    16
#define MAX_NUMBER_LEN       12
#define SMS_USER_DATA_LEN   512
#define MAX_VALIDITY_LENGTH   8

/*** MEMORY INFO ***/

typedef struct {
	int Unread;		/* Number of unread messages */
	int Number;		/* Number of all messages */
} GSM_SMSMemoryStatus;

typedef struct {
	unsigned int Number;	/* Number of message we get from GetSMSStatus */
	unsigned int Unread;	/* Number of unread messages we get from GetSMSStatus */
	unsigned int Changed;	/* because when a message is moved between folders status wouldn't change */
	unsigned int NumberOfFolders;	/* Number of Folders we get from GetFolders */
} SMS_Status;


/*** DATE AND TIME ***/

typedef struct {
	int Year;	/* The complete year specification - e.g. 1999. Y2K :-) */
	int Month;	/* January = 1 */
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Timezone;	/* The difference between local time and GMT.
			   Not that different SMSC software treat this field
			   in the different way. */
} SMS_DateTime;

/*** USER DATA HEADER ***/

/* types of User Data Header */
typedef enum {
	SMS_NoUDH                = 0x00,
	SMS_ConcatenatedMessages = 0x01,
	SMS_Ringtone             = 0x02,
	SMS_OpLogo               = 0x03,
	SMS_CallerIDLogo         = 0x04,
	SMS_MultipartMessage     = 0x05,
	SMS_WAPvCard             = 0x06,
	SMS_WAPvCalendar         = 0x07,
	SMS_WAPvCardSecure       = 0x08,
	SMS_WAPvCalendarSecure   = 0x09,
	SMS_VoiceMessage         = 0x0a,
	SMS_FaxMessage           = 0x0b,
	SMS_EmailMessage         = 0x0c,
	SMS_OtherMessage         = 0x0d,
	SMS_UnknownUDH           = 0x0e
} SMS_UDHType;

typedef struct {
	SMS_UDHType Type;
	union {
		struct {
			unsigned short ReferenceNumber;
			unsigned short MaximumNumber;
			unsigned short CurrentNumber;
		} ConcatenatedShortMessage; /* SMS_ConcatenatedMessages */
		struct {
			bool Store;
			unsigned short MessageCount;
		} SpecialSMSMessageIndication; /* SMS_VoiceMessage, SMS_FaxMessage, SMS_EmailMessage, SMS_OtherMessage */
		struct {
			char NetworkCode[6];
			unsigned int width, height;
		} Logo; /* SMS_OpLogo, SMS_CallerIDLogo */
		struct {
			unsigned int notes; /* Number of the notes */
		} Ringtone; /* SMS_Ringtone */
	} u;
} SMS_UDHInfo;

typedef struct {
	unsigned int Number; /* Number of the present UDH */
	unsigned int Length;
	SMS_UDHInfo UDH[SMS_MAX_UDH_NUMBER];
} SMS_UserDataHeader;

typedef enum {
	SMS_PID, /* Set Protocol Identifier to `Return Call Message' */
	SMS_DCS, /* Set Data Coding Scheme "to indicate the type of message waiting and whether there are some messages or no messages" */
	SMS_UDH  /* Use User Data Header - Special SMS Message Indication; the maximium level of information, may not be supported by all phones */
} SMS_MessageWaitingType;

/*** DATA CODING SCHEME ***/

typedef enum {
	SMS_GeneralDataCoding,
	SMS_MessageWaiting
} SMS_DataCodingSchemeType;

typedef enum {
	SMS_DefaultAlphabet = 0x00,
	SMS_8bit            = 0x01,
	SMS_UCS2            = 0x02
} SMS_AlphabetType;

typedef enum {
	SMS_VoiceMail = 0x00,
	SMS_Fax       = 0x01,
	SMS_Email     = 0x02,
	SMS_Text      = 0x03,
	SMS_Other     = 0x04
} SMS_IndicationType;

typedef struct {
	SMS_DataCodingSchemeType Type;
	union {
		struct {
			unsigned short Class; /* 0 - no class
						 1 - Class 0
						 2 - Class 1
						 3 - Class 2
						 4 - Class 3 */
			bool Compressed;
			SMS_AlphabetType Alphabet;
		} General;
		struct {
			bool Discard;
			SMS_AlphabetType Alphabet; /* ucs16 not supported */
			bool Active;
			SMS_IndicationType Type;
		} MessageWaiting;
	} u;
} SMS_DataCodingScheme;

/*** VALIDITY PERIOD ***/

typedef enum {
	SMS_NoValidityPeriod = 0x00,
	SMS_EnhancedFormat   = 0x01,
	SMS_RelativeFormat   = 0x02,
	SMS_AbsoluteFormat   = 0x03
} SMS_ValidityPeriodFormat;

typedef enum {
	SMS_EnhancedNoValidityPeriod  = 0x00,
	SMS_EnhancedRelativeFormat    = 0x01,
	SMS_EnhancedRelativeSeconds   = 0x02, /* Only one ocetet more is used */
	SMS_EnhancedRelativeSemiOctet = 0x03  /* 3 octets contain relative time in hours, minutes and seconds in semi-octet representation */
} SMS_EnhancedValidityPeriodType;

typedef struct {
	bool extension; /* we need to set it to 0 at the moment; FIXME: how to handle `1' here? */
	bool single_shot;
	SMS_EnhancedValidityPeriodType type;
	union {
		unsigned short relative;
		unsigned short seconds;
		SMS_DateTime hms;
	} period;
} SMS_EnhancedValidityPeriod;

/* Validity of SMS Messages. */

typedef enum {
	SMS_V1H   = 0x0b,
	SMS_V6H   = 0x47,
	SMS_V24H  = 0xa7,
	SMS_V72H  = 0xa9,
	SMS_V1W   = 0xad,
	SMS_VMax  = 0xff
} SMS_ValidityPeriod;

typedef struct {
	SMS_ValidityPeriodFormat VPF;
	union {
		SMS_EnhancedValidityPeriod Enhanced;
		SMS_ValidityPeriod Relative; /* 8 bit */
		SMS_DateTime Absolute;
	} u;
} SMS_MessageValidity;


/*** MESSAGE CENTER ***/

typedef enum {
	SMS_FText   = 0x00, /* Plain text message. */
	SMS_FFax    = 0x22, /* Fax message. */
	SMS_FVoice  = 0x24, /* Voice mail message. */
	SMS_FERMES  = 0x25, /* ERMES message. */
	SMS_FPaging = 0x26, /* Paging. */
	SMS_FUCI    = 0x2d, /* Email message in 8110i. */
	SMS_FEmail  = 0x32, /* Email message. */
	SMS_FX400   = 0x31  /* X.400 message. */
} SMS_MessageFormat;

typedef struct {
	int			No;					/* Number of the SMSC in the phone memory. */
	char			Name[GSM_MAX_SMS_CENTER_NAME_LENGTH];	/* Name of the SMSC. */
	int			DefaultName;				/* >= 1 if default name used, otherwise -1 */
	SMS_MessageFormat	Format;					/* SMS is sent as text/fax/paging/email. */
	SMS_ValidityPeriod	Validity;				/* Validity of SMS Message. */
	SMS_Number	       	SMSC;			       		/* Number of the SMSC. */
	SMS_Number		Recipient;				/* Number of the default recipient. */
} SMS_MessageCenter;

/*** SHORT MESSAGE CORE ***/

typedef enum {                     /* Bits meaning */
	SMS_Deliver         = 0x00, /* 00 0 First 2 digits are taken from */
	SMS_Delivery_Report = 0x01, /* 00 1 GSM 03.40 version 6.1.0 Release 1997 */
	SMS_Submit          = 0x02, /* 01 0 */
	SMS_Submit_Report   = 0x03, /* 01 1 */
	SMS_Command         = 0x04, /* 10 0 mark a report */
	SMS_Status_Report   = 0x05, /* 10 1 Section 9.2.3.1; 3rd digit is to */
	SMS_Picture         = 0x07, /* Looks like Happy N*kia Engineers (TM) invention */
	SMS_TextTemplate    = 0x08, /* text template necessary for 6510 */
	SMS_PictureTemplate = 0x09, /* picture template  "" */
	SMS_SubmitSent      = 0x0A  /* outbox sent template "" */
} SMS_MessageType;

typedef enum {
	SMS_Enquiry            = 0x00, /* Enquiry relating to previosly submitted short message; sets SRR to 1 */
	SMS_CancelStatusReport = 0x01, /* Cancel Status Report Request relating to previously submitted short message; sets SRR to 0 */
	SMS_DeleteSM           = 0x02, /* Delete previousle submitted Short Message; sets SRR to 0 */
	SMS_EnableStatusReport = 0x03  /* Enable Status Report Request relating to previously submitted short message; sets SRR to 0 */
} SMS_CommandType;

typedef struct {
	SMS_CommandType Type;
} SMS_MessageCommand;

/* Datatype for SMS status */
typedef enum {
	SMS_Read   = 0x01,
	SMS_Unread = 0x03,
	SMS_Sent   = 0x05,
	SMS_Unsent = 0x07
} SMS_MessageStatus;

/* In contrast to GSM_MemoryType, SMS_MemoryType is phone dependant */
/*
typedef enum {
} SMS_MemoryType;
*/
typedef enum {
	SMS_NoData       = 0x00,
	SMS_PlainText    = 0x01,
	SMS_BitmapData   = 0x02,
	SMS_RingtoneData = 0x03,
	SMS_iMelodyText  = 0x04,
	SMS_MultiData	 = 0x05,
	SMS_NokiaText 	 = 0x06,
	SMS_AnimationData= 0x07,
	SMS_Concat	 = 0x08,
	SMS_OtherData    = 0x09
} SMS_DataType;

/*** FOLDER INFO ***/

typedef enum {
	SMS_Old		= 0x00,
	SMS_New		= 0x01,
	SMS_Deleted	= 0x02,
	SMS_ToBeRemoved	= 0x03,
	SMS_NotRead	= 0x04,
	SMS_NotReadHandled	= 0x05,
	SMS_Changed	= 0x06
} ActionType;
	
typedef struct {
	ActionType Type;		/* deleted, new, old, ToBeRemoved */
	unsigned int Location;
	SMS_MessageType MessageType;
} SMS_MessagesList;

typedef struct {
	int Number;		/* -1 if folder is not supported by the phone */
	unsigned int Unread;	/* only valid for INBOX */
	unsigned int Changed;
	unsigned int Used;	/* because 'Used' can vary from 'Number' when we have deleted messages */
} SMS_FolderStats;

typedef struct {
	unsigned char Binary[GSM_MAX_SMS_LENGTH];
	int this, total;	/* Number of this part, total number of parts */
} GSM_Multi;

typedef struct {
	int this, total, serial;
} GSM_Concat;

typedef struct {
	SMS_DataType Type;
	unsigned int Length;
	union {
		unsigned char Text[GSM_MAX_SMS_LENGTH];
		GSM_Multi Multi;
		gn_bmp Bitmap;
		GSM_Ringtone Ringtone;
		gn_bmp Animation[4];
		GSM_Concat Concat;
	} u;
} SMS_UserData;

/* Define datatype for SMS messages exported to the user applications. */
typedef struct {
	/* General fields */
	SMS_MessageType Type;            /* Type of the message. */
	bool DeliveryReport;             /* Do we request the delivery report? Only for setting. */
	SMS_MessageStatus Status;        /* Status of the message read/unread/sent/unsent. */
	unsigned int Validity;           /* Message validity in minutes. Only for setting. */
	GSM_MemoryType MemoryType;       /* Memory type where the message is/should be stored. */
	unsigned int Number;             /* Location of the message in the memory/folder. */

	/* Number related fields */
	SMS_Number SMSC;                 /* SMSC Number. */
	SMS_Number Remote;               /* Remote (sender/receipient) number. */

	/* Data format fields */
	SMS_DataCodingScheme DCS;        /* Data Coding Scheme. */
	SMS_UserData UserData[SMS_MAX_PART_NUMBER]; /* User Data. */
	SMS_UserDataHeader UDH;

	/* Date fields */
	SMS_DateTime SMSCTime;           /* SMSC Timestamp. Only for reading. */
	SMS_DateTime Time;               /* Delivery timestamp. Only for reading. */
} GSM_API_SMS;

/* Define datatype for SMS messages, describes precisely GSM Spec 03.40 */
typedef struct {
	unsigned int Type;                             /* Message Type Indicator - 2 bits (9.2.3.1) */
	bool MoreMessages;                             /* More Messages to Send (9.2.3.2) */
	bool ReplyViaSameSMSC;                         /* Reply Path (9.2.3.17) - `Reply via same centre' in the phone */
	bool RejectDuplicates;                         /* Reject Duplicates (9.2.3.25) */
	bool Report;                                   /* Status Report (9.2.3.4, 9.2.3.5 & 9.2.3.26) - `Delivery reports' in the phone */

	unsigned int Number;                           /* Message Number - 8 bits (9.2.3.18) */
	unsigned int Reference;                        /* Message Reference - 8 bit (9.2.3.6) */
	unsigned int PID;                              /* Protocol Identifier - 8 bit (9.2.3.9) */
	unsigned int ReportStatus;                     /* Status - 8 bit (9.2.3.15), Failure Cause (9.2.3.22) */

	unsigned char SMSCTime[MAX_DATETIME_LENGTH];   /* Service Centre Time Stamp (9.2.3.11) */
	unsigned char Time[MAX_DATETIME_LENGTH];       /* Discharge Time (9.2.3.13) */
	unsigned char MessageCenter[MAX_SMSC_NAME_LEN];/* SMSC Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */
	unsigned char RemoteNumber[MAX_NUMBER_LEN];    /* Origination, destination, Recipient Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */

	unsigned int DCS;                              /* Data Coding Scheme (9.2.3.10) */
	unsigned int Length;                           /* User Data Length (9.2.3.16), Command Data Length (9.2.3.20) */
	bool UDHIndicator;
	unsigned char UserData[GSM_MAX_LONG_LENGTH];   /* User Data (9.2.3.24), Command Data (9.2.3.21), extened to Nokia Multipart Messages from Smart Messaging Specification 3.0.0 */
	int UserDataLength;                            /* Length of just previous field */

	SMS_ValidityPeriodFormat ValidityIndicator;
	unsigned char Validity[MAX_VALIDITY_LENGTH];   /* Validity Period Format & Validity Period (9.2.3.3 & 9.2.3.12) - `Message validity' in the phone */


	/* Other fields */
	unsigned int MemoryType;                       /* MemoryType (for 6210/7110): folder indicator */
	unsigned int Status;                           /* Status of the message: sent/read or unsent/unread */
} GSM_SMSMessage;


/*** FOLDERS ***/

/* Datatype for SMS folders ins 6210/7110 */
#define MAX_SMS_FOLDER_NAME_LENGTH 16	/* Max name length is 15 characters and trailing \0 */
typedef struct {
	char Name[MAX_SMS_FOLDER_NAME_LENGTH];    /* Name for SMS folder. */
	bool SMSData;                             /* if folder contains sender, SMSC number and sending date */
	unsigned int Locations[MAX_SMS_MESSAGES]; /* locations of SMS messages in that folder (6210 specific) */
	unsigned int Number;                      /* number of SMS messages in that folder*/
	unsigned int FolderID;                    /* ID od fthe current folder */
} SMS_Folder;

typedef struct {
	SMS_Folder Folder[MAX_SMS_FOLDERS];
	unsigned int FolderID[MAX_SMS_FOLDERS]; /* ID specific for this folder and phone.
	                                         * Used in internal functions. Do not use it. */
	unsigned int Number;                    /* number of SMS folders */
} SMS_FolderList;

/*** CELL BROADCAST ***/

#define GSM_MAX_CB_MESSAGE         (160)

/* Define datatype for Cell Broadcast message */
typedef struct {
	int Channel;                                      /* channel number */
	char Message[GSM_MAX_CB_MESSAGE + 1];
	int New;
} GSM_CBMessage;

GSM_Error sms_prepare(GSM_API_SMS *sms, GSM_SMSMessage *rawsms);

/* Utils */
SMS_DateTime *UnpackDateTime(u8 *Number, SMS_DateTime *dt);

#endif /* __gnokii_sms_h_ */
