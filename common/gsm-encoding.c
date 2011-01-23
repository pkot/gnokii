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

  Copyright (C) 1999-2000 Pavel Janik ml.
  Copyright (C) 2001-2005 Pawel Kot
  Copyright (C) 2002      Markus Plail, Manfred Jonsson
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2003      Martin Goldhahn

  Functions for encoding SMS, calendar and other things.

*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#ifdef HAVE_LOCALE_CHARSET
#  include <libcharset.h>
#else
/* FIXME: We should include here somehow ../intl/localcharset.h, but it may
 * cause problems with MSVC. */
extern const char *locale_charset(void); /* from ../intl/localcharset.c */
#endif

/**
 * base64_alphabet:
 *
 * Mapping from 8-bit binary values to base 64 encoding.
 */
static const char *base64_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * bcd_digits:
 *
 * Mapping from ASCII to BCD digits representing phone numbers and vice versa.
 * BCD digits are those from Table 10.5.118 of 3GPP TS 04.08 with 'a' replaced by 'p'.
 */
static const char *bcd_digits = "0123456789*#pbc";

#if 0

/*
 * We switched from internal representation in ISO/IEC 8859-1 encoding to UCS-2
 * encoding. But let's leave the old stuff for the time being.
 *
 * FIXME: Remove in gnokii 0.6.30
 */

/**
 * GN_CHAR_ALPHABET_SIZE:
 *
 * Number of characters in GSM default alphabet (for ISO/IEC 8859-1 encoding).
 */
#define GN_CHAR_ALPHABET_SIZE 128

/**
 * GN_CHAR_UNI_ESCAPE:
 *
 * Value of the escape character for the GSM Alphabet (in ISO/IEC 8859-1 encoding).
 */
#define GN_CHAR_ESCAPE 0x1b

/**
 * gsm_default_alphabet:
 *
 * Mapping from GSM default alphabet to ISO/IEC 8859-1.
 *
 * ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet.
 * Characters in hex position 10, [12 to 1a] and 24 are not present on
 * latin1 charset, so we cannot reproduce on the screen, they are
 * Greek symbols not even present on some phones.
 */
