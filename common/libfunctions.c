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

#define GNOKII_DEPRECATED /* do not warn about deprecated functions here */

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "device.h"

/* this macro sets the "lasterror" code */
#define LASTERROR(state,nr)	((state->lasterror = nr)) /* do not delete the double brackets! */

API unsigned int gn_lib_version()
{
	/* return the library version number at compile time of libgnokii */
	return LIBGNOKII_VERSION;
}

API gn_error gn_lib_phoneprofile_load( const char *configname, struct gn_statemachine **state )
{
	gn_error error;
	*state = NULL;

	if (!gn_cfg_info) {
		error = gn_cfg_read_default();
		if (GN_ERR_NONE != error)
			return error;
	}

	/* allocate and initialize data structures */
	*state = malloc(sizeof(**state));
	if (!*state)
		return GN_ERR_MEMORYFULL;
	memset(*state, 0, sizeof(**state));

	/* Load the phone configuration */
	if (!gn_cfg_phone_load(configname, *state)) {
		free(*state);
		*state = NULL;
		return GN_ERR_UNKNOWNMODEL;
	}

	return LASTERROR((*state), GN_ERR_NONE);
}

API gn_error gn_lib_phoneprofile_free( struct gn_statemachine **state )
{
	/* free data structure */
	free(*state);
	*state = NULL;

	return GN_ERR_NONE;
}

API void gn_lib_library_free( void )
{
	if (gn_cfg_info) {
		gn_cfg_free_default();
	}
}

/* return last error code */
API gn_error gn_lib_lasterror( struct gn_statemachine *state )
{
	if (state)
		return state->lasterror;
	else
		return GN_ERR_INTERNALERROR;
}


API gn_error gn_lib_phone_open( struct gn_statemachine *state )
{
	const char *aux;
	gn_error error;

	/* should the device be locked with a lockfile ? */
	aux = gn_lib_cfg_get("global", "use_locking");
	if (aux && !strcmp(aux, "yes")) {
		state->lockfile = gn_device_lock(state->config.port_device);
		if (state->lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting.\n"));
			return LASTERROR(state, GN_ERR_BUSY);
		}
	}

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Telephone interface init failed: %s\nQuitting.\n"),
			gn_error_print(error));
		return LASTERROR(state, error);
	}

	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_phone_close( struct gn_statemachine *state )
{
	/* close phone connection */
	gn_sm_functions(GN_OP_Terminate, NULL, state);

	/* remove lockfile if it was created */
	if (state->lockfile) {
		/* gn_device_unlock frees state->lockfile */
		gn_device_unlock(state->lockfile);
	}
	state->lockfile = NULL;

	return LASTERROR(state, GN_ERR_NONE);
}

static gn_error gn_lib_get_phone_information( struct gn_statemachine *state )
{
	gn_data *data = &state->sm_data;
	const char *unknown = _("Unknown");
	gn_error error;

	if (state->config.m_model[0])
		return LASTERROR(state, GN_ERR_NONE);

	// identify phone
	gn_data_clear(data);
	data->model        = state->config.m_model;
	data->manufacturer = state->config.m_manufacturer;
	data->revision     = state->config.m_revision;
	data->imei         = state->config.m_imei;

	error = gn_sm_functions(GN_OP_Identify, data, state);

	if (!data->model[0])
		strcpy(data->model,        unknown);
	if (!data->manufacturer[0])
		strcpy(data->manufacturer, unknown);
	if (!data->revision[0])
		strcpy(data->revision,     unknown);
	if (!data->imei[0])
		strcpy(data->imei,         unknown);

	return LASTERROR(state, error);
}

/* ask phone for static information (model, version, manufacturer, revision and imei) */
API const char *gn_lib_get_phone_model( struct gn_statemachine *state )
{
	const char *aux;

	gn_lib_get_phone_information(state);
	aux = gn_model_get(state->config.m_model); /* e.g. 6310 */
	if (aux)
		return aux;
	else
		return state->config.m_model;
}

