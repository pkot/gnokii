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

static char *phone[] = {
	"Nokia 3360",
	"Nokia 3650",
	"Nokia 5100",
	"Nokia 5140",
	"Nokia 5140i",
	"Nokia 6020",
	"Nokia 6021",
	"Nokia 6100",
	"Nokia 6170",
	"Nokia 6210",
	"Nokia 6220",
	"Nokia 6230",
	"Nokia 6230i",
	"Nokia 6250",
	"Nokia 6310",
	"Nokia 6310i",
	"Nokia 6360",
	"Nokia 6500",
	"Nokia 6510",
	"Nokia 6610",
	"Nokia 6610i",
	"Nokia 6650",
	"Nokia 6800",
	"Nokia 6810",
	"Nokia 6820",
	"Nokia 6820b",
	"Nokia 7110",
	"Nokia 7190",
	"Nokia 7210",
	"Nokia 7250",
	"Nokia 7250i",
	"Nokia 7650",
	"Nokia 8210",
	"Nokia 8250",
	"Nokia 8290",
	"Nokia 8310",
	"Nokia 8850",
	"Nokia 9110",
	"Nokia 9210"
};

#define INVALID_DADDR	((DWORD)-1L)

static DWORD irda_discover_device(SOCKET fd)
{
	DEVICELIST			*list;
	IRDA_DEVICE_INFO	*dev;
	unsigned char		*buf;
	int			s, len, i, j;
	DWORD		daddr = INVALID_DADDR;
	DWORD		t1, t2;
	int phones = sizeof(phone) / sizeof(*phone);

	len = sizeof(*list) + sizeof(*dev) * 9;	/* 10 = max devices in discover */
	buf = malloc(len);
	list = (DEVICELIST *)buf;
	dev = list->Device;

	t1 = timeGetTime();

	do {
		s = len;
		memset(buf, 0, s);

		if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &s) != SOCKET_ERROR) {
			for (i = 0; (i < list->numDevice) && (daddr == INVALID_DADDR); i++) {
				for (j = 0; (j < phones) && (daddr == INVALID_DADDR); j++) {
					if (strncmp(dev[i].irdaDeviceName, phone[j], INFO_LEN) == 0) {
						daddr = *(DWORD*)dev[i].irdaDeviceID;
					}
				}
				if (daddr == INVALID_DADDR) {
					dprintf("unknown: %s\n", dev[i].irdaDeviceName);
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
	SOCKADDR_IRDA	peer;
	SOCKET fd = INVALID_SOCKET;
	DWORD daddr = INVALID_DADDR;

	if (WSAStartup(MAKEWORD(2,0), &wsaData) == 0) {
		fd = socket(AF_IRDA, SOCK_STREAM, 0);	/* Create socket */
		daddr = irda_discover_device(fd);			/* discover the devices */
		if (daddr != INVALID_DADDR)  {
			peer.irdaAddressFamily = AF_IRDA;
			*(DWORD*)peer.irdaDeviceID = daddr;
			strcpy(peer.irdaServiceName, "Nokia:PhoNet");

			if (connect(fd, (struct sockaddr *)&peer, sizeof(peer))) {	/* Connect to service "Nokia:PhoNet" */
				perror("connect");
				closesocket(fd);
				fd = INVALID_SOCKET;
			}
		}
	} else {
		fprintf(stderr, "Not WINSOCK2 :( - Get an upgrade dude!");
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
