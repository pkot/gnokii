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


int device_getfd(struct gn_statemachine *state)
{
	return state->device.fd;
}

int device_open(const char *file, int with_odd_parity, int with_async,
		int with_hw_handshake, gn_connection_type device_type,
		struct gn_statemachine *state)
{
	state->device.type = device_type;

	dprintf("Serial device: opening device %s\n", file);

	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		state->device.fd = serial_opendevice(file, with_odd_parity, with_async, with_hw_handshake, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		state->device.fd = irda_open(state);
		break;
#endif
	case GN_CT_Tekram:
		state->device.fd = tekram_open(file, state);
		break;
#ifndef WIN32
	case GN_CT_TCP:
		state->device.fd = tcp_opendevice(file, with_async, state);
		break;
#endif
	default:
		state->device.fd = -1;
		break;
	}
	return (state->device.fd >= 0);
}

void device_close(struct gn_statemachine *state)
{
	dprintf("Serial device: closing device\n");

	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_close(state->device.fd, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		irda_close(state->device.fd, state);
		break;
#endif
	case GN_CT_Tekram:
		tekram_close(state->device.fd, state);
		break;
#ifndef WIN32
	case GN_CT_TCP:
		tcp_close(state->device.fd, state);
		break;
#endif
	default:
		break;
	}

	if (state->device.device_instance) {
		free(state->device.device_instance);
		state->device.device_instance = NULL;
	}
}

void device_reset(struct gn_statemachine *state)
{
	return;
}

void device_setdtrrts(int dtr, int rts, struct gn_statemachine *state)
{
	dprintf("Serial device: setting RTS to %s and DTR to %s\n", rts ? "high" : "low", dtr ? "high" : "low");

	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_setdtrrts(state->device.fd, dtr, rts, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		break;
#endif
	case GN_CT_Tekram:
		break;
#ifndef WIN32
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

	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_changespeed(state->device.fd, speed, state);
		break;
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		break;
#endif
	case GN_CT_Tekram:
		tekram_changespeed(state->device.fd, speed, state);
		break;
#ifndef WIN32
	case GN_CT_TCP:
		break;
#endif
	default:
		break;
	}
}

size_t device_read(__ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return (serial_read(state->device.fd, buf, nbytes, state));
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		return irda_read(state->device.fd, buf, nbytes, state);
#endif
	case GN_CT_Tekram:
		return tekram_read(state->device.fd, buf, nbytes, state);
#ifndef WIN32
	case GN_CT_TCP:
		return tcp_read(state->device.fd, buf, nbytes, state);
#endif
	default:
		break;
	}
	return 0;
}

size_t device_write(const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return (serial_write(state->device.fd, buf, n, state));
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		return irda_write(state->device.fd, buf, n, state);
#endif
	case GN_CT_Tekram:
		return tekram_write(state->device.fd, buf, n, state);
#ifndef WIN32
	case GN_CT_TCP:
		return tcp_write(state->device.fd, buf, n, state);
#endif
	default:
		break;
	}
	return 0;
}

int device_select(struct timeval *timeout, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_select(state->device.fd, timeout, state);
#ifdef HAVE_IRDA
	case GN_CT_Irda:
		return irda_select(state->device.fd, timeout, state);
#endif
	case GN_CT_Tekram:
		return tekram_select(state->device.fd, timeout, state);
#ifndef WIN32
	case GN_CT_TCP:
		return tcp_select(state->device.fd, timeout, state);
#endif
	default:
		break;
	}
	return -1;
}

gn_error device_nreceived(int *n, struct gn_statemachine *state)
{
	*n = -1;

	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_nreceived(state->device.fd, n, state);
	default:
		return GN_ERR_NOTSUPPORTED;
	}
}

gn_error device_flush(struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_flush(state->device.fd, state);
	default:
		return GN_ERR_NOTSUPPORTED;
	}
}
