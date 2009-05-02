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

  Library for parsing and creating Multimedia Messaging Service messages (MMS).

*/

#include "config.h"
#include <time.h> /* for ctime() */

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "gnokii-internal.h"

/**
 * mms_fields - mapping of all headers defined by the standard
 *
 * See OMA-MMS-ENC-V1_3-20050214-D - Chapter 7.4 "Header Field Names and Assigned Numbers"
 */
gn_mms_field mms_fields[] = {
	{GN_MMS_Bcc,			GN_MMS_FIELD_IS_ENCSTRING,	0, "Bcc"},
	{GN_MMS_Cc,			GN_MMS_FIELD_IS_ENCSTRING,	0, "Cc"},
	{GN_MMS_Content_Location,	GN_MMS_FIELD_IS_STRING,		0, "Content-Location"},
	{GN_MMS_Content_Type,		GN_MMS_FIELD_IS_CONTENT_TYPE,	0, "Content-Type"},
	{GN_MMS_Date,			GN_MMS_FIELD_IS_DATE,		0, "Date"},
	{GN_MMS_Delivery_Report,	GN_MMS_FIELD_IS_YESNO,		1, "Delivery-Report"},
	{GN_MMS_Delivery_Time,		GN_MMS_FIELD_IS_OCTECT,		1, "Delivery-Time"},
	{GN_MMS_Expiry,			GN_MMS_FIELD_IS_EXPIRY,		1, "Expiry"},
	{GN_MMS_From,			GN_MMS_FIELD_IS_STRING,		0, "From"},
	{GN_MMS_Message_Class,		GN_MMS_FIELD_IS_OCTECT,		1, "Message-Class"},
	{GN_MMS_Message_ID,		GN_MMS_FIELD_IS_STRING,		1, "Message-ID"},
	{GN_MMS_Message_Type,		GN_MMS_FIELD_IS_OCTECT,		1, "Message-Type"},
	{GN_MMS_MMS_Version,		GN_MMS_FIELD_IS_OCTECT,		1, "MMS-Version"},
	{GN_MMS_Message_Size,		GN_MMS_FIELD_IS_LONG,		1, "Message-Size"},
	{GN_MMS_Priority,		GN_MMS_FIELD_IS_OCTECT,		1, "Priority"},
	{GN_MMS_Read_Reply,		GN_MMS_FIELD_IS_YESNO,		1, "Read-Reply"},
	{GN_MMS_Report_Allowed,		GN_MMS_FIELD_IS_YESNO,		1, "Report-Allowed"},
	{GN_MMS_Response_Status,	GN_MMS_FIELD_IS_OCTECT,		1, "Response-Status"},
	{GN_MMS_Response_Text,		GN_MMS_FIELD_IS_ENCSTRING,	1, "Response-Text"},
	{GN_MMS_Sender_Visibility,	GN_MMS_FIELD_IS_OCTECT,		1, "Sender-Visibility"},
	{GN_MMS_Status,			GN_MMS_FIELD_IS_OCTECT,		1, "Status"},
	{GN_MMS_Subject,		GN_MMS_FIELD_IS_ENCSTRING,	0, "Subject"},
	{GN_MMS_To,			GN_MMS_FIELD_IS_ENCSTRING,	0, "To"},
	{GN_MMS_Transaction_Id,		GN_MMS_FIELD_IS_STRING,		1, "Transaction-Id"}
};

/**
 * gn_mms_field_lookup - search the field corresponding to the given @id
 *
 * @id: the identifier of the field
 *
 * Return value: a pointer to @gn_mms_field or NULL if not found
 */
const gn_mms_field *gn_mms_field_lookup(char id)
{
	int i;

	id |= 0x80;
	for (i = 0; i < sizeof(mms_fields) / sizeof(mms_fields[0]); i++) {
		if (mms_fields[i].id == id) {
			return &mms_fields[i];
		}
	}
	return NULL;
}

/**
 * gn_mms_dec_uintvar - decode and unsigned integer of variable length
 * @source: to buffer to decode
 * @source_len: the number of bytes in the @source buffer
 * @number: the destination of the decoding
 * @decoded_len: the number of bytes from @source that have been converted
 *
 * Decodes at most 5 octects from @source into the unsigned integer @number (at most 32 bits).
 * See WAP-230-WSP-20010705-a Chapter 8.1.2 "Variable Length Unsigned Integers"
  *
 * Return value: a @gn_error code
 */
