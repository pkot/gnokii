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
#include <assert.h>

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

#define	REQUEST_DEF	REQUEST_DEFN(1024)

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
static gn_error gnapplet_get_phone_info(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_identify(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_read_phonebook(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_write_phonebook(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_delete_phonebook(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_memory_status(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_get_network_info(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_get_rf_level(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_get_power_info(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_get_status(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_folder_list(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_folder_status(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_folder_create(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_folder_delete(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_message_read(gn_data *data, struct gn_statemachine *state);

static gn_error gnapplet_incoming_info(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_phonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_netinfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_power(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_debug(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_sms(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

static gn_incoming_function_type gnapplet_incoming_functions[] = {
	{ GNAPPLET_MSG_INFO,		gnapplet_incoming_info },
	{ GNAPPLET_MSG_PHONEBOOK,	gnapplet_incoming_phonebook },
	{ GNAPPLET_MSG_NETINFO,		gnapplet_incoming_netinfo },
	{ GNAPPLET_MSG_POWER,		gnapplet_incoming_power },
	{ GNAPPLET_MSG_DEBUG,		gnapplet_incoming_debug },
	{ GNAPPLET_MSG_SMS,		gnapplet_incoming_sms },
	{ 0,				NULL}
};

gn_driver driver_gnapplet = {
	gnapplet_incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"gnapplet|series60|3650|6600|sx1",	/* Supported models */
		0,			/* Max RF Level */
		100,			/* Min RF Level */
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


static void gnapplet_get_sms_number(char *buf, pkt_buffer *pkt, int smsc)
{
	char num[GN_PHONEBOOK_NUMBER_MAX_LENGTH];
	int type;

	pkt_get_string(num, sizeof(num), pkt);

	type = (num[0] == '+') ? GN_GSM_NUMBER_International : GN_GSM_NUMBER_Unknown;
	buf[0] = char_semi_octet_pack(num, buf + 1, type);
	if (smsc) {
		if (buf[0] % 2) buf[0]++;
		if (buf[0]) buf[0] = buf[0] / 2 + 1;
	}
}


static void gnapplet_get_time(char *buf, pkt_buffer *pkt)
{
	gn_timestamp dt;

	pkt_get_timestamp(&dt, pkt);
	sms_timestamp_pack(&dt, buf);
}


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
	case GN_OP_WritePhonebook:
		return gnapplet_write_phonebook(data, state);
	case GN_OP_DeletePhonebook:
		return gnapplet_delete_phonebook(data, state);
	case GN_OP_GetMemoryStatus:
		return gnapplet_memory_status(data, state);
	case GN_OP_GetNetworkInfo:
		return gnapplet_get_network_info(data, state);
	case GN_OP_GetRFLevel:
		return gnapplet_get_rf_level(data, state);
	case GN_OP_GetBatteryLevel:
	case GN_OP_GetPowersource:
		return gnapplet_get_power_info(data, state);
	case GN_OP_GetSMSStatus:
		return gnapplet_sms_get_status(data, state);
	case GN_OP_GetSMSFolders:
		return gnapplet_sms_folder_list(data, state);
	case GN_OP_GetSMSFolderStatus:
		return gnapplet_sms_folder_status(data, state);
	case GN_OP_CreateSMSFolder:
		return gnapplet_sms_folder_create(data, state);
	case GN_OP_DeleteSMSFolder:
		return gnapplet_sms_folder_delete(data, state);
	case GN_OP_GetSMS:
		return gnapplet_sms_message_read(data, state);
	default:
		dprintf("gnapplet unimplemented operation: %d\n", op);
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static gn_error gnapplet_initialise(struct gn_statemachine *state)
{
	gn_error err;
	gn_data d;
	int i;

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_gnapplet, sizeof(gn_driver));

	if (!(DRVINSTANCE(state) = calloc(1, sizeof(gnapplet_driver_instance))))
		return GN_ERR_MEMORYFULL;

	switch (state->config.connection_type) {
	case GN_CT_Irda:
	case GN_CT_Serial:
	case GN_CT_Infrared:
	case GN_CT_TCP:
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

	gn_data_clear(&d);
	if ((err = gnapplet_identify(&d, state)) != GN_ERR_NONE) {
		FREE(DRVINSTANCE(state));
		return err;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_get_phone_info(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_INFO_ID_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_INFO);
}


static gn_error gnapplet_identify(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	gn_error error;

	if (!drvinst->manufacturer[0]) {
		if ((error = gnapplet_get_phone_info(data, state)) != GN_ERR_NONE)
			return error;
	}

	if (data->manufacturer) snprintf(data->manufacturer, 20, "%s", drvinst->manufacturer);
	if (data->model) snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", drvinst->model);
	if (data->imei) snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", drvinst->imei);
	if (data->revision) snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW %s", drvinst->sw_version, drvinst->hw_version);
	//data->phone = drvinst->phone;

	return GN_ERR_NONE;
}


static gn_error gnapplet_incoming_info(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_INFO_ID_RESP:
		if (error != GN_ERR_NONE) return error;
		drvinst->proto_major = pkt_get_uint16(&pkt);
		drvinst->proto_minor = pkt_get_uint16(&pkt);
		/* Compatibility isn't important early in the development */
		/* if (proto_major != GNAPPLET_MAJOR_VERSION) { */
		if (drvinst->proto_major != GNAPPLET_MAJOR_VERSION || drvinst->proto_minor != GNAPPLET_MINOR_VERSION) {
			dprintf("gnapplet version: %d.%d, gnokii driver: %d.%d\n",
				drvinst->proto_major, drvinst->proto_minor,
				GNAPPLET_MAJOR_VERSION, GNAPPLET_MINOR_VERSION);
			return GN_ERR_INTERNALERROR;
		}
		pkt_get_string(drvinst->manufacturer, sizeof(drvinst->manufacturer), &pkt);
		pkt_get_string(drvinst->model, sizeof(drvinst->model), &pkt);
		pkt_get_string(drvinst->imei, sizeof(drvinst->imei), &pkt);
		pkt_get_string(drvinst->sw_version, sizeof(drvinst->sw_version), &pkt);
		pkt_get_string(drvinst->hw_version, sizeof(drvinst->hw_version), &pkt);
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
	pkt_put_uint32(&pkt, data->phonebook_entry->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_write_phonebook(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	gn_phonebook_subentry *se;
	int i, need_defnumber;
	REQUEST_DEF;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;
	if (!data->phonebook_entry->name[0])
		return gnapplet_delete_phonebook(data, state);

	need_defnumber = 1;
	for (i = 0; i < data->phonebook_entry->subentries_count; i++) {
		se = data->phonebook_entry->subentries + i;
		if (se->entry_type == GN_PHONEBOOK_ENTRY_Number && strcmp(se->data.number, data->phonebook_entry->number) == 0) {
			need_defnumber = 0;
			break;
		}
	}

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PHONEBOOK_WRITE_REQ);
	pkt_put_uint16(&pkt, data->phonebook_entry->memory_type);
	pkt_put_uint32(&pkt, data->phonebook_entry->location);

	pkt_put_uint16(&pkt, data->phonebook_entry->subentries_count + 1 + need_defnumber);

	pkt_put_uint16(&pkt, GN_PHONEBOOK_ENTRY_Name);
	pkt_put_uint16(&pkt, 0);
	pkt_put_string(&pkt, data->phonebook_entry->name);

	if (need_defnumber) {
		pkt_put_uint16(&pkt, GN_PHONEBOOK_ENTRY_Number);
		pkt_put_uint16(&pkt, GN_PHONEBOOK_NUMBER_General);
		pkt_put_string(&pkt, data->phonebook_entry->number);
	}

	for (i = 0; i < data->phonebook_entry->subentries_count; i++) {
		se = data->phonebook_entry->subentries + i;
		pkt_put_uint16(&pkt, se->entry_type);
		pkt_put_uint16(&pkt, se->number_type);
		pkt_put_string(&pkt, se->data.number);
	}

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_delete_phonebook(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PHONEBOOK_DELETE_REQ);
	pkt_put_uint16(&pkt, data->phonebook_entry->memory_type);
	pkt_put_uint32(&pkt, data->phonebook_entry->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_memory_status(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->memory_status) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PHONEBOOK_STATUS_REQ);
	pkt_put_uint16(&pkt, data->memory_status->memory_type);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_incoming_phonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i, n, type, subtype;
	gn_phonebook_entry *entry;
	gn_phonebook_subentry *se;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_PHONEBOOK_READ_RESP:
		if (!(entry = data->phonebook_entry)) return GN_ERR_INTERNALERROR;
		entry->empty = false;
		entry->caller_group = 5;
		entry->name[0] = '\0';
		entry->number[0] = '\0';
		entry->subentries_count = 0;
		memset(&entry->date, 0, sizeof(entry->date));
		if (error != GN_ERR_NONE) return error;
		n = pkt_get_uint16(&pkt);
		assert(n < GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER);
		for (i = 0; i < n; i++) {
			se = entry->subentries + entry->subentries_count;
			type = pkt_get_uint16(&pkt);
			subtype = pkt_get_uint16(&pkt);
			switch (type) {
			case GN_PHONEBOOK_ENTRY_Name:
				pkt_get_string(entry->name, sizeof(entry->name), &pkt);
				break;
			case GN_PHONEBOOK_ENTRY_Number:
				se->entry_type = type;
				se->number_type = subtype;
				se->id = 0;
				pkt_get_string(se->data.number, sizeof(se->data.number), &pkt);
				entry->subentries_count++;
				if (!entry->number[0]) {
					snprintf(entry->number, sizeof(entry->number), "%s", se->data.number);
				}
				break;
			case GN_PHONEBOOK_ENTRY_Date:
				se->entry_type = type;
				se->number_type = subtype;
				se->id = 0;
				pkt_get_timestamp(&se->data.date, &pkt);
				entry->subentries_count++;
				entry->date = se->data.date;
				break;
			default:
				se->entry_type = type;
				se->number_type = subtype;
				se->id = 0;
				pkt_get_string(se->data.number, sizeof(se->data.number), &pkt);
				entry->subentries_count++;
				break;
			}
		}
		break;

	case GNAPPLET_MSG_PHONEBOOK_WRITE_RESP:
		if (!(entry = data->phonebook_entry)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		entry->memory_type = pkt_get_uint16(&pkt);
		entry->location = pkt_get_uint32(&pkt);
		break;

	case GNAPPLET_MSG_PHONEBOOK_DELETE_RESP:
		if (!(entry = data->phonebook_entry)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		entry->memory_type = pkt_get_uint16(&pkt);
		entry->location = pkt_get_uint32(&pkt);
		break;

	case GNAPPLET_MSG_PHONEBOOK_STATUS_RESP:
		if (!data->memory_status) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		data->memory_status->memory_type = pkt_get_uint16(&pkt);
		data->memory_status->used = pkt_get_uint32(&pkt);
		data->memory_status->free = pkt_get_uint32(&pkt);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_get_network_info(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->network_info) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_NETINFO_GETCURRENT_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_NETINFO);
}


static gn_error gnapplet_get_rf_level(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->rf_unit || !data->rf_level) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_NETINFO_GETRFLEVEL_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_NETINFO);
}


static gn_error gnapplet_incoming_netinfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_network_info *netinfo;
	uint16_t cellid, lac;
	uint32_t strength;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_NETINFO_GETCURRENT_RESP:
		if (!(netinfo = data->network_info)) return GN_ERR_INTERNALERROR;
		memset(netinfo, 0, sizeof(gn_network_info));
		if (error != GN_ERR_NONE) return error;
		cellid = pkt_get_uint16(&pkt);
		netinfo->cell_id[0] = cellid / 256;
		netinfo->cell_id[1] = cellid % 256;
		lac = pkt_get_uint16(&pkt);
		netinfo->LAC[0] = lac / 256;
		netinfo->LAC[1] = lac % 256;
		pkt_get_uint8(&pkt); /* registration status */
		pkt_get_string(netinfo->network_code, sizeof(netinfo->network_code), &pkt);
		break;

	case GNAPPLET_MSG_NETINFO_GETRFLEVEL_RESP:
		if (!data->rf_unit || !data->rf_level) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		*data->rf_unit = GN_RF_Percentage;
		*data->rf_level = pkt_get_uint8(&pkt);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_get_power_info(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->battery_unit && !data->battery_level && !data->power_source) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_POWER_INFO_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_POWER);
}


static gn_error gnapplet_incoming_power(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	uint8_t percent, source;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_POWER_INFO_RESP:
		percent = pkt_get_uint8(&pkt);
		source = pkt_get_uint8(&pkt);
		if (error != GN_ERR_NONE) return error;
		if (data->battery_unit) *data->battery_unit = GN_RF_Percentage;
		if (data->battery_level) *data->battery_level = percent;
		if (data->power_source) *data->power_source = source;
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_incoming_debug(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	char msg[1024];
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_DEBUG_NOTIFICATION:
		if (error != GN_ERR_NONE) return error;
		pkt_get_string(msg, sizeof(msg), &pkt);
		dprintf("PHONE: %s\n", msg);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_UNSOLICITED;
}


static gn_error gnapplet_sms_get_status(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->sms_status) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_STATUS_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_list(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->sms_folder_list) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_LIST_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_status(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->sms_folder) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_STATUS_REQ);
	pkt_put_uint16(&pkt, data->sms_folder->folder_id);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_create(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->sms_folder) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_CREATE_REQ);
	pkt_put_string(&pkt, data->sms_folder->name);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_delete(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	REQUEST_DEF;

	if (!data->sms_folder) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_DELETE_REQ);
	pkt_put_uint16(&pkt, data->sms_folder->folder_id);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_validate(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (!data->sms_folder) return GN_ERR_INTERNALERROR;

	if (data->sms_folder->folder_id != data->raw_sms->memory_type) {
		data->sms_folder->folder_id = data->raw_sms->memory_type;
		if ((error = gnapplet_sms_folder_status(data, state)) != GN_ERR_NONE)
			return error;
	}

	if (data->raw_sms->number < 1) return GN_ERR_INVALIDLOCATION;
	if (data->raw_sms->number > data->sms_folder->number)
		return GN_ERR_EMPTYLOCATION;

	return GN_ERR_NONE;
}


static gn_error gnapplet_sms_message_read(gn_data *data, struct gn_statemachine *state)
{
	gnapplet_driver_instance *drvinst = DRVINSTANCE(state);
	gn_error error;
	REQUEST_DEF;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	if ((error = gnapplet_sms_validate(data, state)) != GN_ERR_NONE) return error;
	data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_MESSAGE_READ_REQ);
	pkt_put_uint16(&pkt, data->raw_sms->memory_type);
	pkt_put_uint32(&pkt, data->raw_sms->number);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_incoming_sms(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_sms_folder_list *folders;
	gn_sms_folder *folder;
	gn_sms_raw *rawsms;
	int i;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_SMS_STATUS_RESP:
		if (!data->sms_status) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		data->sms_status->number = pkt_get_uint32(&pkt);
		data->sms_status->unread = pkt_get_uint32(&pkt);
		data->sms_status->changed = pkt_get_bool(&pkt);
		data->sms_status->folders_count = pkt_get_uint16(&pkt);
		break;

	case GNAPPLET_MSG_SMS_FOLDER_LIST_RESP:
		if (!(folders = data->sms_folder_list)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		memset(folders, 0, sizeof(gn_sms_folder_list));
		folders->number = pkt_get_uint16(&pkt);
		assert(folders->number <= GN_SMS_FOLDER_MAX_NUMBER);
		for (i = 0; i < folders->number; i++) {
			folders->folder[i].folder_id = pkt_get_uint16(&pkt);
			pkt_get_string(folders->folder[i].name, sizeof(folders->folder[i].name), &pkt);
			folders->folder_id[i] = folders->folder[i].folder_id;
		}
		break;

	case GNAPPLET_MSG_SMS_FOLDER_STATUS_RESP:
		if (!(folder = data->sms_folder)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		folder->folder_id = pkt_get_uint16(&pkt);
		folder->number = pkt_get_uint32(&pkt);
		assert(folder->number <= GN_SMS_MESSAGE_MAX_NUMBER);
		for (i = 0; i < folder->number; i++) {
			folder->locations[i] = pkt_get_uint32(&pkt);
		}
		break;

	case GNAPPLET_MSG_SMS_FOLDER_CREATE_RESP:
		if (!(folder = data->sms_folder)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		memset(folder, 0, sizeof(gn_sms_folder));
		folder->folder_id = pkt_get_uint16(&pkt);
		pkt_get_string(folder->name, sizeof(folder->name), &pkt);
		break;

	case GNAPPLET_MSG_SMS_FOLDER_DELETE_RESP:
		if (!(folder = data->sms_folder)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		memset(folder, 0, sizeof(gn_sms_folder));
		folder->folder_id = pkt_get_uint16(&pkt);
		break;

	case GNAPPLET_MSG_SMS_MESSAGE_READ_RESP:
		if (!(rawsms = data->raw_sms)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		memset(rawsms, 0, sizeof(rawsms));
		rawsms->type = pkt_get_uint8(&pkt);
		rawsms->more_messages = pkt_get_bool(&pkt);
		rawsms->reply_via_same_smsc = pkt_get_bool(&pkt);
		rawsms->reject_duplicates = pkt_get_bool(&pkt);
		rawsms->report = pkt_get_bool(&pkt);
		rawsms->number = pkt_get_uint8(&pkt);
		rawsms->reference = pkt_get_uint8(&pkt);
		rawsms->pid = pkt_get_uint8(&pkt);
		rawsms->report_status = pkt_get_uint8(&pkt);
		gnapplet_get_time(rawsms->smsc_time, &pkt);
		gnapplet_get_time(rawsms->time, &pkt);
		gnapplet_get_sms_number(rawsms->message_center, &pkt, 1);
		gnapplet_get_sms_number(rawsms->remote_number, &pkt, 0);
		rawsms->dcs = pkt_get_uint8(&pkt);
		rawsms->udh_indicator = pkt_get_bool(&pkt) ? 0x40 : 0x00;
		rawsms->length = pkt_get_bytes(rawsms->user_data, sizeof(rawsms->user_data), &pkt);
		rawsms->user_data_length = rawsms->length;
		rawsms->validity_indicator = pkt_get_uint8(&pkt);
		pkt_get_bytes(rawsms->validity, sizeof(rawsms->validity), &pkt);
		rawsms->memory_type = pkt_get_uint16(&pkt);
		rawsms->status = pkt_get_uint8(&pkt);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}
