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
  Copyright (C) 2002       Ladis Michl, Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2003       BORBELY Zoltan, Pawel Kot

  PhoneManager Utilities: rfcomm channel autodetection
  Copyright (C) 2003-2004 Edd Dumbill <edd@usefulinc.com>
  Copyright (C) 2005-2007 Bastien Nocera <hadess@hadess.net>

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "devices/unixbluetooth.h"

#if defined(HAVE_BLUETOOTH_BLUEZ) || defined(HAVE_BLUETOOTH_NETGRAPH) || defined(HAVE_BLUETOOTH_NETBT)

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#ifdef HAVE_BLUETOOTH_NETGRAPH	/* FreeBSD / netgraph */

#include <bluetooth.h>
#include <sdp.h>

#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
#define BDADDR_ANY NG_HCI_BDADDR_ANY
#define GNOKII_SERIAL_PORT_CLASS	SDP_SERVICE_CLASS_SERIAL_PORT
#define GNOKII_DIALUP_NETWORK_CLASS	SDP_SERVICE_CLASS_DIALUP_NETWORKING
#define sockaddr_rc sockaddr_rfcomm
#define rc_family rfcomm_family
#define rc_bdaddr rfcomm_bdaddr
#define rc_channel rfcomm_channel

#else
#ifdef HAVE_BLUETOOTH_NETBT	/* FreeBSD / netbt */

#include <bluetooth.h>
#include <sdp.h> 

#define GNOKII_SERIAL_PORT_CLASS	SDP_SERVICE_CLASS_SERIAL_PORT
#define GNOKII_DIALUP_NETWORK_CLASS	SDP_SERVICE_CLASS_DIALUP_NETWORKING
#define sockaddr_rc sockaddr_bt
#define rc_family bt_family
#define rc_bdaddr bt_bdaddr
#define rc_channel bt_channel

#else	/* Linux / BlueZ support */

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#define GNOKII_SERIAL_PORT_CLASS	SERIAL_PORT_SVCLASS_ID
#define GNOKII_DIALUP_NETWORK_CLASS	DIALUP_NET_SVCLASS_ID

#endif /* HAVE_BLUETOOTH_NETBT */
#endif /* HAVE_BLUETOOTH_NETGRAPH */

#if defined(HAVE_BLUETOOTH_NETGRAPH) || defined(HAVE_BLUETOOTH_NETBT) /* FreeBSD / NetBSD */

/*
** FreeBSD version of the find_service_channel function.
** Written by Guido Falsi <mad@madpilot.net>.
** Contains code taken from FreeBSD's sdpcontrol and rfcomm_sppd
** programs, which are Copyright (c) 2001-2003 Maksim Yevmenkin
** <m_evmenkin@yahoo.com>.
**
** Also thanks to Iain Hibbert for his suggestions.
*/

#define attrs_len	(sizeof(attrs)/sizeof(attrs[0]))
#define NRECS   25      /* request this much records from the SDP server */
#define BSIZE   256     /* one attribute buffer size */
#define values_len      (sizeof(values)/sizeof(values[0]))

