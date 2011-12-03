/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2000      Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000-2001 Chris Kemp
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002      Ladis Michl

  This file provides useful functions for all phones
  See README for more details on supported mobile phones.

  Various routines are called p(hones/)gen(eric)_...

*/

#ifndef _gnokii_phones_generic_h
#define _gnokii_phones_generic_h

#include "gnokii.h"

/* Generic Functions */
gn_error pgen_incoming_default(int messagetype, unsigned char *buffer, int length, struct gn_statemachine *state);
gn_error pgen_terminate(gn_data *data, struct gn_statemachine *state);

#endif
