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

  Functions for encoding SMS, calendar and other things.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-encoding.h"

#define GN_CHAR_ALPHABET_SIZE 128

#define GN_CHAR_ESCAPE 0x1b

static unsigned char gsm_default_alphabet[GN_CHAR_ALPHABET_SIZE] = {

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

static unsigned char gsm_reverse_default_alphabet[256];
static bool reversed = false;

static void tbl_setup_reverse()
{
	int i;

	if (reversed) return;
	memset(gsm_reverse_default_alphabet, 0x3f, 256);
	for (i = GN_CHAR_ALPHABET_SIZE - 1; i >= 0; i--)
		gsm_reverse_default_alphabet[ gsm_default_alphabet[i] ] = i;
	gsm_reverse_default_alphabet['?'] = 0x3f;
	reversed = true;
}

static bool char_is_escape(unsigned char value)
{
	return (value == GN_CHAR_ESCAPE);
}

/*
 * In GSM specification there are 10 characters in the extension
 * of the default alphabet. Their values look a bit random, they are
 * only 10, and probably they will never change, so hardcoding them
 * here is rather safe.
 */

static bool char_def_alphabet_ext(unsigned char value)
{
	wchar_t retval;

	if (mbtowc(&retval, &value, 1) == -1) return false;
	return (value == 0x0c ||
		value == '^' ||
		value == '{' ||
		value == '}' ||
		value == '\\' ||
		value == '[' ||
		value == '~' ||
		value == ']' ||
		value == '|' ||
		retval == 0x20ac);
}

static unsigned char char_decode_def_alphabet_ext(unsigned char value)
{
	switch (value) {
	case 0x0a: return 0x0c; break; /* form feed */
	case 0x14: return '^';  break;
	case 0x28: return '{';  break;
	case 0x29: return '}';  break;
	case 0x2f: return '\\'; break;
	case 0x3c: return '[';  break;
	case 0x3d: return '~';  break;
	case 0x3e: return ']';  break;
	case 0x40: return '|';  break;
	case 0x65: return 0xa4; break; /* euro */
	default: return '?';    break; /* invalid character */
	}
}

static unsigned char char_encode_def_alphabet_ext(unsigned char value)
{
	switch (value) {
	case 0x0c: return 0x0a; break; /* from feed */
	case '^':  return 0x14; break;
	case '{':  return '{';  break;
	case '}':  return '}';  break;
	case '\\': return '\\'; break;
	case '[':  return '[';  break;
	case '~':  return '~';  break;
	case ']':  return ']';  break;
	case '|':  return '|';  break;
	case 0xa4: return 0x65; break; /* euro */
	default: return 0x00;   break; /* invalid character */
	}
}

API bool gn_char_def_alphabet(unsigned char *string)
{
	unsigned int i, len = strlen(string);

	tbl_setup_reverse();
	for (i = 0; i < len; i++)
		if (!char_def_alphabet_ext(string[i]) &&
		    gsm_reverse_default_alphabet[string[i]] == 0x3f &&
		    string[i] != '?')
			return false;
	return true;
}

static unsigned char char_encode_def_alphabet(unsigned char value)
{
	tbl_setup_reverse();
	return gsm_reverse_default_alphabet[value];
}

static wchar_t char_encode_uni_alphabet(unsigned char value)
{
	wchar_t retval;

	if (mbtowc(&retval, &value, 1) == -1) return '?';
	else return retval;
}

static unsigned char char_decode_def_alphabet(unsigned char value)
{
	if (value < GN_CHAR_ALPHABET_SIZE) {
		return gsm_default_alphabet[value];
	} else {
		return '?';
	}
}

static unsigned char char_decode_uni_alphabet(wchar_t value)
{
	unsigned char retval;

	if (wctomb(&retval, value) == -1) return '?';
	else return retval;
}


#define GN_BYTE_MASK ((1 << bits) - 1)

int char_unpack_7bit(unsigned int offset, unsigned int in_length, unsigned int out_length,
		     unsigned char *input, unsigned char *output)
{
	unsigned char *out_num = output; /* Current pointer to the output buffer */
	unsigned char *in_num = input;  /* Current pointer to the input buffer */
	unsigned char rest = 0x00;
	int bits;

	bits = offset ? offset : 7;

	while ((in_num - input) < in_length) {

		*out_num = ((*in_num & GN_BYTE_MASK) << (7 - bits)) | rest;
		rest = *in_num >> bits;

		/* If we don't start from 0th bit, we shouldn't go to the
		   next char. Under *OUT we have now 0 and under Rest -
		   _first_ part of the char. */
		if ((in_num != input) || (bits == 7)) out_num++;
		in_num++;

		if ((out_num - output) >= out_length) break;

		/* After reading 7 octets we have read 7 full characters but
		   we have 7 bits as well. This is the next character */
		if (bits == 1) {
			*out_num = rest;
			out_num++;
			bits = 7;
			rest = 0x00;
		} else {
			bits--;
		}
	}

	return out_num - output;
}

int char_pack_7bit(unsigned int offset, unsigned char *input,
		   unsigned char *output, unsigned int *in_len)
{

	unsigned char *out_num = output; /* Current pointer to the output buffer */
	unsigned char *in_num = input;  /* Current pointer to the input buffer */
	int bits;		     /* Number of bits directly copied to
					the output buffer */

	bits = (7 + offset) % 8;

	/* If we don't begin with 0th bit, we will write only a part of the
	   first octet */
	if (offset) {
		*out_num = 0x00;
		out_num++;
	}

	while ((in_num - input) < strlen(input)) {
		unsigned char byte;
		bool double_char = false;

		if (char_def_alphabet_ext(*in_num)) {
			byte = GN_CHAR_ESCAPE;
			double_char = true;
			goto skip;
next_char:
			byte = char_encode_def_alphabet_ext(*in_num);
			double_char = false;
			(*in_len)++;
		} else {
			byte = char_encode_def_alphabet(*in_num);
		}
skip:
		*out_num = byte >> (7 - bits);
		/* If we don't write at 0th bit of the octet, we should write
		   a second part of the previous octet */
		if (bits != 7)
			*(out_num-1) |= (byte & ((1 << (7-bits)) - 1)) << (bits+1);

		bits--;

		if (bits == -1) bits = 7;
		else out_num++;

		if (double_char) goto next_char;

		in_num++;
	}

	return (out_num - output);
}

void char_decode_ascii(unsigned char* dest, const unsigned char* src, int len)
{
	int i, j;

	for (i = 0, j = 0; j < len; i++, j++) {
		if (char_is_escape(src[j]))
			dest[i] = char_decode_def_alphabet_ext(src[++j]);
		else
			dest[i] = char_decode_def_alphabet(src[j]);
	}
	dest[i] = 0;
	return;
}

unsigned int char_encode_ascii(unsigned char* dest, const unsigned char* src, unsigned int len)
{
	int i, j;

	for (i = 0, j = 0; j < len; i++, j++) {
		if (char_def_alphabet_ext(src[j])) {
			dest[i++] = GN_CHAR_ESCAPE;
			dest[i] = char_encode_def_alphabet_ext(src[j]);
		} else {
			dest[i] = char_encode_def_alphabet(src[j]);
		}
	}
	return i;
}

void char_decode_hex(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	char buf[3];

	buf[2] = '\0';
	for (i = 0; i < (len / 2); i++) {
		buf[0] = *(src + i * 2); buf[1] = *(src + i * 2 + 1);
		dest[i] = char_decode_def_alphabet(strtol(buf, NULL, 16));
	}
	return;
}

void char_encode_hex(unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < (len / 2); i++) {
		sprintf(dest + i * 2, "%x", char_encode_def_alphabet(src[i]));
	}
	return;
}

