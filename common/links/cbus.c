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
  Copyright (C) 2001-2003 Ladislav Michl <ladis@linux-mips.org>

 */

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header files */
#include "config.h"
#include "compat.h"
#include "misc.h"

#include "gnokii-internal.h"

#include "device.h"
#include "links/utils.h"
#include "links/cbus.h"
#include "links/atbus.h"


static gn_error bus_write(unsigned char *d, int len, struct gn_statemachine *state)
{
	int res;
	while (len) {
		res = device_write(d, len, state);
		if (res == -1) {
			if (errno != EAGAIN) {
				dprintf("I/O error : %m?!\n");
				return GN_ERR_FAILED;
			}
		} else {
			d += res;
			len -= res;
		}
	}
	return GN_ERR_NONE;
}

static gn_error bus_read(unsigned int timeout, struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	int res;
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * timeout;
	res = device_select(&tv, state);
	if (res > 0)
		res = device_read(bi->rx_buffer, CBUS_MAX_FRAME_LENGTH, state);
	else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0) {
		bi->rx_buffer_pos = 0;
		bi->rx_buffer_count = res;
		return GN_ERR_NONE;
	} else
		return GN_ERR_INTERNALERROR;
}

static gn_error send_packet(unsigned char *msg, int len, unsigned short cmd, bool init, struct gn_statemachine *state)
{
	unsigned char p[CBUS_MAX_FRAME_LENGTH], csum = 0;
	int i, pos;

	p[0] = (init) ? 0x38 : 0x34;
	p[1] = 0x19;
	p[2] = cmd & 0xff;
	p[3] = (init) ? 0x70 : 0x68;
	p[4] = len & 0xff;
	p[5] = len >> 8;
	memcpy(p+6, msg, len);

	pos = 6+len;
	for (i = 0; i < pos; i++)
		csum ^= p[i];
	p[pos++] = csum;
#if 0
	if (cmd == 0x3c) {
		at_dprintf("CBUS -> ", msg, len);
	} else {
		dprintf("CBUS -> ");
		for (i = 0; i < pos; i++)
			dprintf("%02x ", p[i]);
		dprintf("\n");
	}
#endif
	return bus_write(p, pos, state);
}

static void dump_packet(cbus_instance *bi)
{
	int i;

	dprintf("CBUS %s ", (bi->frame_header1 == 0x19) ? "<-" : "->");
	dprintf("hdr: (%02x,%02x) ", bi->frame_header1, bi->frame_header2);
	dprintf("cmd: (%02x,%02x) ", bi->frame_type1, bi->frame_type2);
	dprintf("len: %d\n", bi->message_len);
	if (bi->message_len == 0)
		return;
	if (bi->frame_type1 == 0x3c || bi->frame_type1 == 0x3e) {
		at_dprintf("        data: ", bi->buffer, bi->message_len);
	} else {
		dprintf("        data: ");
		for (i = 0; i < bi->message_len; i++)
			dprintf("%02x ", bi->buffer[i]);
		dprintf("\n");
	}
}

/*
 * State machine for receive handling. Called once for each character
 * received from the phone.
 */
static bool state_machine(unsigned char rx_byte, struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	
	/* checksum is XOR of all bytes in the frame */
	if (bi->state != CBUS_RX_GetCSum) bi->checksum ^= rx_byte;

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
			bi->checksum = bi->prev_rx_byte ^ rx_byte;
			bi->buffer_count = 0;
		}
		break;

	case CBUS_RX_FrameType1:
		bi->frame_type1 = rx_byte;
		bi->state = CBUS_RX_FrameType2;
		break;

	case CBUS_RX_FrameType2:
		bi->frame_type2 = rx_byte;
		bi->state = CBUS_RX_GetLengthLB;
		break;

	/* message length - low byte */
	case CBUS_RX_GetLengthLB:
		bi->message_len = rx_byte;
		bi->state = CBUS_RX_GetLengthHB;
		break;

	/* message length - high byte */
	case CBUS_RX_GetLengthHB:
		bi->message_len = bi->message_len | rx_byte << 8;
		/* there are also empty messages */
		if (bi->message_len == 0)
			bi->state = CBUS_RX_GetCSum;
		else
			bi->state = CBUS_RX_GetMessage;
		break;

	/* get each byte of the message body */
	case CBUS_RX_GetMessage:
		bi->buffer[bi->buffer_count++] = rx_byte;
		/* avoid buffer overflow */
		if (bi->buffer_count > CBUS_MAX_MSG_LENGTH) {
			dprintf("CBUS: Message buffer overun - resetting\n");
			bi->state = CBUS_RX_Header;
			break;
		}

		if (bi->buffer_count == bi->message_len) {
			bi->buffer[bi->buffer_count] = 0;
			bi->state = CBUS_RX_GetCSum;
		}
		break;

	/* get checksum */
	case CBUS_RX_GetCSum:
		bi->state = CBUS_RX_Header;
		/* compare against calculated checksum. */
		if (bi->checksum == rx_byte) {
			dump_packet(bi);
			if (bi->frame_header1 == 0x19) {
				dprintf("CBUS: sending ack\n");
				bus_write("\xa5", 1, state);
				return true;
			}
			bi->state = CBUS_RX_GetAck;
		} else
			dprintf("CBUS: Checksum error %02x/%02x\n",
				bi->checksum, rx_byte);
		break;

	/* get ack */
	case CBUS_RX_GetAck:
 		bi->state = CBUS_RX_Header;
		if (rx_byte == 0xa5) {
			if (bi->frame_header1 != 0x19)
				dprintf("CBUS: got ack\n");
			return true;
		}
		dprintf("CBUS: ack expected, but got %02x\n", rx_byte);
 		break;
	default:
		break;
	}

	bi->prev_rx_byte = rx_byte;
	return false;
}

