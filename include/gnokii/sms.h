/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for SMS library.

*/

#ifndef __gnokii_sms_h_
#define __gnokii_sms_h_

#include "misc.h"
#include "gsm-error.h"
#include "gsm-common.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"

/* Maximum length of SMS center name */

#define GSM_MAX_SMS_CENTER_NAME_LENGTH	20

/* Limits of SMS messages. */

#define GSM_MAX_SMS_CENTER_LENGTH       40
#define GSM_MAX_SENDER_LENGTH           40
#define GSM_MAX_DESTINATION_LENGTH      40

#define GSM_MAX_SMS_LENGTH             160
#define GSM_MAX_8BIT_SMS_LENGTH        140

#define SMS_MAX_PART_NUMBER              3

/* FIXME: what value should be here? (Pawel Kot) */
#define SMS_MAX_UDH_NUMBER              10

/* Maximal number of SMS folders */
#define MAX_SMS_FOLDERS 24
#define MAX_SMS_MESSAGES 190

/*** MEMORY INFO ***/

typedef struct {
	int Unread;		/* Number of unread messages */
	int Number;		/* Number of all messages */
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
	SMS_NumberType          Type;
	char			Name[GSM_MAX_SMS_CENTER_NAME_LENGTH];	/* Name of the SMSC. */
	int			DefaultName;				/* >= 1 if default name used, otherwise -1 */
	SMS_MessageFormat	Format;					/* SMS is sent as text/fax/paging/email. */
	SMS_ValidityPeriod	Validity;				/* Validity of SMS Message. */
	char			Number[GSM_MAX_SMS_CENTER_LENGTH];	/* Number of the SMSC. */
	char			Recipient[GSM_MAX_SMS_CENTER_LENGTH];	/* Number of the default recipient. */
} SMS_MessageCenter;

/*** SHORT MESSAGE CORE ***/

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

typedef enum {
	SMS_NoData       = 0x00,
	SMS_PlainText    = 0x01,
	SMS_BitmapData   = 0x02,
	SMS_RingtoneData = 0x03,
	SMS_OtherData    = 0x04
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
	unsigned int Number;	/* Number of message we get from GetSMSStatus */
	unsigned int Unread;	/* Number of unread messages we get from GetSMSStatus */
	unsigned int Changed;	/* because when a message is moved between folders status wouldn't change */
	unsigned int NumberOfFolders;	/* Number of Folders we get from GetFolders */
} SMS_Status;

typedef struct {
	SMS_DataType Type;
	unsigned int Length;
	union {
		unsigned char Text[GSM_MAX_SMS_LENGTH];
		GSM_Bitmap Bitmap;
		GSM_Ringtone Ringtone;
	} u;
} SMS_UserData;

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
	SMS_UserData UserData[SMS_MAX_PART_NUMBER];    /* User Data (9.2.3.24), Command Data (9.2.3.21), extened to Nokia Multipart Messages from Smart Messaging Specification 3.0.0 */
	SMS_DataCodingScheme DCS;                      /* Data Coding Scheme (9.2.3.10) */
	SMS_MessageValidity Validity;                  /* Validity Period Format & Validity Period (9.2.3.3 & 9.2.3.12) - `Message validity' in the phone */

	unsigned short UDH_No;                         /* Number of present UDHs */
	unsigned int UDH_Length;                       /* Length of the whole UDH */
	SMS_UDHInfo UDH[SMS_MAX_UDH_NUMBER];           /* User Data Header Indicator & User Data Header (9.2.3.23 & 9.2.3.24) */

	SMS_DateTime SMSCTime;                         /* Service Centre Time Stamp (9.2.3.11) */
	SMS_DateTime Time;                             /* Discharge Time (9.2.3.13) */

	/* Other fields */
	SMS_MemoryType MemoryType;                     /* memoryType (for 6210/7110: folder indicator */
	SMS_MessageStatus Status;                      /* Status of the message: sent/read or unsent/unread */

//	SMS_CommandType Command;                       /* Command Type - 8 bits (9.2.3.19); FIXME: use it!!!! */
//	unsigned char Parameter[???];                  /* Parameter Indicator (9.2.3.27); FIXME: how to use it??? */
} GSM_SMSMessage;