static unsigned char gsm_default_alphabet[GN_CHAR_ALPHABET_SIZE] = {
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

#endif

/**
 * GN_CHAR_UNI_ALPHABET_SIZE:
 *
 * Number of characters in GSM default alphabet (for UCS-2 encoding).
 */
#define GN_CHAR_UNI_ALPHABET_SIZE 128

/**
 * GN_CHAR_UNI_ESCAPE:
 *
 * Value of the escape character for the GSM Alphabet (in UCS-2 encoding).
 */
#define GN_CHAR_UNI_ESCAPE 0x001b

/**
 * gsm_default_unicode_alphabet:
 *
 * Mapping from GSM default alphabet to UCS-2.
 *
 * ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet. Mapping to UCS-2.
 * Mapping according to http://unicode.org/Public/MAPPINGS/ETSI/GSM0338.TXT
 */
static unsigned int gsm_default_unicode_alphabet[GN_CHAR_UNI_ALPHABET_SIZE] = {
	/* @       £       $       ¥       è       é       ù       ì */
	0x0040, 0x00a3, 0x0024, 0x00a5, 0x00e8, 0x00e9, 0x00f9, 0x00ec,
	/* ò       Ç       \n      Ø       ø       \r      Å       å */
	0x00f2, 0x00c7, 0x000a, 0x00d8, 0x00f8, 0x000d, 0x00c5, 0x00e5,
	/* Δ       _       Φ       Γ       Λ       Ω       Π       Ψ */
	0x0394, 0x005f, 0x03a6, 0x0393, 0x039b, 0x03a9, 0x03a0, 0x03a8,
	/* Σ       Θ       Ξ      NBSP     Æ       æ       ß       É */
	0x03a3, 0x0398, 0x039e, 0x00a0, 0x00c6, 0x00e6, 0x00df, 0x00c9,
	/* ' '     !       "       #       ¤       %       &       ' */
	0x0020, 0x0021, 0x0022, 0x0023, 0x00a4, 0x0025, 0x0026, 0x0027,
	/* (       )       *       +       ,       -       .       / */
	0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	/* 0       1       2       3       4       5       6       7 */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	/* 8       9       :       ;       <       =       >       ? */
	0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	/* ¡       A       B       C       D       E       F       G */
	0x00a1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	/* H       I       J       K       L       M       N       O */
	0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	/* P       Q       R       S       T       U       V       W */
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	/* X       Y       Z       Ä       Ö       Ñ       Ü       § */
	0x0058, 0x0059, 0x005a, 0x00c4, 0x00d6, 0x00d1, 0x00dc, 0x00a7,
	/* ¿       a       b       c       d       e       f       g */
	0x00bf, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	/* h       i       j       k       l       m       n       o */
	0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	/* p       q       r       s       t       u       v       w */
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	/* x       y       z       ä       ö       ñ       ü       à */
	0x0078, 0x0079, 0x007a, 0x00e4, 0x00f6, 0x00f1, 0x00fc, 0x00e0
};

static char application_encoding[64] = "";

/**
 * char_def_alphabet:
 * @value: the UCS-2 character to validate
 *
 * Returns: true if the given character matches default alphabet, false otherwise
 *
 * It could be possibly optimized but let's face it: nowadays full
 * lookup of 128 elements table is not that time consuming.
 */
static int char_def_alphabet(unsigned int value)
{
	int i;
	for (i = 0; i < GN_CHAR_UNI_ALPHABET_SIZE; i++) {
		if (gsm_default_unicode_alphabet[i] == value) {
			return true;
		}
	}
	return false;
}

/**
 * char_is_escape:
 * @value: the char to test
 *
 * Returns: non zero if @value is an escape character, zero otherwise
 *
 * Determines if @value is an escape character for GSM Alphabet.
 */
static bool char_is_escape(unsigned int value)
{
	return (value == GN_CHAR_UNI_ESCAPE);
}

/**
 * get_langinfo_codeset:
 *
 * Returns: a constant string representing a charset encoding
 *
 * Gets the current charset encoding.
 * Uses different methods on different platforms.
 */
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

/**
 * gn_char_get_encoding:
 *
 * Returns: a constant string representing a charset encoding
 *
 * Gets the encoding set by the application or the default one.
 */
GNOKII_API const char *gn_char_get_encoding()
{
	const char *coding;
	if (*application_encoding) 
		coding = application_encoding; /* app has overriden encoding setting */
	else
		coding = get_langinfo_codeset(); /* return default codeset */
	return coding;
}

/**
 * gn_char_set_encoding:
 * @encoding: a string representing the name of a charset encoding
 *
 * Sets the encoding preferred by the application.
 */
void gn_char_set_encoding(const char* encoding)
{
	snprintf(application_encoding, sizeof(application_encoding), "%s", encoding);
}

/**
 * char_mblen:
 * @src: the string to measure
 *
 * Returns: the lenght of the string
 *
 * Detects the correct length of a string (also for multibyte chars like "umlaute").
 */
int char_mblen(const char *src)
{
	int len = mbstowcs(NULL, src, 0);
	dprintf("char_mblen(%s): %i\n", src, len);
	return len;
}

#ifndef ICONV_CONST
#  define ICONV_CONST const
#endif

/**
 * char_mbtowc:
 * @wchar_t: buffer for the converted wide char string
 * @src: buffer with the multibyte string to be converted
 * @maxlen: size of @wchar_t buffer
 * @mbs: pointer to a variable holding the shift state
 * or NULL to use a global variable
 *
 * Returns: the number of bytes from @src that have been used
 * or -1 in case of error
 *
 * Converts a multibyte string to a wide char string.
 * Uses iconv() if it is available and iconv_open() succeeds, else mbrtowc()
 * if available, else mbtowc().
 */
static int char_mbtowc(wchar_t *dst, const char *src, int maxlen, MBSTATE *mbs)
{
#ifdef HAVE_ICONV
	size_t nconv;
	ICONV_CONST char *pin;
	char *pout;
	size_t inlen;
	size_t outlen;
	iconv_t cd;

	pin = (char *)src;
	pout = (char *)dst;
	/* Let's assume that we have at most 4-bytes wide characters */
	inlen = maxlen;
	outlen = maxlen * sizeof(wchar_t);

	cd = iconv_open("WCHAR_T", gn_char_get_encoding());
	if (cd == (iconv_t)-1)
		goto fallback;
	nconv = iconv(cd, &pin, &inlen, &pout, &outlen);
	if ((nconv == (size_t)-1) && (pin == src))
		perror("char_mbtowc/iconv");
	iconv_close(cd);

	return (char*)dst == pout ? -1 : pin-src;
fallback:
#endif
	if (maxlen >= MB_CUR_MAX)
		maxlen = MB_CUR_MAX - 1;
#ifdef HAVE_WCRTOMB
	return mbrtowc(dst, src, maxlen, mbs);
#else
	return mbtowc(dst, src, maxlen);
#endif
}

/**
 * char_wctomb:
 * @dst: buffer for the converted multibyte string
 * @src: buffer with the wide char string to be converted
 * @mbs: pointer to a variable holding the shift state
 * or NULL to use a global variable
 *
 * Returns: the number of bytes from @src that have been used
 * or -1 in case of error
 *
 * Converts a wide char string to a multibyte string.
 * Uses iconv() if it is available and iconv_open() succeeds, else wcrtomb()
 * if available, else wctomb().
 */
static int char_wctomb(char *dst, wchar_t src, MBSTATE *mbs)
{
#ifdef HAVE_ICONV
	size_t nconv;
	ICONV_CONST char *pin;
	char *pout;
	size_t inlen;
	size_t outlen;
	iconv_t cd;

	pin = (char *)&src;
	pout = (char *)dst;
	inlen = sizeof(wchar_t);
	outlen = 4;

	cd = iconv_open(gn_char_get_encoding(), "WCHAR_T");
	if (cd == (iconv_t)-1)
		goto fallback;
	nconv = iconv(cd, &pin, &inlen, &pout, &outlen);
	if (nconv == (size_t)-1)
		perror("char_wctomb/iconv");
	iconv_close(cd);

	return nconv == -1 ? -1 : pout-dst;
fallback:
#endif
#ifdef HAVE_WCRTOMB
	return wcrtomb(dst, src, mbs);
#else
	return wctomb(dst, src);
#endif
}

/**
 * char_def_alphabet_ext:
 * @value: the character to test UCS-2 encoded
 *
 * Returns: non zero if the character can be represented with the Extended GSM Alphabet,
 * zero otherwise
 *
 * Checks if @value is a character defined by the Extended GSM Alphabet.
 *
 * In GSM specification there are 10 characters in the extension
 * of the default alphabet. Their values look a bit random, they are
 * only 10, and probably they will never change, so hardcoding them
 * here is rather safe.
 */
bool char_def_alphabet_ext(unsigned int value)
{
	return (value == 0x0c ||
		value == '^' ||
		value == '{' ||
		value == '}' ||
		value == '\\' ||
		value == '[' ||
		value == '~' ||
		value == ']' ||
		value == '|' ||
		value == 0x20ac);
}

/**
 * char_def_alphabet_ext_count:
 * @input: input string
 * @lengh: input string length
 *
 * Returns: number of extended GSM alphabet characters in the input string
 */
int char_def_alphabet_ext_count(unsigned char *input, int length)
{
	int i, retval = 0;
	for (i = 0; i < length; i++)
		if (char_def_alphabet_ext(input[i]))
			retval++;
	return retval;
}

/**
 * char_def_alphabet_ext_decode:
 * @value: the character to decode
 *
 * Returns: the decoded character, or '?' if @value can't be decoded
 *
 * Converts a character from Extended GSM Alphabet to UCS-2.
 */
static unsigned int char_def_alphabet_ext_decode(unsigned char value)
{
	dprintf("Default extended alphabet\n");
	switch (value) {
	case 0x0a: return 0x000c; break; /* form feed */
	case 0x14: return 0x005e; break; /* ^ */
	case 0x28: return 0x007b; break; /* { */
	case 0x29: return 0x007d; break; /* } */
	case 0x2f: return 0x005c; break; /* \ */
	case 0x3c: return 0x005b; break; /* [ */
	case 0x3d: return 0x007e; break; /* ~ */
	case 0x3e: return 0x005d; break; /* ] */
	case 0x40: return 0x007c; break; /* | */
	case 0x65: return 0x20ac; break; /* € */
	default:   return 0x003f; break; /* invalid character, set ? */
	}
}

/**
 * char_def_alphabet_ext_encode:
 * @value: the UCS-2 character to encode
 *
 * Returns: the encoded character, or 0 if @value can't be encoded
 *
 * Converts a character from UCS-2 to Extended GSM Alphabet.
 */
static unsigned char char_def_alphabet_ext_encode(unsigned int value)
{
	switch (value) {
	case 0x0c: return 0x0a; /* form feed */
	case '^':  return 0x14;
	case '{':  return 0x28;
	case '}':  return 0x29;
	case '\\': return 0x2f;
	case '[':  return 0x3c;
	case '~':  return 0x3d;
	case ']':  return 0x3e;
	case '|':  return 0x40;
	case 0x20ac: return 0x65; /* euro */
	default: return 0x00; /* invalid character */
	}
}

/**
 * gn_char_def_alphabet:
 * @string: the string to test
 *
 * Returns: %true if the string can be represented with the GSM Alphabet,
 * %false otherwise
 *
 * Checks if @value is a string composed only by characters defined by
 * the default GSM alphabet or its extension.
 */
GNOKII_API int gn_char_def_alphabet(unsigned char *string)
{
	unsigned int i, ucs2len, inlen = strlen(string);
	char *ucs2str;

	/* First, let's know the encoding. We convert it from something to UCS-2 */
	ucs2str = calloc(2 * inlen, sizeof(unsigned char));
	if (!ucs2str)
		/* We are in trouble here. Whatever would be returned is irrelevant */
		return true;
	ucs2len = ucs2_encode(ucs2str, 2 * inlen, string, inlen);

	for (i = 0; i < ucs2len / 2; i++) {
		unsigned int a = 0xff & ucs2str[2 * i], b = 0xff & ucs2str[2 * i + 1];
		/*
		 * We need the following tests:
		 *  - check in the default alphabet table
		 *  - check in the extended default alphabet table
		 */
		if (!char_def_alphabet(256 * a + b) &&
		    !char_def_alphabet_ext(256 * a + b)) {
			free(ucs2str);
			return false;
		}
	}
	free(ucs2str);
	return true;
}

/**
 * char_def_alphabet_encode:
 * @value: the character to encode
 *
 * Returns: the encoded character, or '?' if @value can't be encoded
 *
 * Converts a character from UCS-2 to Default GSM Alphabet.
 * It could be possibly optimized but let's face it: nowadays full
 * lookup of 128 elements table is not that time consuming.
 */
unsigned char char_def_alphabet_encode(unsigned int value)
{
	int i;
	for (i = 0; i < GN_CHAR_UNI_ALPHABET_SIZE; i++) {
		if (gsm_default_unicode_alphabet[i] == value) {
			return i;
		}
	}
	return '?';
}

/**
 * char_def_alphabet_decode:
 * @value: the character to decode
 *
 * Returns: the decoded character or '?' if @value can't be decoded
 *
 * Converts a character from Default GSM Alphabet to UCS-2.
 */
unsigned int char_def_alphabet_decode(unsigned char value)
{
	if (value < GN_CHAR_UNI_ALPHABET_SIZE) {
		return gsm_default_unicode_alphabet[value];
	} else {
		return 0x003f; /* '?' */
	}
}

#define GN_BYTE_MASK ((1 << bits) - 1)

/**
 * char_7bit_unpack:
 * @offset: the bit offset inside the first byte of @input from which to start reading data
 * @in_length: length of @input in bytes
 * @out_length: size of @output in bytes
 * @input: buffer with the string to be converted
 * @output: buffer for the converted string, not NUL terminated
 *
 * Returns: the number of bytes used in @output
 *
 * Converts a packed sequence of 7-bit characters from @input into an array
 * of 8-bit characters in @output.
 * Source characters are stored in a char array of @in_length elements.
 */
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

/**
 * char_7bit_pack:
 * @offset: the bit offset inside the first byte of @output from which to start writing data
 * @input: buffer with the string to be converted
 * @output: buffer for the converted string, not NUL terminated
 * @in_len: length of @input to be set; includes extended alphabet escape char
 *
 * Returns: the number of bytes used in @output
 *
 * Converts an array of 8-bit characters from @input into a packed sequence
 * of 7-bit characters in @output.
 */
int char_7bit_pack(unsigned int offset, unsigned char *input,
		   unsigned char *output, unsigned int *in_len)
{

	unsigned char *out_num = output; /* Current pointer to the output buffer */
	unsigned int in_num;
	int bits;		     /* Number of bits directly copied to output buffer */
	unsigned int ucs2len, i = 0, len = strlen(input);
	char *ucs2str;

	/* FIXME: do it outside this function */
	/* FIXME: scheduled for 0.6.31 */
	/* First, let's know the encoding. We convert it from something to UCS-2 */
	ucs2str = calloc(2 * len, sizeof(unsigned char));
	if (!ucs2str)
		return 0;
	ucs2len = ucs2_encode(ucs2str, 2 * len, input, len);

	bits = (7 + offset) % 8;

	/* If we don't begin with 0th bit, we will write only a part of the
	   first octet */
	if (offset) {
		*out_num = 0x00;
		out_num++;
	}

	*in_len = 0;

	while (i < ucs2len / 2) {
		unsigned char byte;
		bool double_char = false;
		unsigned int a = 0xff & ucs2str[2 * i], b = 0xff & ucs2str[2 * i + 1];

		in_num = 256 * a + b;
		if (char_def_alphabet_ext(in_num)) {
			byte = GN_CHAR_UNI_ESCAPE;
			double_char = true;
			goto skip;
next_char:
			byte = char_def_alphabet_ext_encode(in_num);
			double_char = false;
			(*in_len) += 2;
		} else {
			byte = char_def_alphabet_encode(in_num);
			(*in_len)++;
		}
skip:
		*out_num = byte >> (7 - bits);
		/* If we don't write at 0th bit of the octet, we should write
		   a second part of the previous octet */
		if (bits != 7)
			*(out_num-1) |= (byte & ((1 << (7-bits)) - 1)) << (bits+1);

		bits--;

		if (bits == -1)
			bits = 7;
		else
			out_num++;

		if (double_char)
			goto next_char;

		i++;
	}

	free(ucs2str);
	return (out_num - output);
}

/**
 * char_default_alphabet_decode:
 * @dest: buffer for the converted string, NUL terminated
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes
 *
 * Converts a string from GSM Alphabet to ISO/IEC 8859-1.
 * In the worst case where each character in @src must be converted from the
 * Extended GSM Alphabet, size of @dest must be @len + 1; in general it must be
 * at least @len - number_of_escape_chars + 1
 */
int char_default_alphabet_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int j, pos = 0;
	MBSTATE mbs;

	MBSTATE_DEC_CLEAR(mbs);

	for (j = 0; j < len; j++) {
		wchar_t wc;
		int length;

		if (char_is_escape(src[j])) {
			wc = char_def_alphabet_ext_decode(src[++j]);
		} else {
			wc = char_def_alphabet_decode(src[j]);
		}
		length = char_uni_alphabet_decode(wc, dest, &mbs);
		dest += length;
		pos += length;
	}
	*dest = 0;
	return pos;
}

