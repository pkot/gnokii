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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000-2001 Chris Kemp, Marcin Wiacek
  Copyright (C) 2000-2004 Pawel Kot
  Copyright (C) 2001      Bartek Klepacz
  Copyright (C) 2002      Markus Plail, Pavel Machek
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2003      Tomi Ollila

  Functions to read and write common file types.

*/

#include "config.h"
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

#include "gnokii-internal.h"
#include "gnokii.h"
#include "gsm-filetypes.h"
#include "phones/nokia.h"

#ifdef HAVE_X11_XPM_H
#  include <X11/xpm.h>
#endif

#include "misc.h"

/* Ringtone File Functions */
/* ####################### */


/* Function to convert scale field in to correct number. */
static int ringtone_get_duration(char *num)
{
	int duration = 0;

	switch (atoi(num)) {
	case 1:
		duration = 128;
		break;
	case 2:
		duration = 64;
		break;
	case 4:
		duration = 32;
		break;
	case 8:
		duration = 16;
		break;
	case 16:
		duration = 8;
		break;
	case 32:
		duration = 4;
		break;
	}
	return duration;
}


static int ringtone_get_scale(char *num)
{
	/* This may well need improving. */
	int scale=0;

	if ((atoi(num)) < 4) scale = (atoi(num));
	if ((atoi(num)) > 4) scale = (atoi(num)) - 4;

	return scale;
}


/* Currently only reads rtttl and ott files - can be later extended to midi etc. */

gn_error gn_file_ringtone_read(char *filename, gn_ringtone *ringtone)
{
	FILE *file;
	gn_error error;
	gn_filetypes filetype;

	file = fopen(filename, "rb");

	if (!file)
		return GN_ERR_FAILED;

	/* FIXME: for now identify the filetype based on the extension */
	/* I don't like this but I haven't got any .ott files to work out a better way */

	filetype = GN_FT_RTTTL;
	if (strstr(filename, ".ott")) filetype = GN_FT_OTT; /* OTT files saved by NCDS3 */
	else if (strstr(filename, ".mid")) filetype = GN_FT_MIDI;
	else if (strstr(filename, ".raw")) filetype = GN_FT_NOKRAW_TONE;

	error = GN_ERR_NONE;

	rewind(file);  /* Not necessary for now but safer */

	switch (filetype) {
	case GN_FT_RTTTL:
		error = file_rtttl_load(file, ringtone);
		fclose(file);
		break;
	case GN_FT_OTT:
		error = file_ott_load(file, ringtone);
		fclose(file);
		break;
	case GN_FT_MIDI:
		error = file_midi_load(file, ringtone);
		fclose(file);
		break;
	case GN_FT_NOKRAW_TONE:
		error = file_nokraw_load(file, ringtone);
		fclose(file);
		break;
	default:
		error = GN_ERR_WRONGDATAFORMAT;
		break;
	}

	return error;
}


gn_error file_ott_load(FILE *file, gn_ringtone *ringtone)
{
	char buffer[2000];
	int i;

	i = fread(buffer, 1, 2000, file);
	if (!feof(file)) return GN_ERR_INVALIDSIZE;
	return gn_ringtone_unpack(ringtone, buffer, i);
}


gn_error file_rtttl_load(FILE *file, gn_ringtone *ringtone)
{
	int nr_note = 0;

	int default_note_scale = 2;
	int default_note_duration = 4;
	unsigned char buffer[2000];
	unsigned char *def, *notes, *ptr;

	if (fread(buffer, 1, 2000, file) < 1)
		return GN_ERR_FAILED;

	/* This is for buggy RTTTL ringtones without name. */
	if (buffer[0] != RTTTL_SEP[0]) {
		strtok(buffer, RTTTL_SEP);
		snprintf(ringtone->name, sizeof(ringtone->name), "%s", buffer);
		def = strtok(NULL, RTTTL_SEP);
		notes = strtok(NULL, RTTTL_SEP);
	} else {
		snprintf(ringtone->name, sizeof(ringtone->name), "GNOKII");
		def = strtok(buffer, RTTTL_SEP);
		notes = strtok(NULL, RTTTL_SEP);
	}

	ptr = strtok(def, ", ");
	/* Parsing the <defaults> section. */
	ringtone->tempo = 63;

	while (ptr) {

		switch(*ptr) {
		case 'd':
		case 'D':
			default_note_duration = ringtone_get_duration(ptr+2);
			break;
		case 'o':
		case 'O':
			default_note_scale = ringtone_get_scale(ptr+2);
			break;
		case 'b':
		case 'B':
			ringtone->tempo = atoi(ptr+2);
			break;
		}

		ptr = strtok(NULL,", ");
	}

	dprintf("default_note_duration = %d\n", default_note_duration);
	dprintf("default_note_scale = %d\n", default_note_scale);

	/* Parsing the <note-command>+ section. */
	ptr = strtok(notes, ", ");
	while (ptr && (nr_note < GN_RINGTONE_MAX_NOTES)) {

		/* [<duration>] */
		ringtone->notes[nr_note].duration = ringtone_get_duration(ptr);
		if (ringtone->notes[nr_note].duration == 0)
			ringtone->notes[nr_note].duration = default_note_duration;

		/* Skip all numbers in duration specification. */
		while (isdigit(*ptr))
			ptr++;

		/* <note> */

		if ((*ptr >= 'a') && (*ptr <= 'g')) ringtone->notes[nr_note].note = ((*ptr - 'a') * 2) + 10;
		else if ((*ptr >= 'A') && (*ptr <= 'G')) ringtone->notes[nr_note].note = ((*ptr - 'A') * 2) + 10;
		else if ((*ptr == 'H') || (*ptr == 'h')) ringtone->notes[nr_note].note = 12;
		else ringtone->notes[nr_note].note = 255;

		if ((ringtone->notes[nr_note].note > 13) && (ringtone->notes[nr_note].note != 255))
			ringtone->notes[nr_note].note -= 14;

		ptr++;

		if (*ptr == '#') {
			ringtone->notes[nr_note].note++;
			if ((ringtone->notes[nr_note].note == 5) || (ringtone->notes[nr_note].note == 13))
				ringtone->notes[nr_note].note++;
			ptr++;
		}

		/* Check for dodgy rtttl */
		/* [<special-duration>] */
		if (*ptr == '.') {
			ringtone->notes[nr_note].duration *= 1.5;
			ptr++;
		}

		/* [<scale>] */
		if (ringtone->notes[nr_note].note != 255) {
			if (isdigit(*ptr)) {
				ringtone->notes[nr_note].note += ringtone_get_scale(ptr) * 14;
				ptr++;
			} else {
				ringtone->notes[nr_note].note += default_note_scale * 14;
			}
		}

		/* [<special-duration>] */
		if (*ptr == '.') {
			ringtone->notes[nr_note].duration *= 1.5;
			ptr++;
		}

		nr_note++;
		ptr = strtok(NULL, ", ");
	}

	ringtone->notes_count = nr_note;

	return GN_ERR_NONE;
}


