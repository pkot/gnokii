/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Functions for encoding SMS, calendar and other things.

  $Log$
  Revision 1.1  2001-10-24 22:37:25  pkot
  Moved encoding functions to a separate file


*/

#include <stdlib.h>

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
	'H',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
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

wchar_t EncodeWithUnicodeAlphabet(unsigned char value)
{
	wchar_t retval;

	if (mbtowc(&retval, &value, 1) == -1) return '?';
	else return retval;
}

unsigned char DecodeWithDefaultAlphabet(unsigned char value)
{
	return GSM_DefaultAlphabet[value];
}

unsigned char DecodeWithUnicodeAlphabet(wchar_t value)
{
	unsigned char retval;

	if (wctomb(&retval, value) == -1) return '?';
	else return retval;
}


#define ByteMask ((1 << Bits) - 1)

int Unpack7BitCharacters(int offset, int in_length, int out_length,
			 unsigned char *input, unsigned char *output)
{
        unsigned char *OUT = output; /* Current pointer to the output buffer */
        unsigned char *IN  = input;  /* Current pointer to the input buffer */
        unsigned char Rest = 0x00;
        int Bits;

        Bits = offset ? offset : 7;

        while ((IN - input) < in_length) {

                *OUT = ((*IN & ByteMask) << (7 - Bits)) | Rest;
                Rest = *IN >> Bits;

                /* If we don't start from 0th bit, we shouldn't go to the
                   next char. Under *OUT we have now 0 and under Rest -
                   _first_ part of the char. */
                if ((IN != input) || (Bits == 7)) OUT++;
                IN++;

                if ((OUT - output) >= out_length) break;

                /* After reading 7 octets we have read 7 full characters but
                   we have 7 bits as well. This is the next character */
                if (Bits == 1) {
                        *OUT = Rest;
                        OUT++;
                        Bits = 7;
                        Rest = 0x00;
                } else {
                        Bits--;
                }
        }

        return OUT - output;
}

int Pack7BitCharacters(int offset, unsigned char *input, unsigned char *output)
{

        unsigned char *OUT = output; /* Current pointer to the output buffer */
        unsigned char *IN  = input;  /* Current pointer to the input buffer */
        int Bits;                    /* Number of bits directly copied to
                                        the output buffer */

        Bits = (7 + offset) % 8;

        /* If we don't begin with 0th bit, we will write only a part of the
           first octet */
        if (offset) {
                *OUT = 0x00;
                OUT++;
        }

        while ((IN - input) < strlen(input)) {

                unsigned char Byte = EncodeWithDefaultAlphabet(*IN);

                *OUT = Byte >> (7 - Bits);
                /* If we don't write at 0th bit of the octet, we should write
                   a second part of the previous octet */
                if (Bits != 7)
                        *(OUT-1) |= (Byte & ((1 << (7-Bits)) - 1)) << (Bits+1);

                Bits--;

                if (Bits == -1) Bits = 7;
                else OUT++;

                IN++;
        }

        return (OUT - output);
}
