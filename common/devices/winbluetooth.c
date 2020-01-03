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

#ifdef HAVE_BLUETOOTH

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

int bluetooth_open(const char *addr, uint8_t channel, struct gn_statemachine *state)
{
	WSADATA wsd;
	SOCKET fd = INVALID_SOCKET;
	SOCKADDR_BTH sa;

	/* Initialize */
	if (WSAStartup(MAKEWORD(2,0), &wsd)) {
		dprintf("WSAStartup() failed.\n");
		fprintf(stderr, _("Failed to initialize socket subsystem: need WINSOCK2. Please upgrade.\n"));
		return -1;
	}
	/* Create a bluetooth socket */
	if ((fd = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) == INVALID_SOCKET) {
		perror("socket");
		dprintf("Failed to create a bluetooth socket\n");
		WSACleanup();
		return -1;
	}
	/* Prepare socket structure for the bluetooth socket */
	memset(&sa, 0, sizeof(sa));
	sa.addressFamily = AF_BTH;
	if (str2ba(addr, &sa.btAddr)) {
		dprintf("Incorrect bluetooth address given in the config file\n");
		closesocket(fd);
		WSACleanup();
		return -1;
	}
	sa.port = channel & 0xff;
	/* Connect to the bluetooth socket */
	if (connect(fd, (SOCKADDR *)&sa, sizeof(sa))) {
		perror("socket");
		dprintf("Failed to connect to bluetooth socket\n");
		closesocket(fd);
		WSACleanup();
		return -1;
	}
	return (int)fd;
}

int bluetooth_close(int fd, struct gn_statemachine *state)
{
	shutdown(fd, 0);
	closesocket((SOCKET)fd);
	WSACleanup();
	return 0;
}

int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return send((SOCKET)fd, bytes, size, 0);
}

int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return recv((SOCKET)fd, bytes, size, 0);
}

int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET((SOCKET)fd, &readfds);

	return select(0 /* ignored on Win32 */, &readfds, NULL, NULL, timeout);
}

#endif /* HAVE_BLUETOOTH */
