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

  $Log$
  Revision 1.5  2001-09-14 12:15:28  pkot
  Cleanups from 0.3.3 (part1)

  Revision 1.4  2001/09/09 21:45:49  machek
  Cleanups from Ladislav Michl <ladis@psi.cz>:

  *) do *not* internationalize debug messages

  *) some whitespace fixes, do not use //

  *) break is unneccessary after return

  Revision 1.3  2001/03/23 13:40:23  chris
  Pavel's patch and a few fixes.

  Revision 1.2  2001/03/21 23:36:05  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.1  2001/03/06 10:40:32  machek
  Added file with functions usefull for different links.

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
