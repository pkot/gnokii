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

  This file provides decoding functions specific to the newer 
  Nokia 7110/6510 series.

*/

#ifndef __nokia_decoding_h
#define __nokia_decoding_h

#include "gsm-error.h"
#include "gsm-common.h"
#include "gsm-data.h"

#define PNOKIA_MEMORY_SPEEDDIALS	0x0e	/* Speed dials */

/* Entry Types for the enhanced phonebook */
#define PNOKIA_ENTRYTYPE_POINTER		0x1a	/* Pointer to other memory */
#define PNOKIA_ENTRYTYPE_NAME		0x07	/* Name always the only one */
#define PNOKIA_ENTRYTYPE_EMAIL		0x08	/* Email Adress (TEXT) */
#define PNOKIA_ENTRYTYPE_POSTAL		0x09	/* Postal Address (Text) */
#define PNOKIA_ENTRYTYPE_NOTE		0x0a	/* Note (Text) */
#define PNOKIA_ENTRYTYPE_NUMBER		0x0b	/* Phonenumber */
#define PNOKIA_ENTRYTYPE_RINGTONE	0x0c	/* Ringtone */
#define PNOKIA_ENTRYTYPE_DATE		0x13	/* Date for a Called List */
#define PNOKIA_ENTRYTYPE_LOGO		0x1b	/* Group logo */
#define PNOKIA_ENTRYTYPE_LOGOSWITCH	0x1c	/* Group logo on/off */
#define PNOKIA_ENTRYTYPE_GROUP		0x1e	/* Group number for phonebook entry */
#define PNOKIA_ENTRYTYPE_URL		0x2c	/* Group number for phonebook entry */

/* Calendar note types */
#define PNOKIA_NOTE_MEETING		0x01	/* Metting */
#define PNOKIA_NOTE_CALL		0x02	/* Call */
#define PNOKIA_NOTE_BIRTHDAY		0x04	/* Birthday */
#define PNOKIA_NOTE_REMINDER		0x08	/* Reminder */

GSM_Error DecodePhonebook(unsigned char *blockstart, int length, GSM_Data *data, int blocks, int memtype, int speeddialpos);
GSM_Error DecodeCalendar(unsigned char *message, int length, GSM_Data *data);


#endif /* __nokia_decoding_h */
