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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001-2002  Pavel Machek
  Copyright (C) 2001-2002  Pawe³ Kot
  Copyright (C) 2002       BORBELY Zoltan

  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries and other stuff.

  WARNING: this code is the test tool. Well, our test tool is now
  really powerful and useful :-)

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
#  include <io.h>
#  include "win32/getopt.h"

#else

#  include <unistd.h>
#  include <termios.h>
#  include <fcntl.h>
#  include <getopt.h>

#endif


#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include "gnokii-app.h"
#include "gnokii.h"
#include <sys/stat.h>

#define MAX_INPUT_LINE_LEN 512

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
	OPT_GETSECURITYCODESTATUS,
	OPT_CHANGESECURITYCODE,
	OPT_SETDATETIME,
	OPT_GETDATETIME,
	OPT_SETALARM,
	OPT_GETALARM,
	OPT_DIALVOICE,
	OPT_ANSWERCALL,
	OPT_HANGUP,
	OPT_GETTODO,
	OPT_WRITETODO,
	OPT_DELETEALLTODOS,
	OPT_GETCALENDARNOTE,
	OPT_WRITECALENDARNOTE,
	OPT_DELCALENDARNOTE,
	OPT_GETDISPLAYSTATUS,
	OPT_GETPHONEBOOK,
	OPT_WRITEPHONEBOOK,
	OPT_DELETEPHONEBOOK,
	OPT_GETSPEEDDIAL,
	OPT_SETSPEEDDIAL,
	OPT_GETSMS,
	OPT_DELETESMS,
	OPT_SENDSMS,
	OPT_SAVESMS,
	OPT_SENDLOGO,
	OPT_SENDRINGTONE,
	OPT_GETSMSC,
	OPT_SETSMSC,
	OPT_GETWELCOMENOTE,
	OPT_SETWELCOMENOTE,
	OPT_PMON,
	OPT_NETMONITOR,
	OPT_IDENTIFY,
	OPT_SENDDTMF,
	OPT_RESET,
	OPT_SETLOGO,
	OPT_GETLOGO,
	OPT_VIEWLOGO,
	OPT_GETRINGTONE,
	OPT_SETRINGTONE,
	OPT_PLAYRINGTONE,
	OPT_RINGTONECONVERT,
	OPT_GETPROFILE,
	OPT_SETPROFILE,
	OPT_GETACTIVEPROFILE,
	OPT_SETACTIVEPROFILE,
	OPT_DISPLAYOUTPUT,
	OPT_KEYPRESS,
	OPT_ENTERCHAR,
	OPT_DIVERT,
	OPT_SMSREADER,
	OPT_FOOGLE,
	OPT_GETSECURITYCODE,
	OPT_GETWAPBOOKMARK,
	OPT_WRITEWAPBOOKMARK,
	OPT_GETWAPSETTING,
	OPT_WRITEWAPSETTING,
	OPT_ACTIVATEWAPSETTING,
	OPT_DELETEWAPBOOKMARK,
	OPT_DELETESMSFOLDER,
	OPT_CREATESMSFOLDER,
	OPT_LISTNETWORKS,
	OPT_GETNETWORKINFO,
	OPT_GETLOCKSINFO,
	OPT_GETRINGTONELIST,
	OPT_DELETERINGTONE,
	OPT_SHOWSMSFOLDERSTATUS,
	OPT_GETFILELIST,
	OPT_GETFILEID,
	OPT_GETFILE,
	OPT_GETALLFILES,
	OPT_PUTFILE,
	OPT_DELETEFILE
} opt_index;

static FILE *logfile = NULL;
static char *lockfile = NULL;

/* Local variables */
static char *profile_get_call_alert_string(int code)
{
	switch (code) {
	case GN_PROFILE_CALLALERT_Ringing:	return "Ringing";
	case GN_PROFILE_CALLALERT_Ascending:	return "Ascending";
	case GN_PROFILE_CALLALERT_RingOnce:	return "Ring once";
	case GN_PROFILE_CALLALERT_BeepOnce:	return "Beep once";
	case GN_PROFILE_CALLALERT_CallerGroups:	return "Caller groups";
	case GN_PROFILE_CALLALERT_Off:		return "Off";
	default:				return "Unknown";
	}
}

static char *profile_get_volume_string(int code)
{
	switch (code) {
	case GN_PROFILE_VOLUME_Level1:		return "Level 1";
	case GN_PROFILE_VOLUME_Level2:		return "Level 2";
	case GN_PROFILE_VOLUME_Level3:		return "Level 3";
	case GN_PROFILE_VOLUME_Level4:		return "Level 4";
	case GN_PROFILE_VOLUME_Level5:		return "Level 5";
	default:				return "Unknown";
	}
}

static char *profile_get_keypad_tone_string(int code)
{
	switch (code) {
	case GN_PROFILE_KEYVOL_Off:		return "Off";
	case GN_PROFILE_KEYVOL_Level1:		return "Level 1";
	case GN_PROFILE_KEYVOL_Level2:		return "Level 2";
	case GN_PROFILE_KEYVOL_Level3:		return "Level 3";
	default:				return "Unknown";
	}
}

static char *profile_get_message_tone_string(int code)
{
	switch (code) {
	case GN_PROFILE_MESSAGE_NoTone:		return "No tone";
	case GN_PROFILE_MESSAGE_Standard:	return "Standard";
	case GN_PROFILE_MESSAGE_Special:	return "Special";
	case GN_PROFILE_MESSAGE_BeepOnce:	return "Beep once";
	case GN_PROFILE_MESSAGE_Ascending:	return "Ascending";
	default:				return "Unknown";
	}
}

static char *profile_get_warning_tone_string(int code)
{
	switch (code) {
	case GN_PROFILE_WARNING_Off:		return "Off";
	case GN_PROFILE_WARNING_On:		return "On";
	default:				return "Unknown";
	}
}

static char *profile_get_vibration_string(int code)
{
	switch (code) {
	case GN_PROFILE_VIBRATION_Off:		return "Off";
	case GN_PROFILE_VIBRATION_On:		return "On";
	default:				return "Unknown";
	}
}

static void short_version(void)
{
	fprintf(stderr, _("GNOKII Version %s\n"), VERSION);
}

/* This function shows the copyright and some informations usefull for
   debugging. */
static void version(void)
{
	fprintf(stderr, _("Copyright (C) Hugh Blemings <hugh@blemings.org>, 1999, 2000\n"
			  "Copyright (C) Pavel Janik ml. <Pavel.Janik@suse.cz>, 1999, 2000\n"
			  "Copyright (C) Pavel Machek <pavel@ucw.cz>, 2001\n"
			  "Copyright (C) Pawel Kot <pkot@linuxnews.pl>, 2001-2004\n"
			  "Copyright (C) BORBELY Zoltan <bozo@andrews.hu>, 2002\n"
			  "gnokii is free software, covered by the GNU General Public License, and you are\n"
			  "welcome to change it and/or distribute copies of it under certain conditions.\n"
			  "There is absolutely no warranty for gnokii.  See GPL for details.\n"
			  "Built %s %s\n"), __TIME__, __DATE__);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options. */
static int usage(FILE *f, int retval)
{
	fprintf(f, _("Usage:\n"
		     "General options:\n"
		     "          gnokii --help\n"
		     "          gnokii --monitor [delay|once]\n"
		     "          gnokii --version\n"
		     "SMS options:\n"
		     "          gnokii --getsms memory_type start [end] [-f file] [-F file] [-d]\n"
		     "          gnokii --deletesms memory_type start [end]\n"
		     "          gnokii --sendsms destination [--smsc message_center_number |\n"
		     "                 --smscno message_center_index] [-r] [-C n] [-v n]\n"
		     "                 [--long n] [-i]\n"
		     "          gnokii --savesms [--sender from] [--smsc message_center_number |\n"
		     "                 --smscno message_center_index] [--folder folder_id]\n"
		     "                 [--location number] [--sent | --read] [--deliver] \n"
		     "                 [--datetime YYMMDDHHMMSS] \n"
		     "          gnokii --getsmsc [start_number [end_number]] [-r|--raw]\n"
		     "          gnokii --setsmsc\n"
		     "          gnokii --createsmsfolder name\n"
		     "          gnokii --deletesmsfolder number\n"
		     "          gnokii --showsmsfolderstatus\n"
		     "          gnokii --smsreader\n"
		     "Phonebook options:\n"
		     "          gnokii --getphonebook memory_type start_number [end_number|end]\n"
		     "                 [[-r|--raw]|[-v|--vcard]|[-l|--ldif]]\n"
		     "          gnokii --writephonebook [[-o|--overwrite]|[-f|--find-free]]\n"
		     "                 [-m|--memory-type|--memory] [-n|--memory-location|--memory]\n"
		     "                 [[-v|--vcard]|[-l|--ldif]]\n"
		     "          gnokii --deletephonebook memory_type start_number [end_number|end]\n"
		     "Calendar options:\n"
		     "          gnokii --getcalendarnote start_number [end_number|end] [-v]\n"
		     "          gnokii --writecalendarnote vcalendarfile number\n"
		     "          gnokii --deletecalendarnote start [end_number|end]\n"
		     "ToDo options:\n"
		     "          gnokii --gettodo start_number [end_number|end] [-v]\n"
		     "          gnokii --writetodo vcalendarfile number\n"
		     "          gnokii --deletealltodos\n"
		     "Dialling and call options:\n"
		     "          gnokii --getspeeddial number\n"
		     "          gnokii --setspeeddial number memory_type location\n"
		     "          gnokii --dialvoice number\n"
		     "          gnokii --senddtmf string\n"
		     "          gnokii --answercall callid\n"
		     "          gnokii --hangup callid\n"
		     "          gnokii --divert {--op|-o} {register|enable|query|disable|erasure}\n"
		     "                 {--type|-t} {all|busy|noans|outofreach|notavail}\n"
		     "                 {--call|-c} {all|voice|fax|data}\n"
		     "                 [{--timeout|-m} time_in_seconds]\n"
		     "                 [{--number|-n} number]\n"
		     "Phone settings:\n"
		     "          gnokii --getdisplaystatus\n"
		     "          gnokii --displayoutput\n"
		     "          gnokii --getprofile [start_number [end_number]] [-r|--raw]\n"
		     "          gnokii --setprofile\n"
		     "          gnokii --getactiveprofile\n"
		     "          gnokii --setactiveprofile profile_number\n"
		     "          gnokii --netmonitor {reset|off|field|devel|next|nr}\n"
		     "          gnokii --reset [soft|hard]\n"
		     "          gnokii --setdatetime [YYYY [MM [DD [HH [MM]]]]]\n"
		     "          gnokii --getdatetime\n"
		     "          gnokii --setalarm [HH MM]\n"
		     "          gnokii --getalarm\n"
		     "WAP options:\n"
		     "          gnokii --getwapbookmark number\n"
		     "          gnokii --writewapbookmark name URL\n"
		     "          gnokii --deletewapbookmark number\n"
		     "          gnokii --getwapsetting number [-r]\n"
		     "          gnokii --writewapsetting\n"
		     "          gnokii --activatewapsetting number\n"
		     "Logo options:\n"
		     "          gnokii --sendlogo {caller|op|picture} destination logofile\n"
		     "                 [network code]\n"
		     "          gnokii --sendringtone rtttlfile destination\n"
		     "          gnokii --setlogo op [logofile] [network code]\n"
		     "          gnokii --setlogo startup [logofile]\n"
		     "          gnokii --setlogo caller [logofile] [caller group number] [group name]\n"
		     "          gnokii --setlogo {dealer|text} [text]\n"
		     "          gnokii --getlogo op [logofile] [network code]\n"
		     "          gnokii --getlogo startup [logofile] [network code]\n"
		     "          gnokii --getlogo caller [caller group number] [logofile]\n"
		     "                 [network code]\n"
		     "          gnokii --getlogo {dealer|text}\n"
		     "          gnokii --viewlogo logofile\n"
		     "Ringtone options:\n"
		     "          gnokii --getringtone rtttlfile [location] [-r|--raw]\n"
		     "          gnokii --setringtone rtttlfile [location] [-r|--raw] [--name name]\n"
		     "          gnokii --playringtone rtttlfile [--volume vol]\n"
		     "          gnokii --ringtoneconvert source destination\n"
		     "          gnokii --getringtonelist\n"
		     "          gnokii --deleteringtone start [end]\n"
		     "Security options:\n"
		     "          gnokii --identify\n"
		     "          gnokii --getlocksinfo\n"
		));
#ifdef SECURITY
	fprintf(f, _("          gnokii --entersecuritycode PIN|PIN2|PUK|PUK2\n"
		     "          gnokii --getsecuritycodestatus\n"
		     "          gnokii --getsecuritycode\n"
		     "          gnokii --changesecuritycode PIN|PIN2|PUK|PUK2\n"
		));
#endif
	fprintf(f, _("File options:\n"
		     "          gnokii --getfilelist remote_path\n"
		     "          gnokii --getfileid remote_filename\n"
		     "          gnokii --getfile remote_filename [local_filename]\n"
		     "          gnokii --getallfiles remote_path\n"
		     "          gnokii --putfile local_filename remote_filename\n"
		     "          gnokii --deletefile remote_filename\n"
		));
	fprintf(f, _("Misc options:\n"
		     "          gnokii --keysequence\n"
		     "          gnokii --enterchar\n"
		     "          gnokii --listnetworks\n"
		     "          gnokii --getnetworkinfo\n"
		));
	exit(retval);
}

/* businit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

static struct gn_statemachine state;
static gn_data data;
static gn_cb_message cb_queue[16];
static int cb_ridx = 0;
static int cb_widx = 0;
static gn_ringtone_list ringtone_list;
static int ringtone_list_initialised = 0;


static void busterminate(void)
{
	gn_sm_functions(GN_OP_Terminate, NULL, &state);
	if (lockfile) gn_device_unlock(lockfile);
}

static void businit(void)
{
	gn_error error;
	char *aux;
	static bool atexit_registered = false;

	gn_data_clear(&data);

	aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
	/* Defaults to 'no' */
	if (aux && !strcmp(aux, "yes")) {
		lockfile = gn_device_lock(state.config.port_device);
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
	/* signal(SIGINT, bussignal); */

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(&state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Telephone interface init failed: %s\nQuitting.\n"), gn_error_print(error));
		exit(2);
	}
}

/* This function checks that the argument count for a given options is withing
   an allowed range. */
static int checkargs(int opt, struct gnokii_arg_len gals[], int argc)
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

static void sendsms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: gnokii --sendsms destination\n"
	                  "               [--smsc message_center_number | --smscno message_center_index]\n"
	                  "               [-r] [-C n] [-v n] [--long n] [--animation file;file;file;file]\n"
	                  "               [--concat this;total;serial]\n"
			  " Give the text of the message to the standard input.\n"
			  "   destination - phone number where to send SMS\n"
			  "   --smsc      - phone number of the SMSC\n"
			  "   --smscno    - index of the SMSC set stored in the phone\n"
			  "   -r          - require delivery report\n"
			  "   -C n        - message Class; values [0, 3]\n"
			  "   -v n        - set validity period to n minutes\n"
			  "   --long n    - read n bytes from the input; default is 160\n"
			  "\n"
		));
	exit(exitval);
}

static void getcalendarnote_usage(FILE *f, int exitval)
{
	fprintf(f, _("usage: gnokii --getcalendarnote start_number [end_number | end] [-v]\n"
					 "       start_number - entry number in the phone calendar (numeric)\n"
					 "       end_number   - until this entry in the phone calendar (numeric, optional)\n"
					 "       end          - the string \"end\" indicates all entries from start to end\n"
					 "       -v           - output in iCalendar format\n"
					 "  NOTE: if no end is given, only the start entry is written\n"
	));
	exit(exitval);
}

static gn_error readtext(gn_sms_user_data *udata, int input_len)
{
	char message_buffer[255 * GN_SMS_MAX_LENGTH];
	int chars_read;

#ifndef	WIN32
	if (isatty(0))
#endif
		fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:"));

	/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return GN_ERR_INTERNALERROR;
	} else if (chars_read > input_len || chars_read > sizeof(udata->u.text) - 1) {
		fprintf(stderr, _("Input too long! (%d, maximum is %d)\n"), chars_read, input_len);
		return GN_ERR_INTERNALERROR;
	}

	/* Null terminate. */
	message_buffer[chars_read] = 0x00;
	if (udata->type != GN_SMS_DATA_iMelody && chars_read > 0 && message_buffer[chars_read - 1] == '\n') 
		message_buffer[--chars_read] = 0x00;
	strncpy(udata->u.text, message_buffer, chars_read);
	udata->u.text[chars_read] = 0;
	udata->length = chars_read;

	return GN_ERR_NONE;
}

static gn_error loadbitmap(gn_bmp *bitmap, char *s, int type)
{
	gn_error error;
	bitmap->type = type;
	error = gn_bmp_null(bitmap, &state.driver.phone);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Could not null bitmap: %s\n"), gn_error_print(error));
		return error;
	}
	error = gn_file_bitmap_read(s, bitmap, &state.driver.phone);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Could not load bitmap from %s: %s\n"), s, gn_error_print(error));
		return error;
	}
	return GN_ERR_NONE;
}

static void init_ringtone_list(void)
{
	gn_error error;

	if (ringtone_list_initialised) return;

	memset(&ringtone_list, 0, sizeof(ringtone_list));
	data.ringtone_list = &ringtone_list;

	error = gn_sm_functions(GN_OP_GetRingtoneList, &data, &state);

	data.ringtone_list = NULL;

	if (error != GN_ERR_NONE) {
		ringtone_list.count = 0;
		ringtone_list.userdef_location = 0;
		ringtone_list.userdef_count = 0;
		ringtone_list_initialised = -1;
	} else
		ringtone_list_initialised = 1;
}

static char *get_ringtone_name(int id)
{
	int i;

	init_ringtone_list();

	for (i = 0; i < ringtone_list.count; i++) {
		if (ringtone_list.ringtone[i].location == id)
			return ringtone_list.ringtone[i].name;
	}

	return "Unknown";
}

/* Send  SMS messages. */
static int sendsms(int argc, char *argv[])
{
	gn_sms sms;
	gn_error error;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	int input_len, i, curpos = 0;

	struct option options[] = {
		{ "smsc",    required_argument, NULL, '1'},
		{ "smscno",  required_argument, NULL, '2'},
		{ "long",    required_argument, NULL, '3'},
		{ "picture", required_argument, NULL, '4'},
		{ "8bit",    0,                 NULL, '8'},
		{ "imelody", 0,                 NULL, 'i'},
		{ "animation",required_argument,NULL, 'a'},
		{ "concat",  required_argument, NULL, 'o'},
		{ NULL,      0,                 NULL, 0}
	};

	input_len = GN_SMS_MAX_LENGTH;

	/* The memory is zeroed here */
	gn_sms_default_submit(&sms);

	memset(&sms.remote.number, 0, sizeof(sms.remote.number));
	strncpy(sms.remote.number, argv[0], sizeof(sms.remote.number) - 1);
	if (sms.remote.number[0] == '+') 
		sms.remote.type = GN_GSM_NUMBER_International;
	else 
		sms.remote.type = GN_GSM_NUMBER_Unknown;

	optarg = NULL;
	optind = 0;

	while ((i = getopt_long(argc, argv, "r8co:C:v:ip:", options, NULL)) != -1) {
		switch (i) {       /* -c for compression. not yet implemented. */
		case '1': /* SMSC number */
			strncpy(sms.smsc.number, optarg, sizeof(sms.smsc.number) - 1);
			if (sms.smsc.number[0] == '+') 
				sms.smsc.type = GN_GSM_NUMBER_International;
			else
				sms.smsc.type = GN_GSM_NUMBER_Unknown;
			break;

		case '2': /* SMSC number index in phone memory */
			data.message_center = calloc(1, sizeof(gn_sms_message_center));
			data.message_center->id = atoi(optarg);
			if (data.message_center->id < 1 || data.message_center->id > 5) {
				free(data.message_center);
				sendsms_usage(stderr, -1);
			}
			if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
				strcpy(sms.smsc.number, data.message_center->smsc.number);
				sms.smsc.type = data.message_center->smsc.type;
			}
			free(data.message_center);
			break;

		case '3': /* we send long message */
			input_len = atoi(optarg);
			if (input_len > 255 * GN_SMS_MAX_LENGTH) {
				fprintf(stderr, _("Input too long!\n"));
				return -1;
			}
			break;

		case 'p':
		case '4': /* we send multipart message - picture message; FIXME: This seems not yet implemented */
			sms.udh.number = 1;
			break;

		case 'a': /* Animation */ {
			char buf[10240];
			char *s = buf, *t;
			strcpy(buf, optarg);
			sms.user_data[curpos].type = GN_SMS_DATA_Animation;
			for (i = 0; i < 4; i++) {
				t = strchr(s, ';');
				if (t)
					*t++ = 0;
				loadbitmap(&sms.user_data[curpos].u.animation[i], s, i ? GN_BMP_EMSAnimation2 : GN_BMP_EMSAnimation);
				s = t;
			}
			sms.user_data[++curpos].type = GN_SMS_DATA_None;
			curpos = -1;
			break;
		}

		case 'o': /* Concat header */ {
			dprintf("Adding concat header\n");
			sms.user_data[curpos].type = GN_SMS_DATA_Concat;
			if (3 != sscanf(optarg, "%d;%d;%d", &sms.user_data[curpos].u.concat.curr, 
					&sms.user_data[curpos].u.concat.total, 
					&sms.user_data[curpos].u.concat.serial)) {
				fprintf(stderr, _("Incorrect --concat option\n"));
				break;
			}
			curpos++;
			break;
		}

		case 'r': /* request for delivery report */
			sms.delivery_report = true;
			break;

		case 'C': /* class Message */
			switch (*optarg) {
			case '0': sms.dcs.u.general.m_class = 1; break;
			case '1': sms.dcs.u.general.m_class = 2; break;
			case '2': sms.dcs.u.general.m_class = 3; break;
			case '3': sms.dcs.u.general.m_class = 4; break;
			default: sendsms_usage(stderr, -1);
			}
			break;

		case 'v':
			sms.validity = atoi(optarg);
			break;

		case '8':
			sms.dcs.u.general.alphabet = GN_SMS_DCS_8bit;
			input_len = GN_SMS_8BIT_MAX_LENGTH ;
			break;

		case 'i':
			sms.user_data[0].type = GN_SMS_DATA_iMelody;
			sms.user_data[1].type = GN_SMS_DATA_None;
			error = readtext(&sms.user_data[0], input_len);
			if (error != GN_ERR_NONE) return error;
			if (sms.user_data[0].length < 1) {
				fprintf(stderr, _("Empty message. Quitting.\n"));
				return -1;
			}
			curpos = -1;
			break;
		default:
			sendsms_usage(stderr, -1);
		}
	}

	if (!sms.smsc.number[0]) {
		data.message_center = calloc(1, sizeof(gn_sms_message_center));
		data.message_center->id = 1;
		if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
			strcpy(sms.smsc.number, data.message_center->smsc.number);
			sms.smsc.type = data.message_center->smsc.type;
		}
		free(data.message_center);
	}

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	if (curpos != -1) {
		error = readtext(&sms.user_data[curpos], input_len);
		if (error != GN_ERR_NONE) return error;
		if (sms.user_data[curpos].length < 1) {
			fprintf(stderr, _("Empty message. Quitting.\n"));
			return -1;
		}
		sms.user_data[curpos].type = GN_SMS_DATA_Text;
		if (!gn_char_def_alphabet(sms.user_data[curpos].u.text))
			sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;
		sms.user_data[++curpos].type = GN_SMS_DATA_None;
	}

	data.sms = &sms;

	/* Send the message. */
	error = gn_sms_send(&data, &state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	return error;
}

