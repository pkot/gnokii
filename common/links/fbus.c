/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FBUS_(whatever).

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
#include "gsm-statemachine.h"
#include "device.h"

#include "links/fbus.h"

static bool FBUS_OpenSerial(bool dlr3);
static void FBUS_RX_StateMachine(unsigned char rx_byte);
static GSM_Error FBUS_SendMessage(u16 messagesize, u8 messagetype, unsigned char *message);
static int FBUS_TX_SendAck(u8 message_type, u8 message_seq);

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */

/* Some globals */

static GSM_Link *glink;
static GSM_Statemachine *statemachine;
static FBUS_Link flink;		/* FBUS specific stuff, internal to this file */


/*--------------------------------------------*/

static bool FBUS_OpenSerial(bool dlr3)
{
	if (dlr3) dlr3 = 1;
	/* Open device. */
	if (!device_open(glink->PortDevice, false, false, false, GCT_Serial)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}
	device_changespeed(115200);

	/* clearing the RTS bit and setting the DTR bit */
	device_setdtrrts((1-dlr3), 0);

	return (true);
}


static bool FBUS_OpenIR(void)
{
	struct timeval timeout;
	unsigned char init_char = 0x55;
	unsigned char end_init_char = 0xc1;
	unsigned char connect_seq[] = { FBUS_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02 };
	unsigned int count, retry;

	if (!device_open(glink->PortDevice, false, false, false, GCT_Infrared)) {
		perror(_("Couldn't open FBUS device"));
		return false;
	}

	/* clearing the RTS bit and setting the DTR bit */
	device_setdtrrts(1, 0);

	for (retry = 0; retry < 5; retry++) {
		dprintf("IR init, retry=%d\n", retry);

		device_changespeed(9600);

		for (count = 0; count < 32; count++) {
			device_write(&init_char, 1);
		}
		device_write(&end_init_char, 1);
		usleep(100000);

		device_changespeed(115200);

		FBUS_SendMessage(7, 0x02, connect_seq);

		/* Wait for 1 sec. */
		timeout.tv_sec	= 1;
		timeout.tv_usec	= 0;

		if (device_select(&timeout)) {
			dprintf("IR init succeeded\n");
			return(true);
		}
	}

	return (false);
}


/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

