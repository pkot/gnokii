/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sun Dec 17 2000
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_LOWLEVEL_H
#define XGNOKII_LOWLEVEL_H

#include <pthread.h>
#include <glib.h>
#include "gsm-common.h"

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
  GSM_SMSMessage *sms;
  GSM_Error status;
} D_SMSMessage;

typedef struct {
  bool working;
  struct {
    gchar *model;
    gchar *imei;
    gchar *revision;
    gchar *version;
  } phone;
  struct {
    gint    unRead;
    gint    number;
    GSList *messages;
  } sms;
  struct {
    bool callerGroups:1;
    bool sms:1;
    bool netMonitor:1;
    bool dtmf:1;
    bool speedDial:1;
    bool keyboard:1;
    bool calendar:1;
    bool data:1;
  } supported;
} PhoneMonitor;

extern pthread_t monitor_th;
extern PhoneMonitor phoneMonitor;
extern pthread_mutex_t smsMutex;
extern pthread_cond_t  smsCond;
extern pthread_mutex_t sendSMSMutex;
extern pthread_cond_t  sendSMSCond;
extern void InitPhoneMonitor (void);
extern void *Connect (void *a);
extern void InsertEvent (PhoneEvent *event);

#endif
