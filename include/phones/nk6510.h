/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2002 Markus Plail

  This file provides functions specific to the 6510 series.
  See README for more details on supported mobile phones.

  The various routines are called P6510_(whatever).

*/

#ifndef __phones_nk6510_h
#define __phones_nk6510_h

#include "gsm-data.h"

typedef enum {
	GOP6510_GetSMSFolders = GOP_Max,
	GOP6510_GetSMSFolderStatus,
	GOP6510_GetPicture,
	GOP6510_Subscribe,
	GOP6510_Max       /* don't append anything after this entry */
} GSM6510_Operation;

extern bool P6510_LinkOK;

/* Message types */
#define P6510_MSG_COMMSTATUS	0x01	/* Communication status */
#define P6510_MSG_SMS		0x02	/* SMS handling */
#define P6510_MSG_PHONEBOOK	0x03	/* Phonebook functions */
#define P6510_MSG_DIVERT	0x06	/* Call Divert */
#define P6510_MSG_SECURITY	0x08	/* PIN and stuff */
#define P6510_MSG_NETSTATUS	0x0a	/* Network status */
#define P6510_MSG_KEYPRESS	0x0c	/* keypress emulation? */
#define P6510_MSG_SUBSCRIBE	0x10	/* subscribe to channels */
#define P6510_MSG_CALENDAR	0x13	/* Calendar notes */
#define P6510_MSG_FOLDER	0x14	/* Folders handling */
#define P6510_MSG_BATTERY	0x17	/* Battery info */
#define P6510_MSG_CLOCK		0x19	/* Date & alarm */
#define P6510_MSG_IDENTITY	0x1b	/* Brief product info */
#define P6510_MSG_RINGTONE	0x1f	/* Ringtone handling */
#define P6510_MSG_PROFILE	0x39	/* Profiles */
#define P6510_MSG_NOTKNOWN	0x3E	/*          */
#define P6510_MSG_WAP		0x3E	/* WAP */
#define P6510_MSG_RADIO    	0x43	/* Radio (6510/8310) */
#define P6510_MSG_TODO    	0x55	/* ToDo */
#define P6510_MSG_STLOGO	0x7a	/* Startup logo */
#define P6510_MSG_VERREQ	0xd1	/* HW&SW version request */
#define P6510_MSG_VERRESP	0xd2	/* HW&SW version response */

/* SMS handling message subtypes (send) */
#define P6510_SUBSMS_SEND_SMS		0x01	/* Send SMS */
#define P6510_SUBSMS_SET_CELLBRD	0x20	/* Set cell broadcast */
#define P6510_SUBSMS_SET_SMSC		0x23	/* Set SMS center */
#define P6510_SUBSMS_GET_SMSC		0x14	/* Get SMS center */
/* SMS handling message subtypes (recv) */
#define P6510_SUBSMS_SMS_SEND_STATUS	0x03 /* SMS sending status */
#define P6510_SUBSMS_SMS_SEND_OK	0x00	/* SMS sent */
#define P6510_SUBSMS_SMS_SEND_FAIL	0x01	/* SMS send failed */
#define P6510_SUBSMS_SMS_RCVD		0x10	/* SMS received */
#define P6510_SUBSMS_SMSC_RCV		0x15	/* SMS center info received */
#define P6510_SUBSMS_CELLBRD_OK		0x21	/* Set cell broadcast success*/
#define P6510_SUBSMS_CELLBRD_FAIL	0x22	/* Set cell broadcast failure */
#define P6510_SUBSMS_READ_CELLBRD	0x23	/* Read cell broadcast */
#define P6510_SUBSMS_SMSC_OK		0x31	/* Set SMS center success*/
#define P6510_SUBSMS_SMSC_FAIL		0x32	/* Set SMS center failure */

/* Clock handling message subtypes (send) */
#define P6510_SUBCLO_GET_DATE	0x0a	/* Get date & time */
#define P6510_SUBCLO_GET_ALARM	0x1b	/* Get alarm */
/* Clock handling message subtypes (recv) */
#define P6510_SUBCLO_DATE_RCVD	0x0b	/* Received date & time */
#define P6510_SUBCLO_DATE_UPD_RCVD	0x0e	/* Received update on date & time */
#define P6510_SUBCLO_ALARM_RCVD	0xff	/* Received alarm */
/* Alarm on/off */
#define P6510_ALARM_ENABLED	0x02	/* Alarm enabled */
#define P6510_ALARM_DISABLED	0x01	/* Alarm disabled */

