/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This is the main part of 640 support.
	
  Last modification: Fri May 19 15:31:26 EST 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#ifndef WIN32

#define		__mbus_640_c	/* "Turn on" prototypes in mbus-640.h */

#include	"misc.h"

#include	<termios.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/ioctl.h>

#if __unices__
#  include <sys/file.h>
#endif

#include	<string.h>
#include	<pthread.h>
#include	<errno.h>

#include	"gsm-common.h"
#include	"mbus-640.h"
#include	"phones/nokia.h"

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					MB640_LinkOK;
char          PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
char          *MB640_Revision = 0,
              *MB640_RevisionDate = 0,
              *MB640_Model = 0,
              MB640_VersionInfo[64];

GSM_Functions			MB640_Functions = {
		MB640_Initialise,
		MB640_Terminate,
		MB640_GetMemoryLocation,
		MB640_WritePhonebookLocation,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		MB640_GetMemoryStatus,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
 		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		MB640_GetRFLevel,
		MB640_GetBatteryLevel,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		MB640_GetIMEI,
		MB640_GetRevision,
		MB640_GetModel,
		PNOK_GetManufacturer,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		MB640_GetBitmap,
		MB640_SetBitmap,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		MB640_Reset,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		MB640_SendRLPFrame,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED,
		UNIMPLEMENTED
};

	/* FIXME - these are guesses only... */
GSM_Information			MB640_Information = {
		"640",					/* Models */
		4, 						/* Max RF Level */
		0,						/* Min RF Level */
		GRF_Arbitrary,			/* RF level units */
		4,    				/* Max Battery Level */
		0,						/* Min Battery Level */
		GBU_Arbitrary,			/* Battery level units */
		GDT_None,				/* No date/time support */
		GDT_None,				/* No alarm support */
		0,						/* Max alarms = 0 */
		0, 0,                   /* Startup logo size */
		0, 0,                   /* Op logo size */
		0, 0                    /* Caller logo size */
};

char MB640_2_KOI8[] = 
{
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ','!','"','#','$','&','%','\'','(',')','*','+',',','-','.','/',
  '0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',

  '!','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
  'P','Q','R','S','T','U','V','W','X','Y','Z','[','\\',']','^','_',
  '?','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
  'p','q','r','s','t','u','v','w','x','y','z','{','|','}','~',' ',
  ' ',' ',' ','á','â','÷','ç','ä','å','ö','ú','é','ê','ë','ì','í',
  'î','ï','ð','ò','ó','ô','õ','æ','è','ã','þ','û','ý','ÿ','ù','ø',
  ' ',' ',' ','Á','Â','×','Ç','Ä','Å','Ö','Ú','É','Ê','Ë','Ì','Í',
  'Î','Ï','Ð','Ò','Ó','Ô','Õ','Æ','È','Ã','Þ','Û','Ý','ß','Ù','Ø',

  ' ','ü','à','ñ',' ','Ü','À','Ñ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ','E',' ',' ','@','$','L','Y',' ',
};

char KOI8_2_MB640[] = 
{
   ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
   ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
   ' ','!','"','#',0xFC,'&','%','\'','(',')','*','+',',','-','.','/',
   '0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',

   0xFB,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
   'P','Q','R','S','T','U','V','W','X','Y','Z','[','\\',']','^','_',
   '?','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
   'p','q','r','s','t','u','v','w','x','y','z','{','|','}','~',' ',

   ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
   ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
   ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
   ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
/*  À    Á    Â    Ã    Ä    Å    Æ    Ç    È    É    Ê    Ë    Ì    Í    Î    Ï*/
  0xC6,0xA3,0xA4,0xB9,0xA7,0xA8,0xB7,0xA6,0xB8,0xAB,0xBC,0xAD,0xAE,0xAF,0xB0,0xB1,
/*  Ð    Ñ    Ò    Ó    Ô    Õ    Ö    ×    Ø    Ù    Ú    Û    Ü    Ý    Þ    ß*/
  0xB2,0xC7,0xB3,0xB4,0xB5,0xB6,0xA9,0xA5,0xBD,0xBE,0xAA,0xBB,0xC5,0xBC,0xBA,0xBF,
/*  à    á    â    ã    ä    å    æ    ç    è    é    ê    ë    ì    í    î    ï*/
  0xC2,0x83,0x84,0x99,0x87,0x88,0x97,0x86,0x98,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,
/*  ð    ñ    ò    ó    ô    õ    ö    ÷    ø    ù    ú    û    ü    ý    þ    ÿ*/
  0x92,0xC3,0x93,0x94,0x95,0x96,0x89,0x85,0x9D,0x9E,0x8A,0x9B,0xC1,0x9C,0x9A,0x9F,
};

