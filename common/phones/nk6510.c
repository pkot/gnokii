/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2002      Markus Plail
  Copyright (C) 2002-2006 Pawel Kot, BORBELY Zoltan

  This file provides functions specific to the Nokia 6510 series.
  See README for more details on supported mobile phones.

  The various routines are called nk6510_(whatever).

*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "compat.h"
#include "misc.h"

#include "gnokii-internal.h"
#include "nokia-decoding.h"
#include "phones/generic.h"
#include "phones/nk6510.h"
#include "links/fbus.h"
#include "links/fbus-phonet.h"
#include "phones/nokia.h"
#include "map.h"

#define DRVINSTANCE(s) (*((nk6510_driver_instance **)(&(s)->driver.driver_instance)))
#define FREE(p) do { free(p); (p) = NULL; } while (0)

#define SEND_MESSAGE_BLOCK(type, length) \
do { \
	if (sm_message_send(length, type, req, state)) return GN_ERR_NOTREADY; \
	return sm_block(type, data, state); \
} while (0)

#define SEND_MESSAGE_WAITFOR(type, length) \
do { \
	if (sm_message_send(length, type, req, state)) return GN_ERR_NOTREADY; \
	return sm_wait_for(type, data, state); \
} while (0)

/* This is for Series40 3rd Edition and later */

/* Mapping from path to memory type */
typedef struct {
	gn_memory_type type;
	char *path;
} path2mt_t;

static struct map *file_cache_map = NULL;

static path2mt_t s40_30_mt_mappings[] = {
	{ GN_MT_IN, "C:\\predefmessages\\1\\" },
	{ GN_MT_OUS, "C:\\predefmessages\\2\\" },
	{ GN_MT_OU, "C:\\predefmessages\\3\\" },
	{ GN_MT_AR, "C:\\predefmessages\\4\\" },
	{ GN_MT_DR, "C:\\predefmessages\\5\\" },
	{ GN_MT_TE, "C:\\predefmessages\\6\\" },
	{ GN_MT_F1, "C:\\predefmessages\\20\\" },
	{ GN_MT_F2, "C:\\predefmessages\\21\\" },
	{ GN_MT_F3, "C:\\predefmessages\\22\\" },
	{ GN_MT_F4, "C:\\predefmessages\\23\\" },
	{ GN_MT_F5, "C:\\predefmessages\\24\\" },
	{ GN_MT_F6, "C:\\predefmessages\\25\\" },
	{ GN_MT_F7, "C:\\predefmessages\\26\\" },
	{ GN_MT_F8, "C:\\predefmessages\\27\\" },
	{ GN_MT_F9, "C:\\predefmessages\\28\\" },
	{ GN_MT_F10, "C:\\predefmessages\\29\\" },
	{ GN_MT_F11, "C:\\predefmessages\\2A\\" },
	{ GN_MT_F12, "C:\\predefmessages\\2B\\" },
	{ GN_MT_F13, "C:\\predefmessages\\2C\\" },
	{ GN_MT_F14, "C:\\predefmessages\\2D\\" },
	{ GN_MT_F15, "C:\\predefmessages\\2E\\" },
	{ GN_MT_F16, "C:\\predefmessages\\2F\\" },
	{ GN_MT_F17, "C:\\predefmessages\\30\\" },
	{ GN_MT_F18, "C:\\predefmessages\\31\\" },
	{ GN_MT_F19, "C:\\predefmessages\\32\\" },
	{ GN_MT_F20, "C:\\predefmessages\\33\\" },
	{ -1, NULL }
};

static int match_sms_folder_mt(gn_memory_type mt)
{
	int i;
	for (i = 0; (s40_30_mt_mappings[i].type != mt) && (s40_30_mt_mappings[i].path != NULL); i++);
	if (s40_30_mt_mappings[i].path == NULL)
		return -1;
	else
		return i;
}

static int match_sms_folder_str(const char *str)
{
	int i;
	char path[128];

	sprintf(path, "C:\\predefmessages\\%s\\", str);
	for (i = 0; (s40_30_mt_mappings[i].path != NULL) && strcmp(path, s40_30_mt_mappings[i].path); i++) {
#if 0
		dprintf("comparing %s and %s\n", path, s40_30_mt_mappings[i].path);
#endif
	}
	if (s40_30_mt_mappings[i].path == NULL)
		return -1;
	else
		return i;
}

static gn_sms_message_status GetMessageStatus_S40_30(const char *filename)
{
	if (!filename || strlen(filename) < 27)
		return GN_SMS_Unknown;

	switch (filename[26]) {
	case '1':
	case '3': /* SMS sending failed */
	case '6':
		return GN_SMS_Unsent;
	case '2':
		return GN_SMS_Sent;
	case '4':
		return GN_SMS_Unread;
	case '5':
		return GN_SMS_Read;
	case 'A':
		/* MMS is being downloaded */
		return GN_SMS_Unread;
	default:
		dprintf("Unknown message status '%c'\n", filename[26]);
		return GN_SMS_Unknown;
	}
}

#define ALLOC_CHUNK	128
/*
 * Helper function for file list operations.
 * When adding a new file to a file list, we need to allocate additional
 * memory for the file structure. Not to waste too much time for that,
 * initially allocate space for 128 files, if it is not enough increase
 * to 256 files, if it is not enough -- 512 files, ...
 * FIXME: add a name of this algorithm.
 */
static void inc_filecount(gn_file_list *fl)
{
	fl->file_count++;
	/* First initialization */
	if (!fl->files) {
		fl->size = ALLOC_CHUNK;
		fl->files = calloc(fl->size, sizeof(char *));
		return;
	}

	/*
	 * If we're out of space, allocate twice as much memory as we have
	 * right now.
	 */
	if (fl->file_count >= fl->size) {
		fl->size *= 2;
		fl->files = realloc(fl->files, fl->size * sizeof(char *));
		return;
	}
	return;
}

