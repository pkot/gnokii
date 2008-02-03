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

  Copyright (C) 2002-2003 BORBELY Zoltan <bozo@andrews.hu>
  Copyright (C) 2002      Pawel Kot

  Library for call management.

*/

#include "config.h"

#include <stdio.h>

#include "gnokii-internal.h"
#include "gnokii.h"

static gn_call calltable[GN_CALL_MAX_PARALLEL];

static gn_call *search_call(int call_id, struct gn_statemachine *state)
{
	int i;

	for (i = 0; i < GN_CALL_MAX_PARALLEL; i++)
		if (calltable[i].state == state && calltable[i].call_id == call_id)
			return calltable + i;
	return NULL;
}

GNOKII_API void gn_call_notifier(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state)
{
	gn_call *call;

	call = search_call(call_info->call_id, state);

	switch (call_status) {
	case GN_CALL_Incoming:
		if (call != NULL)
			break;
		if ((call = search_call(0, NULL)) == NULL) {
			dprintf("Call table overflow!\n");
			break;
		}
		call->state = state;
		call->call_id = call_info->call_id;
		call->status = GN_CALL_Ringing;
		call->type = call_info->type;
		snprintf(call->remote_number, sizeof(call->remote_number), "%s", call_info->number);
		snprintf(call->remote_name, sizeof(call->remote_name), "%s", call_info->name);
		gettimeofday(&call->start_time, NULL);
		memset(&call->answer_time, 0, sizeof(call->answer_time));
		call->local_originated = false;
		break;

	case GN_CALL_LocalHangup:
	case GN_CALL_RemoteHangup:
		if (call == NULL)
			break;
		memset(call, 0, sizeof(*call));
		call->status = GN_CALL_Idle;
		break;

	case GN_CALL_Established:
		if (call == NULL) {
			if ((call = search_call(0, NULL)) == NULL) {
				dprintf("Call table overflow!\n");
				break;
			}
			call->state = state;
			call->call_id = call_info->call_id;
			call->type = call_info->type;
			snprintf(call->remote_number, sizeof(call->remote_number), "%s", call_info->number);
			snprintf(call->remote_name, sizeof(call->remote_name), "%s", call_info->name);
			gettimeofday(&call->start_time, NULL);
			memcpy(&call->answer_time, &call->start_time, sizeof(call->answer_time));
			call->local_originated = false;
		} else
			gettimeofday(&call->answer_time, NULL);
		call->status = GN_CALL_Established;
		break;

	case GN_CALL_Resumed:
		if (call == NULL)
			break;
		call->status = GN_CALL_Established;
		break;

	case GN_CALL_Held:
		if (call == NULL)
			break;
		call->status = GN_CALL_Held;
		break;

	default:
		dprintf("Invalid call notification code: %d\n", call_status);
		break;
	}
}

GNOKII_API gn_error gn_call_dial(int *call_id, gn_data *data, struct gn_statemachine *state)
{
	gn_call *call;
	gn_error err;

	*call_id = -1;
	if ((call = search_call(0, NULL)) == NULL) {
		dprintf("Call table overflow!\n");
		return GN_ERR_INTERNALERROR;
	}

	if ((err = gn_sm_functions(GN_OP_MakeCall, data, state)) != GN_ERR_NONE)
		return err;

	call->state = state;
	call->call_id = data->call_info->call_id;
	call->status = GN_CALL_Dialing;
	call->type = data->call_info->type;
	snprintf(call->remote_number, sizeof(call->remote_number), "%s", data->call_info->number);
	snprintf(call->remote_name, sizeof(call->remote_name), "%s", data->call_info->name);
	gettimeofday(&call->start_time, NULL);
	memset(&call->answer_time, 0, sizeof(call->answer_time));
	call->local_originated = true;

	*call_id = call - calltable;

	return GN_ERR_NONE;
}