/* Calendar handling message subtypes (send) */
#define P6510_SUBCAL_ADD_MEETING	0x01	/* Add meeting note */
#define P6510_SUBCAL_ADD_CALL		0x03	/* Add call note */
#define P6510_SUBCAL_ADD_BIRTHDAY	0x05	/* Add birthday note */
#define P6510_SUBCAL_ADD_REMINDER	0x07	/* Add reminder note */
#define P6510_SUBCAL_DEL_NOTE		0x0b	/* Delete note */
#define P6510_SUBCAL_GET_NOTE		0x19	/* Get note */
#define P6510_SUBCAL_GET_FREEPOS	0x31	/* Get first free position */
#define P6510_SUBCAL_GET_INFO		0x3a	/* Calendar sumary */
/* Calendar handling message subtypes (recv) */
#define P6510_SUBCAL_ADD_MEETING_RESP	0x02	/* Add meeting note response */
#define P6510_SUBCAL_ADD_CALL_RESP	0x04	/* Add call note response */
#define P6510_SUBCAL_ADD_BIRTHDAY_RESP	0x06	/* Add birthday note response */
#define P6510_SUBCAL_ADD_REMINDER_RESP	0x08	/* Add reminder note response */
#define P6510_SUBCAL_DEL_NOTE_RESP	0x0c	/* Dletete note response */
#define P6510_SUBCAL_NOTE_RCVD		0x1a	/* Received note */
#define P6510_SUBCAL_FREEPOS_RCVD	0x32	/* Received first free position */
#define P6510_SUBCAL_INFO_RCVD		0x3b	/* Received calendar summary*/
/* Calendar note types */
#define P6510_NOTE_MEETING		0x01	/* Metting */
#define P6510_NOTE_CALL			0x02	/* Call */
#define P6510_NOTE_BIRTHDAY		0x04	/* Birthday */
#define P6510_NOTE_REMINDER		0x08	/* Reminder */

/* Phone Memory types */
#define P6510_MEMORY_DIALLED	0x01	/* Dialled numbers */
#define P6510_MEMORY_MISSED	0x02	/* Missed calls */
#define P6510_MEMORY_RECEIVED	0x03	/* Received calls */
#define P6510_MEMORY_PHONE	0x05	/* Telephone phonebook */
#define P6510_MEMORY_SIM	0x06	/* SIM phonebook */
#define P6510_MEMORY_SPEEDDIALS	0x0e	/* Speed dials */
#define P6510_MEMORY_GROUPS	0x10	/* Caller groups */

#define P6510_MEMORY_DC		0x01	/* ME dialled calls list */
#define P6510_MEMORY_MC		0x02	/* ME missed (unanswered received) calls list */
#define P6510_MEMORY_RC		0x03	/* ME received calls list */
#define P6510_MEMORY_FD		0x04	/* ?? SIM fixdialling-phonebook */
#define P6510_MEMORY_ME		0x05	/* ME (Mobile Equipment) phonebook */
#define P6510_MEMORY_SM		0x06	/* SIM phonebook */
#define P6510_MEMORY_ON		0x07	/* ?? SIM (or ME) own numbers list */
#define P6510_MEMORY_EN		0x08	/* ?? SIM (or ME) emergency number */
#define P6510_MEMORY_MT		0x09	/* ?? combined ME and SIM phonebook */
#define P6510_MEMORY_VOICE	0x0b	/* Voice Mailbox */
#define P6510_MEMORY_IN		0x02	/* INBOX */
#define P6510_MEMORY_OU		0x03	/* OUTBOX */
#define P6510_MEMORY_AR		0x04	/* ARCHIVE */
#define P6510_MEMORY_TE		0x05	/* TEMPLATES */
#define P6510_MEMORY_F1		0x06	/* MY FOLDERS 1 */
#define P6510_MEMORY_F2		0x07
#define P6510_MEMORY_F3		0x08
#define P6510_MEMORY_F4		0x09
#define P6510_MEMORY_F5		0x10
#define P6510_MEMORY_F6		0x11
#define P6510_MEMORY_F7		0x12
#define P6510_MEMORY_F8		0x13
#define P6510_MEMORY_F9		0x14
#define P6510_MEMORY_F10	0x15
#define P6510_MEMORY_F11	0x16
#define P6510_MEMORY_F12	0x17
#define P6510_MEMORY_F13	0x18
#define P6510_MEMORY_F14	0x19
#define P6510_MEMORY_F15	0x20
#define P6510_MEMORY_F16	0x21
#define P6510_MEMORY_F17	0x22
#define P6510_MEMORY_F18	0x23
#define P6510_MEMORY_F19	0x24
#define P6510_MEMORY_F20	0x25	/* MY FOLDERS 20 */
/* This is used when the memory type is unknown. */
#define P6510_MEMORY_XX 0xff

/* Entry Types for the enhanced phonebook */
#define P6510_ENTRYTYPE_POINTER		0x1a	/* Pointer to other memory */
#define P6510_ENTRYTYPE_NAME		0x07	/* Name always the only one */
#define P6510_ENTRYTYPE_EMAIL		0x08	/* Email Adress (TEXT) */
#define P6510_ENTRYTYPE_POSTAL		0x09	/* Postal Address (Text) */
#define P6510_ENTRYTYPE_NOTE		0x0a	/* Note (Text) */
#define P6510_ENTRYTYPE_NUMBER		0x0b	/* Phonenumber */
#define P6510_ENTRYTYPE_RINGTONE	0x0c	/* Ringtone */
#define P6510_ENTRYTYPE_DATE		0x13	/* Date for a Called List */
#define P6510_ENTRYTYPE_LOGO		0x1b	/* Group logo */
#define P6510_ENTRYTYPE_LOGOSWITCH	0x1c	/* Group logo on/off */
#define P6510_ENTRYTYPE_GROUP		0x1e	/* Group number for phonebook entry */
#define P6510_ENTRYTYPE_URL		0x2c	/* Group number for phonebook entry */

/* Entry types for the security commands */
#define P6510_SUBSEC_ENABLE_EXTENDED_CMDS	0x64  /* Enable extended commands */
#define P6510_SUBSEC_NETMONITOR         	0x7e    /* Netmonitor */

#endif  /* __phones_nk6510_h */
