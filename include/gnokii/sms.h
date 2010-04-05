/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002      Pavel Machek, Markus Plail, BORBELY Zoltan
  Copyright (C) 2003      Ladis Michl

  Include file for the SMS library.

*/

#ifndef _gnokii_sms_h
#define _gnokii_sms_h

#include <gnokii/error.h>
#include <gnokii/common.h>
#include <gnokii/bitmaps.h>
#include <gnokii/ringtones.h>
#include <gnokii/encoding.h>

/* Maximum length of SMS center name */
#define GN_SMS_CENTER_NAME_MAX_LENGTH  20

/* Limits of SMS messages. */
#define GN_SMS_MAX_LENGTH             160
#define GN_SMS_8BIT_MAX_LENGTH        140
#define GN_SMS_LONG_MAX_LENGTH	    10240

#define GN_SMS_PART_MAX_NUMBER          3

/* FIXME: what value should be here? (Pawel Kot) */
#define GN_SMS_UDH_MAX_NUMBER          10

/* Maximal number of SMS folders */
#define GN_SMS_FOLDER_MAX_NUMBER       64
#define GN_SMS_MESSAGE_MAX_NUMBER    1024

#define GN_SMS_DATETIME_MAX_LENGTH      7
#define GN_SMS_SMSC_NUMBER_MAX_LENGTH  20
#define GN_SMS_NUMBER_MAX_LENGTH       20
#define GN_SMS_USER_DATA_MAX_LENGTH   256
#define GN_SMS_VP_MAX_LENGTH            8

/* flags for encoding/decoding PDU */
#define GN_SMS_PDU_DEFAULT		0
#define GN_SMS_PDU_NOSMSC		1

/*** MEMORY INFO ***/
typedef struct {
	int Unread;		/* Number of unread messages */
	int Number;		/* Number of all messages */
} gn_sms_memory_status;

typedef struct {
	/* Number of message we get from GetSMSStatus */
	unsigned int number;
	/* Number of unread messages we get from GetSMSStatus */
	unsigned int unread;
	/* when a message is moved between folders status wouldn't change */
	unsigned int changed;
	/* Number of Folders we get from GetFolders */
	unsigned int folders_count;
	/* Message store used for new received messages (this is used
	 * internally by AT_SetSMSMemoryType() in common/phones/atgen.c). */
	/* Note: this field is unused since revision 1.128 of common/phones/atgen.c */
	gn_memory_type new_message_store;
} gn_sms_status;


/*** USER DATA HEADER ***/
/* types of User Data Header */
typedef enum {
	GN_SMS_UDH_None                 = 0x00,
	GN_SMS_UDH_ConcatenatedMessages = 0x01,
	GN_SMS_UDH_Ringtone             = 0x02,
	GN_SMS_UDH_OpLogo               = 0x03,
	GN_SMS_UDH_CallerIDLogo         = 0x04,
	GN_SMS_UDH_MultipartMessage     = 0x05,
	GN_SMS_UDH_WAPvCard             = 0x06,
	GN_SMS_UDH_WAPvCalendar         = 0x07,
	GN_SMS_UDH_WAPvCardSecure       = 0x08,
	GN_SMS_UDH_WAPvCalendarSecure   = 0x09,
	GN_SMS_UDH_VoiceMessage         = 0x0a,
	GN_SMS_UDH_FaxMessage           = 0x0b,
	GN_SMS_UDH_EmailMessage         = 0x0c,
	GN_SMS_UDH_WAPPush              = 0x0d,
	GN_SMS_UDH_OtherMessage         = 0x0e,
	GN_SMS_UDH_Unknown              = 0x0f
} gn_sms_udh_type;

GNOKII_API const char *gn_sms_udh_type2str(gn_sms_udh_type t);

typedef struct {
	gn_sms_udh_type type;
	union {
		struct {
			unsigned short reference_number;
			unsigned short maximum_number;
			unsigned short current_number;
		} concatenated_short_message; /* SMS_ConcatenatedMessages */
		struct {
			int store;
			unsigned short message_count;
		} special_sms_message_indication; /* GN_SMS_VoiceMessage, GN_SMS_FaxMessage,
						     GN_SMS_EmailMessage, GN_SMS_OtherMessage */
		struct {
			char network_code[6];
			unsigned int width, height;
		} logo; /* GN_SMS_OpLogo, GN_SMS_CallerIDLogo */
		struct {
			unsigned int notes; /* Number of the notes */
		} ringtone; /* GN_SMS_Ringtone */
	} u;
} gn_sms_udh_info;

