/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001  	   Pavel Machek

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries and other stuff.

  WARNING: this code is the test tool. Well, our test tool is now
  really powerful and useful :-)

  $Log$
  Revision 1.136  2001-06-10 23:49:49  pkot
  Small fixes to hide compilation warnings and allow gnokii.c to compile

  Revision 1.135  2001/06/10 11:42:26  machek
  Cleanup: some formating, made Statemachine global, converted to new
  structure w.r.t. SMS-es

  Revision 1.134  2001/05/24 20:47:30  chris
  More updating of 7110 code and some of xgnokii_lowlevel changed over.

  Revision 1.133  2001/04/23 17:20:01  pkot
  Added possibility for viewing logos (currently nol and ngg) on console (Bartek Klepacz)

  Revision 1.132  2001/03/21 23:36:06  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.131  2001/03/19 23:43:46  pkot
  Solaris / BSD '#if defined' cleanup

  Revision 1.130  2001/03/13 01:23:18  pkot
  Windows updates (Manfred Jonsson)

  Revision 1.129  2001/03/13 01:21:39  pkot
  *BSD updates (Bert Driehuis)

  Revision 1.128  2001/03/08 00:49:06  pkot
  Fixed bug (introduced by me) in getmemory function. Now gnokii.c should compile

  Revision 1.127  2001/03/08 00:18:13  pkot
  Fixed writephonebook once again. Someone kick me please...

  Revision 1.126  2001/03/07 21:46:12  pkot
  Fixed writephonebook patch

  Revision 1.125  2001/03/06 22:19:14  pkot
  Cleanups and fixes in gnokii.c:
   - reindenting
   - fixed bug reported by Gabriele Zappi
   - fixed small bugs introduced by Pavel Machek

  Revision 1.124  2001/02/28 21:42:00  machek
  Possibility to force overwrite in --getsms (-F). Possibility to get
  multiple files (use --getsms -f xyzzy%d), cleanup.

  Revision 1.123  2001/02/20 21:55:11  pkot
  Small #include updates

  Revision 1.122  2001/02/16 14:29:53  chris
  Restructure of common/.  Fixed a problem in fbus-phonet.c
  Lots of dprintfs for Marcin
  Any size xpm can now be loaded (eg for 7110 startup logos)
  nk7110 code detects 7110/6210 and alters startup logo size to suit
  Moved Marcin's extended phonebook code into gnokii.c

  Revision 1.121  2001/02/06 21:15:35  chris
  Preliminary irda support for 7110 etc.  Not well tested!

  Revision 1.120  2001/02/06 08:13:32  pkot
  One more include in gnokii.c needed

  Revision 1.119  2001/02/05 12:29:37  pkot
  Changes needed to let unicode work

  Revision 1.118  2001/02/01 15:17:33  pkot
  Fixed --identify and added Manfred's manufacturer patch

  Revision 1.117  2001/01/31 23:45:27  pkot
  --identify should work ok now

  Revision 1.116  2001/01/24 20:19:55  machek
  Do not retry identification, if it is not implemented, it is bad idea.

  Revision 1.115  2001/01/22 01:25:10  hugh
  Tweaks for 3810 series, datacalls seem to be broken so need to do
  some more debugging...

  Revision 1.114  2001/01/17 02:54:55  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.

  Revision 1.113  2001/01/15 17:00:49  pkot
  Initial keypress sequence support. Disable compilation warning

  Revision 1.112  2001/01/12 14:09:13  pkot
  More cleanups. This time mainly in the code.

  Revision 1.111  2001/01/10 16:32:18  pkot
  Documentation updates.
  FreeBSD fix for 3810 code.
  Added possibility for deleting SMS just after reading it in gnokii.
  2110 code updates.
  Many cleanups.

  Revision 1.110  2001/01/08 15:11:37  pkot
  Documentation updates.
  Fixed some bugs and removed FIXMEs.
  We need to move some stuff from configure.in to aclocal.m4

  Revision 1.109  2000/12/29 15:39:07  pkot
  Reverted a change in fbus-3810.c which broke compling with --enable-debug.
  Small fixes in gnokii.c

  Revision 1.108  2000/12/19 16:18:16  pkot
  configure script updates and added shared function for configfile reading

  
*/

#include "config.h"
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#if __unices__
#  include <strings.h>	/* for memset */
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32

#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
#define stat _stat
#include "win32/getopt.h"

#else

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <getopt.h>

#endif

#ifdef USE_NLS
#include <locale.h>
#endif

#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-networks.h"
#include "cfgreader.h"
#include "gnokii.h"
#include "gsm-filetypes.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"
#include "gsm-statemachine.h"

char *model;      /* Model from .gnokiirc file. */
char *Port;       /* Serial port from .gnokiirc file */
char *Initlength; /* Init length from .gnokiirc file */
char *Connection; /* Connection type from .gnokiirc file */
char *BinDir;     /* Binaries directory from .gnokiirc file - not used here yet */
/* Local variables */

char *GetProfileCallAlertString(int code)
{
	switch (code) {
	case PROFILE_CALLALERT_RINGING:		return "Ringing";
	case PROFILE_CALLALERT_ASCENDING:	return "Ascending";
	case PROFILE_CALLALERT_RINGONCE:	return "Ring once";
	case PROFILE_CALLALERT_BEEPONCE:	return "Beep once";
	case PROFILE_CALLALERT_CALLERGROUPS:	return "Caller groups";
	case PROFILE_CALLALERT_OFF:		return "Off";
	default:				return "Unknown";
	}
}

char *GetProfileVolumeString(int code)
{
	switch (code) {
	case PROFILE_VOLUME_LEVEL1:		return "Level 1";
	case PROFILE_VOLUME_LEVEL2:		return "Level 2";
	case PROFILE_VOLUME_LEVEL3:		return "Level 3";
	case PROFILE_VOLUME_LEVEL4:		return "Level 4";
	case PROFILE_VOLUME_LEVEL5:		return "Level 5";
	default:				return "Unknown";
	}
}

char *GetProfileKeypadToneString(int code)
{
	switch (code) {
	case PROFILE_KEYPAD_OFF:		return "Off";
	case PROFILE_KEYPAD_LEVEL1:		return "Level 1";
	case PROFILE_KEYPAD_LEVEL2:		return "Level 2";
	case PROFILE_KEYPAD_LEVEL3:		return "Level 3";
	default:				return "Unknown";
	}
}

char *GetProfileMessageToneString(int code)
{
	switch (code) {
	case PROFILE_MESSAGE_NOTONE:		return "No tone";
	case PROFILE_MESSAGE_STANDARD:		return "Standard";
	case PROFILE_MESSAGE_SPECIAL:		return "Special";
	case PROFILE_MESSAGE_BEEPONCE:		return "Beep once";
	case PROFILE_MESSAGE_ASCENDING:		return "Ascending";
	default:				return "Unknown";
	}
}

char *GetProfileWarningToneString(int code)
{
	switch (code) {
	case PROFILE_WARNING_OFF:		return "Off";
	case PROFILE_WARNING_ON:		return "On";
	default:				return "Unknown";
	}
}

char *GetProfileVibrationString(int code)
{
	switch (code) {
	case PROFILE_VIBRATION_OFF:		return "Off";
	case PROFILE_VIBRATION_ON:		return "On";
	default:				return "Unknown";
	}
}

/* This function shows the copyright and some informations usefull for
   debugging. */

int version(void)
{
	fprintf(stdout, _("GNOKII Version %s\n"
			  "Copyright (C) Hugh Blemings <hugh@linuxcare.com>, 1999, 2000\n"
			  "Copyright (C) Pavel Janík ml. <Pavel.Janik@linux.cz>, 1999, 2000\n"
			  "Copyright (C) Pavel Machek <pavel@ucw.cz>, 2001\n"
			  "gnokii is free software, covered by the GNU General Public License, and you are\n"
			  "welcome to change it and/or distribute copies of it under certain conditions.\n"
			  "There is absolutely no warranty for gnokii.  See GPL for details.\n"
			  "Built %s %s for %s on %s \n"), VERSION, __TIME__, __DATE__, model, Port);
	return 0;
}

/* The function usage is only informative - it prints this program's usage and
   command-line options. */