gn_error gn_mms_dec_uintvar(const unsigned char *source, size_t source_len, unsigned int *number, size_t *decoded_len)
{
	*decoded_len = 0;
	if (source_len < 1)
		return GN_ERR_WRONGDATAFORMAT;

	*number = 0;
	*decoded_len = 0;
	while (*decoded_len < 5) {
		(*decoded_len)++;
		*number <<= 7;
		*number += *source & 0x7f;
		/* number ends when "continuation bit" is unset */
		if (!(*source & 0x80))
			return GN_ERR_NONE;
		source++;
	}

	return GN_ERR_WRONGDATAFORMAT;
}

#define APPEND(buf, fmt, ...) \
do { \
	size_t count, newsize; \
	char *astring, *nbuf; \
	count = asprintf(&astring, fmt, __VA_ARGS__); \
	if (!astring) \
		return GN_ERR_MEMORYFULL; \
	newsize = *dest_length + count; \
	nbuf = realloc(buf, newsize + 1); \
	if (!nbuf) { \
		free(buf); \
		free(astring); \
		return GN_ERR_MEMORYFULL; \
	} \
	buf = nbuf; \
	strcat(buf + *dest_length, astring); \
	free(astring); \
	*dest_length = newsize; \
} while (0)

/**
 * gn_mms_pdu2txtmime - convert an MMS from PDU to human readable format or to MIME
 * @buffer: to buffer to decode
 * @length: the number of bytes in the @source buffer
 * @dest_buffer: the destination of the decoding; must be free()'d by the caller
 * @dest_length: a pointer to the number of octects in @dest_buffer
 * @mime: 0 to convert to text, 1 to convert to MIME
 *
 * Return value: a @gn_error code; in case of error @length contain the position of the error
 */
static gn_error gn_mms_pdu2txtmime(unsigned const char *buffer, size_t *length, unsigned char **dest_buffer, size_t *dest_length, int mime)
{
	size_t i, j, decoded_len;
	const gn_mms_field *field;
	unsigned int number;
	gn_error error;
	char string[256];

	*dest_length = 0;
	*dest_buffer = calloc(1, *dest_length + 1);
	if (!*dest_buffer)
		return GN_ERR_NONE;
	for (i = 0; i < *length; i++) {
		if ((buffer[i] & 0x80) == 0x80) {
			field = gn_mms_field_lookup(buffer[i]);
			/* TODO check if decoding overrides the input buffer */
			if (field) {
				if (mime && field->x)
					APPEND(*dest_buffer, "%s", "X-");
				/*
				 * Decode according to WAP-230-WSP-20010705-a, Approved Version 5 July 2001 8.4.2.1 Basic rules
				 */
				i++;
				switch (field->type) {
				case GN_MMS_FIELD_IS_OCTECT:
					/*
					 * Decode a Short-Integer - could use gn_mms_dec_uintvar() but it's easier this way
					 */
					APPEND(*dest_buffer, "%s: 0x%x\n", field->header, buffer[i] & 0x7f);
					break;
				case GN_MMS_FIELD_IS_LONG:
					/* FALL THROUGH */
				case GN_MMS_FIELD_IS_DATE:
					/*
					 * Decode a Long-Integer
					 */
					decoded_len = buffer[i];
					i++;
					number = 0;
					for (j = 0; j < decoded_len; j++) {
						number = number * 256 + buffer[i + j];
					}
					if (field->type == GN_MMS_FIELD_IS_DATE) {
						/* ctime() adds a trailing \n */
						APPEND(*dest_buffer, "%s: %s", field->header, ctime((time_t *) &number));
					} else {
						APPEND(*dest_buffer, "%s: %u\n", field->header, number);
					}
					decoded_len--;
					i += decoded_len;
					break;
				case GN_MMS_FIELD_IS_STRING:
					/*
					 * Decode Text-string, Quoted-string and Extension-media
					 */
					if (buffer[i] <= 30) {
						decoded_len = buffer[i];
						i++;
					} else if (buffer[i] == 31) {
						i++;
						error = gn_mms_dec_uintvar(&buffer[i], *length - i, &number, &decoded_len);
						if (error)
							return error;
						i += decoded_len;
						decoded_len = number;
						/* skip Address-present-token (FIXME: skipping makes sense only for received MMS) */
						i++;
						/* subtract 2 to skip Address-present-token and NUL terminator */
						decoded_len -= 2;
					} else if (buffer[i] == '"' || buffer[i] == 0x7f) {
						i++;
						decoded_len = strlen(&buffer[i]);
					} else {
						decoded_len = strlen(&buffer[i]);
					}
					strncpy(string, &buffer[i], decoded_len);
					string[decoded_len] = '\0';
					i += decoded_len;
					APPEND(*dest_buffer, "%s: %s\n", field->header, string);
					break;
				case GN_MMS_FIELD_IS_CONTENT_TYPE:
					/*
					 * Decode Content-Type
					 * See WAP-230-WSP-20010705-a, Approved Version 5 July 2001 8.4.2.24 Content type field
					 */
					APPEND(*dest_buffer, "%s: Decoding of Content-Type not yet implemented!\n", field->header);
					return GN_ERR_NONE;
				default:
					dprintf("Unhandled value type %d\n", field->type);
					return GN_ERR_INTERNALERROR;
				}
			} else {
				/* Text-Value */
				dprintf("Unknown field 0x%x\n", buffer[i]);
				return GN_ERR_FAILED;
			}
		} else {
			dprintf("Out of sync at offset 0x%x value 0x%x\n", i, buffer[i]);
			return GN_ERR_FAILED;
		}
	}
	return GN_ERR_NONE;
}