typedef struct {
	unsigned int number; /* Number of the present UDH */
	unsigned int length;
	gn_sms_udh_info udh[GN_SMS_UDH_MAX_NUMBER];
} gn_sms_udh;

typedef enum {
	GN_SMS_MW_PID,  /* Set Protocol Identifier to `Return Call Message' */
	GN_SMS_MW_DCS,  /* Set Data Coding Scheme "to indicate the type of
			 * message waiting and whether there are some messages
			 * or no messages" */
	GN_SMS_MW_UDH   /* Use User Data Header - Special SMS Message
			 * Indication; the maximium level of information,
			 * may not be supported	by all phones */
} gn_sms_message_waiting_type;

/*** DATA CODING SCHEME ***/
typedef enum {
	GN_SMS_DCS_GeneralDataCoding,
	GN_SMS_DCS_MessageWaiting
} gn_sms_dcs_type;

typedef enum {
	GN_SMS_DCS_DefaultAlphabet = 0x00,
	GN_SMS_DCS_8bit            = 0x01,
	GN_SMS_DCS_UCS2            = 0x02
} gn_sms_dcs_alphabet_type;

typedef enum {
	GN_SMS_DCS_VoiceMail = 0x00,
	GN_SMS_DCS_Fax       = 0x01,
	GN_SMS_DCS_Email     = 0x02,
	GN_SMS_DCS_Text      = 0x03,
	GN_SMS_DCS_Other     = 0x04
} gn_sms_dcs_indication_type;

typedef struct {
	gn_sms_dcs_type type;
	union {
		struct {
			/* Message class: 
			 * 0 - no class
			 * 1 - Class 0
			 * 2 - Class 1
			 * 3 - Class 2
			 * 4 - Class 3 */
			unsigned short m_class;
			int compressed;
			gn_sms_dcs_alphabet_type alphabet;
		} general;
		struct {
			int discard;
			gn_sms_dcs_alphabet_type alphabet; /* ucs16 not supported */
			int active;
			gn_sms_dcs_indication_type type;
		} message_waiting;
	} u;
} gn_sms_dcs;

/*** VALIDITY PERIOD ***/
typedef enum {
	GN_SMS_VP_None           = 0x00,
	GN_SMS_VP_EnhancedFormat = 0x01,
	GN_SMS_VP_RelativeFormat = 0x02,
	GN_SMS_VP_AbsoluteFormat = 0x03
} gn_sms_vp_format;

typedef enum {
	GN_SMS_VPE_None              = 0x00,
	GN_SMS_VPE_RelativeFormat    = 0x01,
	GN_SMS_VPE_RelativeSeconds   = 0x02, /* Only one octet more is used */
	GN_SMS_VPE_RelativeSemiOctet = 0x03  /* 3 octets contain relative time in hours, minutes and seconds in semi-octet representation */
} gn_sms_vp_enhanced_type;

typedef struct {
	int extension; /* we need to set it to 0 at the moment; FIXME: how to handle `1' here? */
	int single_shot;
	gn_sms_vp_enhanced_type type;
	union {
		unsigned short relative;
		unsigned short seconds;
		gn_timestamp hms;
	} period;
} gn_sms_vp_enhanced;

/* Validity of SMS Messages. */
typedef enum {
	GN_SMS_VP_1H   = 0x0b,
	GN_SMS_VP_6H   = 0x47,
	GN_SMS_VP_24H  = 0xa7,
	GN_SMS_VP_72H  = 0xa9,
	GN_SMS_VP_1W   = 0xad,
	GN_SMS_VP_Max  = 0xff
} gn_sms_vp_time;

GNOKII_API const char *gn_sms_vp_time2str(gn_sms_vp_time t);

typedef struct {
	gn_sms_vp_format vpf;
	union {
		gn_sms_vp_enhanced enhanced;
		gn_sms_vp_time relative;	/* 8 bit */
		gn_timestamp absolute;
	} u;
} gn_sms_vp;


/*** MESSAGE CENTER ***/

