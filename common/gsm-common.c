/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log$
  Revision 1.4  2001-03-21 23:36:04  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.3  2001/02/28 21:26:51  machek
  Added StrToMemoryType utility function

  Revision 1.2  2001/02/03 23:56:15  chris
  Start of work on irda support (now we just need fbus-irda.c!)
  Proper unicode support in 7110 code (from pkot)

  Revision 1.1  2001/01/12 14:28:39  pkot
  Forgot to add this file. ;-)


*/

#include <gsm-common.h>

/* Coding functions */
#define NUMBER_OF_7_BIT_ALPHABET_ELEMENTS 128
static unsigned char GSM_DefaultAlphabet[NUMBER_OF_7_BIT_ALPHABET_ELEMENTS] = {

	/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
	/* Characters in hex position 10, [12 to 1a] and 24 are not present on
	   latin1 charset, so we cannot reproduce on the screen, however they are
	   greek symbol not present even on my Nokia */
	
	'@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec, 
	0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
	'?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
	' ',  '!',  '\"', '#',  0xa4,  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
	0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

unsigned char EncodeWithDefaultAlphabet(unsigned char value)
{
	unsigned char i;
	
	if (value == '?') return  0x3f;
	
	for (i = 0; i < NUMBER_OF_7_BIT_ALPHABET_ELEMENTS; i++)
		if (GSM_DefaultAlphabet[i] == value)
			return i;
	
	return '?';
}

unsigned char DecodeWithDefaultAlphabet(unsigned char value)
{
	return GSM_DefaultAlphabet[value];
}

wchar_t EncodeWithUnicodeAlphabet(unsigned char value)
{
	wchar_t retval;
        
	if (mbtowc(&retval, &value, 1) == -1) return '?';
	else return retval;
}

unsigned char DecodeWithUnicodeAlphabet(wchar_t value)
{
	unsigned char retval;

	if (wctomb(&retval, value) == -1) return '?';
	else return retval;
}


void DecodeAscii (unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < len; i++)
		dest[i] = DecodeWithDefaultAlphabet(src[i]);
	return;
}

void EncodeAscii (unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < len; i++)
		dest[i] = EncodeWithDefaultAlphabet(src[i]);
	return;
}

void DecodeUnicode (unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	wchar_t wc;

	for (i = 0; i < len; i++) {
	  wc = src[(2*i)+1] | (src[2*i] << 8);
		dest[i] = DecodeWithUnicodeAlphabet(wc);
	}
	dest[len]=0;
	return;
}

void EncodeUnicode (unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	wchar_t wc;

	for (i = 0; i < len; i++) {
		wc = EncodeWithUnicodeAlphabet(src[i]);
		dest[i*2] = (wc >> 8) &0xff;
		dest[(i*2)+1] = wc & 0xff;
	}
	return;
}

GSM_Error Unimplemented(void)
{
	return GE_NOTIMPLEMENTED;
}

GSM_MemoryType StrToMemoryType(const char *s)
{
#define X(a) if (!strcmp(s, #a)) return GMT_##a;
	X(ME);
	X(SM);
	X(FD);
	X(ON);
	X(EN);
	X(DC);
	X(RC);
	X(MC);
	X(LD);
	X(MT);
	return GMT_XX;
#undef X
}


/* FIXME - a better way?? */
void GSM_DataClear(GSM_Data *data)
{
	int c;

	for (c=0;c<sizeof(GSM_Data);c++) ((unsigned char *)data)[c]=0;
}
