/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Error codes.

*/

#ifndef __gsm_error_h_
#define __gsm_error_h_

#include "compat.h"

/* Define standard GSM error/return code values. These codes are also used for
   some internal functions such as SIM read/write in the model specific code. */

typedef enum {
	/* General codes */
	GE_NONE = 0,              /* No error. */
	GE_FAILED,                /* Command failed. */
	GE_UNKNOWNMODEL,          /* Model specified isn't known/supported. */
	GE_INVALIDSECURITYCODE,   /* Invalid Security code. */
	GE_INTERNALERROR,         /* Problem occured internal to model specific code. */
	GE_NOTIMPLEMENTED,        /* Command called isn't implemented in model. */
	GE_NOTSUPPORTED,          /* Function not supported by the phone */
	GE_USERCANCELED,          /* User aborted the action. */
	GE_UNKNOWN,               /* Unknown error - well better than nothing!! */
	GE_MEMORYFULL,            /* The specified memory is full. */

	/* Statemachine */
	GE_NOLINK,                /* Couldn't establish link with phone. */
	GE_TIMEOUT,               /* Command timed out. */
	GE_TRYAGAIN,              /* Try again. */
	GE_WAITING,               /* Waiting for the next part of the message. */
	GE_NOTREADY,              /* Device not ready. */
	GE_BUSY,                  /* Command is still being executed. */
	
	/* Locations */
	GE_INVALIDLOCATION,       /* The given memory location is empty. */
	GE_INVALIDMEMORYTYPE,     /* Invalid type of memory. */
	GE_EMPTYLOCATION,         /* The given location is empty. */

	/* Format */
	GE_ENTRYTOOLONG,          /* The given entry is too long */
	GE_WRONGDATAFORMAT,       /* Data format is not valid */
	GE_INVALIDSIZE,           /* Wrong size of the object */

	/* The following are here in anticipation of data call requirements. */
	GE_LINEBUSY,              /* Outgoing call requested reported line busy */
	GE_NOCARRIER,             /* No Carrier error during data call setup ? */

	/* The following value signals the current frame is unhandled */
	GE_UNHANDLEDFRAME,        /* The current frame isn't handled by the incoming function */
	GE_UNSOLICITED,           /* Unsolicited message received. */

	/* Other */
	GE_NONEWCBRECEIVED        /* Attempt to read CB when no new CB received */
} GSM_Error;

API char *print_error(GSM_Error e);
API GSM_Error ISDNCauseToGSMError(char **src, char **msg, unsigned char loc, unsigned char cause);

#endif
