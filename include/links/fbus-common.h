/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2002-2003 Pawe³ Kot
  Copyright (C) 2002      Ladis Michl
  Copyright (C) 2004      BORBELY Zoltan

  This file provides an API for accessing functions via fbus over irda.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_links_fbus_common_h
#define _gnokii_links_fbus_common_h

#include "gnokii.h"

/* Nokia mobile phone. */
#define FBUS_DEVICE_PHONE 0x00

/* Our PC. */
#define FBUS_DEVICE_PC 0x0c

/* States for receive code. */
enum fbus_rx_state {
	FBUS_RX_Sync,
	FBUS_RX_Discarding,
	FBUS_RX_GetDestination,
	FBUS_RX_GetSource,
	FBUS_RX_GetType,
	FBUS_RX_GetLength1,
	FBUS_RX_GetLength2,
	FBUS_RX_GetMessage,
	FBUS_RX_EchoSource,
	FBUS_RX_EchoType,
	FBUS_RX_EchoLength1,
	FBUS_RX_EchoLength2,
	FBUS_RX_EchoMessage
};

#endif /* _gnokii_links_fbus_common_h */
