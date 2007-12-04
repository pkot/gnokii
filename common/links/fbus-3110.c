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
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2002      Ladis Michl
  Copyright (C) 2002-2003 BORBELY Zoltan
  Copyright (C) 2003-2004 Osma Suominen

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called fb3110_(whatever).

*/

#include "config.h"

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */

#include "compat.h"
#include "misc.h"
#include "gnokii-internal.h"
#include "device.h"

#include "links/fbus-3110.h"
#include "links/utils.h"

static bool fb3110_serial_open(struct gn_statemachine *state);
static void fb3110_rx_frame_handle(fb3110_incoming_frame *i, struct gn_statemachine *state);
static void fb3110_rx_state_machine(unsigned char rx_byte, struct gn_statemachine *state);
static gn_error fb3110_tx_frame_send(u8 frame_type, u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer, struct gn_statemachine *state);
static gn_error fb3110_message_send(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state);
static void fb3110_tx_ack_send(u8 messagetype, u8 checksum, struct gn_statemachine *state);
static void fb3110_sequence_number_update(struct gn_statemachine *state);
static int fb3110_message_type_fold(int type);

/* FIXME - win32 stuff! */

#define FBUSINST(s) (*((fb3110_link **)(&(s)->link.link_instance)))


/*--------------------------------------------*/

