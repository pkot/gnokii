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

  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries and other stuff.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif
#include <signal.h>
#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>	/* for memset */
#endif
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif
#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif
#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif


#ifdef WIN32

#  include <windows.h>
#  include <process.h>
#  include <getopt.h>

#else

#  include <unistd.h>
#  include <termios.h>
#  include <fcntl.h>
#  include <getopt.h>

#endif

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#endif

#include "gnokii-app.h"
#include "gnokii.h"

struct gnokii_arg_len {
	int gal_opt; /* option name (opt_index) */
	int gal_min; /* minimal number of arguments to give */
	int gal_max; /* maximal number of arguments to give */
	int gal_flags;
};

/* This is used for checking correct argument count. If it is used then if
   the user specifies some argument, their count should be equivalent to the
   count the programmer expects. */
#define GAL_XOR 0x01

typedef enum {
	OPT_HELP = 256,
	OPT_VERSION,
	OPT_MONITOR,
	OPT_ENTERSECURITYCODE,
	OPT_GETSECURITYCODESTATUS,	/* 260 */
	OPT_CHANGESECURITYCODE,
	OPT_SETDATETIME,
	OPT_GETDATETIME,
	OPT_SETALARM,
	OPT_GETALARM,
	OPT_DIALVOICE,
	OPT_ANSWERCALL,
	OPT_HANGUP,
	OPT_GETTODO,
	OPT_WRITETODO,			/* 270 */
	OPT_DELETEALLTODOS,
	OPT_GETCALENDARNOTE,
	OPT_WRITECALENDARNOTE,
	OPT_DELCALENDARNOTE,
	OPT_GETDISPLAYSTATUS,
	OPT_GETPHONEBOOK,
	OPT_WRITEPHONEBOOK,
	OPT_DELETEPHONEBOOK,
	OPT_GETSPEEDDIAL,
	OPT_SETSPEEDDIAL,		/* 280 */
	OPT_GETSMS,
	OPT_DELETESMS,
	OPT_SENDSMS,
	OPT_SAVESMS,
	OPT_SENDLOGO,
	OPT_SENDRINGTONE,
	OPT_GETSMSC,
	OPT_SETSMSC,
	OPT_GETWELCOMENOTE,
	OPT_SETWELCOMENOTE,		/* 290 */
	OPT_PMON,
	OPT_NETMONITOR,
	OPT_IDENTIFY,
	OPT_SENDDTMF,
	OPT_RESET,
	OPT_SETLOGO,
	OPT_GETLOGO,
	OPT_VIEWLOGO,
	OPT_GETRINGTONE,
	OPT_SETRINGTONE,		/* 300 */
	OPT_PLAYRINGTONE,
	OPT_RINGTONECONVERT,
	OPT_GETPROFILE,
	OPT_SETPROFILE,
	OPT_GETACTIVEPROFILE,
	OPT_SETACTIVEPROFILE,
	OPT_DISPLAYOUTPUT,
	OPT_KEYPRESS,
	OPT_ENTERCHAR,
	OPT_DIVERT,			/* 310 */
	OPT_SMSREADER,
	OPT_FOOGLE,
	OPT_GETSECURITYCODE,
	OPT_GETWAPBOOKMARK,
	OPT_WRITEWAPBOOKMARK,
	OPT_GETWAPSETTING,
	OPT_WRITEWAPSETTING,
	OPT_ACTIVATEWAPSETTING,
	OPT_DELETEWAPBOOKMARK,
	OPT_DELETESMSFOLDER,		/* 320 */
	OPT_CREATESMSFOLDER,
	OPT_LISTNETWORKS,
	OPT_GETNETWORKINFO,
	OPT_GETLOCKSINFO,
	OPT_GETRINGTONELIST,
	OPT_DELETERINGTONE,
	OPT_SHOWSMSFOLDERSTATUS,
	OPT_GETFILELIST,
	OPT_GETFILEDETAILSBYID,
	OPT_GETFILEID,			/* 330 */
	OPT_GETFILE,
	OPT_GETFILEBYID,
	OPT_GETALLFILES,
	OPT_PUTFILE,
	OPT_DELETEFILE,
	OPT_DELETEFILEBYID,
	OPT_CONFIGFILE,
	OPT_CONFIGMODEL,
	OPT_SHELL,
	OPT_GETMMS,			/* 340 */
	OPT_DELETEMMS
} opt_index;

static FILE *logfile = NULL;
static char *configfile = NULL;
static char *configmodel = NULL;

static void short_version(void)
{
	fprintf(stderr, _("GNOKII Version %s\n"), VERSION);
}

/* This function shows the copyright and some informations useful for
   debugging. */
