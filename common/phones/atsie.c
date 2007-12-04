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

  Copyright (C) 2001 Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002 Ladis Michl

  This file provides functions specific to at commands on siemens
  phones. See README for more details on supported mobile phones.

*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atsie.h"
#include "links/atbus.h"


static at_send_function_type writephonebook;

static gn_error WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	gn_phonebook_entry newphone;
	char *rptr, *wptr;

	if (writephonebook == NULL)
		return GN_ERR_UNKNOWN;
	if (data->phonebook_entry != NULL) {
		memcpy(&newphone, data->phonebook_entry, sizeof(gn_phonebook_entry));
		rptr = data->phonebook_entry->name;
		wptr = newphone.name;
		data->phonebook_entry = &newphone;
	}
	return (*writephonebook)(data, state);
}

void at_siemens_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	/* names for s35 etc must be escaped */
	if (foundmodel && !strncasecmp("35", foundmodel + 1, 2))
		writephonebook = at_insert_send_function(GN_OP_WritePhonebook, WritePhonebook, state);
}
