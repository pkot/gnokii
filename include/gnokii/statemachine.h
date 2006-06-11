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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2002-2003 Pawel Kot, BORBELY Zoltan

  Header file for the statemachine.

*/

#ifndef _gnokii_statemachine_h
#define _gnokii_statemachine_h

#include <gnokii/error.h>
#include <gnokii/data.h>

/* Small structure used in gn_driver */
/* Messagetype is passed to the function in case it is a 'generic' one */
typedef struct {
	unsigned char message_type;
	gn_error (*functions)(int messagetype, unsigned char *buffer, int length,
			      gn_data *data, struct gn_statemachine *state);
} gn_incoming_function_type;

/* This structure contains the 'callups' needed by the statemachine */
/* to deal with messages from the phone and other information */
typedef struct {
	/* These make up a list of functions, one for each message type and NULL terminated */
	gn_incoming_function_type *incoming_functions;
	gn_error (*default_function)(int messagetype, unsigned char *buffer, int length, struct gn_statemachine *state);
	gn_phone phone;
	gn_error (*functions)(gn_operation op, gn_data *data, struct gn_statemachine *state);
	void *driver_instance;
} gn_driver;

/* How many message types we can wait for at one moment */
#define GN_SM_WAITINGFOR_MAX_NUMBER 3

/* The states the statemachine can take */
typedef enum {
	GN_SM_Startup,            /* Not yet initialised */
	GN_SM_Initialised,        /* Ready! */
	GN_SM_MessageSent,        /* A command has been sent to the link(phone) */
	GN_SM_WaitingForResponse, /* We are waiting for a response from the link(phone) */
	GN_SM_ResponseReceived    /* A response has been received - waiting for the phone layer to collect it */
} gn_state;

/* All properties of the state machine */
struct gn_statemachine {
	gn_state current_state;
	gn_config config;
	gn_device device;
	gn_link link;
	gn_driver driver;
	char *lockfile;
	
	/* Store last message for resend purposes */
	unsigned char last_msg_type;
	unsigned int last_msg_size;
	void *last_msg;

	/* The responses we are waiting for */
	unsigned char waiting_for_number;
	unsigned char received_number;
	unsigned char waiting_for[GN_SM_WAITINGFOR_MAX_NUMBER];
	gn_error response_error[GN_SM_WAITINGFOR_MAX_NUMBER];
	/* Data structure to be filled in with the response */
	gn_data *data[GN_SM_WAITINGFOR_MAX_NUMBER];

	/* libfunctions internal data structure */
	gn_error lasterror;
	gn_data sm_data;
	union {
		gn_phonebook_entry pb_entry;
	} u;
};

GNOKII_API gn_state gn_sm_loop(int timeout, struct gn_statemachine *state);

/* General way to call any driver function */
GNOKII_API gn_error gn_sm_functions(gn_operation op, gn_data *data, struct gn_statemachine *sm);

#endif	/* _gnokii_statemachine_h */
