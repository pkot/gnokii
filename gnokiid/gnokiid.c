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
"          --help            display usage information."
"          --version         displays version and copyright information."
"          --debug           uses stdin/stdout for virtual modem comms.\n"));
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{
	GSM_ConnectionType connection = GCT_Serial;

	/* For GNU gettext */
#ifdef USE_NLS
	textdomain("gnokii");
#endif

	short_version();

	if (readconfig(&Model, &Port, &Initlength, &Connection, &BinDir) < 0) {
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

	if (!strcmp(Connection, "infrared")) {
		connection = GCT_Infrared;
	}

	GTerminateThread = false;

	if (VM_Initialise(Model, Port, Initlength, connection, BinDir, DebugMode, true) == false) {
		exit (-1);
	}

	while (1) {
		if (GTerminateThread == true) {
			VM_Terminate();
			exit(1);
		}
		sleep (1);
	}
	exit (0);
}