int usage(void)
{
	fprintf(stdout, _("   usage: gnokii [--help|--monitor|--version]\n"
			  "          gnokii --getmemory memory_type start [end]\n"
			  "          gnokii --writephonebook [-i]\n"
			  "          gnokii --getspeeddial number\n"
			  "          gnokii --setspeeddial number memory_type location\n"
			  "          gnokii --getsms memory_type start [end] [-f file] [-F file] [-d]\n"
			  "          gnokii --deletesms memory_type start [end]\n"
			  "          gnokii --sendsms destination [--smsc message_center_number |\n"
			  "                 --smscno message_center_index] [-r] [-C n] [-v n]\n"
			  "                 [--long n]\n"
			  "          gnokii --savesms [-m] [-l n] [-i]\n"
			  "          gnokii --getsmsc message_center_number\n"
			  "          gnokii --setdatetime [YYYY [MM [DD [HH [MM]]]]]\n"
			  "          gnokii --getdatetime\n"
			  "          gnokii --setalarm HH MM\n"
			  "          gnokii --getalarm\n"
			  "          gnokii --dialvoice number\n"
			  "          gnokii --getcalendarnote index [-v]\n"
			  "          gnokii --writecalendarnote vcardfile number\n"
			  "          gnokii --deletecalendarnote index\n"
			  "          gnokii --getdisplaystatus\n"
			  "          gnokii --netmonitor {reset|off|field|devel|next|nr}\n"
			  "          gnokii --identify\n"
			  "          gnokii --senddtmf string\n"
			  "          gnokii --sendlogo {caller|op} destination logofile [network code]\n"
			  "          gnokii --sendringtone destination rtttlfile\n"
			  "          gnokii --setlogo op [logofile] [network code]\n"
			  "          gnokii --setlogo startup [logofile]\n"
			  "          gnokii --setlogo caller [logofile] [caller group number] [group name]\n"
			  "          gnokii --setlogo {dealer|text} [text]\n"
			  "          gnokii --getlogo op [logofile] [network code]\n"
			  "          gnokii --getlogo startup [logofile] [network code]\n"
			  "          gnokii --getlogo caller [logofile][caller group number][network code]\n"
			  "          gnokii --getlogo {dealer|text}\n"
			  "          gnokii --viewlogo logofile\n"
			  "          gnokii --setringtone rtttlfile\n"
			  "          gnokii --reset [soft|hard]\n"
			  "          gnokii --getprofile [number]\n"
			  "          gnokii --displayoutput\n"
			  "          gnokii --keysequence\n"
		));
#ifdef SECURITY
	fprintf(stdout, _(
		"          gnokii --entersecuritycode PIN|PIN2|PUK|PUK2\n"
		"          gnokii --getsecuritycodestatus\n"
		));
#endif
	exit(-1);
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

static GSM_Statemachine State;
static GSM_Data data;

void fbusinit(void (*rlp_handler)(RLP_F96Frame *frame))
{
	int count=0;
	GSM_Error error;
	GSM_ConnectionType connection=GCT_Serial;

	GSM_DataClear(&data);

	if (!strcmp(Connection, "infrared")) connection=GCT_Infrared;
	if (!strcmp(Connection, "irda"))     connection=GCT_Irda;

	/* Initialise the code for the GSM interface. */     

	error = GSM_Initialise(model, Port, Initlength, connection, rlp_handler, &State);

	if (error != GE_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
		exit(-1);
	}

	/* First (and important!) wait for GSM link to be active. We allow 10
	   seconds... */

	while (count++ < 200 && *GSM_LinkOK == false)
		usleep(50000);

	if (*GSM_LinkOK == false) {
		fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting.\n"));
		exit(-1);
	}
}

/* This function checks that the argument count for a given options is withing
   an allowed range. */
int checkargs(int opt, struct gnokii_arg_len gals[], int argc)
{
	int i;

	/* Walk through the whole array with options requiring arguments. */
	for (i = 0; !(gals[i].gal_min == 0 && gals[i].gal_max == 0); i++) {

		/* Current option. */
		if (gals[i].gal_opt == opt) {

			/* Argument count checking. */
			if (gals[i].gal_flags == GAL_XOR) {
				if (gals[i].gal_min == argc || gals[i].gal_max == argc)
					return 0;
			} else {
				if (gals[i].gal_min <= argc && gals[i].gal_max >= argc)
					return 0;
			}

			return 1;
		}
	}

	/* We do not have options without arguments in the array, so check them. */
	if (argc == 0) return 0;
	else return 1;
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */
int main(int argc, char *argv[])
{
	int c, i, rc = -1;
	int nargc = argc-2;
	char **nargv;

	/* Every option should be in this array. */
	static struct option long_options[] =
	{
		/* FIXME: these comments are nice, but they would be more usefull as docs for the user */
		// Display usage.
		{ "help",               no_argument,       NULL, OPT_HELP },

		// Display version and build information.
		{ "version",            no_argument,       NULL, OPT_VERSION },

		// Monitor mode
		{ "monitor",            no_argument,       NULL, OPT_MONITOR },

#ifdef SECURITY

		// Enter Security Code mode
		{ "entersecuritycode",  required_argument, NULL, OPT_ENTERSECURITYCODE },

		// Get Security Code status
		{ "getsecuritycodestatus",  no_argument,   NULL, OPT_GETSECURITYCODESTATUS },

#endif

		// Set date and time
		{ "setdatetime",        optional_argument, NULL, OPT_SETDATETIME },

		// Get date and time mode
		{ "getdatetime",        no_argument,       NULL, OPT_GETDATETIME },

		// Set alarm
		{ "setalarm",           required_argument, NULL, OPT_SETALARM },

		// Get alarm
		{ "getalarm",           no_argument,       NULL, OPT_GETALARM },

		// Voice call mode
		{ "dialvoice",          required_argument, NULL, OPT_DIALVOICE },

		// Get calendar note mode
		{ "getcalendarnote",    required_argument, NULL, OPT_GETCALENDARNOTE },

		// Write calendar note mode
		{ "writecalendarnote",  required_argument, NULL, OPT_WRITECALENDARNOTE },

		// Delete calendar note mode
		{ "deletecalendarnote", required_argument, NULL, OPT_DELCALENDARNOTE },

		// Get display status mode
		{ "getdisplaystatus",   no_argument,       NULL, OPT_GETDISPLAYSTATUS },

		// Get memory mode
		{ "getmemory",          required_argument, NULL, OPT_GETMEMORY },

		// Write phonebook (memory) mode
		{ "writephonebook",     optional_argument, NULL, OPT_WRITEPHONEBOOK },

		// Get speed dial mode
		{ "getspeeddial",       required_argument, NULL, OPT_GETSPEEDDIAL },

		// Set speed dial mode
		{ "setspeeddial",       required_argument, NULL, OPT_SETSPEEDDIAL },

		// Get SMS message mode
		{ "getsms",             required_argument, NULL, OPT_GETSMS },

		// Delete SMS message mode
		{ "deletesms",          required_argument, NULL, OPT_DELETESMS },

		// Send SMS message mode
		{ "sendsms",            required_argument, NULL, OPT_SENDSMS },

		// Ssve SMS message mode
		{ "savesms",            optional_argument, NULL, OPT_SAVESMS },

		// Send logo as SMS message mode
		{ "sendlogo",           required_argument, NULL, OPT_SENDLOGO },

		// Send ringtone as SMS message
		{ "sendringtone",       required_argument, NULL, OPT_SENDRINGTONE },

		// Set ringtone
		{ "setringtone",        required_argument, NULL, OPT_SETRINGTONE },

		// Get SMS center number mode
		{ "getsmsc",            required_argument, NULL, OPT_GETSMSC },

		// For development purposes: run in passive monitoring mode
		{ "pmon",               no_argument,       NULL, OPT_PMON },

		// NetMonitor mode
		{ "netmonitor",         required_argument, NULL, OPT_NETMONITOR },

		// Identify
		{ "identify",           no_argument,       NULL, OPT_IDENTIFY },

		// Send DTMF sequence
		{ "senddtmf",           required_argument, NULL, OPT_SENDDTMF },

		// Resets the phone
		{ "reset",              optional_argument, NULL, OPT_RESET },

		// Set logo
		{ "setlogo",            optional_argument, NULL, OPT_SETLOGO },

		// Get logo
		{ "getlogo",            required_argument, NULL, OPT_GETLOGO },

		// View logo
		{ "viewlogo",           required_argument, NULL, OPT_VIEWLOGO },

		// Show profile
		{ "getprofile",         optional_argument, NULL, OPT_GETPROFILE },

		// Show texts from phone's display
		{ "displayoutput",      no_argument,       NULL, OPT_DISPLAYOUTPUT },

		// Simulate pressing the keys
		{ "keysequence",        no_argument,       NULL, OPT_KEYPRESS },
    
		// For development purposes: insert you function calls here
		{ "foogle",             no_argument,       NULL, OPT_FOOGLE },

		{ 0, 0, 0, 0},
	};

	/* Every command which requires arguments should have an appropriate entry
	   in this array. */
	struct gnokii_arg_len gals[] =
	{

#ifdef SECURITY
		{ OPT_ENTERSECURITYCODE, 1, 1, 0 },
#endif

		{ OPT_SETDATETIME,       0, 5, 0 },
		{ OPT_SETALARM,          2, 2, 0 },
		{ OPT_DIALVOICE,         1, 1, 0 },
		{ OPT_GETCALENDARNOTE,   1, 2, 0 },
		{ OPT_WRITECALENDARNOTE, 2, 2, 0 },
		{ OPT_DELCALENDARNOTE,   1, 1, 0 },
		{ OPT_GETMEMORY,         2, 3, 0 },
		{ OPT_GETSPEEDDIAL,      1, 1, 0 },
		{ OPT_SETSPEEDDIAL,      3, 3, 0 },
		{ OPT_GETSMS,            2, 5, 0 },
		{ OPT_DELETESMS,         2, 3, 0 },
		{ OPT_SENDSMS,           1, 10, 0 },
		{ OPT_SAVESMS,           0, 6, 0 },
		{ OPT_SENDLOGO,          3, 4, GAL_XOR },
		{ OPT_SENDRINGTONE,      2, 2, 0 },
		{ OPT_GETSMSC,           1, 1, 0 },
		{ OPT_GETWELCOMENOTE,    1, 1, 0 },
		{ OPT_SETWELCOMENOTE,    1, 1, 0 },
		{ OPT_NETMONITOR,        1, 1, 0 },
		{ OPT_SENDDTMF,          1, 1, 0 },
		{ OPT_SETLOGO,           1, 4, 0 },
		{ OPT_GETLOGO,           1, 4, 0 },
		{ OPT_VIEWLOGO,          1, 1, 0 },
		{ OPT_SETRINGTONE,       1, 1, 0 },
		{ OPT_RESET,             0, 1, 0 },
		{ OPT_GETPROFILE,        0, 1, 0 },
		{ OPT_WRITEPHONEBOOK,    0, 1, 0 },

		{ 0, 0, 0, 0 },
	};

	opterr = 0;

	/* For GNU gettext */
#ifdef USE_NLS
	textdomain("gnokii");
	setlocale(LC_ALL, "");
#endif

	/* Read config file */
	if (readconfig(&model, &Port, &Initlength, &Connection, &BinDir) < 0) {
		exit(-1);
	}

	/* Handle command line arguments. */
	c = getopt_long(argc, argv, "", long_options, NULL);
	if (c == -1) 		/* No argument given - we should display usage. */
		usage();

	/* We have to build an array of the arguments which will be passed to the
	   functions.  Please note that every text after the --command will be
	   passed as arguments.  A syntax like gnokii --cmd1 args --cmd2 args will
	   not work as expected; instead args --cmd2 args is passed as a
	   parameter. */
	if((nargv = malloc(sizeof(char *) * argc)) != NULL) {
		for(i = 2; i < argc; i++)
			nargv[i-2] = argv[i];
	
		if(checkargs(c, gals, nargc)) {
			free(nargv); /* Wrong number of arguments - we should display usage. */
			usage();
		}

#ifdef __svr4__
		/* have to ignore SIGALARM */
		sigignore(SIGALRM);
#endif

		switch(c) {
		// First, error conditions
		case '?':
		case ':':
			fprintf(stderr, _("Use '%s --help' for usage informations.\n"), argv[0]);
			break;
		// Then, options with no arguments
		case OPT_HELP:
			usage();
		case OPT_VERSION:
			return version();
		}

		/* Initialise the code for the GSM interface. */     
		fbusinit(NULL);

		switch(c) {
		case OPT_MONITOR:
			rc = monitormode();
			break;
#ifdef SECURITY
		case OPT_ENTERSECURITYCODE:
			rc = entersecuritycode(optarg);
			break;
		case OPT_GETSECURITYCODESTATUS:
			rc = getsecuritycodestatus();
			break;
#endif
		case OPT_GETDATETIME:
			rc = getdatetime();
			break;
		case OPT_GETALARM:
			rc = getalarm();
			break;
		case OPT_GETDISPLAYSTATUS:
			rc = getdisplaystatus();
			break;
		case OPT_PMON:
			rc = pmon();
			break;
		case OPT_WRITEPHONEBOOK:
			rc = writephonebook(nargc, nargv);
			break;
		// Now, options with arguments
		case OPT_SETDATETIME:
			rc = setdatetime(nargc, nargv);
			break;
		case OPT_SETALARM:
			rc = setalarm(nargv);
			break;
		case OPT_DIALVOICE:
			rc = dialvoice(optarg);
			break;
		case OPT_GETCALENDARNOTE:
			rc = getcalendarnote(nargc, nargv);
			break;
		case OPT_DELCALENDARNOTE:
			rc = deletecalendarnote(optarg);
			break;
		case OPT_WRITECALENDARNOTE:
			rc = writecalendarnote(nargv);
			break;
		case OPT_GETMEMORY:
			rc = getmemory(nargc, nargv);
			break;
		case OPT_GETSPEEDDIAL:
			rc = getspeeddial(optarg);
			break;
		case OPT_SETSPEEDDIAL:
			rc = setspeeddial(nargv);
			break;
		case OPT_GETSMS:
			rc = getsms(argc, argv);
			break;
		case OPT_DELETESMS:
			rc = deletesms(nargc, nargv);
			break;
		case OPT_SENDSMS:
			rc = sendsms(nargc, nargv);
			break;
		case OPT_SAVESMS:
			rc = savesms(argc, argv);
			break;
		case OPT_SENDLOGO:
			rc = sendlogo(nargc, nargv);
			break;
		case OPT_GETSMSC:
			rc = getsmsc(optarg);
			break;
		case OPT_NETMONITOR:
			rc = netmonitor(optarg);
			break;
		case OPT_IDENTIFY:
			rc = identify();
			break;
		case OPT_SETLOGO:
			rc = setlogo(nargc, nargv);
			break;
		case OPT_GETLOGO:
			rc = getlogo(nargc, nargv);
			break;
		case OPT_VIEWLOGO:
			rc = viewlogo(optarg);
			break;
		case OPT_SETRINGTONE:
			rc = setringtone(nargc, nargv);
			break;
		case OPT_SENDRINGTONE:
			rc = sendringtone(nargc, nargv);
			break;
		case OPT_GETPROFILE:
			rc = getprofile(nargc, nargv);
			break;
		case OPT_DISPLAYOUTPUT:
			rc = displayoutput();
			break;
		case OPT_KEYPRESS:
			rc = presskeysequence();
			break;
#ifndef WIN32
		case OPT_FOOGLE:
			rc = foogle(nargv);
			break;
#endif
		case OPT_SENDDTMF:
			rc = senddtmf(optarg);
			break;
		case OPT_RESET:
			rc = reset(optarg);
			break;
		default:
			fprintf(stderr, _("Unknown option: %d\n"), c);
			break;

		}
		return(rc);
	}

	fprintf(stderr, _("Wrong number of arguments\n"));
	exit(-1);
}

/* Send  SMS messages. */
int sendsms(int argc, char *argv[])
{
	GSM_SMSMessage SMS;
	GSM_Error error;
	char UDH[GSM_MAX_USER_DATA_HEADER_LENGTH];
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[GSM_MAX_CONCATENATED_SMS_LENGTH];
	int input_len, chars_read;
	int i, offset, nr_msg, aux;

	struct option options[] = {
		{ "smsc",    required_argument, NULL, '1'},
		{ "smscno",  required_argument, NULL, '2'},
		{ "long",	  required_argument, NULL, '3'},
		{ NULL,      0,                 NULL, 0}
	};

	input_len = GSM_MAX_SMS_LENGTH;

	/* Default settings:
	   - no delivery report
	   - no Class Message
	   - no compression
	   - 7 bit data
	   - SMSC no. 1
	   - message validity for 3 days
	   - unset user data header indicator
	*/

	SMS.Type = GST_MO;
	SMS.Class = -1;
	SMS.Compression = false;
	SMS.EightBit = false;
	SMS.MessageCenter.No = 1;
	SMS.Validity = 4320; /* 4320 minutes == 72 hours */
	SMS.UDHType = GSM_NoUDH;

	strcpy(SMS.Destination,argv[0]);

	optarg = NULL;
	optind = 0;

	while ((i = getopt_long(argc, argv, "r8cC:v:", options, NULL)) != -1) {
		switch (i) {       // -8 is for 8-bit data, -c for compression. both are not yet implemented.
		case '1': /* SMSC number */
			SMS.MessageCenter.No = 0;
			strcpy(SMS.MessageCenter.Number,optarg);
			break;
		case '2': /* SMSC number index in phone memory */
			SMS.MessageCenter.No = atoi(optarg);

			if (SMS.MessageCenter.No < 1 || SMS.MessageCenter.No > 5)
				usage();
			break;
		case '3': /* we send long message */
			input_len = atoi(optarg);
			if (input_len > GSM_MAX_CONCATENATED_SMS_LENGTH) {
				fprintf(stderr, _("Input too long!\n"));	
				exit(-1);
			}
			break;
		case 'r': /* request for delivery report */
			SMS.Type = GST_DR;
			break;
		case 'C': /* class Message */
			switch (*optarg) {
			case '0':
				SMS.Class = 0;
				break;
			case '1':
				SMS.Class = 1;
				break;
			case '2':
				SMS.Class = 2;
				break;
			case '3':
				SMS.Class = 3;
				break;
			default:
				usage();
			}
			break;
		case 'v':
			SMS.Validity = atoi(optarg);
			break;
		default:
			usage(); // Would be better to have an sendsms_usage() here.
		}
	}

	/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, input_len, stdin);

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));	
		return -1;
	} else if (chars_read > input_len) {
		fprintf(stderr, _("Input too long!\n"));	
		return -1;
	}

	/*  Null terminate. */
	message_buffer[chars_read] = 0x00;	

	/* We send concatenated SMS if and only if we have more then
	   GSM_MAX_SMS_LENGTH characters to send */
	if (chars_read > GSM_MAX_SMS_LENGTH) {
		SMS.UDHType = GSM_ConcatenatedMessages;
	}

	/* offset - length of the UDH */
	/* nr_msg - number of the messages to send */
	offset = 0;
	nr_msg = 1;
	switch (SMS.UDHType) {
  	case GSM_NoUDH:
		break;
    	case GSM_ConcatenatedMessages:
		UDH[0] = 0x05;	// UDH length
		UDH[1] = 0x00;	// concatenated messages (IEI)
		UDH[2] = 0x03;	// IEI data length
		UDH[3] = 0x01;	// reference number
		UDH[4] = 0x00;	// number of messages
		UDH[5] = 0x00;	// message reference number
		offset = 6;
		/* only 153 chars in single uncompressed message */
		nr_msg += chars_read/153;
		break;
    	default:
		break;
	}

	UDH[4] = nr_msg;	// number of messages
	/* Initialise the GSM interface. */     

	for (i = 0; i < nr_msg; i++) {

		UDH[5] = i + 1;	// message reference number
		aux = i*153;	// aux - how many character we have already sent

		if (SMS.UDHType) {
			memcpy(SMS.UDH,UDH,offset);
			strncpy (SMS.MessageText, message_buffer+aux, 153);
			SMS.MessageText[153] = 0;
		} else {
			strncpy (SMS.MessageText,message_buffer,chars_read);
			SMS.MessageText[chars_read] = 0;
		}

		/* Send the message. */
		error = GSM->SendSMSMessage(&SMS,0);

		if (error == GE_SMSSENDOK) {
			fprintf(stdout, _("Send succeeded!\n"));
		} else {
			fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);
			break;
		}
		sleep(10);
	}

	GSM->Terminate();

	return 0;
}

