/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sat May 13 20:33:48 CEST 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

/* Do not compile this file under Win32 systems. */

#ifndef WIN32

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "unixserial.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
  #include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* If the target operating system does not have cfsetspeed, we can emulate
   it. */

#ifndef HAVE_CFSETSPEED
  #if defined(HAVE_CFSETISPEED) && defined(HAVE_CFSETOSPEED)
     #define cfsetspeed(t, speed) \
     (cfsetispeed(t, speed) || (cfsetospeed(t, speed))
  #else
    static int cfsetspeed(struct termios *t, int speed) {
    #ifdef HAVE_TERMIOS_CSPEED
      t->c_ispeed = speed;
      t->c_ospeed = speed;
    #else
      t->c_cflag |= speed;
    #endif
      return 0;
    }
  #endif
#endif

#ifndef O_NONBLOCK
  #define O_NONBLOCK  0
#endif

/* Structure to backup the setting of the terminal. */

struct termios serial_termios;

/* Open the serial port and store the settings. */

int serial_open(__const char *__file, int __oflag) {

  int __fd;

  __fd = open(__file, __oflag);

  if (__fd >= 0)
    tcgetattr(__fd, &serial_termios);

  return __fd;
}

/* Close the serial port and restore old settings. */

int serial_close(int __fd) {

  if (__fd >= 0)
    tcsetattr(__fd, TCSANOW, &serial_termios);

  return (close(__fd));
}

/* Open a device with standard options. */

int serial_opendevice(__const char *__file, int __with_odd_parity) {

  int fd;
  struct termios tp;

  /* Open device */

  fd = serial_open(__file, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd < 0)
    return fd;

  /* Allow process/thread to receive SIGIO */

  fcntl(fd, F_SETOWN, getpid());

  /* Make filedescriptor asynchronous. */

  fcntl(fd, F_SETFL, FASYNC);

  /* Set port settings for canonical input processing */

  tp.c_cflag = B0 | CS8 | CLOCAL | CREAD;
  if (__with_odd_parity) {
    tp.c_cflag |= (PARENB | PARODD);
    tp.c_iflag = 0;
  }
  else
    tp.c_iflag = IGNPAR;

  tp.c_oflag = 0;
  tp.c_lflag = 0;
  tp.c_cc[VMIN] = 1;
  tp.c_cc[VTIME] = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &tp);

  return fd;
}

/* Set the DTR and RTS bit of the serial device. */

void serial_setdtrrts(int __fd, int __dtr, int __rts) {

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

/* Change the speed of the serial device. */

void serial_changespeed(int __fd, int __speed) {

#ifndef SGTTY
  struct termios t;
#else
  struct sgttyb t;
#endif

  int speed=B9600;

  switch (__speed) {
    case 9600:   speed = B9600;   break;
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
  }

#ifndef SGTTY
  tcgetattr(__fd, &t);

  // This is not needed! We set up the speed via cfsetspeed
  //  t.c_cflag &= ~CBAUD;
  //  t.c_cflag |= speed;
  cfsetspeed(&t, speed);

  tcsetattr(__fd, TCSADRAIN, &t);
#else
  ioctl(__fd, TIOCGETP, &t);

  t.sg_ispeed = speed;
  t.sg_ospeed = speed;

  ioctl(__fd, TIOCSETN, &t);
#endif
}

/* Read from serial device. */

size_t serial_read(int __fd, __ptr_t __buf, size_t __nbytes) {

  return (read(__fd, __buf, __nbytes));
}

/* Write to serial device. */

size_t serial_write(int __fd, __const __ptr_t __buf, size_t __n) {

  return (write(__fd, __buf, __n));
}

#endif  /* WIN32 */
