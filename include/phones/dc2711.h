/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002 Ladislav Michl <ladis@linux-mips.org>

  This file provides functions specific to Dancall phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_dc2711_h
#define _gnokii_dc2711_h

#include "gnokii-internal.h"

void dc2711_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);

#endif
