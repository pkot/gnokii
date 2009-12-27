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

  Copyright (C) 2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2001 Manfred Jonsson <manfred.jonsson@gmx.de>
  Copyright (C) 2002-2008 Pawel Kot
  Copyright (C) 2003 Ladis Michl
  Copyright (C) 2004 Ron Yorston, Hugo Hass

  This file provides functions specific to generic AT command compatible
  phones. See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atgen_h_
#define _gnokii_atgen_h_

#include "gnokii-internal.h"
#include "map.h"

typedef enum {
	GN_OP_AT_GetCharset = GN_OP_Max,
	GN_OP_AT_SetCharset,
	GN_OP_AT_SetPDUMode,
	GN_OP_AT_Prompt,
	GN_OP_AT_GetMemoryRange,
	GN_OP_AT_Ring,
	GN_OP_AT_IncomingSMS,
	GN_OP_AT_GetSMSMemorySize,
	GN_OP_AT_PrepareDateTime,
	GN_OP_AT_Max	/* don't append anything after this entry */
} at_operation;

typedef enum {
	AT_CHAR_UNKNOWN		= 0x00,
	AT_CHAR_GSM		= 0x01,
	AT_CHAR_CP437		= 0x02,
	AT_CHAR_HEXGSM		= 0x04,
	AT_CHAR_HEX437		= 0x08,
	AT_CHAR_UCS2		= 0x10,
} at_charset;

typedef gn_error (*at_recv_function_type)(int type, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
typedef gn_error (*at_send_function_type)(gn_data *data, struct gn_statemachine *state);
typedef gn_error (*at_error_function_type)(int type, int code, struct gn_statemachine *state);

at_recv_function_type at_insert_recv_function(int type, at_recv_function_type func, struct gn_statemachine *state);
at_send_function_type at_insert_send_function(int type, at_send_function_type func, struct gn_statemachine *state);
at_error_function_type at_insert_manufacturer_error_function(at_error_function_type func, struct gn_statemachine *state);

typedef struct {
	at_send_function_type functions[GN_OP_AT_Max];
	gn_incoming_function_type incoming_functions[GN_OP_AT_Max];
	at_error_function_type manufacturer_error;
	int if_pos;
	int no_smsc;

	/* CBPS (phonebook related) */
	gn_memory_type memorytype;
	int memoryoffset;
	int memorysize;
	gn_memory_type smsmemorytype;
	at_charset availcharsets;
	at_charset defaultcharset;
	at_charset charset;

	/* CPMS (sms related) */
	int smmemorysize;
	int mememorysize;

	/* CNMI -- sms notifications */
	/* 0: buffer in TA;
	 * 1: discard indication and reject new SMs when TE-TA link is
	 *    reserved; otherwise forward directly;
	 * 2: buffer new Sms when TE-TA link is reserved and flush them to TE
	 *    after reservation; otherwise forward directly to the TE;
	 * 3: forward directly to TE;
	 */
	/* Default should be 3. Specific drivers can overwrite. */
	int cnmi_mode;

	/* For call notifications via AT+CLIP */
	int clip_supported;
	gn_call_type last_call_type;
	gn_call_status last_call_status;

	/* For AT+CPAS */
	gn_call_status prev_state;

	/* callbacks */
	void (*on_cell_broadcast)(gn_cb_message *msg, struct gn_statemachine *state, void *callback_data);
	void (*call_notification)(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data);
	gn_error (*on_sms)(gn_sms *message, struct gn_statemachine *state, void *callback_data);
	void (*reg_notification)(gn_network_info *info, void *callback_data);

	/* callback local data */
	/* to be passed as callback_data to on_cell_broadcast */
	void *cb_callback_data;
	/* to be passed as callback_data to call_notification */
	void *call_callback_data;
	/* to be passed as callback_data to on_sms */
	void *sms_callback_data;
	/* to be passed as callback_data to reg_notification */
	void *reg_callback_data;

	char *timezone;

	/* indicates whether phone is in PDU mode */
	int pdumode;

	/* cached information for AT+XXXX=? commands */
	struct map *cached_capabilities;

	/*
	 * Indicated whether phone supports extended registration status
	 * 0 - not known (we haven't asked yet)
	 * 1 - extended status not supported
	 * 2 - extended status supported
	 */
	int extended_reg_status;

	/*
	 * Some phones use encoding setting to encode just names and the other
	 * entities are encoded using ASCII (Nokia). Other phones encode every
	 * output with the given encoding and require the same for the input
	 * (Sony Ericsson).
	 */
	int encode_memory_type;
	int encode_number;

	/*
	 * Some phones reply with +CREG command with LAC information but with
	 * the bytes swapped. Default is the natural order.
	 */
	int lac_swapped;

	/*
	 * Some phones support extended phonebook commands:
	 * AT+SPBR/AT+SPBW
	 */
	int extended_phonebook;

	/*
	 * This is for weird encoding found in some Samsung phones.
	 */
	int ucs2_as_utf8;
} at_driver_instance;

#define AT_DRVINST(s) (*((at_driver_instance **)(&(s)->driver.driver_instance)))

typedef struct {
	char *line1;
	char *line2;
	char *line3;
	char *line4; /* When reading SMS there are 4 ouput lines. Maybe create a table here? */
	int length;
} at_line_buffer;

gn_error at_memory_type_set(gn_memory_type mt, struct gn_statemachine *state);
gn_error at_error_get(unsigned char *buffer, struct gn_statemachine *state);
gn_error at_set_charset(gn_data *data, struct gn_statemachine *state, at_charset charset);

/* There are shared between various AT drivers */
void splitlines(at_line_buffer *buf);
char *skipcrlf(unsigned char *str);
char *findcrlf(unsigned char *str, int test, int maxlength);
char *strip_quotes(char *s);

void at_decode(int charset, char *dst, char *src, int len, int ucs2_as_utf8);
size_t at_encode(at_charset charset, char *dst, size_t dst_len, const char *src, size_t len);

extern char *memorynames[];

#endif
