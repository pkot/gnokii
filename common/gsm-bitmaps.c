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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000       Marcin Wiacek
  Copyright (C) 2000-2001  Chris Kemp
  Copyright (C) 2001-2002  Markus Plail
  Copyright (C) 2001-2003  Pawel Kot
  Copyright (C) 2002       Pavel Machek <pavel@ucw.cz>

  Based on work from mygnokii, thanks go to Marcin Wiacek.

  Functions for common bitmap operations.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"

/* A few useful functions for bitmaps */

GNOKII_API gn_error gn_bmp_null(gn_bmp *bmp, gn_phone *info)
{
	if (!bmp || !info)
		return GN_ERR_INTERNALERROR;
	snprintf(bmp->netcode, sizeof(bmp->netcode), "000 00");
	bmp->width = info->operator_logo_width;
	bmp->height = info->operator_logo_height;
	bmp->size = ceiling_to_octet(bmp->width * bmp->height);
	gn_bmp_clear(bmp);
	return GN_ERR_NONE;
}

GNOKII_API void gn_bmp_point_set(gn_bmp *bmp, int x, int y)
{
	switch (bmp->type) {
	case GN_BMP_NewOperatorLogo:
	case GN_BMP_StartupLogo:
		bmp->bitmap[((y / 8) * bmp->width) + x] |= 1 << (y % 8);
		break;
	/* Testing only! */	
	case GN_BMP_PictureMessage:
		bmp->bitmap[9 * y + (x / 8)] |= 1 << (7 - (x % 8));
		break;
	case GN_BMP_EMSPicture:
	case GN_BMP_EMSAnimation:
	case GN_BMP_EMSAnimation2:
	case GN_BMP_OperatorLogo:
	case GN_BMP_CallerLogo:
	default:
		bmp->bitmap[(y * bmp->width + x) / 8] |= 1 << (7 - ((y * bmp->width + x) % 8));
		break;
	}
}

GNOKII_API void gn_bmp_point_clear(gn_bmp *bmp, int x, int y)
{
	switch (bmp->type) {
	case GN_BMP_StartupLogo:
	case GN_BMP_NewOperatorLogo:
		bmp->bitmap[((y / 8) * bmp->width) + x] &= ~(1 << (y % 8));
		break;
	/* Testing only! */	
	case GN_BMP_PictureMessage:
		bmp->bitmap[9 * y + (x / 8)] &= ~(1 << (7 - (x % 8)));
		break;
	case GN_BMP_EMSPicture:
	case GN_BMP_EMSAnimation:
	case GN_BMP_EMSAnimation2:
	case GN_BMP_OperatorLogo:
	case GN_BMP_CallerLogo:
	default:
		bmp->bitmap[(y*bmp->width+x)/8] &= ~(1 << (7-((y*bmp->width+x)%8)));
		break;
	}
}

GNOKII_API int gn_bmp_point(gn_bmp *bmp, int x, int y)
{
	int i = 0;

	switch (bmp->type) {
	case GN_BMP_OperatorLogo:
	case GN_BMP_CallerLogo:
		i = (bmp->bitmap[(y * bmp->width + x) / 8] & 1 << (7-((y * bmp->width + x) % 8)));
		break;
	case GN_BMP_PictureMessage:
		i = (bmp->bitmap[9 * y + (x / 8)] & 1 << (7 - (x % 8)));
		break;
	case GN_BMP_StartupLogo:
	case GN_BMP_NewOperatorLogo:
		i = (bmp->bitmap[((y / 8) * bmp->width) + x] & 1 << (y % 8));
		break;
	default:
		/* Let's guess */
		i = (bmp->bitmap[(y * bmp->width + x) / 8] & 1 << (7-((y * bmp->width + x) % 8)));
		break;
	}
	return ((i == 0) ? false : true);
}

GNOKII_API void gn_bmp_clear(gn_bmp *bmp)
{
	if (bmp) memset(bmp->bitmap, 0, (bmp->size > GN_BMP_MAX_SIZE) ? GN_BMP_MAX_SIZE : bmp->size);
}

GNOKII_API void gn_bmp_resize(gn_bmp *bitmap, gn_bmp_types target, gn_phone *info)
{
	gn_bmp backup;
	int x, y, copywidth, copyheight;

	/* Copy into the backup */
	memcpy(&backup, bitmap, sizeof(gn_bmp));

	switch (target) {
	case GN_BMP_StartupLogo:
		bitmap->width = info->startup_logo_width;
		bitmap->height = info->startup_logo_height;
		if (info->models && ((!strncmp(info->models, "6510", 4)) || (!strncmp(info->models, "7110", 4))))
			bitmap->size = ceiling_to_octet(bitmap->height) * bitmap->width;
		else 
			bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);
		break;
	case GN_BMP_NewOperatorLogo:
		bitmap->width = info->operator_logo_width;
		bitmap->height = info->operator_logo_height;
		bitmap->size = ceiling_to_octet(bitmap->height) * bitmap->width;
		break;
	case GN_BMP_OperatorLogo:
		bitmap->width = info->operator_logo_width;
		bitmap->height = info->operator_logo_height;
		bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);
		break;
	case GN_BMP_CallerLogo:
		bitmap->width = info->caller_logo_width;
		bitmap->height = info->caller_logo_height;
		bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);
		break;
	case GN_BMP_PictureMessage:
		bitmap->width = 72;
		bitmap->height = 48;
		bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);
		break;
	default:
		bitmap->width = 0;
		bitmap->height = 0;
		bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);
		break;
	}
	bitmap->type = target;

	if (backup.width > bitmap->width) {
		copywidth = bitmap->width;
		dprintf("We lost some part of image - it's cut (width from %i to %i) !\n", backup.width, bitmap->width);
	} else copywidth = backup.width;

	if (backup.height > bitmap->height) {
		copyheight = bitmap->height;
		dprintf("We lost some part of image - it's cut (height from %i to %i) !\n", backup.height, bitmap->height);
	} else copyheight = backup.height;

	gn_bmp_clear(bitmap);

	for (y = 0; y < copyheight; y++) {
		for (x = 0; x < copywidth; x++)
			if (gn_bmp_point(&backup, x, y)) gn_bmp_point_set(bitmap, x, y);
	}
}

