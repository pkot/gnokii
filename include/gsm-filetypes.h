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

int GSM_ReadRingtoneFile(char *FileName, GSM_Ringtone *ringtone);

int GetScale (char *num);
int GetDuration (char *num);

/* Defines the character that separates fields in rtttl files. */
#define RTTTL_SEP ":"


int GSM_ReadBitmapFile(char *FileName, GSM_Bitmap *bitmap);
int GSM_SaveBitmapFile(char *FileName, GSM_Bitmap *bitmap);

void savenol(FILE *file, GSM_Bitmap *bitmap);
void savengg(FILE *file, GSM_Bitmap *bitmap);
void savensl(FILE *file, GSM_Bitmap *bitmap);
void savenlm(FILE *file, GSM_Bitmap *bitmap);
void savexpm(char *filename, GSM_Bitmap *bitmap);

int loadngg(FILE *file, GSM_Bitmap *bitmap);
int loadnol(FILE *file, GSM_Bitmap *bitmap);
int loadnsl(FILE *file, GSM_Bitmap *bitmap);
int loadnlm(FILE *file, GSM_Bitmap *bitmap);
int loadota(FILE *file, GSM_Bitmap *bitmap);
int loadxpm(char *filename, GSM_Bitmap *bitmap);

typedef enum {
  None=0,
  NOL,
  NGG,
  NSL,
  NLM,
  XPMF
} GSM_Filetypes;
