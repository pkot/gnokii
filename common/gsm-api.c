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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Provides a generic API for accessing functions on the phone, wherever
  possible hiding the model specific details.

  The underlying code should run in it's own thread to allow communications to
  the phone to be run independantly of mailing code that calls these API
  functions.

  Unless otherwise noted, all functions herein block until they complete.  The
  functions themselves are defined in a structure in gsm-common.h.

*/

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "gsm-common.h"
#include "data/rlp-common.h"
#include "gsm-statemachine.h"
#include "phones/nk6510.h"
#include "phones/nk7110.h"
#include "phones/nk6100.h"
#include "phones/nk3110.h"
#include "phones/nk2110.h"

GSM_Statemachine GSM_SM;
GSM_Error (*GSM_F)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);


/* Define pointer to the GSM_Information structure used by external code to
   obtain information that varies from model to model. This structure is also
   defined in gsm-common.h */

GSM_Information		*GSM_Info;

/* Initialise interface to the phone. Model number should be a string such as
   3810, 5110, 6110 etc. Device is the serial port to use e.g. /dev/ttyS0, the
   user must have write permission to the device. */

static GSM_Error register_phone(GSM_Phone *phone, char *model, char *setupmodel, GSM_Statemachine *sm)
{
	GSM_Data data;
	GSM_Data *p_data;
	if (setupmodel) {
		GSM_DataClear(&data);
		data.Model = setupmodel;
		p_data = &data;
	} else {
		p_data = NULL;
	}
	if (strstr(phone->Info.Models, model) != NULL)
		return phone->Functions(GOP_Init, p_data, sm);
	return GE_UNKNOWNMODEL;
}

#define REGISTER_PHONE(x, y) { \
	extern GSM_Phone phone_##x; \
	if ((ret = register_phone(&phone_##x, model, y, sm)) != GE_UNKNOWNMODEL) \
		return ret; \
}

GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, void (*rlp_callback)(RLP_F96Frame *frame), GSM_Statemachine *sm)
{
	GSM_Error ret;

	sm->Link.ConnectionType = connection;
	sm->Link.InitLength = atoi(initlength);
	memset(&sm->Link.PortDevice, 0, sizeof(sm->Link.PortDevice));
	strncpy(sm->Link.PortDevice, device, sizeof(sm->Link.PortDevice) - 1);

	REGISTER_PHONE(nokia_7110, NULL);
	REGISTER_PHONE(nokia_6510, NULL);
	REGISTER_PHONE(nokia_6100, NULL);
	REGISTER_PHONE(nokia_3110, NULL);
#ifndef WIN32
	REGISTER_PHONE(nokia_2110, NULL);
#endif
	REGISTER_PHONE(dancall_2711, NULL);
	REGISTER_PHONE(fake, NULL);
	REGISTER_PHONE(at, model);

	return (GE_UNKNOWNMODEL);
}
