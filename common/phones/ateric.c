/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2001 Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002 Ladis Michl

  This file provides functions specific to at commands on ericsson
  phones. See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/ateric.h"
#include "links/atbus.h"


static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	gn_error ret;
	
	ret = at_memory_type_set(data->memory_status->memory_type, state);
	if (ret)
		return ret;
	if (sm_message_send(11, GN_OP_GetMemoryStatus, "AT+CPBR=?\r\n", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_GetMemoryStatus, data, state);
}

/*
 * Reminder: SonyEricsson doesn't return <used> and <total> values of memory usage:
 * > AT+CPBS?
 * +CPBS: "ME"
 */
static gn_error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	char *pos;

	buf.line1 = buffer;
	buf.length = length;
	splitlines(&buf);
	if (buf.line1 == NULL)
		return GN_ERR_INVALIDMEMORYTYPE;
	if (data->memory_status) {
		if (strstr(buf.line2, "+CPBR")) {
			pos = strchr(buf.line2, '-');
			if (pos) {
				data->memory_status->used = atoi(++pos);
				data->memory_status->free = 0;
			} else {
				return GN_ERR_NOTSUPPORTED;
			}
		}
	}
	return GN_ERR_NONE;
}

void at_ericsson_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	at_insert_recv_function(GN_OP_GetMemoryStatus, ReplyMemoryStatus, state);
	at_insert_send_function(GN_OP_GetMemoryStatus, GetMemoryStatus, state);
}
