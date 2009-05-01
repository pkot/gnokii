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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2002-2004 BORBELY Zoltan, Pawel Kot
  Copyright (C) 2002      Ladis Michl, Markus Plail, Pavel Machek, Chris Kemp

*/

#ifndef _gnokii_data_h
#define _gnokii_data_h

#include <gnokii/common.h>
#include <gnokii/sms.h>
#include <gnokii/mms.h>
#include <gnokii/call.h>
#include <gnokii/error.h>
#include <gnokii/rlp-common.h>

/* For models table */
typedef struct {
	const char *model;	  /* e.g. 6310 */
	const char *product_name; /* e.g. NPE-4 */
	int flags;
} gn_phone_model;

/* This is a generic holder for high level information - eg a gn_bmp */
typedef struct {
	gn_sms_folder *sms_folder;
	gn_sms_folder_list *sms_folder_list;
	/*
	 * This is for phone driver, application using libgnokii should not
	 * touch this.
	 */
	gn_sms_raw *raw_sms;
	/*
	 * This is for user communication, phone driver should not have to
	 * touch this one.
	 */
	gn_sms *sms;
	gn_phonebook_entry *phonebook_entry;
	gn_speed_dial *speed_dial;
	gn_memory_status *memory_status;
	gn_sms_message_list *message_list[GN_SMS_MESSAGE_MAX_NUMBER][GN_SMS_FOLDER_MAX_NUMBER];
	gn_sms_status *sms_status;
	gn_sms_folder_stats *folder_stats[GN_SMS_FOLDER_MAX_NUMBER];
	gn_sms_message_center *message_center;
	char *imei;
	char *revision;
	char *model;
	char *manufacturer;
	gn_network_info *network_info;
	gn_wap_bookmark *wap_bookmark;
	gn_wap_setting *wap_setting;
	gn_todo *todo;
	gn_todo_list *todo_list;
	gn_calnote *calnote;
	gn_calnote_list *calnote_list;
	gn_bmp *bitmap;
	gn_ringtone *ringtone;
	gn_profile *profile;
	gn_battery_unit *battery_unit;
	float *battery_level;
	gn_rf_unit *rf_unit;
	float *rf_level;
	gn_display_output *display_output;
	char *incoming_call_nr;
	gn_power_source *power_source;
	gn_timestamp *datetime;
	gn_calnote_alarm *alarm;
	gn_raw_data *raw_data;
	gn_call_divert *call_divert;
	gn_error (*on_sms)(gn_sms *message, struct gn_statemachine *state, void *callback_data);
	int *display_status;
	void (*on_cell_broadcast)(gn_cb_message *message, struct gn_statemachine *state, void *callback_data);
	gn_netmonitor *netmonitor;
	gn_call_info *call_info;
	void (*call_notification)(gn_call_status call_status, gn_call_info *call_info,
				  struct gn_statemachine *state, void *callback_data);
	gn_rlp_f96_frame *rlp_frame;
	int rlp_out_dtx;
	void (*rlp_rx_callback)(gn_rlp_f96_frame *frame);
	gn_security_code *security_code;
	const char *dtmf_string;
	unsigned char reset_type;
	gn_key_code key_code;
	unsigned char character;
	gn_phone_model *phone;
	gn_locks_info *locks_info;
	gn_tone *tone;
	gn_ringtone_list *ringtone_list;
	gn_call_active *call_active;
	gn_file_list *file_list;
	gn_file *file;
	/*
	 * This is a pointer to some data that may be used by any callback
	 * function.
	 */
	void *callback_data;
	/*
	 * This is a callback function for any changes related to the network
	 * registration parameters changes: status, LAC, cell id.
	 */
	void (*reg_notification)(gn_network_info *info, void *callback_data);
	/*
	 * This is callback function for file download progress.
	 * progress is value in range [0, 100].
	 */
	void (*progress_indication)(int progress, void *callback_data);
	/*
	 * This is for phone driver, application using libgnokii should not
	 * touch this.
	 */
	gn_mms_raw *raw_mms;
	/*
	 * This is for user communication, phone driver should not have to
	 * touch this one.
	 */
	gn_mms *mms;
} gn_data;

/* 
 * A structure to hold information about the particular link
 * The link comes 'under' the phone
 */
typedef struct {
	/* A regularly called loop function. Timeout can be used to make the
	 * function block or not */
	gn_error (*loop)(struct timeval *timeout, struct gn_statemachine *state);
	/* A pointer to the function used to send out a message. This is used
	 * by the phone specific code to send a message over the link */
	gn_error (*send_message)(unsigned int messagesize, unsigned char messagetype, unsigned char *message,
				 struct gn_statemachine *state);
	void *link_instance;
	void (*reset)(struct gn_statemachine *state);
} gn_link;