void char_decode_ucs2(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	char buf[5];

	buf[4] = '\0';
	for (i = 0; i < (len / 4); i++) {
		buf[0] = *(src + i * 4); buf[1] = *(src + i * 4 + 1);
		buf[2] = *(src + i * 4 + 2); buf[3] = *(src + i * 4 + 3);
		dest[i] = char_decode_uni_alphabet(strtol(buf, NULL, 16));
	}
	return;
}

void char_encode_ucs2(unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < (len / 4); i++) {
		sprintf(dest + i * 4, "%lx", char_encode_uni_alphabet(src[i]));
	}
	return;
}

void char_decode_unicode(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	wchar_t wc;

	for (i = 0; i < len; i++) {
		wc = src[(2 * i) + 1] | (src[2 * i] << 8);
		dest[i] = char_decode_uni_alphabet(wc);
	}
	dest[len] = 0;
	return;
}

void char_encode_unicode(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	wchar_t wc;

	for (i = 0; i < len; i++) {
		wc = char_encode_uni_alphabet(src[i]);
		dest[i*2] = (wc >> 8) & 0xff;
		dest[(i*2)+1] = wc & 0xff;
	}
	return;
}

/* Conversion bin -> hex and hex -> bin */
void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len)
{
	int i;

	if (!dest) return;

	for (i = 0; i < len; i++) {
		unsigned aux;

		if (src[2 * i] >= '0' && src[2 * i] <= '9') aux = src[2 * i] - '0';
		else if (src[2 * i] >= 'a' && src[2 * i] <= 'f') aux = src[2 * i] - 'a' + 10;
		else if (src[2 * i] >= 'A' && src[2 * i] <= 'F') aux = src[2 * i] - 'A' + 10;
		else {
			dest[0] = 0;
			return;
		}
		dest[i] = aux << 4;
		if (src[2 * i + 1] >= '0' && src[2 * i + 1] <= '9') aux = src[2 * i + 1] - '0';
		else if (src[2 * i + 1] >= 'a' && src[2 * i + 1] <= 'f') aux = src[2 * i + 1] - 'a' + 10;
		else if (src[2 * i + 1] >= 'A' && src[2 * i + 1] <= 'F') aux = src[2 * i + 1] - 'A' + 10;
		else {
			dest[0] = 0;
			return;
		}
		dest[i] |= aux;
	}
}