static void version(void)
{
	fprintf(stderr, _("Copyright (C) Hugh Blemings <hugh@blemings.org>, 1999, 2000\n"
			  "Copyright (C) Pavel Janik ml. <Pavel.Janik@suse.cz>, 1999, 2000\n"
			  "Copyright (C) Pavel Machek <pavel@ucw.cz>, 2001\n"
			  "Copyright (C) Pawel Kot <gnokii@gmail.com>, 2001-2009\n"
			  "Copyright (C) BORBELY Zoltan <bozo@andrews.hu>, 2002\n"
			  "Copyright (C) Daniele Forsi <daniele@forsi.it>, 2004-2009\n"
			  "gnokii is free software, covered by the GNU General Public License, and you are\n"
			  "welcome to change it and/or distribute copies of it under certain conditions.\n"
			  "There is absolutely no warranty for gnokii.  See GPL for details.\n"
			  "Built %s %s\n"), __TIME__, __DATE__);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options. */
static int usage(FILE *f, int argc, char *argv[])
{
	int i;
	int all = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "all")) {
			all = 1;
			break;
		}
	}

	/* Don't print general options if specific help was requested */
	if (all || argc == -1)
		fprintf(f, _("Usage:\n"
			     "	gnokii [CONFIG OPTIONS] OPTIONS\n"
			     "\nCONFIG OPTIONS\n"
			     "		--config filename\n"
			     "		--phone phone_section\n"
			     "\nOPTIONS\n"
			     "Use just one option at a time.\n"
			     "General options:\n"
			     "          --help [all] [monitor] [sms] [mms] [phonebook] [calendar] [todo]\n"
			     "                 [dial] [profile] [settings] [wap] [logo] [ringtone]\n"
			     "                 [security] [file] [other]\n"
			     "          --version\n"
			     "          --shell\n"));

	/* FIXME? throw an error if argv[i] is duplicated or unknown */
	for (i = 1; i < argc; i++) {
		if (all || !strcmp(argv[i], "monitor"))
			monitor_usage(f);
		if (all || !strcmp(argv[i], "sms"))
			sms_usage(f);
		if (all || !strcmp(argv[i], "mms"))
			mms_usage(f);
		if (all || !strcmp(argv[i], "phonebook"))
			phonebook_usage(f);
		if (all || !strcmp(argv[i], "calendar"))
			calendar_usage(f);
		if (all || !strcmp(argv[i], "todo"))
			todo_usage(f);
		if (all || !strcmp(argv[i], "dial"))
			dial_usage(f);
		if (all || !strcmp(argv[i], "profile"))
			profile_usage(f);
		if (all || !strcmp(argv[i], "settings"))
			settings_usage(f);
		if (all || !strcmp(argv[i], "wap"))
			wap_usage(f);
		if (all || !strcmp(argv[i], "logo"))
			logo_usage(f);
		if (all || !strcmp(argv[i], "ringtone"))
			ringtone_usage(f);
		/* Unconditional compile here because security_usage() has its own conditional */
		if (all || !strcmp(argv[i], "security"))
			security_usage(f);
		if (all || !strcmp(argv[i], "file"))
			file_usage(f);
		if (all || !strcmp(argv[i], "other"))
			other_usage(f);
		if (all)
			break;
	}

	return 0;
}

static void gnokii_error_logger(const char *fmt, va_list ap)
{
	if (logfile) {
		vfprintf(logfile, fmt, ap);
		fflush(logfile);
	}
}

#define MAX_PATH_LEN 255
static int install_log_handler(void)
{
	int retval = 0;
	char logname[MAX_PATH_LEN];
	char *path, *basepath;
	char *file = "gnokii-errors";
	int free_path = 0;
#ifndef WIN32
	struct stat buf;
	int st;
	int home = 0;
#endif

#ifdef WIN32
	basepath = getenv("APPDATA");
#elif __MACH__
	basepath = getenv("HOME");
#else
/* freedesktop.org compliancy: http://standards.freedesktop.org/basedir-spec/latest/ar01s03.html */
#define XDG_CACHE_HOME "/.cache" /* $HOME/.cache */
	basepath = getenv("XDG_CACHE_HOME");
	if (!basepath) {
		basepath = getenv("HOME");
		home = 1;
	}
#endif
	if (!basepath)
		path = ".";
	else {
		path = calloc(MAX_PATH_LEN, sizeof(char));
		free_path = 1;
#ifdef WIN32
		snprintf(path, MAX_PATH_LEN, "%s\\gnokii", basepath);
#elif __MACH__
		snprintf(path, MAX_PATH_LEN, "%s/Library/Logs/gnokii", basepath);
#else
		if (home)
			snprintf(path, MAX_PATH_LEN, "%s%s/gnokii", basepath, XDG_CACHE_HOME);
		else
			snprintf(path, MAX_PATH_LEN, "%s/gnokii", basepath);
#endif
	}

#ifndef WIN32
	st = stat(path, &buf);
	if (st)
		mkdir(path, S_IRWXU);
#endif

	snprintf(logname, sizeof(logname), "%s/%s", path, file);

	if ((logfile = fopen(logname, "a")) == NULL) {
		fprintf(stderr, _("Cannot open logfile %s\n"), logname);
		retval = -1;
		goto out;
	}

	gn_elog_handler = gnokii_error_logger;

out:
	if (free_path)
		free(path);
	return retval;
}

