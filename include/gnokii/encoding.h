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

#ifndef __gsm_encoding_h_
#define __gsm_encoding_h_

#include "gsm-common.h"

extern void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len);
extern void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len);

int Unpack7BitCharacters(int offset, int in_length, int out_length,
			unsigned char *input, unsigned char *output);
int Pack7BitCharacters(int offset, unsigned char *input, unsigned char *output);

void DecodeUnicode (unsigned char* dest, const unsigned char* src, int len);
void EncodeUnicode (unsigned char* dest, const unsigned char* src, int len);

void DecodeAscii (unsigned char* dest, const unsigned char* src, int len);
void EncodeAscii (unsigned char* dest, const unsigned char* src, int len);

void DecodeHex (unsigned char* dest, const unsigned char* src, int len);
void EncodeHex (unsigned char* dest, const unsigned char* src, int len);

void DecodeUCS2 (unsigned char* dest, const unsigned char* src, int len);
void EncodeUCS2 (unsigned char* dest, const unsigned char* src, int len);

extern char *GetBCDNumber(u8 *Number, int maxlen);
extern int SemiOctetPack(char *Number, unsigned char *Output, SMS_NumberType type);

#endif
