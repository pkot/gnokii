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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002       Ladis Michl, Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2003       BORBELY Zoltan
  Copyright (C) 2005       Alex Kanavin
  Copyright (C) 2003-2006  Pawel Kot

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "devices/dku2libusb.h"

#ifndef HAVE_LIBUSB
int fbusdku2usb_open(struct gn_statemachine *state)
{
	return -1;
}

int fbusdku2usb_close(struct gn_statemachine *state)
{
	return -1;
}

int fbusdku2usb_write(const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return -1;
}

int fbusdku2usb_read(__ptr_t bytes, int size, struct gn_statemachine *state)
{
	return -1;
}

int fbusdku2usb_select(struct timeval *timeout, struct gn_statemachine *state)
{
	return -1;
}

#else

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define	DEVINSTANCE(s) (*((fbus_usb_interface **)(&(s)->device.device_instance)))

/*
 * Helper function to usbfbus_find_interfaces 
 */
static void find_eps(struct fbus_usb_interface_transport *iface,
		     struct usb_interface_descriptor data_iface,
		     int *found_active, int *found_idle)
{
	struct usb_endpoint_descriptor *ep0, *ep1;

	if (data_iface.bNumEndpoints == 2) {
		ep0 = data_iface.endpoint;
		ep1 = data_iface.endpoint + 1;
		if ((ep0->bEndpointAddress & USB_ENDPOINT_IN) && 
		    ((ep0->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) && 
		    !(ep1->bEndpointAddress & USB_ENDPOINT_IN) && 
		    ((ep1->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)) {
			*found_active = 1;
			iface->data_active_setting = data_iface.bAlternateSetting;
			iface->data_interface_active_description = data_iface.iInterface;
			iface->data_endpoint_read = ep0->bEndpointAddress;
			iface->data_endpoint_write = ep1->bEndpointAddress;
		}
		if (!(ep0->bEndpointAddress & USB_ENDPOINT_IN) && 
		    ((ep0->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) && 
		    (ep1->bEndpointAddress & USB_ENDPOINT_IN) && 
		    ((ep1->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)) {
			*found_active = 1;
			iface->data_active_setting = data_iface.bAlternateSetting;
			iface->data_interface_active_description = data_iface.iInterface;
			iface->data_endpoint_read = ep1->bEndpointAddress;
			iface->data_endpoint_write = ep0->bEndpointAddress;
		}
	}
	if (data_iface.bNumEndpoints == 0) {
		*found_idle = 1;
		iface->data_idle_setting = data_iface.bAlternateSetting;
		iface->data_interface_idle_description = data_iface.iInterface;
	}
}

/*
 * Helper function to usbfbus_find_interfaces 
 */
static int find_fbus_data_interface(unsigned char *buffer, int buflen,
				    struct usb_config_descriptor config,
				    struct fbus_usb_interface_transport *iface)
{
	struct cdc_union_desc *union_header = NULL;
	int i, a;
	int found_active = 0;
	int found_idle = 0;

	if (!buffer) {
		dprintf("Weird descriptor references\n");
		return -EINVAL;
	}
	while (buflen > 0) {
		if (buffer [1] != USB_DT_CS_INTERFACE) {
			dprintf("skipping garbage\n");
			goto next_desc;
		}
		switch (buffer [2]) {
		case CDC_UNION_TYPE: /* we've found it */
			if (union_header) {
				dprintf("More than one union descriptor, skiping ...\n");
				goto next_desc;
			}
			union_header = (struct cdc_union_desc *)buffer;
			break;
		case CDC_FBUS_TYPE: /* maybe check version */
		case CDC_HEADER_TYPE:
			break; /* for now we ignore it */
		default:
			dprintf("Ignoring extra header, type %d, length %d\n", buffer[2], buffer[0]);
			break;
		}
next_desc:
		buflen -= buffer[0];
		buffer += buffer[0];
	}
	if (!union_header) {
		dprintf("No union descriptor, giving up\n");
		return -ENODEV;
	}
	/* Found the slave interface, now find active/idle settings and endpoints */
	iface->data_interface = union_header->bSlaveInterface0;
	/* Loop through all of the interfaces */
	for (i = 0; i < config.bNumInterfaces; i++) {
		/* Loop through all of the alternate settings */
		for (a = 0; a < config.interface[i].num_altsetting; a++) {
			/* Check if this interface is DKU2 FBUS data interface */
			/* and find endpoints */
			if (config.interface[i].altsetting[a].bInterfaceNumber == iface->data_interface) {
				find_eps(iface, config.interface[i].altsetting[a], &found_active, &found_idle);
			}
		}
	}
	if (!found_idle) {
		dprintf("No idle setting\n");
		return -ENODEV;
	}
	if (!found_active) {
		dprintf("No active setting\n");
		return -ENODEV;
	}
	dprintf("Found FBUS interface\n");
	return 0;
}

/*
 * Helper function to usbfbus_find_interfaces
 */
static int get_iface_string(struct usb_dev_handle *usb_handle, char **string, int id)
{
	if (id) {
		if ((*string = malloc(USB_MAX_STRING_SIZE)) == NULL)
			return -ENOMEM;
		*string[0] = '\0';
		return usb_get_string_simple(usb_handle, id, *string, USB_MAX_STRING_SIZE);
	}
	return 0;
}

/*
 * Helper function to usbfbus_find_interfaces 
 */
static struct fbus_usb_interface_transport *check_iface(struct usb_device *dev, int c, int i, int a,
					      struct fbus_usb_interface_transport *current)
{
	struct fbus_usb_interface_transport *next = NULL;

	if ((dev->config[c].interface[i].altsetting[a].bInterfaceClass == USB_CDC_CLASS)
	    && (dev->config[c].interface[i].altsetting[a].bInterfaceSubClass == USB_CDC_FBUS_SUBCLASS)) {
		int err;
		unsigned char *buffer = dev->config[c].interface[i].altsetting[a].extra;
		int buflen = dev->config[c].interface[i].altsetting[a].extralen;

		next = malloc(sizeof(struct fbus_usb_interface_transport));
		if (next == NULL)
			return current;
		next->device = dev;
		next->configuration = dev->config[c].bConfigurationValue;
		next->configuration_description = dev->config[c].iConfiguration;
		next->control_interface = dev->config[c].interface[i].altsetting[a].bInterfaceNumber;
		next->control_interface_description = dev->config[c].interface[i].altsetting[a].iInterface;
		next->control_setting = dev->config[c].interface[i].altsetting[a].bAlternateSetting;

		err = find_fbus_data_interface(buffer, buflen, dev->config[c], next);
		if (err)
			free(next);
		else {
			if (current)
				current->next = next;
			next->prev = current;
			next->next = NULL;
			current = next;
		}
	}
	return current;
}

/*
 * Function usbfbus_find_interfaces ()
 *
 *    Find available USB DKU2 FBUS interfaces on the system
 */
static int usbfbus_find_interfaces(struct gn_statemachine *state)
{
	struct usb_bus *busses;
	struct usb_bus *bus;
	struct usb_device *dev;
	int c, i, a, retval = 0;
	struct fbus_usb_interface_transport *current = NULL;
	struct fbus_usb_interface_transport *tmp = NULL;
	struct usb_dev_handle *usb_handle;
	int n;

	/* For connection type dku2libusb port denotes number of DKU2 device */
	n = atoi(state->config.port_device);
	/* Assume default is first interface */
	if (n < 1) {
		n = 1;
		dprintf("port = %s is not valid for connection = dku2libusb using port = %d instead\n", state->config.port_device, n);
	}

	usb_init();
	usb_find_busses();
	usb_find_devices();

	busses = usb_get_busses();

	for (bus = busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == NOKIA_VENDOR_ID) {
				/* Loop through all of the configurations */
				for (c = 0; c < dev->descriptor.bNumConfigurations; c++) {
					/* Loop through all of the interfaces */
					for (i = 0; i < dev->config[c].bNumInterfaces; i++) {
						/* Loop through all of the alternate settings */
						for (a = 0; a < dev->config[c].interface[i].num_altsetting; a++) {
							/* Check if this interface is DKU2 FBUS */
							/* and find data interface */
							current = check_iface(dev, c, i, a, current);
						}
					}
				}
			}
		}
	}

	/* rewind */
	while (current && current->prev)
		current = current->prev;

	/* Take N-th device on the list */
	while (--n && current) {
		tmp = current; 
		current = current->next;
		/* free the previous element on the list -- won't be needed anymore */
		free(tmp);
	}

	if (current) {
		int s = sizeof(fbus_usb_interface);
		state->device.device_instance = calloc(1, s);
		if (!DEVINSTANCE(state))
			goto cleanup_list;

		DEVINSTANCE(state)->interface = current;
		usb_handle = usb_open(current->device);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->manufacturer, 
			current->device->descriptor.iManufacturer);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->product, 
			current->device->descriptor.iProduct);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->serial, 
			current->device->descriptor.iSerialNumber);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->configuration, 
			current->configuration_description);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->control_interface, 
			current->control_interface_description);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->data_interface_idle, 
			current->data_interface_idle_description);
		get_iface_string(usb_handle, &DEVINSTANCE(state)->data_interface_active, 
			current->data_interface_active_description);
		usb_close(usb_handle);
		retval = 1;
		current = current->next;
	}

