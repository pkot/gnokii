/*

  $Id$

  S M S D

  A Linux/Unix tool for the mobile phones.

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

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak

*/

#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include <pthread.h>
#include <glib.h>
#include "gnokii.h"

#define	MAX_CONSECUTIVE_ERRORS	5

typedef enum {
  Event_SendSMSMessage,
  Event_DeleteSMSMessage,
  Event_Exit
} PhoneAction;

typedef struct {
  PhoneAction event;
  gpointer    data;
} PhoneEvent;

typedef struct {
  gn_sms *sms;
  gn_error status;
} D_SMSMessage;

typedef struct {
  struct {
    const gchar *model;
    const gchar *revision;
    const gchar *version;
  } phone;
  struct {
//    gint    unRead;
    gint    number;
    GSList *messages;
  } sms;
  gint supported;
} PhoneMonitor;

extern pthread_t monitor_th;
extern PhoneMonitor phoneMonitor;
extern pthread_mutex_t smsMutex;
extern pthread_cond_t  smsCond;
extern pthread_mutex_t sendSMSMutex;
extern pthread_cond_t  sendSMSCond;
extern void InitPhoneMonitor (void);
extern void *Connect (void *);
extern void InsertEvent (PhoneEvent *event);

#endif
