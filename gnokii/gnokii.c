/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001       Pavel Machek
  Copyright (C) 2001-2002  Pawe³ Kot

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries and other stuff.

  WARNING: this code is the test tool. Well, our test tool is now
  really powerful and useful :-)

*/

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


#include "gnokii.h"

#include "gsm-sms.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-networks.h"
#include "cfgreader.h"
#include "gsm-filetypes.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"
#include "gsm-statemachine.h"

typedef enum {
	OPT_HELP,
	OPT_VERSION,
	OPT_MONITOR,
	OPT_ENTERSECURITYCODE,
	OPT_GETSECURITYCODESTATUS,
	OPT_SETDATETIME,
	OPT_GETDATETIME,
	OPT_SETALARM,
	OPT_GETALARM,
	OPT_DIALVOICE,
	OPT_GETCALENDARNOTE,
	OPT_WRITECALENDARNOTE,
	OPT_DELCALENDARNOTE,
	OPT_GETDISPLAYSTATUS,
	OPT_GETMEMORY,
	OPT_WRITEPHONEBOOK,
	OPT_GETSPEEDDIAL,
	OPT_SETSPEEDDIAL,
	OPT_GETSMS,
	OPT_DELETESMS,
	OPT_SENDSMS,
	OPT_SAVESMS,
	OPT_SENDLOGO,
	OPT_SENDRINGTONE,
	OPT_GETSMSC,
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
	OPT_SETRINGTONE,
	OPT_GETPROFILE,
	OPT_SETPROFILE,
	OPT_DISPLAYOUTPUT,
	OPT_KEYPRESS,
	OPT_DIVERT,
	OPT_SMSREADER,
	OPT_FOOGLE
} opt_index;

static char *model;      /* Model from .gnokiirc file. */
static char *Port;       /* Serial port from .gnokiirc file */
static char *Initlength; /* Init length from .gnokiirc file */
static char *Connection; /* Connection type from .gnokiirc file */
static char *BinDir;     /* Binaries directory from .gnokiirc file - not used here yet */
static FILE *logfile = NULL;
static char *lockfile = NULL;

/* Local variables */
static char *GetProfileCallAlertString(int code)
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

static char *GetProfileVolumeString(int code)
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

static char *GetProfileKeypadToneString(int code)
{
	switch (code) {
	case PROFILE_KEYPAD_OFF:		return "Off";
	case PROFILE_KEYPAD_LEVEL1:		return "Level 1";
	case PROFILE_KEYPAD_LEVEL2:		return "Level 2";
	case PROFILE_KEYPAD_LEVEL3:		return "Level 3";
	default:				return "Unknown";
	}
}

static char *GetProfileMessageToneString(int code)
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

static char *GetProfileWarningToneString(int code)
{
	switch (code) {
	case PROFILE_WARNING_OFF:		return "Off";
	case PROFILE_WARNING_ON:		return "On";
	default:				return "Unknown";
	}
}

static char *GetProfileVibrationString(int code)
{
	switch (code) {
	case PROFILE_VIBRATION_OFF:		return "Off";
	case PROFILE_VIBRATION_ON:		return "On";
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
			  "Copyright (C) Pavel Janík ml. <Pavel.Janik@suse.cz>, 1999, 2000\n"
			  "Copyright (C) Pavel Machek <pavel@ucw.cz>, 2001\n"
			  "Copyright (C) Pawe³ Kot <pkot@linuxnews.pl>, 2001-2002\n"
			  "gnokii is free software, covered by the GNU General Public License, and you are\n"
			  "welcome to change it and/or distribute copies of it under certain conditions.\n"
			  "There is absolutely no warranty for gnokii.  See GPL for details.\n"
			  "Built %s %s for %s on %s \n"), __TIME__, __DATE__, model, Port);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options. */
