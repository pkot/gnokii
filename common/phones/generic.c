/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides useful functions for all phones  
  See README for more details on supported mobile phones.

  The various routines are called PGEN_(whatever).

  $Log$
  Revision 1.2  2001-03-06 10:39:35  machek
  Function for printing unknown packets can probably be shared across
  all phones.

  Revision 1.1  2001/02/21 19:57:07  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/16 14:29:53  chris
  Restructure of common/.  Fixed a problem in fbus-phonet.c
  Lots of dprintfs for Marcin
  Any size xpm can now be loaded (eg for 7110 startup logos)
  nk7110 code detects 7110/6210 and alters startup logo size to suit
  Moved Marcin's extended phonebook code into gnokii.c

  Revision 1.4  2001/02/03 23:56:17  chris
  Start of work on irda support (now we just need fbus-irda.c!)
  Proper unicode support in 7110 code (from pkot)

  Revision 1.3  2001/01/29 17:14:42  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.2  2001/01/23 15:32:42  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.1  2001/01/14 22:46:59  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gsm-common.h"
#include "phones/generic.h"
#include "misc.h"

/* Useful debug function */
GSM_Error PGEN_DebugMessage(int type, unsigned char *mes, int len)
{
	int i;
  
	dprintf(_("Message debug (type %02x):\n\r"), type);
	for (i = 0; i < len; i++) 
		if (isprint(mes[i])) dprintf(_("[%02x%c]"), mes[i], mes[i]);
		else dprintf(_("[%02x ]"), mes[i]);
	dprintf(_("\n\r"));

	return GE_NONE;
}

/* These two functions provide a generic way of waiting for a response. */
/* The passed int refers to frame type for nokia but can be enumerated */
/*    -1 indicates 'not required' */
/*     0 means 'received' */
/* A generic message pointer is passed to enable resends */
/* Timeout is in 0.1seconds _and is approximate_ (see code) */
/* Messagetype 0 is reserved */
/* FIXME - should more parameters be passed??!!?? */
/* FIXME - tidy up + more comments */
GSM_Error PGEN_CommandResponse(GSM_Link *link, void *message, int *messagesize, int messagetype, int waitfor, int messagealloc)
{
	int retry;
	int time;
	int timeout = 20; /* approx 2 secs */
	struct timeval loop_timeout;
	GSM_Error err = GE_NONE;

	link->CR_waitfor = waitfor;
	link->CR_message = message;
	link->CR_messagelength = messagealloc;
	if (!link->CR_waitfor) link->CR_waitfor = -1;
	if (!link->CR_error) link->CR_error = -1;

	/* Assume 3 retry attempts - do not retry on 'error' */
	for (retry = 0; retry < 3; retry++) {
		link->SendMessage(*messagesize, messagetype, message);
		time = timeout;
		while (time) {
			loop_timeout.tv_sec = 0;
			loop_timeout.tv_usec = 100000;
			err = link->Loop(&loop_timeout); 
			if (err == GE_TIMEOUT) time--;
			if (err == GE_INTERNALERROR) time = 0;
			if (!link->CR_waitfor) {
				/* We've got the message .. */
				message = link->CR_message;
				*messagesize = link->CR_messagelength;
				link->CR_message = NULL;
				link->CR_messagelength = 0;
				err = GE_NONE;
				time = 0;
				retry = 999; 
			}
			if (!link->CR_error) {
				/* Hmm.. Maybe some debug info? */
				err = GE_NOTIMPLEMENTED;
			}
		}
	}

	return err;
}

GSM_Error PGEN_CommandResponseReceive(GSM_Link *link, int MessageType, void *Message, int MessageLength)
{
	if (!link->CR_waitfor) return GE_NOTWAITING;
  
	if (MessageType == link->CR_waitfor) {
		if (MessageLength <= link->CR_messagelength) {
			link->CR_waitfor = 0;
			memcpy(link->CR_message, Message, MessageLength);
			link->CR_messagelength = MessageLength;
		} else link->CR_error = 0;
	}
  
	return GE_NONE;
}

/* If we do not support a message type, print out some debugging info */

GSM_Error PGEN_IncomingDefault(int messagetype, unsigned char *buffer, int length)
{
	dprintf("Unknown Message received [type (%02x) length (%d): \n\r", messagetype, length);
	PGEN_DebugMessage(messagetype, buffer, length);

	return GE_NONE;
}
