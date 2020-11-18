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
#include "gnokii.h"

void* fbusdku2usb_open(gn_config *cfg, int with_odd_parity, int with_async);
void fbusdku2usb_close(void *instance);
size_t fbusdku2usb_write(void *instance, const __ptr_t bytes, size_t size);
size_t fbusdku2usb_read(void *instance, __ptr_t bytes, size_t size);
int fbusdku2usb_select(void *instance, struct timeval *timeout);

#endif /* _gnokii_dku2libusb_h */
