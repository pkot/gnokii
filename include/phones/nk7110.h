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

  Copyright (C) 2000      Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000      Chris Kemp
  Copyright (C) 2001-2003 Pawel Kot, BORBELY Zoltan
  Copyright (C) 2002      Markus Plail

  This file provides functions specific to the Nokia 7110 series.
  See README for more details on supported mobile phones.

  The various routines are called nk7110_(whatever).

*/

#ifndef _gnokii_phones_nk7110_h
#define _gnokii_phones_nk7110_h

#include "misc.h"
#include "gnokii.h"

typedef enum {
	GN_OP_NK7110_GetSMSFolders = GN_OP_Max,
	GN_OP_NK7110_GetSMSFolderStatus,
	GN_OP_NK7110_GetPictureList,
	GN_OP_NK7110_Max       /* don't append anything after this entry */
} gn_nk7100_operation;

/* Message types */
#define NK7110_MSG_COMMSTATUS	0x01	/* Communication status */
#define NK7110_MSG_SMS		0x02	/* SMS handling */
#define NK7110_MSG_PHONEBOOK	0x03	/* Phonebook functions */
#define NK7110_MSG_DIVERT	0x06	/* Call Divert */
#define NK7110_MSG_NETSTATUS	0x0a	/* Network status */
#define NK7110_MSG_CALENDAR	0x13	/* Calendar notes */
#define NK7110_MSG_FOLDER	0x14	/* Folders handling */
#define NK7110_MSG_BATTERY	0x17	/* Battery info */
#define NK7110_MSG_CLOCK	0x19	/* Date & alarm */
#define NK7110_MSG_IDENTITY	0x1b	/* Brief product info */
#define NK7110_MSG_RINGTONE	0x1f	/* Ringtone handling */
#define NK7110_MSG_PROFILE	0x39	/* Profile handling */
#define NK7110_MSG_WAP		0x3f	/* WAP */
#define NK7110_MSG_SECURITY	0x40	/* Security */
#define NK7110_MSG_STLOGO	0x7a	/* Startup logo */
#define NK7110_MSG_KEYPRESS	0xd1	/* Keypress? */
#define NK7110_MSG_KEYPRESS_RESP 0xd2	/* Keypress response */

/* SMS handling message subtypes (send) */
#define NK7110_SUBSMS_SEND_SMS		0x01	/* Send SMS */
#define NK7110_SUBSMS_SET_CELLBRD	0x20	/* Set cell broadcast */
#define NK7110_SUBSMS_SET_SMSC		0x30	/* Set SMS center */
#define NK7110_SUBSMS_GET_SMSC		0x33	/* Get SMS center */
/* SMS handling message subtypes (recv) */
#define NK7110_SUBSMS_SEND_OK		0x02	/* SMS sent */
#define NK7110_SUBSMS_SEND_FAIL		0x03	/* SMS send failed */
#define NK7110_SUBSMS_READ_OK		0x08	/* SMS read */
#define NK7110_SUBSMS_READ_FAIL		0x09	/* SMS read failed */
#define NK7110_SUBSMS_DELETE_OK		0x0b	/* SMS deleted */
#define NK7110_SUBSMS_DELETE_FAIL	0x0c	/* SMS delete failed */
#define NK7110_SUBSMS_SMS_RCVD		0x10	/* SMS received */
#define NK7110_SUBSMS_CELLBRD_OK	0x21	/* Set cell broadcast success */
#define NK7110_SUBSMS_CELLBRD_FAIL	0x22	/* Set cell broadcast failure */
#define NK7110_SUBSMS_READ_CELLBRD	0x23	/* Read cell broadcast */
#define NK7110_SUBSMS_SMSC_OK		0x31	/* Set SMS center success*/
#define NK7110_SUBSMS_SMSC_FAIL		0x32	/* Set SMS center failure */
#define NK7110_SUBSMS_SMSC_RCVD		0x34	/* SMS center received */
#define NK7110_SUBSMS_SMSC_RCVFAIL	0x35	/* SMS center receive failure */
#define NK7110_SUBSMS_SMS_STATUS_OK	0x37	/* SMS status received */
#define NK7110_SUBSMS_FOLDER_STATUS_OK	0x6c	/* SMS folder status received */
#define NK7110_SUBSMS_FOLDER_LIST_OK	0x7b	/* SMS folder list received */
#define NK7110_SUBSMS_PICTURE_LIST_OK	0x97	/* Picture messages list received */

