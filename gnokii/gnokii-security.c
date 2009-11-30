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

  Mainline code for gnokii utility. Security setting handling functions.

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

void security_usage(FILE *f)
{
	fprintf(f, _(
		     "Security options:\n"
		     "          --identify\n"
		     "          --getlocksinfo\n"
		));
#ifdef SECURITY
	fprintf(f, _(
		     "          --entersecuritycode PIN|PIN2|PUK|PUK2|SEC [COMMAND]\n"
		     "          --getsecuritycodestatus\n"
		     "          --getsecuritycode\n"
		     "          --changesecuritycode PIN|PIN2|PUK|PUK2\n"
		));
#endif
}

gn_error identify(struct gn_statemachine *state)
{
	fprintf(stdout, _("IMEI         : %s\n"), gn_lib_get_phone_imei(state));
	fprintf(stdout, _("Manufacturer : %s\n"), gn_lib_get_phone_manufacturer(state));
	fprintf(stdout, _("Model        : %s\n"), gn_lib_get_phone_model(state));
	fprintf(stdout, _("Product name : %s\n"), gn_lib_get_phone_product_name(state));
	fprintf(stdout, _("Revision     : %s\n"), gn_lib_get_phone_revision(state));

	return gn_lib_lasterror(state);
}

gn_error getlocksinfo(gn_data *data, struct gn_statemachine *state)
{
	gn_locks_info locks_info[4];
	gn_error error;
	char *locks_names[] = {"MCC+MNC", "GID1", "GID2", "MSIN"};
	int i;

	gn_data_clear(data);
	data->locks_info = locks_info;

	if ((error = gn_sm_functions(GN_OP_GetLocksInfo, data, state)) != GN_ERR_NONE) {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return error;
	}

	for (i = 0; i < 4; i++) {
		fprintf(stdout, _("%7s : %10s, %7s, %6s, counter %d\n"),
			locks_names[i],
			locks_info[i].data,
			locks_info[i].userlock ? "user" : "factory",
			locks_info[i].closed ? "CLOSED" : "open",
			locks_info[i].counter);
	}
	return GN_ERR_NONE;
}

#ifdef SECURITY

gn_error getsecuritycode(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_security_code sc;

	memset(&sc, 0, sizeof(sc));
	sc.type = GN_SCT_SecurityCode;
	data->security_code = &sc;
	fprintf(stderr, _("Getting security code... \n"));
	error = gn_sm_functions(GN_OP_GetSecurityCode, data, state);
	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Security code is: %s\n"), sc.code);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}
	return error;
}

static int get_password(const char *prompt, char *pass, int length)
{
	int fd = fileno(stdin);
	int count;

	if (isatty(fd)) {
#ifdef HAVE_GETPASS
		const char *s;

		/* FIXME: man getpass says it was removed in POSIX.1-2001 */
		s = getpass(prompt);
		if (s) {
			strncpy(pass, s, length - 1);
			pass[length - 1] = '\0';

			return strlen(pass);
		}
		return -1;
#else
		fprintf(stdout, "%s", prompt);
#endif
	}
	if (fgets(pass, length, stdin)) {
		/* Strip trailing newline like getpass() would do */
		for (count = 0; pass[count] && pass[count] != '\n'; count++)
			;
		pass[count] = '\0';

		return count;
	}
	return -1;
}

int entersecuritycode_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --entersecuritycode PIN|PIN2|PUK|PUK2|SEC\n"));
	return exitval;
}

/* In this mode we get the code from the keyboard and send it to the mobile
   phone. */
gn_error entersecuritycode(char *type, gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_security_code security_code;

	if (!strcmp(type, "PIN"))
		security_code.type = GN_SCT_Pin;
	else if (!strcmp(type, "PUK"))
		security_code.type = GN_SCT_Puk;
	else if (!strcmp(type, "PIN2"))
		security_code.type = GN_SCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		security_code.type = GN_SCT_Puk2;
	else if (!strcmp(type, "SEC"))
		security_code.type = GN_SCT_SecurityCode;
	else
		return entersecuritycode_usage(stderr, -1);

	memset(&security_code.code, 0, sizeof(security_code.code));
	get_password(_("Enter your code: "), security_code.code, sizeof(security_code.code));

	gn_data_clear(data);
	data->security_code = &security_code;

	error = gn_sm_functions(GN_OP_EnterSecurityCode, data, state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Code ok.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}
	return error;
}

gn_error getsecuritycodestatus(gn_data *data, struct gn_statemachine *state)
{
	gn_security_code security_code;
	gn_error err;

	gn_data_clear(data);
	data->security_code = &security_code;

	err = gn_sm_functions(GN_OP_GetSecurityCodeStatus, data, state);
	if (err == GN_ERR_NONE) {
		fprintf(stdout, _("Security code status: "));

		switch(security_code.type) {
		case GN_SCT_SecurityCode:
			fprintf(stdout, _("waiting for Security Code.\n"));
			break;
		case GN_SCT_Pin:
			fprintf(stdout, _("waiting for PIN.\n"));
			break;
		case GN_SCT_Pin2:
			fprintf(stdout, _("waiting for PIN2.\n"));
			break;
		case GN_SCT_Puk:
			fprintf(stdout, _("waiting for PUK.\n"));
			break;
		case GN_SCT_Puk2:
			fprintf(stdout, _("waiting for PUK2.\n"));
			break;
		case GN_SCT_None:
			fprintf(stdout, _("nothing to enter.\n"));
			break;
		default:
			fprintf(stdout, _("unknown\n"));
			break;
		}
	} else
		fprintf(stderr, _("Error: %s\n"), gn_error_print(err));

	return err;
}

int changesecuritycode_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --changesecuritycode PIN|PIN2|PUK|PUK2\n"));
	return exitval;
}

gn_error changesecuritycode(char *type, gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_security_code security_code;
	char newcode2[10];

	memset(&security_code, 0, sizeof(security_code));

	if (!strcmp(type, "PIN"))
		security_code.type = GN_SCT_Pin;
	else if (!strcmp(type, "PUK"))
		security_code.type = GN_SCT_Puk;
	else if (!strcmp(type, "PIN2"))
		security_code.type = GN_SCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		security_code.type = GN_SCT_Puk2;
	/* FIXME: Entering of security_code does not work :-(
	else if (!strcmp(type, "security_code"))
		security_code.type = GN_SCT_security_code;
	*/
	else
		return changesecuritycode_usage(stderr, -1);

	get_password(_("Enter your code: "), security_code.code, sizeof(security_code.code));
	get_password(_("Enter new code: "), security_code.new_code, sizeof(security_code.new_code));
	get_password(_("Retype new code: "), newcode2, sizeof(newcode2));
	if (strcmp(security_code.new_code, newcode2)) {
		fprintf(stderr, _("Error: new code differs\n"));
		return GN_ERR_FAILED;
	}

	gn_data_clear(data);
	data->security_code = &security_code;

	error = gn_sm_functions(GN_OP_ChangeSecurityCode, data, state);
	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Code changed.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

#endif
