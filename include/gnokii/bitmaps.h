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
  Copyrigth (c) 2002 Pawe³ Kot

  Functions for common bitmap operations.

*/

#ifndef _gnokii_gsm_bitmaps_h
#define _gnokii_gsm_bitmaps_h

#include "misc.h"
#include "gsm-error.h"
#include "gsm-common.h"

/* Bitmap types. These do *not* corespond to headers[] in gsm-sms.c. */

typedef enum {
	GN_BMP_None = 0,
	GN_BMP_StartupLogo = 50,
	GN_BMP_PictureMessage,
	GN_BMP_OperatorLogo,
	GN_BMP_CallerLogo,
	GN_BMP_WelcomeNoteText,
	GN_BMP_DealerNoteText,
	GN_BMP_NewOperatorLogo,
	GN_BMP_EMSPicture,
	GN_BMP_EMSAnimation,		/* First bitmap in animation should have this type */
	GN_BMP_EMSAnimation2,		/* ...second, third and fourth should have this type */
} gn_bmp_types;

#define GN_BMP_MAX_SIZE 1000

/* Structure to hold incoming/outgoing bitmaps (and welcome-notes). */

typedef struct {
	u8 height;               /* Bitmap height (pixels) */
	u8 width;                /* Bitmap width (pixels) */
	u16 size;                /* Bitmap size (bytes) */
	gn_bmp_types type;       /* Bitmap type */
	char netcode[7];         /* Network operator code */
	char text[256];          /* Text used for welcome-note or callergroup name */
	char dealertext[256];    /* Text used for dealer welcome-note */
	bool dealerset;          /* Is dealer welcome-note set now ? */
	unsigned char bitmap[GN_BMP_MAX_SIZE]; /* Actual Bitmap */
	char number;             /* Caller group number */
	char ringtone;           /* Ringtone no sent with caller group */
} gn_bmp;

API GSM_Error gn_bmp_null(gn_bmp *bmp, GSM_Information *info);
API void gn_bmp_set_point(gn_bmp *bmp, int x, int y);
API void gn_bmp_clear_point(gn_bmp *bmp, int x, int y);
API bool gn_bmp_is_point(gn_bmp *bmp, int x, int y);
API void gn_bmp_clear(gn_bmp *bmp);
API void gn_bmp_resize(gn_bmp *bitmap, gn_bmp_types target, GSM_Information *info);
API void gn_bmp_print(gn_bmp *bitmap, FILE *f);

/* SMS bitmap functions */
API int gn_bmp_encode_sms(gn_bmp *bitmap, unsigned char *message);
API GSM_Error gn_bmp_read_sms(int type, unsigned char *message, unsigned char *code, gn_bmp *bitmap);

#endif /* _gnokii_gsm_bitmaps_h */
