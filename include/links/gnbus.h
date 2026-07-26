/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2004 BORBELY Zoltan

  This file provides an API for accessing functions via gnbus.
  See README for more details on supported mobile phones.

  The various routines are called GNBUS_(whatever).

*/

#ifndef _gnokii_links_gnbus_h
#define _gnokii_links_gnbus_h

#include "compat.h"

#include "gnokii.h"

#define GNBUS_MAGIC_BYTE	0x5a

enum gnbus_rx_state {
	GNBUS_RX_Discarding,
	GNBUS_RX_Sync,
	GNBUS_RX_GetSequence,
	GNBUS_RX_GetLength1,
	GNBUS_RX_GetLength2,
	GNBUS_RX_GetType,
	GNBUS_RX_GetDummy,
	GNBUS_RX_GetMessage
};

typedef struct {
	enum gnbus_rx_state state;
	int buffer_count;
	struct timeval time_now;
	struct timeval time_last;
	unsigned char sequence;
	int message_type;
	int message_length;
	unsigned char checksum[2];
	int checksum_idx;
	unsigned char *message_buffer;
} gnbus_incoming_message;


typedef struct{
	gnbus_incoming_message i;
} gnbus_link;

#define GNBUSINST(s) (*((gnbus_link **)(&(s)->link.link_instance)))

gn_error gnbus_initialise(struct gn_statemachine *state);

#endif   /* #ifndef _gnokii_links_gnbus_h */
