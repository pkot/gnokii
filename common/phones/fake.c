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

  Copyright (C) 2002 Pavel Machek

  This file provides functions for some functionality testing (eg. SMS)

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

/* Some globals */

static GSM_Error Pfake_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);

GSM_Phone phone_fake = {
	NULL,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"fake",      /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Percentage,        /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Percentage,        /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		60, 96,                /* Startup logo size - 7110 is fixed at init */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	Pfake_Functions
};


/* Initialise is the only function allowed to 'use' state */
static GSM_Error Pfake_Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	char model[10];

	dprintf("Initializing...\n");

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_fake, sizeof(GSM_Phone));

	dprintf("Connecting\n");

	/* Now test the link and get the model */
	GSM_DataClear(&data);
	data.Model = model;

	return GE_NONE;
}

static GSM_Error AT_WriteSMS(GSM_Data *data, GSM_Statemachine *state, char* cmd)
{
	unsigned char req[10240], req2[5120];
	int length, tmp, offset = 0;

	if (!data->RawSMS) return GE_INTERNALERROR;

	/* Prepare the message and count the size */
	req[offset] = 0x00; /* Message Center */

	req2[offset + 1] = 0x01 | 0x10; /* Validity period in relative format */
	if (data->RawSMS->RejectDuplicates) req2[offset + 1] |= 0x04;
	if (data->RawSMS->Report) req2[offset + 1] |= 0x20;
	if (data->RawSMS->UDHIndicator) req2[offset + 1] |= 0x40;
	if (data->RawSMS->ReplyViaSameSMSC) req2[offset + 1] |= 0x80;
	req2[offset + 2] = 0x00; /* Message Reference */

	tmp = data->RawSMS->RemoteNumber[0];
	if (tmp % 2) tmp++;
	tmp /= 2;
	memcpy(req2 + offset + 3, data->RawSMS->RemoteNumber, tmp + 2);
	offset += tmp + 1;

	req2[offset + 4] = data->RawSMS->PID;
	req2[offset + 5] = data->RawSMS->DCS;
	req2[offset + 6] = 0x00; /* Validity period */
	req2[offset + 7] = data->RawSMS->Length;
	memcpy(req2 + offset + 8, data->RawSMS->UserData, data->RawSMS->UserDataLength);

	length = data->RawSMS->UserDataLength + offset + 8;

	fprintf(stdout, "AT+%s=%d\n", cmd, length - data->RawSMS->MessageCenter[0] - 1);

	bin2hex(req, req2, length);
	req[length * 2] = 0x1a;
	req[length * 2 + 1] = 0;
	fprintf(stdout, "%s\n", req);
	return GE_NONE;
}

static GSM_Error Pfake_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return Pfake_Initialise(state);
	case GOP_Terminate:
		return GE_NONE;
	case GOP_SendSMS:
		return AT_WriteSMS(data, state, "???");
	case GOP_GetSMSCenter:
		return GE_NONE;
	default:
		return GE_NOTIMPLEMENTED;
	}
	return GE_INTERNALERROR;
}
