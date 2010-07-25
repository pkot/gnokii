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

static gn_error fake_identify(gn_data *data, struct gn_statemachine *state)
{
	fprintf(stderr, _("Apparently you didn't configure gnokii. Please do it prior to using it.\n"
			  "You can get some clues from comments included in sample config file or give\n"
			  "a try with gnokii-configure utility included in gnokii distribution.\n"));
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

	/* Generate reference number (use a constant to match output from testsuite) */
	data->raw_sms->reference = 1;
	return GN_ERR_NONE;
}

static char *sms_inbox[] = {
	/* SMS-DELIVER */
	/* From: gnokii */
	/* Text: Configure gnokii prior to using it. You can get some clues from comments included in sample config file or try with gnokii-configure from utils dir. */
	"0791214365870921240BD067F77B9D4E0300009011800000004094C3B7DB9C3ED7E565D0D9FD5EA7D320B83CFD9683E86F507D9E769F4169BA0B947DD741E3B01B742ED341F377BB0C1AB3EBE539C82C7FB741E377BB5D76D3E7A0B47BCCAE93CB6450DA0D9A87DB707619347EBBCDE933C89C6697416F39882ECF83EE693A1A7476BFD7E9746BFC769BD3E7BABC0C32CBDF6D509D9E66CF41E4B4DC05",
	/* Text: Hello! vowels àèìòù euro € */
	"0791214365870921240B919999999999F90000902072129025401BC8329BFD0E81ECEF7B993D07FD0907840154AECBDFA04D19",
	/* Text: € */
	"0791214365870921240B919999999999F9000090907000920480029B32",
	/* SMS-STATUS-REPORT */
	/* Status: delivered */
	"07914366999999F906CD098186666666F89080709002818090807002336080009FD6E494"
};

static gn_error at_sms_get_generic(gn_data *data, struct gn_statemachine *state, char *sms)
{
	gn_error e;
	int len = strlen(sms) / 2;
	char *tmp = calloc(len, 1);

	if (!tmp)
		return GN_ERR_FAILED;

	hex2bin(tmp, sms, len);
	e = gn_sms_pdu2raw(data->raw_sms, tmp, len, GN_SMS_PDU_DEFAULT);
	free(tmp);
	return e;
}

static gn_error at_sms_get_static(gn_data *data, struct gn_statemachine *state, int position)
{
	if (position > sizeof(sms_inbox)/sizeof(char *) || sms_inbox[position - 1] == NULL)
		return GN_ERR_EMPTYLOCATION;

	return at_sms_get_generic(data, state, sms_inbox[position - 1]);
}

static gn_error at_sms_get_status_static(gn_data *data, struct gn_statemachine *state)
{
	int i, count = 0;

	for (i = 0; i < sizeof(sms_inbox)/sizeof(char *); i++)
		if (sms_inbox[i])
			count++;

	return count;
}

static gn_error at_sms_delete_static(gn_data *data, struct gn_statemachine *state, int position)
{
	if (position > sizeof(sms_inbox)/sizeof(char *) || !sms_inbox[position - 1])
		return GN_ERR_EMPTYLOCATION;

	sms_inbox[position - 1] = NULL;

	return GN_ERR_NONE;
}

#ifndef WIN32
#define MAX_PATH_LEN 256
static gn_error at_sms_get_from_file(gn_data *data, struct gn_statemachine *state, int position, DIR *d, const char *dirpath)
{
	gn_error e = GN_ERR_EMPTYLOCATION;
	int i, n, offset, size = 256;
	struct dirent *dent;
	FILE *f;
	char *sms_text;
	struct stat buf;
	char path[MAX_PATH_LEN];

	/* iterate to Nth position */
	for (i = 0; i < position; i++) {
		dent = readdir(d);
		if (dent) {
			snprintf(path, MAX_PATH_LEN, "%s/%s", dirpath, dent->d_name);
			n = stat(path, &buf);
			if (!S_ISREG(buf.st_mode))
				i--;
		} else
			goto out;
	}
	f = fopen(path, "r");
	if (!f) {
		e = GN_ERR_INTERNALERROR;
		goto out;
	}
	sms_text = calloc(size, sizeof(char));
	n = size - 1;
	offset = 0;
	while (fgets(sms_text + offset, size, f)) {
		offset += size - 1;
		sms_text = realloc(sms_text, offset + size);
	}
	e = at_sms_get_generic(data, state, sms_text);
	free(sms_text);
	fclose(f);
out:
	closedir(d);
	return e;
}

static gn_error at_sms_delete_from_file(gn_data *data, struct gn_statemachine *state, int position, DIR *d, const char *dirpath)
{
	gn_error e = GN_ERR_NONE;
	int i, n;
	struct dirent *dent;
	struct stat buf;
	char path[MAX_PATH_LEN];

	/* iterate to Nth position */
	for (i = 0; i < position; i++) {
		dent = readdir(d);
		if (dent) {
			snprintf(path, MAX_PATH_LEN, "%s/%s", dirpath, dent->d_name);
			n = stat(path, &buf);
			if (!S_ISREG(buf.st_mode))
				i--;
		} else
			goto out;
	}
	if (unlink(path))
		e = GN_ERR_FAILED;
out:
	closedir(d);
	return e;
}
#endif

