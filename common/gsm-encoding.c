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

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "gnokii-internal.h"

#ifdef HAVE_ICONV
#  include <iconv.h>
#endif
#ifdef HAVE_LANGINFO_CODESET
#  include <langinfo.h>
#endif

static const char *base64_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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
extern const char *locale_charset(void); /* from ../intl/localcharset.c */

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

static unsigned char char_def_alphabet_ext_decode(unsigned char value)
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

static unsigned char char_def_alphabet_ext_encode(unsigned char value)
{
	switch (value) {
	case 0x0c: return 0x0a; /* from feed */
	case '^':  return 0x14;
	case '{':  return 0x28;
	case '}':  return 0x29;
	case '\\': return 0x2f;
	case '[':  return 0x3c;
	case '~':  return 0x3d;
	case ']':  return 0x3e;
	case '|':  return 0x40;
	case 0xa4: return 0x65; /* euro */
	default: return 0x00; /* invalid character */
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

unsigned char char_def_alphabet_encode(unsigned char value)
{
	tbl_setup_reverse();
	return gsm_reverse_default_alphabet[value];
}

unsigned char char_def_alphabet_decode(unsigned char value)
{
	if (value < GN_CHAR_ALPHABET_SIZE) {
		return gsm_default_alphabet[value];
	} else {
		return '?';
	}
}

#define GN_BYTE_MASK ((1 << bits) - 1)

int char_7bit_unpack(unsigned int offset, unsigned int in_length, unsigned int out_length,
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
		   next char. Under *out_num we have now 0 and under Rest -
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

int char_7bit_pack(unsigned int offset, unsigned char *input,
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
			byte = char_def_alphabet_ext_encode(*in_num);
			double_char = false;
			(*in_len)++;
		} else {
			byte = char_def_alphabet_encode(*in_num);
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

void char_ascii_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int i, j;

	for (i = 0, j = 0; j < len; i++, j++) {
		if (char_is_escape(src[j]))
			dest[i] = char_def_alphabet_ext_decode(src[++j]);
		else
			dest[i] = char_def_alphabet_decode(src[j]);
	}
	dest[i] = 0;
	return;
}

unsigned int char_ascii_encode(unsigned char* dest, const unsigned char* src, unsigned int len)
{
	int i, j;

	for (i = 0, j = 0; j < len; i++, j++) {
		if (char_def_alphabet_ext(src[j])) {
			dest[i++] = GN_CHAR_ESCAPE;
			dest[i] = char_def_alphabet_ext_encode(src[j]);
		} else {
			dest[i] = char_def_alphabet_encode(src[j]);
		}
	}
	return i;
}

void char_hex_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int i;
	char buf[3];

	buf[2] = '\0';
	for (i = 0; i < (len / 2); i++) {
		buf[0] = *(src + i * 2); buf[1] = *(src + i * 2 + 1);
		dest[i] = char_def_alphabet_decode(strtol(buf, NULL, 16));
	}
	dest[i] = 0;
	return;
}

void char_hex_encode(unsigned char* dest, const unsigned char* src, int len)
{
	int i;

	for (i = 0; i < (len / 2); i++) {
		sprintf(dest + i * 2, "%x", char_def_alphabet_encode(src[i]));
	}
	return;
}

int char_uni_alphabet_encode(unsigned char const *value, wchar_t *dest)
{
	int length;

	switch (length = mbtowc(dest, value, MB_CUR_MAX)) {
	case -1:
		dprintf("Error calling mctowb!\n");
		*dest = '?';
		length = 1;
	default:
		return length;
	}
}

int char_uni_alphabet_decode(wchar_t value, unsigned char *dest)
{
	int length;

	switch (length = wctomb(dest, value)) {
	case -1:
		*dest = '?';
		length = 1;
	default:
		return length;
	}
}

void char_ucs2_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int i_len = 0, o_len = 0, length;
	char buf[5];

	buf[4] = '\0';
	for (i_len = 0; i_len < len ; i_len++) {
		buf[0] = *(src + i_len * 4);
		buf[1] = *(src + i_len * 4 + 1);
		buf[2] = *(src + i_len * 4 + 2);
		buf[3] = *(src + i_len * 4 + 3);
		switch (length = char_uni_alphabet_decode(strtol(buf, NULL, 16), dest + o_len)) {
		case -1:
			o_len++;
			break;
		default:
			o_len += length;
			break;
		}
	}
	dest[o_len] = 0;
	return;
}

void char_ucs2_encode(unsigned char* dest, const unsigned char* src, int len)
{
	wchar_t wc;
	int i_len = 0, o_len, length;

	for (o_len = 0; i_len < len ; o_len++) {
		switch (length = char_uni_alphabet_encode(src + i_len, &wc)) {
		case -1:
			i_len++;
			break;
		default:
			i_len += length;
			break;
		}
		sprintf(dest + (o_len << 2), "%lx", wc);
	}
	return;
}

unsigned int char_unicode_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int i, length = 0, pos = 0;

	for (i = 0; i < len / 2; i++) {
		length = char_uni_alphabet_decode((src[i * 2] << 8) | src[(i * 2) + 1], dest);
		dest += length;
		pos += length;
	}
	*dest = 0;
	return pos;
}

