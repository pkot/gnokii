/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id$
  
*/

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "phones/nk7110.h"
#include "phones/nk6510.h"
#include "phones/nk6100.h"
#include "phones/nk3110.h"
#include "phones/nk2110.h"
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
static GSM_Statemachine sm;

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


static GSM_Error InitModelInf (void)
{
  GSM_Data data;
  gchar buf[64];
  GSM_Error error;
  char model[64], rev[64], manufacturer[64];

  data.Manufacturer = manufacturer;
  data.Model = model;
  data.Revision = rev;
                          
  error = SM_Functions (GOP_GetModel, &data, &sm);
  if (error != GE_NONE)
    return error;
    
  g_free (phoneMonitor.phone.model);
  phoneMonitor.phone.version = g_strdup (model);
  phoneMonitor.phone.model = GetPhoneModel (model)->model;
  if (phoneMonitor.phone.model == NULL)
    phoneMonitor.phone.model = g_strdup (_("unknown"));

  phoneMonitor.supported = GetPhoneModel (buf)->flags;

  g_free (phoneMonitor.phone.revision);
  phoneMonitor.phone.revision = g_strdup (rev);

#ifdef XDEBUG
  g_print ("Version: %s\n", phoneMonitor.phone.version);
  g_print ("Model: %s\n", phoneMonitor.phone.model);
  g_print ("Revision: %s\n", phoneMonitor.phone.revision);
#endif

  return (GE_NONE);
}


static GSM_Error fbusinit (bool enable_monitoring)
{
  GSM_Error error = GE_NOLINK;
  GSM_ConnectionType connection = GCT_Serial;
  

  if (!strcmp(smsdConfig.connection, "infrared"))
    connection = GCT_Infrared;

  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise (smsdConfig.model, smsdConfig.port,
                          smsdConfig.initlength, connection, NULL, &sm);

#ifdef XDEBUG
  g_print ("fbusinit: error %d\n", error);
#endif

  if (error != GE_NONE) {
    g_print (_("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    return (error);
  }

  /* Why GSM_Initialise returns before initialization is completed?
     We must wait cca 2 seconds. If smsd segfaults during initialiazation
     try to increase number of seconds.
  */
  sleep (2);
  
  return InitModelInf ();
}


void InitPhoneMonitor (void)
{
  phoneMonitor.phone.model = g_strdup (_("unknown"));
  phoneMonitor.phone.version = phoneMonitor.phone.model;
  phoneMonitor.phone.revision = g_strdup (_("unknown"));
  phoneMonitor.supported = 0;
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
  static GSM_Data data;
  GSM_Error error;
  GSM_SMSMessage *msg;
  SMS_Folder folder;
  SMS_FolderList folderlist;
  register gint i;
  

# ifdef XDEBUG
  g_print ("RefreshSMS is running...\n");
# endif

  pthread_mutex_lock (&smsMutex);
  FreeArray (&(phoneMonitor.sms.messages));
  phoneMonitor.sms.number = 0;
  pthread_mutex_unlock (&smsMutex);

  data.SMSFolderList = &folderlist;
  folder.FolderID = 0;
  data.SMSFolder = &folder;
  
  i = 0;
  while (1)
  {
//    GSM_DataClear (&data);
    msg = g_malloc (sizeof (GSM_SMSMessage));
    msg->MemoryType = GMT_SM;
    msg->Number = ++i;
    data.SMSMessage = msg;

    if ((error = GetSMS (&data, &sm)) == GE_NONE)
    {
      pthread_mutex_lock (&smsMutex);
      phoneMonitor.sms.messages = g_slist_append (phoneMonitor.sms.messages, msg);
      phoneMonitor.sms.number++;
      pthread_mutex_unlock (&smsMutex);
      
      if (phoneMonitor.sms.number == number)
      {
        pthread_cond_signal (&smsCond);
        pthread_mutex_unlock (&smsMutex);
        return;
      }
    }
    else if (error == GE_INVALIDSMSLOCATION)   /* All positions are readed */
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
  GSM_Data dt;

  error = d->status = GE_UNKNOWN;
  if (d)
  {
    pthread_mutex_lock (&sendSMSMutex);
    dt.SMSMessage = d->sms;
    error = d->status = SendSMS ( &dt, &sm);
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
  GSM_Data dt;
  GSM_Error error = GE_UNKNOWN;

  dt.SMSMessage = (GSM_SMSMessage *) data;
  if (dt.SMSMessage)
  {
    error = SM_Functions (GOP_DeleteSMS, &dt, &sm);
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
  GSM_Data data;
  GSM_SMSMemoryStatus SMSStatus = {0, 0};
  PhoneEvent *event;
  GSM_Error error;

  data.SMSStatus = &SMSStatus;
  
# ifdef XDEBUG
  g_print ("Initializing connection...\n");
# endif

  if (fbusinit (true) != GE_NONE)
    exit (1);

# ifdef XDEBUG
  g_print ("Phone connected. Starting monitoring...\n");
# endif

  while (1)
  {
    phoneMonitor.working = FALSE;

    if ((error = SM_Functions (GOP_GetSMSStatus, &data, &sm)) == GE_NONE)
    {
      if (phoneMonitor.sms.unRead != SMSStatus.Unread ||
          phoneMonitor.sms.number != SMSStatus.Number)
      {
        phoneMonitor.working = TRUE;
        RefreshSMS (SMSStatus.Number);
        phoneMonitor.working = FALSE;
      }

      phoneMonitor.sms.unRead = SMSStatus.Unread;
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
    
    sleep (2);
  }
}
