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

  This file provides functions specific to at commands on ericsson
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
#include "phones/atsoer.h"
#include "links/atbus.h"

static gn_error se_at_memory_type_set(gn_memory_type mt, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_data data;
	char req[32];
	const char *memory_name;
	int len;
	gn_error ret = GN_ERR_NONE;

	if (mt != drvinst->memorytype) {
		memory_name = gn_memory_type2str(mt);
		if (!memory_name)
			return GN_ERR_INVALIDMEMORYTYPE;

		/* BC is "Own Business Card" as per the Sony Ericsson documentation */
		if (strcmp(memory_name, "ON") == 0)
			memory_name = "BC";
		len = snprintf(req, sizeof(req), "AT+CPBS=\"%s\"\r", memory_name);
		ret = sm_message_send(len, GN_OP_Init, req, state);
		if (ret != GN_ERR_NONE)
			return ret;
		gn_data_clear(&data);
		ret = sm_block_no_retry(GN_OP_Init, &data, state);
		if (ret != GN_ERR_NONE)
			return ret;
		drvinst->memorytype = mt;

		gn_data_clear(&data);
		ret = state->driver.functions(GN_OP_AT_GetMemoryRange, &data, state);
	}
	return ret;
}

static gn_error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_error error;
	char *buf = buffer;
	int counter = 0;

	if (!data->memory_status)
		return GN_ERR_INTERNALERROR;

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	while ((buf = strchr(buf, '\r'))) {
		buf++;
		if (strlen(buf) > 6 &&
			(!strncmp(buf, "+CPBR:", 6) || !strncmp(buf + 1, "+CPBR:", 6)))
			counter++;
	}
	data->memory_status->used = counter;
	data->memory_status->free = drvinst->memorysize - counter;

	dprintf("Got %d entries\n", counter);
	return error;
}


static gn_error AT_GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	at_driver_instance *drvinst = AT_DRVINST(state);
	gn_error ret = GN_ERR_NONE;
	char req[32];

	ret = se_at_memory_type_set(data->memory_status->memory_type,  state);
	if (ret)
		return ret;
	ret = state->driver.functions(GN_OP_AT_GetMemoryRange, data, state);
	if (ret)
		return ret;
	memset(req, 0, 32);
	snprintf(req, 31, "AT+CPBR=%d,%d\r", drvinst->memoryoffset + 1, drvinst->memorysize + drvinst->memoryoffset);
	ret = sm_message_send(strlen(req), GN_OP_GetMemoryStatus, req, state);
	if (ret)
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetMemoryStatus, data, state);
}

static gn_error AT_GetNetworkInfo(gn_data *data, struct gn_statemachine *state)
{
	if (!data->network_info)
		return GN_ERR_INTERNALERROR;

	/* Sony Ericsson phones can't do CREG=2 (only CREG=1), so just
	 * skip that and only do COPS */

	if (sm_message_send(9, GN_OP_GetNetworkInfo, "AT+COPS?\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetNetworkInfo, data, state);
}


void at_sonyericsson_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	/* Sony Ericssons support just mode 2 */
	AT_DRVINST(state)->cnmi_mode = 2;

	at_insert_send_function(GN_OP_GetMemoryStatus, AT_GetMemoryStatus, state);
	at_insert_recv_function(GN_OP_GetMemoryStatus, ReplyMemoryStatus, state);
	at_insert_send_function(GN_OP_GetNetworkInfo, AT_GetNetworkInfo, state);

}
