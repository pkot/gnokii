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

  Mainline code for gnokii utility. SMS handling code.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#if defined(WIN32) && !defined(CYGWIN)
#  include <process.h>
#  define getpid _getpid
#endif

#include <stdio.h>
#include <sys/stat.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "gnokii-app.h"
#include "gnokii.h"

/* Outputs summary of all SMS gnokii commands */
void sms_usage(FILE *f)
{
	fprintf(f, _("SMS options:\n"
		     "          --sendsms destination [--smsc message_center_number |\n"
		     "                 --smscno message_center_index] [-r|--report] [-8|--8bit]\n"
		     "                 [-C|--class n] [-v|--validity n]\n"
		     "                 [-i|--imelody] [-a|--animation file;file;file;file]\n"
		     "                 [-o|--concat this;total;serial] [-w|--wappush url]\n"
		     "          --savesms [--sender from] [--smsc message_center_number |\n"
		     "                 --smscno message_center_index] [-f|--folder folder_id]\n"
		     "                 [-l|--location number] [-s|--sent|-r|--read] [-d|--deliver]\n"
		     "                 [-t|--datetime YYMMDDHHMMSS]\n"
		     "          --getsms memory_type start [end] [-f|--file file]\n"
		     "                 [-F|--force-file file] [-a|--append-file file]\n"
		     "                 [-d|--delete]\n"
		     "          --deletesms memory_type start [end]\n"
		     "          --getsmsc [start_number [end_number]] [-r|--raw]\n"
		     "          --setsmsc\n"
		     "          --createsmsfolder name\n"
		     "          --deletesmsfolder folderid\n"
		     "          --showsmsfolderstatus\n"
		     "          --smsreader\n"));
}

/* Displays usage of --sendsms command */
int sendsms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --sendsms destination              recipient number (msisdn)\n"
			"        --smsc message_center_number       number (msisdn) of the message\n"
			"                                           center\n"
			"        --smscno message_center_index      index of the message center stored\n"
			"                                           in the phone memory\n"
			"        -r\n"
			"        --report                           request delivery report\n"
			"        -v n\n"
			"        --validity n                       set message validity\n"
			"        -C n\n"
			"        --class n                          set message class; possible values\n"
			"                                           are 0, 1, 2, or 3\n"
			"        -8\n"
			"        --8bit                             set 8bit coding\n"
			"        -i\n"
			"        --imelody                          send imelody message, text is read\n"
			"                                           from stdin\n"
			"        -a file;file;file;file\n"
			"        --animation file;file;file;file    send animation message\n"
			"        -o this;total;serial\n"
			"        --concat this;total;serial         send 'this' part of all 'total' parts\n"
			"                                           identified by 'serial'\n"
			"        -w url\n"
			"        --wappush url                      send wappush to the given url\n"
			"Message text is read from standard input.\n"
			"\n"
		));
	return exitval;
}

gn_gsm_number_type get_number_type(const char *number)
{
	gn_gsm_number_type type;

	if (!number)
		return GN_GSM_NUMBER_Unknown;
	if (*number == '+') {
		type = GN_GSM_NUMBER_International;
		number++;
	} else {
		type = GN_GSM_NUMBER_Unknown;
	}
	while (*number) {
		if (!isdigit(*number))
			return GN_GSM_NUMBER_Alphanumeric;
		number++;
	}
	return type;
}

