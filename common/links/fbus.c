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

  Copyright (C) 2000      Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000-2001 Chris Kemp
  Copyright (C) 2001-2006 Pawel Kot
  Copyright (C) 2001      Manfred Jonsson, Pavel Machek
  Copyright (C) 2001-2002 Ladis Michl
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2003      Marcel Holtmann

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FBUS_(whatever).

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
#include "links/fbus.h"
#include "links/utils.h"
#include "gnokii-internal.h"

static void fbus_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state);
static gn_error fbus_send_message(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state);
static int fbus_tx_send_ack(u8 message_type, u8 message_seq, struct gn_statemachine *state);

/* FIXME - win32 stuff! */

#define FBUSINST(s) (*((fbus_link **)(&(s)->link.link_instance)))

#define	IR_MODE(s) ((s)->config.connection_type == GN_CT_Infrared || (s)->config.connection_type == GN_CT_Tekram)


/*--------------------------------------------*/

static bool fbus_serial_open(bool dlr3, struct gn_statemachine *state)
{
	int type;

	if (!state)
		return false;

	if (state->config.connection_type == GN_CT_TCP)
		type = GN_CT_TCP;
	else
		type = GN_CT_Serial;

	if (dlr3)
		dlr3 = 1;
	/* Open device. */
	if (!device_open(state->config.port_device, false, false, false, type, state)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
	device_changespeed(115200, state);

	/* clearing the RTS bit and setting the DTR bit */
	device_setdtrrts((1-dlr3), 0, state);

	return true;
}

static int send_command(char *cmd, int len, struct gn_statemachine *state)
{
	struct timeval timeout;
	unsigned char buffer[255];
	int res, offset = 0, waitformore = 1;

	/* Communication with the phone looks strange here. I am unable to
	 * read the whole answer from the port with DKU-5 cable and
	 * Nokia 6020. The following code seems to work reliably.
	 */
	device_write(cmd, len, state);
	/* Experimental timeout */
	timeout.tv_sec	= 0;
	timeout.tv_usec	= 500000;
        
	res = device_select(&timeout, state);
	/* Read from the port only when select succeeds */
	while (res > 0 && waitformore) {
		/* Avoid 'device temporarily unavailable' error */
		usleep(50000);
		res = device_read(buffer + offset, sizeof(buffer) - offset, state);
		/* The whole answer is read */
		if (strstr(buffer, "OK"))
			waitformore = 0;
		if (res > 0)
			offset += res;
		res = offset;
		/* The phone is already in AT mode */
		if (strchr(buffer, 0x55))
			res = 0;
	}
	return res;
}

static bool at2fbus_serial_open(struct gn_statemachine *state, gn_connection_type type)
{
	unsigned char init_char = 0x55;
	unsigned char end_init_char = 0xc1;
	int count, res;

	if (!state)
		return false;

	/* Open device. */
	if (!device_open(state->config.port_device, false, false, false, type, state)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
 
	device_setdtrrts(0, 0, state);
	usleep(1000000);
	device_setdtrrts(1, 1, state);
	usleep(1000000);
	device_changespeed(19200, state);
	dprintf("Switching to FBUS mode\n");
	/* Here we can be either switched to FBUS or not. Assume 0.5 second
	 * timeout for the answer.
	 */
	res = send_command("AT\r\n", 4, state);
	if (res)
		res = send_command("AT&F\r\n", 6, state);
	if (res)
		res = send_command("AT*NOKIAFBUS\r\n", 14, state);
	device_changespeed(115200, state);

	if (type != GN_CT_Bluetooth && type != GN_CT_TCP) { 
		for (count = 0; count < 32; count++) {
			device_write(&init_char, 1, state);
		}
		device_write(&end_init_char, 1, state);
		usleep(1000000);
	}
 
	return true;
}

static bool fbus_ir_open(struct gn_statemachine *state)
{
	struct timeval timeout;
	unsigned char init_char = 0x55;
	unsigned char end_init_char = 0xc1;
	unsigned char connect_seq[] = { FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02 };
	unsigned int count, retry;

	if (!state)
		return false;

	if (!device_open(state->config.port_device, false, false, false, state->config.connection_type, state)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}

	/* clearing the RTS bit and setting the DTR bit */
	device_setdtrrts(1, 0, state);

	for (retry = 0; retry < 5; retry++) {
		dprintf("IR init, retry=%d\n", retry);

		device_changespeed(9600, state);

		for (count = 0; count < 32; count++) {
			device_write(&init_char, 1, state);
		}
		device_write(&end_init_char, 1, state);
		usleep(100000);

		device_changespeed(115200, state);

		fbus_send_message(7, 0x02, connect_seq, state);

		/* Wait for 1 sec. */
		timeout.tv_sec	= 1;
		timeout.tv_usec	= 0;

		if (device_select(&timeout, state)) {
			dprintf("IR init succeeded\n");
			return true;
		}
	}

	return false;
}


/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

static void fbus_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state)
{
	struct timeval time_diff;
	fbus_incoming_frame *i = &FBUSINST(state)->i;
	int frm_num, seq_num;
	fbus_incoming_message *m;
	unsigned char *message_buffer;

	if (!i)
		return;

	/* XOR the byte with the current checksum */
	i->checksum[i->buffer_count & 1] ^= rx_byte;

	switch (i->state) {
		/* Messages from the phone start with an 0x1e (cable) or 0x1c (IR).
		   We use this to "synchronise" with the incoming data stream. However,
		   if we see something else, we assume we have lost sync and we require
		   a gap of at least 5ms before we start looking again. This is because
		   the data part of the frame could contain a byte which looks like the
		   sync byte */

	case FBUS_RX_Discarding:
		gettimeofday(&i->time_now, NULL);
		timersub(&i->time_now, &i->time_last, &time_diff);
		if (time_diff.tv_sec == 0 && time_diff.tv_usec < 5000) {
			i->time_last = i->time_now;	/* no gap seen, continue discarding */
			break;
		}

		/* else fall through to... */

	case FBUS_RX_Sync:
		if (IR_MODE(state)) {
			if (rx_byte == FBUS_IR_FRAME_ID) {
				/* Initialize checksums. */
				i->checksum[0] = FBUS_IR_FRAME_ID;
				i->checksum[1] = 0;
				i->state = FBUS_RX_GetDestination;
			} else {
				/* Lost frame sync */
				i->state = FBUS_RX_Discarding;
				gettimeofday(&i->time_last, NULL);
			}

		} else {	/* !IR_MODE */
			if (rx_byte == FBUS_FRAME_ID) {
				/* Initialize checksums. */
				i->checksum[0] = FBUS_FRAME_ID;
				i->checksum[1] = 0;
				i->state = FBUS_RX_GetDestination;
			} else {
				/* Lost frame sync */
				i->state = FBUS_RX_Discarding;
				gettimeofday(&i->time_last, NULL);
			}
		}

		break;

	case FBUS_RX_GetDestination:

		i->message_destination = rx_byte;
		i->state = FBUS_RX_GetSource;

		/* When there is a checksum error and things get out of sync we have to manage to resync */
		/* If doing a data call at the time, finding a 0x1e etc is really quite likely in the data stream */
		/* Then all sorts of horrible things happen because the packet length etc is wrong... */
		/* Therefore we test here for a destination of 0x0c and return to the top if it is not */

		if (rx_byte == 0x00) {
			i->state = FBUS_RX_EchoSource;

		} else if (rx_byte != 0x0c) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got 0x%02x\n", rx_byte);
		}

		break;

	case FBUS_RX_GetSource:

		i->message_source = rx_byte;
		i->state = FBUS_RX_GetType;

		/* Source should be 0x00 */

		if (rx_byte != 0x00) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got 0x%02x\n", rx_byte);
		}

		break;

	case FBUS_RX_GetType:

		i->message_type = rx_byte;
		i->state = FBUS_RX_GetLength1;

		break;

	case FBUS_RX_GetLength1:

		i->frame_length = rx_byte << 8;
		i->state = FBUS_RX_GetLength2;

		break;

	case FBUS_RX_GetLength2:

		i->frame_length = i->frame_length + rx_byte;
		i->state = FBUS_RX_GetMessage;
		i->buffer_count = 0;

		break;

	case FBUS_RX_GetMessage:

		if (i->buffer_count >= FBUS_FRAME_MAX_LENGTH) {
			dprintf("FBUS: Message buffer overrun - resetting\n");
			i->state = FBUS_RX_Sync;
			break;
		}

		i->message_buffer[i->buffer_count] = rx_byte;
		i->buffer_count++;

		/* If this is the last byte, it's the checksum. */

		if (i->buffer_count == i->frame_length + (i->frame_length % 2) + 2) {
			i->state = FBUS_RX_Sync;

			/* Is the checksum correct? */
			if (i->checksum[0] == i->checksum[1]) {

				/* Deal with exceptions to the rules - acks and rlp.. */

				if (i->message_type == 0x7f) {
					dprintf("[Received Ack of type %02x, seq: %2x]\n",
						i->message_buffer[0], (unsigned char) i->message_buffer[1]);
					sm_incoming_acknowledge(state);

				} else if (i->message_type == 0xf1) {
					sm_incoming_function(i->message_type, i->message_buffer,
							     i->frame_length - 2, state);
				} else {	/* Normal message type */

					sm_incoming_acknowledge(state);

					/* Add data to the relevant Message buffer */
					/* having checked the sequence number */

					m = &FBUSINST(state)->messages[i->message_type];

					frm_num = i->message_buffer[i->frame_length - 2];
					seq_num = i->message_buffer[i->frame_length - 1];

					/* 0x40 in the sequence number indicates first frame of a message */
					if ((seq_num & 0x40) == 0x40) {
						/* Fiddle around and malloc some memory */
						m->message_length = 0;
						m->frames_to_go = frm_num;
						if (m->malloced != 0) {
							free(m->message_buffer);
							m->malloced = 0;
							m->message_buffer = NULL;
						}
						m->malloced = frm_num * m->message_length;
						m->message_buffer = (unsigned char *)malloc(m->malloced);

					} else if (m->frames_to_go != frm_num) {
						dprintf("Missed a frame in a multiframe message.\n");
						/* FIXME - we should make sure we don't ack the rest etc */
					}

					if (m->malloced < m->message_length + i->frame_length) {
						m->malloced = m->message_length + i->frame_length;
						m->message_buffer = (unsigned char *)realloc(m->message_buffer, m->malloced);
					}

					memcpy(m->message_buffer + m->message_length, i->message_buffer,
					       i->frame_length - 2);

					m->message_length += i->frame_length - 2;

					m->frames_to_go--;

					/* Send an ack (for all for now) */

					fbus_tx_send_ack(i->message_type, seq_num & 0x0f, state);

					/* Finally dispatch if ready */

					if (m->frames_to_go == 0) {
						message_buffer = m->message_buffer;
						m->message_buffer = NULL;
						m->malloced = 0;
						sm_incoming_function(i->message_type, message_buffer,
								     m->message_length, state);
						free(message_buffer);
					}
				}
			} else {
				dprintf("Bad checksum!\n");
			}
		}
		break;

	case FBUS_RX_EchoSource:

		i->message_source = rx_byte;
		i->state = FBUS_RX_EchoType;

		if (rx_byte != 0x0c) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got 0x%02x\n", rx_byte);
		}

		break;

	case FBUS_RX_EchoType:

		i->message_type = rx_byte;
		i->state = FBUS_RX_EchoLength1;

		break;

	case FBUS_RX_EchoLength1:

		i->state = FBUS_RX_EchoLength2;

		break;

	case FBUS_RX_EchoLength2:

		i->frame_length = rx_byte;
		i->state = FBUS_RX_EchoMessage;
		i->buffer_count = 0;

		break;

	case FBUS_RX_EchoMessage:

		if (i->buffer_count >= FBUS_FRAME_MAX_LENGTH) {
			dprintf("FBUS: Message buffer overrun - resetting\n");
			i->state = FBUS_RX_Sync;
			break;
		}

		i->buffer_count++;

		if (i->buffer_count == i->frame_length + (i->frame_length % 2) + 2) {
			i->state = FBUS_RX_Sync;
			dprintf("[Echo cancelled]\n");
		}

		break;
	}
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

