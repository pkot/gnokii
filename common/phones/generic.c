/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PGEN_(whatever).

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gsm-common.h"
#include "phones/generic.h"
#include "misc.h"

/* If we do not support a message type, print out some debugging info */
GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length)
{
	dprintf("Unknown Message received [type (%02x) length (%d): \n", messagetype, length);
	SM_DumpMessage(messagetype, buffer, length);

	return GE_NONE;
}
