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

  Functions for common bitmap operations.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "gsm-common.h"
#include "gsm-bitmaps.h"
#include "gsm-api.h"

/* A few useful functions for bitmaps */

void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	if (bmp->type == GSM_StartupLogo) bmp->bitmap[((y/8)*bmp->width)+x] |= 1 << (y%8);
	if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo) bmp->bitmap[(y*bmp->width+x)/8] |= 1 << (7-((y*bmp->width+x)%8));

	/* Testing only! */
	if (bmp->type == GSM_PictureImage) bmp->bitmap[9*y + (x/8)] |= 1 << (7-(x%8));
}

void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	if (bmp->type == GSM_StartupLogo) bmp->bitmap[((y/8)*bmp->width)+x] &= 255 - (1 << (y%8));
	if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo)
		bmp->bitmap[(y*bmp->width+x)/8] &= 255 - (1 << (7-((y*bmp->width+x)%8)));

	/* Testing only ! */
	if (bmp->type == GSM_PictureImage) bmp->bitmap[9*y + (x/8)] &= 255 - (1 << (7-(x%8)));
}

bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	int i = 0;


	if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo)
		i = (bmp->bitmap[(y*bmp->width+x)/8] & 1 << (7-((y*bmp->width+x)%8)));

	if (bmp->type == SMS_Picture) 
		i = (bmp->bitmap[9 * y + (x / 8)] & 1 << (7 - (x % 8)));

	if ((bmp->type == GSM_StartupLogo) || (bmp->type == GSM_NewOperatorLogo))
		i = (bmp->bitmap[((y/8)*bmp->width) + x] & 1<<((y%8)));

	if (i) return true;
	else return false;
}

void GSM_ClearBitmap(GSM_Bitmap *bmp)
{
	int i;

	for (i = 0; i < bmp->size; i++) bmp->bitmap[i] = 0;
}


void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target, GSM_Information *info)
{
	GSM_Bitmap backup;
	int x, y, copywidth, copyheight;

	/* Copy into the backup */
	memcpy(&backup, bitmap, sizeof(GSM_Bitmap));

	if (target == GSM_StartupLogo) {
		bitmap->width = info->StartupLogoW;
		bitmap->height = info->StartupLogoH;
		bitmap->size = ((bitmap->height / 8) + (bitmap->height % 8 > 0)) * bitmap->width;
	}
	if (target == GSM_OperatorLogo) {
		bitmap->width = info->OpLogoW;
		bitmap->height = info->OpLogoH;
		x = bitmap->width * bitmap->height;
		bitmap->size = (x / 8) + (x % 8 > 0);
	}
	if (target == GSM_CallerLogo) {
		bitmap->width = info->CallerLogoW;
		bitmap->height = info->CallerLogoH;
		x = bitmap->width * bitmap->height;
		bitmap->size = (x / 8) + (x % 8 > 0);
	}
	if (target == GSM_PictureImage) {
		bitmap->width = 72;
		bitmap->height = 48;
		bitmap->size = bitmap->width * bitmap->height / 8;
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

	GSM_ClearBitmap(bitmap);

	for (y = 0; y < copyheight; y++) {
		for (x = 0; x < copywidth; x++)
			if (GSM_IsPointBitmap(&backup, x, y)) GSM_SetPointBitmap(bitmap, x, y);
	}
}

void GSM_PrintBitmap(GSM_Bitmap *bitmap)
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


GSM_Error GSM_ReadSMSBitmap(int type, char *message, char *code, GSM_Bitmap *bitmap)
{
	int offset = 0;

	bitmap->type = type;
	switch (type) {
	case SMS_OpLogo:
		if (!code) return GE_UNKNOWN;

		bitmap->netcode[0] = '0' + (message[0] & 0x0f);
		bitmap->netcode[1] = '0' + (message[0] >> 4);
		bitmap->netcode[2] = '0' + (message[1] & 0x0f);
		bitmap->netcode[3] = ' ';
		bitmap->netcode[4] = '0' + (message[2] & 0x0f);
		bitmap->netcode[5] = '0' + (message[2] >> 4);
		bitmap->netcode[6] = 0;

		break;
	case SMS_CallerIDLogo:
		break;
	case SMS_Picture:
		offset = 2;
		break;
	default: /* error */
		return GE_UNKNOWN;
	}
	bitmap->width = message[0];
	bitmap->height = message[1];

	bitmap->size = (bitmap->width * bitmap->height) / 8;
	memcpy(bitmap->bitmap, message + offset + 2, bitmap->size);

	dprintf("Bitmap from SMS: width %i, height %i\n", bitmap->width, bitmap->height);

	return GE_NONE;
}


/* Returns message length */
int GSM_EncodeSMSBitmap(GSM_Bitmap *bitmap, char *message)
{
	unsigned short size, current = 0;

	switch (bitmap->type) {
	case SMS_OpLogo:
		/* Set the network code */
		dprintf("Operator Logo\n");
		message[current++] = ((bitmap->netcode[1] & 0x0f) << 4) | (bitmap->netcode[0] & 0xf);
		message[current++] = 0xf0 | (bitmap->netcode[2] & 0x0f);
		message[current++] = ((bitmap->netcode[5] & 0x0f) << 4) | (bitmap->netcode[4] & 0xf);
		break;
	case SMS_MultipartMessage:
		dprintf("Picture Image\n");
		/* Type of multipart message - picture image */
		message[current++] = 0x02;
		/* Size of the message */
		size = bitmap->size + 4; /* for bitmap the header */
		message[current++] = (size & 0xff00) >> 8;
		message[current++] = size & 0x00ff;
		break;
	default: /* error */
		dprintf("gulp?\n");
		break;
	}

	/* Info field */
	message[current++] = 0x00;

	/* Logo size */
	message[current++] = bitmap->width;
	message[current++] = bitmap->height;

	/* Number of colors */
	message[current++] = 0x01;

	memcpy(message + current, bitmap->bitmap, bitmap->size);

	return (current + bitmap->size);
}
