/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2004 Hugh Hass, Pawel Kot

  This file provides functions specific to AT commands on SonyEricsson phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atsoer_h
#define _gnokii_atsoer_h

#include "gnokii-internal.h"

void at_sonyericsson_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);

#endif
