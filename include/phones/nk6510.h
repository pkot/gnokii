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

  Copyright (C) 2000      Hugh Blemings, Pavel Janik
  Copyright (C) 2001-2004 BORBELY Zoltan, Pawel Kot
  Copyright (C) 2002      Markus Plail

  This file provides functions specific to the Nokia 6510 series.
  See README for more details on supported mobile phones.

  The various routines are called nk6510_(whatever).

*/

#ifndef _gnokii_phones_nk6510_h
#define _gnokii_phones_nk6510_h

#include "gnokii.h"

typedef enum {
	GN_OP_NK6510_GetSMSFolders = GN_OP_Max,
	GN_OP_NK6510_GetSMSFolderStatus,
	GN_OP_NK6510_GetPicture,
	GN_OP_NK6510_Subscribe,
	GN_OP_NK6510_Max       /* don't append anything after this entry */
} GSM6510_Operation;

/* Message types */
#define NK6510_MSG_COMMSTATUS	0x01	/* Communication status */
#define NK6510_MSG_SMS		0x02	/* SMS handling */
#define NK6510_MSG_PHONEBOOK	0x03	/* Phonebook functions */
#define NK6510_MSG_DIVERT	0x06	/* Call Divert */
#define NK6510_MSG_SECURITY	0x08	/* PIN and stuff */
#define NK6510_MSG_NETSTATUS	0x0a	/* Network status */
#define NK6510_MSG_SOUND	0x0b	/* Sound */
#define NK6510_MSG_KEYPRESS	0x0c	/* keypress emulation? */
#define NK6510_MSG_SUBSCRIBE	0x10	/* subscribe to channels */
#define NK6510_MSG_CALENDAR	0x13	/* Calendar notes */
#define NK6510_MSG_FOLDER	0x14	/* Folders handling */
#define NK6510_MSG_RESET        0x15	/* Reset */
#define NK6510_MSG_BATTERY	0x17	/* Battery info */
#define NK6510_MSG_CLOCK	0x19	/* Date & alarm */
#define NK6510_MSG_IDENTITY	0x1b	/* Brief product info */
#define NK6510_MSG_RINGTONE	0x1f	/* Ringtone handling */
#define NK6510_MSG_PROFILE	0x39	/* Profiles */
#define NK6510_MSG_NOTKNOWN	0x3E	/*          */
#define NK6510_MSG_WAP		0x3F	/* WAP */
#define NK6510_MSG_RADIO    	0x43	/* Radio (6510/8310) + hard reset */
#define NK6510_MSG_TODO    	0x55	/* ToDo */
#define NK6510_MSG_FILE	        0x6d	/* File Handling */
#define NK6510_MSG_STLOGO	0x7a	/* Startup logo */

/* SMS handling message subtypes (send) */
#define NK6510_SUBSMS_SEND_SMS		0x01	/* Send SMS */
#define NK6510_SUBSMS_SET_CELLBRD	0x20	/* Set cell broadcast */
#define NK6510_SUBSMS_SET_SMSC		0x23	/* Set SMS center */
#define NK6510_SUBSMS_GET_SMSC		0x14	/* Get SMS center */
/* SMS handling message subtypes (recv) */
#define NK6510_SUBSMS_SMS_SEND_STATUS	0x03	/* SMS sending status */
#define NK6510_SUBSMS_SMS_SEND_OK	0x00	/* SMS sent */
#define NK6510_SUBSMS_SMS_SEND_FAIL	0x01	/* SMS send failed */
#define NK6510_SUBSMS_SMS_RCVD		0x10	/* SMS received */
#define NK6510_SUBSMS_SMSC_RCV		0x15	/* SMS center info received */
#define NK6510_SUBSMS_CELLBRD_OK	0x21	/* Set cell broadcast success */
#define NK6510_SUBSMS_CELLBRD_FAIL	0x22	/* Set cell broadcast failure */
#define NK6510_SUBSMS_READ_CELLBRD	0x23	/* Read cell broadcast */
#define NK6510_SUBSMS_SMSC_OK		0x31	/* Set SMS center success */
#define NK6510_SUBSMS_SMSC_FAIL		0x32	/* Set SMS center failure */
#define NK6510_SUBSMS_INCOMING		0x04	/* Incoming SMS notification */