static int savesms(int argc, char *argv[])
{
	gn_sms sms;
	gn_error error = GN_ERR_INTERNALERROR;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[255 * GN_SMS_MAX_LENGTH ];
	int input_len, chars_read, i;
	char tmp[3];
#if 0
	int confirm = -1;
	int interactive = 0;
	char ans[8];
#endif
	struct option options[] = {
		{ "smsc",     required_argument, NULL, '0'},
		{ "smscno",   required_argument, NULL, '1'},
		{ "sender",   required_argument, NULL, '2'},
		{ "location", required_argument, NULL, '3'},
		{ "read",     0,                 NULL, 'r'},
		{ "sent",     0,                 NULL, 's'},
		{ "folder",   required_argument, NULL, 'f'},
		{ "deliver",  0                , NULL, 'd'},
		{ "datetime", required_argument, NULL, 't'},
		{ NULL,       0,                 NULL, 0}
	};

	unsigned char memory_type[20];
	struct tm		*now;
	time_t			nowh;

	nowh = time(NULL);
	now  = localtime(&nowh);

	gn_sms_default_submit(&sms);

	sms.smsc_time.year	= now->tm_year;
	sms.smsc_time.month	= now->tm_mon+1;
	sms.smsc_time.day	= now->tm_mday;
	sms.smsc_time.hour	= now->tm_hour;
	sms.smsc_time.minute	= now->tm_min;
	sms.smsc_time.second	= now->tm_sec;
	sms.smsc_time.timezone	= 0;

	sms.smsc_time.year += 1900;

	/* the nokia 7110 will choke if no number is present when we */
	/* try to store a SMS on the phone. maybe others do too */
	/* TODO should this be handled here? report an error instead */
	/* of setting an default? */
	strcpy(sms.remote.number, "0");
	sms.remote.type = GN_GSM_NUMBER_International;
	sms.number = 0;
	input_len = GN_SMS_MAX_LENGTH ;

	optarg = NULL;
	optind = 0;
	argv--;
	argc++;

	/* Option parsing */
	while ((i = getopt_long(argc, argv, "0:1:2:3:rsf:idt:", options, NULL)) != -1) {
		switch (i) {
		case '0': /* SMSC number */
			snprintf(sms.smsc.number, sizeof(sms.smsc.number) - 1, "%s", optarg);
			if (sms.smsc.number[0] == '+') 
				sms.smsc.type = GN_GSM_NUMBER_International;
			else
				sms.smsc.type = GN_GSM_NUMBER_Unknown;
			break;
		case '1': /* SMSC number index in phone memory */
			data.message_center = calloc(1, sizeof(gn_sms_message_center));
			data.message_center->id = atoi(optarg);
			if (data.message_center->id < 1 || data.message_center->id > 5) {
				free(data.message_center);
				sendsms_usage(stderr, -1);
			}
			if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
				strcpy(sms.smsc.number, data.message_center->smsc.number);
				sms.smsc.type = data.message_center->smsc.type;
			}
			free(data.message_center);
			break;
		case '2': /* sender number */
			if (*optarg == '+')
				sms.remote.type = GN_GSM_NUMBER_International;
			else
				sms.remote.type = GN_GSM_NUMBER_Unknown;
			snprintf(sms.remote.number, GN_BCD_STRING_MAX_LENGTH, "%s", optarg);
			break;
		case '3': /* location to write to */
			sms.number = atoi(optarg);
			break;
		case 's': /* mark the message as sent */
		case 'r': /* mark the message as read */
			sms.status = GN_SMS_Sent;
			break;
#if 0
		case 'i': /* Ask before overwriting */
			interactive = 1;
			break;
#endif
		case 'f': /* Specify the folder where to save the message */
			snprintf(memory_type, 19, "%s", optarg);
			if (gn_str2memory_type(memory_type) == GN_MT_XX) {
				fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), optarg);
				return -1;
			}
			break;
		case 'd': /* type Deliver */
			sms.type = GN_SMS_MT_Deliver;
			break;
		case 't': /* set specific date and time of message delivery */
			tmp[2] = 0;
			if (strlen(optarg) != 12) {
				fprintf(stderr, _("Invalid datetime format: %s (should be YYMMDDHHMMSS, all digits)!\n"), optarg);
				return -1;
			}
			for (i = 0; i < 12; i++)
				if (!isdigit(optarg[i])) {
					fprintf(stderr, _("Invalid datetime format: %s (should be YYMMDDHHMMSS, all digits)!\n"), optarg);
					return -1;
				}
			strncpy(tmp, optarg, 2);
			sms.smsc_time.year	= atoi(tmp) + 1900;
			strncpy(tmp, optarg+2, 2);
			sms.smsc_time.month	= atoi(tmp);
			strncpy(tmp, optarg+4, 2);
			sms.smsc_time.day	= atoi(tmp);
			strncpy(tmp, optarg+6, 2);
			sms.smsc_time.hour	= atoi(tmp);
			strncpy(tmp, optarg+8, 2);
			sms.smsc_time.minute	= atoi(tmp);
			strncpy(tmp, optarg+10, 2);
			sms.smsc_time.second	= atoi(tmp);
			if (!gn_timestamp_isvalid(sms.smsc_time)) {
				fprintf(stderr, _("Invalid datetime: %s.\n"), optarg);
				return -1;
			}
			break;
		default:
			usage(stderr, -1);
			return -1;
		}
	}
#if 0
	if (interactive) {
		gn_sms aux;

		aux.number = sms.number;
		data.sms = &aux;
		error = gn_sm_functions(GN_OP_GetSMSnoValidate, &data, &state);
		switch (error) {
		case GN_ERR_NONE:
			fprintf(stderr, _("Message at specified location exists. "));
			while (confirm < 0) {
				fprintf(stderr, _("Overwrite? (yes/no) "));
				gn_get_line(stdin, ans, 7);
				if (!strcmp(ans, _("yes"))) confirm = 1;
				else if (!strcmp(ans, _("no"))) confirm = 0;
			}
			if (!confirm) {
				return 0;
			}
			else break;
		case GN_ERR_EMPTYLOCATION: /* OK. We can save now. */
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return -1;
		}
	}
#endif
	if ((!sms.smsc.number[0]) && (sms.type == GN_SMS_MT_Deliver)) {
		data.message_center = calloc(1, sizeof(gn_sms_message_center));
		data.message_center->id = 1;
		if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
			snprintf(sms.smsc.number, GN_BCD_STRING_MAX_LENGTH, "%s", data.message_center->smsc.number);
			dprintf("SMSC number: %s\n", sms.smsc.number);
			sms.smsc.type = data.message_center->smsc.type;
		}
		free(data.message_center);
	}

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:"));

	chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);

	fprintf(stderr, _("storing sms"));

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return -1;
	} else if (chars_read > input_len || chars_read > sizeof(sms.user_data[0].u.text) - 1) {
		fprintf(stderr, _("Input too long!\n"));
		return -1;
	}

	if (chars_read > 0 && message_buffer[chars_read - 1] == '\n') message_buffer[--chars_read] = 0x00;
	if (chars_read < 1) {
		fprintf(stderr, _("Empty message. Quitting"));
		return -1;
	}
	if (memory_type[0] != '\0')
		sms.memory_type = gn_str2memory_type(memory_type);

	sms.user_data[0].type = GN_SMS_DATA_Text;
	strncpy(sms.user_data[0].u.text, message_buffer, chars_read);
	sms.user_data[0].u.text[chars_read] = 0;
	sms.user_data[1].type = GN_SMS_DATA_None;
	if (!gn_char_def_alphabet(sms.user_data[0].u.text))
		sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;

	data.sms = &sms;
	error = gn_sms_save(&data, &state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Saved to %d!\n"), sms.number);
	else
		fprintf(stderr, _("Saving failed (%s)\n"), gn_error_print(error));

	return error;
}

/* Get SMSC number */
static int getsmsc(int argc, char *argv[])
{
	gn_sms_message_center message_center;
	gn_data data;
	gn_error error;
	int start, stop, i;
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	if (argc > optind) {
		start = atoi(argv[optind]);
		stop = (argc > optind+1) ? atoi(argv[optind+1]) : start;

		if (start > stop) {
			fprintf(stderr, _("Starting SMS center number is greater than stop\n"));
			return -1;
		}

		if (start < 1) {
			fprintf(stderr, _("SMS center number must be greater than 1\n"));
			return -1;
		}
	} else {
		start = 1;
		stop = 5;	/* FIXME: determine it */
	}

	gn_data_clear(&data);
	data.message_center = &message_center;

	for (i = start; i <= stop; i++) {
		memset(&message_center, 0, sizeof(message_center));
		message_center.id = i;

		error = gn_sm_functions(GN_OP_GetSMSCenter, &data, &state);
		switch (error) {
		case GN_ERR_NONE:
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return error;
		}

		if (raw) {
			fprintf(stdout, "%d;%s;%d;%d;%d;%d;%s;%d;%s\n",
				message_center.id, message_center.name,
				message_center.default_name, message_center.format,
				message_center.validity,
				message_center.smsc.type, message_center.smsc.number,
				message_center.recipient.type, message_center.recipient.number);
		} else {
			if (message_center.default_name == -1)
				fprintf(stdout, _("No. %d: \"%s\" (defined name)\n"), message_center.id, message_center.name);
			else
				fprintf(stdout, _("No. %d: \"%s\" (default name)\n"), message_center.id, message_center.name);
			fprintf(stdout, _("SMS center number is %s\n"), message_center.smsc.number);
			fprintf(stdout, _("Default recipient number is %s\n"), message_center.recipient.number);
			fprintf(stdout, _("Messages sent as "));

			switch (message_center.format) {
			case GN_SMS_MF_Text:
				fprintf(stdout, _("Text"));
				break;
			case GN_SMS_MF_Voice:
				fprintf(stdout, _("VoiceMail"));
				break;
			case GN_SMS_MF_Fax:
				fprintf(stdout, _("Fax"));
				break;
			case GN_SMS_MF_Email:
			case GN_SMS_MF_UCI:
				fprintf(stdout, _("Email"));
				break;
			case GN_SMS_MF_ERMES:
				fprintf(stdout, _("ERMES"));
				break;
			case GN_SMS_MF_X400:
				fprintf(stdout, _("X.400"));
				break;
			default:
				fprintf(stdout, _("Unknown"));
				break;
			}

			fprintf(stdout, "\n");
			fprintf(stdout, _("Message validity is "));

			switch (message_center.validity) {
			case GN_SMS_VP_1H:
				fprintf(stdout, _("1 hour"));
				break;
			case GN_SMS_VP_6H:
				fprintf(stdout, _("6 hours"));
				break;
			case GN_SMS_VP_24H:
				fprintf(stdout, _("24 hours"));
				break;
			case GN_SMS_VP_72H:
				fprintf(stdout, _("72 hours"));
				break;
			case GN_SMS_VP_1W:
				fprintf(stdout, _("1 week"));
				break;
			case GN_SMS_VP_Max:
				fprintf(stdout, _("Maximum time"));
				break;
			default:
				fprintf(stdout, _("Unknown"));
				break;
			}

			fprintf(stdout, "\n");
		}
	}

	return 0;
}

/* Set SMSC number */
static int setsmsc()
{
	gn_sms_message_center message_center;
	gn_data data;
	gn_error error;
	char line[256], ch;
	int n;

	gn_data_clear(&data);
	data.message_center = &message_center;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}

		memset(&message_center, 0, sizeof(message_center));
		n = sscanf(line, "%d;%19[^;];%d;%d;%d;%d;%39[^;];%d;%39[^;\n]%c",
			   &message_center.id, message_center.name,
			   &message_center.default_name,
			   (int *)&message_center.format, (int *)&message_center.validity,
			   (int *)&message_center.smsc.type, message_center.smsc.number,
			   (int *)&message_center.recipient.type, message_center.recipient.number,
			   &ch);
		switch (n) {
		case 6:
			message_center.smsc.number[0] = 0;
		case 7:
			message_center.recipient.number[0] = 0;
		case 8:
			break;
		default:
			fprintf(stderr, _("Input line format isn't valid\n"));
			return -1;
		}

		error = gn_sm_functions(GN_OP_SetSMSCenter, &data, &state);
		if (error != GN_ERR_NONE) {
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return 0;
}

/* Creating SMS folder. */
static int createsmsfolder(char *name)
{
	gn_sms_folder	folder;
	gn_data		data;
	gn_error	error;

	gn_data_clear(&data);

	snprintf(folder.name, GN_SMS_FOLDER_NAME_MAX_LENGTH, "%s", name);
	data.sms_folder = &folder;

	error = gn_sm_functions(GN_OP_CreateSMSFolder, &data, &state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	else 
		fprintf(stderr, _("Folder with name: %s successfully created!\n"), folder.name);
	return error;
}

/* Creating SMS folder. */
static int deletesmsfolder(char *number)
{
	gn_sms_folder	folder;
	gn_data		data;
	gn_error	error;

	gn_data_clear(&data);

	folder.folder_id = atoi(number);
	if (folder.folder_id > 0 && folder.folder_id <= GN_SMS_FOLDER_MAX_NUMBER)
		data.sms_folder = &folder;
	else
		fprintf(stderr, _("Error: Number must be between 1 and %i!\n"), GN_SMS_FOLDER_MAX_NUMBER);

	error = gn_sm_functions(GN_OP_DeleteSMSFolder, &data, &state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	else 
		fprintf(stderr, _("Number %i of 'My Folders' successfully deleted!\n"), folder.folder_id);
	return error;
}

static int showsmsfolderstatus(void)
{
	gn_sms_folder_list folders;
	gn_data data;
	gn_error error;
	int i;

	memset(&folders, 0, sizeof(folders));
	gn_data_clear(&data);
	data.sms_folder_list = &folders;

	if ((error = gn_sm_functions(GN_OP_GetSMSFolders, &data, &state)) != GN_ERR_NONE) {
		fprintf(stderr, _("Cannot list available folders: %s\n"), gn_error_print(error));
		return error;
	}

	fprintf(stdout, _("No. Name                             Id #Msg\n"));
	fprintf(stdout, _("============================================\n"));
	for (i = 0; i < folders.number; i++) {
		data.sms_folder = folders.folder + i;
		if ((error = gn_sm_functions(GN_OP_GetSMSFolderStatus, &data, &state)) != GN_ERR_NONE) {
			fprintf(stderr, _("Cannot stat folder \"%s\": %s\n"), folders.folder[i].name, gn_error_print(error));
			return error;
		}
		fprintf(stdout, _("%3d %-32s %2u %4u\n"), i, folders.folder[i].name, folders.folder_id[i], folders.folder[i].number);
	}

	return GN_ERR_NONE;
}


/* Get SMS messages. */
static int getsms(int argc, char *argv[])
{
	int del = 0;
	gn_sms_folder folder;
	gn_sms_folder_list folderlist;
	gn_sms message;
	char *memory_type_string;
	int start_message, end_message, count, mode = 1;
	char filename[64];
	gn_error error = GN_ERR_NONE;
	gn_bmp bitmap;
	gn_phone *phone = &state.driver.phone;
	char ans[5];
	struct stat buf;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[2];
	if (gn_str2memory_type(memory_type_string) == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[2]);
		return -1;
	}

	memset(&filename, 0, sizeof(filename));

	start_message = end_message = atoi(argv[3]);
	if (argc > 4) {
		int i;

		/* [end] can be only argv[4] */
		if (argv[4][0] != '-') {
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
					fprintf(stderr, _("Saving into %s\n"), optarg);
					memset(&filename, 0, sizeof(filename));
					strncpy(filename, optarg, sizeof(filename) - 1);
					if (strlen(optarg) > sizeof(filename) - 1)
						fprintf(stderr, _("Filename too long - will be truncated to 63 characters.\n"));
				} else usage(stderr, -1);
				break;
			default:
				usage(stderr, -1);
			}
		}
	}
	folder.folder_id = 0;
	data.sms_folder = &folder;
	data.sms_folder_list = &folderlist;
	/* Now retrieve the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		bool done = false;
		int offset = 0;

		memset(&message, 0, sizeof(gn_sms));
		message.memory_type = gn_str2memory_type(memory_type_string);
		message.number = count;
		data.sms = &message;

		error = gn_sms_get(&data, &state);
		switch (error) {
		case GN_ERR_NONE:
			switch (message.type) {
			case GN_SMS_MT_Submit:
				fprintf(stdout, _("%d. MO Message "), message.number);
				switch (message.status) {
				case GN_SMS_Read:
					fprintf(stdout, _("(read)\n"));
					break;
				case GN_SMS_Unread:
					fprintf(stdout, _("(unread)\n"));
					break;
				case GN_SMS_Sent:
					fprintf(stdout, _("(sent)\n"));
					break;
				case GN_SMS_Unsent:
					fprintf(stdout, _("(unsent)\n"));
					break;
				default:
					fprintf(stdout, _("(read)\n"));
					break;
				}
				fprintf(stdout, _("Text: %s\n\n"), message.user_data[0].u.text);
				break;
			case GN_SMS_MT_DeliveryReport:
				fprintf(stdout, _("%d. Delivery Report "), message.number);
				if (message.status)
					fprintf(stdout, _("(read)\n"));
				else
					fprintf(stdout, _("(not read)\n"));
				fprintf(stdout, _("Sending date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.smsc_time.day, message.smsc_time.month, message.smsc_time.year, \
					message.smsc_time.hour, message.smsc_time.minute, message.smsc_time.second);
				if (message.smsc_time.timezone) {
					if (message.smsc_time.timezone > 0)
						fprintf(stdout,_("+%02d00"), message.smsc_time.timezone);
					else
						fprintf(stdout,_("%02d00"), message.smsc_time.timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Response date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.time.day, message.time.month, message.time.year, \
					message.time.hour, message.time.minute, message.time.second);
				if (message.smsc_time.timezone) {
					if (message.time.timezone > 0)
						fprintf(stdout,_("+%02d00"), message.time.timezone);
					else
						fprintf(stdout,_("%02d00"), message.time.timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Receiver: %s Msg Center: %s\n"), message.remote.number, message.smsc.number);
				fprintf(stdout, _("Text: %s\n\n"), message.user_data[0].u.text);
				break;
			case GN_SMS_MT_Picture:
				fprintf(stdout, _("Picture Message\n"));
				fprintf(stdout, _("Date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.smsc_time.day, message.smsc_time.month, message.smsc_time.year, \
					message.smsc_time.hour, message.smsc_time.minute, message.smsc_time.second);
				if (message.smsc_time.timezone) {
					if (message.smsc_time.timezone > 0)
						fprintf(stdout,_("+%02d00"), message.smsc_time.timezone);
					else
						fprintf(stdout,_("%02d00"), message.smsc_time.timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg center: %s\n"), message.remote.number, message.smsc.number);
				fprintf(stdout, _("Bitmap:\n"));
				gn_bmp_print(&message.user_data[0].u.bitmap, stdout);
				fprintf(stdout, _("Text:\n"));
				fprintf(stdout, "%s\n", message.user_data[1].u.text);
				break;
			default:
				fprintf(stdout, _("%d. Inbox Message "), message.number);
				switch (message.status) {
				case GN_SMS_Read:
					fprintf(stdout, _("(read)\n"));
					break;
				case GN_SMS_Unread:
					fprintf(stdout, _("(unread)\n"));
					break;
				case GN_SMS_Sent:
					fprintf(stdout, _("(sent)\n"));
					break;
				case GN_SMS_Unsent:
					fprintf(stdout, _("(unsent)\n"));
					break;
				default:
					fprintf(stdout, _("(read)\n"));
					break;
				}
				fprintf(stdout, _("Date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.smsc_time.day, message.smsc_time.month, message.smsc_time.year, \
					message.smsc_time.hour, message.smsc_time.minute, message.smsc_time.second);
				if (message.smsc_time.timezone) {
					if (message.smsc_time.timezone > 0)
						fprintf(stdout,_("+%02d00"), message.smsc_time.timezone);
					else
						fprintf(stdout,_("%02d00"), message.smsc_time.timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.remote.number, message.smsc.number);
				/* No UDH */
				if (!message.udh.number) message.udh.udh[0].type = GN_SMS_UDH_None;
				switch (message.udh.udh[0].type) {
				case GN_SMS_UDH_None:
					fprintf(stdout, _("Text:\n"));
					break;
				case GN_SMS_UDH_OpLogo:
					fprintf(stdout, _("GSM operator logo for %s (%s) network.\n"), bitmap.netcode, gn_network_name_get(bitmap.netcode));
					if (!strcmp(message.remote.number, "+998000005") && !strcmp(message.smsc.number, "+886935074443")) fprintf(stdout, _("Saved by Logo Express\n"));
					if (!strcmp(message.remote.number, "+998000002") || !strcmp(message.remote.number, "+998000003")) fprintf(stdout, _("Saved by Operator Logo Uploader by Thomas Kessler\n"));
					offset = 3;
				case GN_SMS_UDH_CallerIDLogo:
					fprintf(stdout, _("Logo:\n"));
					/* put bitmap into bitmap structure */
					gn_bmp_sms_read(GN_BMP_OperatorLogo, message.user_data[0].u.text + 2 + offset, message.user_data[0].u.text, &bitmap);
					gn_bmp_print(&bitmap, stdout);
					if (*filename) {
						error = GN_ERR_NONE;
						if ((stat(filename, &buf) == 0)) {
							fprintf(stdout, _("File %s exists.\n"), filename);
							fprintf(stdout, _("Overwrite? (yes/no) "));
							gn_line_get(stdin, ans, 4);
							if (!strcmp(ans, _("yes"))) {
								error = gn_file_bitmap_save(filename, &bitmap, phone);
							}
						} else error = gn_file_bitmap_save(filename, &bitmap, phone);
						if (error != GN_ERR_NONE) fprintf(stderr, _("Couldn't save logofile %s!\n"), filename);
					}
					done = true;
					break;
				case GN_SMS_UDH_Ringtone:
					fprintf(stdout, _("Ringtone\n"));
					done = true;
					break;
				case GN_SMS_UDH_ConcatenatedMessages:
					fprintf(stdout, _("Linked (%d/%d):\n"),
						message.udh.udh[0].u.concatenated_short_message.current_number,
						message.udh.udh[0].u.concatenated_short_message.maximum_number);
					break;
				case GN_SMS_UDH_WAPvCard:
					fprintf(stdout, _("WAP vCard:\n"));
					break;
				case GN_SMS_UDH_WAPvCalendar:
					fprintf(stdout, _("WAP vCalendar:\n"));
					break;
				case GN_SMS_UDH_WAPvCardSecure:
					fprintf(stdout, _("WAP vCardSecure:\n"));
					break;
				case GN_SMS_UDH_WAPvCalendarSecure:
					fprintf(stdout, _("WAP vCalendarSecure:\n"));
					break;
				default:
					fprintf(stderr, _("Unknown\n"));
					break;
				}
				if (done) break;
				fprintf(stdout, "%s\n", message.user_data[0].u.text);
				if ((mode != -1) && *filename) {
					char buf[1024];
					sprintf(buf, "%s%d", filename, count);
					mode = gn_file_text_save(buf, message.user_data[0].u.text, mode);
				}
				break;
			}
			if (del) {
				data.sms = &message;
				if (GN_ERR_NONE != gn_sms_delete(&data, &state))
					fprintf(stderr, _("(delete failed)\n"));
				else
					fprintf(stderr, _("(message deleted)\n"));
			}
			break;
		default:
			fprintf(stderr, _("GetSMS %s %d failed! (%s)\n"), memory_type_string, count, gn_error_print(error));
			if (error == GN_ERR_INVALIDMEMORYTYPE)
				fprintf(stderr, _("See the gnokii manual page for the supported memory types with the phone\nyou use.\n"));
			break;
		}
	}

	/* FIXME: We return the value of the last read.
	 * What should we return?
	 */
	return error;
}