static gn_error fbus_loop(struct timeval *timeout, struct gn_statemachine *state)
{
	unsigned char buffer[BUFFER_SIZE];
	int count, res;

	res = device_select(timeout, state);
	if (res > 0) {
		res = device_read(buffer, sizeof(buffer), state);
		for (count = 0; count < res; count++)
			fbus_rx_statemachine(buffer[count], state);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}



/* Prepares the message header and sends it, prepends the message start byte
	   (0x1e) and other values according the value specified when called.
	   Calculates checksum and then sends the lot down the pipe... */

int fbus_tx_send_frame(u8 message_length, u8 message_type, u8 *buffer, struct gn_statemachine *state)
{
	u8 out_buffer[FBUS_TRANSMIT_MAX_LENGTH + 5];
	int count, current = 0;
	unsigned char checksum;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */

	if (IR_MODE(state))
		out_buffer[current++] = FBUS_IR_FRAME_ID;	/* Start of the IR frame indicator */
	else			/* connection_type == GN_CT_Serial */
		out_buffer[current++] = FBUS_FRAME_ID;		/* Start of the frame indicator */

	out_buffer[current++] = FBUS_DEVICE_PHONE;		/* Destination */
	out_buffer[current++] = FBUS_DEVICE_PC;			/* Source */

	out_buffer[current++] = message_type;			/* Type */

	out_buffer[current++] = 0;				/* Length */

	out_buffer[current++] = message_length;			/* Length */

	/* Copy in data if any. */

	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current += message_length;
	}

	/* If the message length is odd we should add pad byte 0x00 */
	if (message_length % 2)
		out_buffer[current++] = 0x00;

	/* Now calculate checksums over entire message and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count += 2)
		checksum ^= out_buffer[count];

	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count += 2)
		checksum ^= out_buffer[count];

	out_buffer[current++] = checksum;

	/* Send it out... */

	if (device_write(out_buffer, current, state) != current)
		return false;

	return true;
}


