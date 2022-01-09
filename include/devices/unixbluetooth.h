/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>

*/

#ifndef _gnokii_unix_bluetooth_h
#define _gnokii_unix_bluetooth_h

#include "compat.h"
#include "gnokii.h"

int bluetooth_open(const char *addr, uint8_t channel, struct gn_statemachine *state);
int bluetooth_close(int fd, struct gn_statemachine *state);
int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state);
int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state);
int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state);

#endif /* _gnokii_unix_bluetooth_h */
