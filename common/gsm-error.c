/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Error codes.

*/

#include "gsm-error.h"

char *print_error(GSM_Error e)
{
	switch (e) {
	case GE_NONE:                     return "No error.";
	case GE_DEVICEOPENFAILED:         return "Couldn't open specified serial device.";
	case GE_UNKNOWNMODEL:             return "Model specified isn't known/supported.";
	case GE_NOLINK:                   return "Couldn't establish link with phone.";
	case GE_TIMEOUT:                  return "Command timed out.";
	case GE_TRYAGAIN:                 return "Try again.";
	case GE_INVALIDSECURITYCODE:      return "Invalid Security code.";
	case GE_NOTIMPLEMENTED:           return "Called command is not implemented for the used model.";
	case GE_INVALIDSMSLOCATION:       return "Invalid SMS location.";
	case GE_INVALIDPHBOOKLOCATION:    return "Invalid phonebook location.";
	case GE_INVALIDMEMORYTYPE:        return "Invalid type of memory.";
	case GE_INVALIDSPEEDDIALLOCATION: return "Invalid speed dial location.";
	case GE_INVALIDCALNOTELOCATION:   return "Invalid calendar note location.";
	case GE_INVALIDDATETIME:          return "Invalid date, time or alarm specification.";
	case GE_EMPTYSMSLOCATION:         return "SMS location is empty.";
	case GE_PHBOOKNAMETOOLONG:        return "Phonebook name is too long.";
	case GE_PHBOOKNUMBERTOOLONG:      return "Phonebook number is too long.";
	case GE_PHBOOKWRITEFAILED:        return "Phonebook write failed.";
	case GE_SMSSENDOK:                return "SMS was send correctly.";
	case GE_SMSSENDFAILED:            return "SMS send fail.";
	case GE_SMSWAITING:               return "Waiting for the next part of SMS.";
	case GE_SMSTOOLONG:               return "SMS message too long.";
	case GE_SMSWRONGFORMAT:           return "Wrong format of the SMS message.";
	case GE_NONEWCBRECEIVED:          return "Attempt to read CB when no new CB received";
	case GE_INTERNALERROR:            return "Problem occured internal to model specific code.";
	case GE_CANTOPENFILE:             return "Can't open file with bitmap/ringtone";
	case GE_WRONGNUMBEROFCOLORS:      return "Wrong number of colors in specified bitmap file";
	case GE_WRONGCOLORS:              return "Wrong colors in bitmap file";
	case GE_INVALIDFILEFORMAT:        return "Invalid format of file";
	case GE_SUBFORMATNOTSUPPORTED:    return "Subformat of file not supported";
	case GE_FILETOOSHORT:             return "Too short file to read";
	case GE_FILETOOLONG:              return "Too long file to read";
	case GE_INVALIDIMAGESIZE:         return "Invalid size of bitmap (in file, sms etc.)";
	case GE_NOTSUPPORTED:             return "Function not supported by the phone";
	case GE_BUSY:                     return "Command is still being executed.";
	case GE_USERCANCELED:             return "User has cancelled the action.";
	case GE_UNKNOWN:                  return "Unknown error - well better than nothing!!";
	case GE_MEMORYFULL:               return "Memory is full";
	case GE_NOTWAITING:               return "Not waiting for a response from the phone";
	case GE_NOTREADY:                 return "Device not ready.";
	case GE_EMPTYMEMORYLOCATION:      return "Empty memory location.";
	case GE_LINEBUSY:                 return "Outgoing call requested reported line busy";
	case GE_NOCARRIER:                return "No Carrier error during data call setup ?";
	case GE_UNHANDLEDFRAME:           return "Unhandled frame received, please see the log!";
	case GE_UNSOLICITED:              return "Unsolicited message received.";
	default:                          return "Unknown error.";
	}
}
