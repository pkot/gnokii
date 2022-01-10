/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Jan�k ml.
  Copyright (C) 2001      Chris Kemp
  Copyright (C) 2001-2011 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan
  Copyright (C) 2002      Pavel Machek, Marcin Wiacek

*/

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "gnokii-internal.h"
#include "device.h"
#include "devices/irda.h"
#include "devices/unixbluetooth.h"
#include "devices/tcp.h"
#include "devices/serial.h"
#include "devices/tekram.h"
#include "devices/dku2libusb.h"
#include "devices/socketphonet.h"

GNOKII_API int device_getfd(struct gn_statemachine *state)
{
	return state->device.fd;
}

int device_open(const char *file, int with_odd_parity, int with_async,
		int with_hw_handshake, gn_connection_type device_type,
		struct gn_statemachine *state)
{
	state->device.type = device_type;
	state->device.device_instance = NULL;

	dprintf("device: opening device %s\n", (device_type == GN_CT_DKU2LIBUSB) ? "USB" : file);

	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		state->device.fd = serial_opendevice(file, with_odd_parity, with_async, with_hw_handshake, state);
		break;
	case GN_CT_Irda:
		state->device.fd = irda_open(state);
		break;
	case GN_CT_Bluetooth:
		state->device.fd = bluetooth_open(state->config.port_device, state->config.rfcomm_cn, state);
		break;
	case GN_CT_Tekram:
		state->device.fd = tekram_open(file, state);
		break;
	case GN_CT_TCP:
		state->device.fd = tcp_opendevice(file, with_async, state);
		break;
	case GN_CT_DKU2LIBUSB:
		state->device.fd = fbusdku2usb_open(state);
		break;
	case GN_CT_SOCKETPHONET:
		state->device.fd = socketphonet_open(file, with_async, state);
		break;
	default:
		state->device.fd = -1;
		break;
	}

	if (state->device.fd < 0)
		return 0;

	/*
	 * handle config file connect_script:
	 */
	if (device_script(state->device.fd, 1, state)) {
		dprintf("gnokii open device: connect_script failure\n");
		device_close(state);
		return 0;
	}

	return 1;
}

void device_close(struct gn_statemachine *state)
{
	dprintf("device: closing device\n");

	/*
	 * handle config file disconnect_script:
	 */
	if (device_script(state->device.fd, 0, state))
		dprintf("gnokii device close: disconnect_script failure\n");

	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		serial_close(state->device.fd, state);
		break;
	case GN_CT_Irda:
		irda_close(state->device.fd, state);
		break;
	case GN_CT_Bluetooth:
		bluetooth_close(state->device.fd, state);
		break;
	case GN_CT_Tekram:
		tekram_close(state->device.fd, state);
		break;
	case GN_CT_TCP:
		tcp_close(state->device.fd, state);
		break;
	case GN_CT_DKU2LIBUSB:
		fbusdku2usb_close(state);
		break;
	case GN_CT_SOCKETPHONET:
		socketphonet_close(state);
		break;
	default:
		break;
	}

	free(state->device.device_instance);
	state->device.device_instance = NULL;
}

void device_setdtrrts(int dtr, int rts, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		dprintf("device: setting RTS to %s and DTR to %s\n", rts ? "high" : "low", dtr ? "high" : "low");
		serial_setdtrrts(state->device.fd, dtr, rts, state);
		break;
	case GN_CT_Irda:
	case GN_CT_Bluetooth:
	case GN_CT_Tekram:
	case GN_CT_TCP:
	case GN_CT_DKU2LIBUSB:
	case GN_CT_SOCKETPHONET:
	default:
		break;
	}
}

void device_changespeed(int speed, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		dprintf("device: setting speed to %d\n", speed);
		serial_changespeed(state->device.fd, speed, state);
		break;
	case GN_CT_Tekram:
		dprintf("device: setting speed to %d\n", speed);
		tekram_changespeed(state->device.fd, speed, state);
		break;
	case GN_CT_Irda:
	case GN_CT_Bluetooth:
	case GN_CT_TCP:
	case GN_CT_DKU2LIBUSB:
	case GN_CT_SOCKETPHONET:
	default:
		break;
	}
}

size_t device_read(__ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_read(state->device.fd, buf, nbytes, state);
	case GN_CT_Irda:
		return irda_read(state->device.fd, buf, nbytes, state);
	case GN_CT_Bluetooth:
		return bluetooth_read(state->device.fd, buf, nbytes, state);
	case GN_CT_Tekram:
		return tekram_read(state->device.fd, buf, nbytes, state);
	case GN_CT_TCP:
		return tcp_read(state->device.fd, buf, nbytes, state);
	case GN_CT_DKU2LIBUSB:
		return fbusdku2usb_read(buf, nbytes, state);
	case GN_CT_SOCKETPHONET:
		return socketphonet_read(state->device.fd, buf, nbytes, state);
	default:
		break;
	}
	return 0;
}

size_t device_write(const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_write(state->device.fd, buf, n, state);
	case GN_CT_Irda:
		return irda_write(state->device.fd, buf, n, state);
	case GN_CT_Bluetooth:
		return bluetooth_write(state->device.fd, buf, n, state);
	case GN_CT_Tekram:
		return tekram_write(state->device.fd, buf, n, state);
	case GN_CT_TCP:
		return tcp_write(state->device.fd, buf, n, state);
	case GN_CT_DKU2LIBUSB:
		return fbusdku2usb_write(buf, n, state);
	case GN_CT_SOCKETPHONET:
		return socketphonet_write(state->device.fd, buf, n, state);
	default:
		break;
	}
	return 0;
}

int device_select(struct timeval *timeout, struct gn_statemachine *state)
{
	switch (state->device.type) {
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_select(state->device.fd, timeout, state);
	case GN_CT_Irda:
		return irda_select(state->device.fd, timeout, state);
	case GN_CT_Bluetooth:
		return bluetooth_select(state->device.fd, timeout, state);
	case GN_CT_Tekram:
		return tekram_select(state->device.fd, timeout, state);
	case GN_CT_TCP:
		return tcp_select(state->device.fd, timeout, state);
	case GN_CT_DKU2LIBUSB:
		return fbusdku2usb_select(timeout, state);
	case GN_CT_SOCKETPHONET:
		return socketphonet_select(state->device.fd, timeout, state);
	default:
		break;
	}
	return -1;
}

gn_error device_nreceived(int *n, struct gn_statemachine *state)
{
	*n = -1;

	switch (state->device.type) {
	case GN_CT_DKU2:
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
	case GN_CT_DKU2:
	case GN_CT_Serial:
	case GN_CT_Infrared:
		return serial_flush(state->device.fd, state);
	default:
		return GN_ERR_NOTSUPPORTED;
	}
}
