/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Include file for encoding functions.

  $Log$
  Revision 1.1  2001-10-24 22:37:25  pkot
  Moved encoding functions to a separate file


*/

#ifndef __gsm_encoding_h_
#define __gsm_encoding_h_

unsigned char EncodeWithDefaultAlphabet(unsigned char value);
wchar_t EncodeWithUnicodeAlphabet(unsigned char value);
unsigned char DecodeWithDefaultAlphabet(unsigned char value);
unsigned char DecodeWithUnicodeAlphabet(wchar_t value);
int Unpack7BitCharacters(int offset, int in_length, int out_length,
                           unsigned char *input, unsigned char *output);
int Pack7BitCharacters(int offset, unsigned char *input, unsigned char *output);

#endif
