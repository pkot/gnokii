/*

  $Id$
  
  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.

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
  & Ján Derfiòák <ja@mail.upjs.sk>.

*/

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-sms.h"
#include "cfgreader.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii.h"

pthread_t monitor_th;
PhoneMonitor phoneMonitor;
pthread_mutex_t memoryMutex;
pthread_cond_t memoryCond;
pthread_mutex_t calendarMutex;
pthread_cond_t calendarCond;
pthread_mutex_t smsMutex;
pthread_mutex_t sendSMSMutex;
pthread_cond_t sendSMSCond;
pthread_mutex_t callMutex;
pthread_mutex_t netMonMutex;
pthread_mutex_t speedDialMutex;
pthread_cond_t speedDialCond;
pthread_mutex_t callerGroupMutex;
pthread_cond_t callerGroupCond;
pthread_mutex_t smsCenterMutex;
pthread_cond_t smsCenterCond;
pthread_mutex_t alarmMutex;
pthread_cond_t alarmCond;
pthread_mutex_t getBitmapMutex;
pthread_cond_t getBitmapCond;
pthread_mutex_t setBitmapMutex;
pthread_cond_t setBitmapCond;
pthread_mutex_t getNetworkInfoMutex;
pthread_cond_t getNetworkInfoCond;
static pthread_mutex_t eventsMutex;
static GSList *ScheduledEvents = NULL;

static GSM_Data gdat;
static char *lockfile = NULL;
GSM_Statemachine statemachine;
/* FIXME - don't really know what should own the statemachine in */
/* the xgnokii scheme of things - Chris */
int isSMSactivated = 0;



inline void GUI_InsertEvent(PhoneEvent * event)
{
#ifdef XDEBUG
	g_print("Inserting Event: %d\n", event->event);
#endif
	pthread_mutex_lock(&eventsMutex);
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
		event = (PhoneEvent *) list->data;
		ScheduledEvents = g_slist_remove_link(ScheduledEvents, list);
		g_slist_free_1(list);
	}
	pthread_mutex_unlock(&eventsMutex);

	return (event);
}

GSM_Error GUI_InitSMSFolders(void)
{
	GSM_Error error;
	SMS_FolderList list;
	gint i;

	if (phoneMonitor.supported & PM_FOLDERS) {

		gdat.SMSFolderList = &list;

		if ((error = SM_Functions(GOP_GetSMSFolders, &gdat, &statemachine)) != GE_NONE)
			return error;
		foldercount = gdat.SMSFolderList->Number;
		for (i = 0; i < gdat.SMSFolderList->Number; i++) {
			strcpy(folders[i], gdat.SMSFolderList->Folder[i].Name);
		}
	} else {
		foldercount = 2;
		strcpy(folders[0], "Inbox");
		strcpy(folders[1], "Outbox");
	}
	return GE_NONE;
}

static inline void FreeElement(gpointer data, gpointer userData)
{
	g_free((GSM_SMSMessage *) data);
}

static inline void FreeArray(GSList ** array)
{
	if (*array) {
		g_slist_foreach(*array, FreeElement, NULL);
		g_slist_free(*array);
		*array = NULL;
	}
}

static GSM_Error InitModelInf(void)
{
	gchar buf[64];
	gint i, j;
	GSM_Error error;
	SMS_Status *SMSStatus;
	SMS_FolderStats *FolderStats[MAX_SMS_FOLDERS];
	SMS_MessagesList *MessagesList[MAX_SMS_MESSAGES][MAX_SMS_FOLDERS];

	GSM_DataClear(&gdat);

	gdat.Model = buf;
	error = SM_Functions(GOP_GetModel, &gdat, &statemachine);
	if (error != GE_NONE) return error;
	phoneMonitor.phone.version = g_malloc(sizeof(gdat.Model));
	phoneMonitor.phone.model = GetModel(gdat.Model);

	if (phoneMonitor.phone.model == NULL)
		phoneMonitor.phone.model = g_strdup(_("unknown"));
	phoneMonitor.supported = GetPhoneModel(buf)->flags;

	gdat.Revision = buf;
	gdat.Model = NULL;
	error = SM_Functions(GOP_GetRevision, &gdat, &statemachine);
	if (error != GE_NONE) return error;
	phoneMonitor.phone.version = g_malloc(sizeof(gdat.Revision));
	phoneMonitor.phone.revision = g_malloc(sizeof(gdat.Revision));
	phoneMonitor.phone.version = g_strdup(gdat.Revision);
	phoneMonitor.phone.revision = g_strdup(gdat.Revision);

	gdat.Revision = NULL;
	gdat.Model = NULL;
	gdat.Imei = buf;
	error = SM_Functions(GOP_GetImei, &gdat, &statemachine);
	if (error != GE_NONE) return error;
	phoneMonitor.phone.imei = g_malloc(sizeof(gdat.Imei));
	phoneMonitor.phone.imei = g_strdup(gdat.Imei);

	SMSStatus = g_malloc(sizeof(SMS_Status));
	memset(SMSStatus, 0, sizeof(SMS_Status));
	gdat.SMSStatus = SMSStatus;

	for (i = 0; i < MAX_SMS_FOLDERS; i++) {
		FolderStats[i] = g_malloc(sizeof(SMS_FolderStats));
		memset(FolderStats[i], 0, sizeof(SMS_FolderStats));
		gdat.FolderStats[i] = FolderStats[i];

		gdat.FolderStats[i]->Number = 0;
		gdat.FolderStats[i]->Changed = 0;
		gdat.FolderStats[i]->Unread = 0;
		gdat.FolderStats[i]->Changed = 0;
		for (j = 0; j < MAX_SMS_MESSAGES; j++) {
			MessagesList[j][i] = g_malloc(sizeof(SMS_MessagesList));
			gdat.MessagesList[j][i] = MessagesList[j][i];

			gdat.MessagesList[j][i]->Type = SMS_Old;
			gdat.MessagesList[j][i]->Location = 0;
			gdat.MessagesList[j][i]->MessageType = SMS_Deliver;
		}
	}

#ifdef XDEBUG
	g_print("Version: %s\n", phoneMonitor.phone.version);
	g_print("Model: %s\n", phoneMonitor.phone.model);
	g_print("IMEI: %s\n", phoneMonitor.phone.imei);
	g_print("Revision: %s\n", phoneMonitor.phone.revision);
#endif
	return GE_NONE;
}


