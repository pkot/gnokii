/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sun Dec 17 2000
  Modified by Jan Derfinak

*/

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "fbus-6110.h"
#include "fbus-3810.h"
#include "smsd.h"
#include "lowlevel.h"

pthread_t monitor_th;
PhoneMonitor phoneMonitor;
pthread_mutex_t smsMutex;
pthread_cond_t  smsCond;
pthread_mutex_t sendSMSMutex;
pthread_cond_t  sendSMSCond;
static pthread_mutex_t eventsMutex;
static GSList *ScheduledEvents = NULL;


inline void InsertEvent (PhoneEvent *event)
{
# ifdef XDEBUG
  g_print ("Inserting Event: %d\n", event->event);
# endif
  pthread_mutex_lock (&eventsMutex);
  ScheduledEvents = g_slist_prepend (ScheduledEvents, event);
  pthread_mutex_unlock (&eventsMutex);
}


inline static PhoneEvent *RemoveEvent (void)
{
  GSList *list;
  PhoneEvent *event = NULL;

  pthread_mutex_lock (&eventsMutex);
  list = g_slist_last (ScheduledEvents);
  if (list)
  {
    event = (PhoneEvent *) list->data;
    ScheduledEvents = g_slist_remove_link (ScheduledEvents, list);
    g_slist_free_1 (list);
  }
  pthread_mutex_unlock (&eventsMutex);

  return (event);
}


static void InitModelInf (void)
{
  gchar buf[64];
  GSM_Error error;
  register gint i = 0;

  while ((error = GSM->GetModel(buf)) != GE_NONE && i++ < 15)
    sleep(1);

  if (error == GE_NONE)
  {
    g_free (phoneMonitor.phone.model);
    phoneMonitor.phone.version = g_strdup (buf);
    phoneMonitor.phone.model = GetModel (buf);
    if (phoneMonitor.phone.model == NULL)
      phoneMonitor.phone.model = g_strdup (_("unknown"));

    phoneMonitor.supported.callerGroups = CallerGroupSupported (buf);
    phoneMonitor.supported.netMonitor = NetmonitorSupported (buf);
    phoneMonitor.supported.sms = SMSSupported (buf);
    phoneMonitor.supported.dtmf = DTMFSupported (buf);
    phoneMonitor.supported.speedDial = SpeedDialSupported (buf);
    phoneMonitor.supported.keyboard = KeyboardSupported (buf);
    phoneMonitor.supported.calendar = CalendarSupported (buf);
    phoneMonitor.supported.data = DataSupported (buf);
  }

  i = 0;
  while ((error = GSM->GetRevision (buf)) != GE_NONE && i++ < 5)
    sleep(1);

  if (error == GE_NONE)
  {
    g_free (phoneMonitor.phone.revision);
    phoneMonitor.phone.revision = g_strdup (buf);
  }

  i = 0;
  while ((error = GSM->GetIMEI (buf)) != GE_NONE && i++ < 5)
    sleep(1);

  if (error == GE_NONE)
  {
    g_free (phoneMonitor.phone.imei);
    phoneMonitor.phone.imei = g_strdup (buf);
  }


#ifdef XDEBUG
  g_print ("Version: %s\n", phoneMonitor.phone.version);
  g_print ("Model: %s\n", phoneMonitor.phone.model);
  g_print ("IMEI: %s\n", phoneMonitor.phone.imei);
  g_print ("Revision: %s\n", phoneMonitor.phone.revision);
#endif
}