/**
 * char_ascii_encode:
 * @dest: buffer for the converted string, not NUL terminated
 * @dest_len: size of @dest in bytes, must be 2 * @len in the worst case
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes
 *
 * Returns: the number of bytes used in the @dest buffer for the converted string
 *
 * Converts a string from ISO/IEC 8859-1 to GSM Alphabet.
 * In the worst case where each character in @src must be converted in the
 * Extended GSM Alphabet, @dest_len must be @len * 2; in general it must be
 * at least @len + number_of_escape_chars
 */
size_t char_ascii_encode(char *dest, size_t dest_len, const char *src, size_t len)
{
	size_t i, j, extra = 0;

	for (i = 0, j = 0; i < dest_len && j < len; i++, j++) {
		if (char_def_alphabet_ext(src[j])) {
			dest[i++] = GN_CHAR_UNI_ESCAPE;
			dest[i] = char_def_alphabet_ext_encode(src[j]);
			extra++;
		} else {
			dest[i] = char_def_alphabet_encode(src[j]);
		}
	}
	return len + extra;
}

/**
 * char_hex_decode:
 * @dest: buffer for the converted string, NUL terminated
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes, length of @dest must be at least (@len / 2) + 1
 *
 * Converts a string from GSM Alphabet in ASCII-encoded hexadecimal bytes to ISO/IEC 8859-1.
 */
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

