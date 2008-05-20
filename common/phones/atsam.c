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

  Copyright 2004 Hugo Haas <hugo@larve.net>
  Copyright 2004 Pawel Kot
  Copyright 2007 Ingmar Steen <iksteen@gmail.com>

  This file provides functions specific to AT commands on Samsung
  phones. See README for more details on supported mobile phones.

*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atsam.h"
#include "links/atbus.h"

static char *sam_scan_entry(at_driver_instance *drvinst, char *buffer, gn_phonebook_entry *entry, gn_phonebook_entry_type type, gn_phonebook_number_type number_type, int ext_str)
{
	char *pos, *endpos;
	size_t len;
	int ix;
	if (!buffer)
		return NULL;

	if (!(pos = strstr(buffer, ",\"")))
		return NULL;
	pos += 2;

	if (ext_str) {
		if (!(endpos = strchr(pos, ',')))
			return NULL;
		*endpos = 0;
		len = atoi(pos);
		pos = endpos + 1;
		endpos = pos + len;
		*endpos = 0;
	} else {
		if (!(endpos = strstr(pos, "\",")))
			return NULL;
		*endpos = 0;
		len = strlen(pos);
	}

	if (len > 0) {
		ix = entry->subentries_count++;
		entry->subentries[ix].entry_type = type;
		entry->subentries[ix].number_type = number_type;
		at_decode(drvinst->charset, entry->subentries[ix].data.number, pos, len);
		if (entry->number[0] == 0 && type == GN_PHONEBOOK_ENTRY_Number)
			snprintf(entry->number, sizeof(entry->number), "%s", entry->subentries[ix].data.number);
	}

	return endpos + 1;
}

static char *sam_find_subentry(gn_phonebook_entry *entry, gn_phonebook_entry_type type)
{
	int i;
	for (i = 0; i < entry->subentries_count; ++i)
		if (entry->subentries[i].entry_type == type)
			return entry->subentries[i].data.number;
	return NULL;
}

static char *sam_find_number_subentry(gn_phonebook_entry *entry, gn_phonebook_number_type type)
{
	int i;
	for (i = 0; i < entry->subentries_count; ++i)
		if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number && entry->subentries[i].number_type == type)
			return entry->subentries[i].data.number;
	return NULL;
}

static gn_error Unsupported(gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NOTSUPPORTED;
}

static gn_error ReplyReadPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *pos, *first_name, *last_name, *tmp;
	gn_error error;
	size_t len = 0;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return (error == GN_ERR_UNKNOWN) ? GN_ERR_INVALIDLOCATION : error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	if (strncmp(buf.line1, "AT+SPBR=", 8)) {
		return GN_ERR_UNKNOWN;
	}

	if (!strncmp(buf.line2, "OK", 2)) {
		/* Empty phonebook location found */
		if (data->phonebook_entry) {
			data->phonebook_entry->number[0] = 0;
			data->phonebook_entry->name[0] = 0;
			data->phonebook_entry->caller_group = GN_PHONEBOOK_GROUP_None;
			data->phonebook_entry->subentries_count = 0;
			data->phonebook_entry->empty = true;
		}
		return GN_ERR_NONE;
	}
	if (strncmp(buf.line2, "+SPBR: ", 7))
		return GN_ERR_UNKNOWN;

	if (data->phonebook_entry) {
		gn_phonebook_entry *entry = data->phonebook_entry;
		data->phonebook_entry->number[0] = 0;
		data->phonebook_entry->name[0] = 0;
		entry->caller_group = GN_PHONEBOOK_GROUP_None;
		entry->subentries_count = 0;
		entry->empty = false;
		pos = buf.line2;

		/* store sub entries */
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Number, GN_PHONEBOOK_NUMBER_Mobile, 0);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Number, GN_PHONEBOOK_NUMBER_Home, 0);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Number, GN_PHONEBOOK_NUMBER_Work, 0);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Number, GN_PHONEBOOK_NUMBER_Fax, 0);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Number, GN_PHONEBOOK_NUMBER_General, 0);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Email, GN_PHONEBOOK_NUMBER_None, 0);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_FirstName, GN_PHONEBOOK_NUMBER_None, 1);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_LastName, GN_PHONEBOOK_NUMBER_None, 1);
		pos = sam_scan_entry(drvinst, pos, entry, GN_PHONEBOOK_ENTRY_Note, GN_PHONEBOOK_NUMBER_None, 1);

		/* compile a name out of first name + last name */
		first_name = sam_find_subentry(entry, GN_PHONEBOOK_ENTRY_FirstName);
		last_name = sam_find_subentry(entry, GN_PHONEBOOK_ENTRY_LastName);
		if(first_name || last_name) {
			if (first_name)
				len += strlen(first_name);
			if (last_name)
				len += strlen(last_name);
			if (!(tmp = (char *)malloc(len + 2))) /* +2 for \0 and space */
				return GN_ERR_INTERNALERROR;
			tmp[0] = 0;
			if (first_name) {
				if (strlen(first_name) + strlen(entry->name) + 1 > sizeof(entry->name))
					return GN_ERR_FAILED;
				strncat(entry->name, first_name, strlen(first_name));
				if (last_name)
					strncat(entry->name, " ", strlen(" "));
			}
			if (strlen(last_name) + strlen(entry->name) + 1 > sizeof(entry->name))
				return GN_ERR_FAILED;
			if (last_name)
				strncat(entry->name, last_name, strlen (last_name));
			free(tmp);
		}
	}
	return GN_ERR_NONE;
}

