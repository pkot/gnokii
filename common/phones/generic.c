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

  $Log$
  Revision 1.3  2001-03-21 23:36:05  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.2  2001/03/06 10:39:35  machek
  Function for printing unknown packets can probably be shared across
  all phones.

  Revision 1.1  2001/02/21 19:57:07  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/16 14:29:53  chris
  Restructure of common/.  Fixed a problem in fbus-phonet.c
  Lots of dprintfs for Marcin
  Any size xpm can now be loaded (eg for 7110 startup logos)
  nk7110 code detects 7110/6210 and alters startup logo size to suit
  Moved Marcin's extended phonebook code into gnokii.c

  Revision 1.4  2001/02/03 23:56:17  chris
  Start of work on irda support (now we just need fbus-irda.c!)
  Proper unicode support in 7110 code (from pkot)

  Revision 1.3  2001/01/29 17:14:42  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.2  2001/01/23 15:32:42  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.1  2001/01/14 22:46:59  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


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
  
	dprintf(_("Message debug (type %02x):\n\r"), type);
	for (i = 0; i < len; i++) 
		if (isprint(mes[i])) dprintf(_("[%02x%c]"), mes[i], mes[i]);
		else dprintf(_("[%02x ]"), mes[i]);
	dprintf(_("\n\r"));

	return GE_NONE;
}



/* If we do not support a message type, print out some debugging info */

GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length)
{
	dprintf("Unknown Message received [type (%02x) length (%d): \n\r", messagetype, length);
	PGEN_DebugMessage(messagetype, buffer, length);

	return GE_NONE;
}
