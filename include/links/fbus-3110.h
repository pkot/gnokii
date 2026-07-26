/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2002      BORBELY Zoltan, Ladis Michl
  Copyright (C) 2003      Osma Suominen

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called fb3110_(whatever).

*/

#ifndef _gnokii_links_fbus_3110_h
#define _gnokii_links_fbus_3110_h

#include "compat.h"
#include "gnokii.h"

#define FB3110_FRAME_MAX_LENGTH		256
#define FB3110_MESSAGE_MAX_TYPES	128
#define FB3110_TRANSMIT_MAX_LENGTH	256
#define FB3110_CONTENT_MAX_LENGTH	120

/* Frame types (first byte of FBUS v1 frame) */
#define FB3110_FRAME_TYPE_OUT_CMD 0x01
#define FB3110_FRAME_TYPE_OUT_RLP 0x02
#define FB3110_FRAME_TYPE_IN_RLP  0x03
#define FB3110_FRAME_TYPE_IN_CMD  0x04

/* States for receive code. */

enum fb3110_rx_states {
	FB3110_RX_Sync,
	FB3110_RX_Discarding,
	FB3110_RX_GetLength,
	FB3110_RX_GetMessage
};

typedef struct{
	int checksum;
	int buffer_count;
	enum fb3110_rx_states state;
	int frame_type;
	int frame_len;
	char buffer[FB3110_FRAME_MAX_LENGTH];
} fb3110_incoming_frame;

typedef struct {
	uint16_t message_length;
	uint8_t message_type;
	uint8_t *buffer;
} fb3110_outgoing_message;


typedef struct{
	fb3110_incoming_frame i;
	uint8_t request_sequence_number;
	uint8_t last_incoming_sequence_number;
} fb3110_link;

gn_error fb3110_initialise(struct gn_statemachine *state);

#endif   /* #ifndef _gnokii_links_fbus_3110_h */