/* Send SMS messages. */
gn_error sendsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_sms sms;
	gn_error error;
	/* The maximum length of an uncompressed concatenated short message is
	   255 * 153 = 39015 default alphabet characters */
	int i, curpos = 0;

	struct option options[] = {
		{ "smsc",      required_argument, NULL, '1' },
		{ "smscno",    required_argument, NULL, '2' },
		{ "long",      required_argument, NULL, 'l' },
		{ "report",    no_argument,       NULL, 'r' },
		{ "validity",  required_argument, NULL, 'v' },
		{ "class",     required_argument, NULL, 'C' },
		{ "8bit",      no_argument,       NULL, '8' },
		{ "imelody",   no_argument,       NULL, 'i' },
		{ "animation", required_argument, NULL, 'a' },
		{ "concat",    required_argument, NULL, 'o' },
		{ "wappush",   required_argument, NULL, 'w' },
		{ NULL,        0,                 NULL, 0 }
	};

	/* The memory is zeroed here */
	gn_sms_default_submit(&sms);

	snprintf(sms.remote.number, sizeof(sms.remote.number), "%s", optarg);
	sms.remote.type = get_number_type(sms.remote.number);
	if (sms.remote.type == GN_GSM_NUMBER_Alphanumeric) {
		dprintf("Invalid phone number\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

	while ((i = getopt_long(argc, argv, "l:rv:C:8ia:o:w:", options, NULL)) != -1) {
		switch (i) {       /* -c for compression. not yet implemented. */
		case '1': /* SMSC number */
			snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", optarg);
			sms.smsc.type = get_number_type(sms.smsc.number);
			break;

		case '2': /* SMSC number index in phone memory */
			data->message_center = calloc(1, sizeof(gn_sms_message_center));
			data->message_center->id = gnokii_atoi(optarg);
			if (errno || data->message_center->id < 1 || data->message_center->id > 5) {
				free(data->message_center);
				return sendsms_usage(stderr, -1);
			}
			if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
				snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", data->message_center->smsc.number);
				sms.smsc.type = data->message_center->smsc.type;
			}
			free(data->message_center);
			break;

		case 'l': /* we send long message */
			/* ignored. Left for compatibility */
			break;

		case 'a': /* Animation */ {
			char buf[10240];
			char *s = buf, *t;
			snprintf(buf, sizeof(buf), "%s", optarg);
			sms.user_data[curpos].type = GN_SMS_DATA_Animation;
			for (i = 0; i < 4; i++) {
				t = strchr(s, ';');
				if (t)
					*t++ = 0;
				loadbitmap(&sms.user_data[curpos].u.animation[i], s, i ? GN_BMP_EMSAnimation2 : GN_BMP_EMSAnimation, state);
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
			case '0':
				sms.dcs.u.general.m_class = 1;
				break;
			case '1':
				sms.dcs.u.general.m_class = 2;
				break;
			case '2':
				sms.dcs.u.general.m_class = 3;
				break;
			case '3':
				sms.dcs.u.general.m_class = 4;
				break;
			default:
				return sendsms_usage(stderr, -1);
			}
			break;

		case 'v':
			sms.validity = gnokii_atoi(optarg);
			if (errno || sms.validity < 0)
				return sendsms_usage(stderr, -1);
			break;

		case '8':
			sms.dcs.u.general.alphabet = GN_SMS_DCS_8bit;
			break;

		case 'i':
			sms.user_data[0].type = GN_SMS_DATA_iMelody;
			sms.user_data[1].type = GN_SMS_DATA_None;
			error = readtext(&sms.user_data[0]);
			if (error != GN_ERR_NONE)
				return error;
			if (sms.user_data[0].length < 1) {
				fprintf(stderr, _("Empty message. Quitting.\n"));
				return GN_ERR_INVALIDSIZE;
			}
			curpos = -1;
			break;
		case 'w': {
			gn_wap_push wp;
			int chars_read;
			char message_buffer[GN_SMS_MAX_LENGTH];

			if (!optarg || strlen(optarg) > 255) {
				fprintf(stderr, _("URL is too long (max 255 chars). Quitting.\n"));
				return GN_ERR_INVALIDSIZE;
			}

			sms.user_data[curpos].type = GN_SMS_DATA_WAPPush;

			gn_wap_push_init(&wp);

			memset(message_buffer, 0, sizeof(message_buffer));
			fprintf(stderr, _("Please enter SMS text. End your input with <cr><control-D>:\n"));
			chars_read = fread(message_buffer, 1, sizeof(message_buffer), stdin);

			wp.url = optarg;
			wp.text = message_buffer;

			if (gn_wap_push_encode(&wp) != GN_ERR_NONE) {
			    fprintf(stderr, _("WAP Push encoding failed!\n"));
			    return GN_ERR_FAILED;
			}

			memcpy(sms.user_data[curpos].u.text, wp.data, wp.data_len);

			sms.user_data[curpos].length = wp.data_len;

			free(wp.data);

			sms.user_data[++curpos].type = GN_SMS_DATA_None;

			curpos = -1;
			break;
		}
		default:
			return sendsms_usage(stderr, -1);
		}
	}
	if (argc > optind) {
		/* There are too many arguments that don't start with '-' */
		return sendsms_usage(stderr, -1);
	}

	if (!sms.smsc.number[0]) {
		data->message_center = calloc(1, sizeof(gn_sms_message_center));
		data->message_center->id = 1;
		if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
			snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", data->message_center->smsc.number);
			sms.smsc.type = data->message_center->smsc.type;
		}
		free(data->message_center);
	}
	/* Either GN_OP_GetSMSCenter failed or it succeded and read an empty number */
	if (!sms.smsc.number[0]) {
		fprintf(stderr, _("Cannot read the SMSC number from your phone. If the sms send will fail, please use --smsc option explicitely giving the number.\n"));
	}
	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	if (curpos != -1) {
		error = readtext(&sms.user_data[curpos]);
		if (error != GN_ERR_NONE)
			return error;
		sms.user_data[curpos].type = GN_SMS_DATA_Text;
		if ((sms.dcs.u.general.alphabet != GN_SMS_DCS_8bit)
		    && !gn_char_def_alphabet(sms.user_data[curpos].u.text))
			sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;
		sms.user_data[++curpos].type = GN_SMS_DATA_None;
	}

	data->sms = &sms;

	/* Send the message. */
	error = gn_sms_send(data, state);

	if (error == GN_ERR_NONE) {
		if (sms.parts > 1) {
			int j;
			fprintf(stderr, _("Message sent in %d parts with reference numbers:"), sms.parts);
			for (j = 0; j < sms.parts; j++)
				fprintf(stderr, " %d", sms.reference[j]);
			fprintf(stderr, "\n");
		} else
			fprintf(stderr, _("Send succeeded with reference %d!\n"), sms.reference[0]);
	} else
		fprintf(stderr, _("SMS Send failed (%s)\n"), gn_error_print(error));

	free(sms.reference);
	return error;
}