/* Local variables */
pthread_t      Thread;
bool					 RequestTerminate;
int            PortFD;
struct termios old_termios; /* old termios */
u8             MB640_TXPacketNumber = 0x00;
char           Model[MB640_MAX_MODEL_LENGTH];
bool           ModelValid     = false;
unsigned char  PacketData[256];
bool           MB640_ACKOK    = false,
               MB640_PacketOK = false,
               MB640_EchoOK   = false;

	/* The following functions are made visible to gsm-api.c and friends. */

	/* Initialise variables and state machine. */
GSM_Error   MB640_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame))
{int rtn;

	/* ConnectionType is ignored in 640 code. */
  RequestTerminate = false;
	MB640_LinkOK     = false;
  memset(MB640_VersionInfo,0,sizeof(MB640_VersionInfo));
  strncpy(PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);
	rtn = pthread_create(&Thread, NULL, (void *) MB640_ThreadLoop, (void *)NULL);
  if(rtn == EAGAIN || rtn == EINVAL)
  {
    return (GE_INTERNALERROR);
  }
	return (GE_NONE);
}

	/* Applications should call MB640_Terminate to close the serial port. */
void MB640_Terminate(void)
{
  /* Request termination of thread */
  RequestTerminate = true;
  /* Now wait for thread to terminate. */
  pthread_join(Thread, NULL);
	/* Close serial port. */
  if( PortFD >= 0 )
  {
    tcsetattr(PortFD, TCSANOW, &old_termios);
    close( PortFD );
  }
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	MB640_GetMemoryLocation(GSM_PhonebookEntry *entry)
{int  timeout,i,len;
 u8   pkt[] = {0x0f, 0x2d, 3, 0, 7, 0x1f, 0x7f, 0xf0, 0, 0, 0, 0}, digit;
 char *digit_map = " 1234567890*#pw+";

  if(!entry->Location) return (GE_INVALIDPHBOOKLOCATION);
  switch(entry->MemoryType)
  {
    case GMT_ME:
/*      if(entry->Location > 100) return (GE_INVALIDPHBOOKLOCATION); */
      pkt[9] = entry->Location - 1;
      break;
    case GMT_LD: 
      if(entry->Location > 5) return (GE_INVALIDPHBOOKLOCATION);
      pkt[9] = entry->Location + 99;
      break;
    case GMT_ON: 
      if(entry->Location > 1) return (GE_INVALIDPHBOOKLOCATION);
      pkt[9] = 115;
      break;
    default: return (GE_INVALIDMEMORYTYPE);
  }

  MB640_PacketOK = false;
  MB640_ACKOK    = false;
  timeout        = 3;
  while(!MB640_PacketOK)
  {
    if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
    if(!--timeout || RequestTerminate)
    {
      return(GE_TIMEOUT);
    }
    usleep(100000);
  }
  entry->Empty = (PacketData[18] == 0 && PacketData[34] == 0);
  if( !entry->Empty )
  {
    for( i = 0; PacketData[34 + i] && i < 16; i++ )
    {
      entry->Name[i] = MB640_2_KOI8[ PacketData[34 + i] ];
    }
    entry->Name[i] = 0;

    len = PacketData[18];
    for( i = 0; i < len; i++ )
    {
      digit = PacketData[19 + i/2];
      entry->Number[i] = digit_map[((i % 2) ? digit : digit >> 4) & 0x0F];
    }
    entry->Number[i] = 0;
    entry->Group = PacketData[50];
  }
  else
  {
    entry->Name[0] = 0;
    entry->Number[0] = 0;
    entry->Group = 255;
  }
	return (GE_NONE);
}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	MB640_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{int timeout,i;

  switch(entry->MemoryType)
  {
    case GMT_ME:
      {u8 pkt[47], digit;
       char *s;

        pkt[0]  = 0x10;
        pkt[1]  = 0x08;
        pkt[2]  = 0x03;
        pkt[3]  = 0x00;
        pkt[4]  = 0x07;
        pkt[5]  = 0x1F;
        pkt[6]  = 0x7F;
        pkt[7]  = 0xF0;
        pkt[8]  = 0x00;
        pkt[9]  = entry->Location - 1;
        pkt[10] = 0x00;
        pkt[11] = 0x00;
        pkt[12] = 0x00;
        pkt[13] = 0x21;
        memset(&pkt[14],0,32);
        pkt[46] = 0x05/*entry->Group*/;

        for( i = 0; entry->Name[i] && i < 15; i++ )
        {
          pkt[30 + i] = KOI8_2_MB640[ (u8)entry->Name[i] ];
        }
        pkt[30 + i] = 0;

        
        for( i = 0, s = entry->Number; *s && i < 30; s++ )
        {
          switch(*s)
          {
            case '1'...'9': digit = *s - '0'; break;
            case '0':       digit = 0xA;      break;
            case '*':       digit = 0xB;      break;
            case '#':       digit = 0xC;      break;
            case 'p':       digit = 0xD;      break;
            case 'w':       digit = 0xE;      break;
            case '+':       digit = 0xF;      break;
            default: continue;
          }
          pkt[15 + i/2] |= (i % 2) ? digit : digit << 4;
          i++;
        }
        pkt[14] = i;

        /* And write it! */
        MB640_PacketOK = false;
        MB640_ACKOK    = false;
        timeout        = 3;
        while(!MB640_PacketOK)
        {
          if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
          if(!--timeout || RequestTerminate)
          {
            return(GE_TIMEOUT);
          }
          usleep(100000);
        }
      }
    break;
    default: return (GE_NOTIMPLEMENTED);
  }
  return (GE_NONE);
}

	/* MB640_GetRFLevel
	   FIXME (sort of...)
	   For now, GetRFLevel and GetBatteryLevel both rely
	   on data returned by the "keepalive" packets.  I suspect
	   that we don't actually need the keepalive at all but
	   will await the official doco before taking it out.  HAB19990511 */
GSM_Error	MB640_GetRFLevel(GSM_RFUnits *units, float *level)
{u8  pkt[] = 
     /*{0x0f, 0x5A, 3, 0, 7, 0x39, 0x7f, 0xf0, 0, 0, 0, 0}*/
     {0x19, 2, 1, 7}
     /*{0,3,0}*/;
 int timeout;

  MB640_PacketOK = false;
  MB640_ACKOK    = false;
  timeout        = 3;
  while(!MB640_PacketOK)
  {
    if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
    if(!--timeout || RequestTerminate)
    {
      return(GE_TIMEOUT);
    }
    usleep(100000);
  }
  *level = 0/*(float)(PacketData[6] * 256 + PacketData[7]) / 256.0*/;
	return (GE_NONE);
}

	/* MB640_GetBatteryLevel - get value from ADC #0
	   FIXME (see above...) */
GSM_Error	MB640_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{u8  pkt[] = {0x19, 2, 1, 0};
 int timeout;

  if (*units == GBU_Arbitrary)
  {
    MB640_PacketOK = false;
    MB640_ACKOK    = false;
    timeout        = 3;
    while(!MB640_PacketOK)
    {
      if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
      if(!--timeout || RequestTerminate)
      {
        return(GE_TIMEOUT);
      }
      usleep(100000);
    }
    *level = (float)(PacketData[6] * 256 + PacketData[7]) / 256.0;
    return (GE_NONE);
  }
  return (GE_INTERNALERROR);
}

/* Really there are no IMEI in NMT phones. Equivalent IMHO is phone
 * Serial Number */
GSM_Error	MB640_GetIMEI(char *imei)
{u8  pkt[] = {0x0f, 0x19, 3, 0, 0x01, 0x0b, 0, 0};
 int timeout;

  MB640_PacketOK = false;
  MB640_ACKOK    = false;
  timeout        = 3;
  while(!MB640_PacketOK)
  {
    if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
    if(!--timeout || RequestTerminate)
    {
      return(GE_TIMEOUT);
    }
    usleep(100000);
  }

  memcpy(imei,&PacketData[14],PacketData[13]);
  imei[PacketData[13]] = 0;
	return (GE_NONE);
}

GSM_Error MB640_GetVersionInfo()
{u8   pkt[] = {0, 3, 0};
 int  timeout;
 char *s = MB640_VersionInfo;

  MB640_PacketOK = false;
  MB640_ACKOK    = false;
  timeout        = 3;
  while(!MB640_PacketOK)
  {
    if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
    if(!--timeout || RequestTerminate)
    {
      return(GE_TIMEOUT);
    }
    usleep(100000);
  }

  strncpy( s, &PacketData[6], sizeof(MB640_VersionInfo) );

  for( MB640_Revision     = s; *s != 0x0A; s++ ) if( !*s ) goto out;
  *s++ = 0;
  for( MB640_RevisionDate = s; *s != 0x0A; s++ ) if( !*s ) goto out;
  *s++ = 0;
  for( MB640_Model        = s; *s != 0x0A; s++ ) if( !*s ) goto out;
  *s++ = 0;
out:
	return (GE_NONE);
}

GSM_Error	MB640_GetRevision(char *revision)
{GSM_Error err = GE_NONE;

  if(!MB640_Revision) err = MB640_GetVersionInfo();
  if(err == GE_NONE) strncpy(revision, MB640_Revision, 64);

	return err;
}

GSM_Error	MB640_GetModel(char *model)
{GSM_Error err = GE_NONE;

  if(!MB640_Model) err = MB640_GetVersionInfo();
  if(err == GE_NONE) strncpy(model, MB640_Model, 64);

	return err;
}

GSM_Error	MB640_GetMemoryStatus(GSM_MemoryStatus *Status)
{
  switch(Status->MemoryType)
  {
    case GMT_ME:
      Status->Used = 0;
      Status->Free = 100;
    break;
    case GMT_LD:
      Status->Used = 5;
      Status->Free = 0;
    break;
    case GMT_ON:
      Status->Used = 1;
      Status->Free = 0;
    break;
    case GMT_SM:
      Status->Used = 0;
      Status->Free = 0;
    break;
    default: return (GE_NOTIMPLEMENTED);
  }
	return (GE_NONE);
}

GSM_Error	MB640_GetBitmap(GSM_Bitmap *Bitmap)
{int timeout;

  switch(Bitmap->type)
  {
    case GSM_StartupLogo:
    {u8  pkt[] = {0x0f, 0x60, 3, 0, 7, 0x3A, 0x7f, 0xf0, 0, 0, 0, 0};
     int i;

      for(i = 0; i < 6; i++)
      {
        pkt[9] = i;
        MB640_PacketOK = false;
        MB640_ACKOK    = false;
        timeout        = 10;
        while(!MB640_PacketOK)
        {
          if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
          if(!--timeout || RequestTerminate)
          {
            return(GE_TIMEOUT);
          }
          usleep(100000);
        }
        memcpy(&Bitmap->bitmap[i * 84],&PacketData[18],84);
      }
      Bitmap->width  = 84;
      Bitmap->height = 48;
      Bitmap->size   = 84*48/8;
    }
    break;
    default: return (GE_NOTIMPLEMENTED);
  }
	return (GE_NONE);
}

GSM_Error	MB640_SetBitmap(GSM_Bitmap *Bitmap)
{int timeout,i;

  switch(Bitmap->type)
  {
    case GSM_StartupLogo:
      {u8 pkt[98];

        pkt[0]  = 0x10;
        pkt[1]  = 0x08;
        pkt[2]  = 0x03;
        pkt[3]  = 0x00;
        pkt[4]  = 0x07;
        pkt[5]  = 0x3A;
        pkt[6]  = 0x7F;
        pkt[7]  = 0xF0;
        pkt[8]  = 0x00;
        pkt[10] = 0x00;
        pkt[11] = 0x00;
        pkt[12] = 0x00;
        pkt[13] = 0x54;

        for(i = 0; i < 6; i++)
        {
          pkt[9]  = i;
          memcpy(&pkt[14],&Bitmap->bitmap[i * 84],84);

          MB640_PacketOK = false;
          MB640_ACKOK    = false;
          timeout        = 10;
          while(!MB640_PacketOK)
          {
            if(!MB640_ACKOK) MB640_SendPacket(pkt, sizeof(pkt));
            if(!--timeout || RequestTerminate)
            {
              return(GE_TIMEOUT);
            }
            usleep(100000);
          }
        }
      }
    break;
    default: return (GE_NOTIMPLEMENTED);
  }
  return (GE_NONE);
}

GSM_Error	MB640_Reset(unsigned char type)
{u8        pkt[] = { 0x43, 0x00, 0x00 };
 int       timeout;

  /* send packet */
  MB640_PacketOK = false;
  MB640_ACKOK    = false;
  timeout        = 3;
  while(!MB640_PacketOK)
  {
    if(!MB640_ACKOK) MB640_SendPacket(pkt, 4);
    if(!--timeout) return (GE_TIMEOUT);
    usleep(250000);
  }
  return (GE_NONE);
}

bool		MB640_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
    return (false);
}

	/* Everything from here down is internal to 640 code. */

