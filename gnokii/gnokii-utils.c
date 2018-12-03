/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 1999-2000  Gary Reuter, Reinhold Jordan
  Copyright (C) 1999-2011  Pawel Kot
  Copyright (C) 2000-2002  Marcin Wiacek, Chris Kemp, Manfred Jonsson
  Copyright (C) 2001       Marian Jancar, Bartek Klepacz
  Copyright (C) 2001-2002  Pavel Machek, Markus Plail
  Copyright (C) 2002       Ladis Michl, Simon Huggins
  Copyright (C) 2002-2004  BORBELY Zoltan
  Copyright (C) 2003       Bertrik Sikken
  Copyright (C) 2004       Martin Goldhahn

  Mainline code for gnokii utility. Utils functions.

*/

#include "compat.h"
#include "misc.h"

#include <stdio.h>
#include <signal.h>
#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#include "gnokii-app.h"
#include "gnokii.h"

gnokii_app_mode askoverwrite(const char *filename, gnokii_app_mode mode)
{
	int confirm = -1;
	char ans[5];
	struct stat buf;

	/* Ask before overwriting */
	if ((mode == GNOKII_APP_MODE_Ask) && (stat(filename, &buf) == 0)) {
		fprintf(stdout, _("File %s exists.\n"), filename);
		while (confirm < 0) {
			fprintf(stdout, _("Overwrite? (yes/no) "));
			gn_line_get(stdin, ans, sizeof(ans) - 1);
			if (!strcmp(ans, _("yes")))
				confirm = 1;
			else if (!strcmp(ans, _("no")))
				confirm = 0;
		}
		if (!confirm)
			return GNOKII_APP_MODE_Cancel;
	}
	return mode;
}

/* FIXME - this should not ask for confirmation here - I'm not sure what calls it though */
gnokii_app_mode writefile(char *filename, char *text, gnokii_app_mode mode)
{
	FILE *file;

	mode = askoverwrite(filename, mode);

	/* append */
	if (mode == GNOKII_APP_MODE_Append)
		file = fopen(filename, "a");
	/* overwrite */
	else
		file = fopen(filename, "w");

	if (!file) {
		fprintf(stderr, _("Can't open file %s for writing!\n"),  filename);
		return GNOKII_APP_MODE_Cancel;
	}
	fprintf(file, "%s\n", text);
	fclose(file);
	/* Return value is used as new mode. Set it to append mode */
	return GNOKII_APP_MODE_Append;
}

gnokii_app_mode writebuffer(const char *filename, const char *buffer, size_t nitems, gnokii_app_mode mode)
{
	/* Return value is used as new mode. Set it to append mode */
	gnokii_app_mode retval = GNOKII_APP_MODE_Append;
	FILE *file;

	mode = askoverwrite(filename, mode);

	/* append */
	if (mode == GNOKII_APP_MODE_Append)
		file = fopen(filename, "ab");
	/* overwrite */
	else
		file = fopen(filename, "wb");

	if (!file) {
		fprintf(stderr, _("Can't open file %s for writing!\n"),  filename);
		retval = GNOKII_APP_MODE_Cancel;
		goto out;
	}
	if (fwrite(buffer, 1, nitems, file) != nitems)
		retval = GNOKII_APP_MODE_Cancel;
	fclose(file);
out:
	return retval;
}

gn_error readtext(gn_sms_user_data *udata)
{
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[255 * GN_SMS_MAX_LENGTH];
	size_t chars_read;

#ifndef	WIN32
	if (isatty(0))
#endif
		fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:\n"));

	/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);
	if (chars_read == 0) {
		if (ferror(stdin)) {
			fprintf(stderr, _("Couldn't read from stdin!\n"));
			return GN_ERR_FAILED;
		}
	} else {
		if (udata->type != GN_SMS_DATA_iMelody && message_buffer[chars_read - 1] == '\n')
			chars_read--;
		if (chars_read > (sizeof(udata->u.text) - 1)) {
	 		fprintf(stderr, _("Input too long! (%d, maximum is %d)\n"), chars_read, (int)(sizeof(udata->u.text) - 1));
			return GN_ERR_ENTRYTOOLONG;
		}
	}
	message_buffer[chars_read] = '\0';

	udata->length = snprintf(udata->u.text, sizeof(udata->u.text), "%s", message_buffer);

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

