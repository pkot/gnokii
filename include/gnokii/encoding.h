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

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Include file for encoding functions.

*/

#ifndef _gnokii_gsm_encoding_h
#define _gnokii_gsm_encoding_h

#include "gsm-common.h"

extern void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len);
extern void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len);

int char_unpack_7bit(unsigned int offset, unsigned int in_length, unsigned int out_length,
		     unsigned char *input, unsigned char *output);
int char_pack_7bit(unsigned int offset, unsigned char *input, unsigned char *output,
		   unsigned int *in_len);

unsigned int char_decode_unicode(unsigned char* dest, const unsigned char* src, int len);
unsigned int char_encode_unicode(unsigned char* dest, const unsigned char* src, int len);

void char_decode_ascii(unsigned char* dest, const unsigned char* src, int len);
unsigned int char_encode_ascii(unsigned char* dest, const unsigned char* src, unsigned int len);

void char_decode_hex(unsigned char* dest, const unsigned char* src, int len);
void char_encode_hex(unsigned char* dest, const unsigned char* src, int len);

void char_decode_ucs2(unsigned char* dest, const unsigned char* src, int len);
void char_encode_ucs2(unsigned char* dest, const unsigned char* src, int len);

API bool gn_char_def_alphabet(unsigned char *string);

extern char *char_get_bcd_number(u8 *Number);
extern int char_semi_octet_pack(char *Number, unsigned char *Output, SMS_NumberType type);

#endif /* _gnokii_gsm_encoding_h */
