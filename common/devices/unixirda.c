/*
 * $Id$
 *
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for Nokia mobile phones.
 *
 * Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
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
 * $Log$
 * Revision 1.2  2001-02-20 21:55:10  pkot
 * Small #include updates
 *
 * Revision 1.1  2001/02/16 14:29:52  chris
 * Restructure of common/.  Fixed a problem in fbus-phonet.c
 * Lots of dprintfs for Marcin
 * Any size xpm can now be loaded (eg for 7110 startup logos)
 * nk7110 code detects 7110/6210 and alters startup logo size to suit
 * Moved Marcin's extended phonebook code into gnokii.c
 *
 * Revision 1.2  2001/02/06 21:15:35  chris
 * Preliminary irda support for 7110 etc.  Not well tested!
 *
 * Revision 1.1  2001/02/03 23:56:17  chris
 * Start of work on irda support (now we just need fbus-irda.c!)
 * Proper unicode support in 7110 code (from pkot)
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include "unixirda.h"
#include "linuxirda.h"


#ifndef AF_IRDA
#define AF_IRDA 23
#endif



static int irda_discover_device(int fd)
{

	struct irda_device_list *list;
	unsigned char *buf;
	int len, i;

	len = sizeof(struct irda_device_list) +
	    sizeof(struct irda_device_info) * 10;	// 10 = max devices in discover

	buf = malloc(len);
	list = (struct irda_device_list *) buf;

	if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &len)) {
		perror("getsockopt");
		return -1;
	}

	if (len > 0) {
		for (i = 0; i < list->len; i++) {
		}

		i = 0;
		return (list->dev[i].daddr);
	}

	return -1;

}


int irda_open(void)
{

	struct sockaddr_irda peer;
	/* struct irda_ias_set ias_query; */
	int fd, daddr;


	/* Create socket */

	fd = socket(AF_IRDA, SOCK_STREAM, 0);

	if (fd < 0) perror("socket");


	/* discover the devices */

	daddr = irda_discover_device(fd);


	/* Connect to service "Nokia:PhoNet" */

	peer.sir_family = AF_IRDA;
	peer.sir_lsap_sel = LSAP_ANY;
	peer.sir_addr = DEV_ADDR_ANY;
	strcpy(peer.sir_name, "Nokia:PhoNet");

	if (connect(fd, (struct sockaddr *)&peer, sizeof(struct sockaddr_irda))) {
		perror("connect");
		return -1;
	}

	/* call recv first to make select work correctly */

	recv(fd, NULL, 0, 0);

	return fd;

}


int irda_close(int fd)
{

	shutdown(fd, 0);
	return close(fd);

}


int irda_write(int __fd, __const __ptr_t __bytes, int __size)
{

	return (send(__fd, __bytes, __size, 0));

}


int irda_read(int __fd, __ptr_t __bytes, int __size)
{

	return (recv(__fd, __bytes, __size, 0));

}


int irda_select(int fd, struct timeval *timeout)
{

	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return (select(fd + 1, &readfds, NULL, NULL, timeout));

}