typedef enum {
	GN_SMS_MF_Text   = 0x00, /* Plain text message. */
	GN_SMS_MF_Fax    = 0x22, /* Fax message. */
	GN_SMS_MF_Voice  = 0x24, /* Voice mail message. */
	GN_SMS_MF_ERMES  = 0x25, /* ERMES message. */
	GN_SMS_MF_Paging = 0x26, /* Paging. */
	GN_SMS_MF_UCI    = 0x2d, /* Email message in 8110i. */
	GN_SMS_MF_Email  = 0x32, /* Email message. */
	GN_SMS_MF_X400   = 0x31  /* X.400 message. */
} gn_sms_message_format;

GNOKII_API const char *gn_sms_message_format2str(gn_sms_message_format t);

typedef struct {
	int id;				/* Number of the SMSC in the phone memory. */
	char name[GN_SMS_CENTER_NAME_MAX_LENGTH];	/* Name of the SMSC. */
	int default_name;		/* >= 1 if default name used, otherwise -1 */
	gn_sms_message_format format;	/* SMS is sent as text/fax/paging/email. */
	gn_sms_vp_time validity;	/* Validity of SMS Message. */
	gn_gsm_number smsc;		/* Number of the SMSC. */
	gn_gsm_number recipient;	/* Number of the default recipient. */
} gn_sms_message_center;

/*** SHORT MESSAGE CORE ***/

typedef enum {
	/* First 2 digits are taken from GSM 03.40 version 6.1.0 Release 1997
	 * Section 9.2.3.1; 3rd digit is to mark a report. */
	GN_SMS_MT_Deliver         = 0x00, /* 00 0 */
	GN_SMS_MT_DeliveryReport  = 0x01, /* 00 1 */
	GN_SMS_MT_Submit          = 0x02, /* 01 0 */
	GN_SMS_MT_SubmitReport    = 0x03, /* 01 1 */
	GN_SMS_MT_Command         = 0x04, /* 10 0 */
	GN_SMS_MT_StatusReport    = 0x05, /* 10 1 */
	/* Looks like Happy N*kia Engineers invention. Text, picture, outbox
	 * sent templates are needed for Nokia 6510 and family. */
	GN_SMS_MT_Picture         = 0x07,
	GN_SMS_MT_TextTemplate    = 0x08,
	GN_SMS_MT_PictureTemplate = 0x09,
	GN_SMS_MT_SubmitSent      = 0x0a
} gn_sms_message_type;

GNOKII_API const char *gn_sms_message_type2str(gn_sms_message_type t);

typedef enum {
	GN_SMS_CT_Enquiry            = 0x00, /* Enquiry relating to previosly
						submitted short message; sets
						SRR to 1 */
	GN_SMS_CT_CancelStatusReport = 0x01, /* Cancel Status Report Request
						relating to previously submitted
						short message; sets SRR to 0 */
	GN_SMS_CT_DeleteSM           = 0x02, /* Delete previously submitted
						Short Message; sets SRR to 0 */
	GN_SMS_CT_EnableStatusReport = 0x03  /* Enable Status Report Request
						relating to previously submitted
						short message; sets SRR to 0 */
} gn_sms_command_type;

typedef struct {
	gn_sms_command_type type;
} gn_sms_message_command;

/* Datatype for SMS Delivery Report Statuses */
typedef enum {
	GN_SMS_DR_Status_None = 0,
	GN_SMS_DR_Status_Invalid,
	GN_SMS_DR_Status_Delivered,
	GN_SMS_DR_Status_Pending,
	GN_SMS_DR_Status_Failed_Temporary,
	GN_SMS_DR_Status_Failed_Permanent /* FIXME: add more reasons for failure? */
} gn_sms_delivery_report_status;

/* Datatype for SMS status */
typedef enum {
	GN_SMS_Unknown= 0x00,
	GN_SMS_Read   = 0x01,
	GN_SMS_Unread = 0x03,
	GN_SMS_Sent   = 0x05,
	GN_SMS_Unsent = 0x07
} gn_sms_message_status;

GNOKII_API const char *gn_sms_message_status2str(gn_sms_message_status t);