/* Delete SMS messages. */
static int deletesms(int argc, char *argv[])
{
	gn_sms message;
	gn_sms_folder folder;
	gn_sms_folder_list folderlist;
	char *memory_type_string;
	int start_message, end_message, count;
	gn_error error = GN_ERR_NONE;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	message.memory_type = gn_str2memory_type(memory_type_string);
	if (message.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return -1;
	}

	start_message = end_message = atoi(argv[1]);
	if (argc > 2) end_message = atoi(argv[2]);

	/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		message.number = count;
		data.sms = &message;
		data.sms_folder = &folder;
		data.sms_folder_list = &folderlist;
		error = gn_sms_delete(&data, &state);

		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Deleted SMS %s %d\n"), memory_type_string, count);
		else
			fprintf(stderr, _("DeleteSMS %s %d failed!(%s)\n\n"), memory_type_string, count, gn_error_print(error));
	}

	/* FIXME: We return the value of the last read.
	 * What should we return?
	 */
	return error;
}

static volatile bool bshutdown = false;

/* SIGINT signal handler. */
static void interrupted(int sig)
{
	signal(sig, SIG_IGN);
	bshutdown = true;
}

#ifdef SECURITY

int get_password(const char *prompt, char *pass, int length)
{
#ifdef WIN32
	fprintf(stdout, "%s", prompt);
	fgets(pass, length, stdin);
#else
	/* FIXME: manual says: Do not use it */
	strncpy(pass, getpass(prompt), length - 1);
	pass[length - 1] = 0;
#endif

	return 0;
}

/* In this mode we get the code from the keyboard and send it to the mobile
   phone. */
static int entersecuritycode(char *type)
{
	gn_error error;
	gn_security_code security_code;

	if (!strcmp(type, "PIN"))
		security_code.type = GN_SCT_Pin;
	else if (!strcmp(type, "PUK"))
		security_code.type = GN_SCT_Puk;
	else if (!strcmp(type, "PIN2"))
		security_code.type = GN_SCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		security_code.type = GN_SCT_Puk2;
	/* FIXME: Entering of security_code does not work :-(
	else if (!strcmp(type, "security_code"))
		security_code.type = GN_SCT_security_code;
	*/
	else
		usage(stderr, -1);

	memset(&security_code.code, 0, sizeof(security_code.code));
	get_password(_("Enter your code: "), security_code.code, sizeof(security_code.code));

	gn_data_clear(&data);
	data.security_code = &security_code;

	error = gn_sm_functions(GN_OP_EnterSecurityCode, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Code ok.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}
	return error;
}

static int getsecuritycodestatus(void)
{
	gn_security_code security_code;
	gn_error err;

	gn_data_clear(&data);
	data.security_code = &security_code;

	err = gn_sm_functions(GN_OP_GetSecurityCodeStatus, &data, &state);
	if (err == GN_ERR_NONE) {
		fprintf(stdout, _("Security code status: "));

		switch(security_code.type) {
		case GN_SCT_SecurityCode:
			fprintf(stdout, _("waiting for Security Code.\n"));
			break;
		case GN_SCT_Pin:
			fprintf(stdout, _("waiting for PIN.\n"));
			break;
		case GN_SCT_Pin2:
			fprintf(stdout, _("waiting for PIN2.\n"));
			break;
		case GN_SCT_Puk:
			fprintf(stdout, _("waiting for PUK.\n"));
			break;
		case GN_SCT_Puk2:
			fprintf(stdout, _("waiting for PUK2.\n"));
			break;
		case GN_SCT_None:
			fprintf(stdout, _("nothing to enter.\n"));
			break;
		default:
			fprintf(stdout, _("Unknown!\n"));
			break;
		}
	} else
		fprintf(stderr, _("Error: %s\n"), gn_error_print(err));

	return err;
}

static int changesecuritycode(char *type)
{
	gn_error error;
	gn_security_code security_code;
	char newcode2[10];

	memset(&security_code, 0, sizeof(security_code));

	if (!strcmp(type, "PIN"))
		security_code.type = GN_SCT_Pin;
	else if (!strcmp(type, "PUK"))
		security_code.type = GN_SCT_Puk;
	else if (!strcmp(type, "PIN2"))
		security_code.type = GN_SCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		security_code.type = GN_SCT_Puk2;
	/* FIXME: Entering of security_code does not work :-(
	else if (!strcmp(type, "security_code"))
		security_code.type = GN_SCT_security_code;
	*/
	else
		usage(stderr, -1);

	get_password(_("Enter your code: "), security_code.code, sizeof(security_code.code));
	get_password(_("Enter new code: "), security_code.new_code, sizeof(security_code.new_code));
	get_password(_("Retype new code: "), newcode2, sizeof(newcode2));
	if (strcmp(security_code.new_code, newcode2)) {
		fprintf(stdout, _("Error: new code differ\n"));
		return -1;
	}

	gn_data_clear(&data);
	data.security_code = &security_code;

	error = gn_sm_functions(GN_OP_ChangeSecurityCode, &data, &state);
	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("Code changed.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

#endif

static void callnotifier(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state)
{
	switch (call_status) {
	case GN_CALL_Incoming:
		fprintf(stdout, _("INCOMING CALL: ID: %d, Number: %s, Name: \"%s\"\n"), call_info->call_id, call_info->number, call_info->name);
		break;
	case GN_CALL_LocalHangup:
		fprintf(stdout, _("CALL %d TERMINATED (LOCAL)\n"), call_info->call_id);
		break;
	case GN_CALL_RemoteHangup:
		fprintf(stdout, _("CALL %d TERMINATED (REMOTE)\n"), call_info->call_id);
		break;
	case GN_CALL_Established:
		fprintf(stdout, _("CALL %d ACCEPTED BY THE REMOTE SIDE\n"), call_info->call_id);
		break;
	case GN_CALL_Held:
		fprintf(stdout, _("CALL %d PLACED ON HOLD\n"), call_info->call_id);
		break;
	case GN_CALL_Resumed:
		fprintf(stdout, _("CALL %d RETRIEVED FROM HOLD\n"), call_info->call_id);
		break;
	default:
		break;
	}

	gn_call_notifier(call_status, call_info, state);
}

/* Voice dialing mode. */
static int dialvoice(char *number)
{
    	gn_call_info call_info;
	gn_error error;
	int call_id;

	memset(&call_info, 0, sizeof(call_info));
	snprintf(call_info.number, sizeof(call_info.number), "%s", number);
	call_info.type = GN_CALL_Voice;
	call_info.send_number = GN_CALL_Default;

	gn_data_clear(&data);
	data.call_info = &call_info;

	if ((error = gn_call_dial(&call_id, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Dialing failed: %s\n"), gn_error_print(error));
	else
		fprintf(stdout, _("Dialled call, id: %d (lowlevel id: %d)\n"), call_id, call_info.call_id);

	return error;
}

/* Answering incoming call */
static int answercall(char *callid)
{
    	gn_call_info callinfo;

	memset(&callinfo, 0, sizeof(callinfo));
	callinfo.call_id = atoi(callid);

	gn_data_clear(&data);
	data.call_info = &callinfo;

	return gn_sm_functions(GN_OP_AnswerCall, &data, &state);
}

/* Hangup the call */
static int hangup(char *callid)
{
    	gn_call_info callinfo;

	memset(&callinfo, 0, sizeof(callinfo));
	callinfo.call_id = atoi(callid);

	gn_data_clear(&data);
	data.call_info = &callinfo;

	return gn_sm_functions(GN_OP_CancelCall, &data, &state);
}


static gn_bmp_types set_bitmap_type(char *s)
{
	if (!strcmp(s, "text"))         return GN_BMP_WelcomeNoteText;
	if (!strcmp(s, "dealer"))       return GN_BMP_DealerNoteText;
	if (!strcmp(s, "op"))           return GN_BMP_OperatorLogo;
	if (!strcmp(s, "startup"))      return GN_BMP_StartupLogo;
	if (!strcmp(s, "caller"))       return GN_BMP_CallerLogo;
	if (!strcmp(s, "picture"))      return GN_BMP_PictureMessage;
	if (!strcmp(s, "emspicture"))   return GN_BMP_EMSPicture;
	if (!strcmp(s, "emsanimation")) return GN_BMP_EMSAnimation;
	return GN_BMP_None;
}

/* FIXME: Integrate with sendsms */
/* The following function allows to send logos using SMS */
static int sendlogo(int argc, char *argv[])
{
	gn_sms sms;
	gn_error error;
	int type;

	gn_sms_default_submit(&sms);

	/* The first argument is the type of the logo. */
	switch (type = set_bitmap_type(argv[0])) {
	case GN_BMP_OperatorLogo:   fprintf(stderr, _("Sending operator logo.\n")); break;
	case GN_BMP_CallerLogo:     fprintf(stderr, _("Sending caller line identification logo.\n")); break;
	case GN_BMP_PictureMessage: fprintf(stderr,_("Sending Multipart Message: Picture Message.\n")); break;
	case GN_BMP_EMSPicture:     fprintf(stderr, _("Sending EMS-compliant Picture Message.\n")); break;
	case GN_BMP_EMSAnimation:   fprintf(stderr, _("Sending EMS-compliant Animation.\n")); break;
	default: 	            fprintf(stderr, _("You should specify what kind of logo to send!\n")); return -1;
	}

	sms.user_data[0].type = GN_SMS_DATA_Bitmap;

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&sms.remote.number, 0, sizeof(sms.remote.number));
	strncpy(sms.remote.number, argv[1], sizeof(sms.remote.number) - 1);
	if (sms.remote.number[0] == '+') sms.remote.type = GN_GSM_NUMBER_International;
	else sms.remote.type = GN_GSM_NUMBER_Unknown;

	if (loadbitmap(&sms.user_data[0].u.bitmap, argv[2], type) != GN_ERR_NONE)
		return -1;
	if (type != sms.user_data[0].u.bitmap.type) {
		fprintf(stderr, _("Cannot send logo: specified and loaded bitmap type differ!\n"));
		return -1;
	}

	/* If we are sending op logo we can rewrite network code. */
	if ((sms.user_data[0].u.bitmap.type == GN_BMP_OperatorLogo) && (argc > 3)) {
		/*
		 * The fourth argument, if present, is the Network code of the operator.
		 * Network code is in this format: "xxx yy".
		 */
		memset(sms.user_data[0].u.bitmap.netcode, 0, sizeof(sms.user_data[0].u.bitmap.netcode));
		strncpy(sms.user_data[0].u.bitmap.netcode, argv[3], sizeof(sms.user_data[0].u.bitmap.netcode) - 1);
		dprintf("Operator code: %s\n", argv[3]);
	}

	sms.user_data[1].type = GN_SMS_DATA_None;
	if (sms.user_data[0].u.bitmap.type == GN_BMP_PictureMessage) {
		sms.user_data[1].type = GN_SMS_DATA_NokiaText;
		readtext(&sms.user_data[1], 120);
		sms.user_data[2].type = GN_SMS_DATA_None;
	}

	data.message_center = calloc(1, sizeof(gn_sms_message_center));
	data.message_center->id = 1;
	if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
		strcpy(sms.smsc.number, data.message_center->smsc.number);
		sms.smsc.type = data.message_center->smsc.type;
	}
	free(data.message_center);

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	/* Send the message. */
	data.sms = &sms;
	error = gn_sms_send(&data, &state);

	if (error == GN_ERR_NONE) fprintf(stderr, _("Send succeeded!\n"));
	else fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	return error;
}

/* Getting logos. */
static gn_error SaveBitmapFileDialog(char *FileName, gn_bmp *bitmap, gn_phone *info)
{
	int confirm;
	char ans[4];
	struct stat buf;
	gn_error error;

	/* Ask before overwriting */
	while (stat(FileName, &buf) == 0) {
		confirm = 0;
		while (!confirm) {
			fprintf(stderr, _("Saving logo. File \"%s\" exists. (O)verwrite, create (n)ew or (s)kip ? "), FileName);
			gn_line_get(stdin, ans, 4);
			if (!strcmp(ans, _("O")) || !strcmp(ans, _("o"))) confirm = 1;
			if (!strcmp(ans, _("N")) || !strcmp(ans, _("n"))) confirm = 2;
			if (!strcmp(ans, _("S")) || !strcmp(ans, _("s"))) return GN_ERR_USERCANCELED;
		}
		if (confirm == 1) break;
		if (confirm == 2) {
			fprintf(stderr, _("Enter name of new file: "));
			gn_line_get(stdin, FileName, 50);
			if (!FileName || (*FileName == 0)) return GN_ERR_USERCANCELED;
		}
	}

	error = gn_file_bitmap_save(FileName, bitmap, info);

	switch (error) {
	case GN_ERR_FAILED:
		fprintf(stderr, _("Failed to write file \"%s\"\n"), FileName);
		break;
	default:
		break;
	}

	return error;
}

static int getlogo(int argc, char *argv[])
{
	gn_bmp bitmap;
	gn_error error;
	gn_phone *info = &state.driver.phone;

	memset(&bitmap, 0, sizeof(gn_bmp));
	bitmap.type = set_bitmap_type(argv[0]);

	/* There is caller group number missing in argument list. */
	if ((bitmap.type == GN_BMP_CallerLogo) && (argc >= 2)) {
		bitmap.number = (argv[1][0] < '0') ? 0 : argv[1][0] - '0';
		if (bitmap.number > 9) bitmap.number = 0;
		argv++;
		argc--;
	}

	if (bitmap.type != GN_BMP_None) {
		data.bitmap = &bitmap;

		error = gn_sm_functions(GN_OP_GetBitmap, &data, &state);

		gn_bmp_print(&bitmap, stderr);
		switch (error) {
		case GN_ERR_NONE:
			switch (bitmap.type) {
			case GN_BMP_DealerNoteText:
				fprintf(stdout, _("Dealer welcome note "));
				if (bitmap.text[0]) fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				else fprintf(stdout, _("currently empty\n"));
				break;
			case GN_BMP_WelcomeNoteText:
				fprintf(stdout, _("Welcome note "));
				if (bitmap.text[0]) fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				else fprintf(stdout, _("currently empty\n"));
				break;
			case GN_BMP_OperatorLogo:
			case GN_BMP_NewOperatorLogo:
				if (!bitmap.width) goto empty_bitmap;
				fprintf(stdout, _("Operator logo for %s (%s) network got succesfully\n"),
					bitmap.netcode, gn_network_name_get(bitmap.netcode));
				if (argc == 3) {
					strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
					if (!strcmp(gn_network_name_get(bitmap.netcode), "unknown")) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return -1;
					}
				}
				break;
			case GN_BMP_StartupLogo:
				if (!bitmap.width) goto empty_bitmap;
				fprintf(stdout, _("Startup logo got successfully\n"));
				if (argc == 3) {
					strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
					if (!strcmp(gn_network_name_get(bitmap.netcode), "unknown")) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return -1;
					}
				}
				break;
			case GN_BMP_CallerLogo:
				if (!bitmap.width)
					fprintf(stderr, _("Your phone doesn't have logo uploaded !\n"));
				else
					fprintf(stdout, _("Caller logo got successfully\n"));
				fprintf(stdout, _("Caller group name: %s, ringing tone: %s (%d)\n"),
					bitmap.text, get_ringtone_name(bitmap.ringtone), bitmap.ringtone);
				if (argc == 4) {
					strncpy(bitmap.netcode, argv[3], sizeof(bitmap.netcode) - 1);
					if (!strcmp(gn_network_name_get(bitmap.netcode), "unknown")) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return -1;
					}
				}
				break;
			default:
				fprintf(stderr, _("Unknown bitmap type.\n"));
				break;
			empty_bitmap:
				fprintf(stderr, _("Your phone doesn't have logo uploaded !\n"));
				return -1;
			}
			if ((argc > 1) && (SaveBitmapFileDialog(argv[1], &bitmap, info) != GN_ERR_NONE)) return -1;
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			break;
		}
	} else {
		fprintf(stderr, _("What kind of logo do you want to get ?\n"));
		return -1;
	}

	return error;
}