static int find_service_channel(bdaddr_t *adapter, bdaddr_t *device, int only_gnapplet, uint16_t svclass_id)
{
	int i, channel = -1;
	char name[64];
	void *ss = NULL;
	uint32_t attrs[] = {
		SDP_ATTR_RANGE( SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
			SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST),
		SDP_ATTR_RANGE( SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET,
			SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET),
	};
	/* Buffer for the attributes */
	static uint8_t          buffer[NRECS * attrs_len][BSIZE];
	/* SDP attributes */
	static sdp_attr_t       values[NRECS * attrs_len];

	/* Initialize attribute values array */
	for (i = 0; i < values_len; i ++) {
		values[i].flags = SDP_ATTR_INVALID;
		values[i].attr = 0;
		values[i].vlen = BSIZE; 
		values[i].value = buffer[i];
	}

	if ((ss = sdp_open(adapter, device)) == NULL)
		return -1;

	if (sdp_error(ss) != 0)
		goto end;

	if (sdp_search(ss, 1, &svclass_id, attrs_len, attrs, values_len, values) != 0)
		goto end;

	for (i = 0; i < values_len; i++) {
		union {
			uint8_t		uint8;
			uint16_t	uint16;
			uint32_t	uint32;
			uint64_t	uint64;
			int128_t	int128;
		} value;
		uint8_t *start, *end;
		uint32_t type, len;

		if (values[i].flags != SDP_ATTR_OK)
			break;

		start = values[i].value;
		end = values[i].value + values[i].vlen;

		switch (values[i].attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			SDP_GET8(type, start);
			switch (type) {
			case SDP_DATA_SEQ8:
				SDP_GET8(len, start);
				break;

			case SDP_DATA_SEQ16:
				SDP_GET16(len, start);
				break;

			case SDP_DATA_SEQ32:
				SDP_GET32(len, start);
				break;

			default:
				goto end;
				break;
			}

			SDP_GET8(type, start);
			switch (type) {
			case SDP_DATA_SEQ8:
				SDP_GET8(len, start);
				break;

			case SDP_DATA_SEQ16:
				SDP_GET16(len, start);
				break;

			case SDP_DATA_SEQ32:
				SDP_GET32(len, start);
				break;

			default:
				goto end;
			}

			while (start < end) {
				SDP_GET8(type, start);
				switch (type) {
				case SDP_DATA_UUID16:
					SDP_GET16(value.uint16, start);
					break;

				case SDP_DATA_UUID32:
					SDP_GET32(value.uint32, start);
					break;

				case SDP_DATA_UUID128:
					SDP_GET_UUID128(&value.int128, start);
					break;

				default:
					goto end;
				}
				if (value.uint16 == 3) {
					SDP_GET8(type, start);
					switch (type) {
					case SDP_DATA_UINT8:
					case SDP_DATA_INT8:
						SDP_GET8(value.uint8, start);
						channel = value.uint8;
						break;

					case SDP_DATA_UINT16:
					case SDP_DATA_INT16:
						SDP_GET16(value.uint16, start);
						channel = value.uint16;
						break;

					case SDP_DATA_UINT32:
					case SDP_DATA_INT32:
						SDP_GET32(value.uint32, start);
						channel = value.uint32;
						break;

					default:
						goto end;
					}
				} else {
					SDP_GET8(type, start);
					switch (type) {
					case SDP_DATA_SEQ8:
					case SDP_DATA_UINT8:
					case SDP_DATA_INT8:
					case SDP_DATA_BOOL:
						SDP_GET8(value.uint8, start);
						break;

					case SDP_DATA_SEQ16:
					case SDP_DATA_UINT16:
					case SDP_DATA_INT16:
					case SDP_DATA_UUID16:
						SDP_GET16(value.uint16, start);
						break;

					case SDP_DATA_SEQ32:
					case SDP_DATA_UINT32:
					case SDP_DATA_INT32:
					case SDP_DATA_UUID32:
						SDP_GET32(value.uint32, start);
						break;

					case SDP_DATA_UINT64:
					case SDP_DATA_INT64:
						SDP_GET64(value.uint64, start);
						break;

					case SDP_DATA_UINT128:
					case SDP_DATA_INT128:
						SDP_GET128(&value.int128, start);
						break;

					default:
						goto end;
					}
				}
			}
			start += len;
			break;

		case SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET:
			if (channel == -1)
				break;
			
			SDP_GET8(type, start);
			switch (type) {
				case SDP_DATA_STR8:
				case SDP_DATA_URL8:
					SDP_GET8(len, start);
					snprintf(name, sizeof(name), "%*.*s", len, len, (char *) start);
					start += len;
					break;

				case SDP_DATA_STR16:
				case SDP_DATA_URL16:
					SDP_GET16(len, start);
					snprintf(name, sizeof(name), "%*.*s", len, len, (char *) start);
					start += len;
					break;

				case SDP_DATA_STR32:
				case SDP_DATA_URL32:
					SDP_GET32(len, start);
					snprintf(name, sizeof(name), "%*.*s", len, len, (char *) start);
					start += len;
					break;

				default:
					goto end;
			}
			if (name == NULL)
				break;

			if (only_gnapplet != 0) {
				if (strcmp(name, "gnapplet") == 0)
					goto end;
				else {
					channel = -1;
					break;
				}
			}
			
			if (strstr(name, "Nokia PC Suite") != NULL) {
				channel = -1;
				break;
			}

			if (strstr(name, "Bluetooth Serial Port") != NULL) {
				channel = -1;
				break;
			}

			if (strstr(name, "m-Router Connectivity") != NULL) {
				channel = -1;
				break;
			}

			goto end;
		}
	}

end:
	sdp_close(ss);
	return channel;
}

#else
/*
 * Taken from gnome-phone-manager
 */
static int get_rfcomm_channel(sdp_record_t *rec, int only_gnapplet)
{
	int channel = -1;
	sdp_list_t *protos = NULL;
	sdp_data_t *data;
	char name[64];

	if (sdp_get_access_protos(rec, &protos) != 0)
		goto end;

	data = sdp_data_get(rec, SDP_ATTR_SVCNAME_PRIMARY);
	if (data)
		snprintf(name, sizeof(name), "%.*s", data->unitSize, data->val.str);

	if (name == NULL)
		goto end;

	/*
	 * If we're only supposed to check for gnapplet, do it here
	 * We ignore it if we're not supposed to check for it.
	 */
	if (only_gnapplet != 0) {
		if (strcmp(name, "gnapplet") == 0)
			channel = sdp_get_proto_port(protos, RFCOMM_UUID);
		goto end;
	}

	/*
	 * We can't seem to connect to the PC Suite channel.
	 */
	if (strstr(name, "Nokia PC Suite") != NULL)
		goto end;
	/*
	 * And that type of channel on Nokia Symbian phones doesn't
	 * work either.
	 */
	if (strstr(name, "Bluetooth Serial Port") != NULL)
		goto end;
	/*
	 * Avoid the m-Router channel, same as the PC Suite on Sony Ericsson
	 * phones.
	 */
	if (strstr(name, "m-Router Connectivity") != NULL)
		goto end;

	channel = sdp_get_proto_port(protos, RFCOMM_UUID);
end:
	sdp_list_foreach(protos, (sdp_list_func_t)sdp_list_free, 0);
	sdp_list_free(protos, 0);
	return channel;
}

