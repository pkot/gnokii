/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to at commands on siemens
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.2  2001-12-14 23:47:52  pkot
  Fixed fatal linker error -- global symbol conflict of writephonebook (Jan Kratochvil)

  Revision 1.1  2001/11/19 13:03:18  pkot
  nk3110.c cleanup

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


static GSM_Error WritePhonebook(GSM_Data *data,  GSM_Statemachine *state)
{
	GSM_PhonebookEntry newphone;
	char *rptr, *wptr;

	if (writephonebook == NULL)
		return GE_UNKNOWN;
	if (data->PhonebookEntry != NULL) {
		memcpy(&newphone, data->PhonebookEntry, sizeof(GSM_PhonebookEntry));
		rptr = data->PhonebookEntry->Name;
		wptr = newphone.Name;
		data->PhonebookEntry = &newphone;
	}
	return (*writephonebook)(data, state);
}


void AT_InitSiemens(GSM_Statemachine *state, char *foundmodel, char *setupmodel) {
	/* names for s35 etc must be escaped */
/*
	if (foundmodel && !strncasecmp("35", foundmodel + 1, 2))
		writephonebook = AT_InsertSendFunction(GOP_WritePhonebook, WritePhonebook);
*/
}
