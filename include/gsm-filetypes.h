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
  Copyright (C) 2001-2003 Pawel Kot, BORBELY Zoltan
  Copyright (C) 2002      Markus Plail
  Copyright (C) 2003      Tomi Ollila

  Functions to read and write common file types.

*/

#ifndef _gnokii_gsm_filetypes_h
#define _gnokii_gsm_filetypes_h

/* Defines the character that separates fields in rtttl files. */
#define RTTTL_SEP ":"

typedef enum {
	GN_FT_None = 0,
	GN_FT_NOL,
	GN_FT_NGG,
	GN_FT_NSL,
	GN_FT_NLM,
	GN_FT_BMP,
	GN_FT_OTA,
	GN_FT_XPMF,
	GN_FT_RTTTL,
	GN_FT_OTT,
	GN_FT_MIDI,
	GN_FT_NOKRAW_TONE
} gn_filetypes;

#endif /* _gnokii_gsm_filetypes_h */