#ifdef WIN32
static gn_error at_sms_get(gn_data *data, struct gn_statemachine *state)
{
	int position;
	if (!data || !data->raw_sms)
		return GN_ERR_INTERNALERROR;
	position = data->raw_sms->number;
	return at_sms_get_static(data, state, position);
}

static gn_error at_sms_delete(gn_data *data, struct gn_statemachine *state)
{
	int position;
	if (!data || !data->raw_sms)
		return GN_ERR_INTERNALERROR;
	position = data->raw_sms->number;
	return at_sms_delete_static(data, state, position);
}

static gn_error at_sms_get_sms_status(gn_data *data, struct gn_statemachine *state)
{
	if (!data || !data->sms_status)
		return GN_ERR_INTERNALERROR;

	data->sms_status->number = 0;
	data->sms_status->unread = 0;
	data->sms_status->changed = 0;
	data->sms_status->folders_count = 0;

	return GN_ERR_NONE;
}
#else
static gn_error at_sms_get(gn_data *data, struct gn_statemachine *state)
{
	gn_error e = GN_ERR_NONE;
	const char *path;
	DIR *d;
	int position;

	if (!data || !data->raw_sms) {
		e = GN_ERR_INTERNALERROR;
		goto out;
	}
	position = data->raw_sms->number;
	if (position < 1) {
		e = GN_ERR_INVALIDLOCATION;
		goto out;
	}

	path = gn_lib_cfg_get("fake_driver", "sms_inbox");
	if (!path || (d = opendir(path)) == NULL)
		e = at_sms_get_static(data, state, position);
	else
		e = at_sms_get_from_file(data, state, position, d, path);

out:
	return e;
}

static int count_files(DIR *d, const char *dirpath)
{
	int i = 0;
	struct dirent *dent;
	struct stat buf;
	char path[MAX_PATH_LEN];

	while ((dent = readdir(d)) != NULL) {
		snprintf(path, MAX_PATH_LEN, "%s/%s", dirpath, dent->d_name);
		if (!stat(path, &buf) && S_ISREG(buf.st_mode))
			i++;
	}
	return i;
}

static gn_error at_sms_get_sms_status(gn_data *data, struct gn_statemachine *state)
{
	DIR *d;
	const char *path;

	if (!data || !data->sms_status)
		return GN_ERR_INTERNALERROR;

	data->sms_status->number = 0;
	data->sms_status->unread = 0;
	data->sms_status->changed = 0;
	data->sms_status->folders_count = 0;

	path = gn_lib_cfg_get("fake_driver", "sms_inbox");
	if (!path || (d = opendir(path)) == NULL) {
		data->sms_status->number = data->sms_status->unread = at_sms_get_status_static(data, state);
	} else {
		data->sms_status->number = data->sms_status->unread = count_files(d, path);
		closedir(d);
	}
	
	return GN_ERR_NONE;
}

static gn_error at_sms_delete(gn_data *data, struct gn_statemachine *state)
{
	gn_error e = GN_ERR_NONE;
	const char *path;
	DIR *d;
	int position;

	if (!data || !data->raw_sms) {
		e = GN_ERR_INTERNALERROR;
		goto out;
	}
	position = data->raw_sms->number;
	if (position < 1) {
		e = GN_ERR_INVALIDLOCATION;
		goto out;
	}

	path = gn_lib_cfg_get("fake_driver", "sms_inbox");
	if (!path || (d = opendir(path)) == NULL)
		e = at_sms_delete_static(data, state, position);
	else
		e = at_sms_delete_from_file(data, state, position, d, path);

out:
	return e;
}
#endif

static gn_error at_get_model(gn_data *data, struct gn_statemachine *state)
{
	if (!data)
		return GN_ERR_INTERNALERROR;

	snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", "fake");

	return GN_ERR_NONE;
}

static gn_error fake_functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	switch (op) {
	case GN_OP_Init:
		return fake_initialise(state);
	case GN_OP_Identify:
		return fake_identify(data, state);
	case GN_OP_Terminate:
		return GN_ERR_NONE;
	case GN_OP_SaveSMS:
		return at_sms_write(data, state, "CMGW");
	case GN_OP_SendSMS:
		return at_sms_write(data, state, "CMGS");
	case GN_OP_GetSMS:
		return at_sms_get(data, state);
	case GN_OP_DeleteSMS:
		return at_sms_delete(data, state);
	case GN_OP_GetSMSCenter:
		return GN_ERR_NONE;
	case GN_OP_GetModel:
		return at_get_model(data, state);
	case GN_OP_GetSMSStatus:
		return at_sms_get_sms_status(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_INTERNALERROR;
}