/* Displays usage of --savesms command */
int savesms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --savesms\n"
			"        --smsc message_center_number    number (msisdn) of the message\n"
			"                                        center\n"
			"        --smscno message_center_index   index of the message center stored\n"
			"                                        in the phone memory\n"
			"        --sender msisdn                 set sender number\n"
			"        -l n\n"
			"        --location n                    save message at location n\n"
			"        -r\n"
			"        --read                          mark message as read\n"
			"        -s\n"
			"        --sent                          mark message as sent\n"
			"        -f ID\n"
			"        --folder ID                     save message to the folder ID where\n"
			"                                        is of type IN, OU...\n"
			"        -d\n"
			"        --deliver                       set message type as sms deliver\n"
			"                                        (to be sent)\n"
			"        -t YYMMDDHHMISS\n"
			"        --datetime YYMMDDHHMISS         set date and time of message delivery\n"
			"Message text is read from standard input.\n"
			"\n"
		));
	return exitval;
}

gn_error savesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_sms sms;
	gn_error error = GN_ERR_INTERNALERROR;
	int i;
	char tmp[3];
#if 0
	int confirm = -1;
	int interactive = 0;
	char ans[8];
#endif
	struct option options[] = {
		{ "smsc",     required_argument, NULL, '0' },
		{ "smscno",   required_argument, NULL, '1' },
		{ "sender",   required_argument, NULL, '2' },
		{ "location", required_argument, NULL, 'l' },
		{ "read",     0,                 NULL, 'r' },
		{ "sent",     0,                 NULL, 's' },
		{ "folder",   required_argument, NULL, 'f' },
		{ "deliver",  0                , NULL, 'd' },
		{ "datetime", required_argument, NULL, 't' },
		{ NULL,       0,                 NULL, 0 }
	};

	char memory_type[20] = "";
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

	/* nokia 7110 will choke if no number is present when we
	 * try to store a SMS on the phone. maybe others do too
	 * TODO should this be handled here? report an error instead
	 * of setting a default? */
	strcpy(sms.remote.number, "0");
	sms.remote.type = GN_GSM_NUMBER_International;
	sms.number = 0;

	/* Option parsing */
	while ((i = getopt_long(argc, argv, "l:rsf:dt:", options, NULL)) != -1) {
		switch (i) {
		case '0': /* SMSC number */
			snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", optarg);
			sms.smsc.type = get_number_type(sms.smsc.number);
			break;
		case '1': /* SMSC number index in phone memory */
			data->message_center = calloc(1, sizeof(gn_sms_message_center));
			data->message_center->id = gnokii_atoi(optarg);
			if (errno || data->message_center->id < 1 || data->message_center->id > 5) {
				free(data->message_center);
				return savesms_usage(stderr, -1);
			}
			if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
				snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", data->message_center->smsc.number);
				sms.smsc.type = data->message_center->smsc.type;
			}
			free(data->message_center);
			break;
		case '2': /* sender number */
			snprintf(sms.remote.number, GN_BCD_STRING_MAX_LENGTH, "%s", optarg);
			sms.remote.type = get_number_type(sms.remote.number);
			break;
		case 'l': /* location to write to */
			sms.number = gnokii_atoi(optarg);
			if (errno || sms.number < 0)
				return savesms_usage(stderr, -1);
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
				fprintf(stderr, _("Unknown memory type %s (use ME, SM, IN, OU, ...)!\n"), optarg);
				fprintf(stderr, _("Run gnokii --showsmsfolderstatus for a list of supported memory types.\n"));
				return GN_ERR_INVALIDMEMORYTYPE;
			}
			break;
		case 'd': /* type Deliver */
			sms.type = GN_SMS_MT_Deliver;
			break;
		case 't': /* set specific date and time of message delivery */
			if (strlen(optarg) != 12) {
				fprintf(stderr, _("Invalid datetime format: %s (should be YYMMDDHHMISS, all digits)!\n"), optarg);
				return GN_ERR_WRONGDATAFORMAT;
			}
			for (i = 0; i < 12; i++)
				if (!isdigit(optarg[i])) {
					fprintf(stderr, _("Invalid datetime format: %s (should be YYMMDDHHMISS, all digits)!\n"), optarg);
					return GN_ERR_WRONGDATAFORMAT;
				}
			snprintf(tmp, sizeof(tmp), "%s", optarg);
			sms.smsc_time.year	= atoi(tmp) + 1900;
			snprintf(tmp, sizeof(tmp), "%s", optarg+2);
			sms.smsc_time.month	= atoi(tmp);
			snprintf(tmp, sizeof(tmp), "%s", optarg+4);
			sms.smsc_time.day	= atoi(tmp);
			snprintf(tmp, sizeof(tmp), "%s", optarg+6);
			sms.smsc_time.hour	= atoi(tmp);
			snprintf(tmp, sizeof(tmp), "%s", optarg+8);
			sms.smsc_time.minute	= atoi(tmp);
			snprintf(tmp, sizeof(tmp), "%s", optarg+10);
			sms.smsc_time.second	= atoi(tmp);
			if (!gn_timestamp_isvalid(sms.smsc_time)) {
				fprintf(stderr, _("Invalid datetime: %s.\n"), optarg);
				return GN_ERR_WRONGDATAFORMAT;
			}
			break;
		default:
			return savesms_usage(stderr, -1);
		}
	}
	if (argc > optind) {
		/* There are too many arguments that don't start with '-' */
		return savesms_usage(stderr, -1);
	}
