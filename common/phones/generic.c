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

/* Useful debug function */
GSM_Error PGEN_DebugMessage(int type, unsigned char *mes, int len)
{
	int i;

	dprintf("Message debug (type %02x):\n", type);
	for (i = 0; i < len; i++)
		if (isprint(mes[i])) dprintf("[%02x%c]", mes[i], mes[i]);
		else dprintf("[%02x ]", mes[i]);
	dprintf("\n");

	return GE_NONE;
}


/* If we do not support a message type, print out some debugging info */

GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length)
{
	dprintf("Unknown Message received [type (%02x) length (%d): \n", messagetype, length);
	PGEN_DebugMessage(messagetype, buffer, length);

	return GE_NONE;
}