static int usage(void)
{
	fprintf(stderr, _("   usage: gnokii [--help|--monitor|--version]\n"
			  "          gnokii --getmemory memory_type start_number [end_number|end]\n"
			  "          gnokii --writephonebook [-i]\n"
			  "          gnokii --getspeeddial number\n"
			  "          gnokii --setspeeddial number memory_type location\n"
			  "          gnokii --getsms memory_type start [end] [-f file] [-F file] [-d]\n"
			  "          gnokii --deletesms memory_type start [end]\n"
			  "          gnokii --sendsms destination [--smsc message_center_number |\n"
			  "                 --smscno message_center_index] [-r] [-C n] [-v n]\n"
			  "                 [--long n]\n"
			  "          gnokii --savesms [-m] [-l n] [-i]\n"
			  "          gnokii --smsreader\n"
			  "          gnokii --getsmsc message_center_number\n"
			  "          gnokii --setdatetime [YYYY [MM [DD [HH [MM]]]]]\n"
			  "          gnokii --getdatetime\n"
			  "          gnokii --setalarm [HH MM]\n"
			  "          gnokii --getalarm\n"
			  "          gnokii --dialvoice number\n"
			  "          gnokii --getcalendarnote start [end] [-v]\n"
			  "          gnokii --writecalendarnote vcardfile number\n"
			  "          gnokii --deletecalendarnote start [end]\n"
			  "          gnokii --getdisplaystatus\n"
			  "          gnokii --netmonitor {reset|off|field|devel|next|nr}\n"
			  "          gnokii --identify\n"
			  "          gnokii --senddtmf string\n"
			  "          gnokii --sendlogo {caller|op|picture} destination logofile [network code]\n"
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
			  "          gnokii --getprofile [start_number [end_number]] [-r|--raw]\n"
			  "          gnokii --setprofile\n"
			  "          gnokii --displayoutput\n"
			  "          gnokii --keysequence\n"
			  "          gnokii --divert {--op|-o} {register|enable|query|disable|erasure}\n"
			  "                 {--type|-t} {all|busy|noans|outofreach|notavail}\n"
			  "                 {--call|-c} {all|voice|fax|data}\n"
			  "                 [{--timeout|-m} time_in_seconds]\n"
			  "                 [{--number|-n} number]\n"
		));
#ifdef SECURITY
	fprintf(stderr, _(
		"          gnokii --entersecuritycode PIN|PIN2|PUK|PUK2\n"
		"          gnokii --getsecuritycodestatus\n"
		));
#endif
	unlock_device(lockfile);
	exit(-1);
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

static GSM_Statemachine State;
static GSM_Data data;

static void fbusinit(void (*rlp_handler)(RLP_F96Frame *frame))
{
	int count = 0;
	GSM_Error error;
	GSM_ConnectionType connection = GCT_Serial;

	GSM_DataClear(&data);

	if (!strcasecmp(Connection, "dau9p"))    connection = GCT_DAU9P; /* Use only with 6210/7110 for faster connection with such cable */
	if (!strcasecmp(Connection, "infrared")) connection = GCT_Infrared;
	if (!strcasecmp(Connection, "irda"))     connection = GCT_Irda;

	lockfile = lock_device(Port);

	/* Initialise the code for the GSM interface. */
	error = GSM_Initialise(model, Port, Initlength, connection, rlp_handler, &State);
	if (error != GE_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! Quitting.\n"));
		unlock_device(lockfile);
		exit(-1);
	}

	/* First (and important!) wait for GSM link to be active. We allow 10
	   seconds... */
	while (count++ < 200 && *GSM_LinkOK == false)
		usleep(50000);

	if (*GSM_LinkOK == false) {
		fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting.\n"));
		unlock_device(lockfile);
		exit(-1);
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

/* Send  SMS messages. */
static int sendsms(int argc, char *argv[])
{
	GSM_SMSMessage SMS;
	GSM_Error error;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[255 * GSM_MAX_SMS_LENGTH];
	int input_len, chars_read;
	int i;

	struct option options[] = {
		{ "smsc",    required_argument, NULL, '1'},
		{ "smscno",  required_argument, NULL, '2'},
		{ "long",    required_argument, NULL, '3'},
		{ "picture", required_argument, NULL, '4'},
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

	memset(&SMS, 0, sizeof(GSM_SMSMessage));

	SMS.Type = SMS_Submit;
	SMS.DCS.Type = SMS_GeneralDataCoding;
	SMS.DCS.u.General.Compressed = false;
	SMS.DCS.u.General.Alphabet = SMS_DefaultAlphabet;
	SMS.DCS.u.General.Class = 0;
	SMS.MessageCenter.No = 1;
	SMS.Validity.VPF = SMS_RelativeFormat;
	SMS.Validity.u.Relative = 4320; /* 4320 minutes == 72 hours */
	SMS.UDH_No = 0;
	SMS.Report = false;

	memset(&SMS.RemoteNumber.number, 0, sizeof(SMS.RemoteNumber.number));
	strncpy(SMS.RemoteNumber.number, argv[0], sizeof(SMS.RemoteNumber.number - 1));
	if (SMS.RemoteNumber.number[0] == '+') SMS.RemoteNumber.type = SMS_International;
	else SMS.RemoteNumber.type = SMS_Unknown;

	optarg = NULL;
	optind = 0;

	while ((i = getopt_long(argc, argv, "r8cC:v:", options, NULL)) != -1) {
		switch (i) {       // -8 is for 8-bit data, -c for compression. both are not yet implemented.
		case '1': /* SMSC number */
			SMS.MessageCenter.No = 0;
			memset(&SMS.MessageCenter.Number, 0, sizeof(SMS.MessageCenter.Number));
			strncpy(SMS.MessageCenter.Number, optarg, sizeof(SMS.MessageCenter.Number) - 1);
			if (SMS.MessageCenter.Number[0] == '+') SMS.MessageCenter.Type = SMS_International;
			else SMS.MessageCenter.Type = SMS_Unknown;
			break;
		case '2': /* SMSC number index in phone memory */
			SMS.MessageCenter.No = atoi(optarg);

			if (SMS.MessageCenter.No < 1 || SMS.MessageCenter.No > 5)
				usage();
			data.MessageCenter = &SMS.MessageCenter;
			error = SM_Functions(GOP_GetSMSCenter, &data, &State);
			break;
		case '3': /* we send long message */
			input_len = atoi(optarg);
			if (input_len > 255 * GSM_MAX_SMS_LENGTH) {
				fprintf(stderr, _("Input too long!\n"));
				return -1;
			}
			break;
		case '4': /* we send multipart message - picture message */
			SMS.UDH_No = 1;
			break;
		case 'r': /* request for delivery report */
			SMS.Report = true;
			break;
		case 'C': /* class Message */
			switch (*optarg) {
			case '0':
				SMS.DCS.u.General.Class = 1;
				break;
			case '1':
				SMS.DCS.u.General.Class = 2;
				break;
			case '2':
				SMS.DCS.u.General.Class = 3;
				break;
			case '3':
				SMS.DCS.u.General.Class = 4;
				break;
			default:
				usage();
			}
			break;
		case 'v':
			SMS.Validity.u.Relative = atoi(optarg);
			break;
		default:
			usage(); /* Would be better to have an sendsms_usage() here. */
		}
	}

	/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, input_len, stdin);

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return -1;
	} else if (chars_read > input_len || chars_read > sizeof(SMS.UserData[0].u.Text) - 1) {
		fprintf(stderr, _("Input too long!\n"));
		return -1;
	}

	/*  Null terminate. */
	message_buffer[chars_read] = 0x00;
	if (chars_read > 0 && message_buffer[chars_read - 1] == '\n') message_buffer[--chars_read] = 0x00;
	if (chars_read < 1) {
		fprintf(stderr, _("Empty message. Quitting"));
		return -1;
	}
	SMS.UserData[0].Type = SMS_PlainText;
	strncpy(SMS.UserData[0].u.Text, message_buffer, chars_read);
	SMS.UserData[0].u.Text[chars_read] = 0;
	data.SMSMessage = &SMS;

	/* Send the message. */
	error = SendSMS(&data, &State);

	if (error == GE_SMSSENDOK) {
		fprintf(stdout, _("Send succeeded!\n"));
	} else {
		fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);
	}

	return 0;
}

static int savesms(int argc, char *argv[])
{
	GSM_SMSMessage SMS;
	GSM_Error error = GE_INTERNALERROR;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	char message_buffer[255 * GSM_MAX_SMS_LENGTH];
	int input_len, chars_read;
	int i, confirm = -1;
	int interactive = 0;
	char ans[8];

	/* Defaults */
	SMS.Type = SMS_Deliver;
	SMS.DCS.Type = SMS_GeneralDataCoding;
	SMS.DCS.u.General.Compressed = false;
	SMS.DCS.u.General.Alphabet = SMS_DefaultAlphabet;
	SMS.DCS.u.General.Class = 0;
	SMS.MessageCenter.No = 1;
	SMS.Validity.VPF = SMS_RelativeFormat;
	SMS.Validity.u.Relative = 4320; /* 4320 minutes == 72 hours */
	SMS.UDH_No = 0;
	SMS.Status = SMS_Unsent;
	SMS.Number = 0;

	input_len = GSM_MAX_SMS_LENGTH;

	/* Option parsing */
	while ((i = getopt(argc, argv, "ml:in:s:c:")) != -1) {
		switch (i) {
		case 'm': /* mark the message as sent */
			SMS.Status = SMS_Sent;
			break;
		case 'l': /* Specify the location */
			SMS.Number = atoi(optarg);
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
			return -1;
		}
	}

	if (interactive) {
		GSM_SMSMessage aux;

		aux.Number = SMS.Number;
		data.SMSMessage = &aux;
		error = SM_Functions(GOP_GetSMS, &data, &State);
		switch (error) {
		case GE_NONE:
			fprintf(stderr, _("Message at specified location exists. "));
			while (confirm < 0) {
				fprintf(stderr, _("Overwrite? (yes/no) "));
				GetLine(stdin, ans, 7);
				if (!strcmp(ans, _("yes"))) confirm = 1;
				else if (!strcmp(ans, _("no"))) confirm = 0;
			}
			if (!confirm) {
				return 0;
			}
			else break;
		case GE_INVALIDSMSLOCATION:
			fprintf(stderr, _("Invalid location\n"));
			return -1;
		default:
			dprintf("Location %d empty. Saving\n", SMS.Number);
			break;
		}
	}
	chars_read = fread(message_buffer, 1, input_len, stdin);

	if (chars_read == 0) {

		fprintf(stderr, _("Couldn't read from stdin!\n"));
		return -1;

	} else if (chars_read > input_len || chars_read > sizeof(SMS.UserData[0].u.Text) - 1) {

		fprintf(stderr, _("Input too long!\n"));
		return -1;

	}

	strncpy(SMS.UserData[0].u.Text, message_buffer, chars_read);
	SMS.UserData[0].u.Text[chars_read] = 0;

/*	if (GSM && GSM->SaveSMSMessage) error = GSM->SaveSMSMessage(&SMS); */

	if (error == GE_NONE) {
		fprintf(stdout, _("Saved!\n"));
	} else {
		fprintf(stdout, _("Saving failed (error=%d)\n"), error);
	}
	sleep(10);

	return 0;
}

/* Get SMSC number */
static int getsmsc(char *MessageCenterNumber)
{
	SMS_MessageCenter	MessageCenter;
	GSM_Data		data;
	GSM_Error		error;

	memset(&MessageCenter, 0, sizeof(MessageCenter));
	MessageCenter.No = atoi(MessageCenterNumber);

	GSM_DataClear(&data);
	data.MessageCenter = &MessageCenter;
	error = SM_Functions(GOP_GetSMSCenter, &data, &State);

	switch (error) {
	case GE_NONE:
		fprintf(stdout, _("%d. SMS center (%s) number is %s\n"), MessageCenter.No, MessageCenter.Name, MessageCenter.Number);
		fprintf(stdout, _("Default recipient number is %s\n"), MessageCenter.Recipient);
		fprintf(stdout, _("Messages sent as "));

		switch (MessageCenter.Format) {
		case SMS_FText:
			fprintf(stdout, _("Text"));
			break;
		case SMS_FVoice:
			fprintf(stdout, _("VoiceMail"));
			break;
		case SMS_FFax:
			fprintf(stdout, _("Fax"));
			break;
		case SMS_FEmail:
		case SMS_FUCI:
			fprintf(stdout, _("Email"));
			break;
		case SMS_FERMES:
			fprintf(stdout, _("ERMES"));
			break;
		case SMS_FX400:
			fprintf(stdout, _("X.400"));
			break;
		default:
			fprintf(stdout, _("Unknown"));
			break;
		}

		printf("\n");
		fprintf(stdout, _("Message validity is "));

		switch (MessageCenter.Validity) {
		case SMS_V1H:
			fprintf(stdout, _("1 hour"));
			break;
		case SMS_V6H:
			fprintf(stdout, _("6 hours"));
			break;
		case SMS_V24H:
			fprintf(stdout, _("24 hours"));
			break;
		case SMS_V72H:
			fprintf(stdout, _("72 hours"));
			break;
		case SMS_V1W:
			fprintf(stdout, _("1 week"));
			break;
		case SMS_VMax:
			fprintf(stdout, _("Maximum time"));
			break;
		default:
			fprintf(stdout, _("Unknown"));
			break;
		}

		fprintf(stdout, "\n");

		break;
	case GE_NOTIMPLEMENTED:
		fprintf(stderr, _("Function not implemented in %s model!\n"), model);
		break;
	default:
		fprintf(stdout, _("SMS center can not be found :-(\n"));
		break;
	}

	return error;
}

/* Get SMS messages. */
static int getsms(int argc, char *argv[])
{
	int del = 0;
	SMS_Folder folder;
	SMS_FolderList folderlist;
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
				} else  usage();
				break;
			default:
				usage();
			}
		}
	}
	data.SMSFolderList = &folderlist;
	folder.FolderID = 0;
	data.SMSFolder = &folder;
	/* Now retrieve the requested entries. */
	for (count = start_message; count <= end_message; count ++) {
		int offset = 0;

		message.Number = count;
		data.SMSMessage = &message;
		dprintf("MemoryType (gnokii.c) : %i\n", message.MemoryType);
		error = GetSMS(&data, &State);

		switch (error) {
		case GE_NONE:
			switch (message.Type) {
			case SMS_Submit:
				fprintf(stdout, _("%d. MO Message "), message.Number);
				switch (message.Status) {
				case SMS_Read:
					fprintf(stdout, _("(read)\n"));
					break;
				case SMS_Unread:
					fprintf(stdout, _("(unread)\n"));
					break;
				case SMS_Sent:
					fprintf(stdout, _("(sent)\n"));
					break;
				case SMS_Unsent:
					fprintf(stdout, _("(unsent)\n"));
					break;
				default:
					fprintf(stdout, _("(read)\n"));
					break;
				}
				fprintf(stdout, _("Text: %s\n\n"), message.UserData[0].u.Text);
				break;
			case SMS_Delivery_Report:
				fprintf(stdout, _("%d. Delivery Report "), message.Number);
				if (message.Status)
					fprintf(stdout, _("(read)\n"));
				else
					fprintf(stdout, _("(not read)\n"));
				fprintf(stdout, _("Sending date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.Time.Day, message.Time.Month, message.Time.Year, \
					message.Time.Hour, message.Time.Minute, message.Time.Second);
				if (message.Time.Timezone) {
					if (message.Time.Timezone > 0)
						fprintf(stdout,_("+%02d00"), message.Time.Timezone);
					else
						fprintf(stdout,_("%02d00"), message.Time.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Response date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.SMSCTime.Day, message.SMSCTime.Month, message.SMSCTime.Year, \
					message.SMSCTime.Hour, message.SMSCTime.Minute, message.SMSCTime.Second);
				if (message.SMSCTime.Timezone) {
					if (message.SMSCTime.Timezone > 0)
						fprintf(stdout,_("+%02d00"),message.SMSCTime.Timezone);
					else
						fprintf(stdout,_("%02d00"),message.SMSCTime.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Receiver: %s Msg Center: %s\n"), message.RemoteNumber.number, message.MessageCenter.Number);
				fprintf(stdout, _("Text: %s\n\n"), message.UserData[0].u.Text);
				break;
			case SMS_Picture:
				fprintf(stdout, _("Picture Message\n"));
				fprintf(stdout, _("Date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.Time.Day, message.Time.Month, message.Time.Year, \
					message.Time.Hour, message.Time.Minute, message.Time.Second);
				if (message.Time.Timezone) {
					if (message.Time.Timezone > 0)
						fprintf(stdout,_("+%02d00"),message.Time.Timezone);
					else
						fprintf(stdout,_("%02d00"),message.Time.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.RemoteNumber.number, message.MessageCenter.Number);
				fprintf(stdout, _("Bitmap:\n"));
				GSM_PrintBitmap(&message.UserData[0].u.Bitmap);
				fprintf(stdout, _("Text:\n"));
				fprintf(stdout, "%s\n", message.UserData[1].u.Text);
				break;
			default:
				fprintf(stdout, _("%d. Inbox Message "), message.Number);
				switch (message.Status) {
				case SMS_Read:
					fprintf(stdout, _("(read)\n"));
					break;
				case SMS_Unread:
					fprintf(stdout, _("(unread)\n"));
					break;
				case SMS_Sent:
					fprintf(stdout, _("(sent)\n"));
					break;
				case SMS_Unsent:
					fprintf(stdout, _("(unsent)\n"));
					break;
				default:
					fprintf(stdout, _("(read)\n"));
					break;
				}
				fprintf(stdout, _("Date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.Time.Day, message.Time.Month, message.Time.Year, \
					message.Time.Hour, message.Time.Minute, message.Time.Second);
				if (message.Time.Timezone) {
					if (message.Time.Timezone > 0)
						fprintf(stdout,_("+%02d00"),message.Time.Timezone);
					else
						fprintf(stdout,_("%02d00"),message.Time.Timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.RemoteNumber.number, message.MessageCenter.Number);
				switch (message.UDH[0].Type) {
				case SMS_OpLogo:
					fprintf(stdout, _("GSM operator logo for %s (%s) network.\n"), bitmap.netcode, GSM_GetNetworkName(bitmap.netcode));
					if (!strcmp(message.RemoteNumber.number, "+998000005") && !strcmp(message.MessageCenter.Number, "+886935074443")) fprintf(stdout, _("Saved by Logo Express\n"));
					if (!strcmp(message.RemoteNumber.number, "+998000002") || !strcmp(message.RemoteNumber.number, "+998000003")) fprintf(stdout, _("Saved by Operator Logo Uploader by Thomas Kessler\n"));
					offset = 3;
				case SMS_CallerIDLogo:
					fprintf(stdout, ("Logo:\n"));
					/* put bitmap into bitmap structure */
					GSM_ReadSMSBitmap(message.UDH[0].Type, message.UserData[0].u.Text+2+offset, message.UserData[0].u.Text, &bitmap);
					GSM_PrintBitmap(&bitmap);
					if (*filename) {
						error = GE_NONE;
						if ((stat(filename, &buf) == 0)) {
							fprintf(stdout, _("File %s exists.\n"), filename);
							fprintf(stdout, _("Overwrite? (yes/no) "));
							GetLine(stdin, ans, 4);
							if (!strcmp(ans, _("yes"))) {
								error = GSM_SaveBitmapFile(filename, &bitmap);
							}
						} else error = GSM_SaveBitmapFile(filename, &bitmap);
						if (error != GE_NONE) fprintf(stderr, _("Couldn't save logofile %s!\n"), filename);
					}
					break;
				case SMS_Ringtone:
					fprintf(stdout, _("Ringtone\n"));
					break;
				case SMS_ConcatenatedMessages:
					fprintf(stdout, _("Linked (%d/%d):\n"),
						message.UDH[0].u.ConcatenatedShortMessage.CurrentNumber,
						message.UDH[0].u.ConcatenatedShortMessage.MaximumNumber);
				case SMS_NoUDH:
					fprintf(stdout, _("Text:\n%s\n\n"), message.UserData[0].u.Text);
					if ((mode != -1) && *filename) {
						char buf[1024];
						sprintf(buf, "%s%d", filename, count);
						mode = GSM_SaveTextFile(buf, message.UserData[0].u.Text, mode);
					}
					break;
				case SMS_WAPvCard:
					fprintf(stdout, _("WAP vCard:\n%s"), message.UserData[0].u.Text);
					break;
				case SMS_WAPvCalendar:
					fprintf(stdout, _("WAP vCalendar:\n%s"), message.UserData[0].u.Text);
					break;
				case SMS_WAPvCardSecure:
					fprintf(stdout, _("WAP vCardSecure:\n%s"), message.UserData[0].u.Text);
					break;
				case SMS_WAPvCalendarSecure:
					fprintf(stdout, _("WAP vCalendarSecure:\n%s"), message.UserData[0].u.Text);
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
			return -1;
		case GE_INVALIDSMSLOCATION:
			fprintf(stderr, _("Invalid location: %s %d\n"), memory_type_string, count);
			break;
		case GE_EMPTYSMSLOCATION:
			fprintf(stderr, _("SMS location %s %d empty.\n"), memory_type_string, count);
			break;
		default:
			fprintf(stdout, _("GetSMS %s %d failed! (%s)\n\n"), memory_type_string, count, print_error(error));
			break;
		}
	}

	return 0;
}

/* Delete SMS messages. */
static int deletesms(int argc, char *argv[])
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

	start_message = end_message = atoi(argv[1]);
	if (argc > 2) end_message = atoi(argv[2]);

	/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		message.Number = count;
		data.SMSMessage = &message;
		error = SM_Functions(GOP_DeleteSMS, &data, &State);

		if (error == GE_NONE)
			fprintf(stdout, _("Deleted SMS %s %d\n"), memory_type_string, count);
		else {
			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), model);
				return -1;
			}
			fprintf(stdout, _("DeleteSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
		}
	}

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
static int entersecuritycode(char *type)
{
	GSM_Error test;
	GSM_SecurityCode SecurityCode;

	if (!strcmp(type, "PIN"))
		SecurityCode.Type = GSCT_Pin;
	else if (!strcmp(type, "PUK"))
		SecurityCode.Type = GSCT_Puk;
	else if (!strcmp(type, "PIN2"))
		SecurityCode.Type = GSCT_Pin2;
	else if (!strcmp(type, "PUK2"))
		SecurityCode.Type = GSCT_Puk2;
	/* FIXME: Entering of SecurityCode does not work :-(
	else if (!strcmp(type, "SecurityCode"))
		SecurityCode.Type = GSCT_SecurityCode;
	*/
	else
		usage();

	memset(&SecurityCode.Code, 0, sizeof(SecurityCode.Code));
#ifdef WIN32
	printf("Enter your code: ");
	fgets(SecurityCode.Code, sizeof(SecurityCode.Code), stdin);
#else
	/* FIXME: manual says: Do not use it */
	strncpy(SecurityCode.Code, getpass(_("Enter your code: ")), sizeof(SecurityCode.Code - 1));
#endif

/*	if (GSM && GSM->EnterSecurityCode) test = GSM->EnterSecurityCode(SecurityCode);*/
	if (test == GE_INVALIDSECURITYCODE)
		fprintf(stdout, _("Error: invalid code.\n"));
	else if (test == GE_NONE)
		fprintf(stdout, _("Code ok.\n"));
	else if (test == GE_NOTIMPLEMENTED)
		fprintf(stderr, _("Function not implemented in %s model!\n"), model);
	else
		fprintf(stdout, _("Other error.\n"));


	return 0;
}

static int getsecuritycodestatus(void)
{
	int Status;

/*	if (GSM && GSM->GetSecurityCodeStatus && GSM->GetSecurityCodeStatus(&Status) == GE_NONE) {

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
	}*/

	return 0;
}


#endif

/* Voice dialing mode. */
static int dialvoice(char *Number)
{
/*	if (GSM && GSM->DialVoice) GSM->DialVoice(Number);*/

	return 0;
}

/* FIXME: Integrate with sendsms */
/* The following function allows to send logos using SMS */
static int sendlogo(int argc, char *argv[])
{
	GSM_SMSMessage SMS;
	GSM_Error error;

	/* Default settings for SMS message:
	   - no delivery report
	   - Class Message 1
	   - no compression
	   - 8 bit data
	   - SMSC no. 1
	   - validity 3 days
	   - set UserDataHeaderIndicator
	*/
	SMS.Type = SMS_Submit;
	SMS.DCS.Type = SMS_GeneralDataCoding;
	SMS.DCS.u.General.Compressed = false;
	SMS.DCS.u.General.Alphabet = SMS_8bit;
	SMS.DCS.u.General.Class = 2;
	SMS.MessageCenter.No = 1;
	SMS.Validity.VPF = SMS_RelativeFormat;
	SMS.Validity.u.Relative = 4320; /* 4320 minutes == 72 hours */
	SMS.UDH_No = 1;
	SMS.Status = SMS_Unsent;
	SMS.Number = 0;

	/* The first argument is the type of the logo. */
	if (!strcmp(argv[0], "op")) {
		SMS.UDH[0].Type = SMS.UserData[0].u.Bitmap.type = SMS_OpLogo;
		fprintf(stderr, _("Sending operator logo.\n"));
	} else if (!strcmp(argv[0], "caller")) {
		SMS.UDH[0].Type = SMS.UserData[0].u.Bitmap.type = SMS_CallerIDLogo;
		fprintf(stderr, _("Sending caller line identification logo.\n"));
	} else if (!strcmp(argv[0], "picture")) {
		SMS.UDH[0].Type = SMS.UserData[0].u.Bitmap.type = SMS_MultipartMessage;
		fprintf(stderr, _("Sending Multipart Message: Picture Message.\n"));
	} else {
		fprintf(stderr, _("You should specify what kind of logo to send!\n"));
		return (-1);
	}

	SMS.UserData[0].Type = SMS_BitmapData;

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&SMS.RemoteNumber.number, 0, sizeof(SMS.RemoteNumber.number));
	strncpy(SMS.RemoteNumber.number, argv[1], sizeof(SMS.RemoteNumber.number) - 1);

	/* The third argument is the bitmap file. */
	GSM_ReadBitmapFile(argv[2], &SMS.UserData[0].u.Bitmap);

	/* If we are sending op logo we can rewrite network code. */
	if (SMS.UDH[0].Type == SMS_OpLogo) {
		/*
		 * The fourth argument, if present, is the Network code of the operator.
		 * Network code is in this format: "xxx yy".
		 */
		if (argc > 3) {
			memcpy(SMS.UserData[0].u.Bitmap.netcode, 0, sizeof(SMS.UserData[0].u.Bitmap.netcode));
			strncpy(SMS.UserData[0].u.Bitmap.netcode, argv[3], sizeof(SMS.UserData[0].u.Bitmap.netcode) - 1);
			dprintf("Operator code: %s\n", argv[3]);
		}
	}

	/* FIXME: read from the stdin */
	if (SMS.UDH[0].Type == SMS_MultipartMessage) {
		SMS.UserData[1].Type = SMS_PlainText;
		strcpy(SMS.UserData[1].u.Text, "testtest");
	}

	/* Send the message. */
	data.SMSMessage = &SMS;

	/* Send the message. */
	error = SM_Functions(GOP_SendSMS, &data, &State);

	if (error == GE_SMSSENDOK)
		fprintf(stdout, _("Send succeeded!\n"));
	else
		fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

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
			if (!strcmp(ans, _("O")) || !strcmp(ans, _("o"))) confirm = 1;
			if (!strcmp(ans, _("N")) || !strcmp(ans, _("n"))) confirm = 2;
			if (!strcmp(ans, _("S")) || !strcmp(ans, _("s"))) return GE_USERCANCELED;
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

static int getlogo(int argc, char *argv[])
{
	GSM_Bitmap bitmap;
	GSM_Error error;

	bitmap.type = GSM_None;

	if (!strcmp(argv[0], "op"))
		bitmap.type = GSM_OperatorLogo;

	if (!strcmp(argv[0], "caller")) {
		/* There is caller group number missing in argument list. */
		if (argc == 3) {
			bitmap.number = argv[2][0] - '0';
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

		data.Bitmap = &bitmap;
		error = SM_Functions(GOP_GetBitmap, &data, &State);

		switch (error) {
		case GE_NONE:
			if (bitmap.type == GSM_DealerNoteText) fprintf(stdout, _("Dealer welcome note "));
			if (bitmap.type == GSM_WelcomeNoteText) fprintf(stdout, _("Welcome note "));
			if (bitmap.type == GSM_DealerNoteText || bitmap.type == GSM_WelcomeNoteText) {
				if (bitmap.text[0]) {
					fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				} else {
					fprintf(stdout, _("currently empty\n"));
				}
			} else {
				if (bitmap.width) {
					memset(&bitmap.netcode, 0, sizeof(bitmap.netcode));
					switch (bitmap.type) {
					case GSM_OperatorLogo:
						fprintf(stdout, _("Operator logo for %s (%s) network got succesfully\n"), bitmap.netcode, GSM_GetNetworkName(bitmap.netcode));
						if (argc == 3) {
							strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
							if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
								fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
								return -1;
							}
						}
						break;
					case GSM_StartupLogo:
						fprintf(stdout, _("Startup logo got successfully\n"));
						if (argc == 3) {
							strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
							if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
								fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
								return -1;
							}
						}
						break;
					case GSM_CallerLogo:
						fprintf(stdout, _("Caller logo got successfully\n"));
						if (argc == 4) {
							strncpy(bitmap.netcode, argv[3], sizeof(bitmap.netcode) - 1);
							if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
								fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
								return -1;
							}
						}
						break;
					default:
						fprintf(stdout, _("Unknown bitmap type.\n"));
						break;
					}
					if (argc > 1) {
						if (SaveBitmapFileOnConsole(argv[1], &bitmap) != GE_NONE) return (-1);
					}
				} else {
					fprintf(stdout, _("Your phone doesn't have logo uploaded !\n"));
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


static int setlogo(int argc, char *argv[])
{
	GSM_Bitmap bitmap, oldbit;
	GSM_NetworkInfo NetworkInfo;
	GSM_Error error = GE_NOTSUPPORTED;
	GSM_Information *info = &State.Phone.Info;
	GSM_Data data;

	bool ok = true;
	int i;

	GSM_DataClear(&data);
	data.Bitmap = &bitmap;
	data.NetworkInfo = &NetworkInfo;

	memset(&bitmap.text, 0, sizeof(bitmap.text));

	if (!strcmp(argv[0], "text") || !strcmp(argv[0], "dealer")) {
		if (!strcmp(argv[0], "text")) bitmap.type = GSM_WelcomeNoteText;
		else bitmap.type = GSM_DealerNoteText;
		if (argc > 1) strncpy(bitmap.text, argv[1], sizeof(bitmap.text) - 1);
	} else {
		if (!strcmp(argv[0], "op") || !strcmp(argv[0], "startup") || !strcmp(argv[0], "caller")) {
			if (argc > 1) {
				if (ReadBitmapFileOnConsole(argv[1], &bitmap) != GE_NONE)
					return(-1);

				if (!strcmp(argv[0], "op")) {
					memset(&bitmap.netcode, 0, sizeof(bitmap.netcode));
					if (bitmap.type != GSM_OperatorLogo || argc < 3) {
						if (SM_Functions(GOP_GetNetworkInfo, &data, &State) == GE_NONE) strncpy(bitmap.netcode, NetworkInfo.NetworkCode, sizeof(bitmap.netcode) - 1);
					}
					GSM_ResizeBitmap(&bitmap, GSM_OperatorLogo, info);
					if (argc == 3) {
						strncpy(bitmap.netcode, argv[2], sizeof(bitmap.netcode) - 1);
						if (!strcmp(GSM_GetNetworkName(bitmap.netcode), "unknown")) {
							fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
							return -1;
						}
					}
				}
				if (!strcmp(argv[0], "startup")) {
					GSM_ResizeBitmap(&bitmap, GSM_StartupLogo, info);
				}
				if (!strcmp(argv[0],"caller")) {
					GSM_ResizeBitmap(&bitmap, GSM_CallerLogo, info);
					if (argc > 2) {
						bitmap.number = argv[2][0] - '0';
						if ((bitmap.number < 0) || (bitmap.number > 9)) bitmap.number = 0;
					} else {
						bitmap.number = 0;
					}
					oldbit.type = GSM_CallerLogo;
					oldbit.number = bitmap.number;
					data.Bitmap = &oldbit;
					if (SM_Functions(GOP_GetBitmap, &data, &State) == GE_NONE) {
						/* We have to get the old name and ringtone!! */
						bitmap.ringtone = oldbit.ringtone;
						strncpy(bitmap.text, oldbit.text, sizeof(bitmap.text) - 1);
					}
					if (argc > 3) strncpy(bitmap.text, argv[3], sizeof(bitmap.text) - 1);
				}
				fprintf(stdout, _("Setting Logo.\n"));
			} else {
				/* FIXME: is it possible to permanently remove op logo ? */
				if (!strcmp(argv[0], "op")) {
					bitmap.type = GSM_OperatorLogo;
					strcpy(bitmap.netcode, "000 00");
					bitmap.width = 72;
					bitmap.height = 14;
					bitmap.size = bitmap.width * bitmap.height / 8;
					GSM_ClearBitmap(&bitmap);
				}
				/* FIXME: how to remove startup and group logos ? */
				fprintf(stdout, _("Removing Logo.\n"));
			}
		} else {
			fprintf(stderr, _("What kind of logo do you want to set ?\n"));
			return -1;
		}
	}

	data.Bitmap = &bitmap;
	error = SM_Functions(GOP_SetBitmap, &data, &State);

	switch (error) {
	case GE_NONE:
		oldbit.type = bitmap.type;
		oldbit.number = bitmap.number;
		data.Bitmap = &oldbit;
		if (SM_Functions(GOP_GetBitmap, &data, &State) == GE_NONE) {
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

					strcpy(oldbit.text, "!");
					data.Bitmap = &oldbit;
					SM_Functions(GOP_SetBitmap, &data, &State);
					SM_Functions(GOP_GetBitmap, &data, &State);
					if (oldbit.text[0] != '!') {
						fprintf(stderr, _("SIM card and PIN is required\n"));
					} else {
						data.Bitmap = &bitmap;
						SM_Functions(GOP_SetBitmap, &data, &State);
						data.Bitmap = &oldbit;
						SM_Functions(GOP_GetBitmap, &data, &State);
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

	return 0;
}


static int viewlogo(char *filename)
{
	GSM_Error error;

	error = GSM_ShowBitmapFile(filename);
	return 0;
}

/* Calendar notes receiving. */
static int getcalendarnote(int argc, char *argv[])
{
	GSM_CalendarNote	CalendarNote;
	GSM_Data		data;
	GSM_Error		error = GE_NONE;
	int			i, first_location, last_location;
	bool			vCal = false;

	struct option options[] = {
		{ "vCard",   optional_argument, NULL, '1'},
		{ NULL,      0,                 NULL, 0}
	};

	optarg = NULL;
	optind = 0;

	first_location = last_location = atoi(argv[0]);
	if ((argc > 1) && (argv[1][0] != '-')) {
		last_location = atoi(argv[1]);
	}

	while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
		switch (i) {
		case 'v':
			vCal = true;
			break;
		default:
			usage(); /* Would be better to have an calendar_usage() here. */
			return -1;
		}
	}

	for (i = first_location; i <= last_location; i++) {
		CalendarNote.Location = i;

		GSM_DataClear(&data);
		data.CalendarNote = &CalendarNote;

		error = SM_Functions(GOP_GetCalendarNote, &data, &State);
		switch (error) {
		case GE_NONE:
			if (vCal) {
				fprintf(stdout, "BEGIN:VCALENDAR\r\n");
				fprintf(stdout, "VERSION:1.0\r\n");
				fprintf(stdout, "BEGIN:VEVENT\r\n");
				fprintf(stdout, "CATEGORIES:");
				switch (CalendarNote.Type) {
				case GCN_REMINDER:
					fprintf(stdout, "MISCELLANEOUS\r\n");
					break;
				case GCN_CALL:
					fprintf(stdout, "PHONE CALL\r\n");
					break;
				case GCN_MEETING:
					fprintf(stdout, "MEETING\r\n");
					break;
				case GCN_BIRTHDAY:
					fprintf(stdout, "SPECIAL OCCASION\r\n");
					break;
				default:
					fprintf(stdout, "UNKNOWN\r\n");
					break;
				}
				fprintf(stdout, "SUMMARY:%s\r\n",CalendarNote.Text);
				fprintf(stdout, "DTSTART:%04d%02d%02dT%02d%02d%02d\r\n", CalendarNote.Time.Year,
					CalendarNote.Time.Month, CalendarNote.Time.Day, CalendarNote.Time.Hour,
					CalendarNote.Time.Minute, CalendarNote.Time.Second);
				if (CalendarNote.Alarm.Year != 0) {
					fprintf(stdout, "DALARM:%04d%02d%02dT%02d%02d%02d\r\n", CalendarNote.Alarm.Year,
						CalendarNote.Alarm.Month, CalendarNote.Alarm.Day, CalendarNote.Alarm.Hour,
						CalendarNote.Alarm.Minute, CalendarNote.Alarm.Second);
				}
				fprintf(stdout, "END:VEVENT\r\n");
				fprintf(stdout, "END:VCALENDAR\r\n");

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

				if (CalendarNote.Alarm.AlarmEnabled == 1) {
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
			break;
		case GE_NOTIMPLEMENTED:
			fprintf(stderr, _("Function not implemented.\n"));
			break;
		default:
			fprintf(stderr, _("The calendar note can not be read\n"));
			break;
		}
	}

	return error;
}

/* Writing calendar notes. */
static int writecalendarnote(char *argv[])
{
	GSM_CalendarNote CalendarNote;
	GSM_Data data;

	GSM_DataClear(&data);
	data.CalendarNote = &CalendarNote;

	if (GSM_ReadVCalendarFile(argv[0], &CalendarNote, atoi(argv[1]))) {
		fprintf(stdout, _("Failed to load vCalendar file.\n"));
		return -1;
	}

	/* Error 22 = Calendar full ;-) */
	if (SM_Functions(GOP_WriteCalendarNote, &data, &State) == GE_NONE)
		fprintf(stdout, _("Succesfully written!\n"));
	else {
		fprintf(stdout, _("Failed to write calendar note!\n"));
		return -1;
	}

	return 0;
}

/* Calendar note deleting. */
static int deletecalendarnote(int argc, char *argv[])
{
	GSM_CalendarNote CalendarNote;
	int i, first_location, last_location;
	GSM_Data data;

	GSM_DataClear(&data);
	data.CalendarNote = &CalendarNote;

	first_location = last_location = atoi(argv[0]);
	if (argc > 1) last_location = atoi(argv[1]);

	for (i = first_location; i <= last_location; i++) {

		CalendarNote.Location = i;

		if (SM_Functions(GOP_DeleteCalendarNote, &data, &State) == GE_NONE) {
			fprintf(stdout, _("   Calendar note deleted.\n"));
		} else {
			fprintf(stderr, _("The calendar note cannot be deleted\n"));
		}

	}

	return 0;
}

/* Setting the date and time. */
static int setdatetime(int argc, char *argv[])
{
	struct tm *now;
	time_t nowh;
	GSM_DateTime Date;
	GSM_Error error;

	nowh = time(NULL);
	now = localtime(&nowh);

	Date.Year = now->tm_year;
	Date.Month = now->tm_mon+1;
	Date.Day = now->tm_mday;
	Date.Hour = now->tm_hour;
	Date.Minute = now->tm_min;
	Date.Second = now->tm_sec;

	if (argc > 0) Date.Year = atoi(argv[0]);
	if (argc > 1) Date.Month = atoi(argv[1]);
	if (argc > 2) Date.Day = atoi(argv[2]);
	if (argc > 3) Date.Hour = atoi (argv[3]);
	if (argc > 4) Date.Minute = atoi(argv[4]);

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

	GSM_DataClear(&data);
	data.DateTime = &Date;
	error = SM_Functions(GOP_SetDateTime, &data, &State);

	return error;
}

/* In this mode we receive the date and time from mobile phone. */
static int getdatetime(void)
{
	GSM_Data	data;
	GSM_DateTime	date_time;
	GSM_Error	error;

	GSM_DataClear(&data);
	data.DateTime = &date_time;

	error = SM_Functions(GOP_GetDateTime, &data, &State);

	switch (error) {
	case GE_NONE:
		fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.Year, date_time.Month, date_time.Day);
		fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.Hour, date_time.Minute, date_time.Second);
		break;
	case GE_NOTIMPLEMENTED:
		fprintf(stdout, _("Function not implemented in %s !\n"), model);
		break;
	default:
		fprintf(stdout, _("Internal error\n"));
		break;
	}

	return error;
}

/* Setting the alarm. */
static int setalarm(int argc, char *argv[])
{
	GSM_DateTime Date;
	GSM_Error error;

	if (argc == 2) {
		Date.Hour = atoi(argv[0]);
		Date.Minute = atoi(argv[1]);
		Date.Second = 0;
		Date.AlarmEnabled = true;
	} else {
		Date.Hour = 0;
		Date.Minute = 0;
		Date.Second = 0;
		Date.AlarmEnabled = false;
	}

	GSM_DataClear(&data);
	data.DateTime = &Date;

	error = SM_Functions(GOP_SetAlarm, &data, &State);

	return 0;
}

/* Getting the alarm. */
static int getalarm(void)
{
	GSM_Error	error;
	GSM_Data	data;
	GSM_DateTime	date_time;

	GSM_DataClear(&data);
	data.DateTime = &date_time;

	error = SM_Functions(GOP_GetAlarm, &data, &State);

	switch (error) {
	case GE_NONE:
		fprintf(stdout, _("Alarm: %s\n"), (date_time.AlarmEnabled==0)?"off":"on");
		fprintf(stdout, _("Time: %02d:%02d\n"), date_time.Hour, date_time.Minute);
		break;
	case GE_NOTIMPLEMENTED:
		fprintf(stdout, _("Function not implemented in %s !\n"), model);
		break;
	default:
		fprintf(stdout, _("Internal error\n"));
		break;
	}

	return error;
}

/* In monitor mode we don't do much, we just initialise the fbus code.
   Note that the fbus code no longer has an internal monitor mode switch,
   instead compile with DEBUG enabled to get all the gumpf. */
static int monitormode(void)
{
	float rflevel = -1, batterylevel = -1;
	GSM_PowerSource powersource = -1;
	GSM_RFUnits rf_units = GRF_Arbitrary;
	GSM_BatteryUnits batt_units = GBU_Arbitrary;
	GSM_Data data;

	GSM_NetworkInfo NetworkInfo;
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

	GSM_SMSMemoryStatus SMSStatus = {0, 0};

//	char Number[20];

	GSM_DataClear(&data);

	/* We do not want to monitor serial line forever - press Ctrl+C to stop the
	   monitoring mode. */
	signal(SIGINT, interrupted);

	fprintf(stderr, _("Entering monitor mode...\n"));

	//sleep(1);
	//GSM->EnableCellBroadcast();

	/* Loop here indefinitely - allows you to see messages from GSM code in
	   response to unknown messages etc. The loops ends after pressing the
	   Ctrl+C. */
	data.RFUnits = &rf_units;
	data.RFLevel = &rflevel;
	data.BatteryUnits = &batt_units;
	data.BatteryLevel = &batterylevel;
	data.PowerSource = &powersource;
	data.SMSStatus = &SMSStatus;
	data.NetworkInfo = &NetworkInfo;

	while (!bshutdown) {
		if (SM_Functions(GOP_GetRFLevel, &data, &State) == GE_NONE)
			fprintf(stdout, _("RFLevel: %d\n"), (int)rflevel);

		if (SM_Functions(GOP_GetBatteryLevel, &data, &State) == GE_NONE)
			fprintf(stdout, _("Battery: %d\n"), (int)batterylevel);

		if (SM_Functions(GOP_GetPowersource, &data, &State) == GE_NONE)
			fprintf(stdout, _("Power Source: %s\n"), (powersource == GPS_ACDC) ? _("AC/DC") : _("battery"));

		data.MemoryStatus = &SIMMemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("SIM: Used %d, Free %d\n"), SIMMemoryStatus.Used, SIMMemoryStatus.Free);

		data.MemoryStatus = &PhoneMemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("Phone: Used %d, Free %d\n"), PhoneMemoryStatus.Used, PhoneMemoryStatus.Free);

		data.MemoryStatus = &DC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("DC: Used %d, Free %d\n"), DC_MemoryStatus.Used, DC_MemoryStatus.Free);

		data.MemoryStatus = &EN_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("EN: Used %d, Free %d\n"), EN_MemoryStatus.Used, EN_MemoryStatus.Free);

		data.MemoryStatus = &FD_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("FD: Used %d, Free %d\n"), FD_MemoryStatus.Used, FD_MemoryStatus.Free);

		data.MemoryStatus = &LD_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("LD: Used %d, Free %d\n"), LD_MemoryStatus.Used, LD_MemoryStatus.Free);

		data.MemoryStatus = &MC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("MC: Used %d, Free %d\n"), MC_MemoryStatus.Used, MC_MemoryStatus.Free);

		data.MemoryStatus = &ON_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("ON: Used %d, Free %d\n"), ON_MemoryStatus.Used, ON_MemoryStatus.Free);

		data.MemoryStatus = &RC_MemoryStatus;
		if (SM_Functions(GOP_GetMemoryStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("RC: Used %d, Free %d\n"), RC_MemoryStatus.Used, RC_MemoryStatus.Free);

		if (SM_Functions(GOP_GetSMSStatus, &data, &State) == GE_NONE)
			fprintf(stdout, _("SMS Messages: Unread %d, Number %d\n"), SMSStatus.Unread, SMSStatus.Number);

//		if (GSM->GetIncomingCallNr(Number) == GE_NONE)
//			fprintf(stdout, _("Incoming call: %s\n"), Number);

		if (SM_Functions(GOP_GetNetworkInfo, &data, &State) == GE_NONE)
			fprintf(stdout, _("Network: %s (%s), LAC: %02x%02x, CellID: %02x%02x\n"), GSM_GetNetworkName (NetworkInfo.NetworkCode), GSM_GetCountryName(NetworkInfo.NetworkCode), NetworkInfo.LAC[0], NetworkInfo.LAC[1], NetworkInfo.CellID[0], NetworkInfo.CellID[1]);

//		if (GSM->ReadCellBroadcast(&CBMessage) == GE_NONE)
//			fprintf(stdout, _("Cell broadcast received on channel %d: %s\n"), CBMessage.Channel, CBMessage.Message);

		sleep(1);
	}

	fprintf(stderr, _("Leaving monitor mode...\n"));

	return 0;
}


static void  PrintDisplayStatus(int Status)
{
	fprintf(stdout, _("Call in progress: %-3s\n"),
		Status & (1 << DS_Call_In_Progress) ? _("on") : _("off"));
	fprintf(stdout, _("Unknown: %-3s\n"),
		Status & (1 << DS_Unknown)          ? _("on") : _("off"));
	fprintf(stdout, _("Unread SMS: %-3s\n"),
		Status & (1 << DS_Unread_SMS)       ? _("on") : _("off"));
	fprintf(stdout, _("Voice call: %-3s\n"),
		Status & (1 << DS_Voice_Call)       ? _("on") : _("off"));
	fprintf(stdout, _("Fax call active: %-3s\n"),
		Status & (1 << DS_Fax_Call)         ? _("on") : _("off"));
	fprintf(stdout, _("Data call active: %-3s\n"),
		Status & (1 << DS_Data_Call)        ? _("on") : _("off"));
	fprintf(stdout, _("Keyboard lock: %-3s\n"),
		Status & (1 << DS_Keyboard_Lock)    ? _("on") : _("off"));
	fprintf(stdout, _("SMS storage full: %-3s\n"),
		Status & (1 << DS_SMS_Storage_Full) ? _("on") : _("off"));
}

#define ESC "\e"

static void NewOutputFn(GSM_DrawMessage *DrawMessage)
{
	int x, y, n;
	static int status;
	static unsigned char screen[DRAW_MAX_SCREEN_HEIGHT][DRAW_MAX_SCREEN_WIDTH];
	static bool init = false;

	if (!init) {
		for (y = 0; y < DRAW_MAX_SCREEN_HEIGHT; y++)
			for (x = 0; x < DRAW_MAX_SCREEN_WIDTH; x++)
				screen[y][x] = ' ';
		status = 0;
		init = true;
	}

	printf(ESC "[1;1H");

	switch (DrawMessage->Command) {
	case GSM_Draw_ClearScreen:
		for (y = 0; y < DRAW_MAX_SCREEN_HEIGHT; y++)
			for (x = 0; x < DRAW_MAX_SCREEN_WIDTH; x++)
				screen[y][x] = ' ';
		break;

	case GSM_Draw_DisplayText:
		x = DrawMessage->Data.DisplayText.x*DRAW_MAX_SCREEN_WIDTH / 84;
		y = DrawMessage->Data.DisplayText.y*DRAW_MAX_SCREEN_HEIGHT / 48;
		n = strlen(DrawMessage->Data.DisplayText.text);
		if (n > DRAW_MAX_SCREEN_WIDTH)
			return;
		if (x + n > DRAW_MAX_SCREEN_WIDTH)
			x = DRAW_MAX_SCREEN_WIDTH - n;
		if (y > DRAW_MAX_SCREEN_HEIGHT)
			y = DRAW_MAX_SCREEN_HEIGHT - 1;
		memcpy(&screen[y][x], DrawMessage->Data.DisplayText.text, n);
		break;

	case GSM_Draw_DisplayStatus:
		status = DrawMessage->Data.DisplayStatus;
		break;

	default:
		return;
	}

	for (y = 0; y < DRAW_MAX_SCREEN_HEIGHT; y++) {
		for (x = 0; x < DRAW_MAX_SCREEN_WIDTH; x++)
			fprintf(stdout, "%c", screen[y][x]);
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
	PrintDisplayStatus(status);
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
	GSM_Data data;
	GSM_Error error;
	GSM_DisplayOutput output;

	GSM_DataClear(&data);
	memset(&output, 0, sizeof(output));
	output.OutputFn = NewOutputFn;
	data.DisplayOutput = &output;

	error = SM_Functions(GOP_DisplayOutput, &data, &State);
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
//				fprintf(stderr, "handling keys (%d).\n", strlen(buf));
//				if (GSM && GSM->HandleString && GSM->HandleString(buf) != GE_NONE)
//					fprintf(stdout, _("Key press simulation failed.\n"));
				memset(buf, 0, 102);
			}
			SM_Loop(&State, 1);
			SM_Functions(GOP_PollDisplay, &data, &State);
		}
		fprintf (stderr, "Shutting down\n");

		fprintf (stderr, _("Leaving display monitor mode...\n"));

		output.OutputFn = NULL;
		error = SM_Functions(GOP_DisplayOutput, &data, &State);
		if (error != GE_NONE)
			fprintf (stderr, _("Error!\n"));
	} else
		fprintf (stderr, _("Error!\n"));

	return 0;
}

/* Reads profile from phone and displays its' settings */
static int getprofile(int argc, char *argv[])
{
	int max_profiles;
	int start, stop, i;
	GSM_Profile p;
	GSM_Error error = GE_NOTSUPPORTED;
	GSM_Data data;

	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[GSM_MAX_MODEL_LENGTH];
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
			usage(); /* FIXME */
			return -1;
		}
	}

	GSM_DataClear(&data);
	data.Model = model;
	while (SM_Functions(GOP_GetModel, &data, &State) != GE_NONE)
		sleep(1);

	p.Number = 0;
	GSM_DataClear(&data);
	data.Profile = &p;
	error = SM_Functions(GOP_GetProfile, &data, &State);

	switch (error) {
	case GE_NONE:
		break;
	case GE_NOTIMPLEMENTED:
		fprintf(stderr, _("Function not implemented in %s model!\n"), model);
		return -1;
	default:
		fprintf(stderr, _("Unspecified error\n"));
		return -1;
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
			fprintf(stderr, _("Profile number must be value from 0 to %d!\n"), max_profiles-1);
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

	GSM_DataClear(&data);
	data.Profile = &p;

	for (i = start; i <= stop; i++) {
		p.Number = i;

		if (p.Number != 0) {
			error = SM_Functions(GOP_GetProfile, &data, &State);
			if (error != GE_NONE ) {
				fprintf(stderr, _("Cannot get profile %d\n"), i);
				return -1;
			}
		}

		if (raw) {
			fprintf(stdout, "%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
				p.Number, p.Name, p.DefaultName, p.KeypadTone,
				p.Lights, p.CallAlert, p.Ringtone, p.Volume,
				p.MessageTone, p.Vibration, p.WarningTone,
				p.CallerGroups, p.AutomaticAnswer);
		} else {
			fprintf(stdout, "%d. \"%s\"\n", p.Number, p.Name);
			if (p.DefaultName == -1) fprintf(stdout, _(" (name defined)\n"));

			fprintf(stdout, _("Incoming call alert: %s\n"), GetProfileCallAlertString(p.CallAlert));

			/* For different phones different ringtones names */

			if (!strcmp(model, "NSE-3"))
				fprintf(stdout, _("Ringing tone: %s (%d)\n"), RingingTones[p.Ringtone], p.Ringtone);
			else
				fprintf(stdout, _("Ringtone number: %d\n"), p.Ringtone);

			fprintf(stdout, _("Ringing volume: %s\n"), GetProfileVolumeString(p.Volume));

			fprintf(stdout, _("Message alert tone: %s\n"), GetProfileMessageToneString(p.MessageTone));

			fprintf(stdout, _("Keypad tones: %s\n"), GetProfileKeypadToneString(p.KeypadTone));

			fprintf(stdout, _("Warning and game tones: %s\n"), GetProfileWarningToneString(p.WarningTone));

			/* FIXME: Light settings is only used for Car */
			if (p.Number == (max_profiles - 2)) fprintf(stdout, _("Lights: %s\n"), p.Lights ? _("On") : _("Automatic"));

			fprintf(stdout, _("Vibration: %s\n"), GetProfileVibrationString(p.Vibration));

			/* FIXME: it will be nice to add here reading caller group name. */
			if (max_profiles != 3) fprintf(stdout, _("Caller groups: 0x%02x\n"), p.CallerGroups);

			/* FIXME: Automatic answer is only used for Car and Headset. */
			if (p.Number >= (max_profiles - 2)) fprintf(stdout, _("Automatic answer: %s\n"), p.AutomaticAnswer ? _("On") : _("Off"));

			fprintf(stdout, "\n");
		}
	}

	return 0;
}

/* Writes profiles to phone */
static int setprofile()
{
	int n;
	GSM_Profile p;
	GSM_Error error = GE_NONE;
	GSM_Data data;
	char line[256], ch;

	GSM_DataClear(&data);
	data.Profile = &p;

	while (fgets(line, sizeof(line), stdin)) {
		n = strlen(line);
		if (n > 0 && line[n-1] == '\n') {
			line[--n] = 0;
		}

		n = sscanf(line, "%d;%39[^;];%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d%c",
			    &p.Number, p.Name, &p.DefaultName, &p.KeypadTone,
			    &p.Lights, &p.CallAlert, &p.Ringtone, &p.Volume,
			    &p.MessageTone, &p.Vibration, &p.WarningTone,
			    &p.CallerGroups, &p.AutomaticAnswer, &ch);
		if (n != 13) {
			fprintf(stderr, _("Input line format isn't valid\n"));
			return GE_UNKNOWN;
		}

		error = SM_Functions(GOP_SetProfile, &data, &State);
		if (error != GE_NONE) {
			fprintf(stderr, _("Cannot set profile %d\n"), p.Number);
			return error;
		}
	}

	return error;
}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */
static int getmemory(int argc, char *argv[])
{
	GSM_PhonebookEntry entry;
	int count, start_entry, end_entry = 0;
	GSM_Error error;
	char *memory_type_string;
	bool all = false;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = argv[0];
	entry.MemoryType = StrToMemoryType(memory_type_string);
	if (entry.MemoryType == GMT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, ...)!\n"), argv[0]);
		return (-1);
	}

	start_entry = atoi(argv[1]);
	if (argc > 2) {
		if (argv[2] && (strlen(argv[2]) == 3) && !strcmp(argv[2], "end")) {
			all = true;
		} else end_entry = atoi(argv[2]);
	}
	else end_entry = start_entry;

	/* Now retrieve the requested entries. */
	count = start_entry;
	while (all || count <= end_entry) {
		entry.Location = count;

		data.PhonebookEntry = &entry;
		error = SM_Functions(GOP_ReadPhonebook, &data, &State);

		switch (error) {
			int i;
		case GE_NONE:
			fprintf(stdout, "%s;%s;%s;%d;%d", entry.Name, entry.Number, memory_type_string, entry.Location, entry.Group);
			for (i = 0; i < entry.SubEntriesCount; i++) {
				fprintf(stdout, ";%d;%d;%d;%s", entry.SubEntries[i].EntryType, entry.SubEntries[i].NumberType,
								entry.SubEntries[i].BlockNumber, entry.SubEntries[i].data.Number);
			}
			fprintf(stdout, "\n");
			if (entry.MemoryType == GMT_MC || entry.MemoryType == GMT_DC || entry.MemoryType == GMT_RC)
				fprintf(stdout, "%02u.%02u.%04u %02u:%02u:%02u\n", entry.Date.Day, entry.Date.Month, entry.Date.Year, entry.Date.Hour, entry.Date.Minute, entry.Date.Second);
			break;
		case GE_NOTIMPLEMENTED:
			fprintf(stderr, _("Function not implemented in %s model!\n"), model);
			return -1;
		case GE_INVALIDMEMORYTYPE:
			fprintf(stderr, _("Memory type %s not supported!\n"), memory_type_string);
			return -1;
		case GE_INVALIDPHBOOKLOCATION:
			fprintf(stderr, _("%s|%d|Bad location or other error!(%d)\n"), memory_type_string, count, error);
			if (all) {
				/* Ensure that we quit the loop */
				all = false;
				count = 0;
				end_entry = -1;
			}
			break;
		case GE_EMPTYMEMORYLOCATION:
			fprintf(stderr, "%d. empty phonebook entry\n", count);
			break;
		default:
			fprintf(stderr, "Unknown error: %s\n", print_error(error));
			return -1;
		}
		count++;
	}
	return 0;
}

/* Read data from stdin, parse and write to phone.  The parsing is relatively
   crude and doesn't allow for much variation from the stipulated format. */
/* FIXME: I guess there's *very* similar code in xgnokii */
static int writephonebook(int argc, char *args[])
{
	GSM_PhonebookEntry entry;
	GSM_Error error = GE_NOTSUPPORTED;
	char *memory_type_string;
	int line_count = 0;
	int subentry;

	char *Line, OLine[100], BackLine[100];
	char *ptr;

	/* Check argument */
	if (argc && (strcmp("-i", args[0])))
		usage();

	Line = OLine;

	memset(&entry, 0, sizeof(GSM_PhonebookEntry));

	/* Go through data from stdin. */
	while (GetLine(stdin, Line, 99)) {
		strcpy(BackLine, Line);
		line_count++;

		ptr = strtok(Line, ";");
		if (ptr) strncpy(entry.Name, ptr, sizeof(entry.Name) - 1);

		ptr = strtok(NULL, ";");
		if (ptr) strncpy(entry.Number, ptr, sizeof(entry.Number) - 1);

		ptr = strtok(NULL, ";");

		if (!ptr) {
			fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, BackLine);
			Line = OLine;
			continue;
		}

		if (!strncmp(ptr, "ME", 2)) {
			memory_type_string = "int";
			entry.MemoryType = GMT_ME;
		} else {
			if (!strncmp(ptr, "SM", 2)) {
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

			if (ptr && *ptr != 0)
				entry.SubEntries[subentry].EntryType = atoi(ptr);
			else
				break;

			ptr = strtok(NULL, ";");
			if (ptr) entry.SubEntries[subentry].NumberType = atoi(ptr);

			/* Phone Numbers need to have a number type. */
			if (!ptr && entry.SubEntries[subentry].EntryType == GSM_Number) {
				fprintf(stderr, _("Missing phone number type on line %d"
						  " entry %d [%s]\n"), line_count, subentry, BackLine);
				subentry--;
				break;
			}

			ptr = strtok(NULL, ";");
			if (ptr) entry.SubEntries[subentry].BlockNumber = atoi(ptr);

			ptr = strtok(NULL, ";");

			/* 0x13 Date Type; it is only for Dailed Numbers, etc.
			   we don't store to this memories so it's an error to use it. */
			if (!ptr || entry.SubEntries[subentry].EntryType == GSM_Date) {
				fprintf(stdout, _("There is no phone number on line %d entry %d [%s]\n"),
					line_count, subentry, BackLine);
				subentry--;
				break;
			} else
				strncpy(entry.SubEntries[subentry].data.Number, ptr, sizeof(entry.SubEntries[subentry].data.Number) - 1);
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
	      		data.PhonebookEntry = &aux;
			error = SM_Functions(GOP_ReadPhonebook, &data, &State);

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
				return 0;
			}
		}

		/* Do write and report success/failure. */
		data.PhonebookEntry = &entry;
		error = SM_Functions(GOP_WritePhonebook, &data, &State);

		if (error == GE_NONE)
			fprintf (stdout, _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), memory_type_string, entry.Location, entry.Name, entry.Number);
		else
			fprintf (stdout, _("Write FAILED (%s): memory type: %s, loc: %d, name: %s, number: %s\n"), print_error(error), memory_type_string, entry.Location, entry.Name, entry.Number);

	}

	return 0;
}

/* Getting speed dials. */
static int getspeeddial(char *Number)
{
	GSM_SpeedDial	SpeedDial;
	GSM_Data	data;
	GSM_Error	error;

	SpeedDial.Number = atoi(Number);

	GSM_DataClear(&data);
	data.SpeedDial = &SpeedDial;

	error = SM_Functions(GOP_GetSpeedDial, &data, &State);

	switch (error) {
	case GE_NONE:
		fprintf(stdout, _("SpeedDial nr. %d: %d:%d\n"), SpeedDial.Number, SpeedDial.MemoryType, SpeedDial.Location);
		break;
	case GE_NOTIMPLEMENTED:
		fprintf(stdout, _("Function not implemented in %s !\n"), model);
		break;
	default:
		fprintf(stdout, _("Internal error\n"));
		break;
	}

	return error;
}

/* Setting speed dials. */
static int setspeeddial(char *argv[])
{
	GSM_SpeedDial entry;
	GSM_Data data;
	GSM_Error error;
	char *memory_type_string;

	GSM_DataClear(&data);
	data.SpeedDial = &entry;

	/* Handle command line args that set type, start and end locations. */

	if (strcmp(argv[1], "ME") == 0) {
		entry.MemoryType = GMT_ME;
		memory_type_string = "ME";
	} else if (strcmp(argv[1], "SM") == 0) {
		entry.MemoryType = GMT_SM;
		memory_type_string = "SM";
	} else {
		fprintf(stderr, _("Unknown memory type %s!\n"), argv[1]);
		return -1;
	}

	entry.Number = atoi(argv[0]);
	entry.Location = atoi(argv[2]);

	if ((error = SM_Functions(GOP_SetSpeedDial, &data, &State)) == GE_NONE )
		fprintf(stdout, _("Succesfully written!\n"));

	return error;
}

/* Getting the status of the display. */
static int getdisplaystatus(void)
{
	GSM_Error error = GE_INTERNALERROR;
	int Status;
	GSM_Data data;

	GSM_DataClear(&data);
	data.DisplayStatus = &Status;

	error = SM_Functions(GOP_GetDisplayStatus, &data, &State);
	if (error == GE_NONE) PrintDisplayStatus(Status);

	return error;
}

static int netmonitor(char *Mode)
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

	memset(&Screen, 0, sizeof(Screen));
//	if (GSM && GSM->NetMonitor) GSM->NetMonitor(mode, Screen);

	if (Screen) fprintf(stdout, "%s\n", Screen);

	return 0;
}

static int identify(void)
{
	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char imei[64], model[64], rev[64], manufacturer[64];

	manufacturer[0] = model[0] = rev[0] = imei[0] = 0;

	data.Manufacturer = manufacturer;
	data.Model = model;
	data.Revision = rev;
	data.Imei = imei;

	/* Retrying is bad idea: what if function is simply not implemented?
	   Anyway let's wait 2 seconds for the right packet from the phone. */
	sleep(2);

	strcpy(imei, _("(unknown)"));
	strcpy(manufacturer, _("(unknown)"));
	strcpy(model, _("(unknown)"));
	strcpy(rev, _("(unknown)"));

	SM_Functions(GOP_Identify, &data, &State);

	fprintf(stdout, _("IMEI:     %s\n"), imei);
	fprintf(stdout, _("Manufacturer: %s\n"), manufacturer);
	fprintf(stdout, _("Model:    %s\n"), model);
	fprintf(stdout, _("Revision: %s\n"), rev);

	return 0;
}

static int senddtmf(char *String)
{
/*	if (GSM && GSM->SendDTMF) GSM->SendDTMF(String);*/
	return 0;
}

/* Resets the phone */
static int reset(char *type)
{
	unsigned char _type = 0x03;

	if (type) {
		if (!strcmp(type, "soft"))
			_type = 0x03;
		else
			if (!strcmp(type, "hard")) {
				_type = 0x04;
			} else {
				fprintf(stderr, _("What kind of reset do you want??\n"));
				return -1;
			}
	}

/*	if (GSM && GSM->Reset) GSM->Reset(_type); */

	return 0;
}

/* pmon allows fbus code to run in a passive state - it doesn't worry about
   whether comms are established with the phone.  A debugging/development
   tool. */
static int pmon(void)
{
	GSM_Error error;
	GSM_ConnectionType connection = GCT_Serial;

	/* Initialise the code for the GSM interface. */
	error = GSM_Initialise(model, Port, Initlength, connection, NULL, &State);

	if (error != GE_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
		return -1;
	}

	while (1) {
		usleep(50000);
	}

	return 0;
}

static int sendringtone(int argc, char *argv[])
{
	GSM_Ringtone ringtone;
	GSM_Error error = GE_NOTSUPPORTED;

	if (GSM_ReadRingtoneFile(argv[0], &ringtone)) {
		fprintf(stdout, _("Failed to load ringtone.\n"));
		return(-1);
	}

/*	if (GSM && GSM->SendRingtone) error = GSM->SendRingtone(&ringtone,argv[1]); */

	if (error == GE_NONE)
		fprintf(stdout, _("Send succeeded!\n"));
	else
		fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

	return 0;
}


static int setringtone(int argc, char *argv[])
{
	GSM_Ringtone ringtone;
	GSM_Error error = GE_NOTSUPPORTED;

	if (GSM_ReadRingtoneFile(argv[0], &ringtone)) {
		fprintf(stdout, _("Failed to load ringtone.\n"));
		return(-1);
	}

/*	if (GSM && GSM->SetRingtone) error = GSM->SetRingtone(&ringtone);*/

	if (error == GE_NONE)
		fprintf(stdout, _("Send succeeded!\n"));
	else
		fprintf(stdout, _("Send failed\n"));

	return 0;
}

static int presskeysequence(void)
{
	char buf[105];

	console_raw();

	memset(&buf[0], 0, 102);
	while (read(0, buf, 100) > 0) {
		fprintf(stderr, "handling keys (%d).\n", strlen(buf));
/*		if (GSM && GSM->HandleString && GSM->HandleString(buf) != GE_NONE)
			fprintf(stdout, _("Key press simulation failed.\n"));*/
		memset(buf, 0, 102);
	}

	return 0;
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
	GSM_CallDivert cd;
	GSM_Data data;
	GSM_Error error;
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

	memset(&cd, 0, sizeof(GSM_CallDivert));

	while ((opt = getopt_long(argc, argv, "o:t:n:c:m:", options, NULL)) != -1) {
		switch (opt) {
		case 'o':
			if (!strcmp("register", optarg)) {
				cd.Operation = GSM_CDV_Register;
			} else if (!strcmp("enable", optarg)) {
				cd.Operation = GSM_CDV_Enable;
			} else if (!strcmp("disable", optarg)) {
				cd.Operation = GSM_CDV_Disable;
			} else if (!strcmp("erasure", optarg)) {
				cd.Operation = GSM_CDV_Erasure;
			} else if (!strcmp("query", optarg)) {
				cd.Operation = GSM_CDV_Query;
			} else {
				usage();
				return -1;
			}
			break;
		case 't':
			if (!strcmp("all", optarg)) {
				cd.DType = GSM_CDV_AllTypes;
			} else if (!strcmp("busy", optarg)) {
				cd.DType = GSM_CDV_Busy;
			} else if (!strcmp("noans", optarg)) {
				cd.DType = GSM_CDV_NoAnswer;
			} else if (!strcmp("outofreach", optarg)) {
				cd.DType = GSM_CDV_OutOfReach;
			} else if (!strcmp("notavail", optarg)) {
				cd.DType = GSM_CDV_NotAvailable;
			} else {
				usage();
				return -1;
			}
			break;
		case 'c':
			if (!strcmp("all", optarg)) {
				cd.CType = GSM_CDV_AllCalls;
			} else if (!strcmp("voice", optarg)) {
				cd.CType = GSM_CDV_VoiceCalls;
			} else if (!strcmp("fax", optarg)) {
				cd.CType = GSM_CDV_FaxCalls;
			} else if (!strcmp("data", optarg)) {
				cd.CType = GSM_CDV_DataCalls;
			} else {
				usage();
				return -1;
			}
			break;
		case 'm':
			cd.Timeout = atoi(optarg);
			break;
		case 'n':
			strncpy(cd.Number.number, optarg, sizeof(cd.Number.number) - 1);
			if (cd.Number.number[0] == '+') cd.Number.type = SMS_International;
			else cd.Number.type = SMS_Unknown;
			break;
		default:
			usage();
			return -1;
		}
	}
	data.CallDivert = &cd;
	error = SM_Functions(GOP_CallDivert, &data, &State);

	if (error == GE_NONE) {
		fprintf(stderr, "Divert succeeded.\n");
	} else {
		fprintf(stderr, "%s\n", print_error(error));
	}
	return 0;
}

static GSM_Error smsslave(GSM_SMSMessage *message)
{
	FILE *output;
	char *s = message->UserData[0].u.Text;
	char buf[10240];
	int i = message->Number;
	int i1, i2, msgno, msgpart;
	static int unknown = 0;
	char c;

	while (*s == 'W')
		s++;
	fprintf(stderr, _("Got message %d: %s\n"), i, s);
	if ((sscanf(s, "%d/%d:%d-%c-", &i1, &i2, &msgno, &c) == 4) && (c == 'X'))
		sprintf(buf, "/tmp/sms/mail_%d_", msgno);
	else if (sscanf(s, "%d/%d:%d-%d-", &i1, &i2, &msgno, &msgpart) == 4)
		sprintf(buf, "/tmp/sms/mail_%d_%03d", msgno, msgpart);
	else	sprintf(buf, "/tmp/sms/unknown_%d_%d", getpid(), unknown++);
	if ((output = fopen(buf, "r"))) {
		fprintf(stderr, _("### Exists?!\n"));
		fclose(output);
	}
	output = fopen(buf, "w+");
	if (strstr(buf, "unknown"))
		fprintf(output, "%s", message->UserData[0].u.Text);
	else {
		s = message->UserData[0].u.Text;
		while (!(*s == '-'))
			s++;
		s++;
		while (!(*s == '-'))
			s++;
		s++;
		fprintf(output, "%s", s);
	}
	fclose(output);
	return GE_NONE;
}

static int smsreader(void)
{
	GSM_Data data;
	GSM_Error error;

	data.OnSMS = smsslave;
	error = SM_Functions(GOP_OnSMS, &data, &State);
	if (error == GE_NONE) {
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);
		fprintf(stderr, _("Entered sms reader mode...\n"));

		while (!bshutdown) {
			SM_Loop(&State, 1);
			/* Some phones may not be able to notify us, thus we give
			   lowlevel chance to poll them */
			error = SM_Functions(GOP_PollSMS, &data, &State);
		}
		fprintf(stderr, _("Shutting down\n"));

		fprintf(stderr, _("Exiting sms reader mode...\n"));
		data.OnSMS = NULL;

		error = SM_Functions(GOP_OnSMS, &data, &State);
		if (error != GE_NONE)
			fprintf(stderr, _("Error!\n"));
	} else
		fprintf(stderr, _("Error!\n"));

	return 0;
}