/**
 * gn_mms_pdu2txt - convert an MMS from PDU to human readable format
 * @buffer: to buffer to decode
 * @length: the number of bytes in the @source buffer
 * @dest_buffer: the destination of the decoding; must be free()'d by the caller
 * @dest_length: a pointer to the number of octects in @dest_buffer
 *
 * Return value: a @gn_error code; in case of error @length contain the position of the error
 */
gn_error gn_mms_pdu2txt(unsigned const char *buffer, size_t *length, unsigned char **dest_buffer, size_t *dest_length)
{
	return  gn_mms_pdu2txtmime(buffer, length, dest_buffer, dest_length, 0);
}

/**
 * gn_mms_pdu2mime - convert an MMS from PDU to MIME format
 * @buffer: to buffer to decode
 * @length: the number of bytes in the @source buffer
 * @dest_buffer: the destination of the decoding; must be free()'d by the caller
 * @dest_length: a pointer to the number of octects in @dest_buffer
 *
 * Return value: a @gn_error code; in case of error @length contain the position of the error
 */
gn_error gn_mms_pdu2mime(unsigned const char *buffer, size_t *length, unsigned char **dest_buffer, size_t *dest_length)
{
	return  gn_mms_pdu2txtmime(buffer, length, dest_buffer, dest_length, 1);
}

#undef APPEND

/**
 * gn_mms_nokia2pdu - extract the PDU from a buffer containing an MMS in Nokia file format
 *
 * @source_buffer: a pointer to the buffer containing the MMS
 * @source_length: the number of octects in @source_buffer
 * @dest_buffer: the address of a pointer to the destination of the decoding; must be free()'d by the caller
 * @dest_length: a pointer to the number of octects in @dest_buffer
 *
 * Return value: a @gn_error code
 */
gn_error gn_mms_nokia2pdu(const unsigned char *source_buffer, size_t source_length, unsigned char **dest_buffer, size_t *dest_length)
{
	const unsigned char *nokia_header, *pdu_start;
	size_t mms_length, total_length;

	if (source_length < GN_MMS_NOKIA_HEADER_LEN)
		return GN_ERR_WRONGDATAFORMAT;

	nokia_header = source_buffer;
	pdu_start = nokia_header + GN_MMS_NOKIA_HEADER_LEN;
	mms_length = (nokia_header[4] << 24) + (nokia_header[5] << 16) + (nokia_header[6] << 8) + nokia_header[7];
	total_length = (nokia_header[8] << 24) + (nokia_header[9] << 16) + (nokia_header[10] << 8) + nokia_header[11];

	dprintf("Nokia header length %d\n", GN_MMS_NOKIA_HEADER_LEN);
	dprintf("\tMMS length %d\n", mms_length);
	dprintf("\tFooter length %d\n", total_length - mms_length - GN_MMS_NOKIA_HEADER_LEN);
	dprintf("\tTotal length %d\n", total_length);

	if (total_length != source_length) {
		dprintf("ERROR: total_length != source_length (%d != %d)\n", total_length, source_length);
		return GN_ERR_WRONGDATAFORMAT;
	}
	if (total_length <= mms_length) {
		dprintf("ERROR: total_length <= mms_length (%d <= %d)\n", total_length, mms_length);
		return GN_ERR_WRONGDATAFORMAT;
	}

	*dest_buffer = malloc(mms_length);
	if (!*dest_buffer)
		return GN_ERR_MEMORYFULL;
	*dest_length = mms_length;

	memcpy(*dest_buffer, pdu_start, mms_length);

	return GN_ERR_NONE;
}

