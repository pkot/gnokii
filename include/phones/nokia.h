/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Manfred Jonsson

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PNOK_...

*/

#ifndef __phones_nokia_h
#define __phones_nokia_h

#include "gsm-error.h"

GSM_Error PNOK_GetManufacturer(char *manufacturer);
void PNOK_DecodeString(unsigned char *dest, size_t max, const unsigned char *src, size_t len);
size_t PNOK_EncodeString(unsigned char *dest, size_t max, const unsigned char *src);

#endif
