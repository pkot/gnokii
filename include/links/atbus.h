/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2001      Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002      Ladis Michl
  
*/

#ifndef _gnokii_atbus_h
#define _gnokii_atbus_h

#include "gnokii.h"

gn_error atbus_initialise(int mode, struct gn_statemachine *state);

/* 
 * Define some result/error codes internal to the AT command functions.
 * Also define a code for an unterminated message. 
 */
typedef enum {
	GN_AT_NONE,		/* NO or unknown result code */
	GN_AT_PROMPT,		/* SMS command waiting for input */
	GN_AT_OK,		/* Command succeded */
	GN_AT_ERROR,		/* Command failed */
	GN_AT_CMS,		/* SMS Command failed */
	GN_AT_CME,		/* Extended error code found */
} at_result;

#define RBUF_SEG	1024

typedef struct {
	/* The buffer for phone responses not only holds the data from the
	 * phone but also a byte which holds the compiled status of the 
	 * response. it is placed at [0]. */
	char *rbuf;
	int rbuf_size;
	int rbuf_pos;
	int binlen;
} atbus_instance;

#define AT_BUSINST(s) (*((atbus_instance **)(&(s)->link.link_instance)))

#endif   /* #ifndef _gnokii_atbus_h */
