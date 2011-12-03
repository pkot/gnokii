/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2006 Bastien Nocera <hadess@hadess.net>

  This file provides functions specific to AT commands on Motorola phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atmot_h_
#define _gnokii_atmot_h_

#include "gnokii.h"

void at_motorola_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);

#endif