int savesms(int argc, char *argv[])
{
	GSM_SMSMessage SMS;
	GSM_Error error;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[GSM_MAX_CONCATENATED_SMS_LENGTH];
	int input_len, chars_read;
	int i, confirm = -1;
	int interactive = 0;
	char ans[8];

	/* Defaults */
	SMS.Type = GST_MO;
	SMS.Class = -1;
	SMS.Compression = false;
	SMS.EightBit = false;
	SMS.MessageCenter.No = 1;
	SMS.Validity = 4320; /* 4320 minutes == 72 hours */
	SMS.UDHType = GSM_NoUDH;
	SMS.Status = GSS_NOTSENTREAD;
	SMS.Location = 0;

	input_len = GSM_MAX_SMS_LENGTH;

	/* Option parsing */
	while ((i = getopt(argc, argv, "ml:in:s:c:")) != -1) {
		switch (i) {
		case 'm': /* mark the message as sent */
			SMS.Status = GSS_SENTREAD;
			break;
		case 'l': /* Specify the location */
			SMS.Location = atoi(optarg);
			break;
		case 'i': /* Ask before overwriting */
			interactive = 1;
			break;
		case 'n': /* Specify the from number */
			break;
		case 's': /* Specify the smsc number */
			break;
		case 'c': /* Specify the smsc location */
			break;
		default:
			usage();
		}
	}

	if (interactive) {
		GSM_SMSMessage aux;

		aux.Location = SMS.Location;
		data.SMSMessage = &aux;
		error = SM_Functions(GOP_GetSMS, &data, &State);
		switch (error) {
		case GE_NONE:
			fprintf(stderr, _("Message at specified location exists. "));
			while (confirm < 0) {
				fprintf(stderr, _("Overwrite? (yes/no) "));
				GetLine(stdin, ans, 7);
				if (!strcmp(ans, "yes")) confirm = 1;
				else if (!strcmp(ans, "no")) confirm = 0;
			}  
			if (!confirm) { GSM->Terminate(); return 0; }
			else break;
		case GE_INVALIDSMSLOCATION:
			fprintf(stderr, _("Invalid location\n"));
			GSM->Terminate();
			return -1;
		default:
/* FIXME: Remove this fprintf when the function is thoroughly tested */
#ifdef DEBUG
			fprintf(stderr, _("Location %d empty. Saving\n"), SMS.Location);
#endif
			break;
		}
	}
	chars_read = fread(message_buffer, 1, input_len, stdin);

	if (chars_read == 0) {

		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return -1;

	} else if (chars_read > input_len) {

		fprintf(stderr, _("Input too long!\n"));
		return -1;

	}

	strncpy (SMS.MessageText, message_buffer, chars_read);
	SMS.MessageText[chars_read] = 0;

	error = GSM->SaveSMSMessage(&SMS);

	if (error == GE_NONE) {
		fprintf(stdout, _("Saved!\n"));
	} else {
		fprintf(stdout, _("Saving failed (error=%d)\n"), error);
	}
	sleep(10);
	GSM->Terminate();

	return 0;
}

/* Get SMSC number */
int getsmsc(char *MessageCenterNumber)
{
	GSM_MessageCenter MessageCenter;

	MessageCenter.No=atoi(MessageCenterNumber);

	if (GSM->GetSMSCenter(&MessageCenter) == GE_NONE) {

		fprintf(stdout, _("%d. SMS center (%s) number is %s\n"), MessageCenter.No, MessageCenter.Name, MessageCenter.Number);
		fprintf(stdout, _("Messages sent as "));

		switch (MessageCenter.Format) {
		case GSMF_Text:
			fprintf(stdout, _("Text"));
			break;
		case GSMF_Paging:
			fprintf(stdout, _("Paging"));
			break;
		case GSMF_Fax:
			fprintf(stdout, _("Fax"));
			break;
		case GSMF_Email:
		case GSMF_UCI:
			fprintf(stdout, _("Email"));
			break;
		case GSMF_ERMES:
			fprintf(stdout, _("ERMES"));
			break;
		case GSMF_X400:
			fprintf(stdout, _("X.400"));
			break;
		default:
			fprintf(stdout, _("Unknown"));
			break;
		}

		printf("\n");
		fprintf(stdout, _("Message validity is "));

		switch (MessageCenter.Validity) {
		case GSMV_1_Hour:
			fprintf(stdout, _("1 hour"));
			break;
		case GSMV_6_Hours:
			fprintf(stdout, _("6 hours"));
			break;
		case GSMV_24_Hours:
			fprintf(stdout, _("24 hours"));
			break;
		case GSMV_72_Hours:
			fprintf(stdout, _("72 hours"));
			break;
		case GSMV_1_Week:
			fprintf(stdout, _("1 week"));
			break;
		case GSMV_Max_Time:
			fprintf(stdout, _("Maximum time"));
			break;
		default:
			fprintf(stdout, _("Unknown"));
			break;
		}

		fprintf(stdout, "\n");

	} else
		fprintf(stdout, _("SMS center can not be found :-(\n"));

	GSM->Terminate();

	return 0;
}

