/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header file */
#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"
#include "links/utils.h"
#include "gsm-statemachine.h"

#define __atbus_c
#include "links/atbus.h"

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */

static void at_printf(char *prefix, char *buf, int len);

/* FIXME - when sending an AT command while another one is still in */
/*         progress, the old command is aborted and the new ignored. */
/*         the result is _one_ error message from the phone. */

/* Some globals */
/* FIXME - if we use more than one phone these should be part of */
/*         a statemachine. */

static GSM_Statemachine *statemachine;
static int binlength = 0;
static char reply_buf[1024];
static int reply_buf_pos = 0;

static int xwrite(unsigned char *d, int len)
{
	int res;

	at_printf("write: ", d, len);

	while (len) {
		res = device_write(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				perror("gnokii I/O error ");
				return -1;
			}
		} else {
			d += res;
			len -= res;
		}
	}
	return 0;
}


static GSM_Error
AT_SendMessage(u16 message_length, u8 message_type, void *msg)
{
	usleep(10000);
	xwrite((char*)msg, message_length);
	return GE_NONE;
}


/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */
void ATBUS_RX_StateMachine(unsigned char rx_char)
{
	reply_buf[reply_buf_pos++] = rx_char;
	reply_buf[reply_buf_pos] = '\0';

	if (reply_buf_pos >= binlength) {
		if (((reply_buf_pos > 3) && (!strncmp(reply_buf+reply_buf_pos-4, "OK\r\n", 4)))
		|| ((reply_buf_pos > 6) && (!strncmp(reply_buf+reply_buf_pos-7, "ERROR\r\n", 7)))
		|| ((reply_buf_pos > 3) && (!strncmp(reply_buf+reply_buf_pos-4, "\r\n> ", 4)))) {


			at_printf("read : ", reply_buf, reply_buf_pos);

			SM_IncomingFunction(statemachine, statemachine->LastMsgType, reply_buf, reply_buf_pos);
			reply_buf_pos = 0;
			binlength = 0;
			return;
		}
/* needed for binary date etc
   TODO: correct reading of variable length integers
		if (reply_buf_pos == 12) {
			if (!strncmp(reply_buf + 3, "ABC", 3) {
				binlength = atoi(reply_buf + 8);
			}
		}
*/
	}
}


bool ATBUS_OpenSerial(int hw_handshake, char *device)
{
	int result;
	result = device_open(device, false, false, hw_handshake, GCT_Serial);
	if (!result) {
		perror(_("Couldn't open ATBUS device"));
		return (false);
	}
	device_changespeed(19200);
	if (hw_handshake) {
		device_setdtrrts(0, 1);
		sleep(1);
		device_setdtrrts(1, 1);
		/* make 7110 happy */
		sleep(1);
	} else {
		device_setdtrrts(1, 1);
	}
	return (true);
}

GSM_Error ATBUS_Loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			ATBUS_RX_StateMachine(buffer[count]);
	} else
		return GE_TIMEOUT;
	/* This traps errors from device_read */
	if (res > 0)
		return GE_NONE;
	else
		return GE_INTERNALERROR;
}


/* Initialise variables and start the link */
/* Fixme we allow serial and irda for connection to reduce */
/* bug reports. this is pretty silly for /dev/ttyS?. */
GSM_Error ATBUS_Initialise(GSM_Statemachine *state, int hw_handshake)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* 'Copy in' the global structures */
	statemachine = state;

	/* Fill in the link functions */
	state->Link.Loop = &ATBUS_Loop;
	state->Link.SendMessage = &AT_SendMessage;

	if ((state->Link.ConnectionType == GCT_Serial)
	|| (state->Link.ConnectionType == GCT_Irda)) {
		if (!ATBUS_OpenSerial(hw_handshake, state->Link.PortDevice))
			return GE_DEVICEOPENFAILED;
	} else {
		fprintf(stderr, "Device not supported by ATBUS");
		return GE_DEVICEOPENFAILED;
	}

	return GE_NONE;
}

static void at_printf(char *prefix, char *buf, int len)
{
#ifdef DEBUG
	int in = 0, out = 0;
	char *pos = prefix;
	char debug_buf[1024];

	while (*pos)
		debug_buf[out++] = *pos++;
	debug_buf[out++] ='[';
	while ((in < len) && (out < 1016)) {
		if (buf[in] == '\n') {
			sprintf(debug_buf + out,"<lf>");
			in++;
			out += 4;
		} else if (buf[in] == '\r') {
			sprintf(debug_buf + out,"<cr>");
			in++;
			out += 4;
		} else if (buf[in] < 32) {
			debug_buf[out++] = '^';
			debug_buf[out++] = buf[in++] + 64;
		} else {
			debug_buf[out++] = buf[in++];
		}
	}
	debug_buf[out++] =']';
	debug_buf[out++] ='\n';
	debug_buf[out] ='\0';
	dprintf(debug_buf);
#endif
}
