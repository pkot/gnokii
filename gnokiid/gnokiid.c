/*
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Mainline code for gnokiid daemon.  Handles command line parsing and
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
#include "gsm-common.h"
#include "gsm-api.h"
#include "at-emulator.h"

/* Prototypes. */

/* Global variables */
bool		DebugMode;	/* When true, run in debug mode */

void version(void)
{

  fprintf(stdout, _("gnokiid Version %s
Copyright (C) Hugh Blemings <hugh@vsb.com.au>, 1999
Copyright (C) Pavel Janík ml. <Pavel.Janik@linux.cz>, 1999
Built %s %s for %s on %s \n"), VERSION, __TIME__, __DATE__, MODEL, PORT);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options.*/

void usage(void)
{

  fprintf(stdout, _("   usage: gnokiid {--help|--version}

          --help            display usage information.

          --version         displays version and copyright information.
          
          --debug           uses stdin/stdout for virtual modem comms.
  \n"));
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{

  /* For GNU gettext */

#ifdef GNOKII_GETTEXT
  textdomain("gnokii");
#endif

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
	}
	else {
		DebugMode = false;	
	}

		/* FIXME Debug mode forced for now! */
//	DebugMode = true;

	ATEM_Initialise(DebugMode);
	while (1) {
		sleep (1);
	}
	exit (0);
}
