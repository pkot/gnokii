/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Mainline code for gnokiid daemon. Handles command line parsing and
  various daemon functions.

  $Log$
  Revision 1.19  2001-06-28 00:28:45  pkot
  Small docs updates (Pawel Kot)

  Revision 1.18  2001/02/21 19:57:09  chris
  More fiddling with the directory layout

  Revision 1.17  2000/12/19 16:18:18  pkot
  configure script updates and added shared function for configfile reading

  
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
bool     DebugMode;       /* When true, run in debug mode */
char     *Model;          /* Model from .gnokiirc file. */
char     *Port;           /* Serial port from .gnokiirc file */
char     *Initlength;     /* Init length from .gnokiirc file */
char     *Connection;     /* Connection type from .gnokiirc file */
char     *BinDir;         /* Directory of the mgnokiidev command */
bool     TerminateThread;

/* Local variables */
char     *DefaultConnection = "serial";
char     *DefaultBinDir = "/usr/local/sbin";

void version(void)
{

        fprintf(stdout, _("gnokiid Version %s\n"
"Copyright (C) Hugh Blemings <hugh@blemings.org>, 1999\n"
"Copyright (C) Pavel Janík ml. <Pavel.Janik@suse.cz>, 1999\n"
"Built %s %s for %s on %s \n"), VERSION, __TIME__, __DATE__, Model, Port);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options.*/

void usage(void)
{

        fprintf(stdout, _("   usage: gnokiid {--help|--version}\n"
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
                connection=GCT_Infrared;
        }

        TerminateThread=false;

        if (VM_Initialise(Model, Port, Initlength, connection, BinDir, DebugMode, true) == false) {
                exit (-1);
        }

        while (1) {
                if (TerminateThread==true) {
                        VM_Terminate();
                        exit(1);
                }
                sleep (1);
        }
        exit (0);
}
