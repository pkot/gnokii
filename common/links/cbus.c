/*

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

  Copyright (C) 2001      Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2001-2003 Ladislav Michl <ladis@linux-mips.org>
  Copyright (C) 2001      Manfred Jonsson
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan

 */

#include <errno.h>

#include "config.h"
#include "compat.h"
#include "device.h"
#include "misc.h"
#include "gnokii-internal.h"
#include "links/utils.h"
#include "links/cbus.h"

static void cbus_wait_for_idle(int timeout, struct gn_statemachine *state)
{
	int n, prev;

	device_nreceived(&n, state);
	do {
		prev = n;
		usleep(timeout);
		if (device_nreceived(&n, state) != GN_ERR_NONE) break;
	} while (n != prev);
}

static gn_error cbus_write(unsigned char *d, size_t len, struct gn_statemachine *state)
{
	size_t res;

	cbus_wait_for_idle(300, state);

	while (len) {
		res = device_write(d, len, state);
		if (res == -1) {
			if (errno != EAGAIN) {
				dprintf("cbus: %s\n", strerror(errno));
				return GN_ERR_FAILED;
			}
		} else {
			d += res;
			len -= res;
		}
	}
	device_flush(state);

	return GN_ERR_NONE;
}

static gn_error cbus_wakeup1(struct gn_statemachine *state)
{
	int i;
	unsigned char p[] = {0x38, 0x19, 0xa6, 0x70, 0x00, 0x00, 0x00};

	for (i = 0; i < 6; i++)
		p[6] ^= p[i];
	return cbus_write(p, sizeof(p), state);
}

static gn_error cbus_wakeup2(struct gn_statemachine *state)
{
	int i;
	unsigned char p[] = {0x38, 0x19, 0x90, 0x70, 0x01, 0x00, 0x44, 0x00};

	for (i = 0; i < 7; i++)
		p[7] ^= p[i];
	return cbus_write(p, sizeof(p), state);
}

static gn_error
cbus_send_message(unsigned int msglen, unsigned char msgtype,
		  unsigned char *msg, struct gn_statemachine *state)
{
	unsigned char p[CBUS_MAX_FRAME_LENGTH], csum = 0;
	int i, pos;

	p[0] = 0x34;
	p[1] = 0x19;
	p[2] = msgtype;
	p[3] = 0x68;
	p[4] = msglen & 0xff;
	p[5] = msglen >> 8;
	memcpy(p+6, msg, msglen);

	pos = 6+msglen;
	for (i = 0; i < pos; i++)
		csum ^= p[i];
	p[pos++] = csum;

	usleep(50000);

	return cbus_write(p, pos, state);
}

static void dump_packet(cbus_instance *bi)
{
	int i;

	dprintf("CBUS %s ", (bi->frame_header1 == 0x19) ? "<-" : "->");
	dprintf("hdr: (%02x,%02x) ", bi->frame_header1, bi->frame_header2);
	dprintf("cmd: (%02x,%02x) ", bi->frame_type1, bi->frame_type2);
	dprintf("len: %d\n", bi->msg_len);
	if (bi->msg_len == 0)
		return;
	if (bi->frame_type1 == 0x3c || bi->frame_type1 == 0x3e) {
		at_dprintf("        data: ", bi->msg, bi->msg_len);
	} else {
		dprintf("        data: ");
		for (i = 0; i < bi->msg_len; i++)
			dprintf("%02x ", bi->msg[i]);
		dprintf("\n");
	}
}

/*
 * State machine for receive handling. Called once for each character
 * received from the phone.
 *
 * Some machines are simply not fast enough to send 0xa5 ack byte. In that case
 * phone starts retransmit last frame until acked by computer (max. 3 times).
 * We are comparing current frame with previous one and only unique frames are
 * send back to statemachine.
 */