/* Define the layout of the SMS message header */
/* Misc notes:
 *   - value -1 indicates in the location field means that the field is not
 *     supported,
 *   - when SMSC/Remote numbers have variable width all other fields should
 *     contain values as its value was 1 ('0x00' as the length) -- if field X
 *     follows SMSCNumber, X's locations would be SMSC's location + 1,
 *   - see the examples in common/phones/7110.c, commmon/phones/6100.c,
 *     common/phones/atgen.c.
 */
typedef struct {
	bool IsSupported;		/* Indicates if SMS is supported */

	short MessageCenter;		/* Location of the MessageCenter */
	bool IsMessageCenterCoded;	/* Indicates if the MessageCenter address is BCD coded */
	bool HasMessageCenterFixedLen;	/* Indicates if the MessageCenter field has always the fixed length */

	short MoreMessages;		/* Location of the MoreMessages bit */
	short ReplyViaSameSMSC;		/* Location of the ReplyPath bit */
	short RejectDuplicates;		/* Location of the RejectDuplicates bit */
	short Report;			/* Location of the Report bit */
	short Number;			/* Location of the MessageReference number */
	short Reference;		/* Location of the Reference bit */
	short PID;			/* Location of the ProtocolIdentifier bit */
	short ReportStatus;		/* Location of the ReportStatus bit */
	short Length;			/* Location of the UserDataLength field */
	short DataCodingScheme;		/* Location of the DataCodingScheme field */
	short UserDataHeader;		/* Location of the UserDataHeader indicator bit */

	short ValidityIndicator;	/* Location of the ValidityType Indicator field */
	short Validity;			/* Location of the Validity field */
	short ValidityLen;		/* Length ot the Validity field. -1 if the length is variable (as with GSM SPEC) */

	short RemoteNumber;		/* Location of the RemoteNumber */
	bool IsRemoteNumberCoded;	/* Indicates if the RemoteNumber address is BCD coded */
	bool HasRemoteNumberFixedLen;	/* Indicates if the MessageCenter field has always the fixed length */

	short SMSCTime;			/* Location of the SMSC Response time */
	short Time;			/* Location of the Delivery time */

	short MemoryType;		/* Location of the Memory Type field */
	short Status;			/* Location of the Status field */
	short UserData;			/* Location of the UserData field */
	bool IsUserDataCoded;		/* Indicates if the UserData should be PDU coded */
} SMSMessage_Layout;

/* Define set of SMS Headers for the phone series */
typedef struct {
	unsigned short Type;
	/* These are used to distinct between frames:
	 *    - saved in outbox/templates
	 *    - used to send message
	 * The only difference between them is the header length. We can
	 * distinct these two layouts only by the function from which we
	 * call it.
	 */
	unsigned short SendHeader;
	unsigned short ReadHeader;
	SMSMessage_Layout Deliver;
	SMSMessage_Layout Submit;
	SMSMessage_Layout DeliveryReport;
	SMSMessage_Layout Picture;
} SMSMessage_PhoneLayout;

extern SMSMessage_PhoneLayout layout;

/*** FOLDERS ***/

/* Datatype for SMS folders ins 6210/7110 */
typedef struct {
	char Name[15];     /* Name for SMS folder */
	bool SMSData;      /* if folder contains sender, SMSC number and sending date */
	unsigned int locations[MAX_SMS_MESSAGES]; /* locations of SMS messages in that folder (6210 specific) */
	unsigned int number;         /* number of SMS messages in that folder*/
	unsigned int FolderID;       /* ID od fthe current folder */
} SMS_Folder;

typedef struct {
	SMS_Folder Folder[MAX_SMS_FOLDERS];
	unsigned int FolderID[MAX_SMS_FOLDERS]; /* ID specific for this folder and phone. */
	                               /* Used in internal functions. Do not use it. */
	unsigned int number;                     /* number of SMS folders */
} SMS_FolderList;

/*** CELL BROADCAST ***/

#define GSM_MAX_CB_MESSAGE         (160)

/* Define datatype for Cell Broadcast message */
typedef struct {
	int Channel;                                      /* channel number */
	char Message[GSM_MAX_CB_MESSAGE + 1];
	int New;
} GSM_CBMessage;

/* Utils */
extern char *GetBCDNumber(u8 *Number);
extern int SemiOctetPack(char *Number, unsigned char *Output, SMS_NumberType type);
extern SMS_DateTime *UnpackDateTime(u8 *Number, SMS_DateTime *dt);

#endif /* __gnokii_sms_h_ */
