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

  $Log$
  Revision 1.1  2001-02-01 15:19:41  pkot
  Fixed --identify and added Manfred's manufacturer patch


*/

#ifndef __phone_nokia_h
#define __phone_nokia_h


#define PNOK_MAX_MANUFACTURER_LENGTH 16 

GSM_Error PNOK_GetManufacturer(char *manufacturer);

#endif
