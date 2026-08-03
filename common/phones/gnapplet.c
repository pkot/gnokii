/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2004-2005 BORBELY Zoltan

  This file provides functions specific to the gnapplet series.
  See README for more details on supported mobile phones.

*/

#include <assert.h>
#include "config.h"
#include "compat.h"
#include "misc.h"
#include "phones/generic.h"
#include "phones/gnapplet.h"
#include "links/gnbus.h"

#include "pkt.h"
#include "gnokii-internal.h"
#include "gnokii.h"

#define	DRVINSTANCE(s) (*((gnapplet_driver_instance **)(&(s)->driver.driver_instance)))
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

#define	SEND_MESSAGE_BLOCK_NO_RETRY(type) \
	do { \
		if (sm_message_send(pkt.offs, type, pkt.addr, state)) return GN_ERR_NOTREADY; \
		return sm_block_no_retry(type, data, state); \
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
static gn_error gnapplet_sms_message_read_nv(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_message_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_message_write(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_message_send(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_message_delete_nv(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_message_delete(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_center_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_sms_center_write(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_note_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_note_write(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_note_delete(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_todo_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_todo_write(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_todo_delete(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_calendar_todo_delete_all(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_clock_datetime_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_clock_datetime_write(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_clock_alarm_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_clock_alarm_write(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_profile_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_profile_active_read(gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_profile_active_write(gn_data *data, struct gn_statemachine *state);

static gn_error gnapplet_incoming_info(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_phonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_netinfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_power(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_debug(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_sms(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_calendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_clock(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error gnapplet_incoming_profile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

static gn_incoming_function_type gnapplet_incoming_functions[] = {
	{ GNAPPLET_MSG_INFO,		gnapplet_incoming_info },
	{ GNAPPLET_MSG_PHONEBOOK,	gnapplet_incoming_phonebook },
	{ GNAPPLET_MSG_NETINFO,		gnapplet_incoming_netinfo },
	{ GNAPPLET_MSG_POWER,		gnapplet_incoming_power },
	{ GNAPPLET_MSG_DEBUG,		gnapplet_incoming_debug },
	{ GNAPPLET_MSG_SMS,		gnapplet_incoming_sms },
	{ GNAPPLET_MSG_CALENDAR,	gnapplet_incoming_calendar },
	{ GNAPPLET_MSG_CLOCK,		gnapplet_incoming_clock },
	{ GNAPPLET_MSG_PROFILE,		gnapplet_incoming_profile },
	{ 0,				NULL}
};

gn_driver driver_gnapplet = {
	gnapplet_incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"gnapplet|symbian|3650|6600|sx1",	/* Supported models */
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
	case GN_OP_GetSMSnoValidate:
		return gnapplet_sms_message_read_nv(data, state);
	case GN_OP_GetSMS:
		return gnapplet_sms_message_read(data, state);
	case GN_OP_SaveSMS:
		return gnapplet_sms_message_write(data, state);
	case GN_OP_SendSMS:
		return gnapplet_sms_message_send(data, state);
	case GN_OP_DeleteSMSnoValidate:
		return gnapplet_sms_message_delete_nv(data, state);
	case GN_OP_DeleteSMS:
		return gnapplet_sms_message_delete(data, state);
	case GN_OP_GetSMSCenter:
		return gnapplet_sms_center_read(data, state);
	case GN_OP_SetSMSCenter:
		return gnapplet_sms_center_write(data, state);
	case GN_OP_GetCalendarNote:
		return gnapplet_calendar_note_read(data, state);
	case GN_OP_WriteCalendarNote:
		return gnapplet_calendar_note_write(data, state);
	case GN_OP_DeleteCalendarNote:
		return gnapplet_calendar_note_delete(data, state);
	case GN_OP_GetToDo:
		return gnapplet_calendar_todo_read(data, state);
	case GN_OP_WriteToDo:
		return gnapplet_calendar_todo_write(data, state);
	case GN_OP_DeleteAllToDos:
		return gnapplet_calendar_todo_delete_all(data, state);
	case GN_OP_GetDateTime:
		return gnapplet_clock_datetime_read(data, state);
	case GN_OP_SetDateTime:
		return gnapplet_clock_datetime_write(data, state);
	case GN_OP_GetAlarm:
		return gnapplet_clock_alarm_read(data, state);
	case GN_OP_SetAlarm:
		return gnapplet_clock_alarm_write(data, state);
	case GN_OP_GetProfile:
		return gnapplet_profile_read(data, state);
	case GN_OP_GetActiveProfile:
		return gnapplet_profile_active_read(data, state);
	case GN_OP_SetActiveProfile:
		return gnapplet_profile_active_write(data, state);
	default:
		dprintf("gnapplet unimplemented operation: %d\n", op);
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static gn_error gnapplet_initialise(struct gn_statemachine *state)
{
	gn_error err;
	gn_data d;

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

	if (data->manufacturer) snprintf(data->manufacturer, GN_MANUFACTURER_MAX_LENGTH, "%s", drvinst->manufacturer);
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
	REQUEST_DEF;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PHONEBOOK_READ_REQ);
	pkt_put_uint16(&pkt, data->phonebook_entry->memory_type);
	pkt_put_uint32(&pkt, data->phonebook_entry->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_write_phonebook(gn_data *data, struct gn_statemachine *state)
{
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
	REQUEST_DEF;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PHONEBOOK_DELETE_REQ);
	pkt_put_uint16(&pkt, data->phonebook_entry->memory_type);
	pkt_put_uint32(&pkt, data->phonebook_entry->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PHONEBOOK);
}


static gn_error gnapplet_memory_status(gn_data *data, struct gn_statemachine *state)
{
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
		entry->empty = true;
		entry->caller_group = 5;
		entry->name[0] = '\0';
		entry->number[0] = '\0';
		entry->subentries_count = 0;
		memset(&entry->date, 0, sizeof(entry->date));
		if (error != GN_ERR_NONE) return error;
		entry->empty = false;
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
	REQUEST_DEF;

	if (!data->network_info) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_NETINFO_GETCURRENT_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_NETINFO);
}


static gn_error gnapplet_get_rf_level(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->rf_unit || !data->rf_level) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_NETINFO_GETRFLEVEL_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_NETINFO);
}


static gn_error gnapplet_incoming_netinfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_network_info *netinfo;
	uint16_t cellid, lac;
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
		if (data->battery_unit) *data->battery_unit = GN_BU_Percentage;
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
	REQUEST_DEF;

	if (!data->sms_status) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_STATUS_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_list(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->sms_folder_list) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_LIST_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_status(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->sms_folder) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_STATUS_REQ);
	pkt_put_uint16(&pkt, data->sms_folder->folder_id);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_create(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->sms_folder) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_FOLDER_CREATE_REQ);
	pkt_put_string(&pkt, data->sms_folder->name);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_folder_delete(gn_data *data, struct gn_statemachine *state)
{
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


static gn_error gnapplet_sms_message_read_nv(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_MESSAGE_READ_REQ);
	pkt_put_uint16(&pkt, data->raw_sms->memory_type);
	pkt_put_uint32(&pkt, data->raw_sms->number);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_message_read(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	if ((error = gnapplet_sms_validate(data, state)) != GN_ERR_NONE) return error;
	data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];

	return gnapplet_sms_message_read_nv(data, state);
}


static gn_error gnapplet_sms_message_write(gn_data *data, struct gn_statemachine *state)
{
	unsigned char buf[256];
	gn_error error;
	int n;
	REQUEST_DEF;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	n = sizeof(buf);
	if ((error = gn_sms_raw2pdu(buf, &n, data->raw_sms, 0)) != GN_ERR_NONE)
		return error;
	//if ((error = gnapplet_sms_validate(data, state)) != GN_ERR_NONE) return error;
	//data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_MESSAGE_WRITE_REQ);
	pkt_put_uint16(&pkt, data->raw_sms->memory_type);
	pkt_put_uint32(&pkt, data->raw_sms->number);
	pkt_put_bytes(&pkt, buf, n);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_message_send(gn_data *data, struct gn_statemachine *state)
{
	unsigned char buf[256];
	gn_error error;
	int n;
	REQUEST_DEF;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	n = sizeof(buf);
	if ((error = gn_sms_raw2pdu(buf, &n, data->raw_sms, 0)) != GN_ERR_NONE)
		return error;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_MESSAGE_SEND_REQ);
	pkt_put_bytes(&pkt, buf, n);

	SEND_MESSAGE_BLOCK_NO_RETRY(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_message_delete_nv(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_MESSAGE_DELETE_REQ);
	pkt_put_uint16(&pkt, data->raw_sms->memory_type);
	pkt_put_uint32(&pkt, data->raw_sms->number);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_message_delete(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	if ((error = gnapplet_sms_validate(data, state)) != GN_ERR_NONE) return error;
	data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];

	return gnapplet_sms_message_delete_nv(data, state);
}

#if 0
static gn_error gnapplet_sms_message_move_nv(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_MESSAGE_MOVE_REQ);
	pkt_put_uint16(&pkt, data->raw_sms->memory_type);
	pkt_put_uint32(&pkt, data->raw_sms->number);
	pkt_put_uint16(&pkt, 0); /* FIXME */

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_message_move(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;

	if ((error = gnapplet_sms_validate(data, state)) != GN_ERR_NONE) return error;
	data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];

	return gnapplet_sms_message_move_nv(data, state);
}
#endif

static gn_error gnapplet_sms_center_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->message_center) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_CENTER_READ_REQ);
	pkt_put_uint16(&pkt, data->message_center->id - 1);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_sms_center_write(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->message_center) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_SMS_CENTER_WRITE_REQ);
	pkt_put_uint16(&pkt, data->message_center->id - 1);
	pkt_put_string(&pkt, data->message_center->name);
	pkt_put_int16(&pkt, data->message_center->default_name);
	pkt_put_uint8(&pkt, data->message_center->format);
	pkt_put_uint8(&pkt, data->message_center->validity);
	pkt_put_uint8(&pkt, data->message_center->smsc.type);
	pkt_put_string(&pkt, data->message_center->smsc.number);
	pkt_put_uint8(&pkt, data->message_center->recipient.type);
	pkt_put_string(&pkt, data->message_center->recipient.number);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_SMS);
}


static gn_error gnapplet_incoming_sms(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_sms_folder_list *folders;
	gn_sms_folder *folder;
	gn_sms_raw *rawsms;
	gn_sms_message_center *smsc;
	unsigned char buf[256];
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
		memset(rawsms, 0, sizeof(*rawsms));
		i = pkt_get_bytes(buf, sizeof(buf), &pkt);
		//rawsms->memory_type = pkt_get_uint16(&pkt);
		rawsms->status = pkt_get_uint8(&pkt);
		/* the applet sends plain GSM 03.40 PDUs (via the Symbian SMS stack) */
		if ((error = gn_sms_pdu2raw(rawsms, buf, i, GN_SMS_PDU_DEFAULT)) != GN_ERR_NONE)
			return error;
		break;

	case GNAPPLET_MSG_SMS_MESSAGE_SEND_RESP:
		if (!(rawsms = data->raw_sms)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		break;
		
	case GNAPPLET_MSG_SMS_MESSAGE_DELETE_RESP:
		if (!(rawsms = data->raw_sms)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		break;
		
	case GNAPPLET_MSG_SMS_MESSAGE_MOVE_RESP:
		if (!(rawsms = data->raw_sms)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		break;
		
	case GNAPPLET_MSG_SMS_CENTER_READ_RESP:
		if (!(smsc=data->message_center)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		memset(smsc, 0, sizeof(*smsc));
		smsc->id = pkt_get_uint16(&pkt) + 1;
		pkt_get_string(smsc->name, sizeof(smsc->name), &pkt);
		smsc->default_name = pkt_get_int16(&pkt);
		smsc->format = pkt_get_uint8(&pkt);
		smsc->validity = pkt_get_uint8(&pkt);
		smsc->smsc.type = pkt_get_uint8(&pkt);
		pkt_get_string(smsc->smsc.number, sizeof(smsc->smsc.number), &pkt);
		smsc->recipient.type = pkt_get_uint8(&pkt);
		pkt_get_string(smsc->recipient.number, sizeof(smsc->recipient.number), &pkt);
		break;
		
	case GNAPPLET_MSG_SMS_CENTER_WRITE_RESP:
		if (!(smsc=data->message_center)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		smsc->id = pkt_get_uint16(&pkt) + 1;
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_calendar_note_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->calnote) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CALENDAR_NOTE_READ_REQ);
	pkt_put_uint32(&pkt, data->calnote->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CALENDAR);
}


static gn_error gnapplet_calendar_note_write(gn_data *data, struct gn_statemachine *state)
{
	gn_timestamp null_ts;
	REQUEST_DEF;

	if (!data->calnote) return GN_ERR_INTERNALERROR;

	memset(&null_ts, 0, sizeof(null_ts));
	pkt_put_uint16(&pkt, GNAPPLET_MSG_CALENDAR_NOTE_WRITE_REQ);
	pkt_put_uint32(&pkt, data->calnote->location);
	pkt_put_uint8(&pkt, data->calnote->type);
	pkt_put_timestamp(&pkt, &data->calnote->time);
	if (data->calnote->end_time.year)
		pkt_put_timestamp(&pkt, &data->calnote->end_time);
	else
		pkt_put_timestamp(&pkt, &null_ts);
	if (data->calnote->alarm.enabled && data->calnote->alarm.timestamp.year != 0)
		pkt_put_timestamp(&pkt, &data->calnote->alarm.timestamp);
	else {
		pkt_put_timestamp(&pkt, &null_ts);
	}
	pkt_put_string(&pkt, data->calnote->text);
	if (data->calnote->type == GN_CALNOTE_CALL)
		pkt_put_string(&pkt, data->calnote->phone_number);
	else
		pkt_put_string(&pkt, "");
	if (data->calnote->type == GN_CALNOTE_MEETING)
		pkt_put_string(&pkt, data->calnote->mlocation);
	else
		pkt_put_string(&pkt, "");
	pkt_put_uint16(&pkt, data->calnote->recurrence);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CALENDAR);
}


static gn_error gnapplet_calendar_note_delete(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->calnote) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CALENDAR_NOTE_DELETE_REQ);
	pkt_put_uint32(&pkt, data->calnote->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CALENDAR);
}


static gn_error gnapplet_calendar_todo_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->todo) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CALENDAR_TODO_READ_REQ);
	pkt_put_uint32(&pkt, data->todo->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CALENDAR);
}


static gn_error gnapplet_calendar_todo_write(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->todo) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CALENDAR_TODO_WRITE_REQ);
	pkt_put_uint32(&pkt, data->todo->location);
	pkt_put_string(&pkt, data->todo->text);
	pkt_put_uint8(&pkt, data->todo->priority);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CALENDAR);
}


static gn_error gnapplet_calendar_todo_delete(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->todo) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CALENDAR_TODO_DELETE_REQ);
	pkt_put_uint32(&pkt, data->todo->location);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CALENDAR);
}


static gn_error gnapplet_calendar_todo_delete_all(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_todo todo;
	gn_todo *todo_save;

	todo_save = data->todo;
	memset(&todo, 0, sizeof(todo));
	data->todo = &todo;

	do {
		data->todo->location = 1;
		error = gnapplet_calendar_todo_delete(data, state);
	} while (error == GN_ERR_NONE);

	data->todo = todo_save;

	return GN_ERR_NONE;
}


static gn_error gnapplet_incoming_calendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_calnote *cal;
	gn_todo *todo;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_CALENDAR_NOTE_READ_RESP:
		if (!(cal = data->calnote)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		cal->location = pkt_get_uint32(&pkt);
		cal->type = pkt_get_uint8(&pkt);
		pkt_get_timestamp(&cal->time, &pkt);
		pkt_get_timestamp(&cal->end_time, &pkt);
		pkt_get_timestamp(&cal->alarm.timestamp, &pkt);
		cal->alarm.enabled = (cal->alarm.timestamp.year != 0);
		pkt_get_string(cal->text, sizeof(cal->text), &pkt);
		pkt_get_string(cal->phone_number, sizeof(cal->phone_number), &pkt);
		pkt_get_string(cal->mlocation, sizeof(cal->mlocation), &pkt);
		cal->recurrence = pkt_get_uint16(&pkt);
		break;

	case GNAPPLET_MSG_CALENDAR_NOTE_WRITE_RESP:
		if (!(cal = data->calnote)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		cal->location = pkt_get_uint32(&pkt);
		break;

	case GNAPPLET_MSG_CALENDAR_NOTE_DELETE_RESP:
		if (!(cal = data->calnote)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		cal->location = pkt_get_uint32(&pkt);
		break;

	case GNAPPLET_MSG_CALENDAR_TODO_READ_RESP:
		if (!(todo = data->todo)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		todo->location = pkt_get_uint32(&pkt);
		pkt_get_string(todo->text, sizeof(todo->text), &pkt);
		todo->priority = pkt_get_uint8(&pkt);
		break;

	case GNAPPLET_MSG_CALENDAR_TODO_WRITE_RESP:
		if (!(todo = data->todo)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		todo->location = pkt_get_uint32(&pkt);
		break;

	case GNAPPLET_MSG_CALENDAR_TODO_DELETE_RESP:
		if (!(todo = data->todo)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		todo->location = pkt_get_uint32(&pkt);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_clock_datetime_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->datetime) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CLOCK_DATETIME_READ_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CLOCK);
}


static gn_error gnapplet_clock_datetime_write(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->datetime) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CLOCK_DATETIME_WRITE_REQ);
	pkt_put_timestamp(&pkt, data->datetime);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CLOCK);
}


static gn_error gnapplet_clock_alarm_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->alarm) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CLOCK_ALARM_READ_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CLOCK);
}


