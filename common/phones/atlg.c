/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2008 Pawel Kot

  This file provides functions specific to at commands on LG phones.
  See README for more details on supported mobile phones.

*/

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/atlg.h"

void at_lg_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	AT_DRVINST(state)->lac_swapped = 1;
}