/* Clock handling message subtypes (send) */
#define NK7110_SUBCLO_SET_DATE		0x60	/* Set date & time */
#define NK7110_SUBCLO_GET_DATE		0x62	/* Get date & time */
#define NK7110_SUBCLO_SET_ALARM		0x6B	/* Set alarm */
#define NK7110_SUBCLO_GET_ALARM		0x6D	/* Get alarm */
/* Clock handling message subtypes (recv) */
#define NK7110_SUBCLO_DATE_SET		0x61	/* Date & time set */
#define NK7110_SUBCLO_DATE_RCVD		0x63	/* Received date & time */
#define NK7110_SUBCLO_ALARM_SET		0x6C	/* Alarm set */
#define NK7110_SUBCLO_ALARM_RCVD	0x6E	/* Received alarm */
/* Alarm on/off */
#define NK7110_ALARM_ENABLED		0x02	/* Alarm enabled */
#define NK7110_ALARM_DISABLED		0x01	/* Alarm disabled */

/* Calendar handling message subtypes (send) */
#define NK7110_SUBCAL_ADD_MEETING	0x01	/* Add meeting note */
#define NK7110_SUBCAL_ADD_CALL		0x03	/* Add call note */
#define NK7110_SUBCAL_ADD_BIRTHDAY	0x05	/* Add birthday note */
#define NK7110_SUBCAL_ADD_REMINDER	0x07	/* Add reminder note */
#define NK7110_SUBCAL_DEL_NOTE		0x0b	/* Delete note */
#define NK7110_SUBCAL_GET_NOTE		0x19	/* Get note */
#define NK7110_SUBCAL_GET_FREEPOS	0x31	/* Get first free position */
#define NK7110_SUBCAL_GET_INFO		0x3a	/* Calendar summary */
/* Calendar handling message subtypes (recv) */
#define NK7110_SUBCAL_ADD_MEETING_RESP	0x02	/* Add meeting note response */
#define NK7110_SUBCAL_ADD_CALL_RESP	0x04	/* Add call note response */
#define NK7110_SUBCAL_ADD_BIRTHDAY_RESP	0x06	/* Add birthday note response */
#define NK7110_SUBCAL_ADD_REMINDER_RESP	0x08	/* Add reminder note response */
#define NK7110_SUBCAL_DEL_NOTE_RESP	0x0c	/* Delete note response */
#define NK7110_SUBCAL_NOTE_RCVD		0x1a	/* Received note */
#define NK7110_SUBCAL_FREEPOS_RCVD	0x32	/* Received first free position */
#define NK7110_SUBCAL_INFO_RCVD		0x3b	/* Received calendar summary*/
/* Calendar note types */
#define NK7110_NOTE_MEETING		0x01	/* Meeting */
#define NK7110_NOTE_CALL		0x02	/* Call */
#define NK7110_NOTE_BIRTHDAY		0x04	/* Birthday */
#define NK7110_NOTE_REMINDER		0x08	/* Reminder */

/* Phone Memory types */
#define NK7110_MEMORY_DIALLED		0x01	/* Dialled numbers */
#define NK7110_MEMORY_MISSED		0x02	/* Missed calls */
#define NK7110_MEMORY_RECEIVED		0x03	/* Received calls */
#define NK7110_MEMORY_PHONE		0x05	/* Telephone phonebook */
#define NK7110_MEMORY_SIM		0x06	/* SIM phonebook */
#define NK7110_MEMORY_SPEEDDIALS	0x0e	/* Speed dials */
#define NK7110_MEMORY_GROUPS		0x10	/* Caller groups */

