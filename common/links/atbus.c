/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log$
  Revision 1.4  2001-09-09 21:45:49  machek
  Cleanups from Ladislav Michl <ladis@psi.cz>:

  *) do *not* internationalize debug messages

  *) some whitespace fixes, do not use //

  *) break is unneccessary after return

  Revision 1.3  2001/08/20 23:27:37  pkot
  Add hardware shakehand to the link layer (Manfred Jonsson)

  Revision 1.2  2001/08/09 11:51:39  pkot
  Generic AT support updates and cleanup (Manfred Jonsson)

  Revision 1.1  2001/07/27 00:02:21  pkot
  Generic AT support for the new structure (Manfred Jonsson)

*/

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header file */
#include "config.h"
#ifndef DEBUG
#define DEBUG
#endif
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


/* Some globals */
static GSM_Link *glink;
static GSM_Statemachine *statemachine;
static int binlength = 0;
static char reply_buf[1024];
static int reply_buf_pos = 0;


static int xwrite(unsigned char *d, int len)
{
	int res;
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
		|| ((reply_buf_pos > 6) && (!strncmp(reply_buf+reply_buf_pos-7, "ERROR\r\n", 7)))) {
			SM_IncomingFunction(statemachine, 1, reply_buf, reply_buf_pos);
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


bool ATBUS_OpenSerial(int hw_handshake)
{
	int result;
	result = device_open(glink->PortDevice, false, false, hw_handshake, GCT_Serial);
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
		device_setdtrrts(0, 0);
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
GSM_Error ATBUS_Initialise(GSM_Statemachine *state, int hw_handshake)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* 'Copy in' the global structures */
	glink = &(state->Link);
	statemachine = state;

	/* Fill in the link functions */
	glink->Loop = &ATBUS_Loop;
	glink->SendMessage = &AT_SendMessage;

	if (glink->ConnectionType == GCT_Serial) {
		if (!ATBUS_OpenSerial(hw_handshake))
			return GE_DEVICEOPENFAILED;
	} else {
		fprintf(stderr, "Device not supported by ATBUS");
		return GE_DEVICEOPENFAILED;
	}

	return GE_NONE;
}
