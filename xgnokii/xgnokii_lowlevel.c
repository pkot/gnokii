/*

  $Id$
  
  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-sms.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii.h"
#include "gsm-statemachine.h"

pthread_t monitor_th;
PhoneMonitor phoneMonitor;
pthread_mutex_t memoryMutex;
pthread_cond_t  memoryCond;
pthread_mutex_t calendarMutex;
pthread_cond_t  calendarCond;
pthread_mutex_t smsMutex;
pthread_mutex_t sendSMSMutex;
pthread_cond_t  sendSMSCond;
pthread_mutex_t callMutex;
pthread_mutex_t netMonMutex;
pthread_mutex_t speedDialMutex;
pthread_cond_t  speedDialCond;
pthread_mutex_t callerGroupMutex;
pthread_cond_t  callerGroupCond;
pthread_mutex_t smsCenterMutex;  
pthread_cond_t  smsCenterCond;
pthread_mutex_t alarmMutex;  
pthread_cond_t  alarmCond;
pthread_mutex_t getBitmapMutex;
pthread_cond_t  getBitmapCond;
pthread_mutex_t setBitmapMutex;
pthread_cond_t  setBitmapCond;
pthread_mutex_t getNetworkInfoMutex;
pthread_cond_t  getNetworkInfoCond;
static pthread_mutex_t eventsMutex;
static GSList *ScheduledEvents = NULL;

static GSM_Statemachine statemachine;
/* FIXME - don't really know what should own the statemachine in */
/* the xgnokii scheme of things - Chris */


inline void GUI_InsertEvent(PhoneEvent *event)
{
#ifdef XDEBUG
	g_print ("Inserting Event: %d\n", event->event);
#endif
	pthread_mutex_lock (&eventsMutex);
	ScheduledEvents = g_slist_prepend(ScheduledEvents, event);
	pthread_mutex_unlock(&eventsMutex);
}


inline static PhoneEvent *RemoveEvent(void)
{
	GSList *list;
	PhoneEvent *event = NULL;

	pthread_mutex_lock(&eventsMutex);
	list = g_slist_last(ScheduledEvents);
	if (list) {
		event = (PhoneEvent *)list->data;
		ScheduledEvents = g_slist_remove_link(ScheduledEvents, list);
		g_slist_free_1(list);
	}
	pthread_mutex_unlock(&eventsMutex);

	return (event);
}

void GUI_InitSMSFolders(void)
{
	GSM_Error error;
	GSM_SMSMessage *msg;
	SMS_Folder *fld;
	SMS_FolderList *list;
	GSM_RawData *raw;
	GSM_Data gdat;
	gint i;

	if (phoneMonitor.supported & PM_FOLDERS) {
		GSM_DataClear(&gdat);
  		msg = g_malloc(sizeof(GSM_SMSMessage));
		fld = g_malloc(sizeof(SMS_Folder));
		list = g_malloc(sizeof(SMS_FolderList));
		raw = g_malloc(sizeof(GSM_RawData));

		msg->MemoryType = 0x08;
		msg->Number = 1;
		gdat.SMSMessage = msg;
		gdat.SMSFolder = fld;
		gdat.SMSFolderList = list;
		gdat.RawData = raw;

		error = GetSMS(&gdat, &statemachine);
		foldercount = gdat.SMSFolderList->number;
		
		for (i = 0; i < gdat.SMSFolderList->number; i++) {
			strcpy(folders[i], gdat.SMSFolderList->Folder[i].Name);
		}
	} else {
		foldercount = 2;
		strcpy(folders[0], "Inbox");
		strcpy(folders[1], "Outbox");
	}
}

static GSM_Error InitModelInf(void)
{
	gchar buf[64];
	GSM_Error error;
	GSM_Data data;

	GSM_DataClear(&data);
	data.Model = buf;
	error = SM_Functions(GOP_GetModel, &data, &statemachine);

	if (error != GE_NONE) return error;

	g_free(phoneMonitor.phone.model);
	phoneMonitor.phone.version = g_strdup(buf);
	phoneMonitor.phone.model = GetModel(buf);
	if (phoneMonitor.phone.model == NULL)
		phoneMonitor.phone.model = g_strdup(_("unknown"));
	phoneMonitor.supported = GetPhoneModel(buf)->flags;

	data.Revision = buf;
	error = SM_Functions(GOP_GetRevision, &data, &statemachine);

	if (error != GE_NONE) return error;

	g_free(phoneMonitor.phone.revision);
	phoneMonitor.phone.revision = g_strdup(buf);

	data.Imei = buf;
	error = SM_Functions(GOP_GetImei, &data, &statemachine);

	if (error != GE_NONE) return error;

	g_free(phoneMonitor.phone.imei);
	phoneMonitor.phone.imei = g_strdup(buf);

#ifdef XDEBUG
	g_print("Version: %s\n", phoneMonitor.phone.version);
	g_print("Model: %s\n", phoneMonitor.phone.model);
	g_print("IMEI: %s\n", phoneMonitor.phone.imei);
	g_print("Revision: %s\n", phoneMonitor.phone.revision);
#endif
	return GE_NONE;
}