/* Sending logos. */
gn_error ReadBitmapFileDialog(char *FileName, gn_bmp *bitmap, gn_phone *info)
{
	gn_error error;

	error = gn_file_bitmap_read(FileName, bitmap, info);
	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error while reading file \"%s\": %s\n"), FileName, gn_error_print(error));
		break;
	}
	return error;
}


static int setlogo(int argc, char *argv[])
{
	gn_bmp bitmap, oldbit;
	gn_network_info networkinfo;
	gn_error error = GN_ERR_NOTSUPPORTED;
	gn_phone *phone = &state.driver.phone;
	gn_data data;
	bool ok = true;
	int i;

	gn_data_clear(&data);
	data.bitmap = &bitmap;
	data.network_info = &networkinfo;

	memset(&bitmap.text, 0, sizeof(bitmap.text));

	bitmap.type = set_bitmap_type(argv[0]);
	switch (bitmap.type) {
	case GN_BMP_WelcomeNoteText:
	case GN_BMP_DealerNoteText:
		if (argc > 1) strncpy(bitmap.text, argv[1], sizeof(bitmap.text) - 1);
		break;
	case GN_BMP_OperatorLogo:
		error = (argc < 2) ? gn_bmp_null(&bitmap, phone) : ReadBitmapFileDialog(argv[1], &bitmap, phone);
		if (error != GN_ERR_NONE) return error;
			
		memset(&bitmap.netcode, 0, sizeof(bitmap.netcode));
		if (argc < 3)
			if (gn_sm_functions(GN_OP_GetNetworkInfo, &data, &state) == GN_ERR_NONE)
				strncpy(bitmap.netcode, networkinfo.network_code, sizeof(bitmap.netcode) - 1);

		if (!strncmp(state.driver.phone.models, "6510", 4))
			gn_bmp_resize(&bitmap, GN_BMP_NewOperatorLogo, phone);
		else
			gn_bmp_resize(&bitmap, GN_BMP_OperatorLogo, phone);

		if (argc == 3) {
			strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
			if (!strcmp(gn_network_name_get(bitmap.netcode), "unknown")) {
				fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
				return -1;
			}
		}
		break;
	case GN_BMP_StartupLogo:
		if ((argc > 1) && ((error = ReadBitmapFileDialog(argv[1], &bitmap, phone)) != GN_ERR_NONE)) return error;
		gn_bmp_resize(&bitmap, GN_BMP_StartupLogo, phone);
		break;
	case GN_BMP_CallerLogo:
		if ((argc > 1) && ((error = ReadBitmapFileDialog(argv[1], &bitmap, phone)) != GN_ERR_NONE)) return error;
		gn_bmp_resize(&bitmap, GN_BMP_CallerLogo, phone);
		if (argc > 2) {
			bitmap.number = (argv[2][0] < '0') ? 0 : argv[2][0] - '0';
			if (bitmap.number > 9) bitmap.number = 0;
			dprintf("%i \n", bitmap.number);
			oldbit.type = GN_BMP_CallerLogo;
			oldbit.number = bitmap.number;
			data.bitmap = &oldbit;
			if (gn_sm_functions(GN_OP_GetBitmap, &data, &state) == GN_ERR_NONE) {
				/* We have to get the old name and ringtone!! */
				bitmap.ringtone = oldbit.ringtone;
				strncpy(bitmap.text, oldbit.text, sizeof(bitmap.text) - 1);
			}
			if (argc > 3) strncpy(bitmap.text, argv[3], sizeof(bitmap.text) - 1);
		}
		break;
	default:
		fprintf(stderr, _("What kind of logo do you want to set ?\n"));
		return -1;
	}

	data.bitmap = &bitmap;
	error = gn_sm_functions(GN_OP_SetBitmap, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		oldbit.type = bitmap.type;
		oldbit.number = bitmap.number;
		data.bitmap = &oldbit;
		if (gn_sm_functions(GN_OP_GetBitmap, &data, &state) == GN_ERR_NONE) {
			switch (bitmap.type) {
			case GN_BMP_WelcomeNoteText:
			case GN_BMP_DealerNoteText:
				if (strcmp(bitmap.text, oldbit.text)) {
					fprintf(stderr, _("Error setting"));
					if (bitmap.type == GN_BMP_DealerNoteText) fprintf(stderr, _(" dealer"));
					fprintf(stderr, _(" welcome note - "));

					/* I know, it looks horrible, but...
					 * I set it to the short string - if it won't be set
					 * it means, PIN is required. If it will be correct, previous
					 * (user) text was too long.
					 *
					 * Without it, I could have such thing:
					 * user set text to very short string (for example, "Marcin")
					 * then enable phone without PIN and try to set it to the very long (too long for phone)
					 * string (which start with "Marcin"). If we compare them as only length different, we could think,
					 * that phone accepts strings 6 chars length only (length of "Marcin")
					 * When we make it correct, we don't have this mistake
					 */

					strcpy(oldbit.text, "!");
					data.bitmap = &oldbit;
					gn_sm_functions(GN_OP_SetBitmap, &data, &state);
					gn_sm_functions(GN_OP_GetBitmap, &data, &state);
					if (oldbit.text[0] != '!') {
						fprintf(stderr, _("SIM card and PIN is required\n"));
					} else {
						data.bitmap = &bitmap;
						gn_sm_functions(GN_OP_SetBitmap, &data, &state);
						data.bitmap = &oldbit;
						gn_sm_functions(GN_OP_GetBitmap, &data, &state);
						fprintf(stderr, _("too long, truncated to \"%s\" (length %i)\n"),oldbit.text,strlen(oldbit.text));
					}
					ok = false;
				}
				break;
			case GN_BMP_StartupLogo:
				for (i = 0; i < oldbit.size; i++) {
					if (oldbit.bitmap[i] != bitmap.bitmap[i]) {
						fprintf(stderr, _("Error setting startup logo - SIM card and PIN is required\n"));
						fprintf(stderr, _("i: %i, %2x %2x\n"), i, oldbit.bitmap[i], bitmap.bitmap[i]);
						ok = false;
						break;
					}
				}
				break;
			default:
				break;

			}
		}
		if (ok) fprintf(stderr, _("Done.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

static int viewlogo(char *filename)
{
	gn_error error;
	error = gn_file_bitmap_show(filename);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Could not load bitmap: %s\n"), gn_error_print(error));
	return error;
}

/* ToDo notes receiving. */
static int gettodo(int argc, char *argv[])
{
	gn_todo_list	todolist;
	gn_todo		todo;
	gn_data		data;
	gn_error	error = GN_ERR_NONE;
	bool		vcal = false;
	int		i, first_location, last_location;

	struct option options[] = {
		{ "vCal",    optional_argument, NULL, '1'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	first_location = last_location = atoi(argv[0]);
	if ((argc > 1) && (argv[1][0] != '-')) {
		if (!strcasecmp(argv[1], "end"))
			last_location = INT_MAX;
		else
			last_location = atoi(argv[1]);
	}
	if (last_location < first_location)
		last_location = first_location;

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vcal = true;
			break;
		default:
			usage(stderr, -1); /* Would be better to have an calendar_usage() here. */
			return -1;
		}
	}

	for (i = first_location; i <= last_location; i++) {
		todo.location = i;

		gn_data_clear(&data);
		data.todo = &todo;
		data.todo_list = &todolist;

		error = gn_sm_functions(GN_OP_GetToDo, &data, &state);
		switch (error) {
		case GN_ERR_NONE:
			if (vcal) {
				gn_todo2ical(stdout, &todo);
			} else {
				fprintf(stdout, _("Todo: %s\n"), todo.text);
				fprintf(stdout, _("Priority: "));
				switch (todo.priority) {
				case GN_TODO_LOW:
					fprintf(stdout, _("low\n"));
					break;
				case GN_TODO_MEDIUM:
					fprintf(stdout, _("medium\n"));
					break;
				case GN_TODO_HIGH:
					fprintf(stdout, _("high\n"));
					break;
				default:
					fprintf(stdout, _("unknown\n"));
					break;
				}
			}
			break;
		default:
			fprintf(stderr, _("The ToDo note could not be read: %s\n"), gn_error_print(error));
			if (last_location == INT_MAX)
				last_location = 0;
			break;
		}
	}
	return error;
}

/* ToDo notes writing */
static int writetodo(char *argv[])
{
	gn_todo todo;
	gn_data data;
	gn_error error;
	int location;
	FILE *f;

	gn_data_clear(&data);
	data.todo = &todo;

	f = fopen(argv[0], "r");
	if (!f) {
		fprintf(stderr, _("Cannot read file %s\n"), argv[0]);
		return GN_ERR_FAILED;
	}

	location = atoi(argv[1]);

	error = gn_ical2todo(f, &todo, location);
	fclose(f);
#ifndef WIN32
	if (error == GN_ERR_NOTIMPLEMENTED) {
		switch (gn_vcal_file_todo_read(argv[0], &todo, location)) {
		case 0:
			error = GN_ERR_NONE;
			break;
		default:
			error = GN_ERR_FAILED;
			break;
		}
	}
#endif
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load vCalendar file: %s\n"), gn_error_print(error));
		return error;
	}

	error = gn_sm_functions(GN_OP_WriteToDo, &data, &state);

	if (error == GN_ERR_NONE) {
		fprintf(stderr, _("Succesfully written!\n"));
		fprintf(stderr, _("Priority %d. %s\n"), data.todo->priority, data.todo->text);
	} else
		fprintf(stderr, _("Failed to write todo note: %s\n"), gn_error_print(error));
	return error;
}

/* Deleting all ToDo notes */
static int deletealltodos()
{
	gn_data data;
	gn_error error;

	gn_data_clear(&data);

	error = gn_sm_functions(GN_OP_DeleteAllToDos, &data, &state);
	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Succesfully deleted all ToDo notes!\n"));
	else
		fprintf(stderr, _("Failed to delete todo note: %s\n"), gn_error_print(error));
	return error;
}

/* Calendar notes receiving. */
static int getcalendarnote(int argc, char *argv[])
{
	gn_calnote_list		calnotelist;
	gn_calnote		calnote;
	gn_data			data;
	gn_error		error = GN_ERR_NONE;
	int			i, first_location, last_location;
	bool			vcal = false;

	struct option options[] = {
		{ "vCal",    optional_argument, NULL, '1'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	first_location = last_location = atoi(argv[0]);
	if ((argc > 1) && (argv[1][0] != '-')) {
		if (!strcasecmp(argv[1], "end"))
			last_location = INT_MAX;
		else
			last_location = atoi(argv[1]);
	}
	if (last_location < first_location)
		last_location = first_location;

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vcal = true;
			break;
		default:
			getcalendarnote_usage(stderr, -1);
		}
	}

	for (i = first_location; i <= last_location; i++) {
		memset(&calnote, 0, sizeof(calnote));
		memset(&calnotelist, 0, sizeof(calnotelist));
		calnote.location = i;

		gn_data_clear(&data);
		data.calnote = &calnote;
		data.calnote_list = &calnotelist;

		error = gn_sm_functions(GN_OP_GetCalendarNote, &data, &state);

		if (error == GN_ERR_NONE) {
			if (vcal) {
				gn_calnote2ical(stdout, &calnote);
			} else {  /* plaint text output */
				fprintf(stdout, _("   Type of the note: "));

				switch (calnote.type) {
				case GN_CALNOTE_REMINDER:
					fprintf(stdout, _("Reminder\n"));
					break;
				case GN_CALNOTE_CALL:
					fprintf(stdout, _("Call\n"));
					break;
				case GN_CALNOTE_MEETING:
					fprintf(stdout, _("Meeting\n"));
					break;
				case GN_CALNOTE_BIRTHDAY:
					fprintf(stdout, _("Birthday\n"));
					break;
				default:
					fprintf(stdout, _("Unknown\n"));
					break;
				}

				fprintf(stdout, _("   Date: %d-%02d-%02d\n"), calnote.time.year,
					calnote.time.month,
					calnote.time.day);

				fprintf(stdout, _("   Time: %02d:%02d:%02d\n"), calnote.time.hour,
					calnote.time.minute,
					calnote.time.second);

				if (calnote.alarm.enabled) {
					fprintf(stdout, _("   Alarm date: %d-%02d-%02d\n"),
						calnote.alarm.timestamp.year,
						calnote.alarm.timestamp.month,
						calnote.alarm.timestamp.day);

					fprintf(stdout, _("   Alarm time: %02d:%02d:%02d\n"),
						calnote.alarm.timestamp.hour,
						calnote.alarm.timestamp.minute,
						calnote.alarm.timestamp.second);
				}

				switch (calnote.recurrence) {
				case GN_CALNOTE_NEVER:
					break;
				case GN_CALNOTE_DAILY:
					fprintf(stdout, _("   Repeat: every day\n"));
					break;
				case GN_CALNOTE_WEEKLY:
					fprintf(stdout, _("   Repeat: every week\n"));
					break;
				case GN_CALNOTE_2WEEKLY:
					fprintf(stdout, _("   Repeat: every 2 weeks\n"));
					break;
				case GN_CALNOTE_MONTHLY:
					fprintf(stdout, _("   Repeat: every month\n"));
					break;
				case GN_CALNOTE_YEARLY:
					fprintf(stdout, _("   Repeat: every year\n"));
					break;
				default:
					fprintf(stdout, _("   Repeat: %d hours\n"), calnote.recurrence);
					break;
				}

				fprintf(stdout, _("   Text: %s\n"), calnote.text);

				if (calnote.type == GN_CALNOTE_CALL)
					fprintf(stdout, _("   Phone: %s\n"), calnote.phone_number);
			}

		} else { /* error != GN_ERR_NONE */
			fprintf(stderr, _("The calendar note can not be read: %s\n"), gn_error_print(error));
			/* stop processing if the last note was specified as "end" */
			if (last_location == INT_MAX)
				last_location = 0;
		}
	}

	return error;
}

/* Writing calendar notes. */
static int writecalendarnote(char *argv[])
{
	gn_calnote calnote;
	gn_data data;
	gn_error error;
	int location;
	FILE *f;

	gn_data_clear(&data);
	data.calnote = &calnote;

	f = fopen(argv[0], "r");
	if (f == NULL) {
		fprintf(stderr, _("Cannot read file %s\n"), argv[0]);
		return GN_ERR_FAILED;
	}

	location = atoi(argv[1]);

	error = gn_ical2calnote(f, &calnote, location);
	fclose(f);
#ifndef WIN32
	if (error == GN_ERR_NOTIMPLEMENTED) {
		switch (gn_vcal_file_event_read(argv[0], &calnote, location)) {
		case 0:
			error = GN_ERR_NONE;
			break;
		default:
			error = GN_ERR_FAILED;
			break;
		}
	}
#endif
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load vCalendar file: %s\n"), gn_error_print(error));
		return error;
	}
	
	error = gn_sm_functions(GN_OP_WriteCalendarNote, &data, &state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Succesfully written!\n"));
	else
		fprintf(stderr, _("Failed to write calendar note: %s\n"), gn_error_print(error));
	return error;
}

/* Calendar note deleting. */
static int deletecalendarnote(int argc, char *argv[])
{
	gn_calnote calnote;
	gn_calnote_list clist;
	int i, first_location, last_location;
	gn_data data;
	gn_error error = GN_ERR_NONE;

	gn_data_clear(&data);
	memset(&calnote, 0, sizeof(gn_calnote));
	data.calnote = &calnote;
	data.calnote_list = &clist;

	first_location = last_location = atoi(argv[0]);
	if ((argc > 1) && (argv[1][0] != '-')) {
		if (!strcasecmp(argv[1], "end"))
			last_location = INT_MAX;
		else
			last_location = atoi(argv[1]);
	}
	if (last_location < first_location)
		last_location = first_location;

	for (i = first_location; i <= last_location; i++) {

		calnote.location = i;

		error = gn_sm_functions(GN_OP_DeleteCalendarNote, &data, &state);
		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Calendar note deleted.\n"));
		else {
			fprintf(stderr, _("The calendar note cannot be deleted: %s\n"), gn_error_print(error));
			if (last_location == INT_MAX)
				last_location = 0;
		}
	}

	return error;
}

/* Setting the date and time. */
static int setdatetime(int argc, char *argv[])
{
	struct tm *now;
	time_t nowh;
	gn_timestamp date;
	gn_error error;

	nowh = time(NULL);
	now = localtime(&nowh);

	date.year = now->tm_year;
	date.month = now->tm_mon+1;
	date.day = now->tm_mday;
	date.hour = now->tm_hour;
	date.minute = now->tm_min;
	date.second = now->tm_sec;

	if (argc > 0) date.year = atoi(argv[0]);
	if (argc > 1) date.month = atoi(argv[1]);
	if (argc > 2) date.day = atoi(argv[2]);
	if (argc > 3) date.hour = atoi (argv[3]);
	if (argc > 4) date.minute = atoi(argv[4]);

	if (date.year < 1900) {

		/* Well, this thing is copyrighted in U.S. This technique is known as
		   Windowing and you can read something about it in LinuxWeekly News:
		   http://lwn.net/1999/features/Windowing.phtml. This thing is beeing
		   written in Czech republic and Poland where algorithms are not allowed
		   to be patented. */

		if (date.year > 90)
			date.year = date.year + 1900;
		else
			date.year = date.year + 2000;
	}

	gn_data_clear(&data);
	data.datetime = &date;
	error = gn_sm_functions(GN_OP_SetDateTime, &data, &state);

	return error;
}

/* In this mode we receive the date and time from mobile phone. */
static int getdatetime(void)
{
	gn_data	data;
	gn_timestamp	date_time;
	gn_error	error;

	gn_data_clear(&data);
	data.datetime = &date_time;

	error = gn_sm_functions(GN_OP_GetDateTime, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.year, date_time.month, date_time.day);
		fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.hour, date_time.minute, date_time.second);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Setting the alarm. */
static int setalarm(int argc, char *argv[])
{
	gn_calnote_alarm alarm;
	gn_error error;

	if (argc == 2) {
		alarm.timestamp.hour = atoi(argv[0]);
		alarm.timestamp.minute = atoi(argv[1]);
		alarm.timestamp.second = 0;
		alarm.enabled = true;
	} else {
		alarm.timestamp.hour = 0;
		alarm.timestamp.minute = 0;
		alarm.timestamp.second = 0;
		alarm.enabled = false;
	}

	gn_data_clear(&data);
	data.alarm = &alarm;

	error = gn_sm_functions(GN_OP_SetAlarm, &data, &state);

	return error;
}

/* Getting the alarm. */
static int getalarm(void)
{
	gn_error error;
	gn_calnote_alarm alarm;

	gn_data_clear(&data);
	data.alarm = &alarm;

	error = gn_sm_functions(GN_OP_GetAlarm, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("Alarm: %s\n"), (alarm.enabled)?"on":"off");
		fprintf(stdout, _("Time: %02d:%02d\n"), alarm.timestamp.hour, alarm.timestamp.minute);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

static void storecbmessage(gn_cb_message *message)
{
	if (cb_queue[cb_widx].is_new) {
		/* queue is full */
		return;
	}

	cb_queue[cb_widx] = *message;
	cb_widx = (cb_widx + 1) % (sizeof(cb_queue) / sizeof(gn_cb_message));
}

static gn_error readcbmessage(gn_cb_message *message)
{
	if (!cb_queue[cb_ridx].is_new)
		return GN_ERR_NONEWCBRECEIVED;
	
	*message = cb_queue[cb_ridx];
	cb_queue[cb_ridx].is_new = false;
	cb_ridx = (cb_ridx + 1) % (sizeof(cb_queue) / sizeof(gn_cb_message));

	return GN_ERR_NONE;
}

static void displaycall(int call_id)
{
/* FIXME!!! */
#ifndef WIN32
	gn_call *call;
	struct timeval now, delta;
	char *s;

	if ((call = gn_call_get_active(call_id)) == NULL) {
		fprintf(stdout, _("CALL%d: IDLE\n"), call_id);
		return;
	}

	gettimeofday(&now, NULL);
	switch (call->status) {
	case GN_CALL_Ringing:
	case GN_CALL_Incoming:
		s = "RINGING";
		timersub(&now, &call->start_time, &delta);
		break;
	case GN_CALL_Dialing:
		s = "DIALING";
		timersub(&now, &call->start_time, &delta);
		break;
	case GN_CALL_Established:
		s = "ESTABLISHED";
		timersub(&now, &call->answer_time, &delta);
		break;
	case GN_CALL_Held:
		s = "ON HOLD";
		timersub(&now, &call->answer_time, &delta);
		break;
	default:
		s = "UNKNOWN STATE";
		memset(&delta, 0, sizeof(delta));
		break;
	}

	fprintf(stderr, _("CALL%d: %s %s(%s) (callid: %d, duration: %d sec)\n"),
		call_id, s, call->remote_number, call->remote_name,
		call->call_id, (int)delta.tv_sec);
#endif
}

/* In monitor mode we don't do much, we just initialise the fbus code.
   Note that the fbus code no longer has an internal monitor mode switch,
   instead compile with DEBUG enabled to get all the gumpf. */
static int monitormode(int argc, char *argv[])
{
	float rflevel = -1, batterylevel = -1;
	gn_power_source powersource = -1;
	gn_rf_unit rfunit = GN_RF_Arbitrary;
	gn_battery_unit battunit = GN_BU_Arbitrary;
	gn_data data;
	gn_error error;
	int i, d;

	gn_network_info networkinfo;
	gn_cb_message cbmessage;
	gn_memory_status simmemorystatus   = {GN_MT_SM, 0, 0};
	gn_memory_status phonememorystatus = {GN_MT_ME, 0, 0};
	gn_memory_status dc_memorystatus   = {GN_MT_DC, 0, 0};
	gn_memory_status en_memorystatus   = {GN_MT_EN, 0, 0};
	gn_memory_status fd_memorystatus   = {GN_MT_FD, 0, 0};
	gn_memory_status ld_memorystatus   = {GN_MT_LD, 0, 0};
	gn_memory_status mc_memorystatus   = {GN_MT_MC, 0, 0};
	gn_memory_status on_memorystatus   = {GN_MT_ON, 0, 0};
	gn_memory_status rc_memorystatus   = {GN_MT_RC, 0, 0};

	gn_sms_status smsstatus = {0, 0, 0, 0};
	/*
	char Number[20];
	*/
	gn_data_clear(&data);

	/* We do not want to monitor serial line forever - press Ctrl+C to stop the
	   monitoring mode. */
	signal(SIGINT, interrupted);

	fprintf(stderr, _("Entering monitor mode...\n"));

	/* Loop here indefinitely - allows you to see messages from GSM code in
	   response to unknown messages etc. The loops ends after pressing the
	   Ctrl+C. */
	data.rf_unit = &rfunit;
	data.rf_level = &rflevel;
	data.battery_unit = &battunit;
	data.battery_level = &batterylevel;
	data.power_source = &powersource;
	data.sms_status = &smsstatus;
	data.network_info = &networkinfo;
	data.on_cell_broadcast = storecbmessage;
	data.call_notification = callnotifier;

	gn_sm_functions(GN_OP_SetCallNotification, &data, &state);

	memset(cb_queue, 0, sizeof(cb_queue));
	cb_ridx = 0;
	cb_widx = 0;
	gn_sm_functions(GN_OP_SetCellBroadcast, &data, &state);

	if (argc)
		d = !strcasecmp(argv[0], "once") ? -1 : atoi(argv[0]);
	else
		d = 1;

	while (!bshutdown) {
		if (gn_sm_functions(GN_OP_GetRFLevel, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("RFLevel: %d\n"), (int)rflevel);

		if (gn_sm_functions(GN_OP_GetBatteryLevel, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("Battery: %d\n"), (int)batterylevel);

		if (gn_sm_functions(GN_OP_GetPowersource, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("Power Source: %s\n"), (powersource == GN_PS_ACDC) ? _("AC/DC") : _("battery"));

		data.memory_status = &simmemorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("SIM: Used %d, Free %d\n"), simmemorystatus.used, simmemorystatus.free);

		data.memory_status = &phonememorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("Phone: Used %d, Free %d\n"), phonememorystatus.used, phonememorystatus.free);

		data.memory_status = &dc_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("DC: Used %d, Free %d\n"), dc_memorystatus.used, dc_memorystatus.free);

		data.memory_status = &en_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("EN: Used %d, Free %d\n"), en_memorystatus.used, en_memorystatus.free);

		data.memory_status = &fd_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("FD: Used %d, Free %d\n"), fd_memorystatus.used, fd_memorystatus.free);

		data.memory_status = &ld_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("LD: Used %d, Free %d\n"), ld_memorystatus.used, ld_memorystatus.free);

		data.memory_status = &mc_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("MC: Used %d, Free %d\n"), mc_memorystatus.used, mc_memorystatus.free);

		data.memory_status = &on_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("ON: Used %d, Free %d\n"), on_memorystatus.used, on_memorystatus.free);

		data.memory_status = &rc_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("RC: Used %d, Free %d\n"), rc_memorystatus.used, rc_memorystatus.free);

		if (gn_sm_functions(GN_OP_GetSMSStatus, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("SMS Messages: Unread %d, Number %d\n"), smsstatus.unread, smsstatus.number);

		if (gn_sm_functions(GN_OP_GetNetworkInfo, &data, &state) == GN_ERR_NONE)
			fprintf(stdout, _("Network: %s (%s), LAC: %02x%02x, CellID: %02x%02x\n"), gn_network_name_get(networkinfo.network_code), gn_country_name_get(networkinfo.network_code), networkinfo.LAC[0], networkinfo.LAC[1], networkinfo.cell_id[0], networkinfo.cell_id[1]);

		gn_call_check_active(&state);
		for (i = 0; i < GN_CALL_MAX_PARALLEL; i++)
			displaycall(i);

		if (readcbmessage(&cbmessage) == GN_ERR_NONE)
			fprintf(stdout, _("Cell broadcast received on channel %d: %s\n"), cbmessage.channel, cbmessage.message);

		if (d < 0) break;
		sleep(d);
	}

	data.on_cell_broadcast = NULL;
	error = gn_sm_functions(GN_OP_SetCellBroadcast, &data, &state);

	fprintf(stderr, _("Leaving monitor mode...\n"));

	return error;
}