gn_error file_nokraw_load(FILE *file, gn_ringtone *ringtone)
{
	unsigned char buf[4096];
	int n;
	gn_error err;

	snprintf(ringtone->name, sizeof(ringtone->name), "GNOKII");

	if ((n = fread(buf, 1, sizeof(buf), file)) < 0) return GN_ERR_UNKNOWN;

	if (buf[0] == 0x00 && buf[1] == 0x02 && buf[2] == 0xfc && buf[3] == 0x09)
		err = pnok_ringtone_from_raw(ringtone, buf + 4, n - 4);
	else if (buf[0] == 0x02 && buf[1] == 0xfc && buf[2] == 0x09)
		err = pnok_ringtone_from_raw(ringtone, buf + 3, n - 3);
	else
		err = pnok_ringtone_from_raw(ringtone, buf, n);

	return err;
}


/* Save the ringtone file - this will overwrite the file */
/* Confirming must be done before this is called */
gn_error gn_file_ringtone_save(char *filename, gn_ringtone *ringtone)
{
	FILE *file;
	gn_error error;

	file = fopen(filename, "wb");

	if (!file) return GN_ERR_FAILED;

	/* FIXME... */
	/* We need a way of passing these functions a filetype rather than rely on the extension */
	if (strstr(filename, ".ott")) {
		error = file_ott_save(file, ringtone);
	} else if (strstr(filename, ".mid")) {
		error = file_midi_save(file, ringtone);
	} else if (strstr(filename, ".raw3")) {
		error = file_nokraw_save(file, ringtone, 0);
	} else if (strstr(filename, ".raw")) {
		error = file_nokraw_save(file, ringtone, 1);
	} else {
		error = file_rtttl_save(file, ringtone);
	}
	fclose(file);
	return error;
}


gn_error file_ott_save(FILE *file, gn_ringtone *ringtone)
{
	char buffer[2000];
	int i = 2000;

	/* PackRingtone writes up to i chars and returns in i the number written */
	gn_ringtone_pack(ringtone, buffer, &i);

	if (i < 2000) {
		fwrite(buffer, 1, i, file);
		return GN_ERR_NONE;
	} else {
		return GN_ERR_INVALIDSIZE;
	}
}

gn_error file_rtttl_save(FILE *file, gn_ringtone *ringtone)
{
	int default_duration, default_scale = 2, current_note;
	int buffer[6];
	int i, j, k = 0;

	/* Saves ringtone name */
	fprintf(file, "%s:", ringtone->name);

	/* Find the most frequently used duration and use this for the default */
	for (i = 0; i < 6; i++) buffer[i] = 0;
	for (i = 0; i < ringtone->notes_count; i++) {
		switch (ringtone->notes[i].duration) {
		case 192:
			buffer[0]++; break;
		case 128:
			buffer[0]++; break;
		case 96:
			buffer[1]++; break;
		case 64:
			buffer[1]++; break;
		case 48:
			buffer[2]++; break;
		case 32:
			buffer[2]++; break;
		case 24:
			buffer[3]++; break;
		case 16:
			buffer[3]++; break;
		case 12:
			buffer[4]++; break;
		case 8:
			buffer[4]++; break;
		case 6:
			buffer[5]++; break;
		case 4:
			buffer[5]++; break;
		}
	}

	/* Now find the most frequently used */
	j = 0;
	for (i = 0; i < 6; i++) {
		if (buffer[i] > j) {
			k = i;
			j = buffer[i];
		}
	}

	/* Finally convert and save the default duration */
	switch (k) {
	case 0:
		default_duration = 128;
		fprintf(file, "d=1,");
		break;
	case 1:
		default_duration = 64;
		fprintf(file, "d=2,");
		break;
	case 2:
		default_duration = 32;
		fprintf(file, "d=4,");
		break;
	case 3:
		default_duration = 16;
		fprintf(file, "d=8,");
		break;
	case 4:
		default_duration = 8;
		fprintf(file, "d=16,");
		break;
	case 5:
		default_duration = 4;
		fprintf(file, "d=32,");
		break;
	default:
		default_duration = 16;
		fprintf(file, "d=8,");
		break;
	}

	/* Find the most frequently used scale and use this for the default */
	for (i = 0; i < 6; i++) buffer[i] = 0;
	for (i = 0; i < ringtone->notes_count; i++) {
		if (ringtone->notes[i].note != 255) {
			buffer[ringtone->notes[i].note/14]++;
		}
	}
	j = 0;
	for (i = 0; i < 6; i++) {
		if (buffer[i] > j) {
			default_scale = i;
			j = buffer[i];
		}
	}

	/* Save the default scale and tempo */
	fprintf(file, "o=%i,", default_scale+4);
	fprintf(file, "b=%i:", ringtone->tempo);

	dprintf("default_note_duration=%d\n", default_duration);
	dprintf("default_note_scale=%d\n", default_scale);
	dprintf("Number of notes=%d\n",ringtone->notes_count);

	/* Now loop round for each note */
	for (i = 0; i < ringtone->notes_count; i++) {
		current_note = ringtone->notes[i].note;

		/* This note has a duration different than the default. We must save it */
		if (ringtone->notes[i].duration != default_duration) {
			switch (ringtone->notes[i].duration) {
			case 192:                      //192=128*1.5
				fprintf(file, "1"); break;
			case 128:
				fprintf(file, "1"); break;
			case 96:                       //96=64*1.5
				fprintf(file, "2"); break;
			case 64:
				fprintf(file, "2"); break;
			case 48:                       //48=32*1.5
				fprintf(file, "4"); break;
			case 32:
				fprintf(file, "4"); break;
			case 24:                       //24=16*1.5;
				fprintf(file, "8"); break;
			case 16:
				fprintf(file, "8"); break;
			case 12:                       //12=8*1.5
				fprintf(file, "16"); break;
			case 8:
				fprintf(file, "16"); break;
			case 6:                        //6=4*1.5
				fprintf(file, "32"); break;
			case 4:
				fprintf(file, "32"); break;
			default:
				break;
			}
		}

		/* Now save the actual note */
		switch (gn_note_get(current_note)) {
		case GN_RINGTONE_Note_C  : fprintf(file, "c");  break;
		case GN_RINGTONE_Note_Cis: fprintf(file, "c#"); break;
		case GN_RINGTONE_Note_D  : fprintf(file, "d");  break;
		case GN_RINGTONE_Note_Dis: fprintf(file, "d#"); break;
		case GN_RINGTONE_Note_E  : fprintf(file, "e");  break;
		case GN_RINGTONE_Note_F  : fprintf(file, "f");  break;
		case GN_RINGTONE_Note_Fis: fprintf(file, "f#"); break;
		case GN_RINGTONE_Note_G  : fprintf(file, "g");  break;
		case GN_RINGTONE_Note_Gis: fprintf(file, "g#"); break;
		case GN_RINGTONE_Note_A  : fprintf(file, "a");  break;
		case GN_RINGTONE_Note_Ais: fprintf(file, "a#"); break;
		case GN_RINGTONE_Note_H  : fprintf(file, "h");  break;
		default                  : fprintf(file, "p");  break; /*Pause ? */
		}

		/* Saving info about special duration */
		if (ringtone->notes[i].duration == 128 * 1.5 ||
		    ringtone->notes[i].duration == 64 * 1.5 ||
		    ringtone->notes[i].duration == 32 * 1.5 ||
		    ringtone->notes[i].duration == 16 * 1.5 ||
		    ringtone->notes[i].duration == 8 * 1.5 ||
		    ringtone->notes[i].duration == 4 * 1.5)
			fprintf(file, ".");

		/* This note has a scale different than the default, so save it */
		if ( (current_note != 255) && (current_note/14 != default_scale))
			fprintf(file, "%i",(current_note/14) + 4);

		/* And a separator before next note */
		if (i != ringtone->notes_count - 1)
			fprintf(file, ",");
	}

	return GN_ERR_NONE;
}

