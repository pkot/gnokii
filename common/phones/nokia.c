/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Manfred Jonsson
  Copyright (C) 2002 BORBELY Zoltan

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PNOK_(whatever).

  The auth code is written specially for gnokii project by Odinokov Serge.
  If you have some special requests for Serge just write him to
  apskaita@post.omnitel.net or serge@takas.lt

  Reimplemented in C by Pavel Janík ml.
*/

#include <string.h>

#include "gsm-common.h"
#include "phones/nokia.h"


/* This function provides a way to detect the manufacturer of a phone */
/* because it is only used by the fbus/mbus protocol phones and only */
/* nokia is using those protocols, the result is clear. */
/* the error reporting is also very simple */
/* the strncpy and PNOK_MAX_MODEL_LENGTH is only here as a reminder */

GSM_Error PNOK_GetManufacturer(char *manufacturer)
{
	strcpy(manufacturer, "Nokia");
	return (GE_NONE);
}


void PNOK_DecodeString(unsigned char *dest, size_t max, const unsigned char *src, size_t len)
{
	int i, n;

	n = (len >= max) ? max-1 : len;

	for (i = 0; i < n; i++)
		dest[i] = src[i];
	dest[i] = 0;
}

size_t PNOK_EncodeString(unsigned char *dest, size_t max, const unsigned char *src)
{
	size_t i;

	for (i = 0; i < max && src[i]; i++)
		dest[i] = src[i];
	return i;
}

/* Nokia authentication protocol is used in the communication between Nokia
   mobile phones (e.g. Nokia 6110) and Nokia Cellular Data Suite software,
   commercially sold by Nokia Corp.

   The authentication scheme is based on the token send by the phone to the
   software. The software does it's magic (see the function
   FB61_GetNokiaAuth()) and returns the result back to the phone. If the
   result is correct the phone responds with the message "Accessory
   connected!" displayed on the LCD. Otherwise it will display "Accessory not
   supported" and some functions will not be available for use.

   The specification of the protocol is not publicly available, no comment. */

void PNOK_GetNokiaAuth(unsigned char *Imei, unsigned char *MagicBytes, unsigned char *MagicResponse)
{
	int i, j, CRC=0;
	unsigned char Temp[16]; /* This is our temporary working area. */

	/* Here we put FAC (Final Assembly Code) and serial number into our area. */

	memcpy(Temp, Imei + 6, 8);

	/* And now the TAC (Type Approval Code). */

	memcpy(Temp + 8, Imei + 2, 4);

	/* And now we pack magic bytes from the phone. */

	memcpy(Temp + 12, MagicBytes, 4);

	for (i = 0; i <= 11; i++)
		if (Temp[i + 1] & 1)
			Temp[i]<<=1;

	switch (Temp[15] & 0x03) {
	case 1:
	case 2:
		j = Temp[13] & 0x07;
		for (i = 0; i <= 3; i++)
			Temp[i+j] ^= Temp[i+12];
		break;

	default:
		j = Temp[14] & 0x07;
		for (i = 0; i <= 3; i++)
			Temp[i + j] |= Temp[i + 12];
	}

	for (i = 0; i <= 15; i++)
		CRC ^= Temp[i];

	for (i = 0; i <= 15; i++) {
		switch (Temp[15 - i] & 0x06) {
		case 0:
			j = Temp[i] | CRC;
			break;

		case 2:
		case 4:
			j = Temp[i] ^ CRC;
			break;

		case 6:
			j = Temp[i] & CRC;
			break;
		}

		if (j == CRC) j = 0x2c;

		if (Temp[i] == 0) j = 0;

		MagicResponse[i] = j;
	}
}