static void printdisplaystatus(int status)
{
	fprintf(stdout, _("Call in progress: %-3s\n"),
		status & (1 << GN_DISP_Call_In_Progress) ? _("on") : _("off"));
	fprintf(stdout, _("Unknown: %-3s\n"),
		status & (1 << GN_DISP_Unknown)          ? _("on") : _("off"));
	fprintf(stdout, _("Unread SMS: %-3s\n"),
		status & (1 << GN_DISP_Unread_SMS)       ? _("on") : _("off"));
	fprintf(stdout, _("Voice call: %-3s\n"),
		status & (1 << GN_DISP_Voice_Call)       ? _("on") : _("off"));
	fprintf(stdout, _("Fax call active: %-3s\n"),
		status & (1 << GN_DISP_Fax_Call)         ? _("on") : _("off"));
	fprintf(stdout, _("Data call active: %-3s\n"),
		status & (1 << GN_DISP_Data_Call)        ? _("on") : _("off"));
	fprintf(stdout, _("Keyboard lock: %-3s\n"),
		status & (1 << GN_DISP_Keyboard_Lock)    ? _("on") : _("off"));
	fprintf(stdout, _("SMS storage full: %-3s\n"),
		status & (1 << GN_DISP_SMS_Storage_Full) ? _("on") : _("off"));
}

#ifdef WIN32
#  define ESC "\033"
#else
#  define ESC "\e"
#endif

