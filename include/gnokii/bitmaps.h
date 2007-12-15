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
  Copyright (C) 2000      Marcin Wiacek, Chris Kemp
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002      Pavel Machek

  Functions for common bitmap operations.

*/

#ifndef _gnokii_bitmaps_h
#define _gnokii_bitmaps_h

#include <stdio.h>

#include <gnokii/error.h>
#include <gnokii/common.h>

/* Bitmap types. These do *not* correspond to headers[] in gsm-sms.c. */

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
	GN_BMP_EMSAnimation,	/* First bitmap in animation should have this type */
	GN_BMP_EMSAnimation2,	/* ...second, third and fourth should have this type */
} gn_bmp_types;

#define GN_BMP_MAX_SIZE 1000

#define MCC_MNC_SIZE	8

/* Structure to hold incoming/outgoing bitmaps (and welcome-notes). */
typedef struct {
	unsigned char height;    /* Bitmap height (pixels) */
	unsigned char width;     /* Bitmap width (pixels) */
	unsigned int size;       /* Bitmap size (bytes) */
	gn_bmp_types type;       /* Bitmap type */
	char netcode[MCC_MNC_SIZE]; /* Network operator code */
	char text[256];          /* Text used for welcome-note or callergroup name */
	char dealertext[256];    /* Text used for dealer welcome-note */
	int dealerset;           /* Is dealer welcome-note set now ? */
	unsigned char bitmap[GN_BMP_MAX_SIZE]; /* Actual Bitmap */
	char number;             /* Caller group number */
	int ringtone;            /* Ringtone no sent with caller group */
	unsigned char ringtone_id[6];
} gn_bmp;

GNOKII_API gn_error gn_file_bitmap_read(char *filename, gn_bmp *bitmap, gn_phone *info);
GNOKII_API gn_error gn_file_bitmap_save(char *filename, gn_bmp *bitmap, gn_phone *info);
GNOKII_API gn_error gn_file_bitmap_show(char *filename);

GNOKII_API gn_error gn_bmp_null(gn_bmp *bmp, gn_phone *info);
GNOKII_API void gn_bmp_point_set(gn_bmp *bmp, int x, int y);
GNOKII_API void gn_bmp_point_clear(gn_bmp *bmp, int x, int y);
GNOKII_API int  gn_bmp_point(gn_bmp *bmp, int x, int y);
GNOKII_API void gn_bmp_clear(gn_bmp *bmp);
GNOKII_API void gn_bmp_resize(gn_bmp *bitmap, gn_bmp_types target, gn_phone *info);
GNOKII_API void gn_bmp_print(gn_bmp *bitmap, FILE *f);

GNOKII_API int gn_bmp_sms_encode(gn_bmp *bitmap, unsigned char *message);
GNOKII_API gn_error gn_bmp_sms_read(int type, unsigned char *message, unsigned char *code, gn_bmp *bitmap);

#endif /* _gnokii_bitmaps_h */
