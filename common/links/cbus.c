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
  Copyright (C) 2001, 2002 Ladislav Michl <ladis@psi.cz>

 */

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header files */
#include "config.h"
#include "misc.h"
#include "gsm-data.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"

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

static gn_error bus_read(unsigned char *rx_byte, unsigned int timeout, struct gn_statemachine *state)
{
	int res;
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * timeout;
/*	res = device_select(&tv, state);*/
	res = device_select(NULL, state);
	if (res > 0)
		res = device_read(rx_byte, 1, state);
	else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}

static gn_error send_packet(unsigned char *msg, int len, unsigned short cmd, bool init, struct gn_statemachine *state)
{
	unsigned char p[1024], csum = 0;
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
	p[pos] = csum;
	dprintf("CBUS: sending ");
	for (i = 0; i <= pos; i++)
		dprintf("%02x ", p[i]);
	dprintf("\n");

	return bus_write(p, pos+1, state);
}

static void send_ack(struct gn_statemachine *state)
{
	dprintf("CBUS: sending ack\n");
	bus_write("\xa5", 1, state);
}

static void dump_packet(cbus_instance *bi)
{
	int i;
	
	fprintf(stderr, "Hdr: %02x %02x   ", bi->frame_header1, bi->frame_header2);
	fprintf(stderr, (bi->frame_header1 == 0x19) ? "->\n" : "<-\n");
	fprintf(stderr, "Cmd: %02x %02x\n", bi->frame_type1, bi->frame_type2);
	fprintf(stderr, "Len: %d\n", bi->message_len);
	fprintf(stderr, "Data: ");
	for (i = 0; i < bi->buffer_count; i++)
		fprintf(stderr, "%02x ", bi->buffer[i]);
	fprintf(stderr, "\n");
}

static void incoming_frame(unsigned char cmd1, unsigned char cmd2)
{
	switch(cmd2) {
	case 0x68:
		switch (cmd1) {
		case 0x3d:
			dprintf("CBUS: <- AT command confirmed\n");
			break;
		case 0x3e:
			dprintf("CBUS: <- AT command reply\n");
			break;
		default:
			dprintf("CBUS: Unknown incoming frame (%02x/68)\n", cmd1);
		}
		break;
	case 0x70:
		switch (cmd1) {
		case 0x91:
			dprintf("CBUS: <- Bus ready\n");
			break;
		default:
			dprintf("CBUS: Unknown incoming frame (%02x/70)\n", cmd1);
			break;
		}
		break;
	default:
		dprintf("CBUS: Unknown incoming frame (%02x/%02x)\n", cmd1, cmd2);
		break;
	}
}

static void outgoing_frame(unsigned char cmd1, unsigned char cmd2)
{
	switch(cmd2) {
	case 0x68:
		switch (cmd1) {
		case 0x3c:
			dprintf("CBUS: -> AT command\n");
			break;
		case 0x3f:
			dprintf("CBUS: -> AT reply confirmed\n");
			break;
		default:
			dprintf("CBUS: Unknown frame (%02x/68)\n", cmd1);
			break;
		}
		break;
	case 0x70:
		switch (cmd1) {
		case 0xa6:
			dprintf("CBUS: -> Init 0xa6\n");
			break;
		case 0x90:
			dprintf("CBUS: -> Init 0x90\n");
			break;
		default:
			dprintf("CBUS: Unknown frame (%02x/70)\n", cmd1);
			break;
		}
		break;
	default:
		dprintf("CBUS: Unknown frame (%02x/%02x)\n", cmd1, cmd2);
		break;
	}
}

/*
 * State machine for receive handling. Called once for each character
 * received from the phone.
 */
