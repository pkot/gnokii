/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2000      Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001      Tamas Bondar
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2003      Osma Suominen

  This file provides functions specific to the Nokia 3110 series.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_phones_nk3110_h
#define _gnokii_phones_nk3110_h

#include "misc.h"
#include "gnokii.h"

/* Phone Memory Sizes */
#define P3110_MEMORY_SIZE_SM 20
#define P3110_MEMORY_SIZE_ME 0

/* Number of times to try resending SMS (empirical) */
#define P3110_SMS_SEND_RETRY_COUNT 4

typedef struct {
	bool sim_available;
	unsigned char user_data[GN_SMS_MAX_LENGTH];
	int user_data_count;
	void (*rlp_rx_callback)(gn_rlp_f96_frame *frame);
} nk3110_driver_instance;

#endif  /* #ifndef _gnokii_phones_nk3110_h */
