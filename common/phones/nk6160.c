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
//#include "phones/nk6160.h"
#include "links/m2bus.h"
#include "phones/nokia.h"
#include "gsm-encoding.h"
#include "gsm-api.h"

/* Some globals */

/* FIXME: we should get rid on it */
static GSM_Statemachine *State = NULL;
static int logoslice;

/* static functions prototypes */
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Initialise(GSM_Statemachine *state);
static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error WritePhonebook(GSM_Data *data, GSM_Statemachine *state);
static GSM_Error GetBitmap(GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
#endif

static GSM_Error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error IncomingStartupLogo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state);

#ifdef  SECURITY
#endif

//static int GetMemoryType(GSM_MemoryType memory_type);

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 0xd2, IncomingPhoneInfo },
	{ 0x40, IncomingPhonebook },
	{ 0xdd, IncomingStartupLogo },
#ifdef	SECURITY
#endif
	{ 0, NULL}
};

GSM_Phone phone_nokia_6160 = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	/* Mobile phone information */
	{
		"6160|5120",           /* Supported models */
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

static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	switch (op) {
	case GOP_Init:
		return Initialise(state);
	case GOP_Terminate:
		return PGEN_Terminate(data, state);
//	case GOP_GetImei:
	case GOP_GetModel:
	case GOP_GetRevision:
	case GOP_GetManufacturer:
	case GOP_Identify:
		return Identify(data, state);
	case GOP_ReadPhonebook:
		return ReadPhonebook(data, state);
	case GOP_WritePhonebook:
		return WritePhonebook(data, state);
	case GOP_GetBitmap:
		return GetBitmap(data, state);
#ifdef	SECURITY
#endif
	default:
		dprintf("NK6160 unimplemented operation: %d\n", op);
		return GE_NOTIMPLEMENTED;
	}
}

/* Initialise is the only function allowed to 'use' state */
static GSM_Error Initialise(GSM_Statemachine *state)
{
	GSM_Error err;
	char model[GSM_MAX_MODEL_LENGTH+1];
	GSM_Data data;
	PhoneModel *pm;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_6160, sizeof(GSM_Phone));

	switch (state->Link.ConnectionType) {
	case GCT_Serial:
	case GCT_Infrared:
		err = M2BUS_Initialise(&(state->Link), state);
		break;
	default:
		return GE_NOTSUPPORTED;
	}

	if (err != GE_NONE) {
		dprintf("Error in link initialisation\n");
		return GE_NOTSUPPORTED;
	}

	SM_Initialise(state);

	/* We need to identify the phone first in order to know whether we can
	   authorize or set keytable */
	memset(model, 0, sizeof(model));
	GSM_DataClear(&data);
	data.Model = model;

	if ((err = Identify(&data, state)) != GE_NONE) return err;
	dprintf("model: '%s'\n", model);
	if ((pm = GetPhoneModel(model)) == NULL) {
		dump(_("Unsupported phone model \"%s\"\n"), model);
		dump(_("Please read Docs/Reporting-HOWTO and send a bug report!\n"));
		return GE_INTERNALERROR;
	}

	State = state;

	return GE_NONE;
}


