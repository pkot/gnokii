/*
  
  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log$
  Revision 1.11  2001-11-27 12:19:01  pkot
  Cleanup, indentation, ANSI complaint preprocesor symbols (Jan Kratochvil, me)

  Revision 1.10  2001/11/14 10:46:12  pkot
  Small cleanup with __unices__

  Revision 1.9  2001/09/14 12:15:28  pkot
  Cleanups from 0.3.3 (part1)

  Revision 1.8  2001/08/20 23:27:37  pkot
  Add hardware shakehand to the link layer (Manfred Jonsson)

  Revision 1.7  2001/07/03 00:03:36  pkot
  Small fixes to let gnokii compile and work under solaris (thanks to Artur Kubiak)

  Revision 1.6  2001/03/21 23:36:04  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.5  2001/03/19 23:43:46  pkot
  Solaris/ *BSD '#if defined' cleanup

  Revision 1.4  2001/03/13 01:21:38  pkot
  *BSD updates (Bert Driehuis)

  Revision 1.3  2001/03/06 22:27:46  pkot
  Misc docs and Makefiles updates and cleanups

  Revision 1.2  2001/02/21 19:57:05  chris
  More fiddling with the directory layout

*/

#include "misc.h"

/* Do not compile this file under Win32 systems. */

#ifndef WIN32

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include <termios.h>
#include "devices/unixserial.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#  include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

/* If the target operating system does not have cfsetspeed, we can emulate
   it. */

#ifndef HAVE_CFSETSPEED
#  if defined(HAVE_CFSETISPEED) && defined(HAVE_CFSETOSPEED)
#    define cfsetspeed(t, speed) \
	    (cfsetispeed(t, speed) || cfsetospeed(t, speed))
#  else
static int cfsetspeed(struct termios *t, int speed)
{
#    ifdef HAVE_TERMIOS_CSPEED
	t->c_ispeed = speed;
	t->c_ospeed = speed;
#    else
	t->c_cflag |= speed;
#    endif			/* HAVE_TERMIOS_CSPEED */
	return 0;
}
#  endif			/* HAVE_CFSETISPEED && HAVE_CFSETOSPEED */
#endif				/* HAVE_CFSETSPEED */

#ifndef O_NONBLOCK
#  define O_NONBLOCK  0
#endif

/* Structure to backup the setting of the terminal. */
struct termios serial_termios;

/* Open the serial port and store the settings. */
int serial_open(__const char *__file, int __oflag)
{
	int __fd;
	int retcode;

	__fd = open(__file, __oflag);
	if (__fd == -1) {
		perror("Gnokii serial_open: open");
		return (-1);
	}

	retcode = tcgetattr(__fd, &serial_termios);
	if (retcode == -1) {
		perror("Gnokii serial_open:tcgetattr");
		/* Don't call serial_close since serial_termios is not valid */
		close(__fd);
		return (-1);
	}

	return __fd;
}

/* Close the serial port and restore old settings. */
int serial_close(int __fd)
{
	if (__fd >= 0)
		tcsetattr(__fd, TCSANOW, &serial_termios);

	return (close(__fd));
}

/* Open a device with standard options. */
int serial_opendevice(__const char *__file, int __with_odd_parity,
		      int __with_async, int __with_hw_handshake)
{
	int fd;
	int retcode;
	struct termios tp;

	/* Open device */
	fd = serial_open(__file, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (fd < 0) return fd;

	/* Allow process/thread to receive SIGIO */
#if !(__unices__)
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETOWN)");
		serial_close(fd);
		return (-1);
	}
#endif

	/* Make filedescriptor asynchronous. */
	if (__with_async) {
		retcode = fcntl(fd, F_SETFL, FASYNC);
		if (retcode == -1) {
			perror("Gnokii serial_opendevice: fnctl(F_SETFL)");
			serial_close(fd);
			return (-1);
		}
	}

	/* Initialise the port settings */
	memcpy(&tp, &serial_termios, sizeof(struct termios));

	/* Set port settings for canonical input processing */
	tp.c_cflag = B0 | CS8 | CLOCAL | CREAD;
	if (__with_odd_parity) {
		tp.c_cflag |= (PARENB | PARODD);
		tp.c_iflag = 0;
	} else
		tp.c_iflag = IGNPAR;
	if (__with_hw_handshake)
		tp.c_cflag |= CRTSCTS;
	else
		tp.c_cflag &= ~CRTSCTS;

	tp.c_oflag = 0;
	tp.c_lflag = 0;
	tp.c_cc[VMIN] = 1;
	tp.c_cc[VTIME] = 0;

	retcode = tcflush(fd, TCIFLUSH);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: tcflush");
		serial_close(fd);
		return (-1);
	}

	retcode = tcsetattr(fd, TCSANOW, &tp);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: tcsetattr");
		serial_close(fd);
		return (-1);
	}

	return fd;
}

/* Set the DTR and RTS bit of the serial device. */
void serial_setdtrrts(int __fd, int __dtr, int __rts)
{
	unsigned int flags;

	flags = TIOCM_DTR;

	if (__dtr)
		ioctl(__fd, TIOCMBIS, &flags);
	else
		ioctl(__fd, TIOCMBIC, &flags);

	flags = TIOCM_RTS;

	if (__rts)
		ioctl(__fd, TIOCMBIS, &flags);
	else
		ioctl(__fd, TIOCMBIC, &flags);
}


int serial_select(int fd, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return (select(fd + 1, &readfds, NULL, NULL, timeout));
}


/* Change the speed of the serial device. */
void serial_changespeed(int __fd, int __speed)
{
#ifndef SGTTY
	struct termios t;
#else
	struct sgttyb t;
#endif
	int speed = B9600;

	switch (__speed) {
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
		speed = B115200;
		break;
	}

#ifndef SGTTY
	tcgetattr(__fd, &t);

	// This is not needed! We set up the speed via cfsetspeed
	//  t.c_cflag &= ~CBAUD;
	//  t.c_cflag |= speed;
	if (cfsetspeed(&t, speed) == -1)
		dprintf(_("Serial port speed setting failed\n"));

	tcsetattr(__fd, TCSADRAIN, &t);
#else
	ioctl(__fd, TIOCGETP, &t);

	t.sg_ispeed = speed;
	t.sg_ospeed = speed;

	ioctl(__fd, TIOCSETN, &t);
#endif
}

/* Read from serial device. */
size_t serial_read(int __fd, __ptr_t __buf, size_t __nbytes)
{
	return (read(__fd, __buf, __nbytes));
}

/* Write to serial device. */
size_t serial_write(int __fd, __const __ptr_t __buf, size_t __n)
{
	return (write(__fd, __buf, __n));
}

#endif /* WIN32 */
