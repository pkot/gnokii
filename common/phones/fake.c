/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2001 Markus Plail, Pawe³ Kot
  Copyright (C) 2002 Pavel Machek

  Released under the terms of the GNU GPL, see file COPYING for more details.

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

static const SMSMessage_Layout at_deliver = {
	true,						/* Is the SMS type supported */
	 1, true, false,				/* SMSC */
	 2,  2, -1,  2, -1, -1,  4, -1, 13,  5,  2,
	 -1, -1, -1,					/* Validity */
	 3, true, false,				/* Remote Number */
	 6, -1,						/* Time */
	-1, -1,						/* Nonstandard fields */
	14, true					/* User Data */
};

static const SMSMessage_Layout at_submit = {
	true,						/* Is the SMS type supported */
	-1, true, false,				/* SMSC */
	-1,  1,  1,  1, -1,  2,  4, -1,  7,  5,  1,
	 6, -1, -1,					/* Validity */
	 3, true, false,				/* Remote Number */
	-1, -1,						/* Time */
	-1, -1,						/* Nonstandard fields */
	 8, true					/* User Data */
};

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
	GSM_Error err;
	int try = 0, connected = 0;

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
	unsigned char req[10240];
	int length, i;

	if (!data->RawData) return GE_INTERNALERROR;

	length = data->RawData->Length;

	if (length < 0) return GE_SMSWRONGFORMAT;
	fprintf(stdout, "AT+%s=%d\n", cmd, length - data->RawData->Data[0] - 1);

	bin2hex(req, data->RawData->Data, data->RawData->Length);
	req[data->RawData->Length * 2] = 0x1a;
	req[data->RawData->Length * 2 + 1] = 0;
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