cleanup_list:
	while (current) {
		tmp = current->next;
		free(current);
		current = tmp;
	}
	return retval;
}

/*
 * Function usbfbus_free_interfaces ()
 *
 *    Free the list of discovered USB DKU2 FBUS interfaces on the system
 */
static void usbfbus_free_interfaces(fbus_usb_interface *iface)
{
	if (iface == NULL)
		return;
	free(iface->manufacturer);
	free(iface->product);
	free(iface->serial);
	free(iface->configuration);
	free(iface->control_interface);
	free(iface->data_interface_idle);
	free(iface->data_interface_active);
	free(iface->interface);
	free(iface);
}

/*
 * Function usbfbus_connect_request (self)
 *
 *    Open the USB connection
 *
 */
static int usbfbus_connect_request(struct gn_statemachine *state)
{
	int ret;

	DEVINSTANCE(state)->interface->dev_data = usb_open(DEVINSTANCE(state)->interface->device);

	ret = usb_set_configuration(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->configuration);
	if (ret < 0) {
		dprintf("Can't set configuration: %d\n", ret);
	}

	ret = usb_claim_interface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->control_interface);
	if (ret < 0) {
		dprintf("Can't claim control interface: %d\n", ret);
		goto err1;
	}

	ret = usb_set_altinterface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->control_setting);
	if (ret < 0) {
		dprintf("Can't set control setting: %d\n", ret);
		goto err2;
	}

	ret = usb_claim_interface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->data_interface);
	if (ret < 0) {
		dprintf("Can't claim data interface: %d\n", ret);
		goto err2;
	}

	ret = usb_set_altinterface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->data_active_setting);
	if (ret < 0) {
		dprintf("Can't set data active setting: %d\n", ret);
		goto err3;
	}
	return 1;

