/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, Pavel Janmk ml and others 1999
	   Released under the terms of the GNU GPL, see file COPYING
	   for more details.
	
	   This file:  mbus-640.c  Version 0.3.?
	   
	   ... a starting point for whoever wants to take a crack
	   at 640 support!  Please follow the conventions used
	   in fbus-3810.[ch] and fbus-6110.[ch] wherever possible.
	   Thy tabs &| indents shall be 4 characters wide.
	
	   These functions are only ever called through the GSM_Functions
	   structure defined in gsm-common.h and set up in gsm-api.c */

#ifndef WIN32

#define		__mbus_640_c	/* "Turn on" prototypes in mbus-640.h */

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
#include	<string.h>
#include	<pthread.h>
#include	<errno.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"mbus-640.h"

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					MB640_LinkOK;
char          PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];

GSM_Functions			MB640_Functions = {
		MB640_Initialise,
		MB640_Terminate,
		MB640_GetMemoryLocation,
		MB640_WritePhonebookLocation,
		MB640_GetSpeedDial,
		MB640_SetSpeedDial,
		MB640_GetMemoryStatus,
		MB640_GetSMSStatus,
		MB640_GetSMSCenter,
		MB640_SetSMSCenter,
 		MB640_GetSMSMessage,
		MB640_DeleteSMSMessage,
		MB640_SendSMSMessage,
		MB640_GetRFLevel,
		MB640_GetBatteryLevel,
		MB640_GetPowerSource,
		MB640_GetDisplayStatus,
		MB640_EnterSecurityCode,
		MB640_GetSecurityCodeStatus,
		MB640_GetIMEI,
		MB640_GetRevision,
		MB640_GetModel,
		MB640_GetDateTime,
		MB640_SetDateTime,
		MB640_GetAlarm,
		MB640_SetAlarm,
		MB640_DialVoice,
		MB640_DialData,
		MB640_GetIncomingCallNr,
		MB640_GetNetworkInfo,
		MB640_GetCalendarNote,
		MB640_WriteCalendarNote,
		MB640_DeleteCalendarNote,
		MB640_Netmonitor,
		MB640_SendDTMF,
		MB640_GetBitmap,
		MB640_SetBitmap,
		MB640_Reset,
		MB640_GetProfile,
		MB640_SetProfile,
		MB640_SendRLPFrame
};

	/* FIXME - these are guesses only... */
GSM_Information			MB640_Information = {
		"640",					/* Models */
		4, 						/* Max RF Level */
		0,						/* Min RF Level */
		GRF_Arbitrary,			/* RF level units */
		4, 						/* Max Battery Level */
		0,						/* Min Battery Level */
		GBU_Arbitrary,			/* Battery level units */
		GDT_None,				/* No date/time support */
		GDT_None,				/* No alarm support */
		0						/* Max alarms = 0 */
};

/* Local variables */
pthread_t      Thread;
bool					 RequestTerminate;
int            PortFD;
struct termios old_termios; /* old termios */
u8             MB640_TXPacketNumber = 4;
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
{
	return (GE_NOTIMPLEMENTED);
}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	MB640_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_DeleteSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SendSMSMessage(GSM_SMSMessage *SMS)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB640_GetRFLevel
	   FIXME (sort of...)
	   For now, GetRFLevel and GetBatteryLevel both rely
	   on data returned by the "keepalive" packets.  I suspect
	   that we don't actually need the keepalive at all but
	   will await the official doco before taking it out.  HAB19990511 */
