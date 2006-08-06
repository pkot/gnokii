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
#include <unistd.h>
#define _GNU_SOURCE
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

static char *profile_get_call_alert_string(int code)
{
	switch (code) {
	case GN_PROFILE_CALLALERT_Ringing:	return _("Ringing");
	case GN_PROFILE_CALLALERT_Ascending:	return _("Ascending");
	case GN_PROFILE_CALLALERT_RingOnce:	return _("Ring once");
	case GN_PROFILE_CALLALERT_BeepOnce:	return _("Beep once");
	case GN_PROFILE_CALLALERT_CallerGroups:	return _("Caller groups");
	case GN_PROFILE_CALLALERT_Off:		return _("Off");
	default:				return _("Unknown");
	}
}

static char *profile_get_volume_string(int code)
{
	switch (code) {
	case GN_PROFILE_VOLUME_Level1:		return _("Level 1");
	case GN_PROFILE_VOLUME_Level2:		return _("Level 2");
	case GN_PROFILE_VOLUME_Level3:		return _("Level 3");
	case GN_PROFILE_VOLUME_Level4:		return _("Level 4");
	case GN_PROFILE_VOLUME_Level5:		return _("Level 5");
	default:				return _("Unknown");
	}
}

static char *profile_get_keypad_tone_string(int code)
{
	switch (code) {
	case GN_PROFILE_KEYVOL_Off:		return _("Off");
	case GN_PROFILE_KEYVOL_Level1:		return _("Level 1");
	case GN_PROFILE_KEYVOL_Level2:		return _("Level 2");
	case GN_PROFILE_KEYVOL_Level3:		return _("Level 3");
	default:				return _("Unknown");
	}
}

static char *profile_get_message_tone_string(int code)
{
	switch (code) {
	case GN_PROFILE_MESSAGE_NoTone:		return _("No tone");
	case GN_PROFILE_MESSAGE_Standard:	return _("Standard");
	case GN_PROFILE_MESSAGE_Special:	return _("Special");
	case GN_PROFILE_MESSAGE_BeepOnce:	return _("Beep once");
	case GN_PROFILE_MESSAGE_Ascending:	return _("Ascending");
	default:				return _("Unknown");
	}
}

static char *profile_get_warning_tone_string(int code)
{
	switch (code) {
	case GN_PROFILE_WARNING_Off:		return _("Off");
	case GN_PROFILE_WARNING_On:		return _("On");
	default:				return _("Unknown");
	}
}

static char *profile_get_vibration_string(int code)
{
	switch (code) {
	case GN_PROFILE_VIBRATION_Off:		return _("Off");
	case GN_PROFILE_VIBRATION_On:		return _("On");
	default:				return _("Unknown");
	}
}

void getprofile_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --getprofile [start_number [end_number]] [-r|--raw]\n"));
	exit(exitval);
}

/* Reads profile from phone and displays its' settings */
int getprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	int max_profiles;
	int start, stop, i;
	gn_profile p;
	gn_error error = GN_ERR_NOTSUPPORTED;

	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[64];
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			getprofile_usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	gn_data_clear(data);
	data->model = model;
	while (gn_sm_functions(GN_OP_GetModel, data, state) != GN_ERR_NONE)
		sleep(1);

	p.number = 0;
	gn_data_clear(data);
	data->profile = &p;
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
		start = atoi(argv[optind]);
		stop = (argc > optind + 1) ? atoi(argv[optind + 1]) : start;

		if (start > stop) {
			fprintf(stderr, _("Starting profile number is greater than stop\n"));
			return -1;
		}

		if (start < 0) {
			fprintf(stderr, _("Profile number must be value from 0 to %d!\n"), max_profiles - 1);
			return -1;
		}

		if (stop >= max_profiles) {
			fprintf(stderr, _("This phone supports only %d profiles!\n"), max_profiles);
			return -1;
		}
	} else {
		start = 0;
		stop = max_profiles - 1;
	}

	gn_data_clear(data);
	data->profile = &p;

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
			fprintf(stdout, "%d. \"%s\"\n", p.number, p.name);
			if (p.default_name == -1) fprintf(stdout, _(" (name defined)\n"));
			fprintf(stdout, _("Incoming call alert: %s\n"), profile_get_call_alert_string(p.call_alert));
			fprintf(stdout, _("Ringing tone: %s (%d)\n"), get_ringtone_name(p.ringtone, data, state), p.ringtone);
			fprintf(stdout, _("Ringing volume: %s\n"), profile_get_volume_string(p.volume));
			fprintf(stdout, _("Message alert tone: %s\n"), profile_get_message_tone_string(p.message_tone));
			fprintf(stdout, _("Keypad tones: %s\n"), profile_get_keypad_tone_string(p.keypad_tone));
			fprintf(stdout, _("Warning and game tones: %s\n"), profile_get_warning_tone_string(p.warning_tone));

			/* FIXME: Light settings is only used for Car */
			if (p.number == (max_profiles - 2)) fprintf(stdout, _("Lights: %s\n"), p.lights ? _("On") : _("Automatic"));
			fprintf(stdout, _("Vibration: %s\n"), profile_get_vibration_string(p.vibration));

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
int setprofile(gn_data *data, struct gn_statemachine *state)
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
			return GN_ERR_UNKNOWN;
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
int getactiveprofile(gn_data *data, struct gn_statemachine *state)
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

/* Select the specified profile */
int setactiveprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_profile p;
	gn_error error;

	gn_data_clear(data);
	data->profile = &p;
	p.number = atoi(optarg);

	error = gn_sm_functions(GN_OP_SetActiveProfile, data, state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Cannot set active profile to %d: %s\n"), p.number, gn_error_print(error));
	return error;
}

