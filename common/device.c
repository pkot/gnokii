/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
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

#ifdef HAVE_BLUETOOTH
#include "devices/bluetooth.h"
const static gn_device_ops _bluetooth_ops = {
	bluetooth_open,
	bluetooth_close,
	bluetooth_select,
	bluetooth_read,
	bluetooth_write,
	bluetooth_getfd,
};
#  define bluetooth_ops	&_bluetooth_ops
#else
#  define bluetooth_ops	NULL
#endif

#ifdef HAVE_LIBUSB
#include "devices/dku2libusb.h"
const static gn_device_ops _dku2libusb_ops = {
	fbusdku2usb_open,
	fbusdku2usb_close,
	fbusdku2usb_select,
	fbusdku2usb_read,
	fbusdku2usb_write,
};
#  define dku2libusb_ops	&_dku2libusb_ops
#else
#  define dku2libusb_ops	NULL
#endif

#ifdef HAVE_IRDA
#include "devices/irda.h"
const static gn_device_ops _irda_ops = {
	irda_open,
	irda_close,
	irda_select,
	irda_read,
	irda_write,
	irda_getfd,
};
#  define irda_ops	&_irda_ops
#else
#  define irda_ops	NULL
#endif

#include "devices/serial.h"
const static gn_device_ops _serial_ops = {
	serial_open,
	serial_close,
	serial_select,
	serial_read,
	serial_write,
	serial_getfd,
	serial_nreceived,
	serial_flush,
	serial_changespeed,
	serial_setdtrrts,
};
#define serial_ops	&_serial_ops

#ifdef HAVE_SOCKETPHONET
#include "devices/socketphonet.h"
const static gn_device_ops _phonet_ops = {
	socketphonet_open,
	socketphonet_close,
	socketphonet_select,
	socketphonet_read,
	socketphonet_write,
	socketphonet_getfd,
};
#  define phonet_ops	&_phonet_ops
#else
#  define phonet_ops	NULL
#endif

#ifndef WIN32
#include "devices/tcp.h"
const static gn_device_ops _tcp_ops = {
	tcp_open,
	tcp_close,
	tcp_select,
	tcp_read,
	tcp_write,
	tcp_getfd,
};
#  define tcp_ops	&_tcp_ops
#else
#  define tcp_ops	NULL
#endif

#include "devices/tekram.h"
const static gn_device_ops _tekram_ops = {
	tekram_open,
	tekram_close,
	tekram_select,
	tekram_read,
	tekram_write,
	tekram_getfd,
	NULL,
	NULL,
	tekram_changespeed,
	tekram_setdtrrts,
};
#define tekram_ops	&_tekram_ops

GNOKII_API int device_getfd(struct gn_statemachine *state)
{
	gn_device *device = &state->device;

	if (device->ops->getfd && device->instance)
		return device->ops->getfd(device->instance);

	return -1;
}

int device_open(int with_odd_parity, int with_async,
		gn_connection_type device_type, struct gn_statemachine *state)
{
	gn_config *cfg = &state->config;
	gn_device *device = &state->device;

	device->type = GN_CT_NONE;
	device->instance = NULL;

	dprintf("device: opening device %s\n", (device_type == GN_CT_DKU2LIBUSB) ?
		"USB" : cfg->port_device);

	switch (device_type) {
	case GN_CT_DAU9P:
	case GN_CT_DLR3P:
	case GN_CT_DKU2:
	case GN_CT_Infrared:
	case GN_CT_M2BUS:
	case GN_CT_Serial:
		device->ops = serial_ops;
		break;
	case GN_CT_Irda:
		device->ops = irda_ops;
		break;
	case GN_CT_Bluetooth:
		device->ops = bluetooth_ops;
		break;
	case GN_CT_Tekram:
		device->ops = tekram_ops;
		break;
	case GN_CT_TCP:
		device->ops = tcp_ops;
		break;
	case GN_CT_DKU2LIBUSB:
		device->ops = dku2libusb_ops;
		break;
	case GN_CT_SOCKETPHONET:
		device->ops = phonet_ops;
		break;
	default:
		device->ops = NULL;
		break;
	}

	if (!device->ops)
		return 0;

	device->instance = device->ops->open(cfg, with_odd_parity, with_async);
	if (!device->instance)
		return 0;

	device->type = device_type;

	/*
	 * handle config file connect_script:
	 */
	if (device_script(device_getfd(state), 1, state)) {
		dprintf("gnokii open device: connect_script failure\n");
		device_close(state);
		return 0;
	}

	return 1;
}

void device_close(struct gn_statemachine *state)
{
	gn_device *device = &state->device;

	if (!device->instance)
		return;

	dprintf("device: closing device\n");

	/*
	 * handle config file disconnect_script:
	 */
	if (device_script(device_getfd(state), 0, state))
		dprintf("gnokii device close: disconnect_script failure\n");

	device->ops->close(device->instance);
	free(device->instance);
	device->instance = NULL;
	device->type = GN_CT_NONE;
}

void device_setdtrrts(int dtr, int rts, struct gn_statemachine *state)
{
	if (state->device.ops->setdtrrts && state->config.set_dtr_rts) {
		dprintf("device: setting RTS to %s and DTR to %s\n", rts ? "high" : "low", dtr ? "high" : "low");
		state->device.ops->setdtrrts(state->device.instance, dtr, rts);
	}
}

void device_changespeed(int speed, struct gn_statemachine *state)
{
	if (state->device.ops->changespeed) {
		dprintf("device: setting speed to %d\n", speed);
		state->device.ops->changespeed(state->device.instance, speed);
	}
}

size_t device_read(__ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	return state->device.ops->read(state->device.instance, buf, nbytes);
}

size_t device_write(const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	return state->device.ops->write(state->device.instance, buf, n);
}

int device_select(struct timeval *timeout, struct gn_statemachine *state)
{
	return state->device.ops->select(state->device.instance, timeout);
}

gn_error device_nreceived(int *n, struct gn_statemachine *state)
{
	*n = -1;

	if (state->device.ops->nreceived)
		return state->device.ops->nreceived(state->device.instance, n);

	return GN_ERR_NOTSUPPORTED;
}

gn_error device_flush(struct gn_statemachine *state)
{
	if (state->device.ops->flush)
		return state->device.ops->flush(state->device.instance);

	return GN_ERR_NOTSUPPORTED;
}