gn_error file_nokraw_save(FILE *file, gn_ringtone *ringtone, int dct4)
{
	int n;
	char buf[4096];
	gn_error err;

	n = sizeof(buf);
	if ((err = pnok_ringtone_to_raw(buf, &n, ringtone, dct4)) != GN_ERR_NONE) return err;

	if (fwrite(buf, n, 1, file) != 1) return GN_ERR_UNKNOWN;

	return GN_ERR_NONE;
}

/* Bitmap file functions */
/* ##################### */

gn_error gn_file_bitmap_read(char *filename, gn_bmp *bitmap, gn_phone *info)
{
	FILE *file;
	unsigned char buffer[9];
	int error;
	size_t count;
	gn_filetypes filetype = GN_FT_None;

	file = fopen(filename, "rb");

	if (!file)
		return GN_ERR_FAILED;

	count = fread(buffer, 1, 9, file); /* Read the header of the file. */

	/* Attempt to identify filetype */

	if (count >= 3 && memcmp(buffer, "NOL", 3) == 0) {               /* NOL files have 'NOL' at the start */
		filetype = GN_FT_NOL;
	} else if (count >= 3 && memcmp(buffer, "NGG", 3) == 0) {        /* NGG files have 'NGG' at the start */
		filetype = GN_FT_NGG;
	} else if (count >= 4 && memcmp(buffer, "FORM", 4) == 0) {       /* NSL files have 'FORM' at the start */
		filetype = GN_FT_NSL;
	} else if (count >= 3 && memcmp(buffer, "NLM", 3) == 0) {        /* NLM files have 'NLM' at the start */
		filetype = GN_FT_NLM;
	} else if (count >= 2 && memcmp(buffer, "BM", 2) == 0) {         /* BMP, I61 and GGP files have 'BM' at the start */
		filetype = GN_FT_BMP;
	} else if (count >= 9 && memcmp(buffer, "/* XPM */", 9) == 0) {  /* XPM files have 'XPM' at the start */
		filetype = GN_FT_XPMF;
	}

	if ((filetype == GN_FT_None) && strstr(filename, ".otb")) filetype = GN_FT_OTA; /* OTA files saved by NCDS3 */

	rewind(file);

	switch (filetype) {
	case GN_FT_NOL:
		error = file_nol_load(file, bitmap, info);
		break;
	case GN_FT_NGG:
		error = file_ngg_load(file, bitmap, info);
		break;
	case GN_FT_NSL:
		error = file_nsl_load(file, bitmap);
		break;
	case GN_FT_NLM:
		error = file_nlm_load(file, bitmap);
		break;
	case GN_FT_OTA:
		error = file_ota_load(file, bitmap, info);
		break;
	case GN_FT_BMP:
		error = file_bmp_load(file, bitmap);
		break;
	case GN_FT_XPMF:
#ifdef XPM
		error = file_xpm_load(filename, bitmap);
		break;
#else
		fprintf(stderr, _("Sorry, gnokii was not compiled with XPM support.\n"));
		/* FALLTHRU */
#endif
	default:
		error = GN_ERR_WRONGDATAFORMAT;
		break;
	}

	if (file) fclose(file);
	return error;
}


#ifdef XPM
gn_error file_xpm_load(char *filename, gn_bmp *bitmap)
{
	int error, x, y;
	XpmImage image;
	XpmInfo info;

	error = XpmReadFileToXpmImage(filename, &image, &info);

	switch (error) {
	case XpmColorError:
		return GN_ERR_WRONGDATAFORMAT;
	case XpmColorFailed:
		return GN_ERR_WRONGDATAFORMAT;
	case XpmOpenFailed:
		return GN_ERR_FAILED;
	case XpmFileInvalid:
		return GN_ERR_WRONGDATAFORMAT;
	case XpmSuccess:
	default:
		break;
	}

	if (image.ncolors != 2) return GN_ERR_WRONGDATAFORMAT;

	bitmap->height = image.height;
	bitmap->width = image.width;
	bitmap->size = ((bitmap->width + 7) / 8) * bitmap->height;

	if (bitmap->size > GN_BMP_MAX_SIZE) {
		fprintf(stderr, _("Bitmap too large\n"));
		return GN_ERR_INVALIDSIZE;
	}

	gn_bmp_clear(bitmap);

	for (y = 0; y < image.height; y++) {
		for (x = 0; x < image.width; x++) {
			if (image.data[y * image.width + x] == 0) gn_bmp_point_set(bitmap, x, y);
		}
	}

	return GN_ERR_NONE;
}
#endif

/* Based on the article from the Polish Magazine "Bajtek" 11/92 */
/* Marcin-Wiacek@Topnet.PL */

