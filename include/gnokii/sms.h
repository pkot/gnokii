/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for SMS library.

  $Log$
  Revision 1.6  2001-11-22 17:56:53  pkot
  smslib update. sms sending

  Revision 1.5  2001/11/20 16:22:23  pkot
  First attempt to read Picture Messages. They should appear when you enable DEBUG. Nokia seems to break own standards. :/ (Markus Plail)

  Revision 1.4  2001/11/19 13:09:40  pkot
  Begin work on sms sending

  Revision 1.3  2001/11/18 00:54:32  pkot
  Bugfixes. I18n of the user responses. UDH support in libsms. Business Card UDH Type

  Revision 1.2  2001/11/13 16:12:21  pkot
  Preparing libsms to get to work. 6210/7110 SMS and SMS Folder updates

  Revision 1.1  2001/11/08 16:23:20  pkot
  New version of libsms. Not functional yet, but it reasonably stable API.

  Revision 1.1  2001/07/09 23:06:26  pkot
  Moved sms.* files from my hard disk to CVS


*/

#ifndef __gnokii_sms_h_
#define __gnokii_sms_h_

#include "misc.h"
#include "gsm-error.h"

/* Maximum length of SMS center name */

#define GSM_MAX_SMS_CENTER_NAME_LENGTH	(20)

/* Limits of SMS messages. */

#define GSM_MAX_SMS_CENTER_LENGTH  (40)
#define GSM_MAX_SENDER_LENGTH      (40)
#define GSM_MAX_DESTINATION_LENGTH (40)

#define GSM_MAX_SMS_LENGTH         (160)
#define GSM_MAX_8BIT_SMS_LENGTH    (140)

#define SMS_MAX_ADDRESS_LENGTH      (40)

/* FIXME: what value should be here? (Pawel Kot) */
//#define GSM_MAX_USER_DATA_HEADER_LENGTH (10)
#define SMS_MAX_UDH_HEADER_NUMBER 10

/*** MEMORY INFO ***/

typedef struct {
	int Unread; /* Number of unread messages */
	int Number; /* Number of all messages */
} GSM_SMSMemoryStatus;

/*** DATE AND TIME ***/

typedef struct {
	int Year;          /* The complete year specification - e.g. 1999. Y2K :-) */
	int Month;	     /* January = 1 */
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Timezone;      /* The difference between local time and GMT */
} SMS_DateTime;

/*** USER DATA HEADER ***/

/* types of User Data Header */
typedef enum {
	SMS_NoUDH                = 0x00,
	SMS_ConcatenatedMessages = 0x01,
	SMS_OpLogo               = 0x02,
	SMS_CallerIDLogo         = 0x03,
	SMS_Ringtone             = 0x04,
	SMS_VoiceMessage         = 0x05,
	SMS_FaxMessage           = 0x06,
	SMS_EmailMessage         = 0x07,
	SMS_OtherMessage         = 0x08,
	SMS_BusinessCard         = 0x09,
	SMS_UnknownUDH           = 0x0a
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
//			...
		} Logo; /* SMS_OpLogo, SMS_CallerIDLogo */
		struct {
//			...
		} Ringtone; /* SMS_Ringtone */
	} u;
} SMS_UDHInfo;

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

/*** MESSAGE CENTER ***/

typedef struct {
	int			No;					/* Number of the SMSC in the phone memory. */
	SMS_NumberType          Type;
	char			Name[GSM_MAX_SMS_CENTER_NAME_LENGTH];	/* Name of the SMSC. */
	SMS_IndicationType	Format;					/* SMS is sent as text/fax/paging/email. */
	SMS_ValidityPeriod	Validity;				/* Validity of SMS Message. */
	char			Number[GSM_MAX_SMS_CENTER_LENGTH];	/* Number of the SMSC. */
	char			Recipient[GSM_MAX_SMS_CENTER_LENGTH];	/* Number of the default recipient. */
} SMS_MessageCenter;

/*** SHORT MESSAGE CORE ***/

typedef struct {
	SMS_NumberType type;
	char number[SMS_MAX_ADDRESS_LENGTH];
} SMS_Number;

