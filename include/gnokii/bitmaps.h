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

#ifndef __gsm_bitmaps_h__
#define __gsm_bitmaps_h__

#include "gsm-error.h"

/* Bitmap types. */

typedef enum {
	GSM_None = 0,
	GSM_StartupLogo,
	GSM_PictureImage,
	GSM_OperatorLogo,
	GSM_CallerLogo,
	GSM_WelcomeNoteText,
	GSM_DealerNoteText,
	GSM_NewOperatorLogo
} GSM_Bitmap_Types;

#define GSM_MAX_BITMAP_SIZE 864

/* Structure to hold incoming/outgoing bitmaps (and welcome-notes). */

typedef struct {
	u8 height;               /* Bitmap height (pixels) */
	u8 width;                /* Bitmap width (pixels) */
	u16 size;                /* Bitmap size (bytes) */
	GSM_Bitmap_Types type;   /* Bitmap type */
	char netcode[7];         /* Network operator code */
	char text[256];          /* Text used for welcome-note or callergroup name */
	char dealertext[256];    /* Text used for dealer welcome-note */
	bool dealerset;          /* Is dealer welcome-note set now ? */
	unsigned char bitmap[GSM_MAX_BITMAP_SIZE]; /* Actual Bitmap */
	char number;             /* Caller group number */
	char ringtone;           /* Ringtone no sent with caller group */
} GSM_Bitmap;

void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y);
void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y);
bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y);
void GSM_ClearBitmap(GSM_Bitmap *bmp);
void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target, GSM_Information *info);
void GSM_PrintBitmap(GSM_Bitmap *bitmap);

/* SMS bitmap functions */
int GSM_EncodeSMSBitmap(GSM_Bitmap *bitmap, char *message);
GSM_Error GSM_ReadSMSBitmap(int type, char *message, char *code, GSM_Bitmap *bitmap);

#endif
