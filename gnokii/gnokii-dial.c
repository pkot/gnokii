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

  Mainline code for gnokii utility. Dialling and call handling functions.

*/

#include "config.h"
#include "compat.h"

#include <stdio.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>

#include "gnokii-app.h"
#include "gnokii.h"

void dial_usage(FILE *f)
{
	fprintf(f, _("Dialling and call options:\n"
		     "          --getspeeddial location\n"
		     "          --setspeeddial number memory_type location\n"
		     "          --dialvoice number\n"
		     "          --senddtmf string\n"
		     "          --answercall callid\n"
		     "          --hangup callid\n"
		     "          --divert {--op|-o} {register|enable|query|disable|erasure}\n"
		     "                 {--type|-t} {all|busy|noans|outofreach|notavail}\n"
		     "                 {--call|-c} {all|voice|fax|data}\n"
		     "                 [{--timeout|-m} time_in_seconds]\n"
		     "                 [{--number|-n} number]\n"
		     "All number, location and callid options need to be numeric.\n"
		     ));
}

int getspeeddial_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --getspeeddial location\n"
			"                        location - location in the speed dial memory\n"
	));
	return exitval;
}

/* Getting speed dials. */
gn_error getspeeddial(char *number, gn_data *data, struct gn_statemachine *state)
{
	gn_speed_dial	speeddial;
	gn_error	error;

	speeddial.number = gnokii_atoi(number);
	if (errno || speeddial.number < 0)
		return getspeeddial_usage(stderr, -1);

	gn_data_clear(data);
	data->speed_dial = &speeddial;

	error = gn_sm_functions(GN_OP_GetSpeedDial, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("SpeedDial nr. %d: %d:%d\n"), speeddial.number, speeddial.memory_type, speeddial.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

int setspeeddial_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --setspeeddial number memory_type location\n"
			"                        number      - phone number to be stored\n"
			"                        memory_type - memory type for the speed dial\n"
			"                        location    - location in the speed dial memory\n"
	));
	return exitval;
}

/* Setting speed dials. */
gn_error setspeeddial(char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_speed_dial entry;
	gn_error error;
	char *memory_type_string;

	gn_data_clear(data);
	data->speed_dial = &entry;

	/* Handle command line args that set type, start and end locations. */
	if (strcmp(argv[optind], "ME") == 0) {
		entry.memory_type = GN_MT_ME;
		memory_type_string = "ME";
	} else if (strcmp(argv[optind], "SM") == 0) {
		entry.memory_type = GN_MT_SM;
		memory_type_string = "SM";
	} else {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[optind]);
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	entry.number = gnokii_atoi(optarg);
	if (errno || entry.number < 0)
		return setspeeddial_usage(stderr, -1);
	entry.location = gnokii_atoi(argv[optind+1]);
	if (errno || entry.location < 0)
		return setspeeddial_usage(stderr, -1);

	error = gn_sm_functions(GN_OP_SetSpeedDial, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Successfully written!\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Voice dialing mode. */
gn_error dialvoice(char *number, gn_data *data, struct gn_statemachine *state)
{
    	gn_call_info call_info;
	gn_error error;
	int call_id;

	memset(&call_info, 0, sizeof(call_info));
	snprintf(call_info.number, sizeof(call_info.number), "%s", number);
	call_info.type = GN_CALL_Voice;
	call_info.send_number = GN_CALL_Default;

	gn_data_clear(data);
	data->call_info = &call_info;

	if ((error = gn_call_dial(&call_id, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Dialing failed: %s\n"), gn_error_print(error));
	else
		fprintf(stdout, _("Dialled call, id: %d (lowlevel id: %d)\n"), call_id, call_info.call_id);

	return error;
}

gn_error senddtmf(char *string, gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	gn_data_clear(data);
	data->dtmf_string = string;

	error = gn_sm_functions(GN_OP_SendDTMF, data, state);

	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	}

	return error;
}

int answercall_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --answercall callid\n"
			"                        callid - call identifier\n"
	));
	return exitval;
}

/* Answering incoming call */
gn_error answercall(char *callid, gn_data *data, struct gn_statemachine *state)
{
    	gn_call_info callinfo;
	gn_error error;

	memset(&callinfo, 0, sizeof(callinfo));
	callinfo.call_id = gnokii_atoi(callid);
	if (errno || callinfo.call_id < 0)
		return answercall_usage(stderr, -1);

	gn_data_clear(data);
	data->call_info = &callinfo;

	error = gn_sm_functions(GN_OP_AnswerCall, data, state);
	
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	}
	
	return error;
}

int hangup_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: --hangup callid\n"
			"                        callid - call identifier\n"
	));
	return exitval;
}

/* Hangup the call */
gn_error hangup(char *callid, gn_data *data, struct gn_statemachine *state)
{
    	gn_call_info callinfo;
	gn_error error;

	memset(&callinfo, 0, sizeof(callinfo));
	callinfo.call_id = gnokii_atoi(callid);
	if (errno || callinfo.call_id < 0)
		return hangup_usage(stderr, -1);

	gn_data_clear(data);
	data->call_info = &callinfo;

	error = gn_sm_functions(GN_OP_CancelCall, data, state);

	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	}
	
	return error;
}

