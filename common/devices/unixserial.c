/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

/* [global] option "serial_write_usleep" default: */
#define SERIAL_WRITE_USLEEP_DEFAULT (-1)

#include "misc.h"
#include "cfgreader.h"

/* Do not compile this file under Win32 systems. */

#ifndef WIN32

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

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

/* Script handling: */

static void device_script_cfgfunc(const char *section,const char *key,const char *value)
{
	setenv(key, value, 1 /* overwrite */); /* errors ignored */
}

static int device_script(int fd, const char *section)
{
	pid_t pid;
	const char *scriptname = CFG_Get(CFG_Info, "global", section);
	int status;

	if (!scriptname)
		return(0);

	errno = 0;
	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, _("device_script(\"%s\"): fork() failure: %s!\n"), scriptname, strerror(errno));
		return(-1);

	case 0: /* child */
		CFG_GetForeach(CFG_Info, section, device_script_cfgfunc);
		errno = 0;
		if (dup2(fd, 0) != 0 || dup2(fd, 1) != 1 || close(fd)) {
			fprintf(stderr, _("device_script(\"%s\"): file descriptor prepare: %s\n"), scriptname, strerror(errno));
			_exit(-1);
		}
		/* FIXME: close all open descriptors - how to track them?
		 */
		execl("/bin/sh", "sh", "-c", scriptname, NULL);
		fprintf(stderr, _("device_script(\"%s\"): execute script: %s\n"), scriptname, strerror(errno));
		_exit(-1);
		/* NOTREACHED */

	default:
		if (pid == waitpid(pid, &status, 0 /* options */) && WIFEXITED(status) && !WEXITSTATUS(status))
			return(0);
		fprintf(stderr, _("device_script(\"%s\"): child script failure: %s, exit code=%d\n"), scriptname,
			(WIFEXITED(status) ? _("normal exit") : _("abnormal exit")),
			(WIFEXITED(status) ? WEXITSTATUS(status) : -1));
		errno = EIO;
		return(-1);

	}
	/* NOTREACHED */
}

int serial_close_all_openfds[0x10];	/* -1 when entry not used, fd otherwise */
int serial_close(int __fd);

static void serial_close_all(void)
{
	int i;

	dprintf("serial_close_all() executed\n");
	for (i = 0; i < ARRAY_LEN(serial_close_all_openfds); i++)
		if (serial_close_all_openfds[i] != -1)
			serial_close(serial_close_all_openfds[i]);
}

#if 0	/* Disabled for now as atexit() functions are then called multiple times for pthreads! */
static void unixserial_interrupted(int signo)
{
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
#endif

/* Open the serial port and store the settings. */
int serial_open(__const char *__file, int __oflag)
{
	int __fd;
	int retcode, i;
	static bool atexit_registered = false;

	if (!atexit_registered) {
		memset(serial_close_all_openfds, -1, sizeof(serial_close_all_openfds));
#if 0	/* Disabled for now as atexit() functions are then called multiple times for pthreads! */
		signal(SIGINT, unixserial_interrupted);
#endif
		atexit(serial_close_all);
		atexit_registered = true;
	}

	__fd = open(__file, __oflag);
	if (__fd == -1) {
		perror("Gnokii serial_open: open");
		return (-1);
	}

	for (i = 0; i < ARRAY_LEN(serial_close_all_openfds); i++)
		if (serial_close_all_openfds[i] == -1 || serial_close_all_openfds[i] == __fd) {
			serial_close_all_openfds[i] = __fd;
			break;
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
	int i;

	for (i = 0; i < ARRAY_LEN(serial_close_all_openfds); i++)
		if (serial_close_all_openfds[i] == __fd)
			serial_close_all_openfds[i] = -1;		/* fd closed */

	/* handle config file disconnect_script:
	 */
	if (device_script(__fd, "disconnect_script") == -1)
		fprintf(stderr, _("Gnokii serial_close: disconnect_script\n"));

	if (__fd >= 0) {
#if 1 /* HACK */
		serial_termios.c_cflag |= HUPCL;	/* production == 1 */
#else
		serial_termios.c_cflag &= ~HUPCL;	/* debugging  == 0 */
#endif

		tcsetattr(__fd, TCSANOW, &serial_termios);
	}

	return (close(__fd));
}

/* Open a device with standard options.
 * Use value (-1) for "__with_hw_handshake" if its specification is required from the user.
 */
int serial_opendevice(__const char *__file, int __with_odd_parity,
		      int __with_async, int __with_hw_handshake)
{
	int fd;
	int retcode, baudrate = 0;
	struct termios tp;
	char *s;

	/* handle config file handshake override: */
	s = CFG_Get(CFG_Info, "global", "handshake");

	if (s && (!strcasecmp(s, "software") || !strcasecmp(s, "rtscts")))
		__with_hw_handshake = false;
	else if (s && (!strcasecmp(s, "hardware") || !strcasecmp(s, "xonxoff")))
		__with_hw_handshake = true;
	else if (s)
		fprintf(stderr, _("Unrecognized [%s] option \"%s\", use \"%s\" or \"%s\" value, ignoring!"),
				 "global", "handshake", "software", "hardware");

	if (__with_hw_handshake == -1) {
		fprintf(stderr, _("[%s] option \"%s\" not found, trying to use \"%s\" value!"),
				  "global", "handshake", "software");
		__with_hw_handshake = false;
	}

	/* Open device */

	/* O_NONBLOCK MUST be used here as the CLOCAL may be currently off
	 * and if DCD is down the "open" syscall would be stuck wating for DCD.
	 */
	fd = serial_open(__file, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (fd < 0) return fd;

	/* Initialise the port settings */
	memcpy(&tp, &serial_termios, sizeof(struct termios));

	/* Set port settings for canonical input processing */
	tp.c_cflag = B0 | CS8 | CLOCAL | CREAD | HUPCL;
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

	/* Set speed */
	s = CFG_Get(CFG_Info, "global", "serial_baudrate"); /* baud rate string */

	if (s) baudrate = atoi(s);
	if (baudrate && serial_changespeed(fd, baudrate) != GE_NONE)
		baudrate = 0;
	if (!baudrate)
		serial_changespeed(fd, 19200 /* default value */ );

	/* We need to turn off O_NONBLOCK now (we have CLOCAL set so it is safe).
	 * When we run some device script it really doesn't expect NONBLOCK!
	 */

	retcode = fcntl(fd, F_SETFL, 0);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETFL)");
		serial_close(fd);
		return(-1);
	}

	/* handle config file connect_script:
	 */
	if (device_script(fd,"connect_script") == -1) {
		fprintf(stderr,"Gnokii serial_opendevice: connect_script\n");
		serial_close(fd);
		return(-1);
	}

	/* Allow process/thread to receive SIGIO */

#if !(__unices__)
	retcode = fcntl(fd, F_SETOWN, getpid());
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETOWN)");
		serial_close(fd);
		return(-1);
	}
