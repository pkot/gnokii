/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

*/

#ifndef __links_fbus_phonet_h
#define __links_fbus_phonet_h

#include "fbus-common.h"

#define PHONET_MAX_FRAME_LENGTH    1010
#define PHONET_MAX_TRANSMIT_LENGTH 1010
#define PHONET_MAX_CONTENT_LENGTH  1000

/* This byte is at the beginning of all GSM Frames sent over PhoNet. */
#define FBUS_PHONET_FRAME_ID 0x14

GSM_Error PHONET_Initialise(GSM_Link *newlink, GSM_Statemachine *state);

typedef struct {
	int BufferCount;
	enum FBUS_RX_States state;
	int MessageSource;
	int MessageDestination;
	int MessageType;
	int MessageLength;
	char MessageBuffer[PHONET_MAX_FRAME_LENGTH];
} PHONET_IncomingMessage;


#endif   /* #ifndef __links_fbus_phonet_h */