GNOKII_API gn_error gn_call_answer(int call_id)
{
	gn_data data;
	gn_call_info call_info;

	if (calltable[call_id].status == GN_CALL_Idle)
		return GN_ERR_NONE;

	memset(&call_info, 0, sizeof(call_info));
	call_info.call_id = calltable[call_id].call_id;
	gn_data_clear(&data);
	data.call_info = &call_info;

	return gn_sm_functions(GN_OP_AnswerCall, &data, calltable[call_id].state);
}

GNOKII_API gn_error gn_call_cancel(int call_id)
{
	gn_data data;
	gn_call_info call_info;

	if (calltable[call_id].status == GN_CALL_Idle)
		return GN_ERR_NONE;

	memset(&call_info, 0, sizeof(call_info));
	call_info.call_id = calltable[call_id].call_id;
	gn_data_clear(&data);
	data.call_info = &call_info;

	return gn_sm_functions(GN_OP_CancelCall, &data, calltable[call_id].state);
}

GNOKII_API gn_call *gn_call_get_active(int call_id)
{
	if (calltable[call_id].status == GN_CALL_Idle)
		return NULL;

	return calltable + call_id;
}

GNOKII_API gn_error gn_call_check_active(struct gn_statemachine *state)
{
	gn_data data;
	gn_call_active active[GN_CALL_MAX_PARALLEL];
	gn_call *call;
	gn_error err;
	int i, j, got;

	memset(active, 0, sizeof(*active));
	gn_data_clear(&data);
	data.call_active = active;

	/* initialize active call table */
	for (i = 0; i < GN_CALL_MAX_PARALLEL; i++)
		active[i].state = GN_CALL_Idle;

	if ((err = gn_sm_functions(GN_OP_GetActiveCalls, &data, state)) != GN_ERR_NONE)
		return (err == GN_ERR_NOTIMPLEMENTED || err == GN_ERR_NOTSUPPORTED) ? GN_ERR_NONE : err;

	/* delete terminated calls */
	for (j = 0; j < GN_CALL_MAX_PARALLEL; j++) {
		if (calltable[j].state != state)
			continue;
		got = 0;
		for (i = 0; i < GN_CALL_MAX_PARALLEL; i++) {
			if (calltable[j].call_id == active[i].call_id) {
				got = 1;
				break;
			}
		}
		if (!got) {
			memset(calltable + j, 0, sizeof(gn_call));
			calltable[j].status = GN_CALL_Idle;
		}
	}

	for (i = 0; i < GN_CALL_MAX_PARALLEL; i++) {
		if (active[i].state == GN_CALL_Idle)
			continue;

		dprintf("call state: %d\n", active[i].state);
		if (!(call = search_call(active[i].call_id, state))) {
			/* incoming call */
			if (active[i].state == GN_CALL_RemoteHangup || active[i].state == GN_CALL_LocalHangup)
				continue;
			if ((call = search_call(0, NULL)) == NULL) {
				dprintf("Call table overflow!\n");
				return GN_ERR_MEMORYFULL;
			}

			call->state = state;
			call->call_id = active[i].call_id;
			call->status = active[i].state;
			call->type = GN_CALL_Voice;
			snprintf(call->remote_number, sizeof(call->remote_number), "%s", active[i].number);
			snprintf(call->remote_name, sizeof(call->remote_name), "%s", active[i].name);
			gettimeofday(&call->start_time, NULL);
			memset(&call->answer_time, 0, sizeof(call->answer_time));
			call->local_originated = false;

		} else {
			/* already registered */
			if (active[i].state == GN_CALL_RemoteHangup || active[i].state == GN_CALL_LocalHangup) {
				memset(call, 0, sizeof(gn_call));
				call->status = GN_CALL_Idle;
			} else {
				if (call->status != GN_CALL_Established && active[i].state == GN_CALL_Established) {
					gettimeofday(&call->answer_time, NULL);
				}
				call->status = active[i].state;
			}
		}
	}

	return GN_ERR_NONE;
}
