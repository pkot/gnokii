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
  Copyright (C) 2000 Chris Kemp

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

  The various routines are called phonet_(whatever).

*/


/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */
#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "device.h"
#include "links/fbus-phonet.h"
#include "gnokii-internal.h"

#ifdef HAVE_IRDA

static void phonet_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state);
static gn_error phonet_send_message(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state);


#define FBUSINST(s) ((phonet_incoming_message *)((s)->link.link_instance))


/*--------------------------------------------*/

static bool phonet_open(struct gn_statemachine *state)
{
	int result;

	/* Open device. */
	result = device_open(state->config.port_device, false, false, false, GN_CT_Irda, state);

	if (!result) {
		perror(_("Couldn't open PHONET device"));
		return false;
	}

	return (true);
}

/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

static void phonet_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state)
{
	phonet_incoming_message *i = FBUSINST(state);

	switch (i->state) {

	case FBUS_RX_Sync:
		if (rx_byte == FBUS_PHONET_FRAME_ID) {
			i->state = FBUS_RX_GetDestination;
		}
		break;

	case FBUS_RX_GetDestination:

		i->message_destination = rx_byte;
		i->state = FBUS_RX_GetSource;

		if (rx_byte != 0x0c) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got %2x\n", rx_byte);
		}
		break;

	case FBUS_RX_GetSource:

		i->message_source = rx_byte;
		i->state = FBUS_RX_GetType;

		/* Source should be 0x00 */

		if (rx_byte!=0x00)  {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got %2x\n", rx_byte);
		}

		break;

	case FBUS_RX_GetType:

		i->message_type = rx_byte;
		i->state = FBUS_RX_GetLength1;

		break;

	case FBUS_RX_GetLength1:

		i->message_length = rx_byte << 8;
		i->state = FBUS_RX_GetLength2;

		break;

	case FBUS_RX_GetLength2:

		i->message_length = i->message_length + rx_byte;
		i->state = FBUS_RX_GetMessage;
		i->buffer_count=0;

		break;

	case FBUS_RX_GetMessage:

		i->message_buffer[i->buffer_count] = rx_byte;
		i->buffer_count++;

		if (i->buffer_count > PHONET_FRAME_MAX_LENGTH) {
			dprintf("PHONET: Message buffer overun - resetting\n");
			i->state = FBUS_RX_Sync;
			break;
		}

		/* Is that it? */

		if (i->buffer_count == i->message_length) {
			sm_incoming_function(i->message_type, i->message_buffer, i->message_length, state);
			i->state = FBUS_RX_Sync;
		}
		break;
	default:
		i->state = FBUS_RX_Sync;
		break;
	}
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

static gn_error phonet_loop(struct timeval *timeout, struct gn_statemachine *state)
{
	gn_error	error = GN_ERR_INTERNALERROR;
	unsigned char	buffer[255];
	int		count, res;

	res = device_select(timeout, state);

	if (res > 0) {
		res = device_read(buffer, 255, state);
		for (count = 0; count < res; count++) {
			phonet_rx_statemachine(buffer[count], state);
		}
		if (res > 0) {
			error = GN_ERR_NONE;	/* This traps errors from device_read */
		}
	} else if (!res) {
		error = GN_ERR_TIMEOUT;
	}

	return error;
}

/* Main function to send an fbus message */

static gn_error phonet_send_message(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state)
{

	u8 out_buffer[PHONET_TRANSMIT_MAX_LENGTH + 5];
	int current = 0;
	int total, sent;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */

	out_buffer[current++] = FBUS_PHONET_FRAME_ID;
	out_buffer[current++] = FBUS_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FBUS_DEVICE_PC;    /* Source */

	out_buffer[current++] = messagetype; /* Type */

	out_buffer[current++] = (messagesize & 0xff00) >> 8;
	out_buffer[current++] = (messagesize & 0x00ff);

	/* Copy in data if any. */

	if (messagesize > 0) {
		memcpy(out_buffer + current, message, messagesize);
		current += messagesize;
	}

	/* Send it out... */

	total = current;
	current = 0;
	sent = 0;

	do {
		sent = device_write(out_buffer + current, total - current, state);
		if (sent < 0) return (false);
		else current += sent;
	} while (current < total);

	sm_incoming_acknowledge(state);

	return GN_ERR_NONE;
}



/* Initialise variables and start the link */

gn_error phonet_initialise(struct gn_statemachine *state)
{
	gn_error error = GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &phonet_loop;
	state->link.send_message = &phonet_send_message;

	if ((FBUSINST(state) = calloc(1, sizeof(phonet_incoming_message))) == NULL)
		return GN_ERR_MEMORYFULL;

	if ((state->config.connection_type == GN_CT_Infrared) || (state->config.connection_type == GN_CT_Irda)) {
		if (phonet_open(state) == true) {
			error = GN_ERR_NONE;
		}
	}
	if (error != GN_ERR_NONE) {
		free(FBUSINST(state));
		FBUSINST(state) = NULL;
		return error;
	}

	/* Init variables */
	FBUSINST(state)->state = FBUS_RX_Sync;
	FBUSINST(state)->buffer_count = 0;

	return error;
}

#else /* HAVE_IRDA */

gn_error phonet_initialise(struct gn_statemachine *state)
{
	return GN_ERR_NOTSUPPORTED;
}

#endif /* HAVE_IRDA */
