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

  Copyright (C) 2000      Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001-2003 Pawel Kot, BORBELY Zoltan

  This file provides functions specific to the Nokia 6100/5100 series.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_phones_nk6100_h
#define _gnokii_phones_nk6100_h

#include "gnokii.h"

/* Phone Memory types */

#define NK6100_MEMORY_MT 0x01 /* ?? combined ME and SIM phonebook */
#define NK6100_MEMORY_ME 0x02 /* ME (Mobile Equipment) phonebook */
#define NK6100_MEMORY_SM 0x03 /* SIM phonebook */
#define NK6100_MEMORY_FD 0x04 /* ?? SIM fixdialling-phonebook */
#define NK6100_MEMORY_ON 0x05 /* ?? SIM (or ME) own numbers list */
#define NK6100_MEMORY_EN 0x06 /* ?? SIM (or ME) emergency number */
#define NK6100_MEMORY_DC 0x07 /* ME dialed calls list */
#define NK6100_MEMORY_RC 0x08 /* ME received calls list */
#define NK6100_MEMORY_MC 0x09 /* ME missed (unanswered received) calls list */
#define NK6100_MEMORY_VOICE 0x0b /* Voice Mailbox */
/* This is used when the memory type is unknown. */
#define NK6100_MEMORY_XX 0xff

#define	NK6100_MAX_SMS_MESSAGES	12 /* maximum number of sms messages */

#define	NK6100_CAP_OLD_CALL_API	1
#define	NK6100_CAP_NBS_UPLOAD	2
#define NK6100_CAP_PB_UNICODE	4
#define	NK6100_CAP_OLD_KEY_API	8
#define	NK6100_CAP_NO_PSTATUS	16
#define	NK6100_CAP_NO_CB	32
#define	NK6100_CAP_CAL_UNICODE	64
#define	NK6100_CAP_NO_PB_GROUP	128 /* phone doesn't support categories in phonebook entries (caller groups) */

typedef struct {
	gn_key_code key;
	int repeat;
} nk6100_keytable;

typedef struct {
	/* callbacks */
	void (*on_cell_broadcast)(gn_cb_message *msg, struct gn_statemachine *state, void *callback_data);
	void (*call_notification)(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data);
	void (*rlp_rx_callback)(gn_rlp_f96_frame *frame);
	gn_error (*on_sms)(gn_sms *message, struct gn_statemachine *state, void *callback_data);

	unsigned char magic_bytes[4];
	bool sms_notification_in_progress;
	bool sms_notification_lost;
	gn_display_output *display_output;
	nk6100_keytable keytable[256];
	int capabilities;
	int max_sms;

	char model[GN_MODEL_MAX_LENGTH];
	char imei[GN_IMEI_MAX_LENGTH];
	char sw_version[10];
	char hw_version[10];
	gn_phone_model *pm;

	/* callback local data */
	void *cb_callback_data;	/* to be passed as callback_data to on_cell_broadcast */
	void *call_callback_data;	/* to be passed as callback_data to call_notification */
	void *sms_callback_data;	/* to be passed as callback_data to on_sms */
} nk6100_driver_instance;

void pnok_get_nokia_auth(unsigned char *imei, unsigned char *magic_bytes, unsigned char *magic_response);

#endif  /* #ifndef _gnokii_phones_nk6100_h */
