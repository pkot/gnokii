/*

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
  Copyright (C) 2001-2011 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan
  Copyright (C) 2002      Pavel Machek, Marcin Wiacek
  Copyright (C) 2006      Helge Deller

  Goal of this code is to provide a binary compatible layer to access
  libgnokii.so functions from external programs which are not part
  of this gnokii package.
  One of those external programs is e.g. the Mobile Phone import/export
  filter of the KDE Adressbook (kaddressbook).
  Helge Deller - April 2006

*/

#if !defined(GNOKII_DEPRECATED)
# define GNOKII_DEPRECATED /* do not warn about deprecated functions here */
#endif

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "device.h"

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

/* this macro sets the "lasterror" code */
#define LASTERROR(state,nr)	((state->lasterror = nr)) /* do not delete the double brackets! */

/**
 * gn_lib_init:
 *
 * returns: GN_ERR_NONE on success, an error on failure
 *
 * Initializes the library for use by programs.
 * Call this function before any other in this library.
 */
GNOKII_API gn_error gn_lib_init()
{
	static bool initialized = false;

	if (initialized == true)
		return GN_ERR_NONE;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#endif
	initialized = true;

	return GN_ERR_NONE;
}

GNOKII_API unsigned int gn_lib_version()
{
	/* return the library version number at compile time of libgnokii */
	return LIBGNOKII_VERSION;
}

static gn_error load_from_file(const char *configfile, const char *configname)
{
	if (configfile && *configfile)
		return gn_cfg_file_read(configfile);
	else
		return gn_cfg_read_default();
}

static gn_error load_from_memory(char **lines)
{
	return gn_cfg_memory_read(lines);
}

