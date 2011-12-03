/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001 Pavel Machek, Chris Kemp
  Copyright (C) 2002 Pawel Kot, Ladis Michl, Manfred Jonsson

  This file provides functions common to all parts in the link layer.
  See README for more details on supported mobile phones.


*/

#ifndef _gnokii_links_utils_h
#define _gnokii_links_utils_h

#include "gnokii.h"

/* Needs to be a multiplication of 64 */
#define BUFFER_SIZE	256

gn_error link_terminate(struct gn_statemachine *state);
void at_dprintf(char *prefix, char *buf, int len);

#endif
