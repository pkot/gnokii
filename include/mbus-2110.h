/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for 2110 code.	

  $Log$
  Revision 1.21  2001-06-28 00:28:46  pkot
  Small docs updates (Pawel Kot)


*/

#ifndef		__mbus_2110_h
#define		__mbus_2110_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

	/* Global variables */
extern bool					MB21_LinkOK;
extern GSM_Functions		MB21_Functions;
extern GSM_Information		MB21_Information;

#endif	/* __mbus_2110_h */
