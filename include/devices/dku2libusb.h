/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002       Marcel Holtmann <marcel@holtmann.org>
  Copyright (C) 2005       Alex Kanavin
  Copyright (C) 2006       Pawel Kot

*/

#ifndef _gnokii_dku2libusb_h
#define _gnokii_dku2libusb_h

#include "compat.h"
#include "misc.h"
#include "gnokii.h"

#ifdef HAVE_LIBUSB

#include <usb.h>

/* Information about a USB DKU2 FBUS interface present on the system */
struct fbus_usb_interface_transport {
	struct fbus_usb_interface_transport *prev, *next;	/* Next and previous interfaces in the list */
	struct usb_device *device;		/* USB device that has the interface */
	int configuration;			/* Device configuration */
	int configuration_description;		/* Configuration string descriptor number */
	int control_interface;			/* DKU2 FBUS master interface */
	int control_setting;			/* DKU2 FBUS master interface setting */
	int control_interface_description;	/* DKU2 FBUS master interface string descriptor number
						 * If non-zero, use usb_get_string_simple() from
						 * libusb to retrieve human-readable description
						 */
	int data_interface;			/* DKU2 FBUS data/slave interface */
	int data_idle_setting;			/* DKU2 FBUS data/slave idle setting */
	int data_interface_idle_description;	/* DKU2 FBUS data/slave interface string descriptor number
						 * in idle setting */
	int data_active_setting;		/* DKU2 FBUS data/slave active setting */
	int data_interface_active_description;	/* DKU2 FBUS data/slave interface string descriptor number
						 * in active setting */
	int data_endpoint_read;			/* DKU2 FBUS data/slave interface read endpoint */
	int data_endpoint_write;		/* DKU2 FBUS data/slave interface write endpoint */
	usb_dev_handle *dev_control;		/* libusb handler for control interace */
	usb_dev_handle *dev_data;		/* libusb handler for data interface */
};

/* USB-specific FBUS interface information */
typedef struct {
	/* Manufacturer, e.g. Nokia */
	char *manufacturer;
	/* Product, e.g. Nokia 6680 */
	char *product;
	/* Product serial number */
	char *serial;
	/* USB device configuration description */
	char *configuration;
	/* Control interface description */
	char *control_interface;
	/* Idle data interface description, typically empty */
	char *data_interface_idle;
	/* Active data interface description, typically empty */
	char *data_interface_active;
	/* Internal information for the transport layer in the library */
	struct fbus_usb_interface_transport *interface;
} fbus_usb_interface;

/* "Union Functional Descriptor" from CDC spec 5.2.3.X
 * used to find data/slave DKU2 FBUS interface */
#pragma pack(1)
struct cdc_union_desc {
	u_int8_t      bLength;
	u_int8_t      bDescriptorType;
	u_int8_t      bDescriptorSubType;

	u_int8_t      bMasterInterface0;
	u_int8_t      bSlaveInterface0;
};
#pragma pack()

/* Nokia is the vendor we are interested in */
#define NOKIA_VENDOR_ID	0x0421

/* CDC class and subclass types */
#define USB_CDC_CLASS			0x02
#define USB_CDC_FBUS_SUBCLASS		0xfe

/* class and subclass specific descriptor types */
#define CDC_HEADER_TYPE			0x00
#define CDC_UNION_TYPE			0x06
#define CDC_FBUS_TYPE			0x15

/* Interface descriptor */
#define USB_DT_CS_INTERFACE		0x24

#define USB_MAX_STRING_SIZE		256
#define USB_FBUS_TIMEOUT		10000 /* 10 seconds */

int fbusdku2usb_open(struct gn_statemachine *state);
int fbusdku2usb_close(struct gn_statemachine *state);
int fbusdku2usb_write(const __ptr_t bytes, int size, struct gn_statemachine *state);
int fbusdku2usb_read(__ptr_t bytes, int size, struct gn_statemachine *state);
int fbusdku2usb_select(struct timeval *timeout, struct gn_statemachine *state);

#else

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

#endif

#endif /* _gnokii_dku2libusb_h */