static bool fb3110_serial_open(struct gn_statemachine *state)
{
	if (!state)
		return false;

	/* Open device. */
	if (!device_open(state->config.port_device, false, false, false, GN_CT_Serial, state)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
	device_changespeed(115200, state);

	return true;
}

/*
 * Handles one complete frame. What to do with it depends on the frame
 * type. Frames are acknowledged if needed and information about them is
 * passed to the main statemachine.
 */
 
static void fb3110_rx_frame_handle(fb3110_incoming_frame *i, struct gn_statemachine *state)
{
	int count;

	if (!i)
		return;

	dprintf("--> %02x:%02x:", i->frame_type, i->frame_len);
	for (count = 0; count < i->buffer_count; count++)
		dprintf("%02hhx:", i->buffer[count]);
	dprintf("\n");

	if (i->frame_type == FB3110_FRAME_TYPE_IN_RLP) { /* RLP frame */

		/* Transfer RLP frame to state machine. The messagetype
		 * should always be 0x02 in this case, so the frame type
		 * doesn't have to be passed to the statemachine and on to
		 * the driver - RLP frames can be told apart by messagetype.
		 * But check message type anyway just to make sure. */

		if (i->buffer[0] != 0x02)
			dprintf("RLP frame with non-0x02 message type (0x%02x) received!\n", i->buffer[0]);

		sm_incoming_function(i->buffer[0], i->buffer, i->frame_len, state);

	} else if (i->buffer[1] < 0x08 || i->buffer[1] > 0x0f) { /* msg frame */

		/* Send an ack */
		fb3110_tx_ack_send(i->buffer[0], i->buffer[1], state);

		if (FBUSINST(state)->last_incoming_sequence_number == i->buffer[1]) {
			dprintf("Duplicate message received. Sent ack, not handling.\n");
			return;
		}

		FBUSINST(state)->last_incoming_sequence_number = i->buffer[1];

		/* Transfer message to state machine */
		sm_incoming_function(fb3110_message_type_fold(i->buffer[0]), i->buffer, i->frame_len, state);

	} else { /* ack frame */
		fb3110_sequence_number_update(state);
		dprintf("Received ack for message type 0x%02x\n", i->buffer[0]);
		sm_incoming_acknowledge(state);
	}
}


/* 
 * RX_State machine for receive handling.
 * Called once for each character received from the phone.
 */
static void fb3110_rx_state_machine(unsigned char rx_byte, struct gn_statemachine *state)
{
	fb3110_incoming_frame *i = &FBUSINST(state)->i;

	if (!i)
		return;

	switch (i->state) {

	/* Phone is currently off.  Wait for 0x55 before
	   restarting */
	case FB3110_RX_Discarding:
		if (rx_byte != 0x55)
			break;

		/* Seen 0x55, restart at 0x04 */
		i->state = FB3110_RX_Sync;

		dprintf("restarting.\n");

		/* FALLTHROUGH */

	/* Messages from the phone start with an 0x04 during
	   "normal" operation, 0x03 when in data/fax mode.  We
	   use this to "synchronise" with the incoming data
	   stream. */
	case FB3110_RX_Sync:
		if (rx_byte == 0x04 || rx_byte == 0x03) {
			i->frame_type = rx_byte;
			i->checksum = rx_byte;
			i->state = FB3110_RX_GetLength;
		}
		break;

	/* Next byte is the length of the message including
	   the message type byte but not including the checksum. */
	case FB3110_RX_GetLength:
		i->frame_len = rx_byte;
		i->buffer_count = 0;
		i->checksum ^= rx_byte;
		i->state = FB3110_RX_GetMessage;
		break;

	/* Get each byte of the message.  We deliberately
	   get one too many bytes so we get the checksum
	   here as well. */
	case FB3110_RX_GetMessage:
		i->buffer[i->buffer_count] = rx_byte;
		i->buffer_count++;

		if (i->buffer_count >= FB3110_FRAME_MAX_LENGTH) {
			dprintf("FBUS: Message buffer overun - resetting\n");
			i->state = FB3110_RX_Sync;
			break;
		}

		/* If this is the last byte, it's the checksum. */
		if (i->buffer_count > i->frame_len) {

			/* Is the checksum correct? */
			if (rx_byte == i->checksum) {
				/* Frame received OK, deal with it! */
				fb3110_rx_frame_handle(i, state);
			} else {
				/* Checksum didn't match so ignore. */
				dprintf("Bad checksum!\n");
			}
			i->state = FB3110_RX_Sync;
		}
		i->checksum ^= rx_byte;
		break;
	}
}


/* 
 * This is the main loop function which must be called regularly
 * timeout can be used to make it 'busy' or not 
 */
static gn_error fb3110_loop(struct timeval *timeout, struct gn_statemachine *state)
{
	unsigned char buffer[BUFFER_SIZE];
	int count, res;

	res = device_select(timeout, state);
	if (res > 0) {
		res = device_read(buffer, sizeof(buffer), state);
		for (count = 0; count < res; count++)
			fb3110_rx_state_machine(buffer[count], state);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}


/* 
 * Prepares the message header and sends it, prepends the message start
 * byte (0x01) and other values according the value specified when called.
 * Calculates checksum and then sends the lot down the pipe... 
 */
static gn_error fb3110_tx_frame_send(u8 frame_type, u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer, struct gn_statemachine *state)
{

	u8 out_buffer[FB3110_TRANSMIT_MAX_LENGTH];
	int count, current = 0;
	unsigned char checksum;

	/* Check message isn't too long, once the necessary
	   header and trailer bytes are included. */
	if ((message_length + 5) > FB3110_TRANSMIT_MAX_LENGTH) {
		fprintf(stderr, _("fb3110_tx_frame_send - message too long!\n"));
		return GN_ERR_INTERNALERROR;
	}

	/* Now construct the message header. */
	out_buffer[current++] = frame_type;         /* Start of frame */
	out_buffer[current++] = message_length + 2; /* Length */
	out_buffer[current++] = message_type;       /* Type */
	out_buffer[current++] = sequence_byte;      /* Sequence number */

	/* Copy in data if any. */
	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current += message_length;
	}

	/* Now calculate checksum over entire message
	   and append to message. */
	checksum = 0;
	for (count = 0; count < current; count++)
		checksum ^= out_buffer[count];
	out_buffer[current++] = checksum;

	dprintf("<-- ");
	for (count = 0; count < current; count++)
		dprintf("%02hhx:", out_buffer[count]);
	dprintf("\n");

	/* Send it out... */
	if (device_write(out_buffer, current, state) != current)
		return GN_ERR_INTERNALERROR;

	return GN_ERR_NONE;
}


/* 
 * Main function to send an fbus message 
 */
static gn_error fb3110_message_send(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state)
{
	u8 seqnum, frame_type;

	/* Data (RLP) frame always have message type 0x01 */
	if (messagetype == 0x01) {
		/* this is a bit of a hack: outgoing RLP frames have and
		   extra byte between msgtype and seqno, indicating DTX;
		   thus we swap seqnum with message[0] */
		seqnum = message[0];
		message[0] = 0xd9; /* seqno constant for RLP frames */
		frame_type = FB3110_FRAME_TYPE_OUT_RLP;
	} else { /* normal command frame */
		seqnum = FBUSINST(state)->request_sequence_number;
		frame_type = FB3110_FRAME_TYPE_OUT_CMD;
	}

	return fb3110_tx_frame_send(frame_type, messagesize, messagetype, seqnum, message, state);
}


/* 
 * Sends the "standard" acknowledge message back to the phone in response to
 * a message it sent automatically or in response to a command sent to it.
 */
static void fb3110_tx_ack_send(u8 messagetype, u8 seqno, struct gn_statemachine *state)
{
	if (fb3110_tx_frame_send(FB3110_FRAME_TYPE_OUT_CMD, 0, messagetype, (seqno & 0x1f) - 0x08, NULL, state))
		dprintf("Failed to acknowledge message type %02x.\n", messagetype);
	else
		dprintf("Acknowledged message type %02x.\n", messagetype);
}


static void fb3110_reset(struct gn_statemachine *state)
{
	/* Init variables */
	FBUSINST(state)->i.state = FB3110_RX_Sync;
}

/* 
 * Initialise variables and start the link
 * newlink is actually part of state - but the link code should not
 * anything about state. State is only passed around to allow for
 * muliple state machines (one day...)
 */
gn_error fb3110_initialise(struct gn_statemachine *state)
{
	unsigned char init_char = 0x55;
	unsigned char count;
	static int try = 0;

	if (!state)
		return GN_ERR_FAILED;

	try++;
	if (try > 2) return GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &fb3110_loop;
	state->link.send_message = &fb3110_message_send;
	state->link.reset = &fb3110_reset;

	/* Check for a valid init length */
	if (state->config.init_length == 0)
		state->config.init_length = 100;

	/* Start up the link */
	if ((FBUSINST(state) = calloc(1, sizeof(fb3110_link))) == NULL)
		return GN_ERR_MEMORYFULL;

	FBUSINST(state)->request_sequence_number = 0x10;

	/* Since 0x08 is within the range of seqnos used for computer->ME,
	 * it should be pretty safe to assume that the phone doesn't
	 * initiate communication with that seqno. If we use e.g. 0x00 or
	 * 0x30, some nasty errors may happen. */
	FBUSINST(state)->last_incoming_sequence_number = 0x08;

	if (!fb3110_serial_open(state)) {
		free(FBUSINST(state));
		FBUSINST(state) = NULL;
		return GN_ERR_FAILED;
	}

	/* Send init string to phone, this is a bunch of 0x55 characters.
	   Timing is empirical. I believe that we need/can do this for any
	   phone to get the UART synced */
	for (count = 0; count < state->config.init_length; count++) {
		usleep(1000);
		device_write(&init_char, 1, state);
	}

	fb3110_reset(state);

	return GN_ERR_NONE;
}


/* Any command we originate must have a unique SequenceNumber. Observation to
 * date suggests that these values start at 0x10 and cycle up to 0x17
 * before repeating again. Perhaps more accurately, the numbers cycle
 * 0,1,2,3..7 with bit 4 of the byte premanently set. 
 */
static void fb3110_sequence_number_update(struct gn_statemachine *state)
{
	FBUSINST(state)->request_sequence_number++;

	if (FBUSINST(state)->request_sequence_number > 0x17 || FBUSINST(state)->request_sequence_number < 0x10)
		FBUSINST(state)->request_sequence_number = 0x10;
}

/* The 3110 protocol has a tendency to use different message types to
 * indicate successful and failed actions. These are not handled very well by
 * the gnokii internals, so they are "folded" together and dealt with in the
 * 3110 code instead. The logic is that each success/failure message type
 * pair is reported as the type of the message to indicate success.
 * The argument is the original message type i.e. first byte of message. */

static int fb3110_message_type_fold(int type)
{
	switch (type) {
	case 0x16:
	case 0x17:
		return 0x16;	/* initialization */
	case 0x21:
	case 0x22:
		return 0x21;	/* send DTMF */
	case 0x28:
	case 0x29:
		return 0x28;	/* SMS sent */
	case 0x2a:
	case 0x2b:
		return 0x2a;	/* SMS saved */
	case 0x2c:
	case 0x2d:
		return 0x2c;	/* SMS read */
	case 0x2e:
	case 0x2f:
		return 0x2e;	/* SMS deleted */
	case 0x3d:
	case 0x3e:
		return 0x3d;	/* SMSC set */
	case 0x44:
	case 0x45:
		return 0x44;	/* mem location set */
	case 0x46:
	case 0x47:
		return 0x46;	/* mem location read */
	default:
		return type;	/* all others left as-is */
	}
}
