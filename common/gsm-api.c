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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001      Chris Kemp, Manfred Jonsson
  Copyright (C) 2001-2002 Pavel Machek
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002-2003 Ladis Michl
  Copyright (C) 2002-2004 BORBELY Zoltan

  Provides a generic API for accessing functions on the phone, wherever
  possible hiding the model specific details.

  The underlying code should run in it's own thread to allow communications to
  the phone to be run independantly of mailing code that calls these API
  functions.

  Unless otherwise noted, all functions herein block until they complete.  The
  functions themselves are defined in a structure in gsm-common.h.

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "cfgreader.h"
#include "phones/nk6510.h"
#include "phones/nk7110.h"
#include "phones/nk6100.h"
#include "phones/nk3110.h"
#include "phones/nk2110.h"


#if defined(WIN32) && defined(_USRDLL)
/* 
 * Define the entry point for the DLL application.
 *	
 * We don't do anything special here (yet) but the code is needed
 * in order to create a DLL.
 */
BOOL APIENTRY DllMain(HANDLE hModule, 
		      DWORD  ul_reason_for_call, 
		      LPVOID lpReserved)
{
	/* 
	 * For now this is enough to satisfy the compiler and linker.
	 * Currently no extra code is needed for initializing or
	 * deinitializing of the DLL.
	 */
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif	/* defined(WIN32) && defined(_USRDLL) */

struct gn_statemachine gn_sm;
gn_error (*gn_gsm_f)(gn_operation op, gn_data *data, struct gn_statemachine *state);


/* Define pointer to the gn_phone structure used by external code to
   obtain information that varies from model to model. This structure is also
   defined in gsm-common.h */
GNOKII_API gn_phone *gn_gsm_info;

/* Initialise interface to the phone. Model number should be a string such as
   3810, 5110, 6110 etc. Device is the serial port to use e.g. /dev/ttyS0, the
   user must have write permission to the device. */
static gn_error register_driver(gn_driver *driver, const char *model, char *setupmodel, struct gn_statemachine *sm)
{
	gn_data *data = NULL;
	gn_data *p_data;
	gn_error error = GN_ERR_UNKNOWNMODEL;

	if (setupmodel) {
		data = calloc(1, sizeof(gn_data));
		if (!data)
			return GN_ERR_INTERNALERROR;
		data->model = setupmodel;
		p_data = data;
	} else {
		p_data = NULL;
	}
	if (strstr(driver->phone.models, model) != NULL)
		error = driver->functions(GN_OP_Init, p_data, sm);

	if (data)
		free(data);
	return error;
}

#define REGISTER_DRIVER(x, y) { \
	extern gn_driver driver_##x; \
	if ((ret = register_driver(&driver_##x, sm->config.model, y, sm)) != GN_ERR_UNKNOWNMODEL) \
		return ret; \
}

GNOKII_API gn_error gn_gsm_initialise(struct gn_statemachine *sm)
{
	gn_error ret;

	dprintf("phone instance config:\n");
	dprintf("model = %s\n", sm->config.model);
	dprintf("port = %s\n", sm->config.port_device);
	dprintf("connection = %s\n", gn_lib_get_connection_name(sm->config.connection_type));
	if (sm->config.init_length) {
		dprintf("initlength = %d\n", sm->config.init_length);
	} else {
		dprintf("initlength = default\n");
	}
	dprintf("serial_baudrate = %d\n", sm->config.serial_baudrate);
	dprintf("serial_write_usleep = %d\n", sm->config.serial_write_usleep);
	dprintf("handshake = %s\n", sm->config.hardware_handshake ? "hardware" : "software");
	dprintf("require_dcd = %d\n", sm->config.require_dcd);
	dprintf("smsc_timeout = %d\n", sm->config.smsc_timeout / 10);
	if (*sm->config.connect_script)
		dprintf("connect_script = %s\n", sm->config.connect_script);
	if (*sm->config.disconnect_script)
		dprintf("disconnect_script = %s\n", sm->config.disconnect_script);
	dprintf("rfcomm_channel = %d\n", (int)sm->config.rfcomm_cn);
	dprintf("sm_retry = %d\n", sm->config.sm_retry);

	if (sm->config.model[0] == '\0') return GN_ERR_UNKNOWNMODEL;
	if (sm->config.port_device[0] == '\0') return GN_ERR_FAILED;

	REGISTER_DRIVER(nokia_7110, NULL);
	REGISTER_DRIVER(nokia_6510, NULL);
	REGISTER_DRIVER(nokia_6100, NULL);
	REGISTER_DRIVER(nokia_3110, NULL);
#if 0
#ifndef WIN32
	REGISTER_DRIVER(nokia_2110, NULL);
#endif
#endif
	REGISTER_DRIVER(fake, NULL);
	REGISTER_DRIVER(at, sm->config.model);
	REGISTER_DRIVER(nokia_6160, NULL);
	REGISTER_DRIVER(gnapplet, NULL);
#ifdef HAVE_PCSC
	REGISTER_DRIVER(pcsc, sm->config.model);
#endif

	return GN_ERR_UNKNOWNMODEL;
}