#if 0
	if (interactive) {
		gn_sms aux;

		aux.number = sms.number;
		data->sms = &aux;
		error = gn_sm_functions(GN_OP_GetSMSnoValidate, data, state);
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
		data->message_center = calloc(1, sizeof(gn_sms_message_center));
		data->message_center->id = 1;
		if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
			snprintf(sms.smsc.number, GN_BCD_STRING_MAX_LENGTH, "%s", data->message_center->smsc.number);
			dprintf("SMSC number: %s\n", sms.smsc.number);
			sms.smsc.type = data->message_center->smsc.type;
		}
		free(data->message_center);
	}

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	error = readtext(&sms.user_data[0]);
	if (error != GN_ERR_NONE)
		return error;
	if (sms.user_data[0].length < 1) {
		fprintf(stderr, _("Empty message. Quitting.\n"));
		return GN_ERR_WRONGDATAFORMAT;
	}
	if (memory_type[0] != '\0')
		sms.memory_type = gn_str2memory_type(memory_type);

	sms.user_data[0].type = GN_SMS_DATA_Text;
	sms.user_data[1].type = GN_SMS_DATA_None;
	if (!gn_char_def_alphabet(sms.user_data[0].u.text))
		sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;

	fprintf(stderr, _("Storing SMS... "));

	data->sms = &sms;
	error = gn_sms_save(data, state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Saved to %d!\n"), sms.number);
	else
		fprintf(stderr, _("Saving failed (%s)\n"), gn_error_print(error));

	return error;
}

/* Displays usage of --getsms command */
int getsms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getsms memory start [end]  retrieves messages from memory type (SM, ME,\n"
			"                                    IN, OU, ...) starting from\n"
			"                                    location start and ending with end;\n"
			"                                    if end option is omitted, just one entry\n"
			"                                    is read;\n"
			"                                    start must be a number, end may be a number\n"
			"                                    or 'end' string;\n"
			"                                    if 'end' is used entries are being read\n"
			"                                    until empty location\n"
			"        --force-file filename\n"
			"        -F filename                 save sms to the file overwriting it if\n"
			"                                    the file already exists;\n"
			"                                    file is in mbox format\n"
			"        --file filename\n"
			"        -f filename                 as above but user is prompted before\n"
			"                                    overwriting the file\n"
			"        --append-file filename\n"
			"        -a filename                 as above but append to the existing file\n"
			"        --delete\n"
			"        -d                          delete message after reading\n"
			"\n"
		));
	return exitval;
}