static GSM_Error fbusinit(bool enable_monitoring)
{
	static GSM_Error error = GE_NOLINK;
	GSM_ConnectionType connection = GCT_Serial;

	if (!strcmp(xgnokiiConfig.connection, "infrared"))
		connection = GCT_Infrared;

	if (!strcmp(xgnokiiConfig.connection, "irda"))
		connection = GCT_Irda;

	/* Initialise the code for the GSM interface. */     

	if (error == GE_NOLINK)
		error = GSM_Initialise(xgnokiiConfig.model, xgnokiiConfig.port,
				       xgnokiiConfig.initlength, connection, RLP_DisplayF96Frame, &statemachine);

#ifdef XDEBUG
	g_print("fbusinit: error %d\n", error);
#endif

	if (error != GE_NONE) {
		g_print(_("GSM/FBUS init failed!\n"));
		/* FIXME: should popup some message... */
		return (error);
	}

	return InitModelInf();
}


void GUI_InitPhoneMonitor (void)
{
  phoneMonitor.phone.model = g_strdup (_("unknown"));
  phoneMonitor.phone.version = phoneMonitor.phone.model;
  phoneMonitor.phone.revision = g_strdup (_("unknown"));
  phoneMonitor.phone.imei = g_strdup (_("unknown"));
  phoneMonitor.supported = 0;
  phoneMonitor.rfLevel = phoneMonitor.batteryLevel = -1;
  phoneMonitor.powerSource = GPS_BATTERY;
  phoneMonitor.working = NULL;
  phoneMonitor.alarm = FALSE;
  phoneMonitor.sms.unRead = phoneMonitor.sms.number = 0;
  phoneMonitor.sms.messages = NULL;
  phoneMonitor.call.callInProgress = CS_Idle;
  *phoneMonitor.call.callNum = '\0';
  phoneMonitor.netmonitor.number = 0;
  *phoneMonitor.netmonitor.screen = *phoneMonitor.netmonitor.screen3 = 
  *phoneMonitor.netmonitor.screen4 = *phoneMonitor.netmonitor.screen5 = '\0';
  pthread_mutex_init (&memoryMutex, NULL);
  pthread_cond_init (&memoryCond, NULL);
  pthread_mutex_init (&calendarMutex, NULL);
  pthread_cond_init (&calendarCond, NULL);
  pthread_mutex_init (&smsMutex, NULL);
  pthread_mutex_init (&sendSMSMutex, NULL);
  pthread_cond_init (&sendSMSCond, NULL);
  pthread_mutex_init (&callMutex, NULL);
  pthread_mutex_init (&eventsMutex, NULL);
  pthread_mutex_init (&callMutex, NULL);
  pthread_mutex_init (&netMonMutex, NULL);
  pthread_mutex_init (&speedDialMutex, NULL);
  pthread_cond_init (&speedDialCond, NULL);
  pthread_mutex_init (&callerGroupMutex, NULL);
  pthread_cond_init (&callerGroupCond, NULL);
  pthread_mutex_init (&smsCenterMutex, NULL);
  pthread_cond_init (&smsCenterCond, NULL);
  pthread_mutex_init (&getBitmapMutex, NULL);
  pthread_cond_init (&getBitmapCond, NULL);
  pthread_mutex_init (&setBitmapMutex, NULL);
  pthread_cond_init (&setBitmapCond, NULL);
  pthread_mutex_init (&getNetworkInfoMutex, NULL);
  pthread_cond_init (&getNetworkInfoCond, NULL);
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
	SMS_Folder *fld;
	SMS_FolderList *list;
	GSM_RawData *raw;
	register gint i;
	gint msg_num = 0, current_folder, locations = -1;

	/* FIXME: how should msg_num be initialized ? */
# ifdef XDEBUG
	g_print ("RefreshSMS is running...\n");
# endif

	pthread_mutex_lock(&smsMutex);
	FreeArray(&(phoneMonitor.sms.messages));
	phoneMonitor.sms.number = 0;
	pthread_mutex_unlock(&smsMutex);
	if (phoneMonitor.supported & PM_CALLERGROUP) 
		current_folder = GMT_IN; /* We have a phone with folder support -> Inbox */
	else 
		current_folder = GMT_SM; /* without folder support -> SIM memory */
	
	if (phoneMonitor.sms.unRead == 0) i = 0; else i = 1;
	while (1) {
		GSM_Data gdat;
		GSM_DataClear(&gdat);
	  	msg = g_malloc(sizeof(GSM_SMSMessage));
		fld = g_malloc(sizeof(SMS_Folder));
		list = g_malloc(sizeof(SMS_FolderList));
		raw = g_malloc(sizeof(GSM_RawData));
		msg->MemoryType = current_folder;
		if ((phoneMonitor.supported & PM_FOLDERS) && (locations != -1))
			msg->Number = msg_num;
		else
			msg->Number = i;

		gdat.SMSMessage = msg;
		gdat.SMSFolder = fld;
		gdat.SMSFolderList = list;
		gdat.RawData = raw;
		if (((error = GetSMS(&gdat, &statemachine)) == GE_NONE) && i != 0) {
			dprintf("Found valid SMS ...\n");
			pthread_mutex_lock(&smsMutex);
			phoneMonitor.sms.messages = g_slist_append(phoneMonitor.sms.messages, msg);
			phoneMonitor.sms.number++;
			pthread_mutex_unlock(&smsMutex);
			if (phoneMonitor.sms.number == number) return;
		} else if (error == GE_INVALIDSMSLOCATION && i != 0) {  /* All positions are read */
			g_free(msg);
			g_free(list);
			g_free(raw);
			g_free(fld);
			break;
		} else {
			g_free(msg);
			g_free(list);
			g_free(raw);
			g_free(fld);
			usleep(750000);
		}
		if (phoneMonitor.supported & PM_FOLDERS) {
			if (i == MAX_SMS_MESSAGES || i == locations) {
				current_folder += 0x08;
				if (current_folder == 0x28) current_folder++;
				i = - 1;
			}
			if ((current_folder != 0x08 && i > -1) || (phoneMonitor.sms.unRead == 0 && i > -1)) {
				locations = gdat.SMSFolder->number;
				msg_num = gdat.SMSFolder->locations[i];
			}
		}
		i++;
	}
}

