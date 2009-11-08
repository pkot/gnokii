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
  Copyright (C) 2002       Ladis Michl, Manfred Jonsson, Jan Kratochvil
  Copyright (C) 2001-2004  Pawel Kot
  Copyright (C) 2002-2003  BORBELY Zoltan

  This file provides a virtual modem interface to the GSM phone by calling
  code in gsm-api.c, at-emulator.c and datapump.c. The code here provides
  the overall framework and coordinates switching between command mode
  (AT-emulator) and "online" mode where the data pump code translates data
  from/to the GSM handset and the modem data/fax stream.

*/

#include "config.h"

/*
 * BEWARE! UGLY HACK!
 * We will define _XOPEN_SOURCE but it will disable the u_char, u_short,
 * u_int and u_long types on OpenBSD. It seems only the OpenBSD is affected,
 * so this is the workaround - bozo
 */
#ifdef	__OpenBSD__
#  include <sys/types.h>
#  define HAVE_MSGHDR_MSG_CONTROL 1
#endif

/* same for FreeBSD */
#ifdef	__FreeBSD__
#  include <sys/types.h>
#  undef	__BSD_VISIBLE
#  define	__BSD_VISIBLE	1
#endif

/* same for NetBSD */
#ifdef	__NetBSD__
#  include <sys/types.h>
#endif

/* This is the correct way to include stdlib with _XOPEN_SOURCE = 500 defined.
 * Needed for clean unlockpt() declaration.
 * msghdr structure in Solaris depends on _XOPEN_SOURCE = 500 too. - bozo
 */
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <grp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/param.h>

#include "compat.h"
#include "misc.h"
#include "gnokii-internal.h"
#include "data/at-emulator.h"
#include "data/datapump.h"
#include "device.h"

/* Defines */

#ifndef AF_LOCAL 
#  ifdef AF_UNIX
#    define AF_LOCAL AF_UNIX
#  else
#    error AF_LOCAL not defined
#  endif
#endif

/* Prototypes */
static int  VM_PtySetup(const char *bindir);
static gn_error VM_GSMInitialise(struct gn_statemachine *sm);

/* Global variables */

extern bool GTerminateThread;
int ConnectCount;
bool CommandMode;

/* Local variables */

static int PtyRDFD;	/* File descriptor for reading and writing to/from */
static int PtyWRFD;	/* pty interface - only different in debug mode. */

static bool UseSTDIO;	/* Use STDIO for debugging purposes instead of pty */

struct vm_queue queue;

/* If initialised in debug mode, stdin/out is used instead
   of ptys for interface. */
int gn_vm_initialise(const char *iname, const char *bindir, int debug_mode, int GSMInit)
{
	static struct gn_statemachine State;
	sm = &State;
	queue.n = 0;
	queue.head = 0;
	queue.tail = 0;

	CommandMode = true;

	if (debug_mode == true) {
		UseSTDIO = true;
	} else {
		UseSTDIO = false;
	}

	if (GSMInit) {
		dprintf("Initialising GSM\n");
		if (gn_cfg_phone_load(iname, sm) != GN_ERR_NONE) return false;
		if ((VM_GSMInitialise(sm) != GN_ERR_NONE)) {
			fprintf (stderr, _("gn_vm_initialise - VM_GSMInitialise failed!\n"));
			return (false);
		}
	}
	GSMInit = false;

	if (VM_PtySetup(bindir) < 0) {
		fprintf (stderr, _("gn_vm_initialise - VM_PtySetup failed!\n"));
		return (false);
	}

	if (gn_atem_initialise(PtyRDFD, PtyWRFD, sm) != true) {
		fprintf (stderr, _("gn_vm_initialise - gn_atem_initialise failed!\n"));
		return (false);
	}

	if (dp_Initialise(PtyRDFD, PtyWRFD) != true) {
		fprintf (stderr, _("gn_vm_initialise - dp_Initialise failed!\n"));
		return (false);
	}

	return (true);
}

void gn_vm_loop(void)
{
	fd_set rfds;
	struct timeval tv;
	int res;
	int nfd, devfd;
	int i, n;
	char buf[256], *d;

	devfd = device_getfd(sm);
	nfd = (PtyRDFD > devfd) ? PtyRDFD + 1 : devfd + 1;

	while (!GTerminateThread) {
		if (CommandMode && gn_atem_initialised && queue.n > 0) {
			d = queue.buf + queue.head;
			queue.head = (queue.head + 1) % sizeof(queue.buf);
			queue.n--;
			gn_atem_incoming_data_handle(d, 1);
			continue;
		}

		FD_ZERO(&rfds);
		FD_SET(devfd, &rfds);
		if ( queue.n < sizeof(queue.buf) ) {
			FD_SET(PtyRDFD, &rfds);
		}
		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		res = select(nfd, &rfds, NULL, NULL, &tv);

		switch (res) {
		case 0: /* Timeout */
			continue;

		case -1:
			perror("gn_vm_loop - select");
			exit (-1);

		default:
			break;
		}

		if (FD_ISSET(PtyRDFD, &rfds)) {
			n = sizeof(queue.buf) - queue.n < sizeof(buf) ?
				sizeof(queue.buf) - queue.n :
				sizeof(buf);
			if ( (n = read(PtyRDFD, buf, n)) <= 0 ) gn_vm_terminate();

			for (i = 0; i < n; i++) {
				queue.buf[queue.tail++] = buf[i];
				queue.tail %= sizeof(queue.buf);
				queue.n++;
			}
		}
		if (FD_ISSET(devfd, &rfds)) gn_sm_loop(1, sm);
	}
}