/* Get SMS messages. */
gn_error getsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	int i, del = 0;
	gn_sms_folder folder;
	gn_sms_folder_list folderlist;
	gn_sms message;
	char *memory_type_string;
	int start_message, end_message, count, mode = 1;
	int folder_count = -1;
	int messages_read = 0;
	char filename[64];
	gn_error error = GN_ERR_NONE;
	gn_bmp bitmap;
	gn_phone *phone = &state->driver.phone;
	char ans[5];
	struct stat buf;
	char *message_text;
	bool cont = true;
	bool all = false;

	struct option options[] = {
		{ "delete",     no_argument,       NULL, 'd' },
		{ "file",       required_argument, NULL, 'f' },
		{ "force-file", required_argument, NULL, 'F' },
		{ "append-file",required_argument, NULL, 'a' },
		{ NULL,         0,                 NULL, 0 }
	};

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = optarg;
	if (gn_str2memory_type(memory_type_string) == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, IN, OU, ...)!\n"), optarg);
		fprintf(stderr, _("Run gnokii --showsmsfolderstatus for a list of supported memory types.\n"));
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	start_message = gnokii_atoi(argv[optind]);
	if (errno || start_message < 0)
		return getsms_usage(stderr, -1);
	end_message = parse_end_value_option(argc, argv, optind + 1, start_message);
	if (errno || end_message < 0)
		return getsms_usage(stderr, -1);

	/* Let's determine number of messages in given folder */
	if (end_message == INT_MAX) {
		gn_error e;
		int j;

		all = true;
		memset(&folderlist, 0, sizeof(folderlist));
		gn_data_clear(data);
		data->sms_folder_list = &folderlist;

		e = gn_sm_functions(GN_OP_GetSMSFolders, data, state);

		if (e == GN_ERR_NONE) {
			for (j = 0; j < folderlist.number; j++) {
				data->sms_folder = folderlist.folder + j;
				if (folderlist.folder_id[j] == gn_str2memory_type(memory_type_string)) {
					e = gn_sm_functions(GN_OP_GetSMSFolderStatus, data, state);
					if (e == GN_ERR_NONE) {
						folder_count = folderlist.folder[j].number;
						dprintf("Folder %s (%s) has %u messages\n", folderlist.folder[j].name, gn_memory_type2str(folderlist.folder_id[j]), folderlist.folder[j].number);
					}
				}
			}
		}
	}

	*filename = '\0';
	/* parse all options (beginning with '-') */
	while ((i = getopt_long(argc, argv, "f:F:a:d", options, NULL)) != -1) {
		switch (i) {
		case 'd':
			del = 1;
			dprintf("del\n");
			break;
		/* append mode */
		case 'a':
			mode = 2;
			goto parsefile;
		/* force mode -- don't ask to overwrite */
		case 'F':
			mode = 0;
		case 'f':
parsefile:
			if (optarg) {
				snprintf(filename, sizeof(filename), "%s", optarg);
				fprintf(stderr, _("Saving into %s\n"), filename);
			} else
				return getsms_usage(stderr, -1);
			break;
		default:
			return getsms_usage(stderr, -1);
		}
	}
	if (argc - optind > 3) {
		/* There are too many arguments that don't start with '-' */
		return getsms_usage(stderr, -1);
	}

	folder.folder_id = 0;
	data->sms_folder = &folder;
	data->sms_folder_list = &folderlist;
	count = start_message;

	/* Now retrieve the requested entries. */
	while (cont) {
		bool done = false;
		int offset = 0;

		memset(&message, 0, sizeof(gn_sms));
		message.memory_type = gn_str2memory_type(memory_type_string);
		message.number = count;
		data->sms = &message;

		dprintf("Getting message #%d from %s\n", count, memory_type_string);
		error = gn_sms_get(data, state);
		switch (error) {
		case GN_ERR_NONE:
			messages_read++;
			message_text = NULL;
			fprintf(stdout, _("%d. %s (%s)\n"), message.number, gn_sms_message_type2str(message.type), gn_sms_message_status2str(message.status));
			switch (message.type) {
			case GN_SMS_MT_StatusReport:
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
				fprintf(stdout, _("Delivery date/time: %02d/%02d/%04d %02d:%02d:%02d "), \
					message.time.day, message.time.month, message.time.year, \
					message.time.hour, message.time.minute, message.time.second);
				if (message.smsc_time.timezone) {
					if (message.time.timezone > 0)
						fprintf(stdout,_("+%02d00"), message.time.timezone);
					else
						fprintf(stdout,_("%02d00"), message.time.timezone);
				}
				fprintf(stdout, "\n");
				fprintf(stdout, _("Recipient: %s Msg Center: %s\n"), message.remote.number, message.smsc.number);
				fprintf(stdout, _("Text:\n"));
				message_text = message.user_data[0].u.text;
				break;
			case GN_SMS_MT_Picture:
			case GN_SMS_MT_PictureTemplate:
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
				fprintf(stdout, _("Bitmap:\n"));
				gn_bmp_print(&message.user_data[0].u.bitmap, stdout);
				fprintf(stdout, _("Text:\n"));
				message_text = message.user_data[1].u.text;
				break;
			default:
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
				switch (message.type) {
				case GN_SMS_MT_Submit:
				case GN_SMS_MT_SubmitSent:
					fprintf(stdout, _("Recipient: %s Msg Center: %s\n"), message.remote.number, message.smsc.number);
					break;
				default:
					fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.remote.number, message.smsc.number);
					break;
				}
				/* No UDH */
				if (!message.udh.number)
					message.udh.udh[0].type = GN_SMS_UDH_None;
				fprintf(stdout, _("%s:\n"), gn_sms_udh_type2str(message.udh.udh[0].type));
				switch (message.udh.udh[0].type) {
				case GN_SMS_UDH_None:
					break;
				case GN_SMS_UDH_OpLogo:
					fprintf(stdout, _("GSM operator logo for %s (%s) network.\n"), bitmap.netcode, gn_network_name_get(bitmap.netcode));
					if (!strcmp(message.remote.number, "+998000005") && !strcmp(message.smsc.number, "+886935074443"))
						fprintf(stdout, _("Saved by Logo Express\n"));
					if (!strcmp(message.remote.number, "+998000002") || !strcmp(message.remote.number, "+998000003"))
						fprintf(stdout, _("Saved by Operator Logo Uploader by Thomas Kessler\n"));
					offset = 3;
				case GN_SMS_UDH_CallerIDLogo:
					/* put bitmap into bitmap structure */
					gn_bmp_sms_read(GN_BMP_OperatorLogo, message.user_data[0].u.text + 2 + offset, message.user_data[0].u.text, &bitmap);
					gn_bmp_print(&bitmap, stdout);
					if (*filename) {
						error = GN_ERR_NONE;
						/* mode == 1, interactive mode
						 * mode == 0, overwrite mode
						 */
						if (mode && (stat(filename, &buf) == 0)) {
							fprintf(stdout, _("File %s exists.\n"), filename);
							fprintf(stdout, _("Overwrite? (yes/no) "));
							gn_line_get(stdin, ans, 4);
							if (!strcmp(ans, _("yes"))) {
								error = gn_file_bitmap_save(filename, &bitmap, phone);
							}
						} else
							error = gn_file_bitmap_save(filename, &bitmap, phone);
						if (error != GN_ERR_NONE)
							fprintf(stderr, _("Couldn't save logofile %s!\n"), filename);
					}
					done = true;
					break;
				case GN_SMS_UDH_Ringtone:
					done = true;
					break;
				case GN_SMS_UDH_ConcatenatedMessages:
					fprintf(stdout, _("Linked (%d/%d):\n"),
						message.udh.udh[0].u.concatenated_short_message.current_number,
						message.udh.udh[0].u.concatenated_short_message.maximum_number);
					break;
				case GN_SMS_UDH_WAPvCard:
				case GN_SMS_UDH_WAPvCalendar:
				case GN_SMS_UDH_WAPvCardSecure:
				case GN_SMS_UDH_WAPvCalendarSecure:
				default:
					break;
				}
				if (done)
					break;
				message_text = message.user_data[0].u.text;
				break;
			}
			if (message_text) {
				fprintf(stdout, "%s\n", message_text);
				if ((mode != -1) && *filename) {
					char buf[1024];
					char *mbox = gn_sms2mbox(&message, "gnokii");
					snprintf(buf, 1023, "%s", filename);
					mode = writefile(buf, mbox, mode);
					free(mbox);
				}
			}
			if (del && mode != -1) {
				data->sms = &message;
				if (GN_ERR_NONE != gn_sms_delete(data, state))
					fprintf(stderr, _("(delete failed)\n"));
				else
					fprintf(stderr, _("(message deleted)\n"));
			}
			break;
		default:
			fprintf(stderr, _("Getting SMS failed (location %d from %s memory)! (%s)\n"), count, memory_type_string, gn_error_print(error));
			if (error == GN_ERR_INVALIDMEMORYTYPE) {
				fprintf(stderr, _("Unknown memory type %s (use ME, SM, IN, OU, ...)!\n"), memory_type_string);
				fprintf(stderr, _("Run gnokii --showsmsfolderstatus for a list of supported memory types.\n"));
			}
			break;
		}
		if (mode == -1)
			cont = false;
		if (count >= end_message)
			cont = false;
		if ((folder_count > 0) && (messages_read >= (folder_count - start_message + 1)))
			cont = false;
		/* Avoid infinite loops */
		if (all && error != GN_ERR_NONE && error != GN_ERR_EMPTYLOCATION)
			cont = false;
		count++;
	}

	/* FIXME: We return the value of the last read.
	 * What should we return?
	 */
	return error;
}

