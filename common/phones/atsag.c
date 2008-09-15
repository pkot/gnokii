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

  Copyright (C) 2008 Pawel Kot

  This file provides functions specific to at commands on Sagem phones.
  See README for more details on supported mobile phones.

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
#include "phones/atsag.h"

static gn_error AT_GetModel(gn_data *data, struct gn_statemachine *state)
{
	/* Sagem myW-8 replies to "AT+GMM" with "+CGMM: myW-7 UMTS" and to "AT+CGMM" with "+CGMM: myW-8 UMTS"
	   so don't use the first command that gives the wrong answer and isn't correctly
	   handled by ReplyIdentify() in atgen.c because of the mismatch AT+GMM / +CGMM
	 */

	if (sm_message_send(8, GN_OP_Identify, "AT+CGMM\r", state))
		return GN_ERR_NOTREADY;
	return sm_block_no_retry(GN_OP_Identify, data, state);
}

void at_sagem_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	AT_DRVINST(state)->lac_swapped = 1;
	AT_DRVINST(state)->encode_number = 1;

	/* fix for Sagem myW-8 returning the wrong answer to GN_OP_GetModel */
	at_insert_send_function(GN_OP_GetModel, AT_GetModel, state);
}