/**
 * char_hex_encode:
 * @dest: buffer for the converted string, NUL terminated
 * @dest_len: length of @dest in bytes, must be at least (@len * 2) + 1
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes
 *
 * Returns: the number of bytes used in the @dest buffer for the converted string
 *
 * Converts a string from ISO/IEC 8859-1 to GSM Alphabet in ASCII-encoded hexadecimal bytes.
 */
size_t char_hex_encode(char *dest, size_t dest_len, const char *src, size_t len)
{
	int i, n = dest_len / 2 >= len ? len : dest_len / 2;

	for (i = 0; i < n; i++)
		snprintf(dest + i * 2, 3, "%02X", char_def_alphabet_encode(src[i]));
	return len * 2;
}

/**
 * char_uni_alphabet_encode:
 * @value: pointer to the character to be converted
 * @n: maximum number of bytes of @value that will be examined
 * @dest: buffer for the converted character
 * @mbs: pointer to a variable holding the shift state
 * or NULL to use a global variable
 *
 * Returns: the number of bytes from @value used by the converted string
 * or -1 in case of error
 *
 * Converts a character from multibyte to wide.
 */
size_t char_uni_alphabet_encode(const char *value, size_t n, wchar_t *dest, MBSTATE *mbs)
{
	int length;

	length = char_mbtowc(dest, value, n, mbs);
	return length;
}

