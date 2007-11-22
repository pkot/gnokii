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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pavel Machek, Chris Kemp
  Copyright (C) 2002 Pawel Kot, Ladis Michl, Manfred Jonsson

  This file provides functions common to all parts in the link layer.
  See README for more details on supported mobile phones.


*/

#ifndef _gnokii_links_utils_h
#define _gnokii_links_utils_h

#include "gnokii.h"

/* Needs to be a multiplication of 64 */
#define BUFFER_SIZE	256

gn_error link_terminate(struct gn_statemachine *state);
void at_dprintf(char *prefix, char *buf, int len);

#endif