static void busterminate(void)
{
	SM_Functions(GOP_Terminate, NULL, &statemachine);
	if (lockfile) unlock_device(lockfile);
}


static GSM_Error fbusinit(bool enable_monitoring)
{
	static GSM_Error error = GE_NOLINK;
	GSM_ConnectionType connection = GCT_Serial;
	static bool atexit_registered = false;
	char *aux;

	if (!strcmp(xgnokiiConfig.connection, "infrared"))
		connection = GCT_Infrared;
	if (!strcmp(xgnokiiConfig.connection, "irda"))
		connection = GCT_Irda;
	if (!strcmp(xgnokiiConfig.connection, "tcp"))
		connection = GCT_TCP;
	if (!strcmp(xgnokiiConfig.connection, "dau9p"))
		connection = GCT_DAU9P;

	/* register cleanup function */
	if (!atexit_registered) {
		atexit_registered = true;
		atexit(busterminate);
	}

	aux = CFG_Get(CFG_Info, "global", "use_locking");
	/* Defaults to 'no' */
	if (aux && !strcmp(aux, "yes")) {
		lockfile = lock_device(xgnokiiConfig.port);
		if (lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting\n"));
			exit(1);
		}
	}

	/* Initialise the code for the GSM interface. */

	if (error == GE_NOLINK)
		error = GSM_Initialise(xgnokiiConfig.model, xgnokiiConfig.port,
				       xgnokiiConfig.initlength, connection, RLP_DisplayF96Frame,
				       &statemachine);

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


void GUI_InitPhoneMonitor(void)
{
	phoneMonitor.phone.model = g_strdup(_("unknown"));
	phoneMonitor.phone.version = phoneMonitor.phone.model;
	phoneMonitor.phone.revision = g_strdup(_("unknown"));
	phoneMonitor.phone.imei = g_strdup(_("unknown"));
	phoneMonitor.supported = 0;
	phoneMonitor.rfLevel = phoneMonitor.batteryLevel = -1;
	phoneMonitor.powerSource = GPS_BATTERY;
	phoneMonitor.working = NULL;
	phoneMonitor.alarm = FALSE;
	phoneMonitor.sms.changed = phoneMonitor.sms.unRead = phoneMonitor.sms.number = 0;
	phoneMonitor.sms.messages = NULL;
	phoneMonitor.call.callInProgress = CS_Idle;
	*phoneMonitor.call.callNum = '\0';
	phoneMonitor.netmonitor.number = 0;
	*phoneMonitor.netmonitor.screen = *phoneMonitor.netmonitor.screen3 =
	    *phoneMonitor.netmonitor.screen4 = *phoneMonitor.netmonitor.screen5 = '\0';
	pthread_mutex_init(&memoryMutex, NULL);
	pthread_cond_init(&memoryCond, NULL);
	pthread_mutex_init(&calendarMutex, NULL);
	pthread_cond_init(&calendarCond, NULL);
	pthread_mutex_init(&smsMutex, NULL);
	pthread_mutex_init(&sendSMSMutex, NULL);
	pthread_cond_init(&sendSMSCond, NULL);
	pthread_mutex_init(&callMutex, NULL);
	pthread_mutex_init(&eventsMutex, NULL);
	pthread_mutex_init(&callMutex, NULL);
	pthread_mutex_init(&netMonMutex, NULL);
	pthread_mutex_init(&speedDialMutex, NULL);
	pthread_cond_init(&speedDialCond, NULL);
	pthread_mutex_init(&callerGroupMutex, NULL);
	pthread_cond_init(&callerGroupCond, NULL);
	pthread_mutex_init(&smsCenterMutex, NULL);
	pthread_cond_init(&smsCenterCond, NULL);
	pthread_mutex_init(&getBitmapMutex, NULL);
	pthread_cond_init(&getBitmapCond, NULL);
	pthread_mutex_init(&setBitmapMutex, NULL);
	pthread_cond_init(&setBitmapCond, NULL);
	pthread_mutex_init(&getNetworkInfoMutex, NULL);
	pthread_cond_init(&getNetworkInfoCond, NULL);
}

static gint compare_number(const GSM_SMSMessage * a, const GSM_SMSMessage * b)
{
	dprintf("a: %i b:%i\n", a->Number, b->Number);
	if (a->Number == b->Number)
		return 0;
	else
		return 1;
}


static void RefreshSMS(const gint number)
{
	GSM_Error error;
	GSM_API_SMS *msg, *tmp_msg;
	SMS_Folder *fld;
	SMS_FolderList *list;
	GSM_RawData *raw;
	GSList *tmp_list;
	gint i, j, dummy;

#ifdef XDEBUG
	g_print("RefreshSMS is running...\n");
#endif

	dprintf("RefreshSMS: changed: %i\n", gdat.SMSStatus->Changed);
	dprintf("RefreshSMS: unread: %i, total: %i\n", gdat.SMSStatus->Unread,
		gdat.SMSStatus->Number);
	for (i = 0; i < gdat.SMSStatus->NumberOfFolders; i++) {
		dummy = 0;
		for (j = 0; j < gdat.FolderStats[i]->Used; j++) {
			if ((gdat.MessagesList[j][i]->Type == SMS_Changed) ||
			    (gdat.MessagesList[j][i]->Type == SMS_NotRead) ||
			    (gdat.MessagesList[j][i]->Type == SMS_New))
				dprintf("RefreshSMS: change #%i in folder %i at location %i!\n",
					++dummy, i, gdat.MessagesList[j][i]->Location);
		}
	}
	if (phoneMonitor.supported & PM_FOLDERS) {

		for (i = 0; i < gdat.SMSStatus->NumberOfFolders; i++) {
			for (j = 0; j < gdat.FolderStats[i]->Used; j++) {
				if (gdat.MessagesList[j][i]->Type == SMS_Deleted ||
				    gdat.MessagesList[j][i]->Type == SMS_Changed) {
					dprintf("We got a deleted message here to handle!\n");
					pthread_mutex_lock(&smsMutex);
					msg = g_malloc(sizeof(GSM_SMSMessage));
					msg->Number = gdat.MessagesList[j][i]->Location;
					tmp_list =
					    g_slist_find_custom(phoneMonitor.sms.messages, msg,
								(GCompareFunc) compare_number);
					tmp_msg = (GSM_API_SMS *) tmp_list->data;
					phoneMonitor.sms.messages =
					    g_slist_remove(phoneMonitor.sms.messages, tmp_msg);
					g_free(tmp_msg);
					pthread_mutex_unlock(&smsMutex);
					if (gdat.MessagesList[j][i]->Type == SMS_Deleted)
						gdat.MessagesList[j][i]->Type = SMS_ToBeRemoved;	/* FreeDeletedMessages has to find it */
				}
				if (gdat.MessagesList[j][i]->Type == SMS_New ||
				    gdat.MessagesList[j][i]->Type == SMS_NotRead ||
				    gdat.MessagesList[j][i]->Type == SMS_Changed) {
					msg = g_malloc(sizeof(GSM_SMSMessage));
					list = g_malloc(sizeof(SMS_FolderList));
					raw = g_malloc(sizeof(GSM_RawData));
					gdat.SMS = msg;
					gdat.SMSFolder = NULL;
					gdat.SMSFolderList = list;
					gdat.RawData = raw;

					gdat.SMS->Number = gdat.MessagesList[j][i]->Location;
					gdat.SMS->MemoryType =(GSM_MemoryType) i + 12 ;
					dprintf("#: %i, mt: %i\n", gdat.SMS->Number, gdat.SMS->MemoryType);
					if ((error = GetSMSnoValidate(&gdat, &statemachine)) == GE_NONE) {
						dprintf("Found valid SMS ...\n %s\n",
							msg->UserData[0].u.Text);
						pthread_mutex_lock(&smsMutex);
						phoneMonitor.sms.messages =
						    g_slist_append(phoneMonitor.sms.messages, msg);
						pthread_mutex_unlock(&smsMutex);
					}
					if (gdat.MessagesList[j][i]->Type == SMS_New ||
					    gdat.MessagesList[j][i]->Type == SMS_Changed)
						gdat.MessagesList[j][i]->Type = SMS_Old;
					if (gdat.MessagesList[j][i]->Type == SMS_NotRead)
						gdat.MessagesList[j][i]->Type = SMS_NotReadHandled;
				}
			}
			gdat.FolderStats[i]->Changed = 0;	/* now we handled the changes and can reset */
		}
		gdat.SMSStatus->Changed = 0;	/* now we handled the changes and can reset */

	} else {
		pthread_mutex_lock(&smsMutex);
		FreeArray(&(phoneMonitor.sms.messages));
		phoneMonitor.sms.number = 0;
		pthread_mutex_unlock(&smsMutex);
		i = 0;
		while (1) {
			i++;
			fld = g_malloc(sizeof(SMS_Folder));
			list = g_malloc(sizeof(SMS_FolderList));
			raw = g_malloc(sizeof(GSM_RawData));
			msg = g_malloc(sizeof(GSM_SMSMessage));

			msg->MemoryType = GMT_SM;
			msg->Number = i;

			gdat.SMS = msg;
			gdat.SMSFolder = fld;
			gdat.SMSFolderList = list;
			gdat.RawData = raw;
			if ((error = GetSMS(&gdat, &statemachine)) == GE_NONE) {
				dprintf("Found valid SMS ...\n");
				pthread_mutex_lock(&smsMutex);
				phoneMonitor.sms.messages =
				    g_slist_append(phoneMonitor.sms.messages, msg);
				phoneMonitor.sms.number++;
				pthread_mutex_unlock(&smsMutex);
				if (phoneMonitor.sms.number == number)
					return;
			} else if (error == GE_INVALIDSMSLOCATION) {	/* All positions are read */
				g_free(list);
				g_free(raw);
				g_free(fld);
				g_free(msg);
				break;
			} else {
				g_free(list);
				g_free(raw);
				g_free(fld);
				g_free(msg);
				usleep(750000);
			}
		}
	}
}

static gint A_GetMemoryStatus(gpointer data)
{
	GSM_Error error;
	D_MemoryStatus *ms = (D_MemoryStatus *) data;
	GSM_Data gdat;

	error = ms->status = GE_UNKNOWN;
	if (ms) {
		/* GSM_DataClear(&gdat); */
		pthread_mutex_lock(&memoryMutex);
		gdat.MemoryStatus = &(ms->memoryStatus);
		error = ms->status = SM_Functions(GOP_GetMemoryStatus, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}
	return (error);
}


static gint A_GetMemoryLocation(gpointer data)
{
	GSM_Error error;
	D_MemoryLocation *ml = (D_MemoryLocation *) data;
	GSM_Data gdat;

	error = ml->status = GE_UNKNOWN;

	if (ml) {
		/* GSM_DataClear(&gdat); */
		pthread_mutex_lock(&memoryMutex);
		gdat.PhonebookEntry = (ml->entry);
		error = ml->status = SM_Functions(GOP_ReadPhonebook, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}

	return (error);
}


static gint A_GetMemoryLocationAll(gpointer data)
{
	GSM_PhonebookEntry entry;
	GSM_Error error;
	D_MemoryLocationAll *mla = (D_MemoryLocationAll *) data;
	register gint i, j, read = 0;

	error = mla->status = GE_NONE;
	entry.MemoryType = mla->type;
	/* GSM_DataClear(&gdat); */
	gdat.PhonebookEntry = &entry;

	pthread_mutex_lock(&memoryMutex);
	for (i = mla->min; i <= mla->max; i++) {
		entry.Location = i;
		error = SM_Functions(GOP_ReadPhonebook, &gdat, &statemachine);
		if (error != GE_NONE && error != GE_INVALIDPHBOOKLOCATION &&
		    error != GE_EMPTYMEMORYLOCATION && error != GE_INVALIDMEMORYTYPE) {
			gint err_count = 0;

			while (error != GE_NONE) {
				g_print(_
					("%s: line %d: Can't get memory entry number %d from memory %d! %d\n"),
					__FILE__, __LINE__, i, entry.MemoryType, error);
				if (err_count++ > 3) {
					mla->ReadFailed(i);
					mla->status = error;
					pthread_cond_signal(&memoryCond);
					pthread_mutex_unlock(&memoryMutex);
					return (error);
				}
				error = SM_Functions(GOP_ReadPhonebook, &gdat, &statemachine);
				sleep(2);
			}
		} else {
			if (error == GE_INVALIDMEMORYTYPE) {

		/* If the memory type was invalid - just fill up the rest */
		/* (Markus) */
		
				entry.Empty = true;
				entry.Name[0] = 0;
				entry.Number[0] = 0;
				for (j = mla->min; j <= mla->max; j++) error = mla->InsertEntry(&entry);
				pthread_cond_signal(&memoryCond);
				pthread_mutex_unlock(&memoryMutex);
				return GE_NONE;
			}
		}
		if (error == GE_NONE) read++;
		error = mla->InsertEntry(&entry); 
		/* FIXME: It only works this way at the moment */
		/*		if (error != GE_NONE)
				break;*/
		if (read == mla->used) {
			mla->status = error;
			entry.Empty = true;
			entry.Name[0] = 0;
			entry.Number[0] = 0;
			for (j = i + 1; j <= mla->max; j++) error = mla->InsertEntry(&entry);
			pthread_cond_signal(&memoryCond);
			pthread_mutex_unlock(&memoryMutex);
			return GE_NONE;
		}

	}
	mla->status = error;
	pthread_cond_signal(&memoryCond);
	pthread_mutex_unlock(&memoryMutex);
	return (error);
}


static gint A_WriteMemoryLocation(gpointer data)
{
	GSM_Error error;
	D_MemoryLocation *ml = (D_MemoryLocation *) data;
	GSM_Data gdat;

	error = ml->status = GE_UNKNOWN;

	/* GSM_DataClear(&gdat); */
	gdat.PhonebookEntry = (ml->entry);

	if (ml) {
		pthread_mutex_lock(&memoryMutex);
		error = ml->status = SM_Functions(GOP_WritePhonebook, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}

	return (error);
}

static gint A_WriteMemoryLocationAll(gpointer data)
{
	GSM_Error error;
	D_MemoryLocationAll *mla = (D_MemoryLocationAll *) data;

	error = mla->status = GE_UNKNOWN;

	return error;
}

static gint A_GetCalendarNote(gpointer data)
{
	GSM_Error error;
	D_CalendarNote *cn = (D_CalendarNote *) data;
	GSM_Data gdat;

	error = cn->status = GE_UNKNOWN;

	if (cn) {
		pthread_mutex_lock(&calendarMutex);
		/* GSM_DataClear(&gdat); */
		gdat.CalendarNote = cn->entry;
		error = cn->status = SM_Functions(GOP_GetCalendarNote, &gdat, &statemachine);
		pthread_cond_signal(&calendarCond);
		pthread_mutex_unlock(&calendarMutex);
	}

	return (error);
}


static gint A_GetCalendarNoteAll(gpointer data)
{
	GSM_CalendarNotesList list;
	GSM_CalendarNote entry;
	D_CalendarNoteAll *cna = (D_CalendarNoteAll *) data;
	GSM_Error error;
	GSM_Data gdat;
	register gint i = 1;

	pthread_mutex_lock(&calendarMutex);
	while (1) {
		entry.Location = i++;

		/* GSM_DataClear(&gdat); */
		gdat.CalendarNote = &entry;
		gdat.CalendarNotesList = &list;
		if ((error = SM_Functions(GOP_GetCalendarNote, &gdat, &statemachine)) != GE_NONE)
			break;

		if (cna->InsertEntry(&entry) != GE_NONE)
			break;
	}

	pthread_mutex_unlock(&calendarMutex);
	g_free(cna);
	if (error == GE_INVALIDCALNOTELOCATION)
		return (GE_NONE);
	else
		return (error);
}


static gint A_WriteCalendarNote(gpointer data)
{
	GSM_Error error;
	D_CalendarNote *cn = (D_CalendarNote *) data;
	GSM_Data gdat;

	error = cn->status = GE_UNKNOWN;

	if (cn) {
		pthread_mutex_lock(&calendarMutex);
		/* GSM_DataClear(&gdat); */
		gdat.CalendarNote = cn->entry;
		error = cn->status = SM_Functions(GOP_WriteCalendarNote, &gdat, &statemachine);
		pthread_cond_signal(&calendarCond);
		pthread_mutex_unlock(&calendarMutex);
	}

	return (error);
}


static gint A_DeleteCalendarNote(gpointer data)
{
	GSM_CalendarNote *note = (GSM_CalendarNote *) data;
	GSM_Error error = GE_UNKNOWN;
	GSM_Data gdat;

	if (note) {
		/* GSM_DataClear(&gdat); */
		gdat.CalendarNote = note;
		error = SM_Functions(GOP_DeleteCalendarNote, &gdat, &statemachine);
		g_free(note);
	}

	return (error);
}

static gint A_GetCallerGroup(gpointer data)
{
	GSM_Bitmap bitmap;
	GSM_Error error;
	D_CallerGroup *cg = (D_CallerGroup *) data;
	GSM_Data tmp;

	error = cg->status = GE_UNKNOWN;

	if (cg) {
		bitmap.type = GSM_CallerLogo;
		bitmap.number = cg->number;

		pthread_mutex_lock(&callerGroupMutex);
		GSM_DataClear(&tmp);
		tmp.Bitmap = &bitmap;
		error = cg->status = SM_Functions(GOP_GetBitmap, &tmp, &statemachine);
		strncpy(cg->text, bitmap.text, 256);
		cg->text[255] = '\0';
		pthread_cond_signal(&callerGroupCond);
		pthread_mutex_unlock(&callerGroupMutex);
	}
	return (error);
}


static gint A_SendCallerGroup(gpointer data)
{
	GSM_Bitmap bitmap;
	D_CallerGroup *cg = (D_CallerGroup *) data;
	GSM_Error error;
	GSM_Data gdat;

	if (!cg)
		return (GE_UNKNOWN);

	bitmap.type = GSM_CallerLogo;
	bitmap.number = cg->number;
	/* GSM_DataClear(&gdat); */
	gdat.Bitmap = &bitmap;
	if ((error = SM_Functions(GOP_GetBitmap, &gdat, &statemachine)) != GE_NONE) {
		g_free(cg);
		return (error);
	}
	strncpy(bitmap.text, cg->text, 256);
	bitmap.text[255] = '\0';
	g_free(cg);
	return (SM_Functions(GOP_SetBitmap, &gdat, &statemachine));
}


static gint A_GetSMSCenter(gpointer data)
{
	D_SMSCenter *c = (D_SMSCenter *) data;
	GSM_Error error;
	GSM_Data tmp_gdat;

	error = c->status = GE_UNKNOWN;
	if (c) {
		pthread_mutex_lock(&smsCenterMutex);
		GSM_DataClear(&tmp_gdat);
		tmp_gdat.MessageCenter = c->center;
		error = c->status = SM_Functions(GOP_GetSMSCenter, &tmp_gdat, &statemachine);
		pthread_cond_signal(&smsCenterCond);
		pthread_mutex_unlock(&smsCenterMutex);
	}
	return (error);
}


static gint A_SetSMSCenter(gpointer data)
{
	D_SMSCenter *smsc = (D_SMSCenter *) data;
	GSM_Error error;
	GSM_Data tmp_gdat;

	error = smsc->status = GE_UNKNOWN;
	if (smsc) {
		GSM_DataClear(&tmp_gdat);
		tmp_gdat.MessageCenter = smsc->center;
		pthread_mutex_lock(&smsCenterMutex);
		error = smsc->status = SM_Functions(GOP_GetSMSCenter, &tmp_gdat, &statemachine);
		g_free(smsc);
		pthread_cond_signal(&smsCenterCond);
		pthread_mutex_unlock(&smsCenterMutex);
	}
	return (error);
}


static gint A_SendSMSMessage(gpointer data)
{
	D_SMSMessage *d = (D_SMSMessage *) data;
	GSM_Error error;
	GSM_Data tmp_gdat;

	error = d->status = GE_UNKNOWN;
	if (d) {
		GSM_DataClear(&tmp_gdat);
		tmp_gdat.SMS = d->sms;
		pthread_mutex_lock(&sendSMSMutex);
		error = d->status = SendSMS(&tmp_gdat, &statemachine);
		pthread_cond_signal(&sendSMSCond);
		pthread_mutex_unlock(&sendSMSMutex);
	}
	return (error);
}


static gint A_DeleteSMSMessage(gpointer data)
{
	GSM_API_SMS *sms = (GSM_API_SMS *) data;
	GSM_Error error = GE_UNKNOWN;
	GSM_Data tmp_gdat;

	if (sms) {
		GSM_DataClear(&tmp_gdat);
		tmp_gdat.SMS = sms;
		error = SM_Functions(GOP_DeleteSMS, &tmp_gdat, &statemachine);
		g_free(sms);
	}
	return (error);
}


static gint A_GetSpeedDial(gpointer data)
{
	D_SpeedDial *d = (D_SpeedDial *) data;
	GSM_Error error;
	GSM_Data gdat;

	error = d->status = GE_UNKNOWN;

	if (d) {
		/* GSM_DataClear(&gdat); */
		gdat.SpeedDial = &d->entry;
		pthread_mutex_lock(&speedDialMutex);
		error = d->status = SM_Functions(GOP_GetSpeedDial, &gdat, &statemachine);
		pthread_cond_signal(&speedDialCond);
		pthread_mutex_unlock(&speedDialMutex);
	}

	return (error);
}


static gint A_SendSpeedDial(gpointer data)
{
	D_SpeedDial *d = (D_SpeedDial *) data;
	GSM_Error error;

	error = d->status = GE_UNKNOWN;

	if (d) {
		//pthread_mutex_lock (&speedDialMutex);
//    error = d->status = GSM->SetSpeedDial (&(d->entry));
		g_free(d);
		//pthread_cond_signal (&speedDialCond);
		//pthread_mutex_unlock (&speedDialMutex);
	}

	return (error);
}


static gint A_SendDTMF(gpointer data)
{
	gchar *buf = (gchar *) data;
	GSM_Error error = GE_UNKNOWN;

	if (buf) {
//    error = GSM->SendDTMF (buf);
		g_free(buf);
	}

	return (error);
}


static gint A_NetMonOnOff(gpointer data)
{
	gint mode = GPOINTER_TO_INT(data);
	GSM_Data gdat;
	GSM_NetMonitor nm;

	/* GSM_DataClear(&gdat); */
	gdat.NetMonitor = &nm;
	if (mode)
		nm.Field = 0xf3;
	else
		nm.Field = 0xf1;
	return SM_Functions(GOP_NetMonitor, &gdat, &statemachine);
}


static gint A_NetMonitor(gpointer data)
{
	gint number = GPOINTER_TO_INT(data);

	if (data == 0)
		phoneMonitor.netmonitor.number = 0;
	else
		phoneMonitor.netmonitor.number = number;

	return (0);
}


static gint A_DialVoice(gpointer data)
{
	gchar *number = (gchar *) data;
	GSM_Error error = GE_UNKNOWN;

	if (number) {
//    error = GSM->DialVoice (number);
		g_free(number);
	}

	return (error);
}


static gint A_GetAlarm(gpointer data)
{
	D_Alarm *a = (D_Alarm *) data;
	GSM_Error error;
	GSM_Data gdat;

	error = GE_UNKNOWN;

	if (a) {
		a->status = GE_UNKNOWN;
		pthread_mutex_lock(&alarmMutex);
/*    GSM_DataClear(&gdat); */
		gdat.DateTime = &a->time;
		error = a->status = SM_Functions(GOP_GetAlarm, &gdat, &statemachine);
		pthread_cond_signal(&alarmCond);
		pthread_mutex_unlock(&alarmMutex);
	}

	return (error);
}


static gint A_SetAlarm(gpointer data)
{
	D_Alarm *a = (D_Alarm *) data;
	GSM_Error error;
	GSM_Data gdat;

	error = a->status = GE_UNKNOWN;

	if (a) {
/*		GSM_DataClear(&gdat); */
		gdat.DateTime = &a->time;
		error = a->status = SM_Functions(GOP_SetAlarm, &gdat, &statemachine);
		g_free(a);
	}
	return (error);
}


static gint A_PressKey(gpointer data)
{
	gdat.KeyCode = GPOINTER_TO_INT(data);

	return SM_Functions(GOP_PressPhoneKey, &gdat, &statemachine);
}

static gint A_ReleaseKey(gpointer data)
{
	gdat.KeyCode = GPOINTER_TO_INT(data);

	return SM_Functions(GOP_ReleasePhoneKey, &gdat, &statemachine);
}

static gint A_GetBitmap(gpointer data)
{
	GSM_Error error;
	D_Bitmap *d = (D_Bitmap *) data;
	GSM_Data gdat;

/*	GSM_DataClear(&gdat); */
	pthread_mutex_lock(&getBitmapMutex);
	gdat.Bitmap = d->bitmap;
	error = d->status = SM_Functions(GOP_GetBitmap, &gdat, &statemachine);
	pthread_cond_signal(&getBitmapCond);
	pthread_mutex_unlock(&getBitmapMutex);
	return error;
}

static gint A_SetBitmap(gpointer data)
{
	GSM_Error error;
	D_Bitmap *d = (D_Bitmap *) data;
	GSM_Bitmap bitmap;
	GSM_Data gdat;

/* GSM_DataClear(&gdat); */
	pthread_mutex_lock(&setBitmapMutex);
	if (d->bitmap->type == GSM_CallerLogo) {
		bitmap.type = d->bitmap->type;
		bitmap.number = d->bitmap->number;
		gdat.Bitmap = &bitmap;
		error = d->status = SM_Functions(GOP_GetBitmap, &gdat, &statemachine);
		if (error == GE_NONE) {
			strncpy(d->bitmap->text, bitmap.text, sizeof(bitmap.text));
			d->bitmap->ringtone = bitmap.ringtone;
			gdat.Bitmap = d->bitmap;
			error = d->status = SM_Functions(GOP_SetBitmap, &gdat, &statemachine);
		}
	} else {
		gdat.Bitmap = d->bitmap;
		error = d->status = SM_Functions(GOP_SetBitmap, &gdat, &statemachine);
	}
	pthread_cond_signal(&setBitmapCond);
	pthread_mutex_unlock(&setBitmapMutex);
	return error;
}

static gint A_GetNetworkInfo(gpointer data)
{
	GSM_Error error;
	D_NetworkInfo *d = (D_NetworkInfo *) data;
	GSM_Data tmp;

	GSM_DataClear(&tmp);

	pthread_mutex_lock(&getNetworkInfoMutex);
	tmp.NetworkInfo = d->info;
	error = d->status = SM_Functions(GOP_GetNetworkInfo, &tmp, &statemachine);
	pthread_cond_signal(&getNetworkInfoCond);
	pthread_mutex_unlock(&getNetworkInfoMutex);
	return error;
}

static gint A_Exit(gpointer data)
{
	pthread_exit(0);
	return (0);		/* just to be proper */
}


gint(*DoAction[])(gpointer) = {
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
	    A_PressKey,
	    A_ReleaseKey,
	    A_GetBitmap,
	    A_SetBitmap,
	    A_GetNetworkInfo,
	    A_Exit};


void *GUI_Connect(void *a)
{
/* Define required unit types for RF and Battery level meters. */
	GSM_RFUnits rf_units = GRF_Percentage;
	GSM_BatteryUnits batt_units = GBU_Percentage;
	GSM_DateTime Alarm;
	GSM_NetMonitor netmonitor;
	gint displaystatus;
	time_t newtime, oldtime;

	gchar number[INCALL_NUMBER_LENGTH];
	PhoneEvent *event;
	GSM_Error error;

#ifdef XDEBUG
	g_print("Initializing connection...\n");
#endif

	phoneMonitor.working = _("Connecting...");
	while (fbusinit(true) != GE_NONE)
		sleep(1);

#ifdef XDEBUG
	g_print("Phone connected. Starting monitoring...\n");
#endif

	sleep(1);

	gdat.RFLevel = &phoneMonitor.rfLevel;
	gdat.RFUnits = &rf_units;
	gdat.PowerSource = &phoneMonitor.powerSource;
	gdat.BatteryUnits = &batt_units;
	gdat.BatteryLevel = &phoneMonitor.batteryLevel;
	gdat.DateTime = &Alarm;
	gdat.IncomingCallNr = number;
	oldtime = time(&oldtime);
	oldtime += 2;

	while (1) {
		phoneMonitor.working = NULL;

/* FIXME - this loop goes mad on my 7110 - so I've put in a usleep */
		usleep(50000);

		if (SM_Functions(GOP_GetRFLevel, &gdat, &statemachine) != GE_NONE)
			phoneMonitor.rfLevel = -1;

		if (rf_units == GRF_Arbitrary)
			phoneMonitor.rfLevel *= 25;

		if (SM_Functions(GOP_GetPowersource, &gdat, &statemachine) == GE_NONE
		    && phoneMonitor.powerSource == GPS_ACDC)
			phoneMonitor.batteryLevel = ((gint) phoneMonitor.batteryLevel + 25) % 125;
		else {
			if (SM_Functions(GOP_GetBatteryLevel, &gdat, &statemachine) != GE_NONE)
				phoneMonitor.batteryLevel = -1;
			if (batt_units == GBU_Arbitrary)
				phoneMonitor.batteryLevel *= 25;
		}

		if (SM_Functions(GOP_GetAlarm, &gdat, &statemachine) == GE_NONE &&
		    Alarm.AlarmEnabled != 0)
			phoneMonitor.alarm = TRUE;
		else
			phoneMonitor.alarm = FALSE;

		if (SM_Functions(GOP_GetIncomingCallNr, &gdat, &statemachine) == GE_NONE) {
#ifdef XDEBUG
			g_print("Call in progress: %s\n", phoneMonitor.call.callNum);
#endif
			gdat.DisplayStatus = &displaystatus;
			SM_Functions(GOP_GetDisplayStatus, &gdat, &statemachine);
			if (displaystatus & (1<<DS_Call_In_Progress)) {
				pthread_mutex_lock (&callMutex);
				phoneMonitor.call.callInProgress = CS_InProgress;
				pthread_mutex_unlock (&callMutex);
			} else {
				pthread_mutex_lock (&callMutex);
				phoneMonitor.call.callInProgress = CS_Waiting;
				strncpy (phoneMonitor.call.callNum, number, INCALL_NUMBER_LENGTH);
				pthread_mutex_unlock (&callMutex);
			}
		} else {
			pthread_mutex_lock(&callMutex);
			phoneMonitor.call.callInProgress = CS_Idle;
			*phoneMonitor.call.callNum = '\0';
			pthread_mutex_unlock(&callMutex);
		}

		pthread_mutex_lock(&netMonMutex);

		if (phoneMonitor.netmonitor.number) {

			gdat.NetMonitor = &netmonitor;
			gdat.NetMonitor->Field = phoneMonitor.netmonitor.number;
			SM_Functions(GOP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen, gdat.NetMonitor->Screen, 
			       sizeof(gdat.NetMonitor->Screen));

			gdat.NetMonitor->Field = 3;
			SM_Functions(GOP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen3, gdat.NetMonitor->Screen, 
			       sizeof(gdat.NetMonitor->Screen));

			gdat.NetMonitor->Field = 4;
			SM_Functions(GOP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen4, gdat.NetMonitor->Screen, 
			       sizeof(gdat.NetMonitor->Screen));

			gdat.NetMonitor->Field = 5;
			SM_Functions(GOP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen5, gdat.NetMonitor->Screen, 
			       sizeof(gdat.NetMonitor->Screen));

		} else {
			*phoneMonitor.netmonitor.screen = *phoneMonitor.netmonitor.screen3 =
			    *phoneMonitor.netmonitor.screen4 = *phoneMonitor.netmonitor.screen5 =
			    '\0';
		}
		pthread_mutex_unlock(&netMonMutex);

		while ((event = RemoveEvent()) != NULL) {
#ifdef XDEBUG
			g_print("Processing Event: %d\n", event->event);
#endif
			phoneMonitor.working = _("Working...");
			if (event->event <= Event_Exit)
				if ((error = DoAction[event->event] (event->data)) != GE_NONE)
					g_print(_("Event %d failed with return code %d!\n"),
						event->event, error);
			g_free(event);
		}
		newtime = time(&newtime);
		if ((newtime < oldtime + 2) || !isSMSactivated)  continue;
		oldtime = newtime;

		if ((GetFolderChanges(&gdat, &statemachine, (phoneMonitor.supported & PM_FOLDERS)))
		    == GE_NONE) {
			dprintf("old UR: %i, new UR: %i, old total: %i, new total: %i\n",
				phoneMonitor.sms.unRead, gdat.SMSStatus->Unread,
				phoneMonitor.sms.number, gdat.SMSStatus->Number);
			phoneMonitor.sms.changed += gdat.SMSStatus->Changed;
			if (gdat.SMSStatus->Changed) {
				phoneMonitor.working = _("Refreshing SMSes...");
				RefreshSMS(gdat.SMSStatus->Number);
				phoneMonitor.working = NULL;
			}
			phoneMonitor.sms.unRead = gdat.SMSStatus->Unread;
		}

	}
}