/* Get SMS messages. */
int getsms(int argc, char *argv[])
{
	int del = 0;
	GSM_SMSMessage message;
	char *memory_type_string;
	int start_message, end_message, count, mode = 1;
	char filename[64];
	GSM_Error error;
	GSM_Bitmap bitmap;
	char ans[5];
	struct stat buf;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[2];
	message.MemoryType = StrToMemoryType(memory_type_string);
	if (message.MemoryType == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[2]);
		return (-1);
	}

	memset(&filename, 0, 64);

	start_message = atoi(argv[3]);
	if (argc > 4) {
		int i;

		/* [end] can be only argv[4] */
		if (argv[4][0] == '-') {
			end_message = start_message;
		} else {
			end_message = atoi(argv[4]);
		}

		/* parse all options (beginning with '-' */
		while ((i = getopt(argc, argv, "f:F:d")) != -1) {
			switch (i) {
			case 'd':
				del = 1;
				break;
			case 'F':
				mode = 0;
			case 'f':
				if (optarg) {
					dprintf(_("Saving into %s\n"), optarg);
					strncpy(filename, optarg, 64);
					if (strlen(optarg) > 63) {
						fprintf(stderr, _("Filename too long - will be truncated to 63 characters.\n"));
						filename[63] = 0;
					} else {
						filename[strlen(optarg)] = 0;
					}
				} else  usage();
				break;
			default:
				usage();
			}
		}
	} else {
		end_message = start_message;
	}

	/* Now retrieve the requested entries. */
	for (count = start_message; count <= end_message; count ++) {

		message.Location = count;
		data.SMSMessage = &message;
		error = SM_Functions(GOP_GetSMS, &data, &State);

		switch (error) {
		case GE_NONE:
			switch (message.Type) {
			case GST_MO:
				fprintf(stdout, _("%d. Outbox Message "), message.MessageNumber);
				if (message.Status)
					fprintf(stdout, _("(sent)\n"));
				else
					fprintf(stdout, _("(not sent)\n"));
				fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 
				break;
			case GST_DR:
				fprintf(stdout, _("%d. Delivery Report "), message.MessageNumber);
				if (message.Status)
					fprintf(stdout, _("(read)\n"));
				else
					fprintf(stdout, _("(not read)\n"));
				fprintf(stdout, _("Sending date/time: %d/%d/%d %d:%02d:%02d "), \
					message.Time.Day, message.Time.Month, message.Time.Year, \
					message.Time.Hour, message.Time.Minute, message.Time.Second);
				if (message.Time.Timezone) {
					if (message.Time.Timezone > 0)
						fprintf(stdout,_("+%02d00"), message.Time.Timezone);
					else
						fprintf(stdout,_("%02d00"), message.Time.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Response date/time: %d/%d/%d %d:%02d:%02d "), \
					message.SMSCTime.Day, message.SMSCTime.Month, message.SMSCTime.Year, \
					message.SMSCTime.Hour, message.SMSCTime.Minute, message.SMSCTime.Second);
				if (message.SMSCTime.Timezone) {
					if (message.SMSCTime.Timezone > 0)
						fprintf(stdout,_("+%02d00"),message.SMSCTime.Timezone);
					else
						fprintf(stdout,_("%02d00"),message.SMSCTime.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Receiver: %s Msg Center: %s\n"), message.Sender, message.MessageCenter.Number);
				fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 
				break;
			default:
				fprintf(stdout, _("%d. Inbox Message "), message.MessageNumber);
				if (message.Status)
					fprintf(stdout, _("(read)\n"));
				else
					fprintf(stdout, _("(not read)\n"));
				fprintf(stdout, _("Date/time: %d/%d/%d %d:%02d:%02d "), \
					message.Time.Day, message.Time.Month, message.Time.Year, \
					message.Time.Hour, message.Time.Minute, message.Time.Second);
				if (message.Time.Timezone) {
					if (message.Time.Timezone > 0)
						fprintf(stdout,_("+%02d00"),message.Time.Timezone);
					else
						fprintf(stdout,_("%02d00"),message.Time.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.Sender, message.MessageCenter.Number);
				switch (message.UDHType) {
				case GSM_OpLogo:
					fprintf(stdout, _("GSM operator logo for %s (%s) network.\n"), bitmap.netcode, GSM_GetNetworkName(bitmap.netcode));
					if (!strcmp(message.Sender, "+998000005") && !strcmp(message.MessageCenter.Number, "+886935074443")) dprintf(_("Saved by Logo Express\n"));
					if (!strcmp(message.Sender, "+998000002") || !strcmp(message.Sender, "+998000003")) dprintf(_("Saved by Operator Logo Uploader by Thomas Kessler\n"));
				case GSM_CallerIDLogo:
					fprintf(stdout, ("Logo:\n"));
					/* put bitmap into bitmap structure */
					GSM_ReadSMSBitmap(&message, &bitmap);
					GSM_PrintBitmap(&bitmap);
					if (*filename) {
						error = GE_NONE;
						if ((stat(filename, &buf) == 0)) {
							fprintf(stdout, _("File %s exists.\n"), filename);
							fprintf(stderr, _("Overwrite? (yes/no) "));
							GetLine(stdin, ans, 4);
							if (!strcmp(ans, "yes")) {
								error = GSM_SaveBitmapFile(filename, &bitmap);
							}
						} else error = GSM_SaveBitmapFile(filename, &bitmap);	       
						if (error!=GE_NONE) fprintf(stderr, _("Couldn't save logofile %s!\n"), filename);
					}
					break;
				case GSM_RingtoneUDH:
					fprintf(stdout, ("Ringtone\n"));
					break;
				case GSM_ConcatenatedMessages:
					fprintf(stdout, _("Linked (%d/%d):\n"),message.UDH[5],message.UDH[4]);
				case GSM_NoUDH:
					fprintf(stdout, _("Text:\n%s\n\n"), message.MessageText);
					if ((mode != -1) && *filename) {
						char buf[1024];
						sprintf(buf, "%s%d", filename, count);
						mode = GSM_SaveTextFile(buf, message.MessageText, mode);
					}
					break;
				default:
					fprintf(stderr, _("Unknown\n"));
					break;
				}
				break;
			}
			if (del) {
				data.SMSMessage = &message;
				if (GE_NONE != SM_Functions(GOP_DeleteSMS, &data, &State))
					fprintf(stdout, _("(delete failed)\n"));
				else
					fprintf(stdout, _("(message deleted)\n"));
			}
			break;
		case GE_NOTIMPLEMENTED:
			fprintf(stderr, _("Function not implemented in %s model!\n"), model);
			GSM->Terminate();
			return -1;	
		case GE_INVALIDSMSLOCATION:
			fprintf(stderr, _("Invalid location: %s %d\n"), memory_type_string, count);
			break;
		case GE_EMPTYSMSLOCATION:
			fprintf(stderr, _("SMS location %s %d empty.\n"), memory_type_string, count);
			break;
		default:
			fprintf(stdout, _("GetSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
			break;
		}
	}

	GSM->Terminate();

	return 0;
}

/* Delete SMS messages. */
int deletesms(int argc, char *argv[])
{
	GSM_SMSMessage message;
	char *memory_type_string;
	int start_message, end_message, count;
	GSM_Error error;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	message.MemoryType = StrToMemoryType(memory_type_string);
	if (message.MemoryType == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return (-1);
	}
                
	start_message = atoi (argv[1]);
	if (argc > 2) end_message = atoi (argv[2]);
	else end_message = start_message;


	/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count ++) {

		message.Location = count;
		data.SMSMessage = &message;
		error = SM_Functions(GOP_DeleteSMS, &data, &State);

		if (error == GE_NONE)
			fprintf(stdout, _("Deleted SMS %s %d\n"), memory_type_string, count);
		else {
			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), model);
				GSM->Terminate();
				return -1;	
			}
			fprintf(stdout, _("DeleteSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
		}
	}

	GSM->Terminate();

	return 0;
}

static volatile bool bshutdown = false;

/* SIGINT signal handler. */
static void interrupted(int sig)
{
	signal(sig, SIG_IGN);
	bshutdown = true;
}

#ifdef SECURITY

/* In this mode we get the code from the keyboard and send it to the mobile
   phone. */
int entersecuritycode(char *type)
{
	GSM_Error test;
	GSM_SecurityCode SecurityCode;

	if (!strcmp(type,"PIN"))
		SecurityCode.Type=GSCT_Pin;
	else if (!strcmp(type,"PUK"))
		SecurityCode.Type=GSCT_Puk;
	else if (!strcmp(type,"PIN2"))
		SecurityCode.Type=GSCT_Pin2;
	else if (!strcmp(type,"PUK2"))
		SecurityCode.Type=GSCT_Puk2;
	// FIXME: Entering of SecurityCode does not work :-(
	//  else if (!strcmp(type,"SecurityCode"))
	//    SecurityCode.Type=GSCT_SecurityCode;
	else    usage();

#ifdef WIN32
	printf("Enter your code: ");
	gets(SecurityCode.Code);
#else
	strcpy(SecurityCode.Code,getpass(_("Enter your code: ")));
#endif

	test = GSM->EnterSecurityCode(SecurityCode);
	if (test == GE_INVALIDSECURITYCODE)
		fprintf(stdout, _("Error: invalid code.\n"));
	else if (test == GE_NONE)
		fprintf(stdout, _("Code ok.\n"));
	else if (test == GE_NOTIMPLEMENTED)
		fprintf(stderr, _("Function not implemented in %s model!\n"), model);
	else
		fprintf(stdout, _("Other error.\n"));

	GSM->Terminate();

	return 0;
}

int getsecuritycodestatus(void)
{
	int Status;

	if (GSM->GetSecurityCodeStatus(&Status) == GE_NONE) {

		fprintf(stdout, _("Security code status: "));

		switch(Status) {
		case GSCT_SecurityCode:
			fprintf(stdout, _("waiting for Security Code.\n"));
			break;
		case GSCT_Pin:
			fprintf(stdout, _("waiting for PIN.\n"));
			break;
		case GSCT_Pin2:
			fprintf(stdout, _("waiting for PIN2.\n"));
			break;
		case GSCT_Puk:
			fprintf(stdout, _("waiting for PUK.\n"));
			break;
		case GSCT_Puk2:
			fprintf(stdout, _("waiting for PUK2.\n"));
			break;
		case GSCT_None:
			fprintf(stdout, _("nothing to enter.\n"));
			break;
		default:
			fprintf(stdout, _("Unknown!\n"));
			break;
		}
	}

	GSM->Terminate();

	return 0;
}


#endif

/* Voice dialing mode. */
int dialvoice(char *Number)
{
	GSM->DialVoice(Number);

	GSM->Terminate();

	return 0;
}

/* The following function allows to send logos using SMS */
int sendlogo(int argc, char *argv[])
{
	GSM_SMSMessage SMS;
	GSM_Bitmap bitmap;
	GSM_Error error;

	char UserDataHeader[7] = {	0x06, /* UDH Length */
					0x05, /* IEI: application port addressing scheme, 16 bit address */
					0x04, /* IEI length */
					0x15, /* destination address: high byte */
					0x00, /* destination address: low byte */
					0x00, /* originator address */
					0x00};

	char Data[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	int current=0;

	/* Default settings for SMS message:
	   - no delivery report
	   - Class Message 1
	   - no compression
	   - 8 bit data
	   - SMSC no. 1
	   - validity 3 days
	   - set UserDataHeaderIndicator
	*/

	SMS.Type = GST_MO;
	SMS.Class = 1;
	SMS.Compression = false;
	SMS.EightBit = true;
	SMS.MessageCenter.No = 1;
	SMS.Validity = 4320; /* 4320 minutes == 72 hours */

	/* The first argument is the type of the logo. */
	if (!strcmp(argv[0], "op")) {
		SMS.UDHType = GSM_OpLogo;
		UserDataHeader[4] = 0x82; /* NBS port 0x1582 */
		fprintf(stdout, _("Sending operator logo.\n"));
	} else if (!strcmp(argv[0], "caller")) {
		SMS.UDHType = GSM_CallerIDLogo;
		UserDataHeader[4] = 0x83; /* NBS port 0x1583 */
		fprintf(stdout, _("Sending caller line identification logo.\n"));
	} else {
		fprintf(stderr, _("You should specify what kind of logo to send!\n"));
		return (-1);
	}

	/* The second argument is the destination, ie the phone number of recipient. */
	strcpy(SMS.Destination,argv[1]);

	/* The third argument is the bitmap file. */
	GSM_ReadBitmapFile(argv[2], &bitmap);

	/* If we are sending op logo we can rewrite network code. */
	if (!strcmp(argv[0], "op")) {
		/*
		 * The fourth argument, if present, is the Network code of the operator.
		 * Network code is in this format: "xxx yy".
		 */
		if (argc > 3) {
			strcpy(bitmap.netcode, argv[3]);
#ifdef DEBUG
			fprintf(stdout, _("Operator code: %s\n"), argv[3]);
#endif
		}

		/* Set the network code */
		Data[current++] = ((bitmap.netcode[1] & 0x0f) << 4) | (bitmap.netcode[0] & 0xf);
		Data[current++] = 0xf0 | (bitmap.netcode[2] & 0x0f);
		Data[current++] = ((bitmap.netcode[5] & 0x0f) << 4) | (bitmap.netcode[4] & 0xf);
	}

	/* Set the logo size */
	current++;
	Data[current++] = bitmap.width;
	Data[current++] = bitmap.height;

	Data[current++] = 0x01;

	memcpy(SMS.UDH,UserDataHeader,7);
	memcpy(SMS.MessageText,Data,current);
	memcpy(SMS.MessageText+current,bitmap.bitmap,bitmap.size);

	/* Send the message. */
	error = GSM->SendSMSMessage(&SMS,current+bitmap.size);

	if (error == GE_SMSSENDOK)
		fprintf(stdout, _("Send succeeded!\n"));
	else
		fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

	GSM->Terminate();
	return 0;
}

/* Getting logos. */
GSM_Error SaveBitmapFileOnConsole(char *FileName, GSM_Bitmap *bitmap)
{
	int confirm;
	char ans[4];
	struct stat buf;
	GSM_Error error;

	/* Ask before overwriting */
	while (stat(FileName, &buf) == 0) {
		confirm = 0;
		while (!confirm) {
			fprintf(stderr, _("Saving logo. File \"%s\" exists. (O)verwrite, create (n)ew or (s)kip ? "), FileName);
			GetLine(stdin, ans, 4);
			if (!strcmp(ans, "O") || !strcmp(ans, "o")) confirm = 1;
			if (!strcmp(ans, "N") || !strcmp(ans, "n")) confirm = 2;
			if (!strcmp(ans, "S") || !strcmp(ans, "s")) return GE_USERCANCELED;
		}  
		if (confirm == 1) break;
		if (confirm == 2) {
			fprintf(stderr, _("Enter name of new file: "));
			GetLine(stdin, FileName, 50);
			if (!FileName || (*FileName == 0)) return GE_USERCANCELED;
		}
	}
  
	error = GSM_SaveBitmapFile(FileName, bitmap);
  
	switch (error) {
	case GE_CANTOPENFILE:
		fprintf(stderr, _("Failed to write file \"%s\"\n"), FileName);
		break;
	default:
		break;
	}
  
	return error;
}

int getlogo(int argc, char *argv[])
{
	GSM_Bitmap bitmap;
	GSM_Error error;
	GSM_Statemachine *sm = &State;

	bitmap.type=GSM_None;

	if (!strcmp(argv[0], "op"))
		bitmap.type = GSM_OperatorLogo;
    
	if (!strcmp(argv[0], "caller")) {
		/* There is caller group number missing in argument list. */
		if (argc == 3) {     
			bitmap.number=argv[2][0]-'0';
			if ((bitmap.number < 0) || (bitmap.number > 9)) bitmap.number = 0;
		} else {
			bitmap.number = 0;
		}
		bitmap.type = GSM_CallerLogo;
	}
    
	if (!strcmp(argv[0],"startup"))
		bitmap.type = GSM_StartupLogo;
    	else if (!strcmp(argv[0], "dealer"))
		bitmap.type = GSM_DealerNoteText;  
    	else if (!strcmp(argv[0], "text"))
		bitmap.type=GSM_WelcomeNoteText;  

	if (bitmap.type != GSM_None) {
    
		fprintf(stdout, _("Getting Logo\n"));
        
		data.Bitmap=&bitmap;
		error=SM_Functions(GOP_GetBitmap, &data, sm);
 
		switch (error) {
		case GE_NONE:
			if (bitmap.type == GSM_DealerNoteText) fprintf(stdout, _("Dealer welcome note "));
			if (bitmap.type == GSM_WelcomeNoteText) fprintf(stdout, _("Welcome note "));	
			if (bitmap.type == GSM_DealerNoteText || bitmap.type==GSM_WelcomeNoteText) {
				if (bitmap.text[0]) {
					fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				} else {
					fprintf(stdout, _("currently empty\n"));
				}
			} else {
				if (bitmap.width) {
					switch (bitmap.type) {
					case GSM_OperatorLogo:
						fprintf(stdout,"Operator logo for %s (%s) network got succesfully\n",bitmap.netcode,GSM_GetNetworkName(bitmap.netcode));
						if (argc==3) {
							strncpy(bitmap.netcode,argv[2], 7);
							if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
								fprintf(stderr, "Sorry, gnokii doesn't know %s network !\n", bitmap.netcode);
								return -1;
							}
						}
						break;
					case GSM_StartupLogo:
						fprintf(stdout, "Startup logo got successfully\n");
						if (argc == 3) {
							strncpy(bitmap.netcode,argv[2], 7);
							if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
								fprintf(stderr, "Sorry, gnokii doesn't know %s network !\n", bitmap.netcode);
								return -1;
							}
						}
						break;
					case GSM_CallerLogo:
						fprintf(stdout,"Caller logo got successfully\n");
						if (argc == 4) {
							strncpy(bitmap.netcode,argv[3],7);
							if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
								fprintf(stderr, "Sorry, gnokii doesn't know %s network !\n", bitmap.netcode);
								return -1;
							}
						}
						break;
					default:
						fprintf(stdout,"Unknown bitmap type.\n");
						break;
					if (argc > 1) {
						if (SaveBitmapFileOnConsole(argv[1], &bitmap) != GE_NONE) return (-1);
					}
					}
				} else {
					fprintf(stdout,"Your phone doesn't have logo uploaded !\n");
					return -1;
				}
			}
			break;
		case GE_NOTIMPLEMENTED:
			fprintf(stderr, _("Function not implemented !\n"));
			return -1;
		case GE_NOTSUPPORTED:
			fprintf(stderr, _("This kind of logo is not supported !\n"));
			return -1;
		default:
			fprintf(stderr, _("Error getting logo !\n"));
			return -1;
		}
	} else {
		fprintf(stderr, _("What kind of logo do you want to get ?\n"));
		return -1;
	}

	return 0;
}


/* Sending logos. */
GSM_Error ReadBitmapFileOnConsole(char *FileName, GSM_Bitmap *bitmap)
{
	GSM_Error error;
  
	error = GSM_ReadBitmapFile(FileName, bitmap);
  
	switch (error) {
	case GE_CANTOPENFILE:
		fprintf(stderr, _("Failed to read file \"%s\"\n"), FileName);
		break;
	case GE_WRONGNUMBEROFCOLORS:
		fprintf(stderr, _("Wrong number of colors in \"%s\" logofile (accepted only 2-colors files) !\n"), FileName);
		break;
	case GE_WRONGCOLORS:
		fprintf(stderr, _("Wrong colors in \"%s\" logofile !\n"), FileName);
		break;
	case GE_INVALIDFILEFORMAT:
		fprintf(stderr, _("Invalid format of \"%s\" logofile !\n"), FileName);
		break;
	case GE_SUBFORMATNOTSUPPORTED:
		fprintf(stderr, _("Sorry, gnokii doesn't support used subformat in file \"%s\" !\n"), FileName);
		break;
	case GE_FILETOOSHORT:
		fprintf(stderr, _("\"%s\" logofile is too short !\n"), FileName);
		break;  
	case GE_INVALIDIMAGESIZE:
		fprintf(stderr, _("Bitmap size doesn't supported by fileformat or different from 72x14, 84x48 and 72x28 !\n"));
		break;
	default:
		break;
	}
  
	return error;
}


int setlogo(int argc, char *argv[])
{
	GSM_Bitmap bitmap,oldbit;
	GSM_NetworkInfo NetworkInfo;
	GSM_Error error;
  
	bool ok = true;
	int i;
  
	if (!strcmp(argv[0],"text") || !strcmp(argv[0],"dealer")) {
		if (!strcmp(argv[0], "text")) bitmap.type = GSM_WelcomeNoteText;
		else bitmap.type = GSM_DealerNoteText;
		bitmap.text[0] = 0x00;
		if (argc > 1) strncpy(bitmap.text, argv[1], 255);
	} else {
		if (!strcmp(argv[0], "op") || !strcmp(argv[0], "startup") || !strcmp(argv[0], "caller")) {
			if (argc > 1) {
				if (ReadBitmapFileOnConsole(argv[1], &bitmap) != GE_NONE) {
					GSM->Terminate();
					return(-1);
				}

				if (!strcmp(argv[0], "op")) {
					if (bitmap.type != GSM_OperatorLogo || argc < 3) {
						if (GSM->GetNetworkInfo(&NetworkInfo) == GE_NONE) strncpy(bitmap.netcode, NetworkInfo.NetworkCode, 7);
					}
					GSM_ResizeBitmap(&bitmap, GSM_OperatorLogo, GSM_Info);
					if (argc == 3) {
						strncpy(bitmap.netcode, argv[2], 7);
						if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
							fprintf(stderr, "Sorry, gnokii doesn't know %s network !\n", bitmap.netcode);
							return -1;
						}
					}
				}
				if (!strcmp(argv[0], "startup")) {
					GSM_ResizeBitmap(&bitmap, GSM_StartupLogo, GSM_Info);
				}
				if (!strcmp(argv[0],"caller")) {
					GSM_ResizeBitmap(&bitmap, GSM_CallerLogo, GSM_Info);
					if (argc > 2) {
						bitmap.number = argv[2][0] - '0';
						if ((bitmap.number < 0) || (bitmap.number > 9)) bitmap.number = 0;
					} else {
						bitmap.number = 0;
					}
					oldbit.type = GSM_CallerLogo;
					oldbit.number = bitmap.number;
					if (GSM->GetBitmap(&oldbit) == GE_NONE) {
						/* We have to get the old name and ringtone!! */
						bitmap.ringtone = oldbit.ringtone;
						strncpy(bitmap.text, oldbit.text, 255);
					}
					if (argc > 3) strncpy(bitmap.text, argv[3], 255);	  
				}
				fprintf(stdout, _("Setting Logo.\n"));
			} else {
				/* FIX ME: is it possible to permanently remove op logo ? */
				if (!strcmp(argv[0], "op"))
				{
					bitmap.type = GSM_OperatorLogo;
					strncpy(bitmap.netcode, "000 00", 7);
					bitmap.width = 72;
					bitmap.height = 14;
					bitmap.size = bitmap.width * bitmap.height / 8;
					GSM_ClearBitmap(&bitmap);
				}
				/* FIX ME: how to remove startup and group logos ? */
				fprintf(stdout, _("Removing Logo.\n"));
			}  
		} else {
			fprintf(stderr, _("What kind of logo do you want to set ?\n"));
			GSM->Terminate();
			return -1;
		}
	}
    
	error=GSM->SetBitmap(&bitmap);
  
	switch (error) {
	case GE_NONE:
		oldbit.type = bitmap.type;
		oldbit.number = bitmap.number;
		if (GSM->GetBitmap(&oldbit) == GE_NONE) {
			if (bitmap.type == GSM_WelcomeNoteText ||
			    bitmap.type == GSM_DealerNoteText) {
				if (strcmp(bitmap.text, oldbit.text)) {
					fprintf(stderr, _("Error setting"));
					if (bitmap.type == GSM_DealerNoteText) fprintf(stderr, _(" dealer"));
					fprintf(stderr, _(" welcome note - "));

					/* I know, it looks horrible, but... */
					/* I set it to the short string - if it won't be set */
					/* it means, PIN is required. If it will be correct, previous */
					/* (user) text was too long */

					/* Without it, I could have such thing: */
					/* user set text to very short string (for example, "Marcin") */
					/* then enable phone without PIN and try to set it to the very long (too long for phone) */
					/* string (which start with "Marcin"). If we compare them as only length different, we could think, */
					/* that phone accepts strings 6 chars length only (length of "Marcin") */
					/* When we make it correct, we don't have this mistake */
			
					strcpy(oldbit.text, "!\0");
					GSM->SetBitmap(&oldbit);
					GSM->GetBitmap(&oldbit);
					if (oldbit.text[0]!='!') {
						fprintf(stderr, _("SIM card and PIN is required\n"));
					} else {
						GSM->SetBitmap(&bitmap);
						GSM->GetBitmap(&oldbit);
						fprintf(stderr, _("too long, truncated to \"%s\" (length %i)\n"),oldbit.text,strlen(oldbit.text));
					}
					ok = false;
				}
			} else {
				if (bitmap.type == GSM_StartupLogo) {
					for (i = 0; i < oldbit.size; i++) {
						if (oldbit.bitmap[i] != bitmap.bitmap[i]) {
							fprintf(stderr, _("Error setting startup logo - SIM card and PIN is required\n"));
							ok = false;
							break;
						}
					}
				}
			}
		}
		if (ok) fprintf(stdout, _("Done.\n"));
		break;
	case GE_NOTIMPLEMENTED:
		fprintf(stderr, _("Function not implemented.\n"));
		break;
	case GE_NOTSUPPORTED:
		fprintf(stderr, _("This kind of logo is not supported.\n"));
		break;
	default:
		fprintf(stderr, _("Error !\n"));
		break;
	}
  
	GSM->Terminate();

	return 0;
}


int viewlogo(char *filename)
{
	GSM_Error error;

	error = GSM_ShowBitmapFile(filename);
	return 0;
}

/* Calendar notes receiving. */
int getcalendarnote(int argc, char *argv[])
{
	GSM_CalendarNote CalendarNote;
	int i;
	bool vCal = false;

	struct option options[] = {
		{ "vCard",    optional_argument, NULL, '1'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	CalendarNote.Location = atoi(argv[0]);

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {       
		case 'v':
			vCal=true;
			break;
		default:
			usage(); // Would be better to have an calendar_usage() here.
		}
	}

	if (GSM->GetCalendarNote(&CalendarNote) == GE_NONE) {

		if (vCal) {
			fprintf(stdout, "BEGIN:VCALENDAR\n");
			fprintf(stdout, "VERSION:1.0\n");
			fprintf(stdout, "BEGIN:VEVENT\n");
			fprintf(stdout, "CATEGORIES:");
			switch (CalendarNote.Type) {
			case GCN_REMINDER:
				fprintf(stdout, "MISCELLANEOUS\n");
				break;
			case GCN_CALL:
				fprintf(stdout, "PHONE CALL\n");
				break;
			case GCN_MEETING:
				fprintf(stdout, "MEETING\n");
				break;
			case GCN_BIRTHDAY:
				fprintf(stdout, "SPECIAL OCCASION\n");
				break;
			default:
				fprintf(stdout, "UNKNOWN\n");
				break;
			}
			fprintf(stdout, "SUMMARY:%s\n",CalendarNote.Text);
			fprintf(stdout, "DTSTART:%04d%02d%02dT%02d%02d%02d\n", CalendarNote.Time.Year,
				CalendarNote.Time.Month, CalendarNote.Time.Day, CalendarNote.Time.Hour,
				CalendarNote.Time.Minute, CalendarNote.Time.Second);
			if (CalendarNote.Alarm.Year!=0) {
				fprintf(stdout, "DALARM:%04d%02d%02dT%02d%02d%02d\n", CalendarNote.Alarm.Year,
					CalendarNote.Alarm.Month, CalendarNote.Alarm.Day, CalendarNote.Alarm.Hour,
					CalendarNote.Alarm.Minute, CalendarNote.Alarm.Second);
			}
			fprintf(stdout, "END:VEVENT\n");
			fprintf(stdout, "END:VCALENDAR\n");

		} else {  /* not vCal */
			fprintf(stdout, _("   Type of the note: "));

			switch (CalendarNote.Type) {
			case GCN_REMINDER:
				fprintf(stdout, _("Reminder\n"));
				break;
			case GCN_CALL:
				fprintf(stdout, _("Call\n"));
				break;
			case GCN_MEETING:
				fprintf(stdout, _("Meeting\n"));
				break;
			case GCN_BIRTHDAY:
				fprintf(stdout, _("Birthday\n"));
				break;
			default:
				fprintf(stdout, _("Unknown\n"));
				break;
			}

			fprintf(stdout, _("   Date: %d-%02d-%02d\n"), CalendarNote.Time.Year,
				CalendarNote.Time.Month,
				CalendarNote.Time.Day);

			fprintf(stdout, _("   Time: %02d:%02d:%02d\n"), CalendarNote.Time.Hour,
				CalendarNote.Time.Minute,
				CalendarNote.Time.Second);

			if (CalendarNote.Alarm.Year!=0) {
				fprintf(stdout, _("   Alarm date: %d-%02d-%02d\n"), CalendarNote.Alarm.Year,
					CalendarNote.Alarm.Month,
					CalendarNote.Alarm.Day);

				fprintf(stdout, _("   Alarm time: %02d:%02d:%02d\n"), CalendarNote.Alarm.Hour,
					CalendarNote.Alarm.Minute,
					CalendarNote.Alarm.Second);
			}

			fprintf(stdout, _("   Text: %s\n"), CalendarNote.Text);

			if (CalendarNote.Type == GCN_CALL)
				fprintf(stdout, _("   Phone: %s\n"), CalendarNote.Phone);
		}
	} else {
		fprintf(stderr, _("The calendar note can not be read\n"));

		GSM->Terminate();
		return -1;
	}

	GSM->Terminate();

	return 0;
}

/* Writing calendar notes. */
int writecalendarnote(char *argv[])
{
	GSM_CalendarNote CalendarNote;

	if (GSM_ReadVCalendarFile(argv[0], &CalendarNote, atoi(argv[1]))) {
		fprintf(stdout, _("Failed to load vCalendar file.\n"));
		return(-1);
	}

        /* Error 22=Calendar full ;-) */
	if ((GSM->WriteCalendarNote(&CalendarNote)) == GE_NONE)
		fprintf(stdout, _("Succesfully written!\n"));
	else
		fprintf(stdout, _("Failed to write calendar note!\n"));

	GSM->Terminate();

	return 0;
}

/* Calendar note deleting. */
int deletecalendarnote(char *Index)
{
	GSM_CalendarNote CalendarNote;

	CalendarNote.Location=atoi(Index);

	if (GSM->DeleteCalendarNote(&CalendarNote) == GE_NONE) {
		fprintf(stdout, _("   Calendar note deleted.\n"));
	} else {
		fprintf(stderr, _("The calendar note can not be deleted\n"));

		GSM->Terminate();
		return -1;
	}

	GSM->Terminate();

	return 0;
}

/* Setting the date and time. */
int setdatetime(int argc, char *argv[])
{
	struct tm *now;
	time_t nowh;
	GSM_DateTime Date;

	nowh=time(NULL);
	now=localtime(&nowh);

	Date.Year = now->tm_year;
	Date.Month = now->tm_mon+1;
	Date.Day = now->tm_mday;
	Date.Hour = now->tm_hour;
	Date.Minute = now->tm_min;
	Date.Second = now->tm_sec;

	if (argc > 0) Date.Year = atoi (argv[0]);
	if (argc > 1) Date.Month = atoi (argv[1]);
	if (argc > 2) Date.Day = atoi (argv[2]);
	if (argc > 3) Date.Hour = atoi (argv[3]);
	if (argc > 4) Date.Minute = atoi (argv[4]);

	if (Date.Year < 1900) {

		/* Well, this thing is copyrighted in U.S. This technique is known as
		   Windowing and you can read something about it in LinuxWeekly News:
		   http://lwn.net/1999/features/Windowing.phtml. This thing is beeing
		   written in Czech republic and Poland where algorithms are not allowed
		   to be patented. */

		if (Date.Year > 90)
			Date.Year = Date.Year + 1900;
		else
			Date.Year = Date.Year + 2000;
	}

	/* FIXME: Error checking should be here. */
	GSM->SetDateTime(&Date);

	GSM->Terminate();

	return 0;
}

/* In this mode we receive the date and time from mobile phone. */
int getdatetime(void) {

	GSM_DateTime date_time;

	if (GSM->GetDateTime(&date_time)==GE_NONE) {
		fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.Year, date_time.Month, date_time.Day);
		fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.Hour, date_time.Minute, date_time.Second);
	}

	GSM->Terminate();

	return 0;
}

