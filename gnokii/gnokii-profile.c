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

  Mainline code for gnokii utility. Profile handling functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>

#include "gnokii-app.h"
#include "gnokii.h"

void profile_usage(FILE *f)
{
	fprintf(f, _("Profile options:\n"
		     "          --getprofile [start_number [end_number]] [-r|--raw]\n"
		     "          --setprofile\n"
		     "          --getactiveprofile\n"
		     "          --setactiveprofile profile_number\n"
		));
}

int getprofile_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getprofile [start_number [end_number]] [-r|--raw]\n"));
	return exitval;
}

/* Reads profile from phone and displays its' settings */
gn_error getprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	int max_profiles;
	int start, stop, i;
	gn_profile p;
	gn_ringtone_list rl;
	gn_error error = GN_ERR_NOTSUPPORTED;

	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[64] = "";
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			return getprofile_usage(stderr, -1); /* FIXME */
		}
	}

	gn_data_clear(data);
	data->model = model;
	while (gn_sm_functions(GN_OP_GetModel, data, state) != GN_ERR_NONE)
		sleep(1);

	p.number = 0;
	memset(&rl, 0, sizeof(gn_ringtone_list));
	gn_data_clear(data);
	data->profile = &p;
	data->ringtone_list = &rl;
	error = gn_sm_functions(GN_OP_GetProfile, data, state);

	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return error;
	}

	max_profiles = 7; /* This is correct for 6110 (at least my). How do we get
			     the number of profiles? */

	/* For N5110 */
	/* FIXME: It should be set to 3 for N5130 and 3210 too */
	if (!strcmp(model, "NSE-1"))
		max_profiles = 3;

	if (argc > optind) {
		start = gnokii_atoi(argv[optind]);
		if (errno || start < 0)
			return getprofile_usage(stderr, -1);
		stop = (argc > optind + 1) ? gnokii_atoi(argv[optind + 1]) : start;
		if (errno || stop < 0)
			return getprofile_usage(stderr, -1);

		if (start > stop) {
			fprintf(stderr, _("Starting profile number is greater than stop\n"));
			return GN_ERR_FAILED;
		}

		if (start < 0) {
			fprintf(stderr, _("Profile number must be value from 0 to %d!\n"), max_profiles - 1);
			return GN_ERR_FAILED;
		}

		if (stop >= max_profiles) {
			fprintf(stderr, _("This phone supports only %d profiles!\n"), max_profiles);
			return GN_ERR_FAILED;
		}
	} else {
		start = 0;
		stop = max_profiles - 1;
	}

	gn_data_clear(data);
	data->profile = &p;
	data->ringtone_list = &rl;

	for (i = start; i <= stop; i++) {
		p.number = i;

		if (p.number != 0) {
			error = gn_sm_functions(GN_OP_GetProfile, data, state);
			if (error != GN_ERR_NONE) {
				fprintf(stderr, _("Cannot get profile %d\n"), i);
				fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
				return error;
			}
		}

		if (raw) {
			fprintf(stdout, "%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
				p.number, p.name, p.default_name, p.keypad_tone,
				p.lights, p.call_alert, p.ringtone, p.volume,
				p.message_tone, p.vibration, p.warning_tone,
				p.caller_groups, p.automatic_answer);
		} else {
			/* Translators: %d is the profile number, %s is the profile name. Example: 1. "Outdoors" */
			fprintf(stdout, _("%d. \"%s\"\n"), p.number, p.name);
			if (p.default_name == -1) fprintf(stdout, _(" (name defined)\n"));
			fprintf(stdout, _("Incoming call alert: %s\n"), gn_profile_callalert_type2str(p.call_alert));
			fprintf(stdout, _("Ringing tone: %s (%d)\n"), get_ringtone_name(p.ringtone, data, state), p.ringtone);
			fprintf(stdout, _("Ringing volume: %s\n"), gn_profile_volume_type2str(p.volume));
			fprintf(stdout, _("Message alert tone: %s\n"), gn_profile_message_type2str(p.message_tone));
			fprintf(stdout, _("Keypad tones: %s\n"), gn_profile_keyvol_type2str(p.keypad_tone));
			fprintf(stdout, _("Warning and game tones: %s\n"), gn_profile_warning_type2str(p.warning_tone));

			/* FIXME: Light settings is only used for Car */
			if (p.number == (max_profiles - 2)) fprintf(stdout, _("Lights: %s\n"), p.lights ? _("On") : _("Automatic"));
			fprintf(stdout, _("Vibration: %s\n"), gn_profile_vibration_type2str(p.vibration));

			/* FIXME: it will be nice to add here reading caller group name. */
			if (max_profiles != 3) fprintf(stdout, _("Caller groups: 0x%02x\n"), p.caller_groups);

			/* FIXME: Automatic answer is only used for Car and Headset. */
			if (p.number >= (max_profiles - 2)) fprintf(stdout, _("Automatic answer: %s\n"), p.automatic_answer ? _("On") : _("Off"));
			fprintf(stdout, "\n");
		}
	}

	return error;
}

/* Writes profiles to phone */
gn_error setprofile(gn_data *data, struct gn_statemachine *state)
{
	int n;
	gn_profile p;
	gn_error error = GN_ERR_NONE;
	char line[256], ch;

	gn_data_clear(data);
	data->profile = &p;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}

		n = sscanf(line, "%d;%39[^;];%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d%c",
			    &p.number, p.name, &p.default_name, &p.keypad_tone,
			    &p.lights, &p.call_alert, &p.ringtone, &p.volume,
			    &p.message_tone, &p.vibration, &p.warning_tone,
			    &p.caller_groups, &p.automatic_answer, &ch);
		if (n != 13) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			return GN_ERR_WRONGDATAFORMAT;
		}

		error = gn_sm_functions(GN_OP_SetProfile, data, state);
		if (error != GN_ERR_NONE) {
			fprintf(stderr, _("Cannot set profile: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return error;
}

/* Queries the active profile */
gn_error getactiveprofile(gn_data *data, struct gn_statemachine *state)
{
	gn_profile p;
	gn_error error;

	gn_data_clear(data);
	data->profile = &p;

	error = gn_sm_functions(GN_OP_GetActiveProfile, data, state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Cannot get active profile: %s\n"), gn_error_print(error));
		return error;
	}

	error = gn_sm_functions(GN_OP_GetProfile, data, state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Cannot get profile %d\n"), p.number);
	else
		fprintf(stdout, _("Active profile: %d (%s)\n"), p.number, p.name);

	return error;
}

int setactiveprofile_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --setactiveprofile profile_number\n"));
	return exitval;
}

/* Select the specified profile */
gn_error setactiveprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_profile p;
	gn_error error;

	gn_data_clear(data);
	data->profile = &p;
	p.number = gnokii_atoi(optarg);
	if (errno || p.number < 0)
		return setactiveprofile_usage(stderr, -1);

	error = gn_sm_functions(GN_OP_SetActiveProfile, data, state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Cannot set active profile to %d: %s\n"), p.number, gn_error_print(error));
	return error;
}

