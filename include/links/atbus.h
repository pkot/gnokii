/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#ifndef __atbus_h
#define __atbus_h

GSM_Error ATBUS_Initialise(GSM_Statemachine *state, int hw_handshake);

/* Define some result/error codes internal to the AT command functions.
   Also define a code for an unterminated message. */

typedef enum {
	GEAT_NONE,		/* NO or unknown result code */
	GEAT_PROMPT,		/* SMS command waiting for input */
	GEAT_OK,		/* Command succceded */
	GEAT_ERROR,		/* Command failed */
	GEAT_CMS,		/* SMS Command failed */
	GEAT_CME,		/* Extended error code found */
} GSMAT_Result;

#endif   /* #ifndef __atbus_h */
