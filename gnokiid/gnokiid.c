/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Mainline code for gnokiid daemon. Handles command line parsing and
  various daemon functions.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#include "misc.h"
#include "cfgreader.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "data/virtmodem.h"


/* Global variables */
bool DebugMode;		/* When true, run in debug mode */
char *Model;		/* Model from .gnokiirc file. */
char *Port;		/* Serial port from .gnokiirc file */
char *Initlength;	/* Init length from .gnokiirc file */
char *Connection;	/* Connection type from .gnokiirc file */
char *BinDir;		/* Directory of the mgnokiidev command */
char *lockfile = NULL;
bool GTerminateThread;

/* Local variables */
char *DefaultConnection = "serial";
char *DefaultBinDir = "/usr/local/sbin";

static void short_version()
{
	fprintf(stderr, _("GNOKIID Version %s\n"), VERSION);
}

static void version()
{
	fprintf(stderr, _("Copyright (C) Hugh Blemings <hugh@blemings.org>, 1999\n"
			  "Copyright (C) Pavel Janík ml. <Pavel.Janik@suse.cz>, 1999\n"
			  "Built %s %s for %s on %s \n"),
			  __TIME__, __DATE__, Model, Port);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options.*/
static void usage()
{

	fprintf(stderr, _("   usage: gnokiid {--help|--version}\n"
"          --help            display usage information\n"
"          --version         displays version and copyright information\n"
"          --debug           uses stdin/stdout for virtual modem comms\n"));
}

/* cleanup function registered by atexit() and called at exit() */
static void busterminate(void)
{
	vm_terminate();
	if (lockfile) gn_unlock_device(lockfile);
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{
	char *aux;
	static bool atexit_registered = false;

	/* For GNU gettext */
#ifdef ENABLE_NLS
	textdomain("gnokii");
#endif

	short_version();

	if (gn_cfg_readconfig(&Model, &Port, &Initlength, &Connection, &BinDir) < 0) {
		exit(-1);
	}

	/* Handle command line arguments. */
	if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
		usage();
		exit(0);
	}

	/* Display version, copyright and build information. */
	if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
		version();
		exit(0);
	}

	if (argc >= 2 && strcmp(argv[1], "--debug") == 0) {
		DebugMode = true;
	} else {
		DebugMode = false;
	}

	/* register cleanup function */
	if (!atexit_registered) {
		atexit_registered = true;
		atexit(busterminate);
	}

	aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
	/* Defaults to 'no' */
	if (aux && !strcmp(aux, "yes")) {
		lockfile = gn_lock_device(Port);
		if (lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting\n"));
			exit(1);
		}
	}

	while (1) {
		if (vm_initialise(Model, Port, Initlength, Connection, BinDir, DebugMode, true) == false) {
			exit (-1);
		}

		GTerminateThread = false;

		vm_loop();
	}

	exit (0);
}
