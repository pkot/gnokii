/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002 Ladislav Michl <ladis@linux-mips.org>
  Copyright (C) 2007 Ingmar Steen <iksteen@gmail.com>

  This file provides functions specific to AT commands on Samsung phones.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_atsam_h
#define _gnokii_atsam_h

#include "gnokii-internal.h"

void at_samsung_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state);
size_t decode_ucs2_as_utf8(char *dst, const char *src, int len);

#endif
