/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  Functions for common bitmap operations.

  Last modification: Sun 19th Nov 2000
  Modified by Chris Kemp

*/

void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y);
void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y);
bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y);
void GSM_ClearBitmap(GSM_Bitmap *bmp);
void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target);
void GSM_PrintBitmap(GSM_Bitmap *bitmap);

/* SMS bitmap functions */

GSM_Error GSM_ReadSMSBitmap(GSM_SMSMessage *message, GSM_Bitmap *bitmap);
int  GSM_SaveSMSBitmap(GSM_SMSMessage *message, GSM_Bitmap *bitmap);