/* Displays usage of --deletesms command */
int deletesms_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --deletesms memory start [end]  deletes messages from memory type\n"
			"                                       (SM, ME, IN, OU, ...) starting\n"
			"                                       from location start and ending with end;\n"
			"                                       if end option is omitted, just one entry\n"
			"                                       is removed;\n"
			"                                       if 'end' is used entries are being removed\n"
			"                                       until empty location\n"
			"\n"
		));
	return exitval;
}

/* Delete SMS messages. */
gn_error deletesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_sms message;
	gn_sms_folder folder;
	gn_sms_folder_list folderlist;
	char *memory_type_string;
	int start_message, end_message, count;
	gn_error error = GN_ERR_NONE;

	/* Handle command line args that set type, start and end locations. */
	memory_type_string = optarg;
	message.memory_type = gn_str2memory_type(memory_type_string);
	if (message.memory_type == GN_MT_XX) {
		fprintf(stderr, _("Unknown memory type %s (use ME, SM, IN, OU, ...)!\n"), optarg);
		fprintf(stderr, _("Run gnokii --showsmsfolderstatus for a list of supported memory types.\n"));
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	start_message = gnokii_atoi(argv[optind]);
	if (errno || start_message < 0)
		return deletesms_usage(stderr, -1);
	end_message = parse_end_value_option(argc, argv, optind + 1, start_message);
	if (errno || end_message < 0)
		return deletesms_usage(stderr, -1);

	/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count++) {
		message.number = count;
		data->sms = &message;
		data->sms_folder = &folder;
		data->sms_folder_list = &folderlist;
		error = gn_sms_delete(data, state);

		if (error == GN_ERR_NONE)
			fprintf(stderr, _("Deleted SMS (location %d from memory %s)\n"), count, memory_type_string);
		else {
			if ((error == GN_ERR_INVALIDLOCATION) && (end_message == INT_MAX) && (count > start_message))
				return GN_ERR_NONE;
			fprintf(stderr, _("Deleting SMS failed (location %d from %s memory)! (%s)\n"), count, memory_type_string, gn_error_print(error));
		}
	}

	/* FIXME: We return the value of the last read.
	 * What should we return?
	 */
	return error;
}