/* Setting the alarm. */
int setalarm(char *argv[])
{
	GSM_DateTime Date;

	Date.Hour = atoi(argv[0]);
	Date.Minute = atoi(argv[1]);

	GSM->SetAlarm(1, &Date);

	GSM->Terminate();

	return 0;
}

/* Getting the alarm. */
int getalarm(void)
{
	GSM_DateTime date_time;

	if (GSM->GetAlarm(0, &date_time)==GE_NONE) {
		fprintf(stdout, _("Alarm: %s\n"), (date_time.AlarmEnabled==0)?"off":"on");
		fprintf(stdout, _("Time: %02d:%02d\n"), date_time.Hour, date_time.Minute);
	}

	GSM->Terminate();

	return 0;
}

/* In monitor mode we don't do much, we just initialise the fbus code.
   Note that the fbus code no longer has an internal monitor mode switch,
   instead compile with DEBUG enabled to get all the gumpf. */
int monitormode(void)
{
	float rflevel = -1, batterylevel = -1;
//	GSM_PowerSource powersource = -1;
	GSM_RFUnits rf_units = GRF_Arbitrary;
	GSM_BatteryUnits batt_units = GBU_Arbitrary;
	GSM_Statemachine *sm = &State;
	GSM_Data data;

//	GSM_NetworkInfo NetworkInfo;
//	GSM_CBMessage CBMessage;

	GSM_MemoryStatus SIMMemoryStatus   = {GMT_SM, 0, 0};
	GSM_MemoryStatus PhoneMemoryStatus = {GMT_ME, 0, 0};
	GSM_MemoryStatus DC_MemoryStatus   = {GMT_DC, 0, 0};
	GSM_MemoryStatus EN_MemoryStatus   = {GMT_EN, 0, 0};
	GSM_MemoryStatus FD_MemoryStatus   = {GMT_FD, 0, 0};
	GSM_MemoryStatus LD_MemoryStatus   = {GMT_LD, 0, 0};
	GSM_MemoryStatus MC_MemoryStatus   = {GMT_MC, 0, 0};
	GSM_MemoryStatus ON_MemoryStatus   = {GMT_ON, 0, 0};
	GSM_MemoryStatus RC_MemoryStatus   = {GMT_RC, 0, 0};

//	GSM_SMSStatus SMSStatus = {0, 0};

//	char Number[20];
	
	GSM_DataClear(&data);

	/* We do not want to monitor serial line forever - press Ctrl+C to stop the
	   monitoring mode. */
	signal(SIGINT, interrupted);

	fprintf (stderr, _("Entering monitor mode...\n"));

	//sleep(1);
	//GSM->EnableCellBroadcast();

	/* Loop here indefinitely - allows you to see messages from GSM code in
	   response to unknown messages etc. The loops ends after pressing the
	   Ctrl+C. */
	data.RFUnits=&rf_units;
	data.RFLevel=&rflevel;
	data.BatteryUnits=&batt_units;
	data.BatteryLevel=&batterylevel;

	while (!bshutdown) {
		if (SM_Functions(GOP_GetRFLevel,&data,sm) == GE_NONE)
			fprintf(stdout, _("RFLevel: %d\n"), (int)rflevel);

		if (SM_Functions(GOP_GetBatteryLevel,&data,sm) == GE_NONE)
			fprintf(stdout, _("Battery: %d\n"), (int)batterylevel);

//		if (GSM->GetPowerSource(&powersource) == GE_NONE)
//			fprintf(stdout, _("Power Source: %s\n"), (powersource==GPS_ACDC)?_("AC/DC"):_("battery"));

		data.MemoryStatus=&SIMMemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("SIM: Used %d, Free %d\n"), SIMMemoryStatus.Used, SIMMemoryStatus.Free);

		data.MemoryStatus=&PhoneMemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("Phone: Used %d, Free %d\n"), PhoneMemoryStatus.Used, PhoneMemoryStatus.Free);

		data.MemoryStatus=&DC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("DC: Used %d, Free %d\n"), DC_MemoryStatus.Used, DC_MemoryStatus.Free);

		data.MemoryStatus=&EN_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("EN: Used %d, Free %d\n"), EN_MemoryStatus.Used, EN_MemoryStatus.Free);

		data.MemoryStatus=&FD_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("FD: Used %d, Free %d\n"), FD_MemoryStatus.Used, FD_MemoryStatus.Free);

		data.MemoryStatus=&LD_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("LD: Used %d, Free %d\n"), LD_MemoryStatus.Used, LD_MemoryStatus.Free);

		data.MemoryStatus=&MC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("MC: Used %d, Free %d\n"), MC_MemoryStatus.Used, MC_MemoryStatus.Free);

		data.MemoryStatus=&ON_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("ON: Used %d, Free %d\n"), ON_MemoryStatus.Used, ON_MemoryStatus.Free);

		data.MemoryStatus=&RC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus,&data,sm) == GE_NONE)
			fprintf(stdout, _("RC: Used %d, Free %d\n"), RC_MemoryStatus.Used, RC_MemoryStatus.Free);