unsigned int char_unicode_encode(unsigned char* dest, const unsigned char* src, int len)
{
	int length, offset = 0, pos = 0;
	wchar_t  wc;

	while (offset < len) {
		switch (length = char_uni_alphabet_encode(src + offset, &wc)) {
		case -1:
			dest[pos++] =  wc >> 8 & 0xFF;
			dest[pos++] =  wc & 0xFF;
			offset++;
			break;
		default:
			dest[pos++] =  wc >> 8 & 0xFF;
			dest[pos++] =  wc & 0xFF;
			offset += length;
			break;
		}
	}
	return pos;
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
int char_semi_octet_pack(char *number, unsigned char *output, gn_gsm_number_type type)
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
	if (type == GN_GSM_NUMBER_International) in_num++; /* Skip '+' */
	if ((type == GN_GSM_NUMBER_Unknown) && (*in_num == '+')) in_num++; /* Optional '+' in Unknown number type */

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

char *char_bcd_number_get(u8 *number)
{
	static char buffer[GN_BCD_STRING_MAX_LENGTH] = "";
	int length = number[0]; /* This is the length of BCD coded number */
	int count, digit;

	if (length > GN_BCD_STRING_MAX_LENGTH) length = GN_BCD_STRING_MAX_LENGTH;
	memset(buffer, 0, GN_BCD_STRING_MAX_LENGTH);
	switch (number[1]) {
	case GN_GSM_NUMBER_Alphanumeric:
		char_7bit_unpack(0, length, length, number + 2, buffer);
		buffer[length] = 0;
		break;
	case GN_GSM_NUMBER_International:
		sprintf(buffer, "+");
		if (length == GN_BCD_STRING_MAX_LENGTH) length--; /* avoid overflow */
	case GN_GSM_NUMBER_Unknown:
	case GN_GSM_NUMBER_National:
	case GN_GSM_NUMBER_Network:
	case GN_GSM_NUMBER_Subscriber:
	case GN_GSM_NUMBER_Abbreviated:
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

static const char *get_langinfo_codeset(void)
{
	static const char *codeset = NULL;

	if (!codeset) {
#ifdef HAVE_LANGINFO_CODESET
		codeset = nl_langinfo(CODESET);
#else
		codeset = locale_charset();
#endif
	}
	return codeset;
}

/* UTF-8 conversion functions */
int utf8_decode(char *outstring, int outlen, const char *instring, int inlen)
{
	size_t nconv;

#if defined(HAVE_ICONV)
	char *pin, *pout;
	iconv_t cd;

	pin = (char *)instring;
	pout = outstring;

	cd = iconv_open(get_langinfo_codeset(), "UTF-8");
	nconv = iconv(cd, &pin, &inlen, &pout, &outlen);
	iconv_close(cd);
	*pout = 0;
#else
	unsigned char *pin, *pout;

	pin = (unsigned char *)instring;
	pout = outstring;

	while (inlen > 0 && outlen > 0) {
		if (*pin < 0x80) {
			*pout = *pin;
			nconv = 1;
		} else if (*pin < 0xc0) {
			*pout = '?';
			nconv = 1;
		} else if (*pin < 0xe0) {
			*pout = '?';
			nconv = 2;
		} else if (*pin < 0xf0) {
			*pout = '?';
			nconv = 3;
		} else if (*pin < 0xf8) {
			*pout = '?';
			nconv = 4;
		} else if (*pin < 0xfc) {
			*pout = '?';
			nconv = 5;
		} else {
			*pout = '?';
			nconv = 6;
		}
		inlen -= nconv;
		outlen--;
		pin += nconv;
		if (*pout++ == '\0') break;
	}
	nconv = 0;
#endif
	return (nconv < 0) ?  -1 : (char *)pout - outstring;
}

int utf8_encode(char *outstring, int outlen, const char *instring, int inlen)
{
#if defined(HAVE_ICONV)
	size_t outleft, inleft, nconv;
	char *pin, *pout;
	iconv_t cd;

	outleft = outlen;
	inleft = inlen;
	pin = (char *)instring;
	pout = outstring;

	cd = iconv_open("UTF-8", get_langinfo_codeset());

	nconv = iconv(cd, &pin, &inleft, &pout, &outleft);
	*pout = 0;
	iconv_close(cd);
#else
	size_t nconv;
	unsigned char *pin, *pout;

	nconv = 0;
	pin = (unsigned char *)instring;
	pout = outstring;

	while (inlen > 0 && outlen > 0) {
		if (*pin >= 0x80)
			*pout = '?';
		else
			*pout = *pin;

		inlen--;
		outlen--;
		pin++;
		if (*pout++ == '\0') break;
	}
#endif
	return (nconv < 0) ?  -1 : (char *)pout - outstring;
}


/* BASE64 functions */
int string_base64(const char *instring)
{
	for (; *instring; instring++)
		if (*instring & 0x80)
			return 1;
	return 0;
}

/*
   encodes a null-terminated input string with base64 encoding.
   the buffer outstring needs to be at least 1.333 times bigger than the input string length
*/
int base64_encode(char *outstring, int outlen, const char *instring, int inlen)
{
	char *in1, *pin, *pout;
	char *outtemp = NULL;
	int inleft, outleft, inprocessed;
	unsigned int i1, i2, i3, i4;


	/* copy the input string, need multiple of 3 chars */
	in1 = malloc(inlen + 3);
	memset(in1, 0, inlen + 3);
	memcpy(in1, instring, inlen);

	pout = outstring;
	inleft = inlen;
	outleft = outlen;
	inprocessed = 0;
	pin = in1;

	for (; (outleft > 3) && (inprocessed < inlen); ) {
		if (!(*pin))
			break;

		/* calculate the indexes */
		i1 = (pin[0] & 0xFC) >> 2;
		i2 = ((pin[0] & 0x03) << 4) | ((pin[1] & 0xF0) >> 4);
		i3 = ((pin[1] & 0x0F) << 2) | ((pin[2] & 0xC0) >> 6);
		i4 = pin[2] & 0x3F;
		pin += 3;

		/* assign the characters or the padding char, if necessary */
		*(pout++) = base64_alphabet[i1];
		*(pout++) = base64_alphabet[i2];
		inleft--;
		if (!inleft) {
			*(pout++) = '=';
		} else {
			*(pout++) = base64_alphabet[i3];
			inleft--;
		}

		if (!inleft)
			*(pout++) = '=';
		else {
			*(pout++) = base64_alphabet[i4];
			inleft--;
		}

		/* update the counters */
		inprocessed += 3;
		outleft -= 4;
	}

	if (outleft > 0) *pout = 0;
	if (outtemp)
		free(outtemp);

	free(in1);

	return pout - outstring;
}

int base64_decode(char *dest, int destlen, const char *source, int inlen)
{
	int dtable[256];
	int i, c;
	int dpos = 0;
	int spos = 0;

	for (i = 0; i < 255; i++) {
		dtable[i] = 0x80;
	}
	for (i = 'A'; i <= 'Z'; i++) {
		dtable[i] = 0 + (i - 'A');
	}
	for (i = 'a'; i <= 'z'; i++) {
		dtable[i] = 26 + (i - 'a');
	}
	for (i = '0'; i <= '9'; i++) {
		dtable[i] = 52 + (i - '0');
	}
	dtable['+'] = 62;
	dtable['/'] = 63;
	dtable['='] = 0;

	/* CONSTANT CONDITION */
	while (1) {
		int a[4], b[4], o[3];

		for (i = 0; i < 4; i++) {
			if (spos >= inlen || dpos >= destlen) {
				goto endloop;
			}
			c = source[spos++];

			if (c == 0) {
				if (i > 0) {
					goto endloop;
				}
				goto endloop;
			}
			if (dtable[c] & 0x80) {
				/* Ignoring errors: discard invalid character. */
				i--;
				continue;
			}
			a[i] = (int) c;
			b[i] = (int) dtable[c];
		}
		o[0] = (b[0] << 2) | (b[1] >> 4);
		o[1] = (b[1] << 4) | (b[2] >> 2);
		o[2] = (b[2] << 6) | b[3];
        	i = a[2] == '=' ? 1 : (a[3] == '=' ? 2 : 3);
		if (i >= 1) dest[dpos++] = o[0];
		if (i >= 2) dest[dpos++] = o[1];
		if (i >= 3) dest[dpos++] = o[2];
		dest[dpos] = 0;
		if (i < 3) {
			goto endloop;
		}
	}
endloop:
	return dpos;
}

int utf8_base64_encode(char *dest, int destlen, const char *in, int inlen)
{
	char *aux;
	int retval;

	aux = calloc(destlen + 1, sizeof(char));

	retval = utf8_encode(aux, destlen, in, inlen);
	if (retval >= 0)
		retval = base64_encode(dest, destlen, aux, retval);

	free(aux);
	return retval;
}

int utf8_base64_decode(char *dest, int destlen, const char *in, int inlen)
{
	char *aux;
	int retval;

	aux = calloc(destlen + 1, sizeof(char));

	retval = base64_decode(aux, destlen, in, inlen);
	if (retval >= 0)
		retval = utf8_decode(dest, destlen, aux, retval);

	free(aux);
	return retval;
}
