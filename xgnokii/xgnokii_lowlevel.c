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
#include "gnokii.h"
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

static char *lockfile = NULL;

gn_sms_status SMSStatus = {0,0,0,0};
gn_sms_folder_stats FolderStats[GN_SMS_FOLDER_MAX_NUMBER];
gn_sms_message_list MessagesList[GN_SMS_MESSAGE_MAX_NUMBER][GN_SMS_FOLDER_MAX_NUMBER];

struct gn_statemachine statemachine;
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

gn_error GUI_InitSMSFolders(void)
{
	gn_data gdat;
	gn_error error;
	gn_sms_folder_list list;
	gint i;

	gn_data_clear(&gdat);
	if (phoneMonitor.supported & PM_FOLDERS) {

		gdat.sms_folder_list = &list;

		if ((error = gn_sm_functions(GN_OP_GetSMSFolders, &gdat, &statemachine)) != GN_ERR_NONE)
			return error;
		foldercount = gdat.sms_folder_list->number;
		for (i = 0; i < gdat.sms_folder_list->number; i++) {
			strcpy(folders[i], gdat.sms_folder_list->folder[i].name);
		}
	} else {
		foldercount = 2;
		strcpy(folders[0], "Inbox");
		strcpy(folders[1], "Outbox");
	}
	return GN_ERR_NONE;
}

static inline void FreeElement(gpointer data, gpointer userData)
{
	g_free((gn_sms *) data);
}

static inline void FreeArray(GSList ** array)
{
	if (*array) {
		g_slist_foreach(*array, FreeElement, NULL);
		g_slist_free(*array);
		*array = NULL;
	}
}

static gn_error InitModelInf(void)
{
	gn_data gdat;
	gn_error error;
	gchar buf[64];
	gint i, j;

	gn_data_clear(&gdat);

	gdat.model = buf;
	error = gn_sm_functions(GN_OP_GetModel, &gdat, &statemachine);
	if (error != GN_ERR_NONE) return error;
	phoneMonitor.phone.version = g_malloc(sizeof(gdat.model));
	phoneMonitor.phone.model = gn_model_get(gdat.model);

	if (phoneMonitor.phone.model == NULL)
		phoneMonitor.phone.model = g_strdup(_("unknown"));
	phoneMonitor.supported = gn_phone_model_get(buf)->flags;

	gdat.revision = buf;
	gdat.model = NULL;
	error = gn_sm_functions(GN_OP_GetRevision, &gdat, &statemachine);
	if (error != GN_ERR_NONE) return error;
	phoneMonitor.phone.version = g_malloc(sizeof(gdat.revision));
	phoneMonitor.phone.revision = g_malloc(sizeof(gdat.revision));
	phoneMonitor.phone.version = g_strdup(gdat.revision);
	phoneMonitor.phone.revision = g_strdup(gdat.revision);

	gdat.revision = NULL;
	gdat.model = NULL;
	gdat.imei = buf;
	error = gn_sm_functions(GN_OP_GetImei, &gdat, &statemachine);
	if (error != GN_ERR_NONE) return error;
	phoneMonitor.phone.imei = g_malloc(sizeof(gdat.imei));
	phoneMonitor.phone.imei = g_strdup(gdat.imei);

	for (i = 0; i < GN_SMS_FOLDER_MAX_NUMBER; i++) {
		FolderStats[i].number = 0;
		FolderStats[i].changed = 0;
		FolderStats[i].unread = 0;
		FolderStats[i].used = 0;
		for (j = 0; j < GN_SMS_MESSAGE_MAX_NUMBER; j++) {
			MessagesList[j][i].status = GN_SMS_FLD_Old;
			MessagesList[j][i].location = 0;
			MessagesList[j][i].message_type = GN_SMS_MT_Deliver;
		}
	}

#ifdef XDEBUG
	g_print("Version: %s\n", phoneMonitor.phone.version);
	g_print("Model: %s\n", phoneMonitor.phone.model);
	g_print("IMEI: %s\n", phoneMonitor.phone.imei);
	g_print("Revision: %s\n", phoneMonitor.phone.revision);
#endif
	return GN_ERR_NONE;
}


static void busterminate(void)
{
	gn_sm_functions(GN_OP_Terminate, NULL, &statemachine);
	if (lockfile) gn_device_unlock(lockfile);
}


