/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for SMS library.

  $Log$
  Revision 1.1  2001-07-09 23:06:26  pkot
  Moved sms.* files from my hard disk to CVS


*/

#ifndef __gnokii_sms_h_
#define __gnokii_sms_h_

#include "misc.h"
#include "gsm-error.h"

#define SMS_MAX_SMS_LENGTH         160
#define SMS_MAX_ADDRESS_LENGTH      40

typedef struct {
	int Year;          /* The complete year specification - e.g. 1999. Y2K :-) */
	int Month;	     /* January = 1 */
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Timezone;      /* The difference between local time and GMT */
} SMS_DateTime;

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
	SMS_UnknownUDH           = 0x09
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
	SMS_Other     = 0x03
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

typedef struct {
	SMS_ValidityPeriodFormat VPF;
	union {
		SMS_EnhancedValidityPeriod Enhanced;
		unsigned short Relative; /* 8 bit */
		SMS_DateTime Absolute;
	} u;
} SMS_ValidityPeriod;

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

typedef struct {
	SMS_NumberType type;
	char number[SMS_MAX_ADDRESS_LENGTH];
} SMS_Number;

/* Define datatype for SMS Message Type */
//typedef enum {
//  SMS_MobileOriginated, /* Mobile Originated (MO) message - Outbox message */
//  SMS_MobileTerminated, /* Mobile Terminated (MT) message - Inbox message */
//  SMS_DeliveryReport /* Delivery Report */
//} SMS_MessageType;

typedef enum {                     /* Bits meaning */
	SMS_Deliver        = 0x00, /* 00 0 First 2 digits are taken from */
	SMS_Deliver_Report = 0x01, /* 00 1 GSM 03.40 version 6.1.0 Release 1997 */
	SMS_Status_Report  = 0x05, /* 10 1 Section 9.2.3.1; 3rd digit is to */
	SMS_Command        = 0x04, /* 10 0 mark a report */
	SMS_Submit         = 0x02, /* 01 0 */
	SMS_Submit_Report  = 0x03  /* 01 1 */
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
/* FIXME - This needs to be made clearer and or turned into a 
   bitfield to allow compound values (read | sent etc.) */
typedef enum {
	SMS_SENT   = 0x01,
	SMS_UNSENT = 0x00,
	SMS_READ   = 0x02,
	SMS_UNREAD = 0x03
} SMS_MessageStatus;

/* Define datatype for SMS messages, used for getting SMS messages from the
   phones memory. */
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
	SMS_Number SMSC;                               /* SMSC Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */
	SMS_Number Address;                            /* Origination, destination, Recipient Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */
	unsigned char Data[SMS_MAX_SMS_LENGTH];        /* User Data (9.2.3.24), Command Data (9.2.3.21) */
//	unsigned char Parameter[???];                  /* Parameter Indicator (9.2.3.27); FIXME: how to use it??? */
	SMS_DataCodingScheme DCS;                      /* Data Coding Scheme (9.2.3.10) */
	SMS_ValidityPeriod Validity;                   /* Validity Period Format & Validity Period (9.2.3.3 & 9.2.3.12) - `Message validity' in the phone */
	SMS_UDHInfo UDH;                               /* User Data Header Indicator & User Data Header (9.2.3.23 & 9.2.3.24) */
//	SMS_CommandType Command;                       /* Command Type - 8 bits (9.2.3.19); FIXME: use it!!!! */
	SMS_DateTime SMSCTime;                         /* Service Centre Time Stamp (9.2.3.11) */
	SMS_DateTime DischargeTime;                    /* Discharge Time (9.2.3.13) */

	/* Other fields */
	SMS_MessageStatus Status;                      /* Status of the message: sent/read or unsent/unread */
	unsigned short Location;                       /* Location of the message in the SIM/Phone memory */
} GSM_SMSMessage;

GSM_Error EncodePDUSMS(GSM_SMSMessage *SMS, char *frame);
GSM_Error DecodePDUSMS();

GSM_Error EncodeTextSMS();
GSM_Error DecodeTextSMS();

#endif /* __gnokii_sms_h_ */
