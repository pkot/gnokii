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

  Include file for the call management library.

*/

#ifndef __gnokii_call_h_
#define __gnokii_call_h_

#include "gsm-common.h"
#include "gsm-data.h"

typedef enum {
	GN_API_CS_Idle,
	GN_API_CS_Ringing,
	GN_API_CS_Dialing,
	GN_API_CS_Established,
	GN_API_CS_Held
} GN_API_CallStatus;

typedef struct {
	GSM_Statemachine *state;
	int CallID;
	GN_API_CallStatus Status;
	GSM_CallType Type;
	char RemoteNumber[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1];
	char RemoteName[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1];
	struct timeval StartTime;
	struct timeval AnswerTime;
	bool LocalOriginated;
} GN_API_Call;

#define	GN_MAX_PARALLEL_CALL 2

API void GN_CallNotifier(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo, GSM_Statemachine *state);
API GSM_Error GN_CallDial(int *CallId, GSM_Data *data, GSM_Statemachine *state);
API GSM_Error GN_CallAnswer(int CallId);
API GSM_Error GN_CallCancel(int CallId);
API GN_API_Call *GN_CallGetActive(int CallId);

#endif
