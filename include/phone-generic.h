/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones  
  See README for more details on supported mobile phones.

  The various routines are called PGEN_...

  Last modification: Thrs 7 Dec 2000
  Modified by Chris Kemp

*/

#include "gsm-common.h"

GSM_Error PGEN_CommandResponse(GSM_Link *link, void *message, int *messagesize, int messagetype, int waitfor, int messagealloc);

GSM_Error PGEN_CommandResponseReceive(GSM_Link *link, int MessageType, void *Message, int MessageLength);









