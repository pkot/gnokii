/*

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

*/

#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "misc.h"

GSM_Error SM_Initialise(GSM_Statemachine *state)
{
	state->CurrentState = Initialised;
	state->NumWaitingFor = 0;
	state->NumReceived = 0;

	return GE_NONE;
}

GSM_Error SM_SendMessage(GSM_Statemachine *state, u16 messagesize, u8 messagetype, void *message)
{
	if (state->CurrentState != Startup) {
#ifdef	DEBUG
	dump("Message sent: ");
	SM_DumpMessage(messagetype, message, messagesize);
#endif
		state->LastMsgSize = messagesize;
		state->LastMsgType = messagetype;
		state->LastMsg = message;
		state->CurrentState = MessageSent;

		/* FIXME - clear KeepAlive timer */
		return state->Link.SendMessage(messagesize, messagetype, message);
	}
	else return GE_NOTREADY;
}

API GSM_State SM_Loop(GSM_Statemachine *state, int timeout)
{
	struct timeval loop_timeout;
	int i;

	if (!state->Link.Loop) {
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

		state->Link.Loop(&loop_timeout);
	}

	/* FIXME - add calling a KeepAlive function here */
	return state->CurrentState;
}

void SM_Reset(GSM_Statemachine *state)
{
	/* Don't reset to initialised if we aren't! */
	if (state->CurrentState != Startup) {
		state->CurrentState = Initialised;
		state->NumWaitingFor = 0;
		state->NumReceived = 0;
	}
}

void SM_IncomingFunction(GSM_Statemachine *state, u8 messagetype, void *message, u16 messagesize)
{
	int c;
	int temp = 1;
	GSM_Data *data, *edata;
	GSM_Error res = GE_INTERNALERROR;
	int waitingfor = -1;

#ifdef	DEBUG
	dump("Message received: ");
	SM_DumpMessage(messagetype, message, messagesize);
#endif
	edata = calloc(1, sizeof(GSM_Data));
	data = edata;

	/* See if we need to pass the function the data struct */
	if (state->CurrentState == WaitingForResponse)
		for (c = 0; c < state->NumWaitingFor; c++)
			if (state->WaitingFor[c] == messagetype) {
				data = state->Data[c];
				waitingfor = c;
			}

	/* Pass up the message to the correct phone function, with data if necessary */
	c = 0;
	while (state->Phone.IncomingFunctions[c].Functions) {
		if (state->Phone.IncomingFunctions[c].MessageType == messagetype) {
			dprintf("Received message type %02x\n", messagetype);
			res = state->Phone.IncomingFunctions[c].Functions(messagetype, message, messagesize, data, state);
			temp = 0;
			break;
		}
		c++;
	}
	if (res == GE_UNSOLICITED) {
		dprintf("Unsolicited frame, skipping...\n");
		free(edata);
		return;
	} else if (res == GE_UNHANDLEDFRAME)
		SM_DumpUnhandledFrame(state, messagetype, message, messagesize);
	if (temp != 0) {
		dprintf("Unknown Frame Type %02x\n", messagetype);
		state->Phone.DefaultFunction(messagetype, message, messagesize, state);
		free(edata);
		return;
	}

	if (state->CurrentState == WaitingForResponse) {
		/* Check if we were waiting for a response and we received it */
		if (waitingfor != -1) {
			state->ResponseError[waitingfor] = res;
			state->NumReceived++;
		}

		/* Check if all waitingfors have been received */
		if (state->NumReceived == state->NumWaitingFor) {
			state->CurrentState = ResponseReceived;
		}
	}
	free(edata);
}

/* This returns the error recorded from the phone function and indicates collection */
GSM_Error SM_GetError(GSM_Statemachine *state, unsigned char messagetype)
{
	int c, d;
	GSM_Error error = GE_NOTREADY;

	if (state->CurrentState == ResponseReceived) {
		for (c = 0; c < state->NumReceived; c++)
			if (state->WaitingFor[c] == messagetype) {
				error = state->ResponseError[c];
				for (d = c + 1 ; d < state->NumReceived; d++) {
					state->ResponseError[d-1] = state->ResponseError[d];
					state->WaitingFor[d-1] = state->WaitingFor[d];
					state->Data[d-1] = state->Data[d];
				}
				state->NumReceived--;
				state->NumWaitingFor--;
				c--; /* For neatness continue in the correct place */
			}
		if (state->NumReceived == 0) {
			state->NumWaitingFor = 0;
			state->CurrentState = Initialised;
		}
	}

	return error;
}



