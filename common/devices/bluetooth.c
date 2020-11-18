/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2003       BORBELY Zoltan
  Copyright (C) 2004       Pawel Kot, Phil Ashby

  Fake definitions for the bluetooth handling functions.

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "devices/bluetooth.h"

#ifndef HAVE_BLUETOOTH

int bluetooth_open(const char *addr, uint8_t channel, struct gn_statemachine *state) { return -1; }
int bluetooth_close(int fd, struct gn_statemachine *state) { return -1; }
int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state) { return -1; }
int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state) { return -1; }
int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state) { return -1; }

#endif /* HAVE_BLUETOOTH */
