/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2005       Alex Kanavin
  Copyright (C) 2006       Pawel Kot

*/

#ifndef _gnokii_dku2libusb_h
#define _gnokii_dku2libusb_h

#include "compat.h"
#include "misc.h"
#include "gnokii.h"

int fbusdku2usb_open(struct gn_statemachine *state);
int fbusdku2usb_close(struct gn_statemachine *state);
int fbusdku2usb_write(const __ptr_t bytes, int size, struct gn_statemachine *state);
int fbusdku2usb_read(__ptr_t bytes, int size, struct gn_statemachine *state);
int fbusdku2usb_select(struct timeval *timeout, struct gn_statemachine *state);

#endif /* _gnokii_dku2libusb_h */
