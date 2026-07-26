/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 2006 Pawel Kot
 *
 */

#include "config.h"

#include <winsock2.h>
#include <mmsystem.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include "compat.h"
#include "gnokii.h"
#include "misc.h"

/* QTTY by Davide Libenzi ( Terminal interface to Symbian QConsole )
 * Copyright (C) 2004  Davide Libenzi
 * Davide Libenzi <davidel@xmailserver.org>
 */
static int str2ba(const char *straddr, BTH_ADDR *btaddr)
{
	int i;
	unsigned int aaddr[6];
	BTH_ADDR tmpaddr = 0;

	if (sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &aaddr[0], &aaddr[1], &aaddr[2], &aaddr[3], &aaddr[4], &aaddr[5]) != 6)
		return 1;
	*btaddr = 0;
	for (i = 0; i < 6; i++) {
		tmpaddr = (BTH_ADDR) (aaddr[i] & 0xff);
		*btaddr = ((*btaddr) << 8) + tmpaddr;
	}
	return 0;
}

void* bluetooth_open(gn_config *cfg, int with_odd_parity, int with_async)
{
	WSADATA wsd;
	SOCKET fd, *data;
	SOCKADDR_BTH sa;

	/* Prepare socket structure for the bluetooth socket */
	memset(&sa, 0, sizeof(sa));
	sa.addressFamily = AF_BTH;
	if (str2ba(cfg->port_device, &sa.btAddr)) {
		dprintf("Incorrect bluetooth address given in the config file\n");
		return NULL;
	}
	sa.port = cfg->rfcomm_cn & 0xff;
	/* Initialize */
	if (WSAStartup(MAKEWORD(2,0), &wsd)) {
		dprintf("WSAStartup() failed.\n");
		fprintf(stderr, _("Failed to initialize socket subsystem: need WINSOCK2. Please upgrade.\n"));
		return NULL;
	}
	/* Create a bluetooth socket */
	if ((fd = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) == INVALID_SOCKET) {
		perror("socket");
		dprintf("Failed to create a bluetooth socket\n");
		WSACleanup();
		return NULL;
	}
	/* Connect to the bluetooth socket */
	if (connect(fd, (SOCKADDR *)&sa, sizeof(sa))) {
		perror("socket");
		dprintf("Failed to connect to bluetooth socket\n");
		closesocket(fd);
		WSACleanup();
		return NULL;
	}
	data = malloc(sizeof(SOCKET));
	if (data == NULL) {
		closesocket(fd);
		WSACleanup();
		return NULL;
	}
	*data = fd;

	return data;
}

void bluetooth_close(void *instance)
{
	shutdown(*(SOCKET *)instance, 0);
	closesocket(*(SOCKET *)instance);
	WSACleanup();
}

size_t bluetooth_write(void *instance, const __ptr_t bytes, size_t size)
{
	return send(*(SOCKET *)instance, bytes, size, 0);
}

size_t bluetooth_read(void *instance, __ptr_t bytes, size_t size)
{
	return recv(*(SOCKET *)instance, bytes, size, 0);
}

int bluetooth_select(void *instance, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(*(SOCKET *)instance, &readfds);

	return select(0 /* ignored on Win32 */, &readfds, NULL, NULL, timeout);
}
