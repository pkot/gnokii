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

  Mainline code for gnokii utility. Phone settings handling functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>
#include <time.h>

#include "gnokii-app.h"
#include "gnokii.h"

void settings_usage(FILE *f)
{
	fprintf(f, _("Phone settings:\n"
		     "          --reset [soft|hard]\n"
		     "          --setdatetime [YYYY [MM [DD [HH [MM]]]]]\n"
		     "          --getdatetime\n"
		     "          --setalarm [HH MM]\n"
		     "          --getalarm\n"
		     ));

}

/* Setting the date and time. */
gn_error setdatetime(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	struct tm *now;
	time_t nowh;
	gn_timestamp date;
	gn_error error;

	nowh = time(NULL);
	now = localtime(&nowh);

	date.year = now->tm_year;
	date.month = now->tm_mon+1;
	date.day = now->tm_mday;
	date.hour = now->tm_hour;
	date.minute = now->tm_min;
	date.second = now->tm_sec;

	switch (argc - optind - 1) {
	case 4:
		date.minute = atoi(argv[optind+4]);
	case 3:
		date.hour   = atoi(argv[optind+3]);
	case 2:
		date.day    = atoi(argv[optind+2]);
	case 1:
		date.month  = atoi(argv[optind+1]);
	case 0:
		date.year   = atoi(argv[optind]);
		break;
	default:
		break;
	}

	if (date.year < 1900) {

		/* Well, this thing is copyrighted in U.S. This technique is known as
		   Windowing and you can read something about it in LinuxWeekly News:
		   http://lwn.net/1999/features/Windowing.phtml. This thing is beeing
		   written in Czech republic and Poland where algorithms are not allowed
		   to be patented. */

		if (date.year > 90)
			date.year = date.year + 1900;
		else
			date.year = date.year + 2000;
	}

	gn_data_clear(data);
	data->datetime = &date;
	error = gn_sm_functions(GN_OP_SetDateTime, data, state);

	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* In this mode we receive the date and time from mobile phone. */
gn_error getdatetime(gn_data *data, struct gn_statemachine *state)
{
	gn_timestamp	date_time;
	gn_error	error;

	gn_data_clear(data);
	data->datetime = &date_time;

	error = gn_sm_functions(GN_OP_GetDateTime, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.year, date_time.month, date_time.day);
		fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.hour, date_time.minute, date_time.second);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Setting the alarm. */
gn_error setalarm(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_calnote_alarm alarm;
	gn_error error;

	if (argc == optind + 2) {
		alarm.timestamp.hour = atoi(argv[optind]);
		alarm.timestamp.minute = atoi(argv[optind + 1]);
		alarm.timestamp.second = 0;
		alarm.enabled = true;
	} else {
		alarm.timestamp.hour = 0;
		alarm.timestamp.minute = 0;
		alarm.timestamp.second = 0;
		alarm.enabled = false;
	}

	gn_data_clear(data);
	data->alarm = &alarm;

	error = gn_sm_functions(GN_OP_SetAlarm, data, state);

	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Getting the alarm. */
gn_error getalarm(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_calnote_alarm alarm;

	gn_data_clear(data);
	data->alarm = &alarm;

	error = gn_sm_functions(GN_OP_GetAlarm, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Alarm: %s\n"), (alarm.enabled)? _("on"): _("off"));
		fprintf(stdout, _("Time: %02d:%02d\n"), alarm.timestamp.hour, alarm.timestamp.minute);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}
/* Resets the phone */
gn_error reset(char *type, gn_data *data, struct gn_statemachine *state)
{
	gn_data_clear(data);
	data->reset_type = 0x03;

	if (type) {
		if (!strcmp(type, "soft"))
			data->reset_type = 0x03;
		else
			if (!strcmp(type, "hard")) {
				data->reset_type = 0x04;
			} else {
				fprintf(stderr, _("What kind of reset do you want??\n"));
				return -1;
			}
	}

	return gn_sm_functions(GN_OP_Reset, data, state);
}