//		if (GSM->GetSMSStatus(&SMSStatus) == GE_NONE)
//			fprintf(stdout, _("SMS Messages: UnRead %d, Number %d\n"), SMSStatus.UnRead, SMSStatus.Number);

//		if (GSM->GetIncomingCallNr(Number) == GE_NONE)
//			fprintf(stdout, _("Incoming call: %s\n"), Number);

//		if (GSM->GetNetworkInfo(&NetworkInfo) == GE_NONE)
//			fprintf(stdout, _("Network: %s (%s), LAC: %s, CellID: %s\n"), GSM_GetNetworkName (NetworkInfo.NetworkCode), GSM_GetCountryName(NetworkInfo.NetworkCode), NetworkInfo.LAC, NetworkInfo.CellID);

//		if (GSM->ReadCellBroadcast(&CBMessage) == GE_NONE)
//			fprintf(stdout, _("Cell broadcast received on channel %d: %s\n"), CBMessage.Channel, CBMessage.Message);
	    
		sleep(1);
	}

	fprintf (stderr, _("Leaving monitor mode...\n"));

	//GSM->Terminate();

	return 0;
}


#define ESC "\e"

static GSM_Error PrettyOutputFn(char *Display, char *Indicators)
{
	if (Display)
		printf(ESC "[10;0H Display is:\n%s\n", Display);
	if (Indicators)
		printf(ESC "[9;0H Indicators: %s                                                    \n", Indicators);
	printf(ESC "[1;1H");
	return GE_NONE;
}

