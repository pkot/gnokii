/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  Functions for common bitmap operations.

  $Log$
  Revision 1.4  2001-11-20 16:22:22  pkot
  First attempt to read Picture Messages. They should appear when you enable DEBUG. Nokia seems to break own standards. :/ (Markus Plail)

  Revision 1.3  2001/06/28 00:28:45  pkot
  Small docs updates (Pawel Kot)


*/

#ifndef __gsm_bitmaps_h__
#define __gsm_bitmaps_h__

void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y);
void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y);
bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y);
void GSM_ClearBitmap(GSM_Bitmap *bmp);
void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target, GSM_Information *info);
void GSM_PrintBitmap(GSM_Bitmap *bitmap);

/* SMS bitmap functions */

GSM_Error GSM_ReadSMSBitmap(int type, char *message, char *code, GSM_Bitmap *bitmap);
int  GSM_SaveSMSBitmap(GSM_SMSMessage *message, GSM_Bitmap *bitmap);

#endif