/**
 * char_uni_alphabet_decode:
 * @value: the character to be converted
 * @dest: buffer for the converted character
 * @mbs: pointer to a variable holding the shift state
 * or NULL to use a global variable
 *
 * Returns: the number of bytes from @value that have been used
 * or -1 in case of error
 *
 * Converts a character from wide to multibyte.
 */
int char_uni_alphabet_decode(wchar_t value, unsigned char *dest, MBSTATE *mbs)
{
	int length;

    switch (length = char_wctomb(dest, value, mbs)) {
	case -1:
		*dest = '?';
		length = 1;
	default:
		return length;
	}
}

/**
 * char_ucs2_decode:
 * @dest: buffer for the converted string, NUL terminated
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes, size of @dest must be at least (@len / 4) + 1
 *
 * Converts a string from UCS-2 encoded as ASCII-encoded hexadecimal bytes to ISO/IEC 8859-1.
 * @len must be a multiple of 4.
 * Used in AT driver for UCS2 encoding commands.
 */
void char_ucs2_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int i_len = 0, o_len = 0, length;
	char buf[5];
	MBSTATE mbs;

	MBSTATE_DEC_CLEAR(mbs);
	buf[4] = '\0';
	for (i_len = 0; i_len < len ; i_len++) {
		buf[0] = *(src + i_len * 4);
		buf[1] = *(src + i_len * 4 + 1);
		buf[2] = *(src + i_len * 4 + 2);
		buf[3] = *(src + i_len * 4 + 3);
		switch (length = char_uni_alphabet_decode(strtol(buf, NULL, 16), dest + o_len, &mbs)) {
		case -1:
			o_len++;
			length = 1;
			break;
		default:
			o_len += length;
			break;
		}
		if ((length == 1) && (dest[o_len-1] == 0))
			return;
	}
	dest[o_len] = 0;
	return;
}

/**
 * char_ucs2_encode:
 * @dest: buffer for the converted string, NUL terminated
 * @dest_len: size of @dest
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes, size of @dest must be at least (@len * 4) + 1
 *
 * Returns: the number of bytes of @dest that have been used
 *
 * Converts a string from ISO/IEC 8859-1 to UCS-2 encoded as ASCII-encoded hexadecimal bytes.
 * This function should convert "ABC" to "004100420043"
 * Used only in AT driver for UCS2 encoding commands.
 * It reads char by char from the input.
 */
#define UCS2_SIZE	4
size_t char_ucs2_encode(char *dest, size_t dest_len, const char *src, size_t len)
{
	wchar_t wc;
	int i, o_len, length;
	MBSTATE mbs;

	MBSTATE_ENC_CLEAR(mbs);
	for (i = 0, o_len = 0; i < len && o_len < dest_len / UCS2_SIZE; o_len++, i++) {
		/*
		 * We read input by convertible chunks. 'length' is length of
		 * the read chunk.
		 */
		length = char_uni_alphabet_encode(src + i, 1, &wc, &mbs);
		/* We stop reading after first unreadable input */
		if (length < 1)
			return o_len * UCS2_SIZE;
		/* We write here 4 chars + NULL termination */
		/* XXX: We should probably check wchar_t size. */
		snprintf(dest + (o_len * UCS2_SIZE), UCS2_SIZE + 1, "%04X", wc);
	}
	return o_len * UCS2_SIZE;
}

