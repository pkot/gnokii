/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2002, BORBELY Zoltan <bozo@andrews.hu>

  Library for call management.

*/

#include <stdio.h>

#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "gsm-call.h"

static gn_call calltable[GN_CALL_MAX_PARALLEL];


static gn_call *search_call(GSM_Statemachine *state, int call_id)
{
	int i;

	for (i = 0; i < GN_CALL_MAX_PARALLEL; i++)
		if (calltable[i].state == state && calltable[i].CallID == call_id)
			return calltable + i;

	return NULL;
}

API void gn_call_notifier(gn_call_status call_status, GSM_CallInfo *call_info, GSM_Statemachine *state)
{
	gn_call *call;

	call = search_call(state, call_info->CallID);

	switch (call_status) {
	case GSM_CS_IncomingCall:
		if (call != NULL) break;
		if ((call = search_call(NULL, 0)) == NULL) {
			dprintf("Call table overflow!\n");
			break;
		}
		call->state = state;
		call->CallID = call_info->CallID;
		call->Status = GN_CALL_Ringing;
		call->Type = call_info->Type;
		strcpy(call->RemoteNumber, call_info->Number);
		strcpy(call->RemoteName, call_info->Name);
		gettimeofday(&call->StartTime, NULL);
		memset(&call->AnswerTime, 0, sizeof(call->AnswerTime));
		call->LocalOriginated = false;
		break;

	case GSM_CS_LocalHangup:
	case GSM_CS_RemoteHangup:
		if (call == NULL) break;
		memset(call, 0, sizeof(*call));
		call->Status = GN_CALL_Idle;
		break;

	case GSM_CS_Established:
		if (call == NULL) {
			if ((call = search_call(NULL, 0)) == NULL) {
				dprintf("Call table overflow!\n");
				break;
			}
			call->state = state;
			call->CallID = call_info->CallID;
			call->Type = call_info->Type;
			strcpy(call->RemoteNumber, call_info->Number);
			strcpy(call->RemoteName, call_info->Name);
			gettimeofday(&call->StartTime, NULL);
			memcpy(&call->AnswerTime, &call->StartTime, sizeof(call->AnswerTime));
			call->LocalOriginated = false;
		} else
			gettimeofday(&call->AnswerTime, NULL);
		call->Status = GN_CALL_Established;
		break;

	case GSM_CS_CallResumed:
		if (call == NULL) break;
		call->Status = GN_CALL_Established;
		break;

	case GSM_CS_CallHeld:
		if (call == NULL) break;
		call->Status = GN_CALL_Held;
		break;

	default:
		dprintf("Invalid call notification code: %d\n", call_status);
		break;
	}
}

API gn_error gn_call_dial(int *call_id, GSM_Data *data, GSM_Statemachine *state)
{
	gn_call *call;
	gn_error err;

	*call_id = -1;
	if ((call = search_call(NULL, 0)) == NULL) {
		dprintf("Call table overflow!\n");
		return GN_ERR_INTERNALERROR;
	}

	if ((err = SM_Functions(GOP_MakeCall, data, state)) != GN_ERR_NONE)
		return err;

	call->state = state;
	call->CallID = data->CallInfo->CallID;
	call->Status = GN_CALL_Dialing;
	call->Type = data->CallInfo->Type;
	strcpy(call->RemoteNumber, data->CallInfo->Number);
	strcpy(call->RemoteName, data->CallInfo->Name);
	gettimeofday(&call->StartTime, NULL);
	memset(&call->AnswerTime, 0, sizeof(call->AnswerTime));
	call->LocalOriginated = true;

	*call_id = call - calltable;

	return GN_ERR_NONE;
}

API gn_error gn_call_answer(int call_id)
{
	GSM_Data data;
	GSM_CallInfo call_info;

	if (calltable[call_id].Status == GN_CALL_Idle) return GN_ERR_NONE;

	memset(&call_info, 0, sizeof(call_info));
	call_info.CallID = calltable[call_id].CallID;
	GSM_DataClear(&data);
	data.CallInfo = &call_info;

	return SM_Functions(GOP_AnswerCall, &data, calltable[call_id].state);
}

API gn_error gn_call_cancel(int call_id)
{
	GSM_Data data;
	GSM_CallInfo call_info;

	if (calltable[call_id].Status == GN_CALL_Idle) return GN_ERR_NONE;

	memset(&call_info, 0, sizeof(call_info));
	call_info.CallID = calltable[call_id].CallID;
	GSM_DataClear(&data);
	data.CallInfo = &call_info;

	return SM_Functions(GOP_AnswerCall, &data, calltable[call_id].state);
}

API gn_call *gn_call_get_active(int call_id)
{
	if (calltable[call_id].Status == GN_CALL_Idle) return NULL;

	return calltable + call_id;
}
