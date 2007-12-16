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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000      Chris Kemp
  Copyright (C) 2002      BORBELY Zoltan, Pawel Kot, Ladis Michl

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FBUS_(whatever).

*/

#ifndef _gnokii_links_fbus_h
#define _gnokii_links_fbus_h

#include <time.h>
#ifdef WIN32
#  include <sys/types.h>
#endif

#include "compat.h"

#include "fbus-common.h"
#include "gnokii.h"

#define FBUS_FRAME_MAX_LENGTH    256
#define FBUS_MESSAGE_MAX_TYPES   256
#define FBUS_TRANSMIT_MAX_LENGTH 256
#define FBUS_CONTENT_MAX_LENGTH  120

/* This byte is at the beginning of all GSM Frames sent over FBUS to Nokia
   phones.  This may have to become a phone dependant parameter... */
#define FBUS_FRAME_ID 0x1e

/* This byte is at the beginning of all GSM Frames sent over IR to Nokia phones. */
#define FBUS_IR_FRAME_ID 0x1c


/* Every (well, almost every) frame from the computer starts with this
   sequence. */

#define FBUS_FRAME_HEADER 0x00, 0x01, 0x00

typedef struct {
	int checksum[2];
	int buffer_count;
	struct timeval time_now;
	struct timeval time_last;
	enum fbus_rx_state state;
	int message_source;
	int message_destination;
	int message_type;
	int frame_length;
	u8 message_buffer[FBUS_FRAME_MAX_LENGTH];
} fbus_incoming_frame;

typedef struct {
	int message_length;
	unsigned char *message_buffer;
	char frames_to_go;
	int malloced;
} fbus_incoming_message;

typedef struct {
	u16 message_length;
	u8 message_type;
	u8 *buffer;
} fbus_outgoing_message;


typedef struct {
	fbus_incoming_frame i;
	fbus_incoming_message messages[FBUS_MESSAGE_MAX_TYPES];
	u8 request_sequence_number;
	u8 init_frame;
} fbus_link;

gn_error fbus_initialise(int attempt, struct gn_statemachine *state);

int fbus_tx_send_frame(u8 message_length, u8 message_type, u8 *buffer, struct gn_statemachine *state);

#endif /* #ifndef _gnokii_links_fbus_h */