/**
 * char_unicode_decode:
 * @dest: buffer for the converted string, NUL terminated
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes
 *
 * Returns: the number of bytes of @dest that have been used
 *
 * Converts a string from UTF-8 to ISO/IEC 8859-1.
 */
unsigned int char_unicode_decode(unsigned char* dest, const unsigned char* src, int len)
{
	int i, length = 0, pos = 0;
	MBSTATE mbs;

	MBSTATE_DEC_CLEAR(mbs);
	for (i = 0; i < len / 2; i++) {
		wchar_t wc = src[i * 2] << 8 | src[(i * 2) + 1];
		length = char_uni_alphabet_decode(wc, dest, &mbs);
		dest += length;
		pos += length;
	}
	*dest = 0;
	return pos;
}

/**
 * char_unicode_encode:
 * @dest: buffer for the converted string, not NUL terminated
 * @src: buffer with the string to be converted
 * @len: length of @src in bytes
 *
 * Returns: the number of bytes of @dest that have been used
 *
 * Converts a string from ISO/IEC 8859-1 to UTF-8.
 */
unsigned int char_unicode_encode(unsigned char* dest, const unsigned char* src, int len)
{
	int pos = 0;
	MBSTATE mbs;
#ifndef HAVE_ICONV
	int length, offset = 0;
	wchar_t  wc;
#endif

	MBSTATE_ENC_CLEAR(mbs);
#ifdef HAVE_ICONV
	pos = ucs2_encode(dest, 2 * len, src, len);
#else
	while (offset < len) {
		length = char_uni_alphabet_encode(src + offset, len - offset, &wc, &mbs);
		switch (length) {
		case -1:
			dest[pos++] =  wc >> 8 & 0xFF;
			dest[pos++] =  wc & 0xFF;
			offset++;
			break;
		case 0: /* Avoid infinite loop */
			offset++;
			break;
		default:
			dest[pos++] =  wc >> 8 & 0xFF;
			dest[pos++] =  wc & 0xFF;
			offset += length;
			break;
		}
	}
#endif
	return pos;
}

/* Conversion bin -> hex and hex -> bin */

/**
 * hex2bin:
 * @dest: buffer for the converted string
 * @src: buffer with the string to be converted
 * @len: length of @src, size of @dest must be at least @len / 2
 *
 * Converts from ASCII-encoded hexadecimal bytes to binary.
 * @len must be a multiple of 2.
 */
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

/**
 * bin2hex:
 * @dest: buffer for the converted string, not NUL terminated
 * @src: buffer with the string to be converted
 * @len: length of @src, size of @dest must be at least @len * 2
 *
 * Converts from binary to ASCII-encoded hexadecimal bytes.
 */
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

/**
 * char_semi_octet_pack:
 * @number: string containing the phone number to convert
 * @output: buffer for the converted phone number, not NUL terminated
 * @type: type of the phone number (eg. %GN_GSM_NUMBER_International)
 *
 * Returns: the number of semi octects used by the whole encoded string
 *
 * This function implements packing of numbers (SMS Center number and
 * destination number) for SMS sending function.
 */