#if 0
// Uncomment it if used
static GSM_Error OutputFn(char *Display, char *Indicators)
{
	if (Display)
		printf("New display is:\n%s\n", Display);
	if (Indicators)
		printf("Indicators: %s\n", Indicators);
	return GE_NONE;
}
#endif

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

int displayoutput(void)
{
	GSM_Data data;
	GSM_Statemachine *sm = &State;
	GSM_Error error;

	data.OutputFn = PrettyOutputFn;

	error = SM_Functions(GOP_DisplayOutput, &data, sm);
	console_raw();
	fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

	if (error == GE_NONE) {

		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);    

		fprintf (stderr, _("Entered display monitoring mode...\n"));
		fprintf (stderr, ESC "c" );

		/* Loop here indefinitely - allows you to read texts from phone's
		   display. The loops ends after pressing the Ctrl+C. */
		while (!bshutdown) {
			char buf[105];
			memset(&buf[0], 0, 102);
			while (read(0, buf, 100) > 0) {
				fprintf(stderr, "handling keys (%d).\n", strlen(buf));
				if (GSM->HandleString(buf) != GE_NONE)
					fprintf(stdout, _("Key press simulation failed.\n"));
				memset(buf, 0, 102);
			}
			SM_Loop(sm, 1);
		}
		fprintf (stderr, "Shutting down\n");

		fprintf (stderr, _("Leaving display monitor mode...\n"));
		data.OutputFn = NULL;

		error = SM_Functions(GOP_DisplayOutput, &data, sm);
		if (error != GE_NONE)
			fprintf (stderr, _("Error!\n"));
	} else
		fprintf (stderr, _("Error!\n"));

	GSM->Terminate();
	return 0;
}

/* Reads profile from phone and displays its' settings */
int getprofile(int argc, char *argv[])
{
	int max_profiles;
	int start, stop, i;
	GSM_Profile profile;
	GSM_Error error;
  
	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[64];

	profile.Number = 0;
	error = GSM->GetProfile(&profile);

	if (error == GE_NONE) {
  
		while (GSM->GetModel(model) != GE_NONE)
			sleep(1);

		max_profiles = 7; /* This is correct for 6110 (at least my). How do we get
				     the number of profiles? */

		/*For N5110*/
		/*FIX ME: It should be set to 3 for N5130 and 3210 too*/
		if (!strcmp(model, "NSE-1"))
			max_profiles = 3;

		if (argc > 0) {
			profile.Number = atoi(argv[0]) - 1;
			start = profile.Number;
			stop = start + 1;

			if (profile.Number < 0) {
				fprintf(stderr, _("Profile number must be value from 1 to %d!\n"), max_profiles);
				GSM->Terminate();
				return -1;
			}

			if (profile.Number >= max_profiles) {
				fprintf(stderr, _("This phone supports only %d profiles!\n"), max_profiles);
				GSM->Terminate();
				return -1;
			}
		} else {
			start = 0;
			stop = max_profiles;
		}

		i = start;
		while (i < stop) {
			profile.Number = i;

			if (profile.Number != 0)
				GSM->GetProfile(&profile);

			fprintf(stdout, "%d. \"%s\"\n", profile.Number + 1, profile.Name);
			if (profile.DefaultName == -1) fprintf(stdout, _(" (name defined)\n"));

			fprintf(stdout, _("Incoming call alert: %s\n"), GetProfileCallAlertString(profile.CallAlert));

			/* For different phones different ringtones names */

			if (!strcmp(model, "NSE-3"))
				fprintf(stdout, _("Ringing tone: %s (%d)\n"), RingingTones[profile.Ringtone], profile.Ringtone);
			else
				fprintf(stdout, _("Ringtone number: %d\n"), profile.Ringtone);

			fprintf(stdout, _("Ringing volume: %s\n"), GetProfileVolumeString(profile.Volume));

			fprintf(stdout, _("Message alert tone: %s\n"), GetProfileMessageToneString(profile.MessageTone));

			fprintf(stdout, _("Keypad tones: %s\n"), GetProfileKeypadToneString(profile.KeypadTone));

			fprintf(stdout, _("Warning and game tones: %s\n"), GetProfileWarningToneString(profile.WarningTone));

			/* FIXME: Light settings is only used for Car */
			if (profile.Number == (max_profiles - 2)) fprintf(stdout, _("Lights: %s\n"), profile.Lights ? _("On") : _("Automatic"));

			fprintf(stdout, _("Vibration: %s\n"), GetProfileVibrationString(profile.Vibration));

			/* FIXME: it will be nice to add here reading caller group name. */
			if (max_profiles != 3) fprintf(stdout, _("Caller groups: 0x%02x\n"), profile.CallerGroups);

			/* FIXME: Automatic answer is only used for Car and Headset. */
			if (profile.Number >= (max_profiles - 2)) fprintf(stdout, _("Automatic answer: %s\n"), profile.AutomaticAnswer ? _("On") : _("Off"));

			fprintf(stdout, "\n");

			i++;
		}
	} else {
		if (error == GE_NOTIMPLEMENTED) {
			fprintf(stderr, _("Function not implemented in %s model!\n"), model);
			GSM->Terminate();
			return -1;
		} else {
			fprintf(stderr, _("Unspecified error\n"));
			GSM->Terminate();
			return -1;
		}
	}

	GSM->Terminate();
	return 0;
}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */
int getmemory(int argc, char *argv[])
{
	GSM_PhonebookEntry entry;
	int count;
	GSM_Error error;
	char *memory_type_string;
	int start_entry;
	int end_entry;
	GSM_Statemachine *sm = &State;
	
	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	entry.MemoryType = StrToMemoryType(memory_type_string);
	if (entry.MemoryType == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return (-1);
	}

	start_entry = atoi (argv[1]);
	if (argc > 2) end_entry = atoi (argv[2]);
	else end_entry = start_entry;

	/* Now retrieve the requested entries. */
	for (count = start_entry; count <= end_entry; count ++) {

		entry.Location = count;

      		data.PhonebookEntry=&entry;
		error=SM_Functions(GOP_ReadPhonebook,&data,sm);

		if (error == GE_NONE) {
			fprintf(stdout, "%s;%s;%s;%d;%d\n", entry.Name, entry.Number, memory_type_string, entry.Location, entry.Group);
			if (entry.MemoryType == GMT_MC || entry.MemoryType == GMT_DC || entry.MemoryType == GMT_RC)
				fprintf(stdout, "%02u.%02u.%04u %02u:%02u:%02u\n", entry.Date.Day, entry.Date.Month, entry.Date.Year, entry.Date.Hour, entry.Date.Minute, entry.Date.Second);
		}  
		else {
			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), model);
				return -1;
			}
			else if (error == GE_INVALIDMEMORYTYPE) {
				fprintf(stderr, _("Memory type %s not supported!\n"), memory_type_string);
				return -1;
			}

			fprintf(stdout, _("%s|%d|Bad location or other error!(%d)\n"), memory_type_string, count, error);
		}
	}
	return 0;
}

/* Read data from stdin, parse and write to phone.  The parsing is relatively
   crude and doesn't allow for much variation from the stipulated format. */
