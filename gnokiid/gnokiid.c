/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Mainline code for gnokiid daemon. Handles command line parsing and
  various daemon functions.

  Last modification: Mon May 15
  Modified by Chris Kemp <ck231@cam.ac.uk>	

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
#include "virtmodem.h"


	/* Prototypes. */
void	read_config(void);

	/* Global variables */
bool		DebugMode;	/* When true, run in debug mode */
char		*Model;		/* Model from .gnokiirc file. */
char		*Port;		/* Serial port from .gnokiirc file */
char		*Initlength;	/* Init length from .gnokiirc file */
char		*Connection;	/* Connection type from .gnokiirc file */
char		*BinDir;	/* Directory of the mgnokiidev command */
bool  TerminateThread;

	/* Local variables */
char		*DefaultModel = MODEL;	/* From Makefile */
char		*DefaultPort = PORT;

char		*DefaultConnection = "serial";
char		*DefaultBinDir = "/usr/local/sbin";

void version(void)
{

  fprintf(stdout, _("gnokiid Version %s
Copyright (C) Hugh Blemings <hugh@linuxcare.com>, 1999
Copyright (C) Pavel Janík ml. <Pavel.Janik@linux.cz>, 1999
Built %s %s for %s on %s \n"), VERSION, __TIME__, __DATE__, Model, Port);
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

    GSM_ConnectionType connection = GCT_Serial;

		/* For GNU gettext */

	#ifdef USE_NLS
  		textdomain("gnokii");
	#endif

	read_config();

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

	if (!strcmp(Connection, "infrared")) {
		connection=GCT_Infrared;
	}

	TerminateThread=false;

	if (VM_Initialise(Model, Port, Initlength, connection, BinDir, DebugMode) == false) 
	  exit (-1);

	while (1) {
	  if (TerminateThread==true) {
	    VM_Terminate();
	    exit(1);
	  }
	  sleep (1);
	}
	exit (0);
}

void	read_config(void)
{
    struct CFG_Header 	*cfg_info;
	char				*homedir;
	char				rcfile[200];

	homedir = getenv("HOME");

	strncpy(rcfile, homedir, 200);
	strncat(rcfile, "/.gnokiirc", 200);

      /* Try opening .gnokirc from users home directory first */
    if ((cfg_info = CFG_ReadFile(rcfile)) == NULL) {
        /* It failed so try for /etc/gnokiirc */
      if ( (cfg_info = CFG_ReadFile("/etc/gnokiirc")) == NULL ) {
	  /* That failed too so go with defaults... */
        fprintf(stderr, _("Couldn't open %s or /etc/gnokiirc, using default config\n"), rcfile);
      }
    }

    Model = CFG_Get(cfg_info, "global", "model");
    if (Model == NULL) {
		Model = DefaultModel;
    }

    Port = CFG_Get(cfg_info, "global", "port");
    if (Port == NULL) {
		Port = DefaultPort;
    }

    Initlength = CFG_Get(cfg_info, "global", "initlength");
    if (Initlength == NULL) {
		Initlength = "default";
    }

    Connection = CFG_Get(cfg_info, "global", "connection");
    if (Connection == NULL) {
		Connection = DefaultConnection;
    }

    BinDir = CFG_Get(cfg_info, "global", "bindir");
    if (BinDir == NULL) {
                BinDir = DefaultBinDir;
    }

}