/* Application should call gn_vm_terminate to shut down
   the virtual modem thread */
void gn_vm_terminate(void)
{
	/* Request termination of thread */
	GTerminateThread = true;

	if (!UseSTDIO) {
		close (PtyRDFD);
		close (PtyWRFD);
	}

	/* Shutdown device */
	gn_sm_functions(GN_OP_Terminate, NULL, sm);
}

/* The following two functions are based on the skeleton from
 * W. Richard Stevens' "UNIX Network Programming", Volume 1, Second Edition.
 */

static int gread(int fd, void *ptr, size_t nbytes, int *recvfd)
{
	struct msghdr msg;
	struct iovec iov[1];
	int n;

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
#else
	int newfd;

	msg.msg_accrights = (caddr_t) &newfd;
	msg.msg_accrightslen = sizeof(int);
#endif /* HAVE_MSGHDR_MSG_CONTROL */

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ((n = recvmsg(fd, &msg, 0)) <= 0) return n;

#ifdef HAVE_MSGHDR_MSG_CONTROL
	if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
	    cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		if (cmptr->cmsg_level != SOL_SOCKET) perror("control level != SOL_SOCKET");
		if (cmptr->cmsg_type != SCM_RIGHTS) perror("control type != SCM_RIGHTS");
		*recvfd = *((int *)CMSG_DATA(cmptr));
	} else {
		*recvfd = -1;
	}
#else
	if (msg.msg_accrightslen == sizeof(int)) *recvfd = newfd;
	else *recvfd = -1;
#endif /* HAVE_MSGHDR_MSG_CONTROL */
	return (n);
}

static int gopen(const char *command)
{
	int fd = -1;
	int sockfd[2], status;
	pid_t childpid;
	char c, argsockfd[10];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) < 0) return -1;

	switch (childpid = fork()) {
	case -1:
		/* Critical error */
		return -1;
	case 0:
		/* Child */
		close(sockfd[0]);
		snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[1]);
		execl(command, "mgnokiidev", argsockfd, NULL);
		perror("execl: ");
	default:
		/* Parent */
		break;
	}

	close(sockfd[1]);
	waitpid(childpid, &status, 0);
	if (WIFEXITED(status) == 0) perror("wait: ");
	if ((status = WEXITSTATUS(status)) == 0) {
		gread(sockfd[0], &c, 1, &fd);
	} else {
		errno = status;
		fd = -1;
	}
	close(sockfd[0]);

#ifdef USE_UNIX98PTYS
	/*
	 * I don't know why but it's required to operate correctly.
	 * bozo -- tested on: Linux 2.4.17
	 */
	if (fd >= 0 && unlockpt(fd)) {
		close(fd);
		fd = -1;
	}
#endif

	return(fd);
}

/* Open pseudo tty interface and (in due course create a symlink
   to be /dev/gnokii etc. ) */
static int VM_PtySetup(const char *bindir)
{
	char mgnokiidev[200];

	if (UseSTDIO) {
		PtyRDFD = STDIN_FILENO;
		PtyWRFD = STDOUT_FILENO;
		return (0);
	}

	if (bindir) {
		strncpy(mgnokiidev, bindir, sizeof(mgnokiidev));
		strncat(mgnokiidev, "/", sizeof(mgnokiidev) - strlen(mgnokiidev));
	} else {
		mgnokiidev[0] = 0;
	}

	strncat(mgnokiidev, "mgnokiidev", sizeof(mgnokiidev) - strlen(mgnokiidev));

	if (access(mgnokiidev, X_OK) != 0) {
		fprintf(stderr, _("Cannot access %s, check the bindir in your config file!\n"), mgnokiidev);
		exit(1);
	}

	PtyRDFD = gopen(mgnokiidev);

	if (PtyRDFD < 0) {
		fprintf (stderr, _("Couldn't open pty!\n"));
		return(-1);
	}
	PtyWRFD = PtyRDFD;

	return (0);
}

/* Initialise GSM interface, returning gn_error as appropriate  */
static gn_error VM_GSMInitialise(struct gn_statemachine *sm)
{
	gn_error error;

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(sm);

	if (error != GN_ERR_NONE)
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model?). Quitting.\n"));

	return (error);
}
