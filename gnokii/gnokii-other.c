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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 1999-2000  Gary Reuter, Reinhold Jordan
  Copyright (C) 1999-2006  Pawel Kot
  Copyright (C) 2000-2002  Marcin Wiacek, Chris Kemp, Manfred Jonsson
  Copyright (C) 2001       Marian Jancar, Bartek Klepacz
  Copyright (C) 2001-2002  Pavel Machek, Markus Plail
  Copyright (C) 2002       Ladis Michl, Simon Huggins
  Copyright (C) 2002-2004  BORBELY Zoltan
  Copyright (C) 2003       Bertrik Sikken
  Copyright (C) 2004       Martin Goldhahn

  Mainline code for gnokii utility. Other phone handling functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>

#ifdef WIN32
#  include <io.h>
#endif

#include "gnokii-app.h"
#include "gnokii.h"

void other_usage(FILE *f)
{
	fprintf(f, _("Misc options:\n"
		     "          --keysequence\n"
		     "          --enterchar\n"
		     "          --listnetworks\n"
		     "          --getnetworkinfo\n"
		));
}

/* pmon allows fbus code to run in a passive state - it doesn't worry about
   whether comms are established with the phone.  A debugging/development
   tool. */
gn_error pmon(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(state);

	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model?). Quitting.\n"));
		return error;
	}

	while (1) {
		usleep(50000);
	}

	return GN_ERR_NONE;
}

static gn_error presskey(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	error = gn_sm_functions(GN_OP_PressPhoneKey, data, state);
	if (error == GN_ERR_NONE)
		error = gn_sm_functions(GN_OP_ReleasePhoneKey, data, state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Failed to press key: %s\n"), gn_error_print(error));
	return error;
}

gn_error presskeysequence(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	unsigned char *syms = "0123456789#*PGR+-UDMN";
	gn_key_code keys[] = {GN_KEY_0, GN_KEY_1, GN_KEY_2, GN_KEY_3,
			      GN_KEY_4, GN_KEY_5, GN_KEY_6, GN_KEY_7,
			      GN_KEY_8, GN_KEY_9, GN_KEY_HASH,
			      GN_KEY_ASTERISK, GN_KEY_POWER, GN_KEY_GREEN,
			      GN_KEY_RED, GN_KEY_INCREASEVOLUME,
			      GN_KEY_DECREASEVOLUME, GN_KEY_UP, GN_KEY_DOWN,
			      GN_KEY_MENU, GN_KEY_NAMES};
	unsigned char ch, *pos;

	gn_data_clear(data);
	console_raw();

	while (read(0, &ch, 1) > 0) {
		if ((pos = strchr(syms, toupper(ch))) != NULL)
			data->key_code = keys[pos - syms];
		else
			continue;
		error = presskey(data, state);
	}

	return error;
}

gn_error enterchar(gn_data *data, struct gn_statemachine *state)
{
	unsigned char ch;
	gn_error error = GN_ERR_NONE;

	gn_data_clear(data);
	console_raw();

	while ((error = GN_ERR_NONE) && (read(0, &ch, 1) > 0)) {
		switch (ch) {
		case '\r':
			break;
		case '\n':
			data->key_code = GN_KEY_MENU;
			presskey(data, state);
			break;
#ifdef WIN32
		case '\033':
#else
		case '\e':
#endif
			data->key_code = GN_KEY_NAMES;
			presskey(data, state);
			break;
		default:
			data->character = ch;
			error = gn_sm_functions(GN_OP_EnterChar, data, state);
			if (error != GN_ERR_NONE)
				fprintf(stderr, _("Error entering char: %s\n"), gn_error_print(error));
			break;
		}
	}

	return error;
}

void list_gsm_networks(void)
{

	gn_network network;
	int i = 0;

	printf(_("Network  Name\n"));
	printf(_("-----------------------------------------\n"));
	while (gn_network_get(&network, i++))
		printf(_("%-7s  %s (%s)\n"), network.code, network.name, gn_country_name_get(network.code));
}

gn_error getnetworkinfo(gn_data *data, struct gn_statemachine *state)
{
	gn_network_info networkinfo;
	gn_error error;
	int lac, cid;
	char country[4] = {0, 0, 0, 0};

	gn_data_clear(data);
	memset(&networkinfo, 0, sizeof(networkinfo));

	data->network_info = &networkinfo;
	data->reg_notification = NULL;
	data->callback_data = NULL;

	if ((error = gn_sm_functions(GN_OP_GetNetworkInfo, data, state)) != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return error;
	}

	/* Ugly, ugly, ... */
        if (networkinfo.cell_id[2] == 0 && networkinfo.cell_id[3] == 0)  
        	cid = (networkinfo.cell_id[0] << 8) + networkinfo.cell_id[1];
	else  
		cid = (networkinfo.cell_id[0] << 24) + (networkinfo.cell_id[1] << 16) + (networkinfo.cell_id[2] << 8) + networkinfo.cell_id[3];
	lac = (networkinfo.LAC[0] << 8) + networkinfo.LAC[1];
	memcpy(country, networkinfo.network_code, 3);

	fprintf(stdout, _("Network      : %s (%s)\n"),
			gn_network_name_get((char *)networkinfo.network_code),
			gn_country_name_get((char *)country));
	fprintf(stdout, _("Network code : %s\n"), (*networkinfo.network_code ? networkinfo.network_code : _("undefined")));
	fprintf(stdout, _("LAC          : %04x (%d)\n"), lac, lac);
	fprintf(stdout, _("Cell id      : %08x (%d)\n"), cid, cid);

	return 0;
}