/* This is a "convenience" function to allow quick test of new API stuff which
   doesn't warrant a "proper" command line function. */
#ifndef WIN32
static int foogle(char *argv[])
{
	/* Fill in what you would like to test here... */
	SM_Functions(GOP_OnSMS, &data, &State);
	return 0;
}
#endif

static void  gnokii_error_logger(const char *fmt, va_list ap) {
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
		fprintf(stderr, "HOME variable missing\n");
		return -1;
#endif
	}

	snprintf(logname, sizeof(logname), "%s/%s", home, file);

	if ((logfile = fopen(logname, "a")) == NULL) {
		perror("fopen");
		return -1;
	}

	GSM_ELogHandler = gnokii_error_logger;

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
	static struct option long_options[] =
	{
		/* FIXME: these comments are nice, but they would be more usefull as docs for the user */
		/* Display usage. */
		{ "help",               no_argument,       NULL, OPT_HELP },

		/* Display version and build information. */
		{ "version",            no_argument,       NULL, OPT_VERSION },

		/* Monitor mode */
		{ "monitor",            no_argument,       NULL, OPT_MONITOR },

#ifdef SECURITY

		/* Enter Security Code mode */
		{ "entersecuritycode",  required_argument, NULL, OPT_ENTERSECURITYCODE },

		/* Get Security Code status */
		{ "getsecuritycodestatus",  no_argument,   NULL, OPT_GETSECURITYCODESTATUS },

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

		/* Get calendar note mode */
		{ "getcalendarnote",    required_argument, NULL, OPT_GETCALENDARNOTE },

		/* Write calendar note mode */
		{ "writecalendarnote",  required_argument, NULL, OPT_WRITECALENDARNOTE },

		/* Delete calendar note mode */
		{ "deletecalendarnote", required_argument, NULL, OPT_DELCALENDARNOTE },

		/* Get display status mode */
		{ "getdisplaystatus",   no_argument,       NULL, OPT_GETDISPLAYSTATUS },

		/* Get memory mode */
		{ "getmemory",          required_argument, NULL, OPT_GETMEMORY },

		/* Write phonebook (memory) mode */
		{ "writephonebook",     optional_argument, NULL, OPT_WRITEPHONEBOOK },

		/* Get speed dial mode */
		{ "getspeeddial",       required_argument, NULL, OPT_GETSPEEDDIAL },

		/* Set speed dial mode */
		{ "setspeeddial",       required_argument, NULL, OPT_SETSPEEDDIAL },

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

		/* Set ringtone */
		{ "setringtone",        required_argument, NULL, OPT_SETRINGTONE },

		/* Get SMS center number mode */
		{ "getsmsc",            required_argument, NULL, OPT_GETSMSC },

		/* For development purposes: run in passive monitoring mode */
		{ "pmon",               no_argument,       NULL, OPT_PMON },

		/* NetMonitor mode */
		{ "netmonitor",         required_argument, NULL, OPT_NETMONITOR },

		/* Identify */
		{ "identify",           no_argument,       NULL, OPT_IDENTIFY },

		/* Send DTMF sequence */
		{ "senddtmf",           required_argument, NULL, OPT_SENDDTMF },

		/* Resets the phone */
		{ "reset",              optional_argument, NULL, OPT_RESET },

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

		/* Show texts from phone's display */
		{ "displayoutput",      no_argument,       NULL, OPT_DISPLAYOUTPUT },

		/* Simulate pressing the keys */
		{ "keysequence",        no_argument,       NULL, OPT_KEYPRESS },

		/* Divert calls */
		{ "divert",		required_argument, NULL, OPT_DIVERT },

		/* SMS reader */
		{ "smsreader",          no_argument,       NULL, OPT_SMSREADER },

		/* For development purposes: insert you function calls here */
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
		{ OPT_SETALARM,          0, 2, 0 },
		{ OPT_DIALVOICE,         1, 1, 0 },
		{ OPT_GETCALENDARNOTE,   1, 3, 0 },
		{ OPT_WRITECALENDARNOTE, 2, 2, 0 },
		{ OPT_DELCALENDARNOTE,   1, 2, 0 },
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
		{ OPT_GETPROFILE,        0, 3, 0 },
		{ OPT_SETPROFILE,        0, 0, 0 },
		{ OPT_WRITEPHONEBOOK,    0, 1, 0 },
		{ OPT_DIVERT,            6, 10, 0 },

		{ 0, 0, 0, 0 },
	};

	if (install_log_handler()) {
		fprintf(stderr, "WARNING: cannot open logfile, logs will be directed to stderr\n");
	}

	opterr = 0;

	/* For GNU gettext */
