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
#include "devices/irda.h"
#include "devices/unixbluetooth.h"
#include "devices/tcp.h"
#include "devices/serial.h"
#include "devices/tekram.h"
#include "devices/dku2libusb.h"
#include "devices/socketphonet.h"

#include <errno.h>
#include <sys/wait.h>

GNOKII_API int device_getfd(struct gn_statemachine *state)
{
	return state->device.fd;
}

/* Script handling: */
static void device_script_cfgfunc(const char *section, const char *key, const char *value)
{
	setenv(key, value, 1); /* errors ignored */
}

int device_script(int fd, const char *section, struct gn_statemachine *state)
{
	pid_t pid;
	const char *scriptname;
	int status;

	if (!strcmp(section, "connect_script"))
		scriptname = state->config.connect_script;
	else
		scriptname = state->config.disconnect_script;
	if (scriptname[0] == '\0')
		return 0;

	errno = 0;
	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, _("device_script(\"%s\"): fork() failure: %s!\n"), scriptname, strerror(errno));
		return -1;

	case 0: /* child */
		cfg_foreach(section, device_script_cfgfunc);
		errno = 0;
		if (dup2(fd, 0) != 0 || dup2(fd, 1) != 1 || close(fd)) {
			fprintf(stderr, _("device_script(\"%s\"): file descriptor preparation failure: %s\n"), scriptname, strerror(errno));
			_exit(-1);
		}
		/* FIXME: close all open descriptors - how to track them?
		 */
		execl("/bin/sh", "sh", "-c", scriptname, NULL);
		fprintf(stderr, _("device_script(\"%s\"): script execution failure: %s\n"), scriptname, strerror(errno));
		_exit(-1);
		/* NOTREACHED */

	default:
		if (pid == waitpid(pid, &status, 0 /* options */) && WIFEXITED(status) && !WEXITSTATUS(status))
			return 0;
		fprintf(stderr, _("device_script(\"%s\"): child script execution failure: %s, exit code=%d\n"), scriptname,
			(WIFEXITED(status) ? _("normal exit") : _("abnormal exit")),
			(WIFEXITED(status) ? WEXITSTATUS(status) : -1));
		errno = EIO;
		return -1;

	}
	/* NOTREACHED */
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
	if (device_script(state->device.fd, "connect_script", state) == -1) {
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
	if (device_script(state->device.fd, "disconnect_script", state) == -1)
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

void device_reset(struct gn_statemachine *state)
{
	return;
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
