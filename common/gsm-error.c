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

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002      Pavel Machek
  Copyright (C) 2002-2004 BORBELY Zoltan

  Error codes.

*/

#include "config.h"
#include "compat.h"
#include "misc.h"
#include "gnokii.h"

GNOKII_API char *gn_error_print(gn_error e)
{
	switch (e) {
	case GN_ERR_NONE:                  return _("No error.");
	case GN_ERR_FAILED:                return _("Command failed.");
	case GN_ERR_UNKNOWNMODEL:          return _("Model specified isn't known/supported.");
	case GN_ERR_INVALIDSECURITYCODE:   return _("Invalid Security code.");
	case GN_ERR_INTERNALERROR:         return _("Problem occurred internal to model specific code.");
	case GN_ERR_NOTIMPLEMENTED:        return _("Command called isn't implemented in model.");
	case GN_ERR_NOTSUPPORTED:          return _("Function or connection type not supported by the phone or by the phone driver.");
	case GN_ERR_USERCANCELED:          return _("User aborted the action.");
	case GN_ERR_UNKNOWN:               return _("Unknown error - well better than nothing!!");
	case GN_ERR_MEMORYFULL:            return _("The specified memory is full.");
	case GN_ERR_NOLINK:                return _("Couldn't establish link with phone.");
	case GN_ERR_TIMEOUT:               return _("Command timed out.");
	case GN_ERR_TRYAGAIN:              return _("Try again.");
	case GN_ERR_WAITING:               return _("Waiting for the next part of the message.");
	case GN_ERR_NOTREADY:              return _("Device not ready.");
	case GN_ERR_BUSY:                  return _("Command is still being executed.");
	case GN_ERR_INVALIDLOCATION:       return _("The given memory location is invalid.");
	case GN_ERR_INVALIDMEMORYTYPE:     return _("Invalid type of memory.");
	case GN_ERR_EMPTYLOCATION:         return _("The given location is empty.");
	case GN_ERR_ENTRYTOOLONG:          return _("The given entry is too long.");
	case GN_ERR_WRONGDATAFORMAT:       return _("Data format is not valid.");
	case GN_ERR_INVALIDSIZE:           return _("Wrong size of the object.");
	case GN_ERR_LINEBUSY:              return _("Outgoing call requested reported line busy.");
	case GN_ERR_NOCARRIER:             return _("No Carrier error during data call setup?");
	case GN_ERR_UNHANDLEDFRAME:        return _("The current frame isn't handled by the incoming function.");
	case GN_ERR_UNSOLICITED:           return _("Unsolicited message received.");
	case GN_ERR_NONEWCBRECEIVED:       return _("Attempt to read CB when no new CB received.");
	case GN_ERR_SIMPROBLEM:            return _("SIM card missing or damaged.");
	case GN_ERR_CODEREQUIRED:          return _("PIN or PUK code required.");
	case GN_ERR_NOTAVAILABLE:          return _("The requested information is not available.");
	case GN_ERR_NOCONFIG:              return _("Config file cannot be read.");
	case GN_ERR_NOPHONE:               return _("Either global or given phone section cannot be found.");
	case GN_ERR_NOLOG:                 return _("Incorrect logging section configuration.");
	case GN_ERR_NOMODEL:               return _("No phone model specified in the config file.");
	case GN_ERR_NOPORT:                return _("No port specified in the config file.");
	case GN_ERR_NOCONNECTION:          return _("No connection type specified in the config file.");
	case GN_ERR_LOCKED:                return _("Device is locked and cannot be unlocked.");
	case GN_ERR_ASYNC:                 return _("The actual response will be sent asynchronously.");
	default:                           return _("Unknown error.");
	}
}

