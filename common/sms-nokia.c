/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2002-2004 Pawel Kot
  Copyright (C) 2002      Ladis Michl, BORBELY Zoltan

  Various function used by Nokia in their SMS extension protocols.

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "sms-nokia.h"

#include "gnokii-internal.h"
#include "gnokii.h"

/**
 * PackSmartMessagePart - Adds Smart Message header to the certain part of the message
 * @msg: place to store the header
 * @size: size of the part
 * @type: type of the part
 * @first: indicator of the first part
 *
 * This function adds the header as specified in the Nokia Smart Messaging
 * Specification v3.
 */
int sms_nokia_smart_message_part_pack(unsigned char *msg, unsigned int size,
				      unsigned int type, bool first)
{
	unsigned char current = 0;

	if (first) msg[current++] = 0x30; /* SM version. Here 3.0 */
	msg[current++] = type;
	msg[current++] = (size & 0xff00) >> 8; /* Length for picture part, hi */
	msg[current++] = size & 0x00ff;        /* length lo */
	return current;
}

/* Returns used length */
int sms_nokia_text_encode(unsigned char *text, unsigned char *message, bool first)
{
	int len, current = 0;
	/* FIXME: unicode length is not as simple as strlen */
	int type = GN_SMS_MULTIPART_DEFAULT;

	len = strlen(text);

	current = sms_nokia_smart_message_part_pack(message, len, type, first);

	if (type == GN_SMS_MULTIPART_UNICODE)
		len = char_unicode_encode(message + current, text, strlen(text));
	else
		memcpy(message + current, text, strlen(text));
	current += len;
	return current;
}

int sms_nokia_bitmap_encode(gn_bmp *bitmap, unsigned char *message, bool first)
{
	unsigned int current;

	/* FIXME: allow for the different sizes */
	current = sms_nokia_smart_message_part_pack(message, 256, GN_SMS_MULTIPART_BITMAP, first);
	return gn_bmp_sms_encode(bitmap, message + current) + current;
}
