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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001      Chris Kemp, Manfred Jonsson
  Copyright (C) 2001-2002 Pavel Machek, Ladis Michl
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2003      Osma Suominen

*/

#include "config.h"

#include "gnokii-internal.h"
#include "gnokii.h"
#include "misc.h"

gn_error sm_initialise(struct gn_statemachine *state)
{
	state->current_state = GN_SM_Initialised;
	state->waiting_for_number = 0;
	state->received_number = 0;

	return GN_ERR_NONE;
}

gn_error sm_message_send(u16 messagesize, u8 messagetype, void *message, struct gn_statemachine *state)
{
	if (state->current_state != GN_SM_Startup) {
#ifdef	DEBUG
		dprintf("Message sent: ");
		sm_message_dump(gn_log_debug, messagetype, message, messagesize);
#endif
		state->last_msg_size = messagesize;
		state->last_msg_type = messagetype;
		state->last_msg = message;
		state->current_state = GN_SM_MessageSent;

		/* FIXME - clear KeepAlive timer */
		return state->link.send_message(messagesize, messagetype, message, state);
	}
	else return GN_ERR_NOTREADY;
}

GNOKII_API gn_state gn_sm_loop(int timeout, struct gn_statemachine *state)
{
	struct timeval loop_timeout;
	int i;

	if (!state->link.loop) {
		dprintf("No Loop function. Aborting.\n");
		abort();
	}
	for (i = 0; i < timeout; i++) {
		/*
		 * Some select() implementation (e.g. Linux) will modify the
		 * timeval structure - bozo
		 */
		loop_timeout.tv_sec = 0;
		loop_timeout.tv_usec = 100000;

		state->link.loop(&loop_timeout, state);
	}

	/* FIXME - add calling a KeepAlive function here */
	return state->current_state;
}

void sm_reset(struct gn_statemachine *state)
{
	/* Don't reset to initialised if we aren't! */
	if (state->current_state != GN_SM_Startup) {
		state->current_state = GN_SM_Initialised;
		state->waiting_for_number = 0;
		state->received_number = 0;
		if (state->link.reset)
			state->link.reset(state);
	}
}

void sm_incoming_function(u8 messagetype, void *message, u16 messagesize, struct gn_statemachine *state)
{
	int c;
	int temp = 1;
	gn_data *data, *edata;
	gn_error res = GN_ERR_INTERNALERROR;
	int waitingfor = -1;

#ifdef	DEBUG
	dprintf("Message received: ");
	sm_message_dump(gn_log_debug, messagetype, message, messagesize);
#endif
	edata = (gn_data *)calloc(1, sizeof(gn_data));
	data = edata;

	/* See if we need to pass the function the data struct */
	if (state->current_state == GN_SM_WaitingForResponse)
		for (c = 0; c < state->waiting_for_number; c++)
			if (state->waiting_for[c] == messagetype) {
				data = state->data[c];
				waitingfor = c;
			}

	/* Pass up the message to the correct phone function, with data if necessary */
	c = 0;
	while (state->driver.incoming_functions[c].functions) {
		if (state->driver.incoming_functions[c].message_type == messagetype) {
			dprintf("Received message type %02x\n", messagetype);
			res = state->driver.incoming_functions[c].functions(messagetype, message, messagesize, data, state);
			temp = 0;
			break;
		}
		c++;
	}
	if (res == GN_ERR_UNSOLICITED) {
		dprintf("Unsolicited frame, skipping...\n");
		free(edata);
		return;
	} else if (res == GN_ERR_UNHANDLEDFRAME) {
		sm_unhandled_frame_dump(messagetype, message, messagesize, state);
	} else if (res == GN_ERR_WAITING) {
		free(edata);
		return;
	}
	if (temp != 0) {
		dprintf("Unknown Frame Type %02x\n", messagetype);
		state->driver.default_function(messagetype, message, messagesize, state);
		free(edata);
		return;
	}

	if (state->current_state == GN_SM_WaitingForResponse) {
		/* Check if we were waiting for a response and we received it */
		if (waitingfor != -1) {
			state->response_error[waitingfor] = res;
			state->received_number++;
		}

		/* Check if all waitingfors have been received */
		if (state->received_number == state->waiting_for_number) {
			state->current_state = GN_SM_ResponseReceived;
		}
	}
	free(edata);
}

