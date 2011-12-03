/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2009 Daniele Forsi

  This file provides functions specific to AT commands on Huawei phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_huawei_h
#define _gnokii_huawei_h

#include "gnokii-internal.h"

void at_huawei_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);

#endif