static cbus_pkt_state state_machine(unsigned char rx_byte, cbus_instance *bi)
{
	cbus_pkt_state state = CBUS_PKT_None;

	/* checksum is XOR of all bytes in the frame */
	if (bi->state != CBUS_RX_GetCSum) bi->checksum ^= rx_byte;

	switch (bi->state) {

	case CBUS_RX_Header:
		switch (rx_byte) {
			case 0x38:
			case 0x34:
				if (bi->prev_rx_byte == 0x19) {
					bi->state = CBUS_RX_FrameType1;
				}
				break;
			case 0x19:
				if ((bi->prev_rx_byte == 0x38) || (bi->prev_rx_byte == 0x34)) {
					bi->state = CBUS_RX_FrameType1;
				}
				break;
			default:
				dprintf("CBUS: discarding (%02x)\n", rx_byte);
				break;
		}
		if (bi->state != CBUS_RX_Header) {
			bi->frame_header1 = bi->prev_rx_byte;
			bi->frame_header2 = rx_byte;
			bi->buffer_count = 0;
			bi->checksum = bi->prev_rx_byte ^ rx_byte;
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
		bi->buffer[bi->buffer_count] = rx_byte;
		/* avoid buffer overflow */		
		if (bi->buffer_count > CBUS_MAX_MSG_LENGTH) {
			dprintf("CBUS: Message buffer overun - resetting\n");
			bi->state = CBUS_RX_Header;
			break;
		}

		if (bi->buffer_count == bi->message_len)
			bi->state = CBUS_RX_GetCSum;
		break;

	/* get checksum */
	case CBUS_RX_GetCSum:
		/* compare against calculated checksum. */
		if (bi->checksum == rx_byte) {
			dump_packet(bi);
			state = CBUS_PKT_Ready;
		} else {
			dprintf("CBUS: Checksum error; expected: %02x, got: %02x\n", bi->checksum, rx_byte);
			state = CBUS_PKT_CSumErr;
		}
		bi->buffer[bi->message_len + 1] = 0;
		bi->state = CBUS_RX_Header;
		break;
	default:
		break;
	}

	bi->prev_rx_byte = rx_byte;
	return state;
}

static gn_error get_packet(cbus_instance *bi, struct gn_statemachine *state)
{
	gn_error res;
	cbus_pkt_state pstate = CBUS_PKT_None;
	unsigned char rx_byte;
	int retry = 20;

	bi->state = CBUS_RX_Header;
	while (pstate == CBUS_PKT_None && --retry) {
		res = bus_read(&rx_byte, 10, state);
		if (!res) {
			retry = 20;
			pstate = state_machine(rx_byte, bi);
		} else
			usleep(1000);
/*			if (--retry) {
				usleep(5000);
				continue;
			}
			return res;
		}
		retry = 20;
		pstate = state_machine(rx_byte);*/
	}
	dprintf("CBUS: Got packet\n");
	return (res) ? GN_ERR_INTERNALERROR : GN_ERR_NONE;
}

static gn_error wait_ack(cbus_instance *bi, struct gn_statemachine *state)
{
	gn_error res;
	unsigned char rx_byte;
	int retry = 555550;

/*	do {
		res = bus_read(&rx_byte, 10, state);
		if (!res) {
			device_flush(state);
			return (rx_byte == 0xa5) ? GE_NONE : GE_INTERNALERROR;
		}
		usleep(5000);
	} while (--retry);*/
	while (--retry) {
		res = bus_read(&rx_byte, 10, state);
		if (!res) 
			return (rx_byte == 0xa5) ? GN_ERR_NONE : GN_ERR_INTERNALERROR;
		else
			usleep(1000);
	}
	return GN_ERR_INTERNALERROR;
}

/* 
 * send packet and wait for 'ack'. 
 */
static gn_error send_packet_ack(unsigned char *msg, int len, unsigned short cmd, bool init, cbus_instance *bi, struct gn_statemachine *state)
{
	gn_error res;
	int retry = 3;

	do {
		retry--;
		res = send_packet(msg, len, cmd, init, state);
		if (res) {
			dprintf("CBUS: send_packet failed\n");
			continue;
		}
		usleep(50000);
again:
		res = get_packet(bi, state);
		if (res) {
			dprintf("CBUS: get_packet failed\n");
			continue;
		}
		if (bi->frame_type1 != cmd) {
			dprintf("CBUS: get_packet is not the right packet\n");
			goto again;
		}
		res = wait_ack(bi, state);
		if (res) {
			dprintf("CBUS: wait_ack failed\n");
			continue;
		}
		dprintf("CBUS: Phone acks\n");
	} while (retry && res);
	
	return res;
}

static gn_error bus_reset(cbus_instance *bi, struct gn_statemachine *state)
{
	gn_error res;

/*	bus_write("\0\0\0\0\0", 5, state);*/
	usleep(1000);
	send_packet_ack("", 0, 0xa6, true, bi, state);
	send_packet_ack("D", 1, 0x90, true, bi, state);
	res = get_packet(bi, state);
	if (res)
		return res;
	if (bi->frame_type1 != 0x91) {
		dprintf("CBUS: Bus reset failed\n");
		return GN_ERR_NOLINK;
	}
	send_ack(state);
	usleep(1000);
	send_ack(state);
	usleep(1000);
	send_ack(state);
	usleep(1000);
	send_ack(state);
	dprintf("CBUS: Bus reset ok\n");
	usleep(100000);
	return GN_ERR_NONE;
}

/*
			send_ack(state);
			clink.AT_message[0] = GEAT_OK;
			strncpy(clink.AT_message + 1, buffer, length);
			clink.AT_message_len = length;
			send_packet("\x3e\x68", 2, 0x3f, false, state);
*/

static gn_error wait_at_confirmation(cbus_instance *bi, struct gn_statemachine *state)
{
	gn_error res;
	
	res = get_packet(bi, state);
	if (res)
		return res;
	if (bi->frame_type1 != 0x3d) {
		dprintf("CBUS: AT command confirmation expected, but got %02x/%02x\n", bi->frame_type1, bi->frame_type2);
		return GN_ERR_INTERNALERROR;
	}
	dprintf("CBUS: AT command confirmed\n");
	return GN_ERR_NONE;
}

static gn_error get_at_reply(cbus_instance *bi, struct gn_statemachine *state)
{
	gn_error res;

	res = get_packet(bi, state);
	if (res)
		return res;
	if (bi->frame_type2 != 0x3e) {
		dprintf("CBUS: AT command reply expected, but got %02x/%02x\n",	bi->frame_type1, bi->frame_type2);
		return GN_ERR_INTERNALERROR;
	}
	dprintf("CBUS: AT reply: %s", bi->buffer);
	send_ack(state);
	return send_packet_ack("\x3e\x68", 2, 0x3f, false, bi, state);
}

static gn_error at_send_message(u16 message_length, u8 message_type, unsigned char *buffer, struct gn_statemachine *state)
{
	gn_error res;
	cbus_instance *bi = CBUSINST(state);
	
	res = send_packet_ack(buffer, message_length, 0x3c, false, bi, state);
	if (res)
		return res;
	dprintf("CBUS: AT command sent - %s\n", buffer);
/*	usleep(100000);*/
	res = wait_at_confirmation(bi, state);
	if (res)
		return res;
	send_ack(state);
	return get_at_reply(bi, state);
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
/*	SM_IncomingFunction(statemachine, statemachine->LastMsgType, clink.AT_message, clink.AT_message_len);*/
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

	if (!error)
		error = bus_reset(businst, state);
	if (error) {
		dprintf("C-bus initialization failed (%d)\n", error);
		free(CBUSINST(state));
		CBUSINST(state) = NULL;
	}
	return error;
}