#ifdef USE_NLS
	textdomain("gnokii");
	setlocale(LC_ALL, "");
#endif

	/* Introduce yourself */
	short_version();

	/* Read config file */
	if (readconfig(&model, &Port, &Initlength, &Connection, &BinDir) < 0) {
		exit(-1);
	}

	/* Handle command line arguments. */
	c = getopt_long(argc, argv, "", long_options, NULL);
	if (c == -1) 		/* No argument given - we should display usage. */
		usage();

	switch(c) {
	/* First, error conditions */
	case '?':
	case ':':
		fprintf(stderr, _("Use '%s --help' for usage informations.\n"), argv[0]);
		exit(0);
	/* Then, options with no arguments */
	case OPT_HELP:
		usage();
	case OPT_VERSION:
		version();
		exit(0);
	}

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
			usage();
		}

#ifdef __svr4__
		/* have to ignore SIGALARM */
		sigignore(SIGALRM);
#endif

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
		case OPT_GETCALENDARNOTE:
			rc = getcalendarnote(nargc, nargv);
			break;
		case OPT_DELCALENDARNOTE:
			rc = deletecalendarnote(nargc, nargv);
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
			rc = getprofile(argc, argv);
			break;
		case OPT_SETPROFILE:
			rc = setprofile();
			break;
		case OPT_DISPLAYOUTPUT:
			rc = displayoutput();
			break;
		case OPT_KEYPRESS:
			rc = presskeysequence();
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
#ifndef WIN32
		case OPT_FOOGLE:
			rc = foogle(nargv);
			break;
#endif
		default:
			fprintf(stderr, _("Unknown option: %d\n"), c);
			break;

		}
		unlock_device(lockfile);
		exit(rc);
	}

	fprintf(stderr, _("Wrong number of arguments\n"));
	unlock_device(lockfile);
	exit(-1);
}
