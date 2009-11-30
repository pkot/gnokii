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

  Mainline code for gnokii utility. Logo handling functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#include <sys/stat.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>

#include "gnokii-app.h"
#include "gnokii.h"

void logo_usage(FILE *f)
{
	fprintf(f, _("Logo options:\n"
		     "          --sendlogo {caller|op|picture} destination logofile\n"
		     "                 [network_code]\n"
		     "          --setlogo op [logofile [network_code]]\n"
		     "          --setlogo startup [logofile]\n"
		     "          --setlogo caller [logofile [caller_group_number [group_name]]]\n"
		     "          --setlogo {dealer|text} [text]\n"
		     "          --getlogo op [logofile [network_code]]\n"
		     "          --getlogo startup [logofile [network_code]]\n"
		     "          --getlogo caller [caller_group_number [logofile\n"
		     "                 [network_code]]]\n"
		     "          --getlogo {dealer|text}\n"
		     "          --viewlogo logofile\n"
		     ));
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
gn_error sendlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_sms sms;
	gn_error error;
	int type;

	gn_sms_default_submit(&sms);

	/* The first argument is the type of the logo. */
	switch (type = set_bitmap_type(optarg)) {
	case GN_BMP_OperatorLogo:   fprintf(stderr, _("Sending operator logo.\n")); break;
	case GN_BMP_CallerLogo:     fprintf(stderr, _("Sending caller line identification logo.\n")); break;
	case GN_BMP_PictureMessage: fprintf(stderr, _("Sending Multipart Message: Picture Message.\n")); break;
	case GN_BMP_EMSPicture:     fprintf(stderr, _("Sending EMS-compliant Picture Message.\n")); break;
	case GN_BMP_EMSAnimation:   fprintf(stderr, _("Sending EMS-compliant Animation.\n")); break;
	default: 	            fprintf(stderr, _("You should specify what kind of logo to send!\n")); return -1;
	}

	sms.user_data[0].type = GN_SMS_DATA_Bitmap;

	/* The second argument is the destination, ie the phone number of recipient. */
	memset(&sms.remote.number, 0, sizeof(sms.remote.number));
	snprintf(sms.remote.number, sizeof(sms.remote.number), "%s", argv[optind]);
	if (sms.remote.number[0] == '+')
		sms.remote.type = GN_GSM_NUMBER_International;
	else
		sms.remote.type = GN_GSM_NUMBER_Unknown;

	if (loadbitmap(&sms.user_data[0].u.bitmap, argv[optind+1], type, state) != GN_ERR_NONE)
		return GN_ERR_FAILED;
	if (type != sms.user_data[0].u.bitmap.type) {
		fprintf(stderr, _("Cannot send logo: specified and loaded bitmap type differ!\n"));
		return GN_ERR_FAILED;
	}

	/* If we are sending op logo we can rewrite network code. */
	if ((sms.user_data[0].u.bitmap.type == GN_BMP_OperatorLogo) && (argc > optind + 2)) {
		/*
		 * The fourth argument, if present, is the Network code of the operator.
		 * Network code is in this format: "xxx yy".
		 */
		snprintf(sms.user_data[0].u.bitmap.netcode, sizeof(sms.user_data[0].u.bitmap.netcode), "%s", argv[optind+2]);
		dprintf("Operator code: %s\n", argv[optind+2]);
	}

	sms.user_data[1].type = GN_SMS_DATA_None;
	if (sms.user_data[0].u.bitmap.type == GN_BMP_PictureMessage) {
		sms.user_data[1].type = GN_SMS_DATA_NokiaText;
		readtext(&sms.user_data[1]);
		sms.user_data[2].type = GN_SMS_DATA_None;
	}

	data->message_center = calloc(1, sizeof(gn_sms_message_center));
	data->message_center->id = 1;
	if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
		snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", data->message_center->smsc.number);
		sms.smsc.type = data->message_center->smsc.type;
	}
	free(data->message_center);

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;

	/* Send the message. */
	data->sms = &sms;
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
		fprintf(stderr, _("Can't open file %s for writing!\n"), FileName);
		break;
	default:
		break;
	}

	return error;
}

