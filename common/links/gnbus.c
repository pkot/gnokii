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

  Copyright (C) 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2004 BORBELY Zoltan

  This file provides an API for accessing functions via gnbus.
  See README for more details on supported mobile phones.

  The various routines are called gnbus_ (whatever).

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
#include "links/gnbus.h"
#include "links/utils.h"

#include "gnokii-internal.h"


/*
 * RX_State machine for receive handling. Called once for each character
 * received from the phone.
 */

static void gnbus_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state)
{
	struct timeval time_diff;
	gnbus_incoming_message *i = &GNBUSINST(state)->i;

	if (!i)
		return;

	/* XOR the byte with the current checksum */
	i->checksum[i->checksum_idx] ^= rx_byte;
	i->checksum_idx ^= 1;

	switch (i->state) {

		/*
		 * Messages from the phone start with a GNBUS_MAGIC_BYTE.
		 * We use this to "synchronise" with the incoming data stream.
		 * However, if we see something else, we assume we have lost
		 * sync and we require a gap of at least 5ms before we start
		 * looking again. This is because the data part of the
		 * frame could contain a byte which looks like the sync byte.
		 */

	case GNBUS_RX_Discarding:
		gettimeofday(&i->time_now, NULL);
		timersub(&i->time_now, &i->time_last, &time_diff);
		if (time_diff.tv_sec == 0 && time_diff.tv_usec < 5000) {
			i->time_last = i->time_now;	/* no gap seen, continue discarding */
			break;
		}

		/* else fall through to... */

	case GNBUS_RX_Sync:
		if (rx_byte == GNBUS_MAGIC_BYTE) {
			/* Initialize checksums. */
			i->checksum[0] = rx_byte;
			i->checksum[1] = 0;
			i->checksum_idx = 1;
			i->state = GNBUS_RX_GetSequence;
		} else {
			/* Lost frame sync */
			i->state = GNBUS_RX_Discarding;
			gettimeofday(&i->time_last, NULL);
		}

		break;

	case GNBUS_RX_GetSequence:

		i->sequence = rx_byte;
		i->state = GNBUS_RX_GetLength1;

		break;

	case GNBUS_RX_GetLength1:

		i->message_length = rx_byte << 8;
		i->state = GNBUS_RX_GetLength2;

		break;

	case GNBUS_RX_GetLength2:

		i->message_length += rx_byte;
		i->state = GNBUS_RX_GetType;

		break;

	case GNBUS_RX_GetType:

		i->message_type = rx_byte;
		i->state = GNBUS_RX_GetDummy;

		break;

	case GNBUS_RX_GetDummy:

		i->state = GNBUS_RX_GetMessage;
		if ((i->message_buffer = malloc(i->message_length + 3)) == NULL) {
			dprintf("GNBUS: receive buffer allocation failed, requested %d bytes.\n", i->message_length + 2);
			i->state = GNBUS_RX_Sync;
		}
		i->buffer_count = 0;

		break;

	case GNBUS_RX_GetMessage:

		i->message_buffer[i->buffer_count++] = rx_byte;

		/* If this is the last byte, it's the checksum. */

		if (i->buffer_count == ((i->message_length + 3) & ~1)) {
			/* Is the checksum correct? */
			if (i->checksum[0] == 0x00 && i->checksum[1] == 0x00) {

				sm_incoming_acknowledge(state);

				/* dispatch if ready */

				sm_incoming_function(i->message_type, i->message_buffer, i->message_length, state);
			} else {
				dprintf("GNBUS: Bad checksum!\n");
			}
			free(i->message_buffer);
			i->message_buffer = NULL;
			i->state = GNBUS_RX_Sync;
		}

		break;
	}
}


/*
 * This is the main loop function which must be called regularly
 * timeout can be used to make it 'busy' or not
 */

static gn_error gnbus_loop(struct timeval *timeout, struct gn_statemachine *state)
{
	unsigned char buffer[BUFFER_SIZE];
	int count, res;

	res = device_select(timeout, state);
	if (res > 0) {
		res = device_read(buffer, sizeof(buffer), state);
		for (count = 0; count < res; count++)
			gnbus_rx_statemachine(buffer[count], state);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}


/*
 * Prepares the message header and sends it, prepends the message
 * with a header, calculates checksum and then sends the lot down
 * the pipe...
 */

static gn_error gnbus_send_message(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state)
{
	unsigned char *out_buffer;
	int count, i = 0;
	unsigned char checksum[2];

	if (messagesize >= 0xfff0) {
		dprintf("GNBUS: message is too big to transmit, size: %d bytes\n", messagesize);
		return GN_ERR_MEMORYFULL;
	}

	if ((out_buffer = malloc(messagesize + 8)) == NULL) {
		dprintf("GNBUS: transmit buffer allocation failed, requested %d bytes.\n", messagesize + 8);
		return GN_ERR_MEMORYFULL;
	}

	/* Now construct the message header. */

	i = 0;
	out_buffer[i++] = GNBUS_MAGIC_BYTE;
	out_buffer[i++] = 0x00;			/* FIXME: sequence */
	out_buffer[i++] = messagesize >> 8;	/* Length MSB */
	out_buffer[i++] = messagesize & 0xff;	/* Length LSB */
	out_buffer[i++] = messagetype;		/* Type */
	out_buffer[i++] = 0x00;			/* Reserved */

	/* Copy in data if any. */

	if (messagesize != 0) {
		memcpy(out_buffer + i, message, messagesize);
		i += messagesize;
	}
	if (messagesize & 1) out_buffer[i++] = 0x00;

	/* Now calculate checksums over entire message and append to message. */

	checksum[0] = 0;
	checksum[1] = 0;
	for (count = 0; count < i; count++)
		checksum[count & 1] ^= out_buffer[count];

	out_buffer[i++] = checksum[0];
	out_buffer[i++] = checksum[1];

	/* Send it out... */

#if 0
	dprintf("GNBUS: Sending message 0x%02x / 0x%04x", messagetype, messagesize);
	for (count = 0; count < i; count++) {
		if (count % 16 == 0) dprintf("\n");
		dprintf(" %02x", out_buffer[count]);
	}
	dprintf("\n");
#endif

	if (device_write(out_buffer, i, state) != i) {
		free(out_buffer);
		return GN_ERR_INTERNALERROR;
	}

	free(out_buffer);
	return GN_ERR_NONE;
}


static void gnbus_reset(struct gn_statemachine *state)
{
	GNBUSINST(state)->i.state = GNBUS_RX_Sync;
	GNBUSINST(state)->i.checksum_idx = 0;
}

/*
 * Initialise variables and start the link
 */
gn_error gnbus_initialise(struct gn_statemachine *state)
{
	int conn_type;

	if (!state)
		return GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &gnbus_loop;
	state->link.send_message = &gnbus_send_message;
	state->link.reset = &gnbus_reset;

	/* Start up the link */
	if ((GNBUSINST(state) = calloc(1, sizeof(gnbus_link))) == NULL)
		return GN_ERR_MEMORYFULL;

	gnbus_reset(state);

	if (state->config.connection_type == GN_CT_Irda && strcasecmp(state->config.port_device, "IrDA:IrCOMM"))
		conn_type = GN_CT_Serial;
	else
		conn_type = state->config.connection_type;

	if (!device_open(state->config.port_device, false, false, false, conn_type, state)) {
		perror(_("Couldn't open GNBUS device"));
		free(GNBUSINST(state));
		GNBUSINST(state) = NULL;
		return GN_ERR_FAILED;
	}

	return GN_ERR_NONE;
}
