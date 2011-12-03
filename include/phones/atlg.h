/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2008 Pawel Kot

  This file provides functions specific to AT commands on LG phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atlg_h_
#define _gnokii_atlg_h_

#include "gnokii.h"

void at_lg_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);

#endif
