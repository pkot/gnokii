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
  Copyright (C) 2002 BORBELY Zoltan

  This file provides an API for accessing functions via m2bus.
  See README for more details on supported mobile phones.

  The various routines are called m2bus_(whatever).

*/

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */

#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "gsm-statemachine.h"
#include "device.h"
#include "links/m2bus.h"

#include "gnokii-internal.h"

static void m2bus_rx_statemachine(unsigned char rx_byte);
static gn_error m2bus_send_message(u16 messagesize, u8 messagetype, unsigned char *message);
static int m2bus_tx_send_ack(u8 message_seq);

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */

/* Some globals */

static gn_link *glink;
static struct gn_statemachine *statemachine;
static m2bus_link flink;	/* M2BUS specific stuff, internal to this file */


/*--------------------------------------------*/

static bool m2bus_serial_open(void)
{
	/* Open device. */
	if (!device_open(glink->port_device, true, false, false, GN_CT_Serial)) {
		perror(_("Couldn't open M2BUS device"));
		return false;
	}
	device_changespeed(9600);

	/*
	 * Need to "toggle" the dtr/rts lines in the right sequence it seems
	 * for the interface to work. Base time value is units of 50ms it
	 * seems.
	 */
	device_setdtrrts(0, 1);

	return true;
}


/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */
static void m2bus_rx_statemachine(unsigned char rx_byte)
{
	struct timeval time_diff;
	m2bus_incoming_message *i = &flink.i;

#if 0
	dprintf("rx_byte: %02x, state: %d\n", rx_byte, i->state);
#endif

	/* XOR the byte with the current checksum */
	i->checksum ^= rx_byte;

	switch (i->state) {

		/* Messages from the phone start with an 0x1f (cable) or 0x14 (IR).
		   We use this to "synchronise" with the incoming data stream. However,
		   if we see something else, we assume we have lost sync and we require
		   a gap of at least 5ms before we start looking again. This is because
		   the data part of the frame could contain a byte which looks like the
		   sync byte */

	case M2BUS_RX_Discarding:
		gettimeofday(&i->time_now, NULL);
		timersub(&i->time_now, &i->time_last, &time_diff);
		if (time_diff.tv_sec == 0 && time_diff.tv_usec < 5000) {
			i->time_last = i->time_now;	/* no gap seen, continue discarding */
			break;
		}

		/* else fall through to... */

	case M2BUS_RX_Sync:
		if (glink->connection_type == GN_CT_Infrared) {
			if (rx_byte == M2BUS_IR_FRAME_ID) {
				/* Initialize checksums. */
				i->checksum = M2BUS_IR_FRAME_ID;
				i->state = M2BUS_RX_GetDestination;
			} else {
				/* Lost frame sync */
				i->state = M2BUS_RX_Discarding;
				gettimeofday(&i->time_last, NULL);
			}

		} else {	/* glink->connection_type == GN_CT_Serial */
			if (rx_byte == M2BUS_FRAME_ID) {
				/* Initialize checksums. */
				i->checksum = M2BUS_FRAME_ID;
				i->state = M2BUS_RX_GetDestination;
			} else {
				/* Lost frame sync */
				i->state = M2BUS_RX_Discarding;
				gettimeofday(&i->time_last, NULL);
			}
		}

		break;

	case M2BUS_RX_GetDestination:

		i->message_destination = rx_byte;
		i->state = M2BUS_RX_GetSource;

		/*
		 * When there is a checksum error and things get out of sync
		 * we have to manage to resync. If doing a data call at the
		 * time, finding a 0x1f etc is really quite likely in the data
		 * stream. Then all sorts of horrible things happen because
		 * the packet length etc is wrong... Therefore we test here
		 * for a destination of 0x00 or 0x1d and return to the top if
		 * it is not
		 */

		if (rx_byte != M2BUS_DEVICE_PC && rx_byte != M2BUS_DEVICE_PHONE) {
			i->state = M2BUS_RX_Sync;
			dprintf("The m2bus stream is out of sync - expected destination, got %2x\n", rx_byte);
		}

		break;

	case M2BUS_RX_GetSource:

		i->message_source = rx_byte;
		i->state = M2BUS_RX_GetType;

		if (i->message_destination == M2BUS_DEVICE_PC && rx_byte != M2BUS_DEVICE_PHONE) {
			i->state = M2BUS_RX_Sync;
			dprintf("The m2bus stream is out of sync - expected source=PHONE, got %2x\n", rx_byte);
		}
		else if (i->message_destination == M2BUS_DEVICE_PHONE && rx_byte != M2BUS_DEVICE_PC) {
			i->state = M2BUS_RX_Sync;
			dprintf("The m2bus stream is out of sync - expected source=PC, got %2x\n", rx_byte);
		}

		break;

	case M2BUS_RX_GetType:

		i->message_type = rx_byte;
		if (rx_byte == 0x7f) {
			i->message_length = 0;
			i->state = M2BUS_RX_GetMessage;
			i->buffer_count = 0;
			if ((i->message_buffer = malloc(2)) == NULL) {
				dprintf("M2BUS: receive buffer allocation failed, requested %d bytes.\n", 2);
				i->state = M2BUS_RX_Sync;
			}
		} else i->state = M2BUS_RX_GetLength1;

		break;

	case M2BUS_RX_GetLength1:

		i->message_length = rx_byte << 8;
		i->state = M2BUS_RX_GetLength2;

		break;

	case M2BUS_RX_GetLength2:

		i->message_length += rx_byte;
		i->state = M2BUS_RX_GetMessage;
		i->buffer_count = 0;
		if ((i->message_buffer = malloc(i->message_length + 2)) == NULL) {
			dprintf("M2BUS: receive buffer allocation failed, requested %d bytes.\n", i->message_length + 2);
			i->state = M2BUS_RX_Sync;
		}

		break;

	case M2BUS_RX_GetMessage:

		i->message_buffer[i->buffer_count++] = rx_byte;

		/* If this is the last byte, it's the checksum. */

		if (i->buffer_count == i->message_length + 2) {
			/* Is the checksum correct? */
			if (i->checksum == 0x00) {

				/* Deal with exceptions to the rules - acks and rlp.. */

				if (i->message_destination != M2BUS_DEVICE_PC) {
					/* echo */
				} else if (i->message_type == 0x7f) {
					dprintf("[Received Ack, seq: %2x]\n", i->message_buffer[0]);

				} else {	/* Normal message type */

					/* Send an ack (for all for now) */

					m2bus_tx_send_ack(i->message_buffer[i->message_length]);

					/* Finally dispatch if ready */

					sm_incoming_function(statemachine, i->message_type, i->message_buffer, i->message_length);
				}
			} else {
				dprintf("M2BUS: Bad checksum!\n");
			}
			free(i->message_buffer);
			i->message_buffer = NULL;
			i->state = M2BUS_RX_Sync;
		}
		break;
	}
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

static gn_error m2bus_loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, sizeof(buffer));
		for (count = 0; count < res; count++)
			m2bus_rx_statemachine(buffer[count]);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}