/* Checksum calculation */
u8 MB640_GetChecksum( u8 * packet )
{u8           checksum = 0;
 unsigned int i,len = packet[2];

  if( packet[2] == 0x7F ) len = 4;   /* ACK packet length                 */
  else                    len += 5;  /* Add sizes of header, command and  *
                                      * packet_number to packet length    */
  for( i = 0; i < len; i++ ) checksum ^= packet[i]; /* calculate checksum */
  return checksum;
}

/* Handler called when characters received from serial port. 
 * and process them. */
void MB640_SigHandler(int status)
{unsigned char        buffer[256],ack[5],b;
 int                  i,res;
 static unsigned int  Index = 0,
                      Length = 5;
 static unsigned char pkt[256];
#ifdef DEBUG
 int                  j;
#endif

  res = read(PortFD, buffer, 256);
  if( res < 0 ) return;
  for(i = 0; i < res ; i++)
  {
    b = buffer[i];
    if(!Index && b != 0x00 && b != 0xE9)
    {
      /* something strange goes from phone. Just ignore it */
#ifdef DEBUG
      fprintf( stdout, "Get [%02X %c]\n", b, b >= 0x20 ? b : '.' );
#endif /* DEBUG */
      continue;
    }
    else
    {
      pkt[Index++] = b;
      if(Index == 3) Length = (b == 0x7F) ? 5 : b + 6;
      if(Index >= Length)
      {
        if(pkt[0] == 0xE9 && pkt[1] == 0x00) /* packet from phone */
        {
#ifdef DEBUG
          fprintf( stdout, _("Phone: ") );
          for( j = 0; j < Length; j++ )
          {
            b = pkt[j];
            fprintf( stdout, "[%02X %c]", b, b >= 0x20 ? b : '.' );
          }
          fprintf( stdout, "\n" );
#endif /* DEBUG */
           /* ensure that we received valid packet */
          if(pkt[Length - 1] == MB640_GetChecksum(pkt))
          {
            if(pkt[2] == 0x7F) /* acknowledge by phone */
            {
              /* Set ACKOK flag */
              MB640_ACKOK    = true;
              /* Increase TX packet number */
              MB640_TXPacketNumber++;
            }
            else
            {
              /* Copy packet data  */
              memcpy(PacketData,pkt,Length);
              /* send acknowledge packet to phone */
              usleep(1000);
              ack[0] = 0x00;                     /* Construct the header.   */
              ack[1] = pkt[0];                   /* Send back id            */
              ack[2] = 0x7F;                     /* Set special size value  */
              ack[3] = pkt[Length - 2];          /* Send back packet number */
              ack[4] = MB640_GetChecksum( ack ); /* Set checksum            */
#ifdef DEBUG
              fprintf( stdout, _("PC   : ") );
              for( j = 0; j < 5; j++ )
              {
                b = ack[j];
                fprintf( stdout, "[%02X %c]", b, b >= 0x20 ? b : '.' );
              }
              fprintf( stdout, "\n" );
#endif /* DEBUG */
              if( write( PortFD, ack, 5 ) != 5 )
              {
                perror( _("Write error!\n") );
              }
              /* Set validity flag */
              MB640_PacketOK = true;
            }
          }
        }
        else
        {
          MB640_EchoOK = true;
        }
        /* Look for new packet */
        Index  = 0;
        Length = 5;
      }
    }
  }
}

