/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002      Jan Kratochvil, Pavel Machek, Manfred Jonsson, Ladis Michl
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2002-2011 Pawel Kot

*/

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

#include "config.h"
#include "misc.h"
#include "devices/tcp.h"
#include "devices/serial.h"


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

static int tcp_open(const char *file)
{
	int fd;
#ifdef HAVE_GETADDRINFO
	struct addrinfo hints, *result, *rp;
#else
	struct sockaddr_in addr;
	struct hostent *hostent;
#endif
	int gai_errorcode;
	char *filedup, *portstr, *end;
	unsigned long portul;

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		perror(_("Gnokii tcp_open: socket()"));
		return -1;
	}
	if (!(filedup = strdup(file))) {
		perror(_("Gnokii tcp_open: strdup()"));
		goto fail_close;
	}
	if (!(portstr = strchr(filedup, ':'))) {
		fprintf(stderr, _("Gnokii tcp_open: colon (':') not found in connect strings \"%s\"!\n"), filedup);
		goto fail_free;
	}
	*portstr++ = '\0';
	portul = strtoul(portstr, &end, 0);
	if ((end && *end) || portul >= 0x10000) {
		fprintf(stderr, _("Gnokii tcp_open: Port string \"%s\" not valid for IPv4 connection!\n"), portstr);
		goto fail_free;
	}

#ifdef HAVE_GETADDRINFO
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_TCP;

	gai_errorcode = getaddrinfo(filedup, portstr, &hints, &result);
	if (gai_errorcode != 0) {
		fprintf(stderr, "%s\n", gai_strerror(gai_errorcode));
		goto fail_free;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((rp->ai_family != PF_INET) &&
		    (rp->ai_family != PF_INET6))
			continue;
		if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
	}

	freeaddrinfo(result);

	if (rp == NULL) {
		fprintf(stderr, _("Gnokii tcp_open: Cannot connect!\n"));
		goto fail_free;
	}
#else
	if (!(hostent = gethostbyname(filedup))) {
		fprintf(stderr, _("Gnokii tcp_open: Unknown host \"%s\"!\n"), filedup);
		goto fail_free;
	}
	if (hostent->h_addrtype != AF_INET || hostent->h_length != sizeof(addr.sin_addr) || !hostent->h_addr_list[0]) {
		fprintf(stderr, _("Gnokii tcp_open: Address resolve for host \"%s\" not compatible!\n"), filedup);
		goto fail_free;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(portul);
	memcpy(&addr.sin_addr, hostent->h_addr_list[0], sizeof(addr.sin_addr));

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror(_("Gnokii tcp_open: connect()"));
		goto fail_free;
	}
#endif

	free(filedup);
	return fd;

fail_free:
	free(filedup);

fail_close:
	close(fd);
	return -1;
}

int tcp_close(int fd, struct gn_statemachine *state)
{
	return close(fd);
}

int tcp_opendevice(const char *file, int with_async, struct gn_statemachine *state)
{
	int fd;
	int retcode;

	/* Open device */
	fd = tcp_open(file);

	if (fd < 0)
		return fd;

#if !(__unices__)
	/* Allow process/thread to receive SIGIO */
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1) {
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

size_t tcp_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	return read(fd, buf, nbytes);
}

size_t tcp_write(int fd, const __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	return write(fd, buf, n);
}
