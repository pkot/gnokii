/* -*- linux-c -*-

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

  Copyright (C) 2001 Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2001, 2002 Ladislav Michl <ladis@linux-mips.org>

 */

#ifndef _cbus_h
#define _cbus_h

#define CBUS_MAX_FRAME_LENGTH 256
#define CBUS_MAX_TRANSMIT_LENGTH 256
#define CBUS_MAX_MSG_LENGTH 256

typedef enum {
	CBUS_RX_Header,
	CBUS_RX_FrameType1,
	CBUS_RX_FrameType2,
	CBUS_RX_GetLengthLB,
	CBUS_RX_GetLengthHB,
	CBUS_RX_GetMessage,
	CBUS_RX_GetCSum,
	CBUS_RX_GetAck
} cbus_rx_state;

typedef enum {
	CBUS_PKT_None,
	CBUS_PKT_Ready,
	CBUS_PKT_CSumErr
} cbus_pkt_state;

typedef struct{
	int checksum;
	int buffer_count;
	cbus_rx_state state;
	int frame_header1;
	int frame_header2;
	int frame_type1;
	int frame_type2;
	int message_len;
	unsigned char buffer[CBUS_MAX_FRAME_LENGTH];
	unsigned char prev_rx_byte;
	unsigned char at_reply[CBUS_MAX_MSG_LENGTH];
} cbus_instance;

#define CBUSINST(s) ((cbus_instance *)((s)->link.link_instance))

gn_error cbus_initialise(struct gn_statemachine *state);

#endif   /* #ifndef _cbus_h */
