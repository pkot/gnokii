/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

*/

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_BLUETOOTH
#  include <bluetooth/bluetooth.h>
#endif

/* Various header file */
#include "config.h"
#include "compat.h"
#include "misc.h"
#include "links/atbus.h"
#include "links/utils.h"

#include "gnokii-internal.h"
#include "gnokii.h"

#include "device.h"

/* 
 * FIXME - when sending an AT command while another one is still in progress,
 * the old command is aborted and the new ignored. the result is _one_ error
 * message from the phone.
 */

static int xwrite(unsigned char *d, int len, struct gn_statemachine *sm)
{
	int res;

	at_dprintf("write: ", d, len);

	while (len) {
		res = device_write(d, len, sm);
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

static bool atbus_serial_open(int mode, char *device, struct gn_statemachine *sm)
{
	int result = device_open(device, false, false, mode, GN_CT_Serial, sm);

	if (!result) {
		perror(_("Couldn't open ATBUS device"));
		return false;
	}
	if (mode) {
		/* 
		 * make 7110 with dlr-3 happy. the nokia dlr-3 cable provides
		 * hardware handshake lines but is, at least at initialization,
		 * slow. to be properly detected, state changes must be longer
		 * than 700 ms. if the timing is too fast all commands after dtr
		 * high will be ignored by the phone until dtr is toggled again.
		 * to reset the phone and set a sane state, dtr must pulled low.
		 * when irda is turned on in the phone, dtr must pulled high to
		 * switch on the serial line of the phone (this will switch off
		 * irda). only set it high after serial line initialization
		 * (when it probably was low before) is not enough. so we do
		 * high, low and high again, always with one second apply time.
		 * also wait one second before sending commands or init will
		 * fail.
		 */
		device_setdtrrts(1, 1, sm);
		sleep(1);
		device_setdtrrts(0, 1, sm);
		sleep(1);
		device_setdtrrts(1, 1, sm);
		sleep(1);
	} else {
		device_setdtrrts(1, 1, sm);
	}
	return true;
}

static gn_error at_send_message(unsigned int message_length, unsigned char message_type, unsigned char *msg, struct gn_statemachine *sm)
{
	usleep(10000);
	sm_incoming_acknowledge(sm);
	return xwrite(msg, message_length, sm) ? GN_ERR_UNKNOWN : GN_ERR_NONE;
}

char *findcrlfbw(unsigned char *str, int len)
{
	while ((*str != '\n') && (*str-1 != '\r') && len--)
		str--;
	return len ? str+1 : NULL;
}

/* 
 * rx state machine for receive handling. called once for each character
 * received from the phone. 
 */
static void atbus_rx_statemachine(unsigned char rx_char, struct gn_statemachine *sm)
{
	int error;
	atbus_instance *bi = AT_BUSINST(sm);
	
	bi->rbuf[bi->rbuf_pos++] = rx_char;
	bi->rbuf[bi->rbuf_pos] = '\0';

	if (bi->rbuf_pos < bi->binlen)
		return;

	bi->rbuf[0] = GN_AT_NONE;
	/* first check if <cr><lf> is found at end of reply_buf.
	 * none: the needed length is greater 4 because we don't
	 * need to enter if no result/error will be found. */
	if (bi->rbuf_pos > 4 && !strncmp(bi->rbuf + bi->rbuf_pos - 2, "\r\n", 2)) {
		/* try to find previous <cr><lf> */
		char *start = findcrlfbw(bi->rbuf + bi->rbuf_pos - 2, bi->rbuf_pos - 1);
		/* if not found, start at buffer beginning */
		if (!start)
			start = bi->rbuf+1;
		/* there are certainly more that 2 chars in buffer */
		if (!strncmp(start, "OK", 2))
			bi->rbuf[0] = GN_AT_OK;
		else if (bi->rbuf_pos > 7 && !strncmp(start, "ERROR", 5))
			bi->rbuf[0] = GN_AT_ERROR;
		/* FIXME: use error codes some useful way */
		else if (sscanf(start, "+CMS ERROR: %d", &error) == 1)
			bi->rbuf[0] = GN_AT_ERROR;
		else if (sscanf(start, "+CME ERROR: %d", &error) == 1)
			bi->rbuf[0] = GN_AT_ERROR;
	}
	/* check if SMS prompt is found */
	if (bi->rbuf_pos > 4 && !strncmp(bi->rbuf + bi->rbuf_pos - 4, "\r\n> ", 4))
		bi->rbuf[0] = GN_AT_PROMPT;
	if (bi->rbuf[0] != GN_AT_NONE) {
		at_dprintf("read : ", bi->rbuf + 1, bi->rbuf_pos - 1);
		sm_incoming_function(sm->last_msg_type, bi->rbuf, bi->rbuf_pos - 1, sm);
		bi->rbuf_pos = 1;
		bi->binlen = 1;
		return;
	}
#if 0
	/* needed for binary date etc */
	TODO: correct reading of variable length integers
	if (reply_buf_pos == 12) {
		if (!strncmp(reply_buf + 3, "ABC", 3) {
			binlength = atoi(reply_buf + 8);
		}
	}
#endif
}

static gn_error atbus_loop(struct timeval *timeout, struct gn_statemachine *sm)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout, sm);
	if (res > 0) {
		res = device_read(buffer, 255, sm);
		for (count = 0; count < res; count++)
			atbus_rx_statemachine(buffer[count], sm);
	} else
		return GN_ERR_TIMEOUT;
	/* This traps errors from device_read */
	if (res > 0)
		return GN_ERR_NONE;
	else
		return GN_ERR_INTERNALERROR;
}


/* Initialise variables and start the link */
/* Fixme we allow serial and irda for connection to reduce */
/* bug reports. this is pretty silly for /dev/ttyS?. */
gn_error atbus_initialise(int mode, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	atbus_instance *businst;

	if (!(businst = malloc(sizeof(atbus_instance))))
		return GN_ERR_FAILED;

	/* Fill in the link functions */
	state->link.loop = &atbus_loop;
	state->link.send_message = &at_send_message;
	businst->rbuf_pos = 1;
	businst->binlen = 1;
	AT_BUSINST(state) = businst;

	switch (state->config.connection_type) {
	case GN_CT_Serial:
	case GN_CT_Irda:
		if (!atbus_serial_open(mode, state->config.port_device, state)) {
			error = GN_ERR_FAILED;
			goto err;
		}
		break;
#ifdef HAVE_BLUETOOTH
	case GN_CT_Bluetooth:
		if (!device_open(state->config.port_device, false, false, false, state->config.connection_type, state)) {
			error = GN_ERR_FAILED;
			goto err;
		}
		break;
#endif
	default:
		dprintf("Device not supported by AT bus\n");
		error = GN_ERR_FAILED;
		goto err;
	}

	goto out;
err:
	dprintf("AT bus initialization failed (%d)\n", error);
	free(AT_BUSINST(state));
	AT_BUSINST(state) = NULL;
out:
	return error;
}