/* businit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

static struct gn_statemachine *state;
static gn_data *data;
static void busterminate(void)
{
	gn_lib_phone_close(state);
	gn_lib_phoneprofile_free(&state);
	if (logfile)
		fclose(logfile);
	gn_lib_library_free();
}

static int businit(void)
{
	gn_error err;
	if ((err = gn_lib_phoneprofile_load_from_file(configfile, configmodel, &state)) != GN_ERR_NONE) {
		fprintf(stderr, "%s\n", gn_error_print(err));
		if (configfile)
			fprintf(stderr, _("File: %s\n"), configfile);
		if (configmodel)
			fprintf(stderr, _("Phone section: [phone_%s]\n"), configmodel);
		return 2;
	}

	/* register cleanup function */
	atexit(busterminate);
	/* signal(SIGINT, bussignal); */

	if (install_log_handler()) {
		fprintf(stderr, _("WARNING: cannot open logfile, logs will be directed to stderr\n"));
	}

	if ((err = gn_lib_phone_open(state)) != GN_ERR_NONE) {
		fprintf(stderr, "%s\n", gn_error_print(err));
		return 2;
	}
	data = &state->sm_data;
	return 0;
}

/* This function checks that the argument count for a given options is within
   an allowed range. */
static int checkargs(int opt, struct gnokii_arg_len gals[], int argc, int has_arg)
{
	int i;

	switch (has_arg) {
	case required_argument:
		has_arg = 1;
		break;
	default:
		has_arg = 0;
	}
	/* Walk through the whole array with options requiring arguments. */
	for (i = 0; gals[i].gal_opt != 0; i++) {

		/* Current option. */
		if (gals[i].gal_opt == opt) {

			/* Argument count checking. */
			if (gals[i].gal_flags == GAL_XOR) {
				if (gals[i].gal_min == argc - optind + has_arg ||
				    gals[i].gal_max == argc - optind + has_arg)
					return 0;
			} else {
				if (gals[i].gal_min <= argc - optind + has_arg &&
				    gals[i].gal_max >= argc - optind + has_arg)
					return 0;
			}
			return 1;
		}
	}

	/* We do not have options without arguments in the array, so check them. */
	if (argc-optind+has_arg == 0)
		return 0;
	else
		return 1;
}

/* This is a "convenience" function to allow quick test of new API stuff which
   doesn't warrant a "proper" command line function. */
#ifndef WIN32
static int foogle(int argc, char *argv[])
{
	/* Fill in what you would like to test here... */
	/* remember to call businit(); if you need to communicate with your phone */
	return GN_ERR_NONE;
}
#endif

