/*

  $Id$
 
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>

*/

#include "config.h"

#ifdef HAVE_BLUETOOTH

#include "devices/unixbluetooth.h"

static char *phone[] = {
	"Nokia 6210",
	"Nokia 6310",
	"Nokia 6310i",
	"Nokia 7650",
	"Nokia 8910"
};

int bluetooth_open(bdaddr_t *bdaddr, int channel)
{
	struct sockaddr_rc laddr, raddr;
	int fd;

	if ((fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror("Can't create socket");
		return -1;
	}

	laddr.rc_family = AF_BLUETOOTH;
	bacpy(&laddr.rc_bdaddr, BDADDR_ANY);
	laddr.rc_channel = 0;

	if (bind(fd, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		perror("Can't bind socket");
		close(fd);
		return -1;
	}

	raddr.rc_family = AF_BLUETOOTH;
	bacpy(&raddr.rc_bdaddr, bdaddr);
	raddr.rc_channel = htobs(channel);

	if (connect(fd, (struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
		perror("Can't connect");
		close(fd);
		return -1;
	}

	return fd;
}

int bluetooth_close(int fd)
{
	return close(fd);
}

int bluetooth_write(int fd, const __ptr_t bytes, int size)
{
	return write(fd, bytes, size);
}

int bluetooth_read(int fd, __ptr_t bytes, int size)
{
	return read(fd, bytes, size);
}

int bluetooth_select(int fd, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return select(fd + 1, &readfds, NULL, NULL, timeout);
}

#else /* HAVE_BLUETOOTH */

int bluetooth_open(bdaddr_t *bdaddr, int channel) { return -1; }
int bluetooth_close(int fd) { return -1; }
int bluetooth_write(int fd, const __ptr_t bytes, int size) { return -1; }
int bluetooth_read(int fd, __ptr_t bytes, int size) { return -1; }
int bluetooth_select(int fd, struct timeval *timeout) { return -1; }

#endif /* HAVE_BLUETOOTH */
