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

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.

  This file provides an API for accessing functions via fbus.
  See README for more details on supported mobile phones.

  The various routines are called FB3110_(whatever).

*/

#ifndef __links_fbus_3110_h
#define __links_fbus_3110_h

#include <time.h>
#include "gsm-statemachine.h"
#include "config.h"
#include "compat.h"

#ifdef WIN32
#  include <sys/types.h>
#endif

#define FB3110_MAX_FRAME_LENGTH 256
#define FB3110_MAX_MESSAGE_TYPES 128
#define FB3110_MAX_TRANSMIT_LENGTH 256
#define FB3110_MAX_CONTENT_LENGTH 120

/* This byte is at the beginning of all GSM Frames sent over FBUS to Nokia
   phones.  This may have to become a phone dependant parameter... */
#define FB3110_FRAME_ID 0x01


/* States for receive code. */

enum FB3110_RX_States {
	FB3110_RX_Sync,
	FB3110_RX_Discarding,
	FB3110_RX_GetLength,
	FB3110_RX_GetMessage
};


typedef struct{
	int Checksum;
	int BufferCount;
	enum FB3110_RX_States State;
	int FrameType;
	int FrameLength;
	char Buffer[FB3110_MAX_FRAME_LENGTH];
} FB3110_IncomingFrame;

typedef struct {
	u16 message_length;
	u8 message_type;
	u8 *buffer;
} FB3110_OutgoingMessage;


typedef struct{
	FB3110_IncomingFrame i;
	u8 RequestSequenceNumber;
} FB3110_Link;

gn_error FB3110_Initialise(GSM_Link *newlink, GSM_Statemachine *state);

#endif   /* #ifndef __links_fbus_3110_h */
