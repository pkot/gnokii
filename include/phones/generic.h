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

*/

#ifndef __phones_generic_h
#define __phones_generic_h

#include "gsm-error.h"

/* Generic Functions */

GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length);


#endif








