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

  Copyright (C) 2001-2002 Pavel Machek, Ladis Michl, Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2001 Chris Kemp
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2002-2004 BORBELY Zoltan

*/

#include "config.h"

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "links/utils.h"
#include "device.h"

gn_error link_terminate(struct gn_statemachine *state)
{
	if (!state)
		return GN_ERR_FAILED;

	/* device_close(&(state->Device)); */
	if (state->link.link_instance) {
		free(state->link.link_instance);
		state->link.link_instance = NULL;
	}
	device_close(state);
	return GN_ERR_NONE; /* FIXME */
}

void at_dprintf(char *prefix, char *buf, int len)
{
#ifdef DEBUG
	int in = 0, out = 0;
	char *pos = prefix;
	char debug_buf[1024];

	while (*pos)
		debug_buf[out++] = *pos++;
	debug_buf[out++] ='[';
	while ((in < len) && (out < 1016)) {
		if (buf[in] == '\n') {
			snprintf(debug_buf + out, sizeof(debug_buf) - out, "<lf>");
			in++;
			out += 4;
		} else if (buf[in] == '\r') {
			snprintf(debug_buf + out, sizeof(debug_buf) - out, "<cr>");
			in++;
			out += 4;
		} else if (buf[in] < 32) {
			debug_buf[out++] = '^';
			debug_buf[out++] = buf[in++] + 64;
		} else {
			debug_buf[out++] = buf[in++];
		}
	}
	debug_buf[out++] =']';
	debug_buf[out++] ='\n';
	debug_buf[out] ='\0';
	dprintf("%s", debug_buf);
#endif
}
