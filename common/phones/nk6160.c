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

  Copyright (C) 2002 BORBELY Zoltan

  This file provides functions specific to the 6160 series.
  See README for more details on supported mobile phones.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "links/m2bus.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

#include "gnokii-internal.h"
#include "gsm-api.h"

/* Some globals */

/* FIXME: we should get rid on it */
static struct gn_statemachine *state = NULL;
static int logoslice;

/* static functions prototypes */
static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error initialise(struct gn_statemachine *state);
static gn_error identify(gn_data *data, struct gn_statemachine *state);
static gn_error phonebook_read(gn_data *data, struct gn_statemachine *state);
static gn_error phonebook_write(gn_data *data, struct gn_statemachine *state);
static gn_error bitmap_get(gn_data *data, struct gn_statemachine *state);

static gn_error phonebook_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error phone_info_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error bitmap_startup_logo_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

static gn_incoming_function_type incoming_functions[] = {
	{ 0xd2, phone_info_incoming },
	{ 0x40, phonebook_incoming },
	{ 0xdd, bitmap_startup_logo_incoming },
	{ 0, NULL}
};

gn_driver nokia_6160 = {
	incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"6160|5120",           /* Supported models */
		4,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GN_RF_Arbitrary,       /* RF level units */
		4,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GN_BU_Arbitrary,       /* Battery level units */
		GN_DT_DateTime,        /* Have date/time support */
		GN_DT_TimeOnly,	     /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		48, 84,                /* Startup logo size */
		14, 72,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	functions,
	NULL
};

static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	switch (op) {
	case GN_OP_Init:
		return initialise(state);
	case GN_OP_Terminate:
		return pgen_terminate(data, state);
/*	case GN_OP_GetImei: */
	case GN_OP_GetModel:
	case GN_OP_GetRevision:
	case GN_OP_GetManufacturer:
	case GN_OP_Identify:
		return identify(data, state);
	case GN_OP_ReadPhonebook:
		return phonebook_read(data, state);
	case GN_OP_WritePhonebook:
		return phonebook_write(data, state);
	case GN_OP_GetBitmap:
		return bitmap_get(data, state);
	default:
		dprintf("NK6160 unimplemented operation: %d\n", op);
		return GN_ERR_NOTIMPLEMENTED;
	}
}

/* Initialise is the only function allowed to 'use' state */
static gn_error initialise(struct gn_statemachine *lstate)
{
	gn_error err;
	char model[GN_MODEL_MAX_LENGTH + 1];
	gn_data data;
	gn_phone_model *pm;

	/* Copy in the phone info */
	memcpy(&(lstate->driver), &nokia_6160, sizeof(gn_driver));

	switch (lstate->link.connection_type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		err = m2bus_initialise(&(lstate->link), lstate);
		break;
	default:
		return GN_ERR_NOTSUPPORTED;
	}

	if (err != GN_ERR_NONE) {
		dprintf("Error in link initialisation\n");
		return GN_ERR_NOTSUPPORTED;
	}

	sm_initialise(lstate);

	/* We need to identify the phone first in order to know whether we can
	   authorize or set keytable */
	memset(model, 0, sizeof(model));
	gn_data_clear(&data);
	data.model = model;

	if ((err = identify(&data, lstate)) != GN_ERR_NONE) return err;
	dprintf("model: '%s'\n", model);
	if ((pm = gn_get_phone_model(model)) == NULL) {
		dump(_("Unsupported phone model \"%s\"\n"), model);
		dump(_("Please read Docs/Reporting-HOWTO and send a bug report!\n"));
		return GN_ERR_INTERNALERROR;
	}

	state = lstate;

	return GN_ERR_NONE;
}


