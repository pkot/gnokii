/*

  $Id$
 
  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2002       Ladis Michl, Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2003       BORBELY Zoltan, Pawel Kot

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "devices/unixbluetooth.h"

#if defined(HAVE_BLUETOOTH_BLUEZ) || defined(HAVE_BLUETOOTH_NETGRAPH)

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#ifdef HAVE_BLUETOOTH_NETGRAPH	/* FreeBSD / netgraph */

#include <bitstring.h>
#include <netgraph/bluetooth/include/ng_hci.h>
#include <netgraph/bluetooth/include/ng_l2cap.h>
#include <netgraph/bluetooth/include/ng_btsocket.h>

#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
#define BDADDR_ANY NG_HCI_BDADDR_ANY
#define sockaddr_rc sockaddr_rfcomm
#define rc_family rfcomm_family
#define rc_bdaddr rfcomm_bdaddr
#define rc_channel rfcomm_channel
#define bacpy(dst, src) memcpy((dst), (src), sizeof(bdaddr_t))

#ifndef HAVE_BT_ATON

static int bt_aton(const char *str, bdaddr_t *ba)
{
	char ch;
	unsigned int b[6];

	memset(ba, 0, sizeof(*ba));
	if (sscanf(str, "%x:%x:%x:%x:%x:%x%c", b + 0, b + 1, b + 2, b + 3, b + 4, b + 5, &ch) != 6) return 0;
	if ((b[0] | b[1] | b[2] | b[3] | b[4] | b[5]) > 0xff) return 0;

	ba->b[0] = b[0];
	ba->b[1] = b[1];
	ba->b[2] = b[2];
	ba->b[3] = b[3];
	ba->b[4] = b[4];
	ba->b[5] = b[5];

	return 1;
}

#endif	/* HAVE_BT_ATON */

static int str2ba(const char *str, bdaddr_t *ba)
{
	return !bt_aton(str, ba);
}

#else	/* Linux / BlueZ support */

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#endif

int bluetooth_open(const char *addr, uint8_t channel, struct gn_statemachine *state)
{
	bdaddr_t bdaddr;
	struct sockaddr_rc laddr, raddr;
	int fd;

	if (str2ba((char *)addr, &bdaddr)) {
		fprintf(stderr, _("Invalid bluetooth address \"%s\"\n"), addr);
		return -1;
	}

	if ((fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror(_("Can't create socket"));
		return -1;
	}

	memset(&raddr, 0, sizeof(raddr));
	raddr.rc_family = AF_BLUETOOTH;
	bacpy(&raddr.rc_bdaddr, &bdaddr);
	raddr.rc_channel = channel;
	if (connect(fd, (struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
		perror(_("Can't connect"));
		close(fd);
		return -1;
	}

	return fd;
}

int bluetooth_close(int fd, struct gn_statemachine *state)
{
	sleep(2);
	return close(fd);
}

int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return write(fd, bytes, size);
}

int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return read(fd, bytes, size);
}

int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return select(fd + 1, &readfds, NULL, NULL, timeout);
}

#endif	/* HAVE_BLUETOOTH_BLUEZ || HAVE_BLUETOOTH_NETGRAPH */
