/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Manfred Jonsson

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  The various routines are called PNOK_...

*/

#ifndef _gnokii_phones_nokia_h
#define _gnokii_phones_nokia_h

#include "gnokii/error.h"

#define	PNOK_MSG_ID_SMS 0x02

gn_error pnok_manufacturer_get(char *manufacturer);
void pnok_string_decode(unsigned char *dest, size_t max, const unsigned char *src, size_t len);
size_t pnok_string_encode(unsigned char *dest, size_t max, const unsigned char *src);

/* Common functions for misc Nokia drivers */
/* Call divert: nk6100, nk7110 */
gn_error pnok_call_divert(gn_data *data, struct gn_statemachine *state);
gn_error pnok_call_divert_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
int pnok_fbus_sms_encode(unsigned char *req, gn_data *data, struct gn_statemachine *state);

/* Security functions: nk6100, nk7100 */
gn_error pnok_extended_cmds_enable(unsigned char type, gn_data *data, struct gn_statemachine *state);
gn_error pnok_call_make(gn_data *data, struct gn_statemachine *state);
gn_error pnok_call_answer(gn_data *data, struct gn_statemachine *state);
gn_error pnok_call_cancel(gn_data *data, struct gn_statemachine *state);
gn_error pnok_netmonitor(gn_data *data, struct gn_statemachine *state);
gn_error pnok_security_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

#endif