/* Main function to send an fbus message */
/* Splits up the message into frames if necessary */

static gn_error fbus_send_message(unsigned int messagesize, unsigned char messagetype, unsigned char *message, struct gn_statemachine *state)
{
	u8 seqnum, frame_buffer[FBUS_CONTENT_MAX_LENGTH + 2];
	u8 nom, lml;		/* number of messages, last message len */
	int i;

	if (!FBUSINST(state))
		return GN_ERR_FAILED;

	seqnum = 0x40 + FBUSINST(state)->request_sequence_number;
	FBUSINST(state)->request_sequence_number =
	    (FBUSINST(state)->request_sequence_number + 1) & 0x07;
	/*
	 * For the very first time sequence number should be ORed with
	 * 0x20. It should initialize sequence counter in the phone.
	 */
	if (FBUSINST(state)->init_frame) {
		seqnum |= 0x20;
		FBUSINST(state)->init_frame = 0;
	}

	if (messagesize > FBUS_CONTENT_MAX_LENGTH) {

		nom = (messagesize + FBUS_CONTENT_MAX_LENGTH - 1)
		    / FBUS_CONTENT_MAX_LENGTH;
		lml = messagesize - ((nom - 1) * FBUS_CONTENT_MAX_LENGTH);

		for (i = 0; i < nom - 1; i++) {

			memcpy(frame_buffer, message + (i * FBUS_CONTENT_MAX_LENGTH),
			       FBUS_CONTENT_MAX_LENGTH);
			frame_buffer[FBUS_CONTENT_MAX_LENGTH] = nom - i;
			frame_buffer[FBUS_CONTENT_MAX_LENGTH + 1] = seqnum;

			fbus_tx_send_frame(FBUS_CONTENT_MAX_LENGTH + 2,
					  messagetype, frame_buffer, state);

			seqnum = FBUSINST(state)->request_sequence_number;
			FBUSINST(state)->request_sequence_number = (FBUSINST(state)->request_sequence_number + 1) & 0x07;
		}

		memcpy(frame_buffer, message + ((nom - 1) * FBUS_CONTENT_MAX_LENGTH), lml);
		frame_buffer[lml] = 0x01;
		frame_buffer[lml + 1] = seqnum;
		fbus_tx_send_frame(lml + 2, messagetype, frame_buffer, state);

	} else {

		memcpy(frame_buffer, message, messagesize);
		frame_buffer[messagesize] = 0x01;
		frame_buffer[messagesize + 1] = seqnum;
		fbus_tx_send_frame(messagesize + 2, messagetype,
				  frame_buffer, state);
	}
	return GN_ERR_NONE;
}


