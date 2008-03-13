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

  Copyright (C) 2001 Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002 Ladis Michl
  Copyright (C) 2003 Pawel Kot

  This file provides functions specific to at commands on nokia
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
#include "phones/atnok.h"
#include "links/atbus.h"

static at_send_function_type writephonebook;

static gn_error WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	if (writephonebook == NULL)
		return GN_ERR_UNKNOWN;
	if (data->memory_status && data->memory_status->memory_type == GN_MT_ME)
		return GN_ERR_NOTSUPPORTED;
	return (*writephonebook)(data, state);
}

static gn_error ReplyIncomingSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	at_line_buffer buf;
	char *memory_name, *pos;
	int index;
	gn_memory_type mem;
	int freesms = 0;
	gn_error error = GN_ERR_NONE;

	if (!drvinst->on_sms)
		return GN_ERR_UNSOLICITED;

	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);

	mem = GN_MT_XX;

	if (strncmp(buf.line1, "+CMTI: ", 7))
		return GN_ERR_UNSOLICITED;

	pos = strrchr(buf.line1, ',');
	if (pos == NULL)
		return GN_ERR_UNSOLICITED;
	pos[0] = '\0';
	pos++;
	index = atoi(pos);

	memory_name = strip_quotes(buf.line1 + 7);
	if (memory_name == NULL)
		return GN_ERR_UNSOLICITED;
	mem = gn_str2memory_type(memory_name);
	if (mem == GN_MT_XX)
		return GN_ERR_UNSOLICITED;

	/* Ugly workaround for at least Nokia behaviour. Reply is of form:
	 * +CMTI: <memory>, <location>
	 * <memory> is the memory where the message is stored and can be "ME" or "SM".
	 * <location> is a place in "MT" memory which consists of "SM" and "ME".
	 * order is that "SM" memory goes first, "ME" memory goes second.
	 * So if the memory is "ME" we need to substract from <location> size of "SM"
	 * memory to get the location (we cannot read from "MT" memory
	 */
	if (mem == GN_MT_ME) {
		if (drvinst->smmemorysize < 0)
			error = gn_sm_functions(GN_OP_AT_GetSMSMemorySize, data, state);
		/* ignore errors */
		if ((error == GN_ERR_NONE) && (index > drvinst->smmemorysize))
			index -= drvinst->smmemorysize;
	}

	dprintf("Received message folder %s index %d\n", gn_memory_type2str(mem), index);

	if (!data->sms) {
		freesms = 1;
		data->sms = calloc(1, sizeof(gn_sms));
		if (!data->sms)
			return GN_ERR_INTERNALERROR;
	}

	memset(data->sms, 0, sizeof(gn_sms));
	data->sms->memory_type = mem;
	data->sms->number = index;

	error = gn_sms_get(data, state);
	if (error == GN_ERR_NONE) {
		error = GN_ERR_UNSOLICITED;
		drvinst->on_sms(data->sms, state, drvinst->sms_callback_data);
	}

	if (freesms)
		free(data->sms);

	return error;
}

void at_nokia_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	/* block writing of phone memory on nokia phones other than */
	/* 8210. if you write to the phonebook of a eg 7110 all extended */
	/* information will be lost. */
	if (strncasecmp("8210", foundmodel, 4))
		writephonebook = at_insert_send_function(GN_OP_WritePhonebook, WritePhonebook, state);

	/* premicell does not want sms centers in PDU packets (send & */
	/* receive) */
	if (!strncasecmp("0301", foundmodel, 4))
		AT_DRVINST(state)->no_smsc = 1;

	/* Nokias support just mode 1 */
	AT_DRVINST(state)->cnmi_mode = 1;

	at_insert_recv_function(GN_OP_AT_IncomingSMS, ReplyIncomingSMS, state);
}