static gn_error fbusinit(bool enable_monitoring)
{
	static gn_error error = GN_ERR_NOLINK;
	static bool atexit_registered = false;
	char *aux;

	/* register cleanup function */
	if (!atexit_registered) {
		atexit_registered = true;
		atexit(busterminate);
	}

	aux = gn_cfg_get(gn_cfg_info, "global", "use_locking");
	/* Defaults to 'no' */
	if (aux && !strcmp(aux, "yes")) {
		lockfile = gn_device_lock(xgnokiiConfig.port);
		if (lockfile == NULL) {
			fprintf(stderr, _("Lock file error. Exiting\n"));
			exit(1);
		}
	}

	/* Initialise the code for the GSM interface. */
	error = gn_gsm_initialise(&statemachine);

#ifdef XDEBUG
	g_print("fbusinit: error %d\n", error);
#endif

	if (error != GN_ERR_NONE) {
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
	phoneMonitor.powerSource = GN_PS_BATTERY;
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

static gint compare_number(const gn_sms * a, const gn_sms * b)
{
	dprintf("a: %i b:%i\n", a->number, b->number);
	if (a->number == b->number)
		return 0;
	else
		return 1;
}

static gint compare_folder_and_number(const gn_sms *a, const gn_sms *b)
{
	dprintf("memory type a: %i memory type b: %i\n", a->memory_type, b->memory_type);
	dprintf("message number a: %i message number b: %i\n", a->number, b->number);
	if (a->memory_type == b->memory_type)
		if (a->number == b->number)
			return 0;
		else
			return 1;
	else
		return 1;
}


static void RefreshSMS(const gint number)
{
	gn_data gdat;
	gn_error error;
	gn_sms *msg, *tmp_msg;
	gn_sms_folder *fld;
	gn_sms_folder_list *list;
	gn_sms_raw *raw;
	GSList *tmp_list;
	gint i, j, dummy;

#ifdef XDEBUG
	g_print("RefreshSMS is running...\n");
#endif

	gn_data_clear(&gdat);

	dprintf("RefreshSMS: changed: %i\n", SMSStatus.changed);
	dprintf("RefreshSMS: unread: %i, total: %i\n", SMSStatus.unread,
		SMSStatus.number);
	for (i = 0; i < SMSStatus.folders_count; i++) {
		dummy = 0;
		for (j = 0; j < FolderStats[i].used; j++) {
			if ((MessagesList[j][i].status == GN_SMS_FLD_Changed) ||
			    (MessagesList[j][i].status == GN_SMS_FLD_NotRead) ||
			    (MessagesList[j][i].status == GN_SMS_FLD_New))
				dprintf("RefreshSMS: change #%i in folder %i at location %i!\n",
					++dummy, i, MessagesList[j][i].location);
		}
	}
	if (phoneMonitor.supported & PM_FOLDERS) {

		for (i = 0; i < SMSStatus.folders_count; i++) {
			for (j = 0; j < FolderStats[i].used; j++) {
				if (MessagesList[j][i].status == GN_SMS_FLD_Deleted ||
				    MessagesList[j][i].status == GN_SMS_FLD_Changed) {
					dprintf("We got a deleted message here to handle!\n");
					pthread_mutex_lock(&smsMutex);
					msg = g_malloc0(sizeof(gn_sms));

/* 0 + 12 = GN_MT_IN, 1 + 12 = GN_MT_OU ... => gsm-common.h definition of gn_memory_type */ 
					msg->memory_type = i + 12;
					msg->number = MessagesList[j][i].location;
					tmp_list =
					    g_slist_find_custom(phoneMonitor.sms.messages, msg,
								(GCompareFunc) compare_folder_and_number);
					tmp_msg = (gn_sms *) tmp_list->data;
					phoneMonitor.sms.messages =
					    g_slist_remove(phoneMonitor.sms.messages, tmp_msg);
					g_free(tmp_msg);
					pthread_mutex_unlock(&smsMutex);
					if (MessagesList[j][i].status == GN_SMS_FLD_Deleted)
						MessagesList[j][i].status = GN_SMS_FLD_ToBeRemoved;	/* FreeDeletedMessages has to find it */
				}
				if (MessagesList[j][i].status == GN_SMS_FLD_New ||
				    MessagesList[j][i].status == GN_SMS_FLD_NotRead ||
				    MessagesList[j][i].status == GN_SMS_FLD_Changed) {
					msg = g_malloc0(sizeof(gn_sms));
					list = g_malloc(sizeof(gn_sms_folder_list));
					raw = g_malloc(sizeof(gn_sms_raw));
					gdat.sms = msg;
					gdat.sms_folder = NULL;
					gdat.sms_folder_list = list;
					gdat.raw_sms = raw;

					gdat.sms->number = MessagesList[j][i].location;
					gdat.sms->memory_type =(gn_memory_type) i + 12 ;
					dprintf("#: %i, mt: %i\n", gdat.sms->number, gdat.sms->memory_type);
					if ((error = gn_sms_get_no_validate(&gdat, &statemachine)) == GN_ERR_NONE) {
						dprintf("Found valid SMS ...\n %s\n",
							msg->user_data[0].u.text);
						pthread_mutex_lock(&smsMutex);
						phoneMonitor.sms.messages =
						    g_slist_append(phoneMonitor.sms.messages, msg);
						pthread_mutex_unlock(&smsMutex);
					}
					if (MessagesList[j][i].status == GN_SMS_FLD_New ||
					    MessagesList[j][i].status == GN_SMS_FLD_Changed)
						MessagesList[j][i].status = GN_SMS_FLD_Old;
					if (MessagesList[j][i].status == GN_SMS_FLD_NotRead)
						MessagesList[j][i].status = GN_SMS_FLD_NotReadHandled;
				}
			}
			FolderStats[i].changed = 0;	/* now we handled the changes and can reset */
		}
		SMSStatus.changed = 0;	/* now we handled the changes and can reset */

	} else {
		pthread_mutex_lock(&smsMutex);
		FreeArray(&(phoneMonitor.sms.messages));
		phoneMonitor.sms.number = 0;
		pthread_mutex_unlock(&smsMutex);

		fld = g_malloc(sizeof(gn_sms_folder));
		list = g_malloc(sizeof(gn_sms_folder_list));
		gdat.sms_folder = fld;
		gdat.sms_folder_list = list;

		i = 0;
		while (1) {
			i++;
			msg = g_malloc0(sizeof(gn_sms));

			msg->memory_type = GN_MT_SM;
			msg->number = i;

			gdat.sms = msg;
			if ((error = gn_sms_get(&gdat, &statemachine)) == GN_ERR_NONE) {
				dprintf("Found valid SMS ...\n");
				pthread_mutex_lock(&smsMutex);
				phoneMonitor.sms.messages =
				    g_slist_append(phoneMonitor.sms.messages, msg);
				phoneMonitor.sms.number++;
				pthread_mutex_unlock(&smsMutex);
				if (phoneMonitor.sms.number == number) {
					g_free(list);
					g_free(fld);
					g_free(msg);
					return;
				}
			} else if (error == GN_ERR_INVALIDLOCATION) {	/* All positions are read */
				g_free(list);
				g_free(fld);
				g_free(msg);
				break;
			} else {
				g_free(msg);
				usleep(750000);
			}
		}
	}
}

static gint A_GetMemoryStatus(gpointer data)
{
	gn_error error;
	D_MemoryStatus *ms = (D_MemoryStatus *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = ms->status = GN_ERR_UNKNOWN;
	if (ms) {
		pthread_mutex_lock(&memoryMutex);
		gdat.memory_status = &(ms->memoryStatus);
		error = ms->status = gn_sm_functions(GN_OP_GetMemoryStatus, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}
	return (error);
}


static gint A_GetMemoryLocation(gpointer data)
{
	gn_error error;
	D_MemoryLocation *ml = (D_MemoryLocation *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = ml->status = GN_ERR_UNKNOWN;

	if (ml) {
		pthread_mutex_lock(&memoryMutex);
		gdat.phonebook_entry = (ml->entry);
		error = ml->status = gn_sm_functions(GN_OP_ReadPhonebook, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}

	return (error);
}


static gint A_GetMemoryLocationAll(gpointer data)
{
	gn_phonebook_entry entry;
	gn_error error;
	D_MemoryLocationAll *mla = (D_MemoryLocationAll *)data;
	register gint i, j, read = 0;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = mla->status = GN_ERR_NONE;
	entry.memory_type = mla->type;
	gdat.phonebook_entry = &entry;

	pthread_mutex_lock(&memoryMutex);
	for (i = mla->min; i <= mla->max; i++) {
		entry.location = i;
		error = gn_sm_functions(GN_OP_ReadPhonebook, &gdat, &statemachine);
		if (error != GN_ERR_NONE && error != GN_ERR_INVALIDLOCATION &&
		    error != GN_ERR_EMPTYLOCATION && error != GN_ERR_INVALIDMEMORYTYPE) {
			gint err_count = 0;

			while (error != GN_ERR_NONE) {
				g_print(_
					("%s: line %d: Can't get memory entry number %d from memory %d! %d\n"),
					__FILE__, __LINE__, i, entry.memory_type, error);
				if (err_count++ > 3) {
					mla->ReadFailed(i);
					mla->status = error;
					pthread_cond_signal(&memoryCond);
					pthread_mutex_unlock(&memoryMutex);
					return (error);
				}
				error = gn_sm_functions(GN_OP_ReadPhonebook, &gdat, &statemachine);
				sleep(2);
			}
		} else {
			if (error == GN_ERR_INVALIDMEMORYTYPE) {

		/* If the memory type was invalid - just fill up the rest */
		/* (Markus) */
		
				entry.empty = true;
				entry.name[0] = 0;
				entry.number[0] = 0;
				for (j = mla->min; j <= mla->max; j++) error = mla->InsertEntry(&entry);
				pthread_cond_signal(&memoryCond);
				pthread_mutex_unlock(&memoryMutex);
				return GN_ERR_NONE;
			}
		}
		if (error == GN_ERR_NONE) read++;
		dprintf("Name: %s\n", entry.name);
		error = mla->InsertEntry(&entry); 
		/* FIXME: It only works this way at the moment */
		/*		if (error != GN_ERR_NONE)
				break;*/
		if (read == mla->used) {
			mla->status = error;
			entry.empty = true;
			entry.name[0] = 0;
			entry.number[0] = 0;
			for (j = i + 1; j <= mla->max; j++) error = mla->InsertEntry(&entry);
			pthread_cond_signal(&memoryCond);
			pthread_mutex_unlock(&memoryMutex);
			return GN_ERR_NONE;
		}

	}
	mla->status = error;
	pthread_cond_signal(&memoryCond);
	pthread_mutex_unlock(&memoryMutex);
	return (error);
}


static gint A_WriteMemoryLocation(gpointer data)
{
	gn_error error;
	D_MemoryLocation *ml = (D_MemoryLocation *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = ml->status = GN_ERR_UNKNOWN;

	gdat.phonebook_entry = (ml->entry);

	if (ml) {
		pthread_mutex_lock(&memoryMutex);
		error = ml->status = gn_sm_functions(GN_OP_WritePhonebook, &gdat, &statemachine);
		pthread_cond_signal(&memoryCond);
		pthread_mutex_unlock(&memoryMutex);
	}

	return (error);
}

static gint A_WriteMemoryLocationAll(gpointer data)
{
	gn_error error;
	D_MemoryLocationAll *mla = (D_MemoryLocationAll *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = mla->status = GN_ERR_UNKNOWN;

	return error;
}

static gint A_GetCalendarNote(gpointer data)
{
	gn_error error;
	D_CalendarNote *cn = (D_CalendarNote *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = cn->status = GN_ERR_UNKNOWN;

	if (cn) {
		pthread_mutex_lock(&calendarMutex);
		gdat.calnote = cn->entry;
		error = cn->status = gn_sm_functions(GN_OP_GetCalendarNote, &gdat, &statemachine);
		pthread_cond_signal(&calendarCond);
		pthread_mutex_unlock(&calendarMutex);
	}

	return (error);
}


static gint A_GetCalendarNoteAll(gpointer data)
{
	gn_calnote_list list;
	gn_calnote entry;
	D_CalendarNoteAll *cna = (D_CalendarNoteAll *) data;
	gn_error error;
	register gint i = 1;
	gn_data gdat;

	gn_data_clear(&gdat);

	pthread_mutex_lock(&calendarMutex);
	while (1) {
		entry.location = i;

		gdat.calnote = &entry;
		gdat.calnote_list = &list;
		if ((error = gn_sm_functions(GN_OP_GetCalendarNote, &gdat, &statemachine)) != GN_ERR_NONE)
			break;
		/* This is necessary for phones with calendar notes index (7110/6510) */
		entry.location = i++;
		if (cna->InsertEntry(&entry) != GN_ERR_NONE)
			break;
	}

	pthread_mutex_unlock(&calendarMutex);
	g_free(cna);
	if (error == GN_ERR_INVALIDLOCATION)
		return GN_ERR_NONE;
	else
		return error;
}


static gint A_WriteCalendarNote(gpointer data)
{
	gn_error error;
	D_CalendarNote *cn = (D_CalendarNote *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = cn->status = GN_ERR_UNKNOWN;

	if (cn) {
		pthread_mutex_lock(&calendarMutex);
		gdat.calnote = cn->entry;
		error = cn->status = gn_sm_functions(GN_OP_WriteCalendarNote, &gdat, &statemachine);
		pthread_cond_signal(&calendarCond);
		pthread_mutex_unlock(&calendarMutex);
	}

	return (error);
}


static gint A_DeleteCalendarNote(gpointer data)
{
	gn_calnote *note = (gn_calnote *) data;
	gn_error error = GN_ERR_UNKNOWN;
	gn_data gdat;

	gn_data_clear(&gdat);

	if (note) {
		gdat.calnote = note;
		error = gn_sm_functions(GN_OP_DeleteCalendarNote, &gdat, &statemachine);
		g_free(note);
	}

	return (error);
}

static gint A_GetCallerGroup(gpointer data)
{
	gn_bmp bitmap;
	gn_error error;
	D_CallerGroup *cg = (D_CallerGroup *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = cg->status = GN_ERR_UNKNOWN;

	if (cg) {
		bitmap.type = GN_BMP_CallerLogo;
		bitmap.number = cg->number;

		pthread_mutex_lock(&callerGroupMutex);
		gdat.bitmap = &bitmap;
		error = cg->status = gn_sm_functions(GN_OP_GetBitmap, &gdat, &statemachine);
		snprintf(cg->text, 256, "%s", bitmap.text);
		pthread_cond_signal(&callerGroupCond);
		pthread_mutex_unlock(&callerGroupMutex);
	}
	return (error);
}


static gint A_SendCallerGroup(gpointer data)
{
	gn_bmp bitmap;
	D_CallerGroup *cg = (D_CallerGroup *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	if (!cg)
		return GN_ERR_UNKNOWN;

	bitmap.type = GN_BMP_CallerLogo;
	bitmap.number = cg->number;
	gdat.bitmap = &bitmap;
	if ((error = gn_sm_functions(GN_OP_GetBitmap, &gdat, &statemachine)) != GN_ERR_NONE) {
		g_free(cg);
		return (error);
	}
	strncpy(bitmap.text, cg->text, 256);
	bitmap.text[255] = '\0';
	g_free(cg);
	return (gn_sm_functions(GN_OP_SetBitmap, &gdat, &statemachine));
}


static gint A_GetSMSCenter(gpointer data)
{
	D_SMSCenter *c = (D_SMSCenter *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = c->status = GN_ERR_UNKNOWN;
	if (c) {
		pthread_mutex_lock(&smsCenterMutex);
		gdat.message_center = c->center;
		error = c->status = gn_sm_functions(GN_OP_GetSMSCenter, &gdat, &statemachine);
		pthread_cond_signal(&smsCenterCond);
		pthread_mutex_unlock(&smsCenterMutex);
	}
	return (error);
}


static gint A_SetSMSCenter(gpointer data)
{
	D_SMSCenter *smsc = (D_SMSCenter *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = smsc->status = GN_ERR_UNKNOWN;
	if (smsc) {
		gdat.message_center = smsc->center;
		pthread_mutex_lock(&smsCenterMutex);
		error = smsc->status = gn_sm_functions(GN_OP_GetSMSCenter, &gdat, &statemachine);
		g_free(smsc);
		pthread_cond_signal(&smsCenterCond);
		pthread_mutex_unlock(&smsCenterMutex);
	}
	return (error);
}


static gint A_SendSMSMessage(gpointer data)
{
	D_SMSMessage *d = (D_SMSMessage *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = d->status = GN_ERR_UNKNOWN;
	if (d) {
		gdat.sms = d->sms;
		pthread_mutex_lock(&sendSMSMutex);
		error = d->status = gn_sms_send(&gdat, &statemachine);
		pthread_cond_signal(&sendSMSCond);
		pthread_mutex_unlock(&sendSMSMutex);
	}
	return (error);
}


static gint A_DeleteSMSMessage(gpointer data)
{
	gn_sms *sms = (gn_sms *) data;
	gn_error error = GN_ERR_UNKNOWN;
	gn_data gdat;

	gn_data_clear(&gdat);

	if (sms) {
		gdat.sms = sms;
		if (phoneMonitor.supported & PM_FOLDERS)
			error = gn_sms_delete_no_validate(&gdat, &statemachine);
		else
			error = gn_sms_delete(&gdat, &statemachine);
		g_free(sms);
	}
	return (error);
}


static gint A_GetSpeedDial(gpointer data)
{
	D_SpeedDial *d = (D_SpeedDial *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = d->status = GN_ERR_UNKNOWN;

	if (d) {
		gdat.speed_dial = &d->entry;
		pthread_mutex_lock(&speedDialMutex);
		error = d->status = gn_sm_functions(GN_OP_GetSpeedDial, &gdat, &statemachine);
		pthread_cond_signal(&speedDialCond);
		pthread_mutex_unlock(&speedDialMutex);
	}

	return (error);
}


static gint A_SendSpeedDial(gpointer data)
{
	D_SpeedDial *d = (D_SpeedDial *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = d->status = GN_ERR_UNKNOWN;

	if (d) {
		gdat.speed_dial = &d->entry;
		pthread_mutex_lock(&speedDialMutex);
		error = d->status = gn_sm_functions(GN_OP_SetSpeedDial, &gdat, &statemachine);
		pthread_cond_signal(&speedDialCond);
		pthread_mutex_unlock(&speedDialMutex);
	}

	return (error);
}


static gint A_SendDTMF(gpointer data)
{
	gchar *buf = (gchar *) data;
	gn_error error = GN_ERR_UNKNOWN;
	gn_data gdat;

	gn_data_clear(&gdat);

	if (buf) {
		gdat.dtmf_string = buf;
		error = gn_sm_functions(GN_OP_SendDTMF, &gdat, &statemachine);
		gdat.dtmf_string = NULL;
		g_free(buf);
	}

	return (error);
}


static gint A_NetMonOnOff(gpointer data)
{
	gint mode = GPOINTER_TO_INT(data);
	gn_netmonitor nm;
	gn_data gdat;

	gn_data_clear(&gdat);

	gdat.netmonitor = &nm;
	if (mode)
		nm.field = 0xf3;
	else
		nm.field = 0xf1;
	return gn_sm_functions(GN_OP_NetMonitor, &gdat, &statemachine);
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
	gn_error error;
	gn_call_info CallInfo;
	int CallId;
	gn_data gdat;

	gn_data_clear(&gdat);

	if (!number) return GN_ERR_UNKNOWN;

	memset(&CallInfo, 0, sizeof(CallInfo));
	snprintf(CallInfo.number, sizeof(CallInfo.number), "%s", number);
	CallInfo.type = GN_CALL_Voice;
	CallInfo.send_number = GN_CALL_Default;
	g_free(number);

	gdat.call_info = &CallInfo;
	error = gn_call_dial(&CallId, &gdat, &statemachine);
	gdat.call_info = NULL;

	return (error);
}


static gint A_GetAlarm(gpointer data)
{
	D_Alarm *a = (D_Alarm *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = GN_ERR_UNKNOWN;

	if (a) {
		a->status = GN_ERR_UNKNOWN;
		pthread_mutex_lock(&alarmMutex);
		gdat.datetime = &a->alarm.timestamp;
		gdat.alarm = &a->alarm;
		error = a->status = gn_sm_functions(GN_OP_GetAlarm, &gdat, &statemachine);
		pthread_cond_signal(&alarmCond);
		pthread_mutex_unlock(&alarmMutex);
	}

	return (error);
}


static gint A_SetAlarm(gpointer data)
{
	D_Alarm *a = (D_Alarm *) data;
	gn_error error;
	gn_data gdat;

	gn_data_clear(&gdat);

	error = a->status = GN_ERR_UNKNOWN;

	if (a) {
		gdat.alarm = &a->alarm;
		error = a->status = gn_sm_functions(GN_OP_SetAlarm, &gdat, &statemachine);
		g_free(a);
	}
	return (error);
}


static gint A_PressKey(gpointer data)
{
	gn_data gdat;

	gn_data_clear(&gdat);
	gdat.key_code = GPOINTER_TO_INT(data);

	return gn_sm_functions(GN_OP_PressPhoneKey, &gdat, &statemachine);
}

static gint A_ReleaseKey(gpointer data)
{
	gn_data gdat;

	gn_data_clear(&gdat);
	gdat.key_code = GPOINTER_TO_INT(data);

	return gn_sm_functions(GN_OP_ReleasePhoneKey, &gdat, &statemachine);
}

static gint A_GetBitmap(gpointer data)
{
	gn_error error;
	D_Bitmap *d = (D_Bitmap *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	pthread_mutex_lock(&getBitmapMutex);
	gdat.bitmap = d->bitmap;
	error = d->status = gn_sm_functions(GN_OP_GetBitmap, &gdat, &statemachine);
	pthread_cond_signal(&getBitmapCond);
	pthread_mutex_unlock(&getBitmapMutex);
	return error;
}

static gint A_SetBitmap(gpointer data)
{
	gn_error error;
	D_Bitmap *d = (D_Bitmap *) data;
	gn_bmp bitmap;
	gn_data gdat;

	gn_data_clear(&gdat);

	pthread_mutex_lock(&setBitmapMutex);
	if (d->bitmap->type == GN_BMP_CallerLogo) {
		bitmap.type = d->bitmap->type;
		bitmap.number = d->bitmap->number;
		gdat.bitmap = &bitmap;
		error = d->status = gn_sm_functions(GN_OP_GetBitmap, &gdat, &statemachine);
		if (error == GN_ERR_NONE) {
			strncpy(d->bitmap->text, bitmap.text, sizeof(bitmap.text));
			d->bitmap->ringtone = bitmap.ringtone;
			gdat.bitmap = d->bitmap;
			error = d->status = gn_sm_functions(GN_OP_SetBitmap, &gdat, &statemachine);
		}
	} else {
		gdat.bitmap = d->bitmap;
		error = d->status = gn_sm_functions(GN_OP_SetBitmap, &gdat, &statemachine);
	}
	pthread_cond_signal(&setBitmapCond);
	pthread_mutex_unlock(&setBitmapMutex);
	return error;
}

static gint A_GetNetworkInfo(gpointer data)
{
	gn_error error;
	D_NetworkInfo *d = (D_NetworkInfo *) data;
	gn_data gdat;

	gn_data_clear(&gdat);

	pthread_mutex_lock(&getNetworkInfoMutex);
	gdat.network_info = d->info;
	error = d->status = gn_sm_functions(GN_OP_GetNetworkInfo, &gdat, &statemachine);
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
	gn_rf_unit rf_units = GN_RF_Percentage;
	gn_battery_unit batt_units = GN_BU_Percentage;
	gn_calnote_alarm Alarm;
	gn_netmonitor netmonitor;
	gn_data gdat;
	gint displaystatus, i, j;
	time_t newtime, oldtime;

	gn_call *call;
	PhoneEvent *event;
	gn_error error;

#ifdef XDEBUG
	g_print("Initializing connection...\n");
#endif

	phoneMonitor.working = _("Connecting...");
	if (fbusinit(true) != GN_ERR_NONE) {
#ifdef XDEBUG
		g_print("Initialization failed...\n");
#endif
		/* FIXME: Add some popup */
		exit(1);
	}

#ifdef XDEBUG
	g_print("Phone connected. Starting monitoring...\n");
#endif

	sleep(1);

	gn_data_clear(&gdat);

	gdat.rf_level = &phoneMonitor.rfLevel;
	gdat.rf_unit = &rf_units;
	gdat.power_source = &phoneMonitor.powerSource;
	gdat.battery_unit = &batt_units;
	gdat.battery_level = &phoneMonitor.batteryLevel;
	gdat.datetime = &Alarm.timestamp;
	gdat.alarm = &Alarm;
	oldtime = time(&oldtime);
	oldtime += 2;

	while (1) {
		phoneMonitor.working = NULL;

/* FIXME - this loop goes mad on my 7110 - so I've put in a usleep */
		usleep(50000);

		if ((call = gn_call_get_active(0)) != NULL) {
#ifdef XDEBUG
			g_print("Call in progress: %s\n", phoneMonitor.call.callNum);
#endif
			gdat.display_status = &displaystatus;
			gn_sm_functions(GN_OP_GetDisplayStatus, &gdat, &statemachine);
			if (displaystatus & (1<<GN_DISP_Call_In_Progress)) {
				pthread_mutex_lock (&callMutex);
				phoneMonitor.call.callInProgress = CS_InProgress;
				pthread_mutex_unlock (&callMutex);
			} else {
				pthread_mutex_lock (&callMutex);
				phoneMonitor.call.callInProgress = CS_Waiting;
				strncpy (phoneMonitor.call.callNum, call->remote_number, INCALL_NUMBER_LENGTH);
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

			gdat.netmonitor = &netmonitor;
			gdat.netmonitor->field = phoneMonitor.netmonitor.number;
			gn_sm_functions(GN_OP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen, gdat.netmonitor->screen,
			       sizeof(gdat.netmonitor->screen));

			gdat.netmonitor->field = 3;
			gn_sm_functions(GN_OP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen3, gdat.netmonitor->screen,
			       sizeof(gdat.netmonitor->screen));

			gdat.netmonitor->field = 4;
			gn_sm_functions(GN_OP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen4, gdat.netmonitor->screen,
			       sizeof(gdat.netmonitor->screen));

			gdat.netmonitor->field = 5;
			gn_sm_functions(GN_OP_NetMonitor, &gdat, &statemachine);
			memcpy(phoneMonitor.netmonitor.screen5, gdat.netmonitor->screen,
			       sizeof(gdat.netmonitor->screen));

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
				if ((error = DoAction[event->event] (event->data)) != GN_ERR_NONE)
					g_print(_("Event %d failed with return code %d!\n"),
						event->event, error);
			g_free(event);
		}

		/* two seconds delay for SMS, RF/power level */
		newtime = time(&newtime);
		if (newtime < oldtime + 2)  continue;
		oldtime = newtime;

		if (gn_sm_functions(GN_OP_GetRFLevel, &gdat, &statemachine) != GN_ERR_NONE)
			phoneMonitor.rfLevel = -1;

		if (gn_sm_functions(GN_OP_GetPowersource, &gdat, &statemachine) == GN_ERR_NONE
		    && phoneMonitor.powerSource == GN_PS_ACDC)
			phoneMonitor.batteryLevel = ((gint) phoneMonitor.batteryLevel + 25) % 125;
		else {
			if (gn_sm_functions(GN_OP_GetBatteryLevel, &gdat, &statemachine) != GN_ERR_NONE)
				phoneMonitor.batteryLevel = -1;
			if (batt_units == GN_BU_Arbitrary)
				phoneMonitor.batteryLevel *= 25;
		}

		if (rf_units == GN_RF_Arbitrary)
			phoneMonitor.rfLevel *= 25;

		if (gn_sm_functions(GN_OP_GetAlarm, &gdat, &statemachine) == GN_ERR_NONE &&
		    Alarm.enabled != 0)
			phoneMonitor.alarm = TRUE;
		else
			phoneMonitor.alarm = FALSE;

		/* only do SMS monitoring when activated */
		if (!isSMSactivated) continue;
		gdat.sms_status = &SMSStatus;
		/* FIXME: This can surely be done easier */
		for (i = 0; i < GN_SMS_FOLDER_MAX_NUMBER; i++) {
			gdat.folder_stats[i] = &FolderStats[i];
			for (j = 0; j < GN_SMS_MESSAGE_MAX_NUMBER; j++)
				gdat.message_list[j][i] = &MessagesList[j][i];
		}

		if ((gn_sms_get_folder_changes(&gdat, &statemachine, (phoneMonitor.supported & PM_FOLDERS))) == GN_ERR_NONE) {
			dprintf("old UR: %i, new UR: %i, old total: %i, new total: %i\n",
				phoneMonitor.sms.unRead, gdat.sms_status->unread,
				phoneMonitor.sms.number, gdat.sms_status->number);
			phoneMonitor.sms.changed += gdat.sms_status->changed;
			if (gdat.sms_status->changed) {
				phoneMonitor.working = _("Refreshing SMSes...");
				RefreshSMS(gdat.sms_status->number);
				phoneMonitor.working = NULL;
			}
			phoneMonitor.sms.unRead = gdat.sms_status->unread;
		}

	}
}
