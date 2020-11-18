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

void* bluetooth_open(gn_config *cfg, int with_odd_parity, int with_async) { return NULL; }
void bluetooth_close(void *instance) { }
size_t bluetooth_write(void *instance, const __ptr_t bytes, size_t size) { return -1; }
size_t bluetooth_read(void *instance, __ptr_t bytes, size_t size) { return -1; }
int bluetooth_select(void *instance, struct timeval *timeout) { return -1; }

#endif /* HAVE_BLUETOOTH */
