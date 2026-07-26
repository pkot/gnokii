/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000      Chris Kemp
  Copyright (C) 2001-2011 Pawel Kot
  Copyright (C) 2002      BORBELY Zoltan, Ladis Michl

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_links_fbus_phonet_h
#define _gnokii_links_fbus_phonet_h

#include "fbus-common.h"

#define PHONET_FRAME_MAX_LENGTH    1010
#define PHONET_TRANSMIT_MAX_LENGTH 1010
#define PHONET_CONTENT_MAX_LENGTH  1000

/* This byte is at the beginning of all GSM Frames sent over PhoNet. */
#define FBUS_PHONET_FRAME_ID 0x14

/* This byte is at the beginning of all GSM Frames sent over Bluetooth to Nokia 6310
   family phones. */
#define FBUS_PHONET_BLUETOOTH_FRAME_ID  0x19

#define FBUS_PHONET_DKU2_FRAME_ID  0x1b
#define FBUS_PHONET_DKU2_DEVICE_PC  0x0c

/* Our PC in the Nokia 6310 family over Bluetooth. */
#define FBUS_PHONET_BLUETOOTH_DEVICE_PC 0x10

gn_error phonet_initialise(struct gn_statemachine *state);

typedef struct {
	int buffer_count;
	enum fbus_rx_state state;
	int message_source;
	int message_destination;
	int message_type;
	int message_length;
	char *message_buffer;
	int message_buffer_size;
	int message_corrupted;
} phonet_incoming_message;

#endif   /* #ifndef _gnokii_links_fbus_phonet_h */
