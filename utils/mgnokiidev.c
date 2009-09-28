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

  Copyright (C) 1999-2000  Hugh Blemings, Pavel Janik ml.
  Copyright (C) 2000-2002  Pawel Kot
  Copyright (C) 2002-2003  BORBELY Zoltan

  The code is based on the skeleton from W. Richard Stevens' "UNIX Network Programming",
  Volume 1, Second Edition.

  Call mgnokiidev with one and only one argument -- a socket descriptor to send a caller
  pty device descriptor.

*/

#include "config.h"

/* See common/data/virtmodem.c for explanation */
#ifdef	__OpenBSD__
#  include <sys/types.h>
#  define HAVE_MSGHDR_MSG_CONTROL 1
#endif

/* See common/data/virtmodem.c for explanation */
#ifdef	__FreeBSD__
#  include <sys/types.h>
#  undef	__BSD_VISIBLE
#  define	__BSD_VISIBLE	1
#endif

/* See common/data/virtmodem.c for explanation */
#ifdef	__NetBSD__
#  include <sys/types.h>
#endif

/* See common/data/virtmodem.c for explanation */
#define _XOPEN_SOURCE 500

#include "misc.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>
#include <sys/param.h>

static int gwrite(int fd, void *ptr, size_t nbytes, int sendfd)
{
	struct msghdr msg;
	struct iovec iov[1];

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr msg;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmptr)) = sendfd;
#else
	msg.msg_accrights = (caddr_t)&sendfd;
	msg.msg_accrightslen = sizeof(int);
#endif /* HAVE_MSGHDR_MSG_CONTROL */

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	return (sendmsg(fd, &msg, 0));
}


/* VM_GetMasterPty()
   Takes a double-indirect character pointer in which to put a slave
   name, and returns an integer file descriptor.  If it returns < 0, an
   error has occurred.  Otherwise, it has returned the master pty
   file descriptor, and fills in *name with the name of the
   corresponding slave pty.  Once the slave pty has been opened,
   you are responsible to free *name.  Code is from Developing Linux
   Applications by Troan and Johnson */
static int GetMasterPty(char **name)
{
	int master = -1;
#ifdef USE_UNIX98PTYS
	master = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (master >= 0) {
		if (!grantpt(master) || !unlockpt(master)) {
			*name = strdup(ptsname(master));
		} else {
			return(-1);
		}
	}
#else /* USE_UNIX98PTYS */
	int i = 0 , j = 0;
	char *ptyp8 = "pqrstuvwxyzPQRST";
	char *ptyp9 = "0123456789abcdef";

	/* create a dummy name to fill in */
	*name = strdup("/dev/ptyXX");

	/* search for an unused pty */
	for (i = 0; i < 16 && master <= 0; i++) {
		for (j = 0; j < 16 && master <= 0; j++) {
			(*name)[8] = ptyp8[i];
			(*name)[9] = ptyp9[j];
			/* open the master pty */
			master = open(*name, O_RDWR | O_NOCTTY | O_NONBLOCK );
		}
	}
	if ((master < 0) && (i == 16) && (j == 16)) {
		/* must have tried every pty unsuccessfully */
		return (master);
	}

	/* By substituting a letter, we change the master pty
	 * name into the slave pty name.
	 */
	(*name)[5] = 't';

#endif /* USE_UNIX98PTYS */

	return (master);
}


int main(int argc, char **argv)
{
	int n, PtyRDFD = -1; /* Pty descriptor */
	char *name = NULL; /* Device name */

	/* For GNU gettext */
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	/* Check we have one and only one command line argument. */
	if (argc != 2) {
		fprintf(stderr, _("mgnokiidev takes one and only one argument!\n"));
		exit(-2);
	}

	PtyRDFD = GetMasterPty(&name);

	if (PtyRDFD < 0 || name == NULL) {
		fprintf (stderr, _("Couldn't open pty!\n"));
		perror("mgnokiidev - GetMasterPty");
		goto error;
	}

	/* Now change permissions to the slave pty */

	/* Now become root */
	setuid(0);

	/* Change group of slave pty to group of mgnokiidev */
	if (chown(name, -1, getgid()) < 0) {
		perror("mgnokiidev - chown");
		goto error;
	}

	/* Change permissions to rw by group */
	if (chmod(name, S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR) < 0) {
		perror("mgnokiidev - chmod");
		goto error;
	}

	/* FIXME: Possible bug - should check that /dev/gnokii doesn't already exist
	   in case multiple users are trying to run gnokii. Well, but will be
	   mgnokiidev called then? I do not think so - you will probably got the
	   message serialport in use or similar. Don't you. I haven't tested it
	   though. */

	/* Remove symlink in case it already exists. Don't care if it fails. */
	unlink("/dev/gnokii");

	/* Create symlink */
	if (symlink(name, "/dev/gnokii") < 0) {
		perror("mgnokiidev - symlink");
		goto error;
	}

	/* Now pass the descriptor to the caller */
	if ((n = gwrite(atoi(argv[1]), "", 1, PtyRDFD)) < 0) {
		perror("mgnokiidev - gwrite");
		goto error;
	}

	/* Done */
	exit(0);
error:
	if (name) free(name);
	exit(-2);
}
