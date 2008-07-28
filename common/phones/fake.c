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

  Copyright (C) 2002 Pavel Machek, Pawel Kot

  This file provides functions for some functionality testing (eg. SMS)

*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "gnokii-internal.h"

/* Some globals */

static gn_error fake_functions(gn_operation op, gn_data *data, struct gn_statemachine *state);

gn_driver driver_fake = {
	NULL,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"fake",      /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GN_RF_Percentage,      /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GN_BU_Percentage,      /* Battery level units */
		GN_DT_DateTime,        /* Have date/time support */
		GN_DT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		60, 96,                /* Startup logo size - 7110 is fixed at init */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	fake_functions,
	NULL
};


/* Initialise is the only function allowed to 'use' state */
static gn_error fake_initialise(struct gn_statemachine *state)
{
	gn_data data;
	char model[GN_MODEL_MAX_LENGTH+1];

	dprintf("Initializing...\n");

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_fake, sizeof(gn_driver));

	dprintf("Connecting\n");

	/* Now test the link and get the model */
	gn_data_clear(&data);
	data.model = model;

	return GN_ERR_NONE;
}

static gn_error at_sms_write(gn_data *data, struct gn_statemachine *state, char* cmd)
{
	unsigned char req[10240], req2[5120];
	int length, tmp, offset = 0;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	/* Do not fill message center so we don't have to emulate that */
	req2[offset] = 0;
	offset += 0;

	req2[offset + 1] = 0x01 | 0x10; /* Validity period in relative format */
	if (data->raw_sms->reject_duplicates) req2[offset + 1] |= 0x04;
	if (data->raw_sms->report) req2[offset + 1] |= 0x20;
	if (data->raw_sms->udh_indicator) req2[offset + 1] |= 0x40;
	if (data->raw_sms->reply_via_same_smsc) req2[offset + 1] |= 0x80;
	req2[offset + 2] = 0x00; /* Message Reference */

	tmp = data->raw_sms->remote_number[0];
	if (tmp % 2) tmp++;
	tmp /= 2;
	memcpy(req2 + offset + 3, data->raw_sms->remote_number, tmp + 2);
	offset += tmp + 1;

	req2[offset + 4] = data->raw_sms->pid;
	req2[offset + 5] = data->raw_sms->dcs;
	req2[offset + 6] = 0x00; /* Validity period */
	req2[offset + 7] = data->raw_sms->length;
	memcpy(req2 + offset + 8, data->raw_sms->user_data, data->raw_sms->user_data_length);

	length = data->raw_sms->user_data_length + offset + 8;

	/* Length in AT mode is the length of the full message minus SMSC field length */
	fprintf(stdout, "AT+%s=%d\n", cmd, length - 1);

	bin2hex(req, req2, length);
	req[length * 2] = 0x1a;
	req[length * 2 + 1] = 0;
	fprintf(stdout, "%s\n", req);
	return GN_ERR_NONE;
}

static gn_error fake_functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	switch (op) {
	case GN_OP_Init:
		return fake_initialise(state);
	case GN_OP_Terminate:
		return GN_ERR_NONE;
	case GN_OP_SendSMS:
		return at_sms_write(data, state, "???");
	case GN_OP_GetSMSCenter:
		return GN_ERR_NONE;
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_INTERNALERROR;
}