static void FBUS_RX_StateMachine(unsigned char rx_byte)
{
	struct timeval time_diff;
	FBUS_IncomingFrame *i = &flink.i;
	int frm_num, seq_num;
	FBUS_IncomingMessage *m;

	/* XOR the byte with the current checksum */
	i->checksum[i->BufferCount & 1] ^= rx_byte;

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
		if (glink->ConnectionType == GCT_Infrared) {
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

		} else {	/* glink->ConnectionType == GCT_Serial */
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

		i->MessageDestination = rx_byte;
		i->state = FBUS_RX_GetSource;

		/* When there is a checksum error and things get out of sync we have to manage to resync */
		/* If doing a data call at the time, finding a 0x1e etc is really quite likely in the data stream */
		/* Then all sorts of horrible things happen because the packet length etc is wrong... */
		/* Therefore we test here for a destination of 0x0c and return to the top if it is not */

		if (rx_byte != 0x0c) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got %2x\n", rx_byte);
		}

		break;

	case FBUS_RX_GetSource:

		i->MessageSource = rx_byte;
		i->state = FBUS_RX_GetType;

		/* Source should be 0x00 */

		if (rx_byte != 0x00) {
			i->state = FBUS_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got %2x\n",rx_byte);
		}

		break;

	case FBUS_RX_GetType:

		i->MessageType = rx_byte;
		i->state = FBUS_RX_GetLength1;

		break;

	case FBUS_RX_GetLength1:

		i->FrameLength = rx_byte << 8;
		i->state = FBUS_RX_GetLength2;

		break;

	case FBUS_RX_GetLength2:

		i->FrameLength = i->FrameLength + rx_byte;
		i->state = FBUS_RX_GetMessage;
		i->BufferCount = 0;

		break;

	case FBUS_RX_GetMessage:

		i->MessageBuffer[i->BufferCount] = rx_byte;
		i->BufferCount++;

		if (i->BufferCount > FBUS_MAX_FRAME_LENGTH) {
			dprintf("FBUS: Message buffer overun - resetting\n");
			i->state = FBUS_RX_Sync;
			break;
		}

		/* If this is the last byte, it's the checksum. */

		if (i->BufferCount == i->FrameLength + (i->FrameLength % 2) + 2) {
			/* Is the checksum correct? */
			if (i->checksum[0] == i->checksum[1]) {

				/* Deal with exceptions to the rules - acks and rlp.. */

				if (i->MessageType == 0x7f) {
					dprintf("[Received Ack of type %02x, seq: %2x]\n",
						i->MessageBuffer[0],(unsigned char) i->MessageBuffer[1]);

				} else {	/* Normal message type */

					/* Add data to the relevant Message buffer */
					/* having checked the sequence number */

					m = &flink.messages[i->MessageType];

					frm_num = i->MessageBuffer[i->FrameLength - 2];
					seq_num = i->MessageBuffer[i->FrameLength - 1];


					/* 0x40 in the sequence number indicates first frame of a message */

					if ((seq_num & 0x40) == 0x40) {
						/* Fiddle around and malloc some memory */
						m->MessageLength = 0;
						m->FramesToGo = frm_num;
						if (m->Malloced != 0) {
							free(m->MessageBuffer);
							m->Malloced = 0;
							m->MessageBuffer = NULL;
						}
						m->Malloced = frm_num * m->MessageLength;
						m->MessageBuffer = (char *) malloc(m->Malloced);

					} else if (m->FramesToGo != frm_num) {
						dprintf("Missed a frame in a multiframe message.\n");
						/* FIXME - we should make sure we don't ack the rest etc */
					}

					if (m->Malloced < m->MessageLength + i->FrameLength) {
						m->Malloced = m->MessageLength + i->FrameLength;
						m->MessageBuffer = (char *) realloc(m->MessageBuffer, m->Malloced);
					}

					memcpy(m->MessageBuffer + m->MessageLength, i->MessageBuffer,
					       i->FrameLength - 2);

					m->MessageLength += i->FrameLength - 2;

					m->FramesToGo--;

					/* Send an ack (for all for now) */

					FBUS_TX_SendAck(i->MessageType, seq_num & 0x0f);

					/* Finally dispatch if ready */

					if (m->FramesToGo == 0) {
						SM_IncomingFunction(statemachine, i->MessageType, m->MessageBuffer, m->MessageLength);
						free(m->MessageBuffer);
						m->MessageBuffer = NULL;
						m->Malloced = 0;
					}
				}
			} else {
				dprintf("Bad checksum!\n");
			}
			i->state = FBUS_RX_Sync;
		}
		break;
	}
}


/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

static GSM_Error FBUS_Loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			FBUS_RX_StateMachine(buffer[count]);
	} else
		return GE_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GE_NONE;
	else
		return GE_INTERNALERROR;
}



/* Prepares the message header and sends it, prepends the message start byte
	   (0x1e) and other values according the value specified when called.
	   Calculates checksum and then sends the lot down the pipe... */

int FBUS_TX_SendFrame(u8 message_length, u8 message_type, u8 * buffer)
{
	u8 out_buffer[FBUS_MAX_TRANSMIT_LENGTH + 5];
	int count, current = 0;
	unsigned char checksum;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */

	if (glink->ConnectionType == GCT_Infrared)
		out_buffer[current++] = FBUS_IR_FRAME_ID;	/* Start of the IR frame indicator */
	else			/* ConnectionType == GCT_Serial */
		out_buffer[current++] = FBUS_FRAME_ID;	/* Start of the frame indicator */

	out_buffer[current++] = FBUS_DEVICE_PHONE;	/* Destination */
	out_buffer[current++] = FBUS_DEVICE_PC;	/* Source */

	out_buffer[current++] = message_type;	/* Type */

	out_buffer[current++] = 0;	/* Length */

	out_buffer[current++] = message_length;	/* Length */

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

	if (device_write(out_buffer, current) != current)
		return (false);

	return (true);
}


/* Main function to send an fbus message */
/* Splits up the message into frames if necessary */

