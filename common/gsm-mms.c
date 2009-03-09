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

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "gnokii-internal.h"

/**
 * gn_mms_nokia2pdu - fill MMS fields from a buffer containing an MMS in Nokia file format
 *
 * @source_buffer: a pointer to the buffer containing the MMS
 * @source_length: the number of octects in @source_buffer
 * @mms: a pointer to the @mms to fill with decoded data
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

	/* Decode Nokia header */

	char_unicode_decode(string, nokia_header + 12, sizeof(string));
	mms->subject = strdup(string);
	char_unicode_decode(string, nokia_header + 94, sizeof(string));
	mms->from = strdup(string);

	return GN_ERR_NONE;
}

/**
 * gn_mms_nokia2pdu - extract the PDU from a buffer containing an MMS in Nokia file format
 *
 * @source_buffer: a pointer to the buffer containing the MMS
 * @source_length: the number of octects in @source_buffer
 * @dest_buffer: a pointer to the buffer in which the PDU will be copied (must be free()'d by the caller)
 * @dest_length: the number of octects in @dest_buffer
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
		dprintf("ERROR: total_length <= mms_length (%d != %d)\n", total_length, mms_length);
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
 * gn_mms_get- High-level function for reading MMS
 * @data: GSM data for the phone driver
 * @state: current statemachine state
 *
 * This function is the frontend for reading MMS. Note that mms field
 * in the gn_data structure must be initialized and @data->mms->buffer
 * must be free()'d by the caller.
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

	switch (data->mms->buffer_format) {
	case GN_MMS_FORMAT_TEXT:
		error = gn_mms_nokia2mms(data->file->file, data->file->file_length, data->mms);
		break;
	case GN_MMS_FORMAT_MIME:
		error = GN_ERR_NOTIMPLEMENTED;
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

