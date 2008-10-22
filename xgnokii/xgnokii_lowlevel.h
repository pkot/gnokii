/*

  $Id$
  
  X G N O K I I

  A Linux/Unix GUI for the mobile phones.

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
  & 1999-2005 Jan Derfinak.
  Copyright (C) 2002      Markus Plail
  Copyright (C) 2002-2003 Pawel Kot, BORBELY Zoltan
  Copyright (C) 2003      Tomi Ollila

*/

#ifndef XGNOKII_LOWLEVEL_H
#define XGNOKII_LOWLEVEL_H

#include <pthread.h>
#include <glib.h>
#include "gnokii.h"

#define INCALL_NUMBER_LENGTH	20
#define NETMON_SCREEN_LENGTH	60

typedef enum {
	CS_Idle,
	CS_Waiting,
	CS_InProgress
} CallState;

typedef enum {
	Event_GetMemoryStatus,
	Event_GetMemoryLocation,
	Event_GetMemoryLocationAll,
	Event_WriteMemoryLocation,
	Event_WriteMemoryLocationAll,
	Event_DeleteMemoryLocation,
	Event_GetCalendarNote,
	Event_GetCalendarNoteAll,
	Event_WriteCalendarNote,
	Event_DeleteCalendarNote,
	Event_GetCallerGroup,
	Event_SendCallerGroup,
	Event_GetSMSCenter,
	Event_SetSMSCenter,
	Event_SendSMSMessage,
	Event_DeleteSMSMessage,
	Event_GetSpeedDial,
	Event_SendSpeedDial,
	Event_SendDTMF,
	Event_NetMonitorOnOff,
	Event_NetMonitor,
	Event_DialVoice,
	Event_GetAlarm,
	Event_SetAlarm,
	Event_PressKey,
	Event_ReleaseKey,
	Event_GetBitmap,
	Event_SetBitmap,
	Event_GetNetworkInfo,
	Event_PlayTone,
	Event_GetRingtone,
	Event_SetRingtone,
	Event_DeleteRingtone,
	Event_GetRingtoneList,
	Event_Exit
} PhoneAction;

typedef struct {
	PhoneAction event;
	gpointer data;
} PhoneEvent;

typedef struct {
	gn_speed_dial entry;
	gn_error status;
} D_SpeedDial;

typedef struct {
	gn_sms *sms;
	gn_error status;
} D_SMSMessage;

typedef struct {
	gn_sms_message_center *center;
	gn_error status;
} D_SMSCenter;

typedef struct {
	guchar number;
	gchar text[256];
	gint status;
} D_CallerGroup;

typedef struct {
	gn_calnote_alarm alarm;
	gint status;
} D_Alarm;

typedef struct {
	gn_memory_status memoryStatus;
	gint status;
} D_MemoryStatus;

typedef struct {
	gn_phonebook_entry *entry;
	gint status;
} D_MemoryLocation;

typedef struct {
	gint min;
	gint max;
	gint used;
	gn_memory_type type;
	gint status;
	gint (*InsertEntry)(gn_phonebook_entry *);
	gint(*ReadFailed) (gint);
} D_MemoryLocationAll;

typedef struct {
	gn_calnote *entry;
	gint status;
} D_CalendarNote;

typedef struct {
	gint status;
	gint (*InsertEntry)(gn_calnote *);
	gint(*ReadFailed) (gint);
} D_CalendarNoteAll;

typedef struct {
	gn_error status;
	gn_bmp *bitmap;
} D_Bitmap;

typedef struct {
	gn_error status;
	gn_network_info *info;
} D_NetworkInfo;

typedef struct {
	int frequency;
	int volume;
	int usec;
} D_PlayTone;

typedef struct {
	gn_ringtone *ringtone;
	gn_error status;
} D_Ringtone;

typedef struct {
	gfloat rfLevel;
	gfloat batteryLevel;
	gn_power_source powerSource;
	gchar *working;
	bool alarm;
	struct {
		gchar *model;
		gchar *imei;
		gchar *revision;
		gchar *product_name;
	} phone;
	struct {
		gint unRead;
		gint number;
		gint changed;
		GSList *messages;
	} sms;
	struct {
		CallState callInProgress;
		gchar callNum[INCALL_NUMBER_LENGTH];
	} call;
	struct {
		gint number;
		gchar screen[NETMON_SCREEN_LENGTH];
		gchar screen3[NETMON_SCREEN_LENGTH];
		gchar screen4[NETMON_SCREEN_LENGTH];
		gchar screen5[NETMON_SCREEN_LENGTH];
	} netmonitor;
	gint supported;
} PhoneMonitor;

extern pthread_t monitor_th;
extern PhoneMonitor phoneMonitor;
extern pthread_mutex_t memoryMutex;
extern pthread_cond_t memoryCond;
extern pthread_mutex_t calendarMutex;
extern pthread_cond_t calendarCond;
extern pthread_mutex_t smsMutex;
extern pthread_mutex_t sendSMSMutex;
extern pthread_cond_t sendSMSCond;
extern pthread_mutex_t callMutex;
extern pthread_mutex_t netMonMutex;
extern pthread_mutex_t speedDialMutex;
extern pthread_cond_t speedDialCond;
extern pthread_mutex_t callerGroupMutex;
extern pthread_cond_t callerGroupCond;
extern pthread_mutex_t smsCenterMutex;
extern pthread_cond_t smsCenterCond;
extern pthread_mutex_t alarmMutex;
extern pthread_cond_t alarmCond;
extern pthread_mutex_t getBitmapMutex;
extern pthread_cond_t getBitmapCond;
extern pthread_mutex_t setBitmapMutex;
extern pthread_cond_t setBitmapCond;
extern pthread_mutex_t getNetworkInfoMutex;
extern pthread_cond_t getNetworkInfoCond;
extern pthread_mutex_t ringtoneMutex;
extern pthread_cond_t ringtoneCond;
extern void GUI_InitPhoneMonitor(void);
extern void *GUI_Connect(void *a);
extern void GUI_InsertEvent(PhoneEvent * event);
extern gn_error GUI_InitSMSFolders(void);
extern int isSMSactivated;
extern struct gn_statemachine *statemachine;

#endif