/* FIXME: I guess there's *very* similar code in xgnokii */
int writephonebook(int argc, char *args[])
{
	GSM_PhonebookEntry entry;
	GSM_Error error;
	char *memory_type_string;
	int line_count=0;
	int subentry;

	char *Line, OLine[100], BackLine[100];
	char *ptr;

	/* Check argument */
	if (argc && (strcmp("-i", args[0])))
		usage();

	Line = OLine;

	/* Go through data from stdin. */
	while (GetLine(stdin, Line, 99)) {
		strcpy(BackLine, Line);
		line_count++;

		ptr = strtok(Line, ";");
		if (ptr) strcpy(entry.Name, ptr);
		else entry.Name[0] = 0;

		ptr = strtok(NULL, ";");
		if (ptr) strcpy(entry.Number, ptr);
		else entry.Number[0] = 0;

		ptr = strtok(NULL, ";");

		if (!ptr) {
			fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, BackLine);
			Line = OLine;
			continue;
		}

		if (!strncmp(ptr,"ME", 2)) {
			memory_type_string = "int";
			entry.MemoryType = GMT_ME;
		} else {
			if (!strncmp(ptr,"SM", 2)) {
				memory_type_string = "sim";
				entry.MemoryType = GMT_SM;
			} else {
				fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, BackLine);
				break;
			}
		}

		ptr = strtok(NULL, ";");
		if (ptr) entry.Location = atoi(ptr);
		else entry.Location = 0;

		ptr = strtok(NULL, ";");
		if (ptr) entry.Group = atoi(ptr);
		else entry.Group = 0;

		if (!ptr) {
			fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, BackLine);
			continue;
		}

		for (subentry = 0; ; subentry++) {
			ptr = strtok(NULL, ";");

			if (ptr &&  *ptr != 0)
				entry.SubEntries[subentry].EntryType = atoi(ptr);
			else
				break;

			ptr = strtok(NULL, ";");
			if (ptr)
				entry.SubEntries[subentry].NumberType=atoi(ptr);

			/* Phone Numbers need to have a number type. */
			if (!ptr && entry.SubEntries[subentry].EntryType == GSM_Number) {
				fprintf(stderr, _("Missing phone number type on line %d"
						  " entry %d [%s]\n"), line_count, subentry, BackLine);
				subentry--;
				break;
			}

			ptr = strtok(NULL, ";");
			if (ptr)
				entry.SubEntries[subentry].BlockNumber=atoi(ptr);

			ptr = strtok(NULL, ";");

			/* 0x13 Date Type; it is only for Dailed Numbers, etc.
			   we don't store to this memories so it's an error to use it. */
			if (!ptr || entry.SubEntries[subentry].EntryType == GSM_Date) {
				fprintf(stdout, _("There is no phone number on line %d entry %d [%s]\n"),
					line_count, subentry, BackLine);
				subentry--;
				break;
			} else
				strcpy(entry.SubEntries[subentry].data.Number, ptr);
		}

		entry.SubEntriesCount = subentry;

		/* This is to send other exports (like from 6110) to 7110 */
		if (!entry.SubEntriesCount) {
			entry.SubEntriesCount = 1;
			entry.SubEntries[subentry].EntryType   = GSM_Number;
			entry.SubEntries[subentry].NumberType  = GSM_General;
			entry.SubEntries[subentry].BlockNumber = 2;
			strcpy(entry.SubEntries[subentry].data.Number, entry.Number);
		}

		Line = OLine;

		if (argc) {
			GSM_PhonebookEntry aux;

			aux.Location = entry.Location;
			error = GSM->GetMemoryLocation(&aux);
			
			if (error == GE_NONE) {
				if (!aux.Empty) {
					int confirm = -1;
					char ans[8];

					fprintf(stdout, _("Location busy. "));
					while (confirm < 0) {
						fprintf(stdout, _("Overwrite? (yes/no) "));
						GetLine(stdin, ans, 7);
						if (!strcmp(ans, _("yes"))) confirm = 1;
						else if (!strcmp(ans, _("no"))) confirm = 0;
					}
					if (!confirm) continue;
				}
			} else {
				fprintf(stderr, _("Unknown error (%d)\n"), error);
				GSM->Terminate();
				return 0;
			}
		}

		/* Do write and report success/failure. */
		error = GSM->WritePhonebookLocation(&entry);

		if (error == GE_NONE)
			fprintf (stdout, _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), memory_type_string, entry.Location, entry.Name, entry.Number);
		else
			fprintf (stdout, _("Write FAILED(%d): memory type: %s, loc: %d, name: %s, number: %s\n"), error, memory_type_string, entry.Location, entry.Name, entry.Number);

	}

	GSM->Terminate();
	return 0;
}

/* Getting speed dials. */
int getspeeddial(char *Number)
{
	GSM_SpeedDial entry;
	entry.Number = atoi(Number);
	if (GSM->GetSpeedDial(&entry) == GE_NONE)
		fprintf(stdout, _("SpeedDial nr. %d: %d:%d\n"), entry.Number, entry.MemoryType, entry.Location);
	GSM->Terminate();
	return 0;
}

/* Setting speed dials. */
int setspeeddial(char *argv[])
{
	GSM_SpeedDial entry;

	char *memory_type_string;

	/* Handle command line args that set type, start and end locations. */

	if (strcmp(argv[1], "ME") == 0) {
		entry.MemoryType = 0x02;
		memory_type_string = "ME";
	} else if (strcmp(argv[1], "SM") == 0) {
		entry.MemoryType = 0x03;
		memory_type_string = "SM";
	} else {
		fprintf(stderr, _("Unknown memory type %s!\n"), argv[1]);
		return -1;
	}

	entry.Number = atoi(argv[0]);
	entry.Location = atoi(argv[2]);

	if (GSM->SetSpeedDial(&entry) == GE_NONE) {
		fprintf(stdout, _("Succesfully written!\n"));
	}

	GSM->Terminate();
	return 0;
}

/* Getting the status of the display. */
int getdisplaystatus(void)
{ 
	int Status;

	GSM->GetDisplayStatus(&Status);

	fprintf(stdout, _("Call in progress: %s\n"), Status & (1<<DS_Call_In_Progress)?_("on"):_("off"));
	fprintf(stdout, _("Unknown: %s\n"),          Status & (1<<DS_Unknown)?_("on"):_("off"));
	fprintf(stdout, _("Unread SMS: %s\n"),       Status & (1<<DS_Unread_SMS)?_("on"):_("off"));
	fprintf(stdout, _("Voice call: %s\n"),       Status & (1<<DS_Voice_Call)?_("on"):_("off"));
	fprintf(stdout, _("Fax call active: %s\n"),  Status & (1<<DS_Fax_Call)?_("on"):_("off"));
	fprintf(stdout, _("Data call active: %s\n"), Status & (1<<DS_Data_Call)?_("on"):_("off"));
	fprintf(stdout, _("Keyboard lock: %s\n"),    Status & (1<<DS_Keyboard_Lock)?_("on"):_("off"));
	fprintf(stdout, _("SMS storage full: %s\n"), Status & (1<<DS_SMS_Storage_Full)?_("on"):_("off"));

	GSM->Terminate();
	return 0;
}

int netmonitor(char *Mode)
{
	unsigned char mode = atoi(Mode);
	char Screen[50];

	if (!strcmp(Mode, "reset"))
		mode = 0xf0;
	else if (!strcmp(Mode, "off"))
		mode = 0xf1;
	else if (!strcmp(Mode, "field"))
		mode = 0xf2;
	else if (!strcmp(Mode, "devel"))
		mode = 0xf3;
	else if (!strcmp(Mode, "next"))
		mode = 0x00;

	memset(&Screen, 0, 50);
	GSM->NetMonitor(mode, Screen);

	if (Screen)
		fprintf(stdout, "%s\n", Screen);

	GSM->Terminate();
	return 0;
}

int identify(void)
{
	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char imei[64], model[64], rev[64], manufacturer[64];
	GSM_Statemachine *sm = &State;

	data.Model=model;
	data.Revision=rev;
	data.Imei=imei;

	/* Retrying is bad idea: what if function is simply not implemented?
	   Anyway let's wait 2 seconds for the right packet from the phone. */
	sleep(2);

	strcpy(imei, "(unknown)");
	strcpy(manufacturer, "(unknown)");
	strcpy(model, "(unknown)");
	strcpy(rev, "(unknown)");

	SM_Functions(GOP_Identify, &data, sm);

	fprintf(stdout, _("IMEI:     %s\n"), imei);
	fprintf(stdout, _("Manufacturer: %s\n"), manufacturer);
	fprintf(stdout, _("Model:    %s\n"), model);
	fprintf(stdout, _("Revision: %s\n"), rev);

	//GSM->Terminate();

	return 0;
}

int senddtmf(char *String)
{
	GSM->SendDTMF(String);
	GSM->Terminate();
	return 0;
}

/* Resets the phone */
int reset( char *type)
{
	unsigned char _type = 0x03;

	if (type) {
		if(!strcmp(type, "soft"))
			_type = 0x03;
		else
			if(!strcmp(type, "hard"))
				_type = 0x04;
			else {
				fprintf(stderr, _("What kind of reset do you want??\n"));
				return -1;
			}
	}

	GSM->Reset(_type);
	GSM->Terminate();

	return 0;
}

/* pmon allows fbus code to run in a passive state - it doesn't worry about
   whether comms are established with the phone.  A debugging/development
   tool. */
int pmon(void)
{ 

	GSM_Error error;
	GSM_ConnectionType connection=GCT_Serial;
	GSM_Statemachine sm;

	/* Initialise the code for the GSM interface. */     
	error = GSM_Initialise(model, Port, Initlength, connection, NULL, &sm);

	if (error != GE_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
		return -1;
	}

	while (1) {
		usleep(50000);
	}

	return 0;
}

int sendringtone(int argc, char *argv[])
{
	GSM_Ringtone ringtone;
	GSM_Error error;

	if (GSM_ReadRingtoneFile(argv[0], &ringtone)) {
		fprintf(stdout, _("Failed to load ringtone.\n"));
		return(-1);
	}  

	error = GSM->SendRingtone(&ringtone,argv[1]);

	if (error == GE_NONE) 
		fprintf(stdout, _("Send succeeded!\n"));
	else
		fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

	GSM->Terminate();
	return 0;

}


int setringtone(int argc, char *argv[])
{
	GSM_Ringtone ringtone;
	GSM_Error error;

	if (GSM_ReadRingtoneFile(argv[0], &ringtone)) {
		fprintf(stdout, _("Failed to load ringtone.\n"));
		return(-1);
	}  

	error = GSM->SetRingtone(&ringtone);

	if (error == GE_NONE) 
		fprintf(stdout, _("Send succeeded!\n"));
	else
		fprintf(stdout, _("Send failed\n"));

	GSM->Terminate();
	return 0;

}

int presskeysequence(void)
{
	char buf[105];

	console_raw();

	memset(&buf[0], 0, 102);
	while (read(0, buf, 100) > 0) {
		fprintf(stderr, "handling keys (%d).\n", strlen(buf));
		if (GSM->HandleString(buf) != GE_NONE)
			fprintf(stdout, _("Key press simulation failed.\n"));
		memset(buf, 0, 102);
	}

	GSM->Terminate();
	return 0;
}
 
/* This is a "convenience" function to allow quick test of new API stuff which
   doesn't warrant a "proper" command line function. */
#ifndef WIN32
int foogle(char *argv[])
{
	/* Initialise the code for the GSM interface. */     
	fbusinit(NULL);
	// Fill in what you would like to test here...
	return 0;
}
#endif