/* Clock handling message subtypes (send) */
#define NK6510_SUBCLO_GET_DATE		0x0a	/* Get date & time */
#define NK6510_SUBCLO_GET_ALARM		0x02	/* Get alarm */
/* Clock handling message subtypes (recv) */
#define NK6510_SUBCLO_DATE_RCVD		0x0b	/* Received date & time */
#define NK6510_SUBCLO_SET_DATE_RCVD	0x02	/* Received date & time set OK */
#define NK6510_SUBCLO_SET_ALARM_RCVD	0x12	/* Received alarm set OK */
#define NK6510_SUBCLO_DATE_UPD_RCVD	0x0e	/* Received update on date & time */
#define NK6510_SUBCLO_ALARM_TIME_RCVD	0x1a	/* Received alarm time */
#define NK6510_SUBCLO_ALARM_STATE_RCVD	0x20	/* Received alarm state (on/off) */
/* Alarm on/off */
#define NK6510_ALARM_ENABLED		0x02	/* Alarm enabled */
#define NK6510_ALARM_DISABLED		0x01	/* Alarm disabled */

/* Calendar handling message subtypes (send) */
#define NK6510_SUBCAL_ADD_MEETING	0x01	/* Add meeting note */
#define NK6510_SUBCAL_ADD_CALL		0x03	/* Add call note */
#define NK6510_SUBCAL_ADD_BIRTHDAY	0x05	/* Add birthday note */
#define NK6510_SUBCAL_ADD_REMINDER	0x07	/* Add reminder note */
#define NK6510_SUBCAL_DEL_NOTE		0x0b	/* Delete note */
#define NK6510_SUBCAL_GET_NOTE		0x19	/* Get note */
#define NK6510_SUBCAL_GET_FREEPOS	0x31	/* Get first free position */
#define NK6510_SUBCAL_GET_INFO		0x3a	/* Calendar summary */
/* Calendar handling message subtypes (recv) */
#define NK6510_SUBCAL_ADD_MEETING_RESP	0x02	/* Add meeting note response */
#define NK6510_SUBCAL_ADD_CALL_RESP	0x04	/* Add call note response */
#define NK6510_SUBCAL_ADD_BIRTHDAY_RESP	0x06	/* Add birthday note response */
#define NK6510_SUBCAL_ADD_REMINDER_RESP	0x08	/* Add reminder note response */
#define NK6510_SUBCAL_DEL_NOTE_RESP	0x0c	/* Delete note response */
#define NK6510_SUBCAL_NOTE_RCVD		0x1a	/* Received note */
#define NK6510_SUBCAL_FREEPOS_RCVD	0x32	/* Received first free position */
#define NK6510_SUBCAL_INFO_RCVD		0x3b	/* Received calendar summary */
#define NK6510_SUBCAL_NOTE2_RCVD	0x7e	/* Received note (with more details) */
#define NK6510_SUBCAL_ADD_NOTE_RESP	0x66	/* Add calendar note response */
#define NK6510_SUBCAL_INFO2_RCVD	0x9f	/* Received calendar summary */
#define NK6510_SUBCAL_FREEPOS2_RCVD	0x96	/* Received first free position */
#define NK6510_SUBCAL_UNSUPPORTED	0xf0	/* Error code for unsupported FBUS frames */

/* Calendar note types */
#define NK6510_NOTE_REMINDER		0x00	/* Reminder */
#define NK6510_NOTE_MEETING		0x01	/* Meeting */
#define NK6510_NOTE_CALL		0x02	/* Call */
#define NK6510_NOTE_BIRTHDAY		0x04	/* Birthday */
#define NK6510_NOTE_MEMO		0x08	/* Memo */

/* Phone Memory types */
#define NK6510_MEMORY_DIALLED		0x01	/* Dialled numbers */
#define NK6510_MEMORY_MISSED		0x02	/* Missed calls */
#define NK6510_MEMORY_RECEIVED		0x03	/* Received calls */
#define NK6510_MEMORY_PHONE		0x05	/* Telephone phonebook */
#define NK6510_MEMORY_SIM		0x06	/* SIM phonebook */
#define NK6510_MEMORY_SPEEDDIALS	0x0e	/* Speed dials */
#define NK6510_MEMORY_GROUPS		0x10	/* Caller groups */

