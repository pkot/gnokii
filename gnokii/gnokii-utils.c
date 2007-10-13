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

  Mainline code for gnokii utility. Utils functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#include <signal.h>

#include "gnokii-app.h"
#include "gnokii.h"

gn_error readtext(gn_sms_user_data *udata, int input_len)
{
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[255 * GN_SMS_MAX_LENGTH];
	int chars_read;

#ifndef	WIN32
	if (isatty(0))
#endif
		fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:\n"));

	/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return GN_ERR_INTERNALERROR;
	}
	if (udata->type != GN_SMS_DATA_iMelody && chars_read > 0 && message_buffer[chars_read - 1] == '\n') 
		chars_read--;
	if (chars_read > input_len || chars_read > sizeof(udata->u.text) - 1) {
		fprintf(stderr, _("Input too long! (%d, maximum is %d)\n"), chars_read, input_len);
		return GN_ERR_INTERNALERROR;
	}

	/* No need to null terminate the source string because we use strncpy(). */
	strncpy(udata->u.text, message_buffer, chars_read);
	udata->u.text[chars_read] = 0;
	udata->length = chars_read;

	return GN_ERR_NONE;
}

gn_error loadbitmap(gn_bmp *bitmap, char *s, int type, struct gn_statemachine *state)
{
	gn_error error;
	bitmap->type = type;
	error = gn_bmp_null(bitmap, &state->driver.phone);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Could not null bitmap: %s\n"), gn_error_print(error));
		return error;
	}
	error = gn_file_bitmap_read(s, bitmap, &state->driver.phone);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Could not load bitmap from %s: %s\n"), s, gn_error_print(error));
		return error;
	}
	return GN_ERR_NONE;
}

/* 
 * Does almost the same as atoi().
 * Returns error in case when the string is not numerical or when strtol returns an error.
 * Modifies errno variable.
 */
int gnokii_atoi(char *string)
{
	char *aux = NULL;
	int num;

	errno = 0;
	num = strtol(string, &aux, 10);
	if (*aux)
		errno = ERANGE;
	return num;
}

/* Calculates end value from the command line of the form:
 * gnokii --something [...] start [end]
 * where end can be either number or 'end' string.
 * If end is less than start, it is set to start value.
 */
int parse_end_value_option(int argc, char *argv[], int pos, int start_value)
{
	int retval = start_value;

	if (argc > pos && (argv[pos][0] != '-')) {
		if (!strcasecmp(argv[pos], "end"))
			retval = INT_MAX;
		else
			retval = gnokii_atoi(argv[pos]);
	}
	if (retval < start_value) {
		fprintf(stderr, _("Warning: end value (%d) is less than start value (%d). Setting it to the start value (%d)\n"),
			retval, start_value, start_value);
		retval = start_value;
	}

	return retval;
}

volatile bool bshutdown = false;

/* SIGINT signal handler. */
void interrupted(int sig)
{
	signal(sig, SIG_IGN);
	bshutdown = true;
}

void console_raw(void)
{
#ifndef WIN32
	struct termios it;

	tcgetattr(fileno(stdin), &it);
	it.c_lflag &= ~(ICANON);
	//it.c_iflag &= ~(INPCK|ISTRIP|IXON);
	it.c_cc[VMIN] = 1;
	it.c_cc[VTIME] = 0;

	tcsetattr(fileno(stdin), TCSANOW, &it);
#endif
}
