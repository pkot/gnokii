/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copytight (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the 7110 series. 
  See README for more details on supported mobile phones.

  The various routines are called P7110_(whatever).

  $Log$
  Revision 1.1  2001-01-14 22:46:59  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define __phone_7110_c  /* Turn on prototypes in phone-7110.h */
#include "misc.h"
#include "gsm-common.h"
#include "phone-generic.h"
#include "phone-7110.h"
#include "fbus-generic.h"


/* Some globals */
/* Note that these could be created in initialise */
/* which would enable multiphone support */


GSM_Link link;
GSM_IncomingFunctionType P7110_IncomingFunctions[] = {
  { 0x03, P7110_GenericCRHandler },
  { 0x17, P7110_GenericCRHandler },
  { 0x1b, P7110_GenericCRHandler }
};
GSM_Phone phone = {
   3,  /* No of functions in array */
   P7110_IncomingFunctions
};


/* Mobile phone information */

GSM_Information P7110_Information = {
  "7110|6210", /* Supported models */
  7,                     /* Max RF Level */
  0,                     /* Min RF Level */
  GRF_Arbitrary,         /* RF level units */
  7,                     /* Max Battery Level */
  0,                     /* Min Battery Level */
  GBU_Arbitrary,         /* Battery level units */
  GDT_DateTime,          /* Have date/time support */
  GDT_TimeOnly,	         /* Alarm supports time only */
  1                      /* Alarms available - FIXME */
};


/* Here we initialise model specific functions called by 'gnokii'. */
/* This too needs fixing .. perhaps pass the link a 'request' of certain */
/* type and the link then searches the phone functions.... */

GSM_Functions P7110_Functions = {
  P7110_Initialise,
  P7110_Terminate,
  P7110_ReadPhonebook, /* GetMemoryLocation */
  UNIMPLEMENTED, /* WritePhonebookLocation */
  UNIMPLEMENTED, /* GetSpeedDial */
  UNIMPLEMENTED, /* SetSpeedDial */
  P7110_GetMemoryStatus, /* GetMemoryStatus */
  UNIMPLEMENTED, /* GetSMSStatus */
  UNIMPLEMENTED, /* GetSMSCentre */
  UNIMPLEMENTED, /* SetSMSCentre */
  UNIMPLEMENTED, /* GetSMSMessage */
  UNIMPLEMENTED, /* DeleteSMSMessage */
  UNIMPLEMENTED, /* SendSMSMessage */
  UNIMPLEMENTED, /* SaveSMSMessage */
  UNIMPLEMENTED, /* GetRFLevel */
  P7110_GetBatteryLevel, /* GetBatteryLevel */
  UNIMPLEMENTED, /* GetPowerSource */
  UNIMPLEMENTED, /* GetDisplayStatus */
  UNIMPLEMENTED, /* EnterSecurityCode */
  UNIMPLEMENTED, /* GetSecurityCodeStatus */
  P7110_GetIMEI,        /* GetIMEI */
  P7110_GetRevision,    /* GetRevision */
  P7110_GetModel,       /* GetModel */
  UNIMPLEMENTED, /* GetDateTime */
  UNIMPLEMENTED, /* SetDateTime */
  UNIMPLEMENTED, /* GetAlarm */
  UNIMPLEMENTED, /* SetAlarm */
  UNIMPLEMENTED, /* DialVoice */
  UNIMPLEMENTED, /* DialData */
  UNIMPLEMENTED, /* GetIncomingCallNr */
  UNIMPLEMENTED, /* GetNetworkInfo */
  UNIMPLEMENTED, /* GetCalendarNote */
  UNIMPLEMENTED, /* WriteCalendarNote */
  UNIMPLEMENTED, /* DeleteCalendarNote */
  UNIMPLEMENTED, /* NetMonitor */
  UNIMPLEMENTED, /* SendDTMF */
  UNIMPLEMENTED, /* GetBitmap */
  UNIMPLEMENTED, /* SetBitmap */
  UNIMPLEMENTED, /* SetRingtone */
  UNIMPLEMENTED, /* SendRingtone */
  UNIMPLEMENTED, /* Reset */ 
  UNIMPLEMENTED, /* GetProfile */
  UNIMPLEMENTED, /* SetProfile */
  P7110_SendRLPFrame,   /* SendRLPFrame */
  UNIMPLEMENTED, /* CancelCall */
  UNIMPLEMENTED, /* EnableDisplayOutput */
  UNIMPLEMENTED, /* DisableDisplayOutput */
  UNIMPLEMENTED, /* EnableCellBroadcast */
  UNIMPLEMENTED, /* DisableCellBroadcast */
  UNIMPLEMENTED  /* ReadCellBroadcast */
};

/* LinkOK is always true for now... */
bool P7110_LinkOK=true;

void P7110_Terminate()
{
  
};


void P7110_DebugMessage(unsigned char *mes, int len)
{
  int i;
  
  fprintf(stdout,"Message debug:\n\r");
  for(i=0;i<len;i++) 
    if (isprint(mes[i]))
      fprintf(stdout, "[%02x%c]", mes[i], mes[i]);
    else
      fprintf(stdout, "[%02x ]", mes[i]);
  fprintf(stdout,"\n\r");
}



/* Simple unicode stuff from Marcin for now */
/*
Simple UNICODE decoding and encoding from/to iso-8859-2
Martin Kacer <M.Kacer@sh.cvut.cz>

Following table contains triplets:
first unicode byte, second unicode byte, iso-8859-2 character

If character is not found, first unicode byte is set to zero
and second one is the same as iso-8859-2 character.
*/
static const char unicode_table[][3] =
{
	/* C< D< E< N< R< S< T< Uo Z< */
	{0x01, 0x0C, 0xC8}, {0x01, 0x0E, 0xCF}, {0x01, 0x1A, 0xCC},
	{0x01, 0x47, 0xD2}, {0x01, 0x58, 0xD8}, {0x01, 0x60, 0xA9},
	{0x01, 0x64, 0xAB}, {0x01, 0x6E, 0xD9}, {0x01, 0x7D, 0xAE},
	/* c< d< e< n< r< s< t< uo z< */
	{0x01, 0x0D, 0xE8}, {0x01, 0x0F, 0xEF}, {0x01, 0x1B, 0xEC},
	{0x01, 0x48, 0xF2}, {0x01, 0x59, 0xF8}, {0x01, 0x61, 0xB9},
	{0x01, 0x65, 0xBB}, {0x01, 0x6F, 0xF9}, {0x01, 0x7E, 0xBE},
	/* A< A, C' D/ E, L< L' L/ */
	{0x01, 0x02, 0xC3}, {0x01, 0x04, 0xA1}, {0x01, 0x06, 0xC6},
	{0x01, 0x10, 0xD0}, {0x01, 0x18, 0xCA}, {0x01, 0x3D, 0xA5},
	{0x01, 0x39, 0xC5}, {0x01, 0x41, 0xA3},
	/* N' O" R' S' S, T, U" Z' Z. */
	{0x01, 0x43, 0xD1}, {0x01, 0x50, 0xD5}, {0x01, 0x54, 0xC0},
	{0x01, 0x5A, 0xA6}, {0x01, 0x5E, 0xAA}, {0x01, 0x62, 0xDE},
	{0x01, 0x70, 0xDB}, {0x01, 0x79, 0xAC}, {0x01, 0x7B, 0xAF},
	/* a< a, c' d/ e, l< l' l/ */
	{0x01, 0x03, 0xE3}, {0x01, 0x05, 0xB1}, {0x01, 0x07, 0xE6},
	{0x01, 0x11, 0xF0}, {0x01, 0x19, 0xEA}, {0x01, 0x3E, 0xB5},
	{0x01, 0x3A, 0xE5}, {0x01, 0x42, 0xB3},
	/* n' o" r' s' s, t, u" z' z. */
	{0x01, 0x44, 0xF1}, {0x01, 0x51, 0xF5}, {0x01, 0x55, 0xE0},
	{0x01, 0x5B, 0xB6}, {0x01, 0x5F, 0xBA}, {0x01, 0x63, 0xFE},
	{0x01, 0x71, 0xFB}, {0x01, 0x7A, 0xBC}, {0x01, 0x7C, 0xBF},
	{0x00, 0x00, 0x00}
};

/* Makes "normal" string from Unicode.
   Dest - where to save destination string,
   Src - where to get string from,
   Len - len of string AFTER converting */
static void UnicodeDecode (char* dest, const char* src, int len)
{
	int i, j;
	char ch;

	for ( i = 0;  i < len;  ++i )
	{
		ch = src[i*2+1];   /* default is to cut off the first byte */
		for ( j = 0;  unicode_table[j][2] != 0x00;  ++j )
			if ( src[i*2] == unicode_table[j][0] &&
			     src[i*2+1] == unicode_table[j][1] )
				ch = unicode_table[j][2];
		dest[i] = ch;
	}
	dest[len] = '\0';
}





bool P7110_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx) 
{
  return false;
}

