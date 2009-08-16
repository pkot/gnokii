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
  Copyright (C) 2001-2004  Pawel Kot
  Copyright (C) 2002       Manfred Jonsson
  Copyright (C) 2002-2003  BORBELY Zoltan

  Mainline code for gnokiid daemon. Handles command line parsing and
  various daemon functions.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#include "config.h"

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "misc.h"
#include "gnokii-internal.h"

/* Global variables */
bool DebugMode;		/* When true, run in debug mode */
const char *BinDir;	/* Directory of the mgnokiidev command */
char *lockfile = NULL;
bool GTerminateThread;


static void short_version()
{
	fprintf(stderr, _("GNOKIID Version %s\n"), VERSION);
}

static void version()
{
	fprintf(stderr, _("Copyright (C) Hugh Blemings <hugh@blemings.org>, 1999\n"
			  "Copyright (C) Pavel Janik ml. <Pavel.Janik@suse.cz>, 1999\n"
			  "Built %s %s\n"),
			  __TIME__, __DATE__);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options.*/
static void usage()
{

	fprintf(stderr, _("   usage: gnokiid [--help|--version|--debug]\n"
"          --help            display usage information\n"
"          --version         displays version and copyright information\n"
"          --debug           uses stdin/stdout for virtual modem comms\n"));
}

/* cleanup function registered by atexit() and called at exit() */
static void busterminate(void)
{
	gn_vm_terminate();
	gn_device_unlock(lockfile);
	gn_lib_library_free();
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{
	const char *aux;
	static bool atexit_registered = false;
	struct gn_statemachine temp_state;

	/* For GNU gettext */
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	short_version();

	/* Handle command line arguments. */
	switch (argc) {
	case 1:
		/* Default */
		DebugMode = false;
		break;
	case 2:
		if (strcmp(argv[1], "--version") == 0) {
			/* Display version, copyright and build information. */
			version();
			exit(0);
		} else if (strcmp(argv[1], "--debug") == 0) {
			/* Use stdin/stdout for I/O */
			DebugMode = true;
			break;
		} else if (strcmp(argv[1], "--help") == 0) {
			usage();
			exit(0);
		}
		/* FALL TROUGH */
	default:
		usage();
		exit(1);
	}

	aux = gn_lib_cfg_get("global", "model");
	if (!aux) {
		fprintf(stderr, "%s\n", gn_error_print(GN_ERR_NOCONFIG));
		exit(1);
		}

	if (strncmp(aux, "5110", 4) &&
	    strncmp(aux, "5130", 4) &&
	    strncmp(aux, "6110", 4) &&
	    strncmp(aux, "6130", 4) &&
	    strncmp(aux, "6150", 4)) {
		fprintf(stderr, _("gnokiid purpose is to work only with the phones that do not have AT Hayes\ncommands interpreter.\n"));
		exit(1);
	}

	BinDir = gn_lib_cfg_get("global", "bindir");
	if (!BinDir) BinDir = gn_lib_cfg_get("gnokiid", "bindir");
	if (!BinDir) BinDir = SBINDIR;

	if (gn_cfg_phone_load("", &temp_state) != GN_ERR_NONE) exit(-1);

	gn_elog_handler = NULL;

	if (temp_state.config.use_locking) {
		lockfile = gn_device_lock(temp_state.config.port_device);
		if (lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting.\n"));
			exit(1);
		}
	}

	/* register cleanup function */
	if (!atexit_registered) {
		atexit_registered = true;
		atexit(busterminate);
	}

	while (1) {
		if (gn_vm_initialise("", BinDir, DebugMode, true) == false) {
			exit (-1);
		}

		GTerminateThread = false;

		gn_vm_loop();
	}

	exit (0);
}
