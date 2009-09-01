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

  Copyright (C) 2009 Daniele Forsi

  Include file for the MMS library.

*/

#ifndef _gnokii_mms_h
#define _gnokii_mms_h

/* WAP-230-WSP-20010705-a Table 38 "Well-Known Parameter Assignments" */
#define GN_MMS_Type_with_multipart	0x89 /* deprecated, use 0x99 */
#define GN_MMS_Start_with_multipart	0x8a /* deprecated, use 0x9a */

/*
 * From field
 *
 * WAP-209-MMSEncapsulation-20020105-a - Chapter 7.2.11 "From field"
 */
#define GN_MMS_Address_Present_Token	128
#define GN_MMS_Insert_Address_Token	129

/*
 * X-Mms-Message-Type
 *
 * WAP-209-MMSEncapsulation-20020105-a - Chapter 7.2.14 "Message-Type field"
 * OMA-MMS-ENC-V1_3-20050214-D - Chapter 7.3.28 "X-Mms-Message-Type field"
 * OMA-MMS-ENC-V1_2-20050301-A - (?? Chapter 6.1.1 or 7.2.23 ??)
 */
typedef enum {
	GN_MMS_MESSAGE_m_send_req	= 128,	/* an MMS that has to be sent */
	GN_MMS_MESSAGE_m_retrieve_conf	= 132,	/* an MMS that has been received */
	GN_MMS_MESSAGE_UNKNOWN		= 0	/* a convenience value, not defined by the standard */
} gn_mms_message;

/*
 * Content-Type
 * 
 * WAP-209-MMSEncapsulation-20020105-a - Chapter 7.2.4. "Content-Type field"
 * WAP-230-WSP-20010705-a Appendix A, Table 40 refers to http://www.wapforum.org/wina
 * See http://www.wapforum.org/wina/wsp-content-type.htm for full list
 * WAP-230-WSP-20010705-a - Chapter 8.4.2.24 "Content type field"
 * OMA-MMS-ENC-V1_3-20050214-D - Chapter 7.3.9 "Content-Type field"
 */
typedef enum {
	GN_MMS_CONTENT_application_vnd_wap_multipart_mixed	= 0xa3,
	GN_MMS_CONTENT_application_vnd_wap_multipart_related	= 0xb3,
	GN_MMS_CONTENT_text_plain	= 0x83,
	GN_MMS_CONTENT_image_jpeg	= 0x9e,
	GN_MMS_CONTENT_UNKNOWN		= 0	/* a convenience value, not defined by the standard */
} gn_mms_content;

/*
 * OMA-MMS-ENC-V1_3-20050214-D - Chapter 7.4 "Header Field Names and Assigned Numbers"
 * with msb set
 */
typedef enum {
	GN_MMS_Bcc			= 0x81,
	GN_MMS_Cc			= 0x82,
	GN_MMS_Content_Location		= 0x83,
	GN_MMS_Content_Type		= 0x84,
	GN_MMS_Date			= 0x85,
	GN_MMS_Delivery_Report		= 0x86,
	GN_MMS_Delivery_Time		= 0x87,
	GN_MMS_Expiry			= 0x88,
	GN_MMS_From			= 0x89,
	GN_MMS_Message_Class		= 0x8A,
	GN_MMS_Message_ID		= 0x8B,
	GN_MMS_Message_Type		= 0x8C,
	GN_MMS_MMS_Version		= 0x8D,
	GN_MMS_Message_Size		= 0x8E,
	GN_MMS_Priority			= 0x8F,
	GN_MMS_Read_Reply		= 0x90,
	GN_MMS_Report_Allowed		= 0x91,
	GN_MMS_Response_Status		= 0x92,
	GN_MMS_Response_Text		= 0x93,
	GN_MMS_Sender_Visibility	= 0x94,
	GN_MMS_Status			= 0x95,
	GN_MMS_Subject			= 0x96,
	GN_MMS_To			= 0x97,
	GN_MMS_Transaction_Id		= 0x98,
	GN_MMS_FIELD_ID_UNKNOWN		= 0	/* a convenience value, not defined by the standard */
} gn_mms_field_id;

/* FIXME check conformance before claiming to be 1.2, also do not hardcode string, use a function? */
#define GN_MMS_DEFAULT_VERSION 0x92
#define GN_MMS_DEFAULT_VERSION_STRING "1.2"
/* a convenience value, not defined by the standard */
#define GN_MMS_ANY_VERSION 0x00

/* Some useful values, not defined by the standard */
#define GN_MMS_MAX_VALUE_SIZE 80
#define GN_MMS_NOKIA_HEADER_LEN 0xb0
/* TODO implement Nokia footer with variable length */
#define GN_MMS_NOKIA_FOOTER_LEN 0x03

typedef struct {
	gn_mms_content type;
	char *name;
} gn_mms_content_type;

typedef enum {
	GN_MMS_PART_CONTENT_NAME_STRING	= 0x01
} gn_mms_part_flags;

typedef struct {
	gn_mms_part_flags flags;
	char *filename;
	char *content_type;
	size_t length;
	char *content;
} gn_mms_part;

typedef struct {
	gn_mms_field_id id;
	char type;
	int x;
	char *header;
} gn_mms_field;

typedef struct {
	gn_mms_field_id id;
	size_t length;
	char *value;
} gn_mms_field_value;

typedef enum {
	GN_MMS_FORMAT_UNKNOWN = 0,
	GN_MMS_FORMAT_TEXT,
	GN_MMS_FORMAT_MIME,
	GN_MMS_FORMAT_PDU,
	GN_MMS_FORMAT_RAW
} gn_mms_format;

typedef struct {
	unsigned int number;			/* Location of the message in the memory/folder. */
	gn_memory_type memory_type;		/* Memory type where the message is/should be stored. */
	gn_sms_message_status status;		/* Read, Unread, Sent, Unsent, ... */
	size_t buffer_length;
	unsigned char *buffer;
	int num_values;				/* How many items in values array */
	gn_mms_field_value *values[];
} gn_mms_raw;

typedef struct {
	unsigned int number;			/* Location of the message in the memory/folder. */
	gn_memory_type memory_type;		/* Memory type where the message is/should be stored. */
	gn_sms_message_status status;		/* Read, Unread, Sent, Unsent, ... */
	gn_mms_message type;			/* GN_MMS_X_Mms_Message_Type */
	char *tid;
	int version;
	char *from;				/* Sender of this message */
	char *to;				/* Recipient of this message */
	char *subject;
	gn_mms_format buffer_format;		/* What kind of information is stored in buffer */
	size_t buffer_length;
	unsigned char *buffer;
	int num_parts;				/* How many items in parts array */
	gn_mms_part *parts[];
} gn_mms;

/* Convenience values, not defined by the standard */
#define GN_MMS_FIELD_IS_STRING		1
#define GN_MMS_FIELD_IS_ENCSTRING	1 /* FIXME? */
#define GN_MMS_FIELD_IS_UINTVAR		2
#define GN_MMS_FIELD_IS_OCTECT		3
#define GN_MMS_FIELD_IS_YESNO		3 /* FIXME? */
#define GN_MMS_FIELD_IS_LONG		4
#define GN_MMS_FIELD_IS_DATE		5
#define GN_MMS_FIELD_IS_EXPIRY		6
#define GN_MMS_FIELD_IS_CONTENT_TYPE	7

#endif /* _gnokii_mms_h */
