/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2001       Chris Kemp
 * Copyright (C) 2002       Ladis Michl
 * Copyright (C) 2002-2003  BORBELY Zoltan, Pawel Kot
 *
 */

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"

#include "devices/serial.h"
#include "devices/tekram.h"

#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

void* tekram_open(gn_config *cfg, int with_odd_parity, int with_async)
{
#if defined(O_NOCTTY) && defined(O_NONBLOCK) && defined (O_RDWR)
	return serial_init(cfg->port_device, O_RDWR | O_NOCTTY | O_NONBLOCK);
#elif defined (O_RDWR)
	return serial_init(cfg->port_device, O_RDWR);
#else
	return serial_init(cfg->port_device, 0);
#endif
}

void tekram_close(void *instance)
{
	serial_setdtrrts(instance, 0, 0);
	serial_close(instance);
}

void tekram_reset(void *instance)
{
	serial_setdtrrts(instance, 0, 0);
	usleep(50000);
	serial_setdtrrts(instance, 1, 0);
	usleep(1000);
	serial_setdtrrts(instance, 1, 1);
	usleep(50);
	serial_changespeed(instance, 9600);
}

void tekram_changespeed(void *instance, int speed)
{
	unsigned char speedbyte;
	switch (speed) {
	default:
	case 9600:	speedbyte = TEKRAM_PW | TEKRAM_B9600;   break;
	case 19200:	speedbyte = TEKRAM_PW | TEKRAM_B19200;  break;
	case 38400:	speedbyte = TEKRAM_PW | TEKRAM_B38400;  break;
	case 57600:	speedbyte = TEKRAM_PW | TEKRAM_B57600;  break;
	case 115200:	speedbyte = TEKRAM_PW | TEKRAM_B115200; break;
	}
	tekram_reset(instance);
	serial_setdtrrts(instance, 1, 0);
	usleep(7);
	serial_write(instance, &speedbyte, 1);
	usleep(100000);
	serial_setdtrrts(instance, 1, 1);
	serial_changespeed(instance, speed);
}

size_t tekram_read(void *instance, __ptr_t buf, size_t nbytes)
{
	return serial_read(instance, buf, nbytes);
}

size_t tekram_write(void *instance, const __ptr_t buf, size_t n)
{
	return serial_write(instance, buf, n);
}

int tekram_select(void *instance, struct timeval *timeout)
{
	return serial_select(instance, timeout);
}
