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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001      Chris Kemp
  Copyrught (C) 2001-2004 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan
  Copyright (C) 2002      Pavel Machek, Marcin Wiacek
  Copyright (C) 2006      Helge Deller

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "device.h"

API unsigned int gn_lib_version()
{
	/* return the library version number at compile time of libgnokii */
	return LIBGNOKII_VERSION;
}

API gn_error gn_lib_phoneprofile_load( const char *configname, struct gn_statemachine **state )
{
	/* allocate and initialize data structures */
	*state = malloc(sizeof(*state));
	memset(*state, 0, sizeof(*state));

	/* Read config file */
        if (gn_cfg_read_default() < 0)
                return GN_ERR_INTERNALERROR;

	/* Load the phone configuration */
	if (!gn_cfg_phone_load(configname, *state))
                return GN_ERR_UNKNOWNMODEL;

	return GN_ERR_NONE;
}

API gn_error gn_lib_phoneprofile_free( struct gn_statemachine **state )
{
	/* free data structure */
	free(*state);
	*state = NULL;

	return GN_ERR_NONE;
}

API gn_error gn_lib_phone_open( struct gn_statemachine *state, gn_data **data )
{
	char *aux;
	gn_error error;

	/* allocate and initialize data structures */
	*data = malloc(sizeof(*data));
	gn_data_clear(*data);

	/* should the device be locked with a lockfile ? */
	aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
	if (aux && !strcmp(aux, "yes")) {
		state->lockfile = gn_device_lock(state->config.port_device);
		if (state->lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting.\n"));
			free(*data);
			*data = NULL;
			return GN_ERR_BUSY;
		}
	}

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Telephone interface init failed: %s\nQuitting.\n"),
			gn_error_print(error));
		free(*data);
		*data = NULL;
		return error;
	}

	return GN_ERR_NONE;
}

API gn_error gn_lib_phone_close( struct gn_statemachine *state,  gn_data **data )
{
	/* close phone connection */
	gn_sm_functions(GN_OP_Terminate, NULL, state);

	/* remove lockfile if it was created */
	if (state->lockfile) {
		gn_device_unlock(state->lockfile);
		free(state->lockfile);
	}
	state->lockfile = NULL;

	/* free data structure */
	free(*data);
	*data = NULL;

	return GN_ERR_NONE;
}