typedef enum {                     /* Bits meaning */
	SMS_Deliver         = 0x00, /* 00 0 First 2 digits are taken from */
	SMS_Delivery_Report = 0x01, /* 00 1 GSM 03.40 version 6.1.0 Release 1997 */
	SMS_Submit          = 0x02, /* 01 0 */
	SMS_Submit_Report   = 0x03, /* 01 1 */
	SMS_Command         = 0x04, /* 10 0 mark a report */
	SMS_Status_Report   = 0x05, /* 10 1 Section 9.2.3.1; 3rd digit is to */
	SMS_Picture         = 0x07  /* Looks like Happy N*kia Engineers (TM) invention */
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
typedef enum {
	GMT_IN = 0x08, /* Inbox in 6210/7110 */
	GMT_OU = 0x10, /* Outbox in 6210/7110 */
	GMT_AR = 0x18, /* Archive in 6210/6110 */
	GMT_TE = 0x20, /* Templates in 6210/7110 */
	GMT_F1 = 0x29, /* 1st CUSTOM FOLDER in 6210/7110*/
	GMT_F2 = 0x31,
	GMT_F3 = 0x39,
	GMT_F4 = 0x41,
	GMT_F5 = 0x49,
	GMT_F6 = 0x51,
	GMT_F7 = 0x59,
	GMT_F8 = 0x61,
	GMT_F9 = 0x69,
	GMT_F10 = 0x71,
	GMT_F11 = 0x79,
	GMT_F12 = 0x81,
	GMT_F13 = 0x89,
	GMT_F14 = 0x91,
	GMT_F15 = 0x99,
	GMT_F16 = 0xA1,
	GMT_F17 = 0xA9,
	GMT_F18 = 0xB1,
	GMT_F19 = 0xB9,
	GMT_F20 = 0xC1 /* 20th CUSTOM FOLDER in 6210/7110 */
} SMS_MemoryType;
        
/* Define datatype for SMS messages, describes precisely GSM Spec 03.40 */
typedef struct {
	/* Specification fields */
	SMS_MessageType Type;                          /* Message Type Indicator - 2 bits (9.2.3.1) */
	bool MoreMessages;                             /* More Messages to Send (9.2.3.2) */
	bool ReplyViaSameSMSC;                         /* Reply Path (9.2.3.17) - `Reply via same centre' in the phone */
	bool RejectDuplicates;                         /* Reject Duplicates (9.2.3.25) */
	bool Report;                                   /* Status Report (9.2.3.4, 9.2.3.5 & 9.2.3.26) - `Delivery reports' in the phone */

	unsigned short Number;                         /* Message Number - 8 bits (9.2.3.18) */
	unsigned short Reference;                      /* Message Reference - 8 bit (9.2.3.6) */
	unsigned short PID;                            /* Protocol Identifier - 8 bit (9.2.3.9) */
	unsigned short ReportStatus;                   /* Status - 8 bit (9.2.3.15), Failure Cause (9.2.3.22) */
	unsigned short Length;                         /* User Data Length (9.2.3.16), Command Data Length (9.2.3.20) */

	SMS_MessageCenter MessageCenter;               /* SMSC Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */
	SMS_Number RemoteNumber;                       /* Origination, destination, Recipient Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */
	unsigned char MessageText[GSM_MAX_SMS_LENGTH]; /* User Data (9.2.3.24), Command Data (9.2.3.21) */
	SMS_DataCodingScheme DCS;                      /* Data Coding Scheme (9.2.3.10) */
	SMS_MessageValidity Validity;                  /* Validity Period Format & Validity Period (9.2.3.3 & 9.2.3.12) - `Message validity' in the phone */
  
	unsigned short UDH_No;                         /* Number of presend UDHs */
	unsigned int UDH_Length;                       /* Length of the whole UDH */
	SMS_UDHInfo UDH[SMS_MAX_UDH_HEADER_NUMBER];    /* User Data Header Indicator & User Data Header (9.2.3.23 & 9.2.3.24) */

	SMS_DateTime SMSCTime;                         /* Service Centre Time Stamp (9.2.3.11) */
	SMS_DateTime Time;                             /* Discharge Time (9.2.3.13) */

	/* Other fields */
        SMS_MemoryType MemoryType;                     /* memoryType (for 6210/7110: folder indicator */
	SMS_MessageStatus Status;                      /* Status of the message: sent/read or unsent/unread */

//	SMS_CommandType Command;                       /* Command Type - 8 bits (9.2.3.19); FIXME: use it!!!! */
//	unsigned char Parameter[???];                  /* Parameter Indicator (9.2.3.27); FIXME: how to use it??? */
} GSM_SMSMessage;

/*** FOLDERS ***/

/* Maximal number of SMS folders */
#define MAX_SMS_FOLDERS 24

/* Datatype for SMS folders ins 6210/7110 */
typedef struct {
	char Name[15];     /* Name for SMS folder */
	bool SMSData;      /* if folder contains sender, SMSC number and sending date */
	u8 locations[160]; /* locations of SMS messages in that folder (6210 specific) */
	u8 number;         /* number of SMS messages in that folder*/
	u8 FolderID;       /* ID od fthe current folder */
} SMS_Folder;

typedef struct {
	SMS_Folder Folder[MAX_SMS_FOLDERS];
	u8 FolderID[MAX_SMS_FOLDERS]; /* ID specific for this folder and phone. */
	                               /* Used in internal functions. Do not use it. */
	u8 number;                     /* number of SMS folders */
} SMS_FolderList;

/*** CELL BROADCAST ***/

#define GSM_MAX_CB_MESSAGE         (160)

/* Define datatype for Cell Broadcast message */
typedef struct {
	int Channel;                                      /* channel number */
	char Message[GSM_MAX_CB_MESSAGE + 1];
	int New;
} GSM_CBMessage;

extern int EncodePDUSMS(GSM_SMSMessage *SMS, char *frame);
extern GSM_Error DecodePDUSMS(unsigned char *message, GSM_SMSMessage *SMS, int MessageLength);
/* Do not use these yet */
extern GSM_Error EncodeTextSMS();
extern GSM_Error DecodeTextSMS(unsigned char *message, GSM_SMSMessage *SMS);

/* FIXME: make this static */
extern char *GetBCDNumber(u8 *Number);

#endif /* __gnokii_sms_h_ */
