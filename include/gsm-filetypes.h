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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Functions to read and write common file types.

*/

#ifndef _gnokii_gsm_filetypes_h
#define _gnokii_gsm_filetypes_h

#include "gsm-error.h"
#include "gsm-common.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"

int GSM_ReadVCalendarFileEvent(char *FileName, GSM_CalendarNote *cnote, int number);
int GSM_ReadVCalendarFileTodo(char *FileName, GSM_ToDo *cnote, int number);

int GetvCalTime(GSM_DateTime *dt, char *time);
int FillCalendarNote(GSM_CalendarNote *note, char *type,
		     char *text, char *desc, char *time, char *alarm);
int FillToDo(GSM_ToDo *note, char *text, char *todo_priority);


/* Ringtone Files */

gn_error GSM_ReadRingtoneFile(char *FileName, GSM_Ringtone *ringtone);
gn_error GSM_SaveRingtoneFile(char *FileName, GSM_Ringtone *ringtone);

int GetScale (char *num);
int GetDuration (char *num);

/* Defines the character that separates fields in rtttl files. */
#define RTTTL_SEP ":"

gn_error saverttl(FILE *file, GSM_Ringtone *ringtone);
gn_error saveott(FILE *file, GSM_Ringtone *ringtone);

gn_error loadrttl(FILE *file, GSM_Ringtone *ringtone);
gn_error loadott(FILE *file, GSM_Ringtone *ringtone);


/* Bitmap Files */

gn_error GSM_ReadBitmapFile(char *FileName, gn_bmp *bitmap, GSM_Information *info);
gn_error GSM_SaveBitmapFile(char *FileName, gn_bmp *bitmap, GSM_Information *info);
int GSM_SaveTextFile(char *FileName, char *text, int mode);
gn_error GSM_ShowBitmapFile(char *FileName);

void savenol(FILE *file, gn_bmp *bitmap, GSM_Information *info);
void savengg(FILE *file, gn_bmp *bitmap, GSM_Information *info);
void savensl(FILE *file, gn_bmp *bitmap, GSM_Information *info);
void savenlm(FILE *file, gn_bmp *bitmap);
void saveota(FILE *file, gn_bmp *bitmap);
void savebmp(FILE *file, gn_bmp *bitmap);

#ifdef XPM
void savexpm(char *filename, gn_bmp *bitmap);
#endif

gn_error loadngg(FILE *file, gn_bmp *bitmap, GSM_Information *info);
gn_error loadnol(FILE *file, gn_bmp *bitmap, GSM_Information *info);
gn_error loadnsl(FILE *file, gn_bmp *bitmap);
gn_error loadnlm(FILE *file, gn_bmp *bitmap);
gn_error loadota(FILE *file, gn_bmp *bitmap, GSM_Information *info);
gn_error loadbmp(FILE *file, gn_bmp *bitmap);

#ifdef XPM
gn_error loadxpm(char *filename, gn_bmp *bitmap);
#endif

typedef enum {
	None = 0,
	NOL,
	NGG,
	NSL,
	NLM,
	BMP,
	OTA,
	XPMF,
	RTTL,
	OTT
} GSM_Filetypes;

#endif /* _gnokii_gsm_filetypes_h */