static GSM_Error ReadPhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x1f, 0x01, 0x04, 0x86, 0x00};

	dprintf("Reading phonebook location (%d)\n", data->PhonebookEntry->Location);

	req[6] = data->PhonebookEntry->Location;
	if (SM_SendMessage(state, 7, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static GSM_Error WritePhonebook(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[512] = {0x00, 0x01, 0x1f, 0x01, 0x04, 0x87, 0x00};
	GSM_PhonebookEntry *pe;
	unsigned char *pos;
	int namelen, numlen;

	pe = data->PhonebookEntry;
	pos = req + 6;
	namelen = strlen(pe->Name);
	numlen = strlen(pe->Number);
	dprintf("Writing phonebook location (%d): %s\n", pe->Location, pe->Name);
	if (namelen > GSM_MAX_PHONEBOOK_NAME_LENGTH) {
		dprintf("name too long\n");
		return GE_ENTRYTOOLONG;
	}
	if (numlen > GSM_MAX_PHONEBOOK_NUMBER_LENGTH) {
		dprintf("number too long\n");
		return GE_ENTRYTOOLONG;
	}
	if (pe->SubEntriesCount > 1) {
		dprintf("6160 doesn't support subentries\n");
		return GE_UNKNOWN;
	}
	if ((pe->SubEntriesCount == 1) && ((pe->SubEntries[0].EntryType != GSM_Number)
		|| (pe->SubEntries[0].NumberType != GSM_General) || (pe->SubEntries[0].BlockNumber != 2)
		|| strcmp(pe->SubEntries[0].data.Number, pe->Number))) {
		dprintf("6160 doesn't support subentries\n");
		return GE_UNKNOWN;
	}

	*pos++ = pe->Location;
	strcpy(pos, pe->Number);
	pos += numlen + 1;
	strcpy(pos, pe->Name);
	pos += namelen + 1;

	if (SM_SendMessage(state, pos-req, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0x40);
}

static GSM_Error IncomingPhonebook(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	GSM_PhonebookEntry *pe;
	unsigned char *pos;
	unsigned char respheader[] = {0x01, 0x00, 0xc9, 0x04, 0x01};

	if (memcmp(message, respheader, sizeof(respheader)))
		return GE_UNHANDLEDFRAME;

	switch (message[5]) {
	/* read phonebook reply */
	case 0x86:
		if (!data->PhonebookEntry) break;

		pe = data->PhonebookEntry;
		pos = message + 8;

		switch (message[7]) {
		case 0x01: break;
		case 0x05: return GE_INVALIDLOCATION;
		default: return GE_UNHANDLEDFRAME;
		}
		snprintf(pe->Number, sizeof(pe->Number), "%s", pos);
		pos += strlen(pos) + 1;
		snprintf(pe->Name, sizeof(pe->Name), "%s", pos);
		memset(&pe->Date, 0, sizeof(pe->Date));
		pe->SubEntriesCount = 0;
		pe->Group = 5;
		pe->Empty = (*pe->Name != 0);
		break;

	/* write phonebook reply */
	case 0x87:
		switch (message[7]) {
		case 0x01: break;
		case 0x05: return GE_INVALIDLOCATION;
		default: return GE_UNHANDLEDFRAME;
		}
		break;

	default:
		return GE_UNHANDLEDFRAME;
	}

	return GE_NONE;
}

static GSM_Error PhoneInfo(GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char req[] = {0x00, 0x01, 0x00, 0x03, 0x00};
	GSM_Error error;

	dprintf("Getting phone info...\n");
	if (SM_SendMessage(state, 5, 0xd1, req) != GE_NONE) return GE_NOTREADY;
	error = SM_Block(state, data, 0xd2);
	return error;
}

static GSM_Error Identify(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error;

	dprintf("Identifying...\n");
	if (data->Manufacturer) PNOK_GetManufacturer(data->Manufacturer);

	if ((error = PhoneInfo(data, state)) != GE_NONE) return error;

	/* Check that we are back at state Initialised */
	if (SM_Loop(state, 0) != Initialised) return GE_UNKNOWN;
	return GE_NONE;
}

static GSM_Error IncomingPhoneInfo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	unsigned char *pos;

	if (data->Model) {
		snprintf(data->Model, 6, "%s", message + 21);
	}
	if (data->Revision) {
		snprintf(data->Revision, GSM_MAX_REVISION_LENGTH, "SW: %s", message + 6);
		if ((pos = strchr(data->Revision, '\n')) != NULL) *pos = 0;
	}
	dprintf("Phone info:\n%s\n", message + 4);
	return GE_NONE;
}


static GSM_Error GetStartupSlice(GSM_Data *data, GSM_Statemachine *state, int s)
{
	unsigned char req[] = {0x00, 0x01, 0x07, 0x07, 0x08, 0x00};

	logoslice = s;
	req[5] = (unsigned char)s + 1;

	if (SM_SendMessage(state, 6, 0x40, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 0xdd);
}

static GSM_Error GetBitmap(GSM_Data *data, GSM_Statemachine *state)
{
	GSM_Error error;
	int i;

	if (!data->Bitmap) return GE_INTERNALERROR;

	switch (data->Bitmap->type) {
	case GSM_StartupLogo:
		data->Bitmap->height = phone_nokia_6160.Info.StartupLogoH;
		data->Bitmap->width = phone_nokia_6160.Info.StartupLogoW;
		data->Bitmap->size = ceiling_to_octet(data->Bitmap->height * data->Bitmap->width);
		GSM_ClearBitmap(data->Bitmap);
		for (i = 0; i < 6; i++)
			if ((error = GetStartupSlice(data, state, i)) != GE_NONE)
				return error;
		break;
	case GSM_WelcomeNoteText:
	case GSM_DealerNoteText:
	case GSM_OperatorLogo:
	case GSM_CallerLogo:
	case GSM_None:
	case GSM_PictureMessage:
		return GE_NOTSUPPORTED;

	default:
		return GE_INTERNALERROR;
	}

	return GE_NONE;
}

static GSM_Error IncomingStartupLogo(int messagetype, unsigned char *message, int length, GSM_Data *data, GSM_Statemachine *state)
{
	int x, i;
	unsigned char b;

	if (message[0] != 0x01 || message[1] != 0x00 || message[2] != 0x07 || message[3] != 0x08)
		return GE_UNHANDLEDFRAME;

	if (!data->Bitmap || data->Bitmap->type != GSM_StartupLogo)
		return GE_INTERNALERROR;

	for (x = 0; x < 84; x++)
		for (b = message[5 + x], i = 0; b != 0; b >>= 1, i++)
			if ( b & 0x01 )
				GSM_SetPointBitmap(data->Bitmap, x, 8 * logoslice + i);

	return GE_NONE;
}


#if 0
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
#endif
