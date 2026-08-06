/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001       Chris Kemp, Manfred Jonsson, Jank Kratochvil
  Copyright (C) 2002       Ladis Michl, Pavel Machek
  Copyright (C) 2001-2011  Pawel Kot
  Copyright (C) 2002-2004  BORBELY Zoltan

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "gnokii-internal.h"
#include "devices/serial.h"

#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#  include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifdef HAVE_SYS_MODEM_H
#  include <sys/modem.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#  include <sys/filio.h>
#endif

#ifdef HAVE_ERRNO_H
#  include <errno.h>
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

struct instance_data {
	int fd;
	int write_usleep;
	struct termios ti;
	bool require_dcd;
};

#define THIS(m) (((struct instance_data *)(instance))->m)

/*
 * Open the serial port and store the settings.
 * Returns instance data on success, or NULL if an error occurred.
 */
void* serial_init(const char *file, int oflag)
{
	int fd, ret;
	struct instance_data *data;

	fd = open(file, oflag);
	if (fd == -1) {
		perror("Gnokii serial_open: open");
		return NULL;
	}

	data = calloc(sizeof(struct instance_data), 1);
	if (data == NULL) {
		perror("Gnokii serial_open: calloc");
		close(fd);
		return NULL;
	}

	/* Per instance backup of the terminal settings. */
	ret = tcgetattr(fd, &data->ti);
	if (ret == -1) {
		perror("Gnokii serial_open: tcgetattr");
		close(fd);
		free(data);
		return NULL;
	}
	data->fd = fd;

	return data;
}

/*
 * Open a device with standard options.
 * Use value (-1) for "with_hw_handshake" if its specification is required from the user.
 */
void* serial_open(gn_config *cfg, int with_odd_parity, int with_async)
{
	int ret;
	struct termios tp;
	struct instance_data *data;

	/* Open device */

	/*
	 * O_NONBLOCK MUST be used here as the CLOCAL may be currently off
	 * and if DCD is down the "open" syscall would be stuck waiting for DCD.
	 */
	data = (struct instance_data *)serial_init(cfg->port_device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (data == NULL)
		return NULL;

	/* Initialise the port settings */
	memcpy(&tp, &data->ti, sizeof(struct termios));

	/* Set port settings for canonical input processing */
	tp.c_cflag = B0 | CS8 | CLOCAL | CREAD | HUPCL;
	if (with_odd_parity) {
		tp.c_cflag |= (PARENB | PARODD);
		tp.c_iflag = 0;
	} else
		tp.c_iflag = IGNPAR;
#ifdef CRTSCTS
	if (cfg->hardware_handshake)
		tp.c_cflag |= CRTSCTS;
	else
		tp.c_cflag &= ~CRTSCTS;
#endif

	tp.c_oflag = 0;
	tp.c_lflag = 0;
	tp.c_cc[VMIN] = 1;
	tp.c_cc[VTIME] = 0;

	ret = tcflush(data->fd, TCIFLUSH);
	if (ret == -1) {
		perror("Gnokii serial_opendevice: tcflush");
		goto bail;
	}

	ret = tcsetattr(data->fd, TCSANOW, &tp);
	if (ret == -1) {
		perror("Gnokii serial_opendevice: tcsetattr");
		goto bail;
	}

	if (serial_changespeed(data, cfg->serial_baudrate) != GN_ERR_NONE)
		serial_changespeed(data, 19200 /* default value */);

#if !(__unices__)
	/* Allow process/thread to receive SIGIO */
	ret = fcntl(data->fd, F_SETOWN, getpid());
	if (ret == -1) {
		perror("Gnokii serial_opendevice: fcntl(F_SETOWN)");
		goto bail;
	}
#endif

	/* Make filedescriptor asynchronous. */
	if (with_async) {
		/*
		 * We need to supply FNONBLOCK (or O_NONBLOCK) again as it would get reset
		 * by F_SETFL as a side-effect!
		 */
#ifdef FNONBLOCK
		ret = fcntl(data->fd, F_SETFL, (with_async ? FASYNC : 0) | FNONBLOCK);
#else
#  ifdef FASYNC
		ret = fcntl(data->fd, F_SETFL, (with_async ? FASYNC : 0) | O_NONBLOCK);
#  else
		ret = fcntl(data->fd, F_SETFL, O_NONBLOCK);
		if (ret != -1)
			ret = ioctl(data->fd, FIOASYNC, &with_async);
#  endif
#endif
		if (ret == -1) {
			perror("Gnokii serial_opendevice: fcntl(F_SETFL)");
			goto bail;
		}
	}
	data->write_usleep = cfg->serial_write_usleep;
	data->require_dcd = cfg->require_dcd;

	return data;
bail:
	serial_close(data);
	free(data);
	return NULL;
}


/*
 * Close the serial port and restore old settings.
 * Returns zero on success, -1 if an error occurred or fd was invalid.
 */
void serial_close(void *instance)
{
	THIS(ti).c_cflag |= HUPCL;	/* production == 1 */
	tcsetattr(THIS(fd), TCSANOW, &THIS(ti));
	close(THIS(fd));
}

/* Set the DTR and RTS bit of the serial device. */
void serial_setdtrrts(void *instance, int dtr, int rts)
{
	unsigned int flags;

	flags = TIOCM_DTR;

	if (dtr)
		ioctl(THIS(fd), TIOCMBIS, &flags);
	else
		ioctl(THIS(fd), TIOCMBIC, &flags);

	flags = TIOCM_RTS;

	if (rts)
		ioctl(THIS(fd), TIOCMBIS, &flags);
	else
		ioctl(THIS(fd), TIOCMBIC, &flags);
}

int unix_select(int fd, struct timeval *timeout)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return select(fd + 1, &readfds, NULL, NULL, timeout);
}