GSM_Error	MB640_GetRFLevel(GSM_RFUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB640_GetBatteryLevel
	   FIXME (see above...) */
GSM_Error	MB640_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetIMEI(char *imei)
{
  strcpy(imei, "N/A");
	return (GE_NONE);
}

GSM_Error	MB640_GetRevision(char *revision)
{
  strcpy(revision,"N/A");
	return (GE_NONE);
}

GSM_Error	MB640_GetModel(char *model)
{
  strcpy( model, "N/A" );
  return (GE_NONE);
}

/* This function sends to the mobile phone a request for the SMS Center */

GSM_Error	MB640_GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Our "Not implemented" functions */
GSM_Error	MB640_GetMemoryStatus(GSM_MemoryStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetSMSStatus(GSM_SMSStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetPowerSource(GSM_PowerSource *source)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetDisplayStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_EnterSecurityCode(GSM_SecurityCode SecurityCode)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetSecurityCodeStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_DialVoice(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_DialData(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetIncomingCallNr(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetNetworkInfo (GSM_NetworkInfo *NetworkInfo)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_WriteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_DeleteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_Netmonitor(unsigned char mode, char *Screen)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SendDTMF(char *String)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_GetBitmap(GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SetBitmap(GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_Reset(unsigned char type)
{GSM_Error err = GE_NONE;
 u8        pkt[] = { 0xE5, 0x43, 0x00, 0x00 };

  /* send packet */
  return err;
}

GSM_Error	MB640_GetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB640_SetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
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
{unsigned char        buffer[256],b;
 int                  i,res,j;
 static unsigned int  Index = 0,
                      Length = 5;
 static unsigned char pkt[256];

  res = read(PortFD, buffer, 256);

  for(i = 0; i < res ; i++)
  {
    b = buffer[i];
    if(!Index && b != 0x00 && b != 0xE9)
    {
      /* something strange goes from phone. Just ignore it */
#ifdef DEBUG
      fprintf( stdout, "Get [%02X %c]\n", b, b > 0x20 ? b : '.' );
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
            fprintf( stdout, "[%02X %c]", b, b > 0x20 ? b : '.' );
          }
          fprintf( stdout, "\n" );
#endif /* DEBUG */
           /* ensure that we received valid packet */
          if(pkt[Length - 1] == MB640_GetChecksum(pkt))
          {
            if(pkt[2] == 0x7F) /* acknowledge by phone */
            {
              MB640_ACKOK = true;
            }
            else
            {
              /* Copy packet data  */
              memcpy(PacketData,pkt,Length);
              /* Set validity flag */
              MB640_PacketOK = true;
              /* send acknowledge  */
              usleep(1000);
              MB640_SendACK(pkt[Length - 2]);
              /* Increase TX packet number */
              MB640_TXPacketNumber++;
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
  flags = TIOCM_DTR | TIOCM_RTS;
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
{u8             pkt[256], b;
 int            i, current = 0;

  /* FIXME - we should check for the message length ... */
  pkt[current++] = 0x00;                     /* Construct the header.      */
  pkt[current++] = 0xE9;
  pkt[current++] = length - 1;               /* Set data size              */
  memcpy( pkt + current, buffer, length );   /* Copy in data.              */
  current += length;
  pkt[current++] = MB640_TXPacketNumber;         /* Set packet number          *
/
  pkt[current++] = MB640_GetChecksum( pkt ); /* Calculate and set checksum */
#ifdef DEBUG
  fprintf( stdout, _("PC   : ") );
  for( i = 0; i < current; i++ )
  {
    b = pkt[i];
    fprintf( stdout, "[%02X %c]", b, b > 0x20 ? b : '.' );
  }
  fprintf( stdout, "\n" );
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

GSM_Error MB640_SendACK( u8 packet_number )
{u8  b,pkt[5];
 int i;

  pkt[0] = 0x00;                     /* Construct the header.   */
  pkt[1] = 0xE9;
  pkt[2] = 0x7F;                     /* Set special size value  */
  pkt[3] = packet_number;            /* Send back packet number */
  pkt[4] = MB640_GetChecksum( pkt ); /* Set checksum            */
  if( write( PortFD, pkt, 5 ) != 5 )
  {
    perror( _("Write error!\n") );
    return (GE_INTERNALERROR);
  }
#ifdef DEBUG
  fprintf( stdout, _("PC   : ") );
  for( i = 0; i < 5; i++ )
  {
    b = pkt[i];
    fprintf( stdout, "[%02X %c]", b, b > 0x20 ? b : '.' );
  }
  fprintf( stdout, "\n" );
#endif /* DEBUG */
  return (GE_NONE);
}

	/* This is the main loop for the MB21 functions.  When MB21_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB21_Terminate function. */
void	MB640_ThreadLoop(void)
{int timeout;
 u8  /* pkt0[] = {0xe5, 0x43, 0x00, 0x00}, */
     pkt1[] = {0xe5, 0x00, 0x03, 0x00};
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

  /* Get phone info */
  MB640_PacketOK = false;
  timeout        = 3;
  while(!MB640_PacketOK)
  {
    MB640_SendPacket(pkt1, 4);
    if(!--timeout || RequestTerminate)
    {
      return;
    }
    usleep(250000);
  }
  fprintf( stdout, "\nConnected to phone:\n%s\n\n", PacketData + 6 );
  MB640_LinkOK = true;

	while (!RequestTerminate) {
		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}
}


#endif