/**
 * gn_mms_nokia2mms - fill MMS fields from a buffer containing an MMS in Nokia file format
 *
 * @source_buffer: a pointer to the buffer containing the MMS
 * @source_length: the number of octects in @source_buffer
 * @mms: a pointer to the @mms to fill with decoded data
 *
 * Return value: a @gn_error code
 */
gn_error gn_mms_nokia2mms(const unsigned char *source_buffer, size_t source_length, gn_mms *mms)
{
	const unsigned char *nokia_header, *pdu_start;
	size_t mms_length, total_length;
	char string[80];

	if (source_length < GN_MMS_NOKIA_HEADER_LEN)
		return GN_ERR_WRONGDATAFORMAT;

	nokia_header = source_buffer;
	pdu_start = nokia_header + GN_MMS_NOKIA_HEADER_LEN;
	mms_length = (nokia_header[4] << 24) + (nokia_header[5] << 16) + (nokia_header[6] << 8) + nokia_header[7];
	total_length = (nokia_header[8] << 24) + (nokia_header[9] << 16) + (nokia_header[10] << 8) + nokia_header[11];

	dprintf("Nokia header length %d\n", GN_MMS_NOKIA_HEADER_LEN);
	dprintf("\tMMS length %d\n", mms_length);
	dprintf("\tFooter length %d\n", total_length - mms_length - GN_MMS_NOKIA_HEADER_LEN);
	dprintf("\tTotal length %d\n", total_length);

	if (total_length != source_length) {
		dprintf("ERROR: total_length != source_length (%d != %d)\n", total_length, source_length);
		return GN_ERR_WRONGDATAFORMAT;
	}
	if (total_length <= mms_length) {
		dprintf("ERROR: total_length <= mms_length (%d != %d)\n", total_length, mms_length);
		return GN_ERR_WRONGDATAFORMAT;
	}

	/* Decode Nokia header */

	char_unicode_decode(string, nokia_header + 12, sizeof(string));
	mms->subject = strdup(string);
	char_unicode_decode(string, nokia_header + 94, sizeof(string));
	mms->from = strdup(string);

	return GN_ERR_NONE;
}

/**
 * gn_mms_nokia2txt - fill a text buffer from a buffer containing an MMS in Nokia file format
 *
 * @source_buffer: a pointer to the buffer containing the MMS
 * @source_length: the number of octects in @source_buffer
 * @dest_buffer: the address of a pointer to the destination of the decoding; must be free()'d by the caller
 * @dest_length: a pointer to the number of octects in @dest_buffer
 *
 * Return value: a @gn_error code
 */
gn_error gn_mms_nokia2txt(const unsigned char *source_buffer, size_t source_length, unsigned char **dest_buffer, size_t *dest_length)
{
	gn_error error;
	unsigned char *pdu_buffer;
	size_t pdu_length;

	error = gn_mms_nokia2pdu(source_buffer, source_length, &pdu_buffer, &pdu_length);
	if (error != GN_ERR_NONE)
		return error;

	error = gn_mms_pdu2txt(pdu_buffer, &pdu_length, dest_buffer, dest_length);
	if (pdu_buffer)
		free(pdu_buffer);

	return error;
}

/**
 * gn_mms_nokia2mime - fill a buffer in MIME format from a buffer containing an MMS in Nokia file format
 *
 * @source_buffer: a pointer to the buffer containing the MMS
 * @source_length: the number of octects in @source_buffer
 * @dest_buffer: the address of a pointer to the destination of the decoding; must be free()'d by the caller
 * @dest_length: a pointer to the number of octects in @dest_buffer
 *
 * Return value: a @gn_error code
 */
