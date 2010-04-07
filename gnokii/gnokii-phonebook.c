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

  Mainline code for gnokii utility. Phonebook functions.

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

void phonebook_usage(FILE *f)
{
	fprintf(f, _("Phonebook options:\n"
		     "          --getphonebook memory_type start_number [end_number|end]\n"
		     "                 [[-r|--raw]|[-v|--vcard]|[-l|--ldif]]\n"
		     "          --writephonebook [[-o|--overwrite]|[-f|--find-free]]\n"
		     "                 [-m|--memory-type|--memory memory_type]\n"
		     "                 [-n|--memory-location|--location number]\n"
		     "                 [[-v|--vcard]|[-l|--ldif]]\n"
		     "          --deletephonebook memory_type start_number [end_number|end]\n"));
}

/* Displays usage of --getphonebook command */
int getphonebook_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getphonebook memory start [end]  reads phonebook entries from memory type\n"
			"                                         (SM, ME, IN, OU, ...) messages starting\n"
			"                                         from location start and ending with end;\n"
			"                                         if end option is omitted, just one entry\n"
			"                                         is read;\n"
			"                                         if 'end' is used all entries are read\n"
			"       -r\n"
			"       --raw                             output in raw form (comma separated)\n"
			"       -v\n"
			"       --vcard                           output in vcard format\n"
			"       -l\n"
			"       --ldif                            output in ldif format\n"
			"\n"
		));
	return exitval;
}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */
gn_error getphonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_phonebook_entry entry;
	gn_memory_status memstat;
	int i, count, start_entry, end_entry, num_entries = INT_MAX, explicit_end = true;
	gn_error error = GN_ERR_NONE;
	char *memory_type_string;
	int type = 0; /* Output type:
				0 - not formatted
				1 - CSV
				2 - vCard
				3 - LDIF
			*/
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r' },
		{ "vcard",  no_argument, NULL, 'v' },
		{ "ldif",   no_argument, NULL, 'l' },
		{ NULL,     0,           NULL, 0 }
	};

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = optarg;
	memstat.memory_type = gn_str2memory_type(memory_type_string);
	if (memstat.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), optarg);
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	start_entry = gnokii_atoi(argv[optind]);
	if (errno || start_entry < 0)
		return getphonebook_usage(stderr, -1);
	end_entry = parse_end_value_option(argc, argv, optind + 1, start_entry);
	if (errno || end_entry < 0)
		return getphonebook_usage(stderr, -1);

	i = getopt_long(argc, argv, "rvl", options, NULL);
	switch (i) {
	case 'r':
		type = 1;
		break;
	case 'v':
		type = 2;
		break;
	case 'l':
		type = 3;
		break;
	case -1:
		/* default */
		break;
	default:
		return getphonebook_usage(stderr, -1);
	}
	if (argc - optind > 2) {
		/* There are too many arguments that don't start with '-' */
		return getphonebook_usage(stderr, -1);
	}

	if (end_entry == INT_MAX) {
		explicit_end = false;
		data->memory_status = &memstat;
		if ((error = gn_sm_functions(GN_OP_GetMemoryStatus, data, state)) == GN_ERR_NONE) {
			num_entries = memstat.used;
			end_entry = memstat.used + memstat.free;
			/* FIXME warn the user like parse_end_value_option() when end_entry < start_entry */
			if (end_entry < start_entry) end_entry = start_entry;
		}
	}

	/* Now retrieve the requested entries. */
	count = start_entry;
	while (num_entries > 0 && count <= end_entry) {
		memset(&entry, 0, sizeof(gn_phonebook_entry));
		entry.memory_type = memstat.memory_type;
		entry.location = count;

		data->phonebook_entry = &entry;
		error = gn_sm_functions(GN_OP_ReadPhonebook, data, state);

		switch (error) {
			int i;
		case GN_ERR_NONE:
			if (entry.empty != false)
				break;
			num_entries--;
			switch (type) {
			case 1:
				gn_file_phonebook_raw_write(stdout, &entry, memory_type_string);
				break;
			case 2: {
					char *s;

					s = gn_phonebook2vcardstr(&entry);
					fprintf (stdout, "%s", s);
					free (s);
				}
				break;
			case 3:
				gn_phonebook2ldif(stdout, &entry);
				break;
			default:
				fprintf(stdout, _("%d. %s: %s\n"), entry.location, _("Name"), entry.name);
				/* Print out personal information */
				if (entry.person.has_person) {
					if (entry.person.honorific_prefixes[0])
						fprintf(stdout, "%s ", entry.person.honorific_prefixes);
					if (entry.person.given_name[0])
						fprintf(stdout, "%s ", entry.person.given_name);
					if (entry.person.family_name[0])
						fprintf(stdout, "%s", entry.person.family_name);
					fprintf(stdout, "\n");
				}

				fprintf(stdout, _("%s: %s\n"), _("Caller group"), gn_phonebook_group_type2str(entry.caller_group));

				/* FIXME: AT driver doesn't set subentries */
				if (!entry.subentries_count && entry.number) {
					fprintf(stdout, _("%s: %s\n"), _("Number"), entry.number);
				}

				/* Print out address information */
				if (entry.address.has_address) {
					fprintf(stdout, _("Address information:\n"));
					if (entry.address.post_office_box[0])
						fprintf(stdout, _("  %s: %s\n"), _("Post office address"), entry.address.post_office_box);
					if (entry.address.extended_address[0])
						fprintf(stdout, _("  %s: %s\n"), _("Extended address"), entry.address.extended_address);
					if (entry.address.street[0])
						fprintf(stdout, _("  %s: %s\n"), _("Street"), entry.address.street);
					if (entry.address.city[0])
						fprintf(stdout, _("  %s: %s\n"), _("City"), entry.address.city);
					if (entry.address.state_province[0])
						fprintf(stdout, _("  %s: %s\n"), _("State or province"), entry.address.state_province);
					if (entry.address.zipcode[0])
						fprintf(stdout, _("  %s: %s\n"), _("Zip code"), entry.address.zipcode);
					if (entry.address.country[0])
						fprintf(stdout, _("  %s: %s\n"), _("Country"), entry.address.country);
				}
				dprintf("subentries count: %d\n", entry.subentries_count);
				for (i = 0; i < entry.subentries_count; i++) {
					fprintf(stdout, "%s: ", gn_subentrytype2string(entry.subentries[i].entry_type, entry.subentries[i].number_type));
					switch (entry.subentries[i].entry_type) {
					case GN_PHONEBOOK_ENTRY_ExtGroup:
					        fprintf(stdout, "%d", entry.subentries[i].data.id);
					        break;
					case GN_PHONEBOOK_ENTRY_Birthday:
					case GN_PHONEBOOK_ENTRY_Date:
						fprintf(stdout, _("%04u.%02u.%02u %02u:%02u:%02u"), entry.subentries[i].data.date.year, entry.subentries[i].data.date.month, entry.subentries[i].data.date.day, entry.subentries[i].data.date.hour, entry.subentries[i].data.date.minute, entry.subentries[i].data.date.second);
						break;
					case GN_PHONEBOOK_ENTRY_Image:
						break;
					default:
						fprintf(stdout, "%s", entry.subentries[i].data.number);
						break;
					}
					fprintf(stdout, "\n");
				}
				if ((entry.memory_type == GN_MT_MC ||
				     entry.memory_type == GN_MT_DC ||
				     entry.memory_type == GN_MT_RC) &&
				    entry.date.year)
					fprintf(stdout, _("Date: %04u.%02u.%02u %02u:%02u:%02u\n"), entry.date.year, entry.date.month, entry.date.day, entry.date.hour, entry.date.minute, entry.date.second);
				break;
			}
			break;
		case GN_ERR_EMPTYLOCATION:
			fprintf(stderr, _("Empty memory location. Skipping.\n"));
			break;
		default:
			fprintf(stderr, _("Error reading from the location %d in memory %s\n"), count, memory_type_string);
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			/* some older phones might return GN_ERR_INVALIDLOCATION for lower numbered locations
			so make it non fatal when a numeric end location was given */
			if (!((error == GN_ERR_INVALIDLOCATION) && explicit_end)) {
				return error;
			}
		}
		count++;
	}
	/* ignore non fatal errors that might have occurred above */
	return GN_ERR_NONE;
}

