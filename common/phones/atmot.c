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

  Copyright (C) 2006, 2007 Bastien Nocera <hadess@hadess.net>

  This file provides functions specific to at commands on motorola
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
#include "phones/atmot.h"

static at_recv_function_type identify;

static gn_error ReplyIdentify(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state)
{
	at_line_buffer buf;
	gn_error error;
	char *model;

	/* If we got incorrect answer, fallback to the default handler;
	 * we got error handling in there.
	 * strlen(buffer) < 2 is to avoid overflow with buffer + 1
	 */
	if (strlen(buffer) < 2 || strncmp(buffer + 1, "AT+CGMM", 7) != 0) {
		return (*identify)(messagetype, buffer, length, data, state);
	}

	if ((error = at_error_get(buffer, state)) != GN_ERR_NONE)
		return error;

	buf.line1 = buffer + 1;
	buf.length = length;
	splitlines(&buf);

	/* The line usually looks like:
	 * +CGMM: "GSM900","GSM1800","GSM1900","GSM850","MODEL=V547"
	 */
	model = strstr(buf.line2, "MODEL=");
	if (!model) {
		snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", strip_quotes(buf.line2 + 1 + strlen("+CGMM: ")));
	} else {
		snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", strip_quotes(model + strlen("MODEL=")));
		model = strchr(data->model, '"');
		if (model)
			*model = '\0';
	}

	return GN_ERR_NONE;
}

void at_motorola_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	/* Motorolas support mode 3 and 0, but mode 0 is pretty useless */
	AT_DRVINST(state)->cnmi_mode = 3;

	identify = at_insert_recv_function(GN_OP_Identify, ReplyIdentify, state);
}
