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

  Mainline code for gnokii utility. WAP handling functions.

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

void wap_usage(FILE *f)
{
	fprintf(f, _("WAP options:\n"
		     "          --getwapbookmark number\n"
		     "          --writewapbookmark name URL\n"
		     "          --deletewapbookmark number\n"
		     "          --getwapsetting number [-r|--raw]\n"
		     "          --writewapsetting\n"
		     "          --activatewapsetting number\n"
		     ));
}

int getwapbookmark_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getwapbookmark number\n"));
	return exitval;
}

/* Getting WAP bookmarks. */
gn_error getwapbookmark(char *number, gn_data *data, struct gn_statemachine *state)
{
	gn_wap_bookmark	wapbookmark;
	gn_error	error;

	wapbookmark.location = gnokii_atoi(number);
	if (errno || wapbookmark.location < 0)
		return getwapbookmark_usage(stderr, -1);

	gn_data_clear(data);
	data->wap_bookmark = &wapbookmark;

	error = gn_sm_functions(GN_OP_GetWAPBookmark, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("WAP bookmark nr. %d:\n"), wapbookmark.location);
		fprintf(stdout, _("%s: %s\n"), _("Name"), wapbookmark.name);
		fprintf(stdout, _("%s: %s\n"), _("URL"), wapbookmark.URL);

		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

int writewapbookmark_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --writebookmark name URL\n"));
	return exitval;
}

