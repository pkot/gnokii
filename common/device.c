/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

*/

#include "misc.h"
#include "gsm-api.h"
#include "device.h"
#ifdef HAVE_IRDA
#  include "devices/unixirda.h"
#endif
#ifndef WIN32
#  include "devices/unixserial.h"
#  include "devices/tekram.h"
#  include "devices/tcp.h"
#else
#  include "devices/winserial.h"
#endif

/* The filedescriptor we use. */
static int device_portfd = -1;

/* The device type to use */
static gn_connection_type devicetype;

int device_getfd(struct gn_statemachine *state)
{
	return device_portfd;
}

int device_open(const char *file, int with_odd_parity, int with_async,
		int with_hw_handshake, gn_connection_type device_type,
		struct gn_statemachine *state)
{
	devicetype = device_type;

	dprintf("Serial device: opening device %s\n", file);

	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		device_portfd = serial_opendevice(file, with_odd_parity, with_async, with_hw_handshake, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		device_portfd = irda_open(state);
		break;
#endif
#ifndef WIN32
	case GN_CT_Tekram:
		device_portfd = tekram_open(file, state);
		break;
	case GN_CT_TCP:
		device_portfd = tcp_opendevice(file, with_async, state);
		break;
#endif
	default:
		break;
	}
	return (device_portfd >= 0);
}

void device_close(struct gn_statemachine *state)
{
	dprintf("Serial device: closing device\n");

	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_close(device_portfd, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		irda_close(device_portfd, state);
		break;
#endif
#ifndef WIN32
	case GN_CT_Tekram:
		tekram_close(device_portfd, state);
		break;
	case GN_CT_TCP:
		tcp_close(device_portfd, state);
		break;
#endif
	default:
		break;
	}
}

void device_reset(struct gn_statemachine *state)
{
	return;
}

void device_setdtrrts(int dtr, int rts, struct gn_statemachine *state)
{
	dprintf("Serial device: setting RTS to %s and DTR to %s\n", rts ? "high" : "low", dtr ? "high" : "low");

	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_setdtrrts(device_portfd, dtr, rts, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		break;
#endif
#ifndef WIN32
	case GN_CT_Tekram:
	case GN_CT_TCP:
		break;
#endif
	default:
		break;
	}
}

void device_changespeed(int speed, struct gn_statemachine *state)
{
	dprintf("Serial device: setting speed to %d\n", speed);

	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_changespeed(device_portfd, speed, state);
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		break;
#endif
#ifndef WIN32
	case GN_CT_Tekram:
		tekram_changespeed(device_portfd, speed, state);
	case GN_CT_TCP:
		break;
#endif
	default:
		break;
	}
}

size_t device_read(__ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return (serial_read(device_portfd, buf, nbytes, state));
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		return irda_read(device_portfd, buf, nbytes, state);
#endif
#ifndef WIN32
	case GN_CT_Tekram:
		return tekram_read(device_portfd, buf, nbytes, state);
	case GN_CT_TCP:
		return tcp_read(device_portfd, buf, nbytes, state);
#endif
	default:
		break;
	}
	return 0;
}

size_t device_write(const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return (serial_write(device_portfd, buf, n, state));
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		return irda_write(device_portfd, buf, n, state);
#endif
#ifndef WIN32
	case GN_CT_Tekram:
		return tekram_write(device_portfd, buf, n, state);
	case GN_CT_TCP:
		return tcp_write(device_portfd, buf, n, state);
#endif
	default:
		break;
	}
	return 0;
}

int device_select(struct timeval *timeout, struct gn_statemachine *state)
{
	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_select(device_portfd, timeout, state);
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		return irda_select(device_portfd, timeout, state);
#endif
#ifndef WIN32
	case GN_CT_Tekram:
		return tekram_select(device_portfd, timeout, state);
	case GN_CT_TCP:
		return tcp_select(device_portfd, timeout, state);
#endif
	default:
		break;
	}
	return -1;
}

gn_error device_nreceived(int *n, struct gn_statemachine *state)
{
	*n = -1;

	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_nreceived(device_portfd, n, state);
	default:
		return GN_ERR_NOTSUPPORTED;
	}
}

gn_error device_flush(struct gn_statemachine *state)
{
	switch (devicetype) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_flush(device_portfd, state);
	default:
		return GN_ERR_NOTSUPPORTED;
	}
}
