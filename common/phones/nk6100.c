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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>
  Copyright (C) 2002 BORBELY Zoltan

  This file provides functions specific to the 6100 series.
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
#include "phones/nk6100.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"
#include "gsm-sms.h"

#define	DRVINSTANCE(s) ((NK6100_DriverInstance *)((s)->Phone.DriverInstance))
#define	FREE(p) do { free(p); (p) = NULL; } while (0)

/* Some globals */

/* static functions prototypes */
static gn_error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static gn_error Initialise(GSM_Statemachine *state);
static gn_error Authentication(GSM_Statemachine *state, char *imei);
static gn_error BuildKeytable(GSM_Statemachine *state);
static gn_error Identify(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetSpeedDial(GSM_Data *data, GSM_Statemachine *state);
static gn_error PhoneInfo(GSM_Data *data, GSM_Statemachine *state);
static gn_error PhoneInfo2(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetRFLevel(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetPhoneID(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error SendSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetOnSMS(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static gn_error SaveSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static gn_error DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetBitmap(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetBitmap(GSM_Data *data, GSM_Statemachine *state);
static gn_error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static gn_error WritePhonebook(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetPowersource(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetWelcomeMessage(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetOperatorLogo(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetDateTime(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetDateTime(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetAlarm(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetAlarm(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetProfile(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetProfile(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static gn_error WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static gn_error DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetDisplayStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error DisplayOutput(GSM_Data *data, GSM_Statemachine *state);
static gn_error PollDisplay(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetSMSCenter(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetCellBroadcast(GSM_Data *data, GSM_Statemachine *state);
static gn_error NetMonitor(GSM_Data *data, GSM_Statemachine *state);
static gn_error MakeCall1(GSM_Data *data, GSM_Statemachine *state);
static gn_error AnswerCall1(GSM_Data *data, GSM_Statemachine *state);
static gn_error CancelCall1(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetCallNotification(GSM_Data *data, GSM_Statemachine *state);
static gn_error MakeCall(GSM_Data *data, GSM_Statemachine *state);
static gn_error AnswerCall(GSM_Data *data, GSM_Statemachine *state);
static gn_error CancelCall(GSM_Data *data, GSM_Statemachine *state);
static gn_error SendRLPFrame(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetRLPRXCallback(GSM_Data *data, GSM_Statemachine *state);
static gn_error SendDTMF(GSM_Data *data, GSM_Statemachine *state);
static gn_error Reset(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetRingtone(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetRawRingtone(GSM_Data *data, GSM_Statemachine *state);
static gn_error SetRawRingtone(GSM_Data *data, GSM_Statemachine *state);
static gn_error MakeCall2(GSM_Data *data, GSM_Statemachine *state);
static gn_error AnswerCall2(GSM_Data *data, GSM_Statemachine *state);
static gn_error CancelCall2(GSM_Data *data, GSM_Statemachine *state);
static gn_error PressOrReleaseKey(GSM_Data *data, GSM_Statemachine *state, bool press);
static gn_error EnterChar(GSM_Data *data, GSM_Statemachine *state);
static gn_error NBSUpload(GSM_Data *data, GSM_Statemachine *state, SMS_DataType type);
static gn_error get_imei(GSM_Data *data, GSM_Statemachine *state);
static gn_error get_phone_info(GSM_Data *data, GSM_Statemachine *state);
static gn_error get_hw(GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
static gn_error EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state);
static gn_error GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state);
static gn_error ChangeSecurityCode(GSM_Data *data, GSM_Statemachine *state);
#endif

static gn_error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingPhoneInfo2(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingSMS1(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingDisplay(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingCallInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingRLPFrame(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingKey(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static gn_error IncomingMisc(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
static gn_error IncomingSecurityCode(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
#endif

static int GetMemoryType(GSM_MemoryType memory_type);
static void FlushLostSMSNotifications(GSM_Statemachine *state);

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 0x01, IncomingCallInfo },
	{ 0x02, IncomingSMS1 },
	{ 0x03, IncomingPhonebook },
	{ 0x04, IncomingPhoneStatus },
	{ 0x05, IncomingProfile },
	{ 0x06, PNOK_IncomingCallDivert },
#ifdef	SECURITY
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

GSM_Phone phone_nokia_6100 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"6110|6130|6150|6190|5110|5130|5190|3210|3310|3330|3360|3410|8210|8290", /* Supported models */
		4,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GRF_Arbitrary,         /* RF level units */
		4,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GBU_Arbitrary,         /* Battery level units */
		GDT_DateTime,          /* Have date/time support */
		GDT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		48, 84,                /* Startup logo size */
		14, 72,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	Functions,
	NULL
};

struct {
	char *Model;
	char *SWVersion;
	int Capabilities;
} static P6100_capabilities[] = {
    	/*
	 * Capability setup for phone models.
	 * Example:
	 * { "NSE-3",	NULL,		P6100_CAP_OLD_CALL_API }
	 * Set NULL in the second field for all software versions.
	 */
	{ "NSE-3",	"-4.06",	P6100_CAP_NBS_UPLOAD },
	{ "NHM-5",      NULL,           P6100_CAP_PB_UNICODE },
	{ "NHM-6",      NULL,           P6100_CAP_PB_UNICODE },
	{ "NHM-2",      NULL,           P6100_CAP_PB_UNICODE },
	{ NULL,		NULL,		0 }
};

static gn_error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	if (!DRVINSTANCE(state) && op != GOP_Init) return GN_ERR_INTERNALERROR;

	switch (op) {
	case GOP_Init:
		if (DRVINSTANCE(state)) return GN_ERR_INTERNALERROR;
		return Initialise(state);
	case GOP_Terminate:
		FREE(DRVINSTANCE(state));
		return PGEN_Terminate(data, state);
	case GOP_GetSpeedDial:
		return GetSpeedDial(data, state);
	case GOP_SetSpeedDial:
		return SetSpeedDial(data, state);
	case GOP_GetImei:
	case GOP_GetModel:
	case GOP_GetRevision:
	case GOP_GetManufacturer:
	case GOP_Identify:
		return Identify(data, state);
	case GOP_GetBitmap:
		return GetBitmap(data, state);
	case GOP_SetBitmap:
		return SetBitmap(data, state);
	case GOP_GetBatteryLevel:
		return GetBatteryLevel(data, state);
	case GOP_GetRFLevel:
		return GetRFLevel(data, state);
	case GOP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GOP_ReadPhonebook:
		return ReadPhonebook(data, state);
	case GOP_WritePhonebook:
		return WritePhonebook(data, state);
	case GOP_GetPowersource:
		return GetPowersource(data, state);
	case GOP_GetSMSStatus:
		return GetSMSStatus(data, state);
	case GOP_GetNetworkInfo:
		return GetNetworkInfo(data, state);
	case GOP_SendSMS:
		return SendSMSMessage(data, state);
	case GOP_OnSMS:
		return SetOnSMS(data, state);
	case GOP_PollSMS:
		SM_Loop(state, 1);
		return GN_ERR_NONE;
	case GOP_GetSMS:
		return GetSMSMessage(data, state);
	case GOP_SaveSMS:
		return SaveSMSMessage(data, state);
	case GOP_DeleteSMS:
		return DeleteSMSMessage(data, state);
	case GOP_GetDateTime:
		return GetDateTime(data, state);
	case GOP_SetDateTime:
		return SetDateTime(data, state);
	case GOP_GetAlarm:
		return GetAlarm(data, state);
	case GOP_SetAlarm:
		return SetAlarm(data, state);
	case GOP_GetProfile:
		return GetProfile(data, state);
	case GOP_SetProfile:
		return SetProfile(data, state);
	case GOP_GetCalendarNote:
		return GetCalendarNote(data, state);
	case GOP_WriteCalendarNote:
		return WriteCalendarNote(data, state);
	case GOP_DeleteCalendarNote:
		return DeleteCalendarNote(data, state);
	case GOP_GetDisplayStatus:
		return GetDisplayStatus(data, state);
	case GOP_DisplayOutput:
		return DisplayOutput(data, state);
	case GOP_PollDisplay:
		return PollDisplay(data, state);
	case GOP_GetSMSCenter:
		return GetSMSCenter(data, state);
	case GOP_SetSMSCenter:
		return SetSMSCenter(data, state);
	case GOP_SetCellBroadcast:
		return SetCellBroadcast(data, state);
	case GOP_NetMonitor:
		return NetMonitor(data, state);
	case GOP_MakeCall:
		return MakeCall(data, state);
	case GOP_AnswerCall:
		return AnswerCall(data, state);
	case GOP_CancelCall:
		return CancelCall(data, state);
	case GOP_SetCallNotification:
		return SetCallNotification(data, state);
	case GOP_SendRLPFrame:
		return SendRLPFrame(data, state);
	case GOP_SetRLPRXCallback:
		return SetRLPRXCallback(data, state);
#ifdef	SECURITY
	case GOP_EnterSecurityCode:
		return EnterSecurityCode(data, state);
	case GOP_GetSecurityCodeStatus:
		return GetSecurityCodeStatus(data, state);
	case GOP_ChangeSecurityCode:
		return ChangeSecurityCode(data, state);
#endif
	case GOP_SendDTMF:
		return SendDTMF(data, state);
	case GOP_Reset:
		return Reset(data, state);
	case GOP_GetRingtone:
		return GN_ERR_NOTSUPPORTED;
	case GOP_SetRingtone:
		return SetRingtone(data, state);
	case GOP_GetRawRingtone:
		return GetRawRingtone(data, state);
	case GOP_SetRawRingtone:
		return SetRawRingtone(data, state);
	case GOP_PressPhoneKey:
		return PressOrReleaseKey(data, state, true);
	case GOP_ReleasePhoneKey:
		return PressOrReleaseKey(data, state, false);
	case GOP_EnterChar:
		return EnterChar(data, state);
	case GOP_CallDivert:
		return PNOK_CallDivert(data, state);
	default:
		dprintf("NK61xx unimplemented operation: %d\n", op);
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

static gn_error IdentifyPhone(GSM_Statemachine *state)
{
	NK6100_DriverInstance *drvinst = DRVINSTANCE(state);
	gn_error err;
	GSM_Data data;
	char revision[GSM_MAX_REVISION_LENGTH];

	GSM_DataClear(&data);
	data.Model = drvinst->Model;
	data.Imei = drvinst->IMEI;
	data.Revision = revision;

	if ((err = get_imei(&data, state)) != GN_ERR_NONE ||
	    (err = get_phone_info(&data, state)) != GN_ERR_NONE ||
	    (err = get_hw(&data, state)) != GN_ERR_NONE) {
		return err;
	}
	if ((drvinst->PM = GetPhoneModel(data.Model)) == NULL) {
		dump(_("Unsupported phone model \"%s\"\n"), data.Model);
		dump(_("Please read Docs/Reporting-HOWTO and send a bug report!\n"));
		return GN_ERR_INTERNALERROR;
	}

	if (drvinst->PM->flags & PM_AUTHENTICATION) {
		/* Now test the link and authenticate ourself */
		if ((err = PhoneInfo(&data, state)) != GN_ERR_NONE) return err;
	}

	sscanf(revision, "SW %9[^ 	,], HW %9s", drvinst->SWVersion, drvinst->HWVersion);

	return GN_ERR_NONE;
}

static bool match_phone(NK6100_DriverInstance *drvinst, int i)
{
	if (P6100_capabilities[i].Model != NULL && strcmp(P6100_capabilities[i].Model, drvinst->Model))
		return false;

	if (P6100_capabilities[i].SWVersion != NULL) {
		if (P6100_capabilities[i].SWVersion[0] == '-' && strcmp(P6100_capabilities[i].SWVersion + 1, drvinst->SWVersion) >= 0)
			return true;
		if (P6100_capabilities[i].SWVersion[0] == '+' && strcmp(P6100_capabilities[i].SWVersion + 1, drvinst->SWVersion) <= 0)
			return true;
		if (!strcmp(P6100_capabilities[i].SWVersion, drvinst->SWVersion))
			return true;
		return false;
	}

	return true;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error Initialise(GSM_Statemachine *state)
{
	gn_error err;
	int i;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6100, sizeof(GSM_Phone));

	if (!(DRVINSTANCE(state) = calloc(1, sizeof(NK6100_DriverInstance))))
		return GN_ERR_MEMORYFULL;

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		state->Link.ConnectionType = GCT_DAU9P;
	case GCT_Infrared:
	case GCT_DAU9P:
		err = FBUS_Initialise(&(state->Link), state, 0);
		break;
#ifdef HAVE_IRDA
	case GCT_Irda:
		err = PHONET_Initialise(&(state->Link), state);
		break;
#endif
	default:
		FREE(DRVINSTANCE(state));
		return GN_ERR_NOTSUPPORTED;
	}

	if (err != GN_ERR_NONE) {
		dprintf("Error in link initialisation\n");
		FREE(DRVINSTANCE(state));
		return GN_ERR_NOTSUPPORTED;
	}

	SM_Initialise(state);

	/* We need to identify the phone first in order to know whether we can
	   authorize or set keytable */

	if ((err = IdentifyPhone(state)) != GN_ERR_NONE) {
		FREE(DRVINSTANCE(state));
		return err;
	}

	for (i = 0; !match_phone(DRVINSTANCE(state), i); i++) ;
	DRVINSTANCE(state)->Capabilities = P6100_capabilities[i].Capabilities;

	if (DRVINSTANCE(state)->PM->flags & PM_AUTHENTICATION) {
		/* Now test the link and authenticate ourself */
		if ((err = Authentication(state, DRVINSTANCE(state)->IMEI)) != GN_ERR_NONE) {
			FREE(DRVINSTANCE(state));
			return err;
		}
	}

	if (DRVINSTANCE(state)->PM->flags & PM_KEYBOARD)
		if (BuildKeytable(state) != GN_ERR_NONE) {
			FREE(DRVINSTANCE(state));
			return GN_ERR_NOTSUPPORTED;
		}

	return GN_ERR_NONE;
}

static gn_error Identify(GSM_Data *data, GSM_Statemachine *state)
{
	NK6100_DriverInstance *drvinst = DRVINSTANCE(state);

	if (data->Manufacturer) PNOK_GetManufacturer(data->Manufacturer);
	if (data->Model) strcpy(data->Model, drvinst->Model);
	if (data->Imei) strcpy(data->Imei, drvinst->IMEI);
	if (data->Revision) snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW %s, HW %s", drvinst->SWVersion, drvinst->HWVersion);
	data->Phone = drvinst->PM;

	return GN_ERR_NONE;
}


static gn_error GetPhoneStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01};

	dprintf("Getting phone status...\n");
	if (SM_SendMessage(state, 4, 0x04, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x04);
}

static gn_error GetRFLevel(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting rf level...\n");
	return GetPhoneStatus(data, state);
}

static gn_error GetBatteryLevel(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting battery level...\n");
	return GetPhoneStatus(data, state);
}

static gn_error GetPowersource(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Getting power source...\n");
	return GetPhoneStatus(data, state);
}

static gn_error GetPhoneID(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03};

	dprintf("Getting phone id...\n");
	if (SM_SendMessage(state, 4, 0x04, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x04);
}

static gn_error IncomingPhoneStatus(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
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
		if (data->RFLevel && data->RFUnits) {
			switch (*data->RFUnits) {
			case GRF_CSQ:
				*data->RFLevel = csq_map[message[5]];
				break;
			case GRF_Arbitrary:
			default:
				*data->RFUnits = GRF_Arbitrary;
				*data->RFLevel = message[5];
				break;
			}
		}
		if (data->PowerSource) {
			*data->PowerSource = message[7];
		}
		if (data->BatteryLevel && data->BatteryUnits) {
			*data->BatteryUnits = GBU_Arbitrary;
			*data->BatteryLevel = message[8];
		}
		break;

	/* Get Phone ID */
	case 0x04:
		if (data->Imei) {
			snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 5);
			dprintf("Received imei %s\n", data->Imei);
		}
		if (data->Revision) {
			sscanf(message + 35, " %9s", hw);
			sscanf(message + 40, " %9s", sw);
			snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW %s, HW %s", sw, hw);
			dprintf("Received revision %s\n", data->Revision);
		}
		if (data->Model) {
			snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 21);
			dprintf("Received model %s\n", data->Model);
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetMemoryStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00};

	dprintf("Getting memory status...\n");
	req[4] = GetMemoryType(data->MemoryStatus->MemoryType);
	if (SM_SendMessage(state, 5, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static gn_error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00};

	dprintf("Reading phonebook location (%d/%d)\n", data->PhonebookEntry->MemoryType, data->PhonebookEntry->Location);
	req[4] = GetMemoryType(data->PhonebookEntry->MemoryType);
	req[5] = data->PhonebookEntry->Location;
	if (SM_SendMessage(state, 7, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static gn_error WritePhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[FBUS_MAX_FRAME_LENGTH] = {FBUS_FRAME_HEADER, 0x04};
	GSM_PhonebookEntry *pe;
	unsigned char *pos;
	int namelen, numlen;

	pe = data->PhonebookEntry;
	pos = req+4;
	namelen = strlen(pe->Name);
	numlen = strlen(pe->Number);
	dprintf("Writing phonebook location (%d/%d): %s\n", pe->MemoryType, pe->Location, pe->Name);
	if (namelen > GSM_MAX_PHONEBOOK_NAME_LENGTH) {
		dprintf("name too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	if (numlen > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}
	if (pe->SubEntriesCount > 1) {
		dprintf("61xx doesn't support subentries\n");
		return GN_ERR_UNKNOWN;
	}
	if ((pe->SubEntriesCount == 1) && ((pe->SubEntries[0].EntryType != GSM_Number)
		|| (pe->SubEntries[0].NumberType != GSM_General) || (pe->SubEntries[0].BlockNumber != 2)
		|| strcmp(pe->SubEntries[0].data.Number, pe->Number))) {
		dprintf("61xx doesn't support subentries\n");
		return GN_ERR_UNKNOWN;
	}
	*pos++ = GetMemoryType(pe->MemoryType);
	*pos++ = pe->Location;
	if (DRVINSTANCE(state)->Capabilities & P6100_CAP_PB_UNICODE) {
		*pos++ = (2 * namelen);
		namelen = char_encode_unicode(pos, pe->Name, namelen);
	} else {
		*pos++ = namelen;
		PNOK_EncodeString(pos, namelen, pe->Name);
	}
	pos += namelen;
	*pos++ = numlen;
	PNOK_EncodeString(pos, numlen, pe->Number);
	pos += numlen;
	*pos++ = (pe->Group == 5) ? 0xff : pe->Group;
	if (SM_SendMessage(state, pos-req, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static gn_error GetCallerGroupData(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x00};

	req[4] = data->Bitmap->number;
	if (SM_SendMessage(state, 5, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static gn_error GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	dprintf("Reading bitmap...\n");
	switch (data->Bitmap->type) {
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

static gn_error GetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x16, 0x00};

	req[4] = data->SpeedDial->Number;

	if (SM_SendMessage(state, 5, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static gn_error SetSpeedDial(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x19, 0x00, 0x00, 0x00};

	req[4] = data->SpeedDial->Number;
	req[5] = GetMemoryType(data->SpeedDial->MemoryType);
	req[6] = data->SpeedDial->Location;

	if (SM_SendMessage(state, 7, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x03);
}

static gn_error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	GSM_PhonebookEntry *pe;
	gn_bmp *bmp;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	case 0x02:
		if (data->PhonebookEntry) {
			pe = data->PhonebookEntry;
			pos = message + 5;
			pe->Empty = false;
			n = *pos++;
			/* It seems that older phones (at least Nokia 5110 and 6130)
			   set message[4] to 0. Newer ones set is to the location
			   number. It can be the distinction when to read the name */
			if (message[4] != 0)
				char_decode_unicode(pe->Name, pos, n);
			else
				PNOK_DecodeString(pe->Name, sizeof(pe->Name), pos, n);
			pos += n;
			n = *pos++;
			PNOK_DecodeString(pe->Number, sizeof(pe->Number), pos, n);
			pos += n;
			pe->Group = *pos++;
			pos++;
			pe->Date.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			pe->Date.Month = *pos++;
			pe->Date.Day = *pos++;
			pe->Date.Hour = *pos++;
			pe->Date.Minute = *pos++;
			pe->Date.Second = *pos++;
			pe->SubEntriesCount = 0;
		}
		break;
	case 0x03:
		if ((message[4] == 0x7d) || (message[4] == 0x74)) {
			return GN_ERR_INVALIDLOCATION;
		}
		return GN_ERR_UNHANDLEDFRAME;
	case 0x05:
		break;
	case 0x06:
		switch (message[4]) {
		case 0x7d:
		case 0x90:
			return GN_ERR_ENTRYTOOLONG;
		case 0x74:
			return GN_ERR_INVALIDLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
	case 0x08:
		dprintf("\tMemory location: %d\n", data->MemoryStatus->MemoryType);
		dprintf("\tUsed: %d\n", message[6]);
		dprintf("\tFree: %d\n", message[5]);
		if (data->MemoryStatus) {
			data->MemoryStatus->Used = message[6];
			data->MemoryStatus->Free = message[5];
			return GN_ERR_NONE;
		}
		break;
	case 0x09:
		switch (message[4]) {
		case 0x6f:
			return GN_ERR_TIMEOUT;
		case 0x7d:
			return GN_ERR_INTERNALERROR;
		case 0x8d:
			return GN_ERR_INVALIDSECURITYCODE;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
	case 0x11:
		if (data->Bitmap) {
			bmp = data->Bitmap;
			pos = message+4;
			bmp->number = *pos++;
			n = *pos++;
			PNOK_DecodeString(bmp->text, sizeof(bmp->text), pos, n);
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
		if (data->SpeedDial) {
			switch (message[4]) {
			case P6100_MEMORY_ME:
				data->SpeedDial->MemoryType = GMT_ME;
				break;
			case P6100_MEMORY_SM:
				data->SpeedDial->MemoryType = GMT_SM;
				break;
			default:
				return GN_ERR_UNHANDLEDFRAME;
			}
			data->SpeedDial->Location = message[5];
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
		return GN_ERR_INVALIDLOCATION;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error PhoneInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10};

	dprintf("Getting phone info (new way)...\n");

	if (SM_SendMessage(state, 4, 0x64, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x64);
}

static gn_error Authentication(GSM_Statemachine *state, char *imei)
{
	gn_error error;
	GSM_Data data;
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

	GSM_DataClear(&data);

	if ((error = SM_SendMessage(state, 7, 0x02, connect1)) != GN_ERR_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x02)) != GN_ERR_NONE)
		return error;

	if ((error = SM_SendMessage(state, 5, 0x02, connect2)) != GN_ERR_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x02)) != GN_ERR_NONE)
		return error;

	if ((error = SM_SendMessage(state, 7, 0x02, connect3)) != GN_ERR_NONE)
		return error;
	if ((error = SM_Block(state, &data, 0x02)) != GN_ERR_NONE)
		return error;

	if ((error = PhoneInfo(&data, state)) != GN_ERR_NONE) return error;

	PNOK_GetNokiaAuth(imei, DRVINSTANCE(state)->MagicBytes, magic_connect + 4);

	if ((error = SM_SendMessage(state, 45, 0x64, magic_connect)) != GN_ERR_NONE)
		return error;

	return GN_ERR_NONE;
}

static gn_error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	char hw[10], sw[10];

	switch (message[3]) {
	/* Phone ID recvd */
	case 0x11:
		if (data->Imei) {
			snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 9);
			dprintf("Received imei %s\n", data->Imei);
		}
		if (data->Model) {
			snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", message + 25);
			dprintf("Received model %s\n", data->Model);
		}
		if (data->Revision) {
			sscanf(message + 39, " %9s", hw);
			sscanf(message + 44, " %9s", sw);
			snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW %s, HW %s", sw, hw);
			dprintf("Received revision %s\n", data->Revision);
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

		memcpy(DRVINSTANCE(state)->MagicBytes, message + 50, 4);
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error PhoneInfo2(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x03, 0x00 };

	dprintf("Getting phone info (old way)...\n");

	if (SM_SendMessage(state, 5, 0xd1, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0xd2);

}

static gn_error IncomingPhoneInfo2(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	char sw[10];

	if (data->Model) {
		snprintf(data->Model, 6, "%s", message + 21);
	}
	if (data->Revision) {
		sscanf(message + 6, " %9s", sw);
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW %s, HW ????", sw);
	}
	dprintf("Phone info:\n%s\n", message + 4);
	return GN_ERR_NONE;
}


static int GetMemoryType(GSM_MemoryType memory_type)
{
	int result;

	switch (memory_type) {
	case GMT_MT:
		result = P6100_MEMORY_MT;
		break;
	case GMT_ME:
		result = P6100_MEMORY_ME;
		break;
	case GMT_SM:
		result = P6100_MEMORY_SM;
		break;
	case GMT_FD:
		result = P6100_MEMORY_FD;
		break;
	case GMT_ON:
		result = P6100_MEMORY_ON;
		break;
	case GMT_EN:
		result = P6100_MEMORY_EN;
		break;
	case GMT_DC:
		result = P6100_MEMORY_DC;
		break;
	case GMT_RC:
		result = P6100_MEMORY_RC;
		break;
	case GMT_MC:
		result = P6100_MEMORY_MC;
		break;
	default:
		result = P6100_MEMORY_XX;
		break;
	}
	return (result);
}


static gn_error GetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x33, 0x64, 0x00};

	req[5] = data->MessageCenter->No;

	if (SM_SendMessage(state, 6, 0x02, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static gn_error SetSMSCenter(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[64] = {FBUS_FRAME_HEADER, 0x30, 0x64};
	SMS_MessageCenter *smsc;
	unsigned char *pos;

	smsc = data->MessageCenter;
	pos = req+5;
	*pos++ = smsc->No;
	pos++;
	switch (smsc->Format) {
		case SMS_FText:
			*pos++ = 0x00;
			break;
		case SMS_FFax:
			*pos++ = 0x22;
			break;
		case SMS_FVoice:
			*pos++ = 0x24;
			break;
		case SMS_FERMES:
			*pos++ = 0x25;
			break;
		case SMS_FPaging:
			*pos++ = 0x26;
			break;
		case SMS_FX400:
			*pos++ = 0x31;
			break;
		case SMS_FEmail:
			*pos++ = 0x32;
			break;
		default:
			return GN_ERR_NOTSUPPORTED;
	}
	pos++;
	switch (smsc->Validity) {
		case SMS_V1H:
			*pos++ = 0x0b;
			break;
		case SMS_V6H:
			*pos++ = 0x47;
			break;
		case SMS_V24H:
			*pos++ = 0xa7;
			break;
		case SMS_V72H:
			*pos++ = 0xa9;
			break;
		case SMS_V1W:
			*pos++ = 0xad;
			break;
		case SMS_VMax:
			*pos++ = 0xff;
			break;
		default:
			return GN_ERR_NOTSUPPORTED;
	}
	*pos = (char_semi_octet_pack(smsc->Recipient.Number, pos + 1, smsc->Recipient.Type) + 1) / 2 + 1;
	pos += 12;
	*pos = (char_semi_octet_pack(smsc->SMSC.Number, pos + 1, smsc->SMSC.Type) + 1) / 2 + 1;
	pos += 12;
	if (smsc->DefaultName < 1) {
		snprintf(pos, 13, "%s", smsc->Name);
		pos += strlen(pos)+1;
	} else
		*pos++ = 0;

	if (SM_SendMessage(state, pos-req, 0x02, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static gn_error SetCellBroadcast(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req_ena[] = {FBUS_FRAME_HEADER, 0x20, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01};
	unsigned char req_dis[] = {FBUS_FRAME_HEADER, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char *req;

	req = data->OnCellBroadcast ? req_ena : req_dis;
	DRVINSTANCE(state)->OnCellBroadcast = data->OnCellBroadcast;

	if (SM_SendMessage(state, 10, 0x02, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x02);
}

static gn_error SendSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x01, 0x02, 0x00};
	GSM_Data dtemp;
	gn_error error;
	int len;

	/*
	 * FIXME:
	 * Ugly hack. The phone seems to drop the SendSMS message if the link
	 * had been idle too long. -- bozo
	 */
	GSM_DataClear(&dtemp);
	GetNetworkInfo(&dtemp, state);


	len = PNOK_FBUS_EncodeSMS(data, state, req + 6);
	len += 6;

	if (SM_SendMessage(state, len, PNOK_MSG_SMS, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	do {
		error = SM_BlockNoRetryTimeout(state, data, PNOK_MSG_SMS, state->Link.SMSTimeout);
	} while (!state->Link.SMSTimeout && error == GN_ERR_TIMEOUT);

	return error;
}

static bool CheckIncomingSMS(GSM_Statemachine *state, int pos)
{
	GSM_Data data;
	GSM_API_SMS sms;
	gn_error error;

	if (!DRVINSTANCE(state)->OnSMS) {
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
	sms.MemoryType = GMT_SM;
	sms.Number = pos;
	GSM_DataClear(&data);
	data.SMS = &sms;

	dprintf("trying to fetch sms#%hd\n", sms.Number);
	if ((error = gn_sms_get(&data, state)) != GN_ERR_NONE) {
		DRVINSTANCE(state)->sms_notification_in_progress = false;
		return false;
	}

	DRVINSTANCE(state)->OnSMS(&sms);

	dprintf("deleting sms#%hd\n", sms.Number);
	DeleteSMSMessage(&data, state);

	DRVINSTANCE(state)->sms_notification_in_progress = false;

	return true;
}

static void FlushLostSMSNotifications(GSM_Statemachine *state)
{
	int i;

	while (!DRVINSTANCE(state)->sms_notification_in_progress && DRVINSTANCE(state)->sms_notification_lost) {
		DRVINSTANCE(state)->sms_notification_lost = false;
		for (i = 1; i <= P6100_MAX_SMS_MESSAGES; i++)
			CheckIncomingSMS(state, i);
	}
}

static gn_error SetOnSMS(GSM_Data *data, GSM_Statemachine *state)
{
	if (data->OnSMS) {
		DRVINSTANCE(state)->OnSMS = data->OnSMS;
		DRVINSTANCE(state)->sms_notification_lost = true;
		FlushLostSMSNotifications(state);
	} else {
		DRVINSTANCE(state)->OnSMS = NULL;
	}

	return GN_ERR_NONE;
}

static gn_error IncomingSMS1(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	SMS_MessageCenter *smsc;
	GSM_CBMessage cbmsg;
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
		case GN_ERR_UNKNOWN: return GN_ERR_FAILED;
		default:         return error;
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
		break;

	/* Read CellBroadcast */
	case 0x23:
		if (DRVINSTANCE(state)->OnCellBroadcast) {
			memset(&cbmsg, 0, sizeof(cbmsg));
			cbmsg.New = true;
			cbmsg.Channel = message[7];
			n = char_unpack_7bit(0, length-10, sizeof(cbmsg.Message)-1, message+10, cbmsg.Message);
			char_decode_ascii(cbmsg.Message, cbmsg.Message, n);
			DRVINSTANCE(state)->OnCellBroadcast(&cbmsg);
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
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		return GN_ERR_UNHANDLEDFRAME;

	/* SMS center received */
	case 0x34:
		if (data->MessageCenter) {
			smsc = data->MessageCenter;
			pos = message + 4;
			smsc->No = *pos++;
			pos++;
			switch (*pos++) {
			case 0x00:	/* text */
				smsc->Format = SMS_FText;
				break;
			case 0x22:	/* fax */
				smsc->Format = SMS_FFax;
				break;
			case 0x24:	/* voice */
				smsc->Format = SMS_FVoice;
				break;
			case 0x25:	/* ERMES */
				smsc->Format = SMS_FERMES;
				break;
			case 0x26:	/* paging */
				smsc->Format = SMS_FPaging;
				break;
			case 0x31:	/* X.400 */
				smsc->Format = SMS_FX400;
				break;
			case 0x32:	/* email */
				smsc->Format = SMS_FEmail;
				break;
			default:
				return GN_ERR_UNHANDLEDFRAME;
			}
			pos++;
			switch (*pos++) {
			case 0x0b:	/*  1 hour */
				smsc->Validity = SMS_V1H;
				break;
			case 0x47:	/*  6 hours */
				smsc->Validity = SMS_V6H;
				break;
			case 0xa7:	/* 24 hours */
				smsc->Validity = SMS_V24H;
				break;
			case 0xa9:	/* 72 hours */
				smsc->Validity = SMS_V72H;
				break;
			case 0xad:	/*  1 week */
				smsc->Validity = SMS_V1W;
				break;
			case 0xff:	/* max.time */
				smsc->Validity = SMS_VMax;
				break;
			default:	/* Probably some error, but better treat it as 24h */
				smsc->Validity = SMS_V24H;
				break;
			}
			if (pos[0] % 2) pos[0]++;
			pos[0] = pos[0] / 2 + 1;
			snprintf(smsc->Recipient.Number, sizeof(smsc->Recipient.Number), "%s", char_get_bcd_number(pos));
			smsc->Recipient.Type = pos[1];
			pos += 12;
			snprintf(smsc->SMSC.Number, sizeof(smsc->SMSC.Number), "%s", char_get_bcd_number(pos));
			smsc->SMSC.Type = pos[1];
			pos += 12;
			/* FIXME: codepage must be investigated - bozo */
			if (pos[0] == 0x00) {
				snprintf(smsc->Name, sizeof(smsc->Name), _("Set %d"), smsc->No);
				smsc->DefaultName = smsc->No;
			} else {
				snprintf(smsc->Name, sizeof(smsc->Name), "%s", pos);
				smsc->DefaultName = -1;
			}
		}
		break;

	/* SMS center error recv */
	case 0x35:
		switch (message[4]) {
		case 0x01:
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


static gn_error GetSMSStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x36, 0x64 };

	if (SM_SendMessage(state, 5, 0x14, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static gn_error GetSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x07, 0x02 /* Unknown */, 0x00 /* Location */, 0x01, 0x64 };

	if (!data->RawSMS) return GN_ERR_INTERNALERROR;

	req[5] = data->RawSMS->Number;
	if (SM_SendMessage(state, 8, 0x02, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static gn_error SaveSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x04, 
				  0x07, /* status */
				  0x02, 
				  0x00, /* number */
				  0x02 }; /* type */
	int len;

	if (!data->RawSMS) return GN_ERR_INTERNALERROR;

	if (44 + data->RawSMS->UserDataLength > sizeof(req))
		return GN_ERR_WRONGDATAFORMAT;

	if (data->RawSMS->Type == SMS_Deliver) {	/* Inbox */
		dprintf("INBOX!\n");
		req[4] 		= 0x03;			/* SMS State - GSM_Unread */
		req[7] 		= 0x00;			/* SMS Type */
	}

	if (data->RawSMS->Status == SMS_Sent)
		req[4] -= 0x02;

	req[6] = data->RawSMS->Number;

#if 0
	req[20] = 0x01; /* SMS Submit */
	if (data->RawSMS->UDHIndicator)		req[20] |= 0x40;
	if (data->RawSMS->ValidityIndicator)	req[20] |= 0x10;

	req[24] = data->RawSMS->UserDataLength;
	memcpy(req + 37, data->RawSMS->Validity, 7);
	memcpy(req + 44, data->RawSMS->UserData, data->RawSMS->UserDataLength);
#endif
	len = PNOK_FBUS_EncodeSMS(data, state, req + 8);
	len += 8;

	if (SM_SendMessage(state, len, 0x14, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static gn_error DeleteSMSMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x0a, 0x02, 0x00 /* Location */ };

	if (!data->SMS) return GN_ERR_INTERNALERROR;
	req[5] = data->SMS->Number;
	if (SM_SendMessage(state, 6, 0x14, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x14);
}

static gn_error IncomingSMS(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int i;

	switch (message[3]) {
	/* save sms succeeded */
	case 0x05:
		dprintf("Message stored at %d\n", message[5]);
		if (data->RawSMS) data->RawSMS->Number = message[5];
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

	/* read sms */
	case 0x08:
		for (i = 0; i < length; i++)
			dprintf("[%02x(%d)]", message[i], i);
		dprintf("\n");

		if (!data->RawSMS) return GN_ERR_INTERNALERROR;

		memset(data->RawSMS, 0, sizeof(GSM_SMSMessage));

#define	getdata(d,dr,s)	(message[(data->RawSMS->Type == SMS_Deliver) ? (d) : ((data->RawSMS->Type == SMS_Delivery_Report) ? (dr) : (s))])
		switch (message[7]) {
		case 0x00: data->RawSMS->Type	= SMS_Deliver; break;
		case 0x01: data->RawSMS->Type	= SMS_Delivery_Report; break;
		case 0x02: data->RawSMS->Type	= SMS_Submit; break;
		default: return GN_ERR_UNHANDLEDFRAME;
		}
		data->RawSMS->Number		= message[6];
		data->RawSMS->MemoryType	= GMT_SM;
		data->RawSMS->Status		= message[4];

		data->RawSMS->DCS		= getdata(22, 21, 23);
		data->RawSMS->Length		= getdata(23, 22, 24);
		data->RawSMS->UDHIndicator	= message[20];
		memcpy(data->RawSMS->UserData, &getdata(43, 22, 44), data->RawSMS->Length);

		if (data->RawSMS->Type == SMS_Delivery_Report) {
			data->RawSMS->ReplyViaSameSMSC = message[11];
			memcpy(data->RawSMS->Time, message + 42, 7);
		}
		if (data->RawSMS->Type != SMS_Submit) {
			memcpy(data->RawSMS->SMSCTime, &getdata(36, 35, 0), 7);
			memcpy(data->RawSMS->MessageCenter, message + 8, 12);
			memcpy(data->RawSMS->RemoteNumber, &getdata(24, 23, 0), 12);
		}
#undef	getdata
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
		case 0x07:
			dprintf("\tEmpty SMS location.\n");
			return GN_ERR_EMPTYLOCATION;
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
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* sms status succeded */
	case 0x37:
		dprintf("Message: SMS Status Received\n");
		dprintf("\tThe number of messages: %d\n", message[10]);
		dprintf("\tUnread messages: %d\n", message[11]);
		data->SMSStatus->Unread = message[11];
		data->SMSStatus->Number = message[10];
		break;

	/* sms status failed */
	case 0x38:
		dprintf("Message: SMS Status error, probably not authorized by PIN\n");
		return GN_ERR_INTERNALERROR;

	/* unknown */
	default:
		dprintf("Unknown message.\n");
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}


static gn_error GetNetworkInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x70 };

	if (SM_SendMessage(state, 4, 0x0a, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x0a);
}

static gn_error IncomingNetworkInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	switch (message[3]) {
	/* Network info */
	case 0x71:
		if (data->NetworkInfo) {
			data->NetworkInfo->CellID[0] = message[10];
			data->NetworkInfo->CellID[1] = message[11];
			data->NetworkInfo->LAC[0] = message[12];
			data->NetworkInfo->LAC[1] = message[13];
			data->NetworkInfo->NetworkCode[0] = '0' + (message[14] & 0x0f);
			data->NetworkInfo->NetworkCode[1] = '0' + (message[14] >> 4);
			data->NetworkInfo->NetworkCode[2] = '0' + (message[15] & 0x0f);
			data->NetworkInfo->NetworkCode[3] = ' ';
			data->NetworkInfo->NetworkCode[4] = '0' + (message[16] & 0x0f);
			data->NetworkInfo->NetworkCode[5] = '0' + (message[16] >> 4);
			data->NetworkInfo->NetworkCode[6] = 0;
		}
		break;
	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetWelcomeMessage(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x16};

	if (SM_SendMessage(state, 4, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static gn_error GetOperatorLogo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x33, 0x01};

	req[4] = data->Bitmap->number;
	if (SM_SendMessage(state, 5, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static gn_error SetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512 + GN_BMP_MAX_SIZE] = {FBUS_FRAME_HEADER};
	gn_bmp *bmp;
	unsigned char *pos;
	int len;

	bmp = data->Bitmap;
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
		*pos = PNOK_EncodeString(pos+1, len, bmp->text);
		pos += *pos+1;
		if (SM_SendMessage(state, pos-req, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		return SM_Block(state, data, 0x05);

	case GN_BMP_DealerNoteText:
		len = strlen(bmp->text);
		if (len > 255) {
			dprintf("DealerNoteText is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		*pos++ = 0x18;
		*pos++ = 0x01;	/* one block */
		*pos++ = 0x03;
		*pos = PNOK_EncodeString(pos+1, len, bmp->text);
		pos += *pos+1;
		if (SM_SendMessage(state, pos-req, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		return SM_Block(state, data, 0x05);

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
		if (SM_SendMessage(state, pos-req, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		return SM_Block(state, data, 0x05);

	case GN_BMP_OperatorLogo:
		if (bmp->size > GN_BMP_MAX_SIZE) {
			dprintf("OperatorLogo is too long\n");
			return GN_ERR_INTERNALERROR;
		}
		if (DRVINSTANCE(state)->Capabilities & P6100_CAP_NBS_UPLOAD)
			return NBSUpload(data, state, SMS_BitmapData);
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
		if (SM_SendMessage(state, pos-req, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		return SM_Block(state, data, 0x05);

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
		*pos = PNOK_EncodeString(pos+1, len, bmp->text);
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
		if (SM_SendMessage(state, pos-req, 0x03, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		return SM_Block(state, data, 0x03);

	case GN_BMP_None:
	case GN_BMP_PictureMessage:
		return GN_ERR_NOTSUPPORTED;
	default:
		return GN_ERR_INTERNALERROR;
	}
}

static gn_error GetProfileFeature(GSM_Data *data, GSM_Statemachine *state, int id)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x13, 0x01, 0x00, 0x00};

	req[5] = data->Profile->Number;
	req[6] = (unsigned char)id;

	if (SM_SendMessage(state, 7, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static gn_error SetProfileFeature(GSM_Data *data, GSM_Statemachine *state, int id, int value)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01};

	req[5] = data->Profile->Number;
	req[6] = (unsigned char)id;
	req[7] = (unsigned char)value;
	dprintf("Setting profile %d feature %d to %d\n", req[5], req[6], req[7]);

	if (SM_SendMessage(state, 9, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static gn_error GetProfile(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x1a, 0x00};
	GSM_Profile *prof;
	gn_error error;
	int i;

	if (!data->Profile)
		return GN_ERR_UNKNOWN;
	prof = data->Profile;
	req[4] = prof->Number;

	if (SM_SendMessage(state, 5, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	if ((error = SM_Block(state, data, 0x05)) != GN_ERR_NONE)
		return error;

	for (i = 0; i <= 0x09; i++) {
		if ((error = GetProfileFeature(data, state, i)) != GN_ERR_NONE)
			return error;
	}

	if (prof->DefaultName > -1) {
		/* For N5110 */
		/* FIXME: It should be set for N5130 and 3210 too */
		if (!strcmp(DRVINSTANCE(state)->Model, "NSE-1")) {
			switch (prof->DefaultName) {
			case 0x00:
				snprintf(prof->Name, sizeof(prof->Name), _("Personal"));
				break;
			case 0x01:
				snprintf(prof->Name, sizeof(prof->Name), _("Car"));
				break;
			case 0x02:
				snprintf(prof->Name, sizeof(prof->Name), _("Headset"));
				break;
			default:
				snprintf(prof->Name, sizeof(prof->Name), _("Unknown (%d)"), prof->DefaultName);
				break;
			}
		} else {
			switch (prof->DefaultName) {
			case 0x00:
				snprintf(prof->Name, sizeof(prof->Name), _("General"));
				break;
			case 0x01:
				snprintf(prof->Name, sizeof(prof->Name), _("Silent"));
				break;
			case 0x02:
				snprintf(prof->Name, sizeof(prof->Name), _("Meeting"));
				break;
			case 0x03:
				snprintf(prof->Name, sizeof(prof->Name), _("Outdoor"));
				break;
			case 0x04:
				snprintf(prof->Name, sizeof(prof->Name), _("Pager"));
				break;
			case 0x05:
				snprintf(prof->Name, sizeof(prof->Name), _("Car"));
				break;
			case 0x06:
				snprintf(prof->Name, sizeof(prof->Name), _("Headset"));
				break;
			default:
				snprintf(prof->Name, sizeof(prof->Name), _("Unknown (%d)"), prof->DefaultName);
				break;
			}
		}
	}

	return GN_ERR_NONE;
}

static gn_error SetProfile(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[64] = {FBUS_FRAME_HEADER, 0x1c, 0x01, 0x03};
	GSM_Profile *prof;
	gn_error error;

	if (!data->Profile)
		return GN_ERR_UNKNOWN;
	prof = data->Profile;
	dprintf("Setting profile %d (%s)\n", prof->Number, prof->Name);

	if (prof->Number == 0) {
		/* We cannot rename the General profile! - bozo */

		/*
		 * FIXME: We must to do something. We cannot ask the name
		 * of the General profile, because it's language dependent.
		 * But without SetProfileName we aren't able to set features.
		 */
		dprintf("You cannot rename General profile\n");
		return GN_ERR_NOTSUPPORTED;
	} else if (prof->DefaultName > -1) {
		prof->Name[0] = 0;
	}

	req[7] = prof->Number;
	req[8] = PNOK_EncodeString(req+9, 39, prof->Name);
	req[6] = req[8] + 2;

	if (SM_SendMessage(state, req[8]+9, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	if ((error = SM_Block(state, data, 0x05)) != GN_ERR_NONE)
		return error;

	error  = SetProfileFeature(data, state, 0x00, prof->KeypadTone);
	error |= SetProfileFeature(data, state, 0x01, prof->Lights);
	error |= SetProfileFeature(data, state, 0x02, prof->CallAlert);
	error |= SetProfileFeature(data, state, 0x03, prof->Ringtone);
	error |= SetProfileFeature(data, state, 0x04, prof->Volume);
	error |= SetProfileFeature(data, state, 0x05, prof->MessageTone);
	error |= SetProfileFeature(data, state, 0x06, prof->Vibration);
	error |= SetProfileFeature(data, state, 0x07, prof->WarningTone);
	error |= SetProfileFeature(data, state, 0x08, prof->CallerGroups);
	error |= SetProfileFeature(data, state, 0x09, prof->AutomaticAnswer);

	return (error == GN_ERR_NONE) ? GN_ERR_NONE : GN_ERR_UNKNOWN;
}

static gn_error SetRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[7+GSM_MAX_RINGTONE_PACKAGE_LENGTH] = {FBUS_FRAME_HEADER, 0x36, 0x00, 0x00, 0x78};
	int size;

	if (!data || !data->Ringtone) return GN_ERR_INTERNALERROR;

	if (DRVINSTANCE(state)->Capabilities & P6100_CAP_NBS_UPLOAD)
		return NBSUpload(data, state, SMS_RingtoneData);

	size = GSM_MAX_RINGTONE_PACKAGE_LENGTH;
	GSM_PackRingtone(data->Ringtone, req + 7, &size);
	req[4] = data->Ringtone->Location;

	if (SM_SendMessage(state, 7 + size, 0x05, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x05);
}

static gn_error IncomingProfile(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	gn_bmp *bmp;
	GSM_Profile *prof;
	unsigned char *pos;
	int i;
	bool found;

	switch (message[3]) {
	/* Set profile feat. OK */
	case 0x11:
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

	/* Get profile feature */
	case 0x14:
		if (data->Profile) {
			prof = data->Profile;
			switch (message[6]) {
			case 0x00:
				prof->KeypadTone = message[8];
				break;
			case 0x01:
				prof->Lights = message[8];
				break;
			case 0x02:
				prof->CallAlert = message[8];
				break;
			case 0x03:
				prof->Ringtone = message[8];
				break;
			case 0x04:
				prof->Volume = message[8];
				break;
			case 0x05:
				prof->MessageTone = message[8];
				break;
			case 0x06:
				prof->Vibration = message[8];
				break;
			case 0x07:
				prof->WarningTone = message[8];
				break;
			case 0x08:
				prof->CallerGroups = message[8];
				break;
			case 0x09:
				prof->AutomaticAnswer = message[8];
				break;
			default:
				return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;

	/* Get Welcome Message */
	case 0x17:
		if (data->Bitmap) {
			bmp = data->Bitmap;
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
					PNOK_DecodeString(bmp->text, sizeof(bmp->text), pos+1, *pos);
					pos += *pos + 1;
					break;
				case 0x03:
					if (bmp->type != GN_BMP_DealerNoteText) {
						pos += *pos + 1;
						continue;
					}
					PNOK_DecodeString(bmp->text, sizeof(bmp->text), pos+1, *pos);
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
		if (data->Profile) {
			if (message[9] == 0x00) {
				data->Profile->DefaultName = message[8];
				data->Profile->Name[0] = 0;
			} else {
				data->Profile->DefaultName = -1;
				PNOK_DecodeString(data->Profile->Name, sizeof(data->Profile->Name), message+10, message[9]);
			}
			break;
		} else {
			return GN_ERR_UNKNOWN;
		}
		break;

	/* Set profile name OK */
	case 0x1d:
		switch (message[4]) {
		case 0x01:
			break;
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
		if (data->Bitmap) {
			bmp = data->Bitmap;
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


static gn_error GetDateTime(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x62};

	if (SM_SendMessage(state, 4, 0x11, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static gn_error SetDateTime(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x60, 0x01, 0x01, 0x07,
			       0x00, 0x00,	/* year - H/L */
			       0x00, 0x00,	/* month, day */
			       0x00, 0x00,	/* yours, minutes */
			       0x00};		/* Unknown, but not seconds - try 59 and wait 1 sec. */

	req[7] = data->DateTime->Year >> 8;
	req[8] = data->DateTime->Year & 0xff;
	req[9] = data->DateTime->Month;
	req[10] = data->DateTime->Day;
	req[11] = data->DateTime->Hour;
	req[12] = data->DateTime->Minute;

	if (SM_SendMessage(state, 14, 0x11, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static gn_error GetAlarm(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6d};

	if (SM_SendMessage(state, 4, 0x11, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static gn_error SetAlarm(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x6b, 0x01, 0x20, 0x03,
			       0x02,		/* should be alarm on/off, but it doesn't works */
			       0x00, 0x00,	/* hours, minutes */
			       0x00};		/* Unknown, but not seconds - try 59 and wait 1 sec. */

	if (data->DateTime->AlarmEnabled) {
		req[8] = data->DateTime->Hour;
		req[9] = data->DateTime->Minute;
	} else {
		dprintf("Clearing the alarm clock isn't supported\n");
		return GN_ERR_NOTSUPPORTED;
	}

	if (SM_SendMessage(state, 11, 0x11, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x11);
}

static gn_error IncomingPhoneClockAndAlarm(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	GSM_DateTime *date;
	unsigned char *pos;

	switch (message[3]) {
	/* Date and time set */
	case 0x61:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Date and time received */
	case 0x63:
		if (data->DateTime) {
			date = data->DateTime;
			pos = message+8;
			date->Year = (pos[0] << 8) | pos[1];
			pos += 2;
			date->Month = *pos++;
			date->Day = *pos++;
			date->Hour = *pos++;
			date->Minute = *pos++;
			date->Second = *pos++;

			dprintf("Message: Date and time\n");
			dprintf("   Time: %02d:%02d:%02d\n", date->Hour, date->Minute, date->Second);
			dprintf("   Date: %4d/%02d/%02d\n", date->Year, date->Month, date->Day);
		}
		break;

	/* Alarm set */
	case 0x6c:
		switch (message[4]) {
		case 0x01:
			break;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Alarm received */
	case 0x6e:
		if (data->DateTime) {
			date = data->DateTime;
			pos = message + 8;
			date->AlarmEnabled = (*pos++ == 2);
			date->Hour = *pos++;
			date->Minute = *pos++;
			date->Second = 0;

			dprintf("Message: Alarm\n");
			dprintf("   Alarm: %02d:%02d\n", date->Hour, date->Minute);
			dprintf("   Alarm is %s\n", date->AlarmEnabled ? "on" : "off");
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error GetCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x66, 0x00};

	req[4] = data->CalendarNote->Location;

	if (SM_SendMessage(state, 5, 0x13, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x13);
}

static gn_error WriteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x64, 0x01, 0x10,
				 0x00,	/* Length of the rest of the frame. */
				 0x00};	/* The type of calendar note */
	GSM_CalendarNote *note;
	unsigned char *pos;
	unsigned int numlen;

	if (!data->CalendarNote)
		return GN_ERR_UNKNOWN;

	note = data->CalendarNote;
	pos = req + 7;
	numlen = strlen(note->Phone);
	if (numlen > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		return GN_ERR_UNKNOWN;
	}

	*pos++ = note->Type;

	*pos++ = note->Time.Year >> 8;
	*pos++ = note->Time.Year & 0xff;
	*pos++ = note->Time.Month;
	*pos++ = note->Time.Day;
	*pos++ = note->Time.Hour;
	*pos++ = note->Time.Minute;
	*pos++ = note->Time.Timezone;

	if (note->Alarm.Year) {
		*pos++ = note->Alarm.Year >> 8;
		*pos++ = note->Alarm.Year & 0xff;
		*pos++ = note->Alarm.Month;
		*pos++ = note->Alarm.Day;
		*pos++ = note->Alarm.Hour;
		*pos++ = note->Alarm.Minute;
		*pos++ = note->Alarm.Timezone;
	} else {
		memset(pos, 0x00, 7);
		pos += 7;
	}
	

	*pos = PNOK_EncodeString(pos+1, 255, note->Text);
	pos += *pos+1;

	if (note->Type == GCN_CALL) {
		*pos++ = numlen;
		memcpy(pos, note->Phone, numlen);
		pos += numlen;
	} else {
		*pos++ = 0;
	}

	if (SM_SendMessage(state, pos-req, 0x13, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x13);
}

static gn_error DeleteCalendarNote(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x68, 0x00};

	req[4] = data->CalendarNote->Location;

	if (SM_SendMessage(state, 5, 0x13, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x13);
}

static gn_error IncomingCalendar(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	GSM_CalendarNote *note;
	unsigned char *pos;
	int n;

	switch (message[3]) {
	/* Write cal.note report */
	case 0x65:
		switch (message[4]) {
			case 0x01:
				return GN_ERR_NONE;
			case 0x73:
			case 0x7d:
				return GN_ERR_UNKNOWN;
			default:
				return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* Calendar note recvd */
	case 0x67:
		switch (message[4]) {
		case 0x01:
			break;
		case 0x93:
			return GN_ERR_EMPTYLOCATION;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		if (data->CalendarNote) {
			note = data->CalendarNote;
			pos = message + 8;

			/* FIXME: this supposed to be replaced by general date unpacking function :-) */
			note->Type = *pos++;
			note->Time.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			note->Time.Month = *pos++;
			note->Time.Day = *pos++;
			note->Time.Hour = *pos++;
			note->Time.Minute = *pos++;
			note->Time.Second = *pos++;

			note->Alarm.Year = (pos[0] << 8) + pos[1];
			pos += 2;
			note->Alarm.Month = *pos++;
			note->Alarm.Day = *pos++;
			note->Alarm.Hour = *pos++;
			note->Alarm.Minute = *pos++;
			note->Alarm.Second = *pos++;
			note->Alarm.AlarmEnabled = (note->Alarm.Year != 0);
			n = *pos++;
			PNOK_DecodeString(note->Text, sizeof(note->Text), pos, n);
			pos += n;

			if (note->Type == GCN_CALL) {
				/* This will be replaced later :-) */
				n = *pos++;
				PNOK_DecodeString(note->Phone, sizeof(note->Phone), pos, n);
				pos += n;
			} else {
				note->Phone[0] = 0;
			}
		}
		break;

	/* Del. cal.note report */
	case 0x69:
		switch (message[4]) {
		case 0x01:
			break;
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

static gn_error GetDisplayStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x51};

	if (SM_SendMessage(state, 4, 0x0d, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x0d);
}

static gn_error PollDisplay(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Data dummy;

	GSM_DataClear(&dummy);
	GetDisplayStatus(&dummy, state);

	return GN_ERR_NONE;
}

static gn_error DisplayOutput(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x53, 0x00};

	if (data->DisplayOutput->OutputFn) {
		DRVINSTANCE(state)->DisplayOutput = data->DisplayOutput;
		req[4] = 0x01;
	} else {
		DRVINSTANCE(state)->DisplayOutput = NULL;
		req[4] = 0x02;
	}

	if (SM_SendMessage(state, 5, 0x0d, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x0d);
}

static gn_error IncomingDisplay(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int state_table[8] = { 1 << DS_Call_In_Progress, 1 << DS_Unknown,
			       1 << DS_Unread_SMS, 1 << DS_Voice_Call,
			       1 << DS_Fax_Call, 1 << DS_Data_Call,
			       1 << DS_Keyboard_Lock, 1 << DS_SMS_Storage_Full };
	unsigned char *pos;
	int n, x, y, st;
	GSM_DrawMessage drawmsg;
	GSM_DisplayOutput *disp = DRVINSTANCE(state)->DisplayOutput;
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
			if (n > DRAW_MAX_SCREEN_WIDTH) return GN_ERR_INTERNALERROR;

			/*
			 * WARNING: the following part can damage your mind!
			 * This is so ugly, but there's no other way. Nokia
			 * forgot the clear screen from the protocol, so we
			 * must implement some heuristic here... :-( - bozo
			 */
			time_limit.tv_sec = 0;
			time_limit.tv_usec = 200000;
			gettimeofday(&now, NULL);
			timersub(&now, &disp->Last, &time_diff);
			if (y > 9 && timercmp(&time_diff, &time_limit, >))
				disp->State = true;
			disp->Last = now;

			if (disp->State && y > 9) {
				disp->State = false;
				memset(&drawmsg, 0, sizeof(drawmsg));
				drawmsg.Command = GSM_Draw_ClearScreen;
				disp->OutputFn(&drawmsg);
			}
			/* Maybe this is unneeded, please leave uncommented
			if (x == 0 && y == 46 && pos[1] != 'M') {
				disp->State = true;
			}
			*/

			memset(&drawmsg, 0, sizeof(drawmsg));
			drawmsg.Command = GSM_Draw_DisplayText;
			drawmsg.Data.DisplayText.x = x;
			drawmsg.Data.DisplayText.y = y;
			char_decode_unicode(drawmsg.Data.DisplayText.text, pos, n << 1);
			disp->OutputFn(&drawmsg);

			dprintf("(x,y): %d,%d, len: %d, data: %s\n", x, y, n, drawmsg.Data.DisplayText.text);
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
		if (data->DisplayStatus)
			*data->DisplayStatus = st;
		if (disp) {
			memset(&drawmsg, 0, sizeof(drawmsg));
			drawmsg.Command = GSM_Draw_DisplayStatus;
			drawmsg.Data.DisplayStatus = st;
			disp->OutputFn(&drawmsg);
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


static gn_error EnableExtendedCmds(GSM_Data *data, GSM_Statemachine *state, unsigned char type)
{
	unsigned char req[] = {0x00, 0x01, 0x64, 0x00};

	if (type == 0x06) {
		dump(_("Tried to activate CONTACT SERVICE\n"));
		return GN_ERR_INTERNALERROR;
	}

	req[3] = type;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error NetMonitor(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x7e, 0x00};
	gn_error error;

	req[3] = data->NetMonitor->Field;

	if ((error = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE) return error;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error Reset(GSM_Data *data, GSM_Statemachine *state)
{
	if (!data) return GN_ERR_INTERNALERROR;
	if (data->ResetType != 0x03 && data->ResetType != 0x04) return GN_ERR_INTERNALERROR;

	return EnableExtendedCmds(data, state, data->ResetType);
}

static gn_error GetRawRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x9e, 0x00};
	gn_error error;

	if (!data || !data->Ringtone || !data->RawData) return GN_ERR_INTERNALERROR;

	req[3] = data->Ringtone->Location;

	if ((error = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE) return error;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error SetRawRingtone(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512] = {0x00, 0x01, 0xa0, 0x00, 0x00,
				  0x0c, 0x2c, 0x01,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x02, 0xfc, 0x09};
	gn_error error;

	if (!data || !data->Ringtone || !data->RawData || !data->RawData->Data)
		return GN_ERR_INTERNALERROR;

	req[3] = data->Ringtone->Location;
	snprintf(req + 8, 13, "%s", data->Ringtone->name);
	memcpy(req + 24, data->RawData->Data, data->RawData->Length);

	if ((error = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE) return error;

	if (SM_SendMessage(state, 24 + data->RawData->Length, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error MakeCall2(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[5 + GSM_MAX_PHONEBOOK_NUMBER_LENGTH] = {0x00, 0x01, 0x7c, 0x01};
	int n;
	gn_error err;

	switch (data->CallInfo->Type) {
	case GSM_CT_VoiceCall:
		break;

	case GSM_CT_NonDigitalDataCall:
	case GSM_CT_DigitalDataCall:
		dprintf("Unsupported call type %d\n", data->CallInfo->Type);
		return GN_ERR_NOTSUPPORTED;

	default:
		dprintf("Invalid call type %d\n", data->CallInfo->Type);
		return GN_ERR_INTERNALERROR;
	}

	n = strlen(data->CallInfo->Number);
	if (n > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}

	if ((err = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE)
		return err;

	strcpy(req + 4, data->CallInfo->Number);

	if (SM_SendMessage(state, 5 + n, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error AnswerCall2(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[4] = {0x00, 0x01, 0x7c, 0x02};
	gn_error err;

	if ((err = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE)
		return err;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error CancelCall2(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[4] = {0x00, 0x01, 0x7c, 0x03};
	gn_error err;

	if ((err = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE)
		return err;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error get_imei(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x66};
	gn_error err;

	if ((err = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE)
		return err;

	if (SM_SendMessage(state, 3, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error get_phone_info(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0xc8, 0x01};
	gn_error err;

	if ((err = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE)
		return err;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error get_hw(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0xc8, 0x05};
	gn_error err;

	if ((err = EnableExtendedCmds(data, state, 0x01)) != GN_ERR_NONE)
		return err;

	if (SM_SendMessage(state, 4, 0x40, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static gn_error IncomingSecurity(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	char *aux, *aux2;

	switch (message[2]) {
	/* FIXME: maybe "Enable extended cmds" reply? - bozo */
	case 0x64:
		break;

	/* IMEI */
	case 0x66:
		if (data->Imei) {
			dprintf("IMEI: %s\n", message + 4);
			snprintf(data->Imei, GSM_MAX_IMEI_LENGTH, "%s", message + 4);
		}
		break;

	/* call management (old style) */
	case 0x7c:
		if (message[3] < 0x01 || message[3] > 0x03)
			return GN_ERR_UNHANDLEDFRAME;
		break;

	/* Netmonitor */
	case 0x7e:
		if (message[3] != 0x00 && data->NetMonitor)
			snprintf(data->NetMonitor->Screen, sizeof(data->NetMonitor->Screen), "%s", message + 4);
		break;

	/* Get bin ringtone */
	case 0x9e:
		switch (message[4]) {
		case 0x00:
			break;
		case 0x0a:
			return GN_ERR_UNKNOWN;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		if (!data->Ringtone) return GN_ERR_INTERNALERROR;
		data->Ringtone->Location = message[3];
		snprintf(data->Ringtone->name, sizeof(data->Ringtone->name), "%s", message + 8);
		if (data->RawData && data->RawData->Data) {
			memcpy(data->RawData->Data, message + 24, length - 24);
			data->RawData->Length = length - 24;
		}
		break;
	
	/* Set bin ringtone result */
	case 0xa0:
		if (message[3] != 0x02) return GN_ERR_UNHANDLEDFRAME;
		break;

	case 0xc8:
		switch (message[3]) {
		case 0x01:
			if (data->Revision) {
				aux = message + 7;
				aux2 = index(aux, 0x0a);
				if (data->Revision[0]) {
					dprintf("Niepusty\n");
					strcat(data->Revision, ", SW ");
					strncat(data->Revision, aux,
						aux2 - aux);
				} else {
					snprintf(data->Revision, aux2 - aux + 4,
						 "SW %s", aux);
				}
				dprintf("Received %s\n", data->Revision);
			}
			aux = index(message + 5, 0x0a);
			aux++;
			aux = index(aux, 0x0a);
			aux++;
			if (data->Model) {
				aux2 = index(aux, 0x0a);
				*aux2 = 0;
				snprintf(data->Model, GSM_MAX_MODEL_LENGTH, "%s", aux);
				dprintf("Received model %s\n", data->Model);
			}
			break;
		case 0x05:
			if (data->Revision) {
				if (data->Revision[0]) {
					dprintf("Niepusty\n");
					strcat(data->Revision, ", HW ");
					strncat(data->Revision, message + 5,
						GSM_MAX_REVISION_LENGTH);
				} else {
					snprintf(data->Revision, GSM_MAX_REVISION_LENGTH,
						 "HW %s", message + 5);
				}
				dprintf("Received %s\n", data->Revision);
			}
			break;
		default:
			return GN_ERR_NOTIMPLEMENTED;
		}
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}

static gn_error MakeCall1(GSM_Data *data, GSM_Statemachine *state)
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
	GSM_Data dtemp;

	n = strlen(data->CallInfo->Number);
	if (n > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GN_ERR_ENTRYTOOLONG;
	}

	/*
	 * FIXME:
	 * Ugly hack. The phone seems to drop the SendSMS message if the link
	 * had been idle too long. -- bozo
	 */
	GSM_DataClear(&dtemp);
	GetNetworkInfo(&dtemp, state);

	pos = req + 4;
	*pos++ = (unsigned char)n;
	memcpy(pos, data->CallInfo->Number, n);
	pos += n;

	switch (data->CallInfo->Type) {
	case GSM_CT_VoiceCall:
		dprintf("Voice Call\n");
		switch (data->CallInfo->SendNumber) {
		case GSM_CSN_Never:
			voice_end[5] = 0x02;
			break;
		case GSM_CSN_Always:
			voice_end[5] = 0x03;
			break;
		case GSM_CSN_Default:
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
		if (SM_SendMessage(state, pos - req, 0x01, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		break;

	case GSM_CT_NonDigitalDataCall:
		dprintf("Non Digital Data Call\n");
		memcpy(pos, data_nondigital_end, ARRAY_LEN(data_nondigital_end));
		pos += ARRAY_LEN(data_nondigital_end);
		if (SM_SendMessage(state, pos - req, 0x01, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		usleep(10000);
		dprintf("after nondigital1\n");
		if (SM_SendMessage(state, ARRAY_LEN(data_nondigital_final), 0x01, data_nondigital_final) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		dprintf("after nondigital2\n");
		break;

	case GSM_CT_DigitalDataCall:
		dprintf("Digital Data Call\n");
		if (SM_SendMessage(state, ARRAY_LEN(data_digital_pred1), 0x01, data_digital_pred1) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		usleep(500000);
		dprintf("after digital1\n");
		if (SM_SendMessage(state, ARRAY_LEN(data_digital_pred2), 0x01, data_digital_pred2) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		usleep(500000);
		dprintf("after digital2\n");
		memcpy(pos, data_digital_end, ARRAY_LEN(data_digital_end));
		pos += ARRAY_LEN(data_digital_end);
		if (SM_SendMessage(state, pos - req, 0x01, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
		dprintf("after digital3\n");
		break;

	default:
		dprintf("Invalid call type %d\n", data->CallInfo->Type);
		return GN_ERR_INTERNALERROR;
	}

	return SM_BlockNoRetryTimeout(state, data, 0x01, 500);
}

static gn_error AnswerCall1(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req1[] = {FBUS_FRAME_HEADER, 0x42, 0x05, 0x01, 0x07,
				0xa2, 0x88, 0x81, 0x21, 0x15, 0x63, 0xa8, 0x00, 0x00,
				0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x06, 0x00, 0x00};

	if (SM_SendMessage(state, sizeof(req1), 0x01, req1) != GN_ERR_NONE) return GN_ERR_NOTREADY;

	req2[4] = data->CallInfo->CallID;

	if (SM_SendMessage(state, sizeof(req2), 0x01, req2) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x01);
}

static gn_error CancelCall1(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00, 0x85};

	req[4] = data->CallInfo->CallID;

	if (SM_SendMessage(state, 6, 0x01, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	SM_BlockNoRetry(state, data, 0x01);
	return GN_ERR_NONE;
}

static gn_error SetCallNotification(GSM_Data *data, GSM_Statemachine *state)
{
	DRVINSTANCE(state)->CallNotification = data->CallNotification;

	return GN_ERR_NONE;
}

static gn_error MakeCall(GSM_Data *data, GSM_Statemachine *state)
{
	if (DRVINSTANCE(state)->Capabilities & P6100_CAP_OLD_CALL_API)
		return MakeCall2(data, state);
	else
		return MakeCall1(data, state);
}

static gn_error AnswerCall(GSM_Data *data, GSM_Statemachine *state)
{
	if (DRVINSTANCE(state)->Capabilities & P6100_CAP_OLD_CALL_API)
		return AnswerCall2(data, state);
	else
		return AnswerCall1(data, state);
}

static gn_error CancelCall(GSM_Data *data, GSM_Statemachine *state)
{
	if (DRVINSTANCE(state)->Capabilities & P6100_CAP_OLD_CALL_API)
		return CancelCall2(data, state);
	else
		return CancelCall1(data, state);
}

static gn_error SendDTMF(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[5+256] = {FBUS_FRAME_HEADER, 0x50};
	int len;

	if (!data || !data->DTMFString) return GN_ERR_INTERNALERROR;

	len = strlen(data->DTMFString);
	if (len < 0 || len >= 256) return GN_ERR_INTERNALERROR;

	req[4] = len;
	memcpy(req + 5, data->DTMFString, len);

	if (SM_SendMessage(state, 5 + len, 0x01, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x01);
}

static gn_error IncomingCallInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	GSM_CallInfo cinfo;
	unsigned char *pos;

	switch (message[3]) {
	/* Call going msg */
	case 0x02:
		if (data->CallInfo)
			data->CallInfo->CallID = message[4];
		break;

	/* Call in progress */
	case 0x03:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (DRVINSTANCE(state)->CallNotification)
			DRVINSTANCE(state)->CallNotification(GSM_CS_Established, &cinfo, state);
		if (!data->CallInfo) return GN_ERR_UNSOLICITED;
		data->CallInfo->CallID = message[4];
		break;

	/* Remote end hang up */
	case 0x04:
		isdn_cause2gn_error(NULL, NULL, message[7], message[6]);
		if (data->CallInfo) {
			data->CallInfo->CallID = message[4];
			return GN_ERR_UNKNOWN;
		}
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (DRVINSTANCE(state)->CallNotification)
			DRVINSTANCE(state)->CallNotification(GSM_CS_RemoteHangup, &cinfo, state);
		return GN_ERR_UNSOLICITED;

	/* incoming call alert */
	case 0x05:
		memset(&cinfo, 0, sizeof(cinfo));
		pos = message + 4;
		cinfo.CallID = *pos++;
		pos++;
		if (*pos >= sizeof(cinfo.Number))
			return GN_ERR_UNHANDLEDFRAME;
		memcpy(cinfo.Number, pos + 1, *pos);
		pos += *pos + 1;
		if (*pos >= sizeof(cinfo.Name))
			return GN_ERR_UNHANDLEDFRAME;
		memcpy(cinfo.Name, pos + 1, *pos);
		pos += *pos + 1;
		if (DRVINSTANCE(state)->CallNotification)
			DRVINSTANCE(state)->CallNotification(GSM_CS_IncomingCall, &cinfo, state);
		return GN_ERR_UNSOLICITED;
	
	/* answered call */
	case 0x07:
		return GN_ERR_UNSOLICITED;

	/* terminated call */
	case 0x09:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (DRVINSTANCE(state)->CallNotification)
			DRVINSTANCE(state)->CallNotification(GSM_CS_LocalHangup, &cinfo, state);
		if (!data->CallInfo) return GN_ERR_UNSOLICITED;
		data->CallInfo->CallID = message[4];
		break;
	
	/* message after "terminated call" */
	case 0x0a:
		return GN_ERR_UNSOLICITED;

	/* call held */
	case 0x23:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (DRVINSTANCE(state)->CallNotification)
			DRVINSTANCE(state)->CallNotification(GSM_CS_CallHeld, &cinfo, state);
		return GN_ERR_UNSOLICITED;

	/* call resumed */
	case 0x25:
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.CallID = message[4];
		if (DRVINSTANCE(state)->CallNotification)
			DRVINSTANCE(state)->CallNotification(GSM_CS_CallResumed, &cinfo, state);
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


static gn_error SendRLPFrame(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[32] = {0x00, 0xd9};

	/*
	 * Discontinuos transmission (DTX).
	 * See section 5.6 of GSM 04.22 version 7.0.1.
	 */

	if (data->RLP_OutDTX) req[1] = 0x01;
	memcpy(req + 2, (unsigned char *) data->RLP_Frame, 30);

	/*
	 * It's ugly like the hell, maybe we should implement SM_SendFrame
	 * and extend the GSM_Link structure. We should clean this up in the
	 * future... - bozo
	 * return SM_SendFrame(state, 32, 0xf0, req);
	 */

	return FBUS_TX_SendFrame(32, 0xf0, req);
}

static gn_error SetRLPRXCallback(GSM_Data *data, GSM_Statemachine *state)
{
	DRVINSTANCE(state)->RLP_RXCallback = data->RLP_RX_Callback;

	return GN_ERR_NONE;
}

static gn_error IncomingRLPFrame(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	RLP_F96Frame frame;

	/*
	 * We do not need RLP frame parsing to be done when we do not have
	 * callback specified.
	 */

	if (!DRVINSTANCE(state)->RLP_RXCallback) return GN_ERR_NONE;

	/*
	 * Anybody know the official meaning of the first two bytes?
	 * Nokia 6150 sends junk frames starting D9 01, and real frames starting
	 * D9 00. We'd drop the junk frames anyway because the FCS is bad, but
	 * it's tidier to do it here. We still need to call the callback function
	 * to give it a chance to handle timeouts and/or transmit a frame
	 */

	if (message[0] == 0xd9 && message[1] == 0x01) {
		DRVINSTANCE(state)->RLP_RXCallback(NULL);
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

	DRVINSTANCE(state)->RLP_RXCallback(&frame);

	return GN_ERR_NONE;
}


#ifdef  SECURITY
static gn_error EnterSecurityCode(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[32] = {FBUS_FRAME_HEADER, 0x0a};
	unsigned char *pos;
	int len;

	if (!data->SecurityCode) return GN_ERR_INTERNALERROR;

	len = strlen(data->SecurityCode->Code);
	if (len < 0 || len >= 10) return GN_ERR_INTERNALERROR;

	pos = req + 4;
	*pos++ = data->SecurityCode->Type;
	memcpy(pos, data->SecurityCode->Code, len);
	pos += len;
	*pos++ = 0;
	*pos++ = 0;

	if (SM_SendMessage(state, pos - req, 0x08, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x08);
}

static gn_error GetSecurityCodeStatus(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07};

	if (!data->SecurityCode) return GN_ERR_INTERNALERROR;

	if (SM_SendMessage(state, 4, 0x08, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x08);
}

static gn_error ChangeSecurityCode(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[32] = {FBUS_FRAME_HEADER, 0x04};
	unsigned char *pos;
	int len1, len2;

	if (!data->SecurityCode) return GN_ERR_INTERNALERROR;

	len1 = strlen(data->SecurityCode->Code);
	len2 = strlen(data->SecurityCode->NewCode);
	if (len1 < 0 || len1 >= 10 || len2 < 0 || len2 >= 10) return GN_ERR_INTERNALERROR;

	pos = req + 4;
	*pos++ = data->SecurityCode->Type;
	memcpy(pos, data->SecurityCode->Code, len1);
	pos += len1;
	*pos++ = 0;
	memcpy(pos, data->SecurityCode->NewCode, len2);
	pos += len2;
	*pos++ = 0;

	if (SM_SendMessage(state, pos - req, 0x08, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x08);
}

static gn_error IncomingSecurityCode(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	switch (message[3]) {
	/* change security code ok */
	case 0x05:
		break;

	/* change security code error */
	case 0x06:
		if (message[4] != 0x88) return GN_ERR_UNHANDLEDFRAME;
		dprintf("Message: Security code wrong.\n");
		return GN_ERR_INVALIDSECURITYCODE;
		
	/* security code status */
	case 0x08:
		dprintf("Message: Security Code status received: ");
		switch (message[4]) {
		case GSCT_SecurityCode: dprintf("waiting for Security Code.\n"); break;
		case GSCT_Pin: 	dprintf("waiting for PIN.\n"); break;
		case GSCT_Pin2: dprintf("waiting for PIN2.\n"); break;
		case GSCT_Puk: 	dprintf("waiting for PUK.\n"); break;
		case GSCT_Puk2: dprintf("waiting for PUK2.\n"); break;
		case GSCT_None: dprintf("nothing to enter.\n"); break;
		default: dprintf("Unknown!\n"); return GN_ERR_UNHANDLEDFRAME;
		}
		if (data->SecurityCode) data->SecurityCode->Type = message[4];
		break;
	
	/* security code OK */
	case 0x0b:
		dprintf("Message: Security code accepted.\n");
		break;
	
	/* security code wrong */
	case 0x0c:
		if (message[4] != 0x88) return GN_ERR_UNHANDLEDFRAME;
		dprintf("Message: Security code wrong.\n");
		return GN_ERR_INVALIDSECURITYCODE;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}
#endif


static gn_error PressOrReleaseKey(GSM_Data *data, GSM_Statemachine *state, bool press)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x42, 0x00, 0x00, 0x01};

	req[4] = press ? 0x01 : 0x02;
	req[5] = data->KeyCode;

	if (SM_SendMessage(state, 7, 0x0c, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	return SM_Block(state, data, 0x0c);
}

static int ParseKey(GSM_Statemachine *state, GSM_KeyCode key, unsigned char **ppos)
{
	unsigned char ch;
	int n;

	ch = **ppos;
	(*ppos)++;

	if (key == GSM_KEY_NONE) return ch ? -1 : 0;

	for (n = 1; ch; n++) {
		DRVINSTANCE(state)->Keytable[ch].Key = key;
		DRVINSTANCE(state)->Keytable[ch].Repeat = n;
		ch = **ppos;
		(*ppos)++;
	}

	return 0;
}

static gn_error BuildKeytable(GSM_Statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x40, 0x01};
	gn_error error;
	GSM_Data data;
	int i;

	/*
	 * This function builds a table, where the index is the character
	 * code to type, the value is the key code and the repeat count
	 * to press this key. The GSM_KEY_NONE means this character cannot
	 * be represented, the GSM_KEY_ASTERISK key means special symbols.
	 * We just initialise the table here, it will be filled by the
	 * IncomingKey() function which will be called as the result of
	 * this message which we send here.
	 */
	for (i = 0; i < 256; i++) {
		DRVINSTANCE(state)->Keytable[i].Key = GSM_KEY_NONE;
		DRVINSTANCE(state)->Keytable[i].Repeat = 0;
	}

	GSM_DataClear(&data);

	if (SM_SendMessage(state, 5, 0x0c, req) != GN_ERR_NONE) return GN_ERR_NOTREADY;
	if ((error = SM_Block(state, &data, 0x0c)) != GN_ERR_NONE) return error;

	return GN_ERR_NONE;
}

static gn_error PressKey(GSM_Statemachine *state, GSM_KeyCode key, int d)
{
	GSM_Data data;
	gn_error error;

	GSM_DataClear(&data);

	data.KeyCode = key;

	if ((error = PressOrReleaseKey(&data, state, true)) != GN_ERR_NONE) return error;
	if (d) usleep(d*1000);
	if ((error = PressOrReleaseKey(&data, state, false)) != GN_ERR_NONE) return error;

	return GN_ERR_NONE;
}

static gn_error EnterChar(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_KeyCode key;
	gn_error error;
	NK6100_Keytable *Keytable = DRVINSTANCE(state)->Keytable;
	int i, r;

	/*
	 * Beware! Horrible kludge here. Try to write some sms manually with
	 * characters in different case and insert some symbols too. This
	 * function just "emulates" this process.
	 */

	if (isupper(data->Character)) {
		/*
		 * the table contains only lowercase characters and symbols,
		 * so we must translate into lowercase.
		 */
		i = tolower(data->Character);
		if (Keytable[i].Key == GSM_KEY_NONE) return GN_ERR_UNKNOWN;
	} else if (islower(data->Character)) {
		/*
		 * Ok, the character in proper case, but the phone in upper
		 * case mode (default). We must switch it into lowercase.
		 * We just have to press the hash key.
		 */
		i = data->Character;
		if (Keytable[i].Key == GSM_KEY_NONE) return GN_ERR_UNKNOWN;
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GN_ERR_NONE) return error;
	} else {
		/*
		 * This is a special character (number, space, symbol) which
		 * represents themself.
		 */
		i = data->Character;
		if (Keytable[i].Key == GSM_KEY_NONE) return GN_ERR_UNKNOWN;
	}
	if (Keytable[i].Key == GSM_KEY_ASTERISK) {
		/*
		 * This is a special symbol character, so we have to press
		 * the asterisk key, the down key the repeat count minus one
		 * times and finish with the ok (menu) key.
		 */
		if ((error = PressKey(state, GSM_KEY_ASTERISK, 0)) != GN_ERR_NONE) return error;
		key = GSM_KEY_DOWN;
		r = 1;
	}
	else {
		key = Keytable[i].Key;
		r = 0;
	}

	for (; r < Keytable[i].Repeat; r++) {
		if ((error = PressKey(state, key, 0)) != GN_ERR_NONE) return error;
	}

	if (islower(data->Character)) {
		/*
		 * This was a lowercase character. We must press the hash
		 * key again to go back to uppercase mode. We might store
		 * the lowercase/uppercase state, but it's much simpler.
		 */
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GN_ERR_NONE) return error;
	}
	else if (key == GSM_KEY_DOWN) {
		/* This was a symbol character, press the menu key to finish */
		if ((error = PressKey(state, GSM_KEY_MENU, 0)) != GN_ERR_NONE) return error;
	} else {
		/*
		 * Ok, the requested character is ready. But don't forget if
		 * we press the same key after it in a hurry it will toggle
		 * this char again! (Pressing key 2 will result "A", but if
		 * you press the key 2 again it will be "B".) Pressing the
		 * hash key is an easy workaround for it.
		 */
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GN_ERR_NONE) return error;
		if ((error = PressKey(state, GSM_KEY_HASH, 0)) != GN_ERR_NONE) return error;
	}

	return GN_ERR_NONE;
}

static gn_error IncomingKey(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
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
		if (ParseKey(state, GSM_KEY_1, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_2, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_3, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_4, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_5, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_6, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_7, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_8, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_9, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_0, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_NONE, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_NONE, &pos)) return GN_ERR_UNHANDLEDFRAME;
		if (ParseKey(state, GSM_KEY_ASTERISK, &pos)) return GN_ERR_UNHANDLEDFRAME;
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}


static gn_error NBSUpload(GSM_Data *data, GSM_Statemachine *state, SMS_DataType type)
{
	unsigned char req[512] = {0x0c, 0x01};
	GSM_API_SMS sms;
	GSM_SMSMessage rawsms;
	gn_error err;
	int n;

	gn_sms_default_submit(&sms);
	sms.UserData[0].Type = type;
	sms.UserData[1].Type = SMS_NoData;

	switch (type) {
	case SMS_BitmapData:
		memcpy(&sms.UserData[0].u.Bitmap, data->Bitmap, sizeof(gn_bmp));
		break;
	case SMS_RingtoneData:
		memcpy(&sms.UserData[0].u.Ringtone, data->Ringtone, sizeof(GSM_Ringtone));
		break;
	default:
		return GN_ERR_INTERNALERROR;
	}

	memset(&rawsms, 0, sizeof(rawsms));

	if ((err = sms_prepare(&sms, &rawsms)) != GN_ERR_NONE) return err;

	n = 2 + rawsms.UserDataLength;
	if (n > sizeof(req)) return GN_ERR_INTERNALERROR;
	memcpy(req + 2, rawsms.UserData, rawsms.UserDataLength);

	return SM_SendMessage(state, n, 0x12, req);
}


static gn_error IncomingMisc(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	if (messagetype == 0xda && message[0] == 0x00 && message[1] == 0x00)
	{
		/* seems like a keepalive */
		return GN_ERR_UNSOLICITED;
	}

	return GN_ERR_UNHANDLEDFRAME;
}