GSM_Error P7110_Initialise(char *port_device, char *initlength,
		 GSM_ConnectionType connection,
		 void (*rlp_callback)(RLP_F96Frame *frame))
{
  strncpy(link.PortDevice,port_device,20);
  link.InitLength=atoi(initlength);
  link.ConnectionType=connection;
  FBUS_Initialise(&link, &phone);

  /* Now do any phone specific init */

  return GE_NONE;
}

GSM_Error P7110_GenericCRHandler(int messagetype, char *buffer, int length)
{
  return PGEN_CommandResponseReceive(&link, messagetype, buffer, length);
}

GSM_Error P7110_GetIMEI(char *imei)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x01};
  int len=4;

  if (PGEN_CommandResponse(&link,req,&len,0x1b,0x1b,100)==GE_NONE) {
    snprintf(imei,P7110_MAX_IMEI_LENGTH,"%s",req+4);
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}

GSM_Error P7110_GetModel(char *model)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};
  int len=6;

  if (PGEN_CommandResponse(&link,req,&len,0x1b,0x1b,100)==GE_NONE) {
    snprintf(model,P7110_MAX_MODEL_LENGTH,"%s",req+22);
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


GSM_Error P7110_GetRevision(char *revision)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x03, 0x01, 0x32};
  int len=6;

  if (PGEN_CommandResponse(&link,req,&len,0x1b,0x1b,100)==GE_NONE) {
    snprintf(revision,P7110_MAX_REVISION_LENGTH,"%s",req+7);
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}

