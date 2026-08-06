/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002      Jan Kratochvil, Pavel Machek, Manfred Jonsson, Ladis Michl
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2002-2011 Pawel Kot

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "devices/tcp.h"

#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

#include <limits.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#  include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifndef O_NONBLOCK
#  define O_NONBLOCK  0
#endif

static int tcp_opensocket(const char *file)
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

void* tcp_open(gn_config *cfg, int with_odd_parity, int with_async)
{
	int fd, retcode, *data;

	/* Open device */

	fd = tcp_opensocket(cfg->port_device);
	if (fd < 0)
		return NULL;

#if !(__unices__)
	/* Allow process/thread to receive SIGIO */
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1) {
		perror(_("Gnokii tcp_opendevice: fcntl(F_SETOWN)"));
		close(fd);
		return NULL;
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
		close(fd);
		return NULL;
	}

	data = malloc(sizeof(int));
	if (data == NULL) {
		close(fd);
		return NULL;
	}
	*data = fd;

	return data;
}

void tcp_close(void *instance)
{
	close(*(int *)instance);
}

int tcp_getfd(void *instance)
{
	return *(int *)instance;
}

extern int unix_select(int fd, struct timeval *timeout);

int tcp_select(void *instance, struct timeval *timeout)
{
	return unix_select(*(int *)instance, timeout);
}

size_t tcp_read(void *instance, __ptr_t buf, size_t nbytes)
{
	return read(*(int *)instance, buf, nbytes);
}

size_t tcp_write(void *instance, const __ptr_t buf, size_t n)
{
	return write(*(int *)instance, buf, n);
}