#endif

	/* Make filedescriptor asynchronous. */

	/* We need to supply FNONBLOCK (or O_NONBLOCK) again as it would get reset
	 * by F_SETFL as a side-effect!
	 */
	retcode = fcntl(fd, F_SETFL, (__with_async ? FASYNC : 0) | FNONBLOCK);
	if (retcode == -1) {
		perror("Gnokii serial_opendevice: fnctl(F_SETFL)");
		serial_close(fd);
		return(-1);
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


/* Change the speed of the serial device.
 * RETURNS: Success
 */
GSM_Error serial_changespeed(int __fd, int __speed)
{
	GSM_Error retcode = GE_NONE;
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
	default:
		fprintf(stderr, _("Serial port speed %d not supported!\n"), __speed);
		return(GE_NOTSUPPORTED);
	}

#ifndef SGTTY
	if (tcgetattr(__fd, &t)) retcode = GE_INTERNALERROR;

	if (cfsetspeed(&t, speed) == -1) {
		dprintf("Serial port speed setting failed\n");
		retcode = GE_INTERNALERROR;
	}

	tcsetattr(__fd, TCSADRAIN, &t);
#else
	if (ioctl(__fd, TIOCGETP, &t)) retcode = GE_INTERNALERROR;

	t.sg_ispeed = speed;
	t.sg_ospeed = speed;

	if (ioctl(__fd, TIOCSETN, &t)) retcode = GE_INTERNALERROR;
#endif
	return(retcode);
}

/* Read from serial device. */
size_t serial_read(int __fd, __ptr_t __buf, size_t __nbytes)
{
	return (read(__fd, __buf, __nbytes));
}

#if !defined(TIOCMGET) && defined(TIOCMODG)
#  define TIOCMGET TIOCMODG
#endif

static void check_dcd(int __fd)
{
#ifdef TIOCMGET
	int mcs;

	if (ioctl(__fd, TIOCMGET, &mcs) || !(mcs & TIOCM_CAR)) {
		fprintf(stderr, _("ERROR: Modem DCD is down and global/require_dcd parameter is set!\n"));
		exit(EXIT_FAILURE);		/* Hard quit of all threads */
	}
#else
	/* Impossible!! (eg. Coherent) */
#endif
}

/* Write to serial device. */
size_t serial_write(int __fd, __const __ptr_t __buf, size_t __n)
{
	size_t r = 0;
	ssize_t got;
	static long serial_write_usleep = LONG_MIN;
	static int require_dcd = -1;

	if (serial_write_usleep == LONG_MIN) {
		char *s = CFG_Get(CFG_Info, "global", "serial_write_usleep");

		serial_write_usleep = (!s ?
			SERIAL_WRITE_USLEEP_DEFAULT : atol(CFG_Get(CFG_Info, "global", "serial_write_usleep")));
	}

	if (require_dcd == -1) {
		require_dcd = (!!CFG_Get(CFG_Info, "global", "require_dcd"));
#ifndef TIOCMGET
		if (require_dcd)
			fprintf(stderr, _("WARNING: global/require_dcd argument was set but it is not supported on this system!\n"));
#endif
	}

	if (require_dcd)
		check_dcd(__fd);

	if (serial_write_usleep < 0)
		return(write(__fd, __buf, __n));

	while (__n > 0) {
		got = write(__fd, __buf, 1);
		if (got <= 0)
			return((!r ? -1 : r));
		__buf++;
		__n--;
		r++;
		if (serial_write_usleep)
			usleep(serial_write_usleep);
	}
	return(r);
}

#endif /* WIN32 */