/*
 * Determine whether the given device supports Serial or Dial-Up Networking,
 * and if so what the RFCOMM channel number for the service is.
 */
static int find_service_channel(bdaddr_t *adapter, bdaddr_t *device, int only_gnapplet, uint16_t svclass_id)
{
	sdp_session_t *sdp = NULL;
	sdp_list_t *search = NULL, *attrs = NULL, *recs = NULL, *tmp;
	uuid_t browse_uuid, service_id;
	uint32_t range = 0x0000ffff;
	int channel = -1;

	sdp = sdp_connect(adapter, device, SDP_RETRY_IF_BUSY);
	if (!sdp)
		goto end;

	sdp_uuid16_create(&browse_uuid, PUBLIC_BROWSE_GROUP);
	sdp_uuid16_create(&service_id, svclass_id);
	search = sdp_list_append(NULL, &browse_uuid);
	search = sdp_list_append(search, &service_id);

	attrs = sdp_list_append(NULL, &range);

	if (sdp_service_search_attr_req(sdp, search,
					 SDP_ATTR_REQ_RANGE, attrs,
					 &recs))
		goto end;

	for (tmp = recs; tmp != NULL; tmp = tmp->next) {
		sdp_record_t *rec = tmp->data;

		/*
		 * If this service is better than what we've
		 * previously seen, try and get the channel number.
		 */
		channel = get_rfcomm_channel(rec, only_gnapplet);
		if (channel > 0)
			goto end;
	}

end:
	sdp_list_free(recs, (sdp_free_func_t)sdp_record_free);
	sdp_list_free(search, NULL);
	sdp_list_free(attrs, NULL);
	sdp_close(sdp);

	return channel;
}

#endif

static uint8_t get_serial_channel(bdaddr_t *device, int only_gnapplet)
{
	bdaddr_t src;
	int channel;
	uint8_t retval;

	bacpy(&src, BDADDR_ANY);

	channel = find_service_channel(&src, device, only_gnapplet, GNOKII_SERIAL_PORT_CLASS);
	if (channel < 0)
		channel = find_service_channel(&src, device, only_gnapplet, GNOKII_DIALUP_NETWORK_CLASS);

	if (channel < 0)
		retval = 0;
	else
		retval = channel;
	return retval;
}

/* From: http://www.kegel.com/dkftpbench/nonblocking.html */
static int setNonblocking(int fd)
{
	int retcode, flags;

#if defined(O_NONBLOCK)
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	retcode = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	flags = 1;
	retcode = ioctl(fd, FIOASYNC, &flags);
#endif

	return retcode;
}

int bluetooth_open(const char *addr, uint8_t channel, struct gn_statemachine *state)
{
	bdaddr_t bdaddr;
	struct sockaddr_rc raddr;
	int fd;

	if (str2ba((char *)addr, &bdaddr)) {
		fprintf(stderr, _("Invalid bluetooth address \"%s\"\n"), addr);
		return -1;
	}

	if ((fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror(_("Can't create socket"));
		return -1;
	}

	memset(&raddr, 0, sizeof(raddr));
	raddr.rc_family = AF_BLUETOOTH;
	bacpy(&raddr.rc_bdaddr, &bdaddr);
	dprintf("Channel: %d\n", channel);
	if (channel < 1) {
		if (!strcmp(state->config.model, "gnapplet") ||
		    !strcmp(state->config.model, "symbian"))
			channel = get_serial_channel(&bdaddr, 1);
		else
			channel = get_serial_channel(&bdaddr, 0);
	}
	dprintf("Channel: %d\n", channel);

	/* If none channel found, fail. */
	if (channel < 1) {
		fprintf(stderr, _("Cannot find any appropriate rfcomm channel and none was specified in the config.\n"));
		close(fd);
		return -1;
	}

	dprintf("Using channel: %d\n", channel);
	raddr.rc_channel = channel;
	
	if (connect(fd, (struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
		perror(_("Can't connect"));
		close(fd);
		return -1;
	}

	/* Ignore errors. If the socket was not set in the async way,
	 * we can live with that.
	 */
	setNonblocking(fd);

	return fd;
}

int bluetooth_close(int fd, struct gn_statemachine *state)
{
	sleep(2);
	return close(fd);
}

int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return write(fd, bytes, size);
}

int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return read(fd, bytes, size);
}

int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	return select(fd + 1, &readfds, NULL, NULL, timeout);
}

#endif	/* HAVE_BLUETOOTH_BLUEZ || HAVE_BLUETOOTH_NETGRAPH || HAVE_BLUETOOTH_NETBT */