static void cbus_rx_statemachine(unsigned char rx_byte, struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);

	if (!bi)
		return;
	
	if (bi->state != CBUS_RX_CSum) bi->csum ^= rx_byte;

	switch (bi->state) {

	case CBUS_RX_Header:
		switch (rx_byte) {
		case 0x38:
		case 0x34:
			if (bi->prev_rx_byte == 0x19)
				bi->state = CBUS_RX_FrameType1;
			break;
		case 0x19:
			if ((bi->prev_rx_byte == 0x38) || (bi->prev_rx_byte == 0x34))
				bi->state = CBUS_RX_FrameType1;
			break;
		default:
			dprintf("CBUS: discarding (%02x)\n", rx_byte);
			break;
		}
		if (bi->state != CBUS_RX_Header) {
			bi->frame_header1 = bi->prev_rx_byte;
			bi->frame_header2 = rx_byte;
			bi->csum = bi->prev_rx_byte ^ rx_byte;
			bi->unique = bi->msg_pos = 0;
		}
		break;

	case CBUS_RX_FrameType1:
		bi->unique = bi->frame_type1 != rx_byte;
		bi->frame_type1 = rx_byte;
		bi->state = CBUS_RX_FrameType2;
		break;

	case CBUS_RX_FrameType2:
		bi->unique |= bi->frame_type1 != rx_byte;
		bi->frame_type2 = rx_byte;
		bi->state = CBUS_RX_LengthLB;
		break;

	/* message length - low byte */
	case CBUS_RX_LengthLB:
		bi->msg_len = rx_byte;
		bi->state = CBUS_RX_LengthHB;
		break;

	/* message length - high byte */
	case CBUS_RX_LengthHB:
		bi->msg_len = bi->msg_len | rx_byte << 8;
		/* there are also empty messages */
		if (bi->msg_len == 0)
			bi->state = CBUS_RX_CSum;
		else
			bi->state = CBUS_RX_Message;
		break;

	/* get each byte of the message body */
	case CBUS_RX_Message:
		bi->unique |= bi->msg[bi->msg_pos] != rx_byte;
		bi->msg[bi->msg_pos++] = rx_byte;
		/* avoid buffer overflow */
		if (bi->msg_pos == CBUS_MAX_MSG_LENGTH) {
			dprintf("CBUS: Message buffer overun - resetting\n");
			bi->state = CBUS_RX_Header;
			break;
		}

		if (bi->msg_pos == bi->msg_len) {
			bi->msg[bi->msg_pos] = 0;
			bi->state = CBUS_RX_CSum;
		}
		break;

	case CBUS_RX_CSum:
		/* crc error? */
		if (bi->csum != rx_byte) {
			dprintf("CBUS: Checksum error!\n");
			bi->state = CBUS_RX_Header;
			break;
		}
		dump_packet(bi);
		/* Deal with exceptions to the rules - acks... */
		if (bi->frame_header1 == 0x19) {
			dprintf("CBUS: sending ack\n");
			device_write("\xa5", 1, state);
			device_flush(state);
			if (bi->unique)
				sm_incoming_function(bi->frame_type1, bi->msg,
						     bi->msg_len, state);
			else
				dprintf("CBUS: discarding frame.\n");
		}
		bi->state = CBUS_RX_Ack;
		break;

	/* get ack */
	case CBUS_RX_Ack:
		if (rx_byte == 0xa5) {
			if (bi->frame_header1 != 0x19) {
				dprintf("CBUS: got ack\n");
				sm_incoming_acknowledge(state);
			}
		} else
			dprintf("CBUS: ack expected, got 0x%02x\n", rx_byte);
		bi->state = CBUS_RX_Header;
 		break;
	default:
		break;
	}
	bi->prev_rx_byte = rx_byte;
}

static gn_error bus_reset(struct gn_statemachine *state)
{
#ifndef WIN32
	tcsendbreak(device_getfd(state), 0);
#endif
	cbus_wakeup1(state);
	sm_block_no_retry_timeout(0, 100, NULL, state);
	cbus_wakeup2(state);
	sm_block_no_retry_timeout(0, 100, NULL, state);
	return GN_ERR_NONE;
}

static bool cbus_open_serial(char *device, struct gn_statemachine *state)
{
	int result = device_open(device, false, false, false, GN_CT_Serial, state);
	if (!result) {
		perror(_("Couldn't open CBUS device"));
		return false;
	}
	device_changespeed(9600, state);
	device_setdtrrts(1, 0, state);
	return true;
}

/* 
 * This is the main loop function which must be called regularly
 * timeout can be used to make it 'busy' or not 
 */
static gn_error cbus_loop(struct timeval *timeout, struct gn_statemachine *state)
{
	int count;
	size_t res;
	unsigned char buffer[BUFFER_SIZE];

	res = device_select(timeout, state);
	if (res > 0) {
		res = device_read(buffer, sizeof(buffer), state);
		for (count = 0; count < res; count++)
			cbus_rx_statemachine(buffer[count], state);
	} else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}

/* Initialise variables and start the link */
gn_error cbus_initialise(struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	cbus_instance *businst;

	if (!state)
		return GN_ERR_FAILED;

	if (!(businst = malloc(sizeof(cbus_instance))))
		return GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &cbus_loop;
	state->link.send_message = &cbus_send_message;
	state->link.reset = NULL;
	CBUSINST(state) = businst;

	if (state->config.connection_type == GN_CT_Serial) {
		if (!cbus_open_serial(state->config.port_device, state))
			error = GN_ERR_FAILED;
	} else {
		dprintf("Device not supported by C-bus\n");
		error = GN_ERR_FAILED;
	}

	if (error) {
		dprintf("C-bus initialization failed (%d)\n", error);
		free(CBUSINST(state));
		CBUSINST(state) = NULL;
	} else {
		error = bus_reset(state);
	}
	return error;
}