/* Functions prototypes */
static gn_error NK6510_Functions(gn_operation op, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_Initialise(struct gn_statemachine *state);
static gn_error NK6510_Terminate(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetModel(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetRevision(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetIMEI(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_Identify(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetBatteryLevel(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetRFLevel(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_SetBitmap(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetBitmap(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_WritePhonebookLocation(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_ReadPhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeletePhonebookLocation(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetSpeedDial(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SetSpeedDial(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetMemoryStatus(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetNetworkInfo(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetSMSCenter(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetAlarm(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SetDateTime(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SetAlarm(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_WriteCalendarNote(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_WriteCalendarNote2(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteCalendarNote(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_DeleteWAPBookmark(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetWAPBookmark(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_WriteWAPBookmark(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetWAPSetting(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_ActivateWAPSetting(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_WriteWAPSetting(gn_data *data, struct gn_statemachine *state);

/*
static gn_error NK6510_PollSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetPicture(gn_data *data, struct gn_statemachine *state);
*/
static gn_error NK6510_SendSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SaveSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetSMS_S40_30(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetSMSnoValidate(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_CreateSMSFolder(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteSMSFolder(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetSMSFolders(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetSMSStatus(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteSMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteSMS_S40_30(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteSMSnoValidate(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetMMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetMMS_S40_30(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetMMSList_S40_30(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteMMS(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteMMS_S40_30(gn_data *data, struct gn_statemachine *state);

/*
static gn_error NK6510_CallDivert(gn_data *data, struct gn_statemachine *state);
*/
static gn_error NK6510_GetRingtoneList(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetRawRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SetRawRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SetRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteRingtone(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_PlayTone(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetProfile(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_SetProfile(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetAnykeyAnswer(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_PressOrReleaseKey(gn_data *data, struct gn_statemachine *state, bool press);
static gn_error NK6510_Subscribe(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetActiveCalls(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_MakeCall(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_CancelCall(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_AnswerCall(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_Reset(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_GetFileListCache(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetFileList(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetFileId(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetFile(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetFileById(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_PutFile(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteFile(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteFileById(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_GetFileDetailsById(gn_data *data, struct gn_statemachine *state);

#ifdef  SECURITY
static gn_error NK6510_GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_EnterSecurityCode(gn_data *data, struct gn_statemachine *state);
#endif

static gn_error NK6510_GetToDo(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_DeleteAllToDoLocations(gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_WriteToDo(gn_data *data, struct gn_statemachine *state);

static gn_error NK6510_IncomingIdentify(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingPhonebook(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingNetwork(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingBattLevel(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingStartup(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingSMS(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingFolder(int messagetype, unsigned char *buffer, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingClock(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingCalendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
/*
static gn_error NK6510_IncomingCallDivert(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
*/
static gn_error NK6510_IncomingRingtone(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingProfile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingKeypress(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingSubscribe(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingCommStatus(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingWAP(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingToDo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingSound(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingReset(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingRadio(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
static gn_error NK6510_IncomingFile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);

#ifdef  SECURITY
static gn_error NK6510_IncomingSecurity(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state);
#endif

static int sms_encode(gn_data *data, struct gn_statemachine *state, unsigned char *req);
static int get_memory_type(gn_memory_type memory_type);
static gn_memory_type get_gn_memory_type(int memory_type);

/* Some globals */

static gn_incoming_function_type nk6510_incoming_functions[] = {
	{ NK6510_MSG_RESET,	NK6510_IncomingReset },
	{ NK6510_MSG_FOLDER,	NK6510_IncomingFolder },
	{ NK6510_MSG_SMS,	NK6510_IncomingSMS },
	{ NK6510_MSG_PHONEBOOK,	NK6510_IncomingPhonebook },
	{ NK6510_MSG_NETSTATUS,	NK6510_IncomingNetwork },
	{ NK6510_MSG_CALENDAR,	NK6510_IncomingCalendar },
	{ NK6510_MSG_BATTERY,	NK6510_IncomingBattLevel },
	{ NK6510_MSG_CLOCK,	NK6510_IncomingClock },
	{ NK6510_MSG_IDENTITY,	NK6510_IncomingIdentify },
	{ NK6510_MSG_STLOGO,	NK6510_IncomingStartup },
	/*
	{ NK6510_MSG_DIVERT,	NK6510_IncomingCallDivert },
	*/
	{ NK6510_MSG_PROFILE,    NK6510_IncomingProfile },
	{ NK6510_MSG_RINGTONE,	NK6510_IncomingRingtone },
	{ NK6510_MSG_KEYPRESS,	NK6510_IncomingKeypress },
#ifdef  SECURITY
	{ NK6510_MSG_SECURITY,	NK6510_IncomingSecurity },
#endif
	{ NK6510_MSG_SUBSCRIBE,	NK6510_IncomingSubscribe },
	{ NK6510_MSG_COMMSTATUS,	NK6510_IncomingCommStatus },
	{ NK6510_MSG_WAP,	NK6510_IncomingWAP },
	{ NK6510_MSG_TODO,	NK6510_IncomingToDo },
	{ NK6510_MSG_SOUND,	NK6510_IncomingSound },
	{ NK6510_MSG_RADIO,	NK6510_IncomingRadio },
	{ NK6510_MSG_FILE,      NK6510_IncomingFile },
	{ 0, NULL }
};

gn_driver driver_nokia_6510 = {
	nk6510_incoming_functions,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"6510|6310|8310|6310i|6360|6610|6100|5100|3510|3510i|3595|6800|6810|6820|6820b|6610i|6230|6650|7210|7250|7250i|7600|6170|6020|6230i|5140|5140i|6021|6500|6220|3120b|3100|3120|6015i|6101|6680|6280|3220|6136|6233|6822|6300|6030|3110c|series60|series40", /* Supported models */
		7,                     /* Max RF Level */
		0,                     /* Min RF Level */
		GN_RF_Percentage,      /* RF level units */
		7,                     /* Max Battery Level */
		0,                     /* Min Battery Level */
		GN_BU_Percentage,      /* Battery level units */
		GN_DT_DateTime,        /* Have date/time support */
		GN_DT_TimeOnly,	       /* Alarm supports time only */
		1,                     /* Alarms available - FIXME */
		65, 96,                /* Startup logo size */
		21, 78,                /* Op logo size */
		14, 72                 /* Caller logo size */
	},
	NK6510_Functions,
	NULL
};

/* FIXME - a little macro would help here... */

static gn_error NK6510_Functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	if (!DRVINSTANCE(state) && op != GN_OP_Init) return GN_ERR_INTERNALERROR;

	switch (op) {
	case GN_OP_Init:
		return NK6510_Initialise(state);
	case GN_OP_Terminate:
		return NK6510_Terminate(data, state);
	case GN_OP_GetModel:
		return NK6510_GetModel(data, state);
	case GN_OP_GetRevision:
		return NK6510_GetRevision(data, state);
	case GN_OP_GetImei:
		return NK6510_GetIMEI(data, state);
	case GN_OP_Identify:
		return NK6510_Identify(data, state);
	case GN_OP_GetBatteryLevel:
		return NK6510_GetBatteryLevel(data, state);
	case GN_OP_GetRFLevel:
		return NK6510_GetRFLevel(data, state);
	case GN_OP_GetMemoryStatus:
		return NK6510_GetMemoryStatus(data, state);
	case GN_OP_GetBitmap:
		return NK6510_GetBitmap(data, state);
	case GN_OP_SetBitmap:
		return NK6510_SetBitmap(data, state);
	case GN_OP_ReadPhonebook:
		return NK6510_ReadPhonebook(data, state);
	case GN_OP_WritePhonebook:
		return NK6510_WritePhonebookLocation(data, state);
	case GN_OP_DeletePhonebook:
		return NK6510_DeletePhonebookLocation(data, state);
	case GN_OP_GetNetworkInfo:
		return NK6510_GetNetworkInfo(data, state);
	case GN_OP_GetSpeedDial:
		return NK6510_GetSpeedDial(data, state);
	case GN_OP_SetSpeedDial:
		return NK6510_SetSpeedDial(data, state);
	case GN_OP_GetSMSCenter:
		return NK6510_GetSMSCenter(data, state);
	case GN_OP_GetDateTime:
		return NK6510_GetDateTime(data, state);
	case GN_OP_GetAlarm:
		return NK6510_GetAlarm(data, state);
	case GN_OP_SetDateTime:
		return NK6510_SetDateTime(data, state);
	case GN_OP_SetAlarm:
		return NK6510_SetAlarm(data, state);
	case GN_OP_GetToDo:
		return NK6510_GetToDo(data, state);
	case GN_OP_WriteToDo:
		return NK6510_WriteToDo(data, state);
	case GN_OP_DeleteAllToDos:
		return NK6510_DeleteAllToDoLocations(data, state);
	case GN_OP_GetCalendarNote:
		return NK6510_GetCalendarNote(data, state);
	case GN_OP_WriteCalendarNote:
		return NK6510_WriteCalendarNote(data, state);
	case GN_OP_DeleteCalendarNote:
		return NK6510_DeleteCalendarNote(data, state);
	case GN_OP_DeleteWAPBookmark:
		return NK6510_DeleteWAPBookmark(data, state);
	case GN_OP_GetWAPBookmark:
		return NK6510_GetWAPBookmark(data, state);
	case GN_OP_WriteWAPBookmark:
		return NK6510_WriteWAPBookmark(data, state);
	case GN_OP_GetWAPSetting:
		return NK6510_GetWAPSetting(data, state);
	case GN_OP_ActivateWAPSetting:
		return NK6510_ActivateWAPSetting(data, state);
	case GN_OP_WriteWAPSetting:
		return NK6510_WriteWAPSetting(data, state);
	case GN_OP_OnSMS:
		DRVINSTANCE(state)->on_sms = data->on_sms;
		DRVINSTANCE(state)->sms_callback_data = data->callback_data;
		return NK6510_Subscribe(data, state);
	case GN_OP_SetCallNotification:
		DRVINSTANCE(state)->call_notification = data->call_notification;
		DRVINSTANCE(state)->call_callback_data = data->callback_data;
		return NK6510_Subscribe(data, state);
	/* case GN_OP_PollSMS:
		break;
	case GONK6510_GetPicture:
		return NK6510_GetPicture(data, state);
		*/
	case GN_OP_DeleteSMS:
		return NK6510_DeleteSMS(data, state);
	case GN_OP_DeleteSMSnoValidate:
		return NK6510_DeleteSMSnoValidate(data, state);
	case GN_OP_GetSMSStatus:
		return NK6510_GetSMSStatus(data, state);
		/*
	case GN_OP_CallDivert:
		return NK6510_CallDivert(data, state);
		*/
	case GN_OP_SaveSMS:
		return NK6510_SaveSMS(data, state);
	case GN_OP_SendSMS:
		return NK6510_SendSMS(data, state);
	case GN_OP_GetSMSFolderStatus:
		return NK6510_GetSMSFolderStatus(data, state);
	case GN_OP_GetSMS:
		return NK6510_GetSMS(data, state);
	case GN_OP_GetSMSnoValidate:
		return NK6510_GetSMSnoValidate(data, state);
	case GN_OP_GetUnreadMessages:
		return GN_ERR_NONE;
	case GN_OP_GetSMSFolders:
		return NK6510_GetSMSFolders(data, state);
	case GN_OP_CreateSMSFolder:
		return NK6510_CreateSMSFolder(data, state);
	case GN_OP_DeleteSMSFolder:
		return NK6510_DeleteSMSFolder(data, state);
	case GN_OP_GetProfile:
		return NK6510_GetProfile(data, state);
	case GN_OP_SetProfile:
		return NK6510_SetProfile(data, state);
	case GN_OP_PressPhoneKey:
		return NK6510_PressOrReleaseKey(data, state, true);
	case GN_OP_ReleasePhoneKey:
		return NK6510_PressOrReleaseKey(data, state, false);
	case GN_OP_GetActiveCalls:
		return NK6510_GetActiveCalls(data, state);
	case GN_OP_MakeCall:
		return NK6510_MakeCall(data, state);
	case GN_OP_CancelCall:
		return NK6510_CancelCall(data, state);
	case GN_OP_AnswerCall:
		return NK6510_AnswerCall(data, state);
#ifdef  SECURITY
	case GN_OP_GetSecurityCodeStatus:
		return NK6510_GetSecurityCodeStatus(data, state);
	case GN_OP_EnterSecurityCode:
		return NK6510_EnterSecurityCode(data, state);
#endif
	case GN_OP_Subscribe:
		return NK6510_Subscribe(data, state);
	case GN_OP_GetRawRingtone:
		return NK6510_GetRawRingtone(data, state);
	case GN_OP_SetRawRingtone:
		return NK6510_SetRawRingtone(data, state);
	case GN_OP_GetRingtone:
		return NK6510_GetRingtone(data, state);
	case GN_OP_SetRingtone:
		return NK6510_SetRingtone(data, state);
	case GN_OP_GetRingtoneList:
		return NK6510_GetRingtoneList(data, state);
	case GN_OP_DeleteRingtone:
		return NK6510_DeleteRingtone(data, state);
	case GN_OP_PlayTone:
		return NK6510_PlayTone(data, state);
	case GN_OP_Reset:
		return NK6510_Reset(data, state);
	case GN_OP_GetFileList:
		return NK6510_GetFileList(data, state);
	case GN_OP_GetFileId:
		return NK6510_GetFileId(data, state);
	case GN_OP_GetFile:
		return NK6510_GetFile(data, state);
	case GN_OP_GetFileById:
		return NK6510_GetFileById(data, state);
	case GN_OP_PutFile:
		return NK6510_PutFile(data, state);
	case GN_OP_DeleteFile:
		return NK6510_DeleteFile(data, state);
	case GN_OP_DeleteFileById:
		return NK6510_DeleteFileById(data, state);
	case GN_OP_GetFileDetailsById:
		return NK6510_GetFileDetailsById(data, state);
	case GN_OP_GetMMS:
		return NK6510_GetMMS(data, state);
	case GN_OP_DeleteMMS:
		return NK6510_DeleteMMS(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_NONE;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error NK6510_Initialise(struct gn_statemachine *state)
{
	gn_data data;
	char model[GN_MODEL_MAX_LENGTH+1];
	gn_error err = GN_ERR_NONE;
	bool connected = false;
	unsigned int attempt = 0;

	memset(model, 0, GN_MODEL_MAX_LENGTH+1);
	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_nokia_6510, sizeof(gn_driver));

	if (!(DRVINSTANCE(state) = calloc(1, sizeof(nk6510_driver_instance))))
		return GN_ERR_INTERNALERROR;

	dprintf("Connecting\n");
	while (!connected) {
		if (attempt > 2) break;
		switch (state->config.connection_type) {
		case GN_CT_DAU9P:
			attempt++;
		case GN_CT_DLR3P:
			if (attempt > 1) {
				attempt = 3;
				break;
			}
		case GN_CT_Serial:
		case GN_CT_TCP:
			err = fbus_initialise(attempt++, state);
			break;
		case GN_CT_Bluetooth:
			/* Quoting Marcel Holtmann from gnokii-ml:
			 * "I discovered some secrets of the Nokia Bluetooth support in the 6310
			 * generation. They use a special SDP entry, which is not in the public
			 * browse group of their SDP database. And here it is:
			 *
			 *	Attribute Identifier : 0x0 - ServiceRecordHandle
			 *	  Integer : 0x10006
			 *	Attribute Identifier : 0x1 - ServiceClassIDList
			 *	  Data Sequence
			 *	    UUID128 : 0x00005002-0000-1000-8000-0002ee00-0001
			 *	    Attribute Identifier : 0x4 - ProtocolDescriptorList
			 *	  Data Sequence
			 *	    Data Sequence
			 *	      UUID16 : 0x0100 - L2CAP
			 *	    Data Sequence
			 *	      UUID16 : 0x0003 - RFCOMM
			 *	      Channel/Port (Integer) : 0xe
			 *
			 * The main secret of this is that they use RFCOMM channel 14 for their
			 * communication protocol."
			 */
			dprintf("Forcing rfcomm_channel = 14 for FBUS connection\n");
			state->config.rfcomm_cn = 14;
		case GN_CT_Infrared:
		case GN_CT_DKU2:
		case GN_CT_DKU2LIBUSB:
		case GN_CT_Irda:
		case GN_CT_SOCKETPHONET:
			err = phonet_initialise(state);
			/* Don't loop forever */
			attempt = 3;
			break;
		default:
			return GN_ERR_NOTSUPPORTED;
		}

		if (err != GN_ERR_NONE) {
			dprintf("Error in link initialisation: %d\n", err);
			continue;
		}

		sm_initialise(state);

		/* Now test the link and get the model */
		gn_data_clear(&data);
		data.model = model;
		err = state->driver.functions(GN_OP_GetModel, &data, state);
		if (err != GN_ERR_NONE) {
			/* We could call GN_OP_Terminate here, but it frees driver instance
			 * which would be needed in the sequent tries
			 */
			pgen_terminate(&data, state);
		} else {
			connected = true;
		}
	}

	if (!connected) {
		FREE(DRVINSTANCE(state));
		return err;
	}

	/* Assign phone capabilities config */
	DRVINSTANCE(state)->pm = gn_phone_model_get(data.model);

	/* Change the defaults for Nokia 8310 */
	if (!strncmp(data.model, "NHM-7", 5)) {
		state->driver.phone.operator_logo_width = 72;
		state->driver.phone.operator_logo_height = 14;
		state->driver.phone.startup_logo_width = 84;
		state->driver.phone.startup_logo_height = 48;
		state->driver.phone.max_battery_level = 4;
	}

	if (!strncmp(data.model, "NMM-3", 5)) {
		state->driver.phone.operator_logo_width = 0;
		state->driver.phone.operator_logo_height = 0;
		state->driver.phone.startup_logo_width = 0;
		state->driver.phone.startup_logo_height = 0;
		state->driver.phone.max_battery_level = 4;
	}

	return GN_ERR_NONE;
}

static gn_error NK6510_Terminate(gn_data *data, struct gn_statemachine *state)
{
	FREE(DRVINSTANCE(state));
	map_free(&file_cache_map);
	return pgen_terminate(data, state);
}

/**********************/
/* IDENTIFY FUNCTIONS */
/**********************/

static gn_error NK6510_IncomingIdentify(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int pos, string_length;

	switch (message[3]) {
	case 0x01:
		if (data->imei) {
			snprintf(data->imei, GNOKII_MIN(message[9], GN_IMEI_MAX_LENGTH), "%s", message + 10);
			dprintf("Received imei %s\n", data->imei);
		}
		break;
	case 0x08:
		if (data->revision) {
			pos = 10;
			string_length = 0;
			while (message[pos + string_length] != 0x0a) string_length++;
			snprintf(data->revision, GNOKII_MIN(string_length + 1, GN_REVISION_MAX_LENGTH), "%s", message + pos);
			dprintf("Received revision %s\n", data->revision);
		}
		if (data->model) {
			pos = 10;
			string_length = 0;
			while (message[pos++] != 0x0a) ;  /* skip revision */
			pos++;
			while (message[pos++] != 0x0a) ;  /* skip date */

			string_length = 0;
			while (message[pos + string_length] != 0x0a) string_length++;  /* find model length */
			dprintf("model length: %i\n", string_length);

			snprintf(data->model, GNOKII_MIN(string_length + 1, GN_MODEL_MAX_LENGTH), "%s", message + pos);
			dprintf("Received model %s\n", data->model);
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x2b (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

/* Just as an example.... */
/* But note that both requests are the same type which isn't very 'proper' */
static gn_error NK6510_Identify(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x41};
	unsigned char req2[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};
	gn_error err;

	dprintf("Identifying...\n");
	pnok_manufacturer_get(data->manufacturer);
	if (sm_message_send(5, 0x1b, req, state)) return GN_ERR_NOTREADY;
	if (sm_message_send(6, 0x1b, req2, state)) return GN_ERR_NOTREADY;
	sm_wait_for(0x1b, data, state);
	sm_block(0x1b, data, state); /* waits for all requests - returns req2 error */
	err = sm_error_get(0x1b, state);
	if (err != GN_ERR_NONE) return err;

	/* Check that we are back at state Initialised */
	if (gn_sm_loop(0, state) != GN_SM_Initialised) return GN_ERR_UNKNOWN;
	return GN_ERR_NONE;
}

static gn_error NK6510_GetIMEI(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x41};

	dprintf("Getting imei...\n");
	SEND_MESSAGE_BLOCK(0x1b, 5);
}

static gn_error NK6510_GetModel(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Getting model...\n");
	SEND_MESSAGE_BLOCK(0x1b, 6);
}

static gn_error NK6510_GetRevision(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x00};

	dprintf("Getting revision...\n");
	SEND_MESSAGE_BLOCK(0x1b, 6);
}

/*******************************/
/* SMS/PICTURE/FOLDER HANDLING */
/*******************************/
static void ResetLayout(unsigned char *message, gn_data *data)
{
	/* reset everything */
	data->raw_sms->more_messages = 0;
	data->raw_sms->reply_via_same_smsc = 0;
	data->raw_sms->reject_duplicates = 0;
	data->raw_sms->report = 0;
	data->raw_sms->reference = 0;
	data->raw_sms->pid = 0;
	data->raw_sms->report_status  = 0;
	memset(data->raw_sms->smsc_time, 0, sizeof(data->raw_sms->smsc_time));
	memset(data->raw_sms->time, 0, sizeof(data->raw_sms->time));
	memset(data->raw_sms->remote_number, 0, sizeof(data->raw_sms->remote_number));
	memset(data->raw_sms->message_center, 0, sizeof(data->raw_sms->message_center));
	data->raw_sms->udh_indicator  = 0;
	data->raw_sms->dcs = 0;
	data->raw_sms->length = 0;
	memset(data->raw_sms->user_data, 0, sizeof(data->raw_sms->user_data));
	data->raw_sms->validity_indicator = 0;
	memset(data->raw_sms->validity, 0, sizeof(data->raw_sms->validity));
}

static void ParseLayout(unsigned char *message, gn_data *data)
{
	int i, j, subblocks, year;
	unsigned char *block = message;

	ResetLayout(message, data);

	dprintf("Trying to parse message....\n");
	dprintf("Type: %i\n", message[1]);
	dprintf("Length: %i\n", message[2]);

	data->raw_sms->udh_indicator = message[3];
	data->raw_sms->dcs = message[5];
	data->raw_sms->reference = message[4];

	switch (message[1]) {
	case 0x00: /* deliver */
		dprintf("Type: Deliver\n");
		data->raw_sms->type = GN_SMS_MT_Deliver;
		block = message + 16;
		memcpy(data->raw_sms->smsc_time, message + 6, 7);
		break;
	case 0x01: /* status report */
		dprintf("Type: Status Report\n");
		data->raw_sms->type = GN_SMS_MT_StatusReport;
		dprintf("Reference id: %d\n", data->raw_sms->reference);
		block = message + 20;
		memcpy(data->raw_sms->smsc_time, message + 6, 7);
		memcpy(data->raw_sms->time, message + 13, 7);
		break;
	case 0x02: /* submit, templates */
		data->raw_sms->dcs = message[6];
		if (data->raw_sms->memory_type == 5) {
			dprintf("Type: TextTemplate\n");
			data->raw_sms->type = GN_SMS_MT_TextTemplate;
			break;
		}
		switch (data->raw_sms->status) {
		case GN_SMS_Sent:
			dprintf("Type: SubmitSent\n");
			data->raw_sms->type = GN_SMS_MT_SubmitSent;
			break;
		case GN_SMS_Unsent:
			dprintf("Type: Submit\n");
			data->raw_sms->type = GN_SMS_MT_Submit;
			break;
		default:
			dprintf("Wrong type\n");
			break;
		}
		block = message + 8;
		break;
	case 0x80:
		dprintf("Type: Picture\n");
		data->raw_sms->type = GN_SMS_MT_Picture;
		break;
	case 0xa0: /* pictures, still ugly */
		switch (message[2]) {
		case 0x01:
			dprintf("Type: PictureTemplate\n");
			data->raw_sms->type = GN_SMS_MT_PictureTemplate;
			data->raw_sms->length = 256;
			memcpy(data->raw_sms->user_data, message + 13, data->raw_sms->length);
			return;
		case 0x02:
			dprintf("Type: Picture\n");
			data->raw_sms->type = GN_SMS_MT_Picture;
			block = message + 20;
			memcpy(data->raw_sms->smsc_time, message + 10, 7);
			data->raw_sms->length = 256;
			memcpy(data->raw_sms->user_data, message + 50, data->raw_sms->length);
			break;
		default:
			dprintf("Unknown picture message!\n");
			break;
		}
		break;
	default:
		dprintf("Type %02x not yet handled!\n", message[1]);
		break;
	}
	subblocks = block[0];
	/*
	dprintf("Subblocks: %i\n", subblocks);
	*/
	block = block + 1;
	for (i = 0; i < subblocks; i++) {
		/*
		dprintf("  Type of subblock %i: %02x\n", i,  block[0]);
		dprintf("  Length of subblock %i: %i\n", i, block[1]);
		*/
		switch (block[0]) {
		case 0x82: /* number */
			switch (block[2]) {
			case 0x01:
				memcpy(data->raw_sms->remote_number,  block + 4, block[3]);
				break;
			case 0x02:
				memcpy(data->raw_sms->message_center,  block + 4, block[3]);
				break;
			default:
				break;
			}
			break;
		case 0x80: /* User Data */
			if ((data->raw_sms->type != GN_SMS_MT_Picture) && (data->raw_sms->type != GN_SMS_MT_PictureTemplate)) {
				/* Ignore the found user_data block for pictures */
				data->raw_sms->length = block[3];
				data->raw_sms->user_data_length = block[2];
				memcpy(data->raw_sms->user_data, block + 4, block[2]);
			}
			break;
		case 0x08: /* Time blocks (mainly at the end of submit sent messages */
			memcpy(data->raw_sms->smsc_time, block + 3, block[2]);
			break;
		case 0x84: /* Time blocks (not BCD encoded) */
			/* Make it BCD format then ;-) */
			year = ((block[2] << 8) + block[3]) % 100;
			data->raw_sms->smsc_time[0] = (year / 10) + ((year % 10) << 4);
			for (j = 1; j < GN_SMS_DATETIME_MAX_LENGTH; j++) {
				data->raw_sms->smsc_time[j] =
					(block[j+3] / 10) + ((block[j+3] % 10) << 4);
			}
			break;
		default:
			dprintf("Unknown block of type: %02x!\n", block[0]);
			break;
		}
		block = block + block[1];
	}
	return;
}

/* handle messages of type 0x14 (SMS Handling, Folders, Logos.. */
static gn_error NK6510_IncomingFolder(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i, j, status;

	switch (message[3]) {
	/* savesms */
	case 0x01:
		switch (message[4]) {
		case 0x00:
			dprintf("SMS successfully saved\n");
			dprintf("Saved in folder %i at location %i\n", message[8], message[6] * 256 + message[7]);
			data->raw_sms->number = message[6] * 256 + message[7];
			break;
		case 0x02:
			dprintf("SMS saving failed: Invalid location\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x05:
			dprintf("SMS saving failed: Incorrect folder\n");
			return GN_ERR_INVALIDMEMORYTYPE;
		default:
			dprintf("ERROR: unknown (%02x)\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* getsms */
	case 0x03:
		dprintf("Trying to get message #%i in folder #%i\n", message[8] * 256 + message[9], message[7]);
		if (!data->raw_sms)
			return GN_ERR_INTERNALERROR;
		/*
		 * When we're reading from 0 location we usually get response in the form:
		 *   01 56 00 03 02 00 00 00 00 00 00 00 00 00
		 * Treat is as the invalid location. Frame cannot be that short anyway.
		 */
		if (length < 15)
			return GN_ERR_INVALIDLOCATION;
		status = data->raw_sms->status;
		memset(data->raw_sms, 0, sizeof(gn_sms_raw));
		data->raw_sms->status = status;
		ParseLayout(message + 13, data);

		/* Number of SMS in folder */
		data->raw_sms->number = message[8] * 256 + message[9];
		dprintf("Location of SMS in current folder: %d\n", data->raw_sms->number);

		/* MessageType/folder_id */
		data->raw_sms->memory_type = message[7];
		dprintf("Memory type/folder id: %d\n", data->raw_sms->memory_type);

		break;

	/* delete sms */
	case 0x05:
		switch (message[4]) {
		case 0x00:
			dprintf("SMS successfully deleted\n");
			break;
		case 0x02:
		case 0x0a:
			dprintf("SMS deleting failed: Invalid location?\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x05:
			dprintf("SMS saving failed: Incorrect folder\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("ERROR: unknown %i\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* delete sms failed */
	case 0x06:
		switch (message[4]) {
		case 0x02:
			dprintf("Invalid location\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("Unknown reason.\n");
			return GN_ERR_UNHANDLEDFRAME;
		}

	/* sms status */
	case 0x09:
		dprintf("SMS Status received\n");
		if (!data->sms_status)
			return GN_ERR_INTERNALERROR;
		data->sms_status->number = (message[12] * 256 + message[13]) +
			(message[24] * 256 + message[25]) + data->sms_folder->number;
		data->sms_status->unread = (message[14] * 256 + message[15]) +
			(message[26] * 256 + message[27]);
		break;

	/* getfolderstatus */
	case 0x0b:
		/*
		 * Reponse:
		 * 01 78 00 0b 00 00 01 00 00 00
		 * apparently means "empty folder"
		 */
		 dprintf("Empty folder?\n");
		if (!data->sms_folder)
			return GN_ERR_INTERNALERROR;
		data->sms_folder->sms_data = 0;
		memset(data->sms_folder->locations, 0, sizeof(data->sms_folder->locations));
		data->sms_folder->number = 0;
		break;

	/* getfolderstatus */
	case 0x0d:
		dprintf("Message: SMS Folder status received\n" );
		if (!data->sms_folder)
			return GN_ERR_INTERNALERROR;
		data->sms_folder->sms_data = 0;
		memset(data->sms_folder->locations, 0, sizeof(data->sms_folder->locations));

		data->sms_folder->number = message[6] * 256 + message[7];
		dprintf("Message: Number of Entries: %i\n" , data->sms_folder->number);
		if (data->sms_folder->number > GN_SMS_MESSAGE_MAX_NUMBER) {
			dprintf("Shrinking to %d entries. File bug for gnokii.\n", GN_SMS_MESSAGE_MAX_NUMBER);
			data->sms_folder->number = GN_SMS_MESSAGE_MAX_NUMBER;
		}
		if (data->sms_folder->number > 0) {
			dprintf("Message: IDs of Entries : ");
			for (i = 0; i < data->sms_folder->number; i++) {
				data->sms_folder->locations[i] = message[(i * 2) + 8] * 256 + message[(i * 2) + 9];
				dprintf("%d, ", data->sms_folder->locations[i]);
			}
			dprintf("\n");
		}
		break;

	/* get message status */
	case 0x0f:
		if (!data->raw_sms)
			return GN_ERR_INTERNALERROR;

		if (length >= 14) {
			dprintf("Message: SMS message (#%i in folder #%i) status received: %i\n",
				message[10] * 256 + message[11],  message[12], message[13]);

			dprintf("Trying to get message #%i from folder #%i\n", data->raw_sms->number, data->raw_sms->memory_type);

			/* Short Message status */
			data->raw_sms->status = message[13];
		} else {
			dprintf("status not supported?\n");
		}

		break;

	/* create folder */
	case 0x11:
		dprintf("Create SMS folder status received..\n");
		if (!data->sms_folder)
			return GN_ERR_INTERNALERROR;
		memset(data->sms_folder, 0, sizeof(gn_sms_folder));

		switch (message[4]) {
		case 0x00:
			dprintf("SMS Folder successfully created!\n");
			data->sms_folder->folder_id = message[8];
			char_unicode_decode(data->sms_folder->name, message + 10, length - 11);
			dprintf("   Folder ID: %i\n", data->sms_folder->folder_id);
			dprintf("   Name: %s\n", data->sms_folder->name);
			break;
		default:
			dprintf("Failed to create SMS Folder! Reason unknown (%02x)!\n", message[4]);
			return GN_ERR_UNKNOWN;
			break;
		}
		break;

	/* getfolders */
	case 0x13:
		if (!data->sms_folder_list)
			return GN_ERR_INTERNALERROR;
		memset(data->sms_folder_list, 0, sizeof(gn_sms_folder_list));

		data->sms_folder_list->number = message[5];
		dprintf("Message: %d SMS Folders received:\n", data->sms_folder_list->number);

		i = 6;
		for (j = 0; j < data->sms_folder_list->number; j++) {
			int len;
			snprintf(data->sms_folder_list->folder[j].name,
				sizeof(data->sms_folder_list->folder[j].name),
				"%s", "               ");

			if (message[i] != 0x01)
				return GN_ERR_UNHANDLEDFRAME;
			data->sms_folder_list->folder_id[j] = get_gn_memory_type(message[i + 2]);
			dprintf("folder_id[%d]=%d\n", j, data->sms_folder_list->folder_id[j]);
			data->sms_folder_list->folder[j].folder_id = data->sms_folder_list->folder_id[j];
			dprintf("folder_id[%d]=%d\n", j, data->sms_folder_list->folder[j].folder_id);
			dprintf("Folder(%i) name: ", message[i + 2]);
			len = message[i + 3] << 1;
			char_unicode_decode(data->sms_folder_list->folder[j].name, message + i + 4, len);
			dprintf("%s\n", data->sms_folder_list->folder[j].name);
			i += message[i + 1];
		}
		break;

	/* delete folder */
	case 0x15:
		switch (message[4]) {
		case 0x00:
			dprintf("SMS Folder successfully deleted!\n");
			break;
		case 0x68:
			dprintf("SMS Folder could not be deleted! Not existant?\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x6b:
			dprintf("SMS Folder could not be deleted! Not empty?\n");
			return GN_ERR_FAILED;
		default:
			dprintf("SMS Folder could not be deleted! Reason unknown (%02x)\n", message[4]);
			return GN_ERR_FAILED;
		}
		break;

	case 0x17:
		break;

	/* get list of SMS pictures */
	case 0x97:
		dprintf("Getting list of SMS pictures...\n");
		break;

	/* Some errors */
	case 0xc9:
		dprintf("Unknown command???\n");
		return GN_ERR_UNHANDLEDFRAME;

	case 0xca:
		dprintf("Phone not ready???\n");
		return GN_ERR_UNHANDLEDFRAME;

	/*
	 * The phone doesn't support folder list messages.
	 * Let's fallback to the 'file' mode.
	 */
	case 0xf0:
		dprintf("Falling back to file mode.\n");
		if (!data->sms_folder_list)
			return GN_ERR_INTERNALERROR;
		return GN_ERR_NOTSUPPORTED;

	default:
		dprintf("Message: Unknown message of type 14 : %02x  length: %d\n", message[3], length);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_GetSMSStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00, 0x00, 0x01};
	gn_sms_folder status_fld, *old_fld;
	gn_error error;

	dprintf("Getting SMS Status...\n");

	/* Nokia 6510 and family does not show not "fixed" messages from the
	 * Templates folder, ie. when you save a message to the Templates folder,
	 * SMSStatus does not change! Workaround: get Templates folder status, which
	 * does show these messages.
	 */

	old_fld = data->sms_folder;

	data->sms_folder = &status_fld;
	data->sms_folder->folder_id = GN_MT_TE;

	error = NK6510_GetSMSFolderStatus(data, state);
	if (error)
		goto out;

	error = sm_message_send(7, NK6510_MSG_FOLDER, req, state);
	if (error)
		goto out;

	error = sm_block(NK6510_MSG_FOLDER, data, state);
 out:
	data->sms_folder = old_fld;
	return error;
}

static gn_error NK6510_GetSMSFolders_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_file_list fl;
	gn_error error;
	int j, i;

	if (!data->sms_folder_list)
		return GN_ERR_INTERNALERROR;

	dprintf("Using GetSMSFolders for Series40 3rd Ed\n");

	memset(&fl, 0, sizeof(fl));
	snprintf(fl.path, sizeof(fl.path), "c:\\predefmessages\\*.*");
	data->file_list = &fl;
	error = NK6510_GetFileListCache(data, state);
	if (error)
		return error;
	for (i = 0, j = 0; i < fl.file_count; i++) {
		int k = match_sms_folder_str(fl.files[i]->name);
		if (k >= 0) {
#if 0
			dprintf("%s %d\n", s40_30_mt_mappings[k].path, s40_30_mt_mappings[k].type);
#endif
			data->sms_folder_list->folder_id[j] = s40_30_mt_mappings[k].type;
			data->sms_folder_list->folder[j].folder_id = s40_30_mt_mappings[k].type;
			sprintf(data->sms_folder_list->folder[j].name, "%s", gn_memory_type_print(s40_30_mt_mappings[k].type));
			j++;
		}
	}
	data->sms_folder_list->number = j;

	dprintf("Misconfiguration in the phone table detected.\nPlease report to gnokii ml (gnokii-users@nongnu.org).\n");
	dprintf("Model %s (%s) is series40 3rd+ Edition.\n", DRVINSTANCE(state)->pm->product_name, DRVINSTANCE(state)->pm->model);
	DRVINSTANCE(state)->pm->flags |= PM_DEFAULT_S40_3RD;
	return error;
}

static gn_error NK6510_GetSMSFolders(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x12, 0x00, 0x00};

	dprintf("Getting SMS Folders...\n");
	if (!data->sms_folder_list)
		return GN_ERR_INTERNALERROR;

	memset(data->sms_folder_list, 0, sizeof(gn_sms_folder_list));

	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE)
		return NK6510_GetSMSFolders_S40_30(data, state);

	if (sm_message_send(6, NK6510_MSG_FOLDER, req, state))
		return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_FOLDER, data, state);
	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE || error == GN_ERR_NOTSUPPORTED) {
		dprintf("NK6510_GetSMSFolders: before switch to S40_30\nerror: %s (%d)\n", gn_error_print(error), error);
		/* Try file approach */
		error = NK6510_GetSMSFolders_S40_30(data, state);
	}
	return error;
}

static gn_error NK6510_DeleteSMSFolder(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x14,
			       0x06, /* Folder ID */
			       0x00};

	dprintf("Deleting SMS Folder...\n");
	req[4] = data->sms_folder->folder_id + 5;
	if (req[4] < 6) return GN_ERR_INVALIDMEMORYTYPE;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 6);
}

static gn_error NK6510_CreateSMSFolder(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[50] = {FBUS_FRAME_HEADER, 0x10,
				 0x01, 0x00, 0x01,
				 0x00, /* length */
				 0x00, 0x00 };
	int len;

	dprintf("Creating SMS Folder...\n");

	len = char_unicode_encode(req + 10, data->sms_folder->name, strlen(data->sms_folder->name));
	req[7] = len + 6;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, req[7] + 6);
}

static gn_error NK6510_GetSMSFolderStatus_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_file_list fl;
	gn_error error;
	int j, k;

	if (!data->sms_folder)
		return GN_ERR_INTERNALERROR;

	dprintf("Using GetSMSFolderStatus for Series40 3rd Ed\n");

	k = match_sms_folder_mt(data->sms_folder->folder_id);
	if (k < 0)
		return GN_ERR_INVALIDMEMORYTYPE;

	memset(&fl, 0, sizeof(fl));
	snprintf(fl.path, sizeof(fl.path), "%s*.*", s40_30_mt_mappings[k].path);

	/* Get file list */
	data->file_list = &fl;
	error = NK6510_GetFileListCache(data, state);
	if (error)
		return error;

	data->sms_folder->number = 0;
	/* Extract just SMS */
	for (j = 0; j < fl.file_count; j++) {
		if (!strncmp("2010", fl.files[j]->name+20, 4) /* standard SMS */
		 || !strncmp("4030", fl.files[j]->name+20, 4) /* SMS in Templates */
		)
			data->sms_folder->number++;
	}
	dprintf("%d out of %d are SMS\n", data->sms_folder->number, fl.file_count);
	if (data->sms_folder->number > GN_SMS_MESSAGE_MAX_NUMBER) {
		dprintf("Shrinking to %d entries. File a bug for gnokii.\n", GN_SMS_MESSAGE_MAX_NUMBER);
		data->sms_folder->number = GN_SMS_MESSAGE_MAX_NUMBER;
	}
	return error;
}

static gn_error NK6510_GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0C,
			       0x02, /* 0x01 SIM, 0x02 ME*/
			       0x00, /* Folder ID */
			       0x0F, 0x55, 0x55, 0x55};
	gn_error error;
	int i;
	gn_sms_folder phone;

	if (!data->sms_folder)
		return GN_ERR_INTERNALERROR;

	/* Let's avoid suprises */
	data->sms_folder->number = 0;

	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE)
		return NK6510_GetSMSFolderStatus_S40_30(data, state);

	req[5] = get_memory_type(data->sms_folder->folder_id);

	dprintf("Getting SMS Folder (%i) status (%i)...\n", req[5], req[4]);

	/*
	 * Inbox and outbox messages can be either in SIM or phone memory.
	 * Other folders are just in the phone memory.
	 */
	if ((req[5] == NK6510_MEMORY_IN) || (req[5] == NK6510_MEMORY_OU)) { /* special case IN/OUTBOX */
		dprintf("Special case IN/OUTBOX in GetSMSFolderStatus!\n");

		/* Get ME folder list */
		dprintf("Get message list from ME\n");
		if (sm_message_send(10, NK6510_MSG_FOLDER, req, state))
			return GN_ERR_NOTREADY;
		error = sm_block(NK6510_MSG_FOLDER, data, state);
		if (error)
			return error;

		memcpy(&phone, data->sms_folder, sizeof(gn_sms_folder));

		/* Get SM folder list */
		dprintf("Get message list from SM\n");
		req[4] = 0x01;
		if (sm_message_send(10, NK6510_MSG_FOLDER, req, state)) return GN_ERR_NOTREADY;
		error = sm_block(NK6510_MSG_FOLDER, data, state);

		/* FIXME: make it dynamic buffer */
		if (phone.number + data->sms_folder->number > GN_SMS_MESSAGE_MAX_NUMBER) {
			dprintf("Shrinking to %d entries. File a bug for gnokii.\n", GN_SMS_MESSAGE_MAX_NUMBER);
			phone.number = GN_SMS_MESSAGE_MAX_NUMBER - data->sms_folder->number;
			/* This shouldn't happen. But let's be safe. */
			if (phone.number < 0)
				phone.number = 0;
		}
		/* Append messages from ME to SM */
		for (i = 0; i < phone.number; i++) {
			data->sms_folder->locations[data->sms_folder->number] = phone.locations[i] + 1024;
			data->sms_folder->number++;
		}
		dprintf("Total number of messages in the folder: %d\n", data->sms_folder->number);
		return GN_ERR_NONE;
	}
	dprintf("Get message list from the folder (ME)\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 10);
}

static gn_error NK6510_GetSMSMessageStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0E,
			       0x02, /* 0x01 SIM, 0x02 phone*/
			       0x00, /* Folder ID */
			       0x00, /* Location Hi */
			       0x00, /* Location Lo */
			       0x55, 0x55};

	if ((data->raw_sms->memory_type == GN_MT_IN) || (data->raw_sms->memory_type == GN_MT_OU)) {
		if (data->raw_sms->number > 1024) {
			/* Do nothing */
		} else {
			req[4] = 0x01;
		}
	}

	dprintf("Getting SMS message (%i in folder %i) status...\n", data->raw_sms->number, data->raw_sms->memory_type);

	req[5] = get_memory_type(data->raw_sms->memory_type);
	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 10);
}

static gn_error NK6510_GetSMSnoValidate(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02,
				   0x02, /* 0x01 for INBOX, 0x02 for others */
				   0x00, /* folder_id */
				   0x00, /* Location Hi */
				   0x02, /* Location Lo */
				   0x01, 0x00};
	gn_error error;

	dprintf("Getting SMS (no validate) ...\n");

	error = NK6510_GetSMSMessageStatus(data, state);

	if ((data->raw_sms->memory_type == GN_MT_IN) || (data->raw_sms->memory_type == GN_MT_OU)) {
		if (data->raw_sms->number > 1024) {
			data->raw_sms->number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = get_memory_type(data->raw_sms->memory_type);
	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 10);
}

static gn_error ValidateSMS(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	/* Handle memory_type = 0 explicitely, because sms_folder->folder_id = 0 by default */
	if (data->raw_sms->memory_type == 0)
		return GN_ERR_INVALIDMEMORYTYPE;

	/* see if the message we want is from the last read folder, i.e. */
	/* we don't have to get folder status again */
	if ((!data->sms_folder) || (!data->sms_folder_list))
		return GN_ERR_INTERNALERROR;
	if (data->raw_sms->memory_type != data->sms_folder->folder_id) {
		if ((error = NK6510_GetSMSFolders(data, state)) != GN_ERR_NONE)
			return error;

		if ((get_memory_type(data->raw_sms->memory_type) >
		     data->sms_folder_list->folder_id[data->sms_folder_list->number - 1]) ||
		    (data->raw_sms->memory_type < 12))
			return GN_ERR_INVALIDMEMORYTYPE;
		data->sms_folder->folder_id = data->raw_sms->memory_type;
		dprintf("Folder id: %d\n", data->sms_folder->folder_id);
		if ((error = NK6510_GetSMSFolderStatus(data, state)) != GN_ERR_NONE)
			return error;
	}

	if (data->sms_folder->number < data->raw_sms->number) {
		if (data->raw_sms->number < GN_SMS_MESSAGE_MAX_NUMBER)
			return GN_ERR_EMPTYLOCATION;
		else
			return GN_ERR_INVALIDLOCATION;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_DeleteSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x04,
				   0x02, /* 0x01 for SM, 0x02 for ME */
				   0x00, /* folder_id */
				   0x00, /* Location Hi */
				   0x00, /* Location Lo */
				   0x0F, 0x55};
	gn_error error;

	dprintf("Deleting SMS...\n");

	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE)
		return NK6510_DeleteSMS_S40_30(data, state);

	error = ValidateSMS(data, state);
	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE || error == GN_ERR_NOTSUPPORTED) {
		dprintf("NK6510_DeleteSMS: before switch to S40_30\nerror: %s (%d)\n", gn_error_print(error), error);
		/* Try file method */
		error = NK6510_DeleteSMS_S40_30(data, state);
		if (error != GN_ERR_NONE)
			dprintf("%s\n", gn_error_print(error));
		else {
			dprintf("Misconfiguration in the phone table detected.\nPlease report to gnokii ml (gnokii-users@nongnu.org).\n");
			dprintf("Model %s (%s) is series40 3rd+ Edition.\n", DRVINSTANCE(state)->pm->product_name, DRVINSTANCE(state)->pm->model);
			DRVINSTANCE(state)->pm->flags |= PM_DEFAULT_S40_3RD;
		}
		return error;
	}

	if (data->raw_sms->number < 1)
		return GN_ERR_EMPTYLOCATION;

	data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];

	if ((data->raw_sms->memory_type == GN_MT_IN) || (data->raw_sms->memory_type == GN_MT_OU)) {
		if (data->raw_sms->number > 1024) {
			data->raw_sms->number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = get_memory_type(data->raw_sms->memory_type);
	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 10);
}

static gn_error NK6510_DeleteSMSnoValidate(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x04,
				   0x02, /* 0x01 for SM, 0x02 for ME */
				   0x00, /* folder_id */
				   0x00, /* Location Hi */
				   0x00, /* Location Lo */
				   0x0F, 0x55};

	dprintf("Deleting SMS (noValidate)...\n");

	if ((data->raw_sms->memory_type == GN_MT_IN) || (data->raw_sms->memory_type == GN_MT_OU)) {
		if (data->raw_sms->number > 1024) {
			data->raw_sms->number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}

	req[5] = get_memory_type(data->raw_sms->memory_type);
	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 10);
}

static gn_error NK6510_GetSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02,
				   0x02, /* 0x01 for SM, 0x02 for ME */
				   0x00, /* folder_id */
				   0x00, /* Location Hi */
				   0x00, /* Location Lo */
				   0x01, 0x00};
	gn_error error;

	dprintf("Getting SMS #%d...\n", data->raw_sms->number);

	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE)
		return NK6510_GetSMS_S40_30(data, state);

	error = ValidateSMS(data, state);
	if (DRVINSTANCE(state)->pm->flags & PM_SMSFILE || error == GN_ERR_NOTSUPPORTED) {
		dprintf("NK6510_GetSMS: before switch to S40_30\nerror: %s (%d)\n", gn_error_print(error), error);
		/* Try file method */
		error = NK6510_GetSMS_S40_30(data, state);
		if (error != GN_ERR_NONE)
			dprintf("%s\n", gn_error_print(error));
		else {
			dprintf("Misconfiguration in the phone table detected.\nPlease report to gnokii ml (gnokii-users@nongnu.org).\n");
			dprintf("Model %s (%s) is series40 3rd+ Edition.\n", DRVINSTANCE(state)->pm->product_name, DRVINSTANCE(state)->pm->model);
			DRVINSTANCE(state)->pm->flags |= PM_DEFAULT_S40_3RD;
		}
		return error;
	}

	data->raw_sms->number = data->sms_folder->locations[data->raw_sms->number - 1];
	dprintf("Getting SMS from location %d\n", data->raw_sms->number);

	error = NK6510_GetSMSMessageStatus(data, state);

	if ((data->raw_sms->memory_type == GN_MT_IN) || (data->raw_sms->memory_type == GN_MT_OU)) {
		if (data->raw_sms->number > 1024) {
			dprintf("Subtracting 1024 from memory location number\n");
			data->raw_sms->number -= 1024;
		} else {
			req[4] = 0x01;
		}
	}
	dprintf("Getting SMS from location %d\n", data->raw_sms->number);

	req[5] = get_memory_type(data->raw_sms->memory_type);
	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;
	SEND_MESSAGE_BLOCK(NK6510_MSG_FOLDER, 10);
}

static gn_error NK6510_GetSMS_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_file_list fl, fl2;
	gn_file fi;
	gn_error error;
	int j, i, cont_len, tota_len, offset;
	unsigned char *bin;

	if (!data->raw_sms)
		return GN_ERR_INTERNALERROR;

	if (data->raw_sms->number < 1) {
		dprintf("Getting SMS %d\n", data->raw_sms->number);
		return GN_ERR_INVALIDLOCATION;
	}

	dprintf("Using GetSMS for Series40 3rd Ed\n");

	/* Find path */
	for (i = 0; (s40_30_mt_mappings[i].type != data->raw_sms->memory_type) && (s40_30_mt_mappings[i].path != NULL); i++);
	if (s40_30_mt_mappings[i].path != NULL) {
		memset(&fl, 0, sizeof(fl));
		snprintf(fl.path, sizeof(fl.path), "%s*.*", s40_30_mt_mappings[i].path);
	} else
		return GN_ERR_INVALIDMEMORYTYPE;

	/* Get file list */
	data->file_list = &fl;
	data->file = NULL;
	error = NK6510_GetFileListCache(data, state);
	if (error)
		return error;

	/* Extract just SMS */
	memset(&fl2, 0, sizeof(fl2));
	for (j = 0; j < fl.file_count; j++) {
		if (!strncmp("2010", fl.files[j]->name+20, 4) /* standard SMS */
		 || !strncmp("4030", fl.files[j]->name+20, 4) /* SMS in Templates */
		) {
			strcpy(fl2.path, fl.path);
			inc_filecount(&fl2);
			fl2.files[fl2.file_count-1] = fl.files[j];
		}
	}

	dprintf("%d out of %d are SMS\n", fl2.file_count, fl.file_count);

	/* Assign filename */
	dprintf("Getting #%d out of %d messages\n", data->raw_sms->number, fl2.file_count);
	if (fl2.file_count >= data->raw_sms->number) {
		memset(&fi, 0, sizeof(fi));
		dprintf("Getting SMS #%d (path: %s, file: %s)\n", data->raw_sms->number, s40_30_mt_mappings[i].path, fl2.files[data->raw_sms->number-1]->name);
		snprintf(fi.name, sizeof(fi.name), "%s%s", s40_30_mt_mappings[i].path, fl2.files[data->raw_sms->number - 1]->name);
	} else
		/* Needs to be INVALID not EMPTY for --getsms IN 1 end to work */
		return GN_ERR_INVALIDLOCATION;

	data->file = &fi;
	/* Get file */
	error = NK6510_GetFile(data, state);
	if (error)
		return error;

	data->raw_sms->status = GetMessageStatus_S40_30(fl2.files[data->raw_sms->number - 1]->name);

#define HEADER_LEN	0xb0	/* 176 */

	bin = fi.file;

	offset = HEADER_LEN;
	/* content length */
	cont_len = (bin[4] << 24) + (bin[5] << 16) + (bin[6] << 8) + bin[7];
	/* total length */
	tota_len = (bin[8] << 24) + (bin[9] << 16) + (bin[10] << 8) + bin[11];
	error = gn_sms_pdu2raw(data->raw_sms, bin + offset, cont_len, GN_SMS_PDU_NOSMSC);

	offset += data->raw_sms->length;
	/* TODO: decode data in the footer? (e.g. SMSC, multiple recipients numbers */

	return error;
}

static gn_error NK6510_DeleteSMS_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_file_list fl, fl2;
	gn_file fi;
	gn_error error;
	int j, i;

	if (!data->raw_sms)
		return GN_ERR_INTERNALERROR;

	if (data->raw_sms->number < 1) {
		dprintf("Deleting SMS %d\n", data->raw_sms->number);
		return GN_ERR_INVALIDLOCATION;
	}

	dprintf("Using DeleteSMS for Series40 3rd Ed\n");

	/* Find path */
	for (i = 0; (s40_30_mt_mappings[i].type != data->raw_sms->memory_type) && (s40_30_mt_mappings[i].path != NULL); i++);
	if (s40_30_mt_mappings[i].path != NULL) {
		memset(&fl, 0, sizeof(fl));
		snprintf(fl.path, sizeof(fl.path), "%s*.*", s40_30_mt_mappings[i].path);
	} else
		return GN_ERR_INVALIDMEMORYTYPE;

	/* Get file list */
	data->file_list = &fl;
	data->file = NULL;
	error = NK6510_GetFileListCache(data, state);
	if (error)
		return error;

	/* Extract just SMS */
	memset(&fl2, 0, sizeof(fl2));
	for (j = 0; j < fl.file_count; j++) {
		if (!strncmp("2010", fl.files[j]->name+20, 4) /* standard SMS */
		 || !strncmp("4030", fl.files[j]->name+20, 4) /* SMS in Templates */
		) {
			strcpy(fl2.path, fl.path);
			inc_filecount(&fl2);
			fl2.files[fl2.file_count-1] = fl.files[j];
		}
	}

	dprintf("%d out of %d are SMS\n", fl2.file_count, fl.file_count);

	/* Assign filename */
	dprintf("Deleting #%d out of %d messages\n", data->raw_sms->number, fl2.file_count);
	if (fl2.file_count >= data->raw_sms->number) {
		memset(&fi, 0, sizeof(fi));
		dprintf("Deleting SMS #%d (path: %s, file: %s)\n", data->raw_sms->number, s40_30_mt_mappings[i].path, fl2.files[data->raw_sms->number-1]->name);
		snprintf(fi.name, sizeof(fi.name), "%s%s", s40_30_mt_mappings[i].path, fl2.files[data->raw_sms->number - 1]->name);
	} else
		/* Needs to be INVALID not EMPTY for --deletesms IN 1 end to work */
		return GN_ERR_INVALIDLOCATION;

	data->file = &fi;
	/* Delete file */
	error = NK6510_DeleteFile(data, state);
	return error;
}

static gn_error NK6510_SaveSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = { FBUS_FRAME_HEADER, 0x00,
				   0x02,			/* 1 = SIM, 2 = ME	*/
				   0x02,			/* Folder		*/
				   0x00, 0x00,			/* Location		*/
				   0x03 };			/* SMS state		*/
#if 0
	unsigned char req2[200] = { FBUS_FRAME_HEADER, 0x16,
				    0x02,			/* 1 = SIM, 2 = ME	*/
				    0x02,			/* Folder		*/
				    0x00, 0x01};		/* Location		*/
	unsigned char desc[10];
#endif
	gn_error error = GN_ERR_NONE;
	int len = 9;
	unsigned char ans[5];
	/* int i; */

	dprintf("Saving sms\n");
	if (data->raw_sms->memory_type == GN_MT_IN && data->raw_sms->type == GN_SMS_MT_Submit)
		return GN_ERR_INVALIDMEMORYTYPE;
	if (data->raw_sms->memory_type != GN_MT_IN &&
	    data->raw_sms->type == GN_SMS_MT_Deliver &&
	    data->raw_sms->status != GN_SMS_Sent)
		return GN_ERR_INVALIDMEMORYTYPE;
	if (data->raw_sms->memory_type == GN_MT_TE ||
	    data->raw_sms->memory_type == GN_MT_SM ||
	    data->raw_sms->memory_type == GN_MT_ME)
		return GN_ERR_INVALIDMEMORYTYPE;

	req[5] = get_memory_type(data->raw_sms->memory_type);

	req[6] = data->raw_sms->number / 256;
	req[7] = data->raw_sms->number % 256;

	if (data->raw_sms->type == GN_SMS_MT_Submit) req[8] = 0x07;
	if (data->raw_sms->status == GN_SMS_Sent) req[8] -= 2;

	memset(req + 15, 0x00, sizeof(req) - 15);

	len += sms_encode(data, state, req + 9);
	/*
	for (i = 0; i < len; i++) dprintf("%02x ", req[i]);
	dprintf("\n");
	*/
	fprintf(stdout, _("6510 series phones seem to be quite sensitive to malformed SMS messages\n"
			  "It may have to be sent to Nokia Service if something fails!\n"
			  "Do you really want to continue? "));
	fprintf(stdout, _("(yes/no) "));
	gn_line_get(stdin, ans, 4);
	if (strcmp(ans, _("yes"))) return GN_ERR_USERCANCELED;

	if (sm_message_send(len, NK6510_MSG_FOLDER, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_FOLDER, data, state);
#if 0
	if (error == GN_ERR_NONE) {
		req2[5] = get_memory_type(data->raw_sms->memory_type);
		if (req2[5] == 0x05) req2[5] = 0x03;
		req2[6] = data->raw_sms->number / 256;
		req2[7] = data->raw_sms->number % 256;

		len = 8;
		memcpy(req2 + len, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
		len += 16;

		req2[len++] = 0;
		req2[len++] = 0;
		if (sm_message_send(len, NK6510_MSG_FOLDER, req2, state)) return GN_ERR_NOTREADY;
		return sm_block(NK6510_MSG_FOLDER, data, state);
	}
#endif
	return error;
}

/****************/
/* SMS HANDLING */
/****************/

static gn_error NK6510_IncomingSMS(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error	e = GN_ERR_NONE;
	unsigned int parts_no, offset, i;

	dprintf("Frame of type 0x02 (SMS handling) received!\n");

	if (!data)
		return GN_ERR_INTERNALERROR;

	switch (message[3]) {
	case 0x2c:
	case NK6510_SUBSMS_INCOMING: { /* 0x04 */
		int freerawsms = 0, freesms = 0;

		dprintf("Incoming SMS notification\n");
		if (!data->raw_sms) {
			freerawsms = 1;
			data->raw_sms = calloc(1, sizeof(gn_sms_raw));
		}
		if (!data->sms) {
			freesms = 1;
			data->sms = calloc(1, sizeof(gn_sms));
		}
		if (!data->raw_sms || !data->sms) {
			e = GN_ERR_INTERNALERROR;
			goto err;
		}
		ParseLayout(message + 9, data);
		e = gn_sms_parse(data);
		if ((e == GN_ERR_NONE) && DRVINSTANCE(state)->on_sms)
			e = DRVINSTANCE(state)->on_sms(data->sms, state, DRVINSTANCE(state)->sms_callback_data);

err:
		if (freerawsms && data->raw_sms)
			free(data->raw_sms);
		if (freesms && data->sms)
			free(data->sms);
		break;
	}
	case NK6510_SUBSMS_SMSC_RCV: /* 0x15 */
		switch (message[4]) {
		case 0x00:
			dprintf("SMSC Received\n");
			break;
		case 0x02:
			dprintf("SMSC reception failed\n");
			e = GN_ERR_EMPTYLOCATION;
			break;
		default:
			dprintf("Unknown response subtype: %02x\n", message[4]);
			e = GN_ERR_UNHANDLEDFRAME;
			break;
		}

		if (e != GN_ERR_NONE)
			break;

		data->message_center->id = message[8];
		data->message_center->format = message[10];
		data->message_center->validity = message[12];  /* due to changes in format */
		data->message_center->name[0] = '\0';

		parts_no = message[13];
		offset = 14;

		for (i = 0; i < parts_no; i++) {
			switch (message[offset]) {
			case 0x82: /* Number */
				switch (message[offset + 2]) {
				case 0x01: /* Default number */
					if (message[offset + 4] % 2)
						message[offset + 4]++;
					message[offset + 4] = message[offset + 4] / 2 + 1;
					snprintf(data->message_center->recipient.number,
						 GN_SMS_NUMBER_MAX_LENGTH + 1, "%s", char_bcd_number_get(message + offset + 4));
					data->message_center->recipient.type = message[offset + 5];
					break;
				case 0x02: /* SMSC number */
					snprintf(data->message_center->smsc.number,
						 GN_SMS_NUMBER_MAX_LENGTH + 1, "%s", char_bcd_number_get(message + offset + 4));
					data->message_center->smsc.type = message[offset + 5];
					break;
				default:
					dprintf("Unknown subtype %02x. Ignoring\n", message[offset + 1]);
					break;
				}
				break;
			case 0x81: /* SMSC name */
				char_unicode_decode(data->message_center->name,
					      message + offset + 4,
					      message[offset + 2]);
				data->message_center->default_name = -1;
				break;
			default:
				dprintf("Unknown subtype %02x. Ignoring\n", message[offset]);
				break;
			}
			offset += message[offset + 1];
		}
		/* Set a default SMSC name if none was received */
		if (!data->message_center->name[0]) {
			snprintf(data->message_center->name, sizeof(data->message_center->name), _("Set %d"), data->message_center->id);
			data->message_center->default_name = data->message_center->id;
		}

		break;

	case NK6510_SUBSMS_SMS_SEND_STATUS: /* 0x03 */
		switch (message[8]) {
		case NK6510_SUBSMS_SMS_SEND_OK: /* 0x00 */
			dprintf("SMS sent (reference: %d)\n", message[10]);
			if (data->raw_sms)
				data->raw_sms->reference = message[10];
			else
				dprintf("Warning: no data->raw_sms allocated and got send_sms() response\n");
			e = GN_ERR_NONE;
			break;

		case NK6510_SUBSMS_SMS_SEND_FAIL: /* 0x01 */
			dprintf("SMS sending failed\n");
			e = GN_ERR_FAILED;
			break;

		default:
			dprintf("Unknown status of the SMS sending -- assuming failure\n");
			e = GN_ERR_FAILED;
			break;
		}
		break;

	case 0x0e:
		dprintf("Ack for request on Incoming SMS\n");
		break;

	case 0x11:
		dprintf("SMS received\n");
		break;

	case 0x22:
		dprintf("SMS has been delivered to the phone and the phone is trying to send it.\n");
		dprintf("No information about sending status yet\n");
		dprintf("SMS sending status will be transmitted asynchronously\n");
		dprintf("Message reference: %d\n", message[5]);
		if (data->raw_sms)
			data->raw_sms->reference = message[5];
		else
			dprintf("Warning: no data->raw_sms allocated and got response for send_sms()\n");
		e = GN_ERR_ASYNC;
		break;
	case NK6510_SUBSMS_SMS_RCVD: /* 0x10 */
	case NK6510_SUBSMS_CELLBRD_OK: /* 0x21 */
	case NK6510_SUBSMS_READ_CELLBRD: /* 0x23 */
	case NK6510_SUBSMS_SMSC_OK: /* 0x31 */
	case NK6510_SUBSMS_SMSC_FAIL: /* 0x32 */
		dprintf("Subtype 0x%02x of type 0x%02x (SMS handling) not implemented\n", message[3], NK6510_MSG_SMS);
		return GN_ERR_NOTIMPLEMENTED;

	default:
		dprintf("Unknown subtype of type 0x%02x (SMS handling): 0x%02x\n", NK6510_MSG_SMS, message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return e;
}

static gn_error NK6510_GetSMSCenter(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, NK6510_SUBSMS_GET_SMSC, 0x01, 0x00};

	req[4] = data->message_center->id;
	SEND_MESSAGE_BLOCK(NK6510_MSG_SMS, 6);
}

/**
 * NK6510_SendSMS - low level SMS sending function for 6310/6510 phones
 * @data: gsm data
 * @state: statemachine
 *
 * Nokia changed the format of the SMS frame completly. Currently there are
 * here some magic numbers (well, many) but hopefully we'll get their meaning
 * soon.
 * 10.07.2002: Almost all frames should be known know :-) (Markus)
 */

static gn_error NK6510_SendSMS(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[256] = {FBUS_FRAME_HEADER, 0x02,
				  0x00, 0x00, 0x00, 0x55, 0x55}; /* What's this? */
	gn_error error;
	unsigned int pos;

	memset(req + 9, 0, 244);
	pos = sms_encode(data, state, req + 9);

	dprintf("Sending SMS...(%d)\n", pos + 9);
	if (sm_message_send(pos + 9, NK6510_MSG_SMS, req, state)) return GN_ERR_NOTREADY;
	do {
		error = sm_block_no_retry_timeout(NK6510_MSG_SMS, state->config.smsc_timeout, data, state);
	} while (!state->config.smsc_timeout && error == GN_ERR_TIMEOUT);
	return error;
}

/*****************/
/* MMS HANDLING  */
/*****************/

static gn_error NK6510_GetMMSList_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_data private_data;
	gn_file_list fl;
	int j, i;

	if (!data->raw_mms || !data->file_list)
		return GN_ERR_INTERNALERROR;

	/* Find path */
	for (i = 0; (s40_30_mt_mappings[i].type != data->raw_mms->memory_type) && (s40_30_mt_mappings[i].path != NULL); i++);
	if (s40_30_mt_mappings[i].path != NULL) {
		memset(&fl, 0, sizeof(fl));
		snprintf(fl.path, sizeof(fl.path), "%s*.*", s40_30_mt_mappings[i].path);
	} else
		return GN_ERR_INVALIDMEMORYTYPE;

	/* Get file list */
	gn_data_clear(&private_data);
	private_data.file_list = &fl;
	private_data.file = NULL;
	error = NK6510_GetFileListCache(&private_data, state);
	if (error)
		return error;

	/* Extract just MMS */
	memset(data->file_list, 0, sizeof(*data->file_list));
	for (j = 0; j < fl.file_count; j++) {
		if (!strncmp("1012", fl.files[j]->name+20, 4) /* standard MMS */
		 || !strncmp("1022", fl.files[j]->name+20, 4) /* MMS Plus */
		 || !strncmp("4012", fl.files[j]->name+20, 4) /* standard MMS in Templates */
		 || !strncmp("4020", fl.files[j]->name+20, 4) /* MMS Plus in Templates */
		) {
			inc_filecount(data->file_list);
			data->file_list->files[data->file_list->file_count-1] = fl.files[j];
		}
	}
	if (data->file_list->file_count)
		strncpy(data->file_list->path, s40_30_mt_mappings[i].path, sizeof(data->file_list->path));

	dprintf("%d out of %d are MMS\n", data->file_list->file_count, fl.file_count);

	/* Do not free fl.files and all fl.files[i] pointers because they are owned and shared by the cache */

	return error;
}

static gn_error NK6510_GetMMS(gn_data *data, struct gn_statemachine *state)
{
	return NK6510_GetMMS_S40_30(data, state);
}

static gn_error NK6510_GetMMS_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_data private_data;
	gn_file_list fl;
	gn_file fi;

	dprintf("Using GetMMS for Series40 3rd Ed\n");

	if (!data->raw_mms)
		return GN_ERR_INTERNALERROR;

	if (data->raw_mms->number < 1)
		return GN_ERR_INVALIDLOCATION;

	gn_data_clear(&private_data);
	memset(&fl, 0, sizeof(fl));
	private_data.file_list = &fl;
	private_data.raw_mms = data->raw_mms;
	error = NK6510_GetMMSList_S40_30(&private_data, state);
	if (error != GN_ERR_NONE)
		return error;

	if (fl.file_count < data->raw_mms->number) {
		/* Needs to be INVALID not EMPTY for --getmms IN 1 end to work */
		error = GN_ERR_INVALIDLOCATION;
	} else {
		memset(&fi, 0, sizeof(fi));
		snprintf(fi.name, sizeof(fi.name), "%s%s", fl.path, fl.files[private_data.raw_mms->number - 1]->name);
		dprintf("Getting MMS #%d (filename: %s)\n", private_data.raw_mms->number, fi.name);

		private_data.file = &fi;
		error = NK6510_GetFile(&private_data, state);

		data->raw_mms->status = GetMessageStatus_S40_30(fl.files[private_data.raw_mms->number - 1]->name);
		data->raw_mms->buffer_length = fi.file_length;
		data->raw_mms->buffer = fi.file;

		if (fi.id)
			free(fi.id);
	}
	/* Do not free all fl.files[i] pointers because they are owned and shared by the cache */
	if (fl.files)
		free(fl.files);

	return error;
}

static gn_error NK6510_DeleteMMS(gn_data *data, struct gn_statemachine *state)
{
	return NK6510_DeleteMMS_S40_30(data, state);
}

static gn_error NK6510_DeleteMMS_S40_30(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_file_list fl;
	gn_file fi;

	dprintf("Using DeleteMMS for Series40 3rd Ed\n");

	if (!data->raw_mms)
		return GN_ERR_INTERNALERROR;

	if (data->raw_mms->number < 1)
		return GN_ERR_INVALIDLOCATION;

	memset(&fl, 0, sizeof(fl));
	data->file_list = &fl;
	error = NK6510_GetMMSList_S40_30(data, state);
	if (error != GN_ERR_NONE)
		return error;

	if (data->file_list->file_count < data->raw_mms->number) {
		/* Needs to be INVALID not EMPTY for --deletemms IN 1 end to work */
		return GN_ERR_INVALIDLOCATION;
	}

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, sizeof(fi.name), "%s%s", fl.path, fl.files[data->raw_mms->number - 1]->name);
	dprintf("Deleting MMS #%d (path: %s, file: %s)\n", data->raw_mms->number, fl.path, fl.files[data->raw_mms->number - 1]->name);
	data->file = &fi;
	error = NK6510_DeleteFile(data, state);

	return error;
}

/*****************/
/* FILE HANDLING */
/*****************/

/*
 * This is a wrapper function over NK6510_GetFileList. Intended usage is
 * just internal. Quite a few functions use filesystem operations and when
 * folder contains many files (>1000) operation is pretty slow. Let's cache
 * it so the subsequent operations don't take that much time.
 * Especially useful for GetSMS in Nokia Series40 3rd Ed and later (*S40_30 functions)
 */
static gn_error NK6510_GetFileListCache(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	gn_file_list *fl;
	int count = NK6510_FILE_CACHE_TIMEOUT;

	dprintf("Trying to retrieve filelist of %s from cache\n", data->file_list->path);

	/*
	 * We call here map_get() twice. First time it is to ensure that we
	 * use proper timeout: the more files are in the folder the longer
	 * it takes to get the file list.
	 */
	fl = map_get(&file_cache_map, data->file_list->path, 0);
	/*
	 * A fl->file_count value of zero would give a timeout of zero which
	 * means "no timeout", so keep the default value in that case.
	 */
	if (fl && fl->file_count)
		count *= fl->file_count;
	fl = map_get(&file_cache_map, data->file_list->path, count);
	if (!fl) {
		dprintf("Cache empty or expired\n");
		error = NK6510_GetFileList(data, state);
		if (error == GN_ERR_NONE) {
			char *path = strdup(data->file_list->path);
			fl = calloc(1, sizeof(gn_file_list));
			memcpy(fl, data->file_list, sizeof(gn_file_list));
			map_add(&file_cache_map, path, fl);
		}
	} else {
		memcpy(data->file_list, fl, sizeof(gn_file_list));
	}
	return error;
}

static gn_error NK6510_GetFileList(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x68, 0x00};
	int i, timeout = NK6510_GETFILELIST_TIMEOUT;

	if (!data->file_list) return GN_ERR_INTERNALERROR;
	data->file_list->file_count = 0;

	i = strlen(data->file_list->path);
	req[5] = char_unicode_encode(req+6, data->file_list->path, i);

	if (data->file_list->file_count)
		timeout = NK6510_FILE_CACHE_TIMEOUT * data->file_list->file_count;
	dprintf("Timeout for getting file list: %d\n", timeout);
	if (sm_message_send(req[5]+9, NK6510_MSG_FILE, req, state)) return GN_ERR_NOTREADY;
	return sm_block_no_retry_timeout(NK6510_MSG_FILE, timeout, data, state);
}

static gn_error NK6510_GetFileDetailsById(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x14, 0x00, 0x00,
					0x00, 0x01,
					0x00, 0x00 }; /* Location */
	int length, i;

	if (!data->file)
		return GN_ERR_INTERNALERROR;

	length = data->file->id[0];
	for (i = 0; i < length; i++) {
		req[8 + i] = data->file->id[i+1];
	}
	length /= 2;
	req[6] = (length & 0xff00) >> 8;
	req[7] = (length & 0x00ff);
	dprintf("Sending: %d %d %d %d\n", req[6], req[7], req[8], req[9]);
	if (sm_message_send(8 + 2 * length, NK6510_MSG_FILE, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK6510_MSG_FILE, data, state);
}

static gn_error NK6510_GetFileId(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x82, 0x00};
	int i;

	if (!data->file) return GN_ERR_INTERNALERROR;

	i = strlen(data->file->name);
	req[5] = char_unicode_encode(req+6, data->file->name, i);

	if (sm_message_send(req[5]+9, NK6510_MSG_FILE, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK6510_MSG_FILE, data, state);
}

static gn_error NK6510_GetFile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x68, 0x00};
	unsigned char req2[512] = {FBUS_FRAME_HEADER, 0x72, 0x00, 0x00, 0x00};
	unsigned char req3[] = {FBUS_FRAME_HEADER, 0x5e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
				0x00, 0x00, 0x00, 0x00, /* Start position */
			        0x00, 0x00, 0x04, 0x00,
				0x00, 0x00, 0x00, 0x00}; /* Size */
	unsigned char req4[] = {FBUS_FRAME_HEADER, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
	gn_error err;
	int i;

	/* Register callback */
	if (data->progress_indication) {
		DRVINSTANCE(state)->progress_indication = data->progress_indication;
		DRVINSTANCE(state)->progress_callback_data = data->callback_data;
	}

	if (!data->file)
		return GN_ERR_INTERNALERROR;
	i = strlen(data->file->name);

	/* Get the file size */
	req[5] = char_unicode_encode(req+6, data->file->name, i);
	if (sm_message_send(req[5] + 9, NK6510_MSG_FILE, req, state))
		return GN_ERR_NOTREADY;
	err = sm_block(NK6510_MSG_FILE, data, state);
	if (err != GN_ERR_NONE)
		return err;
	data->file->file = malloc(data->file->file_length);
	if (!data->file->file)
		return GN_ERR_INTERNALERROR;

	/* Start the transfer */
	req2[7] = char_unicode_encode(req2 + 8, data->file->name, i);
	data->file->togo = 0;
	if (sm_message_send(req2[7] + 12, NK6510_MSG_FILE, req2, state))
		return GN_ERR_NOTREADY;
	err = sm_block(NK6510_MSG_FILE, data, state);
	if (err != GN_ERR_NONE)
		return err;
	if (data->file->togo != data->file->file_length)
		return GN_ERR_INTERNALERROR;

	/* Get the data */
	while (data->file->togo > 0) {
		int progress;

		memcpy(req3+4, data->file->id, NK6510_FILE_ID_LENGTH);
		i = data->file->file_length - data->file->togo;
		req3[11] = (i & 0xff0000) >> 16;
		req3[12] = (i & 0xff00) >> 8;
		req3[13] = i & 0xff;
		if (data->file->togo > 0x100) {
			req3[20] = 0x01;
		} else {
			req3[19] = (data->file->togo & 0xff0000) >> 16;
			req3[20] = (data->file->togo & 0xff00) >> 8;
			req3[21] = data->file->togo & 0xff;
		}
		if (sm_message_send(sizeof(req3), NK6510_MSG_FILE, req3, state))
			return GN_ERR_NOTREADY;
		err = sm_block(NK6510_MSG_FILE, data, state);
		if (err != GN_ERR_NONE)
			return err;
		progress = 100 * (data->file->file_length - data->file->togo) / data->file->file_length;
		if (!DRVINSTANCE(state)->progress_indication) {
			fprintf(stderr, _("Progress: %3d%% completed"), progress);
			/* It's separated for the translators convenience */
			fprintf(stderr, "\r");
		} else
			DRVINSTANCE(state)->progress_indication(progress, DRVINSTANCE(state)->progress_callback_data);
	}
	if (!DRVINSTANCE(state)->progress_indication)
		fprintf(stderr, "\n");

	/* Finish the transfer */
	memcpy(req4+4, data->file->id, NK6510_FILE_ID_LENGTH);
	if (sm_message_send(sizeof(req4), NK6510_MSG_FILE, req4, state))
		return GN_ERR_NOTREADY;
	return sm_block(NK6510_MSG_FILE, data, state);
}

static gn_error NK6510_GetFileById(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0e, 0x00, 0x00,
				0x00, 0x01,
				0x00, 0x00, /* Location */
				0x00, 0x00, 0x00, 0x00, /* Start position */
				0x00, 0x00, 0x00, 0x00}; /* Size */
	gn_error err = GN_ERR_NONE;
	int i, length;

	/* Register callback */
	if (data->progress_indication) {
		DRVINSTANCE(state)->progress_indication = data->progress_indication;
		DRVINSTANCE(state)->progress_callback_data = data->callback_data;
	}

	if (!data->file)
		return GN_ERR_INTERNALERROR;

	length = data->file->id[0];
	for (i = 0; i < length; i++) {
		req[8 + i] = data->file->id[i+1];
	}
	length /= 2;
	req[6] = (length & 0xff00) >> 8;
	req[7] = (length & 0x00ff);
	/* Get the data */
	while (data->file->togo > 0) {
		int progress, offset;

		offset = 9 + 2 * length;
		i = data->file->file_length - data->file->togo;
		req[offset] = (i & 0xff0000) >> 16;
		req[offset + 1] = (i & 0xff00) >> 8;
		req[offset + 2] = i & 0xff;
		if (data->file->togo > 0x100) {
			req[offset + 5] = 0x01;
		} else {
			req[offset + 4] = (data->file->togo & 0xff0000) >> 16;
			req[offset + 5] = (data->file->togo & 0xff00) >> 8;
			req[offset + 6] = data->file->togo & 0xff;
		}
		if (sm_message_send(sizeof(req), NK6510_MSG_FILE, req, state))
			return GN_ERR_NOTREADY;
		err = sm_block(NK6510_MSG_FILE, data, state);
		if (err != GN_ERR_NONE)
			return err;
		progress = 100 * (data->file->file_length - data->file->togo) / data->file->file_length;
		if (!DRVINSTANCE(state)->progress_indication) {
			fprintf(stderr, _("Progress: %3d%% completed"), progress);
			/* It's separated for the translators convenience */
			fprintf(stderr, "\r");
		} else
			DRVINSTANCE(state)->progress_indication(progress, DRVINSTANCE(state)->progress_callback_data);
	}
	if (!DRVINSTANCE(state)->progress_indication)
		fprintf(stderr, "\n");

	return err;
}

static gn_error NK6510_PutFile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req1[512] = {FBUS_FRAME_HEADER, 0x72, 0x11, 0x00, 0x00};
	unsigned char req2[512] = {FBUS_FRAME_HEADER, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
				0x00, 0x00, 0x00, 0x00}; /* Size */
	unsigned char req3[] = {FBUS_FRAME_HEADER, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
	gn_error err;
	int i;

	if (!data->file)
		return GN_ERR_INTERNALERROR;
	i = strlen(data->file->name);

	/* Start the transfer */
	req1[7] = char_unicode_encode(req1+8, data->file->name, i);
	data->file->togo = 0;
	if (sm_message_send(req1[7]+12, NK6510_MSG_FILE, req1, state))
		return GN_ERR_NOTREADY;
	err = sm_block(NK6510_MSG_FILE, data, state);
	if (err != GN_ERR_NONE)
		return err;
	if (data->file->togo != data->file->file_length)
		return GN_ERR_INTERNALERROR;

	/* Put the data */
	while (data->file->togo > 0) {
	        memcpy(req2+4, data->file->id, NK6510_FILE_ID_LENGTH);
		i = data->file->togo;
		if (data->file->togo > 0x100) {
			req2[12] = 0x01;
			data->file->just_sent = 0x100;
		} else {
			req2[11] = (data->file->togo & 0xff0000) >> 16;
			req2[12] = (data->file->togo & 0xff00)   >> 8;
			req2[13] = (data->file->togo & 0xff);
			data->file->just_sent = data->file->togo;
		}
		memcpy(req2+14, data->file->file + data->file->file_length - data->file->togo, data->file->just_sent);
		if (sm_message_send(14+data->file->just_sent, NK6510_MSG_FILE, req2, state))
			return GN_ERR_NOTREADY;
		err = sm_block(NK6510_MSG_FILE, data, state);
		if (err != GN_ERR_NONE)
			return err;
		if (data->file->togo!=i-data->file->just_sent)
			return GN_ERR_INTERNALERROR;
	}

	/* Finish the transfer */
	memcpy(req3+4, data->file->id, NK6510_FILE_ID_LENGTH);
	if (sm_message_send(sizeof(req3), NK6510_MSG_FILE, req3, state)) return GN_ERR_NOTREADY;
	return sm_block(NK6510_MSG_FILE, data, state);
}

static gn_error NK6510_DeleteFile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[512] = {FBUS_FRAME_HEADER, 0x62, 0x00};
	int i;

	if (!data->file)
		return GN_ERR_INTERNALERROR;
	i = strlen(data->file->name);

	req[5] = char_unicode_encode(req+6, data->file->name, i);
	if (sm_message_send(req[5]+9, NK6510_MSG_FILE, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK6510_MSG_FILE, data, state);
}

static gn_error NK6510_DeleteFileById(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[10] = {FBUS_FRAME_HEADER, 0x1E, 0x00, 0x00,
					0x00, 0x01,
					0x00, 0x00}; /* file identifier */
	int i, length;

	if (!data->file)
		return GN_ERR_INTERNALERROR;

	length = data->file->id[0];
	for (i = 0; i < length; i++) {
		req[8 + i] = data->file->id[i+1];
	}
	length /= 2;
	req[6] = (length & 0xff00) >> 8;
	req[7] = (length & 0x00ff);
	if (sm_message_send(8 + length / 2, NK6510_MSG_FILE, req, state)) return GN_ERR_NOTREADY;
	return sm_block(NK6510_MSG_FILE, data, state);
}

static gn_error NK6510_IncomingFile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i, j, frame_length;
	gn_file *file = NULL;
	gn_file_list *fll;
	gn_error error = GN_ERR_NONE;

	switch (message[3]) {
	case 0x0f:
		/* Recv a block of file */
		if (data->file) {
			i = (message[8] << 8) + message[9];
			memcpy(data->file->file + data->file->file_length - data->file->togo, message+10, i);
			data->file->togo -= i;
		}
		break;
	case 0x15: /* Answer for GetFileDetailsById */
		if (!data->file || !data->file_list) {
			error = GN_ERR_INTERNALERROR;
			dprintf("error!\n");
			goto out;
		}
		switch (message[4]) {
		case 0x04:
			error = GN_ERR_EMPTYLOCATION;
			goto out;
		case 0x01: /* OK */
			break;
		default:
			error = GN_ERR_UNKNOWN;
			dprintf("error!\n");
			goto out;
		}
		if (!data->file) {
			error = GN_ERR_INTERNALERROR;
			dprintf("error!\n");
			goto out;
		}
		/* frame length */
		frame_length = 256 * message[8] + message[9];
		file = data->file;
		fll = data->file_list;
		char_unicode_decode(file->name, message + 10, 184);
		dprintf("Filename: %s\n", file->name);
		if (message[196] != 0xff) {
			/* read timestamp */
			file->year = (message[210]<<8) + message[211];
			file->month = message[212];
			file->day = message[213];
			file->hour = message[214];
			file->minute = message[215];
			file->second = message[216];
			dprintf("Timestamp: %04d-%02d-%02d %02d:%02d:%02d\n",
				file->year, file->month, file->day,
				file->hour, file->minute, file->second);
		}
		file->togo = file->file_length = 256 * message[220] + message[221];
		dprintf("Filesize: %d bytes\n", file->file_length);

		switch (message[227]) {
		case 0x00:
			dprintf("directory\n");
			break;
		case 0x01:
			dprintf("java jad file\n");
			break;
		case 0x02:
			dprintf("image\n");
			break;
		case 0x04:
			dprintf("ringtone\n");
			break;
		case 0x10:
			dprintf("java jar file\n");
			break;
		case 0x20:
			dprintf("java rms file\n");
			break;
		default:
			dprintf("unknown file\n");
		}

		data->file_list->file_count = 0;
		j = 0;
		if (length > 0xe8) {
			/* first 4 octets are for the length */
			/* next 4 octets are for the type (?) */
			/* then pairs (len, location) */
			for (i = 250; i + 4 < length ;) {
				int k, len = 2 * (message[i] * 256 + message[i+1]);
				inc_filecount(data->file_list);
				data->file_list->files[j] = calloc(1, sizeof(gn_file));
				data->file_list->files[j]->id = calloc(len + 1, sizeof(char));
				data->file_list->files[j]->id[0] = len;
				for (k = 0; k < len; k++) {
					data->file_list->files[j]->id[k+1] = message[i + 2 + k];
				}
				i += (len + 2);
				j++;
			}
		}
		dprintf("%d subentries\n", data->file_list->file_count);
		break;
	case 0x1f:
		/* file deleted */
		break;
	case 0x59:
		/* Sent a block of file ok */
		if (data->file) {
			data->file->togo -= data->file->just_sent;
		}
		break;
	case 0x5f:
		/* Recv a block of file */
		if (data->file) {
			i = (message[8] << 8) + message[9];
			memcpy(data->file->file + data->file->file_length - data->file->togo, message+10, i);
			data->file->togo -= i;
		}
		break;
	case 0x63:
		/* Deleted ok */
		if (message[4] == 0x06) {
			dprintf("Invalid path\n");
			error = GN_ERR_INVALIDLOCATION;
			goto out;
		}
		break;
	case 0x6d:
	case 0x69:
		if (message[4] == 0x06) {
			dprintf("Invalid path\n");
			error =  GN_ERR_INVALIDLOCATION;
			goto out;
		}
		if (data->file) {
			if (message[4] == 0x0e) {
				dprintf("File not found\n");
				error = GN_ERR_INVALIDLOCATION;
				goto out;
			}
			file = data->file;
		} else if (data->file_list) {
			if (message[4] == 0x0e) {
				dprintf("Empty directory\n");
				goto out;
			}
			inc_filecount(data->file_list);
			data->file_list->files[data->file_list->file_count - 1] = calloc(1, sizeof(gn_file));
			file = data->file_list->files[data->file_list->file_count - 1];
			i = message[31] * 2;
			char_unicode_decode(file->name, message + 32, i);
		}
		if (!file) {
			dprintf("Internal error\n");
			error = GN_ERR_INTERNALERROR;
			goto out;
		}
		dprintf("Filename: %s\n", file->name);
		dprintf("   Status bytes: %02x %02x %02x\n", message[8], message[9], message[29]);
		file->file_length = (message[11] << 16) + (message[12] << 8) + message[13];
		dprintf("   Filesize: %d\n", file->file_length);
		file->year = (message[14]<<8) + message[15];
		file->month = message[16];
		file->day = message[17];
		file->hour = message[18];
		file->minute = message[19];
		file->second = message[20];
		dprintf("   Date: %04u.%02u.%02u\n", file->year, file->month, file->day);
		dprintf("   Time: %02u:%02u:%02u\n", file->hour, file->minute, file->second);
		if (message[4] == 0x0d)
			error = GN_ERR_WAITING;
		break;
	case 0x73:
		if (data->file) {
		        int i;
			if (message[4] == 0x0c) {
				data->file->togo = -1;
			} else if (message[4] == 0x00) {
				data->file->togo = data->file->file_length;
			}

			data->file->id = calloc(NK6510_FILE_ID_LENGTH+1, sizeof(char));
			for (i = 0; i < NK6510_FILE_ID_LENGTH; i++) {
			        data->file->id[i] = message[4 + i];
			}
		}
		break;
	case 0x75:
		/* file transfer complete ok */
		break;
	case 0x83:
		if (data->file) {
			int i;

			data->file->id = calloc(NK6510_FILE_ID_LENGTH+1 , sizeof(char));
			for (i = 0; i < NK6510_FILE_ID_LENGTH; i++) {
				data->file->id[i] = message[4 + i];
			}
		}
		break;
	default:
		error =  GN_ERR_UNKNOWN;
		break;
	}
out:
	return error;
}

/**********************/
/* PHONEBOOK HANDLING */
/**********************/

static gn_error NK6510_IncomingPhonebook(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char blocks;
	int memtype;
	char *req = state->last_msg;

	switch (message[3]) {
	case 0x04:  /* Get status response */
		if (data->memory_status) {
			if (message[5] != 0xff) {
				data->memory_status->used = (message[20] << 8) + message[21];
				data->memory_status->free = ((message[18] << 8) + message[19]) - data->memory_status->used;
				dprintf("Memory status - location = %d, Capacity: %d \n",
					(message[4] << 8) + message[5], (message[18] << 8) + message[19]);
			} else {
				return GN_ERR_INVALIDMEMORYTYPE;
			}
		}
		break;
	case 0x08:  /* Read Memory response */
		/* Check if it is a response to read request */
		if (req && req[3] != 0x7) {
			dprintf("Got read memory response back at unexpected time\n");
			return GN_ERR_UNSOLICITED;
		}
		if (data->phonebook_entry) {
			data->phonebook_entry->empty = true;
			data->phonebook_entry->caller_group = 5; /* no group */
			data->phonebook_entry->name[0] = '\0';
			data->phonebook_entry->number[0] = '\0';
			data->phonebook_entry->subentries_count = 0;
			data->phonebook_entry->date.year = 0;
			data->phonebook_entry->date.month = 0;
			data->phonebook_entry->date.day = 0;
			data->phonebook_entry->date.hour = 0;
			data->phonebook_entry->date.minute = 0;
			data->phonebook_entry->date.second = 0;
		}
		if (data->bitmap) {
			data->bitmap->text[0] = '\0';
		}
		if (message[6] == 0x0f) { /* not found */
			switch (message[10]) {
			case 0x30:
				if (data->phonebook_entry)
					memtype = data->phonebook_entry->memory_type;
				else
					memtype = GN_MT_XX;
				/*
				 * this message has two meanings: "invalid
				 * location" and "memory is empty"
				 */
				switch (memtype) {
				case GN_MT_SM:
				case GN_MT_ME:
					return GN_ERR_EMPTYLOCATION;
				default:
					break;
				}
				return GN_ERR_INVALIDMEMORYTYPE;
			case 0x33:
				return GN_ERR_EMPTYLOCATION;
			case 0x34:
				return GN_ERR_INVALIDLOCATION;
			case 0x31:
				return GN_ERR_INVALIDMEMORYTYPE;
			default:
				return GN_ERR_UNKNOWN;
			}
		}
		dprintf("Received phonebook info\n");
		blocks     = message[21];
		return phonebook_decode(message + 22, length - 21, data, blocks, message[11], 12);

	case 0x0c: /* Write memory location */
		if (message[6] == 0x0f) {
			dprintf("response 0x10 error 0x%x\n", message[10]);
			switch (message[10]) {
			case 0x0f: return GN_ERR_WRONGDATAFORMAT; /* I got this when sending incorrect
								     block (with 0 length) */
			case 0x23: return GN_ERR_WRONGDATAFORMAT; /* block size does not match a definition */
			case 0x36: return GN_ERR_WRONGDATAFORMAT; /* name block is too long */
			case 0x3d: return GN_ERR_FAILED;
			case 0x3e: return GN_ERR_FAILED;
			case 0x43: return GN_ERR_WRONGDATAFORMAT; /* Probably there are incorrect
								     characters to be saved */
			default:   return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;
	case 0x10:
		if (message[6] == 0x0f) {
			dprintf("response 0x10 error 0x%x\n", message[10]);
			switch (message[10]) {
			case 0x33: return GN_ERR_WRONGDATAFORMAT;
			case 0x34: return GN_ERR_INVALIDLOCATION;
			case 0x3b: return GN_ERR_EMPTYLOCATION;
			default:   return GN_ERR_UNHANDLEDFRAME;
			}
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x03 (0x%02x)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x00, 0x55, 0x55, 0x55, 0x00};

	dprintf("Getting memory status...\n");

	req[5] = get_memory_type(data->memory_status->memory_type);
	SEND_MESSAGE_BLOCK(NK6510_MSG_PHONEBOOK, 10);
}

static gn_error NK6510_ReadPhonebookLocation(gn_data *data, struct gn_statemachine *state, int memtype, int location)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
			       0x02, 0x05, /* memory type */
			       0x00, 0x00, 0x00, 0x00,
			       0x00, 0x01, /*location */
			       0x00, 0x00};

	dprintf("Reading phonebook location (%d)\n", location);

	req[9] = memtype;
	req[14] = location >> 8;
	req[15] = location & 0xff;
	SEND_MESSAGE_BLOCK(NK6510_MSG_PHONEBOOK, 18);
}

static gn_error NK6510_GetSpeedDial(gn_data *data, struct gn_statemachine *state)
{
	return NK6510_ReadPhonebookLocation(data, state, NK6510_MEMORY_SPEEDDIALS, data->speed_dial->number);
}

static unsigned char PackBlock(u8 id, u8 size, int *no, u8 *buf, u8 *block, unsigned int maxsize)
{
	if (size + 5 > maxsize) {
		dprintf("Block packing failure -- not enough space\n");
		return 0;
	}
	*(block++) = id;
	*(block++) = 0;
	*(block++) = 0;
	*(block++) = size + 5;
	*(block++) = 0xff; /* no; */
	memcpy(block, buf, size);
	++*no;
	return (size + 5);
}

static gn_error NK6510_SetSpeedDial(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[40] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x01, 0x01, 0x00, 0x00, 0x10,
				 0xFF, 0x0E,
				 0x00, 0x06, /* number */
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				 0x01}; /* blocks */
	char string[40];
	int block = 0;

	dprintf("Setting speeddial...\n");
	req[13] = (data->speed_dial->number >> 8);
	req[13] = data->speed_dial->number & 0xff;

	string[0] = 0xff;
	string[1] = 0x00;
	string[2] = data->speed_dial->location;
	memcpy(string + 3, "\x00\x00\x00\x00", 4);

	if (data->speed_dial->memory_type == GN_MT_SM)
		string[7] = 0x06;
	else
		string[7] = 0x05;
	memcpy(string + 8, "\x0B\x02", 2);

	PackBlock(0x1a, 10, &block, string, req + 22, 40 - 22);
	SEND_MESSAGE_BLOCK(NK6510_MSG_PHONEBOOK, 38);
}

static gn_error SetCallerBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[400] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x01, 0x01, 0x00, 0x00, 0x10,
				 0xFF, 0x10,
				 0x00, 0x06, /* number */
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				 0x01}; /* blocks */
	unsigned int count = 22, block = 0;
	char string[150];
	int i;

/*
  00 01 00 0B 00 01 01 00 00 10
  FE 10
  00 02
  00 00 00 00 00 00 00
  04
  1C 00 00 08 01 01 00 00
  1E 00 00 08 02 02 00 00
  1B 00 00 88 03 48 0E 00 00 7E CE A4 01 7D E0
  .....
  07 00 00 0E 04 08 00 56 00 49 00 50 00 00
*/
	if (!data->bitmap) return GN_ERR_INTERNALERROR;

	req[13] = data->bitmap->number + 1;

	/* Group */
	string[0] = data->bitmap->number + 1;
	string[1] = 0;
	string[2] = 0x55;
	count += PackBlock(0x1e, 3, &block, string, req + count, 400 - count);

	/* Logo */
	string[0] = data->bitmap->width;
	string[1] = data->bitmap->height;
	string[2] = string[3] = 0x00;
	string[4] = 0x7e;
	memcpy(string + 5, data->bitmap->bitmap, data->bitmap->size);
	count += PackBlock(0x1b, data->bitmap->size + 5, &block, string, req + count, 400 - count);

	/* Name */
	i = strlen(data->bitmap->text);
	i = char_unicode_encode((string + 1), data->bitmap->text, i);
	string[0] = i;
	count += PackBlock(0x07, i + 1, &block, string, req + count, 400 - count);

	/* Ringtone */
	if(data->bitmap->ringtone<0) {
		string[0] = 0x00;
		string[7] = 0x00;
		string[8] = 0x00;
		string[9] = 0x00;
		string[10] = 0x01;
		memcpy(string+1, data->bitmap->ringtone_id, 6);
	} else {
		string[0] = 0x00;
		string[1] = 0x00;
		string[2] = 0x00;
		string[3] = 0x00;
		string[4] = 0x00;
		string[5] = (data->bitmap->ringtone&0xff00) >> 8;
		string[6] = (data->bitmap->ringtone&0x00ff);
		string[7] = 0x00;
		string[8] = 0x00;
		string[9] = 0x00;
		string[10] = 0x07;
	}
	count += PackBlock(0x37, 11, &block, string, req + count, 400-count);

	req[21] = block;
	SEND_MESSAGE_BLOCK(NK6510_MSG_PHONEBOOK, count);
}

static gn_error NK6510_ReadPhonebook(gn_data *data, struct gn_statemachine *state)
{
	int memtype = get_memory_type(data->phonebook_entry->memory_type);
	return NK6510_ReadPhonebookLocation(data, state, memtype, data->phonebook_entry->location);
}

static gn_error GetCallerBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07,
			       0x01, 0x01, 0x00, 0x01,
			       0xFE, 0x10,
			       0x00, 0x00, 0x00, 0x00, 0x00,
			       0x03,  /* location */
			       0x00, 0x00};

	/* You can only get logos which have been altered, */
	/* the standard logos can't be read!! */

	req[15] = GNOKII_MIN(data->bitmap->number + 1, GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER);
	req[15] = data->bitmap->number + 1;
	dprintf("Getting caller(%d) logo...\n", req[15]);
	SEND_MESSAGE_BLOCK(NK6510_MSG_PHONEBOOK, 18);
}

static gn_error NK6510_DeletePhonebookLocation(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0f, 0x55, 0x01, 0x04, 0x55, 0x00, 0x10, 0xFF, 0x02,
			       0x00, 0x08,  /* location */
			       0x00, 0x00, 0x00, 0x00,
			       0x05,  /* memory type */
			       0x55, 0x55, 0x55};
	gn_phonebook_entry *entry;

	if (data->phonebook_entry)
		entry = data->phonebook_entry;
	else
		return GN_ERR_TRYAGAIN;

	req[12] = (entry->location >> 8);
	req[13] = entry->location & 0xff;
	req[18] = get_memory_type(entry->memory_type);

	SEND_MESSAGE_BLOCK(NK6510_MSG_PHONEBOOK, 22);
}

static gn_error NK6510_WritePhonebookLocation(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[GN_PHONEBOOK_ENTRY_MAX_LENGTH] = {
		FBUS_FRAME_HEADER, 0x0b, 0x00, 0x01, 0x01, 0x00, 0x00, 0x10,
		0x02, 0x00,  /* memory type */
		0x00, 0x00,  /* location */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* blocks */
	char string[GN_PHONEBOOK_ENTRY_MAX_LENGTH];
	int block, i, j, defaultn;
	unsigned int count;
	gn_phonebook_entry *entry;
	unsigned int postal_count;
	int postal_block = 0;
	gn_error error = GN_ERR_NONE;

	if (data->phonebook_entry)
		entry = data->phonebook_entry;
	else
		return GN_ERR_TRYAGAIN;

	req[11] = get_memory_type(entry->memory_type);
	req[12] = (entry->location >> 8);
	req[13] = entry->location & 0xff;

retry:
	count = 22;
	block = 1;
	if (!entry->empty) {
		/*
		 * Nokia Series40 3rd Ed and later fails on writing caller group block 0x1e.
		 * In that case, let's not write it and let's use First Name, Last Name
		 * not to loose the information.
		 */
		if (DRVINSTANCE(state)->pm->flags & PM_EXTPBK2) {
			int found = 0;

			for (i = 0; i < entry->subentries_count; i++) {
				switch (entry->subentries[i].entry_type) {
				case GN_PHONEBOOK_ENTRY_LastName:
					/* Last name */
					j = char_unicode_encode((string + 1), entry->subentries[i].data.number, strlen(entry->subentries[i].data.number));
					string[0] = j;
					count += PackBlock(0x47, j + 1, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
					found = 1;
					break;
				case GN_PHONEBOOK_ENTRY_FirstName:
					/* First name */
					j = char_unicode_encode((string + 1), entry->subentries[i].data.number, strlen(entry->subentries[i].data.number));
					string[0] = j;
					count += PackBlock(0x46, j + 1, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
					found = 1;
					break;
				default:
					break;
				}
			}
			if (!found) {
				/* Use Name as Last Name */
				j = char_unicode_encode((string + 1), entry->name, strlen(entry->name));
				string[0] = j;
				count += PackBlock(0x47, j + 1, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
			}
		} else {
			/* Name */
			j = char_unicode_encode((string + 1), entry->name, strlen(entry->name));
			string[0] = j;
			count += PackBlock(0x07, j + 1, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);

			/* Group */
			string[0] = entry->caller_group + 1;
			string[1] = 0;
			string[2] = 0x55;
			count += PackBlock(0x1e, 3, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
		}

		/* We don't require the application to fill in any subentry.
		 * if it is not filled in, let's take just one number we have.
		 */
		if (!entry->subentries_count) {
			string[0] = GN_PHONEBOOK_NUMBER_General;
			string[1] = string[2] = string[3] = 0;
			j = strlen(entry->number);
			j = char_unicode_encode((string + 5), entry->number, j);
			string[j + 1] = 0;
			string[4] = j;
			count += PackBlock(0x0b, j + 5, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
		} else {
			/* Default Number */
			defaultn = 999;
			for (i = 0; i < entry->subentries_count; i++)
				if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number)
					if (!strcmp(entry->number, entry->subentries[i].data.number))
						defaultn = i;
			if (defaultn < i) {
				string[0] = entry->subentries[defaultn].number_type;
				string[1] = string[2] = string[3] = 0;
				j = strlen(entry->subentries[defaultn].data.number);
				j = char_unicode_encode((string + 5), entry->subentries[defaultn].data.number, j);
				string[j + 1] = 0;
				string[4] = j;
				count += PackBlock(0x0b, j + 5, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
			}
			/* Rest of the numbers */
			for (i = 0; i < entry->subentries_count; i++) {
				int duplicate = 0;
				/* Ignore duplicates */
				for (j = 0; j < i; j++) {
					if (entry->subentries[i].entry_type == entry->subentries[j].entry_type &&
					    entry->subentries[i].number_type == entry->subentries[j].number_type &&
					    !strcmp(entry->subentries[i].data.number, entry->subentries[j].data.number)) {
					    	dprintf("duplicate subentry!\n");
						duplicate = 1;
						break;
					}
				}
				if (duplicate)
					break;
				switch (entry->subentries[i].entry_type) {
				case GN_PHONEBOOK_ENTRY_ExtendedAddress:
				case GN_PHONEBOOK_ENTRY_Postal:
				case GN_PHONEBOOK_ENTRY_Street:
				case GN_PHONEBOOK_ENTRY_City:
				case GN_PHONEBOOK_ENTRY_StateProvince:
				case GN_PHONEBOOK_ENTRY_ZipCode:
				case GN_PHONEBOOK_ENTRY_Country:
				case GN_PHONEBOOK_ENTRY_LastName:
				case GN_PHONEBOOK_ENTRY_FirstName:
					break;
				case GN_PHONEBOOK_ENTRY_Number:
					if (j < i)
						break;
					if (i != defaultn) {
						string[0] = entry->subentries[i].number_type;
						string[1] = string[2] = string[3] = 0;
						j = strlen(entry->subentries[i].data.number);
						j = char_unicode_encode((string + 5), entry->subentries[i].data.number, j);
						string[j + 1] = 0;
						string[4] = j;
						count += PackBlock(0x0b, j + 5, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
					}
					break;
				case GN_PHONEBOOK_ENTRY_ExtGroup:
					string[0] = 0;
					string[1] = 0;
					string[2] = entry->subentries[i].data.id;
					count += PackBlock(0x43, 3, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
					break;
				case GN_PHONEBOOK_ENTRY_Date:
				case GN_PHONEBOOK_ENTRY_Birthday:
					if (GN_PHONEBOOK_ENTRY_MAX_LENGTH - count < 12) /* 12 is size of date/birthday record */
						break;
					req[count++] = entry->subentries[i].entry_type;
					req[count++] = 0x00;
					req[count++] = 0x00;
					req[count++] = 0x0c;
					req[count++] = 0xff;
					req[count++] = 0x00;
					req[count++] = entry->subentries[i].data.date.year / 256;
					req[count++] = entry->subentries[i].data.date.year % 256;
					req[count++] = entry->subentries[i].data.date.month;
					req[count++] = entry->subentries[i].data.date.day;
					req[count++] = entry->subentries[i].data.date.hour;
					req[count++] = entry->subentries[i].data.date.minute;
					break;
				default:
					j = strlen(entry->subentries[i].data.number);
					j = char_unicode_encode((string + 1), entry->subentries[i].data.number, j);
					string[j + 1] = 0;
					string[0] = j;
					count += PackBlock(entry->subentries[i].entry_type, j + 1, &block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
					break;
				}
			}
			/* Addresses */
			if (GN_PHONEBOOK_ENTRY_MAX_LENGTH - count > 13) { /* 13 is size of base address main part */
				postal_count = count;
				req[count++] = GN_PHONEBOOK_ENTRY_PostalAddress;
				req[count++] = 0x00;
				req[count++] = 0x00;
				req[count++] = 0x08;
				req[count++] = 0xff;
				req[count++] = 0x00;
				req[count++] = 0x00;
				req[count++] = 0x00;
				for (i = 0; i < entry->subentries_count; i++) {
					int duplicate = 0;
					/* Ignore duplicates */
					for (j = 0; j < i; j++) {
						if (entry->subentries[i].entry_type == entry->subentries[j].entry_type &&
						    entry->subentries[i].number_type == entry->subentries[j].number_type &&
						    !strcmp(entry->subentries[i].data.number, entry->subentries[j].data.number)) {
							duplicate = 1;
							break;
						}
					}
					if (duplicate)
						break;
					switch (entry->subentries[i].entry_type) {
					case GN_PHONEBOOK_ENTRY_ExtendedAddress:
					case GN_PHONEBOOK_ENTRY_Postal:
					case GN_PHONEBOOK_ENTRY_Street:
					case GN_PHONEBOOK_ENTRY_City:
					case GN_PHONEBOOK_ENTRY_StateProvince:
					case GN_PHONEBOOK_ENTRY_ZipCode:
					case GN_PHONEBOOK_ENTRY_Country:
						j = strlen(entry->subentries[i].data.number);
						j = char_unicode_encode((string + 1), entry->subentries[i].data.number, j);
						string[j + 1] = 0;
						string[0] = j;
						count += PackBlock(entry->subentries[i].entry_type, j + 1, &postal_block, string, req + count, GN_PHONEBOOK_ENTRY_MAX_LENGTH - count);
						break;
					default:
						break;
					}
				}
				if (postal_block > 0) {
					req[postal_count + 7] = postal_block;
					block++;
				} else {
					count = postal_count;
				}
			}
		}
		req[21] = block - 1;
		dprintf("Writing phonebook entry %s...\n",entry->name);
	} else {
		return NK6510_DeletePhonebookLocation(data, state);
	}
	if (sm_message_send(count, NK6510_MSG_PHONEBOOK, req, state))
		return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_PHONEBOOK, data, state);
	if (error == GN_ERR_FAILED && !(DRVINSTANCE(state)->pm->flags & PM_EXTPBK2)) {
		dprintf("Writing failed. Falling back to a new method.\n");
		dprintf("Misconfiguration in the phone table detected.\nPlease report to gnokii ml (gnokii-users@nongnu.org).\n");
		dprintf("Model %s (%s) is series40 3rd+ Edition.\n", DRVINSTANCE(state)->pm->product_name, DRVINSTANCE(state)->pm->model);
		DRVINSTANCE(state)->pm->flags |= PM_DEFAULT_S40_3RD;
		goto retry;
	}
	return error;
}

/******************/
/* CLOCK HANDLING */
/******************/

static gn_error NK6510_IncomingClock(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	/*
Unknown subtype of type 0x19 (clock handling): 0x1c
UNHANDLED FRAME RECEIVED
request: 0x19 / 0x0004
00 01 00 1b                                     |
reply: 0x19 / 0x0012
01 23 00 1c 06 01 ff ff ff ff 00 00 49 4c 01 3e |
01 56
	*/

	dprintf("Incoming clock!\n");
	if (!data) return GN_ERR_INTERNALERROR;
	switch (message[3]) {
	case NK6510_SUBCLO_SET_DATE_RCVD:
		dprintf("Date/Time successfully set!\n");
		break;
	case NK6510_SUBCLO_SET_ALARM_RCVD:
		dprintf("Alarm successfully set!\n");
		break;
	case NK6510_SUBCLO_DATE_RCVD:
		if (!data->datetime) return GN_ERR_INTERNALERROR;
		dprintf("Date/Time received!\n");
		data->datetime->year = (((unsigned int)message[10]) << 8) + message[11];
		data->datetime->month = message[12];
		data->datetime->day = message[13];
		data->datetime->hour = message[14];
		data->datetime->minute = message[15];
		data->datetime->second = message[16];

		break;
	case NK6510_SUBCLO_ALARM_TIME_RCVD:
		if (!data->alarm) return GN_ERR_INTERNALERROR;
		data->alarm->timestamp.hour = message[14];
		data->alarm->timestamp.minute = message[15];
		break;
	case NK6510_SUBCLO_ALARM_STATE_RCVD:
		if (!data->alarm) return GN_ERR_INTERNALERROR;
		switch(message[37]) {
		case NK6510_ALARM_ENABLED:
			data->alarm->enabled = true;
			break;
		case NK6510_ALARM_DISABLED:
			data->alarm->enabled = false;
			break;
		default:
			data->alarm->enabled = false;
			dprintf("Unknown value of alarm enable byte: 0x%02x\n", message[8]);
			error = GN_ERR_UNKNOWN;
			break;
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (clock handling): 0x%02x\n", NK6510_MSG_CLOCK, message[3]);
		error = GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return error;
}

static gn_error NK6510_GetDateTime(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, NK6510_SUBCLO_GET_DATE, 0x00, 0x00};

	SEND_MESSAGE_BLOCK(NK6510_MSG_CLOCK, 6);
}

static gn_error NK6510_SetDateTime(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x01, 0x00, 0x01,
			       0x01, 0x0c, 0x01, 0x03,
			       0x07, 0xd2,	/* Year */
			       0x08, 0x01,     /* Month & Day */
			       0x15, 0x1f,	/* Hours & Minutes */
			       0x00,		/* Seconds */
			       0x00};

	req[10] = data->datetime->year / 256;
	req[11] = data->datetime->year % 256;
	req[12] = data->datetime->month;
	req[13] = data->datetime->day;
	req[14] = data->datetime->hour;
	req[15] = data->datetime->minute;
	req[16] = data->datetime->second;

	SEND_MESSAGE_BLOCK(NK6510_MSG_CLOCK, 18);
}

static gn_error NK6510_GetAlarmTime(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x19, 0x00, 0x02};

	SEND_MESSAGE_BLOCK(NK6510_MSG_CLOCK, 6);
}

static gn_error NK6510_GetAlarmState(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x1f, 0x01, 0x00};

	SEND_MESSAGE_BLOCK(NK6510_MSG_CLOCK, 6);
}

static gn_error NK6510_GetAlarm(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;

	if ((error = NK6510_GetAlarmState(data, state)) != GN_ERR_NONE) return error;
	return NK6510_GetAlarmTime(data, state);
}

static gn_error NK6510_SetAlarm(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x11, 0x00, 0x01,
			       0x01, 0x0c, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00,		/* Hours, Minutes */
			       0x00, 0x00, 0x00 };

	dprintf("Setting alarm\n");
	if (data->alarm->enabled != 1) return GN_ERR_NOTSUPPORTED;

	req[14] = data->alarm->timestamp.hour;
	req[15] = data->alarm->timestamp.minute;

	SEND_MESSAGE_BLOCK(NK6510_MSG_CLOCK, 19);
}

/*********************/
/* CALENDAR HANDLING */
/********************/

static int calnote_type_map(int type)
{
	switch (type) {
	case NK6510_NOTE_MEETING:
		return GN_CALNOTE_MEETING;
	case NK6510_NOTE_CALL:
		return GN_CALNOTE_CALL;
	case NK6510_NOTE_BIRTHDAY:
		return GN_CALNOTE_BIRTHDAY;
	case NK6510_NOTE_MEMO:
		return GN_CALNOTE_MEMO;
	case NK6510_NOTE_REMINDER:
		return GN_CALNOTE_REMINDER;
	default:
		return type;
	}
}

/* Calendar note new frame description:
 * [00 - 03] xx xx xx xx	FBUS specific (type, etc)
 * [04 - 07] 00 00 00 00	unknown (4 octets)
 * [08 - 11] 00 00 00 00	unknown (4 octets)
 * [12 - 13] xx xx		location (2 octets)
 * [14 - 17] xx xx xx xx	alarm (4 octets):	ff ff ff ff - no alarm,
 *							00 00 00 00 - alarm at event time
 *							00 00 00 05 - 5 minuted before
 *							...
 * [18 - 20] 80 00 00		unknown (3 octets)
 * [21 - 21] xx			icon id (1 octet)
 * [22 - 25] xx xx xx xx	tone (4 octets):	ff ff ff ff - tone
 *							00 00 00 00 - no tone
 * [26 - 26] xx			unknown (1 octet)
 * [27 - 27] xx			note type (1 octet):	01
 *							02
 *							04
 *							08
 * [28 - 33] xx xx xx xx xx xx	start date (6 octets):	YY YY MM DD HH MM
 * [34 - 39] xx xx xx xx xx xx	end date (6 octets):	YY YY MM DD HH MM
 * [40 - 41] xx xx		recurrence (2 octets):	00 - no recurrence
 *							18 - every day
 *							a8 - every week
 *							...
 * [42 - 43] xx xx		in case of Birthday: year of birth; otherwise unknown (ff ff)
 * [44 - 45] 20 00		unknown (2 octets)
 * [46 - 47] xx xx		occurrences (2 octets)
 * [48 - 49] 00 00		unknown (2 octets)
 * [50 - 51] xx xx		first text field length (2 octets) [L1]
 * [52 - 52] xx			second text field length (1 octet) [L2]
 * [53 - 53] 00			unknown (1 octet)
 * [54 - AA] xx xx xx ...	first text field (unicode)
 * [BB - CC] xx xx xx ...	second text field (unicode)
 *
 * AA = 54 + 2 * L1 - 1
 * BB = AA + 1
 * CC = BB + 2 * L2 - 1
 */
static gn_error calnote2_decode(unsigned char *message, int length, gn_data *data)
{
	gn_error e = GN_ERR_NONE;
	int alarm_mark, alarm;
	int len1, len2;
	int tone1, tone2;

	if (!data->calnote) {
		e = GN_ERR_INTERNALERROR;
		goto out;
	}
	/* Type */
	data->calnote->type = calnote_type_map(message[27]);
	/* Location */
	data->calnote->location = message[12] * 256 + message[13];
	/* Start time */
	data->calnote->time.year = message[28] * 256 + message[29];
	data->calnote->time.month = message[30];
	data->calnote->time.day = message[31];
	data->calnote->time.hour = message[32];
	data->calnote->time.minute = message[33];
	data->calnote->time.second = 0;
	/* End time */
	data->calnote->end_time.year = message[34] * 256 + message[35];
	data->calnote->end_time.month = message[36];
	data->calnote->end_time.day = message[37];
	data->calnote->end_time.hour = message[38];
	data->calnote->end_time.minute = message[39];
	data->calnote->end_time.second = 0;
	/* Recurrence */
	data->calnote->recurrence = 256 * message[40] + message[41];
	/* Occurrence */
	data->calnote->occurrences = 256 * message[46] + message[47];
	/* Alarm */
	alarm_mark = 256 * message[14] + message[15];
	alarm = 256 * message[16] + message[17];
	if (alarm_mark == 0xffff && alarm == 0xffff) {
		data->calnote->alarm.enabled = false;
	} else {
		data->calnote->alarm.enabled = true;
		e = calnote_get_alarm(60 * (alarm + 65536 * alarm_mark),
					&(data->calnote->time),
					&(data->calnote->alarm.timestamp));
		if (e != GN_ERR_NONE)
			goto out;
	}
	/* For Birthday and Call types end date does not have sense.
	 * And you cannot set it in the phone. */
	switch (data->calnote->type) {
	case GN_CALNOTE_BIRTHDAY:
		data->calnote->time.year = message[42] * 256 + message[43];
	case GN_CALNOTE_CALL:
	case GN_CALNOTE_REMINDER:
		data->calnote->end_time.year = 0;
		break;
	default:
		break;
	}
	/* Alarm tone */
	tone1 = 256 * message[22] + message[23];
	tone2 = 256 * message[24] + message[25];
	switch (65536 * tone1 + tone2) {
	case 0x00:
		data->calnote->alarm.tone = 0;
		break;
	case 0xffffffff:
		data->calnote->alarm.tone = 1;
		break;
	default:
		data->calnote->alarm.tone = 1;
		break;
	}
	/* Main text */
	len1 = 256 * message[50] + message[51];
	char_unicode_decode(data->calnote->text, message + 54, len1 * 2);
	/* Additional text: location or number */
	len2 = message[52];
	if (len2) {
		switch (data->calnote->type) {
		case GN_CALNOTE_MEETING:
			char_unicode_decode(data->calnote->mlocation,
					message + 54 + len1 * 2, len2 * 2);
			break;
		case GN_CALNOTE_CALL:
			char_unicode_decode(data->calnote->phone_number,
					message + 54 + len1 * 2, len2 * 2);
			break;
		default: /* Not sure about this one */
			/* ignore */
			dprintf("some extra text here\n");
			break;
		}
	}
out:
	return e;
}

#define NEXT_CALNOTE data->calnote_list->location[data->calnote_list->last+i]

static gn_error NK6510_IncomingCalendar(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error e = GN_ERR_NONE;
	int i;

	if (!data || !data->calnote) return GN_ERR_INTERNALERROR;

	switch (message[3]) {
	case NK6510_SUBCAL_NOTE_RCVD: /* 0x1a */
		e = calnote_decode(message, length, data);
		break;
	case NK6510_SUBCAL_NOTE2_RCVD: /* 0x7e */
		e = calnote2_decode(message, length, data);
		break;
	case NK6510_SUBCAL_INFO2_RCVD: /* 0x9f */
		/* message[4]	- number of notes locations in this frame
		 * message[5]	- 0x07
		 * message[6-7] - 0x00 0x00
		 * message[8-9] - number of all notes
		 * ...		- notes location (each is 4 octets)
		 */
		dprintf("Calendar Notes Info received!\n Total count: %i\n", message[8] * 256 + message[9]);
		data->calnote_list->number = message[8] * 256 + message[9];
		dprintf("Location of Notes: ");
		for (i = 0; i < message[4]; i++) {
			if (10 + 4 * i >= length)
				break;
			NEXT_CALNOTE = message[12 + 4 * i] * 256 + message[13 + 4 * i];
			dprintf("%i ", NEXT_CALNOTE);
		}
		dprintf("\n");
		data->calnote_list->last += i;
		if (message[4] == 0)
			data->calnote_list->number = data->calnote_list->last;
		break;
	case NK6510_SUBCAL_INFO_RCVD: /* 0x3b */
		dprintf("Calendar Notes Info received!\n Total count: %i\n", message[4] * 256 + message[5]);
		data->calnote_list->number = message[4] * 256 + message[5];
		dprintf("Location of Notes: ");
		for (i = 0; i < message[6]; i++) {
			if (8 + 2 * i >= length)
				break;
			NEXT_CALNOTE = message[8 + 2 * i] * 256 + message[9 + 2 * i];
			dprintf("%i ", NEXT_CALNOTE);
		}
		dprintf("\n");
		data->calnote_list->last += i;
		if (message[7] != 0)
			data->calnote_list->number = data->calnote_list->last;
		break;
	case NK6510_SUBCAL_FREEPOS_RCVD: /* 0x32 */
		dprintf("First free position received: %i!\n", message[4] * 256 + message[5]);
		data->calnote->location = (((unsigned int)message[4]) << 8) + message[5];
		break;
	case NK6510_SUBCAL_FREEPOS2_RCVD: /* 0x96 */
		dprintf("First free position received: %i!\n", message[7] * 256 + message[8]);
		data->calnote->location = (((unsigned int)message[7]) << 8) + message[8];
		break;
	case NK6510_SUBCAL_DEL_NOTE_RESP: /* 0x0c */
		dprintf("Succesfully deleted calendar note: %i!\n", message[4] * 256 + message[5]);
		break;
	case NK6510_SUBCAL_ADD_MEETING_RESP: /* 0x02 */
	case NK6510_SUBCAL_ADD_CALL_RESP: /* 0x04 */
	case NK6510_SUBCAL_ADD_BIRTHDAY_RESP: /* 0x06 */
	case NK6510_SUBCAL_ADD_REMINDER_RESP: /* 0x08 */
		if (message[6]) e = GN_ERR_FAILED;
		dprintf("Attempt to write calendar note at %i. Status: %i\n",
			message[4] * 256 + message[5],
			1 - message[6]);
		break;
	case NK6510_SUBCAL_ADD_NOTE_RESP: /* 0x66 */
		switch (message[4]) {
		case 0x00:
			dprintf("Calendar note written at %d location\n", message[8] * 256 + message[9]);
			break;
		case 0x04:
			dprintf("Incorrect icon type\n");
			e = GN_ERR_FAILED;
			break;
		default:
			dprintf("Unknown error\n");
			e = GN_ERR_FAILED;
			break;
		}
		break;
	case NK6510_SUBCAL_UNSUPPORTED: /* 0xf0 */
		e = GN_ERR_NOTSUPPORTED;
		break;
	default:
		dprintf("Unknown subtype of type 0x%02x (calendar handling): 0x%02x\n", NK6510_MSG_CALENDAR, message[3]);
		e = GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return e;
}

#define LAST_INDEX (data->calnote_list->last > 0 ? data->calnote_list->last - 1 : 0)
static gn_error NK6510_GetCalendarNotesInfo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x9e, 0xff, 0xff, 0x00, 0x00,
				0x00, 0x00, /* start looking with this position */
				0x00};
	gn_error error;

	if (!data->calnote_list->last)
		data->calnote_list->location[0] = 0;
	do {
		dprintf("Read %d of %d calendar entries\n", data->calnote_list->last, data->calnote_list->number);
		req[8] = data->calnote_list->location[LAST_INDEX] / 256;
		req[9] = data->calnote_list->location[LAST_INDEX] % 256;
		if (sm_message_send(11, NK6510_MSG_CALENDAR, req, state))
			return GN_ERR_NOTREADY;
		dprintf("Message sent.\n");
		error = sm_block(NK6510_MSG_CALENDAR, data, state);
		if (error != GN_ERR_NONE)
			return error;
		dprintf("Message received\n");
	} while (data->calnote_list->last < data->calnote_list->number);
	dprintf("Loop exited\n");
	return error;
}
#undef LAST_INDEX

/* To get fresh information from the phone
 * set data->calnote_list->list to 0
 */
static gn_error NK6510_GetCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NOTREADY;
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x7d, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, /* Location */
				0xff, 0xff, 0xff, 0xff };
	unsigned char date[] = { FBUS_FRAME_HEADER, NK6510_SUBCLO_GET_DATE };
	gn_data	tmpdata;
	gn_timestamp tmptime;

	dprintf("Getting calendar note...\n");
	if (data->calnote->location < 1) {
		error = GN_ERR_INVALIDLOCATION;
	} else {
		tmpdata.datetime = &tmptime;
		dprintf("Getting notes info\n");
		error = NK6510_GetCalendarNotesInfo(data, state);
		dprintf("Got calendar info\n");
		if (error == GN_ERR_NONE) {
			if (!data->calnote_list->number ||
			    data->calnote->location > data->calnote_list->number) {
				error = GN_ERR_EMPTYLOCATION;
			} else {
				error = sm_message_send(4, NK6510_MSG_CLOCK, date, state);
				if (error == GN_ERR_NONE) {
					sm_block(NK6510_MSG_CLOCK, &tmpdata, state);
					req[8] = data->calnote_list->location[data->calnote->location - 1] >> 8;
					req[9] = data->calnote_list->location[data->calnote->location - 1] & 0xff;
					data->calnote->time.year = tmptime.year;

					error = sm_message_send(14, NK6510_MSG_CALENDAR, req, state);
					if (error == GN_ERR_NONE) {
						error = sm_block(NK6510_MSG_CALENDAR, data, state);
					}
				}
			}
		}
	}

	return error;
}

long NK6510_GetNoteAlarmDiff(gn_timestamp *time, gn_timestamp *alarm)
{
	time_t     t_alarm;
	time_t     t_time;
	struct tm  tm_alarm;
	struct tm  tm_time;

	tzset();

	tm_alarm.tm_year  = alarm->year-1900;
	tm_alarm.tm_mon   = alarm->month-1;
	tm_alarm.tm_mday  = alarm->day;
	tm_alarm.tm_hour  = alarm->hour;
	tm_alarm.tm_min   = alarm->minute;
	tm_alarm.tm_sec   = alarm->second;
	tm_alarm.tm_isdst = 0;
	t_alarm = mktime(&tm_alarm);

	tm_time.tm_year  = time->year-1900;
	tm_time.tm_mon   = time->month-1;
	tm_time.tm_mday  = time->day;
	tm_time.tm_hour  = time->hour;
	tm_time.tm_min   = time->minute;
	tm_time.tm_sec   = time->second;
	tm_time.tm_isdst = 0;
	t_time = mktime(&tm_time);

	dprintf("\tAlarm: %02i-%02i-%04i %02i:%02i:%02i\n",
		alarm->day, alarm->month, alarm->year,
		alarm->hour, alarm->minute, alarm->second);
	dprintf("\tDate: %02i-%02i-%04i %02i:%02i:%02i\n",
		time->day, time->month, time->year,
		time->hour, time->minute, time->second);
	dprintf("Difference in alarm time is %f\n", difftime(t_time, t_alarm));

	return difftime(t_time, t_alarm);
}

static gn_error NK6510_FirstCalendarFreePos(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x31 };
	SEND_MESSAGE_BLOCK(NK6510_MSG_CALENDAR, 4);
}

static gn_error NK6510_FirstCalendarFreePos2(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x95, 0x00 };
	SEND_MESSAGE_BLOCK(NK6510_MSG_CALENDAR, 5);
}

static gn_error NK6510_WriteCalendarNote2(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	gn_calnote *calnote;
	int len, count = 54;
	unsigned char req[1024] = { FBUS_FRAME_HEADER,
				    0x65,
				    0x00, /* category: 0x00 calendar, 0x01 todo */
				    0x00, 0x00, 0x00,
				    0x00, 0x00, /* location */
				    0x00, 0x00, 0x00, 0x00,
				    0xff, 0xff, 0xff, 0xff, /* alarm */
				    0x00, 0x00, 0x00,
				    0x00, /* note icon */
				    0xff, 0xff, 0xff, 0xff, /* alarm tone */
				    0x00, /* 0x02 for reminder */
				    0x00, /* note type: 0x00 - reminder
							0x01 - meeting
							0x02 - call
							0x04 - birthday
							0x08 - memo */
				    0x07, 0xd0, 0x01, 0x12, 0x0c, 0x00, /* start datetime */
				    0x07, 0xd0, 0x01, 0x12, 0x0c, 0x00, /* end datetime */
				    0x00, 0x00, /* recurrence */
				    0xff, 0xff, /* birth year */
				    0x20, /* todo priority */
				    0x00, /* todo completed */
				    0x00, 0x00, /* number of occurrences */
				    0x00,
				    0x00, /* text length */
				    0x00, /* phone length/meeting location */
				    0x00, 0x00, 0x00};

	dprintf("WriteCalendarNote2\n");
	if (!data->calnote)
		return GN_ERR_INTERNALERROR;

	calnote = data->calnote;

	/* 6510 needs to seek the first free pos to inhabit with next note */
	error = NK6510_FirstCalendarFreePos2(data, state);
	if (error != GN_ERR_NONE)
		return error;

	/* Set location */
	req[8] = calnote->location / 256;
	req[9] = calnote->location % 256;

	/* text */
	req[49] = char_mblen(calnote->text);
	len = char_unicode_encode(req + 54, calnote->text, strlen(calnote->text));
	count += len;

	/* Set calnote types */
	switch (calnote->type) {
	case GN_CALNOTE_REMINDER:
		req[26] = 0x02;
		/* end date == start date */
		memcpy(&(calnote->end_time), &(calnote->time), sizeof(calnote->time));
		break;
	case GN_CALNOTE_MEETING:
		req[27] = 0x01;
		req[50] = char_mblen(calnote->mlocation);
		count += char_unicode_encode(req + 54 + len, calnote->mlocation, strlen(calnote->mlocation));
		break;
	case GN_CALNOTE_CALL:
		req[27] = 0x02;
		req[50] = strlen(calnote->phone_number);
		count += char_unicode_encode(req + 54 + len, calnote->phone_number, strlen(calnote->phone_number));
		break;
	case GN_CALNOTE_BIRTHDAY:
		req[27] = 0x04;
		/* Birthday year */
		req[42] = calnote->time.year / 256;
		req[43] = calnote->time.year % 256;
		/* Event time: 2005 */
		calnote->time.year = 2005;
		/* end date == start date */
		memcpy(&(calnote->end_time), &(calnote->time), sizeof(calnote->time));
		break;
	case GN_CALNOTE_MEMO:
		req[27] = 0x08;
		break;
	}

	/* Recurrence */
	if (calnote->recurrence >= 8760)
		calnote->recurrence = 0xffff; /* setting 1 year repeat */
	req[40] = calnote->recurrence / 256;
	req[41] = calnote->recurrence % 256;

	/* Occurrences */
	/* FIXME: it doesn't work 
	if (calnote->recurrence) {
		req[46] = calnote->occurrences / 256;
		req[47] = calnote->occurrences % 256;
	}*/

	/* Set start time and end time */
	req[28] = calnote->time.year / 256;
	req[29] = calnote->time.year % 256;
	req[30] = calnote->time.month;
	req[31] = calnote->time.day;
	req[32] = calnote->time.hour;
	req[33] = calnote->time.minute;

	req[34] = calnote->end_time.year / 256;
	req[35] = calnote->end_time.year % 256;
	req[36] = calnote->end_time.month;
	req[37] = calnote->end_time.day;
	req[38] = calnote->end_time.hour;
	req[39] = calnote->end_time.minute;

	/* Alarm */
	if (calnote->alarm.enabled) {
		long seconds;

		/* Alarm for birthday is special. We need to set for the same
		 * year we set the start date. */
		if (calnote->type == GN_CALNOTE_BIRTHDAY)
			calnote->time.year = calnote->alarm.timestamp.year;
		seconds = NK6510_GetNoteAlarmDiff(&calnote->time,
						 &calnote->alarm.timestamp);
		if (seconds >= 0) { /* Otherwise it's an error condition.... */
			req[14] = (seconds / 60) >> 24;
			req[15] = (seconds / 60) >> 16;
			req[16] = (seconds / 60) >> 8;
			req[17] = (seconds / 60);
		}

		if (!calnote->alarm.tone) {
			req[22] = req[23] = req[24] = req[25] = 0;
		}
	}

	SEND_MESSAGE_BLOCK(NK6510_MSG_CALENDAR, count);
}

static gn_error NK6510_WriteCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[350] = { FBUS_FRAME_HEADER,
				   0x01,       /* note type ... */
				   0x00, 0x00, /* location */
				   0x00,       /* entry type */
				   0x00,       /* fixed */
				   0x00, 0x00, 0x00, 0x00, /* Year(2bytes), Month, Day */
				   /* here starts block */
				   0x00, 0x00, 0x00, 0x00,0x00, 0x00}; /* ... depends on note type ... */

	gn_calnote *calnote;
	int count = 0;
	int len = 0;
	long seconds, minutes;
	gn_error error;

	if (DRVINSTANCE(state)->pm->flags & PM_EXTCALENDAR)
		return NK6510_WriteCalendarNote2(data, state);

	dprintf("WriteCalendarNote\n");
	if (!data->calnote)
		return GN_ERR_INTERNALERROR;

	calnote = data->calnote;

	/* 6510 needs to seek the first free pos to inhabit with next note */
	error = NK6510_FirstCalendarFreePos(data, state);
	if (error != GN_ERR_NONE)
		if (error == GN_ERR_NOTSUPPORTED) {
			/*
			 * GN_ERR_NOTSUPPORTED most likely means 0xf0 frame. Experience shows that
			 * with high probability we have series40 3rd+ Ed phone.
			 */
			error = NK6510_WriteCalendarNote2(data, state);
			if (error == GN_ERR_NONE) {
				dprintf("Misconfiguration in the phone table detected.\nPlease report to gnokii ml (gnokii-users@nongnu.org).\n");
				dprintf("Model %s (%s) is series40 3rd+ Edition.\n", DRVINSTANCE(state)->pm->product_name, DRVINSTANCE(state)->pm->model);
				DRVINSTANCE(state)->pm->flags |= PM_DEFAULT_S40_3RD;
			}
			return error;
		}
		return error;

	/* Location */
	req[4] = calnote->location >> 8;
	req[5] = calnote->location & 0xff;

	switch (calnote->type) {
	case GN_CALNOTE_MEETING:
		req[6] = 0x01;
		req[3] = 0x01;
		break;
	case GN_CALNOTE_CALL:
		req[6] = 0x02;
		req[3] = 0x03;
		break;
	case GN_CALNOTE_BIRTHDAY:
		req[6] = 0x04;
		req[3] = 0x05;
		break;
	case GN_CALNOTE_REMINDER:
		req[6] = 0x08;
		req[3] = 0x07;
		break;
	case GN_CALNOTE_MEMO:
		return GN_ERR_INTERNALERROR; /* not yet implemented! */
	}

	req[8]  = calnote->time.year >> 8;
	req[9]  = calnote->time.year & 0xff;
	req[10] = calnote->time.month;
	req[11] = calnote->time.day;

	/* From here starts BLOCK */
	count = 12;
	switch (calnote->type) {

	case GN_CALNOTE_MEETING:
		req[count++] = calnote->time.hour;   /* Field 12 */
		req[count++] = calnote->time.minute; /* Field 13 */
		/* Alarm .. */
		req[count++] = 0xff; /* Field 14 */
		req[count++] = 0xff; /* Field 15 */
		if (calnote->alarm.timestamp.year) {
			seconds = NK6510_GetNoteAlarmDiff(&calnote->time,
							 &calnote->alarm.timestamp);
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* recurrence */
		if (calnote->recurrence >= 8760)
			calnote->recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = calnote->recurrence >> 8;   /* Field 16 */
		req[count++] = calnote->recurrence & 0xff; /* Field 17 */
		/* len of the text */
		req[count++] = strlen(calnote->text);    /* Field 18 */
		/* fixed 0x00 */
		req[count++] = 0x00; /* Field 19 */

		/* Text */
		dprintf("Count before encode = %d\n", count);
		dprintf("Meeting Text is = \"%s\"\n", calnote->text);

		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 20->N */
		count += len;
		break;

	case GN_CALNOTE_CALL:
		req[count++] = calnote->time.hour;   /* Field 12 */
		req[count++] = calnote->time.minute; /* Field 13 */
		/* Alarm .. */
		req[count++] = 0xff; /* Field 14 */
		req[count++] = 0xff; /* Field 15 */
		if (calnote->alarm.timestamp.year) {
			seconds = NK6510_GetNoteAlarmDiff(&calnote->time,
							&calnote->alarm.timestamp);
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				minutes = seconds / 60L;
				count -= 2;
				req[count++] = minutes >> 8;
				req[count++] = minutes & 0xff;
			}
		}
		/* recurrence */
		if (calnote->recurrence >= 8760)
			calnote->recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = calnote->recurrence >> 8;   /* Field 16 */
		req[count++] = calnote->recurrence & 0xff; /* Field 17 */
		/* len of text */
		req[count++] = strlen(calnote->text);    /* Field 18 */
		/* fixed 0x00 */
		req[count++] = strlen(calnote->phone_number);   /* Field 19 */
		/* Text */
		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 20->N */
		count += len;
		len = char_unicode_encode(req + count, calnote->phone_number, strlen(calnote->phone_number)); /* Fields (N+1)->n */
		count += len;
		break;
	case GN_CALNOTE_BIRTHDAY:
		req[count++] = 0x00; /* Field 12 Fixed */
		req[count++] = 0x00; /* Field 13 Fixed */

		/* Alarm .. */
		req[count++] = 0x00;
		req[count++] = 0x00; /* Fields 14, 15 */
		req[count++] = 0xff; /* Field 16 */
		req[count++] = 0xff; /* Field 17 */
		if (calnote->alarm.timestamp.year) {
			/* First I try Time.year = Alarm.year. If negative, I increase year by one,
			   but only once! This is because I may have alarm period across
			   the year border, eg. birthday on 2001-01-10 and alarm on 2000-12-27 */
			calnote->time.year = calnote->alarm.timestamp.year;
			if ((seconds= NK6510_GetNoteAlarmDiff(&calnote->time,
							     &calnote->alarm.timestamp)) < 0L) {
				calnote->time.year++;
				seconds = NK6510_GetNoteAlarmDiff(&calnote->time,
								 &calnote->alarm.timestamp);
			}
			if (seconds >= 0L) { /* Otherwise it's an error condition.... */
				count -= 4;
				req[count++] = seconds >> 24;              /* Field 14 */
				req[count++] = (seconds >> 16) & 0xff;     /* Field 15 */
				req[count++] = (seconds >> 8) & 0xff;      /* Field 16 */
				req[count++] = seconds & 0xff;             /* Field 17 */
			}
		}

		req[count++] = 0x00; /* FIXME: calnote->AlarmType; 0x00 tone, 0x01 silent 18 */

		/* len of text */
		req[count++] = strlen(calnote->text); /* Field 19 */

		/* Text */
		dprintf("Count before encode = %d\n", count);

		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 22->N */
		count += len;
		break;

	case GN_CALNOTE_REMINDER:
		/* recurrence */
		if (calnote->recurrence >= 8760)
			calnote->recurrence = 0xffff; /* setting  1 Year repeat */
		req[count++] = calnote->recurrence >> 8;   /* Field 12 */
		req[count++] = calnote->recurrence & 0xff; /* Field 13 */
		/* len of text */
		req[count++] = strlen(calnote->text);    /* Field 14 */
		/* fixed 0x00 */
		req[count++] = 0x00; /* Field 15 */
		/* Text */
		len = char_unicode_encode(req + count, calnote->text, strlen(calnote->text)); /* Fields 16->N */
		count += len;
		break;
	case GN_CALNOTE_MEMO:
		return GN_ERR_INTERNALERROR; /* not yet implemented! */
	}

	/* padding */
	req[count] = 0x00;
	dprintf("Count after padding = %d\n", count);

	SEND_MESSAGE_BLOCK(NK6510_MSG_CALENDAR, count);
}

static gn_error NK6510_DeleteCalendarNote(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER,
				0x0b,      /* delete calendar note */
				0x00, 0x00}; /*location */
	gn_calnote_list list;
	bool own_list = true;

	if (data->calnote_list)
		own_list = false;
	else {
		memset(&list, 0, sizeof(gn_calnote_list));
		data->calnote_list = &list;
	}

	if (data->calnote_list->number == 0)
		NK6510_GetCalendarNotesInfo(data, state);

	if (data->calnote->location < data->calnote_list->number + 1 &&
	    data->calnote->location > 0) {
		req[4] = data->calnote_list->location[data->calnote->location - 1] >> 8;
		req[5] = data->calnote_list->location[data->calnote->location - 1] & 0xff;
	} else {
		return GN_ERR_INVALIDLOCATION;
	}

	if (own_list)
		data->calnote_list = NULL;
	SEND_MESSAGE_BLOCK(NK6510_MSG_CALENDAR, 6);
}

/********************/
/* INCOMING NETWORK */
/********************/

static gn_error NK6510_IncomingNetwork(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char *blockstart, *operatorname;
	int i;

	switch (message[3]) {
	case 0x01:
	case 0x02:
		blockstart = message + 6;
		for (i = 0; i < message[5]; i++) {
			dprintf("Blockstart: %i, ", blockstart[0]);
			switch (blockstart[0]) {
			case 0x00:
				dprintf("Network status\n");
				switch (blockstart[2]) {
				case 0x00:
					dprintf("Logged into home network.\n");
					break;
				case 0x01:
					dprintf("Logged into a roaming network.\n");
					break;
				case 0x04:
				case 0x09:
					dprintf("Not logged in any network.\n");
					break;
				case 0x06:
				case 0x0b:
					dprintf("Inactive SIM.\n");
					break;
				case 0x08:
					dprintf("Flight mode.\n");
					break;
				default:
					dprintf("Unknown network status 0x%02x!\n", blockstart[2]);
					break;
				}
				operatorname = malloc(blockstart[5] + 1);
				char_unicode_decode(operatorname, blockstart + 6, blockstart[5] << 1);
				dprintf("Operator Name: %s\n", operatorname);
				free(operatorname);
				break;
			case 0x09:  /* Operator details */
				dprintf("Operator details\n");
				/* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
				if (data->network_info) {
					data->network_info->cell_id[0] = blockstart[6];
					data->network_info->cell_id[1] = blockstart[7];
					data->network_info->LAC[0] = blockstart[2];
					data->network_info->LAC[1] = blockstart[3];
					data->network_info->network_code[0] = '0' + (blockstart[8] & 0x0f);
					data->network_info->network_code[1] = '0' + (blockstart[8] >> 4);
					data->network_info->network_code[2] = '0' + (blockstart[9] & 0x0f);
					data->network_info->network_code[3] = ' ';
					data->network_info->network_code[4] = '0' + (blockstart[10] & 0x0f);
					data->network_info->network_code[5] = '0' + (blockstart[10] >> 4);
					data->network_info->network_code[6] = 0;
				}
				break;
			default:
				dprintf("Unknown operator block\n");
				break;
			}
			blockstart += blockstart[1];
		}
		break;
	case 0x0b:
		/*
		  no idea what this is about
		  00 01 00 0b 00 02 00 00 00
		*/
		break;
	case 0x0c: /* RF Level */
		dprintf("RF level: %f\n", message[8]);
		if (data->rf_level) {
			*(data->rf_unit) = GN_RF_Percentage;
			*(data->rf_level) = message[8];
		}
		break;
	case 0x1e: /* RF Level change notify */
		/*
		  01 56 00 1E 0D 65
		  01 56 00 1E 0C 65
		  01 56 00 1E 0B 65
		  01 56 00 1E 09 66
		  01 56 00 1E 08 66
		  01 56 00 1E 0A 66
		*/
		dprintf("RF level: %f\n", message[4]);
		if (data->rf_level) {
			*(data->rf_unit) = GN_RF_Percentage;
			*(data->rf_level) = message[4];
		}
		break;
	case 0x20:
		dprintf("Incoming call(?)\n");
		/*
		  no idea what this is about
		  01 56 00 20 02 00 55
		  01 56 00 20 01 00 55
		*/
		break;
	case 0x24:
		if (length == 18)
			return GN_ERR_EMPTYLOCATION;
		if (data->bitmap) {
			data->bitmap->netcode[0] = '0' + (message[12] & 0x0f);
			data->bitmap->netcode[1] = '0' + (message[12] >> 4);
			data->bitmap->netcode[2] = '0' + (message[13] & 0x0f);
			data->bitmap->netcode[3] = ' ';
			data->bitmap->netcode[4] = '0' + (message[14] & 0x0f);
			data->bitmap->netcode[5] = '0' + (message[14] >> 4);
			data->bitmap->netcode[6] = 0;
			dprintf("Operator %s \n",data->bitmap->netcode);
			data->bitmap->type = GN_BMP_NewOperatorLogo;
			data->bitmap->height = message[21];
			data->bitmap->width = message[20];
			data->bitmap->size = message[25];
			dprintf("size: %i\n", data->bitmap->size);
			memcpy(data->bitmap->bitmap, message + 26, data->bitmap->size);
			dprintf("Logo (%dx%d) \n", data->bitmap->height, data->bitmap->width);
		} else
			return GN_ERR_INTERNALERROR;
		break;
	case 0x26:
		dprintf("Op Logo Set OK\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x0a (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_GetNetworkInfo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x00, 0x00};

	dprintf("Getting network info ...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_NETSTATUS, 5);
}

static gn_error NK6510_GetRFLevel(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0B, 0x00, 0x02, 0x00, 0x00, 0x00};

	dprintf("Getting network info ...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_NETSTATUS, 9);
}

static gn_error GetOperatorBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x23, 0x00, 0x00, 0x55, 0x55, 0x55};

	dprintf("Getting op logo...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_NETSTATUS, 9);
}

static gn_error SetOperatorBitmap(gn_data *data, struct gn_statemachine *state)
{
	/* unsigned char req[1000] = {FBUS_FRAME_HEADER, 0x25, 0x01, 0x55, 0x00, 0x00, 0x55, */
	unsigned char req[1000] = {FBUS_FRAME_HEADER, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x02, /* Blocks */
				  0x0c, 0x08, /* type, length */
				  /* 0x62, 0xf2, 0x20, 0x03, 0x55, 0x55, */
				  0x62, 0xf2, 0x20, 0x03, 0x00, 0x00,
				  0x1a};

	memset(req + 19, 0, 881);
	/*
	  00 01 00 25 01 55 00 00 55 02
	  0C 08 62 F2 20 03 55 55
	  1A F4 4E 15 00 EA 00 EA 5F 91 4E 80 5F 95 51 80 DF C2 DF 7D E0 4E 11 0E 80 55 20 E0 E0 F0 EA 7D E0 E0 7D E0 E
	*/
	if ((data->bitmap->width != state->driver.phone.operator_logo_width) ||
	    (data->bitmap->height != state->driver.phone.operator_logo_height)) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",
			state->driver.phone.operator_logo_height, state->driver.phone.operator_logo_width, data->bitmap->height, data->bitmap->width);
		return GN_ERR_INVALIDSIZE;
	}

	if (strcmp(data->bitmap->netcode, "000 00")) {  /* set logo */
		req[12] = ((data->bitmap->netcode[1] & 0x0f) << 4) | (data->bitmap->netcode[0] & 0x0f);
		req[13] = 0xf0 | (data->bitmap->netcode[2] & 0x0f);
		req[14] = ((data->bitmap->netcode[5] & 0x0f) << 4) | (data->bitmap->netcode[4] & 0x0f);

		req[19] = data->bitmap->size + 8;
		req[20] = data->bitmap->width;
		req[21] = data->bitmap->height;
		req[23] = req[25] = data->bitmap->size;
		memcpy(req + 26, data->bitmap->bitmap, data->bitmap->size);
	}
	dprintf("Setting op logo...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_NETSTATUS, req[19] + req[11] + 14);
}

/*********************/
/* INCOMING BATTERY */
/*********************/

static gn_error NK6510_IncomingBattLevel(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	case 0x0B:
		if (!data->battery_level) return GN_ERR_INTERNALERROR;
		*(data->battery_unit) = GN_BU_Percentage;
		*(data->battery_level) = message[9] * 100 / state->driver.phone.max_battery_level;
		dprintf("Battery level %f\n\n", *(data->battery_level));
		break;
	default:
		dprintf("Unknown subtype of type 0x17 (%d)\n", message[3]);
		return GN_ERR_UNKNOWN;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_GetBatteryLevel(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x0A, 0x02, 0x00};

	dprintf("Getting battery level...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_BATTERY, 6);
}

/*************/
/* RINGTONES */
/*************/

static gn_error NK6510_IncomingRingtone(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i, j, n;
	unsigned char *pos;
	gn_ringtone_list *rl;

	switch (message[3]) {
	case 0x08:
		dprintf("List of ringtones received!\n");
		if (!(rl = data->ringtone_list)) return GN_ERR_INTERNALERROR;
		rl->count = 256 * message[4] + message[5];
		rl->userdef_location = NK6510_RINGTONE_USERDEF_LOCATION;
		rl->userdef_count = 10;
		if (rl->count > GN_RINGTONE_MAX_COUNT) rl->count = GN_RINGTONE_MAX_COUNT;
		i = 6;
		for (j = 0; j < rl->count; j++) {
			if (message[i + 4] != 0x01 || message[i + 6] != 0x00) return GN_ERR_UNHANDLEDFRAME;
			rl->ringtone[j].location = 256 * message[i + 2] + message[i + 3];
			rl->ringtone[j].user_defined = (message[i + 5] == 0x02);
			rl->ringtone[j].readable = 1;
			rl->ringtone[j].writable = rl->ringtone[j].user_defined;
			n = message[i + 7];
			if (n >= sizeof(rl->ringtone[0].name)) n = sizeof(rl->ringtone[0].name) - 1;
			char_unicode_decode(rl->ringtone[j].name, message + i + 8, 2 * n);
			i += 256 * message[i] + message[i + 1];

			dprintf("Ringtone (#%03i) name: %s\n", rl->ringtone[j].location, rl->ringtone[j].name);
		}
		break;

	/* set raw ringtone result */
	case 0x0f:
		if (message[5] != 0x00) return GN_ERR_UNHANDLEDFRAME;
		switch (message[4]) {
		case 0x00:
			break;
		case 0x03:
			dprintf("Invalid location\n");
			return GN_ERR_INVALIDLOCATION;
		case 0x0e:
			dprintf("Ringtone too long. Max is 69 notes.\n");
			return GN_ERR_ENTRYTOOLONG;
		default:
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* delete ringtone result */
	case 0x11:
		if (message[5] != 0x00) return GN_ERR_UNHANDLEDFRAME;
		switch (message[4]) {
		case 0x00: break;
		case 0x03: return GN_ERR_INVALIDLOCATION;
		case 0x0a: return GN_ERR_EMPTYLOCATION;
		default:   return GN_ERR_UNHANDLEDFRAME;
		}
		break;

	/* get raw ringtone result */
	case 0x13:
		if (!data->ringtone || !data->raw_data) return GN_ERR_INTERNALERROR;
		pos = message + 8;
		char_unicode_decode(data->ringtone->name, pos, 2 * message[7]);
		dprintf("Got ringtone: %s\n", data->ringtone->name);
		pos += 2 * message[7];
		i = (pos[0] << 8) + pos[1];
		pos += 2;
		dprintf("Ringtone size: %d\n", i);
		if (data->raw_data->length < i) {
			dprintf("Expected max %d bytes, got %d bytes\n", data->raw_data->length, i);
			return GN_ERR_INVALIDSIZE;
		}
		data->raw_data->length = i;
		memcpy(data->raw_data->data, pos, i);
		break;

	/* get raw ringtone failed */
	case 0x14:
		return GN_ERR_INVALIDLOCATION;

	default:
		dprintf("Unknown subtype of type 0x1f (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_GetRingtoneList(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x07, 0x00, 0x00, 0xFE, 0x00, 0x7D};

	dprintf("Getting list of ringtones...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_RINGTONE, 9);
}

static gn_error NK6510_GetRawRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x12, 0x00, 0x00};

	if (!data->ringtone || !data->raw_data) return GN_ERR_INTERNALERROR;

	dprintf("Getting raw ringtone %d...\n", data->ringtone->location);
	req[4] = data->ringtone->location / 256;
	req[5] = data->ringtone->location % 256;

	SEND_MESSAGE_BLOCK(NK6510_MSG_RINGTONE, 6);
}

static gn_error NK6510_SetRawRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[32768] = {FBUS_FRAME_HEADER, 0x0e, 0x00, 0x00, 0xfe};
	unsigned char *pos;
	int len;

	if (!data->ringtone || !data->raw_data) return GN_ERR_INTERNALERROR;

	dprintf("Setting raw ringtone %d...\n", data->ringtone->location);

	pos = req + 4;
	if (data->ringtone->location < 0) {
		*pos++ = 0x7f;
		*pos++ = 0xff;
	} else {
		*pos++ = data->ringtone->location / 256;
		*pos++ = data->ringtone->location % 256;
	}
	pos++;
	*pos = strlen(data->ringtone->name);
	len = char_unicode_encode(pos + 1, data->ringtone->name, *pos);
	pos += 1 + len;
	*pos++ = data->raw_data->length / 256;
	*pos++ = data->raw_data->length % 256;
	if (pos - req + data->raw_data->length + 2 > sizeof(req)) return GN_ERR_INVALIDSIZE;
	memcpy(pos, data->raw_data->data, data->raw_data->length);
	pos += data->raw_data->length;
	*pos++ = 0;
	*pos++ = 0;

	SEND_MESSAGE_BLOCK(NK6510_MSG_RINGTONE, pos - req);
}

static gn_error NK6510_GetRingtone(gn_data *data, struct gn_statemachine *state)
{
	gn_data d;
	gn_error err;
	gn_raw_data rawdata;
	char buf[4096];

	if (!data->ringtone) return GN_ERR_INTERNALERROR;

	memset(&rawdata, 0, sizeof(gn_raw_data));
	rawdata.data = buf;
	rawdata.length = sizeof(buf);
	gn_data_clear(&d);
	d.ringtone = data->ringtone;
	d.raw_data = &rawdata;

	if ((err = NK6510_GetRawRingtone(&d, state)) != GN_ERR_NONE) return err;

	return pnok_ringtone_from_raw(data->ringtone, rawdata.data, rawdata.length);
}

static gn_error NK6510_SetRingtone(gn_data *data, struct gn_statemachine *state)
{
	gn_data d;
	gn_error err;
	gn_raw_data rawdata;
	char buf[4096];

	if (!data->ringtone) return GN_ERR_INTERNALERROR;

	memset(&rawdata, 0, sizeof(gn_raw_data));
	rawdata.data = buf;
	rawdata.length = sizeof(buf);
	gn_data_clear(&d);
	d.ringtone = data->ringtone;
	d.raw_data = &rawdata;

	if ((err = pnok_ringtone_to_raw(rawdata.data, &rawdata.length, data->ringtone, 1)) != GN_ERR_NONE)
		return err;

	return NK6510_SetRawRingtone(&d, state);
}

static gn_error NK6510_DeleteRingtone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x10, 0x00, 0x00};

	if (!data->ringtone) return GN_ERR_INTERNALERROR;

	if (data->ringtone->location < 0) {
		req[4] = 0x7f;
		req[5] = 0xfe;
	} else {
		req[4] = data->ringtone->location / 256;
		req[5] = data->ringtone->location % 255;
	}

	SEND_MESSAGE_BLOCK(NK6510_MSG_RINGTONE, 6);
}

/*************/
/* START UP  */
/*************/

static gn_error NK6510_IncomingStartup(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/*
UNHANDLED FRAME RECEIVED
request: 0x7a / 0x0005
00 01 00 02 0b                                  |
reply: 0x7a / 0x0036
01 44 00 03 0b 00 00 00 01 38 00 00 00 00 00 00 |  D       8
35 5f 00 00 00 00 00 00 00 3e 00 00 00 00 00 00 | 5_       >
36 97 00 00 00 00 01 55 55 55 55 55 55 55 55 55 | 6      UUUUUUUUU
55 55 55 55 55 55                               | UUUUUU
	*/
	switch (message[3]) {
	case 0x03:
		switch (message[4]) {
		case 0x01:
			dprintf("Greeting text received\n");
			char_unicode_decode(data->bitmap->text, message + 6, length - 7);
			return GN_ERR_NONE;
			break;
		case 0x05:
			if (message[6] == 0)
				dprintf("Anykey answer not set!\n");
			else
				dprintf("Anykey answer set!\n");
			return GN_ERR_NONE;
			break;
		case 0x0f:
			if (!data->bitmap) return GN_ERR_INTERNALERROR;
				/* I'm sure there are blocks here but never mind! */
				/* yes there are:
				   c0 02 00 41   height
				   c0 03 00 60   width
				   c0 04 03 60   size
				*/
			data->bitmap->type = GN_BMP_StartupLogo;
			data->bitmap->height = message[13];
			data->bitmap->width = message[17];
			data->bitmap->size = message[20] * 256 + message[21];

			memcpy(data->bitmap->bitmap, message + 22, data->bitmap->size);
			dprintf("Startup logo got ok - height(%d) width(%d)\n", data->bitmap->height, data->bitmap->width);
			return GN_ERR_NONE;
			break;
		default:
			dprintf("Unknown sub-subtype of type 0x7a subtype 0x03(%d)\n", message[4]);
			return GN_ERR_UNHANDLEDFRAME;
			break;
		}
	case 0x05:
		switch (message[4]) {
		case 0x0f:
			if (message[5] == 0)
				dprintf("Operator logo successfully set!\n");
			else
				dprintf("Setting operator logo failed!\n");
			return GN_ERR_NONE;
			break;
		default:
			dprintf("Unknown sub-subtype of type 0x7a subtype 0x05 (%d)\n", message[4]);
			return GN_ERR_UNHANDLEDFRAME;
			break;
		}

	default:
		dprintf("Unknown subtype of type 0x7a (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
}

static gn_error SetStartupBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[1000] = {FBUS_FRAME_HEADER, 0x04, 0x0f, 0x00, 0x00, 0x00, 0x04,
				   0xc0, 0x02, 0x00, 0x00,           /* Height */
				   0xc0, 0x03, 0x00, 0x00,           /* Width */
				   0xc0, 0x04, 0x03, 0x60 };         /* size */
	int count = 21;

	if ((data->bitmap->width != state->driver.phone.startup_logo_width) ||
	    (data->bitmap->height != state->driver.phone.startup_logo_height)) {
		dprintf("Invalid image size - expecting (%dx%d) got (%dx%d)\n",
			state->driver.phone.startup_logo_height, state->driver.phone.startup_logo_width,
			data->bitmap->height, data->bitmap->width);

	    return GN_ERR_INVALIDSIZE;
	}

	req[12] = data->bitmap->height;
	req[16] = data->bitmap->width;
	memcpy(req + count, data->bitmap->bitmap, data->bitmap->size);
	count += data->bitmap->size;
	dprintf("Setting startup logo...\n");

	SEND_MESSAGE_BLOCK(NK6510_MSG_STLOGO, count);
}

static gn_error GetWelcomeNoteText(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02, 0x01, 0x00};

	dprintf("Getting startup greeting...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_STLOGO, 6);
}

static gn_error NK6510_GetAnykeyAnswer(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02, 0x05, 0x00, 0x7d};

	dprintf("See if anykey answer is set...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_STLOGO, 7);
}

static gn_error GetStartupBitmap(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x02, 0x0f};

	dprintf("Getting startup logo...\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_STLOGO, 5);
}

/***************/
/*   PROFILES **/
/***************/
static gn_error NK6510_IncomingProfile(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	unsigned char *blockstart;
	int i;

	switch (message[3]) {
	case 0x02:
		if (!data->profile) return GN_ERR_INTERNALERROR;
		blockstart = message + 7;
		for (i = 0; i < 11; i++) {
			switch (blockstart[1]) {
			case 0x00:
				dprintf("type: %02x, keypad tone level: %i\n", blockstart[1], blockstart[7]);
				switch (blockstart[7]) {
				case 0x00:
					data->profile->keypad_tone = 0xff;
					break;
				case 0x01:
					data->profile->keypad_tone = 0x00;
					break;
				case 0x02:
					data->profile->keypad_tone = 0x01;
					break;
				case 0x03:
					data->profile->keypad_tone = 0x02;
					break;
				default:
					dprintf("Unknown keypad tone volume!\n");
					break;
				}
				break;
			case 0x02:
				dprintf("type: %02x, call alert: %i\n", blockstart[1], blockstart[7]);
				data->profile->call_alert = blockstart[7];
				break;
			case 0x03:
				dprintf("type: %02x, ringtone: %i\n", blockstart[1], blockstart[7]);
				data->profile->ringtone = blockstart[7];
				break;
			case 0x04:
				dprintf("type: %02x, ringtone volume: %i\n", blockstart[1], blockstart[7]);
				data->profile->volume = blockstart[7] + 6;
				break;
			case 0x05:
				dprintf("type: %02x, message tone: %i\n", blockstart[1], blockstart[7]);
				data->profile->message_tone = blockstart[7];
				break;
			case 0x06:
				dprintf("type: %02x, vibration: %i\n", blockstart[1], blockstart[7]);
				data->profile->vibration = blockstart[7];
				break;
			case 0x07:
				dprintf("type: %02x, warning tone: %i\n", blockstart[1], blockstart[7]);
				data->profile->warning_tone = blockstart[7];
				break;
			case 0x08:
				dprintf("type: %02x, caller groups: %i\n", blockstart[1], blockstart[7]);
				data->profile->caller_groups = blockstart[7];
				break;
			case 0x0c:
				char_unicode_decode(data->profile->name, blockstart + 7, blockstart[6] << 1);
				dprintf("Profile Name: %s\n", data->profile->name);
				break;
			default:
				dprintf("Unknown profile subblock type %02x!\n", blockstart[1]);
				break;
			}
			blockstart = blockstart + blockstart[0];
		}
		return GN_ERR_NONE;
		break;
	case 0x04:
		dprintf("Response to profile writing received!\n");

		blockstart = message + 6;
		for (i = 0; i < message[5]; i++) {
			switch (blockstart[2]) {
			case 0x00:
				if (message[4] == 0x00)
					dprintf("keypad tone level successfully set!\n");
				else
					dprintf("failed to set keypad tone level! error: %i\n", message[4]);
				break;
			case 0x02:
				if (message[4] == 0x00)
					dprintf("call alert successfully set!\n");
				else
					dprintf("failed to set call alert! error: %i\n", message[4]);
				break;
			case 0x03:
				if (message[4] == 0x00)
					dprintf("ringtone successfully set!\n");
				else
					dprintf("failed to set ringtone! error: %i\n", message[4]);
				break;
			case 0x04:
				if (message[4] == 0x00)
					dprintf("ringtone volume successfully set!\n");
				else
					dprintf("failed to set ringtone volume! error: %i\n", message[4]);
				break;
			case 0x05:
				if (message[4] == 0x00)
					dprintf("message tone successfully set!\n");
				else
					dprintf("failed to set message tone! error: %i\n", message[4]);
				break;
			case 0x06:
				if (message[4] == 0x00)
					dprintf("vibration successfully set!\n");
				else
					dprintf("failed to set vibration! error: %i\n", message[4]);
				break;
			case 0x07:
				if (message[4] == 0x00)
					dprintf("warning tone level successfully set!\n");
				else
					dprintf("failed to set warning tone level! error: %i\n", message[4]);
				break;
			case 0x08:
				if (message[4] == 0x00)
					dprintf("caller groups successfully set!\n");
				else
					dprintf("failed to set caller groups! error: %i\n", message[4]);
				break;
			case 0x0c:
				if (message[4] == 0x00)
					dprintf("name successfully set!\n");
				else
					dprintf("failed to set name! error: %i\n", message[4]);
				break;
			default:
				dprintf("Unknown profile subblock type %02x!\n", blockstart[1]);
				break;
			}
			blockstart = blockstart + blockstart[1];
		}
		return GN_ERR_NONE;
		break;
	default:
		dprintf("Unknown subtype of type 0x39 (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
}

static gn_error NK6510_GetProfile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[150] = {FBUS_FRAME_HEADER, 0x01, 0x01, 0x0C, 0x01};
	int i, length = 7;

	for (i = 0; i < 0x0a; i++) {
		req[length] = 0x04;
		req[length + 1] = data->profile->number;
		req[length + 2] = i;
		req[length + 3] = 0x01;
		length += 4;
	}

	req[length] = 0x04;
	req[length + 1] = data->profile->number;
	req[length + 2] = 0x0c;
	req[length + 3] = 0x01;
	length += 4;

	req[length] = 0x04;

	dprintf("Getting profile #%i...\n", data->profile->number);
	NK6510_GetRingtoneList(data, state);
	SEND_MESSAGE_BLOCK(NK6510_MSG_PROFILE, length + 1);
}

static gn_error NK6510_SetProfile(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[150] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x06, 0x03};
	int i, j, blocks = 0, length = 7;

	for (i = 0; i < 0x07; i++) {
		switch (i) {
		case 0x00:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->profile->number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			switch (data->profile->keypad_tone) {
			case 0xff:
				req[length + 3] = req[length + 7] = 0x00;
				break;
			case 0x00:
				req[length + 3] = req[length + 7] = 0x01;
				break;
			case 0x01:
				req[length + 3] = req[length + 7] = 0x02;
				break;
			case 0x02:
				req[length + 3] = req[length + 7] = 0x03;
				break;
			default:
				dprintf("Unknown keypad tone volume!\n");
				break;
			}
			blocks++;
			length += 9;
			break;
		case 0x02:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->profile->number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->profile->call_alert;
			blocks++;
			length += 9;
			break;
		case 0x03:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->profile->number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->profile->ringtone;
			blocks++;
			length += 9;
			break;
		case 0x04:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->profile->number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->profile->volume - 6;
			blocks++;
			length += 9;
			break;
		case 0x05:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->profile->number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->profile->message_tone;
			blocks++;
			length += 9;
			break;
		case 0x06:
			req[length] = 0x09;
			req[length + 1] = i;
			req[length + 2] = data->profile->number;
			memcpy(req + length + 4, "\x00\x00\x01", 3);
			req[length + 8] = 0x03;
			req[length + 3] = req[length + 7] = data->profile->vibration;
			blocks++;
			length += 9;
			break;
		}
	}

	req[length + 1] = 0x0c;
	req[length + 2] = data->profile->number;
	memcpy(req + length + 3, "\xcc\xad\xff", 3);
	/* Name */
	j = strlen(data->profile->name);
	j = char_unicode_encode((req + length + 7), data->profile->name, j);
	/* Terminating 0 */
	req[j + 1] = 0;
	/* Length of the string + length field + terminating 0 */
	req[length + 6] = j + 1;
	req[length] = j + 9;
	length += j + 9;
	blocks++;

	req[length] = 0x09;
	req[length + 1] = 0x07;
	req[length + 2] = data->profile->number;
	memcpy(req + length + 4, "\x00\x00\x01", 3);
	req[length + 3] = req[length + 7] = data->profile->warning_tone;
	length += 8;
	blocks++;

	req[5] = blocks;
	dprintf("length: %i\n", length);
	SEND_MESSAGE_BLOCK(NK6510_MSG_PROFILE, length + 1);
}

/*************/
/*** RADIO  **/
/*************/

static gn_error NK6510_IncomingRadio(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/*
00 01 00 0D 00 00
00 0E 00 03 01 00 00 1C 00 14 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 01 00 00 08 00 00 01 00 01 00 00 0C 00 01 02 00 FF 30 30 30

00 01 00 05 00 00 00 2C 00 02 00 00 00 01 00 00
01 1F 00 06 00 00 00 2C 00 02 00 00 00 01 00 00 00 00 01

00 01 00 05 00 00 00 2C 00 01 00 00 00 2A 00 A8
01 1F 00 06 00 00 00 2C 00 01 00 00 00 2A 00 A8 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

00 01 00 05 00 00 00 2C 00 00 00 00 00 CA 03 28
01 1F 00 06 00 00 00 2C 00 00 00 00 00 CA 03 28 00 00 00 many more 00

00 01 00 05 00 00 00 2C 00 02 00 00 00 01 00 00

	 */

	switch (message[3]) {
	case 0x02:
		/* Hard reset ok */
		return GN_ERR_NONE;
		break;
	default:
		break;
	}

	return GN_ERR_NOTIMPLEMENTED;
}

/*****************/
/*** KEYPRESS  ***/
/*****************/

static gn_error NK6510_IncomingKeypress(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	case 0x12:
		if (length != 6 || message[5] != 0x00) return GN_ERR_UNHANDLEDFRAME;
		if (message[4] == 0x00) break;
		if (message[4] == 0x01) return GN_ERR_UNKNOWN;
		return GN_ERR_UNHANDLEDFRAME;
		break;

	default:
		dprintf("Unknown subtype of type 0x3c (%d)\n", message[4]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}

	return GN_ERR_NONE;
}

static gn_error NK6510_PressOrReleaseKey(gn_data *data, struct gn_statemachine *state, bool press)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01};

	req[6] = press ? 0x00 : 0x01;
	switch (data->key_code) {
	case GN_KEY_UP:   req[8] = 0x01; break;
	case GN_KEY_DOWN: req[8] = 0x02; break;
	default: return GN_ERR_NOTSUPPORTED;
	}

	SEND_MESSAGE_BLOCK(NK6510_MSG_KEYPRESS, 10);
}

#ifdef  SECURITY
/*****************/
/*** SECURITY  ***/
/*****************/

static gn_error NK6510_IncomingSecurity(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	/*
	  01 4e 00 12 05 12 02 00 00 00
	  01 4e 00 09 06 00
	 */
	switch (message[3]) {
	case 0x08:
		dprintf("Security Code OK!\n");
		break;
	case 0x09:
		switch (message[4]) {
		case 0x06:
			dprintf("PIN wrong!\n");
			break;
		case 0x09:
			dprintf("PUK wrong!\n");
			break;
		default:
			dprintf(" unknown security Code wrong!\n");
			break;
		}
		return GN_ERR_INVALIDSECURITYCODE;
		break;
	case 0x12:
		dprintf("Security Code status received: ");
		if (!data->security_code) return GN_ERR_INTERNALERROR;
		switch (message[4]) {
		case 0x01:
			dprintf("waiting for Security Code.\n");
			data->security_code->type = GN_SCT_SecurityCode;
			break;
		case 0x07:
		case 0x02:
			dprintf("waiting for PIN.\n");
			data->security_code->type = GN_SCT_Pin;
			break;
		case 0x03:
			dprintf("waiting for PUK.\n");
			data->security_code->type = GN_SCT_Puk;
			break;
		case 0x05:
			dprintf("PIN ok, SIM ok\n");
			data->security_code->type = GN_SCT_None;
			break;
		case 0x06:
			dprintf("No input status\n");
			data->security_code->type = GN_SCT_None;
			break;
		case 0x16:
			dprintf("No SIM!\n");
			data->security_code->type = GN_SCT_None;
			break;
		case 0x1A:
			dprintf("SIM rejected!\n");
			data->security_code->type = GN_SCT_None;
			break;
		default:
			dprintf("Unknown!\n");
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	default:
		dprintf("Unknown subtype of type 0x08 (%d)\n", message[3]);
		return GN_ERR_NONE;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x11, 0x00};

	if (!data->security_code) return GN_ERR_INTERNALERROR;

	SEND_MESSAGE_BLOCK(NK6510_MSG_SECURITY, 5);
}

static gn_error NK6510_EnterSecurityCode(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[35] = {FBUS_FRAME_HEADER, 0x07,
				 0x02 }; /* 0x02 PIN, 0x03 PUK */
	int len ;

	if (!data->security_code) return GN_ERR_INTERNALERROR;
	switch (data->security_code->type) {
	case GN_SCT_SecurityCode:
		req[4] = 0x01;
		break;
	case GN_SCT_Pin:
		req[4] = 0x02;
		break;
	default:
		dprintf("Unhandled security code type %d\n", data->security_code->type);
		return GN_ERR_NOTSUPPORTED;
	}

	len = strlen(data->security_code->code);
	memcpy(req + 5, data->security_code->code, len);
	req[5 + len] = '\0';

	SEND_MESSAGE_BLOCK(NK6510_MSG_SECURITY, 6 + len);
}
#endif

/*****************/
/*** SUBSCRIBE ***/
/*****************/

static gn_error NK6510_IncomingSubscribe(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	default:
		dprintf("Unknown subtype of type 0x3c (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
	}
	return GN_ERR_NONE;
}

/* FIXME: make it configurable */
static gn_error NK6510_Subscribe(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[100] = {FBUS_FRAME_HEADER, 0x10,
			       0x06, /* number of groups */
			       NK6510_MSG_COMMSTATUS, NK6510_MSG_SMS,
			       NK6510_MSG_NETSTATUS, NK6510_MSG_FOLDER,
			       NK6510_MSG_RESET, NK6510_MSG_BATTERY};

	dprintf("Subscribing to various channels!\n");
	if (sm_message_send(11, NK6510_MSG_SUBSCRIBE, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_ack(state);
}

/*************/
/*** RESET ***/
/*************/

static gn_error NK6510_IncomingReset(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	default:
		dprintf("Incoming Reset\n");
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error NK6510_Reset(gn_data *data, struct gn_statemachine *state)
{
       unsigned char req[] = {FBUS_FRAME_HEADER,0x05,0x80,0x00};
	unsigned char req2[] = {FBUS_FRAME_HEADER,0x01,0,0,0,0,0,0x01};

	if (data->reset_type == 0x03) {
		printf("Soft resetting....\n");
		SEND_MESSAGE_BLOCK(NK6510_MSG_RESET, 6);
	}
	else if (data->reset_type == 0x04) {
		printf("Hard resetting....\n");
		if (sm_message_send(10, NK6510_MSG_RADIO, req2, state)) return GN_ERR_NOTREADY;
		return sm_block(NK6510_MSG_RADIO, data, state);
	} else return GN_ERR_INTERNALERROR;
}

/******************/
/*** COMMSTATUS ***/
/******************/

static gn_call_status NK6510_GetCallStatus_S40_30(unsigned char state)
{
	switch (state) {
	case 1: return GN_CALL_Held;
	case 3: return GN_CALL_Incoming;
	case 4: return GN_CALL_Dialing;
	case 5: return GN_CALL_Ringing;
	default:
		dprintf("Unknown call status %d\n", state);
		return GN_CALL_Idle;
	}
}

static gn_error NK6510_IncomingCommStatus(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	unsigned char *pos;
	int i;
	gn_call_active *ca;
	gn_call_info cinfo;

	/* Please note that most of the meaning is wild guess */
	switch (message[3]) {
	case 0x02: /* Call estabilished */
		dprintf("Call estabilished\n");
		break;

	case 0x03: /* Call status */
		/* fall through */
	case 0x0f: /* Call status */
		dprintf("Call status 0x%02x\n", message[3]);
		memset(&cinfo, 0, sizeof(gn_call_info));
		cinfo.type = GN_CALL_Voice; /* FIXME how to actually detect type? */
		cinfo.call_id = message[4];
		for (i = 6; i < length && message[i]; i += message[i + 1]) {
			switch (message[i]) {
			case 0x01: /* Phone number of incoming call */
			case 0x03: /* Phone number of outgoing call */
				/* message[i + 2] is type of number? 0x10 international? 0x01 network specific? */
				char_unicode_decode(cinfo.number, message + i + 6, 2 * message[i + 5]);
				break;
			case 0x0e:
				char_unicode_decode(cinfo.name, message + i + 8, 2 * message[i + 7]);
				break;
			default:
				dprintf("  Unknown call block type 0x%02x length %d\n",  message[i], message[i + 1]);
			}
		}
		if (DRVINSTANCE(state)->call_notification)
			DRVINSTANCE(state)->call_notification(NK6510_GetCallStatus_S40_30(message[5]), &cinfo, state, DRVINSTANCE(state)->call_callback_data);
		break;

	case 0x04: /* Call hangup */
		dprintf("Call hangup (remote)\n");
		dprintf("Call ID: %i\n", message[4]);
		dprintf("Cause Type: %i\n", message[5]);
		dprintf("Cause ID: %i\n", message[6]);
		break;

	case 0x05: /* Incoming call */
		dprintf("Incoming call\n");
		break;

	case 0x07: /* Call answer initiated */
		dprintf("Call answer initiated\n");
		break;

	case 0x09: /* Call hangup */
		dprintf("Call hangup (local)\n");
		break;

	case 0x0a: /* Hanguping */
		dprintf("Hanguping call (locally)\n");
		break;

	case 0x0c: /* Dialling */
		dprintf("Dialling\n");
		break;

	case 0x10: /* Indicator whether the make call request was successful (?) */
		switch (message[4]) {
		case 0x00:
			dprintf("Make call succeeded.\n");
			break;
		/* Some of dct4 phones respond with this to NK6510_MakeCall()
		 * frames. Let's repond with the error code to retry with
		 * NK6510_MakeCall2(). The exact meaning is not known.
		 */
		case 0x01:
			dprintf("Make call failed.\n");
			error = GN_ERR_NOTSUPPORTED;
			break;
		}
		break;

	case 0x21: /* Call status */
		if (!data->call_active) {
			error = GN_ERR_INTERNALERROR;
			break;
		}
		if (message[5] != 0xff) {
			error = GN_ERR_UNHANDLEDFRAME;
			break;
		}
		pos = message + 6;
		ca = data->call_active;
		memset(ca, 0x00, 2 * sizeof(gn_call_active));
		for (i = 0; i < message[4]; i++) {
			if (pos[0] != 0x64) {
				error = GN_ERR_UNHANDLEDFRAME;
				goto out;
			}
			ca[i].call_id = pos[2];
			ca[i].channel = pos[3];
			switch (pos[4]) {
			case 0x00:
				ca[i].state = GN_CALL_Idle;
				break; /* missing number, wait a little */
			case 0x02:
				ca[i].state = GN_CALL_Dialing;
				break;
			case 0x03:
				ca[i].state = GN_CALL_Ringing;
				break;
			case 0x04:
				ca[i].state = GN_CALL_Incoming;
				break;
			case 0x05:
				ca[i].state = GN_CALL_Established;
				break;
			case 0x06:
				ca[i].state = GN_CALL_Held;
				break;
			case 0x07:
				ca[i].state = GN_CALL_RemoteHangup;
				break;
			default:
				dprintf("Unknown call state in frame: %d\n", pos[4]);
				error = GN_ERR_UNHANDLEDFRAME;
				goto out;
			}
			switch (pos[5]) {
			case 0x00:
				ca[i].prev_state = GN_CALL_Idle;
				break; /* missing number, wait a little */
			case 0x02:
				ca[i].prev_state = GN_CALL_Dialing;
				break;
			case 0x03:
				ca[i].prev_state = GN_CALL_Ringing;
				break;
			case 0x04:
				ca[i].prev_state = GN_CALL_Incoming;
				break;
			case 0x05:
				ca[i].prev_state = GN_CALL_Established;
				break;
			case 0x06:
				ca[i].prev_state = GN_CALL_Held;
				break;
			case 0x07:
				ca[i].prev_state = GN_CALL_RemoteHangup;
				break;
			default:
				dprintf("Unknown previous call state in frame: %d\n", pos[5]);
				error = GN_ERR_UNHANDLEDFRAME;
				goto out;
			}
			char_unicode_decode(ca[i].name, pos + 12, 2 * pos[10]);
			char_unicode_decode(ca[i].number, pos + 112, 2 * pos[11]);
			pos += pos[1];
		}
		dprintf("Call status:\n");
		for (i = 0; i < 2; i++) {
			if (ca[i].state == GN_CALL_Idle)
				continue;
			dprintf("ch#%d: id#%d st#%d pst#%d %s (%s)\n",
				ca[i].channel, ca[i].call_id, ca[i].state, ca[i].prev_state, ca[i].number, ca[i].name);
		}
		break;

	case 0x23: /* Call on hold */
		dprintf("Call on hold\n");
		break;

	case 0x25: /* Call resumed */
		dprintf("Call resumed\n");
		break;

	case 0x27: /* Call switched */
		dprintf("Call switched\n");
		break;

	case 0x53: /* Outgoing call */
		dprintf("Outgoing call\n");
		break;

	case 0x32:
	case 0xd2:
		dprintf("Unknown\n");
		break;

	case 0xf0:
		error = GN_ERR_NOTSUPPORTED;
		break;

	default:
		dprintf("Unknown subtype of type 0x01 (0x%02x)\n", message[3]);
		error = GN_ERR_UNHANDLEDFRAME;
		break;
	}
out:
	return error;
}

static gn_error NK6510_GetActiveCalls(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x20};

	if (!data->call_active)
		return GN_ERR_INTERNALERROR;

	SEND_MESSAGE_BLOCK(NK6510_MSG_COMMSTATUS, 4);
}

static gn_error NK6510_MakeCall2(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01,
				0x00, 0x02, 0x07, 0x04,
				0x01, /* 0x01 voice, 0x02 data */
				0x00, 0x03,
				0x00, /* Length of the block*/
				0x00, 0x00, 0x00};
				/* Next fields are number length and number itself */
	int len;

	if (!data->call_info) {
		error = GN_ERR_INTERNALERROR;
		goto out;
	}

	len = strlen(data->call_info->number);
	if (len > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		dprintf("number too long\n");
		error = GN_ERR_ENTRYTOOLONG;
		goto out;
	}
	len = char_unicode_encode(req + 16, data->call_info->number, len);
	req[11] = len + 6;
	req[15] = len / 2;

	if (sm_message_send(16 + len, NK6510_MSG_COMMSTATUS, req, state)) {
		error = GN_ERR_NOTREADY;
		goto out;
	}
	error = sm_block(NK6510_MSG_COMMSTATUS, data, state);

out:
	return error;
}

static gn_error NK6510_MakeCall(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_NONE;
	unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01};
	unsigned char voice_end[] = {0x05, 0x01, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00};
	int pos = 4, len;
	gn_call_active active[2];
	gn_data d;

	if (!data->call_info) {
		error = GN_ERR_INTERNALERROR;
		goto out;
	}

	/* We need to subscribe to the call channel */
	error = NK6510_Subscribe(data, state);
	if (error != GN_ERR_NONE)
		goto out;

	switch (data->call_info->type) {
	case GN_CALL_Voice:
		break;

	case GN_CALL_NonDigitalData:
	case GN_CALL_DigitalData:
		dprintf("Unsupported call type %d\n", data->call_info->type);
		error = GN_ERR_NOTSUPPORTED;
		goto out;

	default:
		dprintf("Invalid call type %d\n", data->call_info->type);
		error = GN_ERR_INTERNALERROR;
		goto out;
	}

	len = strlen(data->call_info->number);
	if (len > GN_PHONEBOOK_NUMBER_MAX_LENGTH) {
		dprintf("number too long\n");
		error = GN_ERR_ENTRYTOOLONG;
		goto out;
	}
	len = char_unicode_encode(req + pos + 1, data->call_info->number, len);
	req[pos++] = len / 2;
	pos += len;

	switch (data->call_info->send_number) {
	case GN_CALL_Never:
		voice_end[5] = 0x01;
		break;
	case GN_CALL_Always:
		voice_end[5] = 0x00;
		break;
	case GN_CALL_Default:
		voice_end[5] = 0x00;
		break;
	default:
		error = GN_ERR_INTERNALERROR;
		goto out;
	}
	memcpy(req + pos, voice_end, sizeof(voice_end));
	pos += sizeof(voice_end);

	/* Reported by kam on IRC channel. Nokia 7600 does not work with
	 * the above frame. Return GN_ERR_NOTSUPPORTED to retry with
	 * another frame.
	 * FIXME: do the per-model configuration which frame should
	 * be used. */
	if (sm_message_send(pos, NK6510_MSG_COMMSTATUS, req, state)) {
		error = GN_ERR_NOTREADY;
		goto out;
	}
	error = sm_block(NK6510_MSG_COMMSTATUS, data, state);

	if (error == GN_ERR_NOTSUPPORTED)
		error = NK6510_MakeCall2(data, state);

	if (error != GN_ERR_NONE) {
		goto out;
	}

	/* Get active calls detais */
	memset(active, 0, sizeof(*active));
	gn_data_clear(&d);
	d.call_active = active;
	error = NK6510_GetActiveCalls(&d, state);
	if (error != GN_ERR_NONE)
		goto out;

	data->call_info->call_id = active[0].call_id;

out:
	return error;
}

static gn_error NK6510_CancelCall(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x08, 0x00};

	if (!data->call_info)
		return GN_ERR_INTERNALERROR;

	req[4] = data->call_info->call_id;

	if (sm_message_send(5, NK6510_MSG_COMMSTATUS, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_ack(state);
}

static gn_error NK6510_AnswerCall(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x06, 0x00};

	if (!data->call_info)
		return GN_ERR_INTERNALERROR;

	req[4] = data->call_info->call_id;

	if (sm_message_send(5, NK6510_MSG_COMMSTATUS, req, state))
		return GN_ERR_NOTREADY;
	return sm_block_ack(state);
}

/*****************/
/****** WAP ******/
/*****************/

static gn_error NK6510_IncomingWAP(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int string_length, pos, pad = 0;

	switch (message[3]) {
	case 0x02:
	case 0x05:
	case 0x08:
	case 0x0b:
	case 0x0e:
	case 0x11:
	case 0x14:
	case 0x17:
	case 0x1a:
		switch (message[4]) {
		case 0x00:
			dprintf("WAP not activated?\n");
			return GN_ERR_UNKNOWN;
		case 0x01:
			dprintf("Security error. Inside WAP bookmarks menu\n");
			return GN_ERR_UNKNOWN;
		case 0x02:
			dprintf("Invalid or empty\n");
			return GN_ERR_INVALIDLOCATION;
		default:
			dprintf("ERROR: unknown %i\n",message[4]);
			return GN_ERR_UNHANDLEDFRAME;
		}
		break;
	case 0x01:
	case 0x04:
	case 0x10:
		break;
	case 0x07:
		if (!data->wap_bookmark) return GN_ERR_INTERNALERROR;
		dprintf("WAP bookmark received\n");
		string_length = (message[6] << 8 | message[7]) << 1;

		char_unicode_decode(data->wap_bookmark->name, message + 8, string_length);
		dprintf("Name: %s\n", data->wap_bookmark->name);
		pos = string_length + 8;

		string_length = message[pos++] << 8;
		string_length |= message[pos++];
		char_unicode_decode(data->wap_bookmark->URL, message + pos, string_length << 1);
		dprintf("URL: %s\n", data->wap_bookmark->URL);
		break;
	case 0x0a:
		dprintf("WAP bookmark successfully set!\n");
		break;
	case 0x0d:
		dprintf("WAP bookmark successfully deleted!\n");
		break;
	case 0x13:
		dprintf("WAP setting successfully activated!\n");
		break;
	case 0x16:
		if (!data->wap_setting) return GN_ERR_INTERNALERROR;
		dprintf("WAP setting received\n");

		string_length = (message[4] << 8 | message[5]) << 1;
		char_unicode_decode(data->wap_setting->name, message + 6, string_length);
		dprintf("Name: %s\n", data->wap_setting->name);
		pos = string_length + 6;
		if (!(string_length % 2)) pad = 1;

		string_length = message[pos++] << 8;
		string_length = (string_length | message[pos++]) << 1;
		char_unicode_decode(data->wap_setting->home, message + pos, string_length);
		dprintf("Home: %s\n", data->wap_setting->home);
		pos += string_length;

		if (!((string_length + pad) % 2))
			pad = 2;
		else
			pad = 0;

		data->wap_setting->session = message[pos++];
		if (message[pos++] == 0x01)
			data->wap_setting->security = true;
		else
			data->wap_setting->security = false;
		data->wap_setting->bearer = message[pos++];

		while ((message[pos] != 0x01) || (message[pos + 1] != 0x00)) pos++;
		pos += 4;

		data->wap_setting->gsm_data_authentication = message[pos++];
		data->wap_setting->call_type = message[pos++];
		pos++;
		data->wap_setting->call_speed = message[pos++];
		data->wap_setting->gsm_data_login = message[pos++];

		dprintf("GSM data:\n");

		string_length = message[pos++] << 1;
		char_unicode_decode(data->wap_setting->gsm_data_ip, message + pos, string_length);
		dprintf("   IP: %s\n", data->wap_setting->gsm_data_ip);
		pos += string_length;

		string_length = message[pos++] << 8;
		string_length = (string_length | message[pos++]) << 1;
		char_unicode_decode(data->wap_setting->number, message + pos, string_length);
		dprintf("   Number: %s\n", data->wap_setting->number);
		pos += string_length;

		string_length = message[pos++] << 8;
		string_length = (string_length | message[pos++]) << 1;
		char_unicode_decode(data->wap_setting->gsm_data_username, message + pos, string_length);
		dprintf("   Username: %s\n", data->wap_setting->gsm_data_username);
		pos += string_length;

		string_length = message[pos++] << 8;
		string_length = (string_length | message[pos++]) << 1;
		char_unicode_decode(data->wap_setting->gsm_data_password, message + pos, string_length);
		dprintf("   Password: %s\n", data->wap_setting->gsm_data_password);
		pos += string_length;

		while (message[pos] != 0x03) pos++;
		pos += 4;

		dprintf("GPRS:\n");

		data->wap_setting->gprs_authentication = message[pos++];
		data->wap_setting->gprs_connection = message[pos++];
		data->wap_setting->gprs_login = message[pos++];

		string_length = message[pos++] << 1;
		char_unicode_decode(data->wap_setting->access_point_name, message + pos, string_length);
		dprintf("   Access point: %s\n", data->wap_setting->access_point_name);
		pos += string_length;

		string_length = message[pos++] << 8;
		string_length = (string_length | message[pos++]) << 1;
		char_unicode_decode(data->wap_setting->gprs_ip, message + pos, string_length);
		dprintf("   IP: %s\n", data->wap_setting->gprs_ip);
		pos += string_length;

		string_length = message[pos++] << 8;
		string_length = (string_length | message[pos++]) << 1;
		char_unicode_decode(data->wap_setting->gprs_username, message + pos, string_length);
		dprintf("   Username: %s\n", data->wap_setting->gprs_username);
		pos += string_length;

		if (message[pos] != 0x80) {
			string_length = message[pos++] << 8;
			string_length |= message[pos++];
			char_unicode_decode(data->wap_setting->gprs_password, message + pos, string_length);
			dprintf("   Password: %s\n", data->wap_setting->gprs_password);
			pos += string_length;
		} else
			pos += 2;

		break;
	case 0x19:
		dprintf("WAP setting successfully written!\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x3f (%d)\n", message[3]);
		return GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return GN_ERR_NONE;
}

static gn_error SendWAPFrame(gn_data *data, struct gn_statemachine *state, int frame)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x00 };

	dprintf("Sending WAP frame\n");
	req[3] = frame;
	SEND_MESSAGE_BLOCK(NK6510_MSG_WAP, 4);
}

static gn_error PrepareWAP(gn_data *data, struct gn_statemachine *state)
{
	dprintf("Preparing WAP\n");
	return SendWAPFrame(data, state, 0x00);
}

static gn_error FinishWAP(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;

	dprintf("Finishing WAP\n");

	error = SendWAPFrame(data, state, 0x03);
	if (error != GN_ERR_NONE) return error;

	error = SendWAPFrame(data, state, 0x00);
	if (error != GN_ERR_NONE) return error;

	error = SendWAPFrame(data, state, 0x0f);
	if (error != GN_ERR_NONE) return error;

	return SendWAPFrame(data, state, 0x03);
}

static int PackWAPString(unsigned char *dest, unsigned char *string, int length_size)
{
	int length;

	length = strlen(string);
	if (length_size == 2) {
		dest[0] = length / 256;
		dest[1] = length % 256;
	} else {
		dest[0] = length % 256;
	}

	length = char_unicode_encode(dest + length_size, string, length);
	return length + length_size;
}

static gn_error NK6510_WriteWAPSetting(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[1000] = { FBUS_FRAME_HEADER, 0x18,
				    0x00 }; 		/* Location */
	gn_error error;
	int pad = 0, length, pos = 5;

	dprintf("Writing WAP setting\n");
	memset(req + pos, 0, 1000 - pos);
	req[4] = data->wap_setting->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		SendWAPFrame(data, state, 0x03);
		if ((error = PrepareWAP(data, state)) != GN_ERR_NONE) return error;
	}

	/* Name */
	length = strlen(data->wap_setting->name);
	if (!(length % 2)) pad = 1;
	pos += PackWAPString(req + pos, data->wap_setting->name, 1);

	/* Home */
	length = strlen(data->wap_setting->home);
	if (((length + pad) % 2))
		pad = 2;
	else
		pad = 0;
	pos += PackWAPString(req + pos, data->wap_setting->home, 2);

	req[pos++] = data->wap_setting->session;
	pos++;
	if (data->wap_setting->security)	req[pos] = 0x01;
	req[pos++] = data->wap_setting->bearer;

	/* How many parts do we send? */
	req[pos++] = 0x02;
	pos += pad;

	/* GSM data */
	memcpy(req + pos, "\x01\x00", 2);
	pos += 2;
	length = ((strlen(data->wap_setting->gsm_data_ip) + strlen(data->wap_setting->gsm_data_username)
		   + strlen(data->wap_setting->gsm_data_password) + strlen(data->wap_setting->number)) << 1) + 18;
	req[pos++] = length / 256;
	req[pos++] = length % 256;

	req[pos++] = data->wap_setting->gsm_data_authentication;
	req[pos++] = data->wap_setting->call_type;
	pos++;
	req[pos++] = data->wap_setting->call_speed;
	req[pos++] = data->wap_setting->gsm_data_login;

	/* IP */
	pos += PackWAPString(req + pos, data->wap_setting->gsm_data_ip, 1);
	/* Number */
	pos += PackWAPString(req + pos, data->wap_setting->number, 2);
	/* Username */
	pos += PackWAPString(req + pos, data->wap_setting->gsm_data_username, 2);
	/* Password */
	pos += PackWAPString(req + pos, data->wap_setting->gsm_data_password, 2);

	/* Padding */
	if (length % 2) pos++;

	/* GPRS block */
	memcpy(req + pos, "\x03\x00", 2);
	pos += 2;
	length = ((strlen(data->wap_setting->gprs_ip) + strlen(data->wap_setting->gprs_username)
		   + strlen(data->wap_setting->gprs_password) + strlen(data->wap_setting->access_point_name)) << 1) + 14;
	req[pos++] = length / 256;
	req[pos++] = length % 256;

	req[pos++] = 0x00; /* data->wap_setting->gprs_authentication; */
	req[pos++] = data->wap_setting->gprs_connection;
	req[pos++] = data->wap_setting->gprs_login;

	/* Access point */
	pos += PackWAPString(req + pos, data->wap_setting->access_point_name, 1);
	/* IP */
	pos += PackWAPString(req + pos, data->wap_setting->gprs_ip, 2);
	/* Username */
	pos += PackWAPString(req + pos, data->wap_setting->gprs_username, 2);
	/* Password */
	pos += PackWAPString(req + pos, data->wap_setting->gprs_password, 2);

	/* end of block ? */
	memcpy(req + pos, "\x80\x00", 2);
	pos += 2;

	/* no idea what this is about, seems to be some kind of 'end of message' */
	memcpy(req + pos, "\x00\x0c\x07\x6B\x0C\x1E\x00\x00\x00\x55", 10);
	pos += 10;

	if (sm_message_send(pos, NK6510_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK6510_DeleteWAPBookmark(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x0C,
				0x00, 0x00};		/* Location */
	gn_error error;

	dprintf("Deleting WAP bookmark\n");
	req[5] = data->wap_bookmark->location + 1;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(6, NK6510_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK6510_GetWAPBookmark(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x06,
				0x00, 0x00};		/* Location */
	gn_error error;

	dprintf("Getting WAP bookmark\n");
	req[5] = data->wap_bookmark->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(6, NK6510_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK6510_WriteWAPBookmark(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[350] = { FBUS_FRAME_HEADER, 0x09,
				0xFF, 0xFF};		/* Location */
	gn_error error;
	int pos = 6;

	dprintf("Writing WAP bookmark\n");

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	pos += PackWAPString(req + pos, data->wap_bookmark->name, 2);
	pos += PackWAPString(req + pos, data->wap_bookmark->URL, 2);

	if (sm_message_send(pos, NK6510_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK6510_GetWAPSetting(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x15,
				0x00 };		/* Location */
	gn_error error;

	dprintf("Getting WAP setting\n");
	req[4] = data->wap_setting->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(5, NK6510_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

static gn_error NK6510_ActivateWAPSetting(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = { FBUS_FRAME_HEADER, 0x12,
				0x00 };		/* Location */
	gn_error error;

	dprintf("Activating WAP setting\n");
	req[4] = data->wap_setting->location;

	if (PrepareWAP(data, state) != GN_ERR_NONE) {
		FinishWAP(data, state);
		if ((error = PrepareWAP(data, state))) return error;
	}

	if (sm_message_send(5, NK6510_MSG_WAP, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_WAP, data, state);
	if (error) return error;

	return FinishWAP(data, state);
}

/********************/
/***** ToDo *********/
/********************/

static gn_error NK6510_GetToDo_Internal(gn_data *data, struct gn_statemachine *state, int location)
{
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03,
			       0x00, 0x00, 0x80, 0x00,
			       0x00, 0x01};		/* Location */

	req[8] = location / 256;
	req[9] = location % 256;
	dprintf("Getting ToDo\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_TODO, 10);
}

static gn_error NK6510_IncomingToDo(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	int i;
	gn_error error = GN_ERR_NONE;

	switch (message[3]) {
	case 0x02:
		if (!data->todo) {
			error = GN_ERR_INTERNALERROR;
			break;
		}
		switch (message[4]) {
		case 0x00:
			dprintf("ToDo set!\n");
			data->todo->location = message[8] * 256 + message[9];
			break;
		case 0x04:
			dprintf("Invalid priority?\n");
		default:
			dprintf("ToDo setting failed\n");
			error = GN_ERR_FAILED;
		}
		break;
	case 0x04:
		dprintf("ToDo received!\n");
		if (!data->todo) {
			error = GN_ERR_INTERNALERROR;
			break;
		}
		if (message[5] == 0x08) {
			error = GN_ERR_INVALIDLOCATION;
			break;
		}
		if ((message[4] > 0) && (message[4] < 4)) {
			data->todo->priority = message[4];
		}
		dprintf("Priority: %i\n", message[4]);
		char_unicode_decode(data->todo->text, message + 14, length - 16);
		dprintf("Text: \"%s\"\n", data->todo->text);
		break;
	case 0x10:
		dprintf("Next free ToDo location received!\n");
		if (!data->todo) {
			error = GN_ERR_INTERNALERROR;
			break;
		}
		data->todo->location = message[8] * 256 + message[9];
		dprintf("   location: %i\n", data->todo->location);
		break;
	case 0x12:
		dprintf("All ToDo locations deleted!\n");
		break;
	case 0x16:
		dprintf("ToDo locations received!\n");
		if (!data->todo) {
			error = GN_ERR_INTERNALERROR;
			break;
		}
		data->todo_list->number = message[6] * 256 + message[7];
		dprintf("Number of Entries: %i\n", data->todo_list->number);

		dprintf("Locations: ");
		for (i = 0; i < data->todo_list->number; i++) {
			data->todo_list->location[i] = message[12 + (i * 4)] * 256 + message[(i * 4) + 13];
			dprintf("%i ", data->todo_list->location[i]);
		}
		dprintf("\n");
		break;
	default:
		dprintf("Unknown subtype of type 0x01 (%d)\n", message[3]);
		error = GN_ERR_UNHANDLEDFRAME;
		break;
	}
	return error;
}

static gn_error NK6510_GetToDoLocations(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x15,
				0x01, 0x00, 0x00,
				0x00, 0x00, 0x00};

	SEND_MESSAGE_BLOCK(NK6510_MSG_TODO, 10);
}

static gn_error NK6510_DeleteAllToDoLocations(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x11 };

	SEND_MESSAGE_BLOCK(NK6510_MSG_TODO, 4);
}

static gn_error GetNextFreeToDoLocation(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[] = {	FBUS_FRAME_HEADER, 0x0f };

	SEND_MESSAGE_BLOCK(NK6510_MSG_TODO, 4);
}

static gn_error NK6510_GetToDo(gn_data *data, struct gn_statemachine *state)
{
#if 0
	unsigned char req[] = {FBUS_FRAME_HEADER, 0x03,
			       0x00, 0x00, 0x80, 0x00,
			       0x00, 0x01};		/* Location */
#endif
	gn_error error;

	if (data->todo->location < 1) {
		error = GN_ERR_INVALIDLOCATION;
	} else {
		error = NK6510_GetToDoLocations(data, state);
		if (!data->todo_list->number ||
		    data->todo->location > data->todo_list->number) {
			error = GN_ERR_EMPTYLOCATION;
		} else {
			return NK6510_GetToDo_Internal(data, state, data->todo_list->location[data->todo->location - 1]);
		}
	}
	return error;
}

static gn_error NK6510_WriteToDo(gn_data *data, struct gn_statemachine *state)
{
	unsigned char req[300] = {FBUS_FRAME_HEADER, 0x01,
				  0x02, /* prority */
				  0x0E, /* length of the text + 1 */
				  0x80, 0x00,
				  0x00, 0x01};	/* Location */
	unsigned char text[257];
	int length;
	gn_error error;

	error = GetNextFreeToDoLocation(data, state);
	if (error != GN_ERR_NONE) return error;

	length = char_unicode_encode(text, data->todo->text, strlen(data->todo->text));
	if (length > GN_TODO_MAX_LENGTH) return GN_ERR_ENTRYTOOLONG;

	/* priority */
	req[4] = data->todo->priority;

	/* length */
	req[5] = length + 1;

	/* location */
	req[8] = data->todo->location / 256;
	req[9] = data->todo->location % 256;

	/* text */
	memcpy(req + 10, text, length);
	/* 0-terminated string */
	req[10+length] = req[10+length+1] = 0;

	dprintf("Setting ToDo\n");
	if (sm_message_send(length + 12, NK6510_MSG_TODO, req, state)) return GN_ERR_NOTREADY;
	error = sm_block(NK6510_MSG_TODO, data, state);
	if (error == GN_ERR_NONE) {
		error = NK6510_GetToDo_Internal(data, state, data->todo->location);
	}
	return error;
}

/********************/
/****** SOUND *******/
/********************/

static gn_error NK6510_IncomingSound(int messagetype, unsigned char *message, int length, gn_data *data, struct gn_statemachine *state)
{
	switch (message[3]) {
	/* response to init1 - meaning unknown */
	case 0x01:
	case 0x02:
		break;

	/* buzzer on/off - meaning unknown */
	case 0x14:
	case 0x15:
	case 0x16:
		break;

	default:
		return GN_ERR_UNHANDLEDFRAME;
	}

	return GN_ERR_NONE;
}

static gn_error NK6510_PlayTone(gn_data *data, struct gn_statemachine *state)
{
	unsigned char init1[] = {0x00, 0x06, 0x01, 0x00, 0x07, 0x00};
	unsigned char init2[] = {0x00, 0x06, 0x01, 0x14, 0x05, 0x05, 0x00, 0x00,
				 0x00, 0x01, 0x03, 0x08, 0x05, 0x00, 0x00, 0x08,
				 0x00, 0x00};
	unsigned char req[] = {0x00, 0x06, 0x01, 0x14, 0x05, 0x04, 0x00, 0x00,
			       0x00, 0x03, 0x03, 0x08, 0x00, 0x00, 0x00, 0x01,
			       0x00, 0x00, 0x03, 0x08, 0x01, 0x00,
			       0x00, 0x00,	/* frequency */
			       0x00, 0x00, 0x03, 0x08, 0x02, 0x00, 0x00,
			       0x05,		/* volume */
			       0x00, 0x00};
	gn_error err;

	if (!data->tone) return GN_ERR_INTERNALERROR;

	if (sm_message_send(6, 0x0b, init1, state)) return GN_ERR_NOTREADY;
	if ((err = sm_block(NK6510_MSG_SOUND, data, state)) != GN_ERR_NONE) return err;
	if (sm_message_send(18, 0x0b, init2, state)) return GN_ERR_NOTREADY;
	if ((err = sm_block(NK6510_MSG_SOUND, data, state)) != GN_ERR_NONE) return err;

	req[31] = data->tone->volume;
	req[22] = data->tone->frequency / 256;
	req[23] = data->tone->frequency % 255;

	dprintf("Playing tone\n");
	SEND_MESSAGE_BLOCK(NK6510_MSG_SOUND, 34);
}

/********************************/
/* NOT FRAME SPECIFIC FUNCTIONS */
/********************************/

static gn_error NK6510_GetBitmap(gn_data *data, struct gn_statemachine *state)
{
	switch(data->bitmap->type) {
	case GN_BMP_WelcomeNoteText:
		return GetWelcomeNoteText(data, state);
	case GN_BMP_CallerLogo:
		return GetCallerBitmap(data, state);
	case GN_BMP_NewOperatorLogo:
	case GN_BMP_OperatorLogo:
		return GetOperatorBitmap(data, state);
	case GN_BMP_StartupLogo:
		return GetStartupBitmap(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static gn_error NK6510_SetBitmap(gn_data *data, struct gn_statemachine *state)
{
	switch(data->bitmap->type) {
	case GN_BMP_StartupLogo:
		return SetStartupBitmap(data, state);
	case GN_BMP_CallerLogo:
		return SetCallerBitmap(data, state);
	case GN_BMP_OperatorLogo:
	case GN_BMP_NewOperatorLogo:
		return SetOperatorBitmap(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
}

static int get_memory_type(gn_memory_type memory_type)
{
	int result;

	switch (memory_type) {
	case GN_MT_MT:
		result = NK6510_MEMORY_MT;
		break;
	case GN_MT_ME:
		result = NK6510_MEMORY_ME;
		break;
	case GN_MT_SM:
		result = NK6510_MEMORY_SM;
		break;
	case GN_MT_FD:
		result = NK6510_MEMORY_FD;
		break;
	case GN_MT_ON:
		result = NK6510_MEMORY_ON;
		break;
	case GN_MT_EN:
		result = NK6510_MEMORY_EN;
		break;
	case GN_MT_DC:
		result = NK6510_MEMORY_DC;
		break;
	case GN_MT_RC:
		result = NK6510_MEMORY_RC;
		break;
	case GN_MT_MC:
		result = NK6510_MEMORY_MC;
		break;
	case GN_MT_IN:
		result = NK6510_MEMORY_IN;
		break;
	case GN_MT_OU:
		result = NK6510_MEMORY_OU;
		break;
	case GN_MT_OUS:
		result = NK6510_MEMORY_OUS;
		break;
	case GN_MT_AR:
		result = NK6510_MEMORY_AR;
		break;
	case GN_MT_TE:
		result = NK6510_MEMORY_TE;
		break;
	case GN_MT_F1:
		result = NK6510_MEMORY_F1;
		break;
	case GN_MT_F2:
		result = NK6510_MEMORY_F2;
		break;
	case GN_MT_F3:
		result = NK6510_MEMORY_F3;
		break;
	case GN_MT_F4:
		result = NK6510_MEMORY_F4;
		break;
	case GN_MT_F5:
		result = NK6510_MEMORY_F5;
		break;
	case GN_MT_F6:
		result = NK6510_MEMORY_F6;
		break;
	case GN_MT_F7:
		result = NK6510_MEMORY_F7;
		break;
	case GN_MT_F8:
		result = NK6510_MEMORY_F8;
		break;
	case GN_MT_F9:
		result = NK6510_MEMORY_F9;
		break;
	case GN_MT_F10:
		result = NK6510_MEMORY_F10;
		break;
	case GN_MT_F11:
		result = NK6510_MEMORY_F11;
		break;
	case GN_MT_F12:
		result = NK6510_MEMORY_F12;
		break;
	case GN_MT_F13:
		result = NK6510_MEMORY_F13;
		break;
	case GN_MT_F14:
		result = NK6510_MEMORY_F14;
		break;
	case GN_MT_F15:
		result = NK6510_MEMORY_F15;
		break;
	case GN_MT_F16:
		result = NK6510_MEMORY_F16;
		break;
	case GN_MT_F17:
		result = NK6510_MEMORY_F17;
		break;
	case GN_MT_F18:
		result = NK6510_MEMORY_F18;
		break;
	case GN_MT_F19:
		result = NK6510_MEMORY_F19;
		break;
	case GN_MT_F20:
		result = NK6510_MEMORY_F20;
		break;
	default:
		result = NK6510_MEMORY_XX;
		break;
	}
	return (result);
}

static gn_memory_type get_gn_memory_type(int memory_type)
{
	int result;

	switch (memory_type) {
	case NK6510_MEMORY_IN:
		result = GN_MT_IN;
		break;
	case NK6510_MEMORY_OU:
		result = GN_MT_OU;
		break;
	case NK6510_MEMORY_OUS:
		result = GN_MT_OUS;
		break;
	case NK6510_MEMORY_AR:
		result = GN_MT_AR;
		break;
	case NK6510_MEMORY_TE:
		result = GN_MT_TE;
		break;
	case NK6510_MEMORY_F1:
		result = GN_MT_F1;
		break;
	case NK6510_MEMORY_F2:
		result = GN_MT_F2;
		break;
	case NK6510_MEMORY_F3:
		result = GN_MT_F3;
		break;
	case NK6510_MEMORY_F4:
		result = GN_MT_F4;
		break;
	case NK6510_MEMORY_F5:
		result = GN_MT_F5;
		break;
	case NK6510_MEMORY_F6:
		result = GN_MT_F6;
		break;
	case NK6510_MEMORY_F7:
		result = GN_MT_F7;
		break;
	case NK6510_MEMORY_F8:
		result = GN_MT_F8;
		break;
	case NK6510_MEMORY_F9:
		result = GN_MT_F9;
		break;
	case NK6510_MEMORY_F10:
		result = GN_MT_F10;
		break;
	case NK6510_MEMORY_F11:
		result = GN_MT_F11;
		break;
	case NK6510_MEMORY_F12:
		result = GN_MT_F12;
		break;
	case NK6510_MEMORY_F13:
		result = GN_MT_F13;
		break;
	case NK6510_MEMORY_F14:
		result = GN_MT_F14;
		break;
	case NK6510_MEMORY_F15:
		result = GN_MT_F15;
		break;
	case NK6510_MEMORY_F16:
		result = GN_MT_F16;
		break;
	case NK6510_MEMORY_F17:
		result = GN_MT_F17;
		break;
	case NK6510_MEMORY_F18:
		result = GN_MT_F18;
		break;
	case NK6510_MEMORY_F19:
		result = GN_MT_F19;
		break;
	case NK6510_MEMORY_F20:
		result = GN_MT_F20;
		break;
	default:
		result = GN_MT_XX;
		break;
	}
	return (result);
}
/*
01 33 00 17 05 01 05 08 04 00 01 00 00 00
01 33 00 17 05 01 05 08 04 00 01 00 00 00

0x0b / 0x000e
01 33 00 17 05 01 05 08 05 00 01 00 00 00
0x0b / 0x000e
01 33 00 17 05 01 05 08 04 00 01 00 00 00
0x0b / 0x000e
01 33 00 17 05 01 05 08 04 00 01 00 00 00
0x0b / 0x000e
01 33 00 17 05 01 05 08 05 00 01 00 00 00

0x0e / 0x000e
01 42 00 68 55 01 01 08 00 32 01 55 55 55
0x0e / 0x000e
01 42 00 68 55 01 01 08 00 32 01 55 55 55
*/
static int sms_encode(gn_data *data, struct gn_statemachine *state, unsigned char *req)
{
	int pos = 0, udh_length_pos, len;

	req[pos++] = 0x01; /* one big block */
	if (data->raw_sms->type == GN_SMS_MT_Deliver)
		req[pos++] = 0x00; /* message type: deliver */
	else
		req[pos++] = 0x02; /* message type: submit */

	req[pos++] = 0x00; /* will be set at the end */
	/*        is supposed to be the length of the whole message
		  starting from req[10], which is the message type */

	if (data->raw_sms->type == GN_SMS_MT_Deliver) {
		req[pos++] = 0x04; /* SMS Deliver */
	} else {
		req[pos] = 0x01; /* SMS Submit */

		if (data->raw_sms->reply_via_same_smsc)  req[pos] |= 0x80;
		if (data->raw_sms->reject_duplicates)  req[pos] |= 0x04;
		if (data->raw_sms->report)            req[pos] |= 0x20;
		if (data->raw_sms->udh_indicator)      req[pos] |= 0x40;
		if (data->raw_sms->validity_indicator) req[pos] |= 0x10;
		pos++;
		req[pos++] = data->raw_sms->reference;
		req[pos++] = data->raw_sms->pid;
	}

	req[pos++] = data->raw_sms->dcs;
	req[pos++] = 0x00;

	/* FIXME: real date/time */
	if (data->raw_sms->type == GN_SMS_MT_Deliver) {
		memcpy(req + pos, data->raw_sms->smsc_time, 7);
		pos += 7;
		memcpy(req + pos, "\x55\x55\x55", 3);
		pos += 3;
		/* Magic. Nokia new ideas: coding SMS in the sequent blocks */
		req[pos++] = 0x03; /* total blocks */
	} else {
		req[pos++] = 0x04; /* total blocks */
	}

	/* FIXME. Do it in the loop */

	/* Block 1. Remote Number */
	if (data->raw_sms->type == GN_SMS_MT_Submit && data->raw_sms->status != GN_SMS_Sent) {
		memcpy(req + pos, "\x82\x10\x01\x0C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
		pos += 16;
	} else {
		len = data->raw_sms->remote_number[0] + 4;
		if (len % 2) len++;
		len /= 2;
		req[pos] = 0x82; /* type: number */
		req[pos + 1] = (len + 4 > 0x0c) ? (len + 4) : 0x0c; /* offset to next block starting from start of block (req[18]) */
		req[pos + 2] = 0x01; /* first number field => remote_number */
		req[pos + 3] = len; /* actual data length in this block */
		memcpy(req + pos + 4, data->raw_sms->remote_number, len);
		pos += ((len + 4 > 0x0c) ? (len + 4) : 0x0c);
	}

	/* Block 2. SMSC Number */
	if (data->raw_sms->type == GN_SMS_MT_Submit && data->raw_sms->status != GN_SMS_Sent) {
		memcpy(req + pos, "\x82\x10\x02\x0C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
		pos += 16;
	} else {
		len = data->raw_sms->message_center[0] + 1;
		req[pos] = 0x82; /* type: number */
		req[pos + 1] = (len + 4 > 0x0c) ? (len + 4) : 0x0c; /* offset to next block starting from start of block (req[18]) */
		req[pos + 2] = 0x02; /* first number field => remote_number */
		req[pos + 3] = len;
		memcpy(req + pos + 4, data->raw_sms->message_center, len);
		pos += ((len + 4 > 0x0c) ? (len + 4) : 0x0c);
	}

	/* Block 3. User Data */
	req[pos++] = 0x80; /* type: User Data */
	req[pos++] = data->raw_sms->user_data_length + 4; /* same as req[11] but starting from req[42] */

	req[pos++] = data->raw_sms->user_data_length;
	req[pos++] = data->raw_sms->length;

	memcpy(req + pos, data->raw_sms->user_data, data->raw_sms->user_data_length);
	pos += data->raw_sms->user_data_length;

	/* padding */
	udh_length_pos = pos - data->raw_sms->user_data_length - 3;
	if (req[udh_length_pos] % 8 != 0) {
		memcpy(req + pos, "\x55\x55\x55\x55\x55\x55\x55\x55", 8 - req[udh_length_pos] % 8);
		pos += 8 - req[udh_length_pos] % 8;
		req[udh_length_pos] += 8 - req[udh_length_pos] % 8;
	}

	if (data->raw_sms->type == GN_SMS_MT_Submit) {
		/* Block 4. Validity Period */
		req[pos++] = 0x08; /* type: validity */
		req[pos++] = 0x04; /* block length */
		req[pos++] = 0x01; /* data length */
		req[pos++] = data->raw_sms->validity[0];
	}
	req[2] = pos - 1;
	return pos;
}