GSM_Error P7110_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x02};
  int len=4;

  if (PGEN_CommandResponse(&link,req,&len,0x17,0x17,100)==GE_NONE) {
    *units=GBU_Percentage;
    *level=req[5];
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


GSM_Error P7110_GetMemoryStatus(GSM_MemoryStatus *status)
{
  unsigned char req[100] = {FBUS_FRAME_HEADER, 0x03, 0x00, 0x00};
  int len=6;

  req[5] = P7110_GetMemoryType(status->MemoryType);

  if (PGEN_CommandResponse(&link,req,&len,0x03,0x03,100)==GE_NONE) {
    if (req[5]!=0xff) {
      status->Used=(req[16]<<8)+req[17];
      status->Free=((req[14]<<8)+req[15])-status->Used;
#ifdef DEBUG
      fprintf(stderr,"Memory status - location = %d\n\r",(req[8]<<8)+req[9]);
#endif
      return GE_NONE;
    } else return GE_NOTIMPLEMENTED;
  }
  else return GE_NOTIMPLEMENTED;
}



int P7110_GetMemoryType(GSM_MemoryType memory_type)
{

  int result;

  switch (memory_type) {
     case GMT_MT:
	result = P7110_MEMORY_MT;
        break;
     case GMT_ME:
	result = P7110_MEMORY_ME;
        break;
     case GMT_SM:
	result = P7110_MEMORY_SM;
        break;
     case GMT_FD:
	result = P7110_MEMORY_FD;
        break;
     case GMT_ON:
	result = P7110_MEMORY_ON;
        break;
     case GMT_EN:
	result = P7110_MEMORY_EN;
        break;
     case GMT_DC:
	result = P7110_MEMORY_DC;
        break;
     case GMT_RC:
	result = P7110_MEMORY_RC;
        break;
     case GMT_MC:
	result = P7110_MEMORY_MC;
        break;
     default:
        result=P7110_MEMORY_XX;
   }

   return (result);
}



GSM_Error P7110_ReadPhonebook(GSM_PhonebookEntry *entry)
{
  unsigned char req[2000] = {FBUS_FRAME_HEADER, 0x07, 0x01, 0x01, 0x00, 0x01,
                         0x00, 0x00 , /* memory type */ //02,05
                         0x00, 0x00,  /* location */
                         0x00, 0x00};
  int len=14;
  int blocks, blockcount, i;
  char *pBlock;

  req[9] = P7110_GetMemoryType(entry->MemoryType);
  req[10] = (entry->Location>>8);
  req[11] = entry->Location & 0xff;

  if (PGEN_CommandResponse(&link,req,&len,0x03,0x03,2000)==GE_NONE) {

    entry->Empty = true;
    entry->Group = 0;
    entry->Name[0] = '\0';
    entry->Number[0] = '\0';
    entry->SubEntriesCount = 0;
    
    entry->Date.Year = 0;
    entry->Date.Month = 0;
    entry->Date.Day = 0;
    entry->Date.Hour = 0;
    entry->Date.Minute = 0;
    entry->Date.Second = 0;
    
    if( req[6] == 0x0f ) // not found
      {
        if( req[10] == 0x34 ) // not found because inv. location
          return GE_INVALIDPHBOOKLOCATION;
        else return GE_NONE;
      }
    
    blocks     = req[17];
    blockcount = 0;
    
    entry->SubEntriesCount = blocks - 1;

#ifdef DEBUG
      fprintf(stderr, _("Message: Phonebook entry received:\n"));
      fprintf(stderr, _(" Blocks: %d\n"),blocks);
#endif /* DEBUG */
        
    pBlock = req+18;
    
    for( i = 0; i < blocks; i++ )
      {
        GSM_SubPhonebookEntry* pEntry = &entry->SubEntries[blockcount];
	
        switch( pBlock[0] )
	  {
	  case P7110_ENTRYTYPE_NAME:
	    UnicodeDecode (entry->Name, pBlock+6, pBlock[5]/2);
	    entry->Empty = false;
#ifdef DEBUG
	    fprintf(stderr, _("   Name: %s\n"), entry->Name);
#endif /* DEBUG */
	    break;
	  case P7110_ENTRYTYPE_NUMBER:
	    pEntry->EntryType   = pBlock[0];
	    pEntry->NumberType  = pBlock[5];
	    pEntry->BlockNumber = pBlock[4];
	    
	    UnicodeDecode (pEntry->data.Number, pBlock+10, pBlock[9]/2);

	    if( blockcount == 0 )
	      strcpy( entry->Number, pEntry->data.Number );
#ifdef DEBUG
	    fprintf(stderr, _("   Type: %d (%02x)\n"),
		    pEntry->NumberType,
		    pEntry->NumberType);
	    fprintf(stderr, _(" Number: %s\n"),
		    pEntry->data.Number);
#endif /* DEBUG */
	    blockcount++;
	    break;
	  case P7110_ENTRYTYPE_DATE:
	    pEntry->EntryType        = pBlock[0];
	    pEntry->NumberType       = pBlock[5];
	    pEntry->BlockNumber      = pBlock[4];
	    pEntry->data.Date.Year   = (pBlock[6]<<8) + pBlock[7];
	    pEntry->data.Date.Month  = pBlock[8];
	    pEntry->data.Date.Day    = pBlock[9];
	    pEntry->data.Date.Hour   = pBlock[10];
	    pEntry->data.Date.Minute = pBlock[11];
	    pEntry->data.Date.Second = pBlock[12];
#ifdef DEBUG
	    fprintf(stderr, _("   Date: %02u.%02u.%04u\n"), pEntry->data.Date.Day,
		    pEntry->data.Date.Month, pEntry->data.Date.Year );
	    fprintf(stderr, _("   Time: %02u:%02u:%02u\n"), pEntry->data.Date.Hour,
		    pEntry->data.Date.Minute, pEntry->data.Date.Second);
#endif /* DEBUG */
	    blockcount++;
	    break;
	  case P7110_ENTRYTYPE_NOTE:
	  case P7110_ENTRYTYPE_POSTAL:
	  case P7110_ENTRYTYPE_EMAIL:
	    pEntry->EntryType   = pBlock[0];
	    pEntry->NumberType  = 0;
	    pEntry->BlockNumber = pBlock[4];
	    
	    UnicodeDecode (pEntry->data.Number, pBlock+6, pBlock[5]/2);
	    
#ifdef DEBUG
	    fprintf(stderr, _("   Type: %d (%02x)\n"),
		    pEntry->EntryType,
		    pEntry->EntryType);
	    fprintf(stderr, _("   Text: %s\n"),
		    pEntry->data.Number);
#endif /* DEBUG */
	    blockcount++;
	    break;
	  case P7110_ENTRYTYPE_GROUP:
	    entry->Group = pBlock[5];
#ifdef DEBUG
	    fprintf(stderr, _("   Group: %d\n"), entry->Group);
#endif /* DEBUG */
	    break;
	  default:
#ifdef DEBUG
	    fprintf(stderr, _("Unknown Entry Code (%u) received.\n"), pBlock[0] );
#endif /* DEBUG */
	    break;
	  }

        pBlock+=pBlock[3];
      }
    
    entry->SubEntriesCount = blockcount;    
    
    return GE_NONE;
  }
  else return GE_NOTIMPLEMENTED;
}


