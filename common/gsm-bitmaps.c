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
#include "gsm-api.h"

/* A few useful functions for bitmaps */

void GSM_SetPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	if (bmp->type == GSM_StartupLogo) bmp->bitmap[((y/8)*bmp->width)+x] |= 1 << (y%8);
	if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo) bmp->bitmap[(y*bmp->width+x)/8] |= 1 << (7-((y*bmp->width+x)%8));

	/* Testing only! */
	if (bmp->type == GSM_PictureImage) bmp->bitmap[9*y + (x/8)] |= 1 << (7-(x%8));
}

void GSM_ClearPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	if (bmp->type == GSM_StartupLogo) bmp->bitmap[((y/8)*bmp->width)+x] &= 255 - (1 << (y%8));
	if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo) bmp->bitmap[(y*bmp->width+x)/8] &= 255 - (1 << (7-((y*bmp->width+x)%8)));

	/* Testing only ! */
	if (bmp->type == GSM_PictureImage) bmp->bitmap[9*y + (x/8)] &= 255 - (1 << (7-(x%8)));
}

bool GSM_IsPointBitmap(GSM_Bitmap *bmp, int x, int y)
{
	int i = 0;

	if (bmp->type == GSM_StartupLogo) i = (bmp->bitmap[((y/8)*bmp->width) + x] & 1<<((y%8)));
	if (bmp->type == GSM_OperatorLogo || bmp->type == GSM_CallerLogo)
		i = (bmp->bitmap[(y*bmp->width+x)/8] & 1 << (7-((y*bmp->width+x)%8)));
	/* Testing only ! */
	if (bmp->type == GSM_PictureImage) i = (bmp->bitmap[9*y + (x/8)] & 1<<(7-(x%8)));
 
	if (i) return true;
	else return false;
}

void GSM_ClearBitmap(GSM_Bitmap *bmp)
{
	int i;

	for (i = 0; i < bmp->size; i++) bmp->bitmap[i] = 0;
}


void GSM_ResizeBitmap(GSM_Bitmap *bitmap, GSM_Bitmap_Types target, GSM_Information *info)
{
	GSM_Bitmap backup;
	int x,y,copywidth,copyheight;

	/* Copy into the backup */
	memcpy(&backup,bitmap,sizeof(GSM_Bitmap));
      
	if (target==GSM_StartupLogo) {
		bitmap->width=info->StartupLogoW;
		bitmap->height=info->StartupLogoH;
		bitmap->size=((bitmap->height/8)+(bitmap->height%8>0))*bitmap->width;
	}
	if (target==GSM_OperatorLogo) {
		bitmap->width=info->OpLogoW;
		bitmap->height=info->OpLogoH;
		x=bitmap->width*bitmap->height;
		bitmap->size=(x/8)+(x%8>0);
	}
	if (target==GSM_CallerLogo) {
		bitmap->width=info->CallerLogoW;
		bitmap->height=info->CallerLogoH;
		x=bitmap->width*bitmap->height;
		bitmap->size=(x/8)+(x%8>0);
	}
	if (target==GSM_PictureImage) {
		bitmap->width=72;
		bitmap->height=28;
		bitmap->size=bitmap->width*bitmap->height/8;
	}
	bitmap->type=target;

	if (backup.width>bitmap->width) {
		copywidth=bitmap->width;
#ifdef DEBUG
		fprintf(stdout,_("We lost some part of image - it's cut (width from %i to %i) !\n"),backup.width,bitmap->width);
#endif /* DEBUG */
	} else copywidth=backup.width;

	if (backup.height>bitmap->height) {
		copyheight=bitmap->height;
#ifdef DEBUG
		fprintf(stdout,_("We lost some part of image - it's cut (height from %i to %i) !\n"),backup.height,bitmap->height);
#endif /* DEBUG */
	} else copyheight=backup.height;
  

	GSM_ClearBitmap(bitmap);
  
	for (y=0;y<copyheight;y++) {
		for (x=0;x<copywidth;x++)
			if (GSM_IsPointBitmap(&backup,x,y)) GSM_SetPointBitmap(bitmap,x,y);
	}
}

void GSM_PrintBitmap(GSM_Bitmap *bitmap)
{
	int x, y;

	for (y = 0; y < bitmap->height; y++) {
		for (x = 0; x < bitmap->width; x++) {
			if (GSM_IsPointBitmap(bitmap, x, y)) {
				fprintf(stdout, "#");
			} else {
				fprintf(stdout, " ");
			}
		}
		fprintf(stdout, "\n");
	}
}


GSM_Error GSM_ReadSMSBitmap(int type, char *message, char *code, GSM_Bitmap *bitmap)
{
	int offset = 0;

	switch (type) {
	case SMS_OpLogo:
		if (!code) return GE_UNKNOWN;

		bitmap->type = SMS_OpLogo;

		bitmap->netcode[0] = '0' + (message[0] & 0x0f);
		bitmap->netcode[1] = '0' + (message[0] >> 4);
		bitmap->netcode[2] = '0' + (message[1] & 0x0f);
		bitmap->netcode[3] = ' ';
		bitmap->netcode[4] = '0' + (message[2] & 0x0f);
		bitmap->netcode[5] = '0' + (message[2] >> 4);
		bitmap->netcode[6] = 0;

		break;
	case SMS_CallerIDLogo:
		bitmap->type = SMS_CallerIDLogo;
		break;
	case SMS_Picture:
		offset = 2;
		bitmap->type = GSM_PictureImage;
		break;
	default: /* error */
		return GE_UNKNOWN;
		break;
	}
	bitmap->width = message[0];
	bitmap->height = message[1];
  
	bitmap->size = (bitmap->width * bitmap->height) / 8;
	memcpy(bitmap->bitmap, message + offset + 2, bitmap->size);

	dprintf("Bitmap from SMS: width %i, height %i\n", bitmap->width, bitmap->height);

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

	message->Status = SMS_Sent;
	/* Data Coding Scheme */
	message->DCS.Type = SMS_GeneralDataCoding;
	message->DCS.u.General.Class = 2;
	message->DCS.u.General.Compressed = false;
	message->DCS.u.General.Alphabet = SMS_8bit;


	message->MessageCenter.No = 1;
	message->Validity.VPF = SMS_RelativeFormat;
	message->Validity.u.Relative = 4320; /* 4320 minutes == 72 hours */
	message->ReplyViaSameSMSC = false;

	switch (bitmap->type) {
	case GSM_OperatorLogo:
		message->UDH[0].Type = SMS_OpLogo;
		UserDataHeader[4] = 0x82; /* NBS port 0x1582 */
      
		/* Set the network code */
		Data[current++] = ((bitmap->netcode[1] & 0x0f) << 4) | (bitmap->netcode[0] & 0xf);
		Data[current++] = 0xf0 | (bitmap->netcode[2] & 0x0f);
		Data[current++] = ((bitmap->netcode[5] & 0x0f) << 4) | (bitmap->netcode[4] & 0xf);

		break;
	case GSM_CallerLogo:
		message->UDH[0].Type = SMS_CallerIDLogo;
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

	memcpy(message->MessageText, UserDataHeader, 7);
	memcpy(message->MessageText, Data, current);
	memcpy(message->MessageText+current, bitmap->bitmap, bitmap->size);

	return current;
}