static gn_error phoneprofile_load(const char *configname, gn_error error, struct gn_statemachine **state)
{
	*state = NULL;

	if (error == GN_ERR_NONE) {
		/* allocate and initialize data structures */
		*state = malloc(sizeof(**state));
		if (*state) {
			memset(*state, 0, sizeof(**state));

			/* Load the phone configuration */
			error = gn_cfg_phone_load(configname, *state);
		} else {
			error = GN_ERR_MEMORYFULL;
		}
	}
	if (error != GN_ERR_NONE) {
		gn_lib_phoneprofile_free(state);
		gn_lib_library_free();
		return error;
	}
	return LASTERROR((*state), GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_phoneprofile_load_from_file(const char *configfile, const char *configname, struct gn_statemachine **state)
{
	gn_error error = GN_ERR_NONE;

	if (!gn_cfg_info)
		error = load_from_file(configfile, configname);

	return phoneprofile_load(configname, error, state);
}

GNOKII_API gn_error gn_lib_phoneprofile_load_from_external(char **lines, struct gn_statemachine **state)
{
	gn_error error = GN_ERR_NONE;

	if (!gn_cfg_info)
		error = load_from_memory(lines);

	return phoneprofile_load(NULL, error, state);
}

GNOKII_API gn_error gn_lib_phoneprofile_load(const char *configname, struct gn_statemachine **state)
{
	return gn_lib_phoneprofile_load_from_file(NULL, configname, state);
}

GNOKII_API gn_error gn_lib_phoneprofile_free( struct gn_statemachine **state )
{
	/* free data structure */
	free(*state);
	*state = NULL;

	return GN_ERR_NONE;
}

GNOKII_API void gn_lib_library_free( void )
{
	if (gn_cfg_info) {
		gn_cfg_free_default();
	}
}

/* return last error code */
GNOKII_API gn_error gn_lib_lasterror( struct gn_statemachine *state )
{
	if (state)
		return state->lasterror;
	else
		return GN_ERR_INTERNALERROR;
}


GNOKII_API gn_error gn_lib_phone_open( struct gn_statemachine *state )
{
	gn_error error;

	state->lockfile = NULL;
	/* should the device be locked with a lockfile ? */
	if (state->config.use_locking) {
		state->lockfile = gn_device_lock(state->config.port_device);
		if (state->lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting.\n"));
			return LASTERROR(state, GN_ERR_LOCKED);
		}
	}

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Telephone interface init failed: %s\nQuitting.\n"),
			gn_error_print(error));

		/* gn_device_unlock() removes lockfile if it was created and frees state->lockfile */
		gn_device_unlock(state->lockfile);
		state->lockfile = NULL;

		return LASTERROR(state, error);
	}

	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_phone_close( struct gn_statemachine *state )
{
	/* close phone connection */
	gn_sm_functions(GN_OP_Terminate, NULL, state);

	/* gn_device_unlock() removes lockfile if it was created and frees state->lockfile */
	gn_device_unlock(state->lockfile);
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
		snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", unknown);
	if (!data->manufacturer[0])
		snprintf(data->manufacturer, GN_MANUFACTURER_MAX_LENGTH, "%s", unknown);
	if (!data->revision[0])
		snprintf(data->revision, GN_REVISION_MAX_LENGTH, "%s", unknown);
	if (!data->imei[0])
		snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", unknown);

	return LASTERROR(state, error);
}

/* ask phone for static information (model, version, manufacturer, revision and imei) */
GNOKII_API const char *gn_lib_get_phone_model( struct gn_statemachine *state )
{
	const char *aux;

	gn_lib_get_phone_information(state);
	aux = gn_model_get(state->config.m_model); /* e.g. 6310 */
	if (aux)
		return aux;
	else
		return state->config.m_model;
}

GNOKII_API const char *gn_lib_get_phone_product_name( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_model; /* e.g. NPE-4 */
}

GNOKII_API const char *gn_lib_get_phone_manufacturer( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_manufacturer;
}

GNOKII_API const char *gn_lib_get_phone_revision( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_revision;
}

GNOKII_API const char *gn_lib_get_phone_imei( struct gn_statemachine *state )
{
	gn_lib_get_phone_information(state);
	return state->config.m_imei;
}

GNOKII_API const char *gn_lib_cfg_get(const char *section, const char *key)
{
	if (!gn_cfg_info)
		gn_cfg_read_default();
	return gn_cfg_get(gn_cfg_info, section, key);
}



/* Phone addressbook functions */

GNOKII_API gn_error gn_lib_addressbook_memstat( struct gn_statemachine *state,
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

GNOKII_API gn_error gn_lib_phonebook_read_entry( struct gn_statemachine *state,
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

GNOKII_API int gn_lib_phonebook_entry_isempty( struct gn_statemachine *state,
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
GNOKII_API const char *gn_lib_get_pb_name( struct gn_statemachine *state )
{
	return state->u.pb_entry.name;
}

GNOKII_API const char *gn_lib_get_pb_number( struct gn_statemachine *state )
{
	return state->u.pb_entry.number;
}

GNOKII_API gn_phonebook_group_type gn_lib_get_pb_caller_group( struct gn_statemachine *state )
{
	return state->u.pb_entry.caller_group;
}

GNOKII_API gn_memory_type gn_lib_get_pb_memtype( struct gn_statemachine *state )
{
	return state->u.pb_entry.memory_type;
}

GNOKII_API int gn_lib_get_pb_location( struct gn_statemachine *state )
{
	return state->u.pb_entry.location;
}

GNOKII_API gn_timestamp gn_lib_get_pb_date( struct gn_statemachine *state )
{
	return state->u.pb_entry.date;
}

GNOKII_API int gn_lib_get_pb_num_subentries( struct gn_statemachine *state )
{
	return state->u.pb_entry.subentries_count;
}

GNOKII_API gn_error gn_lib_get_pb_subentry( struct gn_statemachine *state, const int index,
	gn_phonebook_entry_type *entry_type, gn_phonebook_number_type *number_type, 
	const char **number )
{
	if (entry_type)	 *entry_type  = state->u.pb_entry.subentries[index].entry_type;
	if (number_type) *number_type = state->u.pb_entry.subentries[index].number_type;
	if (number)	 *number      = state->u.pb_entry.subentries[index].data.number;
	return LASTERROR(state, GN_ERR_NONE);
}


GNOKII_API gn_error gn_lib_phonebook_entry_delete( struct gn_statemachine *state,
	const gn_memory_type memory_type, const int index )
{
	gn_error error;
	gn_data *data = &state->sm_data;

	data->phonebook_entry = &state->u.pb_entry;
	memset(data->phonebook_entry, 0, sizeof(*data->phonebook_entry));
	data->phonebook_entry->empty = 1;
	data->phonebook_entry->memory_type = memory_type;
	data->phonebook_entry->location = index;
	error = gn_sm_functions(GN_OP_WritePhonebook, data, state);
	return LASTERROR(state, error);
}

GNOKII_API gn_error gn_lib_phonebook_prepare_write_entry( struct gn_statemachine *state )
{
	gn_data *data = &state->sm_data;
	gn_data_clear(data);
	memset(&state->u, 0, sizeof(state->u));
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_phonebook_write_entry( struct gn_statemachine *state,
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

GNOKII_API gn_error gn_lib_set_pb_name(struct gn_statemachine *state, const char *name)
{
	snprintf(state->u.pb_entry.name, sizeof(state->u.pb_entry.name), "%s", name);
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_set_pb_number(struct gn_statemachine *state, const char *number)
{
	snprintf(state->u.pb_entry.number, sizeof(state->u.pb_entry.number), "%s", number);
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_set_pb_caller_group( struct gn_statemachine *state, gn_phonebook_group_type grouptype )
{
	state->u.pb_entry.caller_group = grouptype;
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_set_pb_memtype( struct gn_statemachine *state, gn_memory_type memtype )
{
	state->u.pb_entry.memory_type = memtype;
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_set_pb_location( struct gn_statemachine *state, int location )
{
	state->u.pb_entry.location = location;
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_set_pb_date( struct gn_statemachine *state, gn_timestamp timestamp )
{
	state->u.pb_entry.date = timestamp;
	return LASTERROR(state, GN_ERR_NONE);
}

GNOKII_API gn_error gn_lib_set_pb_subentry(struct gn_statemachine *state, const int index, /* index=-1 appends it */
        gn_phonebook_entry_type entry_type, gn_phonebook_number_type number_type, const char *number)
{
	int i = (index==-1) ? gn_lib_get_pb_num_subentries(state) : index;
	if (i<0 || i>=GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER)
		return LASTERROR(state, GN_ERR_INVALIDLOCATION);

	if (index == -1)
		++state->u.pb_entry.subentries_count;

	state->u.pb_entry.subentries[i].entry_type  = entry_type;
	state->u.pb_entry.subentries[i].number_type = number_type;
	snprintf(state->u.pb_entry.subentries[i].data.number, sizeof(state->u.pb_entry.subentries[i].data.number), "%s", number);
	return LASTERROR(state, GN_ERR_NONE);
}


/* helper functions */

GNOKII_API void gn_timestamp_set(gn_timestamp *dt, int year, int month, int day,
			int hour, int minute, int second, int timezone)
{
	dt->year = year;
	dt->month = month;
	dt->day = day;
	dt->hour = hour;
	dt->minute = minute;
	dt->second = second;
	dt->timezone = timezone;
}

GNOKII_API void gn_timestamp_get(gn_timestamp *dt, int *year, int *month, int *day,
			int *hour, int *minute, int *second, int *timezone)
{
	if (year) *year = dt->year;
	if (month) *month = dt->month;
	if (day) *day = dt->day;
	if (hour) *hour = dt->hour;
	if (minute) *minute = dt->minute;
	if (second) *second = dt->second;
	if (timezone) *timezone = dt->timezone;
}

static struct { gn_connection_type ct; const char *str; } connectiontypes[] = {
	{ GN_CT_Serial,     "serial" },
	{ GN_CT_DAU9P,      "dau9p" },
	{ GN_CT_DLR3P,      "dlr3p" },
	{ GN_CT_M2BUS,      "m2bus" },
#ifdef HAVE_IRDA
	{ GN_CT_Infrared,   "infrared" },
	{ GN_CT_Irda,       "irda" },
#endif
#ifdef HAVE_BLUETOOTH
	{ GN_CT_Bluetooth,  "bluetooth" },
#endif
	{ GN_CT_DLR3P,      "dku5" },
	{ GN_CT_DKU2,       "dku2" },
#ifdef HAVE_LIBUSB
	{ GN_CT_DKU2LIBUSB, "dku2libusb" },
#endif
#ifdef HAVE_PCSC
	{ GN_CT_PCSC,       "libpcsclite" },
	{ GN_CT_PCSC,       "pcsc" },
#endif
#ifndef WIN32
	{ GN_CT_TCP,        "tcp" },
#endif
	{ GN_CT_Tekram,     "tekram" },
#ifdef HAVE_SOCKETPHONET
	{ GN_CT_SOCKETPHONET, "phonet" },
#endif
};

GNOKII_API gn_connection_type gn_get_connectiontype(const char *connection_type_string)
{ 
	int i;
	for (i = 0; i < sizeof(connectiontypes)/sizeof(connectiontypes[0]); i++)
		if (!strcasecmp(connection_type_string, connectiontypes[i].str))
			return connectiontypes[i].ct;
	return GN_CT_NONE;
}

GNOKII_API const char *gn_lib_get_supported_connection(const int num)
{
	if (num < 0 || num >= sizeof(connectiontypes)/sizeof(connectiontypes[0]))
		return NULL;
	return connectiontypes[num].str;
}

GNOKII_API const char *gn_lib_get_connection_name(gn_connection_type ct)
{
	int i;
	for (i = 0; i < sizeof(connectiontypes)/sizeof(connectiontypes[0]); i++)
		if (ct == connectiontypes[i].ct)
			return connectiontypes[i].str;
	return NULL;
}

GNOKII_API int gn_lib_is_connectiontype_supported(gn_connection_type ct)
{
	switch (ct) {
	case GN_CT_Serial:
	case GN_CT_DAU9P:
	case GN_CT_DLR3P:
	case GN_CT_M2BUS:
	case GN_CT_Tekram:
		return 1;
	case GN_CT_Infrared:
	case GN_CT_Irda:
#ifdef HAVE_IRDA
		return 1;
#else
		return 0;
#endif
	case GN_CT_Bluetooth:
#ifdef HAVE_BLUETOOTH
		return 1;
#else
		return 0;
#endif
	case GN_CT_DKU2: /* FIXME: How to detect this? */
		return 1;
	case GN_CT_DKU2LIBUSB:
#ifdef HAVE_LIBUSB
		return 1;
#else
		return 0;
#endif
	case GN_CT_PCSC:
#ifdef HAVE_PCSC
		return 1;
#else
		return 0;
#endif
	case GN_CT_TCP:
#ifndef WIN32
		return 0;
#else
		return 1;
#endif
	case GN_CT_SOCKETPHONET:
#ifndef HAVE_SOCKETPHONET
		return 0;
#else
		return 1;
#endif
	default:
		return 0;
	}
}

GNOKII_API gn_error gn_lib_search_one_connected_phone(struct gn_statemachine **state)
{
	/* allocate and initialize data structures */
	*state = malloc(sizeof(**state));
	if (!*state)
		return GN_ERR_MEMORYFULL;
	memset(*state, 0, sizeof(**state));

	/* not yet implemented */
	free(*state);
	*state = NULL;
	return GN_ERR_NOTIMPLEMENTED;
}

