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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2002 	   Pavel Machek <pavel@ucw.cz>

  Functions for common bitmap operations.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-bitmaps.h"
#include "gsm-api.h"

/* A few useful functions for bitmaps */

GSM_Error GSM_NullBitmap(GSM_Bitmap *bmp, GSM_Information *info)
{
	if (!bmp || !info) return GE_INTERNALERROR;
	strcpy(bmp->netcode, "000 00");
	bmp->width = info->OpLogoW;
	bmp->height = info->OpLogoH;
	bmp->size = ceiling_to_octet(bmp->width * bmp->height);
	GSM_ClearBitmap(bmp);
	return GE_NONE;
}

API void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	switch (bmp->type) {
	case GSM_StartupLogo:  bmp->bitmap[((y/8)*bmp->width)+x] |= 1 << (y%8); break;
	case GSM_OperatorLogo:
	case GSM_CallerLogo:   bmp->bitmap[(y*bmp->width+x)/8] |= 1 << (7-((y*bmp->width+x)%8)); break;
		               /* Testing only! */	
	case GSM_PictureImage: bmp->bitmap[9*y + (x/8)] |= 1 << (7-(x%8)); break;
	}
}

API void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	switch (bmp->type) {
	case GSM_StartupLogo:  bmp->bitmap[((y/8)*bmp->width)+x] &= ~(1 << (y%8)); break;
	case GSM_OperatorLogo:
	case GSM_CallerLogo:   bmp->bitmap[(y*bmp->width+x)/8] &= ~(1 << (7-((y*bmp->width+x)%8))); break;
		               /* Testing only! */	
	case GSM_PictureImage: bmp->bitmap[9*y + (x/8)] &= ~(1 << (7-(x%8))); break;
	}
}

API bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	int i = 0;

	switch (bmp->type) {
	case GSM_OperatorLogo:
	case GSM_CallerLogo:
		i = (bmp->bitmap[(y*bmp->width+x)/8] & 1 << (7-((y*bmp->width+x)%8)));
		break;
	case GSM_PictureImage:
		i = (bmp->bitmap[9 * y + (x / 8)] & 1 << (7 - (x % 8)));
		break;
	case GSM_StartupLogo:
	case GSM_NewOperatorLogo:
		i = (bmp->bitmap[((y/8)*bmp->width) + x] & 1<<((y%8)));
		break;
	default:
		break;
	}
	return ((i == 0) ? false : true);
}

API void GSM_ClearBitmap(GSM_Bitmap *bmp)
{
	memset(bmp->bitmap, 0, bmp->size);
}


API void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target, GSM_Information *info)
{
	GSM_Bitmap backup;
	int x, y, copywidth, copyheight;

	/* Copy into the backup */
	memcpy(&backup, bitmap, sizeof(GSM_Bitmap));

	switch (target) {
	case GSM_StartupLogo:
		bitmap->width = info->StartupLogoW;
		bitmap->height = info->StartupLogoH;
		break;
	case GSM_OperatorLogo:
		bitmap->width = info->OpLogoW;
		bitmap->height = info->OpLogoH;
		break;
	case GSM_CallerLogo:
		bitmap->width = info->CallerLogoW;
		bitmap->height = info->CallerLogoH;
		break;
	case GSM_PictureImage:
		bitmap->width = 72;
		bitmap->height = 48;
		break;
	default:
		bitmap->width = 0;
		bitmap->height = 0;
		break;
	}
	bitmap->type = target;
	bitmap->size = ceiling_to_octet(bitmap->height * bitmap->width);

	if (backup.width > bitmap->width) {
		copywidth = bitmap->width;
		dprintf("We lost some part of image - it's cut (width from %i to %i) !\n", backup.width, bitmap->width);
	} else copywidth = backup.width;

	if (backup.height > bitmap->height) {
		copyheight = bitmap->height;
		dprintf("We lost some part of image - it's cut (height from %i to %i) !\n", backup.height, bitmap->height);
	} else copyheight = backup.height;

	GSM_ClearBitmap(bitmap);

	for (y = 0; y < copyheight; y++) {
		for (x = 0; x < copywidth; x++)
			if (GSM_IsPointBitmap(&backup, x, y)) GSM_SetPointBitmap(bitmap, x, y);
	}
}

API void GSM_PrintBitmap(GSM_Bitmap *bitmap)
{
	int x, y;

	for (y = 0; y < bitmap->height; y++) {
		for (x = 0; x < bitmap->width; x++) {
			if (GSM_IsPointBitmap(bitmap, x, y)) {
				fprintf(stdout, "#");
			} else {
				fprintf(stdout, " ");
			}
		}
		fprintf(stdout, "\n");
	}
}


API GSM_Error GSM_ReadSMSBitmap(int type, char *message, char *code, GSM_Bitmap *bitmap)
{
	int offset = 0;

	bitmap->type = type;
	switch (type) {
	case GSM_PictureImage:
		offset = 2;
		break;
	case GSM_OperatorLogo:
		if (!code) return GE_UNKNOWN;

		bitmap->netcode[0] = '0' + (message[0] & 0x0f);
		bitmap->netcode[1] = '0' + (message[0] >> 4);
		bitmap->netcode[2] = '0' + (message[1] & 0x0f);
		bitmap->netcode[3] = ' ';
		bitmap->netcode[4] = '0' + (message[2] & 0x0f);
		bitmap->netcode[5] = '0' + (message[2] >> 4);
		bitmap->netcode[6] = 0;

		break;
	case GSM_CallerLogo:
		break;
	default: /* error */
		return GE_UNKNOWN;
	}
	bitmap->width = message[0];
	bitmap->height = message[1];
	dprintf("offset: %i\n", offset);

	bitmap->size = ceiling_to_octet(bitmap->width * bitmap->height);
	memcpy(bitmap->bitmap, message + offset + 2, bitmap->size);

	dprintf("Bitmap from SMS: width %i, height %i\n", bitmap->width, bitmap->height);

	return GE_NONE;
}


/* Returns message length */
int GSM_EncodeSMSBitmap(GSM_Bitmap *bitmap, char *message)
{
	unsigned short size, current = 0;

	switch (bitmap->type) {
	case GSM_OperatorLogo:
		/* Set the network code */
		dprintf("Operator Logo\n");
		message[current++] = ((bitmap->netcode[1] & 0x0f) << 4) | (bitmap->netcode[0] & 0xf);
		message[current++] = 0xf0 | (bitmap->netcode[2] & 0x0f);
		message[current++] = ((bitmap->netcode[5] & 0x0f) << 4) | (bitmap->netcode[4] & 0xf);
		break;
	case GSM_PictureImage:
		dprintf("Picture Image\n");
		/* Type of multipart message - picture image */
		message[current++] = 0x02;
		/* Size of the message */
		size = bitmap->size + 4; /* for bitmap the header */
		message[current++] = (size & 0xff00) >> 8;
		message[current++] = size & 0x00ff;
		break;
	case GSM_EMSPicture:
		dprintf("EMS picture\n");
		if (bitmap->width % 8) {
			fprintf(stderr, "EMS needs bitmap size 8, 16, 24, ... \n");
			return GE_NOTSUPPORTED;
		}
		message[current++] = bitmap->width/8; /* Horizontal size / 8 */
		message[current++] = bitmap->height;
		break;
	default: /* error */
		dprintf("gulp?\n");
		break;
	}

	switch (bitmap->type) {
	case GSM_EMSPicture:
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

	return (current + bitmap->size);
}
