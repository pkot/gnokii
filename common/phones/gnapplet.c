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

  Copyright (C) 2004 BORBELY Zoltan

  This file provides functions specific to the gnapplet series.
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
#include "phones/generic.h"
#include "phones/gnapplet.h"
#include "links/gnbus.h"

#include "pkt.h"
#include "gnokii-internal.h"
#include "gnokii.h"

#define	DRVINSTANCE(s) ((gnapplet_driver_instance *)((s)->driver.driver_instance))
#define	FREE(p) do { free(p); (p) = NULL; } while (0)

#define	REQUEST_DEFN(n) \
	unsigned char req[n]; \
	pkt_buffer pkt; \
	pkt_buffer_set(&pkt, req, sizeof(req))

#define	REQUEST_DEF	REQUEST_DEFN(32)

#define	REPLY_DEF \
	pkt_buffer pkt; \
	uint16_t code; \
	gn_error error; \
	pkt_buffer_set(&pkt, message, length); \
	code = pkt_get_uint16(&pkt); \
	error = pkt_get_uint16(&pkt)

#define	SEND_MESSAGE_BLOCK(type) \
	do { \
		if (sm_message_send(pkt.offs, type, pkt.addr, state)) return GN_ERR_NOTREADY; \
		return sm_block(type, data, state); \
	} while (0)

/* static functions prototypes */
static gn_error gnapplet_functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_initialise(struct gn_statemachine *state);
static gn_error gnapplet_identify(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_read_phonebook(gn_data *data, struct gn_statemachine *state);

static gn_error gnapplet_incoming_info(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_phonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

static gn_incoming_function_type gnapplet_incoming_functions[] = {
	{ GNAPPLET_MSG_INFO,		gnapplet_incoming_info },
	{ GNAPPLET_MSG_PHONEBOOK,	gnapplet_incoming_phonebook },
	{ 0,				NULL}
};

gn_driver driver_gnapplet = {
	gnapplet_incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"gnapplet|series60",	/* Supported models */
		7,			/* Max RF Level */
		0,			/* Min RF Level */
		GN_RF_Percentage,	/* RF level units */
		7,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GN_BU_Percentage,	/* Battery level units */
		GN_DT_DateTime,		/* Have date/time support */
		GN_DT_TimeOnly,		/* Alarm supports time only */
		1,			/* Alarms available - FIXME */
		48, 84,			/* Startup logo size */
		14, 72,			/* Op logo size */
		14, 72			/* Caller logo size */
	},
	gnapplet_functions,
	NULL
};


static gn_error gnapplet_functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	if (!DRVINSTANCE(state) && op != GN_OP_Init) return GN_ERR_INTERNALERROR;

	switch (op) {
	case GN_OP_Init:
		if (DRVINSTANCE(state)) return GN_ERR_INTERNALERROR;
		return gnapplet_initialise(state);
	case GN_OP_Terminate:
		FREE(DRVINSTANCE(state));
		return pgen_terminate(data, state);
	case GN_OP_GetImei:
	case GN_OP_GetModel:
	case GN_OP_GetRevision:
	case GN_OP_GetManufacturer:
	case GN_OP_Identify:
		return gnapplet_identify(data, state);
	case GN_OP_ReadPhonebook:
		return gnapplet_read_phonebook(data, state);
	default:
		dprintf("gnapplet unimplemented operation: %d\n", op);
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static gn_error gnapplet_initialise(struct gn_statemachine *state)
{
	gn_error err;
	int i;

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_gnapplet, sizeof(gn_driver));

	if (!(DRVINSTANCE(state) = calloc(1, sizeof(gnapplet_driver_instance))))
		return GN_ERR_MEMORYFULL;

	switch (state->config.connection_type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
	case GN_CT_TCP:
	case GN_CT_Irda:
	case GN_CT_Bluetooth:
		err = gnbus_initialise(state);
		break;
	default:
		FREE(DRVINSTANCE(state));
		return GN_ERR_NOTSUPPORTED;
	}

	if (err != GN_ERR_NONE) {
		dprintf("Error in link initialisation\n");
		FREE(DRVINSTANCE(state));
		return GN_ERR_NOTSUPPORTED;
	}

	sm_initialise(state);

	return GN_ERR_NONE;
}


static gn_error gnapplet_identify(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	/*
	if (data->manufacturer) pnok_manufacturer_get(data->manufacturer);
	if (data->model) strcpy(data->model, drvinst->model);
	if (data->imei) strcpy(data->imei, drvinst->imei);
	if (data->revision) snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW %s", drvinst->sw_version, drvinst->hw_version);
	data->phone = drvinst->pm;
	*/

	pkt_put_uint16(&pkt, GNAPPLET_MSG_INFO_ID_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_INFO);
}


static gn_error gnapplet_incoming_info(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	char sw_version[16], hw_version[16];
	int proto_major, proto_minor;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_INFO_ID_RESP:
		if (error != GN_ERR_NONE) return error;
		proto_major = pkt_get_uint16(&pkt);
		proto_minor = pkt_get_uint16(&pkt);
		/* Compatibility isn't important early in the development */
		/* if (proto_major != GNAPPLET_MAJOR_VERSION) { */
		if (proto_major != GNAPPLET_MAJOR_VERSION || proto_minor != GNAPPLET_MINOR_VERSION) {
			dprintf("gnapplet version: %d.%d, gnokii driver: %d.%d\n",
				proto_major, proto_minor,
				GNAPPLET_MAJOR_VERSION, GNAPPLET_MINOR_VERSION);
			return GN_ERR_INTERNALERROR;
		}
		if (data->manufacturer) pkt_get_string(data->manufacturer, 20, &pkt);
		if (data->model) pkt_get_string(data->model, GN_MODEL_MAX_LENGTH, &pkt);
		if (data->imei) pkt_get_string(data->imei, GN_IMEI_MAX_LENGTH, &pkt);
		if (data->revision) {
			pkt_get_string(sw_version, sizeof(sw_version), &pkt);
			pkt_get_string(hw_version, sizeof(hw_version), &pkt);
			snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW %s", sw_version, hw_version);
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_read_phonebook(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PHONEBOOK_READ_REQ);
	pkt_put_uint16(&pkt, data->phonebook_entry->memory_type);
	pkt_put_uint16(&pkt, data->phonebook_entry->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_incoming_phonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i, n;
	char name[64], val[64];
	gn_phonebook_entry *entry;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_PHONEBOOK_READ_RESP:
		if (!(entry = data->phonebook_entry)) return GN_ERR_INTERNALERROR;
		entry->empty = true;
		entry->caller_group = 5;
		entry->name[0] = '\0';
		entry->number[0] = '\0';
		entry->subentries_count = 0;
		if (error != GN_ERR_NONE) return error;
		n = pkt_get_uint16(&pkt);
		for (i = 0; i < n; i++) {
			pkt_get_string(name, sizeof(name), &pkt);
			pkt_get_string(val, sizeof(val), &pkt);
			dprintf("name: [%s] val: [%s]\n", name, val);
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}
