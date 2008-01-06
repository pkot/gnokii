/* -*- linux-c -*-

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

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2001      Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2001-2003 Ladislav Michl <ladis@linux-mips.org>
  Copyrught (C) 2002      Pawel Kot

 */

#ifndef _gnokii_links_cbus_h
#define _gnokii_links_cbus_h

#define CBUS_MAX_FRAME_LENGTH 256
#define CBUS_MAX_MSG_LENGTH 256

typedef enum {
	CBUS_RX_Header,
	CBUS_RX_FrameType1,
	CBUS_RX_FrameType2,
	CBUS_RX_LengthLB,
	CBUS_RX_LengthHB,
	CBUS_RX_Message,
	CBUS_RX_CSum,
	CBUS_RX_Ack
} cbus_rx_state;

typedef struct {
	cbus_rx_state state;
	int msg_len;
	int msg_pos;
	unsigned char prev_rx_byte;
	unsigned char frame_header1;
	unsigned char frame_header2;
	unsigned char frame_type1;
	unsigned char frame_type2;
	unsigned char unique;
	unsigned char csum;
	unsigned char msg[CBUS_MAX_MSG_LENGTH];
} cbus_instance;

#define CBUSINST(s) (*((cbus_instance **)(&(s)->link.link_instance)))

gn_error cbus_initialise(struct gn_statemachine *state);

#endif   /* #ifndef _gnokii_links_cbus_h */