static void m2bus_wait_for_idle(int timeout, bool reset)
{
	int n, prev;

	device_nreceived(&n);
	do {
		prev = n;
		usleep(timeout);
		if (device_nreceived(&n) != GN_ERR_NONE) break;
	} while (n != prev);

	if (reset) {
		device_setdtrrts(0, 0);
		usleep(200000);
		device_setdtrrts(0, 1);
		usleep(100000);
	}
}


/* Prepares the message header and sends it, prepends the message start byte
	   (0x1f) and other values according the value specified when called.
	   Calculates checksum and then sends the lot down the pipe... */

static gn_error m2bus_send_message(u16 messagesize, u8 messagetype, unsigned char *message)
{
	u8 *out_buffer;
	int count, i = 0;
	u8 checksum;

	if (messagesize > 0xffff) {
		dprintf("M2BUS: message is too big to transmit, size: %d bytes\n", messagesize);
		return GN_ERR_MEMORYFULL;
	}

	if ((out_buffer = malloc(messagesize + 8)) == NULL) {
		dprintf("M2BUS: transmit buffer allocation failed, requested %d bytes.\n", messagesize + 8);
		return GN_ERR_MEMORYFULL;
	}

	/*
	 * Checksum problem:
	 * It seems  that some phones have problems with a checksum of 1F.
	 * The frame will be recognized but it will not respond with a ACK
	 * frame.
	 *
	 * Workaround:
	 * If the checksum will be 1F, increment the sequence number so that
	 * the checksum will be different., recalculate the checksum then and
	 * send.
	 *
	 * Source: http://www.flosys.com/tdma/n5160.html
	 */

	do {
		/* Now construct the message header. */

		i = 0;
		if (glink->connection_type == GN_CT_Infrared)
			out_buffer[i++] = M2BUS_IR_FRAME_ID;	/* Start of the IR frame indicator */
		else			/* connection_type == GN_CT_Serial */
			out_buffer[i++] = M2BUS_FRAME_ID;	/* Start of the frame indicator */

		out_buffer[i++] = M2BUS_DEVICE_PHONE;	/* Destination */
		out_buffer[i++] = M2BUS_DEVICE_PC;	/* Source */

		out_buffer[i++] = messagetype;		/* Type */

		out_buffer[i++] = messagesize >> 8;	/* Length MSB */
		out_buffer[i++] = messagesize & 0xff;	/* Length LSB */

		/* Copy in data if any. */

		if (messagesize != 0) {
			memcpy(out_buffer + i, message, messagesize);
			i += messagesize;
		}

		out_buffer[i++] = flink.request_sequence_number++;
		if (flink.request_sequence_number > 63)
			flink.request_sequence_number = 2;

		/* Now calculate checksums over entire message and append to message. */

		checksum = 0;
		for (count = 0; count < i; count++)
			checksum ^= out_buffer[count];

		out_buffer[i++] = checksum;

	} while (checksum == 0x1f);

	/* Send it out... */

#if 0
	dprintf("M2BUS: Sending message 0x%02x / 0x%04x", messagetype, messagesize);
	for (count = 0; count < i; count++) {
		if (count % 16 == 0) dprintf("\n");
		dprintf(" %02x", out_buffer[count]);
	}
	dprintf("\n");
#endif

	m2bus_wait_for_idle(5000, true);

	if (device_write(out_buffer, i) != i) {
		free(out_buffer);
		return GN_ERR_INTERNALERROR;
	}

	device_flush();

	free(out_buffer);
	return GN_ERR_NONE;
}