gn_error getlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_bmp bitmap;
	gn_error error;
	gn_phone *info = &state->driver.phone;

	memset(&bitmap, 0, sizeof(gn_bmp));
	bitmap.type = set_bitmap_type(optarg);

	/* There is caller group number missing in argument list. */
	if ((bitmap.type == GN_BMP_CallerLogo) && (argc >= optind + 1)) {
		bitmap.number = (argv[optind][0] < '0') ? 0 : argv[optind][0] - '0';
		if (bitmap.number > 9)
			bitmap.number = 0;
	}

	if (bitmap.type != GN_BMP_None) {
		data->bitmap = &bitmap;

		error = gn_sm_functions(GN_OP_GetBitmap, data, state);

		gn_bmp_print(&bitmap, stderr);
		switch (error) {
		case GN_ERR_NONE:
			switch (bitmap.type) {
			case GN_BMP_DealerNoteText:
				fprintf(stdout, _("Dealer welcome note "));
				if
					(bitmap.text[0]) fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				else
					fprintf(stdout, _("currently empty\n"));
				break;
			case GN_BMP_WelcomeNoteText:
				fprintf(stdout, _("Welcome note "));
				if (bitmap.text[0])
					fprintf(stdout, _("currently set to \"%s\"\n"), bitmap.text);
				else
					fprintf(stdout, _("currently empty\n"));
				break;
			case GN_BMP_OperatorLogo:
			case GN_BMP_NewOperatorLogo:
				if (!bitmap.width)
					goto empty_bitmap;
				fprintf(stderr, _("Operator logo for %s (%s) network got successfully\n"),
						bitmap.netcode, gn_network_name_get(bitmap.netcode));
				if (argc == optind + 2) {
					snprintf(bitmap.netcode, sizeof(bitmap.netcode), "%s", argv[optind + 1]);
					if (!strcmp(gn_network_name_get(bitmap.netcode), _("unknown"))) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return GN_ERR_FAILED;
					}
				}
				break;
			case GN_BMP_StartupLogo:
				if (!bitmap.width)
					goto empty_bitmap;
				fprintf(stderr, _("Startup logo got successfully\n"));
				if (argc == optind + 2) {
					snprintf(bitmap.netcode, sizeof(bitmap.netcode), "%s", argv[optind + 1]);
					if (!strcmp(gn_network_name_get(bitmap.netcode), _("unknown"))) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return GN_ERR_FAILED;
					}
				}
				break;
			case GN_BMP_CallerLogo:
				if (!bitmap.width)
					fprintf(stderr, _("Your phone doesn't have logo uploaded !\n"));
				else
					fprintf(stderr, _("Caller logo got successfully\n"));
				fprintf(stdout, _("Caller group name: %s, ringing tone: %s (%d)\n"),
					bitmap.text, get_ringtone_name(bitmap.ringtone, data, state), bitmap.ringtone);
				if (argc == optind + 3) {
					snprintf(bitmap.netcode, sizeof(bitmap.netcode), "%s", argv[optind + 2]);
					if (!strcmp(gn_network_name_get(bitmap.netcode), _("unknown"))) {
						fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
						return GN_ERR_FAILED;
					}
				}
				break;
			default:
				fprintf(stderr, _("Unknown bitmap type.\n"));
				break;
			empty_bitmap:
				fprintf(stderr, _("Your phone doesn't have logo uploaded !\n"));
				return GN_ERR_FAILED;
			}
			if ((argc > optind) && (SaveBitmapFileDialog(argv[optind], &bitmap, info) != GN_ERR_NONE))
				return GN_ERR_FAILED;
			break;
		default:
			fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
			break;
		}
	} else {
		fprintf(stderr, _("What kind of logo do you want to get ?\n"));
		return GN_ERR_FAILED;
	}

	return error;
}


/* Sending logos. */
static gn_error ReadBitmapFileDialog(char *FileName, gn_bmp *bitmap, gn_phone *info)
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


