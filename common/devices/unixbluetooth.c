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

#if defined(HAVE_BLUETOOTH_BLUEZ) || defined(HAVE_BLUETOOTH_NETGRAPH)

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#ifdef HAVE_BLUETOOTH_NETGRAPH	/* FreeBSD / netgraph */

#include <bitstring.h>
#include <netgraph/bluetooth/include/ng_hci.h>
#include <netgraph/bluetooth/include/ng_l2cap.h>
#include <netgraph/bluetooth/include/ng_btsocket.h>

#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
#define BDADDR_ANY NG_HCI_BDADDR_ANY
#define sockaddr_rc sockaddr_rfcomm
#define rc_family rfcomm_family
#define rc_bdaddr rfcomm_bdaddr
#define rc_channel rfcomm_channel
#define bacpy(dst, src) memcpy((dst), (src), sizeof(bdaddr_t))

#ifndef HAVE_BT_ATON

static int bt_aton(const char *str, bdaddr_t *ba)
{
	char ch;
	unsigned int b[6];

	memset(ba, 0, sizeof(*ba));
	if (sscanf(str, "%x:%x:%x:%x:%x:%x%c", b + 0, b + 1, b + 2, b + 3, b + 4, b + 5, &ch) != 6) return 0;
	if ((b[0] | b[1] | b[2] | b[3] | b[4] | b[5]) > 0xff) return 0;

	ba->b[0] = b[0];
	ba->b[1] = b[1];
	ba->b[2] = b[2];
	ba->b[3] = b[3];
	ba->b[4] = b[4];
	ba->b[5] = b[5];

	return 1;
}

#endif	/* HAVE_BT_ATON */

static int str2ba(const char *str, bdaddr_t *ba)
{
	return !bt_aton(str, ba);
}

#else	/* Linux / BlueZ support */

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#endif

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
	if (strcmp(name, "gnapplet") == 0) {
		if (only_gnapplet != 0)
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

static int get_serial_channel(bdaddr_t *device)
{
	bdaddr_t src;
	int channel;

	bacpy(&src, BDADDR_ANY);

	channel = find_service_channel(&src, device, 0, SERIAL_PORT_SVCLASS_ID);
	if (channel < 0)
		channel = find_service_channel(&src, device, 0, DIALUP_NET_SVCLASS_ID);

	return channel;
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
	if (channel < 1)
		channel = get_serial_channel(&bdaddr);
	dprintf("Channel: %d\n", channel);
	/* Let's fallback to default channel */
	if (channel < 1)
		channel = 1;
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

#endif	/* HAVE_BLUETOOTH_BLUEZ || HAVE_BLUETOOTH_NETGRAPH */
