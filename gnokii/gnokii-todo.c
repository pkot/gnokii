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

  Mainline code for gnokii utility. Todo functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "gnokii-app.h"
#include "gnokii.h"

void todo_usage(FILE *f)
{
	fprintf(f, _(
		     "ToDo options:\n"
		     "          --gettodo start_number [end_number|end] [-v|--vCal]\n"
		     "          --writetodo vcalendarfile number\n"
		     "          --deletealltodos\n"
		     ));
}

void gettodo_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --gettodo start_number [end_number | end] [-v|--vCal]\n"
			 "       start_number - entry number in the phone todo (numeric)\n"
			 "       end_number   - until this entry in the phone todo (numeric, optional)\n"
			 "       end          - the string \"end\" indicates all entries from start to end\n"
			 "       -v           - output in iCalendar format\n"
			 "  NOTE: if no end is given, only the start entry is written\n"
	));
	exit(exitval);
}

/* ToDo notes receiving. */
int gettodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_todo_list	todolist;
	gn_todo		todo;
	gn_error	error = GN_ERR_NONE;
	bool		vcal = false;
	int		i, first_location, last_location;

	struct option options[] = {
		{ "vCal",    optional_argument, NULL, 'v'},
		{ NULL,      0,                 NULL, 0}
	};

	first_location = atoi(optarg);
	last_location = parse_end_value_option(argc, argv, optind, first_location);

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vcal = true;
			break;
		default:
			gettodo_usage(stderr, -1); /* Would be better to have an calendar_usage() here. */
			return -1;
		}
	}

	for (i = first_location; i <= last_location; i++) {
		todo.location = i;

		gn_data_clear(data);
		data->todo = &todo;
		data->todo_list = &todolist;

		error = gn_sm_functions(GN_OP_GetToDo, data, state);
		switch (error) {
		case GN_ERR_NONE:
			if (vcal) {
				gn_todo2ical(stdout, &todo);
			} else {
				fprintf(stdout, _("Todo: %s\n"), todo.text);
				fprintf(stdout, _("Priority: "));
				switch (todo.priority) {
				case GN_TODO_LOW:
					fprintf(stdout, _("low\n"));
					break;
				case GN_TODO_MEDIUM:
					fprintf(stdout, _("medium\n"));
					break;
				case GN_TODO_HIGH:
					fprintf(stdout, _("high\n"));
					break;
				default:
					fprintf(stdout, _("unknown\n"));
					break;
				}
			}
			break;
		default:
			fprintf(stderr, _("The ToDo note could not be read: %s\n"), gn_error_print(error));
			if (last_location == INT_MAX)
				last_location = 0;
			break;
		}
	}
	return error;
}

/* ToDo notes writing */
int writetodo(char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_todo todo;
	gn_error error;
	int location;
	FILE *f;

	gn_data_clear(data);
	data->todo = &todo;

	f = fopen(optarg, "r");
	if (!f) {
		fprintf(stderr, _("Can't open file %s for reading!\n"), optarg);
		return GN_ERR_FAILED;
	}

	location = atoi(argv[optind]);

	error = gn_ical2todo(f, &todo, location);
	fclose(f);
#ifndef WIN32
	if (error == GN_ERR_NOTIMPLEMENTED) {
		switch (gn_vcal_file_todo_read(optarg, &todo, location)) {
		case 0:
			error = GN_ERR_NONE;
			break;
		default:
			error = GN_ERR_FAILED;
			break;
		}
	}
#endif
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load vCalendar file: %s\n"), gn_error_print(error));
		return error;
	}

	error = gn_sm_functions(GN_OP_WriteToDo, data, state);

	if (error == GN_ERR_NONE) {
		fprintf(stderr, _("Successfully written!\n"));
		fprintf(stderr, _("Priority %d. %s\n"), data->todo->priority, data->todo->text);
	} else
		fprintf(stderr, _("Failed to write ToDo note: %s\n"), gn_error_print(error));
	return error;
}

/* Deleting all ToDo notes */
int deletealltodos(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	gn_data_clear(data);

	error = gn_sm_functions(GN_OP_DeleteAllToDos, data, state);
	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Successfully deleted all ToDo notes!\n"));
	else
		fprintf(stderr, _("Failed to delete ToDo note: %s\n"), gn_error_print(error));
	return error;
}

