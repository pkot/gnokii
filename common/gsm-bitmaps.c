/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Functions for common bitmap operations.
 
  Last modified: Sat 18 Nov 2000 by Chris Kemp

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "gsm-common.h"
#include "gsm-bitmaps.h"


/* A few useful functions for bitmaps */

void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y) {
  if (bmp->type == GSM_StartupLogo) bmp->bitmap[((y/8)*84)+x] |= 1 << (y%8);
  if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo) bmp->bitmap[9*y + (x/8)] |= 1 << (7-(x%8));

  /* Testing only! */
  if (bmp->type == GSM_PictureImage) bmp->bitmap[9*y + (x/8)] |= 1 << (7-(x%8));
}

void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y) {
  if (bmp->type == GSM_StartupLogo) bmp->bitmap[((y/8)*84)+x] &= 255 - (1 << (y%8));
  if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo) bmp->bitmap[9*y + (x/8)] &= 255 - (1 << (7-(x%8)));

  /* Testing only ! */
  if (bmp->type == GSM_PictureImage) bmp->bitmap[9*y + (x/8)] &= 255 - (1 << (7-(x%8)));
}

bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y) {
  int i=0;

  if (bmp->type == GSM_StartupLogo) i=(bmp->bitmap[((y/8)*84) + x] & 1<<((y%8)));
  if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo) i=(bmp->bitmap[9*y + (x/8)] & 1<<(7-(x%8)));
  
  /* Testing only ! */
  if (bmp->type == GSM_PictureImage) i=(bmp->bitmap[9*y + (x/8)] & 1<<(7-(x%8)));
 
  if (i) return true; else return false;
}

void GSM_ClearBitmap(GSM_Bitmap *bmp)
{
  int i;
  for (i=0;i<bmp->size;i++) bmp->bitmap[i]=0;
}


void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target)
{
  GSM_Bitmap new;
  int x,y;
      
  if (target==GSM_StartupLogo) {
    new.width=84;
    new.height=48;
  }
  if (target==GSM_OperatorLogo || target==GSM_CallerLogo) {
    new.width=72;
    new.height=14;
  }
  if (target==GSM_PictureImage) {
    new.width=72;
    new.height=28;
  }
  new.type=target;
  new.size=new.width*new.height/8;
  
#ifdef DEBUG
  if (bitmap->width>new.width)
    fprintf(stdout,_("We lost some part of image - it's cut (width from %i to %i) !\n"),backup.width,width);
#endif /* DEBUG */

#ifdef DEBUG
  if (bitmap->height<height)
    fprintf(stdout,_("We lost some part of image - it's cut (height from %i to %i) !\n"),backup.height,height);
#endif /* DEBUG */
  
  GSM_ClearBitmap(&new);
  
  for (y=0;y<new.height;y++) {
    for (x=0;x<new.width;x++)
      if (GSM_IsPointBitmap(bitmap,x,y)) GSM_SetPointBitmap(&new,x,y);
  }
  
  memcpy(bitmap,&new,sizeof(GSM_Bitmap));
}

void GSM_PrintBitmap(GSM_Bitmap *bitmap)
{
  int x,y;

  for (y=0;y<bitmap->height;y++) {
    for (x=0;x<bitmap->width;x++) {
      if (GSM_IsPointBitmap(bitmap,x,y)) {
        fprintf(stdout, _("#"));
      } else {
        fprintf(stdout, _(" "));
      }
    }
    fprintf(stdout, _("\n"));
  }
}


GSM_Error GSM_ReadSMSBitmap(GSM_SMSMessage *message, GSM_Bitmap *bitmap)
{
  int offset = 1;

  switch (message->UDHType) {
  case GSM_OpLogo:
    if (message->Length!=133+7) return GE_UNKNOWN;
    
    bitmap->type = GSM_OperatorLogo;

    bitmap->netcode[0] = '0' + (message->MessageText[0] & 0x0f);
    bitmap->netcode[1] = '0' + (message->MessageText[0] >> 4);
    bitmap->netcode[2] = '0' + (message->MessageText[1] & 0x0f);
    bitmap->netcode[3] = ' ';
    bitmap->netcode[4] = '0' + (message->MessageText[2] & 0x0f);
    bitmap->netcode[5] = '0' + (message->MessageText[2] >> 4);
    bitmap->netcode[6] = 0;

    offset = 4;
    break;

  case GSM_CallerIDLogo:
    if (message->Length!=130+7) return GE_UNKNOWN;
    bitmap->type=GSM_CallerLogo;
    break;
  default: /* error */
    return GE_UNKNOWN;
    break;
  }
  bitmap->width = message->MessageText[offset];
  bitmap->height = message->MessageText[offset + 1];
  
  if (bitmap->width!=72 || bitmap->height!=14) return GE_INVALIDIMAGESIZE;
  
  bitmap->size = (bitmap->width * bitmap->height) / 8;
  memcpy(bitmap->bitmap, message->MessageText + offset + 3, bitmap->size);

#ifdef DEBUG
  fprintf(stdout, _("Bitmap from SMS: width %i, height %i\n"),bitmap->width,bitmap->height);
#endif

  return GE_NONE;
}


/* Returns message length */

int GSM_SaveSMSBitmap(GSM_SMSMessage *message, GSM_Bitmap *bitmap)
{
  int current=0;
  
  char UserDataHeader[7] = {	0x06, /* UDH Length */
		  		0x05, /* IEI: application port addressing scheme, 16 bit address */
		  		0x04, /* IEI length */
                		0x15, /* destination address: high byte */
				0x00, /* destination address: low byte */
                		0x00, /* originator address */
				0x00};

  char Data[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  
  /* Default settings for SMS message:
      - no delivery report
      - Class Message 1
      - no compression
      - 8 bit data
      - SMSC no. 1
      - validity 3 days
      - set UserDataHeaderIndicator
  */

  message->Type = GST_MO;
  message->Class = 1;
  message->Compression = false;
  message->EightBit = true;
  message->MessageCenter.No = 1;
  message->Validity = 4320; /* 4320 minutes == 72 hours */
  message->ReplyViaSameSMSC = false;

  switch (bitmap->type) {
    case GSM_OperatorLogo:
      message->UDHType = GSM_OpLogo;
      UserDataHeader[4] = 0x82; /* NBS port 0x1582 */
      
      /* Set the network code */
      Data[current++] = ((bitmap->netcode[1] & 0x0f) << 4) | (bitmap->netcode[0] & 0xf);
      Data[current++] = 0xf0 | (bitmap->netcode[2] & 0x0f);
      Data[current++] = ((bitmap->netcode[5] & 0x0f) << 4) | (bitmap->netcode[4] & 0xf);

      break;
    case GSM_CallerLogo:
      message->UDHType = GSM_CallerIDLogo;
      UserDataHeader[4] = 0x83; /* NBS port 0x1583 */
      break;
    default: /* error */
      break;
  }
        
  /* Set the logo size */
  current++;
  Data[current++] = bitmap->width;
  Data[current++] = bitmap->height;

  Data[current++] = 0x01;

  memcpy(message->UDH,UserDataHeader,7);
  memcpy(message->MessageText,Data,current);
  memcpy(message->MessageText+current,bitmap->bitmap,bitmap->size);

  return current;
}