int serial_select(void *instance, struct timeval *timeout)
{
	return unix_select(THIS(fd), timeout);
}

static int serial_wselect(int fd, struct timeval *timeout)
{
	fd_set writefds;

	FD_ZERO(&writefds);
	FD_SET(fd, &writefds);

	return select(fd + 1, NULL, &writefds, NULL, timeout);
}


/*
 * Change the speed of the serial device.
 * RETURNS: Success
 */
gn_error serial_changespeed(void *instance, int speed)
{
	gn_error retcode = GN_ERR_NONE;
#ifndef SGTTY
	struct termios t;
#else
	struct sgttyb t;
#endif
	int new_speed = B9600;

	switch (speed) {
	case 0:
		dprintf("Not setting port speed\n");
		return GN_ERR_NOTSUPPORTED;
	case 2400:
		new_speed = B2400;
		break;
	case 4600:
		new_speed = B4800;
		break;
	case 9600:
		new_speed = B9600;
		break;
	case 19200:
		new_speed = B19200;
		break;
	case 38400:
		new_speed = B38400;
		break;
	case 57600:
		new_speed = B57600;
		break;
	case 115200:
		new_speed = B115200;
		break;
	default:
		fprintf(stderr, _("Serial port speed %d not supported!\n"), speed);
		return GN_ERR_NOTSUPPORTED;
	}

#ifndef SGTTY
	if (tcgetattr(THIS(fd), &t)) retcode = GN_ERR_INTERNALERROR;

	if (cfsetspeed(&t, new_speed) == -1) {
		dprintf("Serial port speed setting failed\n");
		retcode = GN_ERR_INTERNALERROR;
	}

	tcsetattr(THIS(fd), TCSADRAIN, &t);
#else
	if (ioctl(THIS(fd), TIOCGETP, &t)) retcode = GN_ERR_INTERNALERROR;

	t.sg_ispeed = new_speed;
	t.sg_ospeed = new_speed;

	if (ioctl(THIS(fd), TIOCSETN, &t)) retcode = GN_ERR_INTERNALERROR;
#endif
	return retcode;
}

/* Read from serial device. */
size_t serial_read(void *instance, __ptr_t buf, size_t nbytes)
{
	return read(THIS(fd), buf, nbytes);
}

#if !defined(TIOCMGET) && defined(TIOCMODG)
#  define TIOCMGET TIOCMODG
#endif

static void check_dcd(int fd)
{
#ifdef TIOCMGET
	int mcs;

	if (ioctl(fd, TIOCMGET, &mcs) || !(mcs & TIOCM_CAR)) {
		fprintf(stderr, _("ERROR: Modem DCD is down and global/require_dcd parameter is set!\n"));
		exit(EXIT_FAILURE);		/* Hard quit of all threads */
	}
#else
	dprintf("WARNING: global/require_dcd argument was set but it is not supported on this system!\n");
#endif
}

/* Write to serial device. */
size_t serial_write(void *instance, const __ptr_t buf, size_t n)
{
	int fd = THIS(fd);
	size_t r = 0, bs;
	ssize_t got;

	if (THIS(require_dcd))
		check_dcd(fd);

	while (n > 0) {
		bs = (THIS(write_usleep) < 0) ? n : 1;
		got = write(fd, buf + r, bs);
		if (got == 0) {
			dprintf("Serial write: oops, zero byte has written!\n");
		} else if (got < 0) {
#ifdef HAVE_ERRNO_H
			if (errno == EINTR) continue;
			if (errno != EAGAIN) {
				dprintf("Serial write: write error %d\n", errno);
				return -1;
			}
#endif
			dprintf("Serial write: transmitter busy, waiting\n");
			serial_wselect(fd, NULL);
			dprintf("Serial write: transmitter ready\n");
			continue;
		}

		n -= got;
		r += got;
		if (THIS(write_usleep) > 0)
			usleep(THIS(write_usleep));
	}
	return r;
}

gn_error serial_nreceived(void *instance, int *n)
{
	if (ioctl(THIS(fd), FIONREAD, n)) {
		dprintf("serial_nreceived: cannot get the received data size\n");
		return GN_ERR_INTERNALERROR;
	}

	return GN_ERR_NONE;
}

gn_error serial_flush(void *instance)
{
	if (tcdrain(THIS(fd))) {
		dprintf("serial_flush: cannot flush serial device\n");
		return GN_ERR_INTERNALERROR;
	}

	return GN_ERR_NONE;
}
