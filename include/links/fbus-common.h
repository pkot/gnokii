/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2002 Pawe³ Kot

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

*/

#ifndef __links_fbus_common_h
#define __links_fbus_common_h

#include "gsm-statemachine.h"

/* Nokia mobile phone. */
#define FBUS_DEVICE_PHONE 0x00

/* Our PC. */
#define FBUS_DEVICE_PC 0x0c

/* States for receive code. */

enum FBUS_RX_States {
	FBUS_RX_Sync,
	FBUS_RX_Discarding,
	FBUS_RX_GetDestination,
	FBUS_RX_GetSource,
	FBUS_RX_GetType,
	FBUS_RX_GetLength1,
	FBUS_RX_GetLength2,
	FBUS_RX_GetMessage
};

#endif /* __links_fbus_common_h */