static gn_error AT_ReadPhonebook(gn_data *data, struct gn_statemachine *state) {
	at_driver_instance *drvinst = AT_DRVINST(state);
	char req[32];
	gn_error ret;

	ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
	if (ret)
		return ret;
	ret = at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;
	snprintf(req, sizeof(req), "AT+SPBR=%d\r", data->phonebook_entry->location+drvinst->memoryoffset);
	if (sm_message_send(strlen(req), GN_OP_ReadPhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_ReadPhonebook, data, state);
}

static gn_error AT_DeletePhonebook(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int len;
	char req[64];
	gn_error ret;

	if (!data->phonebook_entry)
		return GN_ERR_INTERNALERROR;

	ret = at_memory_type_set(data->phonebook_entry->memory_type, state);
	if (ret)
		return ret;

	len = snprintf(req, sizeof(req), "AT+SPBW=%d\r", data->phonebook_entry->location+drvinst->memoryoffset);

	if (sm_message_send(len, GN_OP_DeletePhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_DeletePhonebook, data, state);
}

#define MAX_REQ 2048
static gn_error AT_WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	int len, ofs;
	char req[MAX_REQ + 1], tmp[MAX_REQ + 1];
	char *mobile, *home, *work, *fax, *general, *email, *first_name, *last_name, *note;
	gn_phonebook_entry *entry = data->phonebook_entry;
	gn_data data2;
	gn_memory_status memstat;
	gn_error ret;
	int ix;

	if (entry->empty)
		return AT_DeletePhonebook(data, state);

	ret = at_memory_type_set(entry->memory_type, state);
	if (ret)
		return ret;

	ret = state->driver.functions(GN_OP_AT_SetCharset, data, state);
	if (ret)
		return ret;

	gn_data_clear(&data2);
	data2.memory_status = &memstat;
	ret = state->driver.functions(GN_OP_GetMemoryStatus, &data2, state);
	if (ret)
		return ret;

	if(entry->location > memstat.used)
		ix = 0;
	else
		ix = entry->location+drvinst->memoryoffset;

	mobile = sam_find_number_subentry(entry, GN_PHONEBOOK_NUMBER_Mobile);
	home = sam_find_number_subentry(entry, GN_PHONEBOOK_NUMBER_Home);
	work = sam_find_number_subentry(entry, GN_PHONEBOOK_NUMBER_Work);
	fax = sam_find_number_subentry(entry, GN_PHONEBOOK_NUMBER_Fax);
	general = sam_find_number_subentry(entry, GN_PHONEBOOK_NUMBER_General);
	if (!(mobile || home || work || fax || general) && entry->number[0] != 0)
		mobile = entry->number;
	email = sam_find_subentry(entry, GN_PHONEBOOK_ENTRY_Email);
	first_name = sam_find_subentry(entry, GN_PHONEBOOK_ENTRY_FirstName);
	last_name = sam_find_subentry(entry, GN_PHONEBOOK_ENTRY_LastName);
	if (!(first_name || last_name) && entry->name[0] != 0)
		first_name = entry->name;
	note = sam_find_subentry(entry, GN_PHONEBOOK_ENTRY_Note);

	ofs = snprintf(req, MAX_REQ, "AT+SPBW=%d,\"", ix);

	if (mobile)
		ofs += at_encode(drvinst->charset, req + ofs, MAX_REQ - ofs, mobile, strlen(mobile)) - 1;
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (home)
		ofs += at_encode(drvinst->charset, req + ofs, MAX_REQ - ofs, home, strlen(home)) - 1;
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (work)
		ofs += at_encode(drvinst->charset, req + ofs, MAX_REQ - ofs, work, strlen(work)) - 1;
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (fax)
		ofs += at_encode(drvinst->charset, req + ofs, MAX_REQ - ofs, fax, strlen(fax)) - 1;
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (general)
		ofs += at_encode(drvinst->charset, req + ofs, MAX_REQ - ofs, general, strlen(general)) - 1;
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (email)
		ofs += at_encode(drvinst->charset, req + ofs, MAX_REQ - ofs, email, strlen(email)) - 1;
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (first_name) {
		len = at_encode(drvinst->charset, tmp, MAX_REQ, first_name, strlen(first_name)) - 1;
		ofs += snprintf(req + ofs, MAX_REQ - ofs, "%d,", len);
		memcpy(req + ofs, tmp, len + 1);
		ofs += len;
	} else
		ofs += snprintf(req + ofs, MAX_REQ - ofs, "0,");
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (last_name) {
		len = at_encode(drvinst->charset, tmp, MAX_REQ, last_name, strlen(last_name)) - 1;
		ofs += snprintf(req + ofs, MAX_REQ - ofs, "%d,", len);
		memcpy(req + ofs, tmp, len + 1);
		ofs += len;
	} else
		ofs += snprintf(req + ofs, MAX_REQ - ofs, "0,");
	strncat(req, "\",\"", MAX_REQ - ofs);
	ofs += 3;

	if (note) {
		len = at_encode(drvinst->charset, tmp, MAX_REQ, note, strlen(note)) - 1;
		ofs += snprintf(req + ofs, MAX_REQ - ofs, "%d,", len);
		memcpy(req + ofs, tmp, len + 1);
		ofs += len;
	} else
		ofs += snprintf(req + ofs, MAX_REQ - ofs, "0,");
	strncat(req, "\",\"0,\"\r", MAX_REQ - ofs);
	ofs += 7;
	req[ofs] = 0;

	if (sm_message_send(ofs, GN_OP_WritePhonebook, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_WritePhonebook, data, state);
}
#undef MAX_REQ

void at_samsung_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state) {
	at_insert_send_function(GN_OP_ReadPhonebook, AT_ReadPhonebook, state);
	at_insert_recv_function(GN_OP_ReadPhonebook, ReplyReadPhonebook, state);
	at_insert_send_function(GN_OP_WritePhonebook, AT_WritePhonebook, state);
	at_insert_send_function(GN_OP_DeletePhonebook, AT_DeletePhonebook, state);
	/* phone lacks many usefull commands :( */
	at_insert_send_function(GN_OP_GetBatteryLevel, Unsupported, state);
	at_insert_send_function(GN_OP_GetPowersource, Unsupported, state);
	at_insert_send_function(GN_OP_GetRFLevel, Unsupported, state);
}