static int m2bus_tx_send_ack(u8 message_seq)
{
	u8 out_buffer[6];

	dprintf("[Sending Ack, seq: %x]\n", message_seq);

	if (glink->connection_type == GN_CT_Infrared)
		out_buffer[0] = M2BUS_IR_FRAME_ID;/* Start of the IR frame indicator */
	else			/* connection_type == GN_CT_Serial */
		out_buffer[0] = M2BUS_FRAME_ID;	/* Start of the frame indicator */

	out_buffer[1] = M2BUS_DEVICE_PHONE;	/* Destination */
	out_buffer[2] = M2BUS_DEVICE_PC;	/* Source */

	out_buffer[3] = 0x7f;			/* Type */

	out_buffer[4] = message_seq;

	/* Now calculate checksums over entire message and append to message. */

	out_buffer[5] = out_buffer[0] ^ out_buffer[1] ^ out_buffer[2] ^ out_buffer[3] ^ out_buffer[4];

	m2bus_wait_for_idle(2000, false);

	if (device_write(out_buffer, 6) != 6)
		return GN_ERR_INTERNALERROR;

	device_flush();

	return GN_ERR_NONE;
}


/* Initialise variables and start the link */
/* newlink is actually part of state - but the link code should not anything about state */
/* state is only passed around to allow for muliple state machines (one day...) */

gn_error m2bus_initialise(gn_link *newlink, struct gn_statemachine *state)
{
	/* 'Copy in' the global structures */
	glink = newlink;
	statemachine = state;

	/* Fill in the link functions */
	glink->loop = m2bus_loop;
	glink->send_message = m2bus_send_message;

	/* Start up the link */
	memset(&flink, 0, sizeof(flink));
	flink.request_sequence_number = 2;
	flink.i.state = M2BUS_RX_Sync;

	if (glink->connection_type == GN_CT_Infrared) {
		return GN_ERR_FAILED;
	} else {		/* connection_type == GN_CT_Serial */
		if (!m2bus_serial_open())
			return GN_ERR_FAILED;
	}

	return GN_ERR_NONE;
}