static gn_error phonebook_read(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x1f, 0x01, 0x04, 0x86, 0x00};

	dprintf("Reading phonebook location (%d)\n", data->phonebook_entry->location);

	req[6] = data->phonebook_entry->location;
	if (sm_message_send(state, 7, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return sm_block(state, data, 0x40);
}

static gn_error phonebook_write(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {0x00, 0x01, 0x1f, 0x01, 0x04, 0x87, 0x00};
	gn_phonebook_entry *pe;
	unsigned char *pos;
	int namelen, numlen;

	pe = data->phonebook_entry;
	pos = req + 6;
	namelen = strlen(pe->name);
	numlen = strlen(pe->number);
	dprintf("Writing phonebook location (%d): %s\n", pe->location, pe->name);
	if (namelen > GN_PHONEBOOK_NAME_MAX_LENGTH) {
		dprintf("name too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	if (numlen > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	if (pe->subentries_count > 1) {
		dprintf("6160 doesn't support subentries\n");
		return GN_ERR_UNKNOWN;
	}
	if ((pe->subentries_count == 1) && ((pe->subentries[0].entry_type != GN_PHONEBOOK_ENTRY_Number)
		|| (pe->subentries[0].number_type != GN_PHONEBOOK_NUMBER_General) || (pe->subentries[0].id != 2)
		|| strcmp(pe->subentries[0].data.number, pe->number))) {
		dprintf("6160 doesn't support subentries\n");
		return GN_ERR_UNKNOWN;
	}

	*pos++ = pe->location;
	strcpy(pos, pe->number);
	pos += numlen + 1;
	strcpy(pos, pe->name);
	pos += namelen + 1;

	if (sm_message_send(state, pos-req, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return sm_block(state, data, 0x40);
}

static gn_error phonebook_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_phonebook_entry *pe;
	unsigned char *pos;
	unsigned char respheader[] = {0x01, 0x00, 0xc9, 0x04, 0x01};

	if (memcmp(message, respheader, sizeof(respheader)))
		return GN_ERR_UNHANDLEDFRAME;

	switch (message[5]) {
	/* read phonebook reply */
	case 0x86:
		if (!data->phonebook_entry) break;

		pe = data->phonebook_entry;
		pos = message + 8;

		switch (message[7]) {
		case 0x01: break;
		case 0x05: return GN_ERR_INVALIDLOCATION;
		default: return GN_ERR_UNHANDLEDFRAME;
		}
		snprintf(pe->number, sizeof(pe->number), "%s", pos);
		pos += strlen(pos) + 1;
		snprintf(pe->name, sizeof(pe->name), "%s", pos);
		memset(&pe->date, 0, sizeof(pe->date));
		pe->subentries_count = 0;
		pe->caller_group = 5;
		pe->empty = (*pe->name != 0);
		break;

	/* write phonebook reply */
	case 0x87:
		switch (message[7]) {
		case 0x01: break;
		case 0x05: return GN_ERR_INVALIDLOCATION;
		default: return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}

static gn_error phone_info(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x00, 0x03, 0x00};
	gn_error error;

	dprintf("Getting phone info...\n");
	if (sm_message_send(state, 5, 0xd1, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	error = sm_block(state, data, 0xd2);
	return error;
}

static gn_error identify(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	dprintf("Identifying...\n");
	if (data->manufacturer) pnok_manufacturer_get(data->manufacturer);

	if ((error = phone_info(data, state)) != GN_ERR_NONE) return error;

	/* Check that we are back at state Initialised */
	if (gn_sm_loop(state, 0) != GN_SM_Initialised) return GN_ERR_UNKNOWN;
	return GN_ERR_NONE;
}

static gn_error phone_info_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char *pos;

	if (data->model) {
		snprintf(data->model, 6, "%s", message + 21);
	}
	if (data->revision) {
		snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW: %s", message + 6);
		if ((pos = strchr(data->revision, '\n')) != NULL) *pos = 0;
	}
	dprintf("Phone info:\n%s\n", message + 4);
	return GN_ERR_NONE;
}


static gn_error get_bitmap_startup_slice(gn_data *data, struct gn_statemachine *state, int s)
{
	unsigned char req[] = {0x00, 0x01, 0x07, 0x07, 0x08, 0x00};

	logoslice = s;
	req[5] = (unsigned char)s + 1;

	if (sm_message_send(state, 6, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return sm_block(state, data, 0xdd);
}

static gn_error bitmap_get(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	int i;

	if (!data->bitmap) return GN_ERR_INTERNALERROR;

	switch (data->bitmap->type) {
	case GN_BMP_StartupLogo:
		data->bitmap->height = nokia_6160.phone.startup_logo_height;
		data->bitmap->width = nokia_6160.phone.startup_logo_width;
		data->bitmap->size = ceiling_to_octet(data->bitmap->height * data->bitmap->width);
		gn_bmp_clear(data->bitmap);
		for (i = 0; i < 6; i++)
			if ((error = get_bitmap_startup_slice(data, state, i)) != GN_ERR_NONE)
				return error;
		break;
	case GN_BMP_WelcomeNoteText:
	case GN_BMP_DealerNoteText:
	case GN_BMP_OperatorLogo:
	case GN_BMP_CallerLogo:
	case GN_BMP_None:
	case GN_BMP_PictureMessage:
		return GN_ERR_NOTSUPPORTED;

	default:
		return GN_ERR_INTERNALERROR;
	}

	return GN_ERR_NONE;
}

static gn_error bitmap_startup_logo_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int x, i;
	unsigned char b;

	if (message[0] != 0x01 || message[1] != 0x00 || message[2] != 0x07 || message[3] != 0x08)
		return GN_ERR_UNHANDLEDFRAME;

	if (!data->bitmap || data->bitmap->type != GN_BMP_StartupLogo)
		return GN_ERR_INTERNALERROR;

	for (x = 0; x < 84; x++)
		for (b = message[5 + x], i = 0; b != 0; b >>= 1, i++)
			if (b & 0x01)
				gn_bmp_set_point(data->bitmap, x, 8 * logoslice + i);

	return GN_ERR_NONE;
}


#if 0
static int get_memory_type(GSM_MemoryType memory_type)
{
	int result;

	switch (memory_type) {
	case GN_MT_MT:
		result = NK6100_MEMORY_MT;
		break;
	case GN_MT_ME:
		result = NK6100_MEMORY_ME;
		break;
	case GN_MT_SM:
		result = NK6100_MEMORY_SM;
		break;
	case GN_MT_FD:
		result = NK6100_MEMORY_FD;
		break;
	case GN_MT_ON:
		result = NK6100_MEMORY_ON;
		break;
	case GN_MT_EN:
		result = NK6100_MEMORY_EN;
		break;
	case GN_MT_DC:
		result = NK6100_MEMORY_DC;
		break;
	case GN_MT_RC:
		result = NK6100_MEMORY_RC;
		break;
	case GN_MT_MC:
		result = NK6100_MEMORY_MC;
		break;
	default:
		result = NK6100_MEMORY_XX;
		break;
	}
	return (result);
}
#endif