gn_error setlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	gn_bmp bitmap, oldbit;
	gn_network_info networkinfo;
	gn_error error = GN_ERR_NOTSUPPORTED;
	gn_phone *phone = &state->driver.phone;
	int i;

	gn_data_clear(data);
	data->bitmap = &bitmap;
	data->network_info = &networkinfo;

	memset(&bitmap.text, 0, sizeof(bitmap.text));

	bitmap.type = set_bitmap_type(optarg);
	switch (bitmap.type) {
	case GN_BMP_WelcomeNoteText:
	case GN_BMP_DealerNoteText:
		if (argc > optind)
			snprintf(bitmap.text, sizeof(bitmap.text), "%s", argv[optind]);
		break;
	case GN_BMP_OperatorLogo:
		error = (argc > optind) ? ReadBitmapFileDialog(argv[optind], &bitmap, phone) : gn_bmp_null(&bitmap, phone);
		if (error != GN_ERR_NONE)
			return error;
			
		memset(&bitmap.netcode, 0, sizeof(bitmap.netcode));
		/* FIXME: ugly as hell */
		if (!strncmp(state->driver.phone.models, "6510", 4))
			gn_bmp_resize(&bitmap, GN_BMP_NewOperatorLogo, phone);
		else
			gn_bmp_resize(&bitmap, GN_BMP_OperatorLogo, phone);

		if (argc > optind + 1) {
			snprintf(bitmap.netcode, sizeof(bitmap.netcode), "%s", argv[optind + 1]);
			if (!strcmp(gn_network_name_get(bitmap.netcode), _("unknown"))) {
				fprintf(stderr, _("Sorry, gnokii doesn't know %s network !\n"), bitmap.netcode);
				return GN_ERR_UNKNOWN;
			}
		} else {
			if (gn_sm_functions(GN_OP_GetNetworkInfo, data, state) == GN_ERR_NONE)
				snprintf(bitmap.netcode, sizeof(bitmap.netcode), "%s", networkinfo.network_code);
		}
		break;
	case GN_BMP_StartupLogo:
		if ((argc > optind) && ((error = ReadBitmapFileDialog(argv[optind], &bitmap, phone)) != GN_ERR_NONE))
			return error;
		gn_bmp_resize(&bitmap, GN_BMP_StartupLogo, phone);
		break;
	case GN_BMP_CallerLogo:
		if ((argc > optind) && ((error = ReadBitmapFileDialog(argv[optind], &bitmap, phone)) != GN_ERR_NONE))
			return error;
		gn_bmp_resize(&bitmap, GN_BMP_CallerLogo, phone);
		if (argc > optind + 1) {
			bitmap.number = (argv[optind+1][0] < '0') ? 0 : argv[optind+1][0] - '0';
			if (bitmap.number > 9)
				bitmap.number = 0;
			dprintf("%i \n", bitmap.number);
			oldbit.type = GN_BMP_CallerLogo;
			oldbit.number = bitmap.number;
			data->bitmap = &oldbit;
			if (gn_sm_functions(GN_OP_GetBitmap, data, state) == GN_ERR_NONE) {
				/* We have to get the old name and ringtone!! */
				bitmap.ringtone = oldbit.ringtone;
				snprintf(bitmap.text, sizeof(bitmap.text), "%s", oldbit.text);
			}
			if (argc > optind + 2)
				snprintf(bitmap.text, sizeof(bitmap.text), "%s", argv[optind + 2]);
		}
		break;
	default:
		fprintf(stderr, _("What kind of logo do you want to set ?\n"));
		return GN_ERR_UNKNOWN;
	}

	data->bitmap = &bitmap;
	error = gn_sm_functions(GN_OP_SetBitmap, data, state);

	switch (error) {
	case GN_ERR_NONE:
		oldbit.type = bitmap.type;
		oldbit.number = bitmap.number;
		data->bitmap = &oldbit;
		if (gn_sm_functions(GN_OP_GetBitmap, data, state) == GN_ERR_NONE) {
			switch (bitmap.type) {
			case GN_BMP_WelcomeNoteText:
			case GN_BMP_DealerNoteText:
				if (strcmp(bitmap.text, oldbit.text)) {
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
					data->bitmap = &oldbit;
					gn_sm_functions(GN_OP_SetBitmap, data, state);
					gn_sm_functions(GN_OP_GetBitmap, data, state);
					
					if (bitmap.type == GN_BMP_DealerNoteText) {
						fprintf(stderr, _("Error setting dealer welcome note - "));
					} else {
						fprintf(stderr, _("Error setting welcome note - "));
					}
					
					if (oldbit.text[0] != '!') {
						fprintf(stderr, _("SIM card and PIN is required\n"));
						return GN_ERR_UNKNOWN;
					} else {
						data->bitmap = &bitmap;
						gn_sm_functions(GN_OP_SetBitmap, data, state);
						data->bitmap = &oldbit;
						gn_sm_functions(GN_OP_GetBitmap, data, state);
						fprintf(stderr, _("too long, truncated to \"%s\" (length %zi)\n"), oldbit.text, strlen(oldbit.text));
						return GN_ERR_ENTRYTOOLONG;
					}
				}
				break;
			case GN_BMP_StartupLogo:
				for (i = 0; i < oldbit.size; i++) {
					if (oldbit.bitmap[i] != bitmap.bitmap[i]) {
						fprintf(stderr, _("Error setting startup logo - SIM card and PIN is required\n"));
						fprintf(stderr, _("i: %i, %2x %2x\n"), i, oldbit.bitmap[i], bitmap.bitmap[i]);
						return GN_ERR_UNKNOWN;
					}
				}
				break;
			default:
				break;

			}
		}
		fprintf(stderr, _("Done.\n"));
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

gn_error viewlogo(char *filename)
{
	gn_error error;
	error = gn_file_bitmap_show(filename);
	if (error != GN_ERR_NONE)
		fprintf(stderr, _("Could not load bitmap: %s\n"), gn_error_print(error));
	return error;
}

