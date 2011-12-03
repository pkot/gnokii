/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2000      Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001      Chris Kemp
  Copyright (C) 2002-2003 BORBELY Zoltan, Pawel Kot
  Copyright (C) 2002      Ladis Michl

  This file provides useful functions for all Nokia phones.
  See README for more details on supported mobile phones.

  The various routines are called PNOK_...

*/

#ifndef _gnokii_phones_nokia_h
#define _gnokii_phones_nokia_h

#include "gnokii.h"

#define	PNOK_MSG_ID_SMS 0x02

gn_error pnok_manufacturer_get(char *manufacturer);
void pnok_string_decode(unsigned char *dest, size_t max, const unsigned char *src, size_t len);
size_t pnok_string_encode(unsigned char *dest, size_t max, const unsigned char *src);

gn_error pnok_ringtone_from_raw(gn_ringtone *ringtone, const unsigned char *raw, int rawlen);
gn_error pnok_ringtone_to_raw(char *raw, int *rawlen, const gn_ringtone *ringtone, int dct4);

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
gn_error pnok_get_locks_info(gn_data *data, struct gn_statemachine *state);
gn_error pnok_play_tone(gn_data *data, struct gn_statemachine *state);
gn_error pnok_security_incoming(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

#endif
