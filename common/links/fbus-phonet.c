/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

  The various routines are called PHONET_(whatever).

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
#include "device.h"

#define __links_fbus_phonet_c
#include "links/fbus.h"
#include "links/fbus-phonet.h"

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */


/* Some globals */

GSM_Link *glink;
GSM_Statemachine *statemachine;
PHONET_IncomingMessage imessage;


/*--------------------------------------------*/

bool PHONET_Open()
{
	int result;

	/* Open device. */
	result = device_open(glink->PortDevice, false, false, false, GCT_Irda);

	if (!result) {
		perror(_("Couldn't open PHONET device"));
		return false;
	}

	return (true);
}

/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

void PHONET_RX_StateMachine(unsigned char rx_byte)
{
	PHONET_IncomingMessage *i = &imessage;

	if (isprint(rx_byte))
		fprintf(stderr, "[%02x%c]", (unsigned char) rx_byte, rx_byte);
	else
		fprintf(stderr, "[%02x ]", (unsigned char) rx_byte);


	switch (i->state) {

	case FBUS_RX_Sync:
		if (rx_byte == FBUS_PHONET_FRAME_ID) {
			i->state = FBUS_RX_GetDestination;
		}
		break;

	case FBUS_RX_GetDestination:

		i->MessageDestination = rx_byte;
		i->state = FBUS_RX_GetSource;

		if (rx_byte != 0x0c) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got %2x\n", rx_byte);
		}
		break;

	case FBUS_RX_GetSource:

		i->MessageSource = rx_byte;
		i->state = FBUS_RX_GetType;

		/* Source should be 0x00 */

		if (rx_byte!=0x00)  {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got %2x\n", rx_byte);
		}

		break;

	case FBUS_RX_GetType:

		i->MessageType = rx_byte;
		i->state = FBUS_RX_GetLength1;

		break;

	case FBUS_RX_GetLength1:

		i->MessageLength = rx_byte << 8;
		i->state = FBUS_RX_GetLength2;

		break;

	case FBUS_RX_GetLength2:

		i->MessageLength = i->MessageLength + rx_byte;
		i->state = FBUS_RX_GetMessage;
		i->BufferCount=0;

		break;

	case FBUS_RX_GetMessage:

		i->MessageBuffer[i->BufferCount] = rx_byte;
		i->BufferCount++;

		if (i->BufferCount > PHONET_MAX_FRAME_LENGTH) {
			dprintf("PHONET: Message buffer overun - resetting\n");
			i->state = FBUS_RX_Sync;
			break;
		}

		/* Is that it? */

		if (i->BufferCount == i->MessageLength) {
			SM_IncomingFunction(statemachine, i->MessageType, i->MessageBuffer, i->MessageLength);
			i->state = FBUS_RX_Sync;
		}
		break;
	default:
		i->state = FBUS_RX_Sync;
		break;
	}
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

GSM_Error PHONET_Loop(struct timeval *timeout)
{
	GSM_Error	error = GE_INTERNALERROR;
	unsigned char	buffer[255];
	int		count, res;

	res = device_select(timeout);

	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++) {
			PHONET_RX_StateMachine(buffer[count]);
		}
		if (res > 0) {
			error = GE_NONE;	/* This traps errors from device_read */
		}
	} else if (!res) {
		error = GE_TIMEOUT;
	}

	return error;
}

/* Main function to send an fbus message */

GSM_Error PHONET_SendMessage(u16 messagesize, u8 messagetype, void *message) {

	u8 out_buffer[PHONET_MAX_TRANSMIT_LENGTH + 5];
	int current = 0;
	int total, sent;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */

	out_buffer[current++] = FBUS_PHONET_FRAME_ID;
	out_buffer[current++] = FBUS_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FBUS_DEVICE_PC;    /* Source */

	out_buffer[current++] = messagetype; /* Type */

	out_buffer[current++] = (messagesize & 0xff00) >> 8;
	out_buffer[current++] = (messagesize & 0x00ff);

	/* Copy in data if any. */

	if (messagesize > 0) {
		memcpy(out_buffer + current, message, messagesize);
		current += messagesize;
	}

	{
		int count;
		dprintf("PC: ");

		for (count = 0; count < current; count++)
			dprintf("%02x:", out_buffer[count]);

		dprintf("\n");
	}

	/* Send it out... */

	total = current;
	current = 0;
	sent = 0;

	do {
		sent = device_write(out_buffer + current, total - current);
		if (sent < 0) return (false);
		else current += sent;
	} while (current < total);

	return GE_NONE;
}



/* Initialise variables and start the link */

GSM_Error PHONET_Initialise(GSM_Link *newlink, GSM_Statemachine *state)
{
	GSM_Error error = GE_INTERNALERROR;

	/* 'Copy in' the global structures */
	glink = newlink;
	statemachine = state;

	/* Fill in the link functions */
	glink->Loop = &PHONET_Loop;
	glink->SendMessage = &PHONET_SendMessage;

	if ((glink->ConnectionType == GCT_Infrared) || (glink->ConnectionType == GCT_Irda)) {
		if (PHONET_Open() == true) {
			error = GE_NONE;

			/* Init variables */
			imessage.state = FBUS_RX_Sync;
			imessage.BufferCount = 0;
		} else {
			error = GE_DEVICEOPENFAILED;
		}
	} else {
		error = GE_DEVICEOPENFAILED;	/* ConnectionType == GCT_Serial etc */
	}

	return error;
}
