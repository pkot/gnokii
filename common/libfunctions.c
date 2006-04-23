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

  Goal of this code is to provide a binary compatible layer to access
  libgnokii.so functions from external programs which are not part
  of this gnokii CVS package.
  One of those external programs is e.g. the Mobile Phone import/export
  filter of the KDE Adressbook (kaddressbook).
  Helge Deller - April 2006

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
	*state = malloc(sizeof(**state));
	if (!*state)
		return GN_ERR_MEMORYFULL;
	memset(*state, 0, sizeof(**state));

	/* Read config file */
	if (gn_cfg_read_default() < 0) {
		free(*state);
		*state = NULL;
		return GN_ERR_INTERNALERROR;
	}

	/* Load the phone configuration */
	if (!gn_cfg_phone_load(configname, *state)) {
		free(*state);
		*state = NULL;
		return GN_ERR_UNKNOWNMODEL;
	}

	return GN_ERR_NONE;
}

API gn_error gn_lib_phoneprofile_free( struct gn_statemachine **state )
{
	/* free data structure */
	free(*state);
	*state = NULL;

	return GN_ERR_NONE;
}

API gn_error gn_lib_phone_open( struct gn_statemachine *state )
{
	char *aux;
	gn_error error;

	/* should the device be locked with a lockfile ? */
	aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
	if (aux && !strcmp(aux, "yes")) {
		state->lockfile = gn_device_lock(state->config.port_device);
		if (state->lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting.\n"));
			return GN_ERR_BUSY;
		}
	}

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Telephone interface init failed: %s\nQuitting.\n"),
			gn_error_print(error));
		return error;
	}

	return GN_ERR_NONE;
}

API gn_error gn_lib_phone_close( struct gn_statemachine *state )
{
	/* close phone connection */
	gn_sm_functions(GN_OP_Terminate, NULL, state);

	/* remove lockfile if it was created */
	if (state->lockfile) {
		gn_device_unlock(state->lockfile);
		free(state->lockfile);
	}
	state->lockfile = NULL;

	return GN_ERR_NONE;
}

static gn_error gn_lib_get_phone_information( struct gn_statemachine *state )
{
	gn_data *data = &state->sm_data;
	const char *unknown = _("Unknown");

	if (state->config.m_model[0])
		return GN_ERR_NONE;

	// identify phone
	gn_data_clear(data);
	data->model        = state->config.m_model;
	data->manufacturer = state->config.m_manufacturer;
	data->revision     = state->config.m_revision;
	data->imei         = state->config.m_imei;

	strcpy(data->model,        unknown);
	strcpy(data->manufacturer, unknown);
	strcpy(data->revision,     unknown);
	strcpy(data->imei,         unknown);

	return gn_sm_functions(GN_OP_Identify, data, state);
}

/* ask phone for static information (model, manufacturer, revision and imei) */
API const char *gn_lib_get_phone_model( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_model;
}

API const char *gn_lib_get_phone_manufacturer( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_manufacturer;
}

API const char *gn_lib_get_phone_revision( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_revision;
}

API const char *gn_lib_get_phone_imei( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_imei;
}