gn_error isdn_cause2gn_error(char **src, char **msg, unsigned char loc, unsigned char cause)
{
	char *s, *m;
	gn_error err = GN_ERR_UNKNOWN;

	if (src == NULL) src = &s;
	if (msg == NULL) msg = &m;

	switch (loc) {
	case 0x00: *src = "user"; break;
	case 0x01: *src = "private network serving the local user"; break;
	case 0x02: *src = "public network serving the local user"; break;
	case 0x03: *src = "transit network"; break;
	case 0x04: *src = "public network serving the remote user"; break;
	case 0x05: *src = "private network serving the remote user"; break;
	case 0x07: *src = "international network"; break;
	case 0x0a: *src = "network beyond inter-working point"; break;
	default:   *src = "unknown"; break;
	}

	switch (cause) {
	case 0x01: *msg = "Unallocated (unassigned) number"; break;
	case 0x02: *msg = "No route to specified transit network"; break;
	case 0x03: *msg = "No route to destination"; break;
	case 0x06: *msg = "Channel unacceptable"; break;
	case 0x07: *msg = "Call awarded and being delivered in an established channel"; break;
	case 0x10: *msg = "Normal call clearing"; break;
	case 0x11: *msg = "User busy"; break;
	case 0x12: *msg = "No user responding"; break;
	case 0x13: *msg = "No answer from user (user alerted)"; break;
	case 0x15: *msg = "Call rejected"; break;
	case 0x16: *msg = "Number changed"; break;
	case 0x1A: *msg = "Non-selected user clearing"; break;
	case 0x1B: *msg = "Destination out of order"; break;
	case 0x1C: *msg = "Invalid number format"; break;
	case 0x1D: *msg = "Facility rejected"; break;
	case 0x1E: *msg = "Response to status enquiry"; break;
	case 0x1F: *msg = "Normal, unspecified"; break;
	case 0x22: *msg = "No circuit or channel available"; break;
	case 0x26: *msg = "Network out of order"; break;
	case 0x29: *msg = "Temporary failure"; break;
	case 0x2A: *msg = "Switching equipment congestion"; break;
	case 0x2B: *msg = "Access information discarded"; break;
	case 0x2C: *msg = "Requested circuit or channel not available"; break;
	case 0x2F: *msg = "Resources unavailable, unspecified"; break;
	case 0x31: *msg = "Quality of service unavailable"; break;
	case 0x32: *msg = "Requested facility not subscribed"; break;
	case 0x39: *msg = "Bearer capability not authorised"; break;
	case 0x3A: *msg = "Bearer capability not presently available"; break;
	case 0x3F: *msg = "Service or option not available, unspecified"; break;
	case 0x41: *msg = "Bearer capability not implemented"; break;
	case 0x42: *msg = "Channel type not implemented"; break;
	case 0x45: *msg = "Requested facility not implemented"; break;
	case 0x46: *msg = "Only restricted digital information bearer"; break;
	case 0x4F: *msg = "Service or option not implemented, unspecified"; break;
	case 0x51: *msg = "Invalid call reference value"; break;
	case 0x52: *msg = "Identified channel does not exist"; break;
	case 0x53: *msg = "A suspended call exists, but this call identity does not"; break;
	case 0x54: *msg = "Call identity in use"; break;
	case 0x55: *msg = "No call suspended"; break;
	case 0x56: *msg = "Call having the requested call identity"; break;
	case 0x58: *msg = "Incompatible destination"; break;
	case 0x5B: *msg = "Invalid transit network selection"; break;
	case 0x5F: *msg = "Invalid message, unspecified"; break;
	case 0x60: *msg = "Mandatory information element is missing"; break;
	case 0x61: *msg = "Message type non-existent or not implemented"; break;
	case 0x62: *msg = "Message not compatible with call state or message or message type non existent or not implemented"; break;
	case 0x63: *msg = "Information element non-existent or not implemented"; break;
	case 0x64: *msg = "Invalid information element content"; break;
	case 0x65: *msg = "Message not compatible"; break;
	case 0x66: *msg = "Recovery on timer expiry"; break;
	case 0x6F: *msg = "Protocol error, unspecified"; break;
	case 0x7F: *msg = "Inter working, unspecified"; break;
	default:   *msg = "Unknown"; break;
	}

	dprintf("\tISDN cause: %02x %02x\n", loc, cause);
	dprintf("\tlocation: %s\n", *src);
	dprintf("\tcause: %s\n", *msg);

	return err;
}
