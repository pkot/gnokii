/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  Functions to read and write common file types.

  Last modification: Mon Mar 20 21:40:04 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

int GSM_ReadVCalendarFile(char *FileName, GSM_CalendarNote *cnote,
  int number);

int GetvCalTime(GSM_DateTime *dt, char *time);
int FillCalendarNote(GSM_CalendarNote *note, char *type,
  char *text, char *time, char *alarm);


/* Ringtone Files */

GSM_Error GSM_ReadRingtoneFile(char *FileName, GSM_Ringtone *ringtone);
GSM_Error GSM_SaveRingtoneFile(char *FileName, GSM_Ringtone *ringtone);

int GetScale (char *num);
int GetDuration (char *num);

/* Defines the character that separates fields in rtttl files. */
#define RTTTL_SEP ":"

GSM_Error saverttl(FILE *file, GSM_Ringtone *ringtone);
GSM_Error saveott(FILE *file, GSM_Ringtone *ringtone);

GSM_Error loadrttl(FILE *file, GSM_Ringtone *ringtone);
GSM_Error loadott(FILE *file, GSM_Ringtone *ringtone);


/* Bitmap Files */

GSM_Error GSM_ReadBitmapFile(char *FileName, GSM_Bitmap *bitmap);
GSM_Error GSM_SaveBitmapFile(char *FileName, GSM_Bitmap *bitmap);
int GSM_SaveTextFile(char *FileName, char *text, int mode);
GSM_Error GSM_ShowBitmapFile(char *FileName);

void savenol(FILE *file, GSM_Bitmap *bitmap);
void savengg(FILE *file, GSM_Bitmap *bitmap);
void savensl(FILE *file, GSM_Bitmap *bitmap);
void savenlm(FILE *file, GSM_Bitmap *bitmap);
void saveota(FILE *file, GSM_Bitmap *bitmap);
void savebmp(FILE *file, GSM_Bitmap *bitmap);

#ifdef XPM
  void savexpm(char *filename, GSM_Bitmap *bitmap);
#endif

GSM_Error loadngg(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadnol(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadnsl(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadnlm(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadota(FILE *file, GSM_Bitmap *bitmap);
GSM_Error loadbmp(FILE *file, GSM_Bitmap *bitmap);

#ifdef XPM
  GSM_Error loadxpm(char *filename, GSM_Bitmap *bitmap);
#endif

typedef enum {
  None=0,
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