typedef enum {
	GN_SMS_DATA_None      = 0x00,
	GN_SMS_DATA_Text      = 0x01,
	GN_SMS_DATA_Bitmap    = 0x02,
	GN_SMS_DATA_Ringtone  = 0x03,
	GN_SMS_DATA_iMelody   = 0x04,
	GN_SMS_DATA_Multi     = 0x05,
	GN_SMS_DATA_NokiaText = 0x06,
	GN_SMS_DATA_Animation = 0x07,
	GN_SMS_DATA_Concat    = 0x08,
	GN_SMS_DATA_WAPPush   = 0x09,
	GN_SMS_DATA_Other     = 0x0a
} gn_sms_data_type;

/*** FOLDER INFO ***/

typedef enum {
	GN_SMS_FLD_Old            = 0x00,
	GN_SMS_FLD_New            = 0x01,
	GN_SMS_FLD_Deleted        = 0x02,
	GN_SMS_FLD_ToBeRemoved    = 0x03,
	GN_SMS_FLD_NotRead        = 0x04,
	GN_SMS_FLD_NotReadHandled = 0x05,
	GN_SMS_FLD_Changed        = 0x06
} gn_sms_location_status;

typedef struct {
	gn_sms_location_status status;	/* deleted, new, old, ToBeRemoved */
	unsigned int location;
	gn_sms_message_type message_type;
} gn_sms_message_list;

typedef struct {
	int number;		/* -1 if folder is not supported by the phone */
	unsigned int unread;	/* only valid for INBOX */
	unsigned int changed;
	unsigned int used;	/* because 'Used' can vary from 'Number' when we have deleted messages */
} gn_sms_folder_stats;

typedef struct {
	unsigned char binary[GN_SMS_MAX_LENGTH];
	int curr, total;	/* Number of this part, total number of parts */
} gn_sms_multi;

typedef struct {
	int curr, total, serial;
} gn_sms_concat;

typedef struct {
	gn_sms_data_type type;
	unsigned int length;	/* Number of bytes used */
	unsigned int chars;	/* Number of chars used */
	union {
		unsigned char text[10 * GN_SMS_MAX_LENGTH + 1];
		gn_sms_multi multi;
		gn_bmp bitmap;
		gn_ringtone ringtone;
		gn_bmp animation[4];
		gn_sms_concat concat;
	} u;
	/* That should be in the union, but for delivery reports we already
	 * set text there. Currently we don't want to break API, so I put it here
	 * Pawel Kot, 2007-11-21
	 */
	gn_sms_delivery_report_status dr_status;
} gn_sms_user_data;

/* Define datatype for SMS messages exported to the user applications. */
typedef struct {
	/* General fields */
	gn_sms_message_type type;           /* Type of the message. */
	int delivery_report;                /* Do we request the delivery report? Only for setting. */
	gn_sms_message_status status;       /* Status of the message read/unread/sent/unsent. */
	unsigned int validity;              /* Message validity in minutes. Only for setting. */
	gn_memory_type memory_type;         /* Memory type where the message is/should be stored. */
	unsigned int number;                /* Location of the message in the memory/folder. */

	/* Number related fields */
	gn_gsm_number smsc;                 /* SMSC Number. */
	gn_gsm_number remote;               /* Remote (sender/recipient) number. */

	/* Data format fields */
	gn_sms_dcs dcs;
	gn_sms_user_data user_data[GN_SMS_PART_MAX_NUMBER];
	gn_sms_udh udh;

	/* Date fields */
	gn_timestamp smsc_time;             /* SMSC Timestamp. Only for reading. */
	gn_timestamp time;                  /* Delivery timestamp. Only for reading. */

	/* Additional fields. Read only. Set by gn_sms_send(). *reference needs to be freed when not needed anymore. */
	unsigned int parts;                 /* Number of parts. For SMS send request, it is set to number of SMS the message was splitted. */
	unsigned int *reference;            /* List of reference numbers of the message sent. */
} gn_sms;

