/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for encoding functions.

*/

#ifndef __gsm_encoding_h_
#define __gsm_encoding_h_

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

#endif