/* Displays usage of --getsmsc command */
int getsmsc_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getsmsc [start [end]]       reads message center details from\n"
			"                                     the phone starting with start location\n"
			"                                     and ending with end location;\n"
			"                                     no end parameter means to read every\n"
			"                                     location to the end;\n"
			"                                     no start parameter means to start with\n"
			"                                     the first location;\n"
			"-r\n"
			"--raw                                output in raw format (comma separated)\n"
			"                                     default is human readable form\n"
			"\n"
		));
	return exitval;
}

/* Get SMSC number */
gn_error getsmsc(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_sms_message_center message_center;
	gn_error error;
	int start, stop, i;
	bool raw = false;
	struct option options[] = {
		{ "raw",    no_argument, NULL, 'r'},
		{ NULL,     0,           NULL, 0}
	};

	while ((i = getopt_long(argc, argv, "r", options, NULL)) != -1) {
		switch (i) {
		case 'r':
			raw = true;
			break;
		default:
			return getsmsc_usage(stderr, -1);
		}
	}

	if (argc > optind + 2) {
		/* There are too many arguments that don't start with '-' */
		return getsmsc_usage(stderr, -1);
	}

	if (argc > optind) {
		start = gnokii_atoi(argv[optind]);
		if (errno || start < 0)
			return getsmsc_usage(stderr, -1);
		stop = parse_end_value_option(argc, argv, optind + 1, start);
		if (errno || stop < 0)
			return getsmsc_usage(stderr, -1);

		if (start < 1) {
			fprintf(stderr, _("SMS center: location number must be greater than 0\n"));
			return GN_ERR_INVALIDLOCATION;
		}
	} else {
		start = 1;
		stop = 5;	/* FIXME: determine it */
	}

	gn_data_clear(data);
	data->message_center = &message_center;

	for (i = start; i <= stop; i++) {
		memset(&message_center, 0, sizeof(message_center));
		message_center.id = i;

		error = gn_sm_functions(GN_OP_GetSMSCenter, data, state);
		switch (error) {
		case GN_ERR_NONE:
			break;
		default:
			/* ignore the error when reading until "end" and at least one entry has alreadly been read */
			if ((error == GN_ERR_INVALIDLOCATION) && (stop == INT_MAX) && (i > start))
				return GN_ERR_NONE;
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
			if (message_center.recipient.number[0])
				fprintf(stdout, _("Default recipient number is %s\n"), message_center.recipient.number);
			else
				fprintf(stdout, _("No default recipient number is defined\n"));
			fprintf(stdout, _("Messages sent as %s\n"), gn_sms_message_format2str(message_center.format));
			fprintf(stdout, _("Message validity is %s\n"), gn_sms_vp_time2str(message_center.validity));
		}
	}

	return GN_ERR_NONE;
}

/* Displays usage of --setsmsc command */
int setsmsc_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --setsmsc\n"
			"SMSC details are read from standard input.\n"
			"\n"
		));
	return exitval;
}