static int fbus_tx_send_ack(u8 message_type, u8 message_seq, struct gn_statemachine *state)
{
	unsigned char request[2];

	request[0] = message_type;
	request[1] = message_seq;

	dprintf("[Sending Ack of type %02x, seq: %x]\n", message_type, message_seq);

	return fbus_tx_send_frame(2, 0x7f, request, state);
}


static void fbus_reset(struct gn_statemachine *state)
{
	int count;

	/* Init variables */
	FBUSINST(state)->i.state = FBUS_RX_Sync;
	FBUSINST(state)->i.buffer_count = 0;
	for (count = 0; count < FBUS_MESSAGE_MAX_TYPES; count++) {
		FBUSINST(state)->messages[count].malloced = 0;
		FBUSINST(state)->messages[count].frames_to_go = 0;
		FBUSINST(state)->messages[count].message_length = 0;
		FBUSINST(state)->messages[count].message_buffer = NULL;
	}
}

/* Initialise variables and start the link */
/* state is only passed around to allow for multiple state machines (one day...) */
gn_error fbus_initialise(int attempt, struct gn_statemachine *state)
{
	unsigned char init_char = 0x55;
	bool connection = false;

	if (!state)
		return GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &fbus_loop;
	state->link.send_message = &fbus_send_message;
	state->link.reset = &fbus_reset;

	/* Check for a valid init length */
	if (state->config.init_length == 0)
		state->config.init_length = 250;

	/* Start up the link */
	if ((FBUSINST(state) = calloc(1, sizeof(fbus_link))) == NULL)
		return GN_ERR_MEMORYFULL;

	FBUSINST(state)->request_sequence_number = 0;
	FBUSINST(state)->init_frame = 1;

	switch (state->config.connection_type) {
	case GN_CT_Infrared:
	case GN_CT_Tekram:
		connection = fbus_ir_open(state);
		break;
	case GN_CT_Serial:
	case GN_CT_TCP:
		switch (attempt) {
		case 0:
		case 1:
			connection = fbus_serial_open(1 - attempt, state);
			break;
		case 2:
			connection = at2fbus_serial_open(state, state->config.connection_type);
			break;
		default:
			break;
		}
		break;
	case GN_CT_DAU9P:
		connection = fbus_serial_open(0, state);
		break;
	case GN_CT_DLR3P:
		switch (attempt) {
		case 0:
			connection = at2fbus_serial_open(state, GN_CT_Serial);
			break;
		case 1:
			connection = fbus_serial_open(1, state);
			break;
		default:
			break;
		}
		break;
#ifdef HAVE_BLUETOOTH
	case GN_CT_Bluetooth:
		connection = at2fbus_serial_open(state, state->config.connection_type);
		break;
#endif
	default:
		break;
	}
	if (!connection) {
		free(FBUSINST(state));
		FBUSINST(state) = NULL;
		return GN_ERR_FAILED;
	}

	/* Send init string to phone, this is a bunch of 0x55 characters. Timing is
	   empirical. */
	/* I believe that we need/can do this for any phone to get the UART synced */

	if (state->config.connection_type != GN_CT_Bluetooth && state->config.connection_type != GN_CT_TCP) {
		int count;
		for (count = 0; count < state->config.init_length; count++) {
			usleep(100);
			device_write(&init_char, 1, state);
		}
	}

	fbus_reset(state);

	return GN_ERR_NONE;
}