err3:
	usb_release_interface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->data_interface);	
err2:
	usb_release_interface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->control_interface);
err1:
	usb_close(DEVINSTANCE(state)->interface->dev_data);
	return 0;
}

/*
 * Function usbfbus_link_disconnect_request (self)
 *
 *    Shutdown the USB link
 *
 */
static int usbfbus_disconnect_request(struct gn_statemachine *state)
{
	int ret;

	if (state->device.fd < 0)
		return 0;
	ret = usb_set_altinterface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->data_idle_setting);
	if (ret < 0)
		dprintf("Can't set data idle setting %d\n", ret);
	ret = usb_release_interface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->data_interface);
	if (ret < 0) 
		dprintf("Can't release data interface %d\n", ret);
	ret = usb_release_interface(DEVINSTANCE(state)->interface->dev_data, DEVINSTANCE(state)->interface->control_interface);
	if (ret < 0) 
		dprintf("Can't release control interface %d\n", ret);
	ret = usb_close(DEVINSTANCE(state)->interface->dev_data);
	if (ret < 0)
		dprintf("Can't close data interface %d\n", ret);
	return ret;	
}

int fbusdku2usb_open(struct gn_statemachine *state)
{
	int retval;

	retval = usbfbus_find_interfaces(state);
	if (retval)
		retval = usbfbus_connect_request(state);
	return (retval ? retval : -1);
}

int fbusdku2usb_close(struct gn_statemachine *state)
{
	usbfbus_disconnect_request(state);
	usbfbus_free_interfaces(DEVINSTANCE(state));
	state->device.device_instance = NULL;
	return 0;
}

int fbusdku2usb_write(const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	return usb_bulk_write(DEVINSTANCE(state)->interface->dev_data,
		DEVINSTANCE(state)->interface->data_endpoint_write,
		(char *) bytes, size, USB_FBUS_TIMEOUT);
}

int fbusdku2usb_read(__ptr_t bytes, int size, struct gn_statemachine *state)
{
	return usb_bulk_read(DEVINSTANCE(state)->interface->dev_data,
		DEVINSTANCE(state)->interface->data_endpoint_read,
		(char *) bytes, size, USB_FBUS_TIMEOUT);
}

int fbusdku2usb_select(struct timeval *timeout, struct gn_statemachine *state)
{
	return 1;
}

#endif
