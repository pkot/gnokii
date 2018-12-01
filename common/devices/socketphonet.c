/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2010 Daniele Forsi

  This file provides an API for accessing functions via the phonet Linux kernel module.
  See README for more details on supported mobile phones.

  The various routines are called socketphonet_(whatever).

*/

/* System header files */
#include <sys/socket.h>
#include <linux/phonet.h>

/* Various header files */
#include "config.h"
#include "compat.h"
#include "links/fbus-common.h"
#include "links/fbus-phonet.h"
#include "devices/serial.h"
#include "devices/socketphonet.h"
#include "gnokii-internal.h"

static struct sockaddr_pn addr = { .spn_family = AF_PHONET, .spn_dev = FBUS_DEVICE_PHONE };

int socketphonet_close(struct gn_statemachine *state)
{
	return close(state->device.fd);
}

int socketphonet_open(const char *interface, int with_async, struct gn_statemachine *state)
{
	int fd, retcode;

	fd = socket(PF_PHONET, SOCK_DGRAM, 0);
	if (fd == -1) {
		perror("socket");
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror("bind");
		close(fd);
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface))) {
		perror("setsockopt");
		close(fd);
		return -1;
	}

	/* Make filedescriptor asynchronous. */

	/* We need to supply FNONBLOCK (or O_NONBLOCK) again as it would get reset
	 * by F_SETFL as a side-effect!
	 */
#ifdef FNONBLOCK
	retcode = fcntl(fd, F_SETFL, (with_async ? FASYNC : 0) | FNONBLOCK);
#else
#  ifdef FASYNC
	retcode = fcntl(fd, F_SETFL, (with_async ? FASYNC : 0) | O_NONBLOCK);
#  else
	retcode = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (retcode != -1)
		retcode = ioctl(fd, FIOASYNC, &with_async);
#  endif
#endif
	if (retcode == -1) {
		perror("fcntl");
		close(fd);
		return -1;
	}

	return fd;
}

size_t socketphonet_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	int received;
	unsigned char *frame = buf;

	received = recvfrom(fd, buf + 8, nbytes - 8, 0, NULL, NULL);
	if (received == -1) {
		perror("recvfrom");
		return -1;
	}

	/* Hack!!! Rebuild header as expected by phonet_rx_statemachine() */
	/* Need to add 2 bytes to re-add spn_obj and spn_dev that the kernel driver doesn't return as payload */
	received += 2;
	frame[0] = FBUS_PHONET_DKU2_FRAME_ID;
	frame[1] = FBUS_PHONET_BLUETOOTH_DEVICE_PC;
	frame[2] = FBUS_DEVICE_PHONE;
	frame[3] = addr.spn_resource;
	frame[4] = received >> 8;
	frame[5] = received & 0xff;
	frame[6] = addr.spn_obj;
	frame[7] = addr.spn_dev;
	return received + 8;
}

size_t socketphonet_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	int sent;
	const unsigned char *frame = buf;

	addr.spn_resource = frame[3];

	sent = sendto(fd, buf + 8, n - 8, 0, (struct sockaddr *)&addr, sizeof(addr));
	if (sent == -1) {
		perror("sendto");
		return -1;
	}

	return sent + 8;
}

int socketphonet_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	return serial_select(fd, timeout, state);
}