API const char *gn_lib_get_phone_product_name( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_model; /* e.g. NPE-4 */
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

API const char *gn_lib_cfg_get(const char *section, const char *key)
{
	if (!gn_cfg_info)
		gn_cfg_read_default();
	return gn_cfg_get(gn_cfg_info, section, key);
}



/* Phone addressbook functions */

API gn_error gn_lib_addressbook_memstat( struct gn_statemachine *state,
	const gn_memory_type memory_type, int *num_used, int *num_free )
{
	gn_error error;
	gn_memory_status memstat;
	gn_data *data = &state->sm_data;

	gn_data_clear(data);
	memset(&memstat, 0, sizeof(memstat));
        memstat.memory_type = memory_type;
        data->memory_status = &memstat;

	error = gn_sm_functions(GN_OP_GetMemoryStatus, data, state);
	if (error == GN_ERR_NONE) {
		if (num_used) *num_used = memstat.used;
		if (num_free) *num_free = memstat.free;
	}

	return LASTERROR(state, error);
}

API gn_error gn_lib_phonebook_read_entry( struct gn_statemachine *state,
	const gn_memory_type memory_type, const int index )
{
	gn_error error;
	gn_data *data = &state->sm_data;

	state->u.pb_entry.memory_type = memory_type;
	state->u.pb_entry.location = index;
	data->phonebook_entry = &state->u.pb_entry;
	error = gn_sm_functions(GN_OP_ReadPhonebook, data, state);
	return LASTERROR(state, error);
}

API int gn_lib_phonebook_entry_isempty( struct gn_statemachine *state,
	const gn_memory_type memory_type, const int index )
{
	gn_error error;

	error = gn_lib_phonebook_read_entry(state, memory_type, index);
	if (error == GN_ERR_EMPTYLOCATION)
		return true;
	if (error == GN_ERR_NONE && state->u.pb_entry.empty)
		return true;
	return false;
}

/* after reading an entry with gn_lib_phonebook_read_entry() ask for the values of the phonebook entry */
API const char *gn_lib_get_pb_name( struct gn_statemachine *state )
{
	return state->u.pb_entry.name;
}

API const char *gn_lib_get_pb_number( struct gn_statemachine *state )
{
	return state->u.pb_entry.number;
}

API gn_phonebook_group_type gn_lib_get_pb_caller_group( struct gn_statemachine *state )
{
	return state->u.pb_entry.caller_group;
}

API gn_memory_type gn_lib_get_pb_memtype( struct gn_statemachine *state )
{
	return state->u.pb_entry.memory_type;
}

API int gn_lib_get_pb_location( struct gn_statemachine *state )
{
	return state->u.pb_entry.location;
}

API gn_timestamp gn_lib_get_pb_date( struct gn_statemachine *state )
{
	return state->u.pb_entry.date;
}

API int gn_lib_get_pb_num_subentries( struct gn_statemachine *state )
{
	return state->u.pb_entry.subentries_count;
}

API gn_error gn_lib_get_pb_subentry( struct gn_statemachine *state, const int index,
	gn_phonebook_entry_type *entry_type, gn_phonebook_number_type *number_type, 
	const char **number )
{
	if (entry_type)	 *entry_type  = state->u.pb_entry.subentries[index].entry_type;
	if (number_type) *number_type = state->u.pb_entry.subentries[index].number_type;
	if (number)	 *number      = state->u.pb_entry.subentries[index].data.number;
	return LASTERROR(state, GN_ERR_NONE);
}


API gn_error gn_lib_phonebook_entry_delete( struct gn_statemachine *state,
	const gn_memory_type memory_type, const int index )
{
	gn_error error;
	gn_data *data = &state->sm_data;

	state->u.pb_entry.memory_type = memory_type;
	state->u.pb_entry.location = index;
	data->phonebook_entry = &state->u.pb_entry;
	memset(data->phonebook_entry, 0, sizeof(*data->phonebook_entry));
	data->phonebook_entry->empty = 1;
	error = gn_sm_functions(GN_OP_WritePhonebook, data, state);
	return LASTERROR(state, error);
}

API gn_error gn_lib_phonebook_prepare_write_entry( struct gn_statemachine *state )
{
	gn_data *data = &state->sm_data;
	gn_data_clear(data);
	memset(&state->u, 0, sizeof(state->u));
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_phonebook_write_entry( struct gn_statemachine *state,
	const gn_memory_type memory_type, const int index )
{
	gn_error error;
	gn_data *data = &state->sm_data;

	state->u.pb_entry.memory_type = memory_type;
	state->u.pb_entry.location = index;
	data->phonebook_entry = &state->u.pb_entry;
	error = gn_sm_functions(GN_OP_WritePhonebook, data, state);
	return LASTERROR(state, error);
}

API gn_error gn_lib_set_pb_name( struct gn_statemachine *state, const char *name )
{
	strncpy(state->u.pb_entry.name, name, sizeof(state->u.pb_entry.name)-1);
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_set_pb_number( struct gn_statemachine *state, const char *number )
{
	strncpy(state->u.pb_entry.number, number, sizeof(state->u.pb_entry.number)-1);
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_set_pb_caller_group( struct gn_statemachine *state, gn_phonebook_group_type grouptype )
{
	state->u.pb_entry.caller_group = grouptype;
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_set_pb_memtype( struct gn_statemachine *state, gn_memory_type memtype )
{
	state->u.pb_entry.memory_type = memtype;
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_set_pb_location( struct gn_statemachine *state, int location )
{
	state->u.pb_entry.location = location;
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_set_pb_date( struct gn_statemachine *state, gn_timestamp timestamp )
{
	state->u.pb_entry.date = timestamp;
	return LASTERROR(state, GN_ERR_NONE);
}

API gn_error gn_lib_set_pb_subentry( struct gn_statemachine *state, const int index, /* index=-1 appends it */
        gn_phonebook_entry_type entry_type, gn_phonebook_number_type number_type, const char *number )
{
	int i = (index==-1) ? gn_lib_get_pb_num_subentries(state) : index;
	if (i<0 || i>=GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER)
		return LASTERROR(state, GN_ERR_INVALIDLOCATION);

	if (index == -1)
		++state->u.pb_entry.subentries_count;

	state->u.pb_entry.subentries[i].entry_type  = entry_type;
	state->u.pb_entry.subentries[i].number_type = number_type;
	strncpy(state->u.pb_entry.subentries[i].data.number, number, sizeof(state->u.pb_entry.subentries[i].data.number)-1);
	return LASTERROR(state, GN_ERR_NONE);
}


/* helper functions */
