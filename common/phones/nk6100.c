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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2002 Georg Moritz, Markus Plail, Jan Kratochvil
  Copyright (C) 2003 Bertrik Sikken

  This file provides functions specific to the Nokia 6100 series.
  See README for more details on supported mobile phones.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"
#include "phones/generic.h"
#include "phones/nokia.h"
#include "phones/nk6100.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "links/m2bus.h"

#include "gnokii-internal.h"
#include "gnokii.h"

#define	DRVINSTANCE(s) (*((nk6100_driver_instance **)(&(s)->driver.driver_instance)))
#define	FREE(p) do { free(p); (p) = NULL; } while (0)

/* static functions prototypes */
static gn_error Functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error Initialise(struct gn_statemachine *state);
static gn_error Authentication(struct gn_statemachine *state, char *imei);
static gn_error BuildKeytable(struct gn_statemachine *state);
static gn_error Identify(gn_data *data, struct gn_statemachine *state);
static gn_error GetSpeedDial(gn_data *data, struct gn_statemachine *state);
static gn_error SetSpeedDial(gn_data *data, struct gn_statemachine *state);
static gn_error PhoneInfo(gn_data *data, struct gn_statemachine *state);
static gn_error GetBatteryLevel(gn_data *data, struct gn_statemachine *state);
static gn_error GetRFLevel(gn_data *data, struct gn_statemachine *state);
static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state);
static gn_error SendSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error SetOnSMS(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error SaveSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error DeleteSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSFolders(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state);
static gn_error GetBitmap(gn_data *data, struct gn_statemachine *state);
static gn_error SetBitmap(gn_data *data, struct gn_statemachine *state);
static gn_error ReadPhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error WritePhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error DeletePhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error GetPowersource(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSStatus(gn_data *data, struct gn_statemachine *state);
static gn_error GetNetworkInfo(gn_data *data, struct gn_statemachine *state);
static gn_error GetWelcomeMessage(gn_data *data, struct gn_statemachine *state);
static gn_error GetOperatorLogo(gn_data *data, struct gn_statemachine *state);
static gn_error GetDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error SetDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error GetAlarm(gn_data *data, struct gn_statemachine *state);
static gn_error SetAlarm(gn_data *data, struct gn_statemachine *state);
static gn_error GetProfile(gn_data *data, struct gn_statemachine *state);
static gn_error SetProfile(gn_data *data, struct gn_statemachine *state);
static gn_error GetActiveProfile(gn_data *data, struct gn_statemachine *state);
static gn_error SetActiveProfile(gn_data *data, struct gn_statemachine *state);
static gn_error GetCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error WriteCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error DeleteCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error GetDisplayStatus(gn_data *data, struct gn_statemachine *state);
static gn_error DisplayOutput(gn_data *data, struct gn_statemachine *state);
static gn_error PollDisplay(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSCenter(gn_data *data, struct gn_statemachine *state);
static gn_error SetSMSCenter(gn_data *data, struct gn_statemachine *state);
static gn_error SetCellBroadcast(gn_data *data, struct gn_statemachine *state);
static gn_error GetActiveCalls1(gn_data *data, struct gn_statemachine *state);
static gn_error MakeCall1(gn_data *data, struct gn_statemachine *state);
static gn_error AnswerCall1(gn_data *data, struct gn_statemachine *state);
static gn_error CancelCall1(gn_data *data, struct gn_statemachine *state);
static gn_error SetCallNotification(gn_data *data, struct gn_statemachine *state);
static gn_error GetActiveCalls(gn_data *data, struct gn_statemachine *state);
static gn_error MakeCall(gn_data *data, struct gn_statemachine *state);
static gn_error AnswerCall(gn_data *data, struct gn_statemachine *state);
static gn_error CancelCall(gn_data *data, struct gn_statemachine *state);
static gn_error SendRLPFrame(gn_data *data, struct gn_statemachine *state);
static gn_error SetRLPRXCallback(gn_data *data, struct gn_statemachine *state);
static gn_error SendDTMF(gn_data *data, struct gn_statemachine *state);
static gn_error Reset(gn_data *data, struct gn_statemachine *state);
static gn_error GetRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error SetRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error GetRawRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error SetRawRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error DeleteRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error PressOrReleaseKey(bool press, gn_data *data, struct gn_statemachine *state);
static gn_error PressOrReleaseKey1(bool press, gn_data *data, struct gn_statemachine *state);
static gn_error PressOrReleaseKey2(bool press, gn_data *data, struct gn_statemachine *state);
static gn_error EnterChar(gn_data *data, struct gn_statemachine *state);
static gn_error NBSUpload(gn_data *data, struct gn_statemachine *state, gn_sms_data_type type);
static gn_error get_imei(gn_data *data, struct gn_statemachine *state);
static gn_error get_phone_info(gn_data *data, struct gn_statemachine *state);
static gn_error get_hw(gn_data *data, struct gn_statemachine *state);
static gn_error get_ringtone_list(gn_data *data, struct gn_statemachine *state);

#ifdef SECURITY
static gn_error EnterSecurityCode(gn_data *data, struct gn_statemachine *state);
static gn_error GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state);
static gn_error ChangeSecurityCode(gn_data *data, struct gn_statemachine *state);
static gn_error get_security_code(gn_data *data, struct gn_statemachine *state);
#endif

static gn_error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingPhoneInfo2(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingSMS1(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingSMS(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingPhonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingProfile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingCalendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingDisplay(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingSecurity(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingCallInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingRLPFrame(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingKey(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error IncomingMisc(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

#ifdef SECURITY
static gn_error IncomingSecurityCode(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
#endif

static int get_memory_type(gn_memory_type memory_type);
static void FlushLostSMSNotifications(struct gn_statemachine *state);

static gn_incoming_function_type nk6100_incoming_functions[] = {
	{ 0x01, IncomingCallInfo },
	{ 0x02, IncomingSMS1 },
	{ 0x03, IncomingPhonebook },
	{ 0x04, IncomingPhoneStatus },
	{ 0x05, IncomingProfile },
	{ 0x06, pnok_call_divert_incoming },
#ifdef SECURITY
	{ 0x08, IncomingSecurityCode },
#endif
	{ 0x0a, IncomingNetworkInfo },
	{ 0x0c, IncomingKey },
	{ 0x0d, IncomingDisplay },
	{ 0x11, IncomingPhoneClockAndAlarm },
	{ 0x13, IncomingCalendar },
	{ 0x14, IncomingSMS },
	{ 0x40, IncomingSecurity },
	{ 0x64, IncomingPhoneInfo },
	{ 0xd2, IncomingPhoneInfo2 },
	{ 0xda, IncomingMisc },
	{ 0xf1, IncomingRLPFrame },
	{ 0, NULL}
};

gn_driver driver_nokia_6100 = {
	nk6100_incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"6110|6130|6150|6190|5110|5130|5190|3210|3310|3330|3360|3390|3410|8210|8250|8290|8850|RPM-1", /* Supported models */
		4,			/* Max RF Level */
		0,			/* Min RF Level */
		GN_RF_Arbitrary,	/* RF level units */
		4,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GN_BU_Arbitrary,	/* Battery level units */
		GN_DT_DateTime,		/* Have date/time support */
		GN_DT_TimeOnly,		/* Alarm supports time only */
		1,			/* Alarms available - FIXME */
		48, 84,			/* Startup logo size */
		14, 72,			/* Op logo size */
		14, 72			/* Caller logo size */
	},
	Functions,
	NULL
};

struct {
	char *model;
	char *sw_version;
	int capabilities;
} static nk6100_capabilities[] = {
	/*
	 * Capability setup for phone models.
	 * Example:
	 * { "NSE-3",	NULL,		NK6100_CAP_OLD_CALL_API }
	 * Set NULL in the second field for all software versions.
	 */
	{ "NSE-3",	"-4.06",	NK6100_CAP_NBS_UPLOAD }, /* 6110 */
	{ "NHM-5",	NULL,           NK6100_CAP_OLD_KEY_API | NK6100_CAP_PB_UNICODE | NK6100_CAP_NO_PSTATUS | NK6100_CAP_OLD_CALL_API | NK6100_CAP_NO_PB_GROUP }, /* 3310 */
	{ "NHM-5NX",	NULL,           NK6100_CAP_OLD_KEY_API | NK6100_CAP_PB_UNICODE | NK6100_CAP_NO_PSTATUS | NK6100_CAP_OLD_CALL_API | NK6100_CAP_NO_PB_GROUP }, /* 3310 */
	{ "NHM-6",      NULL,           NK6100_CAP_PB_UNICODE | NK6100_CAP_NO_PB_GROUP }, /* 3330 */
	{ "NHM-2",      NULL,           NK6100_CAP_PB_UNICODE }, /* 3410 */
	{ "NSM-3D",     NULL,           NK6100_CAP_PB_UNICODE | NK6100_CAP_CAL_UNICODE }, /* 8250 */
	{ "RPM-1",	"-4.23",	NK6100_CAP_NBS_UPLOAD }, /* Card Phone 2.0 */
	{ "NSE-8",	NULL,		NK6100_CAP_OLD_KEY_API | NK6100_CAP_NO_PSTATUS | NK6100_CAP_NO_CB | NK6100_CAP_OLD_CALL_API | NK6100_CAP_NO_PB_GROUP }, /* 3210 */
	{ "NSE-9",	NULL,		NK6100_CAP_OLD_KEY_API | NK6100_CAP_NO_PSTATUS | NK6100_CAP_NO_CB | NK6100_CAP_OLD_CALL_API | NK6100_CAP_NO_PB_GROUP }, /* 3210 */
	{ NULL,		NULL,		0 }
};

static gn_error Functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	if (!DRVINSTANCE(state) && op != GN_OP_Init) return GN_ERR_INTERNALERROR;

	switch (op) {
	case GN_OP_Init:
		if (DRVINSTANCE(state)) return GN_ERR_INTERNALERROR;
		return Initialise(state);
	case GN_OP_Terminate:
		FREE(DRVINSTANCE(state));
		return pgen_terminate(data, state);
	case GN_OP_GetSpeedDial:
		return GetSpeedDial(data, state);
	case GN_OP_SetSpeedDial:
		return SetSpeedDial(data, state);
	case GN_OP_GetImei:
	case GN_OP_GetModel:
	case GN_OP_GetRevision:
	case GN_OP_GetManufacturer:
	case GN_OP_Identify:
		return Identify(data, state);
	case GN_OP_GetBitmap:
		return GetBitmap(data, state);
	case GN_OP_SetBitmap:
		return SetBitmap(data, state);
	case GN_OP_GetBatteryLevel:
		return GetBatteryLevel(data, state);
	case GN_OP_GetRFLevel:
		return GetRFLevel(data, state);
	case GN_OP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GN_OP_ReadPhonebook:
		return ReadPhonebook(data, state);
	case GN_OP_WritePhonebook:
		return WritePhonebook(data, state);
	case GN_OP_DeletePhonebook:
		return DeletePhonebook(data, state);
	case GN_OP_GetPowersource:
		return GetPowersource(data, state);
	case GN_OP_GetSMSStatus:
		return GetSMSStatus(data, state);
	case GN_OP_GetNetworkInfo:
		return GetNetworkInfo(data, state);
	case GN_OP_SendSMS:
		return SendSMSMessage(data, state);
	case GN_OP_OnSMS:
		return SetOnSMS(data, state);
	case GN_OP_PollSMS:
		gn_sm_loop(1, state);
		return GN_ERR_NONE;
	case GN_OP_GetSMS:
		return GetSMSMessage(data, state);
	case GN_OP_SaveSMS:
		return SaveSMSMessage(data, state);
	case GN_OP_DeleteSMS:
		return DeleteSMSMessage(data, state);
	case GN_OP_GetSMSFolders:
		return GetSMSFolders(data, state);
	case GN_OP_GetSMSFolderStatus:
		return GetSMSFolderStatus(data, state);
	case GN_OP_GetDateTime:
		return GetDateTime(data, state);
	case GN_OP_SetDateTime:
		return SetDateTime(data, state);
	case GN_OP_GetAlarm:
		return GetAlarm(data, state);
	case GN_OP_SetAlarm:
		return SetAlarm(data, state);
	case GN_OP_GetProfile:
		return GetProfile(data, state);
	case GN_OP_SetProfile:
		return SetProfile(data, state);
	case GN_OP_GetActiveProfile:
		return GetActiveProfile(data, state);
	case GN_OP_SetActiveProfile:
		return SetActiveProfile(data, state);
	case GN_OP_GetCalendarNote:
		return GetCalendarNote(data, state);
	case GN_OP_WriteCalendarNote:
		return WriteCalendarNote(data, state);
	case GN_OP_DeleteCalendarNote:
		return DeleteCalendarNote(data, state);
	case GN_OP_GetDisplayStatus:
		return GetDisplayStatus(data, state);
	case GN_OP_DisplayOutput:
		return DisplayOutput(data, state);
	case GN_OP_PollDisplay:
		return PollDisplay(data, state);
	case GN_OP_GetSMSCenter:
		return GetSMSCenter(data, state);
	case GN_OP_SetSMSCenter:
		return SetSMSCenter(data, state);
	case GN_OP_SetCellBroadcast:
		return SetCellBroadcast(data, state);
	case GN_OP_NetMonitor:
		return pnok_netmonitor(data, state);
	case GN_OP_GetActiveCalls:
		return GetActiveCalls(data, state);
	case GN_OP_MakeCall:
		return MakeCall(data, state);
	case GN_OP_AnswerCall:
		return AnswerCall(data, state);
	case GN_OP_CancelCall:
		return CancelCall(data, state);
	case GN_OP_SetCallNotification:
		return SetCallNotification(data, state);
	case GN_OP_SendRLPFrame:
		return SendRLPFrame(data, state);
	case GN_OP_SetRLPRXCallback:
		return SetRLPRXCallback(data, state);
#ifdef SECURITY
	case GN_OP_EnterSecurityCode:
		return EnterSecurityCode(data, state);
	case GN_OP_GetSecurityCodeStatus:
		return GetSecurityCodeStatus(data, state);
	case GN_OP_ChangeSecurityCode:
		return ChangeSecurityCode(data, state);
	case GN_OP_GetSecurityCode:
		return get_security_code(data, state);
#endif
	case GN_OP_GetLocksInfo:
		return pnok_get_locks_info(data, state);
	case GN_OP_SendDTMF:
		return SendDTMF(data, state);
	case GN_OP_Reset:
		return Reset(data, state);
	case GN_OP_GetRingtone:
		return GetRingtone(data, state);
	case GN_OP_SetRingtone:
		return SetRingtone(data, state);
	case GN_OP_GetRawRingtone:
		return GetRawRingtone(data, state);
	case GN_OP_SetRawRingtone:
		return SetRawRingtone(data, state);
	case GN_OP_DeleteRingtone:
		return DeleteRingtone(data, state);
	case GN_OP_PlayTone:
		return pnok_play_tone(data, state);
	case GN_OP_PressPhoneKey:
		return PressOrReleaseKey(true, data, state);
	case GN_OP_ReleasePhoneKey:
		return PressOrReleaseKey(false, data, state);
	case GN_OP_EnterChar:
		return EnterChar(data, state);
	case GN_OP_CallDivert:
		return pnok_call_divert(data, state);
	case GN_OP_GetRingtoneList:
		return get_ringtone_list(data, state);
	default:
		dprintf("nk61xx unimplemented operation: %d\n", op);
		return GN_ERR_NOTIMPLEMENTED;
	}
}

/* Nokia authentication protocol is used in the communication between Nokia
   mobile phones (e.g. Nokia 6110) and Nokia Cellular Data Suite software,
   commercially sold by Nokia Corp.

   The authentication scheme is based on the token send by the phone to the
   software. The software does it's magic (see the function
   FB61_GetNokiaAuth()) and returns the result back to the phone. If the
   result is correct the phone responds with the message "Accessory
   connected!" displayed on the LCD. Otherwise it will display "Accessory not
   supported" and some functions will not be available for use.

   The specification of the protocol is not publicly available, no comment.

   The auth code is written specially for gnokii project by Odinokov Serge.
   If you have some special requests for Serge just write him to
   apskaita@post.omnitel.net or serge@takas.lt

   Reimplemented in C by Pavel Janík ml.
*/

void PNOK_GetNokiaAuth(unsigned char *Imei, unsigned char *MagicBytes, unsigned char *MagicResponse)
{
	int i, j, CRC = 0;
	unsigned char Temp[16]; /* This is our temporary working area. */

	/* Here we put FAC (Final Assembly Code) and serial number into our area. */

	memcpy(Temp, Imei + 6, 8);

	/* And now the TAC (Type Approval Code). */

	memcpy(Temp + 8, Imei + 2, 4);

	/* And now we pack magic bytes from the phone. */

	memcpy(Temp + 12, MagicBytes, 4);

	for (i = 0; i <= 11; i++)
		if (Temp[i + 1] & 1)
			Temp[i]<<=1;

	switch (Temp[15] & 0x03) {
	case 1:
	case 2:
		j = Temp[13] & 0x07;
		for (i = 0; i <= 3; i++)
			Temp[i+j] ^= Temp[i+12];
		break;

	default:
		j = Temp[14] & 0x07;
		for (i = 0; i <= 3; i++)
			Temp[i + j] |= Temp[i + 12];
	}

	for (i = 0; i <= 15; i++)
		CRC ^= Temp[i];

	for (i = 0; i <= 15; i++) {
		switch (Temp[15 - i] & 0x06) {
		case 0:
			j = Temp[i] | CRC;
			break;

		case 2:
		case 4:
			j = Temp[i] ^ CRC;
			break;

		case 6:
			j = Temp[i] & CRC;
			break;
		}

		if (j == CRC) j = 0x2c;

		if (Temp[i] == 0) j = 0;

		MagicResponse[i] = j;
	}
}

static gn_error IdentifyPhone(struct gn_statemachine *state)
{
	nk6100_driver_instance *drvinst = DRVINSTANCE(state);
	gn_error err;
	gn_data data;
	char revision[GN_REVISION_MAX_LENGTH];

	revision[0] = '\0';
	gn_data_clear(&data);
	data.model = drvinst->model;
	data.imei = drvinst->imei;
	data.revision = revision;

	if ((err = get_imei(&data, state)) != GN_ERR_NONE ||
	    (err = get_phone_info(&data, state)) != GN_ERR_NONE ||
	    (err = get_hw(&data, state)) != GN_ERR_NONE) {
		return err;
	}
	if ((drvinst->pm = gn_phone_model_get(data.model)) == NULL) {
		dump("Unsupported phone model \"%s\"\n", data.model);
		dump("Please read Docs/Bugs and send a bug report!\n");
		return GN_ERR_INTERNALERROR;
	}

	if (drvinst->pm->flags & PM_AUTHENTICATION) {
		/* Now test the link and authenticate ourself */
		if ((err = PhoneInfo(&data, state)) != GN_ERR_NONE) return err;
	}

	sscanf(revision, "SW %9[^ 	,], HW %9s", drvinst->sw_version, drvinst->hw_version);

	return GN_ERR_NONE;
}

static bool match_phone(nk6100_driver_instance *drvinst, int i)
{
	if (nk6100_capabilities[i].model != NULL && strcmp(nk6100_capabilities[i].model, drvinst->model))
		return false;

	if (nk6100_capabilities[i].sw_version != NULL) {
		if (nk6100_capabilities[i].sw_version[0] == '-' && strcmp(nk6100_capabilities[i].sw_version + 1, drvinst->sw_version) >= 0)
			return true;
		if (nk6100_capabilities[i].sw_version[0] == '+' && strcmp(nk6100_capabilities[i].sw_version + 1, drvinst->sw_version) <= 0)
			return true;
		if (!strcmp(nk6100_capabilities[i].sw_version, drvinst->sw_version))
			return true;
		return false;
	}

	return true;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error Initialise(struct gn_statemachine *state)
{
	gn_error err;
	int i;

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_nokia_6100, sizeof(gn_driver));

	if (!(DRVINSTANCE(state) = calloc(1, sizeof(nk6100_driver_instance))))
		return GN_ERR_MEMORYFULL;

	switch (state->config.connection_type) {
	case GN_CT_Serial:
		state->config.connection_type = GN_CT_DAU9P;
	case GN_CT_Infrared:
	case GN_CT_DAU9P:
	case GN_CT_Tekram:
	case GN_CT_TCP:
		err = fbus_initialise(0, state);
		break;
	case GN_CT_Irda:
		err = phonet_initialise(state);
		break;
	case GN_CT_M2BUS:
		err = m2bus_initialise(state);
		break;
	default:
		err = GN_ERR_NOTSUPPORTED;
	}

	if (err != GN_ERR_NONE) {
		dprintf("Error in link initialisation\n");
		gn_sm_functions(GN_OP_Terminate, NULL, state);
		return GN_ERR_NOTSUPPORTED;
	}

	sm_initialise(state);

	/* We need to identify the phone first in order to know whether we can
	   authorize or set keytable */

	if ((err = IdentifyPhone(state)) != GN_ERR_NONE) {
		gn_sm_functions(GN_OP_Terminate, NULL, state);
		return err;
	}

	for (i = 0; !match_phone(DRVINSTANCE(state), i); i++) ;
	DRVINSTANCE(state)->capabilities = nk6100_capabilities[i].capabilities;

	if (DRVINSTANCE(state)->pm->flags & PM_AUTHENTICATION) {
		/* Now test the link and authenticate ourself */
		if ((err = Authentication(state, DRVINSTANCE(state)->imei)) != GN_ERR_NONE) {
			gn_sm_functions(GN_OP_Terminate, NULL, state);
			return err;
		}
	}

	if (DRVINSTANCE(state)->pm->flags & PM_KEYBOARD) {
		if (DRVINSTANCE(state)->capabilities & NK6100_CAP_OLD_KEY_API) {
			/* FIXME: build a default table */
		} else {
			if (BuildKeytable(state) != GN_ERR_NONE) {
				gn_sm_functions(GN_OP_Terminate, NULL, state);
				return GN_ERR_NOTSUPPORTED;
			}
		}
	}

	if (!strcmp(DRVINSTANCE(state)->model, "RPM-1")) {
		state->driver.phone.max_battery_level = 1;
		DRVINSTANCE(state)->max_sms = 2;
	} else {
		DRVINSTANCE(state)->max_sms = NK6100_MAX_SMS_MESSAGES;
	}

	return GN_ERR_NONE;
}

static gn_error Identify(gn_data *data, struct gn_statemachine *state)
{
	nk6100_driver_instance *drvinst = DRVINSTANCE(state);

	if (data->manufacturer)
		pnok_manufacturer_get(data->manufacturer);
	if (data->model)
		snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", drvinst->model);
	if (data->imei)
		snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", drvinst->imei);
	if (data->revision)
		snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW %s", drvinst->sw_version, drvinst->hw_version);
	data->phone = drvinst->pm;

	return GN_ERR_NONE;
}


static gn_error GetPhoneStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};

	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_NO_PSTATUS)
		return GN_ERR_NOTSUPPORTED;

	dprintf("Getting phone status...\n");
	if (sm_message_send(4, 0x04, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x04, data, state);
}

static gn_error GetRFLevel(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting rf level...\n");
	return GetPhoneStatus(data, state);
}

static gn_error GetBatteryLevel(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting battery level...\n");
	return GetPhoneStatus(data, state);
}

static gn_error GetPowersource(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Getting power source...\n");
	return GetPhoneStatus(data, state);
}

#if 0 /* unused function */
static gn_error GetPhoneID(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03};

	dprintf("Getting phone id...\n");
	if (sm_message_send(4, 0x04, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x04, data, state);
}
#endif

static gn_error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	float csq_map[5] = {0, 8, 16, 24, 31};
	char hw[10], sw[10];

	switch (message[3]) {
	/* Phone status */
	case 0x02:
		dprintf("\tRFLevel: %d\n", message[5]);
		dprintf("\tPowerSource: %d\n", message[7]);
		dprintf("\tBatteryLevel: %d\n", message[8]);
		if (message[5] > 4) return GN_ERR_UNHANDLEDFRAME;
		if (message[7] != 1 && message[7] != 2) return GN_ERR_UNHANDLEDFRAME;
		if (data->rf_level && data->rf_unit) {
			switch (*data->rf_unit) {
			case GN_RF_CSQ:
				*data->rf_level = csq_map[message[5]];
				break;
			case GN_RF_Arbitrary:
			default:
				*data->rf_unit = GN_RF_Arbitrary;
				*data->rf_level = message[5];
				break;
			}
		}
		if (data->power_source) {
			*data->power_source = message[7];
		}
		if (data->battery_level && data->battery_unit) {
			*data->battery_unit = GN_BU_Arbitrary;
			*data->battery_level = message[8];
		}
		break;

	/* Get Phone ID */
	case 0x04:
		if (data->imei) {
			snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", message + 5);
			dprintf("Received imei %s\n", data->imei);
		}
		if (data->revision) {
			sscanf(message + 35, " %9s", hw);
			sscanf(message + 40, " %9s", sw);
			snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW %s", sw, hw);
			dprintf("Received revision %s\n", data->revision);
		}
		if (data->model) {
			snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", message + 21);
			dprintf("Received model %s\n", data->model);
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00};

	dprintf("Getting memory status...\n");
	req[4] = get_memory_type(data->memory_status->memory_type);
	if (sm_message_send(5, 0x03, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x03, data, state);
}

static gn_error ReadPhonebook(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00};

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	dprintf("Reading phonebook location (%d/%d)\n", data->phonebook_entry->memory_type, data->phonebook_entry->location);
	req[4] = get_memory_type(data->phonebook_entry->memory_type);
	req[5] = data->phonebook_entry->location;

	data->phonebook_entry->empty = true;

	if (sm_message_send(7, 0x03, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x03, data, state);
}

static gn_error WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[FBUS_FRAME_MAX_LENGTH] = {FBUS_FRAME_HEADER, 0x04};
	gn_phonebook_entry *pe;
	unsigned char *pos;
	int namelen, numlen;

	pe = data->phonebook_entry;
	pos = req+4;
	namelen = strlen(pe->name);
	numlen = strlen(pe->number);
	dprintf("Writing phonebook location (%d/%d): %s\n", pe->memory_type, pe->location, pe->name);
	if (namelen > GN_PHONEBOOK_NAME_MAX_LENGTH) {
		dprintf("name too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	if (numlen > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	if (pe->subentries_count > 1) {
		dprintf("61xx doesn't support subentries\n");
		return GN_ERR_UNKNOWN;
	}
	if ((pe->subentries_count == 1) && ((pe->subentries[0].entry_type != GN_PHONEBOOK_ENTRY_Number)
		|| ((pe->subentries[0].number_type != GN_PHONEBOOK_NUMBER_General)
		 && (pe->subentries[0].number_type != 0)) || (pe->subentries[0].id != 2)
		|| strcmp(pe->subentries[0].data.number, pe->number))) {
		dprintf("61xx doesn't support subentries\n");
		return GN_ERR_UNKNOWN;
	}
	*pos++ = get_memory_type(pe->memory_type);
	*pos++ = pe->location;
	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_PB_UNICODE)
		namelen = char_unicode_encode(pos + 1, pe->name, namelen);
	else
		namelen = pnok_string_encode(pos + 1, namelen, pe->name);
	*pos++ = namelen;
	pos += namelen;
	*pos++ = numlen;
	pnok_string_encode(pos, numlen, pe->number);
	pos += numlen;
	*pos++ = (pe->caller_group == GN_PHONEBOOK_GROUP_None) ? 0xff : pe->caller_group;
	if (sm_message_send(pos-req, 0x03, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x03, data, state);
}

static gn_error DeletePhonebook(gn_data *data, struct gn_statemachine *state)
{
	gn_data d;
	gn_phonebook_entry entry;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	gn_data_clear(&d);
	memset(&entry, 0, sizeof(entry));
	d.phonebook_entry = &entry;

	entry.location = data->phonebook_entry->location;
	entry.memory_type = data->phonebook_entry->memory_type;
	entry.caller_group = GN_PHONEBOOK_GROUP_None;

	return WritePhonebook(&d, state);
}

static gn_error GetCallerGroupData(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x00};

	req[4] = data->bitmap->number;
	if (sm_message_send(5, 0x03, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x03, data, state);
}

static gn_error GetBitmap(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Reading bitmap...\n");
	switch (data->bitmap->type) {
	case GN_BMP_StartupLogo:
		return GetWelcomeMessage(data, state);
	case GN_BMP_WelcomeNoteText:
		return GetWelcomeMessage(data, state);
	case GN_BMP_DealerNoteText:
		return GetWelcomeMessage(data, state);
	case GN_BMP_OperatorLogo:
		return GetOperatorLogo(data, state);
	case GN_BMP_CallerLogo:
		return GetCallerGroupData(data, state);

	case GN_BMP_None:
	case GN_BMP_PictureMessage:
		return GN_ERR_NOTSUPPORTED;
	default:
		return GN_ERR_INTERNALERROR;
	}
}

static gn_error GetSpeedDial(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x16, 0x00};

	req[4] = data->speed_dial->number;

	if (sm_message_send(5, 0x03, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x03, data, state);
}

static gn_error SetSpeedDial(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x19, 0x00, 0x00, 0x00};

	req[4] = data->speed_dial->number;
	req[5] = get_memory_type(data->speed_dial->memory_type);
	req[6] = data->speed_dial->location;

	if (sm_message_send(7, 0x03, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x03, data, state);
}

static gn_error IncomingPhonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_phonebook_entry *pe;
	gn_bmp *bmp;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	case 0x02:
		if (data->phonebook_entry) {
			pe = data->phonebook_entry;
			pos = message + 5;
			n = *pos++;
			/* It seems that older phones (at least Nokia 5110 and 6130)
			   set message[4] to 0. Newer ones set is to the location
			   number. It can be the distinction when to read the name */
			if ((message[4] != 0) || (DRVINSTANCE(state)->capabilities & NK6100_CAP_PB_UNICODE))
				char_unicode_decode(pe->name, pos, n);
			else
				pnok_string_decode(pe->name, sizeof(pe->name), pos, n);
			pos += n;
			n = *pos++;
			pnok_string_decode(pe->number, sizeof(pe->number), pos, n);
			pos += n;
			if (DRVINSTANCE(state)->capabilities & NK6100_CAP_NO_PB_GROUP) {
				pe->caller_group = GN_PHONEBOOK_GROUP_None;
				pos++;
			} else {
				pe->caller_group = *pos++;
			}
			if (*pos++) {
				/* date is set */
				pe->date.year = (pos[0] << 8) + pos[1];
				pos += 2;
				pe->date.month = *pos++;
				pe->date.day = *pos++;
				pe->date.hour = *pos++;
				pe->date.minute = *pos++;
				pe->date.second = *pos++;
			} else {
				/* date is not set */
				pe->date.year = 0;
				pe->date.month = 0;
				pe->date.day = 0;
				pe->date.hour = 0;
				pe->date.minute = 0;
				pe->date.second = 0;
			}
			pe->subentries_count = 0;
			pe->empty = (pe->name[0] == '\0') && (pe->number[0] == '\0');
		}
		break;
	case 0x03:
		switch (message[4]) {
		case 0x6f: /* This gets returned when issuing --getphonebook
			    * command and the phone displays 'Insert SIM card'
			    * on Nokia 3330 (Daniele Forsi).
			    */
			return GN_ERR_NOTREADY;
		case 0x74:
		case 0x7d:
			return GN_ERR_INVALIDLOCATION;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
	case 0x05:
		break;
	case 0x06:
		switch (message[4]) {
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x7d:
		case 0x90:
			return GN_ERR_ENTRYTOOLONG;
		case 0x74:
			return GN_ERR_INVALIDLOCATION;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
	case 0x08:
		dprintf("\tMemory location: %d\n", message[4]);
		dprintf("\tUsed: %d\n", message[6]);
		dprintf("\tFree: %d\n", message[5]);
		if (data->memory_status) {
			data->memory_status->used = message[6];
			data->memory_status->free = message[5];
			return GN_ERR_NONE;
		}
		break;
	case 0x09:
		switch (message[4]) {
		case 0x6f:
			return GN_ERR_TIMEOUT;
		case 0x7d:
			return GN_ERR_INVALIDMEMORYTYPE;
		case 0x8d:
			return GN_ERR_INVALIDSECURITYCODE;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
	case 0x11:
		if (data->bitmap) {
			bmp = data->bitmap;
			pos = message+4;
			bmp->number = *pos++;
			n = *pos++;
			pnok_string_decode(bmp->text, sizeof(bmp->text), pos, n);
			pos += n;
			bmp->ringtone = *pos++;
			pos++;
			bmp->size = (pos[0] << 8) + pos[1];
			pos += 2;
			pos++;
			bmp->width = *pos++;
			bmp->height = *pos++;
			pos++;
			n = bmp->height * bmp->width / 8;
			if (bmp->size > n) bmp->size = n;
			if (bmp->size > sizeof(bmp->bitmap))
				return GN_ERR_UNHANDLEDFRAME;
			memcpy(bmp->bitmap, pos, bmp->size);
			if (bmp->text[0] == '\0') {
				switch (bmp->number) {
				case 0:
					snprintf(bmp->text, sizeof(bmp->text), "%s", _("Family"));
					break;
				case 1:
					snprintf(bmp->text, sizeof(bmp->text), "%s", _("VIP"));
					break;
				case 2:
					snprintf(bmp->text, sizeof(bmp->text), "%s", _("Friends"));
					break;
				case 3:
					snprintf(bmp->text, sizeof(bmp->text), "%s", _("Colleagues"));
					break;
				case 4:
					snprintf(bmp->text, sizeof(bmp->text), "%s", _("Other"));
					break;
				default:
					break;
				}
			}
		}
		break;
	case 0x12:
		switch (message[4]) {
		case 0x7d:
			return GN_ERR_INVALIDLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
	case 0x14:
		break;
	case 0x15:
		switch (message[4]) {
		case 0x7d:
			return GN_ERR_INVALIDLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* Get speed dial OK */
	case 0x17:
		if (data->speed_dial) {
			switch (message[4]) {
			case NK6100_MEMORY_ME:
				data->speed_dial->memory_type = GN_MT_ME;
				break;
			case NK6100_MEMORY_SM:
				data->speed_dial->memory_type = GN_MT_SM;
				break;
			default:
				return GN_ERR_UNHANDLEDFRAME;
			}
			data->speed_dial->location = message[5];
		}
		break;

	/* Get speed dial error */
	case 0x18:
		return GN_ERR_INVALIDLOCATION;

	/* Set speed dial OK */
	case 0x1a:
		return GN_ERR_NONE;

	/* Set speed dial error */
	case 0x1b:
		switch (message[4]) {
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x74:
			return GN_ERR_INVALIDLOCATION;
		case 0x7d:
			return GN_ERR_INVALIDMEMORYTYPE;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error PhoneInfo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10};

	dprintf("Getting phone info (new way)...\n");

	if (sm_message_send(4, 0x64, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x64, data, state);
}

static gn_error Authentication(struct gn_statemachine *state, char *imei)
{
	gn_error error;
	gn_data data;
	unsigned char connect1[] = {FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};
	unsigned char connect2[] = {FBUS_FRAME_HEADER, 0x20, 0x02};
	unsigned char connect3[] = {FBUS_FRAME_HEADER, 0x0d, 0x01, 0x00, 0x02};

	unsigned char magic_connect[] = {FBUS_FRAME_HEADER, 0x12,
					 /* auth response */
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 'N', 'O', 'K', 'I', 'A', '&', 'N', 'O', 'K', 'I', 'A',
					 'a', 'c', 'c', 'e', 's', 's', 'o', 'r', 'y',
					 0x00, 0x00, 0x00, 0x00};

	gn_data_clear(&data);

	if ((error = sm_message_send(7, 0x02, connect1, state)) != GN_ERR_NONE)
		return error;
	if ((error = sm_block(0x02, &data, state)) != GN_ERR_NONE)
		return error;

	if ((error = sm_message_send(5, 0x02, connect2, state)) != GN_ERR_NONE)
		return error;
	if ((error = sm_block(0x02, &data, state)) != GN_ERR_NONE)
		return error;

	if ((error = sm_message_send(7, 0x02, connect3, state)) != GN_ERR_NONE)
		return error;
	if ((error = sm_block(0x02, &data, state)) != GN_ERR_NONE)
		return error;

	if ((error = PhoneInfo(&data, state)) != GN_ERR_NONE) return error;

	PNOK_GetNokiaAuth(imei, DRVINSTANCE(state)->magic_bytes, magic_connect + 4);

	if ((error = sm_message_send(45, 0x64, magic_connect, state)) != GN_ERR_NONE)
		return error;

	return GN_ERR_NONE;
}

static gn_error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	char hw[10], sw[10];

	switch (message[3]) {
	/* Phone ID recvd */
	case 0x11:
		if (data->imei) {
			snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", message + 9);
			dprintf("Received imei %s\n", data->imei);
		}
		if (data->model) {
			snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", message + 25);
			dprintf("Received model %s\n", data->model);
		}
		if (data->revision) {
			sscanf(message + 39, " %9s", hw);
			sscanf(message + 44, " %9s", sw);
			snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW %s", sw, hw);
			dprintf("Received revision %s\n", data->revision);
		}

		dprintf("Message: Mobile phone identification received:\n");
		dprintf("\tIMEI: %s\n", message + 9);
		dprintf("\tModel: %s\n", message + 25);
		dprintf("\tProduction Code: %s\n", message + 31);
		dprintf("\tHW: %s\n", message + 39);
		dprintf("\tFirmware: %s\n", message + 44);

		/* These bytes are probably the source of the "Accessory not connected"
		   messages on the phone when trying to emulate NCDS... I hope....
		   UPDATE: of course, now we have the authentication algorithm. */
		dprintf("\tMagic bytes: %02x %02x %02x %02x\n", message[50], message[51], message[52], message[53]);

		memcpy(DRVINSTANCE(state)->magic_bytes, message + 50, 4);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


#if 0 /* unused function */
static gn_error PhoneInfo2(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x03, 0x00 };

	dprintf("Getting phone info (old way)...\n");

	if (sm_message_send(5, 0xd1, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0xd2, data, state);

}
#endif

static gn_error PressOrReleaseKey2(bool press, gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x46, 0x00, 0x01, 0x00};

	req[2] = press ? 0x46 : 0x47;
	req[5] = data->key_code;

	if (sm_message_send(6, 0xd1, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0xd2, data, state);
}

static gn_error IncomingPhoneInfo2(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	char sw[10];

	switch (message[2]) {
	/* phone id2 received */
	case 0x03:
		if (data->model) {
			snprintf(data->model, 6, "%s", message + 21);
		}
		if (data->revision) {
			sscanf(message + 6, " %9s", sw);
			snprintf(data->revision, GN_REVISION_MAX_LENGTH, "SW %s, HW ????", sw);
		}
		dprintf("Phone info:\n%s\n", message + 4);
		break;
	/* press key result */
	case 0x46:
		if (message[3] != 0x00) return GN_ERR_UNHANDLEDFRAME;
		break;
	/* release key result */
	case 0x47:
		if (message[3] != 0x00) return GN_ERR_UNHANDLEDFRAME;
		break;
	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static int get_memory_type(gn_memory_type memory_type)
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


static gn_error GetSMSCenter(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x33, 0x64, 0x00};
	int id;

	id = data->message_center->id;
	if ((id < 1) || (id > 255)) return GN_ERR_INVALIDLOCATION;

	req[5] = id;

	if (sm_message_send(6, 0x02, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x02, data, state);
}

static gn_error SetSMSCenter(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[64] = {FBUS_FRAME_HEADER, 0x30, 0x64};
	gn_sms_message_center *smsc;
	unsigned char *pos;

	smsc = data->message_center;
	if ((smsc->id < 1) || (smsc->id > 255))
		return GN_ERR_INVALIDLOCATION;

	pos = req+5;
	*pos++ = smsc->id;
	pos++;
	*pos++ = smsc->format;
	pos++;
	*pos++ = smsc->validity;
	*pos = (char_semi_octet_pack(smsc->recipient.number, pos + 1, smsc->recipient.type) + 1) / 2 + 1;
	pos += 12;
	*pos = (char_semi_octet_pack(smsc->smsc.number, pos + 1, smsc->smsc.type) + 1) / 2 + 1;
	pos += 12;
	if (smsc->default_name < 1) {
		snprintf(pos, 13, "%s", smsc->name);
		pos += strlen(pos)+1;
	} else
		*pos++ = 0;

	if (sm_message_send(pos-req, 0x02, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x02, data, state);
}

static gn_error SetCellBroadcast(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req_ena[] = {FBUS_FRAME_HEADER, 0x20, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01};
	unsigned char req_dis[] = {FBUS_FRAME_HEADER, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char *req;

	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_NO_CB)
		return GN_ERR_NOTSUPPORTED;

	req = data->on_cell_broadcast ? req_ena : req_dis;
	DRVINSTANCE(state)->on_cell_broadcast = data->on_cell_broadcast;
	DRVINSTANCE(state)->cb_callback_data = data->callback_data;

	if (sm_message_send(10, 0x02, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x02, data, state);
}

static gn_error SendSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01, 0x02, 0x00};
	gn_data dtemp;
	gn_error error;
	int len;

	/*
	 * FIXME:
	 * Ugly hack. The phone seems to drop the SendSMS message if the link
	 * had been idle too long. -- bozo
	 */
	gn_data_clear(&dtemp);
	GetNetworkInfo(&dtemp, state);


	len = pnok_fbus_sms_encode(req + 6, data, state);
	len += 6;

	if (sm_message_send(len, PNOK_MSG_ID_SMS, req, state)) return GN_ERR_NOTREADY;
	do {
		error = sm_block_no_retry_timeout(PNOK_MSG_ID_SMS, state->config.smsc_timeout, data, state);
	} while (!state->config.smsc_timeout && error == GN_ERR_TIMEOUT);

	return error;
}

static bool CheckIncomingSMS(struct gn_statemachine *state, int pos)
{
	gn_data data;
	gn_sms sms;
	gn_sms_raw rawsms;
	gn_error error;

	if (!DRVINSTANCE(state)->on_sms) {
		return false;
	}

	/*
	 * libgnokii isn't reentrant anyway, so this simple trick should be
	 * enough - bozo
	 */
	if (DRVINSTANCE(state)->sms_notification_in_progress) {
		DRVINSTANCE(state)->sms_notification_lost = true;
		return false;
	}
	DRVINSTANCE(state)->sms_notification_in_progress = true;

	memset(&sms, 0, sizeof(sms));
	memset(&rawsms, 0, sizeof(rawsms));
	rawsms.memory_type = sms.memory_type = GN_MT_SM;
	rawsms.number = sms.number = pos;
	gn_data_clear(&data);
	data.sms = &sms;

	dprintf("trying to fetch sms#%hd\n", sms.number);
	if ((error = gn_sms_get(&data, state)) != GN_ERR_NONE) {
		DRVINSTANCE(state)->sms_notification_in_progress = false;
		return false;
	}

	DRVINSTANCE(state)->on_sms(&sms, state, DRVINSTANCE(state)->sms_callback_data);

	dprintf("deleting sms#%hd\n", sms.number);
	gn_data_clear(&data);
	data.sms = &sms;
	data.raw_sms = &rawsms;
	rawsms.memory_type = sms.memory_type;
	rawsms.number = sms.number = pos;
	DeleteSMSMessage(&data, state);

	DRVINSTANCE(state)->sms_notification_in_progress = false;

	return true;
}

static void FlushLostSMSNotifications(struct gn_statemachine *state)
{
	int i;

	while (!DRVINSTANCE(state)->sms_notification_in_progress && DRVINSTANCE(state)->sms_notification_lost) {
		DRVINSTANCE(state)->sms_notification_lost = false;
		for (i = 1; i <= DRVINSTANCE(state)->max_sms; i++)
			CheckIncomingSMS(state, i);
	}
}

static gn_error SetOnSMS(gn_data *data, struct gn_statemachine *state)
{
	if (data->on_sms) {
		DRVINSTANCE(state)->on_sms = data->on_sms;
		DRVINSTANCE(state)->sms_callback_data = data->callback_data;
		DRVINSTANCE(state)->sms_notification_lost = true;
		FlushLostSMSNotifications(state);
	} else {
		DRVINSTANCE(state)->on_sms = NULL;
		DRVINSTANCE(state)->sms_callback_data = NULL;
	}

	return GN_ERR_NONE;
}

static gn_error IncomingSMS1(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_sms_message_center *smsc;
	gn_cb_message cbmsg;
	unsigned char *pos;
	int n;
	gn_error error;

	switch (message[3]) {
	/* Message sent */
	case 0x02:
		return GN_ERR_NONE;

	/* Send failed */
	case 0x03:
		/*
		 * The marked part seems to be an ISDN cause code -- bozo
		 * 01 08 00 03 64 [01 32] 00
		 */
		error = isdn_cause2gn_error(NULL, NULL, message[5], message[6]);
		switch (error) {
		case GN_ERR_UNKNOWN:
			return GN_ERR_FAILED;
		default:
			return error;
		}

	/* SMS message received */
	case 0x10:
		dprintf("SMS received, location: %d\n", message[5]);
		CheckIncomingSMS(state, message[5]);
		FlushLostSMSNotifications(state);
		return GN_ERR_UNSOLICITED;

	/* FIXME: unhandled frame, request: Get HW&SW version !!! */
	case 0x0e:
		if (length == 4) return GN_ERR_NONE;
		return GN_ERR_UNHANDLEDFRAME;

	/* Set CellBroadcast OK */
	case 0x21:
		dprintf("Setting CellBroadcast successful\n");
		break;

	/* Set CellBroadcast error */
	case 0x22:
		dprintf("Setting CellBroadcast failed\n");
		break;

	/* Read CellBroadcast */
	case 0x23:
		if (DRVINSTANCE(state)->on_cell_broadcast) {
			char *aux;

			memset(&cbmsg, 0, sizeof(cbmsg));
			cbmsg.is_new = true;
			cbmsg.channel = message[7];
			aux = calloc(sizeof(cbmsg.message), 1);
			n = char_7bit_unpack(0, length-10, sizeof(cbmsg.message)-1, message+10, aux);
			char_default_alphabet_decode(cbmsg.message, aux, n);
			free(aux);
			DRVINSTANCE(state)->on_cell_broadcast(&cbmsg, state, DRVINSTANCE(state)->cb_callback_data);
		}
		return GN_ERR_UNSOLICITED;

	/* Set SMS center OK */
	case 0x31:
		break;

	/* Set SMS center error */
	case 0x32:
		switch (message[4]) {
		case 0x02:
			return GN_ERR_EMPTYLOCATION;
		case 0x06: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x0c: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		return GN_ERR_UNHANDLEDFRAME;

	/* SMS center received */
	case 0x34:
		if (data->message_center) {
			smsc = data->message_center;
			pos = message + 4;
			smsc->id = *pos++;
			pos++;
			smsc->format = *pos++;
			pos++;
			smsc->validity = *pos++;
			if (pos[0] % 2) pos[0]++;
			pos[0] = pos[0] / 2 + 1;
			snprintf(smsc->recipient.number, sizeof(smsc->recipient.number), "%s", char_bcd_number_get(pos));
			smsc->recipient.type = pos[1];
			pos += 12;
			snprintf(smsc->smsc.number, sizeof(smsc->smsc.number), "%s", char_bcd_number_get(pos));
			smsc->smsc.type = pos[1];
			pos += 12;
			/* FIXME: codepage must be investigated - bozo */
			if (pos[0] == 0x00) {
				snprintf(smsc->name, sizeof(smsc->name), _("Set %d"), smsc->id);
				smsc->default_name = smsc->id;
			} else {
				snprintf(smsc->name, sizeof(smsc->name), "%s", pos);
				smsc->default_name = -1;
			}
		}
		break;

	/* SMS center error recv */
	case 0x35:
		switch (message[4]) {
		case 0x01:
			return GN_ERR_EMPTYLOCATION;
		case 0x06: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x0c: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	case 0xc9:
		dprintf("Still waiting....\n");
		return GN_ERR_UNSOLICITED;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetSMSStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x36, 0x64 };

	if (sm_message_send(5, 0x14, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x14, data, state);
}

static gn_error GetSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x07, 0x02 /* Unknown */, 0x00 /* Location */, 0x01, 0x64 };

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	if (data->raw_sms->memory_type != GN_MT_SM) return GN_ERR_INVALIDMEMORYTYPE;
	if ((data->raw_sms->number < 0) || (data->raw_sms->number > 255)) return GN_ERR_INVALIDLOCATION;

	req[5] = data->raw_sms->number;
	if (sm_message_send(8, 0x02, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x14, data, state);
}

static gn_error SaveSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x04,
				  0x07, /* status */
				  0x02,
				  0x00, /* number */
				  0x02 }; /* type */
	int len;

	if (!data->raw_sms) return GN_ERR_INTERNALERROR;
	if ((data->raw_sms->number < 0) || (data->raw_sms->number > 255)) return GN_ERR_INVALIDLOCATION;

	if (44 + data->raw_sms->user_data_length > sizeof(req))
		return GN_ERR_WRONGDATAFORMAT;

	if (data->raw_sms->type == GN_SMS_MT_Deliver) {	/* Inbox */
		dprintf("INBOX!\n");
		req[4]		= 0x03;			/* SMS State - GN_SMS_Unread */
		req[7]		= 0x00;			/* SMS Type */
	}

	if (data->raw_sms->status == GN_SMS_Sent)
		req[4] -= 0x02;

	req[6] = data->raw_sms->number;

#if 0
	req[20] = 0x01; /* SMS Submit */
	if (data->raw_sms->udh_indicator)		req[20] |= 0x40;
	if (data->raw_sms->validity_indicator)	req[20] |= 0x10;

	req[24] = data->raw_sms->user_data_length;
	memcpy(req + 37, data->raw_sms->validity, 7);
	memcpy(req + 44, data->raw_sms->dser_data, data->raw_sms->user_data_length);
#endif
	len = pnok_fbus_sms_encode(req + 8, data, state);
	len += 8;

	if (sm_message_send(len, 0x14, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x14, data, state);
}

static gn_error DeleteSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x0a, 0x02, 0x00 /* Location */ };

	if (!data->sms) return GN_ERR_INTERNALERROR;
	if (data->sms->memory_type != GN_MT_SM) return GN_ERR_INVALIDMEMORYTYPE;
	if ((data->raw_sms->number < 0) || (data->raw_sms->number > 255)) return GN_ERR_INVALIDLOCATION;

	req[5] = data->sms->number;
	if (sm_message_send(6, 0x14, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x14, data, state);
}

static gn_error GetSMSFolders(gn_data *data, struct gn_statemachine *state)
{
	if (!data || !data->sms_folder_list)
		return GN_ERR_INTERNALERROR;

	memset(data->sms_folder_list, 0, sizeof(gn_sms_folder_list));

	data->sms_folder_list->number = 1;
	snprintf(data->sms_folder_list->folder[0].name, sizeof(data->sms_folder_list->folder[0].name), "%s", gn_memory_type_print(GN_MT_SM));
	data->sms_folder_list->folder_id[0] = GN_MT_SM;
	data->sms_folder_list->folder[0].folder_id = NK6100_MEMORY_SM;

	return GN_ERR_NONE;
}

static gn_error GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_sms_status smsstatus = {0, 0, 0, 0}, *save_smsstatus;

	if (!data || !data->sms_folder)
		return GN_ERR_INTERNALERROR;
	if (data->sms_folder->folder_id != NK6100_MEMORY_SM)
		return GN_ERR_INVALIDMEMORYTYPE;

	error = GetSMSFolders(data, state);
	if (error != GN_ERR_NONE) return error;

	save_smsstatus = data->sms_status;
	data->sms_status = &smsstatus;
	error = GetSMSStatus(data, state);
	data->sms_status = save_smsstatus;
	if (error != GN_ERR_NONE) return error;

	data->sms_folder->number = smsstatus.number;

	return GN_ERR_NONE;
}

static gn_error IncomingSMS(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i;

	switch (message[3]) {
	/* save sms succeeded */
	case 0x05:
		dprintf("Message stored at %d\n", message[5]);
		if (data->raw_sms) data->raw_sms->number = message[5];
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
		case 0x06: /* Insert SIM card */
			dprintf("\tInsert SIM card!\n");
			return GN_ERR_NOTREADY;
		case 0x0c: /* waiting for PIN */
			dprintf("\tPIN or PUK code required.\n");
			return GN_ERR_CODEREQUIRED;
		default:
			dprintf("\tUnknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* read sms */
	case 0x08:
		for (i = 0; i < length; i++)
			dprintf("[%02x(%d)]", message[i], i);
		dprintf("\n");

		if (!data->raw_sms) return GN_ERR_INTERNALERROR;

		memset(data->raw_sms, 0, sizeof(gn_sms_raw));

#define	getdata(d,dr,s)	(message[(data->raw_sms->type == GN_SMS_MT_Deliver) ? (d) : ((data->raw_sms->type == GN_SMS_MT_StatusReport) ? (dr) : (s))])
		switch (message[7]) {
		case 0x00: data->raw_sms->type	= GN_SMS_MT_Deliver; break;
		case 0x01: data->raw_sms->type	= GN_SMS_MT_StatusReport; break;
		case 0x02: data->raw_sms->type	= GN_SMS_MT_Submit; break;
		default: return GN_ERR_UNHANDLEDFRAME;
		}
		data->raw_sms->number		= message[6];
		data->raw_sms->memory_type	= GN_MT_SM;
		data->raw_sms->status		= message[4];

		data->raw_sms->dcs		= getdata(22, 21, 23);
		data->raw_sms->length		= getdata(23, 22, 24);
		data->raw_sms->udh_indicator	= message[20];
		data->raw_sms->user_data_length = data->raw_sms->length;
		if (data->raw_sms->udh_indicator & 0x40)
			data->raw_sms->user_data_length -= message[getdata(24, 23, 25)] + 1;
		memcpy(data->raw_sms->user_data, &getdata(43, 22, 44), data->raw_sms->length);

		if (data->raw_sms->type == GN_SMS_MT_StatusReport) {
			data->raw_sms->reply_via_same_smsc = message[11];
			memcpy(data->raw_sms->time, message + 42, 7);
			data->raw_sms->report_status = message[22];
		}
		if (data->raw_sms->type != GN_SMS_MT_Submit) {
			memcpy(data->raw_sms->smsc_time, &getdata(36, 35, 0), 7);
		}
		memcpy(data->raw_sms->message_center, message + 8, 12);
		memcpy(data->raw_sms->remote_number, &getdata(24, 23, 25), 12);
#undef getdata
		break;

	/* read sms failed */
	case 0x09:
		dprintf("SMS reading failed:\n");
		switch (message[4]) {
		case 0x00:
			dprintf("\tUnknown reason!\n");
			return GN_ERR_UNKNOWN;
		case 0x02:
			dprintf("\tInvalid location!\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x06:
			dprintf("\tInsert SIM card!\n");
			return GN_ERR_NOTREADY;
		case 0x07:
			dprintf("\tEmpty SMS location.\n");
			return GN_ERR_EMPTYLOCATION;
		case 0x0c: /* waiting for PIN */
			dprintf("\tPIN or PUK code required.\n");
			return GN_ERR_CODEREQUIRED;
		default:
			dprintf("\tUnknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* delete sms succeeded */
	case 0x0b:
		dprintf("Message: SMS deleted successfully.\n");
		break;

	/* delete sms failed */
	case 0x0c:
		switch (message[4]) {
		case 0x00:
			return GN_ERR_UNKNOWN;
		case 0x02:
			return GN_ERR_INVALIDLOCATION;
		case 0x06: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x0c: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* sms status succeded */
	case 0x37:
		dprintf("Message: SMS Status Received\n");
		dprintf("\tThe number of messages: %d\n", message[10]);
		dprintf("\tUnread messages: %d\n", message[11]);
		if (!data->sms_status) return GN_ERR_INTERNALERROR;
		data->sms_status->unread = message[11];
		data->sms_status->number = message[10];
		break;

	/* sms status failed */
	case 0x38:
		switch (message[4]) {
		case 0x06: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x0c: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* unknown */
	default:
		dprintf("Unknown message.\n");
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}


static gn_error GetNetworkInfo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x70 };

	if (sm_message_send(4, 0x0a, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x0a, data, state);
}

static gn_error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	/* Network info */
	case 0x71:
		dprintf("Message: Network Info Received\n");
		if (message[7] >= 9) {
			if (data->network_info) {
				data->network_info->cell_id[0] = message[10];
				data->network_info->cell_id[1] = message[11];
				data->network_info->LAC[0] = message[12];
				data->network_info->LAC[1] = message[13];
				data->network_info->network_code[0] = '0' + (message[14] & 0x0f);
				data->network_info->network_code[1] = '0' + (message[14] >> 4);
				data->network_info->network_code[2] = '0' + (message[15] & 0x0f);
				data->network_info->network_code[3] = ' ';
				data->network_info->network_code[4] = '0' + (message[16] & 0x0f);
				data->network_info->network_code[5] = '0' + (message[16] >> 4);
				data->network_info->network_code[6] = 0;
			}
		} else if (message[7] >= 2) {
			/* This can happen if handset has not (yet) registered with network (e.g. waiting for PIN code) */
			/* FIXME: print error messages instead of numeric codes, see Docs/protocol/nk6110.txt */
			dprintf("netstatus 0x%02x netsel 0x%02x\n", message[8], message[9]);
			return GN_ERR_NOTAVAILABLE;
		} else {
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetWelcomeMessage(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x16};

	if (sm_message_send(4, 0x05, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x05, data, state);
}

static gn_error GetOperatorLogo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x33, 0x01};

	req[4] = data->bitmap->number;
	if (sm_message_send(5, 0x05, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x05, data, state);
}

static gn_error SetBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512 + GN_BMP_MAX_SIZE] = {FBUS_FRAME_HEADER};
	gn_bmp *bmp;
	unsigned char *pos;
	int len;

	bmp = data->bitmap;
	pos = req+3;

	switch (bmp->type) {
	case GN_BMP_WelcomeNoteText:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("WelcomeNoteText is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x02;
		*pos = pnok_string_encode(pos+1, len, bmp->text);
		pos += *pos+1;
		if (sm_message_send(pos-req, 0x05, req, state)) return GN_ERR_NOTREADY;
		return sm_block(0x05, data, state);

	case GN_BMP_DealerNoteText:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("DealerNoteText is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x03;
		*pos = pnok_string_encode(pos+1, len, bmp->text);
		pos += *pos+1;
		if (sm_message_send(pos-req, 0x05, req, state)) return GN_ERR_NOTREADY;
		return sm_block(0x05, data, state);

	case GN_BMP_StartupLogo:
		if (bmp->size > GN_BMP_MAX_SIZE) {
			dprintf("StartupLogo is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x01;
		*pos++ = bmp->height;
		*pos++ = bmp->width;
		memcpy(pos, bmp->bitmap, bmp->size);
		pos += bmp->size;
		if (sm_message_send(pos-req, 0x05, req, state)) return GN_ERR_NOTREADY;
		return sm_block(0x05, data, state);

	case GN_BMP_OperatorLogo:
		if (bmp->size > GN_BMP_MAX_SIZE) {
			dprintf("OperatorLogo is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		if (DRVINSTANCE(state)->capabilities & NK6100_CAP_NBS_UPLOAD)
			return NBSUpload(data, state, GN_SMS_DATA_Bitmap);
		*pos++ = 0x30;	/* Store Op Logo */
		*pos++ = 0x01;	/* location */
		*pos++ = ((bmp->netcode[1] & 0x0f) << 4) | (bmp->netcode[0] & 0x0f);
		*pos++ = 0xf0 | (bmp->netcode[2] & 0x0f);
		*pos++ = ((bmp->netcode[5] & 0x0f) << 4) | (bmp->netcode[4] & 0x0f);
		*pos++ = (bmp->size + 4) >> 8;
		*pos++ = (bmp->size + 4) & 0xff;
		*pos++ = 0x00;	/* infofield */
		*pos++ = bmp->width;
		*pos++ = bmp->height;
		*pos++ = 0x01;	/* Just BW */
		memcpy(pos, bmp->bitmap, bmp->size);
		pos += bmp->size;
		if (sm_message_send(pos-req, 0x05, req, state)) return GN_ERR_NOTREADY;
		return sm_block(0x05, data, state);

	case GN_BMP_CallerLogo:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("Callergroup name is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		if (bmp->size > GN_BMP_MAX_SIZE) {
			dprintf("CallerLogo is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		*pos++ = 0x13;
		*pos++ = bmp->number;
		*pos = pnok_string_encode(pos+1, len, bmp->text);
		pos += *pos+1;
		*pos++ = bmp->ringtone;
		*pos++ = 0x01;	/* Graphic on. You can use other values as well:
				   0x00 - Off
				   0x01 - On
				   0x02 - View Graphics
				   0x03 - Send Graphics
				   0x04 - Send via IR
				   You can even set it higher but Nokia phones (my
				   6110 at least) will not show you the name of this
				   item in menu ;-)) Nokia is really joking here. */
		*pos++ = (bmp->size + 4) >> 8;
		*pos++ = (bmp->size + 4) & 0xff;
		*pos++ = 0x00;	/* Future extensions! */
		*pos++ = bmp->width;
		*pos++ = bmp->height;
		*pos++ = 0x01;	/* Just BW */
		memcpy(pos, bmp->bitmap, bmp->size);
		pos += bmp->size;
		if (sm_message_send(pos-req, 0x03, req, state)) return GN_ERR_NOTREADY;
		return sm_block(0x03, data, state);

	case GN_BMP_None:
	case GN_BMP_PictureMessage:
		return GN_ERR_NOTSUPPORTED;
	default:
		return GN_ERR_INTERNALERROR;
	}
}

static gn_error GetProfileFeature(int id, gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x13, 0x01, 0x00, 0x00};

	req[5] = data->profile->number;
	req[6] = (unsigned char)id;

	if (sm_message_send(7, 0x05, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x05, data, state);
}

static gn_error SetProfileFeature(gn_data *data, struct gn_statemachine *state, int id, int value)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01};

	req[5] = data->profile->number;
	req[6] = (unsigned char)id;
	req[7] = (unsigned char)value;
	dprintf("Setting profile %d feature %d to %d\n", req[5], req[6], req[7]);

	if (sm_message_send(9, 0x05, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x05, data, state);
}

static gn_error GetProfile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x1a, 0x00};
	gn_profile *prof;
	gn_error error;
	int i;

	if (!data->profile)
		return GN_ERR_UNKNOWN;
	prof = data->profile;
	req[4] = prof->number;

	if (sm_message_send(5, 0x05, req, state)) return GN_ERR_NOTREADY;
	if ((error = sm_block(0x05, data, state)))
		return error;

	for (i = 0; i <= 0x09; i++) {
		if ((error = GetProfileFeature(i, data, state)))
			return error;
	}

	if (prof->default_name > -1) {
		/* For N5110 */
		/* FIXME: It should be set for N5130 and 3210 too */
		if (!strcmp(DRVINSTANCE(state)->model, "NSE-1")) {
			switch (prof->default_name) {
			case 0x00:
				snprintf(prof->name, sizeof(prof->name), _("Personal"));
				break;
			case 0x01:
				snprintf(prof->name, sizeof(prof->name), _("Car"));
				break;
			case 0x02:
				snprintf(prof->name, sizeof(prof->name), _("Headset"));
				break;
			default:
				snprintf(prof->name, sizeof(prof->name), _("Unknown (%d)"), prof->default_name);
				break;
			}
		} else {
			switch (prof->default_name) {
			case 0x00:
				snprintf(prof->name, sizeof(prof->name), _("General"));
				break;
			case 0x01:
				snprintf(prof->name, sizeof(prof->name), _("Silent"));
				break;
			case 0x02:
				snprintf(prof->name, sizeof(prof->name), _("Meeting"));
				break;
			case 0x03:
				snprintf(prof->name, sizeof(prof->name), _("Outdoor"));
				break;
			case 0x04:
				snprintf(prof->name, sizeof(prof->name), _("Pager"));
				break;
			case 0x05:
				snprintf(prof->name, sizeof(prof->name), _("Car"));
				break;
			case 0x06:
				snprintf(prof->name, sizeof(prof->name), _("Headset"));
				break;
			default:
				snprintf(prof->name, sizeof(prof->name), _("Unknown (%d)"), prof->default_name);
				break;
			}
		}
	}

	return GN_ERR_NONE;
}

static gn_error SetProfile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[64] = {FBUS_FRAME_HEADER, 0x1c, 0x01, 0x03};
	gn_profile *prof;
	gn_error error;

	if (!data->profile)
		return GN_ERR_UNKNOWN;
	prof = data->profile;
	dprintf("Setting profile %d (%s)\n", prof->number, prof->name);

	if (prof->number == 0) {
		/* We cannot rename the General profile! - bozo */

		/*
		 * FIXME: We must to do something. We cannot ask the name
		 * of the General profile, because it's language dependent.
		 * But without SetProfileName we aren't able to set features.
		 */
		dprintf("You cannot rename General profile\n");
		return GN_ERR_NOTSUPPORTED;
	} else if (prof->default_name > -1) {
		prof->name[0] = 0;
	}

	req[7] = prof->number;
	req[8] = pnok_string_encode(req+9, 39, prof->name);
	req[6] = req[8] + 2;

	if (sm_message_send(req[8]+9, 0x05, req, state)) return GN_ERR_NOTREADY;
	if ((error = sm_block(0x05, data, state)))
		return error;

	error  = SetProfileFeature(data, state, 0x00, prof->keypad_tone);
	error |= SetProfileFeature(data, state, 0x01, prof->lights);
	error |= SetProfileFeature(data, state, 0x02, prof->call_alert);
	error |= SetProfileFeature(data, state, 0x03, prof->ringtone);
	error |= SetProfileFeature(data, state, 0x04, prof->volume);
	error |= SetProfileFeature(data, state, 0x05, prof->message_tone);
	error |= SetProfileFeature(data, state, 0x06, prof->vibration);
	error |= SetProfileFeature(data, state, 0x07, prof->warning_tone);
	error |= SetProfileFeature(data, state, 0x08, prof->caller_groups);
	error |= SetProfileFeature(data, state, 0x09, prof->automatic_answer);

	return (error == GN_ERR_NONE) ? GN_ERR_NONE : GN_ERR_UNKNOWN;
}

static gn_error GetRingtone(gn_data *data, struct gn_statemachine *state)
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

	if ((err = GetRawRingtone(&d, state)) != GN_ERR_NONE) return err;

	return pnok_ringtone_from_raw(data->ringtone, rawdata.data, rawdata.length);
}

static gn_error SetRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[7 + GN_RINGTONE_PACKAGE_MAX_LENGTH] = {FBUS_FRAME_HEADER, 0x36, 0x00, 0x00, 0x78};
	int size;

	if (!data || !data->ringtone) return GN_ERR_INTERNALERROR;
	if (data->ringtone->location < 0) data->ringtone->location = 17;

	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_NBS_UPLOAD) {
		data->ringtone->location = -1;
		return NBSUpload(data, state, GN_SMS_DATA_Ringtone);
	}

	size = GN_RINGTONE_PACKAGE_MAX_LENGTH;
	gn_ringtone_pack(data->ringtone, req + 7, &size);
	req[4] = data->ringtone->location - 17;

	if (sm_message_send(7 + size, 0x05, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x05, data, state);
}

static gn_error GetActiveProfile(gn_data *data, struct gn_statemachine *state)
{
	if (!data->profile)
		return GN_ERR_UNKNOWN;
	data->profile->number = 0;

	return GetProfileFeature(0x2a, data, state);
}

static gn_error SetActiveProfile(gn_data *data, struct gn_statemachine *state)
{
	if (!data->profile)
		return GN_ERR_UNKNOWN;

	return SetProfileFeature(data, state, 0x2a, data->profile->number);
}

static gn_error IncomingProfile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_bmp *bmp;
	gn_profile *prof;
	unsigned char *pos;
	int i;
	bool found;

	switch (message[3]) {
	/* Set profile feat. OK */
	case 0x11:
		if (length == 4) break;	/* non profile specific, e.g. set active profile */
		switch (message[4]) {
		case 0x01:
			break;
		case 0x7d:
			dprintf("Cannot set profile feature\n");
			return GN_ERR_UNKNOWN;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Set profile feat. ERR */
	case 0x12:
		switch (message[4]) {
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x7d:
			dprintf("Cannot set profile feature\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Get profile feature */
	case 0x14:
		if (data->profile) {
			prof = data->profile;
			switch (message[6]) {
			case 0x00:
				prof->keypad_tone = message[8];
				break;
			case 0x01:
				prof->lights = message[8];
				break;
			case 0x02:
				prof->call_alert = message[8];
				break;
			case 0x03:
				prof->ringtone = message[8];
				break;
			case 0x04:
				prof->volume = message[8];
				break;
			case 0x05:
				prof->message_tone = message[8];
				break;
			case 0x06:
				prof->vibration = message[8];
				break;
			case 0x07:
				prof->warning_tone = message[8];
				break;
			case 0x08:
				prof->caller_groups = message[8];
				break;
			case 0x09:
				prof->automatic_answer = message[8];
				break;
			case 0x2a:
				prof->number = message[8];
				break;
			default:
				return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;

	/* Get profile error */
	case 0x15:
		switch (message[4]) {
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x7d:
			return GN_ERR_INVALIDLOCATION;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* Get Welcome Message */
	case 0x17:
		if (data->bitmap) {
			bmp = data->bitmap;
			pos = message + 5;
			found = false;
			for (i = 0; i < message[4] && !found; i++) {
				switch (*pos++) {
				case 0x01:
					if (bmp->type != GN_BMP_StartupLogo) {
						pos += pos[0] * pos[1] / 8 + 2;
						continue;
					}
					bmp->height = *pos++;
					bmp->width = *pos++;
					bmp->size = bmp->height * bmp->width / 8;
					if (bmp->size > sizeof(bmp->bitmap)) {
						return GN_ERR_UNHANDLEDFRAME;
					}
					memcpy(bmp->bitmap, pos, bmp->size);
					pos += bmp->size;
					break;
				case 0x02:
					if (bmp->type != GN_BMP_WelcomeNoteText) {
						pos += *pos + 1;
						continue;
					}
					pnok_string_decode(bmp->text, sizeof(bmp->text), pos+1, *pos);
					pos += *pos + 1;
					break;
				case 0x03:
					if (bmp->type != GN_BMP_DealerNoteText) {
						pos += *pos + 1;
						continue;
					}
					pnok_string_decode(bmp->text, sizeof(bmp->text), pos+1, *pos);
					pos += *pos + 1;
					break;
				default:
					return GN_ERR_UNHANDLEDFRAME;
				}
				found = true;
			}
			if (!found) return GN_ERR_NOTSUPPORTED;
		}
		break;

	/* Set welcome ok */
	case 0x19:
		break;

	/* Get profile name */
	case 0x1b:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x6f:
			return GN_ERR_NOTREADY;
		case 0x8d:
			return GN_ERR_CODEREQUIRED;
		case 0x93:
			return GN_ERR_EMPTYLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		if (data->profile) {
			if (message[9] == 0x00) {
				data->profile->default_name = message[8];
				data->profile->name[0] = 0;
			} else {
				data->profile->default_name = -1;
				pnok_string_decode(data->profile->name, sizeof(data->profile->name), message+10, message[9]);
			}
			break;
		} else {
			return GN_ERR_UNKNOWN;
		}
		break;

	/* Set profile name OK / ERR */
	case 0x1d:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Set oplogo ok */
	case 0x31:
		break;

	/* Set oplogo error */
	case 0x32:
		switch (message[4]) {
		case 0x7d:
			return GN_ERR_INVALIDLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* Get oplogo */
	case 0x34:
		if (data->bitmap) {
			bmp = data->bitmap;
			pos = message + 5;
			bmp->netcode[0] = '0' + (pos[0] & 0x0f);
			bmp->netcode[1] = '0' + (pos[0] >> 4);
			bmp->netcode[2] = '0' + (pos[1] & 0x0f);
			bmp->netcode[3] = ' ';
			bmp->netcode[4] = '0' + (pos[2] & 0x0f);
			bmp->netcode[5] = '0' + (pos[2] >> 4);
			bmp->netcode[6] = 0;
			pos += 3;
			bmp->size = (pos[0] << 8) + pos[1];
			pos += 2;
			pos++;
			bmp->width = *pos++;
			bmp->height = *pos++;
			pos++;
			i = bmp->height * bmp->width / 8;
			if (bmp->size > i) bmp->size = i;
			if (bmp->size > sizeof(bmp->bitmap)) {
				return GN_ERR_UNHANDLEDFRAME;
			}
			memcpy(bmp->bitmap, pos, bmp->size);
		}
		break;

	/* Get oplogo error */
	case 0x35:
		switch (message[4]) {
			case 0x7d:
				return GN_ERR_UNKNOWN;
			default:
				return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Set ringtone OK */
	case 0x37:
		return GN_ERR_NONE;

	/* Set ringtone error */
	case 0x38:
		switch (message[4]) {
			case 0x7d:
				return GN_ERR_UNKNOWN;
			default:
				return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetDateTime(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x62};

	if (sm_message_send(4, 0x11, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x11, data, state);
}

static gn_error SetDateTime(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x60, 0x01, 0x01, 0x07,
			       0x00, 0x00,	/* year - H/L */
			       0x00, 0x00,	/* month, day */
			       0x00, 0x00,	/* hours, minutes */
			       0x00};		/* Unknown, but not seconds - try 59 and wait 1 sec. */

	req[7] = data->datetime->year >> 8;
	req[8] = data->datetime->year & 0xff;
	req[9] = data->datetime->month;
	req[10] = data->datetime->day;
	req[11] = data->datetime->hour;
	req[12] = data->datetime->minute;

	if (sm_message_send(14, 0x11, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x11, data, state);
}

static gn_error GetAlarm(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6d};

	if (sm_message_send(4, 0x11, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x11, data, state);
}

static gn_error SetAlarm(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6b, 0x01, 0x20, 0x03,
			       0x02,		/* should be alarm on/off, but it doesn't work */
			       0x00, 0x00,	/* hours, minutes */
			       0x00};		/* Unknown, but not seconds - try 59 and wait 1 sec. */

	if (data->alarm->enabled) {
		req[8] = data->alarm->timestamp.hour;
		req[9] = data->alarm->timestamp.minute;
	} else {
		dprintf("Clearing the alarm clock isn't supported\n");
		return GN_ERR_NOTSUPPORTED;
	}

	if (sm_message_send(11, 0x11, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x11, data, state);
}

static gn_error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_timestamp *date;
	gn_calnote_alarm *alarm;
	unsigned char *pos;

	switch (message[3]) {
	/* Date and time set */
	case 0x61:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Date and time received */
	case 0x63:
		dprintf("Message: Date and time\n");
		if (!message[4]) dprintf("   Date: not set\n");
		if (!message[5]) dprintf("   Time: not set\n");
		if (!(message[4] && message[5])) return GN_ERR_NOTAVAILABLE;
		if (data->datetime) {
			date = data->datetime;
			pos = message + 8;
			date->year = (pos[0] << 8) | pos[1];
			pos += 2;
			date->month = *pos++;
			date->day = *pos++;
			date->hour = *pos++;
			date->minute = *pos++;
			date->second = *pos++;

			dprintf("   Time: %02d:%02d:%02d\n", date->hour, date->minute, date->second);
			dprintf("   Date: %4d/%02d/%02d\n", date->year, date->month, date->day);
		}
		break;

	/* Alarm set */
	case 0x6c:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Alarm received */
	case 0x6e:
		if (data->alarm) {
			alarm = data->alarm;
			pos = message + 8;
			alarm->enabled = (*pos++ == 2);
			alarm->timestamp.hour = *pos++;
			alarm->timestamp.minute = *pos++;
			alarm->timestamp.second = 0;

			dprintf("Message: Alarm\n");
			dprintf("   Alarm: %02d:%02d\n", alarm->timestamp.hour, alarm->timestamp.minute);
			dprintf("   Alarm is %s\n", alarm->enabled ? "on" : "off");
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x66, 0x00};

	req[4] = data->calnote->location;

	if (sm_message_send(5, 0x13, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x13, data, state);
}

static gn_error WriteCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x64, 0x01, 0x10,
				 0x00,	/* Length of the rest of the frame. */
				 0x00};	/* The type of calendar note */
	gn_calnote *note;
	unsigned char *pos;
	unsigned int numlen, namelen;

	if (!data->calnote)
		return GN_ERR_UNKNOWN;

	note = data->calnote;
	pos = req + 7;
	numlen = strlen(note->phone_number);
	if (numlen > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		return GN_ERR_UNKNOWN;
	}

	switch (note->type) {
	case GN_CALNOTE_REMINDER: *pos++ = 0x01; break;
	case GN_CALNOTE_CALL: *pos++ = 0x02; break;
	case GN_CALNOTE_MEETING: *pos++ = 0x03; break;
	case GN_CALNOTE_BIRTHDAY: *pos++ = 0x04; break;
	default: return GN_ERR_INTERNALERROR;
	}

	*pos++ = note->time.year >> 8;
	*pos++ = note->time.year & 0xff;
	*pos++ = note->time.month;
	*pos++ = note->time.day;
	*pos++ = note->time.hour;
	*pos++ = note->time.minute;
	*pos++ = note->time.timezone;

	if (note->alarm.timestamp.year) {
		*pos++ = note->alarm.timestamp.year >> 8;
		*pos++ = note->alarm.timestamp.year & 0xff;
		*pos++ = note->alarm.timestamp.month;
		*pos++ = note->alarm.timestamp.day;
		*pos++ = note->alarm.timestamp.hour;
		*pos++ = note->alarm.timestamp.minute;
		*pos++ = note->alarm.timestamp.timezone;
	} else {
		memset(pos, 0x00, 7);
		pos += 7;
	}

	/* FIXME: use some constant not 255 magic number */
	if (!strcmp(DRVINSTANCE(state)->model, "NHM-5")		/* Nokia 3310 */
	 || !strcmp(DRVINSTANCE(state)->model, "NHM-6")) {	/* Nokia 3330 */
		/* in this case we have: length, encoding indicator, text */
		namelen = pnok_string_encode(pos + 2, 255, note->text);
		*pos++ = namelen + 1;	/* length of text + encoding indicator */
		*pos++ = 0x03;		/* encoding type */
	} else {
		if (DRVINSTANCE(state)->capabilities & NK6100_CAP_CAL_UNICODE)
			namelen = char_unicode_encode(pos + 1, note->text, 255);
		else
			namelen = pnok_string_encode(pos + 1, 255, note->text);
		*pos++ = namelen;
	}
	pos += namelen;

	if (note->type == GN_CALNOTE_CALL) {
		*pos++ = numlen;
		memcpy(pos, note->phone_number, numlen);
		pos += numlen;
	} else {
		*pos++ = 0;
	}

	if (sm_message_send(pos - req, 0x13, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x13, data, state);
}

static gn_error DeleteCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x68, 0x00};

	req[4] = data->calnote->location;

	if (sm_message_send(5, 0x13, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x13, data, state);
}

static gn_error IncomingCalendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_calnote *note;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	/* Write cal.note report */
	case 0x65:
		switch (message[4]) {
			case 0x01:
				return GN_ERR_NONE;
			case 0x6f: /* Insert SIM card */
				return GN_ERR_NOTREADY;
			case 0x73:
				return GN_ERR_MEMORYFULL;
			case 0x7d:
				return GN_ERR_UNKNOWN;
		        case 0x81: /* calendar functions are busy. well, this status code is better than nothing */
				return GN_ERR_LINEBUSY;
			case 0x8d: /* waiting for PIN */
				return GN_ERR_CODEREQUIRED;
			default:
				return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Calendar note recvd */
	case 0x67:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		case 0x93:
			return GN_ERR_EMPTYLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		if (data->calnote) {
			note = data->calnote;
			pos = message + 8;

			/* FIXME: this supposed to be replaced by general date unpacking function :-) */
			switch (*pos++) {
			case 0x01: note->type = GN_CALNOTE_REMINDER; break;
			case 0x02: note->type = GN_CALNOTE_CALL; break;
			case 0x03: note->type = GN_CALNOTE_MEETING; break;
			case 0x04: note->type = GN_CALNOTE_BIRTHDAY; break;
			default: return GN_ERR_UNHANDLEDFRAME;
			}
			note->time.year = (pos[0] << 8) + pos[1];
			pos += 2;
			note->time.month = *pos++;
			note->time.day = *pos++;
			note->time.hour = *pos++;
			note->time.minute = *pos++;
			note->time.second = *pos++;

			note->alarm.timestamp.year = (pos[0] << 8) + pos[1];
			pos += 2;
			note->alarm.timestamp.month = *pos++;
			note->alarm.timestamp.day = *pos++;
			note->alarm.timestamp.hour = *pos++;
			note->alarm.timestamp.minute = *pos++;
			note->alarm.timestamp.second = *pos++;
			note->alarm.enabled = (note->alarm.timestamp.year != 0);
			n = *pos++;
			if (!strcmp(DRVINSTANCE(state)->model, "NHM-5")		/* Nokia 3310 */
			 || !strcmp(DRVINSTANCE(state)->model, "NHM-6")) {	/* Nokia 3330 */
				pos++; /* skip encoding byte FIXME: decode accordingly */
				n--;   /* text length */
			}
			if ((DRVINSTANCE(state)->capabilities & NK6100_CAP_CAL_UNICODE))
				char_unicode_decode(note->text, pos, n);
			else
				pnok_string_decode(note->text, sizeof(note->text), pos, n);
			pos += n;

			if (note->type == GN_CALNOTE_CALL) {
				/* This will be replaced later :-) */
				n = *pos++;
				pnok_string_decode(note->phone_number, sizeof(note->phone_number), pos, n);
				pos += n;
			} else {
				note->phone_number[0] = 0;
			}

			/* from Nokia 3310 and 3330 we always read year == 2090 */
			if (note->time.year == 2090) {
				note->time.year = note->alarm.timestamp.year;
				/* FIXME: decrease note->time.year if the new start date is later than alarm date
				   (this happens if you set an alarm for the next year);
				   this would need a gn_timestamp_cmp() but would break again if you set an alarm
				   two years in the future */
			}

			memset(&note->end_time, 0, sizeof(note->end_time));
			note->mlocation[0] = 0;
		}
		break;

	/* Del. cal.note report */
	case 0x69:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x6f: /* Insert SIM card */
			return GN_ERR_NOTREADY;
		case 0x81:
			/* calendar functions are busy. well, this status code is better than nothing */
			return GN_ERR_LINEBUSY;
		case 0x8d: /* waiting for PIN */
			return GN_ERR_CODEREQUIRED;
		case 0x93:
			return GN_ERR_EMPTYLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}

static gn_error GetDisplayStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x51};

	if (sm_message_send(4, 0x0d, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x0d, data, state);
}

static gn_error PollDisplay(gn_data *data, struct gn_statemachine *state)
{
	gn_data dummy;

	gn_data_clear(&dummy);
	GetDisplayStatus(&dummy, state);

	return GN_ERR_NONE;
}

static gn_error DisplayOutput(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x53, 0x00};

	if (data->display_output->output_fn) {
		DRVINSTANCE(state)->display_output = data->display_output;
		req[4] = 0x01;
	} else {
		DRVINSTANCE(state)->display_output = NULL;
		req[4] = 0x02;
	}

	if (sm_message_send(5, 0x0d, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x0d, data, state);
}

static gn_error IncomingDisplay(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int state_table[8] = { 1 << GN_DISP_Call_In_Progress, 1 << GN_DISP_Unknown,
			       1 << GN_DISP_Unread_SMS, 1 << GN_DISP_Voice_Call,
			       1 << GN_DISP_Fax_Call, 1 << GN_DISP_Data_Call,
			       1 << GN_DISP_Keyboard_Lock, 1 << GN_DISP_SMS_Storage_Full };
	unsigned char *pos;
	int n, x, y, st;
	gn_display_draw_msg drawmsg;
	gn_display_output *disp = DRVINSTANCE(state)->display_output;
	struct timeval now, time_diff, time_limit;

	switch (message[3]) {
	/* Display output */
	case 0x50:
		if (disp) {
			switch (message[4]) {
			case 0x01:
				break;
			default:
				return GN_ERR_UNHANDLEDFRAME;
			}
			pos = message + 5;
			y = *pos++;
			x = *pos++;
			n = *pos++;
			if (n > GN_DRAW_SCREEN_MAX_WIDTH) return GN_ERR_INTERNALERROR;

			/*
			 * WARNING: the following part can damage your mind!
			 * This is so ugly, but there's no other way. Nokia
			 * forgot the clear screen from the protocol, so we
			 * must implement some heuristic here... :-( - bozo
			 */
			time_limit.tv_sec = 0;
			time_limit.tv_usec = 200000;
			gettimeofday(&now, NULL);
			timersub(&now, &disp->last, &time_diff);
			if (y > 9 && timercmp(&time_diff, &time_limit, >))
				disp->state = true;
			disp->last = now;

			if (disp->state && y > 9) {
				disp->state = false;
				memset(&drawmsg, 0, sizeof(drawmsg));
				drawmsg.cmd = GN_DISP_DRAW_Clear;
				disp->output_fn(&drawmsg);
			}
			/* Maybe this is unneeded, please leave uncommented
			if (x == 0 && y == 46 && pos[1] != 'M') {
				disp->state = true;
			}
			*/

			memset(&drawmsg, 0, sizeof(drawmsg));
			drawmsg.cmd = GN_DISP_DRAW_Text;
			drawmsg.data.text.x = x;
			drawmsg.data.text.y = y;
			char_unicode_decode(drawmsg.data.text.text, pos, n << 1);
			disp->output_fn(&drawmsg);

			dprintf("(x,y): %d,%d, len: %d, data: %s\n", x, y, n, drawmsg.data.text.text);
		}
		return GN_ERR_UNSOLICITED;

	/* Display status */
	case 0x52:
		st = 0;
		pos = message + 4;
		for ( n = *pos++; n > 0; n--, pos += 2) {
			if ((pos[0] < 1) || (pos[0] > 8))
				return GN_ERR_UNHANDLEDFRAME;
			if (pos[1] == 0x02)
				st |= state_table[pos[0] - 1];
		}
		if (data->display_status)
			*data->display_status = st;
		if (disp) {
			memset(&drawmsg, 0, sizeof(drawmsg));
			drawmsg.cmd = GN_DISP_DRAW_Status;
			drawmsg.data.status = st;
			disp->output_fn(&drawmsg);
		}
		break;

	/* Display status ack */
	case 0x54:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error Reset(gn_data *data, struct gn_statemachine *state)
{
	if (!data) return GN_ERR_INTERNALERROR;
	if (data->reset_type != 0x03 && data->reset_type != 0x04) return GN_ERR_INTERNALERROR;

	return pnok_extended_cmds_enable(data->reset_type, data, state);
}

static gn_error GetRawRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x9e, 0x00};
	gn_error error;

	if (!data || !data->ringtone || !data->raw_data) return GN_ERR_INTERNALERROR;
	if (data->ringtone->location < 0) return GN_ERR_INVALIDLOCATION;

	req[3] = data->ringtone->location - 17;

	if ((error = pnok_extended_cmds_enable(0x01, data, state))) return error;

	if (sm_message_send(4, 0x40, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x40, data, state);
}

static gn_error SetRawRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {0x00, 0x01, 0xa0,
				  0x00,				/* location */
				  0x00, 0x0c, 0x2c, 0x01, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x02, 0xfc, 0x09};	/* data */
	gn_error error;
	int len;

	if (!data || !data->ringtone || !data->raw_data || !data->raw_data->data)
		return GN_ERR_INTERNALERROR;
	if (data->ringtone->location < 0) data->ringtone->location = 17;

	req[3] = data->ringtone->location - 17;
	snprintf(req + 8, 13, "%s", data->ringtone->name);
	if (memcmp(data->raw_data->data, req + 20, 3) == 0) {
		memcpy(req + 20, data->raw_data->data, data->raw_data->length);
		len = 20 + data->raw_data->length;
	} else {
		/* compatibility */
		memcpy(req + 24, data->raw_data->data, data->raw_data->length);
		len = 24 + data->raw_data->length;
	}

	if ((error = pnok_extended_cmds_enable(0x01, data, state))) return error;

	if (sm_message_send(len, 0x40, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x40, data, state);
}

static gn_error DeleteRingtone(gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone ringtone;
	gn_raw_data rawdata;
	unsigned char buf[] = {0x00, 0x02, 0xfc, 0x0b};
	gn_data d;

	if (!data->ringtone) return GN_ERR_INTERNALERROR;

	memset(&ringtone, 0, sizeof(ringtone));
	ringtone.location = (data->ringtone->location < 0) ? 17 : data->ringtone->location;
	memset(&rawdata, 0, sizeof(gn_raw_data));
	rawdata.data = buf;
	rawdata.length = sizeof(buf);
	gn_data_clear(&d);
	d.ringtone = &ringtone;
	d.raw_data = &rawdata;

	return SetRawRingtone(&d, state);
}

static gn_error get_imei(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x66};
	gn_error err;

	if ((err = pnok_extended_cmds_enable(0x01, data, state))) return err;
	if (sm_message_send(3, 0x40, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x40, data, state);
}

static gn_error get_phone_info(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0xc8, 0x01};
	gn_error err;

	if ((err = pnok_extended_cmds_enable(0x01, data, state)))
		return err;

	if (sm_message_send(4, 0x40, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x40, data, state);
}

static gn_error get_hw(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0xc8, 0x05};
	gn_error err;

	if ((err = pnok_extended_cmds_enable(0x01, data, state)))
		return err;

	if (sm_message_send(4, 0x40, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x40, data, state);
}

#ifdef  SECURITY
static gn_error get_security_code(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x6e, 0x01};
	gn_error err;

	if (!data->security_code) return GN_ERR_INTERNALERROR;
	req[3] = data->security_code->type;

	if ((err = pnok_extended_cmds_enable(0x01, data, state)))
		return err;

	if (sm_message_send(4, 0x40, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x40, data, state);
}
#endif

static gn_error IncomingSecurity(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	char *aux, *aux2;

	switch (message[2]) {
	/* IMEI */
	case 0x66:
		if (data->imei) {
			dprintf("IMEI: %s\n", message + 4);
			snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", message + 4);
		}
		break;

#ifdef SECURITY
	/* get security code */
	case 0x6e:
		if (message[4] != 0x01) return GN_ERR_UNKNOWN;
		if (data->security_code) {
			data->security_code->type = message[3];
			snprintf(data->security_code->code, sizeof(data->security_code->code), "%s", message + 5);
		}
		break;
#endif

	/* Get bin ringtone */
	case 0x9e:
		switch (message[4]) {
		case 0x00:
			break;
		case 0x0a:
			return GN_ERR_INVALIDLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		if (!data->ringtone) return GN_ERR_INTERNALERROR;
		data->ringtone->location = message[3] + 17;
		snprintf(data->ringtone->name, sizeof(data->ringtone->name), "%s", message + 8);
		if (data->raw_data->length < length - 20) return GN_ERR_MEMORYFULL;
		if (data->raw_data && data->raw_data->data) {
			memcpy(data->raw_data->data, message + 20, length - 20);
			data->raw_data->length = length - 20;
		}
		break;

	/* Set bin ringtone result */
	case 0xa0:
		switch (message[4]) {
		case 0x00:
			break;
		case 0x0a:
			return GN_ERR_INVALIDLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	case 0xc8:
		switch (message[3]) {
		case 0x01:
			if (data->revision) {
				aux = message + 7;
				aux2 = strchr(aux, 0x0a);
				if (data->revision[0]) {
					strcat(data->revision, ", SW ");
					strncat(data->revision, aux,
						aux2 - aux);
				} else {
					snprintf(data->revision, aux2 - aux + 4,
						 "SW %s", aux);
				}
				dprintf("Received %s\n", data->revision);
			}
			aux = strchr(message + 5, 0x0a);
			aux++;
			aux = strchr(aux, 0x0a);
			aux++;
			if (data->model) {
				aux2 = strchr(aux, 0x0a);
				*aux2 = 0;
				snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", aux);
				dprintf("Received model %s\n", data->model);
			}
			break;
		case 0x05:
			if (data->revision) {
				if (data->revision[0]) {
					strcat(data->revision, ", HW ");
					strncat(data->revision, message + 5,
						GN_REVISION_MAX_LENGTH);
				} else {
					snprintf(data->revision, GN_REVISION_MAX_LENGTH,
						 "HW %s", message + 5);
				}
				dprintf("Received %s\n", data->revision);
			}
			break;
		default:
			return GN_ERR_NOTIMPLEMENTED;
		}
		break;

	default:
		return pnok_security_incoming(messagetype, message, length, data, state);
	}

	return GN_ERR_NONE;
}

static gn_error GetActiveCalls1(gn_data *data, struct gn_statemachine *state)
{
	char req[] = {FBUS_FRAME_HEADER, 0x20};

	if (!data->call_active) return GN_ERR_INTERNALERROR;

	if (sm_message_send(4, 0x01, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x01, data, state);
}

static gn_error MakeCall1(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01};
	/* order: req, voice_end */
	unsigned char voice_end[] = {0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00};
	/* order: req, data_nondigital_end, data_nondigital_final */
	unsigned char data_nondigital_end[]   = { 0x01,  /* make a data call = type 0x01 */
						  0x02,0x01,0x05,0x81,0x01,0x00,0x00,0x01,0x02,0x0a,
						  0x07,0xa2,0x88,0x81,0x21,0x15,0x63,0xa8,0x00,0x00 };
	unsigned char data_nondigital_final[] = { FBUS_FRAME_HEADER, 0x42,0x05,0x01,
						  0x07,0xa2,0xc8,0x81,0x21,0x15,0x63,0xa8,0x00,0x00,
						  0x07,0xa3,0xb8,0x81,0x20,0x15,0x63,0x80,0x01,0x60 };
	/* order: req, data_digital_pred1, data_digital_pred2, data_digital_end */
	unsigned char data_digital_pred1[]    = { FBUS_FRAME_HEADER, 0x42,0x05,0x01,
						  0x07,0xa2,0x88,0x81,0x21,0x15,0x63,0xa8,0x00,0x00,
						  0x07,0xa3,0xb8,0x81,0x20,0x15,0x63,0x80 };
	unsigned char data_digital_pred2[]    = { FBUS_FRAME_HEADER, 0x42,0x05,0x81,
						  0x07,0xa1,0x88,0x89,0x21,0x15,0x63,0xa0,0x00,0x06,
						  0x88,0x90,0x21,0x48,0x40,0xbb,0x07,0xa3,0xb8,0x81,
						  0x20,0x15,0x63,0x80 };
	unsigned char data_digital_end[]      = { 0x01,
						  0x02,0x01,0x05,0x81,0x01,0x00,0x00,0x01,0x02,0x0a,
						  0x07,0xa1,0x88,0x89,0x21,0x15,0x63,0xa0,0x00,0x06,
						  0x88,0x90,0x21,0x48,0x40,0xbb };
	unsigned char *pos;
	int n;
	gn_data dtemp;
	gn_error err;

	n = strlen(data->call_info->number);
	if (n > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}

	/*
	 * FIXME:
	 * Ugly hack. The phone seems to drop the dial message if the link
	 * had been idle too long. -- bozo
	 */
	gn_data_clear(&dtemp);
	GetNetworkInfo(&dtemp, state);

	pos = req + 4;
	*pos++ = (unsigned char)n;
	memcpy(pos, data->call_info->number, n);
	pos += n;

	switch (data->call_info->type) {
	case GN_CALL_Voice:
		dprintf("Voice Call\n");
		switch (data->call_info->send_number) {
		case GN_CALL_Never:
			voice_end[5] = 0x02;
			break;
		case GN_CALL_Always:
			voice_end[5] = 0x03;
			break;
		case GN_CALL_Default:
			voice_end[5] = 0x01;
			break;
		default:
			return GN_ERR_INTERNALERROR;
		}
		/* here in the past: voice_end had 8 bytes, memcpied   9 bytes, sent 1+9 bytes
		 * fbus-6110.c:      req_end   had 9 bytes, memcpied  10 bytes, sent   8 bytes
		 * currently:        voice_end has 8 bytes, memcpying  8 bytes, sent   8 bytes
		 */
		memcpy(pos, voice_end, ARRAY_LEN(voice_end));
		pos += ARRAY_LEN(voice_end);
		if (sm_message_send(pos - req, 0x01, req, state)) return GN_ERR_NOTREADY;
		break;

	case GN_CALL_NonDigitalData:
		dprintf("Non Digital Data Call\n");
		memcpy(pos, data_nondigital_end, ARRAY_LEN(data_nondigital_end));
		pos += ARRAY_LEN(data_nondigital_end);
		if (sm_message_send(pos - req, 0x01, req, state)) return GN_ERR_NOTREADY;
		if (sm_block_ack(state)) return GN_ERR_NOTREADY;
		gn_sm_loop(5, state);
		dprintf("after nondigital1\n");
		if (sm_message_send(ARRAY_LEN(data_nondigital_final), 0x01, data_nondigital_final, state)) return GN_ERR_NOTREADY;
		dprintf("after nondigital2\n");
		break;

	case GN_CALL_DigitalData:
		dprintf("Digital Data Call\n");
		if (sm_message_send(ARRAY_LEN(data_digital_pred1), 0x01, data_digital_pred1, state)) return GN_ERR_NOTREADY;
		if (sm_block_ack(state)) return GN_ERR_NOTREADY;
		gn_sm_loop(5, state);
		dprintf("after digital1\n");
		if (sm_message_send(ARRAY_LEN(data_digital_pred2), 0x01, data_digital_pred2, state)) return GN_ERR_NOTREADY;
		if (sm_block_ack(state)) return GN_ERR_NOTREADY;
		gn_sm_loop(5, state);
		dprintf("after digital2\n");
		memcpy(pos, data_digital_end, ARRAY_LEN(data_digital_end));
		pos += ARRAY_LEN(data_digital_end);
		if (sm_message_send(pos - req, 0x01, req, state)) return GN_ERR_NOTREADY;
		dprintf("after digital3\n");
		break;

	default:
		dprintf("Invalid call type %d\n", data->call_info->type);
		return GN_ERR_INTERNALERROR;
	}

	err = sm_block_no_retry_timeout(0x01, 500, data, state);
	gn_sm_loop(5, state);
	return err;
}

static gn_error AnswerCall1(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req1[] = {FBUS_FRAME_HEADER, 0x42, 0x05, 0x01, 0x07,
				0xa2, 0x88, 0x81, 0x21, 0x15, 0x63, 0xa8, 0x00, 0x00,
				0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x06, 0x00, 0x00};

	if (sm_message_send(sizeof(req1), 0x01, req1, state)) return GN_ERR_NOTREADY;

	req2[4] = data->call_info->call_id;

	if (sm_message_send(sizeof(req2), 0x01, req2, state)) return GN_ERR_NOTREADY;
	return sm_block(0x01, data, state);
}

static gn_error CancelCall1(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00, 0x85};

	req[4] = data->call_info->call_id;

	if (sm_message_send(6, 0x01, req, state)) return GN_ERR_NOTREADY;
	sm_block_no_retry(0x01, data, state);
	return GN_ERR_NONE;
}

static gn_error SetCallNotification(gn_data *data, struct gn_statemachine *state)
{
	DRVINSTANCE(state)->call_notification = data->call_notification;
	DRVINSTANCE(state)->call_callback_data = data->callback_data;

	return GN_ERR_NONE;
}

static gn_error GetActiveCalls(gn_data *data, struct gn_statemachine *state)
{
	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_OLD_CALL_API)
		return GN_ERR_NOTSUPPORTED;
	else
		return GetActiveCalls1(data, state);
}

static gn_error MakeCall(gn_data *data, struct gn_statemachine *state)
{
	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_OLD_CALL_API)
		return pnok_call_make(data, state);
	else
		return MakeCall1(data, state);
}

static gn_error AnswerCall(gn_data *data, struct gn_statemachine *state)
{
	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_OLD_CALL_API)
		return pnok_call_answer(data, state);
	else
		return AnswerCall1(data, state);
}

static gn_error CancelCall(gn_data *data, struct gn_statemachine *state)
{
	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_OLD_CALL_API)
		return pnok_call_cancel(data, state);
	else
		return CancelCall1(data, state);
}

static gn_error SendDTMF(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[5+256] = {FBUS_FRAME_HEADER, 0x50};
	int len;

	if (!data || !data->dtmf_string) return GN_ERR_INTERNALERROR;

	len = strlen(data->dtmf_string);
	if (len < 0 || len >= 256) return GN_ERR_INTERNALERROR;

	req[4] = len;
	memcpy(req + 5, data->dtmf_string, len);

	if (sm_message_send(5 + len, 0x01, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x01, data, state);
}

static gn_error IncomingCallInfo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_call_info cinfo;
	gn_call_active *ca;
	unsigned char *pos;
	int i;

	switch (message[3]) {
	/* Call going msg */
	case 0x02:
		if (data->call_info)
			data->call_info->call_id = message[4];
		break;

	/* Call in progress */
	case 0x03:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.call_id = message[4];
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(GN_CALL_Established, &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		if (!data->call_info) return GN_ERR_UNSOLICITED;
		data->call_info->call_id = message[4];
		break;

	/* Remote end hang up */
	case 0x04:
		isdn_cause2gn_error(NULL, NULL, message[7], message[6]);
		if (data->call_info) {
			data->call_info->call_id = message[4];
			return GN_ERR_UNKNOWN;
		}
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.call_id = message[4];
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(GN_CALL_RemoteHangup, &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		return GN_ERR_UNSOLICITED;

	/* incoming call alert */
	case 0x05:
		memset(&cinfo, 0, sizeof(cinfo));
		pos = message + 4;
		cinfo.call_id = *pos++;
		pos++;
		if (*pos >= sizeof(cinfo.number))
			return GN_ERR_UNHANDLEDFRAME;
		memcpy(cinfo.number, pos + 1, *pos);
		pos += *pos + 1;
		if (*pos >= sizeof(cinfo.name))
			return GN_ERR_UNHANDLEDFRAME;
		memcpy(cinfo.name, pos + 1, *pos);
		pos += *pos + 1;
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(GN_CALL_Incoming, &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		return GN_ERR_UNSOLICITED;

	/* answered call */
	case 0x07:
		return GN_ERR_UNSOLICITED;

	/* terminated call */
	case 0x09:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.call_id = message[4];
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(GN_CALL_LocalHangup, &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		if (!data->call_info) return GN_ERR_UNSOLICITED;
		data->call_info->call_id = message[4];
		break;

	/* message after "terminated call" */
	case 0x0a:
		return GN_ERR_UNSOLICITED;

	/* call status */
	case 0x21:
		if (!data->call_active) return GN_ERR_INTERNALERROR;
		pos = message + 5;
		ca = data->call_active;
		memset(ca, 0x00, 2 * sizeof(gn_call_active));
		for (i = 0; i < message[4]; i++) {
			if (pos[0] != 0x64) return GN_ERR_UNHANDLEDFRAME;
			ca[i].call_id = pos[2];
			ca[i].channel = pos[3];
			switch (pos[4]) {
			case 0x00: ca[i].state = GN_CALL_Idle; break;
			case 0x02: ca[i].state = GN_CALL_Dialing; break;
			case 0x03: ca[i].state = GN_CALL_Ringing; break;
			case 0x04: ca[i].state = GN_CALL_Incoming; break;
			case 0x05: ca[i].state = GN_CALL_Established; break;
			case 0x06: ca[i].state = GN_CALL_Held; break;
			case 0x07: ca[i].state = GN_CALL_RemoteHangup; break;
			default: return GN_ERR_UNHANDLEDFRAME;
			}
			switch (pos[5]) {
			case 0x00: ca[i].prev_state = GN_CALL_Idle; break;
			case 0x01: ca[i].prev_state = GN_CALL_Idle; break;
			case 0x02: ca[i].prev_state = GN_CALL_Dialing; break;
			case 0x03: ca[i].prev_state = GN_CALL_Ringing; break;
			case 0x04: ca[i].prev_state = GN_CALL_Incoming; break;
			case 0x05: ca[i].prev_state = GN_CALL_Established; break;
			case 0x06: ca[i].prev_state = GN_CALL_Held; break;
			case 0x07: ca[i].prev_state = GN_CALL_RemoteHangup; break;
			default: return GN_ERR_UNHANDLEDFRAME;
			}
			pnok_string_decode(ca[i].name, sizeof(ca[i].name), pos + 11, pos[10]);
			pos += pos[1] + 2;
		}
		break;

	/* call held */
	case 0x23:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.call_id = message[4];
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(GN_CALL_Held, &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		return GN_ERR_UNSOLICITED;

	/* call resumed */
	case 0x25:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.call_id = message[4];
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(GN_CALL_Resumed, &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		return GN_ERR_UNSOLICITED;

	/* call switch */
	case 0x27:
		return GN_ERR_UNSOLICITED;

	case 0x40:
		return GN_ERR_UNSOLICITED;

	/* FIXME: response from "Sent after issuing data call (non digital lines)"
	 * that's what we call data_nondigital_final in MakeCall()
	 */
	case 0x43:
		if (message[4] != 0x02) return GN_ERR_UNHANDLEDFRAME;
		return GN_ERR_UNSOLICITED;

	/* FIXME: response from answer1? - bozo */
	case 0x44:
		if (message[4] != 0x68) return GN_ERR_UNHANDLEDFRAME;
		return GN_ERR_UNSOLICITED;

	/* DTMF sent */
	case 0x51:
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error SendRLPFrame(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32] = {0x00, 0xd9};

	/*
	 * Discontinuos transmission (DTX).
	 * See section 5.6 of GSM 04.22 version 7.0.1.
	 */

	if (data->rlp_out_dtx) req[1] = 0x01;
	memcpy(req + 2, (unsigned char *) data->rlp_frame, 30);

	/*
	 * It's ugly like the hell, maybe we should implement SM_SendFrame
	 * and extend the GSM_Link structure. We should clean this up in the
	 * future... - bozo
	 * return SM_SendFrame(state, 32, 0xf0, req);
	 */

	return fbus_tx_send_frame(32, 0xf0, req, state);
}

static gn_error SetRLPRXCallback(gn_data *data, struct gn_statemachine *state)
{
	DRVINSTANCE(state)->rlp_rx_callback = data->rlp_rx_callback;

	return GN_ERR_NONE;
}

static gn_error IncomingRLPFrame(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_rlp_f96_frame frame;

	/*
	 * We do not need RLP frame parsing to be done when we do not have
	 * callback specified.
	 */

	if (!DRVINSTANCE(state)->rlp_rx_callback) return GN_ERR_NONE;

	/*
	 * Anybody know the official meaning of the first two bytes?
	 * Nokia 6150 sends junk frames starting D9 01, and real frames starting
	 * D9 00. We'd drop the junk frames anyway because the FCS is bad, but
	 * it's tidier to do it here. We still need to call the callback function
	 * to give it a chance to handle timeouts and/or transmit a frame
	 */

	if (message[0] == 0xd9 && message[1] == 0x01) {
		DRVINSTANCE(state)->rlp_rx_callback(NULL);
		return GN_ERR_NONE;
	}

	/*
	 * Nokia uses 240 bit frame size of RLP frames as per GSM 04.22
	 * specification, so Header consists of 16 bits (2 bytes). See section
	 * 4.1 of the specification.
	 */

	frame.Header[0] = message[2];
	frame.Header[1] = message[3];

	/*
	 * Next 200 bits (25 bytes) contain the Information. We store the
	 * information in the Data array.
	 */

	memcpy(frame.Data, message + 4, 25);

	/* The last 24 bits (3 bytes) contain FCS. */
	frame.FCS[0] = message[29];
	frame.FCS[1] = message[30];
	frame.FCS[2] = message[31];

	/* Here we pass the frame down in the input stream. */

	DRVINSTANCE(state)->rlp_rx_callback(&frame);

	return GN_ERR_NONE;
}


#ifdef  SECURITY
static gn_error EnterSecurityCode(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32] = {FBUS_FRAME_HEADER, 0x0a};
	unsigned char *pos;
	int len;

	if (!data->security_code) return GN_ERR_INTERNALERROR;

	len = strlen(data->security_code->code);
	if (len < 0 || len >= 10) return GN_ERR_INTERNALERROR;

	pos = req + 4;
	*pos++ = data->security_code->type;
	memcpy(pos, data->security_code->code, len);
	pos += len;
	*pos++ = 0;
	*pos++ = 0;

	if (sm_message_send(pos - req, 0x08, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x08, data, state);
}

static gn_error GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07};

	if (!data->security_code) return GN_ERR_INTERNALERROR;

	if (sm_message_send(4, 0x08, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x08, data, state);
}

static gn_error ChangeSecurityCode(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32] = {FBUS_FRAME_HEADER, 0x04};
	unsigned char *pos;
	int len1, len2;

	if (!data->security_code) return GN_ERR_INTERNALERROR;

	len1 = strlen(data->security_code->code);
	len2 = strlen(data->security_code->new_code);
	if (len1 < 0 || len1 >= 10 || len2 < 0 || len2 >= 10) return GN_ERR_INTERNALERROR;

	pos = req + 4;
	*pos++ = data->security_code->type;
	memcpy(pos, data->security_code->code, len1);
	pos += len1;
	*pos++ = 0;
	memcpy(pos, data->security_code->new_code, len2);
	pos += len2;
	*pos++ = 0;

	if (sm_message_send(pos - req, 0x08, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x08, data, state);
}

static gn_error IncomingSecurityCode(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	/* change security code ok */
	case 0x05:
		break;

	/* security code error */
	case 0x06: /* --getsecuritycodestatus */
	case 0x09: /* --changesecuritycode */
	case 0x0c: /* --entersecuritycode */
		switch (message[4]) {
			case 0x6f: /* Insert SIM card */
				return GN_ERR_NOTREADY;
			case 0x79:
				return GN_ERR_SIMPROBLEM;
			case 0x88:
			case 0x8d:
				dprintf("Message: Security code wrong.\n");
				return GN_ERR_INVALIDSECURITYCODE;
			default:
				return GN_ERR_UNHANDLEDFRAME;
		}

	/* security code status */
	case 0x08:
		dprintf("Message: Security Code status received: ");
		switch (message[4]) {
		case GN_SCT_SecurityCode: dprintf("waiting for Security Code.\n"); break;
		case GN_SCT_Pin:  dprintf("waiting for PIN.\n"); break;
		case GN_SCT_Pin2: dprintf("waiting for PIN2.\n"); break;
		case GN_SCT_Puk:  dprintf("waiting for PUK.\n"); break;
		case GN_SCT_Puk2: dprintf("waiting for PUK2.\n"); break;
		case GN_SCT_None: dprintf("nothing to enter.\n"); break;
		default: dprintf("unknown\n"); return GN_ERR_UNHANDLEDFRAME;
		}
		if (data->security_code) data->security_code->type = message[4];
		break;

	/* security code OK */
	case 0x0b:
		dprintf("Message: Security code accepted.\n");
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}
#endif


static gn_error PressOrReleaseKey1(bool press, gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x42, 0x00, 0x00, 0x01};

	req[4] = press ? 0x01 : 0x02;
	req[5] = data->key_code;

	if (sm_message_send(7, 0x0c, req, state)) return GN_ERR_NOTREADY;
	return sm_block(0x0c, data, state);
}

static int ParseKey(gn_key_code key, unsigned char **ppos, struct gn_statemachine *state)
{
	unsigned char ch;
	int n;

	ch = **ppos;
	(*ppos)++;

	if (key == GN_KEY_NONE) return ch ? -1 : 0;

	for (n = 1; ch; n++) {
		DRVINSTANCE(state)->keytable[ch].key = key;
		DRVINSTANCE(state)->keytable[ch].repeat = n;
		ch = **ppos;
		(*ppos)++;
	}

	return 0;
}

static gn_error BuildKeytable(struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x40, 0x01};
	gn_error error;
	gn_data data;
	int i;

	/*
	 * This function builds a table, where the index is the character
	 * code to type, the value is the key code and the repeat count
	 * to press this key. The GN_KEY_NONE means this character cannot
	 * be represented, the GN_KEY_ASTERISK key means special symbols.
	 * We just initialise the table here, it will be filled by the
	 * IncomingKey() function which will be called as the result of
	 * this message which we send here.
	 */
	for (i = 0; i < 256; i++) {
		DRVINSTANCE(state)->keytable[i].key = GN_KEY_NONE;
		DRVINSTANCE(state)->keytable[i].repeat = 0;
	}

	gn_data_clear(&data);

	if (sm_message_send(5, 0x0c, req, state)) return GN_ERR_NOTREADY;
	if ((error = sm_block(0x0c, &data, state))) return error;

	return GN_ERR_NONE;
}

static gn_error PressOrReleaseKey(bool press, gn_data *data, struct gn_statemachine *state)
{
	if (DRVINSTANCE(state)->capabilities & NK6100_CAP_OLD_KEY_API)
		return PressOrReleaseKey2(press, data, state);
	else
		return PressOrReleaseKey1(press, data, state);
}

static gn_error PressKey(gn_key_code key, int d, struct gn_statemachine *state)
{
	gn_data data;
	gn_error error;

	gn_data_clear(&data);

	data.key_code = key;

	if ((error = PressOrReleaseKey(true, &data, state))) return error;
	if (d) usleep(d*1000);
	if ((error = PressOrReleaseKey(false, &data, state))) return error;

	return GN_ERR_NONE;
}

static gn_error EnterChar(gn_data *data, struct gn_statemachine *state)
{
	gn_key_code key;
	gn_error error;
	nk6100_keytable *keytable = DRVINSTANCE(state)->keytable;
	int i, r;

	/*
	 * Beware! Horrible kludge here. Try to write some sms manually with
	 * characters in different case and insert some symbols too. This
	 * function just "emulates" this process.
	 */

	if (isupper(data->character)) {
		/*
		 * the table contains only lowercase characters and symbols,
		 * so we must translate into lowercase.
		 */
		i = tolower(data->character);
		if (keytable[i].key == GN_KEY_NONE) return GN_ERR_UNKNOWN;
	} else if (islower(data->character)) {
		/*
		 * Ok, the character in proper case, but the phone in upper
		 * case mode (default). We must switch it into lowercase.
		 * We just have to press the hash key.
		 */
		i = data->character;
		if (keytable[i].key == GN_KEY_NONE) return GN_ERR_UNKNOWN;
		if ((error = PressKey(GN_KEY_HASH, 0, state))) return error;
	} else {
		/*
		 * This is a special character (number, space, symbol) which
		 * represents themself.
		 */
		i = data->character;
		if (keytable[i].key == GN_KEY_NONE) return GN_ERR_UNKNOWN;
	}
	if (keytable[i].key == GN_KEY_ASTERISK) {
		/*
		 * This is a special symbol character, so we have to press
		 * the asterisk key, the down key the repeat count minus one
		 * times and finish with the ok (menu) key.
		 */
		if ((error = PressKey(GN_KEY_ASTERISK, 0, state))) return error;
		key = GN_KEY_DOWN;
		r = 1;
	}
	else {
		key = keytable[i].key;
		r = 0;
	}

	for (; r < keytable[i].repeat; r++) {
		if ((error = PressKey(key, 0, state))) return error;
	}

	if (islower(data->character)) {
		/*
		 * This was a lowercase character. We must press the hash
		 * key again to go back to uppercase mode. We might store
		 * the lowercase/uppercase state, but it's much simpler.
		 */
		if ((error = PressKey(GN_KEY_HASH, 0, state))) return error;
	}
	else if (key == GN_KEY_DOWN) {
		/* This was a symbol character, press the menu key to finish */
		if ((error = PressKey(GN_KEY_MENU, 0, state))) return error;
	} else {
		/*
		 * Ok, the requested character is ready. But don't forget if
		 * we press the same key after it in a hurry it will toggle
		 * this char again! (Pressing key 2 will result "A", but if
		 * you press the key 2 again it will be "B".) Pressing the
		 * hash key is an easy workaround for it.
		 */
		if ((error = PressKey(GN_KEY_HASH, 0, state))) return error;
		if ((error = PressKey(GN_KEY_HASH, 0, state))) return error;
	}

	return GN_ERR_NONE;
}

static gn_error IncomingKey(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char *pos;

	switch (message[3]) {
	/* Press key ack */
	case 0x43:
		if (message[4] != 0x01 && message[4] != 0x02)
			return GN_ERR_UNHANDLEDFRAME;
		break;

	/* Get key assignments */
	case 0x41:
		/*
		 * This message contains the string of characters for each
		 * phone key. We will simply parse it and build a reverse
		 * translation table. The last string is a special case. It
		 * contains the available symbols which isn't represented on
		 * the keyboard directly, you can choose from the menu when
		 * you press the asterisk key (think about sms writeing).
		 */
		pos = message + 4;
		if (ParseKey(GN_KEY_1, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_2, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_3, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_4, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_5, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_6, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_7, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_8, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_9, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_0, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_NONE, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_NONE, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(GN_KEY_ASTERISK, &pos, state)) return GN_ERR_UNHANDLEDFRAME;
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error NBSUpload(gn_data *data, struct gn_statemachine *state, gn_sms_data_type type)
{
	unsigned char req[512] = {0x0c, 0x01};
	gn_sms sms;
	gn_sms_raw rawsms;
	gn_error err;
	int n;

	gn_sms_default_submit(&sms);
	sms.user_data[0].type = type;
	sms.user_data[1].type = GN_SMS_DATA_None;

	switch (type) {
	case GN_SMS_DATA_Bitmap:
		memcpy(&sms.user_data[0].u.bitmap, data->bitmap, sizeof(gn_bmp));
		break;
	case GN_SMS_DATA_Ringtone:
		memcpy(&sms.user_data[0].u.ringtone, data->ringtone, sizeof(gn_ringtone));
		break;
	default:
		return GN_ERR_INTERNALERROR;
	}

	memset(&rawsms, 0, sizeof(rawsms));

	if ((err = sms_prepare(&sms, &rawsms)) != GN_ERR_NONE) return err;

	n = 2 + rawsms.user_data_length;
	if (n > sizeof(req)) return GN_ERR_INTERNALERROR;
	memcpy(req + 2, rawsms.user_data, rawsms.user_data_length);

	return sm_message_send(n, 0x12, req, state);
}


static gn_error IncomingMisc(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	if (messagetype == 0xda && message[0] == 0x00 && message[1] == 0x00)
	{
		/* seems like a keepalive */
		return GN_ERR_UNSOLICITED;
	}

	return GN_ERR_UNHANDLEDFRAME;
}


static gn_error get_ringtone_list(gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone_list *rl;
	gn_ringtone ringtone;
	gn_data d;
	gn_error error;

#define ADDRINGTONE(id, str) \
	rl->ringtone[rl->count].location = (id); \
	snprintf(rl->ringtone[rl->count].name, sizeof(rl->ringtone[rl->count].name), "%s", (str)); \
	rl->ringtone[rl->count].user_defined = 0; \
	rl->ringtone[rl->count].readable = 0; \
	rl->ringtone[rl->count].writable = 0; \
	rl->count++;

	if (!(rl = data->ringtone_list)) return GN_ERR_INTERNALERROR;
	rl->userdef_location = 17;
	rl->userdef_count = 0;

	for (rl->count = 0; rl->count < GN_RINGTONE_MAX_COUNT; rl->count++) {
		memset(&ringtone, 0, sizeof(ringtone));
		gn_data_clear(&d);
		d.ringtone = &ringtone;
		ringtone.location = rl->userdef_location + rl->userdef_count;
		error = GetRingtone(&d, state);
		if (error == GN_ERR_NONE) {
			snprintf(rl->ringtone[rl->count].name, sizeof(rl->ringtone[rl->count].name), "%s", ringtone.name);
		} else if (error == GN_ERR_WRONGDATAFORMAT) {
			snprintf(rl->ringtone[rl->count].name, sizeof(rl->ringtone[rl->count].name), "%s", _("Unknown"));
		} else
			break;
		rl->ringtone[rl->count].location = ringtone.location;
		rl->ringtone[rl->count].user_defined = 1;
		rl->ringtone[rl->count].readable = 1;
		rl->ringtone[rl->count].writable = 1;
		rl->userdef_count++;
	}

	if (!memcmp(state->config.model, "61", 2)) {
		ADDRINGTONE(18, "Ring ring");
		ADDRINGTONE(19, "Low");
		ADDRINGTONE(20, "Fly");
		ADDRINGTONE(21, "Mosquito");
		ADDRINGTONE(22, "Bee");
		ADDRINGTONE(23, "Intro");
		ADDRINGTONE(24, "Etude");
		ADDRINGTONE(25, "Hunt");
		ADDRINGTONE(26, "Going up");
		ADDRINGTONE(27, "City Bird");
		ADDRINGTONE(30, "Chase");
		ADDRINGTONE(32, "Scifi");
		ADDRINGTONE(34, "Kick");
		ADDRINGTONE(35, "Do-mi-so");
		ADDRINGTONE(36, "Robo N1X");
		ADDRINGTONE(37, "Dizzy");
		ADDRINGTONE(39, "Playground");
		ADDRINGTONE(43, "That's it!");
		ADDRINGTONE(47, "Grande valse");
		ADDRINGTONE(48, "Helan");
		ADDRINGTONE(49, "Fuga");
		ADDRINGTONE(50, "Menuet");
		ADDRINGTONE(51, "Ode to Joy");
		ADDRINGTONE(52, "Elise");
		ADDRINGTONE(53, "Mozart 40");
		ADDRINGTONE(54, "Piano Concerto");
		ADDRINGTONE(55, "William Tell");
		ADDRINGTONE(56, "Badinerie");
		ADDRINGTONE(57, "Polka");
		ADDRINGTONE(58, "Attraction");
		ADDRINGTONE(60, "Polite");
		ADDRINGTONE(61, "Persuasion");
		ADDRINGTONE(67, "Tick tick");
		ADDRINGTONE(68, "Samba");
		ADDRINGTONE(70, "Orient");
		ADDRINGTONE(71, "Charleston");
		ADDRINGTONE(73, "Jumping");
	} else if (!memcmp(state->config.model, "51", 2)) {
		ADDRINGTONE(18, "Ring ring");
		ADDRINGTONE(19, "Low");
		ADDRINGTONE(20, "Fly");
		ADDRINGTONE(21, "Mosquito");
		ADDRINGTONE(22, "Bee");
		ADDRINGTONE(23, "Intro");
		ADDRINGTONE(24, "Etude");
		ADDRINGTONE(25, "Hunt");
		ADDRINGTONE(26, "Going up");
		ADDRINGTONE(27, "City Bird");
		ADDRINGTONE(30, "Chase");
		ADDRINGTONE(32, "Scifi");
		ADDRINGTONE(34, "Kick");
		ADDRINGTONE(35, "Do-mi-so");
		ADDRINGTONE(36, "Robo N1X");
		ADDRINGTONE(37, "Dizzy");
		ADDRINGTONE(39, "Playground");
		ADDRINGTONE(43, "That's it!");
		ADDRINGTONE(47, "Knock knock");
		ADDRINGTONE(48, "Grande valse");
		ADDRINGTONE(49, "Helan");
		ADDRINGTONE(50, "Fuga");
		ADDRINGTONE(51, "Ode to Joy");
		ADDRINGTONE(52, "Elise");
		ADDRINGTONE(54, "Mozart 40");
		ADDRINGTONE(56, "William Tell");
		ADDRINGTONE(57, "Badinerie");
		ADDRINGTONE(58, "Polka");
		ADDRINGTONE(59, "Attraction");
		ADDRINGTONE(60, "Down");
		ADDRINGTONE(62, "Persuasion");
		ADDRINGTONE(67, "Tick tick");
		ADDRINGTONE(69, "Samba");
		ADDRINGTONE(71, "Orient");
		ADDRINGTONE(72, "Charleston");
		ADDRINGTONE(73, "Songette");
		ADDRINGTONE(74, "Jumping");
		ADDRINGTONE(75, "Lamb");
		ADDRINGTONE(80, "Tango");
	}

	if (!rl->count) {
		rl->userdef_location = 0;
		rl->userdef_count = 0;
		return GN_ERR_NOTIMPLEMENTED;
	}

#undef ADDRINGTONE

	return GN_ERR_NONE;
}
