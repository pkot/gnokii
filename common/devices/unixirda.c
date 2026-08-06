/*
 *
 * G N O K I I
 *
 * A Linux/Unix toolset and driver for the mobile phones.
 *
 * Copyright (C) 1999-2000  Hugh Blemings & Pavel Janík ml.
 * Copyright (C) 2000-2001  Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2001       Chris Kemp
 * Copyright (C) 2001-2004  Pawel Kot
 * Copyright (C) 2002-2004  BORBELY Zoltan
 * Copyright (C) 2004       Phil Ashby
 *
 */

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"

#include <linux/types.h>
#include <linux/irda.h>

#include "devices/irda.h"

#ifndef AF_IRDA
#  define AF_IRDA 23
#endif

#define INFO_LEN		22
#define DISCOVERY_TIMEOUT	60.0
#define DISCOVERY_SLEEP		0.4

/* Maximum number of devices we look for */
#define MAX_DEVICES		20

static double d_time(void)
{
	double		time;
	struct timeval	tv;

	gettimeofday(&tv, NULL);

	time = tv.tv_sec + (((double)tv.tv_usec) / 1000000.0);

	return time;
}

static double d_sleep(double s)
{
	double		time;
	struct timeval	tv1, tv2;

	gettimeofday(&tv1, NULL);
	usleep(s * 1000000);
	gettimeofday(&tv2, NULL);

	time = tv2.tv_sec - tv1.tv_sec + (((double)(tv2.tv_usec - tv1.tv_usec)) / 1000000.0);

	return time;
}

static int irda_discover_device(const char *irda_string)
{
	struct irda_device_list *list;
	struct irda_device_info *dev;
	unsigned char *buf;
	int s, len, i, daddr = -1, fd;
	double t1, t2;

	fd = socket(AF_IRDA, SOCK_STREAM, 0);

	len = sizeof(*list) + sizeof(*dev) * MAX_DEVICES;
	buf = malloc(len);
	list = (struct irda_device_list *)buf;
	dev = list->dev;

	t1 = d_time();

	dprintf("Expecting: %s\n", irda_string);

	do {
		s = len;
		memset(buf, 0, s);

		if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, (socklen_t *)&s) == 0) {
			for (i = 0; (i < list->len) && (daddr == -1); i++) {
				if (strlen(irda_string) == 0) {
					/* We take first entry */
					daddr = dev[i].daddr;
					dprintf("Default: %s\t%x\n", dev[i].info, dev[i].daddr);
				} else {
					if (strncmp(dev[i].info, irda_string, INFO_LEN) == 0) {
						daddr = dev[i].daddr;
						dprintf("Matching: %s\t%x\n", dev[i].info, dev[i].daddr);
					} else {
						dprintf("Not matching: %s\t%x\n", dev[i].info, dev[i].daddr);
					}
				}
			}
		}

		if (daddr == -1) {
			d_sleep(DISCOVERY_SLEEP);
		}

		t2 = d_time();
	} while ((t2 - t1 < DISCOVERY_TIMEOUT) && (daddr == -1));

	free(buf);
	close(fd);

	return daddr;
}

void* irda_open(gn_config *cfg, int with_odd_parity, int with_async)
{
	struct sockaddr_irda peer;
	int fd, daddr, *data;

	if (!strcasecmp(cfg->port_device, "IrDA:IrCOMM")) {
		fprintf(stderr, _("Virtual IrCOMM device unsupported under Linux\n"));
		return NULL;
	}

	daddr = irda_discover_device(cfg->irda_string); /* discover the devices */
	if (daddr == -1)
		return NULL;

	fd = socket(AF_IRDA, SOCK_STREAM, 0);	/* Create socket */
	peer.sir_family = AF_IRDA;
	peer.sir_lsap_sel = LSAP_ANY;
	peer.sir_addr = daddr;
	snprintf(peer.sir_name, sizeof(peer.sir_name), "Nokia:PhoNet");

	if (connect(fd, (struct sockaddr *)&peer, sizeof(peer))) {	/* Connect to service "Nokia:PhoNet" */
		perror("connect");
		close(fd);
		return NULL;
/*	} else { FIXME: It does not work in most cases. Why? Or why it should work?
		recv(fd, NULL, 0, 0);		 call recv first to make select work correctly */
	}

	data = malloc(sizeof(int));
	if (data == NULL) {
		close(fd);
		return NULL;
	}
	*data = fd;

	return data;
}

void irda_close(void *instance)
{
	int fd = *(int *)instance;

	shutdown(fd, 0);
	close(fd);
}

int irda_getfd(void *instance)
{
	return *(int *)instance;
}

size_t irda_write(void *instance, const __ptr_t bytes, size_t size)
{
	return send(*(int *)instance, bytes, size, 0);
}

size_t irda_read(void *instance, __ptr_t bytes, size_t size)
{
	return recv(*(int *)instance, bytes, size, 0);
}

extern int unix_select(int fd, struct timeval *timeout);

int irda_select(void *instance, struct timeval *timeout)
{
	return unix_select(*(int *)instance, timeout);
}
