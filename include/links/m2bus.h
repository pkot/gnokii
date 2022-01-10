/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000 Chris Kemp
  Copyright (C) 2002 BORBELY Zoltan

  This file provides an API for accessing functions via m2bus.
  See README for more details on supported mobile phones.

  The various routines are called M2BUS_(whatever).

*/

#ifndef _gnokii_links_m2bus_h
#define _gnokii_links_m2bus_h

#include "compat.h"

#include "gnokii.h"

/* This byte is at the beginning of all GSM Frames sent over M2BUS to Nokia
   phones.  This may have to become a phone dependent parameter... */
#define M2BUS_FRAME_ID 0x1f

/* This byte is at the beginning of all GSM Frames sent over IR to Nokia phones. */
#define M2BUS_IR_FRAME_ID 0x14

/* Nokia mobile phone. */
#define M2BUS_DEVICE_PHONE 0x00

/* Our PC. */
#define M2BUS_DEVICE_PC 0x1d


enum m2bus_rx_state {
    	M2BUS_RX_Sync,
	M2BUS_RX_Discarding,
	M2BUS_RX_GetDestination,
	M2BUS_RX_GetSource,
	M2BUS_RX_GetType,
	M2BUS_RX_GetLength1,
	M2BUS_RX_GetLength2,
	M2BUS_RX_GetMessage
};

typedef struct {
	enum m2bus_rx_state state;
	int buffer_count;
	struct timeval time_now;
	struct timeval time_last;
	int message_source;
	int message_destination;
	int message_type;
	int message_length;
	unsigned char checksum;
	unsigned char *message_buffer;
} m2bus_incoming_message;


typedef struct{
	m2bus_incoming_message i;
	uint8_t request_sequence_number;
} m2bus_link;

#define M2BUSINST(s) (*((m2bus_link **)(&(s)->link.link_instance)))

gn_error m2bus_initialise(struct gn_statemachine *state);

#endif   /* #ifndef _gnokii_links_m2bus_h */