/* This returns the error recorded from the phone function and indicates collection */
gn_error sm_error_get(unsigned char messagetype, struct gn_statemachine *state)
{
	int c, d;
	gn_error error = GN_ERR_NOTREADY;

	switch (state->current_state) {
	case GN_SM_ResponseReceived:
		for (c = 0; c < state->received_number; c++)
			if (state->waiting_for[c] == messagetype) {
				error = state->response_error[c];
				for (d = c + 1 ; d < state->received_number; d++) {
					state->response_error[d-1] = state->response_error[d];
					state->waiting_for[d-1] = state->waiting_for[d];
					state->data[d-1] = state->data[d];
				}
				state->received_number--;
				state->waiting_for_number--;
				c--; /* For neatness continue in the correct place */
			}
		if (state->received_number == 0) {
			state->waiting_for_number = 0;
			state->current_state = GN_SM_Initialised;
		}
		break;
	/* Here we are fine! */
	case GN_SM_Initialised:
		error = GN_ERR_NONE;
		break;
	default:
		break;
	}

	return error;
}



/* Indicate that the phone code is waiting for a response */
/* This does not actually wait! */
gn_error sm_wait_for(unsigned char messagetype, gn_data *data, struct gn_statemachine *state)
{
	/* If we've received a response, we have to call SM_GetError first */
	if ((state->current_state == GN_SM_Startup) || (state->current_state == GN_SM_ResponseReceived))
		return GN_ERR_NOTREADY;

	if (state->waiting_for_number == GN_SM_WAITINGFOR_MAX_NUMBER) return GN_ERR_NOTREADY;
	state->waiting_for[state->waiting_for_number] = messagetype;
	state->data[state->waiting_for_number] = data;
	state->response_error[state->waiting_for_number] = GN_ERR_BUSY;
	state->waiting_for_number++;

	return GN_ERR_NONE;
}

void sm_incoming_acknowledge(struct gn_statemachine *state)
{
	if (state->current_state == GN_SM_MessageSent)
		state->current_state = GN_SM_WaitingForResponse;
}


/* This function is for convinience only.
   It is called after SM_SendMessage and blocks until a response is received.
   t is in tenths of second.
*/
static gn_error __sm_block_timeout(int waitfor, int t, gn_data *data, struct gn_statemachine *state)
{
	int retry;
	gn_state s;
	gn_error err;
	struct timeval now, next, timeout;

	s = state->current_state;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	gettimeofday(&now, NULL);
	for (retry = 0; retry < 2; retry++) {
		err = sm_wait_for(waitfor, data, state);
		if (err != GN_ERR_NONE) return err;

		timeradd(&now, &timeout, &next);
		do {
			s = gn_sm_loop(1, state);  /* Timeout=100ms */
			gettimeofday(&now, NULL);
		} while (timercmp(&next, &now, >) && (s == GN_SM_MessageSent));
		if (s == GN_SM_WaitingForResponse || s == GN_SM_ResponseReceived) break;

		if (state->config.sm_retry) {
			dprintf("SM_Block Retry - %d\n", retry);
			sm_reset(state);
			sm_message_send(state->last_msg_size, state->last_msg_type, state->last_msg, state);
		} else {
			/* If we don't want retry, we should exit the loop */
			dprintf("SM_Block: exiting the retry loop\n");
			break;
		}
	}

	if (s == GN_SM_ResponseReceived)
		return sm_error_get(waitfor, state);

	timeout.tv_sec = t / 10;
	timeout.tv_usec = (t % 10) * 100000;
	timeradd(&now, &timeout, &next);
	do {
		s = gn_sm_loop(1, state);  /* Timeout=100ms */
		gettimeofday(&now, NULL);
	} while (timercmp(&next, &now, >) && (s != GN_SM_ResponseReceived));

	if (s == GN_SM_ResponseReceived)
		return sm_error_get(waitfor, state);

	sm_reset(state);

	return GN_ERR_TIMEOUT;
}

