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

  Mainline code for gnokii utility. Calendar functions.

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

void calendar_usage(FILE *f)
{
	fprintf(f, _(
		     "Calendar options:\n"
		     "          --getcalendarnote start_number [end_number|end] [-v|--vCal]\n"
		     "          --writecalendarnote vcalendarfile start_number [end_number|end]\n"
		     "          --deletecalendarnote start [end_number|end]\n"
		     ));
}

int getcalendarnote_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --getcalendarnote start_number [end_number | end] [-v|--vCal]\n"
			"                        start_number - entry number in the phone calendar (numeric)\n"
			"                        end_number   - read to this entry in the phone calendar (numeric, optional)\n"
			"                        end          - the string \"end\" indicates all entries from start to end\n"
			"                        -v           - output in iCalendar format\n"
			"  NOTE: if no end is given, only the start entry is read\n"
	));
	return exitval;
}

/* Calendar notes receiving. */
gn_error getcalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_calnote_list		calnotelist;
	gn_calnote		calnote;
	gn_error		error = GN_ERR_NONE;
	int			i, first_location, last_location;
	bool			vcal = false;

	struct option options[] = {
		{ "vCal",    optional_argument, NULL, 'v'},
		{ NULL,      0,                 NULL, 0}
	};

	first_location = gnokii_atoi(optarg);
	if (errno || first_location < 0)
		return getcalendarnote_usage(stderr, -1);
	last_location = parse_end_value_option(argc, argv, optind, first_location);
	if (errno || last_location < 0)
		return getcalendarnote_usage(stderr, -1);

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vcal = true;
			break;
		default:
			return getcalendarnote_usage(stderr, -1);
		}
	}
	if (argc > optind + 1) {
		/* There are too many arguments that don't start with '-' */
		return getcalendarnote_usage(stderr, -1);
	}

	for (i = first_location; i <= last_location; i++) {
		memset(&calnote, 0, sizeof(calnote));
		memset(&calnotelist, 0, sizeof(calnotelist));
		calnote.location = i;

		gn_data_clear(data);
		data->calnote = &calnote;
		data->calnote_list = &calnotelist;

		error = gn_sm_functions(GN_OP_GetCalendarNote, data, state);

		if (error == GN_ERR_NONE) {
			if (vcal) {
				char *ical;

				ical = gn_calnote2icalstr(&calnote);
				fprintf(stdout, "%s\n", ical);
				free (ical);
			} else {  /* Plain text output */
				/* Translators: this is the header of a calendar note. Example: 1 (1). Type: Meeting */
				fprintf(stdout, _("%d (%d). %s: %s\n"), i, calnote.location, _("Type"), gn_calnote_type2str(calnote.type));

				fprintf(stdout, _("   Start date: %d-%02d-%02d\n"), calnote.time.year,
					calnote.time.month,
					calnote.time.day);

				if (calnote.type != GN_CALNOTE_BIRTHDAY) {
					fprintf(stdout, _("   Start time: %02d:%02d:%02d\n"), calnote.time.hour,
						calnote.time.minute,
						calnote.time.second);

					if (calnote.end_time.year) {
						fprintf(stdout, _("   End date: %d-%02d-%02d\n"), calnote.end_time.year,
							calnote.end_time.month,
							calnote.end_time.day);

						fprintf(stdout, _("   End time: %02d:%02d:%02d\n"), calnote.end_time.hour,
							calnote.end_time.minute,
							calnote.end_time.second);
					}
				}

				if (calnote.alarm.enabled) {
					if (calnote.type == GN_CALNOTE_BIRTHDAY) {
						fprintf(stdout, _("   Alarm date: %02d-%02d\n"),
							calnote.alarm.timestamp.month,
							calnote.alarm.timestamp.day);
					} else {
						fprintf(stdout, _("   Alarm date: %d-%02d-%02d\n"),
							calnote.alarm.timestamp.year,
							calnote.alarm.timestamp.month,
							calnote.alarm.timestamp.day);
					}
					fprintf(stdout, _("   Alarm time: %02d:%02d:%02d\n"),
						calnote.alarm.timestamp.hour,
						calnote.alarm.timestamp.minute,
						calnote.alarm.timestamp.second);
					fprintf(stdout, _("   Alarm tone %s\n"),
						calnote.alarm.tone ? _("enabled") : _("disabled"));
				}

				fprintf(stdout, _("   %s: "), _("Repeat"));
				switch (calnote.recurrence) {
				case GN_CALNOTE_NEVER:
				case GN_CALNOTE_DAILY:
				case GN_CALNOTE_WEEKLY:
				case GN_CALNOTE_2WEEKLY:
				case GN_CALNOTE_MONTHLY:
				case GN_CALNOTE_YEARLY:
					fprintf(stdout,"%s\n", gn_calnote_recurrence2str(calnote.recurrence));
					break;
				default:
					fprintf(stdout, _("Every %d hours"), calnote.recurrence);
					break;
				}

				if (calnote.recurrence != GN_CALNOTE_NEVER) {
					fprintf(stdout, _("   The event will be repeated "));
					switch (calnote.occurrences) {
					case 0:
						fprintf(stdout, _("forever."));
						break;
					default:
						fprintf(stdout, _("%d times."), calnote.occurrences);
						break;
					}
					fprintf(stdout, "\n");
				}

				fprintf(stdout, _("   Text: %s\n"), calnote.text);

				switch (calnote.type) {
				case GN_CALNOTE_CALL:
					fprintf(stdout, _("   %s: %s\n"), _("Phone"), calnote.phone_number);
					break;
				case GN_CALNOTE_MEETING:
					fprintf(stdout, _("   %s: %s\n"), _("Location"), calnote.mlocation);
					break;
				case GN_CALNOTE_BIRTHDAY:
				case GN_CALNOTE_REMINDER:
				case GN_CALNOTE_MEMO:
					/* was already printed as calnote.text above */
					break;
				}
			}

		} else { /* error != GN_ERR_NONE */
			/* stop processing if the last note was specified as "end" */
			if (last_location == INT_MAX) {
				/* it's not an error if we read at least a note and the rest is empty */
				if ((i > first_location) && ((error == GN_ERR_EMPTYLOCATION) || (error == GN_ERR_INVALIDLOCATION))) {
					error = GN_ERR_NONE;
				}
				last_location = 0;
			}
			if (error != GN_ERR_NONE) {
				fprintf(stderr, _("The calendar note can not be read: %s\n"), gn_error_print(error));
			}
		}
	}

	return error;
}

