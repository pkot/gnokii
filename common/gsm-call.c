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

static GN_API_Call calltable[GN_MAX_PARALLEL_CALL];


static GN_API_Call *search_call(GSM_Statemachine *state, int CallID)
{
	int i;

	for (i = 0; i < GN_MAX_PARALLEL_CALL; i++)
		if (calltable[i].state == state && calltable[i].CallID == CallID)
			return calltable + i;

	return NULL;
}

API void GN_CallNotifier(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo, GSM_Statemachine *state)
{
	GN_API_Call *call;

	call = search_call(state, CallInfo->CallID);

	switch (CallStatus) {
	case GSM_CS_IncomingCall:
		if (call != NULL) break;
		if ((call = search_call(NULL, 0)) == NULL) {
			dprintf("Call table overflow!\n");
			break;
		}
		call->state = state;
		call->CallID = CallInfo->CallID;
		call->Status = GN_API_CS_Ringing;
		call->Type = CallInfo->Type;
		strcpy(call->RemoteNumber, CallInfo->Number);
		strcpy(call->RemoteName, CallInfo->Name);
		gettimeofday(&call->StartTime, NULL);
		memset(&call->AnswerTime, 0, sizeof(call->AnswerTime));
		call->LocalOriginated = false;
		break;

	case GSM_CS_LocalHangup:
	case GSM_CS_RemoteHangup:
		if (call == NULL) break;
		memset(call, 0, sizeof(*call));
		call->Status = GN_API_CS_Idle;
		break;

	case GSM_CS_Established:
		if (call == NULL) {
			if ((call = search_call(NULL, 0)) == NULL) {
				dprintf("Call table overflow!\n");
				break;
			}
			call->state = state;
			call->CallID = CallInfo->CallID;
			call->Type = CallInfo->Type;
			strcpy(call->RemoteNumber, CallInfo->Number);
			strcpy(call->RemoteName, CallInfo->Name);
			gettimeofday(&call->StartTime, NULL);
			memcpy(&call->AnswerTime, &call->StartTime, sizeof(call->AnswerTime));
			call->LocalOriginated = false;
		} else
			gettimeofday(&call->AnswerTime, NULL);
		call->Status = GN_API_CS_Established;
		break;

	case GSM_CS_CallResumed:
		if (call == NULL) break;
		call->Status = GN_API_CS_Established;
		break;

	case GSM_CS_CallHeld:
		if (call == NULL) break;
		call->Status = GN_API_CS_Held;
		break;

	default:
		dprintf("Invalid call notification code: %d\n", CallStatus);
		break;
	}
}

API GSM_Error GN_CallDial(int *CallId, GSM_Data *data, GSM_Statemachine *state)
{
	GN_API_Call *call;
	GSM_Error err;

	*CallId = -1;
	if ((call = search_call(NULL, 0)) == NULL) {
		dprintf("Call table overflow!\n");
		return GE_INTERNALERROR;
	}

	if ((err = SM_Functions(GOP_MakeCall, data, state)) != GE_NONE)
		return err;

	call->state = state;
	call->CallID = data->CallInfo->CallID;
	call->Status = GN_API_CS_Dialing;
	call->Type = data->CallInfo->Type;
	strcpy(call->RemoteNumber, data->CallInfo->Number);
	strcpy(call->RemoteName, data->CallInfo->Name);
	gettimeofday(&call->StartTime, NULL);
	memset(&call->AnswerTime, 0, sizeof(call->AnswerTime));
	call->LocalOriginated = true;

	*CallId = call - calltable;

	return GE_NONE;
}

API GSM_Error GN_CallAnswer(int CallId)
{
	GSM_Data data;
	GSM_CallInfo CallInfo;

	if (calltable[CallId].Status == GN_API_CS_Idle) return GE_NONE;

	memset(&CallInfo, 0, sizeof(CallInfo));
	CallInfo.CallID = calltable[CallId].CallID;
	GSM_DataClear(&data);
	data.CallInfo = &CallInfo;

	return SM_Functions(GOP_AnswerCall, &data, calltable[CallId].state);
}

API GSM_Error GN_CallCancel(int CallId)
{
	GSM_Data data;
	GSM_CallInfo CallInfo;

	if (calltable[CallId].Status == GN_API_CS_Idle) return GE_NONE;

	memset(&CallInfo, 0, sizeof(CallInfo));
	CallInfo.CallID = calltable[CallId].CallID;
	GSM_DataClear(&data);
	data.CallInfo = &CallInfo;

	return SM_Functions(GOP_AnswerCall, &data, calltable[CallId].state);
}

API GN_API_Call *GN_CallGetActive(int CallId)
{
	if (calltable[CallId].Status == GN_API_CS_Idle) return NULL;

	return calltable + CallId;
}