/* Displays usage of --getphonebook command */
int writephonebook_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage:  --writephonebook [[-o|--overwrite]|[-f|--find-free]]\n"
		     "                 [-m|--memory-type|--memory memory_type]\n"
		     "                 [-n|--memory-location|--location number]\n"
		     "                 [[-v|--vcard]|[-l|--ldif]]\n"
		     "\n"
		));
	return exitval;
}

/* Read data from stdin, parse and write to phone.  The parsing is relatively
   crude and doesn't allow for much variation from the stipulated format. */
/* FIXME: I guess there's *very* similar code in xgnokii */
gn_error writephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_phonebook_entry entry;
	gn_error error = GN_ERR_NONE;
	gn_memory_type default_mt = GN_MT_ME; /* Default memory_type. Changed if given in the command line */
	int default_location = 1; /* default location. Changed if given in the command line */
	int find_free = 0; /* By default don't try to find a free location */
	int confirm = 0; /* By default don't overwrite existing entries */
	int type = 0; /* type of the output:
				0 - CSV (default)
				1 - vCard
				2 - LDIF
			*/
	char *line, oline[MAX_INPUT_LINE_LEN];
	int i;

	struct option options[] = {
		{ "overwrite",		0,			NULL, 'o'},
		{ "vcard",		0,			NULL, 'v'},
		{ "ldif",		0,			NULL, 'l'},
		{ "find-free",		0,			NULL, 'f'},
		{ "memory-type",	required_argument,	NULL, 'm'},
		{ "memory",		required_argument,	NULL, 'm'},
		{ "memory-location",	required_argument,	NULL, 'n'},
		{ "location",		required_argument,	NULL, 'n'},
		{ NULL,			0,			NULL, 0}
	};

	/* Option parsing */
	while ((i = getopt_long(argc, argv, "ovlfm:n:", options, NULL)) != -1) {
		switch (i) {
		case 'o':
			confirm = 1;
			break;
		case 'v':
			if (type)
				return writephonebook_usage(stderr, -1);
			type = 1;
			break;
		case 'l':
			if (type)
				return writephonebook_usage(stderr, -1);
			type = 2;
			break;
		case 'f':
			find_free = 1;
			break;
		case 'm':
			default_mt = gn_str2memory_type(optarg);
			break;
		case 'n':
			default_location = gnokii_atoi(optarg);
			if (errno || default_location < 0)
				return writephonebook_usage(stderr, -1);
			break;
		default:
			return writephonebook_usage(stderr, -1);
		}
	}
	if (argc > optind) {
		/* There are too many arguments that don't start with '-' */
		return writephonebook_usage(stderr, -1);
	}

	line = oline;

	/* Go through data from stdin. */
	while (!feof(stdin)) {
		error = GN_ERR_NONE;

		memset(&entry, 0, sizeof(gn_phonebook_entry));
		entry.memory_type = default_mt;
		entry.location = default_location;
		switch (type) {
		case 1:
			if (gn_vcard2phonebook(stdin, &entry))
				error = GN_ERR_WRONGDATAFORMAT;
			break;
		case 2:
			if (gn_ldif2phonebook(stdin, &entry))
				error = GN_ERR_WRONGDATAFORMAT;
			break;
		default:
			if (!gn_line_get(stdin, line, MAX_INPUT_LINE_LEN))
				goto out; /* it means we read an empty line, but that's not an error */
			else
				error = gn_file_phonebook_raw_parse(&entry, oline);
			break;
		}

		if (error != GN_ERR_NONE)
			goto out;

		if (find_free) {
#if 0
			error = gn_sm_functions(GN_OP_FindFreePhonebookEntry, data, state);
			if (error == GN_ERR_NOTIMPLEMENTED) {
#endif
			for (i = 1; ; i++) {
				gn_phonebook_entry aux;

				memcpy(&aux, &entry, sizeof(gn_phonebook_entry));
				data->phonebook_entry = &aux;
				data->phonebook_entry->location = i;
				error = gn_sm_functions(GN_OP_ReadPhonebook, data, state);
				if (error != GN_ERR_NONE && error != GN_ERR_EMPTYLOCATION) {
					break;
				}
				if (aux.empty || error == GN_ERR_EMPTYLOCATION) {
					entry.location = aux.location;
					error = GN_ERR_NONE;
					break;
				}
			}
#if 0
			}
#endif
			if (error != GN_ERR_NONE)
				goto out;
		}

		if (!confirm) {
			gn_phonebook_entry aux;

			memcpy(&aux, &entry, sizeof(gn_phonebook_entry));
			data->phonebook_entry = &aux;
			error = gn_sm_functions(GN_OP_ReadPhonebook, data, state);

			if (error == GN_ERR_NONE || error == GN_ERR_EMPTYLOCATION) {
				if (!aux.empty && error != GN_ERR_EMPTYLOCATION) {
					char ans[8] = "";

					fprintf(stdout, _("Location busy. "));
					confirm = -1;
					while (confirm < 0) {
						fprintf(stdout, _("Overwrite? (yes/no) "));
						gn_line_get(stdin, ans, 7);
						if (!strcmp(ans, _("yes")))
							confirm = 1;
						else if (!strcmp(ans, _("no")))
							confirm = 0;
						else {
							fprintf(stdout, _("\nIncorrect answer [%s]. Assuming 'no'.\n"), ans);
							confirm = 0;
						}
					}
					/* User chose not to overwrite */
					if (!confirm) continue;
					confirm = 0;
				}
			} else {
				goto out;
			}
		}

		/* Do write and report success/failure. */
		gn_phonebook_entry_sanitize(&entry);
		data->phonebook_entry = &entry;
		error = gn_sm_functions(GN_OP_WritePhonebook, data, state);

		if (error == GN_ERR_NONE) {
			fprintf(stderr, 
				 _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), 
				 gn_memory_type2str(entry.memory_type), entry.location, entry.name, entry.number);
			/* If the location was not specified and there are
			 * multiple entries, don't write them to the same
			 * location */
			default_location++;
		} else
			fprintf(stderr, _("Write failed (%s): memory type: %s, loc: %d, name: %s, number: %s\n"),
				 gn_error_print(error), gn_memory_type2str(entry.memory_type), entry.location, entry.name, entry.number);
	}