static void newoutputfn(gn_display_draw_msg *drawmessage)
{
	int x, y, n;
	static int status;
	static unsigned char screen[GN_DRAW_SCREEN_MAX_HEIGHT][GN_DRAW_SCREEN_MAX_WIDTH];
	static bool init = false;

	if (!init) {
		for (y = 0; y < GN_DRAW_SCREEN_MAX_HEIGHT; y++)
			for (x = 0; x < GN_DRAW_SCREEN_MAX_WIDTH; x++)
				screen[y][x] = ' ';
		status = 0;
		init = true;
	}

	fprintf(stdout, ESC "[1;1H");

	switch (drawmessage->cmd) {
	case GN_DISP_DRAW_Clear:
		for (y = 0; y < GN_DRAW_SCREEN_MAX_HEIGHT; y++)
			for (x = 0; x < GN_DRAW_SCREEN_MAX_WIDTH; x++)
				screen[y][x] = ' ';
		break;

	case GN_DISP_DRAW_Text:
		x = drawmessage->data.text.x*GN_DRAW_SCREEN_MAX_WIDTH / 84;
		y = drawmessage->data.text.y*GN_DRAW_SCREEN_MAX_HEIGHT / 48;
		n = strlen(drawmessage->data.text.text);
		if (n > GN_DRAW_SCREEN_MAX_WIDTH)
			return;
		if (x + n > GN_DRAW_SCREEN_MAX_WIDTH)
			x = GN_DRAW_SCREEN_MAX_WIDTH - n;
		if (y > GN_DRAW_SCREEN_MAX_HEIGHT)
			y = GN_DRAW_SCREEN_MAX_HEIGHT - 1;
		memcpy(&screen[y][x], drawmessage->data.text.text, n);
		break;

	case GN_DISP_DRAW_Status:
		status = drawmessage->data.status;
		break;

	default:
		return;
	}

	for (y = 0; y < GN_DRAW_SCREEN_MAX_HEIGHT; y++) {
		for (x = 0; x < GN_DRAW_SCREEN_MAX_WIDTH; x++)
			fprintf(stdout, "%c", screen[y][x]);
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
	printdisplaystatus(status);
}

static void console_raw(void)
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

static int displayoutput(void)
{
	gn_data data;
	gn_error error;
	gn_display_output output;

	gn_data_clear(&data);
	memset(&output, 0, sizeof(output));
	output.output_fn = newoutputfn;
	data.display_output = &output;

	error = gn_sm_functions(GN_OP_DisplayOutput, &data, &state);
	console_raw();
#ifndef WIN32
	fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
#endif

	switch (error) {
	case GN_ERR_NONE:
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);

		fprintf (stderr, _("Entered display monitoring mode...\n"));
		fprintf (stderr, ESC "c" );

		/* Loop here indefinitely - allows you to read texts from phone's
		   display. The loops ends after pressing the Ctrl+C. */
		while (!bshutdown) {
			char buf[105];
			memset(&buf[0], 0, 102);
/*			while (read(0, buf, 100) > 0) {
				fprintf(stderr, _("handling keys (%d).\n"), strlen(buf));
				if (GSM && GSM->HandleString && GSM->HandleString(buf) != GN_ERR_NONE)
					fprintf(stdout, _("Key press simulation failed.\n"));
				memset(buf, 0, 102);
			}*/
			gn_sm_loop(1, &state);
			gn_sm_functions(GN_OP_PollDisplay, &data, &state);
		}
		fprintf (stderr, _("Shutting down\n"));

		fprintf (stderr, _("Leaving display monitor mode...\n"));

		output.output_fn = NULL;
		error = gn_sm_functions(GN_OP_DisplayOutput, &data, &state);
		if (error != GN_ERR_NONE)
			fprintf (stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	default:
		fprintf (stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Reads profile from phone and displays its' settings */
static int getprofile(int argc, char *argv[])
{
	int max_profiles;
	int start, stop, i;
	gn_profile p;
	gn_error error = GN_ERR_NOTSUPPORTED;
	gn_data data;

	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[64];
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	gn_data_clear(&data);
	data.model = model;
	while (gn_sm_functions(GN_OP_GetModel, &data, &state) != GN_ERR_NONE)
		sleep(1);

	p.number = 0;
	gn_data_clear(&data);
	data.profile = &p;
	error = gn_sm_functions(GN_OP_GetProfile, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		return error;
	}

	max_profiles = 7; /* This is correct for 6110 (at least my). How do we get
			     the number of profiles? */

	/* For N5110 */
	/* FIXME: It should be set to 3 for N5130 and 3210 too */
	if (!strcmp(model, "NSE-1"))
		max_profiles = 3;

	if (argc > optind) {
		start = atoi(argv[optind]);
		stop = (argc > optind + 1) ? atoi(argv[optind + 1]) : start;

		if (start > stop) {
			fprintf(stderr, _("Starting profile number is greater than stop\n"));
			return -1;
		}

		if (start < 0) {
			fprintf(stderr, _("Profile number must be value from 0 to %d!\n"), max_profiles - 1);
			return -1;
		}

		if (stop >= max_profiles) {
			fprintf(stderr, _("This phone supports only %d profiles!\n"), max_profiles);
			return -1;
		}
	} else {
		start = 0;
		stop = max_profiles - 1;
	}

	gn_data_clear(&data);
	data.profile = &p;

	for (i = start; i <= stop; i++) {
		p.number = i;

		if (p.number != 0) {
			error = gn_sm_functions(GN_OP_GetProfile, &data, &state);
			if (error != GN_ERR_NONE) {
				fprintf(stderr, _("Cannot get profile %d\n"), i);
				return error;
			}
		}

		if (raw) {
			fprintf(stdout, "%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
				p.number, p.name, p.default_name, p.keypad_tone,
				p.lights, p.call_alert, p.ringtone, p.volume,
				p.message_tone, p.vibration, p.warning_tone,
				p.caller_groups, p.automatic_answer);
		} else {
			fprintf(stdout, "%d. \"%s\"\n", p.number, p.name);
			if (p.default_name == -1) fprintf(stdout, _(" (name defined)\n"));
			fprintf(stdout, _("Incoming call alert: %s\n"), profile_get_call_alert_string(p.call_alert));
			fprintf(stdout, _("Ringing tone: %s (%d)\n"), get_ringtone_name(p.ringtone), p.ringtone);
			fprintf(stdout, _("Ringing volume: %s\n"), profile_get_volume_string(p.volume));
			fprintf(stdout, _("Message alert tone: %s\n"), profile_get_message_tone_string(p.message_tone));
			fprintf(stdout, _("Keypad tones: %s\n"), profile_get_keypad_tone_string(p.keypad_tone));
			fprintf(stdout, _("Warning and game tones: %s\n"), profile_get_warning_tone_string(p.warning_tone));

			/* FIXME: Light settings is only used for Car */
			if (p.number == (max_profiles - 2)) fprintf(stdout, _("Lights: %s\n"), p.lights ? _("On") : _("Automatic"));
			fprintf(stdout, _("Vibration: %s\n"), profile_get_vibration_string(p.vibration));

			/* FIXME: it will be nice to add here reading caller group name. */
			if (max_profiles != 3) fprintf(stdout, _("Caller groups: 0x%02x\n"), p.caller_groups);

			/* FIXME: Automatic answer is only used for Car and Headset. */
			if (p.number >= (max_profiles - 2)) fprintf(stdout, _("Automatic answer: %s\n"), p.automatic_answer ? _("On") : _("Off"));
			fprintf(stdout, "\n");
		}
	}

	return error;
}

/* Writes profiles to phone */
static int setprofile()
{
	int n;
	gn_profile p;
	gn_error error = GN_ERR_NONE;
	gn_data data;
	char line[256], ch;

	gn_data_clear(&data);
	data.profile = &p;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}

		n = sscanf(line, "%d;%39[^;];%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d%c",
			    &p.number, p.name, &p.default_name, &p.keypad_tone,
			    &p.lights, &p.call_alert, &p.ringtone, &p.volume,
			    &p.message_tone, &p.vibration, &p.warning_tone,
			    &p.caller_groups, &p.automatic_answer, &ch);
		if (n != 13) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			return GN_ERR_UNKNOWN;
		}

		error = gn_sm_functions(GN_OP_SetProfile, &data, &state);
		if (error != GN_ERR_NONE) {
			fprintf(stderr, _("Cannot set profile: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return error;
}

/* Queries the active profile */
static int getactiveprofile()
{
	gn_profile p;
	gn_error error;
	gn_data data;

	gn_data_clear(&data);
	data.profile = &p;

	error = gn_sm_functions(GN_OP_GetActiveProfile, &data, &state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Cannot get active profile: %s\n"), gn_error_print(error));
		return error;
	}

	error = gn_sm_functions(GN_OP_GetProfile, &data, &state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Cannot get profile %d\n"), p.number);
	else
		fprintf(stdout, _("Active profile: %d (%s)\n"), p.number, p.name);

	return error;
}

/* Select the specified profile */
static int setactiveprofile(int argc, char *argv[])
{
	gn_profile p;
	gn_error error;
	gn_data data;

	gn_data_clear(&data);
	data.profile = &p;
	p.number = atoi(argv[0]);

	error = gn_sm_functions(GN_OP_SetActiveProfile, &data, &state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Cannot set active profile to %d: %s\n"), p.number, gn_error_print(error));
	return error;
}

static char *escape_semicolon(char *src)
{
	int i, j, len = strlen(src);
	char *dest;

	dest = calloc(2 * len, sizeof(char));
	for (i = 0, j = 0; i < len; i++, j++) {
		if (src[i] == ';')
			dest[j++] = '\\';
		dest[j] = src[i];
	}
	dest[j] = 0;
	return dest;
}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */
static int getphonebook(int argc, char *argv[])
{
	gn_phonebook_entry entry;
	gn_memory_status memstat;
	int count, start_entry, end_entry, num_entries = INT_MAX;
	gn_error error = GN_ERR_NONE;
	char *memory_type_string;
	char location[32];
	int type = 0; /* Output type:
				0 - not formatted
				1 - CSV
				2 - vCard
				3 - LDIF
			*/

	if (argc < 2) {
		usage(stderr, -1);
	}

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	entry.memory_type = gn_str2memory_type(memory_type_string);
	if (entry.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return -1;
	}

	start_entry = end_entry = atoi(argv[1]);
	switch (argc) {
	case 2: /* Nothing to do */
		break;
	case 4:
		if (!strcmp(argv[3], "-r") || !strcmp(argv[3], "--raw")) type = 1;
		else if (!strcmp(argv[3], "-v") || !strcmp(argv[3], "--vcard")) type = 2;
		else if (!strcmp(argv[3], "-l") || !strcmp(argv[3], "--ldif")) type = 3;
		else usage(stderr, -1);
	case 3:
		if (!strcmp(argv[2], "end")) end_entry = INT_MAX;
		else if (!strcmp(argv[2], "-r") || !strcmp(argv[2], "--raw")) type = 1;
		else if (!strcmp(argv[2], "-v") || !strcmp(argv[2], "--vcard")) type = 2;
		else if (!strcmp(argv[2], "-l") || !strcmp(argv[2], "--ldif")) type = 3;
		else end_entry = atoi(argv[2]);
		break;
	default:
		usage(stderr, -1);
		break;
	}

	if (end_entry == INT_MAX) {
		data.memory_status = &memstat;
		memstat.memory_type = entry.memory_type;
		if ((error = gn_sm_functions(GN_OP_GetMemoryStatus, &data, &state)) == GN_ERR_NONE) {
			num_entries = memstat.used;
		}
	}

	/* Now retrieve the requested entries. */
	count = start_entry;
	while (num_entries > 0 && count <= end_entry) {
		entry.location = count;

		data.phonebook_entry = &entry;
		error = gn_sm_functions(GN_OP_ReadPhonebook, &data, &state);

		switch (error) {
			int i;
		case GN_ERR_NONE:
			num_entries--;
			switch (type) {
			case 1:
				do {
					char *escaped_name = escape_semicolon(entry.name);
					fprintf(stdout, "%s;%s;%s;%d;%d", escaped_name,
						entry.number, memory_type_string,
						entry.location, entry.caller_group);
					free(escaped_name);
				} while (0);
				for (i = 0; i < entry.subentries_count; i++) {
					char *escaped_number = NULL;
					switch (entry.subentries[i].entry_type) {
					case GN_PHONEBOOK_ENTRY_Date:
						break;
					default:
						escaped_number = escape_semicolon(entry.subentries[i].data.number);
						fprintf(stdout, ";%d;%d;%d;%s",
							entry.subentries[i].entry_type,
							entry.subentries[i].number_type,
							entry.subentries[i].id,
							escaped_number);
						free(escaped_number);
						break;
					}
				}
				if (entry.memory_type == GN_MT_MC || entry.memory_type == GN_MT_DC || entry.memory_type == GN_MT_RC)
					fprintf(stdout, "%d;0;0;%02u.%02u.%04u %02u:%02u:%02u", GN_PHONEBOOK_ENTRY_Date,
						entry.date.day, entry.date.month, entry.date.year, entry.date.hour, entry.date.minute, entry.date.second);
				fprintf(stdout, "\n");
				break;
			case 2:
				sprintf(location, "%s%d", memory_type_string, entry.location);
				gn_phonebook2vcard(stdout, &entry, location);
				break;
			case 3:
				gn_phonebook2ldif(stdout, &entry);
				break;
			default:
				fprintf(stdout, _("%d. Name: %s\nNumber: %s\nGroup id: %d\n"), entry.location, entry.name, entry.number, entry.caller_group);
				for (i = 0; i < entry.subentries_count; i++) {
					switch (entry.subentries[i].entry_type) {
					case GN_PHONEBOOK_ENTRY_Email:
						fprintf(stdout, _("Email address: "));
						break;
					case GN_PHONEBOOK_ENTRY_Postal:
						fprintf(stdout, _("Address: "));
						break;
					case GN_PHONEBOOK_ENTRY_Note:
						fprintf(stdout, _("Notes: "));
						break;
					case GN_PHONEBOOK_ENTRY_Number:
						switch (entry.subentries[i].number_type) {
						case GN_PHONEBOOK_NUMBER_Home:
							fprintf(stdout, _("Home number: "));
							break;
						case GN_PHONEBOOK_NUMBER_Mobile:
							fprintf(stdout, _("Cellular number: "));
							break;
						case GN_PHONEBOOK_NUMBER_Fax:
							fprintf(stdout, _("Fax number: "));
							break;
						case GN_PHONEBOOK_NUMBER_Work:
							fprintf(stdout, _("Business number: "));
							break;
						case GN_PHONEBOOK_NUMBER_General:
							fprintf(stdout, _("Preferred number: "));
							break;
						default:
							fprintf(stdout, _("Unknown (%d): "), entry.subentries[i].number_type);
							break;
						}
						break;
					case GN_PHONEBOOK_ENTRY_URL:
						fprintf(stdout, _("WWW address: "));
						break;
					case GN_PHONEBOOK_ENTRY_Date:
						break;
					default:
						fprintf(stdout, _("Unknown (%d): "), entry.subentries[i].entry_type);
						break;
					}
					if (entry.subentries[i].entry_type != GN_PHONEBOOK_ENTRY_Date)
						fprintf(stdout, "%s\n", entry.subentries[i].data.number);
				}
				if (entry.memory_type == GN_MT_MC || entry.memory_type == GN_MT_DC || entry.memory_type == GN_MT_RC)
					fprintf(stdout, _("Date: %02u.%02u.%04u %02u:%02u:%02u\n"), entry.date.day, entry.date.month, entry.date.year, entry.date.hour, entry.date.minute, entry.date.second);
				break;
			}
			break;
		case GN_ERR_EMPTYLOCATION:
			fprintf(stderr, _("Empty memory location. Skipping.\n"));
			break;
		case GN_ERR_INVALIDLOCATION:
			fprintf(stderr, _("Error reading from the location %d in memory %s\n"), count, memory_type_string);
			if (end_entry == INT_MAX) {
				/* Ensure that we quit the loop */
				num_entries = 0;
			}
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			break;
		}
		count++;
	}
	return error;
}


/* Read data from stdin, parse and write to phone.  The parsing is relatively
   crude and doesn't allow for much variation from the stipulated format. */
/* FIXME: I guess there's *very* similar code in xgnokii */
static int writephonebook(int argc, char *args[])
{
	gn_phonebook_entry entry;
	gn_error error = GN_ERR_NONE;
	gn_memory_type default_mt = GN_MT_ME; /* Default memory_type. Changed if given in the command line */
	int default_location = 1; /* default location. Changed if given in the command line */
	int find_free = 0; /* By default don't try to find a free location */
	int confirm = 0; /* By default don't overwrite existing entries */
	int type = 0; /* type of the output:
				0 - CSV (default)
				1 - vCard
				2 - LDIF
			*/
	char *line, oline[MAX_INPUT_LINE_LEN];
	int i;

	struct option options[] = {
		{ "overwrite",		0,			NULL, 'o'},
		{ "vcard",		0,			NULL, 'v'},
		{ "ldif",		0,			NULL, 'l'},
		{ "find-free",		0,			NULL, 'f'},
		{ "memory-type",	required_argument,	NULL, 'm'},
		{ "memory",		required_argument,	NULL, 'm'},
		{ "memory-location",	required_argument,	NULL, 'n'},
		{ "location",		required_argument,	NULL, 'n'},
		{ NULL,			0,			NULL, 0}
	};

	/* Option parsing */
	while ((i = getopt_long(argc, args, "ovlfm:n:", options, NULL)) != -1) {
		switch (i) {
		case 'o':
			confirm = 1;
			break;
		case 'v':
			type = 1;
			break;
		case 'l':
			if (type) usage(stderr, -1);
			type = 2;
			break;
		case 'f':
			find_free = 1;
			break;
		case 'm':
			default_mt = gn_str2memory_type(optarg);
			break;
		case 'n':
			default_location = atoi(optarg);
			break;
		default:
			usage(stderr, -1);
			break;
		}
	}

	line = oline;

	/* Go through data from stdin. */
	while (1) {
		error = GN_ERR_NONE;

		memset(&entry, 0, sizeof(gn_phonebook_entry));
		entry.memory_type = default_mt;
		entry.location = default_location;
		switch (type) {
		case 1:
			if (gn_vcard2phonebook(stdin, &entry))
				error = GN_ERR_WRONGDATAFORMAT;
			break;
		case 2:
			if (gn_ldif2phonebook(stdin, &entry))
				error = GN_ERR_WRONGDATAFORMAT;
			break;
		default:
			if (!gn_line_get(stdin, line, MAX_INPUT_LINE_LEN))
				return 0; /* it means we read an empty line, but that's not an error */
			else
				error = gn_file_phonebook_raw_parse(&entry, oline);
			break;
		}

		if (error == GN_ERR_WRONGDATAFORMAT) {
			fprintf(stderr, "%s\n", gn_error_print(error));
			break;
		}

		error = GN_ERR_NONE;

		if (find_free) {
#if 0
			error = gn_sm_functions(GN_OP_FindFreePhonebookEntry, &data, &state);
			if (error == GN_ERR_NOTIMPLEMENTED) {
#endif
			for (i = 1; ; i++) {
				gn_phonebook_entry aux;

				memcpy(&aux, &entry, sizeof(gn_phonebook_entry));
				data.phonebook_entry = &aux;
				data.phonebook_entry->location = i;
				error = gn_sm_functions(GN_OP_ReadPhonebook, &data, &state);
				if (error != GN_ERR_NONE && error != GN_ERR_EMPTYLOCATION) {
					break;
				}
				if (aux.empty || error == GN_ERR_EMPTYLOCATION) {
					entry.location = aux.location;
					error = GN_ERR_NONE;
					break;
				}
			}
#if 0
			}
#endif
			if (error != GN_ERR_NONE) {
				fprintf(stderr, "%s\n", gn_error_print(error));
				break;
			}
		}

		if (!confirm) {
			gn_phonebook_entry aux;

			memcpy(&aux, &entry, sizeof(gn_phonebook_entry));
			data.phonebook_entry = &aux;
			error = gn_sm_functions(GN_OP_ReadPhonebook, &data, &state);

			if (error == GN_ERR_NONE || error == GN_ERR_EMPTYLOCATION) {
				if (!aux.empty && error != GN_ERR_EMPTYLOCATION) {
					char ans[8];

					fprintf(stdout, _("Location busy. "));
					confirm = -1;
					while (confirm < 0) {
						fprintf(stdout, _("Overwrite? (yes/no) "));
						gn_line_get(stdin, ans, 7);
						if (!strcmp(ans, _("yes"))) confirm = 1;
						else if (!strcmp(ans, _("no"))) confirm = 0;
						else {
							fprintf(stdout, "\nIncorrect answer [%s]. Assuming 'no'.\n", ans);
							confirm = 0;
						}
					}
					/* User chose not to overwrite */
					if (!confirm) continue;
					confirm = 0;
				}
			} else {
				fprintf(stderr, _("Error (%s)\n"), gn_error_print(error));
				return -1;
			}
		}

		/* Do write and report success/failure. */
		gn_phonebook_entry_sanitize(&entry);
		data.phonebook_entry = &entry;
		error = gn_sm_functions(GN_OP_WritePhonebook, &data, &state);

		if (error == GN_ERR_NONE)
			fprintf (stderr, 
				 _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), 
				 gn_memory_type2str(entry.memory_type), entry.location, entry.name, entry.number);
		else
			fprintf (stderr, _("Write FAILED (%s): memory type: %s, loc: %d, name: %s, number: %s\n"), 
				 gn_error_print(error), gn_memory_type2str(entry.memory_type), entry.location, entry.name, entry.number);
	}
	return error;
}

/* Delete phonebook entry */
static int deletephonebook(int argc, char *argv[])
{
	gn_phonebook_entry entry;
	gn_error error;
	char *memory_type_string;
	int i, first_location, last_location;

	if (argc < 2) {
		usage(stderr, -1);
	}

	/* Handle command line args that set memory type and location. */
	memory_type_string = argv[0];
	entry.memory_type = gn_str2memory_type(memory_type_string);
	if (entry.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return -1;
	}

	first_location = last_location = atoi(argv[1]);
	if ((argc > 2) && (argv[2][0] != '-')) {
		if (!strcasecmp(argv[2], "end"))
			last_location = INT_MAX;
		else
			last_location = atoi(argv[2]);
	}
	if (last_location < first_location)
		last_location = first_location;

	for (i = first_location; i <= last_location; i++) {
		entry.location = i;
		entry.empty = true;
		data.phonebook_entry = &entry;
		error = gn_sm_functions(GN_OP_DeletePhonebook, &data, &state);
		switch (error) {
		case GN_ERR_NONE:
			fprintf (stderr, _("Phonebook entry removed: memory type: %s, loc: %d\n"), 
				 gn_memory_type2str(entry.memory_type), entry.location);
			break;
		default:
			if (last_location == INT_MAX)
				last_location = 0;
			else
				fprintf (stderr, _("Phonebook entry removal FAILED (%s): memory type: %s, loc: %d\n"), 
					 gn_error_print(error), gn_memory_type2str(entry.memory_type), entry.location);
			break;
		}
	}
	return GN_ERR_NONE;
}

/* Getting WAP bookmarks. */
static int getwapbookmark(char *number)
{
	gn_wap_bookmark	wapbookmark;
	gn_data	data;
	gn_error	error;

	wapbookmark.location = atoi(number);

	gn_data_clear(&data);
	data.wap_bookmark = &wapbookmark;

	error = gn_sm_functions(GN_OP_GetWAPBookmark, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stdout, _("WAP bookmark nr. %d:\n"), wapbookmark.location);
		fprintf(stdout, _("Name: %s\n"), wapbookmark.name);
		fprintf(stdout, _("URL: %s\n"), wapbookmark.URL);

		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Writing WAP bookmarks. */
static int writewapbookmark(int nargc, char *nargv[])
{
	gn_wap_bookmark	wapbookmark;
	gn_data		data;
	gn_error	error;


	gn_data_clear(&data);
	data.wap_bookmark = &wapbookmark;

	if (nargc != 2) usage(stderr, -1);

	snprintf(&wapbookmark.name[0], WAP_NAME_MAX_LENGTH, nargv[0]);
	snprintf(&wapbookmark.URL[0], WAP_URL_MAX_LENGTH, nargv[1]);

	error = gn_sm_functions(GN_OP_WriteWAPBookmark, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP bookmark nr. %d succesfully written!\n"), wapbookmark.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Deleting WAP bookmarks. */
static int deletewapbookmark(char *number)
{
	gn_wap_bookmark	wapbookmark;
	gn_data		data;
	gn_error	error;

	wapbookmark.location = atoi(number);

	gn_data_clear(&data);
	data.wap_bookmark = &wapbookmark;

	error = gn_sm_functions(GN_OP_DeleteWAPBookmark, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP bookmark nr. %d deleted!\n"), wapbookmark.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Getting WAP settings. */
static int getwapsetting(int argc, char *argv[])
{
	gn_wap_setting	wapsetting;
	gn_data		data;
	gn_error	error;
	bool		raw = false;

	wapsetting.location = atoi(argv[0]);
	switch (argc) {
	case 1:
		break;
	case 2: 
		if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--raw")) 
			raw = true;
		else 
			usage(stderr, -1);
		break;
	}

	gn_data_clear(&data);
	data.wap_setting = &wapsetting;

	error = gn_sm_functions(GN_OP_GetWAPSetting, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		if (raw) {
			fprintf(stdout, ("%i;%s;%s;%i;%i;%i;%i;%i;%i;%i;%s;%s;%s;%s;%i;%i;%i;%s;%s;%s;%s;%s;%s;\n"),
				wapsetting.location, wapsetting.name, wapsetting.home, wapsetting.session,
				wapsetting.security, wapsetting.bearer, wapsetting.gsm_data_authentication,
				wapsetting.call_type, wapsetting.call_speed, wapsetting.gsm_data_login, wapsetting.gsm_data_ip,
				wapsetting.number, wapsetting.gsm_data_username, wapsetting.gsm_data_password,
				wapsetting.gprs_connection, wapsetting.gprs_authentication, wapsetting.gprs_login,
				wapsetting.access_point_name, wapsetting.gprs_ip,  wapsetting.gprs_username, wapsetting.gprs_password,
				wapsetting.sms_service_number, wapsetting.sms_server_number);
		} else {
			fprintf(stdout, _("WAP bookmark nr. %d:\n"), wapsetting.location);
			fprintf(stdout, _("Name: %s\n"), wapsetting.name);
			fprintf(stdout, _("Home: %s\n"), wapsetting.home);
			fprintf(stdout, _("Session mode: "));
			switch (wapsetting.session) {
			case GN_WAP_SESSION_TEMPORARY: 
				fprintf(stdout, _("temporary\n"));
				break;
			case GN_WAP_SESSION_PERMANENT: 
				fprintf(stdout, _("permanent\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("Connection security: "));
			if (wapsetting.security)
				fprintf(stdout, _("yes\n"));
			else
				fprintf(stdout, _("no\n"));
			fprintf(stdout, _("Data bearer: "));
			switch (wapsetting.bearer) {
			case GN_WAP_BEARER_GSMDATA:
				fprintf(stdout, _("GSM data\n"));
				break;
			case GN_WAP_BEARER_GPRS:
				fprintf(stdout, _("GPRS\n"));
				break;
			case GN_WAP_BEARER_SMS:
				fprintf(stdout, _("SMS\n"));
				break;
			case GN_WAP_BEARER_USSD:
				fprintf(stdout, _("USSD\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("GSM data\n"));
			fprintf(stdout, _("   Authentication type: "));
			switch (wapsetting.gsm_data_authentication) {
			case GN_WAP_AUTH_NORMAL:
				fprintf(stdout, _("normal\n"));
				break;
			case GN_WAP_AUTH_SECURE:
				fprintf(stdout, _("secure\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Data call type: "));
			switch (wapsetting.call_type) {
			case GN_WAP_CALL_ANALOGUE:
				fprintf(stdout, _("analogue\n"));
				break;
			case GN_WAP_CALL_ISDN:
				fprintf(stdout, _("ISDN\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Data call speed: "));
			switch (wapsetting.call_speed) {
			case GN_WAP_CALL_AUTOMATIC:
				fprintf(stdout, _("automatic\n"));
				break;
			case GN_WAP_CALL_9600:
				fprintf(stdout, _("9600\n"));
				break;
			case GN_WAP_CALL_14400:
				fprintf(stdout, _("14400\n"));
				break;	
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Login type: "));
			switch (wapsetting.gsm_data_login) {
			case GN_WAP_LOGIN_MANUAL:
				fprintf(stdout, _("manual\n"));
				break;
			case GN_WAP_LOGIN_AUTOLOG:
				fprintf(stdout, _("automatic\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   IP: %s\n"), wapsetting.gsm_data_ip);
			fprintf(stdout, _("   Number: %s\n"), wapsetting.number);
			fprintf(stdout, _("   Username: %s\n"), wapsetting.gsm_data_username);
			fprintf(stdout, _("   Password: %s\n"), wapsetting.gsm_data_password);
			fprintf(stdout, _("GPRS\n"));
			fprintf(stdout, _("   connection: "));
			switch (wapsetting.gprs_connection) {
			case GN_WAP_GPRS_WHENNEEDED: 
				fprintf(stdout, _("when needed\n"));
				break;
			case GN_WAP_GPRS_ALWAYS: 
				fprintf(stdout, _("always\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Authentication type: "));
			switch (wapsetting.gprs_authentication) {
			case GN_WAP_AUTH_NORMAL:
				fprintf(stdout, _("normal\n"));
				break;
			case GN_WAP_AUTH_SECURE:
				fprintf(stdout, _("secure\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Login type: "));
			switch (wapsetting.gprs_login) {
			case GN_WAP_LOGIN_MANUAL: 
				fprintf(stdout, _("manual\n"));
				break;
			case GN_WAP_LOGIN_AUTOLOG:
				fprintf(stdout, _("automatic\n"));
				break;
			default: 
				fprintf(stdout, _("unknown\n"));
				break;
			}
			fprintf(stdout, _("   Access point: %s\n"), wapsetting.access_point_name);
			fprintf(stdout, _("   IP: %s\n"), wapsetting.gprs_ip);
			fprintf(stdout, _("   Username: %s\n"), wapsetting.gprs_username);
			fprintf(stdout, _("   Password: %s\n"), wapsetting.gprs_password);
			fprintf(stdout, _("SMS\n"));
			fprintf(stdout, _("   Service number: %s\n"), wapsetting.sms_service_number);
			fprintf(stdout, _("   Server number: %s\n"), wapsetting.sms_server_number);
		}
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Writes WAP settings to phone */
static int writewapsetting()
{
	int n;
	gn_wap_setting wapsetting;
	gn_error error = GN_ERR_NONE;
	gn_data data;
	char line[1000];

	gn_data_clear(&data);
	data.wap_setting = &wapsetting;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}
		
		n = sscanf(line, "%d;%50[^;];%256[^;];%d;%d;%d;%d;%d;%d;%d;%50[^;];%50[^;];%32[^;];%20[^;];%d;%d;%d;%100[^;];%20[^;];%32[^;];%20[^;];%20[^;];%20[^;];", 
		/*
		n = sscanf(line, "%d;%s;%s;%d;%d;%d;%d;%d;%d;%d;%s;%s;%s;%s;%d;%d;%d;%s;%s;%s;%s;%s;%s;", 
		*/
			   &wapsetting.location, wapsetting.name, wapsetting.home, (int*)&wapsetting.session, (int*)&wapsetting.security,
			   (int*)&wapsetting.bearer, (int*)&wapsetting.gsm_data_authentication, (int*)&wapsetting.call_type,
			   (int*)&wapsetting.call_speed, (int*)&wapsetting.gsm_data_login, wapsetting.gsm_data_ip,
			   wapsetting.number, wapsetting.gsm_data_username, wapsetting.gsm_data_password, 
			   (int*)&wapsetting.gprs_connection, (int*)&wapsetting.gprs_authentication, (int*)&wapsetting.gprs_login, 
			   wapsetting.access_point_name, wapsetting.gprs_ip,  wapsetting.gprs_username, wapsetting.gprs_password,
			   wapsetting.sms_service_number, wapsetting.sms_server_number);

		if (n != 23) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			dprintf("n: %i\n", n);
			return GN_ERR_UNKNOWN;
		}

		error = gn_sm_functions(GN_OP_WriteWAPSetting, &data, &state);
		if (error != GN_ERR_NONE) 
			fprintf(stderr, _("Cannot write WAP setting: %s\n"), gn_error_print(error));
	}
	return error;
}

/* Deleting WAP bookmarks. */
static int activatewapsetting(char *number)
{
	gn_wap_setting	wapsetting;
	gn_data		data;
	gn_error	error;

	wapsetting.location = atoi(number);

	gn_data_clear(&data);
	data.wap_setting = &wapsetting;

	error = gn_sm_functions(GN_OP_ActivateWAPSetting, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("WAP setting nr. %d activated!\n"), wapsetting.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Getting speed dials. */
static int getspeeddial(char *number)
{
	gn_speed_dial	speeddial;
	gn_data		data;
	gn_error	error;

	speeddial.number = atoi(number);

	gn_data_clear(&data);
	data.speed_dial = &speeddial;

	error = gn_sm_functions(GN_OP_GetSpeedDial, &data, &state);

	switch (error) {
	case GN_ERR_NONE:
		fprintf(stderr, _("SpeedDial nr. %d: %d:%d\n"), speeddial.number, speeddial.memory_type, speeddial.location);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Setting speed dials. */
static int setspeeddial(char *argv[])
{
	gn_speed_dial entry;
	gn_data data;
	gn_error error;
	char *memory_type_string;

	gn_data_clear(&data);
	data.speed_dial = &entry;

	/* Handle command line args that set type, start and end locations. */

	if (strcmp(argv[1], "ME") == 0) {
		entry.memory_type = GN_MT_ME;
		memory_type_string = "ME";
	} else if (strcmp(argv[1], "SM") == 0) {
		entry.memory_type = GN_MT_SM;
		memory_type_string = "SM";
	} else {
		fprintf(stderr, _("Unknown memory type %s!\n"), argv[1]);
		return -1;
	}

	entry.number = atoi(argv[0]);
	entry.location = atoi(argv[2]);

	if ((error = gn_sm_functions(GN_OP_SetSpeedDial, &data, &state)) == GN_ERR_NONE )
		fprintf(stderr, _("Succesfully written!\n"));

	return error;
}

/* Getting the status of the display. */
static int getdisplaystatus(void)
{
	gn_error error = GN_ERR_INTERNALERROR;
	int status;
	gn_data data;

	gn_data_clear(&data);
	data.display_status = &status;

	error = gn_sm_functions(GN_OP_GetDisplayStatus, &data, &state);
	if (error == GN_ERR_NONE) printdisplaystatus(status);

	return error;
}

static int netmonitor(char *m)
{
	unsigned char mode = atoi(m);
	gn_netmonitor nm;
	gn_error error;

	if (!strcmp(m, "reset"))
		mode = 0xf0;
	else if (!strcmp(m, "off"))
		mode = 0xf1;
	else if (!strcmp(m, "field"))
		mode = 0xf2;
	else if (!strcmp(m, "devel"))
		mode = 0xf3;
	else if (!strcmp(m, "next"))
		mode = 0x00;

	nm.field = mode;
	memset(&nm.screen, 0, 50);
	data.netmonitor = &nm;

	error = gn_sm_functions(GN_OP_NetMonitor, &data, &state);

	if (nm.screen) fprintf(stdout, "%s\n", nm.screen);

	return error;
}

static int identify(void)
{
	gn_error error;

	/* Hopefully 64 is enough */
	char imei[64], model[64], rev[64], manufacturer[64];

	data.manufacturer = manufacturer;
	data.model = model;
	data.revision = rev;
	data.imei = imei;

	/* Retrying is bad idea: what if function is simply not implemented?
	   Anyway let's wait 2 seconds for the right packet from the phone. */
	sleep(2);

	strcpy(imei, _("(unknown)"));
	strcpy(manufacturer, _("(unknown)"));
	strcpy(model, _("(unknown)"));
	strcpy(rev, _("(unknown)"));

	error = gn_sm_functions(GN_OP_Identify, &data, &state);

	fprintf(stdout, _("IMEI         : %s\n"), imei);
	fprintf(stdout, _("Manufacturer : %s\n"), manufacturer);
	fprintf(stdout, _("Model        : %s\n"), model);
	fprintf(stdout, _("Revision     : %s\n"), rev);

	return error;
}

static int senddtmf(char *string)
{
	gn_data_clear(&data);
	data.dtmf_string = string;

	return gn_sm_functions(GN_OP_SendDTMF, &data, &state);
}

/* Resets the phone */
static int reset(char *type)
{
	gn_data_clear(&data);
	data.reset_type = 0x03;

	if (type) {
		if (!strcmp(type, "soft"))
			data.reset_type = 0x03;
		else
			if (!strcmp(type, "hard")) {
				data.reset_type = 0x04;
			} else {
				fprintf(stderr, _("What kind of reset do you want??\n"));
				return -1;
			}
	}

	return gn_sm_functions(GN_OP_Reset, &data, &state);
}

/* pmon allows fbus code to run in a passive state - it doesn't worry about
   whether comms are established with the phone.  A debugging/development
   tool. */
static int pmon(void)
{
	gn_error error;

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(&state);

	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model?). Quitting.\n"));
		return error;
	}

	while (1) {
		usleep(50000);
	}

	return 0;
}

static int sendringtone(int argc, char *argv[])
{
	gn_sms sms;
	gn_error error = GN_ERR_NOTSUPPORTED;

	gn_sms_default_submit(&sms);
	sms.user_data[0].type = GN_SMS_DATA_Ringtone;
	sms.user_data[1].type = GN_SMS_DATA_None;

	if ((error = gn_file_ringtone_read(argv[0], &sms.user_data[0].u.ringtone))) {
		fprintf(stderr, _("Failed to load ringtone.\n"));
		return error;
	}

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&sms.remote.number, 0, sizeof(sms.remote.number));
	strncpy(sms.remote.number, argv[1], sizeof(sms.remote.number) - 1);
	if (sms.remote.number[0] == '+')
		sms.remote.type = GN_GSM_NUMBER_International;
	else
		sms.remote.type = GN_GSM_NUMBER_Unknown;

	/* Get the SMS Center */
	if (!sms.smsc.number[0]) {
		data.message_center = calloc(1, sizeof(gn_sms_message_center));
		data.message_center->id = 1;
		if (gn_sm_functions(GN_OP_GetSMSCenter, &data, &state) == GN_ERR_NONE) {
			strcpy(sms.smsc.number, data.message_center->smsc.number);
			sms.smsc.type = data.message_center->smsc.type;
		}
		free(data.message_center);
	}

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	/* Send the message. */
	data.sms = &sms;
	error = gn_sms_send(&data, &state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	return error;
}

static int getringtone(int argc, char *argv[])
{
	gn_ringtone ringtone;
	gn_raw_data rawdata;
	gn_error error;
	unsigned char buff[512];
	int i;

	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	rawdata.data = buff;
	rawdata.length = sizeof(buff);
	gn_data_clear(&data);
	data.ringtone = &ringtone;
	data.raw_data = &rawdata;

	if (argc <= optind) {
		usage(stderr, -1);
		return -1;
	}

	if (argc > optind + 1) {
		ringtone.location = atoi(argv[optind + 1]);
	} else {
		init_ringtone_list();
		ringtone.location = ringtone_list.userdef_location;
	}

	if (raw)
		error = gn_sm_functions(GN_OP_GetRawRingtone, &data, &state);
	else
		error = gn_sm_functions(GN_OP_GetRingtone, &data, &state);
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Getting ringtone %d failed: %s\n"), ringtone.location, gn_error_print(error));
		return error;
	}
	fprintf(stderr, _("Getting ringtone %d (\"%s\") succeeded!\n"), ringtone.location, ringtone.name);

	if (raw) {
		FILE *f;

		if ((f = fopen(argv[optind], "wb")) == NULL) {
			fprintf(stderr, _("Failed to save ringtone.\n"));
			return -1;
		}
		fwrite(rawdata.data, 1, rawdata.length, f);
		fclose(f);
	} else {
		if ((error = gn_file_ringtone_save(argv[optind], &ringtone)) != GN_ERR_NONE) {
			fprintf(stderr, _("Failed to save ringtone: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return error;
}

static int setringtone(int argc, char *argv[])
{
	gn_ringtone ringtone;
	gn_raw_data rawdata;
	gn_error error;
	unsigned char buff[512];
	int i, location;

	bool raw = false;
	char name[16] = "";
	struct option options[] = {
		{ "raw",    no_argument,       NULL, 'r'},
		{ "name",   required_argument, NULL, 'n'},
		{ NULL,     0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		case 'n':
			snprintf(name, sizeof(name), "%s", optarg);
			break;
		default:
			usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	rawdata.data = buff;
	rawdata.length = sizeof(buff);
	gn_data_clear(&data);
	data.ringtone = &ringtone;
	data.raw_data = &rawdata;

	if (argc <= optind) {
		usage(stderr, -1);
		return -1;
	}

	location = (argc > optind + 1) ? atoi(argv[optind + 1]) : -1;

	if (raw) {
		FILE *f;

		if ((f = fopen(argv[optind], "rb")) == NULL) {
			fprintf(stderr, _("Failed to load ringtone.\n"));
			return -1;
		}
		rawdata.length = fread(rawdata.data, 1, rawdata.length, f);
		fclose(f);
		ringtone.location = location;
		if (*name)
			snprintf(ringtone.name, sizeof(ringtone.name), "%s", name);
		else
			snprintf(ringtone.name, sizeof(ringtone.name), "GNOKII");
		error = gn_sm_functions(GN_OP_SetRawRingtone, &data, &state);
	} else {
		if ((error = gn_file_ringtone_read(argv[optind], &ringtone))) {
			fprintf(stderr, _("Failed to load ringtone.\n"));
			return error;
		}
		ringtone.location = location;
		if (*name) snprintf(ringtone.name, sizeof(ringtone.name), "%s", name);
		error = gn_sm_functions(GN_OP_SetRingtone, &data, &state);
	}

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("Send failed: %s\n"), gn_error_print(error));

	return error;
}

static int playringtone(int argc, char *argv[])
{
	gn_ringtone ringtone;
	gn_tone tone;
	gn_error error;
	int i, ulen;
	struct timeval dt;
#if (defined HAVE_TIMEOPS) && (defined HAVE_GETTIMEOFDAY)
	struct timeval t1, t2;
#endif

	int volume = 5;
	struct option options[] = {
		{ "volume", required_argument, NULL, 'v'},
		{ NULL,     0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;
	argv++;
	argc--;

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			volume = atoi(optarg);
			break;
		default:
			usage(stderr, -1); /* FIXME */
			return -1;
		}
	}

	memset(&ringtone, 0, sizeof(ringtone));
	memset(&tone, 0, sizeof(tone));
	gn_data_clear(&data);
	data.ringtone = &ringtone;
	data.tone = &tone;

	if (argc <= optind) {
		usage(stderr, -1);
		return -1;
	}

	if ((error = gn_file_ringtone_read(argv[optind], &ringtone))) {
		fprintf(stderr, _("Failed to load ringtone: %s\n"), gn_error_print(error));
		return error;
	}

#if (defined HAVE_TIMEOPS) && (defined HAVE_GETTIMEOFDAY)
	gettimeofday(&t1, NULL);
	tone.frequency = 0;
	tone.volume = 0;
	gn_sm_functions(GN_OP_PlayTone, &data, &state);
	gettimeofday(&t2, NULL);
	timersub(&t2, &t1, &dt);
#else
	dt.tv_sec = 0;
	dt.tv_usec = 20000;
#endif

	signal(SIGINT, interrupted);
	for (i = 0; !bshutdown && i < ringtone.notes_count; i++) {
		tone.volume = volume;
		gn_ringtone_get_tone(&ringtone, i, &tone.frequency, &ulen);
		if ((error = gn_sm_functions(GN_OP_PlayTone, &data, &state)) != GN_ERR_NONE) break;
		if (ulen > 2 * dt.tv_usec + 20000)
			usleep(ulen - 2 * dt.tv_usec - 20000);
		tone.volume = 0;
		if ((error = gn_sm_functions(GN_OP_PlayTone, &data, &state)) != GN_ERR_NONE) break;
		usleep(20000);
	}
	tone.frequency = 0;
	tone.volume = 0;
	gn_sm_functions(GN_OP_PlayTone, &data, &state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Play succeeded!\n"));
	else
		fprintf(stderr, _("Play failed: %s\n"), gn_error_print(error));

	return error;
}

static int ringtoneconvert(int argc, char *argv[])
{
	gn_ringtone ringtone;
	gn_error error;

	memset(&ringtone, 0, sizeof(ringtone));
	gn_data_clear(&data);
	data.ringtone = &ringtone;

	if (argc != 2) {
		usage(stderr, -1);
		return -1;
	}

	if ((error = gn_file_ringtone_read(argv[0], &ringtone)) != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to load ringtone: %s\n"), gn_error_print(error));
		return error;
	}
	if ((error = gn_file_ringtone_save(argv[1], &ringtone)) != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to save ringtone: %s\n"), gn_error_print(error));
		return error;
	}

	fprintf(stderr, _("%d note(s) converted.\n"), ringtone.notes_count);

	return error;
}

static int getringtonelist(void)
{
	int i;

	init_ringtone_list();

	if (ringtone_list_initialised < 0) {
		fprintf(stderr, _("Failed to get the list of ringtones\n"));
		return GN_ERR_UNKNOWN;
	}

	printf("First user defined ringtone location: %3d\n", ringtone_list.userdef_location);
	printf("Number of user defined ringtones: %d\n\n", ringtone_list.userdef_count);
	printf("loc   rwu   name\n");
	printf("===============================\n");
	for (i = 0; i < ringtone_list.count; i++) {
		printf("%3d   %d%d%d   %-20s\n", ringtone_list.ringtone[i].location,
			ringtone_list.ringtone[i].readable,
			ringtone_list.ringtone[i].writable,
			ringtone_list.ringtone[i].user_defined,
			ringtone_list.ringtone[i].name);
	}

	return GN_ERR_NONE;
}

static gn_error deleteringtone(int argc, char *argv[])
{
	gn_ringtone ringtone;
	gn_error error;
	int i, start, end;

	memset(&ringtone, 0, sizeof(ringtone));
	gn_data_clear(&data);
	data.ringtone = &ringtone;

	switch (argc) {
	case 1:
		start = atoi(argv[0]);
		end = start;
		break;
	case 2:
		start = atoi(argv[0]);
		end = atoi(argv[1]);
		break;
	default:
		usage(stderr, -1);
		return -1;
	}

	for (i = start; i <= end; i++) {
		ringtone.location = i;
		if ((error = gn_sm_functions(GN_OP_DeleteRingtone, &data, &state)) == GN_ERR_NONE)
			printf(_("Ringtone %d deleted\n"), i);
		else
			printf(_("Failed to delete ringtone %d: %s\n"), i, gn_error_print(error));
	}

	return GN_ERR_NONE;
}

static int presskey(void)
{
	gn_error error;
	error = gn_sm_functions(GN_OP_PressPhoneKey, &data, &state);
	if (error == GN_ERR_NONE)
		error = gn_sm_functions(GN_OP_ReleasePhoneKey, &data, &state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Failed to press key: %s\n"), gn_error_print(error));
	return error;
}

static int presskeysequence(void)
{
	gn_error error = GN_ERR_NONE;
	unsigned char *syms = "0123456789#*PGR+-UDMN";
	gn_key_code keys[] = {GN_KEY_0, GN_KEY_1, GN_KEY_2, GN_KEY_3,
			      GN_KEY_4, GN_KEY_5, GN_KEY_6, GN_KEY_7,
			      GN_KEY_8, GN_KEY_9, GN_KEY_HASH,
			      GN_KEY_ASTERISK, GN_KEY_POWER, GN_KEY_GREEN,
			      GN_KEY_RED, GN_KEY_INCREASEVOLUME,
			      GN_KEY_DECREASEVOLUME, GN_KEY_UP, GN_KEY_DOWN,
			      GN_KEY_MENU, GN_KEY_NAMES};
	unsigned char ch, *pos;

	gn_data_clear(&data);
	console_raw();

	while (read(0, &ch, 1) > 0) {
		if ((pos = strchr(syms, toupper(ch))) != NULL)
			data.key_code = keys[pos - syms];
		else
			continue;
		error = presskey();
	}

	return error;
}

static int enterchar(void)
{
	unsigned char ch;
	gn_error error = GN_ERR_NONE;

	gn_data_clear(&data);
	console_raw();

	while (read(0, &ch, 1) > 0) {
		switch (ch) {
		case '\r':
			break;
		case '\n':
			data.key_code = GN_KEY_MENU;
			presskey();
			break;
#ifdef WIN32
		case '\033':
#else
		case '\e':
#endif
			data.key_code = GN_KEY_NAMES;
			presskey();
			break;
		default:
			data.character = ch;
			error = gn_sm_functions(GN_OP_EnterChar, &data, &state);
			if (error != GN_ERR_NONE)
				fprintf(stderr, _("Error entering char: %s\n"), gn_error_print(error));
			break;
		}
	}

	return error;
}

/* Options for --divert:
   --op, -o [register|enable|query|disable|erasure]  REQ
   --type, -t [all|busy|noans|outofreach|notavail]   REQ
   --call, -c [all|voice|fax|data]                   REQ
   --timeout, m time_in_seconds                      OPT
   --number, -n number                               OPT
 */
static int divert(int argc, char **argv)
{
	int opt;
	gn_call_divert cd;
	gn_data data;
	gn_error error;
	struct option options[] = {
		{ "op",      required_argument, NULL, 'o'},
		{ "type",    required_argument, NULL, 't'},
		{ "call",    required_argument, NULL, 'c'},
		{ "number",  required_argument, NULL, 'n'},
		{ "timeout", required_argument, NULL, 'm'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	/* Skip --divert */
	argc--; argv++;

	memset(&cd, 0, sizeof(gn_call_divert));

	while ((opt = getopt_long(argc, argv, "o:t:n:c:m:", options, NULL)) != -1) {
		switch (opt) {
		case 'o':
			if (!strcmp("register", optarg)) {
				cd.operation = GN_CDV_Register;
			} else if (!strcmp("enable", optarg)) {
				cd.operation = GN_CDV_Enable;
			} else if (!strcmp("disable", optarg)) {
				cd.operation = GN_CDV_Disable;
			} else if (!strcmp("erasure", optarg)) {
				cd.operation = GN_CDV_Erasure;
			} else if (!strcmp("query", optarg)) {
				cd.operation = GN_CDV_Query;
			} else {
				usage(stderr, -1);
				return -1;
			}
			break;
		case 't':
			if (!strcmp("all", optarg)) {
				cd.type = GN_CDV_AllTypes;
			} else if (!strcmp("busy", optarg)) {
				cd.type = GN_CDV_Busy;
			} else if (!strcmp("noans", optarg)) {
				cd.type = GN_CDV_NoAnswer;
			} else if (!strcmp("outofreach", optarg)) {
				cd.type = GN_CDV_OutOfReach;
			} else if (!strcmp("notavail", optarg)) {
				cd.type = GN_CDV_NotAvailable;
			} else {
				usage(stderr, -1);
				return -1;
			}
			break;
		case 'c':
			if (!strcmp("all", optarg)) {
				cd.ctype = GN_CDV_AllCalls;
			} else if (!strcmp("voice", optarg)) {
				cd.ctype = GN_CDV_VoiceCalls;
			} else if (!strcmp("fax", optarg)) {
				cd.ctype = GN_CDV_FaxCalls;
			} else if (!strcmp("data", optarg)) {
				cd.ctype = GN_CDV_DataCalls;
			} else {
				usage(stderr, -1);
				return -1;
			}
			break;
		case 'm':
			cd.timeout = atoi(optarg);
			break;
		case 'n':
			strncpy(cd.number.number, optarg, sizeof(cd.number.number) - 1);
			if (cd.number.number[0] == '+') cd.number.type = GN_GSM_NUMBER_International;
			else cd.number.type = GN_GSM_NUMBER_Unknown;
			break;
		default:
			usage(stderr, -1);
			return -1;
		}
	}
	data.call_divert = &cd;
	error = gn_sm_functions(GN_OP_CallDivert, &data, &state);

	if (error == GN_ERR_NONE) {
		fprintf(stdout, _("Divert type: "));
		switch (cd.type) {
		case GN_CDV_AllTypes: fprintf(stdout, _("all\n")); break;
		case GN_CDV_Busy: fprintf(stdout, _("busy\n")); break;
		case GN_CDV_NoAnswer: fprintf(stdout, _("noans\n")); break;
		case GN_CDV_OutOfReach: fprintf(stdout, _("outofreach\n")); break;
		case GN_CDV_NotAvailable: fprintf(stdout, _("notavail\n")); break;
		default: fprintf(stdout, _("unknown(0x%02x)\n"), cd.type);
		}
		fprintf(stdout, _("Call type: "));
		switch (cd.ctype) {
		case GN_CDV_AllCalls: fprintf(stdout, _("all\n")); break;
		case GN_CDV_VoiceCalls: fprintf(stdout, _("voice\n")); break;
		case GN_CDV_FaxCalls: fprintf(stdout, _("fax\n")); break;
		case GN_CDV_DataCalls: fprintf(stdout, _("data\n")); break;
		default: fprintf(stdout, _("unknown(0x%02x)\n"), cd.ctype);
		}
		if (cd.number.number[0]) {
			fprintf(stdout, _("Number: %s\n"), cd.number.number);
			fprintf(stdout, _("Timeout: %d\n"), cd.timeout);
		} else
			fprintf(stdout, _("Divert isn't active.\n"));
	} else {
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	}
	return error;
}

static gn_error smsslave(gn_sms *message)
{
	FILE *output;
	char *s = message->user_data[0].u.text;
	char buf[10240];
	int i = message->number;
	int i1, i2, msgno, msgpart;
	static int unknown = 0;
	char c, number[GN_BCD_STRING_MAX_LENGTH];
	char *p = message->remote.number;
	const char *smsdir = "/tmp/sms";

	if (p[0] == '+') {
		p++;
	}
	strcpy(number, p);
	fprintf(stderr, _("SMS received from number: %s\n"), number);

	/* From Pavel Machek's email to the gnokii-ml (msgid: <20020414212455.GB9528@elf.ucw.cz>):
	 *  It uses fixed format of provider in CR. If your message matches
	 *  WWW1/1:1234-5678 where 1234 is msgno and 5678 is msgpart, it should
	 *  work.
	 */
	while (*s == 'W')
		s++;
	fprintf(stderr, _("Got message %d: %s\n"), i, s);
	if ((sscanf(s, "%d/%d:%d-%c-", &i1, &i2, &msgno, &c) == 4) && (c == 'X'))
		snprintf(buf, sizeof(buf), "%s/mail_%d_", smsdir, msgno);
	else if (sscanf(s, "%d/%d:%d-%d-", &i1, &i2, &msgno, &msgpart) == 4)
		snprintf(buf, sizeof(buf), "%s/mail_%d_%03d", smsdir, msgno, msgpart);
	else	snprintf(buf, sizeof(buf), "%s/sms_%s_%d_%d", smsdir, number, getpid(), unknown++);
	if ((output = fopen(buf, "r")) != NULL) {
		fprintf(stderr, _("### Exists?!\n"));
		return GN_ERR_FAILED;
	}
	mkdir(smsdir, 0700);
	if ((output = fopen(buf, "w+")) == NULL) {
		fprintf(stderr, _("### Cannot create file %s\n"), buf);
		return GN_ERR_FAILED;
	}

	/* Skip formatting chars */
	if (!strstr(buf, "mail"))
		fprintf(output, "%s", message->user_data[0].u.text);
	else {
		s = message->user_data[0].u.text;
		while (!(*s == '-'))
			s++;
		s++;
		while (!(*s == '-'))
			s++;
		s++;
		fprintf(output, "%s", s);
	}
	fclose(output);
	return GN_ERR_NONE;
}

static int smsreader(void)
{
	gn_data data;
	gn_error error;

	data.on_sms = smsslave;
	error = gn_sm_functions(GN_OP_OnSMS, &data, &state);
	if (!error) {
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);
		fprintf(stderr, _("Entered sms reader mode...\n"));

		while (!bshutdown) {
			gn_sm_loop(1, &state);
			/* Some phones may not be able to notify us, thus we give
			   lowlevel chance to poll them */
			error = gn_sm_functions(GN_OP_PollSMS, &data, &state);
		}
		fprintf(stderr, _("Shutting down\n"));

		fprintf(stderr, _("Exiting sms reader mode...\n"));
		data.on_sms = NULL;

		error = gn_sm_functions(GN_OP_OnSMS, &data, &state);
		if (error != GN_ERR_NONE)
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	} else
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));

	return error;
}

static int getsecuritycode()
{
	gn_data data;
	gn_error error;
	gn_security_code sc;

	memset(&sc, 0, sizeof(sc));
	sc.type = GN_SCT_SecurityCode;
	data.security_code = &sc;
	fprintf(stderr, _("Getting security code... \n"));
	error = gn_sm_functions(GN_OP_GetSecurityCode, &data, &state);
	fprintf(stdout, _("Security code is: %s\n"), sc.code);
	return error;
}

static void list_gsm_networks(void)
{

	gn_network network;
	int i = 0;

	printf("Network  Name\n");
	printf("-----------------------------------------\n");
	while (gn_network_get(&network, i++))
		printf("%-7s  %s\n", network.code, network.name);
}

static int getnetworkinfo(void)
{
	gn_network_info networkinfo;
	gn_data data;
	gn_error error;
	int cid, lac;
	char country[4] = {0, 0, 0, 0};

	gn_data_clear(&data);
	data.network_info = &networkinfo;

	if ((error = gn_sm_functions(GN_OP_GetNetworkInfo, &data, &state)) != GN_ERR_NONE) {
		return error;
	}

	cid = (networkinfo.cell_id[0] << 8) + networkinfo.cell_id[1];
	lac = (networkinfo.LAC[0] << 8) + networkinfo.LAC[1];
	memcpy(country, networkinfo.network_code, 3);

	fprintf(stdout, _("Network      : %s (%s)\n"),
			gn_network_name_get((char *)networkinfo.network_code),
			gn_country_name_get((char *)country));
	fprintf(stdout, _("Network code : %s\n"), networkinfo.network_code);
	fprintf(stdout, _("LAC          : %04x\n"), lac);
	fprintf(stdout, _("Cell id      : %04x\n"), cid);

	return 0;
}

static int getlocksinfo(void)
{
	gn_locks_info locks_info[4];
	gn_error error;
	char *locks_names[] = {"MCC+MNC", "GID1", "GID2", "MSIN"};
	int i;

	gn_data_clear(&data);
	data.locks_info = locks_info;

	if ((error = gn_sm_functions(GN_OP_GetLocksInfo, &data, &state)) != GN_ERR_NONE) {
		return error;
	}

	for (i = 0; i < 4; i++) {
		fprintf(stdout, _("%7s : %10s, %7s, %6s, counter %d\n"),
			locks_names[i],
			locks_info[i].data,
			locks_info[i].userlock ? "user" : "factory",
			locks_info[i].closed ? "CLOSED" : "open",
			locks_info[i].counter);
	}
	return 0;
}

/* Get file list. */
static int getfilelist(char *path)
{
    	gn_file_list fi;
	gn_error error;
	int i;

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.path, sizeof(fi.path), "%s", path);

	gn_data_clear(&data);
	data.file_list = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFileList, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get info for %s: %s\n"), path, gn_error_print(error));
	else {
		fprintf(stdout, _("Filelist for path %s:\n"), path);
		for(i=0;i<fi.file_count;i++) {
			fprintf(stdout, _("  %s\n"), fi.files[i]->name);
			free(fi.files[i]);
		}
	}
	return error;
}

/* Get file id */
static int getfileid(char *filename)
{
    	gn_file fi;
	gn_error error;
	int i;

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", filename);

	gn_data_clear(&data);
	data.file = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFileId, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get info for %s: %s\n"),filename, gn_error_print(error));
	else {
		fprintf(stdout, _("Fileid for file %s is %02x %02x %02x %02x %02x %02x\n"), filename, fi.id[0], fi.id[1], fi.id[2], fi.id[3], fi.id[4], fi.id[5]);
	}
	return error;
}

/* Delete file */
static int deletefile(char *filename)
{
    	gn_file fi;
	gn_error error;
	int i;

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", filename);

	gn_data_clear(&data);
	data.file = &fi;

	if ((error = gn_sm_functions(GN_OP_DeleteFile, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to delete %s: %s\n"),filename, gn_error_print(error));
	else {
		fprintf(stdout, _("Deleted: %s\n"), filename);
	}
	return error;
}

/* Get file */
static int getfile(int nargc, char *nargv[])
{
    	gn_file fi;
	gn_error error;
	int i;
	FILE *f;
	char filename2[512];

	if (nargc<1) usage(stderr, -1);

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", nargv[0]);

	gn_data_clear(&data);
	data.file = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFile, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get file %s: %s\n"), nargv[0], gn_error_print(error));
	else {
		if (nargc==1) {
			strncpy(filename2, strrchr(nargv[0], '/') + 1, 512);
			fprintf(stdout, _("Got file %s.  Save to [%s]: "), nargv[0], filename2);
			gn_line_get(stdin, filename2, 512);
			if (filename2[0]==0) strncpy(filename2, strrchr(nargv[0], '/') + 1, 512);
			f = fopen(filename2, "w");
		} else {
			f = fopen(nargv[1], "w");
		}
		if (!f) {
			fprintf(stderr, _("Cannot open file %s\n"), filename2);
			return GN_ERR_FAILED;
		}
		fwrite(fi.file, fi.file_length, 1, f);
		fclose(f);
		free(fi.file);
	}
	return error;
}

/* Get all files */
static int getallfiles(char *path)
{
    	gn_file_list fi;
	gn_error error;
	int i;
	FILE *f;
	char filename2[512];

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.path, sizeof(fi.path), "%s", path);

	gn_data_clear(&data);
	data.file_list = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFileList, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get info for %s: %s\n"), path, gn_error_print(error));
	else {
		*(strrchr(path,'/')+1)=0;
		for(i=0; i<fi.file_count; i++) {
			data.file = fi.files[i];
			strncpy(filename2, fi.files[i]->name, 512);
			snprintf(fi.files[i]->name, 512, "%s%s", path, filename2);
			if ((error = gn_sm_functions(GN_OP_GetFile, &data, &state)) != GN_ERR_NONE)
				fprintf(stderr, _("Failed to get file %s: %s\n"), data.file->name, gn_error_print(error));
			else {
				fprintf(stdout, _("Got file %s.\n"),filename2);
				f = fopen(filename2, "w");
				if (!f) {
					fprintf(stderr, _("Cannot open file %s\n"), filename2);
					return GN_ERR_FAILED;
				}
				fwrite(data.file->file, data.file->file_length, 1, f);
				fclose(f);
				free(data.file->file);
			}
			free(fi.files[i]);
		}
	}
	return error;
}

/* Put file */
static int putfile(int nargc, char *nargv[])
{
    	gn_file fi;
	gn_error error;
	int i;
	FILE *f;

	if (nargc != 2) usage(stderr, -1);

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", nargv[1]);

	gn_data_clear(&data);
	data.file = &fi;

	f=fopen(nargv[0],"r");
	if (!f || fseek(f, 0, SEEK_END)) {
		fprintf(stderr, _("Cannot open file %s\n"), nargv[0]);
		return GN_ERR_FAILED;
	}
	fi.file_length = ftell(f);
	rewind(f);
	fi.file = malloc(fi.file_length);
	if (fread(fi.file, 1, fi.file_length, f) != fi.file_length) {
		fprintf(stderr, _("Cannot read file %s\n"), nargv[0]);
		return GN_ERR_FAILED;
	}

	if ((error = gn_sm_functions(GN_OP_PutFile, &data, &state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to put file to %s: %s\n"), nargv[1], gn_error_print(error));

	free(fi.file);

	return error;
}



/* This is a "convenience" function to allow quick test of new API stuff which
   doesn't warrant a "proper" command line function. */
#ifndef WIN32
static int foogle(char *argv[])
{	
	/* Fill in what you would like to test here... */
	return 0;
}
#endif

static void  gnokii_error_logger(const char *fmt, va_list ap)
{
	if (logfile) {
		vfprintf(logfile, fmt, ap);
		fflush(logfile);
	}
}

static int install_log_handler(void)
{
	char logname[256];
	char *home;
#ifdef WIN32
	char *file = "gnokii-errors";
#else
	char *file = ".gnokii-errors";
#endif

	if ((home = getenv("HOME")) == NULL) {
#ifdef WIN32
		home = ".";
#else
		fprintf(stderr, _("HOME variable missing\n"));
		return -1;
#endif
	}

	snprintf(logname, sizeof(logname), "%s/%s", home, file);

	if ((logfile = fopen(logname, "a")) == NULL) {
		perror("fopen");
		return -1;
	}

	gn_elog_handler = gnokii_error_logger;

	return 0;
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */
int main(int argc, char *argv[])
{
	int c, i, rc = -1;
	int nargc = argc - 2;
	char **nargv;

	/* Every option should be in this array. */
	static struct option long_options[] = {
		/* FIXME: these comments are nice, but they would be more usefull as docs for the user */
		/* Display usage. */
		{ "help",               no_argument,       NULL, OPT_HELP },

		/* Display version and build information. */
		{ "version",            no_argument,       NULL, OPT_VERSION },

		/* Monitor mode */
		{ "monitor",            optional_argument, NULL, OPT_MONITOR },

#ifdef SECURITY

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
		{ "setlogo",            optional_argument, NULL, OPT_SETLOGO },

		/* Get logo */
		{ "getlogo",            required_argument, NULL, OPT_GETLOGO },

		/* View logo */
		{ "viewlogo",           required_argument, NULL, OPT_VIEWLOGO },

		/* Show profile */
		{ "getprofile",         optional_argument, NULL, OPT_GETPROFILE },

		/* Set profile */
		{ "setprofile",         no_argument,       NULL, OPT_SETPROFILE },

		/* Gets the active profile */
		{ "getactiveprofile",   no_argument,       NULL, OPT_GETACTIVEPROFILE },

		/* Sets the active profile */
		{ "setactiveprofile",   required_argument, NULL, OPT_SETACTIVEPROFILE },

		/* Show texts from phone's display */
		{ "displayoutput",      no_argument,       NULL, OPT_DISPLAYOUTPUT },

		/* Simulate pressing the keys */
		{ "keysequence",        no_argument,       NULL, OPT_KEYPRESS },

		/* Simulate pressing the keys */
		{ "enterchar",          no_argument,       NULL, OPT_ENTERCHAR },

		/* Divert calls */
		{ "divert",		required_argument, NULL, OPT_DIVERT },

		/* SMS reader */
		{ "smsreader",          no_argument,       NULL, OPT_SMSREADER },

		/* For development purposes: insert you function calls here */
		{ "foogle",             no_argument,       NULL, OPT_FOOGLE },

		/* Get Security Code */
		{ "getsecuritycode",    no_argument,   	   NULL, OPT_GETSECURITYCODE },

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
		{ "getfilelist",        required_argument,       NULL, OPT_GETFILELIST },
                /* Get file id */
		{ "getfileid",          required_argument,       NULL, OPT_GETFILEID },
                /* Get file */
		{ "getfile",            required_argument,       NULL, OPT_GETFILE },
		/* Get all files */
		{ "getallfiles",        required_argument,       NULL, OPT_GETALLFILES },
                /* Put a file */
		{ "putfile",        required_argument,       NULL, OPT_PUTFILE },
		/* Delete a file */
		{ "deletefile",        required_argument,       NULL, OPT_DELETEFILE },

		{ 0, 0, 0, 0},
	};

	/* Every command which requires arguments should have an appropriate entry
	   in this array. */
	static struct gnokii_arg_len gals[] = {
#ifdef SECURITY
		{ OPT_ENTERSECURITYCODE, 1, 1, 0 },
		{ OPT_CHANGESECURITYCODE,1, 1, 0 },
#endif
		{ OPT_SETDATETIME,       0, 5, 0 },
		{ OPT_SETALARM,          0, 2, 0 },
		{ OPT_DIALVOICE,         1, 1, 0 },
		{ OPT_ANSWERCALL,        1, 1, 0 },
		{ OPT_HANGUP,            1, 1, 0 },
		{ OPT_GETTODO,           1, 3, 0 },
		{ OPT_WRITETODO,         2, 2, 0 },
		{ OPT_GETCALENDARNOTE,   1, 3, 0 },
		{ OPT_WRITECALENDARNOTE, 2, 2, 0 },
		{ OPT_DELCALENDARNOTE,   1, 2, 0 },
		{ OPT_GETPHONEBOOK,      2, 4, 0 },
		{ OPT_WRITEPHONEBOOK,    0, 10, 0 },
		{ OPT_DELETEPHONEBOOK,   2, 3, 0 },
		{ OPT_GETSPEEDDIAL,      1, 1, 0 },
		{ OPT_SETSPEEDDIAL,      3, 3, 0 },
		{ OPT_CREATESMSFOLDER,   1, 1, 0 },
		{ OPT_DELETESMSFOLDER,   1, 1, 0 },
		{ OPT_GETSMS,            2, 5, 0 },
		{ OPT_DELETESMS,         2, 3, 0 },
		{ OPT_SENDSMS,           1, 10, 0 },
		{ OPT_SAVESMS,           0, 10, 0 },
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
		{ OPT_GETFILEID,         1, 1, 0 },
		{ OPT_GETFILE,           1, 2, 0 },
		{ OPT_GETALLFILES,       1, 1, 0 },
		{ OPT_PUTFILE,           2, 2, 0 },
		{ OPT_DELETEFILE,           1, 1, 0 },
		{ 0, 0, 0, 0 },
	};

	if (install_log_handler()) {
		fprintf(stderr, _("WARNING: cannot open logfile, logs will be directed to stderr\n"));
	}

	opterr = 0;

	/* For GNU gettext */
#ifdef ENABLE_NLS
	textdomain("gnokii");
	setlocale(LC_ALL, "");
#endif

	/* Introduce yourself */
	short_version();

	/* Handle command line arguments. */
	c = getopt_long(argc, argv, "", long_options, NULL);
	if (c == -1) 		/* No argument given - we should display usage. */
		usage(stderr, -1);

	switch(c) {
	/* First, error conditions */
	case '?':
	case ':':
		fprintf(stderr, _("Use '%s --help' for usage information.\n"), argv[0]);
		exit(1);
	/* Then, options with no arguments */
	case OPT_HELP:
		usage(stdout, -1);
	case OPT_VERSION:
		version();
		exit(0);
	}

	/* Read config file */
	if (gn_cfg_read_default() < 0) {
		exit(1);
	}
	if (!gn_cfg_phone_load("", &state)) exit(1);

	/* We have to build an array of the arguments which will be passed to the
	   functions.  Please note that every text after the --command will be
	   passed as arguments.  A syntax like gnokii --cmd1 args --cmd2 args will
	   not work as expected; instead args --cmd2 args is passed as a
	   parameter. */
	if ((nargv = malloc(sizeof(char *) * argc)) != NULL) {
		for (i = 2; i < argc; i++)
			nargv[i-2] = argv[i];

		if (checkargs(c, gals, nargc)) {
			free(nargv); /* Wrong number of arguments - we should display usage. */
			usage(stderr, -1);
		}

#ifdef __svr4__
		/* have to ignore SIGALARM */
		sigignore(SIGALRM);
#endif

		/* Initialise the code for the GSM interface. */
		if (c != OPT_VIEWLOGO && c != OPT_FOOGLE && c != OPT_LISTNETWORKS && c != OPT_RINGTONECONVERT)
			businit();

		switch(c) {
		case OPT_MONITOR:
			rc = monitormode(nargc, nargv);
			break;
#ifdef SECURITY
		case OPT_ENTERSECURITYCODE:
			rc = entersecuritycode(optarg);
			break;
		case OPT_GETSECURITYCODESTATUS:
			rc = getsecuritycodestatus();
			break;
		case OPT_CHANGESECURITYCODE:
			rc = changesecuritycode(optarg);
			break;
#endif
		case OPT_GETSECURITYCODE:
			rc = getsecuritycode();
			break;
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
		/* Now, options with arguments */
		case OPT_SETDATETIME:
			rc = setdatetime(nargc, nargv);
			break;
		case OPT_SETALARM:
			rc = setalarm(nargc, nargv);
			break;
		case OPT_DIALVOICE:
			rc = dialvoice(optarg);
			break;
		case OPT_ANSWERCALL:
			rc = answercall(optarg);
			break;
		case OPT_HANGUP:
			rc = hangup(optarg);
			break;
		case OPT_GETTODO:
			rc = gettodo(nargc, nargv);
			break;
		case OPT_WRITETODO:
			rc = writetodo(nargv);
			break;
		case OPT_DELETEALLTODOS:
			rc = deletealltodos();
			break;
		case OPT_GETCALENDARNOTE:
			rc = getcalendarnote(nargc, nargv);
			break;
		case OPT_DELCALENDARNOTE:
			rc = deletecalendarnote(nargc, nargv);
			break;
		case OPT_WRITECALENDARNOTE:
			rc = writecalendarnote(nargv);
			break;
		case OPT_GETPHONEBOOK:
			rc = getphonebook(nargc, nargv);
			break;
		case OPT_WRITEPHONEBOOK:
			rc = writephonebook(argc, argv);
			break;
		case OPT_DELETEPHONEBOOK:
			rc = deletephonebook(nargc, nargv);
			break;
		case OPT_GETSPEEDDIAL:
			rc = getspeeddial(optarg);
			break;
		case OPT_SETSPEEDDIAL:
			rc = setspeeddial(nargv);
			break;
		case OPT_CREATESMSFOLDER:
			rc = createsmsfolder(optarg);
			break;
		case OPT_DELETESMSFOLDER:
			rc = deletesmsfolder(optarg);
			break;
		case OPT_SHOWSMSFOLDERSTATUS:
			rc = showsmsfolderstatus();
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
			rc = savesms(nargc, nargv);
			break;
		case OPT_SENDLOGO:
			rc = sendlogo(nargc, nargv);
			break;
		case OPT_GETSMSC:
			rc = getsmsc(argc, argv);
			break;
		case OPT_SETSMSC:
			rc = setsmsc();
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
		case OPT_GETRINGTONE:
			rc = getringtone(argc, argv);
			break;
		case OPT_SETRINGTONE:
			rc = setringtone(argc, argv);
			break;
		case OPT_SENDRINGTONE:
			rc = sendringtone(nargc, nargv);
			break;
		case OPT_PLAYRINGTONE:
			rc = playringtone(argc, argv);
			break;
		case OPT_GETRINGTONELIST:
			rc = getringtonelist();
			break;
		case OPT_DELETERINGTONE:
			rc = deleteringtone(nargc, nargv);
			break;
		case OPT_RINGTONECONVERT:
			rc = ringtoneconvert(nargc, nargv);
			break;
		case OPT_GETPROFILE:
			rc = getprofile(argc, argv);
			break;
		case OPT_SETPROFILE:
			rc = setprofile();
			break;
		case OPT_GETACTIVEPROFILE:
			rc = getactiveprofile();
			break;
		case OPT_SETACTIVEPROFILE:
			rc = setactiveprofile(nargc, nargv);
			break;
		case OPT_DISPLAYOUTPUT:
			rc = displayoutput();
			break;
		case OPT_KEYPRESS:
			rc = presskeysequence();
			break;
		case OPT_ENTERCHAR:
			rc = enterchar();
			break;
		case OPT_SENDDTMF:
			rc = senddtmf(optarg);
			break;
		case OPT_RESET:
			rc = reset(optarg);
			break;
		case OPT_DIVERT:
			rc = divert(argc, argv);
			break;
		case OPT_SMSREADER:
			rc = smsreader();
			break;
		case OPT_GETWAPBOOKMARK:
			rc = getwapbookmark(optarg);
			break;
		case OPT_WRITEWAPBOOKMARK:
			rc = writewapbookmark(nargc, nargv);
			break;
		case OPT_DELETEWAPBOOKMARK:
			rc = deletewapbookmark(optarg);
			break;
		case OPT_GETWAPSETTING:
			rc = getwapsetting(nargc, nargv);
			break;
		case OPT_WRITEWAPSETTING:
			rc = writewapsetting();
			break;
		case OPT_ACTIVATEWAPSETTING:
			rc = activatewapsetting(optarg);
			break;
		case OPT_LISTNETWORKS:
			list_gsm_networks();
			break;
		case OPT_GETNETWORKINFO:
			rc = getnetworkinfo();
			break;
		case OPT_GETLOCKSINFO:
			rc = getlocksinfo();
			break;
		case OPT_GETFILELIST:
			rc = getfilelist(optarg);
			break;
		case OPT_GETFILEID:
			rc = getfileid(optarg);
			break;
		case OPT_GETFILE:
			rc = getfile(nargc, nargv);
			break;
		case OPT_GETALLFILES:
			rc = getallfiles(optarg);
			break;
		case OPT_PUTFILE:
			rc = putfile(nargc, nargv);
			break;
		case OPT_DELETEFILE:
			rc = deletefile(optarg);
			break;
#ifndef WIN32
		case OPT_FOOGLE:
			rc = foogle(nargv);
			break;
#endif
		default:
			fprintf(stderr, _("Unknown option: %d\n"), c);
			break;

		}
		exit(rc);
	}

	fprintf(stderr, _("Wrong number of arguments\n"));
	exit(1);
}