static int parse_options(int argc, char *argv[])
{
	int c;
	int opt_index = -1;
	gn_error rc;

	/* Every option should be in this array. */
	static struct option long_options[] = {
		/* FIXME: these comments are nice, but they would be more useful as docs for the user */
		/* Display usage. */
		{ "help",               optional_argument, NULL, OPT_HELP },

		/* Display version and build information. */
		{ "version",            no_argument,       NULL, OPT_VERSION },

		/* Monitor mode */
		{ "monitor",            optional_argument, NULL, OPT_MONITOR },

		/* Alternate config file location */
		{ "config",             required_argument, NULL, OPT_CONFIGFILE },
		/* Alternate phone section from the config */
		{ "phone",              required_argument, NULL, OPT_CONFIGMODEL },

#ifdef SECURITY

		/* Get Security Code */
		{ "getsecuritycode",    no_argument,   	   NULL, OPT_GETSECURITYCODE },

		/* Enter Security Code mode */
		{ "entersecuritycode",  required_argument, NULL, OPT_ENTERSECURITYCODE },

		/* Get Security Code status */
		{ "getsecuritycodestatus",  no_argument,   NULL, OPT_GETSECURITYCODESTATUS },

		/* Change Security Code */
		{ "changesecuritycode", required_argument, NULL, OPT_CHANGESECURITYCODE },

#endif

		/* Set date and time */
		{ "setdatetime",        optional_argument, NULL, OPT_SETDATETIME },

		/* Get date and time mode */
		{ "getdatetime",        no_argument,       NULL, OPT_GETDATETIME },

		/* Set alarm */
		{ "setalarm",           optional_argument, NULL, OPT_SETALARM },

		/* Get alarm */
		{ "getalarm",           no_argument,       NULL, OPT_GETALARM },

		/* Voice call mode */
		{ "dialvoice",          required_argument, NULL, OPT_DIALVOICE },

		/* Answer the incoming call */
		{ "answercall",         required_argument, NULL, OPT_ANSWERCALL },

		/* Hangup call */
		{ "hangup",             required_argument, NULL, OPT_HANGUP },

		/* Get ToDo note mode */
		{ "gettodo",		required_argument, NULL, OPT_GETTODO },

		/* Write ToDo note mode */
		{ "writetodo",          required_argument, NULL, OPT_WRITETODO },

		/* Delete all ToDo notes mode */
		{ "deletealltodos",     no_argument,       NULL, OPT_DELETEALLTODOS },

		/* Get calendar note mode */
		{ "getcalendarnote",    required_argument, NULL, OPT_GETCALENDARNOTE },

		/* Write calendar note mode */
		{ "writecalendarnote",  required_argument, NULL, OPT_WRITECALENDARNOTE },

		/* Delete calendar note mode */
		{ "deletecalendarnote", required_argument, NULL, OPT_DELCALENDARNOTE },

		/* Get display status mode */
		{ "getdisplaystatus",   no_argument,       NULL, OPT_GETDISPLAYSTATUS },

		/* Get memory mode */
		{ "getphonebook",       required_argument, NULL, OPT_GETPHONEBOOK },

		/* Write phonebook (memory) mode */
		{ "writephonebook",     optional_argument, NULL, OPT_WRITEPHONEBOOK },

		/* Delete phonebook entry from memory mode */
		{ "deletephonebook",    required_argument, NULL, OPT_DELETEPHONEBOOK },

		/* Get speed dial mode */
		{ "getspeeddial",       required_argument, NULL, OPT_GETSPEEDDIAL },

		/* Set speed dial mode */
		{ "setspeeddial",       required_argument, NULL, OPT_SETSPEEDDIAL },

		/* Create SMS folder mode */
		{ "createsmsfolder",    required_argument, NULL, OPT_CREATESMSFOLDER },

		/* Delete SMS folder mode */
		{ "deletesmsfolder",    required_argument, NULL, OPT_DELETESMSFOLDER },

		/* Show SMS folder names and its attributes */
		{ "showsmsfolderstatus",no_argument,       NULL, OPT_SHOWSMSFOLDERSTATUS },

		/* Get SMS message mode */
		{ "getsms",             required_argument, NULL, OPT_GETSMS },

		/* Delete SMS message mode */
		{ "deletesms",          required_argument, NULL, OPT_DELETESMS },

		/* Send SMS message mode */
		{ "sendsms",            required_argument, NULL, OPT_SENDSMS },

		/* Ssve SMS message mode */
		{ "savesms",            optional_argument, NULL, OPT_SAVESMS },

		/* Send logo as SMS message mode */
		{ "sendlogo",           required_argument, NULL, OPT_SENDLOGO },

		/* Send ringtone as SMS message */
		{ "sendringtone",       required_argument, NULL, OPT_SENDRINGTONE },

		/* Get ringtone */
		{ "getringtone",        required_argument, NULL, OPT_GETRINGTONE },

		/* Set ringtone */
		{ "setringtone",        required_argument, NULL, OPT_SETRINGTONE },

		/* Play ringtone */
		{ "playringtone",       required_argument, NULL, OPT_PLAYRINGTONE },

		/* Convert ringtone */
		{ "ringtoneconvert",    required_argument, NULL, OPT_RINGTONECONVERT },

		/* Get list of the ringtones */
		{ "getringtonelist",    no_argument,       NULL, OPT_GETRINGTONELIST },

		/* Delete ringtones */
		{ "deleteringtone",     required_argument, NULL, OPT_DELETERINGTONE },

		/* Get SMS center number mode */
		{ "getsmsc",            optional_argument, NULL, OPT_GETSMSC },

		/* Set SMS center number mode */
		{ "setsmsc",            no_argument,       NULL, OPT_SETSMSC },

		/* For development purposes: run in passive monitoring mode */
		{ "pmon",               no_argument,       NULL, OPT_PMON },

		/* NetMonitor mode */
		{ "netmonitor",         required_argument, NULL, OPT_NETMONITOR },

		/* Identify */
		{ "identify",           no_argument,       NULL, OPT_IDENTIFY },

		/* Send DTMF sequence */
		{ "senddtmf",           required_argument, NULL, OPT_SENDDTMF },

		/* Resets the phone */
		{ "reset",              required_argument, NULL, OPT_RESET },

		/* Set logo */
		{ "setlogo",            required_argument, NULL, OPT_SETLOGO },

		/* Get logo */
		{ "getlogo",            required_argument, NULL, OPT_GETLOGO },

		/* View logo */
		{ "viewlogo",           required_argument, NULL, OPT_VIEWLOGO },

		/* Show profile */
		{ "getprofile",         optional_argument, NULL, OPT_GETPROFILE },

		/* Set profile */
		{ "setprofile",         no_argument,       NULL, OPT_SETPROFILE },

		/* Get the active profile */
		{ "getactiveprofile",   no_argument,       NULL, OPT_GETACTIVEPROFILE },

		/* Set the active profile */
		{ "setactiveprofile",   required_argument, NULL, OPT_SETACTIVEPROFILE },

		/* Show texts from phone's display */
		{ "displayoutput",      no_argument,       NULL, OPT_DISPLAYOUTPUT },

		/* Simulate pressing the keys */
		{ "keysequence",        no_argument,       NULL, OPT_KEYPRESS },

		/* Simulate pressing the keys */
		{ "enterchar",          no_argument,       NULL, OPT_ENTERCHAR },

		/* Divert calls */
		{ "divert",		no_argument,       NULL, OPT_DIVERT },

		/* SMS reader */
		{ "smsreader",          no_argument,       NULL, OPT_SMSREADER },

		/* For development purposes: insert you function calls here */
		{ "foogle",             no_argument,       NULL, OPT_FOOGLE },

		/* Get WAP bookmark */
		{ "getwapbookmark",     required_argument, NULL, OPT_GETWAPBOOKMARK },

		/* Write WAP bookmark */
		{ "writewapbookmark",   required_argument, NULL, OPT_WRITEWAPBOOKMARK },

		/* Delete WAP bookmark */
		{ "deletewapbookmark",  required_argument, NULL, OPT_DELETEWAPBOOKMARK },

		/* Get WAP setting */
		{ "getwapsetting",      required_argument, NULL, OPT_GETWAPSETTING },

		/* Write WAP setting */
		{ "writewapsetting",    no_argument, 	   NULL, OPT_WRITEWAPSETTING },

		/* Activate WAP setting */
		{ "activatewapsetting", required_argument, NULL, OPT_ACTIVATEWAPSETTING },

		/* List GSM networks */
		{ "listnetworks",       no_argument,       NULL, OPT_LISTNETWORKS },

		/* Get network info */
		{ "getnetworkinfo",     no_argument,       NULL, OPT_GETNETWORKINFO },

		/* Get (sim)lock info */
		{ "getlocksinfo",       no_argument,       NULL, OPT_GETLOCKSINFO },

                /* Get file list */
		{ "getfilelist",        required_argument, NULL, OPT_GETFILELIST },

                /* Get file details by id */
		{ "getfiledetailsbyid", optional_argument, NULL, OPT_GETFILEDETAILSBYID },

                /* Get file id */
		{ "getfileid",          required_argument, NULL, OPT_GETFILEID },

                /* Get file */
		{ "getfile",            required_argument, NULL, OPT_GETFILE },

                /* Get file by id */
		{ "getfilebyid",        required_argument, NULL, OPT_GETFILEBYID },

		/* Get all files */
		{ "getallfiles",        required_argument, NULL, OPT_GETALLFILES },

                /* Put a file */
		{ "putfile",            required_argument, NULL, OPT_PUTFILE },

		/* Delete a file */
		{ "deletefile",         required_argument, NULL, OPT_DELETEFILE },

		/* Delete a file by id */
		{ "deletefilebyid",     required_argument, NULL, OPT_DELETEFILEBYID },

		/* shell like interface */
		{ "shell",              no_argument, NULL, OPT_SHELL },

		/* Get MMS message mode */
		{ "getmms",             required_argument, NULL, OPT_GETMMS },

		/* Delete MMS message mode */
		{ "deletemms",          required_argument, NULL, OPT_DELETEMMS },

		{ 0, 0, 0, 0 },
	};

	/* Every command which requires arguments should have an appropriate entry
	   in this array. */
	static struct gnokii_arg_len gals[] = {
		{ OPT_HELP,              1, 100, 0 },
		{ OPT_CONFIGFILE,        1, 100, 0 },
		{ OPT_CONFIGMODEL,       1, 100, 0 },
#ifdef SECURITY
		{ OPT_ENTERSECURITYCODE, 1, 100, 0 },
		{ OPT_CHANGESECURITYCODE,1, 1, 0 },
#endif
		{ OPT_SETDATETIME,       0, 5, 0 },
		{ OPT_SETALARM,          0, 2, 0 },
		{ OPT_DIALVOICE,         1, 1, 0 },
		{ OPT_ANSWERCALL,        1, 1, 0 },
		{ OPT_HANGUP,            1, 1, 0 },
		{ OPT_GETTODO,           1, 3, 0 },
		{ OPT_WRITETODO,         2, 3, 0 },
		{ OPT_GETCALENDARNOTE,   1, 3, 0 },
		{ OPT_WRITECALENDARNOTE, 2, 3, 0 },
		{ OPT_DELCALENDARNOTE,   1, 2, 0 },
		{ OPT_GETPHONEBOOK,      2, 4, 0 },
		{ OPT_WRITEPHONEBOOK,    0, 10, 0 },
		{ OPT_DELETEPHONEBOOK,   2, 3, 0 },
		{ OPT_GETSPEEDDIAL,      1, 1, 0 },
		{ OPT_SETSPEEDDIAL,      3, 3, 0 },
		{ OPT_CREATESMSFOLDER,   1, 1, 0 },
		{ OPT_DELETESMSFOLDER,   1, 1, 0 },
		{ OPT_GETSMS,            2, 6, 0 },
		{ OPT_DELETESMS,         2, 3, 0 },
		{ OPT_SENDSMS,           1, 10, 0 },
		{ OPT_SAVESMS,           0, 12, 0 },
		{ OPT_SENDLOGO,          3, 4, GAL_XOR },
		{ OPT_SENDRINGTONE,      2, 2, 0 },
		{ OPT_GETSMSC,           0, 3, 0 },
		{ OPT_GETWELCOMENOTE,    1, 1, 0 },
		{ OPT_SETWELCOMENOTE,    1, 1, 0 },
		{ OPT_NETMONITOR,        1, 1, 0 },
		{ OPT_SENDDTMF,          1, 1, 0 },
		{ OPT_SETLOGO,           1, 4, 0 },
		{ OPT_GETLOGO,           1, 4, 0 },
		{ OPT_VIEWLOGO,          1, 1, 0 },
		{ OPT_GETRINGTONE,       1, 3, 0 },
		{ OPT_SETRINGTONE,       1, 5, 0 },
		{ OPT_PLAYRINGTONE,      1, 3, 0 },
		{ OPT_RINGTONECONVERT,   2, 2, 0 },
		{ OPT_DELETERINGTONE,    1, 2, 0 },
		{ OPT_RESET,             0, 1, 0 },
		{ OPT_GETPROFILE,        0, 3, 0 },
		{ OPT_SETACTIVEPROFILE,  1, 1, 0 },
		{ OPT_DIVERT,            6, 10, 0 },
		{ OPT_GETWAPBOOKMARK,    1, 1, 0 },
		{ OPT_WRITEWAPBOOKMARK,  2, 2, 0 },
		{ OPT_DELETEWAPBOOKMARK, 1, 1, 0 },
		{ OPT_GETWAPSETTING,     1, 2, 0 },
		{ OPT_ACTIVATEWAPSETTING,1, 1, 0 },
		{ OPT_MONITOR,           0, 1, 0 },
		{ OPT_GETFILELIST,       1, 1, 0 },
		{ OPT_GETFILEDETAILSBYID,0, 1, 0 },
		{ OPT_GETFILEID,         1, 1, 0 },
		{ OPT_GETFILE,           1, 2, 0 },
		{ OPT_GETFILEBYID,       1, 2, 0 },
		{ OPT_GETALLFILES,       1, 1, 0 },
		{ OPT_PUTFILE,           2, 2, 0 },
		{ OPT_DELETEFILE,        1, 1, 0 },
		{ OPT_DELETEFILEBYID,    1, 1, 0 },
		{ OPT_SHELL,             0, 0, 0 },
		{ OPT_GETMMS,            2, 6, 0 },
		{ OPT_DELETEMMS,         2, 3, 0 },
		{ OPT_FOOGLE,            0, 0, 0 },
		{ 0, 0, 0, 0 },
	};

	/* Handle command line arguments.
	 * -c equals to --config
	 * -p equals to --phone
	 */
	c = getopt_long(argc, argv, "c:p:", long_options, &opt_index);

	switch (c) {
	/* No argument given - we should display usage. */
	case -1:
		return usage(stderr, -1, NULL);
	/* First, error conditions */
	case '?':
	case ':':
		/* --shell command does not set argv[0] */
		if (argv[0])
			fprintf(stderr, _("Use '%s --help' for usage information.\n"), argv[0]);
		else
			fprintf(stderr, _("Use '--help' for usage information.\n"));
		return 1;
	/* Then, options with no arguments */
	case OPT_VERSION:
		version();
		return 0;
	/* That's a bit ugly... */
	case 'c':
		c = OPT_CONFIGFILE;
		opt_index = 0;
		break;
	case 'p':
		c = OPT_CONFIGMODEL;
		opt_index = 1;
		break;
	}

	/* We have to build an array of the arguments which will be passed to the
	   functions.  Please note that every text after the --command will be
	   passed as arguments.  A syntax like gnokii --cmd1 args --cmd2 args will
	   not work as expected; instead args --cmd2 args is passed as a
	   parameter. */
	if (checkargs(c, gals, argc, long_options[opt_index].has_arg)) {
		return usage(stderr, -1, NULL);
	}

	/* Other options that do not need initialization */
	switch (c) {
	case OPT_CONFIGFILE:
		if (configfile)
			return usage(stderr, -1, NULL);
		configfile = optarg;
		return parse_options(argc, argv);
	case OPT_CONFIGMODEL:
		if (configmodel)
			return usage(stderr, -1, NULL);
		configmodel = optarg;
		return parse_options(argc, argv);
	case OPT_HELP:
		return usage(stdout, argc, argv);
	case OPT_VIEWLOGO:
		return viewlogo(optarg);
	case OPT_LISTNETWORKS:
		list_gsm_networks();
		return GN_ERR_NONE;
	case OPT_RINGTONECONVERT:
		return ringtoneconvert(argc, argv);
	}

	/* Initialise the code for the GSM interface. */
	if (c != OPT_FOOGLE && state == NULL && businit())
		return -1;

	switch (c) {
	/* Monitoring options */
	case OPT_MONITOR:
		rc = monitormode(argc, argv, data, state);
		break;
	case OPT_GETDISPLAYSTATUS:
		rc = getdisplaystatus(data, state);
		break;
	case OPT_DISPLAYOUTPUT:
		rc = displayoutput(data, state);
		break;
	case OPT_NETMONITOR:
		rc = netmonitor(optarg, data, state);
		break;

	/* SMS options */
	case OPT_SENDSMS:
		rc = sendsms(argc, argv, data, state);
		break;
	case OPT_SAVESMS:
		rc = savesms(argc, argv, data, state);
		break;
	case OPT_GETSMS:
		rc = getsms(argc, argv, data, state);
		break;
	case OPT_DELETESMS:
		rc = deletesms(argc, argv, data, state);
		break;
	case OPT_GETSMSC:
		rc = getsmsc(argc, argv, data, state);
		break;
	case OPT_SETSMSC:
		rc = setsmsc(data, state);
		break;
	case OPT_CREATESMSFOLDER:
		rc = createsmsfolder(optarg, data, state);
		break;
	case OPT_DELETESMSFOLDER:
		rc = deletesmsfolder(optarg, data, state);
		break;
	case OPT_SHOWSMSFOLDERSTATUS:
		rc = showsmsfolderstatus(data, state);
		break;
	case OPT_SMSREADER:
		rc = smsreader(data, state);
		break;

	/* Phonebook options */
	case OPT_GETPHONEBOOK:
		rc = getphonebook(argc, argv, data, state);
		break;
	case OPT_WRITEPHONEBOOK:
		rc = writephonebook(argc, argv, data, state);
		break;
	case OPT_DELETEPHONEBOOK:
		rc = deletephonebook(argc, argv, data, state);
		break;

	/* Calendar options */
	case OPT_GETCALENDARNOTE:
		rc = getcalendarnote(argc, argv, data, state);
		break;
	case OPT_WRITECALENDARNOTE:
		rc = writecalendarnote(argc, argv, data, state);
		break;
	case OPT_DELCALENDARNOTE:
		rc = deletecalendarnote(argc, argv, data, state);
		break;

	/* ToDo options */
	case OPT_GETTODO:
		rc = gettodo(argc, argv, data, state);
		break;
	case OPT_WRITETODO:
		rc = writetodo(argc, argv, data, state);
		break;
	case OPT_DELETEALLTODOS:
		rc = deletealltodos(data, state);
		break;

	/* Dialling and call handling options */
	case OPT_GETSPEEDDIAL:
		rc = getspeeddial(optarg, data, state);
		break;
	case OPT_SETSPEEDDIAL:
		rc = setspeeddial(argv, data, state);
		break;
	case OPT_DIALVOICE:
		rc = dialvoice(optarg, data, state);
		break;
	case OPT_SENDDTMF:
		rc = senddtmf(optarg, data, state);
		break;
	case OPT_ANSWERCALL:
		rc = answercall(optarg, data, state);
		break;
	case OPT_HANGUP:
		rc = hangup(optarg, data, state);
		break;
	case OPT_DIVERT:
		rc = divert(argc, argv, data, state);
		break;

	/* Profile options */
	case OPT_GETPROFILE:
		rc = getprofile(argc, argv, data, state);
		break;
	case OPT_SETPROFILE:
		rc = setprofile(data, state);
		break;
	case OPT_GETACTIVEPROFILE:
		rc = getactiveprofile(data, state);
		break;
	case OPT_SETACTIVEPROFILE:
		rc = setactiveprofile(argc, argv, data, state);
		break;

	/* Phone settings options */
	case OPT_RESET:
		rc = reset(optarg, data, state);
		break;
	case OPT_GETDATETIME:
		rc = getdatetime(data, state);
		break;
	case OPT_SETDATETIME:
		rc = setdatetime(argc, argv, data, state);
		break;
	case OPT_GETALARM:
		rc = getalarm(data, state);
		break;
	case OPT_SETALARM:
		rc = setalarm(argc, argv, data, state);
		break;

	/* WAP options */
	case OPT_GETWAPBOOKMARK:
		rc = getwapbookmark(optarg, data, state);
		break;
	case OPT_WRITEWAPBOOKMARK:
		rc = writewapbookmark(argc, argv, data, state);
		break;
	case OPT_DELETEWAPBOOKMARK:
		rc = deletewapbookmark(optarg, data, state);
		break;
	case OPT_GETWAPSETTING:
		rc = getwapsetting(argc, argv, data, state);
		break;
	case OPT_WRITEWAPSETTING:
		rc = writewapsetting(data, state);
		break;
	case OPT_ACTIVATEWAPSETTING:
		rc = activatewapsetting(optarg, data, state);
		break;

	/* Logo options */
	case OPT_SENDLOGO:
		rc = sendlogo(argc, argv, data, state);
		break;
	case OPT_SETLOGO:
		rc = setlogo(argc, argv, data, state);
		break;
	case OPT_GETLOGO:
		rc = getlogo(argc, argv, data, state);
		break;

	/* Ringtone options */
	case OPT_SENDRINGTONE:
		rc = sendringtone(argc, argv, data, state);
		break;
	case OPT_GETRINGTONE:
		rc = getringtone(argc, argv, data, state);
		break;
	case OPT_SETRINGTONE:
		rc = setringtone(argc, argv, data, state);
		break;
	case OPT_PLAYRINGTONE:
		rc = playringtone(argc, argv, data, state);
		break;
	case OPT_GETRINGTONELIST:
		rc = getringtonelist(data, state);
		break;
	case OPT_DELETERINGTONE:
		rc = deleteringtone(argc, argv, data, state);
		break;

	/* Security options */
	case OPT_IDENTIFY:
		rc = identify(state);
		break;
	case OPT_GETLOCKSINFO:
		rc = getlocksinfo(data, state);
		break;
#ifdef SECURITY
	case OPT_GETSECURITYCODE:
		rc = getsecuritycode(data, state);
		break;
	case OPT_ENTERSECURITYCODE:
		rc = entersecuritycode(optarg, data, state);
		if (rc == GN_ERR_NONE && optind < argc)
			return parse_options(argc, argv);
		break;
	case OPT_GETSECURITYCODESTATUS:
		rc = getsecuritycodestatus(data, state);
		break;
	case OPT_CHANGESECURITYCODE:
		rc = changesecuritycode(optarg, data, state);
			break;
#endif

	/* File options */
	case OPT_GETFILELIST:
		rc = getfilelist(optarg, data, state);
		break;
	case OPT_GETFILEDETAILSBYID:
		rc = getfiledetailsbyid(argc, argv, data, state);
		break;
	case OPT_GETFILEID:
		rc = getfileid(optarg, data, state);
		break;
	case OPT_GETFILE:
		rc = getfile(argc, argv, data, state);
		break;
	case OPT_GETFILEBYID:
		rc = getfilebyid(argc, argv, data, state);
		break;
	case OPT_GETALLFILES:
		rc = getallfiles(optarg, data, state);
		break;
	case OPT_PUTFILE:
		rc = putfile(argc, argv, data, state);
		break;
	case OPT_DELETEFILE:
		rc = deletefile(optarg, data, state);
		break;
	case OPT_DELETEFILEBYID:
		rc = deletefilebyid(optarg, data, state);
		break;

	/* Misc options */
	case OPT_PMON:
		rc = pmon(data, state);
		break;
	case OPT_KEYPRESS:
		rc = presskeysequence(data, state);
		break;
	case OPT_ENTERCHAR:
		rc = enterchar(data, state);
		break;
	case OPT_GETNETWORKINFO:
		rc = getnetworkinfo(data, state);
		break;
	case OPT_SHELL:
		rc = shell(data, state);
		break;
#ifndef WIN32
	case OPT_FOOGLE:
		rc = foogle(argc, argv);
		break;
#endif
	/* MMS options */
	case OPT_GETMMS:
		rc = getmms(argc, argv, data, state);
		break;
	case OPT_DELETEMMS:
		rc = deletemms(argc, argv, data, state);
		break;
	default:
		rc = GN_ERR_FAILED;
		fprintf(stderr, _("Unknown option: %d\n"), c);
		break;
	}
	return rc;
}

