/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.1  2001-07-27 00:02:21  pkot
  Generic AT support for the new structure (Manfred Jonsson)


*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "phones/generic.h"
#include "links/atbus.h"


static GSM_Error Initialise(GSM_Statemachine *state);
static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);
static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data);


#define REPLY_SIMPLETEXT(l1, l2, c, t) \
	if ((0 == strcmp(l1, c)) && (NULL != t)) strcpy(t, l2)


/* Mobile phone information */

static GSM_IncomingFunctionType IncomingFunctions[] = {
	{ 1, Reply },
	{ 0, NULL }
};
 

GSM_Phone phone_at = {
	IncomingFunctions,
	PGEN_IncomingDefault,
	{
		"AT",			/* Supported models */
		99,			/* Max RF Level */
		0,			/* Min RF Level */
		GRF_CSQ,		/* RF level units */
		100,			/* Max Battery Level */
		0,			/* Min Battery Level */
		GBU_Percentage,		/* Battery level units */
		0,			/* Have date/time support */
		0,			/* Alarm supports time only */
		0,			/* Alarms available - FIXME */
		0, 0,			/* Startup logo size - FIXME */
		0, 0,			/* Op logo size */
		0, 0			/* Caller logo size */
	},
	Functions
};


/* LinkOK is always true for now... */
bool ATGEN_LinkOK = true;


static GSM_Error SetEcho(GSM_Data *data, GSM_Statemachine *state)
{
        char req[128];

        sprintf(req, "ATE1\r\n");
	if (SM_SendMessage(state, 6, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error AT_Identify(GSM_Data *data, GSM_Statemachine *state)
{
        char req[128];
	GSM_Error ret;

        sprintf(req, "AT+CGMM\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	if (ret != GE_NONE) return ret;
        sprintf(req, "AT+CGSN\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	if (ret != GE_NONE) return ret;
        sprintf(req, "AT+CGMI\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	if (ret != GE_NONE) return ret;
        sprintf(req, "AT+CGMR\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	ret = SM_Block(state, data, 1);
	return ret;
}


static GSM_Error AT_GetModel(GSM_Data *data, GSM_Statemachine *state)
{
        char req[128];

        sprintf(req, "AT+CGMM\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error AT_GetIMEI(GSM_Data *data, GSM_Statemachine *state)
{
        char req[128];

        sprintf(req, "AT+CGSN\r\n");
	if (SM_SendMessage(state, 9, 1, req) != GE_NONE) return GE_NOTREADY;
	return SM_Block(state, data, 1);
}


static GSM_Error Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
        switch (op) {
        case GOP_Init:
                return Initialise(state);
                break;
        case GOP_Identify:
                return AT_Identify(data, state);
                break;
        case GOP_GetImei:
                return AT_GetIMEI(data, state);
                break;
        case GOP_GetModel:
                return AT_GetModel(data, state);
                break;
        default:
                return GE_NOTIMPLEMENTED;
                break;
        }
}


static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	char *line2, *line3;

	line2 = strchr(buffer, '\r');
	if (line2) {
		*line2 = 0;
		line2++;
		line2++;
		line2++;
	} else {
		line2 = buffer;
	}
	line3 = strchr(line2, '\r');
	if (line3) {
		*line3 = 0;
		line3++;
		line3++;
		line3++;
	} else {
		line3 = line2;
	}
	REPLY_SIMPLETEXT(buffer, line2, "AT+CGSN", data->Imei);
	REPLY_SIMPLETEXT(buffer, line2, "AT+CGMM", data->Model);
	REPLY_SIMPLETEXT(buffer, line2, "AT+CGMI", data->Manufacturer);
	REPLY_SIMPLETEXT(buffer, line2, "AT+CGMR", data->Revision);
	return GE_NONE;
}


static GSM_Error Initialise(GSM_Statemachine *state)
{
	GSM_Data data;
	GSM_Error ret;
	char model[10];

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_at, sizeof(GSM_Phone));

	fprintf(stderr, "Initializing AT capable mobile phone ...\n");
	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		ATBUS_Initialise(state);
		break;
	default:
		return GE_NOTSUPPORTED;
		break;
	}
	SM_Initialise(state);

	SetEcho(&data, state);

	GSM_DataClear(&data);
	data.Model = model;

  	ret = state->Phone.Functions(GOP_GetModel, &data, state);
  	return ret;
}

