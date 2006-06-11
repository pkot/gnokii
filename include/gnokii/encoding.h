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

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2001-2004 Pawel Kot

  Include file for encoding functions.

*/

#ifndef _gnokii_encoding_h
#define _gnokii_encoding_h

GNOKII_API int gn_char_def_alphabet(unsigned char *string);

/*
 *
 * Set encoding to use for exchanging data between Gnokii and the application
 *
 * Normally, when communicating with the application, Gnokii uses the
 * encoding set with the setlocale() call. For the (standard) call
 * setlocale(LC_ALL, ""), this is the encoding specified by the user in the
 * LANG variable. This function can be used to override this behaviour. E.g.,
 * to make Gnokii always interpret and return string values using the UTF-8
 * charset, call gn_set_encoding("UTF-8").
 * 
 * This has nothing to do with the encoding used to communicate with the
 * phone itself, which is a totally different issue.
 *
 * Parameters:
 *     encoding     The name of the encoding (length must not exceed 63 chars)
 *
 */
GNOKII_API void gn_char_set_encoding(const char* encoding);
GNOKII_API const char *gn_char_get_encoding();

#endif /* _gnokii_encoding_h */