#define NK6510_MEMORY_DC		0x01	/* ME dialed calls list */
#define NK6510_MEMORY_MC		0x02	/* ME missed (unanswered received) calls list */
#define NK6510_MEMORY_RC		0x03	/* ME received calls list */
#define NK6510_MEMORY_FD		0x04	/* ?? SIM fixdialling-phonebook */
#define NK6510_MEMORY_ME		0x05	/* ME (Mobile Equipment) phonebook */
#define NK6510_MEMORY_SM		0x06	/* SIM phonebook */
#define NK6510_MEMORY_ON		0x07	/* ?? SIM (or ME) own numbers list */
#define NK6510_MEMORY_EN		0x08	/* ?? SIM (or ME) emergency number */
#define NK6510_MEMORY_MT		0x09	/* ?? combined ME and SIM phonebook */
#define NK6510_MEMORY_VOICE		0x0b	/* Voice Mailbox */
#define NK6510_MEMORY_IN		0x02	/* INBOX */
#define NK6510_MEMORY_OU		0x03	/* OUTBOX */
#define NK6510_MEMORY_AR		0x04	/* ARCHIVE */
#define NK6510_MEMORY_TE		0x05	/* TEMPLATES */
#define NK6510_MEMORY_F1		0x06	/* MY FOLDERS 1 */
#define NK6510_MEMORY_F2		0x07
#define NK6510_MEMORY_F3		0x08
#define NK6510_MEMORY_F4		0x09
#define NK6510_MEMORY_F5		0x10
#define NK6510_MEMORY_F6		0x11
#define NK6510_MEMORY_F7		0x12
#define NK6510_MEMORY_F8		0x13
#define NK6510_MEMORY_F9		0x14
#define NK6510_MEMORY_F10		0x15
#define NK6510_MEMORY_F11		0x16
#define NK6510_MEMORY_F12		0x17
#define NK6510_MEMORY_F13		0x18
#define NK6510_MEMORY_F14		0x19
#define NK6510_MEMORY_F15		0x20
#define NK6510_MEMORY_F16		0x21
#define NK6510_MEMORY_F17		0x22
#define NK6510_MEMORY_F18		0x23
#define NK6510_MEMORY_F19		0x24
#define NK6510_MEMORY_F20		0x25	/* MY FOLDERS 20 */
#define NK6510_MEMORY_OUS		0x1a
/* This is used when the memory type is unknown. */
#define NK6510_MEMORY_XX		0xff

/* Entry Types for the enhanced phonebook */
#define NK6510_ENTRYTYPE_POINTER	0x1a	/* Pointer to other memory */
#define NK6510_ENTRYTYPE_NAME		0x07	/* Name always the only one */
#define NK6510_ENTRYTYPE_EMAIL		0x08	/* Email Adress (TEXT) */
#define NK6510_ENTRYTYPE_POSTAL		0x09	/* Postal Address (Text) */
#define NK6510_ENTRYTYPE_NOTE		0x0a	/* Note (Text) */
#define NK6510_ENTRYTYPE_NUMBER		0x0b	/* Phonenumber */
#define NK6510_ENTRYTYPE_RINGTONE	0x0c	/* Ringtone */
#define NK6510_ENTRYTYPE_DATE		0x13	/* Date for a Called List */
#define NK6510_ENTRYTYPE_LOGO		0x1b	/* Group logo */
#define NK6510_ENTRYTYPE_LOGOSWITCH	0x1c	/* Group logo on/off */
#define NK6510_ENTRYTYPE_GROUP		0x1e	/* Group number for phonebook entry */
#define NK6510_ENTRYTYPE_URL		0x2c	/* Group number for phonebook entry */

/* Entry types for the security commands */
#define NK6510_SUBSEC_ENABLE_EXTENDED_CMDS	0x64	/* Enable extended commands */
#define NK6510_SUBSEC_NETMONITOR         	0x7e	/* Netmonitor */

#define	NK6510_RINGTONE_USERDEF_LOCATION	231

#define NK6510_FILE_ID_LENGTH 6

/*
 * For Nokia Series40 3rd edition, we implement GetSMS with files commands.
 * Algorithm is:
 *   - get file list
 *   - filter sms files by name
 *   - get file
 * It is repeated for every SMS. As 'get file list' is the longest opration
 * it makes sense to cache the results. Define here for how long (seconds)
 * data is cached.
 */
#define NK6510_FILE_CACHE_TIMEOUT	10	/* seconds */
#define NK6510_GETFILELIST_TIMEOUT	500	/* miliseconds */

typedef struct {
	/* callbacks */
	void (*on_cell_broadcast)(gn_cb_message *msg, struct gn_statemachine *state, void *callback_data);
	void (*call_notification)(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data);
	gn_error (*on_sms)(gn_sms *message, struct gn_statemachine *state, void *callback_data);
	void (*progress_indication)(int progress, void *callback_data);

	/* phone model capabilities */
	gn_phone_model *pm;

	/* callback local data */
	void *cb_callback_data;		/* to be passed as callback_data to on_cell_broadcast */
	void *call_callback_data;	/* to be passed as callback_data to call_notification */
	void *sms_callback_data;	/* to be passed as callback_data to on_sms */
	void *progress_callback_data;	/* to be passed as callback_data to progress_indication */
} nk6510_driver_instance;

#endif  /* _gnokii_phones_nk6510_h */
