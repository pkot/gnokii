/*
 *
 * $Id$
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright (C) 2004 Phil Ashby
 *
 */

#include "config.h"

#ifdef HAVE_IRDA

#define WIN32_LEAN_AND_MEAN
#include <winsock.h>
#include <mmsystem.h>

#include "compat.h"
#include "misc.h"
#include "devices/irda.h"
/* 'cause af_irda needs it.. */
#define _WIN32_WINDOWS
#include <af_irda.h>

#define INFO_LEN		22
#define DISCOVERY_TIMEOUT	60000
#define DISCOVERY_SLEEP		400

/* Maximum number of devices we look for */
#define MAX_DEVICES		20

#define INVALID_DADDR	((DWORD)-1L)

static DWORD irda_discover_device(struct gn_statemachine *state, SOCKET fd)
{
	DEVICELIST *list;
	IRDA_DEVICE_INFO *dev;
	unsigned char *buf;
	int s, len, i;
	DWORD daddr = INVALID_DADDR;
	DWORD t1, t2;

	len = sizeof(*list) + sizeof(*dev) * MAX_DEVICES;
	buf = malloc(len);
	list = (DEVICELIST *)buf;
	dev = list->Device;

	t1 = timeGetTime();

	dprintf("Expecting: %s\n", state->config.irda_string);

	do {
		s = len;
		memset(buf, 0, s);

		if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &s) != SOCKET_ERROR) {
			for (i = 0; (i < list->numDevice) && (daddr == INVALID_DADDR); i++) {
				if (strlen(state->config.irda_string) == 0) {
					/* We take first entry */
					daddr = *(DWORD*)dev[i].irdaDeviceID;
					dprintf("Default: %s\t%x\n", dev[i].irdaDeviceName, *(DWORD*)dev[i].irdaDeviceID);
				} else {
					if (strncmp(dev[i].irdaDeviceName, state->config.irda_string, INFO_LEN) == 0) {
						daddr = *(DWORD*)dev[i].irdaDeviceID;
						dprintf("Matching: %s\t%x\n", dev[i].irdaDeviceName, *(DWORD*)dev[i].irdaDeviceID);
					} else {
						dprintf("Not matching: %s\t%x\n", dev[i].irdaDeviceName, *(DWORD*)dev[i].irdaDeviceID);
					}
				}
			}
		}

		if (daddr == INVALID_DADDR) {
			Sleep(DISCOVERY_SLEEP);
		}

		t2 = timeGetTime();
	} while ((t2 - t1 < DISCOVERY_TIMEOUT) && (daddr == INVALID_DADDR));

	free(buf);

	return daddr;
}

int irda_open(struct gn_statemachine *state)
{
	WSADATA wsaData;
	SOCKADDR_IRDA peer;
	SOCKET fd = INVALID_SOCKET;
	DWORD daddr = INVALID_DADDR;
	int x = 1;

	/* Initialize */
	if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
		dprintf("WSAStartup() failed.\n");
		fprintf(stderr, _("Failed to initialize socket subsystem: need WINSOCK2. Please upgrade.\n"));
		return -1;
	}
	/* Create an irda socket */
	if ((fd = socket(AF_IRDA, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		dprintf("Failed to create an irda socket.\n");
		return -1;
	}
	/* Discover devices */
	daddr = irda_discover_device(state, fd); /* discover the devices */
	if (daddr == INVALID_DADDR) {
		dprintf("Failed to discover any irda device.\n");
		closesocket(fd);
		return -1;
	}
	/* Prepare socket structure for irda socket */
	peer.irdaAddressFamily = AF_IRDA;
	*(DWORD*)peer.irdaDeviceID = daddr;
	if (!strcasecmp(state->config.port_device, "IrDA:IrCOMM")) {
		snprintf(peer.irdaServiceName, sizeof(peer.irdaServiceName), "IrDA:IrCOMM");
		if (setsockopt(fd, SOL_IRLMP, IRLMP_9WIRE_MODE, (char *)&x, sizeof(x)) == SOCKET_ERROR) {
			perror("setsockopt");
			dprintf("Failed to set irda socket options.\n");
			closesocket(fd);
			return -1;
		}
	} else
		snprintf(peer.irdaServiceName, sizeof(peer.irdaServiceName), "Nokia:PhoNet");
	/* Connect to the irda socket */
	if (connect(fd, (struct sockaddr *)&peer, sizeof(peer))) {	/* Connect to service "Nokia:PhoNet" */
		perror("connect");
		dprintf("Failed to connect to irda socket\n");
		closesocket(fd);
		return -1;
	}
	return (int)fd;
}

int irda_close(int fd, struct gn_statemachine *state)
{
	shutdown(fd, 0);
	closesocket((SOCKET)fd);
	WSACleanup();
	return 0;
}

int irda_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return send((SOCKET)fd, bytes, size, 0);
}

int irda_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return recv((SOCKET)fd, bytes, size, 0);
}

int irda_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET((SOCKET)fd, &readfds);

	return select(0 /* ignored on Win32 */, &readfds, NULL, NULL, timeout);
}

#endif /* HAVE_IRDA */
