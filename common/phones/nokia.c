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