gn_error sm_block_timeout(int waitfor, int t, gn_data *data, struct gn_statemachine *state)
{
	int retry;
	gn_error err;

	for (retry = 0; retry < 3; retry++) {
		err = __sm_block_timeout(waitfor, t, data, state);
		if (err == GN_ERR_NONE || err != GN_ERR_TIMEOUT)
			return err;
		if (retry < 2) {
			sm_message_send(state->last_msg_size, state->last_msg_type, state->last_msg, state);
		}
	}
	return GN_ERR_TIMEOUT;
}

gn_error sm_block(int waitfor, gn_data *data, struct gn_statemachine *state)
{
	return sm_block_timeout(waitfor, 40, data, state);
}

/* This function is equal to sm_block_timeout except it does not retry the message */
gn_error sm_block_no_retry_timeout(int waitfor, int t, gn_data *data, struct gn_statemachine *state)
{
	return __sm_block_timeout(waitfor, t, data, state);
}

/* This function is equal to sm_block except it does not retry the message */
gn_error sm_block_no_retry(int waitfor, gn_data *data, struct gn_statemachine *state)
{
	return sm_block_no_retry_timeout(waitfor, 100, data, state);
}

/* Block waiting for an ack, don't wait for a reply message of any sort */
gn_error sm_block_ack(struct gn_statemachine *state)
{
	int retry;
	gn_state s;
	gn_error err;
	struct timeval now, next, timeout;

	s = state->current_state;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	gettimeofday(&now, NULL);
	for (retry = 0; retry < 2; retry++) {
		timeradd(&now, &timeout, &next);
		do {
			s = gn_sm_loop(1, state);  /* Timeout=100ms */
			gettimeofday(&now, NULL);
		} while (timercmp(&next, &now, >) && (s == GN_SM_MessageSent));

		if (s == GN_SM_WaitingForResponse || s == GN_SM_ResponseReceived)
			return GN_ERR_NONE;

		dprintf("sm_block_ack Retry - %d\n", retry);
		sm_reset(state);
		err = sm_message_send(state->last_msg_size, state->last_msg_type, state->last_msg, state);
		if (err != GN_ERR_NONE) return err;
	}

	sm_reset(state);

	return GN_ERR_TIMEOUT;
}

/* Just to do things neatly */
GNOKII_API gn_error gn_sm_functions(gn_operation op, gn_data *data, struct gn_statemachine *sm)
{
	if (!sm->driver.functions) {
		dprintf("Sorry, gnokii internal structures were not correctly initialized (op == %d)\n", op);
		return GN_ERR_INTERNALERROR;
	}
	return sm->driver.functions(op, data, sm);
}

/* Dumps a message */
void sm_message_dump(gn_log_func_t lfunc, int messagetype, unsigned char *message, int messagesize)
{
	int i;
	char buf[17];

	buf[16] = 0;

	lfunc("0x%02x / 0x%04x", messagetype, messagesize);

	for (i = 0; i < messagesize; i++) {
		if (i % 16 == 0) {
			if (i != 0) lfunc("| %s", buf);
			lfunc("\n");
			memset(buf, ' ', 16);
		}
		lfunc("%02x ", message[i]);
		if (isprint(message[i])) buf[i % 16] = message[i];
	}

	if (i != 0) lfunc("%*s| %s", i % 16 ? 3 * (16 - i % 16) : 0, "", buf);
	lfunc("\n");
}

/* Prints a warning message about unhandled frames */
void sm_unhandled_frame_dump(int messagetype, unsigned char *message, int messagesize, struct gn_statemachine *state)
{
	dump(_("UNHANDLED FRAME RECEIVED\nrequest: "));
	sm_message_dump(gn_elog_write, state->last_msg_type, state->last_msg, state->last_msg_size);

	dump(_("reply: "));
	sm_message_dump(gn_elog_write, messagetype, message, messagesize);

	dump(_("Please read Docs/Bugs and send a bug report!\n"));
}
