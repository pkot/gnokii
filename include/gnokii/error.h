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
  Copyright (C) 2001-2004 BORBELY Zoltan, Pawel Kot

  Error codes.

*/

#ifndef _gnokii_error_h
#define _gnokii_error_h

/* 
 * Define standard GSM error/return code values. These codes are also used for
 * some internal functions such as SIM read/write in the model specific code. 
 */
typedef enum {
	/* General codes */
	GN_ERR_NONE = 0,		/* No error. */
	GN_ERR_FAILED,			/* Command failed. */
	GN_ERR_UNKNOWNMODEL,		/* Model specified isn't known/supported. */
	GN_ERR_INVALIDSECURITYCODE,	/* Invalid Security code. */
	GN_ERR_INTERNALERROR,		/* Problem occured internal to model specific code. */
	GN_ERR_NOTIMPLEMENTED,		/* Command called isn't implemented in model. */
	GN_ERR_NOTSUPPORTED,		/* Function not supported by the phone */
	GN_ERR_USERCANCELED,		/* User aborted the action. */
	GN_ERR_UNKNOWN,			/* Unknown error - well better than nothing!! */
	GN_ERR_MEMORYFULL,		/* The specified memory is full. */

	/* Statemachine */
	GN_ERR_NOLINK,			/* Couldn't establish link with phone. */
	GN_ERR_TIMEOUT,			/* Command timed out. */
	GN_ERR_TRYAGAIN,		/* Try again. */
	GN_ERR_WAITING,			/* Waiting for the next part of the message. */
	GN_ERR_NOTREADY,		/* Device not ready. */
	GN_ERR_BUSY,			/* Command is still being executed. */
	
	/* Locations */
	GN_ERR_INVALIDLOCATION,		/* The given memory location has not valid location. */
	GN_ERR_INVALIDMEMORYTYPE,	/* Invalid type of memory. */
	GN_ERR_EMPTYLOCATION,		/* The given location is empty. */

	/* Format */
	GN_ERR_ENTRYTOOLONG, 		/* The given entry is too long */
	GN_ERR_WRONGDATAFORMAT,		/* Data format is not valid */
	GN_ERR_INVALIDSIZE,		/* Wrong size of the object */

	/* The following are here in anticipation of data call requirements. */
	GN_ERR_LINEBUSY,		/* Outgoing call requested reported line busy */
	GN_ERR_NOCARRIER,		/* No Carrier error during data call setup ? */

	/* The following value signals the current frame is unhandled */
	GN_ERR_UNHANDLEDFRAME,		/* The current frame isn't handled by the incoming function */
	GN_ERR_UNSOLICITED,		/* Unsolicited message received. */

	/* Other */
	GN_ERR_NONEWCBRECEIVED,		/* Attempt to read CB when no new CB received */
	GN_ERR_SIMPROBLEM,		/* SIM card missing or damaged */
	GN_ERR_CODEREQUIRED,		/* PIN or PUK code required */
	GN_ERR_NOTAVAILABLE,		/* The requested information is not available */

	/* Config */
	GN_ERR_NOCONFIG,		/* Config file cannot be found */
	GN_ERR_NOPHONE,			/* Either global or given phone section cannot be found */
	GN_ERR_NOLOG,			/* Incorrect logging section configuration */
	GN_ERR_NOMODEL,			/* No phone model specified */
	GN_ERR_NOPORT,			/* No port specified */
	GN_ERR_NOCONNECTION,		/* No connection type specified */
	GN_ERR_LOCKED,			/* Device is locked and cannot unlock */

	GN_ERR_ASYNC			/* The actual response will be sent asynchronously */
} gn_error;

GNOKII_API char *gn_error_print(gn_error e);

#endif /* _gnokii_error_h */
