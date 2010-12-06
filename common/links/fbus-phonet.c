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
  Copyright (C) 2000-2001 Chris Kemp
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2001      Manfred Jonsson, Martin Jancar
  Copyright (C) 2002      Ladis Michl
  Copyright (C) 2002-2003 BORBELY Zoltan

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

  The various routines are called phonet_(whatever).

*/

#include "config.h"

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "device.h"
#include "links/fbus-phonet.h"
#include "links/fbus.h"
#include "links/utils.h"
#include "gnokii-internal.h"

static void phonet_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state);
static gn_error phonet_send_message(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state);


#define FBUSINST(s) (*((phonet_incoming_message **)(&(s)->link.link_instance)))

#define FBUS_PHONET_BLUETOOTH_INITSEQ 0xd0, 0x00, 0x01

/*--------------------------------------------*/

static bool phonet_open(struct gn_statemachine *state)
{
	int result, i, n, total = 0;
	unsigned char init_sequence[] = { FBUS_PHONET_BLUETOOTH_FRAME_ID,
					  FBUS_DEVICE_PHONE,
					  FBUS_PHONET_BLUETOOTH_DEVICE_PC,
					  FBUS_PHONET_BLUETOOTH_INITSEQ,
					  0x04};
	unsigned char init_resp[7];
	unsigned char init_pattern[7] = { FBUS_PHONET_BLUETOOTH_FRAME_ID,
					 FBUS_PHONET_BLUETOOTH_DEVICE_PC,
					 FBUS_DEVICE_PHONE,
					 FBUS_PHONET_BLUETOOTH_INITSEQ,
					 0x05};

	if (!state)
		return false;

	memset(&init_resp, 0, 7);

	/* Open device. */
	result = device_open(state->config.port_device, false, false, false,
			     state->config.connection_type, state);

	if (!result) {
		perror(_("Couldn't open PHONET device"));
		return false;
	}

	if (state->config.connection_type == GN_CT_Bluetooth) {
		device_write(&init_sequence, 7, state);
		while (total < 7) {
			n = device_read(&init_resp + total, 7 - total, state);
			if (n > 0)
				total += n;
		}
		for (i = 0; i < n; i++) {
			if (init_resp[i] != init_pattern[i]) {
				dprintf("Incorrect byte in the answer\n");
				return false;
			}
		}
	}

	return true;
}

/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */
static void phonet_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state)
{
	phonet_incoming_message *i = FBUSINST(state);

	/* FIXME: perhaps we should return something here */
	if (!i)
		return;

	switch (i->state) {
	case FBUS_RX_Sync:
		if (rx_byte == FBUS_PHONET_FRAME_ID ||
		    rx_byte == FBUS_PHONET_BLUETOOTH_FRAME_ID ||
		    rx_byte == FBUS_PHONET_DKU2_FRAME_ID) {
			i->state = FBUS_RX_GetDestination;
		}
		break;

	case FBUS_RX_GetDestination:
		i->message_destination = rx_byte;
		i->state = FBUS_RX_GetSource;

		if (rx_byte != FBUS_DEVICE_PC &&
		    rx_byte != FBUS_PHONET_DKU2_DEVICE_PC &&
		    rx_byte != FBUS_PHONET_BLUETOOTH_DEVICE_PC) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got 0x%02x\n", rx_byte);
		}
		break;

	case FBUS_RX_GetSource:
		i->message_source = rx_byte;
		i->state = FBUS_RX_GetType;

		if (rx_byte != FBUS_DEVICE_PHONE) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got 0x%02x\n", rx_byte);
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
		i->buffer_count = 0;
		if (i->message_length > PHONET_FRAME_MAX_LENGTH) {
			dprintf("PHONET: Message buffer overrun - resetting (message length: %d, max: %d)\n", i->message_length, PHONET_FRAME_MAX_LENGTH);
		}
		break;

	case FBUS_RX_GetMessage:
		if (i->message_length > PHONET_FRAME_MAX_LENGTH) {
			/* Ignore this frame that would overflow the buffer */
			if (i->buffer_count % 16 == 0)
				dprintf("\n");
			dprintf("%02x ", rx_byte);
			i->buffer_count++;

			if (i->buffer_count == i->message_length) {
				dprintf("\n");
				i->state = FBUS_RX_Sync;
			}
		} else {
			i->message_buffer[i->buffer_count] = rx_byte;
			i->buffer_count++;

			/* Is that it? */
			if (i->buffer_count == i->message_length) {
				sm_incoming_function(i->message_type, i->message_buffer, i->message_length, state);
				i->state = FBUS_RX_Sync;
			}
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
	unsigned char	buffer[BUFFER_SIZE + 16];
	int		count, res;

	res = device_select(timeout, state);

	if (res > 0) {
		res = device_read(buffer, sizeof(buffer), state);
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

	if (!state)
		return GN_ERR_FAILED;

	if (messagesize > PHONET_TRANSMIT_MAX_LENGTH) {
		return GN_ERR_MEMORYFULL;
	}

	/* Now construct the message header. */

	switch (state->config.connection_type) {
	case GN_CT_Bluetooth:
		out_buffer[current++] = FBUS_PHONET_BLUETOOTH_FRAME_ID;
		out_buffer[current++] = FBUS_DEVICE_PHONE;
		out_buffer[current++] = FBUS_PHONET_BLUETOOTH_DEVICE_PC;
		break;
	case GN_CT_DKU2:
	case GN_CT_DKU2LIBUSB:
		out_buffer[current++] = FBUS_PHONET_DKU2_FRAME_ID;
		out_buffer[current++] = FBUS_DEVICE_PHONE;
		out_buffer[current++] = FBUS_PHONET_DKU2_DEVICE_PC;
		break;
	default:
		out_buffer[current++] = FBUS_PHONET_FRAME_ID;
		out_buffer[current++] = FBUS_DEVICE_PHONE; /* Destination */
		out_buffer[current++] = FBUS_DEVICE_PC;    /* Source */
		break;
	}

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
		if (sent < 0) return GN_ERR_FAILED;
		else current += sent;
	} while (current < total);

	sm_incoming_acknowledge(state);

	return GN_ERR_NONE;
}

static void phonet_reset(struct gn_statemachine *state)
{
	/* Init variables */
	FBUSINST(state)->state = FBUS_RX_Sync;
	FBUSINST(state)->buffer_count = 0;
}

/* Initialise variables and start the link */
gn_error phonet_initialise(struct gn_statemachine *state)
{
	gn_error error = GN_ERR_FAILED;

	if (!state)
		return error;

	/* Fill in the link functions */
	state->link.loop = &phonet_loop;
	state->link.send_message = &phonet_send_message;
	state->link.reset = &phonet_reset;

	if ((FBUSINST(state) = calloc(1, sizeof(phonet_incoming_message))) == NULL)
		return GN_ERR_MEMORYFULL;

	switch (state->config.connection_type) {
	case GN_CT_Infrared:
	case GN_CT_Irda:
	case GN_CT_DKU2:
	case GN_CT_DKU2LIBUSB:
	case GN_CT_Bluetooth:
	case GN_CT_SOCKETPHONET:
		if (phonet_open(state) == true)
			error = GN_ERR_NONE;
		break;
	default:
		break;
	}
	if (error != GN_ERR_NONE) {
		free(FBUSINST(state));
		FBUSINST(state) = NULL;
		return error;
	}

	phonet_reset(state);

	return error;
}
