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

  This file provides functions specific to at commands on siemens
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
#include "phones/atsie.h"
#include "links/atbus.h"
#include "links/cbus.h"

static AT_SendFunctionType writephonebook;

static gn_error WritePhonebook(GSM_Data *data,  GSM_Statemachine *state)
{
	GSM_PhonebookEntry newphone;
	char *rptr, *wptr;

	if (writephonebook == NULL)
		return GN_ERR_UNKNOWN;
	if (data->PhonebookEntry != NULL) {
		memcpy(&newphone, data->PhonebookEntry, sizeof(GSM_PhonebookEntry));
		rptr = data->PhonebookEntry->Name;
		wptr = newphone.Name;
		data->PhonebookEntry = &newphone;
	}
	return (*writephonebook)(data, state);
}


void AT_InitSiemens(GSM_Statemachine *state, char *foundmodel, char *setupmodel)
{
	/* names for s35 etc must be escaped */
	if (foundmodel && !strncasecmp("35", foundmodel + 1, 2))
		writephonebook = AT_InsertSendFunction(GOP_WritePhonebook, WritePhonebook);
}