/* Called by initialisation code to open comm port in asynchronous mode. */
bool MB640_OpenSerial(void)
{
  struct termios    new_termios;
  struct sigaction  sig_io;
  unsigned int      flags;

#ifdef DEBUG
  fprintf(stdout, _("Setting MBUS communication...\n"));
#endif /* DEBUG */
 
  PortFD = open(PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if ( PortFD < 0 )
  { 
    fprintf(stderr, "Failed to open %s ...\n", PortDevice);
    return (false);
  }

#ifdef DEBUG
  fprintf(stdout, "%s opened...\n", PortDevice);
#endif /* DEBUG */

  sig_io.sa_handler = MB640_SigHandler;
  sig_io.sa_flags = 0;
  sigaction (SIGIO, &sig_io, NULL);
  /* Allow process/thread to receive SIGIO */
  fcntl(PortFD, F_SETOWN, getpid());
  /* Make filedescriptor asynchronous. */
  fcntl(PortFD, F_SETFL, FASYNC);
  /* Save old termios */
  tcgetattr(PortFD, &old_termios);
  /* set speed , 8bit, odd parity */
  memset( &new_termios, 0, sizeof(new_termios) );
  new_termios.c_cflag = B9600 | CS8 | CLOCAL | CREAD | PARODD | PARENB; 
  new_termios.c_iflag = 0;
  new_termios.c_lflag = 0;
  new_termios.c_oflag = 0;
  new_termios.c_cc[VMIN] = 1;
  new_termios.c_cc[VTIME] = 0;
  tcflush(PortFD, TCIFLUSH);
  tcsetattr(PortFD, TCSANOW, &new_termios);
  /* setting the RTS & DTR bit */
  flags = TIOCM_DTR;
  ioctl(PortFD, TIOCMBIC, &flags);
  flags = TIOCM_RTS;
  ioctl(PortFD, TIOCMBIS, &flags);
#ifdef DEBUG
  ioctl(PortFD, TIOCMGET, &flags);
  fprintf(stdout, _("DTR is %s.\n"), flags & TIOCM_DTR ? _("up") : _("down"));
  fprintf(stdout, _("RTS is %s.\n"), flags & TIOCM_RTS ? _("up") : _("down"));
  fprintf(stdout, "\n");
#endif /* DEBUG */
  return (true);
}

GSM_Error MB640_SendPacket( u8 *buffer, u8 length )
{u8             pkt[256];
 int            current = 0;

  /* FIXME - we should check for the message length ... */
  pkt[current++] = 0x00;                     /* Construct the header.      */
  pkt[current++] = 0xE9;
  pkt[current++] = length;                   /* Set data size              */
  pkt[current++] = 0xE5;
  memcpy( pkt + current, buffer, length );   /* Copy in data.              */
  current += length;
  pkt[current++] = MB640_TXPacketNumber;         /* Set packet number          */
  pkt[current++] = MB640_GetChecksum( pkt ); /* Calculate and set checksum */
#ifdef DEBUG
  {int i;
   u8  b;
    fprintf( stdout, _("PC   : ") );
    for( i = 0; i < current; i++ )
    {
      b = pkt[i];
      fprintf( stdout, "[%02X %c]", b, b > 0x20 ? b : '.' );
    }
    fprintf( stdout, "\n" );
  }
#endif /* DEBUG */
  /* Send it out... */
  MB640_EchoOK = false;
  if( write(PortFD, pkt, current) != current )
  {
    perror( _("Write error!\n") );
    return (GE_INTERNALERROR);
  }
  /* wait for echo */
  while( !MB640_EchoOK && current-- )
  {
    usleep(1000);
  }
  if( !MB640_EchoOK ) return (GE_TIMEOUT);
  return (GE_NONE);
}


	/* This is the main loop for the MB21 functions.  When MB21_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB21_Terminate function. */
void	MB640_ThreadLoop(void)
{
	/* Do initialisation stuff */
  if (MB640_OpenSerial() != true)
  {
    MB640_LinkOK = false;
    while (!RequestTerminate)
    {
      usleep (100000);
    }
    return;
  }
  MB640_LinkOK = true;

	while (!RequestTerminate) {
		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}
}


#endif
