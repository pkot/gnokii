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
  Revision 1.1  2001-01-14 22:46:59  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#include <string.h>

#include "gsm-common.h"
#include "phone-generic.h"

/* These two functions provide a generic way of waiting for a response. */
/* The passed int refers to frame type for nokia but can be enumerated */
/*    -1 indicates 'not required' */
/*     0 means 'received' */
/* A generic message pointer is passed to enable resends */
/* Timeout is in 0.1seconds _and is approximate_ (see code) */

/* FIXME - should more parameters be passed??!!?? */
/* FIXME - tidy up + more comments */

GSM_Error PGEN_CommandResponse(GSM_Link *link, void *message, int *messagesize, int messagetype, int waitfor, int messagealloc)
{
  int retry;
  int time;
  int timeout=20; /* approx 2 secs */
  struct timeval loop_timeout;
  GSM_Error err=GE_NONE;

  link->CR_waitfor=waitfor;
  link->CR_message=message;
  link->CR_messagelength=messagealloc;
  if (link->CR_waitfor==0) link->CR_waitfor=-1;
  if (link->CR_error==0) link->CR_error=-1;

  /* Assume 3 retry attempts - do not retry on 'error' */

  for(retry=0;retry<3;retry++) {
    link->SendMessage(*messagesize, messagetype, message);
    time=timeout;
    while(time!=0) {
      loop_timeout.tv_sec=0;
      loop_timeout.tv_usec=100000;
      err=link->Loop(&loop_timeout); 
      if (err==GE_TIMEOUT) time--;
      if (err==GE_INTERNALERROR) time=0;
      if (link->CR_waitfor==0) {
	/* We've got the message .. */
	message=link->CR_message;
	*messagesize=link->CR_messagelength;
	link->CR_message=NULL;
	link->CR_messagelength=0;
	err=GE_NONE;
	time=0;
	retry=999; 
      }
      if (link->CR_error==0) {
	/* Hmm.. */
	err=GE_NOTIMPLEMENTED;
      }
    }
  }

  return err;
}


GSM_Error PGEN_CommandResponseReceive(GSM_Link *link, int MessageType, void *Message, int MessageLength)
{
  if (MessageType==link->CR_waitfor) {
    if (MessageLength <= link->CR_messagelength) {
      link->CR_waitfor=0;
      memcpy(link->CR_message,Message,MessageLength);
      link->CR_messagelength=MessageLength;
    }
    else link->CR_error=0;
  }
  
  return GE_NONE;
}