int writecalendarnote_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --writecalendarnote vcalendarfile start_number [end_number|end]\n"
			"                        vcalendarfile - file containing calendar notes in vCal format\n"
			"                        start_number  - first calendar note from vcalendarfile to read\n"
			"                        end_number    - last calendar note from vcalendarfile to read\n"
			"                        end           - read all notes from vcalendarfile from start_number\n"
			"  NOTE: it stores the note at the first empty location.\n"
	));
	return exitval;
}

/* Writing calendar notes. */
gn_error writecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_calnote calnote;
	gn_error error = GN_ERR_NONE;
	int first_location, last_location, i;
	FILE *f;

	f = fopen(optarg, "r");
	if (f == NULL) {
		fprintf(stderr, _("Can't open file %s for reading!\n"), optarg);
		return GN_ERR_FAILED;
	}

	first_location = gnokii_atoi(argv[optind]);
	if (errno || first_location < 0) {
		fclose(f);
		return writecalendarnote_usage(stderr, -1);
	}
	last_location = parse_end_value_option(argc, argv, optind + 1, first_location);
	if (errno || last_location < 0) {
		fclose(f);
		return writecalendarnote_usage(stderr, -1);
	}
	
	for (i = first_location; i <= last_location; i++) {

		memset(&calnote, 0, sizeof(calnote));
		gn_data_clear(data);
		data->calnote = &calnote;

		/* TODO: gn_ical2note expects the pointer to begin file to
		 * iterate. Fix it to not rewind the file each time
		 */
		rewind(f);
		error = gn_ical2calnote(f, &calnote, i);

#ifndef WIN32
		if (error == GN_ERR_NOTIMPLEMENTED) {
			switch (gn_vcal_file_event_read(optarg, &calnote, i)) {
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
			/* when reading until 'end' it's not an error if it tried to read a non existant note */
			if ((last_location == INT_MAX) && (error == GN_ERR_EMPTYLOCATION)) {
				error = GN_ERR_NONE;
			} else {
				fprintf(stderr, _("Failed to load vCalendar file: %s\n"), gn_error_print(error));
			}
			fclose(f);
			return error;
		}
	
		error = gn_sm_functions(GN_OP_WriteCalendarNote, data, state);

		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Successfully written!\n"));
		else
			fprintf(stderr, _("Failed to write calendar note: %s\n"), gn_error_print(error));
	}
	fclose(f);

	return error;
}

int deletecalendarnote_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --deletecalendarnote start_number [end_number | end]\n"
			"                        start_number - first number in the phone calendar (numeric)\n"
			"                        end_number   - until this entry in the phone calendar (numeric, optional)\n"
			"                        end          - the string \"end\" indicates all entries from start to end\n"
			"  NOTE: if no end is given, only the start entry is deleted\n"
	));
	return exitval;
}

/* Calendar note deleting. */
gn_error deletecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_calnote calnote;
	gn_calnote_list clist;
	int i, first_location, last_location;
	gn_error error = GN_ERR_NONE;

	gn_data_clear(data);
	memset(&calnote, 0, sizeof(gn_calnote));
	data->calnote = &calnote;
	memset(&clist, 0, sizeof(gn_calnote_list));
	data->calnote_list = &clist;

	first_location = gnokii_atoi(optarg);
	if (errno || first_location < 0)
		return deletecalendarnote_usage(stderr, -1);
	last_location = parse_end_value_option(argc, argv, optind, first_location);
	if (errno || last_location < 0)
		return deletecalendarnote_usage(stderr, -1);

	for (i = first_location; i <= last_location; i++) {
		calnote.location = i;

		error = gn_sm_functions(GN_OP_DeleteCalendarNote, data, state);
		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Calendar note deleted.\n"));
		else {
			fprintf(stderr, _("The calendar note cannot be deleted: %s\n"), gn_error_print(error));
			if (last_location == INT_MAX)
				last_location = 0;
		}
	}

	return error;
}