static GSM_Error FBUS_SendMessage(u16 messagesize, u8 messagetype, unsigned char *message)
{
	u8 seqnum, frame_buffer[FBUS_MAX_CONTENT_LENGTH + 2];
	u8 nom, lml;		/* number of messages, last message len */
	int i;

	seqnum = 0x40 + flink.RequestSequenceNumber;
	flink.RequestSequenceNumber =
	    (flink.RequestSequenceNumber + 1) & 0x07;

	if (messagesize > FBUS_MAX_CONTENT_LENGTH) {

		nom = (messagesize + FBUS_MAX_CONTENT_LENGTH - 1)
		    / FBUS_MAX_CONTENT_LENGTH;
		lml = messagesize - ((nom - 1) * FBUS_MAX_CONTENT_LENGTH);

		for (i = 0; i < nom - 1; i++) {

			memcpy(frame_buffer, message + (i * FBUS_MAX_CONTENT_LENGTH),
			       FBUS_MAX_CONTENT_LENGTH);
			frame_buffer[FBUS_MAX_CONTENT_LENGTH] = nom - i;
			frame_buffer[FBUS_MAX_CONTENT_LENGTH + 1] = seqnum;

			FBUS_TX_SendFrame(FBUS_MAX_CONTENT_LENGTH + 2,
					  messagetype, frame_buffer);

			seqnum = flink.RequestSequenceNumber;
			flink.RequestSequenceNumber = (flink.RequestSequenceNumber + 1) & 0x07;
		}

		memcpy(frame_buffer, message + ((nom - 1) * FBUS_MAX_CONTENT_LENGTH), lml);
		frame_buffer[lml] = 0x01;
		frame_buffer[lml + 1] = seqnum;
		FBUS_TX_SendFrame(lml + 2, messagetype, frame_buffer);

	} else {

		memcpy(frame_buffer, message, messagesize);
		frame_buffer[messagesize] = 0x01;
		frame_buffer[messagesize + 1] = seqnum;
		FBUS_TX_SendFrame(messagesize + 2, messagetype,
				  frame_buffer);
	}
	return (GE_NONE);
}


static int FBUS_TX_SendAck(u8 message_type, u8 message_seq)
{
	unsigned char request[2];

	request[0] = message_type;
	request[1] = message_seq;

	dprintf("[Sending Ack of type %02x, seq: %x]\n",message_type, message_seq);

	return FBUS_TX_SendFrame(2, 0x7f, request);
}


/* Initialise variables and start the link */
/* newlink is actually part of state - but the link code should not anything about state */
/* state is only passed around to allow for muliple state machines (one day...) */

GSM_Error FBUS_Initialise(GSM_Link *newlink, GSM_Statemachine *state, int type)
{
	unsigned char init_char = 0x55;
	unsigned char count;

	if (type > 2) return GE_DEVICEOPENFAILED;
	/* 'Copy in' the global structures */
	glink = newlink;
	statemachine = state;

	/* Fill in the link functions */
	glink->Loop = &FBUS_Loop;
	glink->SendMessage = &FBUS_SendMessage;

	/* Check for a valid init length */
	if (glink->InitLength == 0)
		glink->InitLength = 250;

	/* Start up the link */
	flink.RequestSequenceNumber = 0;

	if (glink->ConnectionType == GCT_Infrared) {
		if (!FBUS_OpenIR())
			return GE_DEVICEOPENFAILED;
	} else {		/* ConnectionType == GCT_Serial */
		/* FBUS_OpenSerial(0) - try dau-9p
		 * FBUS_OpenSerial(n != 0) - try dlr-3p */
		if (!FBUS_OpenSerial(type))
			return GE_DEVICEOPENFAILED;
	}

	/* Send init string to phone, this is a bunch of 0x55 characters. Timing is
	   empirical. */
	/* I believe that we need/can do this for any phone to get the UART synced */

	for (count = 0; count < glink->InitLength; count++) {
		usleep(100);
		device_write(&init_char, 1);
	}

	/* Init variables */
	flink.i.state = FBUS_RX_Sync;
	flink.i.BufferCount = 0;
	for (count = 0; count < FBUS_MAX_MESSAGE_TYPES; count++) {
		flink.messages[count].Malloced = 0;
		flink.messages[count].FramesToGo = 0;
		flink.messages[count].MessageLength = 0;
	}

	return GE_NONE;
}
