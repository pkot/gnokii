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

#ifndef _gnokii_gsm_call_h
#define _gnokii_gsm_call_h

#include "gsm-common.h"
#include "gsm-data.h"

typedef enum {
	GN_CALL_Idle,
	GN_CALL_Ringing,
	GN_CALL_Dialing,
	GN_CALL_Established,
	GN_CALL_Held
} gn_call_status;

typedef struct {
	GSM_Statemachine *state;
	int CallID;
	gn_call_status Status;
	GSM_CallType Type;
	char RemoteNumber[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1];
	char RemoteName[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1];
	struct timeval StartTime;
	struct timeval AnswerTime;
	bool LocalOriginated;
} gn_call;

#define	GN_CALL_MAX_PARALLEL 2

API void gn_call_notifier(gn_call_status call_status, GSM_CallInfo *call_info, GSM_Statemachine *state);
API gn_error gn_call_dial(int *call_id, GSM_Data *data, GSM_Statemachine *state);
API gn_error gn_call_answer(int call_id);
API gn_error gn_call_cancel(int call_id);
API gn_call *gn_call_get_active(int call_id);

#endif /* _gnokii_gsm_call_h */