static GSM_Error fbusinit(bool enable_monitoring)
{
  int count=0;
  static GSM_Error error=GE_NOLINK;
  GSM_ConnectionType connection=GCT_Serial;

  if (!strcmp(smsdConfig.connection, "infrared"))
    connection = GCT_Infrared;

  /* Initialise the code for the GSM interface. */     

  if (error == GE_NOLINK)
    error = GSM_Initialise (smsdConfig.model, smsdConfig.port,
                            smsdConfig.initlength, connection, RLP_DisplayF96Frame);

#ifdef XDEBUG
  g_print ("fbusinit: error %d\n", error);
#endif

  if (error != GE_NONE) {
    g_print (_("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    return (error);
  }

  while (count++ < 40 && *GSM_LinkOK == false)
    usleep(50000);
#ifdef XDEBUG
  g_print("After usleep. GSM_LinkOK: %d\n", *GSM_LinkOK);
#endif

  if (*GSM_LinkOK == true)
    InitModelInf ();

  return *GSM_LinkOK;
}


void InitPhoneMonitor (void)
{
  phoneMonitor.phone.model = g_strdup (_("unknown"));
  phoneMonitor.phone.version = phoneMonitor.phone.model;
  phoneMonitor.phone.revision = g_strdup (_("unknown"));
  phoneMonitor.phone.imei = g_strdup (_("unknown"));
  phoneMonitor.supported.callerGroups = FALSE;
  phoneMonitor.supported.netMonitor = FALSE;
  phoneMonitor.supported.sms = FALSE;
  phoneMonitor.supported.dtmf = FALSE;
  phoneMonitor.supported.speedDial = FALSE;
  phoneMonitor.supported.keyboard = FALSE;
  phoneMonitor.supported.calendar = FALSE;
  phoneMonitor.supported.data = FALSE;
  phoneMonitor.working = FALSE;
  phoneMonitor.sms.unRead = phoneMonitor.sms.number = 0;
  phoneMonitor.sms.messages = NULL;
  pthread_mutex_init (&smsMutex, NULL);
  pthread_cond_init (&smsCond, NULL);
  pthread_mutex_init (&sendSMSMutex, NULL);
  pthread_cond_init (&sendSMSCond, NULL);
  pthread_mutex_init (&eventsMutex, NULL);
}


static inline void FreeElement (gpointer data, gpointer userData)
{
  g_free ((GSM_SMSMessage *) data);
}


static inline void FreeArray (GSList **array)
{
  if (*array)
  {
    g_slist_foreach (*array, FreeElement, NULL);
    g_slist_free (*array);
    *array = NULL;
  }
}


static void RefreshSMS (const gint number)
{
  GSM_Error error;
  GSM_SMSMessage *msg;
  register gint i;

# ifdef XDEBUG
  g_print ("RefreshSMS is running...\n");
# endif

  pthread_mutex_lock (&smsMutex);
  FreeArray (&(phoneMonitor.sms.messages));
  phoneMonitor.sms.number = 0;
//  pthread_mutex_unlock (&smsMutex);

  i = 0;
  while (1)
  {
    msg = g_malloc (sizeof (GSM_SMSMessage));
    msg->MemoryType = GMT_SM;
    msg->Location = ++i;

    if ((error = GSM->GetSMSMessage (msg)) == GE_NONE)
    {
  //    pthread_mutex_lock (&smsMutex);
      phoneMonitor.sms.messages = g_slist_append (phoneMonitor.sms.messages, msg);
      phoneMonitor.sms.number++;
  //    pthread_mutex_unlock (&smsMutex);
      if (phoneMonitor.sms.number == number)
      {
        pthread_cond_signal (&smsCond);
        pthread_mutex_unlock (&smsMutex);
        return;
      }
    }
    else if (error == GE_INVALIDSMSLOCATION)  /* All positions are readed */
    {
      g_free (msg);
      pthread_cond_signal (&smsCond);
      pthread_mutex_unlock (&smsMutex);
      break;
    }
    else
      g_free (msg);

    usleep (750000);
  }
}


static gint A_SendSMSMessage (gpointer data)
{
  D_SMSMessage *d = (D_SMSMessage *) data;
  GSM_Error error;

  error = d->status = GE_UNKNOWN;
  if (d)
  {
    pthread_mutex_lock (&sendSMSMutex);
    error = d->status = GSM->SendSMSMessage (d->sms, 0);
    pthread_cond_signal (&sendSMSCond);
    pthread_mutex_unlock (&sendSMSMutex);
  }

  if (d->status == GE_SMSSENDOK)
    return (GE_NONE);
  else
    return (error);
}


static gint A_DeleteSMSMessage (gpointer data)
{
  GSM_SMSMessage *sms = (GSM_SMSMessage *) data;
  GSM_Error error = GE_UNKNOWN;

  if (sms)
  {
    error = GSM->DeleteSMSMessage(sms);
//    I don't use copy, I don't need free message.
//    g_free (sms);
  }

  return (error);
}


static gint A_Exit (gpointer data)
{
  pthread_exit (0);
  return (0); /* just to be proper */
}


gint (*DoAction[])(gpointer) = {
  A_SendSMSMessage,
  A_DeleteSMSMessage,
  A_Exit
};


void *Connect (void *a)
{
  GSM_SMSStatus SMSStatus = {0, 0};
  PhoneEvent *event;
  GSM_Error error;


# ifdef XDEBUG
  g_print ("Initializing connection...\n");
# endif

  while (!fbusinit (true))
    sleep (1);

# ifdef XDEBUG
  g_print ("Phone connected. Starting monitoring...\n");
# endif

  while (1)
  {
    phoneMonitor.working = FALSE;

    if (GSM->GetSMSStatus (&SMSStatus) == GE_NONE)
    {
      if (phoneMonitor.sms.unRead != SMSStatus.UnRead ||
          phoneMonitor.sms.number != SMSStatus.Number)
      {
        phoneMonitor.working = TRUE;
        RefreshSMS (SMSStatus.Number);
        phoneMonitor.working = FALSE;
      }

      phoneMonitor.sms.unRead = SMSStatus.UnRead;
    }

    while ((event = RemoveEvent ()) != NULL)
    {
#     ifdef XDEBUG      
      g_print ("Processing Event: %d\n", event->event);
#     endif
      phoneMonitor.working = TRUE;
      if (event->event <= Event_Exit)
        if ((error = DoAction[event->event] (event->data)) != GE_NONE)
          g_print (_("Event %d failed with return code %d!\n"), event->event, error);
      g_free (event);
    }
  }
}