GNOKII_API void gn_bmp_print(gn_bmp *bitmap, FILE *f)
{
	int x, y;

	for (y = 0; y < bitmap->height; y++) {
		for (x = 0; x < bitmap->width; x++) {
			if (gn_bmp_point(bitmap, x, y)) {
				fprintf(f, "#");
			} else {
				fprintf(f, " ");
			}
		}
		fprintf(f, "\n");
	}
}


GNOKII_API gn_error gn_bmp_sms_read(int type, unsigned char *message, unsigned char *code, gn_bmp *bitmap)
{
	int offset = 0;

	bitmap->type = type;
	switch (type) {
	case GN_BMP_PictureMessage:
		offset = 2;
		break;
	case GN_BMP_OperatorLogo:
		if (!code) return GN_ERR_UNKNOWN;

		bitmap->netcode[0] = '0' + (message[0] & 0x0f);
		bitmap->netcode[1] = '0' + (message[0] >> 4);
		bitmap->netcode[2] = '0' + (message[1] & 0x0f);
		bitmap->netcode[3] = ' ';
		bitmap->netcode[4] = '0' + (message[2] & 0x0f);
		bitmap->netcode[5] = '0' + (message[2] >> 4);
		bitmap->netcode[6] = 0;

		break;
	case GN_BMP_CallerLogo:
		break;
	default: /* error */
		return GN_ERR_UNKNOWN;
	}
	bitmap->width = message[0];
	bitmap->height = message[1];

	bitmap->size = ceiling_to_octet(bitmap->width * bitmap->height);
	memcpy(bitmap->bitmap, message + offset + 2, bitmap->size);

	dprintf("Bitmap from SMS: width %i, height %i\n", bitmap->width, bitmap->height);

	return GN_ERR_NONE;
}


/* Returns message length */
GNOKII_API int gn_bmp_sms_encode(gn_bmp *bitmap, unsigned char *message)
{
	unsigned int current = 0;

	switch (bitmap->type) {
	case GN_BMP_OperatorLogo:
		/* Set the network code */
		dprintf("Operator Logo\n");
		message[current++] = ((bitmap->netcode[1] & 0x0f) << 4) | (bitmap->netcode[0] & 0xf);
		message[current++] = 0xf0 | (bitmap->netcode[2] & 0x0f);
		message[current++] = ((bitmap->netcode[5] & 0x0f) << 4) | (bitmap->netcode[4] & 0xf);
		break;
	case GN_BMP_PictureMessage:
		dprintf("Picture Image\n");

		/* Set the logo size */
		message[current++] = 0x00;
		message[current++] = bitmap->width;
		message[current++] = bitmap->height;
		message[current++] = 0x01;

		memcpy(message + current, bitmap->bitmap, bitmap->size);
		current = current + bitmap->size;
		return current;
	case GN_BMP_EMSPicture:
		dprintf("EMS picture\n");
		if (bitmap->width % 8) {
			dprintf("EMS needs bitmap size 8, 16, 24, ... \n");
			return GN_ERR_NOTSUPPORTED;
		}
		message[current++] = bitmap->width / 8 * bitmap->height + 5;
		message[current++] = 0x12;              /* Picture code */
		message[current++] = bitmap->width / 8 * bitmap->height + 3; /* Picture size */;
		message[current++] = 0;		        /* Position in text this picture is at */
		message[current++] = bitmap->width / 8; /* Horizontal size / 8 */
		message[current++] = bitmap->height;
		break;
	case GN_BMP_EMSAnimation:
		dprintf("EMS animation\n");
		message[current++] = 128 + 3;
		message[current++] = 0x0e; 	/* Animation code */
		message[current++] = 128 + 1;   /* Picture size */;
		message[current++] = 0x00; 	/* Position where to display */
	case GN_BMP_EMSAnimation2:
		dprintf("(without header)\n");
		if (bitmap->width != 16) {
			dprintf("EMS animation needs bitmap 16x16 ... \n");
			return GN_ERR_NOTSUPPORTED;
		}
		break;
	default: /* error */
		dprintf("gulp?\n");
		break;
	}

	switch (bitmap->type) {
	case GN_BMP_EMSPicture:
	case GN_BMP_EMSAnimation:
	case GN_BMP_EMSAnimation2:
		break;
	default:			/* Add common nokia headers */
		/* Info field */
		message[current++] = 0x00;

		/* Logo size */
		message[current++] = bitmap->width;
		message[current++] = bitmap->height;

		/* Number of colors */
		message[current++] = 0x01;
	}

	memcpy(message + current, bitmap->bitmap, bitmap->size);
	current += bitmap->size;

	return current;
}
