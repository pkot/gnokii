/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) 1999 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Functions to read and write common file types.
 
  Modified by Chris Kemp

*/

int GSM_ReadBitmapFile(char *FileName, GSM_Bitmap *bitmap);
int GSM_SaveBitmapFile(char *FileName, GSM_Bitmap *bitmap);

void savenol(FILE *file, GSM_Bitmap *bitmap);
void savengg(FILE *file, GSM_Bitmap *bitmap);
void savensl(FILE *file, GSM_Bitmap *bitmap);

int loadngg(FILE *file, GSM_Bitmap *bitmap);
int loadnol(FILE *file, GSM_Bitmap *bitmap);
int loadnsl(FILE *file, GSM_Bitmap *bitmap);
int loadnlm(FILE *file, GSM_Bitmap *bitmap);
int loadota(FILE *file, GSM_Bitmap *bitmap);

typedef enum {
  None=0,
  NOL,
  NGG,
  NSL,
  NLM
} GSM_Filetypes;