/* This loads the image as a startup logo - but is resized as necessary later */
gn_error file_bmp_load(FILE *file, gn_bmp *bitmap)
{
	unsigned char buffer[34];
	bool first_black;
	int w, h, pos, y, x, i, sizeimage;

	gn_bmp_clear(bitmap);

	/* required part of header */
	if (fread(buffer, 1, 34, file) != 34)
		return GN_ERR_FAILED;

	h = buffer[22] + 256 * buffer[21]; /* height of image in the file */
	w = buffer[18] + 256 * buffer[17]; /* width of image in the file */
	dprintf("Image Size in BMP file: %dx%d\n", w, h);
	bitmap->width = w;
	bitmap->height = h;
	bitmap->size = bitmap->width * bitmap->height / 8;

	dprintf("Number of colors in BMP file: ");
	switch (buffer[28]) {
	case 1:
		dprintf("2 (supported by gnokii)\n");
		break;
	case 4:
		dprintf("16 (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	case 8:
		dprintf("256 (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	case 24:
		dprintf("True Color (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	default:
		dprintf("unknown color type (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

	dprintf("Compression in BMP file: ");
	switch (buffer[30]) {
	case 0:
		dprintf("no compression (supported by gnokii)\n");
		break;
	case 1:
		dprintf("RLE8 (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	case 2:
		dprintf("RLE4 (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	default:
		dprintf("unknown compression type (not supported by gnokii)\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

	pos = buffer[10] - 34;
	/* the rest of the header (if exists) and the color palette */
	if (fread(buffer, 1, pos, file) != pos)
		return GN_ERR_WRONGDATAFORMAT;

	dprintf("First color in BMP file: %i %i %i ", buffer[pos-8], buffer[pos-7], buffer[pos-6]);
	if (buffer[pos-8] == 0x00 && buffer[pos-7] == 0x00 && buffer[pos-6] == 0x00) dprintf("(black)");
	if (buffer[pos-8] == 0xff && buffer[pos-7] == 0xff && buffer[pos-6] == 0xff) dprintf("(white)");
	if (buffer[pos-8] == 0x66 && buffer[pos-7] == 0xbb && buffer[pos-6] == 0x66) dprintf("(green)");
	dprintf("\n");

	dprintf("Second color in BMP file: %i %i %i ", buffer[pos-4], buffer[pos-3], buffer[pos-2]);
	if (buffer[pos-4] == 0x00 && buffer[pos-3] == 0x00 && buffer[pos-2] == 0x00) dprintf("(black)");
	if (buffer[pos-4] == 0xff && buffer[pos-3] == 0xff && buffer[pos-2] == 0xff) dprintf("(white)");
	if (buffer[pos-4] == 0x66 && buffer[pos-3] == 0xbb && buffer[pos-2] == 0x66) dprintf("(green)");
	dprintf("\n");

	first_black = false;
	if (buffer[pos-8] == 0 && buffer[pos-7] == 0 && buffer[pos-6] == 0) first_black = true;

	sizeimage = 0;
	pos = 7;
	for (y = h-1; y >= 0; y--) { /* the lines are written from the last one to the first one */
		i = 1;
		for (x = 0; x < w; x++) {
			if (pos == 7) { /* new byte ! */
				if (fread(buffer, 1, 1, file) != 1)
					return GN_ERR_WRONGDATAFORMAT;
				sizeimage++;
				i++;
				if (i == 5) i = 1; /* each line is written in multiples of 4 bytes */
			}
			if (x <= bitmap->width && y <= bitmap->height) { /* we have top left corner ! */
				if (first_black) {
					if ((buffer[0] & (1 << pos)) <= 0) gn_bmp_point_set(bitmap, x, y);
				} else {
					if ((buffer[0] & (1 << pos)) > 0) gn_bmp_point_set(bitmap, x, y);
				}
			}
			pos--;
			if (pos < 0) pos = 7; /* going to the new byte */
		}
		pos = 7; /* going to the new byte */
		if (i != 1) {
			while (i != 5) { /* each line is written in multiples of 4 bytes */
				if (fread(buffer, 1, 1, file) != 1)
					return GN_ERR_WRONGDATAFORMAT;
				sizeimage++;
				i++;
			}
		}
	}
	dprintf("Data size in BMP file: %i\n", sizeimage);
	return GN_ERR_NONE;
}

gn_error file_nol_load(FILE *file, gn_bmp *bitmap, gn_phone *info)
{
	unsigned char buffer[GN_BMP_MAX_SIZE + 20];
	int i, j;

	if (fread(buffer, 1, 20, file) != 20)
		return GN_ERR_FAILED;
	snprintf(bitmap->netcode, sizeof(bitmap->netcode), "%d %02d", buffer[6] + 256 * buffer[7], buffer[8]);

	bitmap->width = buffer[10];
	bitmap->height = buffer[12];
	bitmap->type = GN_BMP_OperatorLogo;
	bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);

	if (((bitmap->height != 14) || (bitmap->width != 72)) && /* standard size */
	    ((bitmap->height != 21) || (bitmap->width != 78)) && /* standard size */
	    (!info || (bitmap->height != info->operator_logo_height) || (bitmap->width != info->operator_logo_width))) {
		dprintf("Invalid Image Size (%dx%d).\n", bitmap->width, bitmap->height);
		return GN_ERR_INVALIDSIZE;
	}

	for (i = 0; i < bitmap->size; i++) {
		if (fread(buffer, 1, 8, file) == 8) {
			bitmap->bitmap[i] = 0;
			for (j = 7; j >= 0; j--)
				if (buffer[7-j] == '1')
					bitmap->bitmap[i] |= (1 << j);
		} else {
			dprintf("too short\n");
			return GN_ERR_INVALIDSIZE;
		}
	}

	/* Some programs writes here fileinfo */
	if (fread(buffer, 1, 1, file) == 1) {
		dprintf("Fileinfo: %c", buffer[0]);
		while (fread(buffer, 1, 1, file) == 1) {
			if (buffer[0] != 0x0A) dprintf("%c", buffer[0]);
		}
		dprintf("\n");
	}
	return GN_ERR_NONE;
}

gn_error file_ngg_load(FILE *file, gn_bmp *bitmap, gn_phone *info)
{
	unsigned char buffer[2000];
	int i, j;

	bitmap->type = GN_BMP_CallerLogo;

	if (fread(buffer, 1, 16, file) != 16)
		return GN_ERR_FAILED;
	bitmap->width = buffer[6];
	bitmap->height = buffer[8];
	bitmap->size = bitmap->height * bitmap->width / 8;

	if (((bitmap->height != 14) || (bitmap->width != 72)) && /* standard size */
	    ((bitmap->height != 21) || (bitmap->width != 78)) && /* standard size */
	    (!info || (bitmap->height != info->operator_logo_height) || (bitmap->width != info->operator_logo_width))) {
		dprintf("Invalid Image Size (%dx%d).\n", bitmap->width, bitmap->height);
		return GN_ERR_INVALIDSIZE;
	}

	for (i = 0; i < bitmap->size; i++) {
		if (fread(buffer, 1, 8, file) == 8){
			bitmap->bitmap[i] = 0;
			for (j = 7; j >= 0;j--)
				if (buffer[7-j] == '1')
					bitmap->bitmap[i] |= (1 << j);
		} else {
			return GN_ERR_INVALIDSIZE;
		}
	}

	/* Some programs writes here fileinfo */
	if (fread(buffer, 1, 1, file) == 1) {
	        dprintf("Fileinfo: %c",buffer[0]);
		while (fread(buffer, 1, 1, file) == 1) {
			if (buffer[0] != 0x0A) dprintf("%c", buffer[0]);
		}
		dprintf("\n");
	}
	return GN_ERR_NONE;
}

gn_error file_nsl_load(FILE *file, gn_bmp *bitmap)
{
	unsigned char block[6], buffer[870];
	int block_size, count;

	bitmap->size = 0;

	while (fread(block, 1, 6, file) == 6) {
		block_size = block[4] * 256 + block[5];
		dprintf("Block %c%c%c%c, size %i\n", block[0], block[1], block[2], block[3], block_size);
		if (!strncmp(block, "FORM", 4)) {
			dprintf("  File ID\n");
		} else {
			if (block_size > 864) return GN_ERR_WRONGDATAFORMAT;

			if (block_size != 0) {

				count = fread(buffer, 1, block_size, file);
				buffer[count] = 0;

				if (!strncmp(block, "VERS", 4)) dprintf("  File saved by: %s\n", buffer);
				if (!strncmp(block, "MODL", 4)) dprintf("  Logo saved from: %s\n", buffer);
				if (!strncmp(block, "COMM", 4)) dprintf("  Phone was connected to COM port: %s\n", buffer);

				if (!strncmp(block, "NSLD", 4)) {
					bitmap->size = block[4] * 256 + block[5];
					switch (bitmap->size) {
					case 864:  /* 6510/7110 startup logo */
						bitmap->height = 65;
						bitmap->width = 96;
						break;
					case 504: /* 6110 startup logo */
						bitmap->height = 48;
						bitmap->width = 84;
						break;
					case 768: /* 6210 */
						bitmap->height = 60;
						bitmap->width = 96;
						break;
					default:
						dprintf("Unknown startup logo!\n");
						return GN_ERR_WRONGDATAFORMAT;
					}
					bitmap->type = GN_BMP_StartupLogo;
					memcpy(bitmap->bitmap, buffer, bitmap->size);
					dprintf("  Startup logo (size %i)\n", block_size);
				}
			}
		}
	}
	if (bitmap->size == 0) return GN_ERR_INVALIDSIZE;
	return GN_ERR_NONE;
}

gn_error file_nlm_load(FILE *file, gn_bmp *bitmap)
{
	unsigned char buffer[84*48];
	int pos, pos2, x, y;
	div_t division;

	if (fread(buffer, 1, 5, file) != 5)
		return GN_ERR_FAILED;
	if (fread(buffer, 1, 1, file) != 1)
		return GN_ERR_FAILED;

	switch (buffer[0]) {
	case 0x00:
		bitmap->type = GN_BMP_OperatorLogo;
		break;
	case 0x01:
		bitmap->type = GN_BMP_CallerLogo;
		break;
	case 0x02:
		bitmap->type = GN_BMP_StartupLogo;
		break;
	case 0x03:
		bitmap->type = GN_BMP_PictureMessage;
		break;
	default:
		return GN_ERR_WRONGDATAFORMAT;
	}

	if (fread(buffer, 1, 4, file) != 4)
		return GN_ERR_FAILED;
	bitmap->width = buffer[1];
	bitmap->height = buffer[2];
	bitmap->size = bitmap->width * bitmap->height / 8;

	division = div(bitmap->width, 8);
	if (division.rem != 0) division.quot++; /* For startup logos */

	if (fread(buffer, 1, (division.quot * bitmap->height), file) != (division.quot * bitmap->height))
		return GN_ERR_INVALIDSIZE;

	gn_bmp_clear(bitmap);

	pos = 0; pos2 = 7;
	for (y = 0; y < bitmap->height; y++) {
		for (x = 0; x < bitmap->width; x++) {
			if ((buffer[pos] & (1 << pos2)) > 0) gn_bmp_point_set(bitmap, x, y);
			pos2--;
			if (pos2 < 0) {pos2 = 7; pos++;} /*going to new byte */
		}
		if (pos2 != 7) {pos2 = 7; pos++;} /* for startup logos-new line means new byte */
	}
	return GN_ERR_NONE;
}

gn_error file_ota_load(FILE *file, gn_bmp *bitmap, gn_phone *info)
{
	char buffer[4];

	/* We could check for extended info here - indicated by the 7th bit being set in the first byte */
	if (fread(buffer, 1, 4, file) != 4)
		return GN_ERR_FAILED;

	bitmap->width = buffer[1];
	bitmap->height = buffer[2];
	bitmap->size = bitmap->width * bitmap->height / 8;

	if (((bitmap->height == 48) && (bitmap->width == 84)) || /* standard size */
	    ((bitmap->height == 60) && (bitmap->width == 96)) || /* standard size */
	    (info && ((bitmap->height == info->startup_logo_height) && (bitmap->width == info->startup_logo_width)))) {
		bitmap->type = GN_BMP_StartupLogo;
	} else if (((bitmap->height == 14) && (bitmap->width == 72)) || /* standard size */
		   (info && ((bitmap->height == info->caller_logo_height) && (bitmap->width == info->caller_logo_width)))) {
		bitmap->type = GN_BMP_CallerLogo;
	} else {
		dprintf("Invalid Image Size (%dx%d).\n", bitmap->width, bitmap->height);
		return GN_ERR_INVALIDSIZE;
	}
	if (fread(bitmap->bitmap, 1, bitmap->size,file) != bitmap->size)
		return GN_ERR_INVALIDSIZE;
	return GN_ERR_NONE;
}

/* This overwrites an existing file - so this must be checked before calling */
gn_error gn_file_bitmap_save(char *filename, gn_bmp *bitmap, gn_phone *info)
{
	FILE *file;
	bool done = false;

	/* XPMs are a bit messy because we have to pass it the filename */

#ifdef XPM
	if (strstr(filename, ".xpm")) {
		file_xpm_save(filename, bitmap);
	} else {
#endif

		file = fopen(filename, "wb");

		if (!file) return GN_ERR_FAILED;

		if (strstr(filename, ".nlm")) {
			file_nlm_save(file, bitmap);
			done = true;
		}
		if (strstr(filename, ".ngg")) {
			file_ngg_save(file, bitmap, info);
			done = true;
		}
		if (strstr(filename, ".nsl")) {
			file_nsl_save(file, bitmap, info);
			done = true;
		}
		if (strstr(filename, ".otb")) {
			file_ota_save(file, bitmap);
			done = true;
		}
		if (strstr(filename, ".nol")) {
			file_nol_save(file, bitmap, info);
			done = true;
		}
		if (strstr(filename, ".bmp") ||
		    strstr(filename, ".ggp") ||
		    strstr(filename, ".i61")) {
			file_bmp_save(file, bitmap);
			done = true;
		}

		if (!done) {
			switch (bitmap->type) {
			case GN_BMP_CallerLogo:
				file_ngg_save(file, bitmap, info);
				break;
			case GN_BMP_OperatorLogo:
			case GN_BMP_NewOperatorLogo:
				file_nol_save(file, bitmap, info);
				break;
			case GN_BMP_StartupLogo:
				file_nsl_save(file, bitmap, info);
				break;
			case GN_BMP_PictureMessage:
				file_nlm_save(file, bitmap);
				break;
			case GN_BMP_WelcomeNoteText:
			case GN_BMP_DealerNoteText:
			case GN_BMP_None:
			default:
				break;
			}
		}
		fclose(file);
#ifdef XPM
	}
#endif
	return GN_ERR_NONE;
}

#ifdef XPM
void file_xpm_save(char *filename, gn_bmp *bitmap)
{
	XpmColor colors[2] = {{".","c","#000000","#000000","#000000","#000000"},
			      {"#","c","#ffffff","#ffffff","#ffffff","#ffffff"}};
	XpmImage image;
	unsigned int data[6240];
	int x, y;

	image.height = bitmap->height;
	image.width = bitmap->width;
	image.cpp = 1;
	image.ncolors = 2;
	image.colorTable = colors;
	image.data = data;

	for (y = 0; y < image.height; y++) {
		for (x = 0; x < image.width; x++)
			if (gn_bmp_point(bitmap, x, y))
				data[y * image.width + x] = 0;
			else
				data[y * image.width + x] = 1;
	}

	XpmWriteFileFromXpmImage(filename, &image, NULL);
}
#endif

/* Based on the article from the Polish Magazine "Bajtek" 11/92 */
/* Marcin-Wiacek@Topnet.PL */
void file_bmp_save(FILE *file, gn_bmp *bitmap)
{
	int x, y, pos, i, sizeimage;
	unsigned char buffer[1];
	div_t division;

	char header[] = {
/* 1st header */	'B', 'M',	/* BMP file ID */
			0x00, 0x00, 0x00, 0x00,	/* Size of file */
			0x00, 0x00,		/* Reserved for future use */
			0x00, 0x00,		/* Reserved for future use */
			62, 0x00, 0x00, 0x00,	/* Offset for image data */

/* 2nd header */	40, 0x00, 0x00, 0x00,	/* Length of this part of header */
			0x00, 0x00, 0x00, 0x00,	/* Width of image */
			0x00, 0x00, 0x00, 0x00,	/* Height of image */
			0x01, 0x00,		/* How many planes in target device */
			0x01, 0x00,		/* How many colors in image. 1 means 2^1=2 colors */
			0x00, 0x00, 0x00, 0x00,	/* Type of compression. 0 means no compression */
/* Sometimes */		0x00, 0x00, 0x00, 0x00,	/* Size of part with image data */
/* ttttttt...*/		0xE8, 0x03, 0x00, 0x00,	/* XPelsPerMeter */
/*hhiiiiissss*/		0xE8, 0x03, 0x00, 0x00,	/* YPelsPerMeter */
/* part of the
   header */		0x02, 0x00, 0x00, 0x00,	/* How many colors from palette is used */
/* doesn't
   exist */		0x00, 0x00, 0x00, 0x00,	/* How many colors from palette is required to display image. 0 means all */

/* Color
   palette */		0xFF, 0xFF, 0xFF,	/* First color in palette in Blue, Green, Red. Here white */
			0x00,			/* Each color in palette is end by 4'th byte */
			0x00, 0x00, 0x00,	/* Second color in palette in Blue, Green, Red. Here black */
			0x00};			/* Each color in palette is end by 4'th byte */

	header[22] = bitmap->height;
	header[18] = bitmap->width;

	pos = 7;
	sizeimage = 0;
	for (y = bitmap->height - 1; y >= 0; y--) { //lines are written from the last to the first
		i = 1;
		for (x = 0; x < bitmap->width; x++) {
			if (pos == 7) { //new byte !
				sizeimage++;
				i++;
				if (i == 5) i = 1; //each line is written in multiples of 4 bytes
			}
			pos--;
			if (pos < 0) pos = 7; //going to new byte
		}
		pos = 7; //going to new byte
		while (i != 5) { //each line is written in multiples of 4 bytes
			sizeimage++;
			i++;
		}
	}
	dprintf("Data size in BMP file: %i\n", sizeimage);
	division = div(sizeimage, 256);
	header[35] = division.quot;
	header[34] = sizeimage - (division.quot * 256);

	sizeimage = sizeimage + sizeof(header);
	dprintf("Size of BMP file: %i\n", sizeimage);
	division = div(sizeimage, 256);
	header[3] = division.quot;
	header[2] = sizeimage - (division.quot * 256);

	fwrite(header, 1, sizeof(header), file);

	pos = 7;
	for (y = bitmap->height - 1; y >= 0; y--) { //lines are written from the last to the first
		i = 1;
		for (x = 0; x < bitmap->width; x++) {
			if (pos == 7) { //new byte !
				if (x != 0) fwrite(buffer, 1, sizeof(buffer), file);
				i++;
				if(i == 5) i = 1; //each line is written in multiples of 4 bytes
				buffer[0] = 0;
			}
			if (gn_bmp_point(bitmap, x, y)) buffer[0] |= (1 << pos);
			pos--;
			if (pos < 0) pos = 7; //going to new byte
		}
		pos = 7; //going to new byte
		fwrite(buffer, 1, sizeof(buffer), file);
		while (i != 5) { //each line is written in multiples of 4 bytes
			buffer[0] = 0;
			fwrite(buffer, 1, sizeof(buffer), file);
			i++;
		}
	}
}

void file_ngg_save(FILE *file, gn_bmp *bitmap, gn_phone *info)
{

	char header[] = {'N', 'G', 'G', 0x00, 0x01, 0x00,
		         0x00, 0x00,		/* Width */
		         0x00, 0x00,		/* Height */
		         0x01, 0x00, 0x01, 0x00,
		         0x00, 0x00		/* Unknown.Can't be checksum - for */
						/* the same logo files can be different */
		         };

	char buffer[8];
	int i, j;

	gn_bmp_resize(bitmap, GN_BMP_CallerLogo, info);

	header[6] = bitmap->width;
	header[8] = bitmap->height;

	fwrite(header, 1, sizeof(header), file);

	for (i = 0; i < bitmap->size; i++) {
		for (j = 7; j >= 0;j--)
			if ((bitmap->bitmap[i] & (1 << j)) > 0) {
				buffer[7-j] = '1';
			} else {
				buffer[7-j] = '0';
			}
		fwrite(buffer, 1, 8, file);
	}
}

void file_nol_save(FILE *file, gn_bmp *bitmap, gn_phone *info)
{

	char header[] = {'N','O','L',0x00,0x01,0x00,
		         0x00,0x00,           /* MCC */
		         0x00,0x00,           /* MNC */
		         0x00,0x00,           /* Width */
		         0x00,0x00,           /* Height */
		         0x01,0x00,0x01,0x00,
		         0x00,                /* Unknown.Can't be checksum - for */
		         /* the same logo files can be different */
		         0x00};
	char buffer[8];
	int i, j, country, net;

	gn_bmp_resize(bitmap, GN_BMP_OperatorLogo, info);

	sscanf(bitmap->netcode, "%d %d", &country, &net);

	header[6] = country % 256;
	header[7] = country / 256;
	header[8] = net % 256;
	header[9] = net / 256;
	header[10] = bitmap->width;
	header[12] = bitmap->height;

	fwrite(header, 1, sizeof(header), file);

	for (i = 0; i < bitmap->size; i++) {
		for (j = 7; j >= 0; j--)
			if ((bitmap->bitmap[i] & (1 << j)) > 0) {
				buffer[7-j] = '1';
			} else {
				buffer[7-j] = '0';
			}
		fwrite(buffer, 1, 8, file);
	}
}

void file_nsl_save(FILE *file, gn_bmp *bitmap, gn_phone *info)
{

	u8 header[] = {'F','O','R','M', 0x01,0xFE,  /* File ID block,      size 1*256+0xFE=510*/
		       'N','S','L','D', 0x01,0xF8}; /* Startup Logo block, size 1*256+0xF8=504*/

	gn_bmp_resize(bitmap, GN_BMP_StartupLogo, info);

        header[4] = (bitmap->size + 6) / 256;
        header[5] = (bitmap->size + 6) % 256;
        header[10] = bitmap->size / 256;
        header[11] = bitmap->size % 256;
	fwrite(header, 1, sizeof(header), file);
	
	fwrite(bitmap->bitmap, 1, bitmap->size, file);
}

void file_ota_save(FILE *file, gn_bmp *bitmap)
{
	char header[] = {0x01,
		         0x00, /* Width */
		         0x00, /* Height */
		         0x01};

	header[1] = bitmap->width;
	header[2] = bitmap->height;

	fwrite(header, 1, sizeof(header), file);

	fwrite(bitmap->bitmap, 1, bitmap->size, file);
}

void file_nlm_save(FILE *file, gn_bmp *bitmap)
{
	char header[] = {'N','L','M', /* Nokia Logo Manager file ID. */
		         0x20,
		         0x01,
		         0x00,        /* 0x00 (OP), 0x01 (CLI), 0x02 (Startup), 0x03 (Picture)*/
		         0x00,
		         0x00,        /* Width. */
		         0x00,        /* Height. */
		         0x01};

	unsigned char buffer[17 * 48];
	int x, y, pos, pos2;
	div_t division;

	switch (bitmap->type) {
	case GN_BMP_OperatorLogo:
	case GN_BMP_NewOperatorLogo:
		header[5] = 0x00;
		break;
	case GN_BMP_CallerLogo:
		header[5] = 0x01;
		break;
	case GN_BMP_StartupLogo:
		header[5] = 0x02;
		break;
	case GN_BMP_PictureMessage:
		header[5] = 0x03;
		break;
	case GN_BMP_WelcomeNoteText:
	case GN_BMP_DealerNoteText:
	case GN_BMP_None:
	default:
		break;
	}

	header[7] = bitmap->width;
	header[8] = bitmap->height;

	pos = 0; pos2 = 7;
	for (y = 0; y < bitmap->height; y++) {
		for (x = 0; x < bitmap->width; x++) {
			if (pos2 == 7) buffer[pos] = 0;
			if (gn_bmp_point(bitmap, x, y)) buffer[pos] |= (1 << pos2);
			pos2--;
			if (pos2 < 0) {pos2 = 7; pos++;} /* going to new line */
		}
		if (pos2 != 7) {pos2 = 7; pos++;} /* for startup logos - new line with new byte */
	}

	division = div(bitmap->width, 8);
	if (division.rem != 0) division.quot++; /* For startup logos */

	fwrite(header, 1, sizeof(header), file);
	fwrite(buffer, 1, (division.quot * bitmap->height), file);
}

gn_error gn_file_bitmap_show(char *filename)
{
	int i, j;
	gn_bmp bitmap;
	gn_error error;

	error = gn_file_bitmap_read(filename, &bitmap, NULL);
	if (error != GN_ERR_NONE)
		return error;

	for (i = 0; i < bitmap.height; i++) {
		for (j = 0; j < bitmap.width; j++) {
			fprintf(stdout, "%c", gn_bmp_point(&bitmap, j, i) ? '#' : ' ');
		}
		fprintf(stdout, "\n");
	}

	return GN_ERR_NONE;
}

/* returns number of the characters before the next delimiter, including it */
static int get_next_token(char *src, int delim)
{
	int i, len = strlen(src);
	int slash_state = 0;

	for (i = 0; i < len; i++) {
		switch (src[i]) {
		case '\\':
			if (slash_state) {
				slash_state = 0;
			} else {
				slash_state = 1;
			}
			break;
		case ';':
			if (slash_state)
				slash_state = 0;
			else
				return i + 1;
			break;
		default:
			if (slash_state)
				slash_state = 0;
			break;
		}
	}
	return i + 1;
}

#define BUG(x) \
	do { \
		if (x) { \
			return GN_ERR_WRONGDATAFORMAT; \
		} \
	} while (0)

#define MAX_INPUT_LINE_LEN 512

#define GET_NEXT_TOKEN()	o = get_next_token(line + offset, ';')
#define STORE_TOKEN(a)		strip_slashes(a, line + offset, sizeof(a) - 1, o - 1)

inline int local_atoi(char *str, int len)
{
	int retval;
	char *aux = strndup(str, len);
	retval = atoi(aux);
	free(aux);
	return retval;
}

GNOKII_API gn_error gn_file_phonebook_raw_parse(gn_phonebook_entry *entry, char *line)
{
	char memory_type_char[3];
	char number[GN_PHONEBOOK_NUMBER_MAX_LENGTH];
	int length, o, offset = 0;
	gn_error error = GN_ERR_NONE;

	memset(entry, 0, sizeof(gn_phonebook_entry));

	length = strlen(line);
	entry->empty = true;
	memory_type_char[2] = 0;

	GET_NEXT_TOKEN();
	switch (o) {
	case 0:
		return GN_ERR_WRONGDATAFORMAT;
	case 1:
		/* empty name: this is a request to delete the entry */
		break;
	default:
		break;
	}
	STORE_TOKEN(entry->name);
	offset += o;

	BUG(offset >= length);

	GET_NEXT_TOKEN();
	switch (o) {
	case 0:
		return GN_ERR_WRONGDATAFORMAT;
	default:
		break;
	}
	STORE_TOKEN(entry->number);
	offset += o;

	BUG(offset >= length);

	GET_NEXT_TOKEN();
	switch (o) {
	case 3:
		break;
	default:
		return GN_ERR_WRONGDATAFORMAT;
	}
	STORE_TOKEN(memory_type_char);
	offset += o;

	BUG(offset >= length);

	entry->memory_type = gn_str2memory_type(memory_type_char);
	/* We can store addressbook entries only in ME or SM (or ON on some models) */
	if (entry->memory_type != GN_MT_ME &&
	    entry->memory_type != GN_MT_SM &&
	    entry->memory_type != GN_MT_ON) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	BUG(offset >= length);

	memset(number, 0, sizeof(number));
	GET_NEXT_TOKEN();
	STORE_TOKEN(number);
	switch (o) {
	case 0:
		return GN_ERR_WRONGDATAFORMAT;
	case 1:
		entry->location = 0;
		break;
	default:
		entry->location = atoi(number);
		break;
	}
	offset += o;

	BUG(offset >= length);

	memset(number, 0, sizeof(number));
	GET_NEXT_TOKEN();
	STORE_TOKEN(number);
	switch (o) {
	case 0:
		return GN_ERR_WRONGDATAFORMAT;
	case 1:
		entry->caller_group = 0;
		break;
	default:
		entry->caller_group = atoi(number);
		break;
	}
	offset += o;
	
	entry->empty = false;

	for (entry->subentries_count = 0; offset < length; entry->subentries_count++) {
		if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) {
			fprintf(stderr, _("Formatting error: too many subentries\n"));
			error = GN_ERR_WRONGDATAFORMAT;
			goto endloop;
		}
		memset(number, 0, sizeof(number));
		GET_NEXT_TOKEN();
		STORE_TOKEN(number);
		switch (o) {
		case 0:
			fprintf(stderr, _("Formatting error: unknown error while reading subentry type\n"));
			error = GN_ERR_WRONGDATAFORMAT;
			goto endloop;
		case 1:
			fprintf(stderr, _("Formatting error: empty entry type\n"));
			entry->subentries[entry->subentries_count].entry_type = 0;
			break;
		default:
			entry->subentries[entry->subentries_count].entry_type = atoi(number);
			break;
		}
		offset += o;

		if (offset > length) {
			fprintf(stderr, _("Formatting error: subentry has only entry type field\n"));
			break;
		}

		memset(number, 0, sizeof(number));
		GET_NEXT_TOKEN();
		STORE_TOKEN(number);
		switch (o) {
		case 0:
			fprintf(stderr, _("Formatting error: unknown error while reading subentry number type\n"));
			error = GN_ERR_WRONGDATAFORMAT;
			goto endloop;
		case 1:
			fprintf(stderr, _("Formatting error: empty number type\n"));
			entry->subentries[entry->subentries_count].number_type = 0;
			/* Number type is required with Number entry type */
			if (entry->subentries[entry->subentries_count].entry_type == GN_PHONEBOOK_ENTRY_Number) {
				error = GN_ERR_WRONGDATAFORMAT;
				goto endloop;
			}
			break;
		default:
			entry->subentries[entry->subentries_count].number_type = atoi(number);
			break;
		}
		offset += o;

		if (offset > length) {
			fprintf(stderr, _("Formatting error: subentry has only entry and number type fields\n"));
			break;
		}

		memset(number, 0, sizeof(number));
		GET_NEXT_TOKEN();
		STORE_TOKEN(number);
		switch (o) {
		case 0:
			fprintf(stderr, _("Formatting error: unknown error while reading subentry id\n"));
			error = GN_ERR_WRONGDATAFORMAT;
			goto endloop;
		case 1:
			fprintf(stderr, _("Formatting error: empty id\n"));
			entry->subentries[entry->subentries_count].id = 0;
			break;
		default:
			entry->subentries[entry->subentries_count].id = atoi(number);
			break;
		}
		offset += o;

		if (offset > length) {
			fprintf(stderr, _("Formatting error: subentry has only entry and number type fields\n"));
			break;
		}

		GET_NEXT_TOKEN();
		switch (entry->subentries[entry->subentries_count].entry_type) {
			case GN_PHONEBOOK_ENTRY_Date:
			case GN_PHONEBOOK_ENTRY_Birthday:
				entry->subentries[entry->subentries_count].data.date.year   = local_atoi(line + offset, 4);
				entry->subentries[entry->subentries_count].data.date.month  = local_atoi(line + offset + 4, 2);
				entry->subentries[entry->subentries_count].data.date.day    = local_atoi(line + offset + 6, 2);
				entry->subentries[entry->subentries_count].data.date.hour   = local_atoi(line + offset + 8, 2);
				entry->subentries[entry->subentries_count].data.date.minute = local_atoi(line + offset + 10, 2);
				entry->subentries[entry->subentries_count].data.date.second = local_atoi(line + offset + 12, 2);
				break;
			case GN_PHONEBOOK_ENTRY_ExtGroup:
				entry->subentries[entry->subentries_count].data.id = local_atoi(line + offset, 3);
				break;
			default:
				STORE_TOKEN(entry->subentries[entry->subentries_count].data.number);
				break;
			}
		switch (o) {
		case 0:
			fprintf(stderr, _("Formatting error: unknown error while reading subentry contents\n"));
			error = GN_ERR_WRONGDATAFORMAT;
			goto endloop;
		case 1:
			fprintf(stderr, _("Formatting error: empty subentry contents\n"));
			break;
		default:
			break;
		}
		offset += o;
	}

endloop:
	/* Fake subentry: this is to send other exports (like from 6110) to 7110 */
	if (!entry->subentries_count) {
		entry->subentries[entry->subentries_count].entry_type   = GN_PHONEBOOK_ENTRY_Number;
		entry->subentries[entry->subentries_count].number_type  = GN_PHONEBOOK_NUMBER_General;
		entry->subentries[entry->subentries_count].id = 2;
		snprintf(entry->subentries[entry->subentries_count].data.number,
			sizeof(entry->subentries[entry->subentries_count].data.number), "%s", entry->number);
		entry->subentries_count = 1;
	}
	return error;
}

GNOKII_API gn_error gn_file_phonebook_raw_write(FILE *f, gn_phonebook_entry *entry, char *memory_type_string)
{
	char escaped_name[2 * GN_PHONEBOOK_NAME_MAX_LENGTH];
	int i;

	add_slashes(escaped_name, entry->name, sizeof(escaped_name), strlen(entry->name));
	fprintf(f, "%s;%s;%s;%d;%d", escaped_name,
		entry->number, memory_type_string,
		entry->location, entry->caller_group);
	if (entry->person.has_person) {
		if (entry->person.honorific_prefixes[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_FormalName,
				entry->person.honorific_prefixes);
		if (entry->person.given_name[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_FirstName,
				entry->person.given_name);
		if (entry->person.family_name[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_LastName,
				entry->person.family_name);
	}
	if (entry->address.has_address) {
		if (entry->address.post_office_box[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_Postal,
				entry->address.post_office_box);
		if (entry->address.extended_address[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_ExtendedAddress,
				entry->address.extended_address);
		if (entry->address.street[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_Street,
				entry->address.street);
		if (entry->address.city[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_City,
				entry->address.city);
		if (entry->address.state_province[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_StateProvince,
				entry->address.state_province);
		if (entry->address.zipcode[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_ZipCode,
				entry->address.zipcode);
		if (entry->address.country[0])
			fprintf(f, ";%d;0;0;%s", GN_PHONEBOOK_ENTRY_Country,
				entry->address.country);
	}

	for (i = 0; i < entry->subentries_count; i++) {
		switch (entry->subentries[i].entry_type) {
		case GN_PHONEBOOK_ENTRY_Birthday:
		case GN_PHONEBOOK_ENTRY_Date:
			fprintf(f, ";%d;0;%d;%04u%02u%02u%02u%02u%02u", entry->subentries[i].entry_type, entry->subentries[i].id,
				entry->subentries[i].data.date.year, entry->subentries[i].data.date.month, entry->subentries[i].data.date.day,
				entry->subentries[i].data.date.hour, entry->subentries[i].data.date.minute, entry->subentries[i].data.date.second);
			break;
		case GN_PHONEBOOK_ENTRY_ExtGroup:
			fprintf(f, ";%d;%d;%d;%03d",
				entry->subentries[i].entry_type,
				entry->subentries[i].number_type,
				entry->subentries[i].id,
				entry->subentries[i].data.id);
			break;
		default:
			add_slashes(escaped_name, entry->subentries[i].data.number, sizeof(escaped_name), strlen(entry->subentries[i].data.number));
			fprintf(f, ";%d;%d;%d;%s",
				entry->subentries[i].entry_type,
				entry->subentries[i].number_type,
				entry->subentries[i].id,
				escaped_name);
			break;
		}
	}
	if ((entry->memory_type == GN_MT_MC ||
		entry->memory_type == GN_MT_DC ||
		entry->memory_type == GN_MT_RC) &&
		entry->date.day != 0)
		fprintf(f, ";%d;0;0;%04u%02u%02u%02u%02u%02u", GN_PHONEBOOK_ENTRY_Date,
			entry->date.year, entry->date.month, entry->date.day, entry->date.hour, entry->date.minute, entry->date.second);
	fprintf(f, "\n");
	return GN_ERR_NONE;
}
