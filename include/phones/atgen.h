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

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atgen_h_
#define _gnokii_atgen_h_

#include "gnokii-internal.h"

typedef enum {
	GN_OP_AT_GetCharset = GN_OP_Max,
	GN_OP_AT_SetPDUMode,
	GN_OP_AT_Prompt,
	GN_OP_AT_Max	/* don't append anything after this entry */
} at_operation;

typedef enum {
	AT_CHAR_NONE,
	AT_CHAR_UNKNOWN,
	AT_CHAR_GSM,
	AT_CHAR_CP437,
	AT_CHAR_HEXGSM,
	AT_CHAR_HEX437,
	AT_CHAR_UCS2
} at_charset;

typedef gn_error (*at_recv_function_type)(int type, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
typedef gn_error (*at_send_function_type)(gn_data *data, struct gn_statemachine *state);

at_recv_function_type at_insert_recv_function(int type, at_recv_function_type func, struct gn_statemachine *state);
at_send_function_type at_insert_send_function(int type, at_send_function_type func, struct gn_statemachine *state);

typedef struct {
	at_send_function_type functions[GOPAT_Max];
	at_recv_function_type incoming_functions[GOPAT_Max];
	int if_pos;

	gn_memory_type memorytype;
	gn_memory _type smsmemorytype;
	at_charset defaultcharset;
	at_charset charset;
} at_driver_instance;

#define AT_DRVINST(s) ((at_driver_instance *)((s)->Phone.DriverInstance))

typedef struct {
	char *line1;
	char *line2;
	char *line3;
	char *line4; /* When reading SMS there are 4 ouput lines. Maybe create a table here? */
	int length;
} at_line_buffer;

gn_error AT_SetMemoryType(gn_memory_type mt, struct gn_statemachine *state);

void splitlines(at_line_buffer *buf);

char *skipcrlf(unsigned char *str);
char *findcrlf(unsigned char *str, int test, int maxlength);

#endif
