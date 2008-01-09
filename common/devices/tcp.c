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

  Copyright (C) 2002      Jan Kratochvil, Pavel Machek, Manfred Jonsson, Ladis Michl
  Copyright (C) 2002-2004 BORBELY Zoltan, Pawel Kot

*/

#include "config.h"
#include "misc.h"
#include "devices/tcp.h"
#include "devices/serial.h"

#ifndef WIN32

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>

#ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
#endif

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#  include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifndef O_NONBLOCK
#  define O_NONBLOCK  0
#endif

/* Open the serial port and store the settings. */

static int tcp_open(const char *file)
{
	int fd;
	struct sockaddr_in addr;
	char *filedup,*portstr,*end;
	unsigned long portul;
	struct hostent *hostent;

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		perror(_("Gnokii tcp_open: socket()"));
		return -1;
	}
	if (!(filedup = strdup(file))) {
	fail_close:
		close(fd);
		return -1;
	}
	if (!(portstr = strchr(filedup, ':'))) {
		fprintf(stderr, _("Gnokii tcp_open: colon (':') not found in connect strings \"%s\"!\n"), filedup);
	fail_free:
		free(filedup);
		goto fail_close;
	}
	*portstr++ = '\0';
	portul = strtoul(portstr, &end, 0);
	if ((end && *end) || portul >= 0x10000) {
		fprintf(stderr, _("Gnokii tcp_open: Port string \"%s\" not valid for IPv4 connection!\n"), portstr);
		goto fail_free;
	}
	if (!(hostent = gethostbyname(filedup))) {
		fprintf(stderr, _("Gnokii tcp_open: Unknown host \"%s\"!\n"), filedup);
		goto fail_free;
	}
	if (hostent->h_addrtype != AF_INET || hostent->h_length != sizeof(addr.sin_addr) || !hostent->h_addr_list[0]) {
		fprintf(stderr, _("Gnokii tcp_open: Address resolve for host \"%s\" not compatible!\n"), filedup);
		goto fail_free;
	}
	free(filedup);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(portul);
	memcpy(&addr.sin_addr, hostent->h_addr_list[0], sizeof(addr.sin_addr));

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror(_("Gnokii tcp_open: connect()"));
		goto fail_close;
	}

	return fd;
}

int tcp_close(int fd, struct gn_statemachine *state)
{
	/* handle config file disconnect_script:
	 */
	if (device_script(fd, "disconnect_script", state) == -1)
		fprintf(stderr, _("Gnokii tcp_close: disconnect_script\n"));

	return close(fd);
}

/* Open a device with standard options.
 * Use value (-1) for "with_hw_handshake" if its specification is required from the user
 */
int tcp_opendevice(const char *file, int with_async, struct gn_statemachine *state)
{
	int fd;
	int retcode;

	/* Open device */

	fd = tcp_open(file);

	if (fd < 0)
		return fd;

	/* handle config file connect_script:
	 */
	if (device_script(fd, "connect_script", state) == -1) {
		fprintf(stderr, _("Gnokii tcp_opendevice: connect_script\n"));
		tcp_close(fd, state);
		return -1;
	}

	/* Allow process/thread to receive SIGIO */

#if !(__unices__)
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1){
		perror(_("Gnokii tcp_opendevice: fcntl(F_SETOWN)"));
		tcp_close(fd, state);
		return -1;
	}
#endif

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
		perror(_("Gnokii tcp_opendevice: fcntl(F_SETFL)"));
		tcp_close(fd, state);
		return -1;
	}

	return fd;
}

int tcp_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	return serial_select(fd, timeout, state);
}


/* Read from serial device. */

size_t tcp_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	return read(fd, buf, nbytes);
}

/* Write to serial device. */

size_t tcp_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	return write(fd, buf, n);
}

#else /* WIN32 */

int tcp_close(int fd, struct gn_statemachine *state)
{
	return -1;
}

int tcp_opendevice(const char *file, int with_async, struct gn_statemachine *state)
{
	return -1;
}

size_t tcp_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	return -1;
}

size_t tcp_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	return -1;
}

int tcp_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	return -1;
}

#endif /* WIN32 */