#define NK7110_MEMORY_DC		0x01	/* ME dialed calls list */
#define NK7110_MEMORY_MC		0x02	/* ME missed (unanswered received) calls list */
#define NK7110_MEMORY_RC		0x03	/* ME received calls list */
#define NK7110_MEMORY_FD		0x04	/* ?? SIM fixdialling-phonebook */
#define NK7110_MEMORY_ME		0x05	/* ME (Mobile Equipment) phonebook */
#define NK7110_MEMORY_SM		0x06	/* SIM phonebook */
#define NK7110_MEMORY_ON		0x07	/* ?? SIM (or ME) own numbers list */
#define NK7110_MEMORY_EN		0x08	/* ?? SIM (or ME) emergency number */
#define NK7110_MEMORY_MT		0x09	/* ?? combined ME and SIM phonebook */
#define NK7110_MEMORY_VOICE		0x0b	/* Voice Mailbox */

#define NK7110_MEMORY_IN		0x08	/* INBOX */
#define NK7110_MEMORY_OU		0x10	/* OUTBOX */
#define NK7110_MEMORY_AR		0x18	/* ARCHIVE */
#define NK7110_MEMORY_TE		0x20	/* TEMPLATES */
#define NK7110_MEMORY_F1		0x29	/* MY FOLDERS 1 */
#define NK7110_MEMORY_F2		0x31
#define NK7110_MEMORY_F3		0x39
#define NK7110_MEMORY_F4		0x41
#define NK7110_MEMORY_F5		0x49
#define NK7110_MEMORY_F6		0x51
#define NK7110_MEMORY_F7		0x59
#define NK7110_MEMORY_F8		0x61
#define NK7110_MEMORY_F9		0x69
#define NK7110_MEMORY_F10	 	0x71
#define NK7110_MEMORY_F11	 	0x79
#define NK7110_MEMORY_F12	 	0x81
#define NK7110_MEMORY_F13	 	0x89
#define NK7110_MEMORY_F14	 	0x91
#define NK7110_MEMORY_F15	 	0x99
#define NK7110_MEMORY_F16	 	0xA1
#define NK7110_MEMORY_F17	 	0xA9
#define NK7110_MEMORY_F18	 	0xB1
#define NK7110_MEMORY_F19	 	0xB9
#define NK7110_MEMORY_F20	 	0xC1	/* MY FOLDERS 20 */
/* This is used when the memory type is unknown. */
#define NK7110_MEMORY_XX 0xff

/* Entry Types for the enhanced phonebook */
#define NK7110_ENTRYTYPE_POINTER	0x04	/* Pointer to other memory */
#define NK7110_ENTRYTYPE_NAME		0x07	/* Name always the only one */
#define NK7110_ENTRYTYPE_EMAIL		0x08	/* Email Adress (TEXT) */
#define NK7110_ENTRYTYPE_POSTAL		0x09	/* Postal Address (Text) */
#define NK7110_ENTRYTYPE_NOTE		0x0a	/* Note (Text) */
#define NK7110_ENTRYTYPE_NUMBER		0x0b	/* Phonenumber */
#define NK7110_ENTRYTYPE_RINGTONE	0x0c	/* Ringtone */
#define NK7110_ENTRYTYPE_DATE		0x13	/* Date for a Called List */
#define NK7110_ENTRYTYPE_LOGO		0x1b	/* Group logo */
#define NK7110_ENTRYTYPE_LOGOSWITCH	0x1c	/* Group logo on/off */
#define NK7110_ENTRYTYPE_GROUP		0x1e	/* Group number for phonebook entry */

/* Entry types for the security commands */
#define NK7110_SUBSEC_ENABLE_EXTENDED_CMDS 0x64	/* Enable extended commands */
#define NK7110_SUBSEC_NETMONITOR	0x7e	/* Netmonitor */

typedef struct {
	bool new_sms;	/* Do we have a new SMS? */
	int ll_memtype;
	int ll_location;
	int userdef_location;

	/* callbacks */
	void (*on_cell_broadcast)(gn_cb_message *msg, void *callback_data);
	void (*call_notification)(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data);
	gn_error (*on_sms)(gn_sms *message, struct gn_statemachine *state, void *callback_data);

	/* callback local data */
	void *cb_callback_data;	/* to be passed as callback_data to on_cell_broadcast */
	void *call_callback_data;	/* to be passed as callback_data to call_notification */
	void *sms_callback_data;	/* to be passed as callback_data to on_sms */
} nk7110_driver_instance;

#endif  /* _gnokii_phones_nk7110_h */
