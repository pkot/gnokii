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

  Copyright (C) 2000      Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000      Chris Kemp
  Copyright (C) 2001-2002 Pawel Kot
  Copyright (C) 2002      Ladis Michl

  This file provides useful functions for all phones.
  See README for more details on supported mobile phones.

  The various routines are called pgen_(whatever).

*/

#include "config.h"

#include "gnokii-internal.h"
#include "links/utils.h"

/* If we do not support a message type, print out some debugging info */
gn_error pgen_incoming_default(int messagetype, unsigned char *buffer, int length, struct gn_statemachine *state)
{
	dprintf("Unknown Message received [type (%02x) length (%d)]: \n", messagetype, length);
	sm_message_dump(gn_log_debug, messagetype, buffer, length);

	return GN_ERR_NONE;
}

gn_error pgen_terminate(gn_data *data, struct gn_statemachine *state)
{
	return link_terminate(state);
}
