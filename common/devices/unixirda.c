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
 * Revision 1.6  2001-08-17 00:18:12  pkot
 * Removed recv() from IrDA initializing procedure (many people)
 *
 * Revision 1.5  2001/06/27 23:52:48  pkot
 * 7110/6210 updates (Marian Jancar)
 *
 * Revision 1.4  2001/06/20 21:27:34  pkot
 * IrDA patch (Marian Jancar)
 *
 * Revision 1.3  2001/02/21 19:57:04  chris
 * More fiddling with the directory layout
 *
 * Revision 1.2  2001/02/20 21:55:10  pkot
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

#include "devices/unixirda.h"
#include "linuxirda.h"


#ifndef AF_IRDA
#define AF_IRDA 23
#endif

#define INFO_LEN		22
#define DISCOVERY_TIMEOUT	60.0
#define DISCOVERY_SLEEP		0.4

static char *phone[] = {
	"Nokia 7110", "Nokia 6210"
};

double d_time(void)
{
	double		time;
	struct timeval	tv;
	
	gettimeofday(&tv, NULL);
	
	time = tv.tv_sec + (((double)tv.tv_usec) / 1000000.0);
	
	return time;
}

double d_sleep(double s)
{
	double		time;
	struct timeval	tv1, tv2;
	
	gettimeofday(&tv1, NULL);
	usleep(s * 1000000);
	gettimeofday(&tv2, NULL);
	
	time = tv2.tv_sec - tv1.tv_sec + (((double)(tv2.tv_usec - tv1.tv_usec)) / 1000000.0);
	
	return time;
}

static int irda_discover_device(void)
{

	struct irda_device_list	*list;
	struct irda_device_info	*dev;
	unsigned char		*buf;
	int			s, len, i, j, daddr = -1, fd;
	double			t1, t2;
	int phones = sizeof(phone) / sizeof(*phone);
	
	fd = socket(AF_IRDA, SOCK_STREAM, 0);
	
	len = sizeof(*list) + sizeof(*dev) * 10;	// 10 = max devices in discover
	buf = malloc(len);
	list = (struct irda_device_list *)buf;
	dev = list->dev;
	
	t1 = d_time();
	
	do {
		s = len;
		memset(buf, 0, s);
		
		if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &s) == 0) {
			for (i = 0; (i < list->len) && (daddr == -1); i++) {
				for (j = 0; (j < phones) && (daddr == -1); j++) {
					if (strncmp(dev[i].info, phone[j], INFO_LEN) == 0) {
						daddr = dev[i].daddr;
						dprintf("%s\t%x\n", dev[i].info, dev[i].daddr);
					}
				}
				if (daddr == -1) {
					dprintf("unknown: %s\t%x\n", dev[i].info, dev[i].daddr);
				}
			}
		}
		
		if (daddr == -1) {
			d_sleep(DISCOVERY_SLEEP);
		}
		
		t2 = d_time();
		
	} while ((t2 - t1 < DISCOVERY_TIMEOUT) && (daddr == -1));
	
	free(buf);
	close(fd);
	
	return daddr;
}

int irda_open(void)
{
	struct sockaddr_irda	peer;
	int			fd = -1, daddr;
	
	daddr = irda_discover_device();			/* discover the devices */
	
	if (daddr != -1)  {
		fd = socket(AF_IRDA, SOCK_STREAM, 0);	/* Create socket */
		peer.sir_family = AF_IRDA;
		peer.sir_lsap_sel = LSAP_ANY;
		peer.sir_addr = daddr;
		strcpy(peer.sir_name, "Nokia:PhoNet");
		
		if (connect(fd, (struct sockaddr *)&peer, sizeof(peer))) {	/* Connect to service "Nokia:PhoNet" */
			perror("connect");
			close(fd);
			fd = -1;
/*		} else { FIXME: It does not work in most cases. Why? Or why it should work?
			recv(fd, NULL, 0, 0);		 call recv first to make select work correctly */
		}
	}
	
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