typedef struct {
	char model[GN_MODEL_MAX_LENGTH];		/* Phone model */
	char irda_string[GN_MODEL_MAX_LENGTH];		/* If IrDA connection used, string shown in the discovery log */
	char port_device[GN_DEVICE_NAME_MAX_LENGTH];	/* Port device to use (e.g. /dev/ttyS0) */
	gn_connection_type connection_type;		/* Connection type (e.g. serial, ir) */
	int init_length;				/* Number of chars sent to sync the serial port */
	int serial_baudrate;				/* Baud rate to use */
	int serial_write_usleep;			/* Inter character delay or <0 to disable */
	int hardware_handshake;				/* Select between hardware and software handshake */
	int require_dcd;				/* DCD signal check */
	int smsc_timeout;				/* How many seconds should we wait for the SMSC response, defaults to 10 seconds */
	char connect_script[256];			/* Script to run when device connection established */
	char disconnect_script[256];			/* Script to run when device connection closed */
	uint8_t rfcomm_cn;				/* RFCOMM channel number to connect */
	unsigned int sm_retry;				/* Indicates whether statemachine should do retries. Defaults to off. */
							/* Use with caution -- may break newer DCT4 phones */

	unsigned int use_locking;			/* Should we use locking system or not */
	/* do not change the following values from userspace */
	char m_model[GN_MODEL_MAX_LENGTH];
	char m_manufacturer[GN_MANUFACTURER_MAX_LENGTH];
	char m_revision[GN_REVISION_MAX_LENGTH];
	char m_imei[GN_IMEI_MAX_LENGTH];
} gn_config;

typedef struct {
	int fd;
	gn_connection_type type;
	void *device_instance;
} gn_device;

typedef enum {
	GN_OP_Init,
	GN_OP_Terminate,
	GN_OP_GetModel,
	GN_OP_GetRevision,
	GN_OP_GetImei,
	GN_OP_GetManufacturer,
	GN_OP_Identify,
	GN_OP_GetBitmap,
	GN_OP_SetBitmap,
	GN_OP_GetBatteryLevel,
	GN_OP_GetRFLevel,
	GN_OP_DisplayOutput,
	GN_OP_GetMemoryStatus,
	GN_OP_ReadPhonebook,
	GN_OP_WritePhonebook,
	GN_OP_DeletePhonebook,
	GN_OP_GetPowersource,
	GN_OP_GetAlarm,
	GN_OP_GetSMSStatus,
	GN_OP_GetIncomingCallNr,
	GN_OP_GetNetworkInfo,
	GN_OP_GetSecurityCode,
	GN_OP_CreateSMSFolder,
	GN_OP_DeleteSMSFolder,
	GN_OP_GetSMS,
	GN_OP_GetSMSnoValidate,
	GN_OP_GetSMSFolders,
	GN_OP_GetSMSFolderStatus,
	GN_OP_GetIncomingSMS,
	GN_OP_GetUnreadMessages,
	GN_OP_GetNextSMS,
	GN_OP_DeleteSMSnoValidate,
	GN_OP_DeleteSMS,
	GN_OP_SendSMS,
	GN_OP_GetSpeedDial,
	GN_OP_GetSMSCenter,
	GN_OP_SetSMSCenter,
	GN_OP_GetDateTime,
	GN_OP_GetToDo,
	GN_OP_GetCalendarNote,
	GN_OP_CallDivert,
	GN_OP_OnSMS, /* set data->on_sms and data->callback_data */
	GN_OP_PollSMS,
	GN_OP_SetAlarm,
	GN_OP_SetDateTime,
	GN_OP_GetProfile,
	GN_OP_SetProfile,
	GN_OP_WriteToDo,
	GN_OP_DeleteAllToDos,
	GN_OP_WriteCalendarNote,
	GN_OP_DeleteCalendarNote,
	GN_OP_SetSpeedDial,
	GN_OP_GetDisplayStatus,
	GN_OP_PollDisplay,
	GN_OP_SaveSMS,
	GN_OP_SetCellBroadcast, /* set data->on_cell_broadcast and data->callback_data */
	GN_OP_NetMonitor,
	GN_OP_MakeCall,
	GN_OP_AnswerCall,
	GN_OP_CancelCall,
	GN_OP_SetCallNotification, /* set data->call_notification and data->callback_data */
	GN_OP_SendRLPFrame,
	GN_OP_SetRLPRXCallback,
	GN_OP_EnterSecurityCode,
	GN_OP_GetSecurityCodeStatus,
	GN_OP_ChangeSecurityCode,
	GN_OP_SendDTMF,
	GN_OP_Reset,
	GN_OP_GetRingtone,
	GN_OP_SetRingtone,
	GN_OP_GetRawRingtone,
	GN_OP_SetRawRingtone,
	GN_OP_PressPhoneKey,
	GN_OP_ReleasePhoneKey,
	GN_OP_EnterChar,
	GN_OP_Subscribe,
	GN_OP_GetWAPBookmark,
	GN_OP_WriteWAPBookmark,
	GN_OP_DeleteWAPBookmark,
	GN_OP_GetWAPSetting,
	GN_OP_ActivateWAPSetting,
	GN_OP_WriteWAPSetting,
	GN_OP_GetLocksInfo,
	GN_OP_GetActiveProfile,
	GN_OP_SetActiveProfile,
	GN_OP_PlayTone,
	GN_OP_GetRingtoneList,
	GN_OP_DeleteRingtone,
	GN_OP_GetActiveCalls,
	GN_OP_GetFileList,
	GN_OP_GetFileId,
	GN_OP_GetFile,
	GN_OP_PutFile,
	GN_OP_DeleteFile,
	GN_OP_GetFileDetailsById,
	GN_OP_GetFileById,
	GN_OP_DeleteFileById,
	GN_OP_GetMMS,
	GN_OP_DeleteMMS,
	GN_OP_Max,	/* don't append anything after this entry */
} gn_operation;

/* Undefined functions in fbus/mbus files */
extern gn_error gn_unimplemented(void);
#define GN_UNIMPLEMENTED (void *) gn_unimplemented

#endif	/* _gnokii_data_h */
