/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

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
#include "links/cbus.h"

static AT_SendFunctionType writephonebook;

static GSM_Error WritePhonebook(GSM_Data *data,  GSM_Statemachine *state)
{
	if (writephonebook == NULL)
		return GE_UNKNOWN;
	if (data->MemoryStatus->MemoryType == GMT_ME)
		return GE_NOTSUPPORTED;
	return (*writephonebook)(data, state);
}


void AT_InitNokia(GSM_Statemachine *state, char *foundmodel, char *setupmodel)
{
	/* block writing of phone memory on nokia phones other than */
	/* 8210. if you write to the phonebook of a eg 7110 all extended */
	/* information will be lost. */
	if (strncasecmp("8210", foundmodel, 4))
		writephonebook = AT_InsertSendFunction(GOP_WritePhonebook,  WritePhonebook);
}
