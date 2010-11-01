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

  Copyright (C) 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2001 Ladis Michl, Marian Jancar
  Copyright (C) 2001-2002 Markus Plail
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan
  Copyright (C) 2002-2003 Ladis Michl
  Copyright (C) 2002 Marcin Wiacek

  This file provides functions specific to the Nokia 7110 series.
  See README for more details on supported mobile phones.

  The various routines are called P7110_(whatever).

*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "gnokii.h"
#include "nokia-decoding.h"
#include "phones/nk7110.h"
#include "phones/generic.h"
#include "phones/nokia.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "links/m2bus.h"

#include "gnokii-internal.h"

#define DRVINSTANCE(s) (*((nk7110_driver_instance **)(&(s)->driver.driver_instance)))
#define FREE(p) do { free(p); (p) = NULL; } while (0)

#define SEND_MESSAGE_BLOCK(type, length) \
do { \
	if (sm_message_send(length, type, req, state)) return GN_ERR_NOTREADY; \
	return sm_block(type, data, state); \
} while (0)

#define SEND_MESSAGE_WAITFOR(type, length) \
do { \
	if (sm_message_send(length, type, req, state)) return GN_ERR_NOTREADY; \
	return sm_wait_for(type, data, state); \
} while (0)

/* Functions prototypes */
static gn_error NK7110_Functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_Initialise(struct gn_statemachine *state);
static gn_error NK7110_GetModel(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetRevision(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetIMEI(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_Identify(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetBatteryLevel(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetRFLevel(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetMemoryStatus(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SetBitmap(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetBitmap(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_WritePhonebookLocation(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_ReadPhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_DeletePhonebookLocation(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetNetworkInfo(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSpeedDial(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSMSCenter(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetClock(char req_type, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SetClock(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SetAlarm(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_WriteCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_DeleteCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SendSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SaveSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSMSnoValidate(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_PollSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_DeleteSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_DeleteSMSnoValidate(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetPictureList(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSMSFolders(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSMSStatus(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetSecurityCode(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_PressOrReleaseKey(gn_data *data, struct gn_statemachine *state, bool press);
static gn_error NK7110_GetRawRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SetRawRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SetRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetRingtoneList(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetProfile(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_SetProfile(gn_data *data, struct gn_statemachine *state);

static gn_error NK7110_GetActiveCalls(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_MakeCall(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_CancelCall(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_AnswerCall(gn_data *data, struct gn_statemachine *state);

static gn_error NK7110_DeleteWAPBookmark(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetWAPBookmark(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_WriteWAPBookmark(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_GetWAPSetting(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_ActivateWAPSetting(gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_WriteWAPSetting(gn_data *data, struct gn_statemachine *state);

static gn_error NK7110_IncomingIdentify(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingNetwork(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingBattLevel(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingStartup(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingFolder(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingClock(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingCalendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingSecurity(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingWAP(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingKeypress(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingProfile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingRingtone(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK7110_IncomingCommstatus(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

static int get_memory_type(gn_memory_type memory_type);
static gn_memory_type get_gn_memory_type(int memory_type);
static gn_error NBSUpload(gn_data *data, struct gn_statemachine *state, gn_sms_data_type type);

static gn_incoming_function_type nk7110_incoming_functions[] = {
	{ NK7110_MSG_FOLDER,		NK7110_IncomingFolder },
	{ NK7110_MSG_SMS,		NK7110_IncomingSMS },
	{ NK7110_MSG_PHONEBOOK,		NK7110_IncomingPhonebook },
	{ NK7110_MSG_NETSTATUS,		NK7110_IncomingNetwork },
	{ NK7110_MSG_CALENDAR,		NK7110_IncomingCalendar },
	{ NK7110_MSG_BATTERY,		NK7110_IncomingBattLevel },
	{ NK7110_MSG_CLOCK,		NK7110_IncomingClock },
	{ NK7110_MSG_IDENTITY,		NK7110_IncomingIdentify },
	{ NK7110_MSG_STLOGO,		NK7110_IncomingStartup },
	{ NK7110_MSG_DIVERT,		pnok_call_divert_incoming },
	{ NK7110_MSG_SECURITY,		NK7110_IncomingSecurity },
	{ NK7110_MSG_WAP,		NK7110_IncomingWAP },
	{ NK7110_MSG_KEYPRESS_RESP,	NK7110_IncomingKeypress },
	{ NK7110_MSG_PROFILE,		NK7110_IncomingProfile },
	{ NK7110_MSG_RINGTONE,		NK7110_IncomingRingtone },
	{ NK7110_MSG_COMMSTATUS,	NK7110_IncomingCommstatus },
	{ 0, NULL }
};

gn_driver driver_nokia_7110 = {
	nk7110_incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"7110|6210|6250|7190",	/* Supported models */
		7,			/* Max RF Level */
		0,			/* Min RF Level */
		GN_RF_Percentage,	/* RF level units */
		7,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GN_BU_Percentage,	/* Battery level units */
		GN_DT_DateTime,		/* Have date/time support */
		GN_DT_TimeOnly,		/* Alarm supports time only */
		1,			/* Alarms available - FIXME */
		60, 96,			/* Startup logo size - 7110 is fixed at init */
		21, 78,			/* Op logo size */
		14, 72			/* Caller logo size */
	},
	NK7110_Functions,
	NULL
};

/* FIXME - a little macro would help here... */
static gn_error NK7110_Functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	if (!DRVINSTANCE(state) && op != GN_OP_Init) return GN_ERR_INTERNALERROR;

	switch (op) {
	case GN_OP_Init:
		return NK7110_Initialise(state);
	case GN_OP_Terminate:
		FREE(DRVINSTANCE(state));
		return pgen_terminate(data, state);
	case GN_OP_GetModel:
		return NK7110_GetModel(data, state);
	case GN_OP_GetRevision:
		return NK7110_GetRevision(data, state);
	case GN_OP_GetImei:
		return NK7110_GetIMEI(data, state);
	case GN_OP_Identify:
		return NK7110_Identify(data, state);
	case GN_OP_GetBatteryLevel:
		return NK7110_GetBatteryLevel(data, state);
	case GN_OP_GetRFLevel:
		return NK7110_GetRFLevel(data, state);
	case GN_OP_GetMemoryStatus:
		return NK7110_GetMemoryStatus(data, state);
	case GN_OP_GetBitmap:
		return NK7110_GetBitmap(data, state);
	case GN_OP_SetBitmap:
		return NK7110_SetBitmap(data, state);
	case GN_OP_ReadPhonebook:
		return NK7110_ReadPhonebook(data, state);
	case GN_OP_WritePhonebook:
		return NK7110_WritePhonebookLocation(data, state);
	case GN_OP_DeletePhonebook:
		return NK7110_DeletePhonebookLocation(data, state);
	case GN_OP_GetNetworkInfo:
		return NK7110_GetNetworkInfo(data, state);
	case GN_OP_GetSpeedDial:
		return NK7110_GetSpeedDial(data, state);
	case GN_OP_GetSMSCenter:
		return NK7110_GetSMSCenter(data, state);
	case GN_OP_GetDateTime:
		return NK7110_GetClock(NK7110_SUBCLO_GET_DATE, data, state);
	case GN_OP_SetDateTime:
		return NK7110_SetClock(data, state);
	case GN_OP_GetAlarm:
		return NK7110_GetClock(NK7110_SUBCLO_GET_ALARM, data, state);
	case GN_OP_SetAlarm:
		return NK7110_SetAlarm(data, state);
	case GN_OP_GetCalendarNote:
		return NK7110_GetCalendarNote(data, state);
	case GN_OP_WriteCalendarNote:
		return NK7110_WriteCalendarNote(data, state);
	case GN_OP_DeleteCalendarNote:
		return NK7110_DeleteCalendarNote(data, state);
	case GN_OP_GetSMS:
		dprintf("Getting SMS (validating)...\n");
		return NK7110_GetSMS(data, state);
	case GN_OP_GetSMSnoValidate:
		dprintf("Getting SMS (no validating)...\n");
		return NK7110_GetSMSnoValidate(data, state);
	case GN_OP_OnSMS:
		DRVINSTANCE(state)->on_sms = data->on_sms;
		DRVINSTANCE(state)->sms_callback_data = data->callback_data;
		/* Register notify when running for the first time */
		if (data->on_sms) {
			DRVINSTANCE(state)->new_sms = true;
			return GN_ERR_NONE; /* FIXME NK7110_GetIncomingSMS(data, state); */
		}
		break;
	case GN_OP_PollSMS:
		if (DRVINSTANCE(state)->new_sms) return GN_ERR_NONE; /* FIXME NK7110_GetIncomingSMS(data, state); */
		break;
	case GN_OP_SendSMS:
		return NK7110_SendSMS(data, state);
	case GN_OP_SaveSMS:
		return NK7110_SaveSMS(data, state);
	case GN_OP_DeleteSMSnoValidate:
		return NK7110_DeleteSMSnoValidate(data, state);
	case GN_OP_DeleteSMS:
		return NK7110_DeleteSMS(data, state);
	case GN_OP_GetSMSStatus:
		return NK7110_GetSMSStatus(data, state);
	case GN_OP_CallDivert:
		return pnok_call_divert(data, state);
	case GN_OP_NetMonitor:
		return pnok_netmonitor(data, state);
	case GN_OP_GetActiveCalls:
		return NK7110_GetActiveCalls(data, state);
	case GN_OP_MakeCall:
		return NK7110_MakeCall(data, state);
	case GN_OP_AnswerCall:
		return NK7110_AnswerCall(data, state);
	case GN_OP_CancelCall:
		return NK7110_CancelCall(data, state);
	case GN_OP_GetSecurityCode:
		return NK7110_GetSecurityCode(data, state);
	case GN_OP_GetSMSFolders:
		return NK7110_GetSMSFolders(data, state);
	case GN_OP_GetSMSFolderStatus:
		return NK7110_GetSMSFolderStatus(data, state);
	case GN_OP_NK7110_GetPictureList:
		return NK7110_GetPictureList(data, state);
	case GN_OP_DeleteWAPBookmark:
		return NK7110_DeleteWAPBookmark(data, state);
	case GN_OP_GetWAPBookmark:
		return NK7110_GetWAPBookmark(data, state);
	case GN_OP_WriteWAPBookmark:
		return NK7110_WriteWAPBookmark(data, state);
	case GN_OP_GetWAPSetting:
		return NK7110_GetWAPSetting(data, state);
	case GN_OP_ActivateWAPSetting:
		return NK7110_ActivateWAPSetting(data, state);
	case GN_OP_WriteWAPSetting:
		return NK7110_WriteWAPSetting(data, state);
	case GN_OP_PressPhoneKey:
		return NK7110_PressOrReleaseKey(data, state, true);
	case GN_OP_ReleasePhoneKey:
		return NK7110_PressOrReleaseKey(data, state, false);
	case GN_OP_GetRawRingtone:
		return NK7110_GetRawRingtone(data, state);
	case GN_OP_SetRawRingtone:
		return NK7110_SetRawRingtone(data, state);
	case GN_OP_GetRingtone:
		return NK7110_GetRingtone(data, state);
	case GN_OP_SetRingtone:
		return NK7110_SetRingtone(data, state);
	case GN_OP_GetRingtoneList:
		return NK7110_GetRingtoneList(data, state);
	case GN_OP_PlayTone:
		return pnok_play_tone(data, state);
	case GN_OP_GetLocksInfo:
		return pnok_get_locks_info(data, state);
	case GN_OP_GetProfile:
		return NK7110_GetProfile(data, state);
	case GN_OP_SetProfile:
		return NK7110_SetProfile(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_NONE;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error NK7110_Initialise(struct gn_statemachine *state)
{
	gn_data data;
	char model[GN_MODEL_MAX_LENGTH+1];
	gn_error err = GN_ERR_NONE;
	bool connected = false;
	unsigned int attempt = 0;

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_nokia_7110, sizeof(gn_driver));

	if (!(DRVINSTANCE(state) = calloc(1, sizeof(nk7110_driver_instance))))
		return GN_ERR_INTERNALERROR;

	dprintf("Connecting\n");
	while (!connected) {
		if (attempt > 2) break;
		switch (state->config.connection_type) {
		case GN_CT_Bluetooth:
		case GN_CT_DAU9P:
			attempt++;
		case GN_CT_DLR3P:
			if (attempt > 1) {
				attempt = 3;
				break;
			}
		case GN_CT_Serial:
		case GN_CT_TCP:
			err = fbus_initialise(attempt++, state);
			break;
		case GN_CT_Infrared:
		case GN_CT_Irda:
			err = phonet_initialise(state);
			attempt = 3;
			break;
		case GN_CT_M2BUS:
			err = m2bus_initialise(state);
			break;
		default:
			FREE(DRVINSTANCE(state));
			return GN_ERR_NOTSUPPORTED;
		}

		if (err != GN_ERR_NONE) {
			dprintf("Error in link initialisation: %d\n", err);
			continue;
		}

		sm_initialise(state);

		/* Now test the link and get the model */
		gn_data_clear(&data);
		data.model = model;
		err = state->driver.functions(GN_OP_GetModel, &data, state);
		if (err == GN_ERR_NONE) {
			connected = true;
		} else {
			/* ignore return value from pgen_terminate(), will use previous error code instead */
			pgen_terminate(&data, state);
		}
	}
	if (!connected) {
		FREE(DRVINSTANCE(state));
		return err;
	}

	/* Check for 7110 and alter the startup logo size */
	if (strcmp(model, "NSE-5") == 0) {
		state->driver.phone.startup_logo_height = 65;
		dprintf("7110 detected - startup logo height set to 65\n");
		DRVINSTANCE(state)->userdef_location = 0x75;
	} else {
		DRVINSTANCE(state)->userdef_location = 0x8a;
	}

	pnok_extended_cmds_enable(0x01, &data, state);

	return GN_ERR_NONE;
}

/*****************************/
/********* IDENTIFY **********/
/*****************************/
static gn_error NK7110_IncomingIdentify(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	case 0x02:
		if (data->imei) {
			int n;
			unsigned char *s = strchr(message + 4, '\n');

			if (s) n = s - message - 3;
			else n = GN_IMEI_MAX_LENGTH;
			snprintf(data->imei, GNOKII_MIN(n, GN_IMEI_MAX_LENGTH), "%s", message + 4);
			dprintf("Received imei %s\n", data->imei);
		}
		break;
	case 0x04:
		if (data->model) {
			int n;
			unsigned char *s = strchr(message + 22, '\n');

			n = s ? s - message - 21 : GN_MODEL_MAX_LENGTH;
			snprintf(data->model, GNOKII_MIN(n, GN_MODEL_MAX_LENGTH), "%s", message + 22);
			dprintf("Received model %s\n", data->model);
		}
		if (data->revision) {
			int n;
			unsigned char *s = strchr(message + 7, '\n');

			n = s ? s - message - 6 : GN_REVISION_MAX_LENGTH;
			snprintf(data->revision, GNOKII_MIN(n, GN_REVISION_MAX_LENGTH), "%s", message + 7);
			dprintf("Received revision %s\n", data->revision);
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x1b (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

/* Just as an example.... */
/* But note that both requests are the same type which isn't very 'proper' */
static gn_error NK7110_Identify(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};

	dprintf("Identifying...\n");
	pnok_manufacturer_get(data->manufacturer);
	if (sm_message_send(4, NK7110_MSG_IDENTITY, req, state)) return GN_ERR_NOTREADY;
	if (sm_message_send(6, NK7110_MSG_IDENTITY, req2, state)) return GN_ERR_NOTREADY;
	sm_wait_for(NK7110_MSG_IDENTITY, data, state);
	sm_block(NK7110_MSG_IDENTITY, data, state); /* waits for all requests - returns req2 error */
	sm_error_get(NK7110_MSG_IDENTITY, state);

	/* Check that we are back at state Initialised */
	if (gn_sm_loop(0, state) != GN_SM_Initialised) return GN_ERR_UNKNOWN;
	return GN_ERR_NONE;
}

static gn_error NK7110_GetModel(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};

	dprintf("Getting model...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_IDENTITY, 6);
}

static gn_error NK7110_GetRevision(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};

	dprintf("Getting revision...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_IDENTITY, 6);
}

static gn_error NK7110_GetIMEI(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};

	dprintf("Getting imei...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_IDENTITY, 4);
}

/*****************************/
/********** BATTERY **********/
/*****************************/
static gn_error NK7110_IncomingBattLevel(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	case 0x03:
		if (data->battery_level) {
			*(data->battery_unit) = GN_BU_Percentage;
			*(data->battery_level) = message[5];
			dprintf("Battery level %f\n",*(data->battery_level));
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GN_ERR_UNKNOWN;
	}
	return GN_ERR_NONE;
}

static gn_error NK7110_GetBatteryLevel(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02};

	dprintf("Getting battery level...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_BATTERY, 4);
}

/*****************************/
/********** NETWORK **********/
/*****************************/
static gn_error NK7110_IncomingNetwork(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char *blockstart;
	gn_error error = GN_ERR_NONE;
	int i;

	switch (message[3]) {
	case 0x71:
		blockstart = message + 6;
		for (i = 0; i < message[4]; i++) {
			switch (blockstart[0]) {
			case 0x01:  /* Operator details */
				/* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
				if (data->network_info) {
					/* Is this correct? */
					data->network_info->cell_id[0] = blockstart[4];
					data->network_info->cell_id[1] = blockstart[5];
					data->network_info->LAC[0] = blockstart[6];
					data->network_info->LAC[1] = blockstart[7];
					data->network_info->network_code[0] = '0' + (blockstart[8] & 0x0f);
					data->network_info->network_code[1] = '0' + (blockstart[8] >> 4);
					data->network_info->network_code[2] = '0' + (blockstart[9] & 0x0f);
					data->network_info->network_code[3] = ' ';
					data->network_info->network_code[4] = '0' + (blockstart[10] & 0x0f);
					data->network_info->network_code[5] = '0' + (blockstart[10] >> 4);
					data->network_info->network_code[6] = 0;
				}
				if (data->bitmap) {
					data->bitmap->netcode[0] = '0' + (blockstart[8] & 0x0f);
					data->bitmap->netcode[1] = '0' + (blockstart[8] >> 4);
					data->bitmap->netcode[2] = '0' + (blockstart[9] & 0x0f);
					data->bitmap->netcode[3] = ' ';
					data->bitmap->netcode[4] = '0' + (blockstart[10] & 0x0f);
					data->bitmap->netcode[5] = '0' + (blockstart[10] >> 4);
					data->bitmap->netcode[6] = 0;
					dprintf("Operator %s\n", data->bitmap->netcode);
				}
				break;
			case 0x04: /* Logo */
				if (data->bitmap) {
					dprintf("Op logo received ok\n");
					data->bitmap->type = GN_BMP_OperatorLogo;
					data->bitmap->size = blockstart[5]; /* Probably + [4]<<8 */
					data->bitmap->height = blockstart[3];
					data->bitmap->width = blockstart[2];
					memcpy(data->bitmap->bitmap, blockstart + 8, data->bitmap->size);
					dprintf("Logo (%dx%d)\n", data->bitmap->height, data->bitmap->width);
				}
				break;
			default:
				dprintf("Unknown operator block %d\n", blockstart[0]);
				break;
			}
			blockstart += blockstart[1];
		}
		break;
	case 0x82:
		if (data->rf_level) {
			*(data->rf_unit) = GN_RF_Percentage;
			*(data->rf_level) = message[4];
			dprintf("RF level %f\n", *(data->rf_level));
		}
		break;
	case 0xa4:
		dprintf("Op Logo Set OK\n");
		break;
	case 0xa5:
		dprintf("Op Logo Set failed\n");
		/* Perhaps this should be WRONGFORMAT */
		error = GN_ERR_FAILED;
		break;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		error = GN_ERR_UNHANDLEDFRAME;
	}
	return error;
}


static gn_error NK7110_GetRFLevel(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x81};

	dprintf("Getting rf level...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_NETSTATUS, 4);
}

static gn_error NK7110_GetNetworkInfo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x70};

	dprintf("Getting Network Info...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_NETSTATUS, 4);
}

static gn_error SetOperatorBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[500] = { FBUS_FRAME_HEADER, 0xa3, 0x01,
				   0x00,              /* logo enabled */
				   0x00, 0xf0, 0x00,  /* network code (000 00) */
				   0x00 ,0x04,
				   0x08,              /* length of rest */
				   0x00, 0x00,        /* Bitmap width / height */
				   0x00,
				   0x00,              /* Bitmap size */
				   0x00, 0x00 };
	int count = 18;

	if ((data->bitmap->width != state->driver.phone.operator_logo_width) ||
	    (data->bitmap->height != state->driver.phone.operator_logo_height)) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",
			state->driver.phone.operator_logo_height,
			state->driver.phone.operator_logo_width,
			data->bitmap->height, data->bitmap->width);
		return GN_ERR_INVALIDSIZE;
	}

	if (strcmp(data->bitmap->netcode, "000 00")) {  /* set logo */
		req[5] = 0x01;      /* Logo enabled */
		req[6] = ((data->bitmap->netcode[1] & 0x0f) << 4) | (data->bitmap->netcode[0] & 0x0f);
		req[7] = 0xf0 | (data->bitmap->netcode[2] & 0x0f);
		req[8] = ((data->bitmap->netcode[5] & 0x0f) << 4) | (data->bitmap->netcode[4] & 0x0f);
		req[11] = 8 + data->bitmap->size;
		req[12] = data->bitmap->width;
		req[13] = data->bitmap->height;
		req[15] = data->bitmap->size;
		memcpy(req + count, data->bitmap->bitmap, data->bitmap->size);
		count += data->bitmap->size;
	}
	dprintf("Setting op logo...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_NETSTATUS, count);
}

static gn_error GetOperatorBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x70};

	dprintf("Getting op logo...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_NETSTATUS, 4);
}


/*****************************/
/********* PHONEBOOK *********/
/*****************************/
static gn_error NK7110_IncomingPhonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	nk7110_driver_instance *drvinst = DRVINSTANCE(state);
	unsigned char *blockstart;
	unsigned char blocks;
	unsigned char subblockcount;
	int memtype, location, memtype_req;

	switch (message[3]) {
	case 0x04:  /* Get status response */
		if (data->memory_status) {
			if (message[5] != 0xff) {
				data->memory_status->used = (message[16] << 8) + message[17];
				data->memory_status->free = ((message[14] << 8) + message[15]) - data->memory_status->used;
				dprintf("Memory status - location = %d\n", (message[8] << 8) + message[9]);
			} else {
				dprintf("Unknown error getting mem status\n");
				return GN_ERR_NOTIMPLEMENTED;
			}
		}
		break;
	case 0x08:  /* Read Memory response */
		memtype = message[11];
		location = (message[12] << 8) + message[13];
		if (data->phonebook_entry) {
			data->phonebook_entry->empty = true;
			data->phonebook_entry->caller_group = 5; /* no group */
			data->phonebook_entry->name[0] = '\0';
			data->phonebook_entry->number[0] = '\0';
			data->phonebook_entry->subentries_count = 0;
			data->phonebook_entry->date.year = 0;
			data->phonebook_entry->date.month = 0;
			data->phonebook_entry->date.day = 0;
			data->phonebook_entry->date.hour = 0;
			data->phonebook_entry->date.minute = 0;
			data->phonebook_entry->date.second = 0;
		}
		if (data->bitmap) {
			data->bitmap->text[0] = '\0';
		}
		if (message[6] == 0x0f) { /* not found */
			switch (message[10]) {
			case 0x30:
				if (data->phonebook_entry)
					memtype_req = data->phonebook_entry->memory_type;
				else
					memtype_req = GN_MT_XX;
				/*
				 * this message has two meanings: "invalid
				 * location" and "memory is empty"
				 */
				switch (memtype_req) {
				case GN_MT_SM:
				case GN_MT_ME:
					return GN_ERR_EMPTYLOCATION;
				default:
					break;
				}
				return GN_ERR_INVALIDMEMORYTYPE;
			case 0x33:
				return GN_ERR_EMPTYLOCATION;
			case 0x34:
				return GN_ERR_INVALIDLOCATION;
			default:
				return GN_ERR_NOTIMPLEMENTED;
			}
		}
		if (drvinst->ll_memtype != memtype || drvinst->ll_location != location) {
			dprintf("skipping entry: ll_memtype: %d, memtype: %d, ll_location: %d, location: %d\n",
				drvinst->ll_memtype, memtype, drvinst->ll_location, location);
			return GN_ERR_UNSOLICITED;
		}
		dprintf("Received phonebook info\n");
		blocks     = message[17];
		blockstart = message + 18;
		subblockcount = 0;
		return phonebook_decode(message + 18, length - 17, data, blocks, message[11], 8);
	case 0x0c:
		if (message[6] == 0x0f) {
			switch (message[10]) {
			case 0x34: return GN_ERR_FAILED; /* invalid location ? */
			case 0x3d: return GN_ERR_FAILED;
			case 0x3e: return GN_ERR_FAILED;
			default:   return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;
	case 0x10:
		dprintf("Entry successfully deleted!\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x03 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK7110_GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x00, 0x00};

	dprintf("Getting memory status...\n");
	req[5] = get_memory_type(data->memory_status->memory_type);
	SEND_MESSAGE_BLOCK(NK7110_MSG_PHONEBOOK, 6);
}

static gn_error GetCallerBitmap(gn_data *data, struct gn_statemachine *state)
{
	nk7110_driver_instance *drvinst = DRVINSTANCE(state);
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
				  0x00, 0x10 , /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00};

	req[11] = GNOKII_MIN(data->bitmap->number + 1, GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER);
	drvinst->ll_memtype = 0x10;
	drvinst->ll_location = req[11];
	dprintf("Getting caller(%d) logo...\n", req[11]);
	SEND_MESSAGE_BLOCK(NK7110_MSG_PHONEBOOK, 14);
}

static unsigned char PackBlock(u8 id, u8 size, u8 no, u8 *buf, u8 *block)
{
	*(block++) = id;
	*(block++) = 0;
	*(block++) = 0;
	*(block++) = size + 6;
	*(block++) = no;
	memcpy(block, buf, size);
	block += size;
	*(block++) = 0;
	return (size + 6);
}

static gn_error SetCallerBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
				  0x00, 0x10,  /* memory type */
				  0x00, 0x00,  /* location */
				  0x00, 0x00, 0x00};
	char string[500];
	int block, i;
	unsigned int count = 18;

	if ((data->bitmap->width != state->driver.phone.caller_logo_width) ||
	    (data->bitmap->height != state->driver.phone.caller_logo_height)) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",state->driver.phone.caller_logo_height, state->driver.phone.caller_logo_width, data->bitmap->height, data->bitmap->width);
	    return GN_ERR_INVALIDSIZE;
	}

	req[13] = data->bitmap->number + 1;
	dprintf("Setting caller(%d) bitmap...\n",data->bitmap->number);
	block = 1;
	/* Name */
	i = strlen(data->bitmap->text);
	i = char_unicode_encode((string + 1), data->bitmap->text, i);
	string[0] = i;
	count += PackBlock(0x07, i + 1, block++, string, req + count);
	/* Ringtone */
	string[0] = data->bitmap->ringtone;
	string[1] = 0;
	count += PackBlock(0x0c, 2, block++, string, req + count);
	/* Number */
	string[0] = data->bitmap->number+1;
	string[1] = 0;
	count += PackBlock(0x1e, 2, block++, string, req + count);
	/* Logo on/off - assume on for now */
	string[0] = 1;
	string[1] = 0;
	count += PackBlock(0x1c, 2, block++, string, req + count);
	/* Logo */
	string[0] = data->bitmap->width;
	string[1] = data->bitmap->height;
	string[2] = 0;
	string[3] = 0;
	string[4] = 0x7e; /* Size */
	memcpy(string + 5, data->bitmap->bitmap, data->bitmap->size);
	count += PackBlock(0x1b, data->bitmap->size + 5, block++, string, req + count);
	req[17] = block - 1;

	SEND_MESSAGE_BLOCK(NK7110_MSG_PHONEBOOK, count);
}

static gn_error NK7110_DeletePhonebookLocation(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0f, 0x00, 0x01, 0x04, 0x00, 0x00, 0x0c, 0x01, 0xff,
				  0x00, 0x00,	/* location */
				  0x00,		/* memory type */
				  0x00, 0x00, 0x00};
	gn_phonebook_entry *entry;

	if (data->phonebook_entry) entry = data->phonebook_entry;
	else return GN_ERR_TRYAGAIN;
	/* Two octets for the memory location */
	req[12] = (entry->location >> 8);
	req[13] = entry->location & 0xff;
	req[14] = get_memory_type(entry->memory_type);

	SEND_MESSAGE_BLOCK(NK7110_MSG_PHONEBOOK, 18);
}

static gn_error NK7110_WritePhonebookLocation(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[500] = {FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0c,
				  0x00, 0x00,	/* memory type */
				  0x00, 0x00,	/* location */
				  0x00, 0x00, 0x00, 0x00};

	char string[500];
	int block, i, j, defaultn;
	unsigned int count = 18;
	gn_phonebook_entry *entry;

	if (data->phonebook_entry) entry = data->phonebook_entry;
	else return GN_ERR_TRYAGAIN;

	req[11] = get_memory_type(entry->memory_type);
	/* Two octets for the memory location */
	req[12] = (entry->location >> 8);
	req[13] = entry->location & 0xff;
	block = 1;
	if (!entry->empty) {
		/* Name */
		i = strlen(entry->name);
		i = char_unicode_encode((string + 1), entry->name, i);
		/* Length of the string + length field + terminating 0 */
		string[0] = i + 2;
		count += PackBlock(0x07, i + 2, block++, string, req + count);
		/* Group */
		string[0] = entry->caller_group + 1;
		string[1] = 0;
		count += PackBlock(0x1e, 2, block++, string, req + count);
		/* We don't require the application to fill in any subentry.
		 * if it is not filled in, let's take just one number we have.
		 */
		if (!entry->subentries_count) {
			string[0] = GN_PHONEBOOK_ENTRY_Number;
			string[1] = string[2] = string[3] = 0;
			j = strlen(entry->number);
			j = char_unicode_encode((string + 5), entry->number, j);
			string[j + 1] = 0;
			string[4] = j;
			count += PackBlock(0x0b, j + 6, block++, string, req + count);
		} else {
			/* Default Number */
			defaultn = 999;
			for (i = 0; i < entry->subentries_count; i++)
				if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number)
					if (!strcmp(entry->number, entry->subentries[i].data.number))
						defaultn = i;
			if (defaultn < i) {
				string[0] = entry->subentries[defaultn].number_type;
				string[1] = string[2] = string[3] = 0;
				j = strlen(entry->subentries[defaultn].data.number);
				j = char_unicode_encode((string + 5), entry->subentries[defaultn].data.number, j);
				string[j + 1] = 0;
				string[4] = j;
				count += PackBlock(0x0b, j + 6, block++, string, req + count);
			}
			/* Rest of the numbers */
			for (i = 0; i < entry->subentries_count; i++)
				if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number) {
					if (i != defaultn) {
						string[0] = entry->subentries[i].number_type;
						string[1] = string[2] = string[3] = 0;
						j = strlen(entry->subentries[i].data.number);
						j = char_unicode_encode((string + 5), entry->subentries[i].data.number, j);
						string[j + 1] = 0;
						string[4] = j;
						count += PackBlock(0x0b, j + 6, block++, string, req + count);
					}
				} else {
					j = strlen(entry->subentries[i].data.number);
					j = char_unicode_encode((string + 1), entry->subentries[i].data.number, j);
					string[j + 1] = 0;
					string[0] = j;
					count += PackBlock(entry->subentries[i].entry_type, j + 2, block++, string, req + count);
				}
		}
		req[17] = block - 1;
		dprintf("Writing phonebook entry %s...\n", entry->name);
	} else {
		return NK7110_DeletePhonebookLocation(data, state);
	}
	SEND_MESSAGE_BLOCK(NK7110_MSG_PHONEBOOK, count);
}

static gn_error NK7110_ReadPhonebookLL(gn_data *data, struct gn_statemachine *state)
{
	nk7110_driver_instance *drvinst = DRVINSTANCE(state);
	unsigned char req[2000] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
				   0x00, 0x00, /* memory type; was: 0x02, 0x05 */
				   0x00, 0x00, /* location */
				   0x00, 0x00};

	dprintf("Reading phonebook location (%d)\n", drvinst->ll_location);

	req[9] = drvinst->ll_memtype;
	req[10] = drvinst->ll_location >> 8;
	req[11] = drvinst->ll_location & 0xff;

	SEND_MESSAGE_BLOCK(NK7110_MSG_PHONEBOOK, 14);
}

static gn_error NK7110_ReadPhonebook(gn_data *data, struct gn_statemachine *state)
{
	nk7110_driver_instance *drvinst = DRVINSTANCE(state);

	drvinst->ll_memtype = get_memory_type(data->phonebook_entry->memory_type);
	drvinst->ll_location = data->phonebook_entry->location;

	return NK7110_ReadPhonebookLL(data, state);
}

static gn_error NK7110_GetSpeedDial(gn_data *data, struct gn_statemachine *state)
{
	nk7110_driver_instance *drvinst = DRVINSTANCE(state);

	drvinst->ll_memtype = NK7110_MEMORY_SPEEDDIALS;
	drvinst->ll_location = data->speed_dial->number;

	return NK7110_ReadPhonebookLL(data, state);
}

/*****************************/
/********** FOLDER ***********/
/*****************************/
static inline unsigned int get_data(gn_sms_message_type t, unsigned int a,
				    unsigned int b, unsigned int c,
				    unsigned int d)
{
	switch (t) {
	case GN_SMS_MT_Deliver:        return a;
	case GN_SMS_MT_Submit:         return b;
	case GN_SMS_MT_StatusReport:   return c;
	case GN_SMS_MT_Picture:        return d;
	default:                       return 0;
	}
}

/**
 * NK7110_IncomingFolder - handle SMS and folder related messages (0x14 type)
 * @messagetype: message type, 0x14 (NK7110_MSG_FOLDER)
 * @message: a pointer to the raw message returned by the statemachine
 * @length: length of the received message
 * @data: gn_data structure where the handler saves parsed data
 *
 * This function parses all incoming events of the 0x14 type. This type is
 * known as the answer to the SMS and folder relates requests. Subtypes
 * handled by this function contain:
 *  o GetSMS (0x08, 0x09)
 *  o DeleteSMS (0x0b, 0x0c)
 *  o SMSStatus (0x37)
 *  o FolderStatus (0x6c)
 *  o FolderList (0x7b)
 *  o PictureMessagesList (0x97)
 * Unknown but reported errors:
 *  o 0xc9 -- unknown command?
 *  o 0xca -- phone not ready?
 */
static gn_error NK7110_IncomingFolder(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned int i, j, T, offset = 47;
	int nextfolder = 0x10;

	/* Message subtype */
	switch (message[3]) {
	/* save sms succeeded */
	case 0x05:
		dprintf("Message stored in folder %i at location %i\n", message[4], (message[5] << 8) | message[6]);
		if (data->raw_sms) data->raw_sms->number = (message[5] << 8) | message[6];
		break;
	/* save sms failed */
	case 0x06:
		dprintf("SMS saving failed:\n");
		switch (message[4]) {
		case 0x02:
			dprintf("\tAll locations busy.\n");
			return GN_ERR_MEMORYFULL;
		case 0x03:
			dprintf("\tInvalid location!\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("\tUnknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}
	case NK7110_SUBSMS_READ_OK: /* GetSMS OK, 0x08 */
		dprintf("Trying to get message # %i from the folder # %i\n", (message[6] << 8) | message[7], message[5]);
		if (!data->raw_sms)
			return GN_ERR_INTERNALERROR;

		memset(data->raw_sms, 0, sizeof(gn_sms_raw));
		T = data->raw_sms->type    = message[8];
		data->raw_sms->number      = (message[6] << 8) | message[7];
		data->raw_sms->memory_type = message[5];
		data->raw_sms->status      = message[4];

		data->raw_sms->more_messages       = 0;
		data->raw_sms->reply_via_same_smsc = message[21];
		data->raw_sms->reject_duplicates   = 0;
		data->raw_sms->report              = 0;

		data->raw_sms->reference     = 0;
		data->raw_sms->pid           = 0;
		data->raw_sms->report_status = 0;

		if (T != GN_SMS_MT_Submit) {
			memcpy(data->raw_sms->smsc_time, message + get_data(T, 37, 38, 36, 34), 7);
			memcpy(data->raw_sms->message_center, message + 9, 12);
			memcpy(data->raw_sms->remote_number, message + get_data(T, 25, 26, 24, 22), 12);
		}
		if (T == GN_SMS_MT_StatusReport)
			memcpy(data->raw_sms->time, message + 43, 7);

		data->raw_sms->dcs = message[23];
		/* This is ugly hack. But the picture message format in 6210
		 * is the real pain in the ass. */
		if (T == GN_SMS_MT_Picture && (message[47] == 0x48) && (message[48] == 0x1c))
			/* 47 (User Data offset) + 256 (72 * 28 / 8 + 4) = 303 */
			offset = 303;
		data->raw_sms->length = message[get_data(T, 24, 25, 0, offset)];
		if (T == GN_SMS_MT_Picture)
			data->raw_sms->length += 256;
		data->raw_sms->udh_indicator = message[get_data(T, 21, 23, 0, 21)];
		memcpy(data->raw_sms->user_data, message + get_data(T, 44, 45, 0, 47), data->raw_sms->length);

		data->raw_sms->user_data_length = length - get_data(T, 44, 45, 0, 47);

		data->raw_sms->validity_indicator = 0;
		memset(data->raw_sms->validity, 0, sizeof(data->raw_sms->validity));
		break;
	case NK7110_SUBSMS_READ_FAIL: /* GetSMS FAIL, 0x09 */
		dprintf("SMS reading failed:\n");
		switch (message[4]) {
		case 0x02:
			dprintf("\tInvalid location!\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x07:
			dprintf("\tEmpty SMS location.\n");
			return GN_ERR_EMPTYLOCATION;
		default:
			dprintf("\tUnknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}

	case NK7110_SUBSMS_DELETE_OK: /* DeleteSMS OK, 0x0b */
		dprintf("SMS deleted\n");
		break;

	case NK7110_SUBSMS_DELETE_FAIL: /* DeleteSMS FAIL, 0x0c */
		switch (message[4]) {
		case 0x02:
			dprintf("Invalid location\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("Unknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}

	case NK7110_SUBSMS_SMS_STATUS_OK: /* SMS status received, 0x37 */
		dprintf("SMS Status received\n");
		/* FIXME: Don't count messages in fixed locations together with other */
		data->sms_status->number = ((message[10] << 8) | message[11]) +
					  ((message[14] << 8) | message[15]) +
					  (data->sms_folder->number);
		data->sms_status->unread = ((message[12] << 8) | message[13]) +
					  ((message[16] << 8) | message[17]);
		break;

	case NK7110_SUBSMS_FOLDER_STATUS_OK: /* Folder status OK, 0x6C */
		dprintf("Message: SMS Folder status received\n");
		if (!data->sms_folder) return GN_ERR_INTERNALERROR;

		data->sms_folder->sms_data = 0;
		memset(data->sms_folder->locations, 0, sizeof(data->sms_folder->locations));
		data->sms_folder->number = (message[4] << 8) | message[5];

		dprintf("Message: Number of Entries: %i\n" , data->sms_folder->number);
		dprintf("Message: IDs of Entries : ");
		for (i = 0; i < data->sms_folder->number; i++) {
			data->sms_folder->locations[i] = (message[(i * 2) + 6] << 8) | message[(i * 2) + 7];
			dprintf("%d, ", data->sms_folder->locations[i]);
		}
		dprintf("\n");
		break;

	case NK7110_SUBSMS_FOLDER_LIST_OK: /* Folder list OK, 0x7B */
		if (!data->sms_folder_list)
			return GN_ERR_INTERNALERROR;
		i = 5;
		memset(data->sms_folder_list, 0, sizeof(gn_sms_folder_list));
		dprintf("Message: %d SMS Folders received:\n", message[4]);

		snprintf(data->sms_folder_list->folder[1].name,
			sizeof(data->sms_folder_list->folder[1].name),
			"%s", "               ");
		data->sms_folder_list->number = message[4];

		for (j = 0; j < message[4]; j++) {
			int len;
			snprintf(data->sms_folder_list->folder[j].name,
				sizeof(data->sms_folder_list->folder[j].name),
				"%s", "               ");
			data->sms_folder_list->folder_id[j] = get_gn_memory_type(message[i]);
			data->sms_folder_list->folder[j].folder_id = data->sms_folder_list->folder_id[j];
			dprintf("Folder Index: %d", data->sms_folder_list->folder_id[j]);
			i += 2;
			dprintf("\tFolder name: ");
			len = 0;
			/* search for the next folder's index number, i.e. length of the folder name */
			while (i + 1 < length && message[i + 1] != nextfolder) {
				i += 2;
				len += 2;
			}
			/* see Docs/protocol/nk7110.txt */
			nextfolder += 0x08;
			if (nextfolder == 0x28) nextfolder++;
			i -= len + 1;
			char_unicode_decode(data->sms_folder_list->folder[j].name, message + i, len);
			dprintf("%s\n", data->sms_folder_list->folder[j].name);
			i += len + 2;
		}
		break;

	case NK7110_SUBSMS_PICTURE_LIST_OK: /* Picture messages list OK, 0x97 */
		dprintf("Getting list of SMS pictures...\n");
		break;

	/* Some errors */
	case 0xc9:
		dprintf("Unknown command???\n");
		return GN_ERR_UNHANDLEDFRAME;

	case 0xca:
		dprintf("Phone not ready???\n");
		return GN_ERR_UNHANDLEDFRAME;

	default:
		dprintf("Message: Unknown message of type 14 : %02x  length: %d\n", message[3], length);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK7110_GetSMSnoValidate(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07,
				0x08, /* FolderID */
				0x00,
				0x01, /* Location */
				0x01, 0x65, 0x01};

	req[4] = get_memory_type(data->raw_sms->memory_type);
	req[5] = (data->raw_sms->number & 0xff00) >> 8;
	req[6] = data->raw_sms->number & 0x00ff;
	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, 10);
}

static gn_error ValidateSMS(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (data->raw_sms->number < 1)
		return GN_ERR_INVALIDLOCATION;

	/* Handle memory_type = 0 explicitely, because sms_folder->folder_id = 0 by default */
	if (data->raw_sms->memory_type == 0) return GN_ERR_INVALIDMEMORYTYPE;

	/* see if the message we want is from the last read folder, i.e. */
	/* we don't have to get folder status again */
	if ((!data->sms_folder) || (!data->sms_folder_list))
		return GN_ERR_INTERNALERROR;

	if (data->raw_sms->memory_type != data->sms_folder->folder_id) {
		if ((error = NK7110_GetSMSFolders(data, state)) != GN_ERR_NONE) return error;
		if ((data->raw_sms->memory_type >
		     data->sms_folder_list->folder_id[data->sms_folder_list->number - 1]) ||
		    (data->raw_sms->memory_type < 12))
			return GN_ERR_INVALIDMEMORYTYPE;
		data->sms_folder->folder_id = data->raw_sms->memory_type;
		if ((error = NK7110_GetSMSFolderStatus(data, state)) != GN_ERR_NONE) return error;
	}

	if (data->sms_folder->number + 2 < data->raw_sms->number) {
		if (data->raw_sms->number > GN_SMS_MESSAGE_MAX_NUMBER)
			return GN_ERR_INVALIDLOCATION;
		else
			return GN_ERR_EMPTYLOCATION;
	} else {
		data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];
	}
	return GN_ERR_NONE;
}


static gn_error NK7110_GetSMS(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	error = ValidateSMS(data, state);
	if (error != GN_ERR_NONE) return error;

	return NK7110_GetSMSnoValidate(data, state);
}


static gn_error NK7110_DeleteSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0a, 0x00, 0x00, 0x00, 0x01};
	gn_error error;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	dprintf("Removing SMS %d\n", data->raw_sms->number);

	error = ValidateSMS(data, state);
	if (error != GN_ERR_NONE) return error;

	req[4] = get_memory_type(data->raw_sms->memory_type);
	req[5] = (data->raw_sms->number & 0xff00) >> 8;
	req[6] = data->raw_sms->number & 0x00ff;
	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, 8);
}

static gn_error NK7110_DeleteSMSnoValidate(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0a, 0x00, 0x00, 0x00, 0x01};

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	dprintf("Removing SMS (no validate) %d\n", data->raw_sms->number);

	req[4] = get_memory_type(data->raw_sms->memory_type);
	req[5] = (data->raw_sms->number & 0xff00) >> 8;
	req[6] = data->raw_sms->number & 0x00ff;
	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, 8);
}

static gn_error NK7110_GetPictureList(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x96,
				0x09, /* location */
				0x0f, 0x07};

	dprintf("Getting picture messages list...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, 7);
}

static gn_error NK7110_GetSMSFolders(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x7a, 0x00, 0x00};

	dprintf("Getting SMS Folders...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, 6);
}

static gn_error NK7110_GetSMSStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x36, 0x64};
	gn_sms_folder status_fld, *old_fld;
	gn_error error;

	dprintf("Getting SMS Status...\n");

	/* Nokia 6210 and family does not show not "fixed" messages from the
	 * Templates folder, ie. when you save a message to the Templates folder,
	 * SMSStatus does not change! Workaround: get Templates folder status, which
	 * does show these messages.
	 */

	old_fld = data->sms_folder;

	data->sms_folder = &status_fld;
	data->sms_folder->folder_id = GN_MT_TE;

	error = NK7110_GetSMSFolderStatus(data, state);
	if (error) goto out;

	error = sm_message_send(6, NK7110_MSG_FOLDER, req, state);
	if (error) goto out;

	error = sm_block(NK7110_MSG_FOLDER, data, state);
 out:
	data->sms_folder = old_fld;
	return error;
}

static gn_error NK7110_GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6B,
			       0x08, /* Folder ID */
			       0x0F, 0x01};
	gn_error error;
	gn_sms_folder read;
	int i;

	req[4] = get_memory_type(data->sms_folder->folder_id);

	dprintf("Getting SMS Folder (%i) status ...\n", req[4]);

	if (req[4] == NK7110_MEMORY_IN) { /* special case INBOX */

		dprintf("Special case INBOX in GetSMSFolderStatus!\n");

		if (sm_message_send(7, NK7110_MSG_FOLDER, req, state)) return GN_ERR_NOTREADY;
		error = sm_block(NK7110_MSG_FOLDER, data, state);
		if (error) return error;

		memcpy(&read, data->sms_folder, sizeof(gn_sms_folder));

		req[4] = 0xf8; /* unread messages from INBOX */
		if (sm_message_send(7, NK7110_MSG_FOLDER, req, state)) return GN_ERR_NOTREADY;
		error = sm_block(NK7110_MSG_FOLDER, data, state);

		for (i = 0; i < read.number; i++) {
			data->sms_folder->locations[data->sms_folder->number] = read.locations[i];
			data->sms_folder->number++;
		}
		return GN_ERR_NONE;
	}

	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, 7);
}

static gn_error NK7110_SaveSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = { FBUS_FRAME_HEADER, 0x04,
				   0x07,		/* sms state */
				   0x10,		/* folder */
				   0x00,0x00,		/* location */
				   0x00 };		/* type */
	int len;

	dprintf("Saving sms\n");

	switch (data->raw_sms->type) {
	case GN_SMS_MT_Submit:
		if (data->raw_sms->status == GN_SMS_Sent)
			req[4] = 0x05;
		else
			req[4] = 0x07;
		req[8] = 0x02;
		break;
	case GN_SMS_MT_Deliver:
		if (data->raw_sms->status == GN_SMS_Sent)
			req[4] = 0x01;
		else
			req[4] = 0x03;
		req[8] = 0x00;
		break;
	default:
		req[4] = 0x07;
		req[8] = 0x00;
		break;
	}

	if ((data->raw_sms->memory_type != GN_MT_ME) && (data->raw_sms->memory_type != GN_MT_SM))
		req[5] = get_memory_type(data->raw_sms->memory_type);
	else
		req[5] = NK7110_MEMORY_OU;

	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;

	if (req[5] == NK7110_MEMORY_TE) return GN_ERR_INVALIDLOCATION;

	len = pnok_fbus_sms_encode(req + 9, data, state);
	len += 9;

	SEND_MESSAGE_BLOCK(NK7110_MSG_FOLDER, len);
}



/*****************************/
/***********  S M S  *********/
/*****************************/
static gn_error NK7110_IncomingSMS(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error e = GN_ERR_NONE;

	if (!data) return GN_ERR_INTERNALERROR;

	switch (message[3]) {
	case NK7110_SUBSMS_SMSC_RCVD: /* 0x34 */
		dprintf("SMSC Received\n");
		data->message_center->id = message[4];
		data->message_center->format = message[6];
		data->message_center->validity = message[8];  /* due to changes in format */

		snprintf(data->message_center->name, sizeof(data->message_center->name), "%s", message + 33);
		data->message_center->default_name = -1;

		if (message[9] % 2) message[9]++;
		message[9] = message[9] / 2 + 1;
		dprintf("%d\n", message[9]);
		snprintf(data->message_center->recipient.number,
			 sizeof(data->message_center->recipient.number),
			 "%s", char_bcd_number_get(message + 9));
		data->message_center->recipient.type = message[10];
		snprintf(data->message_center->smsc.number,
			 sizeof(data->message_center->smsc.number),
			 "%s", char_bcd_number_get(message + 21));
		data->message_center->smsc.type = message[22];
		/* Set a default SMSC name if none was received */
		if (!data->message_center->name[0]) {
			snprintf(data->message_center->name, sizeof(data->message_center->name), _("Set %d"), data->message_center->id);
			data->message_center->default_name = data->message_center->id;
		}

		break;

	case NK7110_SUBSMS_SEND_OK: /* 0x02 */
		dprintf("SMS sent\n");
		e = GN_ERR_NONE;
		break;

	case NK7110_SUBSMS_SEND_FAIL: /* 0x03 */
		dprintf("SMS sending failed\n");
		e = GN_ERR_FAILED;
		break;

	case 0x0e:
		dprintf("Ack for request on Incoming SMS\n");
		break;

	case 0x11:
		dprintf("SMS received\n");
		/* We got here the whole SMS */
		DRVINSTANCE(state)->new_sms = true;
		break;

	case NK7110_SUBSMS_SMS_RCVD: /* 0x10 */
	case NK7110_SUBSMS_CELLBRD_OK: /* 0x21 */
	case NK7110_SUBSMS_CELLBRD_FAIL: /* 0x22 */
	case NK7110_SUBSMS_READ_CELLBRD: /* 0x23 */
	case NK7110_SUBSMS_SMSC_OK: /* 0x31 */
	case NK7110_SUBSMS_SMSC_FAIL: /* 0x32 */
	case NK7110_SUBSMS_SMSC_RCVFAIL: /* 0x35 */
		dprintf("Subtype 0x%02x of type 0x%02x (SMS handling) not implemented\n", message[3], NK7110_MSG_SMS);
		return GN_ERR_NOTIMPLEMENTED;

	default:
		dprintf("Unknown subtype of type 0x%02x (SMS handling): 0x%02x\n", NK7110_MSG_SMS, message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return e;
}

static gn_error NK7110_GetSMSCenter(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, NK7110_SUBSMS_GET_SMSC, 0x64, 0x00};

	req[5] = data->message_center->id;

	SEND_MESSAGE_BLOCK(NK7110_MSG_SMS, 6);
}

static gn_error NK7110_PollSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};
	dprintf("Requesting for the notify of the incoming SMS\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_SMS, 8);
}

static gn_error NK7110_SendSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01, 0x02, 0x00};
	gn_error error;
	int len;

	len = pnok_fbus_sms_encode(req + 6, data, state);
	len += 6;

	if (sm_message_send(len, PNOK_MSG_ID_SMS, req, state)) return GN_ERR_NOTREADY;
	do {
		error = sm_block_no_retry_timeout(PNOK_MSG_ID_SMS, state->config.smsc_timeout, data, state);
	} while (!state->config.smsc_timeout && error == GN_ERR_TIMEOUT);

	return error;
}

/**********************************/
/************* CLOCK **************/
/**********************************/
static gn_error NK7110_IncomingClock(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error e = GN_ERR_NONE;

	if (!data) return GN_ERR_INTERNALERROR;
	switch (message[3]) {
	case NK7110_SUBCLO_DATE_RCVD:
		if (!data->datetime) return GN_ERR_INTERNALERROR;
		data->datetime->year = (((unsigned int)message[8]) << 8) + message[9];
		data->datetime->month = message[10];
		data->datetime->day = message[11];
		data->datetime->hour = message[12];
		data->datetime->minute = message[13];
		data->datetime->second = message[14];

		break;
	case NK7110_SUBCLO_DATE_SET:
		break;
	case NK7110_SUBCLO_ALARM_RCVD:
		if (!data->alarm) return GN_ERR_INTERNALERROR;
		switch(message[8]) {
		case NK7110_ALARM_ENABLED:
			data->alarm->enabled = true;
			break;
		case NK7110_ALARM_DISABLED:
			data->alarm->enabled = false;
			break;
		default:
			data->alarm->enabled = false;
			dprintf("Unknown value of alarm enable byte: 0x%02x\n", message[8]);
			e = GN_ERR_UNKNOWN;
			break;
		}

		data->alarm->timestamp.hour = message[9];
		data->alarm->timestamp.minute = message[10];

		break;
	case NK7110_SUBCLO_ALARM_SET:
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (clock handling): 0x%02x\n", NK7110_MSG_CLOCK, message[3]);
		e = GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return e;
}

static gn_error NK7110_GetClock(char req_type, gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, req_type};

	SEND_MESSAGE_BLOCK(NK7110_MSG_CLOCK, 4);
}

static gn_error NK7110_SetClock(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x60, 0x01, 0x01, 0x07,
				0x00, 0x00,	/* year hi/lo */
				0x00, 0x00,	/* month, day */
				0x00, 0x00,	/* hour, minute */
				0x00};

	if (!data->datetime) return GN_ERR_INTERNALERROR;

	req[7] = data->datetime->year >> 8;
	req[8] = data->datetime->year & 0xff;
	req[9] = data->datetime->month;
	req[10] = data->datetime->day;
	req[11] = data->datetime->hour;
	req[12] = data->datetime->minute;

	SEND_MESSAGE_BLOCK(NK7110_MSG_CLOCK, 14);
}

static gn_error NK7110_SetAlarm(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6b, 0x01, 0x20, 0x03,
				0x02,		/* should be alarm on/off */
				0x00, 0x00,	/* hours, minutes */
				0x00};

	if (!data->alarm) return GN_ERR_INTERNALERROR;

	if (data->alarm->enabled) {
		req[7] = NK7110_ALARM_ENABLED;
		req[8] = data->alarm->timestamp.hour;
		req[9] = data->alarm->timestamp.minute;
	} else {
		req[7] = NK7110_ALARM_DISABLED;
	}

	SEND_MESSAGE_BLOCK(NK7110_MSG_CLOCK, 11);
}


/**********************************/
/*********** CALENDAR *************/
/**********************************/

static gn_error NK7110_IncomingCalendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error e = GN_ERR_NONE;
	int i, year;

	if (!data || !data->calnote) return GN_ERR_INTERNALERROR;

	year = data->calnote->time.year;
	switch (message[3]) {
	case NK7110_SUBCAL_NOTE_RCVD:
		calnote_decode(message, length, data);
		break;
	case NK7110_SUBCAL_INFO_RCVD:
		if (!data->calnote_list) return GN_ERR_INTERNALERROR;
		dprintf("Calendar Notes Info received! %i\n", message[4] * 256 + message[5]);
		data->calnote_list->number = message[4] * 256 + message[5];
		dprintf("Location of Notes: ");
		for (i = 0; i < data->calnote_list->number; i++) {
			if (8 + 2 * i >= length) break;
			data->calnote_list->location[data->calnote_list->last+i] = message[8 + 2 * i] * 256 + message[9 + 2 * i];
			dprintf("%i ", data->calnote_list->location[data->calnote_list->last+i]);
		}
		data->calnote_list->last += i;
		dprintf("\n");
		break;
	case NK7110_SUBCAL_FREEPOS_RCVD:
		dprintf("First free position received: %i!\n", message[4]  * 256 + message[5]);
		data->calnote->location = (((unsigned int)message[4]) << 8) + message[5];
		break;
	case NK7110_SUBCAL_DEL_NOTE_RESP:
		dprintf("Succesfully deleted calendar note: %i!\n", message[4] * 256 + message[5]);
		for (i = 0; i < length; i++) dprintf("%02x ", message[i]);
		dprintf("\n");
		break;

	case NK7110_SUBCAL_ADD_MEETING_RESP:
	case NK7110_SUBCAL_ADD_CALL_RESP:
	case NK7110_SUBCAL_ADD_BIRTHDAY_RESP:
	case NK7110_SUBCAL_ADD_REMINDER_RESP:
		if (message[6]) e = GN_ERR_FAILED;
		dprintf("Attempt to write calendar note at %i. Status: %i\n",
			(message[4] << 8) | message[5],
			1 - message[6]);
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (calendar handling): 0x%02x\n", NK7110_MSG_CALENDAR, message[3]);
		e = GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return e;
}

long NK7110_GetNoteAlarmDiff(gn_timestamp *time, gn_timestamp *alarm)
{
	time_t     t_alarm;
	time_t     t_time;
	struct tm  tm_alarm;
	struct tm  tm_time;

	tzset();

	tm_alarm.tm_year  = alarm->year-1900;
	tm_alarm.tm_mon   = alarm->month-1;
	tm_alarm.tm_mday  = alarm->day;
	tm_alarm.tm_hour  = alarm->hour;
	tm_alarm.tm_min   = alarm->minute;
	tm_alarm.tm_sec   = alarm->second;
	tm_alarm.tm_isdst = 0;
	t_alarm = mktime(&tm_alarm);

	tm_time.tm_year  = time->year-1900;
	tm_time.tm_mon   = time->month-1;
	tm_time.tm_mday  = time->day;
	tm_time.tm_hour  = time->hour;
	tm_time.tm_min   = time->minute;
	tm_time.tm_sec   = time->second;
	tm_time.tm_isdst = 0;
	t_time = mktime(&tm_time);

	dprintf("\tAlarm: %02i-%02i-%04i %02i:%02i:%02i\n",
		alarm->day, alarm->month, alarm->year,
		alarm->hour, alarm->minute, alarm->second);
	dprintf("\tDate: %02i-%02i-%04i %02i:%02i:%02i\n",
		time->day, time->month, time->year,
		time->hour, time->minute, time->second);
	dprintf("Difference in alarm time is %f\n", difftime(t_time, t_alarm) + 3600);

	return difftime(t_time, t_alarm) + 3600;
}

static gn_error NK7110_FirstCalendarFreePos(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x31 };

	SEND_MESSAGE_BLOCK(NK7110_MSG_CALENDAR, 4);
}


static gn_error NK7110_WriteCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = { FBUS_FRAME_HEADER,
				   0x01,       /* note type ... */
				   0x00, 0x00, /* location */
				   0x00,       /* entry type */
				   0x00,       /* fixed */
				   0x00, 0x00, 0x00, 0x00, /* year(2bytes), month, day */
				   /* here starts block */
				   0x00, 0x00, 0x00, 0x00,0x00, 0x00}; /* ... depends on note type ... */

	gn_calnote *calnote;
	int count = 0;
	long seconds, minutes;
	int len = 0;
	gn_error error;

	if (!data->calnote) return GN_ERR_INTERNALERROR;
	calnote = data->calnote;

	/* 6210/7110 needs to seek the first free pos to inhabit with next note */
	error = NK7110_FirstCalendarFreePos(data, state);
	if (error != GN_ERR_NONE) return error;

	/* Location */
	req[4] = calnote->location >> 8;
	req[5] = calnote->location & 0xff;

	dprintf("Location: %d\n", calnote->location);

	switch (calnote->type) {
	case GN_CALNOTE_MEETING:
		dprintf("Type: meeting\n");
		req[6] = 0x01;
		req[3] = 0x01;
		break;
	case GN_CALNOTE_CALL:
		dprintf("Type: call\n");
		req[6] = 0x02;
		req[3] = 0x03;
		break;
	case GN_CALNOTE_BIRTHDAY:
		dprintf("Type: birthday\n");
		req[6] = 0x04;
		req[3] = 0x05;
		break;
	case GN_CALNOTE_REMINDER:
		dprintf("Type: reminder\n");
		req[6] = 0x08;
		req[3] = 0x07;
		break;
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}

	req[8]  = calnote->time.year >> 8;
	req[9]  = calnote->time.year & 0xff;
	req[10] = calnote->time.month;
	req[11] = calnote->time.day;

	/* From here starts BLOCK */
	count = 12;
	switch (calnote->type) {

	case GN_CALNOTE_MEETING:
		req[count++] = calnote->time.hour;   /* Field 12 */
		req[count++] = calnote->time.minute; /* Field 13 */
		/* Alarm .. */
		req[count++] = 0xff; /* Field 14 */
		req[count++] = 0xff; /* Field 15 */
		if (calnote->alarm.timestamp.year) {
			seconds = NK7110_GetNoteAlarmDiff(&calnote->time,
							 &calnote->alarm.timestamp);
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* Recurrence */
		if (calnote->recurrence >= 8760)
			calnote->recurrence = 0xffff; /* setting  1 year repeat */
		req[count++] = calnote->recurrence >> 8;   /* Field 16 */
		req[count++] = calnote->recurrence & 0xff; /* Field 17 */
		/* len of the text */
		req[count++] = strlen(calnote->text);    /* Field 18 */
		/* fixed 0x00 */
		req[count++] = 0x00; /* Field 19 */

		/* Text */
		dprintf("Count before encode = %d\n", count);
		dprintf("Meeting Text is = \"%s\"\n", calnote->text);

		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 20->N */
		count += len;
		break;

	case GN_CALNOTE_CALL:
		req[count++] = calnote->time.hour;   /* Field 12 */
		req[count++] = calnote->time.minute; /* Field 13 */
		/* Alarm .. */
		req[count++] = 0xff; /* Field 14 */
		req[count++] = 0xff; /* Field 15 */
		if (calnote->alarm.timestamp.year) {
			seconds = NK7110_GetNoteAlarmDiff(&calnote->time,
							&calnote->alarm.timestamp);
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* Recurrence */
		if (calnote->recurrence >= 8760)
			calnote->recurrence = 0xffff; /* setting  1 year repeat */
		req[count++] = calnote->recurrence >> 8;   /* Field 16 */
		req[count++] = calnote->recurrence & 0xff; /* Field 17 */
		/* len of text */
		req[count++] = strlen(calnote->text);    /* Field 18 */
		/* fixed 0x00 */
		req[count++] = strlen(calnote->phone_number);   /* Field 19 */
		/* Text */
		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 20->N */
		count += len;
		len = char_unicode_encode(req + count, calnote->phone_number, strlen(calnote->phone_number)); /* Fields (N+1)->n */
		count += len;
		break;

	case GN_CALNOTE_BIRTHDAY:
		req[count++] = 0x00; /* Field 12 Fixed */
		req[count++] = 0x00; /* Field 13 Fixed */

		/* Alarm .. */
		req[count++] = 0x00;
		req[count++] = 0x00; /* Fields 14, 15 */
		req[count++] = 0xff; /* Field 16 */
		req[count++] = 0xff; /* Field 17 */
		if (calnote->alarm.timestamp.year) {
			/* First I try time.year = alarm.year. If negative, I increase year by one,
			   but only once! This is because I may have alarm period across
			   the year border, eg. birthday on 2001-01-10 and alarm on 2000-12-27 */
			calnote->time.year = calnote->alarm.timestamp.year;
			if ((seconds= NK7110_GetNoteAlarmDiff(&calnote->time,
							     &calnote->alarm.timestamp)) < 0L) {
				calnote->time.year++;
				seconds = NK7110_GetNoteAlarmDiff(&calnote->time,
								 &calnote->alarm.timestamp);
			}
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				count -= 4;
				req[count++] = seconds >> 24;              /* Field 14 */
				req[count++] = (seconds >> 16) & 0xff;     /* Field 15 */
				req[count++] = (seconds >> 8) & 0xff;      /* Field 16 */
				req[count++] = seconds & 0xff;             /* Field 17 */
			}
		}

		req[count++] = 0x00; /* FIXME: calnote->AlarmType; 0x00 tone, 0x01 silent 18 */

		/* len of text */
		req[count++] = strlen(calnote->text); /* Field 19 */

		/* Text */
		dprintf("Count before encode = %d\n", count);

		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 22->N */
		count += len;
		break;

	case GN_CALNOTE_REMINDER:
		/* Recurrence */
		if (calnote->recurrence >= 8760)
			calnote->recurrence = 0xffff; /* setting  1 year repeat */
		req[count++] = calnote->recurrence >> 8;   /* Field 12 */
		req[count++] = calnote->recurrence & 0xff; /* Field 13 */
		/* len of text */
		req[count++] = strlen(calnote->text);    /* Field 14 */
		/* fixed 0x00 */
		req[count++] = 0x00; /* Field 15 */
		dprintf("Count before encode = %d\n", count);
		dprintf("Reminder Text is = \"%s\"\n", calnote->text);
		/* Text */
		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 16->N */
		count += len;
		break;

	default:
		return GN_ERR_NOTIMPLEMENTED;
	}

	/* padding */
	req[count] = 0x00;

	dprintf("Count after padding = %d\n", count);

	SEND_MESSAGE_BLOCK(NK7110_MSG_CALENDAR, count);
}

#define LAST_INDEX (data->calnote_list->last > 0 ? data->calnote_list->last - 1 : 0)
static gn_error NK7110_GetCalendarNotesInfo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, NK7110_SUBCAL_GET_INFO, 0xff, 0xfe};
	gn_error error;

	/* Some magic: we need to set req[4-5] to {0xff, 0xfe} with the first loop */
	data->calnote_list->location[0] = 0xff * 256 + 0xfe;
	/* Be sure it is 0 */
	data->calnote_list->last = 0;
	do {
		dprintf("Read %d of %d calendar entries\n", data->calnote_list->last, data->calnote_list->number);
		req[4] = data->calnote_list->location[LAST_INDEX] / 256;
		req[5] = data->calnote_list->location[LAST_INDEX] % 256;
		if (sm_message_send(6, NK7110_MSG_CALENDAR, req, state)) return GN_ERR_NOTREADY;
		error = sm_block(NK7110_MSG_CALENDAR, data, state);
		if (error != GN_ERR_NONE) return error;
	} while (data->calnote_list->last < data->calnote_list->number);
	return error;
}
#undef LAST_INDEX

static gn_error NK7110_GetCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	gn_error	error = GN_ERR_NOTREADY;
	unsigned char	req[] = {FBUS_FRAME_HEADER, NK7110_SUBCAL_GET_NOTE, 0x00, 0x00};
	unsigned char	date[] = {FBUS_FRAME_HEADER, NK7110_SUBCLO_GET_DATE};
	gn_data	tmpdata;
	gn_timestamp	tmptime;
	gn_calnote_list list;

	dprintf("Getting calendar note...\n");
	if (data->calnote->location < 1) {
		error = GN_ERR_INVALIDLOCATION;
	} else {
		data->calnote_list = &list;
		tmpdata.datetime = &tmptime;
		error = NK7110_GetCalendarNotesInfo(data, state);
		if (error == GN_ERR_NONE) {
			if (!data->calnote_list->number ||
			    data->calnote->location > data->calnote_list->number) {
				error = GN_ERR_EMPTYLOCATION;
			} else {
				error = sm_message_send(4, NK7110_MSG_CLOCK, date, state);
				if (error == GN_ERR_NONE) {
					sm_block(NK7110_MSG_CLOCK, &tmpdata, state);
					req[4] = data->calnote_list->location[data->calnote->location - 1] >> 8;
					req[5] = data->calnote_list->location[data->calnote->location - 1] & 0xff;
					data->calnote->time.year = tmptime.year;

					error = sm_message_send(6, NK7110_MSG_CALENDAR, req, state);
					if (error == GN_ERR_NONE) {
						error = sm_block(NK7110_MSG_CALENDAR, data, state);
					}
				}
			}
		}
	}
	return error;
}

static gn_error NK7110_DeleteCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER,
				0x0b,      /* delete calendar note */
				0x00, 0x00}; /*location */
	gn_calnote_list list;
	bool own_list = true;

	if (data->calnote_list)
		own_list = false;
	else {
		memset(&list, 0, sizeof(gn_calnote_list));
		data->calnote_list = &list;
	}

	if (data->calnote_list->number == 0)
		NK7110_GetCalendarNotesInfo(data, state);

	if (data->calnote->location < data->calnote_list->number + 1 &&
	    data->calnote->location > 0) {
		req[4] = data->calnote_list->location[data->calnote->location - 1] >> 8;
		req[5] = data->calnote_list->location[data->calnote->location - 1] & 0xff;
	} else {
		return GN_ERR_INVALIDLOCATION;
	}

	if (own_list) data->calnote_list = NULL;
	SEND_MESSAGE_BLOCK(NK7110_MSG_CALENDAR, 6);
}

#if 0
static gn_error NK7110_DialVoice(char *Number)
{
	/* Doesn't work (yet) */    /* 3 2 1 5 2 30 35 */

	unsigned char req0[100] = { 0x00, 0x01, 0x64, 0x01 };

	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x01, 0x01, 0x01, 0x05, 0x00, 0x01, 0x03, 0x02, 0x91, 0x00, 0x031, 0x32, 0x00};
	/* unsigned char req[100]={0x00, 0x01, 0x7c, 0x01, 0x31, 0x37, 0x30, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01};
	  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x20, 0x01, 0x46};
	  unsigned char req_end[] = {0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01}; */
	int len = 0;

	req[4] = strlen(Number);
	for(i = 0; i < strlen(Number); i++)
		req[5+i] = Number[i];
	memcpy(req + 5 + strlen(Number), req_end, 10);
	len = 6 + strlen(Number);
	len = 4;
	PGEN_CommandResponse(&link, req0, &len, 0x40, 0x40, 100);
	len = 17;
	if (PGEN_CommandResponse(&link, req, &len, 0x01, 0x01, 100) == GN_ERR_NONE)
		return GN_ERR_NONE;
	else
		return GN_ERR_NOTIMPLEMENTED;
	while(1) link.Loop(NULL);

	return GN_ERR_NOTIMPLEMENTED;
}
#endif


/*****************************/
/********* STARTUP ***********/
/*****************************/
static gn_error NK7110_IncomingStartup(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/*
	  01 13 00 ed 1c 00 39 35 32 37 32 00
	  01 13 00 ed 15 00 00 00 00 04 c0 02 00 3c c0 03
	*/
	switch (message[4]) {
	case 0x02:
		dprintf("Startup logo set ok\n");
		return GN_ERR_NONE;
		break;
	case 0x15:
		if (data->bitmap) {
			/* I'm sure there are blocks here but never mind! */
			data->bitmap->type = GN_BMP_StartupLogo;
			data->bitmap->height = message[13];
			data->bitmap->width = message[17];
			data->bitmap->size = ((data->bitmap->height / 8) + (data->bitmap->height % 8 > 0)) * data->bitmap->width; /* Can't see this coded anywhere */
			memcpy(data->bitmap->bitmap, message + 22, data->bitmap->size);
			dprintf("Startup logo got ok - height(%d) width(%d)\n", data->bitmap->height, data->bitmap->width);
		}
		return GN_ERR_NONE;
		break;
	case 0x1c:
		dprintf("Succesfully got security code: ");
		memcpy(data->security_code->code, message + 6, 5);
		dprintf("%s \n", data->security_code->code);
		return GN_ERR_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x7a (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
}

static gn_error GetStartupBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0xee, 0x15};

	dprintf("Getting startup logo...\n");
	SEND_MESSAGE_BLOCK(NK7110_MSG_STLOGO, 5);
}

static gn_error NK7110_GetSecurityCode(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER,
			       0xEE,
			       0x1c};			/* SecurityCode */

	SEND_MESSAGE_BLOCK(NK7110_MSG_STLOGO, 5);
}

static gn_error SetStartupBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[1000] = {FBUS_FRAME_HEADER, 0xec, 0x15, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x02, 0x00,
				   0x00,           /* Height */
				   0xc0, 0x03, 0x00,
				   0x00,           /* Width */
				   0xc0, 0x04, 0x03, 0x00 };
	int count = 21;

	if ((data->bitmap->width != state->driver.phone.startup_logo_width) ||
	    (data->bitmap->height != state->driver.phone.startup_logo_height)) {

		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n", state->driver.phone.startup_logo_height, state->driver.phone.startup_logo_width, data->bitmap->height, data->bitmap->width);
		return GN_ERR_INVALIDSIZE;
	}

	req[12] = data->bitmap->height;
	req[16] = data->bitmap->width;
	memcpy(req + count, data->bitmap->bitmap, data->bitmap->size);
	count += data->bitmap->size;
	dprintf("Setting startup logo...\n");

	SEND_MESSAGE_BLOCK(NK7110_MSG_STLOGO, count);
}

/*****************************/
/********* SECURITY **********/
/*****************************/
static gn_error NK7110_IncomingSecurity(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch(message[2]) {
	default:
		dprintf("Unknown security command\n");
		return pnok_security_incoming(messagetype, message, length, data, state);
	}
	return GN_ERR_NONE;
}

/*****************/
/****** WAP ******/
/*****************/
static gn_error NK7110_IncomingWAP(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int string_length, pos, pad = 0;

	switch (message[3]) {
	case 0x02:
	case 0x05:
	case 0x08:
	case 0x0b:
	case 0x0e:
	case 0x11:
	case 0x14:
	case 0x17:
	case 0x1a:
	case 0x20:
		switch (message[4]) {
		case 0x00:
			dprintf("WAP not activated?\n");
			return GN_ERR_UNKNOWN;
		case 0x01:
			dprintf("Security error. Inside WAP bookmarks menu\n");
			return GN_ERR_UNKNOWN;
		case 0x02:
			dprintf("Invalid or empty\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("ERROR: unknown %i\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	case 0x01:
	case 0x04:
	case 0x10:
		break;
	case 0x07:
		if (!data->wap_bookmark) return GN_ERR_INTERNALERROR;
		dprintf("WAP bookmark received\n");
		string_length = message[6] << 1;

		char_unicode_decode(data->wap_bookmark->name, message + 7, string_length);
		dprintf("Name: %s\n", data->wap_bookmark->name);
		pos = string_length + 7;

		string_length = message[pos++] << 1;
		char_unicode_decode(data->wap_bookmark->URL, message + pos, string_length);
		dprintf("URL: %s\n", data->wap_bookmark->URL);
		break;
	case 0x0a:
		dprintf("WAP bookmark successfully set!\n");
		data->wap_bookmark->location = message[5];
		break;
	case 0x0d:
		dprintf("WAP bookmark successfully deleted!\n");
		break;
	case 0x13:
		dprintf("WAP setting successfully activated!\n");
		break;
	case 0x16:
		if (!data->wap_setting) return GN_ERR_INTERNALERROR;
		dprintf("WAP setting received\n");
		/* If ReadBeforeWrite is set we only want the successors */

		string_length = message[4] << 1;
		if (!data->wap_setting->read_before_write)
			char_unicode_decode(data->wap_setting->name, message + 5, string_length);
		dprintf("Name: %s\n", data->wap_setting->name);
		pos = string_length + 5;
		if (!(string_length % 4)) pad = 1;

		string_length = message[pos++] << 1;
		if (!data->wap_setting->read_before_write)
			char_unicode_decode(data->wap_setting->home, message + pos, string_length);
		dprintf("Home: %s\n", data->wap_setting->home);
		pos += string_length;

		if (!data->wap_setting->read_before_write) {
			data->wap_setting->session = message[pos++];
			switch (message[pos]) {
			case 0x06:
				data->wap_setting->bearer = GN_WAP_BEARER_GSMDATA;
				break;
			case 0x07:
				data->wap_setting->bearer = GN_WAP_BEARER_SMS;
				break;
			default:
				data->wap_setting->bearer = GN_WAP_BEARER_USSD;
				break;
			}
			if (message[pos + 12] == 0x01)
				data->wap_setting->security = true;
			else
				data->wap_setting->security = false;
		} else
			pos ++;

		data->wap_setting->successors[0] = message[pos + 2];
		data->wap_setting->successors[1] = message[pos + 3];
		data->wap_setting->successors[2] = message[pos + 8];
		data->wap_setting->successors[3] = message[pos + 9];
		break;
	case 0x1c:
		switch (message[5]) {
		case 0x00:
			dprintf("SMS:\n");
			pos = 6;
			string_length = message[pos++] << 1;
			char_unicode_decode(data->wap_setting->sms_service_number, message + pos, string_length);
			dprintf("   Service number: %s\n", data->wap_setting->sms_service_number);
			pos += string_length;

			string_length = message[pos++] << 1;
			char_unicode_decode(data->wap_setting->sms_server_number, message + pos, string_length);
			dprintf("   Server number: %s\n", data->wap_setting->sms_server_number);
			pos += string_length;
			break;
		case 0x01:
			dprintf("GSM data:\n");
			pos = 6;
			data->wap_setting->gsm_data_authentication = message[pos++];
			data->wap_setting->call_type = message[pos++];
			data->wap_setting->call_speed = message[pos++];
			pos++;

			string_length = message[pos++] << 1;
			char_unicode_decode(data->wap_setting->gsm_data_ip, message + pos, string_length);
			dprintf("   IP: %s\n", data->wap_setting->gsm_data_ip);
			pos += string_length;

			string_length = message[pos++] << 1;
			char_unicode_decode(data->wap_setting->number, message + pos, string_length);
			dprintf("   Number: %s\n", data->wap_setting->number);
			pos += string_length;

			string_length = message[pos++] << 1;
			char_unicode_decode(data->wap_setting->gsm_data_username, message + pos, string_length);
			dprintf("   Username: %s\n", data->wap_setting->gsm_data_username);
			pos += string_length;

			string_length = message[pos++] << 1;
			char_unicode_decode(data->wap_setting->gsm_data_password, message + pos, string_length);
			dprintf("   Password: %s\n", data->wap_setting->gsm_data_password);
			pos += string_length;
			break;
		default:
			break;
		}
		break;
	case 0x1f:
	case 0x19:
		dprintf("WAP setting successfully written!\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x3f (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error SendWAPFrame(gn_data *data, struct gn_statemachine *state, int frame)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x00 };

	dprintf("Sending WAP frame\n");
	req[3] = frame;
	SEND_MESSAGE_BLOCK(NK7110_MSG_WAP, 4);
}

static gn_error PrepareWAP(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Preparing WAP\n");
	return SendWAPFrame(data, state, 0x00);
}

static gn_error FinishWAP(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	dprintf("Finishing WAP\n");

	error = SendWAPFrame(data, state, 0x03);
	if (error != GN_ERR_NONE) return error;

	error = SendWAPFrame(data, state, 0x00);
	if (error != GN_ERR_NONE) return error;

	error = SendWAPFrame(data, state, 0x0f);
	if (error != GN_ERR_NONE) return error;

	return SendWAPFrame(data, state, 0x03);
}

static int PackWAPString(unsigned char *dest, unsigned char *string, int length_size)
{
	int length;

	length = strlen(string);
	if (length_size == 2) {
		dest[0] = length / 256;
		dest[1] = length % 256;
	} else {
		dest[0] = length % 256;
	}

	length = char_unicode_encode(dest + length_size, string, length);
	return length + length_size;
}

static gn_error NK7110_WriteWAPSetting(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[200] = { FBUS_FRAME_HEADER, 0x18,
				   0x00 };		/* Location */
	unsigned char req1[200] = { FBUS_FRAME_HEADER, 0x1e,
				    0x00 };		/* Location */
	unsigned char req2[] = { FBUS_FRAME_HEADER, 0x15,
				 0x00 };		/* Location */

	gn_error error;
	int i = 0, pos = 5;

	dprintf("Writing WAP setting\n");
	memset(req + pos, 0, 200 - pos);
	req[4] = data->wap_setting->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		SendWAPFrame(data, state, 0x03);
		if ((error = PrepareWAP(data, state))) return error;
	}

	/* First we need to get WAP setting from the location we want to write to
	   because we need wap_setting->successors */
	req2[4] = data->wap_setting->location;
	data->wap_setting->read_before_write = true;
	if (sm_message_send(5, NK7110_MSG_WAP, req2, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error != GN_ERR_NONE) return error;

	/* Name */
	pos += PackWAPString(req + pos, data->wap_setting->name, 1);
	/* Home */
	pos += PackWAPString(req + pos, data->wap_setting->home, 1);

	req[pos++] = data->wap_setting->session;
	req[pos++] = data->wap_setting->bearer;

	req[pos++] = 0x0a;
	if (data->wap_setting->security)	req[pos] = 0x01;
	pos++;
	memcpy(req + pos, "\x00\x80\x00\x00\x00\x00\x00\x00\x00", 9);
	pos += 9;

	if (sm_message_send(pos, NK7110_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error) return error;

	for (i = 0; i < 4; i++) {
		pos = 4;
		memset(req1 + pos, 0, 200 - pos);
		if (i == 0 || i== 2) {  /* SMS/USSD */
			req1[pos++] = data->wap_setting->successors[i];
			req1[pos++] = 0x02;
			req1[pos++] = 0x00; /* SMS */
			/* SMS Service */
			pos += PackWAPString(req1 + pos, data->wap_setting->sms_service_number, 1);
			/* SMS Server */
			pos += PackWAPString(req1 + pos, data->wap_setting->sms_server_number, 1);
		} else {  /* GSMdata */
			req1[pos++] = data->wap_setting->successors[i];
			req1[pos++] = 0x02;
			req1[pos++] = 0x01; /* GSMdata */
			req1[pos++] = data->wap_setting->gsm_data_authentication;
			req1[pos++] = data->wap_setting->call_type;
			req1[pos++] = data->wap_setting->call_speed;
			req1[pos++] = 0x01;
			/* IP */
			pos += PackWAPString(req1 + pos, data->wap_setting->gsm_data_ip, 1);
			/* Number */
			pos += PackWAPString(req1 + pos, data->wap_setting->number, 1);
			/* Username  */
			pos += PackWAPString(req1 + pos, data->wap_setting->gsm_data_username, 1);
			/* Password */
			pos += PackWAPString(req1 + pos, data->wap_setting->gsm_data_password, 1);
		}
		memcpy(req1 + pos, "\x80\x00\x00\x00\x00\x00\x00\x00", 8);
		pos += 8;

		if (sm_message_send(pos, NK7110_MSG_WAP, req1, state)) return GN_ERR_NOTREADY;
		error = sm_block(NK7110_MSG_WAP, data, state);
		if (error) return error;
	}

	return FinishWAP(data, state);
}

static gn_error NK7110_DeleteWAPBookmark(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x0C,
				0x00, 0x00};		/* Location */
	gn_error error;

	dprintf("Deleting WAP bookmark\n");
	req[5] = data->wap_bookmark->location + 1;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(6, NK7110_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK7110_GetWAPBookmark(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x06,
				0x00, 0x00};		/* Location */
	gn_error error;

	dprintf("Getting WAP bookmark\n");
	req[5] = data->wap_bookmark->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(6, NK7110_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK7110_WriteWAPBookmark(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[350] = { FBUS_FRAME_HEADER, 0x09,
				0xFF, 0xFF};		/* Location */
	gn_error error;
	int pos = 6;

	dprintf("Writing WAP bookmark\n");

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	pos += PackWAPString(req + pos, data->wap_bookmark->name, 1);
	pos += PackWAPString(req + pos, data->wap_bookmark->URL, 1);

	if (sm_message_send(pos, NK7110_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK7110_GetWAPSetting(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x15,
				0x00 };		/* location */
	unsigned char req2[] = { FBUS_FRAME_HEADER, 0x1b,
				 0x00 };		/* location */
	gn_error error;
	int i;

	dprintf("Getting WAP setting\n");
	if (!data->wap_setting) return GN_ERR_INTERNALERROR;

	req[4] = data->wap_setting->location;
	memset(data->wap_setting, 0, sizeof(gn_wap_setting));
	data->wap_setting->location = req[4];

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(5, NK7110_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error) return error;

	for (i = 0; i < 4; i++) {
		req2[4] = data->wap_setting->successors[i];
		if (sm_message_send(5, NK7110_MSG_WAP, req2, state)) return GN_ERR_NOTREADY;
		error = sm_block(NK7110_MSG_WAP, data, state);
		if (error) return error;
	}

	return FinishWAP(data, state);
}

static gn_error NK7110_ActivateWAPSetting(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x12,
				0x00 };		/* Location */
	gn_error error;

	dprintf("Activating WAP setting\n");
	req[4] = data->wap_setting->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(5, NK7110_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK7110_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

/*****************************/
/********** KEYPRESS *********/
/*****************************/
static gn_error NK7110_IncomingKeypress(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch(message[2]) {
	case 0x46:
		dprintf("Key successfully pressed\n");
		break;
	case 0x47:
		dprintf("Key successfully released\n");
		break;
	default:
		dprintf("Unknown keypress command\n");
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK7110_PressKey(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x46, 0x00, 0x01,
			       0x0a};		/* keycode */

	dprintf("Pressing key...\n");
	req[5] = data->key_code;
	if (sm_message_send(6, NK7110_MSG_KEYPRESS, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK7110_MSG_KEYPRESS_RESP, data, state);
}

static gn_error NK7110_ReleaseKey(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x47, 0x00, 0x01, 0x0c};

	dprintf("Releasing key...\n");
	if (sm_message_send(6, NK7110_MSG_KEYPRESS, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK7110_MSG_KEYPRESS_RESP, data, state);
}

static gn_error NK7110_PressOrReleaseKey(gn_data *data, struct gn_statemachine *state, bool press)
{
	if (press)
		return NK7110_PressKey(data, state);
	else
		return NK7110_ReleaseKey(data, state);
}

/*****************************/
/********** PROFILE **********/
/*****************************/
static gn_error NK7110_IncomingProfile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	case 0x02:
		if (!data->profile) return GN_ERR_INTERNALERROR;
		switch (message[6]) {
		case 0xff:
			char_unicode_decode(data->profile->name, message + 10, message[9]);
			data->profile->default_name = -1; //!!!FIXME
			break;
		case 0x00:
			data->profile->keypad_tone = (unsigned char)(message[10] - 1);
			break;
		case 0x01:
			//!!!FIXME
			data->profile->lights = message[10];
			break;
		case 0x02:
			switch (message[10]) {
			case 0x00: data->profile->call_alert = GN_PROFILE_CALLALERT_Ringing; break;
			case 0x01: data->profile->call_alert = GN_PROFILE_CALLALERT_Ascending; break;
			case 0x02: data->profile->call_alert = GN_PROFILE_CALLALERT_RingOnce; break;
			case 0x03: data->profile->call_alert = GN_PROFILE_CALLALERT_BeepOnce; break;
			case 0x04: data->profile->call_alert = GN_PROFILE_CALLALERT_CallerGroups; break;
			case 0x05: data->profile->call_alert = GN_PROFILE_CALLALERT_Off; break;
			default: return GN_ERR_UNHANDLEDFRAME; break;
			}
			break;
		case 0x03:
			//!!!FIXME: check it!
			data->profile->ringtone = message[10];
			break;
		case 0x04:
			data->profile->volume = message[10] + 6;
			break;
		case 0x05:
			data->profile->message_tone = message[10];
			break;
		case 0x06:
			data->profile->vibration = message[10];
			break;
		case 0x07:
			data->profile->warning_tone = message[10] ? GN_PROFILE_WARNING_On : GN_PROFILE_WARNING_Off;
			break;
		case 0x08:
			data->profile->caller_groups = message[10];
			break;
		case 0x09:
			data->profile->automatic_answer = message[10];
			break;
		default:
			return GN_ERR_UNHANDLEDFRAME;
			break;
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x39 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error NK7110_GetProfile(gn_data *data, struct gn_statemachine *state)
{
	char req[] = {FBUS_FRAME_HEADER, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00};
	unsigned char i;
	gn_error err;

	if (!data->profile) return GN_ERR_INTERNALERROR;

	req[7] = data->profile->number + 1;
	for (i = 0xff; i != 0x0a; i++) {
		req[8] = i;
		if (sm_message_send(9, NK7110_MSG_PROFILE, req, state)) return GN_ERR_NOTREADY;
		if ((err = sm_block(NK7110_MSG_PROFILE, data, state)) != GN_ERR_NONE) return err;
	}

	return GN_ERR_NONE;
}

static gn_error NK7110_SetProfileFeature(gn_data *data, struct gn_statemachine *state, int id, unsigned char value)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x01, 0x03, 0x00, 0x00, 0x01, 0x00};

	if (!data->profile) return GN_ERR_INTERNALERROR;

	req[7] = id;
	req[8] = data->profile->number;
	req[10] = value;

	if (sm_message_send(11, NK7110_MSG_PROFILE, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK7110_MSG_PROFILE, data, state);
}

static gn_error NK7110_SetProfile(gn_data *data, struct gn_statemachine *state)
{
	gn_profile *p;
	gn_error err;

	if (!(p = data->profile)) return GN_ERR_INTERNALERROR;
	dprintf("Setting profile %d (%s)\n", p->number, p->name);

	if (p->default_name < 0 ) {
		/* FIXME: set profile name */
	}
	if ((err = NK7110_SetProfileFeature(data, state, 0x00, p->keypad_tone + 1)) != GN_ERR_NONE)
		return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x01, p->lights)) != GN_ERR_NONE)
		return err;

	switch (p->call_alert) {
	case GN_PROFILE_CALLALERT_Ringing: err = NK7110_SetProfileFeature(data, state, 0x02, 0x00); break;
	case GN_PROFILE_CALLALERT_Ascending: err = NK7110_SetProfileFeature(data, state, 0x02, 0x01); break;
	case GN_PROFILE_CALLALERT_RingOnce: err = NK7110_SetProfileFeature(data, state, 0x02, 0x02); break;
	case GN_PROFILE_CALLALERT_BeepOnce: err = NK7110_SetProfileFeature(data, state, 0x02, 0x03); break;
	case GN_PROFILE_CALLALERT_CallerGroups: err = NK7110_SetProfileFeature(data, state, 0x02, 0x04); break;
	case GN_PROFILE_CALLALERT_Off: err = NK7110_SetProfileFeature(data, state, 0x02, 0x05); break;
	default: return GN_ERR_UNKNOWN;
	}
	if (err != GN_ERR_NONE) return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x03, p->ringtone)) != GN_ERR_NONE)
		return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x04, p->volume - 6)) != GN_ERR_NONE)
		return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x05, p->message_tone)) != GN_ERR_NONE)
		return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x06, p->vibration)) != GN_ERR_NONE)
		return err;

	switch (p->warning_tone) {
	case GN_PROFILE_WARNING_Off: err = NK7110_SetProfileFeature(data, state, 0x07, 0x00); break;
	case GN_PROFILE_WARNING_On: err = NK7110_SetProfileFeature(data, state, 0x07, 0x01); break;
	default: return GN_ERR_UNKNOWN;
	}
	if (err != GN_ERR_NONE) return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x08, p->caller_groups)) != GN_ERR_NONE)
		return err;

	if ((err = NK7110_SetProfileFeature(data, state, 0x09, p->automatic_answer)) != GN_ERR_NONE)
		return err;

	return GN_ERR_NONE;
}

/*****************************/
/********* RINGTONE **********/
/*****************************/
static gn_error NK7110_IncomingRingtone(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	case 0x23:
		if (!data->ringtone || !data->raw_data) return GN_ERR_INTERNALERROR;
		data->ringtone->location = message[5];
		char_unicode_decode(data->ringtone->name, message + 6, 2 * 15);
		if (data->raw_data->length < length - 36) return GN_ERR_MEMORYFULL;
		if (data->raw_data && data->raw_data->data) {
			memcpy(data->raw_data->data, message + 36, length - 36);
			data->raw_data->length = length - 35;
		}
		break;

	case 0x24:
		return GN_ERR_INVALIDLOCATION;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}

static gn_error NK7110_GetRawRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x22, 0x00, 0x00};

	if (!data || !data->ringtone || !data->raw_data) return GN_ERR_INTERNALERROR;
	if (data->ringtone->location < 0) return GN_ERR_INVALIDLOCATION;

	req[5] = data->ringtone->location;

	SEND_MESSAGE_BLOCK(NK7110_MSG_RINGTONE, 6);
}

static gn_error NK7110_SetRawRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x1f, 0x00, 0x00};
	int len;

	if (!data || !data->ringtone || !data->raw_data || !data->raw_data->data)
		return GN_ERR_INTERNALERROR;
	if (sizeof(req) < 36 + data->raw_data->length) return GN_ERR_MEMORYFULL;
	if (data->ringtone->location < 0) {
		/* FIXME: search first free location */
		data->ringtone->location = DRVINSTANCE(state)->userdef_location;
	}

	req[5] = data->ringtone->location;
	char_unicode_encode(req + 6, data->ringtone->name, strlen(data->ringtone->name));
	memcpy(req + 36, data->raw_data->data, data->raw_data->length);
	len = 36 + data->raw_data->length;

	if (sm_message_send(len, NK7110_MSG_RINGTONE, req, state)) return GN_ERR_NOTREADY;
	return sm_block_ack(state);
}

static gn_error NK7110_GetRingtone(gn_data *data, struct gn_statemachine *state)
{
	gn_data d;
	gn_error err;
	gn_raw_data rawdata;
	char buf[4096];

	if (!data->ringtone) return GN_ERR_INTERNALERROR;

	memset(&rawdata, 0, sizeof(gn_raw_data));
	rawdata.data = buf;
	rawdata.length = sizeof(buf);
	gn_data_clear(&d);
	d.ringtone = data->ringtone;
	d.raw_data = &rawdata;

	if ((err = NK7110_GetRawRingtone(&d, state)) != GN_ERR_NONE) return err;

	return pnok_ringtone_from_raw(data->ringtone, rawdata.data, rawdata.length);
}

static gn_error NK7110_SetRingtone(gn_data *data, struct gn_statemachine *state)
{
	gn_data d;
	gn_error err;
	gn_raw_data rawdata;
	char buf[4096];

	if (!data->ringtone) return GN_ERR_INTERNALERROR;

	memset(&rawdata, 0, sizeof(gn_raw_data));
	rawdata.data = buf;
	rawdata.length = sizeof(buf);
	gn_data_clear(&d);
	d.ringtone = data->ringtone;
	d.raw_data = &rawdata;

	if ((err = pnok_ringtone_to_raw(rawdata.data, &rawdata.length, data->ringtone, 0)) != GN_ERR_NONE)
		return err;

	return NK7110_SetRawRingtone(&d, state);
}

static gn_error NK7110_GetRingtoneList(gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone_list *rl;
	gn_ringtone ringtone;
	gn_data d;
	int i;

#define ADDRINGTONE(id, str) \
	rl->ringtone[rl->count].location = (id); \
	snprintf(rl->ringtone[rl->count].name, sizeof(rl->ringtone[rl->count].name), "%s", (str)); \
	rl->ringtone[rl->count].user_defined = 0; \
	rl->ringtone[rl->count].readable = 0; \
	rl->ringtone[rl->count].writable = 0; \
	rl->count++;

	if (!(rl = data->ringtone_list)) return GN_ERR_INTERNALERROR;
	rl->count = 0;
	rl->userdef_location = DRVINSTANCE(state)->userdef_location;
	rl->userdef_count = 5;

	/* tested on 6210 -- bozo */
	ADDRINGTONE(65, "Ring ring");
	ADDRINGTONE(66, "Low");
	ADDRINGTONE(67, "Do-mi-so");
	ADDRINGTONE(68, "Bee");
	ADDRINGTONE(69, "Cicada");
	ADDRINGTONE(70, "Trio");
	ADDRINGTONE(71, "Intro");
	ADDRINGTONE(72, "Persuasion");
	ADDRINGTONE(73, "Attraction");
	ADDRINGTONE(74, "Playground");
	ADDRINGTONE(75, "Mosquito");
	ADDRINGTONE(76, "Circles");
	ADDRINGTONE(77, "Nokia tune");
	ADDRINGTONE(78, "Sunny walks");
	ADDRINGTONE(79, "Samba");
	ADDRINGTONE(80, "Basic rock");
	ADDRINGTONE(81, "Reveille");
	ADDRINGTONE(82, "Groovy Blue");
	ADDRINGTONE(83, "Brave Scotland");
	ADDRINGTONE(84, "Matilda");
	ADDRINGTONE(85, "Bumblebee");
	ADDRINGTONE(86, "Menuet");
	ADDRINGTONE(87, "Elise");
	ADDRINGTONE(88, "William Tell");
	ADDRINGTONE(89, "Charleston");
	ADDRINGTONE(90, "Fuga");
	ADDRINGTONE(91, "Etude");
	ADDRINGTONE(92, "Hungarian");
	ADDRINGTONE(93, "Valkyrie");
	ADDRINGTONE(94, "Badinerie");
	ADDRINGTONE(95, "Bach #3");
	ADDRINGTONE(96, "Toreador");
	ADDRINGTONE(97, "9th Symphony");
	ADDRINGTONE(98, "WalzeBrilliant");

	memset(&ringtone, 0, sizeof(ringtone));
	gn_data_clear(&d);
	d.ringtone = &ringtone;
	for (i = 0; i < rl->userdef_count; i++) {
		ringtone.location = rl->userdef_location + i;
		if (NK7110_GetRingtone(&d, state) == GN_ERR_NONE) {
			rl->ringtone[rl->count].location = ringtone.location;
			snprintf(rl->ringtone[rl->count].name, sizeof(rl->ringtone[rl->count].name), "%s", ringtone.name);
			rl->ringtone[rl->count].user_defined = 1;
			rl->ringtone[rl->count].readable = 1;
			rl->ringtone[rl->count].writable = 1;
			rl->count++;
		}
	}

#undef ADDRINGTONE

	return GN_ERR_NONE;
}


/*****************************/
/********** OTHERS ***********/
/*****************************/
static gn_error NK7110_GetBitmap(gn_data *data, struct gn_statemachine *state)
{
	switch(data->bitmap->type) {
	case GN_BMP_CallerLogo:
		return GetCallerBitmap(data, state);
	case GN_BMP_StartupLogo:
		return GetStartupBitmap(data, state);
	case GN_BMP_OperatorLogo:
		return GetOperatorBitmap(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static gn_error NK7110_SetBitmap(gn_data *data, struct gn_statemachine *state)
{
	switch(data->bitmap->type) {
	case GN_BMP_CallerLogo:
		return SetCallerBitmap(data, state);
	case GN_BMP_StartupLogo:
		return SetStartupBitmap(data, state);
	case GN_BMP_OperatorLogo:
		return SetOperatorBitmap(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static int get_memory_type(gn_memory_type memory_type)
{
	int result;

	switch (memory_type) {
	case GN_MT_MT:
		result = NK7110_MEMORY_MT;
		break;
	case GN_MT_ME:
		result = NK7110_MEMORY_ME;
		break;
	case GN_MT_SM:
		result = NK7110_MEMORY_SM;
		break;
	case GN_MT_FD:
		result = NK7110_MEMORY_FD;
		break;
	case GN_MT_ON:
		result = NK7110_MEMORY_ON;
		break;
	case GN_MT_EN:
		result = NK7110_MEMORY_EN;
		break;
	case GN_MT_DC:
		result = NK7110_MEMORY_DC;
		break;
	case GN_MT_RC:
		result = NK7110_MEMORY_RC;
		break;
	case GN_MT_MC:
		result = NK7110_MEMORY_MC;
		break;
	case GN_MT_IN:
		result = NK7110_MEMORY_IN;
		break;
	case GN_MT_OU:
		result = NK7110_MEMORY_OU;
		break;
	case GN_MT_AR:
		result = NK7110_MEMORY_AR;
		break;
	case GN_MT_TE:
		result = NK7110_MEMORY_TE;
		break;
	case GN_MT_F1:
		result = NK7110_MEMORY_F1;
		break;
	case GN_MT_F2:
		result = NK7110_MEMORY_F2;
		break;
	case GN_MT_F3:
		result = NK7110_MEMORY_F3;
		break;
	case GN_MT_F4:
		result = NK7110_MEMORY_F4;
		break;
	case GN_MT_F5:
		result = NK7110_MEMORY_F5;
		break;
	case GN_MT_F6:
		result = NK7110_MEMORY_F6;
		break;
	case GN_MT_F7:
		result = NK7110_MEMORY_F7;
		break;
	case GN_MT_F8:
		result = NK7110_MEMORY_F8;
		break;
	case GN_MT_F9:
		result = NK7110_MEMORY_F9;
		break;
	case GN_MT_F10:
		result = NK7110_MEMORY_F10;
		break;
	case GN_MT_F11:
		result = NK7110_MEMORY_F11;
		break;
	case GN_MT_F12:
		result = NK7110_MEMORY_F12;
		break;
	case GN_MT_F13:
		result = NK7110_MEMORY_F13;
		break;
	case GN_MT_F14:
		result = NK7110_MEMORY_F14;
		break;
	case GN_MT_F15:
		result = NK7110_MEMORY_F15;
		break;
	case GN_MT_F16:
		result = NK7110_MEMORY_F16;
		break;
	case GN_MT_F17:
		result = NK7110_MEMORY_F17;
		break;
	case GN_MT_F18:
		result = NK7110_MEMORY_F18;
		break;
	case GN_MT_F19:
		result = NK7110_MEMORY_F19;
		break;
	case GN_MT_F20:
		result = NK7110_MEMORY_F20;
		break;
	default:
		result = NK7110_MEMORY_XX;
		break;
	}
	return result;
}

static gn_memory_type get_gn_memory_type(int memory_type)
{
	int result;

	switch (memory_type) {
	case NK7110_MEMORY_MT:
		result = GN_MT_MT;
		break;
	case NK7110_MEMORY_ME:
		result = GN_MT_ME;
		break;
	case NK7110_MEMORY_SM:
		result = GN_MT_SM;
		break;
	case NK7110_MEMORY_FD:
		result = GN_MT_FD;
		break;
	case NK7110_MEMORY_ON:
		result = GN_MT_ON;
		break;
	case NK7110_MEMORY_DC:
		result = GN_MT_DC;
		break;
	case NK7110_MEMORY_RC:
		result = GN_MT_RC;
		break;
	case NK7110_MEMORY_MC:
		result = GN_MT_MC;
		break;
	case NK7110_MEMORY_IN:
		result = GN_MT_IN;
		break;
	case NK7110_MEMORY_OU:
		result = GN_MT_OU;
		break;
	case NK7110_MEMORY_AR:
		result = GN_MT_AR;
		break;
	case NK7110_MEMORY_TE:
		result = GN_MT_TE;
		break;
	case NK7110_MEMORY_F1:
		result = GN_MT_F1;
		break;
	case NK7110_MEMORY_F2:
		result = GN_MT_F2;
		break;
	case NK7110_MEMORY_F3:
		result = GN_MT_F3;
		break;
	case NK7110_MEMORY_F4:
		result = GN_MT_F4;
		break;
	case NK7110_MEMORY_F5:
		result = GN_MT_F5;
		break;
	case NK7110_MEMORY_F6:
		result = GN_MT_F6;
		break;
	case NK7110_MEMORY_F7:
		result = GN_MT_F7;
		break;
	case NK7110_MEMORY_F8:
		result = GN_MT_F8;
		break;
	case NK7110_MEMORY_F9:
		result = GN_MT_F9;
		break;
	case NK7110_MEMORY_F10:
		result = GN_MT_F10;
		break;
	case NK7110_MEMORY_F11:
		result = GN_MT_F11;
		break;
	case NK7110_MEMORY_F12:
		result = GN_MT_F12;
		break;
	case NK7110_MEMORY_F13:
		result = GN_MT_F13;
		break;
	case NK7110_MEMORY_F14:
		result = GN_MT_F14;
		break;
	case NK7110_MEMORY_F15:
		result = GN_MT_F15;
		break;
	case NK7110_MEMORY_F16:
		result = GN_MT_F16;
		break;
	case NK7110_MEMORY_F17:
		result = GN_MT_F17;
		break;
	case NK7110_MEMORY_F18:
		result = GN_MT_F18;
		break;
	case NK7110_MEMORY_F19:
		result = GN_MT_F19;
		break;
	case NK7110_MEMORY_F20:
		result = GN_MT_F20;
		break;
	default:
		result = GN_MT_XX;
		break;
	}
	return result;
}

static gn_error NBSUpload(gn_data *data, struct gn_statemachine *state, gn_sms_data_type type)
{
    unsigned char req[512] = {0x7c, 0x01, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    gn_sms sms;
    gn_sms_raw rawsms;
    gn_error err;
    int n;

    gn_sms_default_submit(&sms);
    sms.user_data[0].type = type;
    sms.user_data[1].type = GN_SMS_DATA_None;

    switch (type) {
    case GN_SMS_DATA_Ringtone:
	    memcpy(&sms.user_data[0].u.ringtone, data->ringtone, sizeof(gn_ringtone));
	    break;
    default:
	    return GN_ERR_INTERNALERROR;
    }

    memset(&rawsms, 0, sizeof(rawsms));

    if ((err = sms_prepare(&sms, &rawsms)) != GN_ERR_NONE) return err;

    req[10] = rawsms.user_data_length;
    n = 11 + rawsms.user_data_length;
    if (n > sizeof(req)) return GN_ERR_INTERNALERROR;
    memcpy(req + 11, rawsms.user_data, rawsms.user_data_length);

    return sm_message_send(n, 0x00, req, state);
}


/*****************************/
/******* COMM STATUS *********/
/*****************************/
static gn_error NK7110_IncomingCommstatus(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char *pos;
	int i;
	gn_call_active *ca;

	switch (message[3]) {
	/* get call status */
	case 0x21:
		if (!data->call_active) return GN_ERR_INTERNALERROR;
		if (message[5] != 0xff) return GN_ERR_UNHANDLEDFRAME;
		pos = message + 6;
		ca = data->call_active;
		memset(ca, 0x00, 2 * sizeof(gn_call_active));
		for (i = 0; i < message[4]; i++) {
			if (pos[0] != 0x64) return GN_ERR_UNHANDLEDFRAME;
			ca[i].call_id = pos[2];
			ca[i].channel = pos[3];
			switch (pos[4]) {
			case 0x00: ca[i].state = GN_CALL_Idle; break; /* missing number, wait a little */
			case 0x02: ca[i].state = GN_CALL_Dialing; break;
			case 0x03: ca[i].state = GN_CALL_Ringing; break;
			case 0x04: ca[i].state = GN_CALL_Incoming; break;
			case 0x05: ca[i].state = GN_CALL_Established; break;
			case 0x06: ca[i].state = GN_CALL_Held; break;
			case 0x07: ca[i].state = GN_CALL_RemoteHangup; break;
			default: return GN_ERR_UNHANDLEDFRAME;
			}
			switch (pos[5]) {
			case 0x00: ca[i].prev_state = GN_CALL_Idle; break; /* missing number, wait a little */
			case 0x02: ca[i].prev_state = GN_CALL_Dialing; break;
			case 0x03: ca[i].prev_state = GN_CALL_Ringing; break;
			case 0x04: ca[i].prev_state = GN_CALL_Incoming; break;
			case 0x05: ca[i].prev_state = GN_CALL_Established; break;
			case 0x06: ca[i].prev_state = GN_CALL_Held; break;
			case 0x07: ca[i].prev_state = GN_CALL_RemoteHangup; break;
			default: return GN_ERR_UNHANDLEDFRAME;
			}
			char_unicode_decode(ca[i].name, pos + 12, 2 * pos[10]);
			char_unicode_decode(ca[i].number, pos + 112, 2 * pos[11]);
			pos += pos[1];
		}
		dprintf("Call status:\n");
		for (i = 0; i < 2; i++) {
			if (ca[i].state == GN_CALL_Idle) continue;
			dprintf("ch#%d: id#%d st#%d pst#%d %s (%s)\n",
				ca[i].channel, ca[i].call_id, ca[i].state, ca[i].prev_state, ca[i].number, ca[i].name);
		}
		break;

	/* hangup */
	case 0x04:
		dprintf("Hangup!\n");
		dprintf("Call ID: %i\n", message[4]);
		dprintf("Cause Type: %i\n", message[5]);
		dprintf("Cause ID: %i\n", message[6]);
		return GN_ERR_UNKNOWN;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK7110_GetActiveCalls(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x20};

	if (!data->call_active) return GN_ERR_INTERNALERROR;

	if (sm_message_send(4, NK7110_MSG_COMMSTATUS, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK7110_MSG_COMMSTATUS, data, state);
}

static gn_error NK7110_MakeCall(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01};
	unsigned char voice_end[] = {0x05, 0x01, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00};
	int pos = 4, len;
	gn_call_active active[2];
	gn_data d;

	if (!data->call_info) return GN_ERR_INTERNALERROR;

	switch (data->call_info->type) {
	case GN_CALL_Voice:
		break;

	case GN_CALL_NonDigitalData:
	case GN_CALL_DigitalData:
		dprintf("Unsupported call type %d\n", data->call_info->type);
		return GN_ERR_NOTSUPPORTED;

	default:
		dprintf("Invalid call type %d\n", data->call_info->type);
		return GN_ERR_INTERNALERROR;
	}

	len = strlen(data->call_info->number);
	if (len > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	len = char_unicode_encode(req + pos + 1, data->call_info->number, len);
	req[pos++] = len / 2;
	pos += len;

	switch (data->call_info->send_number) {
	case GN_CALL_Never:   voice_end[5] = 0x01; break;
	case GN_CALL_Always:  voice_end[5] = 0x00; break;
	case GN_CALL_Default: voice_end[5] = 0x00; break;
	default: return GN_ERR_INTERNALERROR;
	}
	memcpy(req + pos, voice_end, sizeof(voice_end));
	pos += sizeof(voice_end);

	if (sm_message_send(pos, NK7110_MSG_COMMSTATUS, req, state)) return GN_ERR_NOTREADY;
	if (sm_block_ack(state) != GN_ERR_NONE) return GN_ERR_NOTREADY;

	memset(active, 0, sizeof(*active));
	gn_data_clear(&d);
	d.call_active = active;
	if (NK7110_GetActiveCalls(&d, state) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	data->call_info->call_id = active[0].call_id;

	return GN_ERR_NONE;
}

static gn_error NK7110_CancelCall(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00};

	if (!data->call_info) return GN_ERR_INTERNALERROR;

	req[4] = data->call_info->call_id;

	if (sm_message_send(5, NK7110_MSG_COMMSTATUS, req, state)) return GN_ERR_NOTREADY;
	return sm_block_ack(state);
}

static gn_error NK7110_AnswerCall(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x06, 0x00};

	if (!data->call_info) return GN_ERR_INTERNALERROR;

	req[4] = data->call_info->call_id;

	if (sm_message_send(5, NK7110_MSG_COMMSTATUS, req, state)) return GN_ERR_NOTREADY;
	return sm_block_ack(state);
}
