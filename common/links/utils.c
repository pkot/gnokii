/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FBUS_(whatever).

*/

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */
#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"

#if 0
void
link_dispatch(GSM_Link *glink, GSM_Phone *gphone, int type, u8 *buf, int len)
{
	int c;
	for (c = 0; c < gphone->IncomingFunctionNum; c++)
		if (gphone->IncomingFunctions[c].MessageType == type) {
			gphone->IncomingFunctions[c].Functions(type, buf, len);
			dprintf("Received message type %02x\n", type);

			/* FIXME - Hmm, what do we do with the return value.. */
			return;
		}
	dprintf("Unknown Frame Type %02x\n", type);
	gphone->DefaultFunction(type, buf, len);
}
#endif