int divert_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --divert\n"
			"        -o operation\n"
			"        --op operation       operation to be executed, needs to be one\n"
			"                             of the following: register, enable, query,\n"
			"                             disable, erasure; this is required option\n"
			"        -t type\n"
			"        --type type          type of the operation, needs to be one of\n"
			"                             the following: all, busy, noans, outofreach,\n"
			"                             notavail, unconditional; this is required option\n"
			"        -c calltype\n"
			"        --call calltype      type of call for the operation, needs to be\n"
			"                             one of the following: all, voice, fax, data;\n"
			"                             this is optional\n"
			"        -m sec\n"
			"        --timeout sec        timeout to be set; the value is in seconds\n"
			"        -n msisdn\n"
			"        --number msisdn      number for redirection\n"
			"\n"
		));
	return exitval;
}

/* Options for --divert:
   --op, -o [register|enable|query|disable|erasure]  REQ
   --type, -t [all|busy|noans|outofreach|notavail]   REQ
   --call, -c [all|voice|fax|data]                   REQ
   --timeout, -m time_in_seconds                     OPT
   --number, -n number                               OPT
 */
gn_error divert(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	int opt;
	gn_call_divert cd;
	gn_error error;
	struct option options[] = {
		{ "op",      required_argument, NULL, 'o'},
		{ "type",    required_argument, NULL, 't'},
		{ "call",    required_argument, NULL, 'c'},
		{ "number",  required_argument, NULL, 'n'},
		{ "timeout", required_argument, NULL, 'm'},
		{ NULL,      0,                 NULL, 0}
	};

	memset(&cd, 0, sizeof(gn_call_divert));

	while ((opt = getopt_long(argc, argv, "o:t:n:c:m:", options, NULL)) != -1) {
		switch (opt) {
		case 'o':
			if (!strcmp("register", optarg)) {
				cd.operation = GN_CDV_Register;
			} else if (!strcmp("enable", optarg)) {
				cd.operation = GN_CDV_Enable;
			} else if (!strcmp("disable", optarg)) {
				cd.operation = GN_CDV_Disable;
			} else if (!strcmp("erasure", optarg)) {
				cd.operation = GN_CDV_Erasure;
			} else if (!strcmp("query", optarg)) {
				cd.operation = GN_CDV_Query;
			} else {
				return divert_usage(stderr, -1);
			}
			break;
		case 't':
			if (!strcmp("all", optarg)) {
				cd.type = GN_CDV_AllTypes;
			} else if (!strcmp("busy", optarg)) {
				cd.type = GN_CDV_Busy;
			} else if (!strcmp("noans", optarg)) {
				cd.type = GN_CDV_NoAnswer;
			} else if (!strcmp("outofreach", optarg)) {
				cd.type = GN_CDV_OutOfReach;
			} else if (!strcmp("notavail", optarg)) {
				cd.type = GN_CDV_NotAvailable;
			} else if (!strcmp("unconditional", optarg)) {
				cd.type = GN_CDV_Unconditional;
			} else {
				return divert_usage(stderr, -1);
			}
			break;
		case 'c':
			if (!strcmp("all", optarg)) {
				cd.ctype = GN_CDV_AllCalls;
			} else if (!strcmp("voice", optarg)) {
				cd.ctype = GN_CDV_VoiceCalls;
			} else if (!strcmp("fax", optarg)) {
				cd.ctype = GN_CDV_FaxCalls;
			} else if (!strcmp("data", optarg)) {
				cd.ctype = GN_CDV_DataCalls;
			} else {
				return divert_usage(stderr, -1);
			}
			break;
		case 'm':
			cd.timeout = gnokii_atoi(optarg);
			if (errno || cd.timeout < 0)
				return divert_usage(stderr, -1);
			break;
		case 'n':
			snprintf(cd.number.number, sizeof(cd.number.number), "%s", optarg);
			if (cd.number.number[0] == '+')
				cd.number.type = GN_GSM_NUMBER_International;
			else
				cd.number.type = GN_GSM_NUMBER_Unknown;
			break;
		default:
			return divert_usage(stderr, -1);
		}
	}
	if (argc > optind) {
		/* There are too many arguments that don't start with '-' */
		return divert_usage(stderr, -1);
	}

	data->call_divert = &cd;
	error = gn_sm_functions(GN_OP_CallDivert, data, state);

	if (error == GN_ERR_NONE) {
		fprintf(stdout, _("%s: %s\n"), _("Divert type"), gn_call_divert_type2str(cd.type));
		fprintf(stdout, _("%s: %s\n"), _("Call type"), gn_call_divert_call_type2str(cd.ctype));
		if (cd.number.number[0]) {
			fprintf(stdout, _("%s: %s\n"), _("Number"), cd.number.number);
			fprintf(stdout, _("%s: %d\n"), _("Timeout"), cd.timeout);
		} else
			fprintf(stdout, _("Divert isn't active.\n"));
	} else {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	}
	return error;
}