gn_error gn_mms_nokia2mime(const unsigned char *source_buffer, size_t source_length, unsigned char **dest_buffer, size_t *dest_length)
{
	gn_error error;
	unsigned char *pdu_buffer;
	size_t pdu_length;

	error = gn_mms_nokia2pdu(source_buffer, source_length, &pdu_buffer, &pdu_length);
	if (error != GN_ERR_NONE)
		return error;

	error = gn_mms_pdu2mime(pdu_buffer, &pdu_length, dest_buffer, dest_length);
	if (pdu_buffer)
		free(pdu_buffer);

	return error;
}

/**
 * gn_mms_get- High-level function for reading MMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for reading MMS. Note that @mms field
 * in the @gn_data structure must be initialized and @data->mms->buffer
 * must be free()'d by the caller.
 *
 * Return value: a @gn_error code
 */
GNOKII_API gn_error gn_mms_get(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_mms_raw rawmms;

	if (!data->mms)
		return GN_ERR_INTERNALERROR;
	if (data->mms->number < 0)
		return GN_ERR_EMPTYLOCATION;

	rawmms.number = data->mms->number;
	rawmms.memory_type = data->mms->memory_type;
	data->raw_mms = &rawmms;
	dprintf("%s() memory %s location %d\n", __FUNCTION__, gn_memory_type2str(rawmms.memory_type), rawmms.number);
	error = gn_sm_functions(GN_OP_GetMMS, data, state);
	if (error)
		return error;

	if (!data->file)
		return GN_ERR_INTERNALERROR;

	data->mms->status = rawmms.status;

	switch (data->mms->buffer_format) {
	case GN_MMS_FORMAT_TEXT:
		error = gn_mms_nokia2txt(data->file->file, data->file->file_length, &data->mms->buffer, &data->mms->buffer_length);
		break;
	case GN_MMS_FORMAT_MIME:
		error = gn_mms_nokia2mime(data->file->file, data->file->file_length, &data->mms->buffer, &data->mms->buffer_length);
		break;
	case GN_MMS_FORMAT_PDU:
		error = gn_mms_nokia2pdu(data->file->file, data->file->file_length, &data->mms->buffer, &data->mms->buffer_length);
		break;
	case GN_MMS_FORMAT_RAW:
		data->mms->buffer = data->file->file;
		data->mms->buffer_length = data->file->file_length;
		data->file->file = NULL;
		break;
	default:
		error = GN_ERR_WRONGDATAFORMAT;
	}
	if (data->file->file) {
		free(data->file->file);
		data->file->file = NULL;
	}

	return error;
}

/**
 * gn_mms_delete - High-level function for deleting MMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for deleting MMS. Note that MMS field
 * in the gn_data structure must be initialized.
 */
GNOKII_API gn_error gn_mms_delete(gn_data *data, struct gn_statemachine *state)
{
	gn_mms_raw rawmms;

	if (!data->mms) return GN_ERR_INTERNALERROR;
	memset(&rawmms, 0, sizeof(gn_mms_raw));
	rawmms.number = data->mms->number;
	rawmms.memory_type = data->mms->memory_type;
	data->raw_mms = &rawmms;
	return gn_sm_functions(GN_OP_DeleteMMS, data, state);
}

/***
 *** OTHER FUNCTIONS
 ***/

/**
 * gn_mms_free - free an @mms
 * @mms: a pointer to a @gn_mms returned by gn_mms_alloc()
 *
 * Return value: a @gn_error code
 */
GNOKII_API gn_error gn_mms_free(gn_mms *mms)
{
	if (mms)
		free(mms);

	return GN_ERR_NONE;

}

/**
 * gn_mms_alloc - allocate an @mms
 * @mms: the address of a pointer to a gn_mms that must be freed with gn_mms_free()
 *
 * Return value: a @gn_error code
 */
GNOKII_API gn_error gn_mms_alloc(gn_mms **mms)
{
	*mms = calloc(1, sizeof(gn_mms));

	if (*mms)
		return GN_ERR_NONE;

	return GN_ERR_MEMORYFULL;
}
