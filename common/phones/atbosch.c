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

  Copyright 2002 Ladislav Michl <ladis@psi.cz>

  This file provides functions specific to at commands on ericsson
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
#include "phones/atbosch.h"
#include "links/atbus.h"

static gn_error GetCharset(GSM_Data *data, GSM_Statemachine *state)
{
	data->Model = "HEX";
	return GN_ERR_NONE;
}

static gn_error SetCharset(GSM_Data *data, GSM_Statemachine *state)
{
	return GN_ERR_NONE;
}

static gn_error Unsupported(GSM_Data *data, GSM_Statemachine *state)
{
	return GN_ERR_NOTSUPPORTED;
}

void AT_InitBosch(GSM_Statemachine *state, char *foundmodel, char *setupmodel)
{
	AT_InsertRecvFunction(GOPAT_GetCharset, NULL);
	AT_InsertSendFunction(GOPAT_GetCharset, GetCharset);
	AT_InsertRecvFunction(GOPAT_SetCharset, NULL);
	AT_InsertSendFunction(GOPAT_SetCharset, SetCharset);
	/* phone lacks many usefull commands :( */
	AT_InsertSendFunction(GOP_GetBatteryLevel, Unsupported);
	AT_InsertSendFunction(GOP_GetRFLevel, Unsupported);
	AT_InsertSendFunction(GOP_GetSecurityCodeStatus, Unsupported);
	AT_InsertSendFunction(GOP_EnterSecurityCode, Unsupported);
}