out:
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Write failed (%s): memory type: %s, loc: %d, name: %s, number: %s\n"),
			gn_error_print(error), gn_memory_type2str(entry.memory_type), entry.location, entry.name, entry.number);
	return error;
}

/* Displays usage of --deletephonebook command */
int deletephonebook_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --deletephonebook memory_type start_number [end_number|end]\n"
			"\n"
		));
	return exitval;
}

/* Delete phonebook entry */
gn_error deletephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_phonebook_entry entry;
	gn_error error;
	char *memory_type_string;
	int i, first_location, last_location;

	if (argc < 3)
		return deletephonebook_usage(stderr, -1);

	/* Handle command line args that set memory type and location. */
	memory_type_string = optarg;
	entry.memory_type = gn_str2memory_type(memory_type_string);
	if (entry.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), optarg);
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	first_location = gnokii_atoi(argv[optind]);
	if (errno || first_location < 0)
		return deletephonebook_usage(stderr, -1);
	last_location = parse_end_value_option(argc, argv, optind + 1, first_location);
	if (errno || last_location < 0)
		return deletephonebook_usage(stderr, -1);

	for (i = first_location; i <= last_location; i++) {
		entry.location = i;
		entry.empty = true;
		data->phonebook_entry = &entry;
		error = gn_sm_functions(GN_OP_DeletePhonebook, data, state);
		switch (error) {
		case GN_ERR_NONE:
			fprintf (stderr, _("Phonebook entry removed: memory type: %s, loc: %d\n"), 
				 gn_memory_type2str(entry.memory_type), entry.location);
			break;
		default:
			if (last_location == INT_MAX)
				last_location = 0;
			else
				fprintf (stderr, _("Phonebook entry removal FAILED (%s): memory type: %s, loc: %d\n"), 
					 gn_error_print(error), gn_memory_type2str(entry.memory_type), entry.location);
			break;
		}
	}
	return GN_ERR_NONE;
}