/* Define datatype for SMS messages, describes precisely GSM Spec 03.40 */
typedef struct {
	unsigned int type;		/* Message Type Indicator - 2 bits (9.2.3.1) */
	int more_messages;		/* More Messages to Send (9.2.3.2) */
	int reply_via_same_smsc;	/* Reply Path (9.2.3.17) - `Reply via same centre' in the phone */
	int reject_duplicates;		/* Reject Duplicates (9.2.3.25) */
	int report;			/* Status Report (9.2.3.4, 9.2.3.5 & 9.2.3.26) - `Delivery reports' in the phone */

	unsigned int number;		/* Message Number - 8 bits (9.2.3.18) */
	unsigned int reference;		/* Message Reference - 8 bit (9.2.3.6) */
	unsigned int pid;		/* Protocol Identifier - 8 bit (9.2.3.9) */
	unsigned int report_status;	/* Status - 8 bit (9.2.3.15), Failure Cause (9.2.3.22) */

	unsigned char smsc_time[GN_SMS_DATETIME_MAX_LENGTH];   /* Service Centre Time Stamp (9.2.3.11) */
	unsigned char time[GN_SMS_DATETIME_MAX_LENGTH];       /* Discharge Time (9.2.3.13) */
	unsigned char message_center[GN_SMS_SMSC_NUMBER_MAX_LENGTH];/* SMSC Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */
	unsigned char remote_number[GN_SMS_NUMBER_MAX_LENGTH];    /* Origination, destination, Recipient Address (9.2.3.7, 9.2.3.8, 9.2.3.14) */

	unsigned int dcs;		/* Data Coding Scheme (9.2.3.10) */
	unsigned int length;		/* User Data Length (9.2.3.16), Command Data Length (9.2.3.20) */
	int udh_indicator;
	unsigned char user_data[GN_SMS_LONG_MAX_LENGTH];      /* User Data (9.2.3.24), Command Data (9.2.3.21), extended to Nokia Multipart Messages from Smart Messaging Specification 3.0.0 */
	int user_data_length;		/* Length of just previous field */

	gn_sms_vp_format validity_indicator;
	unsigned char validity[GN_SMS_VP_MAX_LENGTH];   /* Validity Period Format & Validity Period (9.2.3.3 & 9.2.3.12) - `Message validity' in the phone */

	/* Other fields */
	unsigned int memory_type;	/* MemoryType (for 6210/7110): folder indicator */
	unsigned int status;		/* Status of the message: sent/read or unsent/unread */
} gn_sms_raw;


/*** FOLDERS ***/

/*** Datatype for SMS folders ***/
/* Max name length is 32 characters and trailing \0 */
#define GN_SMS_FOLDER_NAME_MAX_LENGTH	33
typedef struct {
	/* Name for SMS folder. */
	char name[GN_SMS_FOLDER_NAME_MAX_LENGTH];
	/* if folder contains sender, SMSC number and sending date */
	int sms_data;
	/* locations of SMS messages in that folder (6210 specific) */
	unsigned int locations[GN_SMS_MESSAGE_MAX_NUMBER];
	/* number of SMS messages in that folder*/
	unsigned int number;
	/* ID of the current folder */
	unsigned int folder_id;
} gn_sms_folder;

typedef struct {
	gn_sms_folder folder[GN_SMS_FOLDER_MAX_NUMBER];
	/* ID specific for this folder and phone. Used in internal functions.
	 * Do not use it. */
	unsigned int folder_id[GN_SMS_FOLDER_MAX_NUMBER];
	/* number of SMS folders */
	unsigned int number;
} gn_sms_folder_list;

/*** CELL BROADCAST ***/

#define GN_CM_MESSAGE_MAX_LENGTH         160

/* Define datatype for Cell Broadcast message */
typedef struct {
	int channel;
	char message[GN_CM_MESSAGE_MAX_LENGTH + 1];
	int is_new;
} gn_cb_message;

GNOKII_API void gn_sms_default_submit(gn_sms *sms);
GNOKII_API void gn_sms_default_deliver(gn_sms *sms);

/* WAPPush */

typedef struct {
	unsigned char wsp_tid;
	unsigned char wsp_pdu;
	unsigned char wsp_hlen;
	unsigned char wsp_content_type;

    	unsigned char version; /* wbxml version */
    	unsigned char public_id;
    	unsigned char charset; /* default 106 = UTF-8 */
    	unsigned char stl;
} gn_wap_push_header;

typedef struct {
	gn_wap_push_header header;
	char *url;
    	char *text;
	char *data;
	int data_len; 
} gn_wap_push;

GNOKII_API void gn_wap_push_init(gn_wap_push *wp);
GNOKII_API gn_error gn_wap_push_encode(gn_wap_push *wp);

#endif /* _gnokii_sms_h */
