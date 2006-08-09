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

  Mainline code for gnokii utility. Ringtone handling functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <signal.h>

#include "gnokii-app.h"
#include "gnokii.h"

static gn_ringtone_list ringtone_list;
static int ringtone_list_initialised = 0;

static void init_ringtone_list(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	if (ringtone_list_initialised) return;

	memset(&ringtone_list, 0, sizeof(ringtone_list));
	data->ringtone_list = &ringtone_list;

	error = gn_sm_functions(GN_OP_GetRingtoneList, data, state);

	data->ringtone_list = NULL;

	if (error != GN_ERR_NONE) {
		ringtone_list.count = 0;
		ringtone_list.userdef_location = 0;
		ringtone_list.userdef_count = 0;
		ringtone_list_initialised = -1;
	} else
		ringtone_list_initialised = 1;
}

char *get_ringtone_name(int id, gn_data *data, struct gn_statemachine *state)
{
	int i;

	init_ringtone_list(data, state);

	for (i = 0; i < ringtone_list.count; i++) {
		if (ringtone_list.ringtone[i].location == id)
			return ringtone_list.ringtone[i].name;
	}

	return _("Unknown");
}

void ringtone_usage(FILE *f)
{
	fprintf(f, _(
		     "Ringtone options:\n"
		     "          --sendringtone rtttlfile destination\n"
		     "          --getringtone rtttlfile [location] [-r|--raw]\n"
		     "          --setringtone rtttlfile [location] [-r|--raw] [--name name]\n"
		     "          --playringtone rtttlfile [--volume vol]\n"
		     "          --ringtoneconvert source destination\n"
		     "          --getringtonelist\n"
		     "          --deleteringtone start [end]\n"
		     ));
}

int sendringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_sms sms;
	gn_error error = GN_ERR_NOTSUPPORTED;

	gn_sms_default_submit(&sms);
	sms.user_data[0].type = GN_SMS_DATA_Ringtone;
	sms.user_data[1].type = GN_SMS_DATA_None;

	if ((error = gn_file_ringtone_read(optarg, &sms.user_data[0].u.ringtone))) {
		fprintf(stderr, _("Failed to load ringtone.\n"));
		return error;
	}

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&sms.remote.number, 0, sizeof(sms.remote.number));
	strncpy(sms.remote.number, argv[optind], sizeof(sms.remote.number) - 1);
	if (sms.remote.number[0] == '+')
		sms.remote.type = GN_GSM_NUMBER_International;
	else
		sms.remote.type = GN_GSM_NUMBER_Unknown;

	/* Get the SMS Center */
	if (!sms.smsc.number[0]) {
		data->message_center = calloc(1, sizeof(gn_sms_message_center));
		data->message_center->id = 1;
		if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
			strcpy(sms.smsc.number, data->message_center->smsc.number);
			sms.smsc.type = data->message_center->smsc.type;
		}
		free(data->message_center);
	}

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	/* Send the message. */
	data->sms = &sms;
	error = gn_sms_send(data, state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	return error;
}

void getringtone_usage(FILE *f, int exitval)
{
	fprintf(f, _(""
		));
	exit(exitval);
}

int getringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone ringtone;
	gn_raw_data rawdata;
	gn_error error;
	unsigned char buff[512];
	int i;

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
			getringtone_usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	rawdata.data = buff;
	rawdata.length = sizeof(buff);
	gn_data_clear(data);
	data->ringtone = &ringtone;
	data->raw_data = &rawdata;

	if (argc <= optind) {
		getringtone_usage(stderr, -1);
		return -1;
	}

	if (argc > optind + 1) {
		ringtone.location = atoi(argv[optind + 1]);
	} else {
		init_ringtone_list(data, state);
		ringtone.location = ringtone_list.userdef_location;
	}

	if (raw)
		error = gn_sm_functions(GN_OP_GetRawRingtone, data, state);
	else
		error = gn_sm_functions(GN_OP_GetRingtone, data, state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Getting ringtone %d failed: %s\n"), ringtone.location, gn_error_print(error));
		return error;
	}
	fprintf(stderr, _("Getting ringtone %d (\"%s\") succeeded!\n"), ringtone.location, ringtone.name);

	if (raw) {
		FILE *f;

		if ((f = fopen(argv[optind], "wb")) == NULL) {
			fprintf(stderr, _("Failed to save ringtone.\n"));
			return -1;
		}
		fwrite(rawdata.data, 1, rawdata.length, f);
		fclose(f);
	} else {
		if ((error = gn_file_ringtone_save(argv[optind], &ringtone)) != GN_ERR_NONE) {
			fprintf(stderr, _("Failed to save ringtone: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return error;
}

void setringtone_usage(FILE *f, int exitval)
{
	fprintf(f, _(""
		));
	exit(exitval);
}

int setringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone ringtone;
	gn_raw_data rawdata;
	gn_error error;
	unsigned char buff[512];
	int i, location;

	bool raw = false;
	char name[16] = "";
	struct option options[] = {
		{ "raw",    no_argument,       NULL, 'r'},
		{ "name",   required_argument, NULL, 'n'},
		{ NULL,     0,                 NULL, 0}
	};

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		case 'n':
			snprintf(name, sizeof(name), "%s", optarg);
			break;
		default:
			setringtone_usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	rawdata.data = buff;
	rawdata.length = sizeof(buff);
	gn_data_clear(data);
	data->ringtone = &ringtone;
	data->raw_data = &rawdata;

	if (argc <= optind) {
		setringtone_usage(stderr, -1);
		return -1;
	}

	location = (argc > optind + 1) ? atoi(argv[optind + 1]) : -1;

	if (raw) {
		FILE *f;

		if ((f = fopen(argv[optind], "rb")) == NULL) {
			fprintf(stderr, _("Failed to load ringtone.\n"));
			return -1;
		}
		rawdata.length = fread(rawdata.data, 1, rawdata.length, f);
		fclose(f);
		ringtone.location = location;
		if (*name)
			snprintf(ringtone.name, sizeof(ringtone.name), "%s", name);
		else
			snprintf(ringtone.name, sizeof(ringtone.name), "GNOKII");
		error = gn_sm_functions(GN_OP_SetRawRingtone, data, state);
	} else {
		if ((error = gn_file_ringtone_read(argv[optind], &ringtone))) {
			fprintf(stderr, _("Failed to load ringtone.\n"));
			return error;
		}
		ringtone.location = location;
		if (*name) snprintf(ringtone.name, sizeof(ringtone.name), "%s", name);
		error = gn_sm_functions(GN_OP_SetRingtone, data, state);
	}

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("Send failed: %s\n"), gn_error_print(error));

	return error;
}

void playringtone_usage(FILE *f, int exitval)
{
	fprintf(f, _(""
		));
	exit(exitval);
}

int playringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone ringtone;
	gn_tone tone;
	gn_error error;
	int i, ulen;
	struct timeval dt;
#if (defined HAVE_TIMEOPS) && (defined HAVE_GETTIMEOFDAY)
	struct timeval t1, t2;
#endif

	int volume = 5;
	struct option options[] = {
		{ "volume", required_argument, NULL, 'v'},
		{ NULL,     0,                 NULL, 0}
	};

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			volume = atoi(optarg);
			break;
		default:
			playringtone_usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	memset(&tone, 0, sizeof(tone));
	gn_data_clear(data);
	data->ringtone = &ringtone;
	data->tone = &tone;

	if (argc <= optind) {
		playringtone_usage(stderr, -1);
		return -1;
	}

	if ((error = gn_file_ringtone_read(argv[optind], &ringtone))) {
		fprintf(stderr, _("Failed to load ringtone: %s\n"), gn_error_print(error));
		return error;
	}

#if (defined HAVE_TIMEOPS) && (defined HAVE_GETTIMEOFDAY)
	gettimeofday(&t1, NULL);
	tone.frequency = 0;
	tone.volume = 0;
	gn_sm_functions(GN_OP_PlayTone, data, state);
	gettimeofday(&t2, NULL);
	timersub(&t2, &t1, &dt);
#else
	dt.tv_sec = 0;
	dt.tv_usec = 20000;
#endif

	signal(SIGINT, interrupted);
	for (i = 0; !bshutdown && i < ringtone.notes_count; i++) {
		tone.volume = volume;
		gn_ringtone_get_tone(&ringtone, i, &tone.frequency, &ulen);
		if ((error = gn_sm_functions(GN_OP_PlayTone, data, state)) != GN_ERR_NONE) break;
		if (ulen > 2 * dt.tv_usec + 20000)
			usleep(ulen - 2 * dt.tv_usec - 20000);
		tone.volume = 0;
		if ((error = gn_sm_functions(GN_OP_PlayTone, data, state)) != GN_ERR_NONE) break;
		usleep(20000);
	}
	tone.frequency = 0;
	tone.volume = 0;
	gn_sm_functions(GN_OP_PlayTone, data, state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Play succeeded!\n"));
	else
		fprintf(stderr, _("Play failed: %s\n"), gn_error_print(error));

	return error;
}

void ringtoneconvert_usage(FILE *f, int exitval)
{
	fprintf(f, _(""
		));
	exit(exitval);
}

int ringtoneconvert(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone ringtone;
	gn_error error;

	memset(&ringtone, 0, sizeof(ringtone));
	gn_data_clear(data);
	data->ringtone = &ringtone;

	if (argc != optind + 1) {
		ringtoneconvert_usage(stderr, -1);
		return -1;
	}

	if ((error = gn_file_ringtone_read(optarg, &ringtone)) != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load ringtone: %s\n"), gn_error_print(error));
		return error;
	}
	if ((error = gn_file_ringtone_save(argv[optind], &ringtone)) != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to save ringtone: %s\n"), gn_error_print(error));
		return error;
	}

	fprintf(stderr, _("%d note(s) converted.\n"), ringtone.notes_count);

	return error;
}

int getringtonelist(gn_data *data, struct gn_statemachine *state)
{
	int i;

	init_ringtone_list(data, state);

	if (ringtone_list_initialised < 0) {
		fprintf(stderr, _("Failed to get the list of ringtones\n"));
		return GN_ERR_UNKNOWN;
	}

	printf(_("First user defined ringtone location: %3d\n"), ringtone_list.userdef_location);
	printf(_("Number of user defined ringtones: %d\n\n"), ringtone_list.userdef_count);
	printf(_("loc   rwu   name\n"));
	printf(_("===============================\n"));
	for (i = 0; i < ringtone_list.count; i++) {
		printf(_("%3d   %d%d%d   %-20s\n"), ringtone_list.ringtone[i].location,
			ringtone_list.ringtone[i].readable,
			ringtone_list.ringtone[i].writable,
			ringtone_list.ringtone[i].user_defined,
			ringtone_list.ringtone[i].name);
	}

	return GN_ERR_NONE;
}

void deleteringtone_usage(FILE *f, int exitval)
{
	fprintf(f, _(""));
	exit(exitval);
}

gn_error deleteringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_ringtone ringtone;
	gn_error error;
	int i, start, end;

	memset(&ringtone, 0, sizeof(ringtone));
	gn_data_clear(data);
	data->ringtone = &ringtone;

	start = atoi(optarg);
	end = parse_end_value_option(argc, argv, optind, start);

	for (i = start; i <= end; i++) {
		ringtone.location = i;
		if ((error = gn_sm_functions(GN_OP_DeleteRingtone, data, state)) == GN_ERR_NONE)
			printf(_("Ringtone %d deleted\n"), i);
		else
			printf(_("Failed to delete ringtone %d: %s\n"), i, gn_error_print(error));
	}

	return GN_ERR_NONE;
}

