/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#include "device.h"

/*
 * Structure to store the filedescriptor we use.
 */

int device_portfd = -1;

/* The device type to use */

GSM_ConnectionType devicetype;

int device_getfd(void)
{
	return device_portfd;
}

int device_open(const char *file, int with_odd_parity, int with_async, int with_hw_handshake, GSM_ConnectionType device_type)
{
	devicetype = device_type;

	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		device_portfd = serial_opendevice(file, with_odd_parity, with_async, with_hw_handshake);
		break;
#ifndef WIN32
	case GCT_Tekram:
		device_portfd = tekram_open(file);
		break;
	case GCT_Irda:
		device_portfd = irda_open();
		break;
#endif
	default:
		break;
	}
	return (device_portfd >= 0);
}

void device_close(void)
{
	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		serial_close(device_portfd);
		break;
#ifndef WIN32
	case GCT_Tekram:
		tekram_close(device_portfd);
		break;
	case GCT_Irda:
		irda_close(device_portfd);
		break;
#endif
	default:
		break;
	}
}

void device_reset(void)
{
	return;
}

void device_setdtrrts(int dtr, int rts)
{
	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		serial_setdtrrts(device_portfd, dtr, rts);
		break;
#ifndef WIN32
	case GCT_Tekram:
		break;
	case GCT_Irda:
		break;
#endif
	default:
		break;
	}
}

void device_changespeed(int speed)
{
	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		serial_changespeed(device_portfd, speed);
		break;
#ifndef WIN32
	case GCT_Tekram:
		tekram_changespeed(device_portfd, speed);
		break;
	case GCT_Irda:
		break;
#endif
	default:
		break;
	}
}

size_t device_read(__ptr_t buf, size_t nbytes)
{
	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		return (serial_read(device_portfd, buf, nbytes));
		break;
#ifndef WIN32
	case GCT_Tekram:
		return (tekram_read(device_portfd, buf, nbytes));
		break;
	case GCT_Irda:
		return irda_read(device_portfd, buf, nbytes);
		break;
#endif
	default:
		break;
	}
	return 0;
}

size_t device_write(const __ptr_t buf, size_t n)
{
	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		return (serial_write(device_portfd, buf, n));
		break;
#ifndef WIN32
	case GCT_Tekram:
		return (tekram_write(device_portfd, buf, n));
		break;
	case GCT_Irda:
		return irda_write(device_portfd, buf, n);
		break;
#endif
	default:
		break;
	}
	return 0;
}

int device_select(struct timeval *timeout)
{
	switch (devicetype) {
	case GCT_Serial:
	case GCT_Infrared:
		return serial_select(device_portfd, timeout);
		break;
#ifndef WIN32
	case GCT_Tekram:
		return tekram_select(device_portfd, timeout);
		break;
	case GCT_Irda:
		return irda_select(device_portfd, timeout);
		break;
#endif
	default:
		break;
	}
	return -1;
}
