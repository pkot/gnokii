/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones  
  See README for more details on supported mobile phones.

  The various routines are called PGEN_...

  $Log$
  Revision 1.2  2001-03-11 11:12:21  machek
  Added header for PGEN_IncomingDefault.

  Revision 1.1  2001/02/21 19:57:12  chris
  More fiddling with the directory layout

  Revision 1.4  2001/01/29 17:14:44  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.3  2001/01/23 15:32:44  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.2  2001/01/17 02:54:56  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.


*/

#ifndef __phones_generic_h
#define __phones_generic_h

#include "gsm-common.h"
#include "misc.h"

/* Generic Functions */

GSM_Error PGEN_CommandResponse(GSM_Link *link, void *message, int *messagesize, int messagetype, int waitfor, int messagealloc);

GSM_Error PGEN_CommandResponseReceive(GSM_Link *link, int MessageType, void *Message, int MessageLength);
GSM_Error PGEN_DebugMessage(int type, unsigned char *mes, int len);
GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length);


#endif