void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len)
{
	int i;

	if (!dest) return;

	for (i = 0; i < len; i++) {
		dest[2 * i] = (src[i] & 0xf0) >> 4;
		if (dest[2 * i] < 10) dest[2 * i] += '0';
		else dest[2 * i] += ('A' - 10);
		dest[2 * i + 1] = src[i] & 0x0f;
		if (dest[2 * i + 1] < 10) dest[2 * i + 1] += '0';
		else dest[2 * i + 1] += ('A' - 10);
	}
}

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */
int char_semi_octet_pack(char *number, unsigned char *output, SMS_NumberType type)
{
	unsigned char *in_num = number;  /* Pointer to the input number */
	unsigned char *out_num = output; /* Pointer to the output */
	int count = 0; /* This variable is used to notify us about count of already
			  packed numbers. */

	/* The first byte in the Semi-octet representation of the address field is
	   the Type-of-Address. This field is described in the official GSM
	   specification 03.40 version 6.1.0, section 9.1.2.5, page 33. We support
	   only international and unknown number. */

	*out_num++ = type;
	if (type == SMS_International) in_num++; /* Skip '+' */
	if ((type == SMS_Unknown) && (*in_num == '+')) in_num++; /* Optional '+' in Unknown number type */

	/* The next field is the number. It is in semi-octet representation - see
	   GSM scpecification 03.40 version 6.1.0, section 9.1.2.3, page 31. */
	while (*in_num) {
		if (count & 0x01) {
			*out_num = *out_num | ((*in_num - '0') << 4);
			out_num++;
		}
		else
			*out_num = *in_num - '0';
		count++; in_num++;
	}

	/* We should also fill in the most significant bits of the last byte with
	   0x0f (1111 binary) if the number is represented with odd number of
	   digits. */
	if (count & 0x01) {
		*out_num = *out_num | 0xf0;
		out_num++;
	}

	return (2 * (out_num - output - 1) - (count % 2));
}

char *char_get_bcd_number(u8 *number)
{
	static char buffer[MAX_BCD_STRING_LENGTH] = "";
	int length = number[0]; /* This is the length of BCD coded number */
	int count, digit;

	if (length > MAX_BCD_STRING_LENGTH) length = MAX_BCD_STRING_LENGTH;
	memset(buffer, 0, MAX_BCD_STRING_LENGTH);
	switch (number[1]) {
	case SMS_Alphanumeric:
		char_unpack_7bit(0, length, length, number + 2, buffer);
		buffer[length] = 0;
		break;
	case SMS_International:
		sprintf(buffer, "+");
		if (length == MAX_BCD_STRING_LENGTH) length--; /* avoid overflow */
	case SMS_Unknown:
	case SMS_National:
	case SMS_Network:
	case SMS_Subscriber:
	case SMS_Abbreviated:
	default:
		for (count = 0; count < length - 1; count++) {
			digit = number[count+2] & 0x0f;
			if (digit < 10) sprintf(buffer, "%s%d", buffer, digit);
			digit = number[count+2] >> 4;
			if (digit < 10) sprintf(buffer, "%s%d", buffer, digit);
		}
		break;
	}
	return buffer;
}