/* Set SMSC number */
gn_error setsmsc(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_message_center message_center;
	gn_error error;
	char line[256], ch;
	int n;

	gn_data_clear(data);
	data->message_center = &message_center;

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
			return GN_ERR_WRONGDATAFORMAT;
		}

		error = gn_sm_functions(GN_OP_SetSMSCenter, data, state);
		if (error != GN_ERR_NONE) {
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			return error;
		}
	}

	return GN_ERR_NONE;
}

/* Displays usage of --createsmsfolder command */
int createsmsfolder_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --createsmsfolder name     creates SMS folder with given name\n"
			"\n"
		));
	return exitval;
}

/* Creating SMS folder. */
gn_error createsmsfolder(char *name, gn_data *data, struct gn_statemachine *state)
{
	gn_sms_folder	folder;
	gn_error	error;

	gn_data_clear(data);

	snprintf(folder.name, GN_SMS_FOLDER_NAME_MAX_LENGTH, "%s", name);
	data->sms_folder = &folder;

	error = gn_sm_functions(GN_OP_CreateSMSFolder, data, state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	else
		fprintf(stderr, _("Folder with name: %s successfully created!\n"), folder.name);
	return error;
}

/* Displays usage of --deletesmsfolder command */
int deletesmsfolder_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --deletesmsfolder folderid    removes SMS folder with given id\n"
			"\n"
		));
	return exitval;
}

/* Deleting SMS folder. */
gn_error deletesmsfolder(char *number, gn_data *data, struct gn_statemachine *state)
{
	gn_sms_folder	folder;
	gn_error	error;

	gn_data_clear(data);

	folder.folder_id = gnokii_atoi(number);
	if (!errno && folder.folder_id > 0 && folder.folder_id <= GN_SMS_FOLDER_MAX_NUMBER)
		data->sms_folder = &folder;
	else {
		fprintf(stderr, _("Error: Number must be between 1 and %i!\n"), GN_SMS_FOLDER_MAX_NUMBER);
		return GN_ERR_INVALIDLOCATION;
	}

	error = gn_sm_functions(GN_OP_DeleteSMSFolder, data, state);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	else
		fprintf(stderr, _("Number %i of 'My Folders' successfully deleted!\n"), folder.folder_id);
	return error;
}

/* Displays usage of --showsmsfolderstatus command */
int showsmsfolderstatus_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --showsmsfolderstatus     shows summary of SMS folders\n"
			"\n"
		));
	return exitval;
}

gn_error showsmsfolderstatus(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_folder_list folders;
	gn_error error;
	int i;

	memset(&folders, 0, sizeof(folders));
	gn_data_clear(data);
	data->sms_folder_list = &folders;

	if ((error = gn_sm_functions(GN_OP_GetSMSFolders, data, state)) != GN_ERR_NONE) {
		fprintf(stderr, _("Cannot list available folders: %s\n"), gn_error_print(error));
		return error;
	}

	fprintf(stdout, _("No. Name                                         Id #Msg\n"));
	fprintf(stdout, _("========================================================\n"));
	for (i = 0; i < folders.number; i++) {
		data->sms_folder = folders.folder + i;
		if ((error = gn_sm_functions(GN_OP_GetSMSFolderStatus, data, state)) != GN_ERR_NONE) {
			fprintf(stderr, _("Cannot get status of folder \"%s\": %s\n"), folders.folder[i].name, gn_error_print(error));
			return error;
		}
		fprintf(stdout, _("%3d %-42s %4s %4u\n"), i, folders.folder[i].name, gn_memory_type2str(folders.folder_id[i]), folders.folder[i].number);
	}

	return GN_ERR_NONE;
}

/* SMS handler for --smsreader mode */
static gn_error smsslave(gn_sms *message, struct gn_statemachine *state, void *callback_data)
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
	snprintf(number, sizeof(number), "%s", p);
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
	else
		snprintf(buf, sizeof(buf), "%s/sms_%s_%d_%d", smsdir, number, getpid(), unknown++);
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

/* Displays usage of --smsreader command */
int smsreader_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --smsreader     periodically check sms on the phone and stores them\n"
			"                       in mbox format\n"
			"\n"
		));
	return exitval;
}

gn_error smsreader(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	data->on_sms = smsslave;
	data->callback_data = NULL;
	error = gn_sm_functions(GN_OP_OnSMS, data, state);
	if (!error) {
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);
		fprintf(stderr, _("Entered sms reader mode...\n"));

		while (!bshutdown) {
			gn_sm_loop(1, state);
			/* Some phones may not be able to notify us, thus we give
			   lowlevel chance to poll them */
			error = gn_sm_functions(GN_OP_PollSMS, data, state);
		}
		fprintf(stderr, _("Shutting down\n"));

		fprintf(stderr, _("Exiting sms reader mode...\n"));
		data->on_sms = NULL;

		error = gn_sm_functions(GN_OP_OnSMS, data, state);
		if (error != GN_ERR_NONE)
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
	} else
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));

	return error;
}