static gint A_GetMemoryStatus(gpointer data)
{
	GSM_Error error;
	D_MemoryStatus *ms = (D_MemoryStatus *)data;
	GSM_Data gdat;

	error = ms->status = GE_UNKNOWN;
	if (ms) {
		GSM_DataClear(&gdat);	  
		pthread_mutex_lock(&memoryMutex);
		gdat.MemoryStatus = &(ms->memoryStatus);
		error = ms->status = SM_Functions(GOP_GetMemoryStatus, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}
	return (error);
}


static gint A_GetMemoryLocation (gpointer data)
{
  GSM_Error error;
  D_MemoryLocation *ml = (D_MemoryLocation *) data;
  GSM_Data gdat;

  error = ml->status = GE_UNKNOWN;

  if (ml)
  {
    GSM_DataClear(&gdat);
    pthread_mutex_lock (&memoryMutex);
    gdat.PhonebookEntry=(ml->entry);
    error = ml->status = SM_Functions(GOP_ReadPhonebook,&gdat,&statemachine);
    pthread_cond_signal (&memoryCond);
    pthread_mutex_unlock (&memoryMutex);
  }

  return (error);
}


static gint A_GetMemoryLocationAll (gpointer data)
{
	GSM_PhonebookEntry entry;
	GSM_Error error;
	D_MemoryLocationAll *mla = (D_MemoryLocationAll *)data;
	register gint i;
	GSM_Data gdat;

	error = mla->status = GE_NONE;
	entry.MemoryType = mla->type;
	GSM_DataClear(&gdat);
	gdat.PhonebookEntry=&entry;

	pthread_mutex_lock (&memoryMutex);
	for (i = mla->min; i <= mla->max; i++) {
		entry.Location = i;
		error = SM_Functions(GOP_ReadPhonebook, &gdat, &statemachine);
		if (error != GE_NONE && error != GE_INVALIDPHBOOKLOCATION && error != GE_EMPTYMEMORYLOCATION) {
			gint err_count = 0;

			while (error != GE_NONE) {
				g_print (_("%s: line %d: Can't get memory entry number %d from memory %d! %d\n"), __FILE__, __LINE__, i, entry.MemoryType, error);
				if (err_count++ > 3) {
					mla->ReadFailed (i);
					mla->status = error;
					pthread_cond_signal (&memoryCond);
					pthread_mutex_unlock (&memoryMutex);
					return (error);
				}
				error = SM_Functions(GOP_ReadPhonebook, &gdat, &statemachine);
				sleep (2);
			}
		}

		/* If the phonebook location was invalid - just fill up the rest */
		/* This works on a 7110 anyway...*/
		if (error == GE_INVALIDPHBOOKLOCATION || error == GE_EMPTYMEMORYLOCATION) {
			entry.Empty = true;
			entry.Name[0] = 0;
			entry.Number[0] = 0;
			for (i = mla->min; i <= mla->max; i++) {
				error = mla->InsertEntry (&entry);
				if (error != GE_NONE) break;
			}	
		}

		error = mla->InsertEntry (&entry);
		if (error != GE_NONE)break;
	}
	mla->status = error;
	pthread_cond_signal (&memoryCond);
	pthread_mutex_unlock (&memoryMutex);
	return (error);
}


static gint A_WriteMemoryLocation (gpointer data)
{
  GSM_Error error;
  D_MemoryLocation *ml = (D_MemoryLocation *) data;
  GSM_Data gdat;

  error = ml->status = GE_UNKNOWN;

  GSM_DataClear(&gdat);
  gdat.PhonebookEntry=(ml->entry);

  if (ml)
  {
    pthread_mutex_lock (&memoryMutex);
    error = ml->status = SM_Functions(GOP_WritePhonebook,&gdat,&statemachine);
    pthread_cond_signal (&memoryCond);
    pthread_mutex_unlock (&memoryMutex);
  }

  return (error);
}


static gint A_WriteMemoryLocationAll (gpointer data)
{
/*  GSM_PhonebookEntry entry; */
  GSM_Error error;
  D_MemoryLocationAll *mla = (D_MemoryLocationAll *) data;
/*  register gint i;
*/
  error = mla->status = GE_NONE;
/*  entry.MemoryType = mla->type;

  pthread_mutex_lock (&memoryMutex);
  for (i = mla->min; i <= mla->max; i++)
  {
    entry.Location = i;
    error = GSM->GetMemoryLocation (&entry);
    if (error != GE_NONE)
    {
      gint err_count = 0;

      while (error != GE_NONE)
      {
        g_print (_("%s: line %d: Can't get memory entry number %d from memory %d! %d\n"),
                 __FILE__, __LINE__, i, entry.MemoryType, error);
        if (err_count++ > 3)
        {
          mla->ReadFailed (i);
          mla->status = error;
          pthread_cond_signal (&memoryCond);
          pthread_mutex_unlock (&memoryMutex);
          return (error);
        }

        error = GSM->GetMemoryLocation (&entry);
        sleep (2);
      }
    }
    error = mla->InsertEntry (&entry);
    if (error != GE_NONE)
      break;
  }
  mla->status = error;
  pthread_cond_signal (&memoryCond);
  pthread_mutex_unlock (&memoryMutex); */
  return (error);
}


static gint A_GetCalendarNote (gpointer data)
{
  GSM_Error error;
  D_CalendarNote *cn = (D_CalendarNote *) data;
  GSM_Data gdat;

  error = cn->status = GE_UNKNOWN;

  if (cn)
  {
    pthread_mutex_lock (&calendarMutex);
    GSM_DataClear(&gdat);
    gdat.CalendarNote = cn->entry;
    error = cn->status = SM_Functions(GOP_GetCalendarNote, &gdat, &statemachine);
    pthread_cond_signal (&calendarCond);
    pthread_mutex_unlock (&calendarMutex);
  }

  return (error);
}


static gint A_GetCalendarNoteAll (gpointer data)
{
  GSM_CalendarNotesList list;
  GSM_CalendarNote entry;
  D_CalendarNoteAll *cna = (D_CalendarNoteAll *) data;
  GSM_Error e;
  GSM_Data gdat;
  register gint i = 1;

  pthread_mutex_lock (&calendarMutex);
  while (1)
  {
    entry.Location = i++;

    GSM_DataClear(&gdat);
    gdat.CalendarNote = &entry;
    gdat.CalendarNotesList = &list;
    if ((e = SM_Functions(GOP_GetCalendarNote, &gdat, &statemachine)) != GE_NONE)
      break;

    if (cna->InsertEntry (&entry) != GE_NONE)
      break;
  }

  pthread_mutex_unlock (&calendarMutex);
  g_free (cna);
  if (e == GE_INVALIDCALNOTELOCATION)
    return (GE_NONE);
  else
    return (e);
}


static gint A_WriteCalendarNote (gpointer data)
{
  GSM_Error error;
  D_CalendarNote *cn = (D_CalendarNote *) data;
  GSM_Data gdat;

  error = cn->status = GE_UNKNOWN;

  if (cn)
  {
    pthread_mutex_lock (&calendarMutex);
    GSM_DataClear(&gdat);
    gdat.CalendarNote = cn->entry;
    error = cn->status = SM_Functions(GOP_WriteCalendarNote, &gdat, &statemachine);
    pthread_cond_signal (&calendarCond);
    pthread_mutex_unlock (&calendarMutex);
  }

  return (error);
}


static gint A_DeleteCalendarNote (gpointer data)
{
  GSM_CalendarNote *note = (GSM_CalendarNote *) data;
  GSM_Error error = GE_UNKNOWN;
  GSM_Data gdat;

  if (note)
  {
    GSM_DataClear(&gdat);
    gdat.CalendarNote = note;
    error = SM_Functions(GOP_DeleteCalendarNote, &gdat, &statemachine);
    g_free (note);
  }

  return (error);
}

static gint A_GetCallerGroup (gpointer data)
{
  GSM_Bitmap bitmap;
  GSM_Error error;
  D_CallerGroup *cg = (D_CallerGroup *) data;
  GSM_Data gdat;

  error = cg->status = GE_UNKNOWN;

  if (cg)
  {
    bitmap.type = GSM_CallerLogo;
    bitmap.number = cg->number;

    pthread_mutex_lock (&callerGroupMutex);
    GSM_DataClear(&gdat);
    gdat.Bitmap = &bitmap;
    error = cg->status = SM_Functions(GOP_GetBitmap, &gdat, &statemachine);
    strncpy (cg->text, bitmap.text, 256);
    cg->text[255] = '\0';
    pthread_cond_signal (&callerGroupCond);
    pthread_mutex_unlock (&callerGroupMutex);
  }

  return (error);
}


static gint A_SendCallerGroup (gpointer data)
{
  GSM_Bitmap bitmap;
  D_CallerGroup *cg = (D_CallerGroup *) data;
  GSM_Error error;
  GSM_Data gdat;

  if (!cg)
    return (GE_UNKNOWN);

  bitmap.type = GSM_CallerLogo;
  bitmap.number = cg->number;
  GSM_DataClear(&gdat);
  gdat.Bitmap = &bitmap;
  if ((error = SM_Functions(GOP_GetBitmap, &gdat, &statemachine)) != GE_NONE)
  {
    g_free (cg);
    return (error);
  }
  strncpy (bitmap.text, cg->text, 256);
  bitmap.text[255] = '\0';
  g_free (cg);
  return (SM_Functions(GOP_SetBitmap, &gdat, &statemachine));
}


static gint A_GetSMSCenter (gpointer data)
{
  D_SMSCenter *c = (D_SMSCenter *) data;
  GSM_Error error;
  GSM_Data gdat;

  error = c->status = GE_UNKNOWN;
  if (c)
  {
    pthread_mutex_lock (&smsCenterMutex);
    GSM_DataClear(&gdat);
    gdat.MessageCenter = c->center;
    error = c->status = SM_Functions(GOP_GetSMSCenter, &gdat, &statemachine);
    pthread_cond_signal (&smsCenterCond);
    pthread_mutex_unlock (&smsCenterMutex);
  }

  return (error);
}


static gint A_SetSMSCenter (gpointer data)
{
  D_SMSCenter *c = (D_SMSCenter *) data;
  GSM_Error error;
  GSM_Data gdat;

  error = c->status = GE_UNKNOWN;
  if (c)
  {
    GSM_DataClear(&gdat);
    gdat.MessageCenter = c->center;
    pthread_mutex_lock (&smsCenterMutex);
    error = c->status = SM_Functions(GOP_GetSMSCenter, &gdat, &statemachine);
    g_free (c);
    pthread_cond_signal (&smsCenterCond);
    pthread_mutex_unlock (&smsCenterMutex);
  }

  return (error);
}


static gint A_SendSMSMessage (gpointer data)
{
  D_SMSMessage *d = (D_SMSMessage *) data;
  GSM_Error error;
  GSM_Data gdat;

  error = d->status = GE_UNKNOWN;
  if (d)
  {
    GSM_DataClear(&gdat);
    gdat.SMSMessage = d->sms;
    pthread_mutex_lock (&sendSMSMutex);
    error = d->status = SendSMS(&gdat, &statemachine);
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
    GSM_Data gdat;
    GSM_DataClear(&gdat);
    gdat.SMSMessage = sms;
    error = SM_Functions(GOP_DeleteSMS, &gdat, &statemachine);
    g_free (sms);
  }

  return (error);
}


static gint A_GetSpeedDial (gpointer data)
{
  D_SpeedDial *d = (D_SpeedDial *) data;
  GSM_Error error;
  GSM_Data gdat;

  error = d->status = GE_UNKNOWN;

  if (d)
  {
    GSM_DataClear(&gdat);
    gdat.SpeedDial = &d->entry;
    pthread_mutex_lock (&speedDialMutex);
    error = d->status = SM_Functions(GOP_GetSpeedDial, &gdat, &statemachine);
    pthread_cond_signal (&speedDialCond);
    pthread_mutex_unlock (&speedDialMutex);
  }

  return (error);
}


static gint A_SendSpeedDial (gpointer data)
{
  D_SpeedDial *d = (D_SpeedDial *) data;
  GSM_Error error;

  error = d->status = GE_UNKNOWN;

  if (d)
  {
    //pthread_mutex_lock (&speedDialMutex);
//    error = d->status = GSM->SetSpeedDial (&(d->entry));
    g_free (d);
    //pthread_cond_signal (&speedDialCond);
    //pthread_mutex_unlock (&speedDialMutex);
  }

  return (error);
}


static gint A_SendDTMF (gpointer data)
{
  gchar *buf = (gchar *) data;
  GSM_Error error = GE_UNKNOWN;

  if (buf) 
  {
//    error = GSM->SendDTMF (buf);
    g_free (buf);
  }

  return (error);
}


static gint A_NetMonOnOff (gpointer data)
{
  gint mode = GPOINTER_TO_INT (data);
  GSM_Data gdat;
  GSM_NetMonitor nm;

  GSM_DataClear(&gdat);
  gdat.NetMonitor = &nm;
  if (mode) nm.Field = 0xf3;
  else nm.Field = 0xf1;
  return SM_Functions(GOP_NetMonitor, &gdat, &statemachine);
}


static gint A_NetMonitor (gpointer data)
{
  gint number = GPOINTER_TO_INT (data);

  if (data == 0)
    phoneMonitor.netmonitor.number = 0;
  else
    phoneMonitor.netmonitor.number = number;
    
  return (0);
}


static gint A_DialVoice (gpointer data)
{
  gchar *number = (gchar *) data;
  GSM_Error error = GE_UNKNOWN;

  if (number)
  {
//    error = GSM->DialVoice (number);
    g_free (number);
  }

  return (error);
}


static gint A_GetAlarm (gpointer data)
{
  D_Alarm *a = (D_Alarm *) data;
  GSM_Error error;
  GSM_Data gdat;

  error = GE_UNKNOWN;

  if (a)
  {
    a->status = GE_UNKNOWN;
    pthread_mutex_lock (&alarmMutex);
    GSM_DataClear(&gdat);
    gdat.DateTime = &a->time;
    error = a->status = SM_Functions(GOP_GetAlarm, &gdat, &statemachine);
    pthread_cond_signal (&alarmCond);
    pthread_mutex_unlock (&alarmMutex);
  }

  return (error);
}


static gint A_SetAlarm (gpointer data)
{
  D_Alarm *a = (D_Alarm *) data;
  GSM_Error error;
  GSM_Data gdat;

  error = a->status = GE_UNKNOWN;

  if (a)
  {
    GSM_DataClear(&gdat);
    gdat.DateTime = &a->time;
    error = a->status = SM_Functions(GOP_SetAlarm, &gdat, &statemachine);
    g_free (a);
  }

  return (error);
}


static gint A_SendKeyStroke (gpointer data)
{
  /*  gchar *buf = (gchar *) data;*/

  /* This is wrong. FIX IT */
  /*  if (buf) 
  {
    FB61_TX_SendMessage(0x07, 0x0c, buf);
    g_free (buf);
    }*/

  return (0);
}

static gint A_GetBitmap(gpointer data) {
  GSM_Error error;
  D_Bitmap *d = (D_Bitmap *)data;
  GSM_Data gdat;

  GSM_DataClear(&gdat);
  pthread_mutex_lock(&getBitmapMutex);
  gdat.Bitmap=d->bitmap;
  error = d->status = SM_Functions(GOP_GetBitmap,&gdat,&statemachine);
  pthread_cond_signal(&getBitmapCond);
  pthread_mutex_unlock(&getBitmapMutex);
  return error;
}

static gint A_SetBitmap(gpointer data) {
  GSM_Error error;
  D_Bitmap *d = (D_Bitmap *)data;
  GSM_Bitmap bitmap;
  GSM_Data gdat;
  
  GSM_DataClear(&gdat);
  pthread_mutex_lock(&setBitmapMutex);
  if (d->bitmap->type == GSM_CallerLogo) {
    bitmap.type = d->bitmap->type;
    bitmap.number = d->bitmap->number;
    gdat.Bitmap=&bitmap;
    error = d->status = SM_Functions(GOP_GetBitmap,&gdat,&statemachine);
    if (error == GE_NONE) {
      strncpy(d->bitmap->text,bitmap.text,sizeof(bitmap.text));
      d->bitmap->ringtone = bitmap.ringtone;
      gdat.Bitmap=d->bitmap;
      error = d->status = SM_Functions(GOP_SetBitmap,&gdat,&statemachine);
    }
  } else {
    gdat.Bitmap=d->bitmap;
    error = d->status = SM_Functions(GOP_SetBitmap,&gdat,&statemachine);
  }
  pthread_cond_signal(&setBitmapCond);
  pthread_mutex_unlock(&setBitmapMutex);
  return error;
}

static gint A_GetNetworkInfo(gpointer data) {
  GSM_Error error;
  D_NetworkInfo *d = (D_NetworkInfo *)data;
  GSM_Data gdat;
  
  GSM_DataClear(&gdat);

  pthread_mutex_lock(&getNetworkInfoMutex);
  gdat.NetworkInfo=d->info;
  error = d->status = SM_Functions(GOP_GetNetworkInfo,&gdat,&statemachine);
  pthread_cond_signal(&getNetworkInfoCond);
  pthread_mutex_unlock(&getNetworkInfoMutex);
  return error;
}

static gint A_Exit (gpointer data)
{
  pthread_exit (0);
  return (0); /* just to be proper */
}


gint (*DoAction[])(gpointer) = {
  A_GetMemoryStatus,
  A_GetMemoryLocation,
  A_GetMemoryLocationAll,
  A_WriteMemoryLocation,
  A_WriteMemoryLocationAll,
  A_GetCalendarNote,
  A_GetCalendarNoteAll,
  A_WriteCalendarNote,
  A_DeleteCalendarNote,
  A_GetCallerGroup,
  A_SendCallerGroup,
  A_GetSMSCenter,
  A_SetSMSCenter,
  A_SendSMSMessage,
  A_DeleteSMSMessage,
  A_GetSpeedDial,
  A_SendSpeedDial,
  A_SendDTMF,
  A_NetMonOnOff,
  A_NetMonitor,
  A_DialVoice,
  A_GetAlarm,
  A_SetAlarm,
  A_SendKeyStroke,
  A_GetBitmap,
  A_SetBitmap,
  A_GetNetworkInfo,
  A_Exit
};


void *GUI_Connect (void *a)
{
  /* Define required unit types for RF and Battery level meters. */
  GSM_RFUnits rf_units = GRF_Percentage;
  GSM_BatteryUnits batt_units = GBU_Percentage;

  GSM_DateTime Alarm;
  GSM_SMSMemoryStatus SMSStatus = {0, 0};
  gchar number[INCALL_NUMBER_LENGTH];
  PhoneEvent *event;
  GSM_Error error;
/*  gint status; */
  GSM_Data data;

  GSM_DataClear(&data);

# ifdef XDEBUG
  g_print ("Initializing connection...\n");
# endif

  phoneMonitor.working = _("Connecting...");
  while (fbusinit(true) != GE_NONE)
    sleep(1);

# ifdef XDEBUG
  g_print ("Phone connected. Starting monitoring...\n");
# endif

  sleep(1);

  data.RFLevel = &phoneMonitor.rfLevel;
  data.RFUnits = &rf_units;
  data.PowerSource = &phoneMonitor.powerSource;
  data.BatteryUnits = &batt_units; 
  data.BatteryLevel = &phoneMonitor.batteryLevel;
  data.DateTime = &Alarm;
  data.SMSStatus = &SMSStatus;
  data.IncomingCallNr = number;

  while (1)
  {
    phoneMonitor.working = NULL;

    /* FIXME - this loop goes mad on my 7110 - so I've put in a usleep */
    usleep(50000);

    if (SM_Functions(GOP_GetRFLevel,&data,&statemachine) != GE_NONE)
      phoneMonitor.rfLevel = -1;

    if (rf_units == GRF_Arbitrary)
      phoneMonitor.rfLevel *= 25;

    if (SM_Functions(GOP_GetPowersource,&data,&statemachine)  == GE_NONE 
        && phoneMonitor.powerSource == GPS_ACDC)
      phoneMonitor.batteryLevel = ((gint) phoneMonitor.batteryLevel + 25) % 125;
    else
    {
      if (SM_Functions(GOP_GetBatteryLevel,&data,&statemachine) != GE_NONE)
        phoneMonitor.batteryLevel = -1;
      if (batt_units == GBU_Arbitrary)
        phoneMonitor.batteryLevel *= 25;
    }

    if (SM_Functions(GOP_GetAlarm,&data,&statemachine) == GE_NONE && Alarm.AlarmEnabled != 0)
      phoneMonitor.alarm = TRUE;
    else
      phoneMonitor.alarm = FALSE;

    if (SM_Functions(GOP_GetSMSStatus,&data,&statemachine) == GE_NONE)
    {
      if (phoneMonitor.sms.unRead != SMSStatus.Unread ||
          phoneMonitor.sms.number != SMSStatus.Number)
      {
        phoneMonitor.working = _("Refreshing SMSes...");
        phoneMonitor.sms.unRead = SMSStatus.Unread; 
	RefreshSMS(SMSStatus.Number);
        phoneMonitor.working = NULL;
      }

      phoneMonitor.sms.unRead = SMSStatus.Unread;
    }

    if (SM_Functions(GOP_GetIncomingCallNr,&data,&statemachine) == GE_NONE)
    {
#   ifdef XDEBUG
      g_print ("Call in progress: %s\n", phoneMonitor.call.callNum);
#   endif

/*    GSM->GetDisplayStatus (&status);
      if (status & (1<<DS_Call_In_Progress))
      {
        pthread_mutex_lock (&callMutex);
        phoneMonitor.call.callInProgress = CS_InProgress;
        pthread_mutex_unlock (&callMutex);
      }
      else
      {
        pthread_mutex_lock (&callMutex);
        phoneMonitor.call.callInProgress = CS_Waiting;
        strncpy (phoneMonitor.call.callNum, number, INCALL_NUMBER_LENGTH);
        pthread_mutex_unlock (&callMutex);
      }*/
    }
    else
    {
      pthread_mutex_lock (&callMutex);
      phoneMonitor.call.callInProgress = CS_Idle;
      *phoneMonitor.call.callNum = '\0';
      pthread_mutex_unlock (&callMutex);
    }

    pthread_mutex_lock (&netMonMutex);
    if (phoneMonitor.netmonitor.number)
    {
//      GSM->NetMonitor (phoneMonitor.netmonitor.number,
//                       phoneMonitor.netmonitor.screen);
//      GSM->NetMonitor (3, phoneMonitor.netmonitor.screen3);
//      GSM->NetMonitor (4, phoneMonitor.netmonitor.screen4);
//      GSM->NetMonitor (5, phoneMonitor.netmonitor.screen5);
    }
    else
    {
      *phoneMonitor.netmonitor.screen = *phoneMonitor.netmonitor.screen3 = 
      *phoneMonitor.netmonitor.screen4 = *phoneMonitor.netmonitor.screen5 = '\0';
    }
    pthread_mutex_unlock (&netMonMutex);

    while ((event = RemoveEvent ()) != NULL)
    {
#     ifdef XDEBUG      
      g_print ("Processing Event: %d\n", event->event);
#     endif
      phoneMonitor.working = _("Working...");
      if (event->event <= Event_Exit)
        if ((error = DoAction[event->event] (event->data)) != GE_NONE)
          g_print (_("Event %d failed with return code %d!\n"), event->event, error);
      g_free (event);
    }
  }
}
