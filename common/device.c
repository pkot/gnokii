/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log$
  Revision 1.10  2001-08-20 23:27:37  pkot
  Add hardware shakehand to the link layer (Manfred Jonsson)

  Revision 1.9  2001/06/28 00:28:45  pkot
  Small docs updates (Pawel Kot)

  Revision 1.8  2001/02/21 19:56:55  chris
  More fiddling with the directory layout

  Revision 1.7  2001/02/09 18:12:53  chris
  Marcel's tekram support

  Revision 1.6  2001/02/03 23:56:12  chris
  Start of work on irda support (now we just need fbus-irda.c!)
  Proper unicode support in 7110 code (from pkot)


*/

#ifndef WIN32

#include "devices/unixserial.h"
#include "devices/unixirda.h"
#include "devices/tekram.h"
#include "device.h"

/*
 * Structure to store the filedescriptor we use.
 *
 */

int device_portfd = -1;

/* The device type to use */

GSM_ConnectionType devicetype;

int device_getfd(void) {

  return device_portfd;
}

int device_open(__const char *__file, int __with_odd_parity, int __with_async, int __with_hw_handshake, GSM_ConnectionType device_type) {

  devicetype=device_type;

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    device_portfd = serial_opendevice(__file, __with_odd_parity, __with_async, __with_hw_handshake);
    break;
  case GCT_Tekram:
    device_portfd = tekram_open(__file);
    break;
  case GCT_Irda:
    device_portfd = irda_open();
    break;
  default:
    break;
  }
  return (device_portfd >= 0);
}

void device_close(void) {

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    serial_close(device_portfd);
    break;
  case GCT_Tekram:
    tekram_close(device_portfd);
    break;
  case GCT_Irda:
    irda_close(device_portfd);
    break;
  default:
    break;
  }
}

void device_reset(void) {
}

void device_setdtrrts(int __dtr, int __rts) {

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    serial_setdtrrts(device_portfd, __dtr, __rts);
    break;
  case GCT_Tekram:
    break;
  case GCT_Irda:
    break;
  default:
    break;
  }
}

void device_changespeed(int __speed) {

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    serial_changespeed(device_portfd, __speed);
    break;
  case GCT_Tekram:
    tekram_changespeed(device_portfd, __speed);
    break;
  case GCT_Irda:
    break;
  default:
    break;
  }
}

size_t device_read(__ptr_t __buf, size_t __nbytes) {

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    return (serial_read(device_portfd, __buf, __nbytes));
    break;
  case GCT_Tekram:
    return (tekram_read(device_portfd, __buf, __nbytes));
    break;
  case GCT_Irda:
    return irda_read(device_portfd, __buf, __nbytes);
    break;
  default:
    break;
  }
  return 0;
}

size_t device_write(__const __ptr_t __buf, size_t __n) {

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    return (serial_write(device_portfd, __buf, __n));
    break;
  case GCT_Tekram:
    return (tekram_write(device_portfd, __buf, __n));
    break;
  case GCT_Irda:
    return irda_write(device_portfd, __buf, __n);
    break;
  default:
    break;
  }
  return 0;
}

int device_select(struct timeval *timeout) {

  switch (devicetype) {
  case GCT_Serial:
  case GCT_Infrared:
    return serial_select(device_portfd, timeout);
    break;
  case GCT_Tekram:
    return tekram_select(device_portfd, timeout);
    break;
  case GCT_Irda:
    return irda_select(device_portfd, timeout);
    break;
  default:
    break;
  }
  return -1;
}

#endif /* WIN32 */

