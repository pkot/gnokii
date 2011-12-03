/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002 Ladislav Michl <ladis@linux-mips.org>

  This file provides functions specific to AT commands on Bosch phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atbosch_h
#define _gnokii_atbosch_h

#include "gnokii-internal.h"

void at_bosch_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);

#endif