static gn_error gnapplet_clock_alarm_write(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->alarm) return GN_ERR_INTERNALERROR;
	if (!data->alarm->enabled) {
		data->alarm->timestamp.hour = 0;
		data->alarm->timestamp.minute = 0;
		data->alarm->timestamp.second = 0;
	}
	data->alarm->timestamp.year = 0;
	data->alarm->timestamp.month = 1;
	data->alarm->timestamp.day = 1;
	data->alarm->timestamp.timezone = 0;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_CLOCK_ALARM_WRITE_REQ);
	pkt_put_bool(&pkt, data->alarm->enabled);
	pkt_put_timestamp(&pkt, &data->alarm->timestamp);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_CLOCK);
}


static gn_error gnapplet_incoming_clock(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_CLOCK_DATETIME_READ_RESP:
		if (!data->datetime) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		pkt_get_timestamp(data->datetime, &pkt);
		break;

	case GNAPPLET_MSG_CLOCK_DATETIME_WRITE_RESP:
		if (data->datetime) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		break;

	case GNAPPLET_MSG_CLOCK_ALARM_READ_RESP:
		if (!data->alarm) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		data->alarm->enabled = pkt_get_bool(&pkt);
		pkt_get_timestamp(&data->alarm->timestamp, &pkt);
		break;

	case GNAPPLET_MSG_CLOCK_ALARM_WRITE_RESP:
		if (data->alarm) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error gnapplet_profile_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->profile) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PROFILE_READ_REQ);
	pkt_put_uint16(&pkt, data->profile->number);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PROFILE);
}


