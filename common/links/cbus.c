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

/* Various header file */
#include "config.h"
#include "misc.h"
#include "gsm-data.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"

#include "links/cbus.h"
#include "links/atbus.h"

/* Some globals */
static GSM_Statemachine *statemachine;

/* CBUS specific stuff, internal to this file */
static CBUS_Link clink;


static gn_error bus_write(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_write(d, len);
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

static gn_error bus_read(unsigned char *rx_byte, unsigned int timeout)
{
	int res;
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * timeout;
/*	res = device_select(&tv);*/
	res = device_select(NULL);
	if (res > 0)
		res = device_read(rx_byte, 1);
	else
		return GN_ERR_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}

static gn_error send_packet(unsigned char *msg, int len, unsigned short cmd, bool init)
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

	return bus_write(p, pos+1);
}

static void send_ack(void)
{
	dprintf("CBUS: sending ack\n");
	bus_write("\xa5", 1);
}

static void dump_packet(CBUS_IncomingFrame *frame)
{
	int i;
	
	fprintf(stderr, "Hdr: %02x %02x   ", frame->FrameHeader1, frame->FrameHeader2);
	fprintf(stderr, (frame->FrameHeader1 == 0x19) ? "->\n" : "<-\n");
	fprintf(stderr, "Cmd: %02x %02x\n", frame->FrameType1, frame->FrameType2);
	fprintf(stderr, "Len: %d\n", frame->MessageLength);
	fprintf(stderr, "Data: ");
	for (i = 0; i < frame->BufferCount; i++)
		fprintf(stderr, "%02x ", frame->buffer[i]);
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

/* state machine for receive handling. Called once for each character
   received from the phone. */

static CBUS_PKT_State state_machine(unsigned char rx_byte)
{
	CBUS_PKT_State state = CBUS_PKT_None;
	CBUS_IncomingFrame *i = &clink.frame;

	/* checksum is XOR of all bytes in the frame */
	if (i->state != CBUS_RX_GetCSum) i->checksum ^= rx_byte;

	switch (i->state) {

	case CBUS_RX_Header:
		switch (rx_byte) {
			case 0x38:
			case 0x34:
				if (i->prev_rx_byte == 0x19) {
					i->state = CBUS_RX_FrameType1;
				}
				break;
			case 0x19:
				if ((i->prev_rx_byte == 0x38) || (i->prev_rx_byte == 0x34)) {
					i->state = CBUS_RX_FrameType1;
				}
				break;
			default:
				dprintf("CBUS: discarding (%02x)\n", rx_byte);
				break;
		}
		if (i->state != CBUS_RX_Header) {
			i->FrameHeader1 = i->prev_rx_byte;
			i->FrameHeader2 = rx_byte;
			i->BufferCount = 0;
			i->checksum = i->prev_rx_byte ^ rx_byte;
		}
		break;

	case CBUS_RX_FrameType1:
		i->FrameType1 = rx_byte;
		i->state = CBUS_RX_FrameType2;
		break;

	case CBUS_RX_FrameType2:
		i->FrameType2 = rx_byte;
		i->state = CBUS_RX_GetLengthLB;
		break;

	/* message length - low byte */
	case CBUS_RX_GetLengthLB:
		i->MessageLength = rx_byte;
		i->state = CBUS_RX_GetLengthHB;
		break;

	/* message length - high byte */
	case CBUS_RX_GetLengthHB:
		i->MessageLength = i->MessageLength | rx_byte << 8;
		/* there are also empty messages */
		if (i->MessageLength == 0)
			i->state = CBUS_RX_GetCSum;
		else
			i->state = CBUS_RX_GetMessage;
		break;

	/* get each byte of the message body */
	case CBUS_RX_GetMessage:
		i->buffer[i->BufferCount] = rx_byte;
		/* avoid buffer overflow */		if (i->BufferCount > CBUS_MAX_MSG_LENGTH) {
		dprintf("CBUS: Message buffer overun - resetting\n");
			i->state = CBUS_RX_Header;
			break;
		}

		if (i->BufferCount == i->MessageLength)
			i->state = CBUS_RX_GetCSum;
		break;

	/* get checksum */
	case CBUS_RX_GetCSum:
		/* compare against calculated checksum. */
		if (i->checksum == rx_byte) {
			dump_packet(i);
			state = CBUS_PKT_Ready;
		} else {
			dprintf("CBUS: Checksum error; expected: %02x, got: %02x\n", i->checksum, rx_byte);
			state = CBUS_PKT_CSumErr;
		}
		i->buffer[i->MessageLength + 1] = 0;
		i->state = CBUS_RX_Header;
		break;
	default:
		break;
	}

	i->prev_rx_byte = rx_byte;
	return state;
}

static gn_error get_packet(void)
{
	gn_error res;
	CBUS_PKT_State state = CBUS_PKT_None;
	unsigned char rx_byte;
	int retry = 20;
	CBUS_IncomingFrame *i = &clink.frame;	

	i->state = CBUS_RX_Header;
	while (state == CBUS_PKT_None && --retry) {
		res = bus_read(&rx_byte, 10);
		if (!res) {
			retry = 20;
			state = state_machine(rx_byte);
		} else
			usleep(1000);
/*			if (--retry) {
				usleep(5000);
				continue;
			}
			return res;
		}
		retry = 20;
		state = state_machine(rx_byte);*/
	}
	dprintf("CBUS: Got packet\n");
	return (res) ? GN_ERR_INTERNALERROR : GN_ERR_NONE;
}

static gn_error wait_ack(void)
{
	gn_error res;
	unsigned char rx_byte;
	int retry = 555550;

/*	do {
		res = bus_read(&rx_byte, 10);
		if (!res) {
			device_flush();
			return (rx_byte == 0xa5) ? GE_NONE : GE_INTERNALERROR;
		}
		usleep(5000);
	} while (--retry);*/
	while (--retry) {
		res = bus_read(&rx_byte, 10);
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
static gn_error send_packet_ack(unsigned char *msg, int len, unsigned short cmd, bool init)
{
	gn_error res;
	int retry = 3;
	CBUS_IncomingFrame *i = &clink.frame;

	do {
		retry--;
		res = send_packet(msg, len, cmd, init);
		if (res) {
			dprintf("CBUS: send_packet failed\n");
			continue;
		}
		usleep(50000);
again:
		res = get_packet();
		if (res) {
			dprintf("CBUS: get_packet failed\n");
			continue;
		}
		if (i->FrameType1 != cmd) {
			dprintf("CBUS: get_packet is not the right packet\n");
			goto again;
		}
		res = wait_ack();
		if (res) {
			dprintf("CBUS: wait_ack failed\n");
			continue;
		}
		dprintf("CBUS: Phone acks\n");
	} while (retry && res);
	
	return res;
}

static gn_error bus_reset(void)
{
	gn_error res;
	CBUS_IncomingFrame *i = &clink.frame;

/*	bus_write("\0\0\0\0\0", 5);*/
	usleep(1000);
	send_packet_ack("", 0, 0xa6, true);
	send_packet_ack("D", 1, 0x90, true);
	res = get_packet();
	if (res)
		return res;
	if (i->FrameType1 != 0x91) {
		dprintf("CBUS: Bus reset failed\n");
		return GN_ERR_NOLINK;
	}
	send_ack();
	usleep(1000);
	send_ack();
	usleep(1000);
	send_ack();
	usleep(1000);
	send_ack();
	dprintf("CBUS: Bus reset ok\n");
	usleep(100000);
	return GN_ERR_NONE;
}

/*
			send_ack();
			clink.AT_message[0] = GEAT_OK;
			strncpy(clink.AT_message + 1, buffer, length);
			clink.AT_message_len = length;
			send_packet("\x3e\x68", 2, 0x3f, false);
*/

static gn_error wait_at_confirmation(void)
{
	gn_error res;
	CBUS_IncomingFrame *i = &clink.frame;
	
	res = get_packet();
	if (res)
		return res;
	if (i->FrameType1 != 0x3d) {
		dprintf("CBUS: AT command confirmation expected, but got %02x/%02x\n", i->FrameType1, i->FrameType2);
		return GN_ERR_INTERNALERROR;
	}
	dprintf("CBUS: AT command confirmed\n");
	return GN_ERR_NONE;
}

static gn_error get_at_reply(void)
{
	gn_error res;
	CBUS_IncomingFrame *i = &clink.frame;

	res = get_packet();
	if (res)
		return res;
	if (i->FrameType2 != 0x3e) {
		dprintf("CBUS: AT command reply expected, but got %02x/%02x\n", i->FrameType1, i->FrameType2);
		return GN_ERR_INTERNALERROR;
	}
	dprintf("CBUS: AT reply: %s", i->buffer);
	send_ack();
	return send_packet_ack("\x3e\x68", 2, 0x3f, false);
}

static gn_error AT_SendMessage(u16 message_length, u8 message_type, unsigned char *buffer)
{
	gn_error res;

	res = send_packet_ack(buffer, message_length, 0x3c, false);
	if (res)
		return res;
	dprintf("CBUS: AT command sent - %s\n", buffer);
/*	usleep(100000);*/
	res = wait_at_confirmation();
	if (res)
		return res;
	send_ack();
	return get_at_reply();
}

static bool CBUS_OpenSerial(char *device)
{
	int result = device_open(device, false, false, false, GCT_Serial);
	if (!result) {
		perror(_("Couldn't open CBUS device"));
		return false;
	}
	device_changespeed(9600);
	device_setdtrrts(1, 0);
	return true;
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

static gn_error CBUS_Loop(struct timeval *timeout)
{
/*	SM_IncomingFunction(statemachine, statemachine->LastMsgType, clink.AT_message, clink.AT_message_len);*/
	return GN_ERR_NONE;
}


/* Initialise variables and start the link */

gn_error CBUS_Initialise(GSM_Statemachine *state)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* 'Copy in' the global structures */
	statemachine = state;
		
	/* Fill in the link functions */
	state->Link.Loop = &CBUS_Loop;
	state->Link.SendMessage = &AT_SendMessage;

	if (state->Link.ConnectionType == GCT_Serial) {
		if (!CBUS_OpenSerial(state->Link.PortDevice))
			return GN_ERR_FAILED;
	} else {
		dprintf("Device not supported by CBUS\n");
		return GN_ERR_FAILED;
	}

	return bus_reset();
}
