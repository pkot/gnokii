/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 2004 Phil Ashby
 *
 */

#include "config.h"

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

static DWORD irda_discover_device(const char *irda_string, SOCKET fd)
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

	dprintf("Expecting: %s\n", irda_string);

	do {
		s = len;
		memset(buf, 0, s);

		if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &s) != SOCKET_ERROR) {
			for (i = 0; (i < list->numDevice) && (daddr == INVALID_DADDR); i++) {
				if (strlen(irda_string) == 0) {
					/* We take first entry */
					daddr = *(DWORD*)dev[i].irdaDeviceID;
					dprintf("Default: %s\t%x\n", dev[i].irdaDeviceName, *(DWORD*)dev[i].irdaDeviceID);
				} else {
					if (strncmp(dev[i].irdaDeviceName, irda_string, INFO_LEN) == 0) {
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

void* irda_open(gn_config *cfg, int with_odd_parity, int with_async)
{
	WSADATA wsaData;
	SOCKADDR_IRDA peer;
	SOCKET fd, *data;
	DWORD daddr = INVALID_DADDR;
	int x = 1;

	/* Initialize */
	if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
		dprintf("WSAStartup() failed.\n");
		fprintf(stderr, _("Failed to initialize socket subsystem: need WINSOCK2. Please upgrade.\n"));
		return NULL;
	}
	/* Create an irda socket */
	if ((fd = socket(AF_IRDA, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		dprintf("Failed to create an irda socket.\n");
		return NULL;
	}
	/* Discover devices */
	daddr = irda_discover_device(cfg->irda_string, fd); /* discover the devices */
	if (daddr == INVALID_DADDR) {
		dprintf("Failed to discover any irda device.\n");
		closesocket(fd);
		return NULL;
	}
	/* Prepare socket structure for irda socket */
	peer.irdaAddressFamily = AF_IRDA;
	*(DWORD*)peer.irdaDeviceID = daddr;
	if (!strcasecmp(cfg->port_device, "IrDA:IrCOMM")) {
		snprintf(peer.irdaServiceName, sizeof(peer.irdaServiceName), "IrDA:IrCOMM");
		if (setsockopt(fd, SOL_IRLMP, IRLMP_9WIRE_MODE, (char *)&x, sizeof(x)) == SOCKET_ERROR) {
			perror("setsockopt");
			dprintf("Failed to set irda socket options.\n");
			closesocket(fd);
			return NULL;
		}
	} else
		snprintf(peer.irdaServiceName, sizeof(peer.irdaServiceName), "Nokia:PhoNet");
	/* Connect to the irda socket */
	if (connect(fd, (struct sockaddr *)&peer, sizeof(peer))) {	/* Connect to service "Nokia:PhoNet" */
		perror("connect");
		dprintf("Failed to connect to irda socket\n");
		closesocket(fd);
		return NULL;
	}
	data = malloc(sizeof(SOCKET));
	if (data == NULL) {
		closesocket(fd);
		return NULL;
	}
	*data = fd;

	return data;
}

void irda_close(void *instance)
{
	shutdown(*(SOCKET *)instance, 0);
	closesocket(*(SOCKET *)instance);
	WSACleanup();
}

size_t irda_write(void *instance, const __ptr_t bytes, size_t size)
{
	return send(*(SOCKET *)instance, bytes, size, 0);
}

size_t irda_read(void *instance, __ptr_t bytes, size_t size)
{
	return recv(*(SOCKET *)instance, bytes, size, 0);
}

int irda_select(void *instance, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(*(SOCKET *)instance, &readfds);

	return select(0 /* ignored on Win32 */, &readfds, NULL, NULL, timeout);
}
