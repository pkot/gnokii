/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to at commands on siemens
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.1  2001-11-19 13:03:18  pkot
  nk3110.c cleanup

*/

void AT_InitSiemens(GSM_Statemachine *state, char* foundmodel, char* setupmodel);
