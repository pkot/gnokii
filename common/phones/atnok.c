/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  This file provides functions specific to at commands on nokia
  phones. See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atnok.h"
#include "links/atbus.h"


static at_send_function_type writephonebook;

static gn_error WritePhonebook(gn_data *data, struct gn_statemachine *state)
{
	if (writephonebook == NULL)
		return GN_ERR_UNKNOWN;
	if (data->memory_status && data->memory_status->memory_type == GN_MT_ME)
		return GN_ERR_NOTSUPPORTED;
	return (*writephonebook)(data, state);
}

void at_nokia_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	/* block writing of phone memory on nokia phones other than */
	/* 8210. if you write to the phonebook of a eg 7110 all extended */
	/* information will be lost. */
	if (strncasecmp("8210", foundmodel, 4))
		writephonebook = at_insert_send_function(GN_OP_WritePhonebook, WritePhonebook, state);
}