/* Indicate that the phone code is waiting for an response */
/* This does not actually wait! */
GSM_Error SM_WaitFor(GSM_Statemachine *state, GSM_Data *data, unsigned char messagetype)
{
	/* If we've received a response, we have to call SM_GetError first */
	if ((state->CurrentState == Startup) || (state->CurrentState == ResponseReceived))
		return GE_NOTREADY;

	if (state->NumWaitingFor == SM_MAXWAITINGFOR) return GE_NOTREADY;
	state->WaitingFor[state->NumWaitingFor] = messagetype;
	state->Data[state->NumWaitingFor] = data;
	state->ResponseError[state->NumWaitingFor] = GE_BUSY;
	state->NumWaitingFor++;
	state->CurrentState = WaitingForResponse;

	return GE_NONE;
}


/* This function is for convinience only
   It is called after SM_SendMessage and blocks until a response is received

   t is in tenths of second
*/
static GSM_Error __SM_BlockTimeout(GSM_Statemachine *state, GSM_Data *data, int waitfor, int t, int noretry)
{
	int retry, timeout;
	GSM_State s;
	GSM_Error err;

	for (retry = 0; retry < 3; retry++) {
		timeout = t;
		err = SM_WaitFor(state, data, waitfor);
		if (err != GE_NONE) return err;

		do {            /* ~3secs timeout */
			s = SM_Loop(state, 1);  /* Timeout=100ms */
			timeout--;
		} while ((timeout > 0) && (s == WaitingForResponse));

		if (s == ResponseReceived) return SM_GetError(state, waitfor);

		dprintf("SM_Block Retry - %d\n", retry);
		SM_Reset(state);
		if ((retry < 2) && (!noretry)) 
			SM_SendMessage(state, state->LastMsgSize, state->LastMsgType, state->LastMsg);
	}

	return GE_TIMEOUT;
}

GSM_Error SM_BlockTimeout(GSM_Statemachine *state, GSM_Data *data, int waitfor, int t)
{
	return __SM_BlockTimeout(state, data, waitfor, t, 0);
}

GSM_Error SM_Block(GSM_Statemachine *state, GSM_Data *data, int waitfor)
{
	return __SM_BlockTimeout(state, data, waitfor, 30, 0);
}

/* This function is equal to SM_Block except it does not retry the message */
GSM_Error SM_BlockNoRetryTimeout(GSM_Statemachine *state, GSM_Data *data, int waitfor, int t)
{
	return __SM_BlockTimeout(state, data, waitfor, t, 1);
}

GSM_Error SM_BlockNoRetry(GSM_Statemachine *state, GSM_Data *data, int waitfor)
{
	return __SM_BlockTimeout(state, data, waitfor, 100, 1);
}

/* Just to do things neatly */
API GSM_Error SM_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *sm)
{
	if (!sm->Phone.Functions) {
		dprintf("Sorry, phone has not yet been converted to new style. Phone.Functions == NULL!\n");
		return GE_INTERNALERROR;
	}
	return sm->Phone.Functions(op, data, sm);
}

/* Dumps a message */
void SM_DumpMessage(int messagetype, unsigned char *message, int messagesize)
{
	int i;
	char buf[17];

	buf[16] = 0;

	dump("0x%02x / 0x%04x", messagetype, messagesize);

	for (i = 0; i < messagesize; i++) {
		if (i % 16 == 0) {
			if (i != 0) dump("| %s", buf);
			dump("\n");
			memset(buf, ' ', 16);
		}
		dump("%02x ", message[i]);
		if (isprint(message[i])) buf[i % 16] = message[i];
	}

	if (i % 16) dump("%*s| %s", 3 * (16 - i % 16), "", buf);
	dump("\n");
}

/* Prints a warning message about unhandled frames */
void SM_DumpUnhandledFrame(GSM_Statemachine *state, int messagetype, unsigned char *message, int messagesize)
{
	dump(_("UNHANDLED FRAME RECEIVED\nrequest: "));
	SM_DumpMessage(state->LastMsgType, state->LastMsg, state->LastMsgSize);

	dump(_("reply: "));
	SM_DumpMessage(messagetype, message, messagesize);

	dump(_("Please read Docs/Reporting-HOWTO and send a bug report!\n"));
}