int char_semi_octet_pack(char *number, unsigned char *output, gn_gsm_number_type type)
{
	char *in_num = number;  /* Pointer to the input number */
	unsigned char *out_num = output; /* Pointer to the output */
	int count = 0; /* This variable is used to notify us about count of already
			  packed numbers. */

	/* The first byte in the Semi-octet representation of the address field is
	   the Type-of-Address. This field is described in the official GSM
	   specification 03.40 version 6.1.0, section 9.1.2.5, page 33. We support
	   only international, unknown and alphanumeric number. */

	*out_num++ = type;

	if (type == GN_GSM_NUMBER_Alphanumeric) {
		count = strlen(number);
		return 2 * char_7bit_pack(0, number, out_num, &count);
	}

	if ((type == GN_GSM_NUMBER_International || type == GN_GSM_NUMBER_Unknown) && *in_num == '+')
		in_num++; /* skip leading '+' */

	/* The next field is the number. It is in semi-octet representation - see
	   GSM specification 03.40 version 6.1.0, section 9.1.2.3, page 31. */
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

/**
 * char_bcd_number_get:
 * @number: a phone number encoded in BCD format
 *
 * Returns: a static buffer with the converted phone number, NUL terminated
 *
 * This function implements unpacking of numbers (SMS Center number and
 * destination number) for SMS receiving function.
 */
char *char_bcd_number_get(u8 *number)
{
	static char buffer[GN_BCD_STRING_MAX_LENGTH] = "";
	int length = number[0]; /* This is the length of BCD coded number */
	int count, digit, i = 0;

	if (length > GN_BCD_STRING_MAX_LENGTH) length = GN_BCD_STRING_MAX_LENGTH;
	switch (number[1]) {
	case GN_GSM_NUMBER_Alphanumeric:
		char_7bit_unpack(0, length, length, number + 2, buffer);
		buffer[length] = 0;
		break;
	case GN_GSM_NUMBER_International:
		snprintf(buffer, sizeof(buffer), "+");
		i++;
		if (length == GN_BCD_STRING_MAX_LENGTH)
			length--; /* avoid overflow */
	case GN_GSM_NUMBER_Unknown:
	case GN_GSM_NUMBER_National:
	case GN_GSM_NUMBER_Network:
	case GN_GSM_NUMBER_Subscriber:
	case GN_GSM_NUMBER_Abbreviated:
	default:
		/* start at 2 to skip length and TON (we can't overflow the buffer because i <= GN_BCD_STRING_MAX_LENGTH - 2) */
		for (count = 2; count <= length; count++) {
			digit = number[count] & 0x0f;
			if (digit < 0x0f)
				buffer[i++] = bcd_digits[digit];
			digit = number[count] >> 4;
			if (digit < 0x0f)
				buffer[i++] = bcd_digits[digit];
		}
		buffer[i] = '\0';
		break;
	}
	return buffer;
}

/* UTF-8 conversion functions */

/**
 * utf8_decode:
 * @outstring: buffer for the converted string, not NUL terminated
 * @outlen: size of @outstring
 * @instring: buffer with the string to be converted
 * @inlen: length of @instring
 *
 * Returns: the number of bytes used in @outstring, or -1 in case of errors
 *
 * Converts a string from UTF-8 to an application specified (or system default) encoding.
 * Uses iconv() if available, else uses internal replacement code.
 */
int utf8_decode(char *outstring, size_t outlen, const char *instring, size_t inlen)
{
	size_t nconv;

#if defined(HAVE_ICONV)
	ICONV_CONST char *pin;
	char *pout;
	iconv_t cd;

	pin = (char *)instring;
	pout = outstring;

	cd = iconv_open(gn_char_get_encoding(), "UTF-8");
	if (cd == (iconv_t)-1)
		return -1;
	nconv = iconv(cd, &pin, &inlen, &pout, &outlen);
	if (nconv == (size_t)-1)
		perror("utf8_decode/iconv");
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

/**
 * utf8_encode:
 * @outstring: buffer for the converted string, not NUL terminated
 * @outlen: size of @outstring
 * @instring: buffer with the string to be converted
 * @inlen: length of @instring
 *
 * Returns: the number of bytes used in @outstring, or -1 in case of errors
 *
 * Converts a string from an application specified (or system default) encoding to UTF-8.
 * Uses iconv() if available, else uses internal replacement code.
 */
int utf8_encode(char *outstring, int outlen, const char *instring, int inlen)
{
#if defined(HAVE_ICONV)
	size_t outleft, inleft, nconv;
	ICONV_CONST char *pin;
	char *pout;
	iconv_t cd;

	outleft = outlen;
	inleft = inlen;
	pin = (char *)instring;
	pout = outstring;

	cd = iconv_open("UTF-8", gn_char_get_encoding());
	if (cd == (iconv_t)-1)
		return -1;

	nconv = iconv(cd, &pin, &inleft, &pout, &outleft);
	if (nconv == (size_t)-1)
		perror("utf8_encode/iconv");
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

/* UCS-2 functions */

/**
 * ucs2_encode:
 * @outstring: buffer for the converted string, not NUL terminated
 * @outlen: size of @outstring
 * @instring: buffer with the string to be converted
 * @inlen: length of @instring
 *
 * Returns: the number of bytes used in @outstring, or -1 in case of errors
 *
 * Converts a string from an application specified (or system default) encoding to UCS-2.
 * Uses iconv() if available, else uses internal replacement code.
 */
int ucs2_encode(char *outstring, int outlen, const char *instring, int inlen)
{
#if defined(HAVE_ICONV)
	size_t outleft, inleft, nconv;
	ICONV_CONST char *pin;
	char *pout;
	iconv_t cd;

	outleft = outlen;
	inleft = inlen;
	pin = (char *)instring;
	pout = outstring;

	cd = iconv_open("UCS-2BE", gn_char_get_encoding());
	if (cd == (iconv_t)-1)
		return -1;

	nconv = iconv(cd, &pin, &inleft, &pout, &outleft);
	if (nconv == (size_t)-1)
		perror("ucs2_encode/iconv");
	iconv_close(cd);
	return (nconv < 0) ?  -1 : (char *)pout - outstring;
#else
	size_t nconv = char_unicode_encode(outstring, instring, inlen);

	return (nconv < 0) ?  -1 : nconv;
#endif
}


/* BASE64 functions */

/**
 * string_base64:
 * @instring: the string to check
 *
 * Returns: 1 if the string must be encoded in base 64, 0 otherwise
 *
 * Verifies if a string must be encoded in base 64.
 */
int string_base64(const char *instring)
{
	for (; *instring; instring++)
		if (*instring & 0x80)
			return 1;
	return 0;
}

/**
 * base64_encode:
 * @outstring: buffer for the converted string, will be NUL terminated
 * @outlen: size of @outstring
 * @instring: buffer with the string to be converted
 * @inlen: length of @instring
 *
 * Returns: the length of the converted string
 *
 * Converts a generic string to base64 encoding.
 * @outlen needs to be at least 4 / 3 times + 1 bigger than @inlen to hold
 * the converted string and the terminator.
 */
int base64_encode(char *outstring, int outlen, const char *instring, int inlen)
{
	const char *pin;
	char *pout;
	char *outtemp = NULL;
	int inleft, outleft;

	pout = outstring;
	inleft = inlen;
	outleft = outlen;
	pin = instring;

	/* This is in case someone passes a buffer not appropriate for outstring */
	while (outleft > 3 && inleft > 0) {
		int a, b, c;
		unsigned int i1, i2, i3, i4;

		a = *pin++;
		b = (inleft > 1) ? *(pin++) : 0;
		c = (inleft > 2) ? *(pin++) : 0;

		/* calculate the indexes */
		i1 = (a & 0xfc) >> 2;
		*(pout++) = base64_alphabet[i1];

		i2 = ((a & 0x03) << 4) | ((b & 0xf0) >> 4);
		*(pout++) = base64_alphabet[i2];

		inleft--;

		i3 = ((b & 0x0f) << 2) | ((c & 0xc0) >> 6);
		if (!inleft) {
			*(pout++) = '=';
		} else {
			*(pout++) = base64_alphabet[i3];
			inleft--;
		}

		i4 = c & 0x3f;
		if (!inleft)
			*(pout++) = '=';
		else {
			*(pout++) = base64_alphabet[i4];
			inleft--;
		}

		outleft -= 4;
	}

	/* terminate the output string */
	*pout = 0;

	if (outtemp)
		free(outtemp);

	return pout - outstring;
}

/**
 * base64_decode:
 * @dest: buffer for the converted string, will be NUL terminated
 * @destlen: size of @dest
 * @source: buffer with the string to be converted
 * @inlen: length of @source
 *
 * Returns: the number of bytes used in @dest
 *
 * Converts a generic string from base 64 encoding.
 * @destlen needs to be at least 3 / 4 + 1 of @inlen to hold the converted
 * string and the terminator.
 */
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

/**
 * utf8_base64_encode:
 * @dest: buffer for the converted string, NUL terminated
 * @destlen: size of @dest
 * @in: buffer with the string to be converted
 * @inlen: length of @in
 *
 * Returns: the number of bytes used by the converted string
 *
 * Converts a string from application default encoding to UTF-8 then to base 64.
 * @dest must be valid and big enough to hold the converted string.
 */
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

/**
 * utf8_base64_decode:
 * @dest: buffer for the converted string
 * @destlen: size of @dest
 * @in: buffer with the string to be converted
 * @inlen: length of @in
 *
 * Returns: the number of bytes used by the converted string
 *
 * Converts a string from base 64 to UTF-8 then to application default encoding.
 * @dest must be valid and big enough to hold the converted string.
 */
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

/**
 * add_slashes:
 * @dest: buffer for the converted string, NUL terminated
 * @src: buffer with the string to be converted
 * @maxlen: size of @dest, must be 2 * @len + 1 in the worst case
 * @len: length of @src
 *
 * Returns: the number of bytes used by the converted string
 *
 * Escapes the following characters (according to rfc 2426):
 * '\n', '\r', ';', ',', '\'.
 */
int add_slashes(char *dest, char *src, int maxlen, int len)
{
	int i, j;

	for (i = 0, j = 0; i < len && j < maxlen; i++, j++) {
		switch (src[i]) {
		case '\n':
			dest[j++] = '\\';
			dest[j] = 'n';
			break;
		case '\r':
			dest[j++] = '\\';
			dest[j] = 'r';
			break;
		case '\\':
		case ';':
		case ',':
			dest[j++] = '\\';
		default:
			dest[j] = src[i];
			break;
		}
	}
	dest[j] = 0;
	return j;
}

/**
 * strip_slashes:
 * @dest: buffer for the converted string, NUL terminated
 * @src: buffer with the string to be converted
 * @maxlen: size of @dest, must be @len + 1 in the worst case
 * @len: length of @src
 *
 * Returns: the number of bytes used by the converted string
 *
 * Unescapes the caracters escaped by add_slashes().
 */
int strip_slashes(char *dest, const char *src, int maxlen, int len)
{
	int i, j, slash_state = 0;

	for (i = 0, j = 0; i < len && j < maxlen; i++) {
		switch (src[i]) {
		case ';':
		case ',':
			if (slash_state) {
				slash_state = 0;
			}
			dest[j++] = src[i];
			break;
		case '\\':
			if (slash_state) {
				dest[j++] = src[i];
				slash_state = 0;
			} else {
				slash_state = 1;
			}
			break;
		case 'n':
			if (slash_state) {
				dest[j++] = '\n';
				slash_state = 0;
			} else {
				dest[j++] = src[i];
			}
			break;
		case 'r':
			if (slash_state) {
				dest[j++] = '\r';
				slash_state = 0;
			} else {
				dest[j++] = src[i];
			}
			break;
		default:
			if (slash_state) {
				dest[j++] = '\\';
				slash_state = 0;
			}
			dest[j++] = src[i];
			break;
		}
	}
	dest[j] = 0;
	return j;
}