#define ARGV_CHUNK 10

int shell(gn_data *data, struct gn_statemachine *state)
{
#ifdef HAVE_READLINE
	char *input = NULL, *tmp, *old;
	int len, i, argc = 1;
	char **argv = NULL;
	int size = ARGV_CHUNK;
	int empty = 1;

	argv = calloc(size, sizeof(char *));
	while (1) {
		argc = 1;
		optind = opterr = optopt = 0;
		optarg = NULL;
		input = readline("gnokii> ");
		old = input;
		if (!input)
			break;
		do {
			if (argc >= size) {
				size += ARGV_CHUNK;
				argv = realloc(argv, size * sizeof(char *));
			}
			while (*input == ' ')
				input++;
			tmp = strstr(input, " ");
			if (tmp) {
				len = tmp - input;
				*tmp = '\0';
			} else
				len = strlen(input);
			if (len > 0) {
				argv[argc++] = strdup(input);
				empty = 0;
                        }
                        input = tmp + (tmp ? 1 : 0);
		} while (input);
		argv[argc] = NULL;
		if (!empty)
        		parse_options(argc, argv);
		for (i = 1; i < argc; i++)
			free(argv[i]);
		free(old);
	}
	free(argv);
	return 0;
#else
	fprintf(stderr, _("gnokii needs to be compiled with readline support to enable this option.\n"));
	return -1;
#endif
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */
int main(int argc, char *argv[])
{
	int rc;

	/* For GNU gettext */
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	opterr = 0;

	/* Introduce yourself */
	short_version();

#ifdef __svr4__
	/* have to ignore SIGALARM */
	sigignore(SIGALRM);
#endif

	rc = parse_options(argc, argv);
	exit(rc);
}
