/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#include "gsm-common.h"
#include "gsm-statemachine.h"

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
		state->LastMsgSize = messagesize;
		state->LastMsgType = messagetype;
		state->LastMsg = message;
		state->CurrentState = MessageSent;

		/* FIXME - clear KeepAlive timer */
		return state->Link.SendMessage(messagesize, messagetype, message);
	}
	else return GE_NOTREADY;
}

GSM_State SM_Loop(GSM_Statemachine *state, int timeout)
{
	struct timeval loop_timeout;
	int i;

	loop_timeout.tv_sec = 0;
	loop_timeout.tv_usec = 100000;

	if (!state->Link.Loop) {
		dprintf("No Loop function. Aborting.\n");
		abort();
	}
	for (i = 0; i < timeout; i++) {
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
	GSM_Data emptydata;
	GSM_Data *data = &emptydata;
	GSM_Error res = GE_INTERNALERROR;
	int waitingfor = -1;

	GSM_DataClear(&emptydata);

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
			dprintf("Received message type %02x\n\r", messagetype);
			res = state->Phone.IncomingFunctions[c].Functions(messagetype, message, messagesize, data);
			temp = 0;
		}
		c++;
	}
	if (temp != 0) {
		dprintf("Unknown Frame Type %02x\n\r", messagetype);
		state->Phone.DefaultFunction(messagetype, message, messagesize);

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
}


/* This returns the error recorded from the phone function and indicates collection */
GSM_Error SM_GetError(GSM_Statemachine *state, unsigned char messagetype)
{
	int c, d;
	GSM_Error error = GE_NOTREADY;
	
	if (state->CurrentState==ResponseReceived) {
		for(c = 0; c < state->NumReceived; c++)
			if (state->WaitingFor[c] == messagetype) {
				error = state->ResponseError[c];
				for(d = c + 1 ;d < state->NumReceived; d++){
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


/* This function is for convinience only */
/* It is called after SM_SendMessage and blocks until a response is received */
GSM_Error SM_Block(GSM_Statemachine *state, GSM_Data *data, int waitfor) 
{
	int retry, timeout;
	GSM_State s;
	GSM_Error err;

	for (retry = 0; retry < 3; retry++) {
		timeout = 30;
		err = SM_WaitFor(state, data, waitfor);
		if (err != GE_NONE) return err; 

		do {            /* ~3secs timeout */
			s = SM_Loop(state, 1);  /* Timeout=100ms */
			timeout--;
		} while ((timeout > 0) && (s == WaitingForResponse));
		
		if (s == ResponseReceived) return SM_GetError(state, waitfor);

		dprintf("SM_Block Retry - %d\n\r", retry);
		SM_Reset(state);
		if (retry < 2) SM_SendMessage(state, state->LastMsgSize, state->LastMsgType, state->LastMsg);
	}

	return GE_TIMEOUT;
}

/* This function is equal to SM_Block except it does not retry the message */
GSM_Error SM_BlockNoRetry(GSM_Statemachine *state, GSM_Data *data, int waitfor)
{
	int retry, timeout;
	GSM_State s;
	GSM_Error err;

	for (retry = 0; retry < 3; retry++) {
		timeout = 30;
		err = SM_WaitFor(state, data, waitfor);
		if (err != GE_NONE) return err; 

		do {            /* ~3secs timeout */
			s = SM_Loop(state, 1);  /* Timeout=100ms */
			timeout--;
		} while ((timeout > 0) && (s == WaitingForResponse));
		
		if (s == ResponseReceived) return SM_GetError(state, waitfor);
	}

	return GE_TIMEOUT;
}

/* Just to do things neatly */
GSM_Error SM_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *sm)
{
	if (!sm->Phone.Functions) {
		dprintf("Sorry, phone has not yet been converted to new style. Phone.Functions == NULL!\n");
		return GE_INTERNALERROR;
	}
	return sm->Phone.Functions(op, data, sm);
}