static gn_error gnapplet_profile_active_read(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->profile) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PROFILE_GET_ACTIVE_REQ);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PROFILE);
}


static gn_error gnapplet_profile_active_write(gn_data *data, struct gn_statemachine *state)
{
	REQUEST_DEF;

	if (!data->profile) return GN_ERR_INTERNALERROR;

	pkt_put_uint16(&pkt, GNAPPLET_MSG_PROFILE_SET_ACTIVE_REQ);
	pkt_put_uint16(&pkt, data->profile->number);

	SEND_MESSAGE_BLOCK(GNAPPLET_MSG_PROFILE);
}


static gn_error gnapplet_incoming_profile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_profile *profile;
	REPLY_DEF;

	switch (code) {

	case GNAPPLET_MSG_PROFILE_READ_RESP:
		if (!(profile = data->profile)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		profile->number = pkt_get_uint16(&pkt);
		pkt_get_string(profile->name, sizeof(profile->name), &pkt);
		profile->default_name = pkt_get_int16(&pkt);
		profile->keypad_tone = pkt_get_uint8(&pkt);
		profile->lights = 0;
		profile->call_alert = pkt_get_uint8(&pkt);
		profile->ringtone = 0; //!!!FIXME
		profile->volume = pkt_get_uint8(&pkt);
		profile->message_tone = 0;
		profile->warning_tone = pkt_get_uint8(&pkt);
		profile->vibration = pkt_get_uint8(&pkt);
		profile->caller_groups = 0;
		profile->automatic_answer = 0;
		break;

	case GNAPPLET_MSG_PROFILE_GET_ACTIVE_RESP:
		if (!(profile = data->profile)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		profile->number = pkt_get_uint16(&pkt);
		break;

	case GNAPPLET_MSG_PROFILE_SET_ACTIVE_RESP:
		if (!(profile = data->profile)) return GN_ERR_INTERNALERROR;
		if (error != GN_ERR_NONE) return error;
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}