/* Writing WAP bookmarks. */
gn_error writewapbookmark(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_wap_bookmark	wapbookmark;
	gn_error	error;

	gn_data_clear(data);
	data->wap_bookmark = &wapbookmark;

	if (argc != optind + 1)
		return writewapbookmark_usage(stderr, -1);

	snprintf(&wapbookmark.name[0], WAP_NAME_MAX_LENGTH, "%s", optarg);
	snprintf(&wapbookmark.URL[0], WAP_URL_MAX_LENGTH, "%s", argv[optind]);

	error = gn_sm_functions(GN_OP_WriteWAPBookmark, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP bookmark nr. %d successfully written!\n"), wapbookmark.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

int deletewapbookmark_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --deletewapbookmark number\n"));
	return exitval;
}

/* Deleting WAP bookmarks. */
gn_error deletewapbookmark(char *number, gn_data *data, struct gn_statemachine *state)
{
	gn_wap_bookmark	wapbookmark;
	gn_error	error;

	wapbookmark.location = gnokii_atoi(number);
	if (errno || wapbookmark.location < 0)
		return deletewapbookmark_usage(stderr, -1);

	gn_data_clear(data);
	data->wap_bookmark = &wapbookmark;

	error = gn_sm_functions(GN_OP_DeleteWAPBookmark, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP bookmark nr. %d deleted!\n"), wapbookmark.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

int getwapsetting_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getwapsetting number [-r|--raw]\n"));
	return exitval;
}

/* Getting WAP settings. */
gn_error getwapsetting(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_wap_setting	wapsetting;
	gn_error	error;
	bool		raw = false;
	int i;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	memset(&wapsetting, 0, sizeof(wapsetting));
	wapsetting.location = gnokii_atoi(optarg);
	if (errno || wapsetting.location < 0)
		return getwapsetting_usage(stderr, -1);

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			return getwapsetting_usage(stderr, -1);
		}
	}

	gn_data_clear(data);
	data->wap_setting = &wapsetting;

	error = gn_sm_functions(GN_OP_GetWAPSetting, data, state);

	switch (error) {
	case GN_ERR_NONE:
		if (raw) {
			fprintf(stdout, ("%i;%s;%s;%i;%i;%i;%i;%i;%i;%i;%s;%s;%s;%s;%i;%i;%i;%s;%s;%s;%s;%s;%s;\n"),
				wapsetting.location, wapsetting.name, wapsetting.home, wapsetting.session,
				wapsetting.security, wapsetting.bearer, wapsetting.gsm_data_authentication,
				wapsetting.call_type, wapsetting.call_speed, wapsetting.gsm_data_login, wapsetting.gsm_data_ip,
				wapsetting.number, wapsetting.gsm_data_username, wapsetting.gsm_data_password,
				wapsetting.gprs_connection, wapsetting.gprs_authentication, wapsetting.gprs_login,
				wapsetting.access_point_name, wapsetting.gprs_ip,  wapsetting.gprs_username, wapsetting.gprs_password,
				wapsetting.sms_service_number, wapsetting.sms_server_number);
		} else {
			fprintf(stdout, _("WAP bookmark nr. %d:\n"), wapsetting.location);
			fprintf(stdout, _("%s: %s\n"), _("Name"), wapsetting.name);
			fprintf(stdout, _("%s: %s\n"), _("Home"), wapsetting.home);
			fprintf(stdout, _("%s: %s\n"), _("Session mode"), gn_wap_session2str(wapsetting.session));
			fprintf(stdout, _("%s: %s\n"), _("Connection security"), (wapsetting.security ? _("yes") : _("no")));
			fprintf(stdout, _("%s: %s\n"), _("Data bearer"), gn_wap_bearer2str(wapsetting.bearer));
			fprintf(stdout, "%s\n", _("GSM data"));
			fprintf(stdout, _("   %s: %s\n"), _("Authentication type"), gn_wap_authentication2str(wapsetting.security));
			fprintf(stdout, _("   %s: %s\n"), _("Data call type"), gn_wap_call_type2str(wapsetting.call_type));
			fprintf(stdout, _("   %s: %s\n"), _("Data call speed"), gn_wap_call_speed2str(wapsetting.call_speed));
			fprintf(stdout, _("   %s: %s\n"), _("Login type"), gn_wap_login2str(wapsetting.gsm_data_login));
			fprintf(stdout, _("   %s: %s\n"), _("IP"), wapsetting.gsm_data_ip);
			fprintf(stdout, _("   %s: %s\n"), _("Number"), wapsetting.number);
			fprintf(stdout, _("   %s: %s\n"), _("Username"), wapsetting.gsm_data_username);
			fprintf(stdout, _("   %s: %s\n"), _("Password"), wapsetting.gsm_data_password);
			fprintf(stdout, "%s\n", _("GPRS"));
			fprintf(stdout, _("   %s: %s\n"), _("Connection"), gn_wap_gprs2str(wapsetting.gprs_connection));
			fprintf(stdout, _("   %s: %s\n"), _("Authentication type"), gn_wap_authentication2str(wapsetting.gprs_authentication));
			fprintf(stdout, _("   %s: %s\n"), _("Login type"), gn_wap_login2str(wapsetting.gprs_login));
			fprintf(stdout, _("   %s: %s\n"), _("Access point"), wapsetting.access_point_name);
			fprintf(stdout, _("   %s: %s\n"), _("IP"), wapsetting.gprs_ip);
			fprintf(stdout, _("   %s: %s\n"), _("Username"), wapsetting.gprs_username);
			fprintf(stdout, _("   %s: %s\n"), _("Password"), wapsetting.gprs_password);
			fprintf(stdout, "%s\n", _("SMS"));
			fprintf(stdout, _("   %s: %s\n"), _("Service number"), wapsetting.sms_service_number);
			fprintf(stdout, _("   %s: %s\n"), _("Server number"), wapsetting.sms_server_number);
		}
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Writes WAP settings to phone */
gn_error writewapsetting(gn_data *data, struct gn_statemachine *state)
{
	int n;
	gn_wap_setting wapsetting;
	gn_error error = GN_ERR_NONE;
	char line[1000];

	gn_data_clear(data);
	data->wap_setting = &wapsetting;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}
		
		n = sscanf(line, "%d;%50[^;];%256[^;];%d;%d;%d;%d;%d;%d;%d;%50[^;];%50[^;];%32[^;];%20[^;];%d;%d;%d;%100[^;];%20[^;];%32[^;];%20[^;];%20[^;];%20[^;];", 
		/*
		n = sscanf(line, "%d;%s;%s;%d;%d;%d;%d;%d;%d;%d;%s;%s;%s;%s;%d;%d;%d;%s;%s;%s;%s;%s;%s;", 
		*/
			   &wapsetting.location, wapsetting.name, wapsetting.home, (int*)&wapsetting.session, (int*)&wapsetting.security,
			   (int*)&wapsetting.bearer, (int*)&wapsetting.gsm_data_authentication, (int*)&wapsetting.call_type,
			   (int*)&wapsetting.call_speed, (int*)&wapsetting.gsm_data_login, wapsetting.gsm_data_ip,
			   wapsetting.number, wapsetting.gsm_data_username, wapsetting.gsm_data_password, 
			   (int*)&wapsetting.gprs_connection, (int*)&wapsetting.gprs_authentication, (int*)&wapsetting.gprs_login, 
			   wapsetting.access_point_name, wapsetting.gprs_ip,  wapsetting.gprs_username, wapsetting.gprs_password,
			   wapsetting.sms_service_number, wapsetting.sms_server_number);

		if (n != 23) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			dprintf("n: %i\n", n);
			return GN_ERR_WRONGDATAFORMAT;
		}

		error = gn_sm_functions(GN_OP_WriteWAPSetting, data, state);
		if (error != GN_ERR_NONE) 
			fprintf(stderr, _("Cannot write WAP setting: %s\n"), gn_error_print(error));
	}
	return error;
}

int activatewapsetting_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --activatewapsetting number\n"));
	return exitval;
}

/* Deleting WAP bookmarks. */
gn_error activatewapsetting(char *number, gn_data *data, struct gn_statemachine *state)
{
	gn_wap_setting	wapsetting;
	gn_error	error;

	wapsetting.location = gnokii_atoi(number);
	if (errno || wapsetting.location < 0)
		return activatewapsetting_usage(stderr, -1);

	gn_data_clear(data);
	data->wap_setting = &wapsetting;

	error = gn_sm_functions(GN_OP_ActivateWAPSetting, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP setting nr. %d activated!\n"), wapsetting.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

