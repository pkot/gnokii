/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright 2004 Hugo Haas <hugo@larve.net>
  Copyright 2004 Pawel Kot
  Copyright 2007 Ingmar Steen <iksteen@gmail.com>

  This file provides functions specific to AT commands on Samsung
  phones. See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atsam.h"
#include "links/atbus.h"

static gn_error Unsupported(gn_data *data, struct gn_statemachine *state)
{
	return GN_ERR_NOTSUPPORTED;
}

void at_samsung_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	if (foundmodel) {
		if (!strncasecmp("SGH-L760", foundmodel, 8))
			AT_DRVINST(state)->ucs2_as_utf8 = 1;
		else if (!strncasecmp("SGH-U600", foundmodel, 8))
			AT_DRVINST(state)->extended_phonebook = 1;
		else if (!strncasecmp("SAMSUNG B2100", foundmodel, 13))
			AT_DRVINST(state)->encode_number = 1;
	}

	/* phone lacks many useful commands :( */
	at_insert_send_function(GN_OP_GetBatteryLevel, Unsupported, state);
	at_insert_send_function(GN_OP_GetPowersource, Unsupported, state);
	at_insert_send_function(GN_OP_GetRFLevel, Unsupported, state);
}

static char hexatoi(char c)
{
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	if ((c >= 'A') && (c <= 'F'))
		return c - 'A' + 10;
	if ((c >= 'a') && (c <= 'f'))
		return c - 'a' + 10;
	return 0;
}

/*
 * These are known weird encodings found in SGH-L760 model
 */
static unsigned char sgh_l760_fixup(unsigned char prev, unsigned char curr)
{
	switch (curr) {
	case 0x98:
		switch (prev) {
		case 0xC4:
			return 0x99;
		default:
			return curr;
		}
	case 0xA3:
		switch (prev) {
		case 0xC4:
			return 0x98;
		case 0xC5:
			return 0x94;
		default:
			return curr;
		}
	case 0xA9:
		switch (prev) {
		case 0xC5:
			return 0x95;
		default:
			return curr;
		}
	default:
		return curr;
	}
}

/*
 * Samsung SGH-l760 was reported to have incorrect encoding
 * With UCS-2 encoding for character ś it uses:
 *   00C5009B
 * instead of:
 *   015B
 * 0xC59B is UTF-8 representation of ś
 */
size_t decode_ucs2_as_utf8(char *dst, const char *src, int len)
{
	unsigned char *aux = calloc(len/4, sizeof(char));
	int i, j;

	/*
	 * First convert string to UTF-8 string:
	 *  in: 00C5009B
	 *  out: C59B
	 */
	for (i = 0, j = 0; i < len/4; i++) {
		unsigned char a, b;
		a = hexatoi(src[4 * i + 2]);
		b = hexatoi(src[4 * i + 3]);
		/* If there's 0x03 instead of 0x00 we have somehow different encoding */
		if ((src[4 * i] == 0x30) && (src[4 * i + 1] == 0x33)) {
			aux[j] = sgh_l760_fixup(aux[j-1], a * 16 + b);
			j++;
		} else {
			aux[j++] = a * 16 + b;
		}
	}
	return utf8_decode(dst, len/4, aux, len/4);
}
