/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Manfred Jonsson

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones  
  See README for more details on supported mobile phones.

  The various routines are called PNOK_(whatever).

  $Log$
  Revision 1.1  2001-02-21 19:57:07  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/16 14:29:53  chris
  Restructure of common/.  Fixed a problem in fbus-phonet.c
  Lots of dprintfs for Marcin
  Any size xpm can now be loaded (eg for 7110 startup logos)
  nk7110 code detects 7110/6210 and alters startup logo size to suit
  Moved Marcin's extended phonebook code into gnokii.c

  Revision 1.1  2001/02/01 15:19:41  pkot
  Fixed --identify and added Manfred's manufacturer patch


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
	strncpy (manufacturer, "Nokia", PNOK_MAX_MANUFACTURER_LENGTH);
	return (GE_NONE);
}