static gn_error get_packet(struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	gn_error error = GN_ERR_NONE;
	bool done = false;
	int retry = 3;

	bi->state = CBUS_RX_Header;
	do {
		if (bi->rx_buffer_pos == bi->rx_buffer_count)
			error = bus_read(5, state);
		if (!error) {
			retry = 3;
			done = state_machine(bi->rx_buffer[bi->rx_buffer_pos++], state);
		}
	} while (!done && --retry);
	
	return done ? GN_ERR_NONE : GN_ERR_TIMEOUT;
}

/* 
 * send packet and wait for 'ack'. 
 */
static gn_error send_packet_ack(unsigned char *msg, int len, unsigned short cmd,
				bool init, struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	gn_error error;
	int retry = 3;

	bi->rx_buffer_pos = 0;
	bi->rx_buffer_count = 0;
	device_flush(state);
	
	error = send_packet(msg, len, cmd, init, state);
	if (error)
		return error;
	
	while (retry--) {
		error = get_packet(state);
		if (error)
			continue;
		if (bi->frame_type1 != cmd) {
			dprintf("CBUS: get_packet is not the right packet\n");
			continue;
		}
		if (!error)
			return GN_ERR_NONE;
	}
	dprintf("CBUS: get_packet failed\n");
	return GN_ERR_FAILED;
}

static gn_error bus_reset(struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	gn_error error;

	bus_write("\0\0\0\0\0\0\0", 7, state);
	send_packet_ack("", 0, 0xa6, true, state);
	send_packet_ack("\x1f", 1, 0x90, true, state);
	error = get_packet(state);
	if (error)
		return error;
	if (bi->frame_type1 != 0x91) {
		dprintf("CBUS: Bus reset failed\n");
		return GN_ERR_NOLINK;
	}
	dprintf("CBUS: Bus reset ok\n");
	return GN_ERR_NONE;
}

static gn_error get_cmd_confirmation(struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	gn_error error;
	
	error = get_packet(state);
	if (error)
		return error;
	if (bi->frame_type1 != 0x3d) {
		dprintf("CBUS: command confirmation expected %02x/%02x\n",
			bi->frame_type1, bi->frame_type2);
		return GN_ERR_FAILED;
	}
	dprintf("CBUS: command confirmed\n");
	return GN_ERR_NONE;
}

static gn_error get_cmd_reply(struct gn_statemachine *state)
{
	cbus_instance *bi = CBUSINST(state);
	gn_error error;

	error = get_packet(state);
	if (error)
		return error;
	if (bi->frame_type1 != 0x3e) {
		dprintf("CBUS: command reply expected %02x/%02x\n",
			bi->frame_type1, bi->frame_type2);
		return GN_ERR_INTERNALERROR;
	}
	bi->at_reply[0] = GN_AT_NONE;
	if (!strncmp(bi->buffer, "OK", 2))
		bi->at_reply[0] = GN_AT_OK;
	else if (!strncmp(bi->buffer, "+CMS ERROR:", 11))
		bi->at_reply[0] = GN_AT_ERROR;
	else if (!strncmp(bi->buffer, "ERROR", 5))
		bi->at_reply[0] = GN_AT_ERROR;
	if (bi->at_reply[0] == GN_AT_NONE) {
		strcpy(bi->at_reply + 1, bi->buffer);
		bi->at_reply[bi->message_len] = 0;
		error = send_packet_ack("\x3e\x68", 2, 0x3f, false, state);
		if (error)
			return error;
		error = get_cmd_reply(state);
		if (error)
			return error;
	}
	return send_packet_ack("\x3e\x68", 2, 0x3f, false, state);
}

static gn_error at_send_message(unsigned int message_length, unsigned char message_type,
				unsigned char *buffer, struct gn_statemachine *state)
{
	gn_error error;

	error = send_packet_ack(buffer, message_length, 0x3c, false, state);
	if (error)
		return error;
	return get_cmd_confirmation(state);
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
static gn_error cbus_loop(struct timeval *timeout, struct gn_statemachine *sm)
{
	gn_error error;
	cbus_instance *bi = CBUSINST(sm);

	error = get_cmd_reply(sm);
	if (error)
		return error;
	sm_incoming_acknowledge(sm);
	sm_incoming_function(sm->last_msg_type, bi->at_reply, strlen(bi->at_reply), sm);
	return GN_ERR_NONE;
}

/* Initialise variables and start the link */
gn_error cbus_initialise(struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	cbus_instance *businst;

	if (!(businst = malloc(sizeof(cbus_instance))))
		return GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &cbus_loop;
	state->link.send_message = &at_send_message;
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
