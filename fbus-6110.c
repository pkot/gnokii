/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml. & Hugh Blemings.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions on the 6110 and similar
  phones. See README for more details on supported mobile phones.

  The various routines are called FB61 (whatever) as a concatenation of FBUS
  and 6110.

  Last modification: Sun May 16 21:04:03 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

/* "Turn on" prototypes in fbus-6110.h */

#define __fbus_6110_c 

/* System header files */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/* Various header file */

#include "misc.h"
#include "gsm-common.h"
#include "fbus-6110.h"
#include "fbus-6110-auth.h"
#include "fbus-6110-ringtones.h"
#include "gsm-networks.h"

/* Global variables used by code in gsm-api.c to expose the functions
   supported by this model of phone. */

bool FB61_LinkOK;

/* Here we initialise model specific functions. */

GSM_Functions FB61_Functions = {
  FB61_Initialise,
  FB61_Terminate,
  FB61_GetMemoryLocation,
  FB61_WritePhonebookLocation,
  FB61_GetMemoryStatus,
  FB61_GetSMSStatus,
  FB61_GetSMSMessage,
  FB61_DeleteSMSMessage,
  FB61_SendSMSMessage,
  FB61_GetRFLevel,
  FB61_GetBatteryLevel,
  FB61_GetPowerSource,
  FB61_EnterPin,
  FB61_GetIMEI,
  FB61_GetRevision,
  FB61_GetModel,
  FB61_GetDateTime,
  FB61_SetDateTime,
  FB61_GetAlarm,
  FB61_SetAlarm,
  FB61_DialVoice,
  FB61_DialData,
  FB61_GetIncomingCallNr
};

/* Mobile phone information */

GSM_Information FB61_Information = {
  "6110|6130|6150|6190|5110|5130|5190", /* Supported models */
  4,                     /* Max RF Level */
  0,                     /* Min RF Level */
  GRF_Arbitrary,         /* RF level units */
  4,                     /* Max Battery Level */
  0,                     /* Min Battery Level */
  GBU_Arbitrary,         /* Battery level units */
  GDT_DateTime,          /* Have date/time support */
  GDT_TimeOnly,	         /* Alarm supports time only */
  1                      /* Only one alarm available */
};

unsigned char GSM_Default_Alphabet[] = {

  /* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
  /* Characters in hex position 10, [12 to 1a] and 24 are not present on
     latin1 charset, so we cannot reproduce on the screen, however they are
     greek symbol not present even on my Nokia */

  '@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec, 
  0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
  '?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
  '?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
  ' ',  '!',  '\"', '#',  0xa4,  '%',  '&',  '\'',
  '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
  0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
  0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
  'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

const char *FB61_MemoryType_String [] = {
  "", 	/* 0x00 */
  "MT", /* 0x01 */
  "ME", /* 0x02 */
  "SM", /* 0x03 */
  "FD", /* 0x04 */
  "ON", /* 0x05 */
  "EN", /* 0x06 */
  "DC", /* 0x07 */
  "RC", /* 0x08 */
  "MC", /* 0x09 */
};

/* Local variables */

int PortFD; /* Filedescriptor of the mobile phone's device */

char PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];

/* This is the connection type used in gnokii. */

GSM_ConnectionType CurrentConnectionType;

int BufferCount;

u8 MessageBuffer[FB61_MAX_RECEIVE_LENGTH];

unsigned char MessageLength,
              MessageType,
              MessageDestination,
              MessageSource,
              MessageUnknown;

/* Magic bytes from the phone. */

unsigned char MagicBytes[4]= { 0x00, 0x00, 0x00, 0x00 };
GSM_Error CurrentMagicError = GE_BUSY;

enum FB61_RX_States RX_State;

u8 RequestSequenceNumber=0x00;
pthread_t Thread;
bool RequestTerminate;
bool DisableKeepalive=false;
struct termios OldTermios; /* To restore termio on close. */

/* Local variables used by get/set phonebook entry code. Buffer is used as a
   source or destination for phonebook data and other functions... Error is
   set to GE_NONE by calling function, set to GE_COMPLETE or an error code by
   handler routines as appropriate. */
		   	   	   
GSM_PhonebookEntry *CurrentPhonebookEntry;
GSM_Error          CurrentPhonebookError;

GSM_SMSMessage     *CurrentSMSMessage;
GSM_Error          CurrentSMSMessageError;
int                CurrentSMSPointer;

GSM_MemoryStatus   *CurrentMemoryStatus;
GSM_Error          CurrentMemoryStatusError;

GSM_SMSStatus      *CurrentSMSStatus;
GSM_Error          CurrentSMSStatusError;

GSM_Error          PINError;

GSM_DateTime       *CurrentDateTime;
GSM_Error          CurrentDateTimeError;

GSM_DateTime       *CurrentAlarm;
GSM_Error          CurrentAlarmError;

GSM_Error          CurrentSetDateTimeError;
GSM_Error          CurrentSetAlarmError;

int                CurrentRFLevel,
                   CurrentBatteryLevel,
                   CurrentPowerSource;

unsigned char      IMEI[FB61_MAX_IMEI_LENGTH];
unsigned char      Revision[FB61_MAX_REVISION_LENGTH];
unsigned char      Model[FB61_MAX_MODEL_LENGTH];

char               CurrentIncomingCall[20];

/* Every (well, almost every) frame from the computer starts with this
   sequence. */

#define FB61_FRAME_HEADER 0x00, 0x01, 0x00

/* Initialise variables and state machine. */

GSM_Error FB61_Initialise(char *port_device, GSM_ConnectionType connection, bool enable_monitoring)
{

  int rtn;

  RequestTerminate = false;
  FB61_LinkOK = false;

  strncpy(PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

  CurrentConnectionType = connection;

  /* Create and start main thread. */

  rtn = pthread_create(&Thread, NULL, (void *)FB61_ThreadLoop, (void *)NULL);

  if (rtn != 0)
    return (GE_INTERNALERROR);

  return (GE_NONE);
}

/* This function send the status request to the phone. */

GSM_Error FB61_TX_SendStatusRequest(void)
{

  /* The status request is of the type 0x04. It's subtype is 0x01. If you have
     another subtypes and it's meaning - just inform Pavel, please. */

  unsigned char request[] = {FB61_FRAME_HEADER, 0x01};

  FB61_TX_SendMessage(4, 0x04, request);

  return (GE_NONE);
}

/* This function translates GMT_MemoryType to FB61_MEMORY_xx */

int FB61_GetMemoryType(GSM_MemoryType memory_type)
{

  int result;

  switch (memory_type) {

     case GMT_MT:
	result = FB61_MEMORY_MT;
        break;

     case GMT_ME:
	result = FB61_MEMORY_ME;
        break;

     case GMT_SM:
	result = FB61_MEMORY_SM;
        break;

     case GMT_FD:
	result = FB61_MEMORY_FD;
        break;

     case GMT_ON:
	result = FB61_MEMORY_ON;
        break;

     case GMT_EN:
	result = FB61_MEMORY_EN;
        break;

     case GMT_DC:
	result = FB61_MEMORY_DC;
        break;

     case GMT_RC:
	result = FB61_MEMORY_RC;
        break;

     case GMT_MC:
	result = FB61_MEMORY_MC;
        break;

     default:
        result=FB61_MEMORY_UNKNOWN;
   }

   return (result);
}

/* This function is used to get storage status from the phone. It currently
   supports two different memory areas - internal and SIM. */

GSM_Error FB61_GetMemoryStatus(GSM_MemoryStatus *Status)
{

  unsigned char req[] = { FB61_FRAME_HEADER,
                          0x07, /* MemoryStatus request */
                          0x00 /* MemoryType */
                        };
  int timeout=20;

  CurrentMemoryStatus = Status;
  CurrentMemoryStatusError = GE_BUSY;

  req[4] = FB61_GetMemoryType(Status->MemoryType);

  FB61_TX_SendMessage(5, 0x03, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentMemoryStatusError == GE_BUSY ) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (GE_NONE);
}

GSM_Error FB61_GetCalendarNote(int location)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x66, 0x00};

  req[4]=location;

  FB61_TX_SendMessage(5, 0x13, req);

  return (GE_NONE);
}

void FB61_InitIR(void)
{
  int i;
  unsigned char init_char     = FB61_SYNC_BYTE;
  unsigned char end_init_char = FB61_IR_END_INIT_BYTE;
  
  for ( i = 0; i < 32; i++ )
    write (PortFD, &init_char, 1);

  write (PortFD, &end_init_char, 1);
  usleep(100000);
}

bool FB61_InitIR115200(void)
{
  u8 connect_seq[] = {FB61_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};

  bool ret         = true;
  u8 nr_read       = 0;
  u8 in_buffer[255];
  struct timeval timeout;
  fd_set ready;
  int no_timeout   = 0;
  int i;
  int done         = 0;

  /* send the connection sequence to phone */
  FB61_TX_SendMessage(7, 0x02, connect_seq);

  /* Wait for 1 sec. */
  timeout.tv_sec  = 1;
  timeout.tv_usec = 0;
  
  do {
    FD_ZERO(&ready);
    FD_SET(PortFD, &ready);
    no_timeout = select(PortFD + 1, &ready, NULL, NULL, &timeout);
    if ( FD_ISSET(PortFD, &ready) ) {
      nr_read = read(PortFD, in_buffer, 1);
      if ( nr_read >= 1 ) {
	for (i=0; i < nr_read; i++) {
	  if ( in_buffer[i] == FB61_IR_FRAME_ID ) {
	    done = 1;
	    ret = true;
	    break;
	  }
	}
      } else {
	done = 1;
	ret = false;
      }
    }

    if ( ! no_timeout ) {

#ifdef DEBUG
      printf ("Timeout in IR-mode\n");
#endif DEBUG

      done = 1;
      ret = false;
    }
  } while ( ! done );
  
  return(ret);
}

/* This function is used to open the IR connection with the phone */

bool FB61_OpenIR(void)
{
  bool ret = false;
  struct termios new_termios;
  struct sigaction sig_io;
  u8 i = 0;
  
  /* Open device. */
  
  PortFD = open (PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);
  
  if (PortFD < 0) {
    perror(_("Couldn't open FB61 infrared device: "));
    return (false);
  }
  
  /* Set up and install handler before enabling async IO on port. */
  
  sig_io.sa_handler = FB61_SigHandler;
  sig_io.sa_flags = 0;
  sigaction (SIGIO, &sig_io, NULL);
  
  /* Allow process/thread to receive SIGIO */
  
  fcntl(PortFD, F_SETOWN, getpid());
  
  /* Make filedescriptor asynchronous. */

  fcntl(PortFD, F_SETFL, FASYNC);
  
  /* Save current port attributes so they can be restored on exit. */
  
  tcgetattr(PortFD, &OldTermios);
  
  /* Set port settings for canonical input processing */
  
  new_termios.c_cflag = FB61_IR_INIT_SPEED | CS8 | CLOCAL | CREAD;
  new_termios.c_iflag = IGNPAR;
  new_termios.c_oflag = 0;
  new_termios.c_lflag = 0;
  new_termios.c_cc[VMIN] = 1;
  new_termios.c_cc[VTIME] = 0;
  
  tcflush(PortFD, TCIFLUSH);
  tcsetattr(PortFD, TCSANOW, &new_termios);

  FB61_InitIR();
  
  new_termios.c_cflag = FB61_BAUDRATE | CS8 | CLOCAL | CREAD;
  tcflush(PortFD, TCIFLUSH);
  tcsetattr(PortFD, TCSANOW, &new_termios);
  
  ret = FB61_InitIR115200();

  if ( ! ret ) {
    for ( i = 0; i < 4 ; i++) {
      usleep (500000);
      
      new_termios.c_cflag = FB61_IR_INIT_SPEED | CS8 | CLOCAL | CREAD;
      tcflush(PortFD, TCIFLUSH);
      tcsetattr(PortFD, TCSANOW, &new_termios);
      
      FB61_InitIR();
      
      new_termios.c_cflag = FB61_BAUDRATE | CS8 | CLOCAL | CREAD;
      tcflush(PortFD, TCIFLUSH);
      tcsetattr(PortFD, TCSANOW, &new_termios);
      
      ret = FB61_InitIR115200();
      if ( ret ) {
	break;
      }
    }
  }
  
  return (ret);
}

/* This is the main loop for the FB61 functions. When FB61_Initialise is
   called a thread is created to run this loop. This loop is exited when the
   application calls the FB61_Terminate function. */

void FB61_ThreadLoop(void)
{

  unsigned char init_char = 0x55;
  unsigned char connect1[] = {FB61_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};
  unsigned char connect2[] = {FB61_FRAME_HEADER, 0x20, 0x02};
  unsigned char connect3[] = {FB61_FRAME_HEADER, 0x0d, 0x01, 0x00, 0x02};
  unsigned char connect4[] = {FB61_FRAME_HEADER, 0x10};

  unsigned char magic_connect[] = {FB61_FRAME_HEADER,
  0x12,

  /* The real magic goes here ... These bytes are filled in with the
     external function FB61_GetNokiaAuth(). */

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

  /* NOKIA&GNOKII Accessory */

  0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x26, 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x20,
  0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x79,
  
  0x00, 0x00, 0x00, 0x00};

  int count, idle_timer, timeout=50;

  CurrentPhonebookEntry = NULL;  

  if ( CurrentConnectionType == GCT_Infrared ) {

#ifdef DEBUG
    printf ("Starting IR mode...!\n");
#endif DEBUG

    if (FB61_OpenIR() != true) {
      FB61_LinkOK = false;
      while (!RequestTerminate)
	usleep (100000);
      return;
    }

  } else { /* CurrentConnectionType == GCT_Serial */

    /* Try to open serial port, if we fail we sit here and don't proceed to the
       main loop. */
   
    if (FB61_OpenSerial() != true) {
      FB61_LinkOK = false;
      
      /* Fail so sit here till calling code works out there is a problem. */
      
      while (!RequestTerminate)
	usleep (100000);
      
      return;
    }
  }
  
  /* Initialise link with phone or what have you */

  /* Send init string to phone, this is a bunch of 0x55 characters. Timing is
     empirical. */

  for (count = 0; count < 250; count ++) {
    usleep(100);
    write(PortFD, &init_char, 1);
  }

  FB61_TX_SendStatusRequest();

  usleep(100);

  FB61_TX_SendMessage(7, 0x02, connect1);

  usleep(100);

  FB61_TX_SendMessage(5, 0x02, connect2);

  usleep(100);

  FB61_TX_SendMessage(7, 0x02, connect3);

  usleep(100);

  FB61_TX_SendMessage(4, 0x64, connect4);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentMagicError == GE_BUSY ) {

    if (--timeout == 0)
      return;

    usleep (100000);
  }                       

  FB61_GetNokiaAuth(IMEI, MagicBytes, magic_connect+4);

  FB61_TX_SendMessage(45, 0x64, magic_connect);
  
  /* Get the primary SMS Center */

  /* It's very strange that if I send this request again (with different
     number) it fails. But if I send it alone it succeds. It seems that 6110
     is refusing to tell you all the SMS Centers information at once :-(
     And even when I'm authenticated correctly... */

  //    FB61_GetSMSCenter(1);

  // 	FB61_GetCalendarNote(1);

  //    FB61_SendRingtone("GNOKIItune", 250);

  idle_timer=0;

  /* Now enter main loop */

  while (!RequestTerminate) {

    if (idle_timer == 0) {
      
      /* Dont send keepalive and status packets when doing other transactions. */
      
      if (!DisableKeepalive)
	FB61_TX_SendStatusRequest();

      idle_timer = 20;
    }
    else
      idle_timer--;

    usleep(100000);		/* Avoid becoming a "busy" loop. */
  }
}

/* Applications should call FB61_Terminate to shut down the FB61 thread and
   close the serial port. */

void FB61_Terminate(void)
{
  /* Request termination of thread */
  RequestTerminate = true;

  /* Now wait for thread to terminate. */
  pthread_join(Thread, NULL);

  /* Close serial port. */
  tcsetattr(PortFD, TCSANOW, &OldTermios);

  close (PortFD);
}

#define ByteMask ((1<<NumOfBytes)-1)

int UnpackEightBitsToSeven(int length, unsigned char *input, unsigned char *output)
{

  int NumOfBytes=7;
  unsigned char Rest=0x00;

  unsigned char *OUT=output; /* Current pointer to the output buffer */
  unsigned char *IN=input;   /* Current pointer to the input buffer */

  while ((IN-input) < length) {

    *OUT = ((*IN & ByteMask) << (7-NumOfBytes)) | Rest;

    Rest = *IN >> NumOfBytes;

    IN++;OUT++;

    if (NumOfBytes==1) {
      *OUT=Rest;
      OUT++;
      NumOfBytes=7;
      Rest=0x00;
    }
    else
      NumOfBytes--;
  }

  return OUT-output;
}

int PackSevenBitsToEight(unsigned char *String, unsigned char* Buffer)
{

  unsigned char *OUT=Buffer; /* Current pointer to the output buffer */
  unsigned char *IN=String;  /* Current pointer to the input buffer */
  int Bits=7;                /* Number of bits directly copied to output buffer */

  while ((IN-String)<strlen(String)) {

    unsigned char Byte=*IN & 0x7f;

    *OUT=Byte>>(7-Bits);

    if (Bits != 7)
      *(OUT-1)|=(Byte & ( (1<<(7-Bits))-1))<<(Bits+1);

    Bits--;

    if (Bits==-1)
      Bits=7;
    else
      OUT++;
      
    IN++;
  }

  return OUT-Buffer;
}

GSM_Error FB61_GetRFLevel(GSM_RFUnits *units, float *level)
{

  /* FIXME - these values are from 3810 code, may be incorrect.  Map from
     values returned in status packet to the the values returned by the AT+CSQ
     command. */

  float	csq_map[5] = {0, 8, 16, 24, 31};
  int timeout=10;
  int rf_level;

  CurrentRFLevel=-1;

  FB61_TX_SendStatusRequest();

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentRFLevel == -1 ) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  /* Make copy in case it changes. */
  rf_level = CurrentRFLevel;

  if (rf_level == -1)
    return (GE_NOLINK);

  /* Now convert between the different units we support. */

  /* Arbitrary units. */
  if (*units == GRF_Arbitrary) {
    *level = rf_level;
    return (GE_NONE);
  }

  /* CSQ units. */
  if (*units == GRF_CSQ) {

    if (rf_level <=4)
      *level = csq_map[rf_level];
    else
      *level = 99; /* Unknown/undefined */

    return (GE_NONE);
  }

  /* Unit type is one we don't handle so return error */
  return (GE_INTERNALERROR);
}

GSM_Error FB61_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{

  int timeout=10;
  int batt_level;

  CurrentBatteryLevel=-1;

  FB61_TX_SendStatusRequest();

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentBatteryLevel == -1 ) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  /* Take copy in case it changes. */
  batt_level = CurrentBatteryLevel;

  if (batt_level != -1) {

    /* Only units we handle at present are GBU_Arbitrary */
    if (*units == GBU_Arbitrary) {
      *level = batt_level;
      return (GE_NONE);
    }

    return (GE_INTERNALERROR);
  }
  else
    return (GE_NOLINK);
}

GSM_Error FB61_GetPowerSource(GSM_PowerSource *source)
{

  int timeout=10;

  CurrentPowerSource=-1;

  FB61_TX_SendStatusRequest();

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentPowerSource == -1 ) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  if (CurrentPowerSource != -1) {
    *source=CurrentPowerSource;
    return (GE_NONE);
  }
  else
    return (GE_NOLINK);
}

GSM_Error FB61_DialVoice(char *Number) {

  unsigned char req[64]={FB61_FRAME_HEADER, 0x01};
  unsigned char req_end[]={0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01};
  int i=0;

  req[4]=strlen(Number);

  for(i=0; i < strlen(Number) ; i++)
   req[5+i]=Number[i];

  memcpy(req+5+strlen(Number), req_end, 10);

  FB61_TX_SendMessage(13+strlen(Number), 0x01, req);

  return(GE_NONE);
}

GSM_Error FB61_DialData(char *Number) {

  unsigned char req[100]={FB61_FRAME_HEADER, 0x01};
  unsigned char req_end[]={0x01, 0x02, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01, 0x02, 0x0a, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x15, 0x63, 0xa8, 0x00, 0x00};
  int i=0;

  req[4]=strlen(Number);

  for(i=0; i < strlen(Number) ; i++)
   req[5+i]=Number[i];

  memcpy(req+5+strlen(Number), req_end, 23);

  FB61_TX_SendMessage(26+strlen(Number), 0x01, req);

  return(GE_NONE);
}

GSM_Error FB61_GetIncomingCallNr(char *Number) {

  if (strlen(CurrentIncomingCall)>0) {
    strcpy(Number, CurrentIncomingCall);
    return GE_NONE;
  }
  else
    return GE_BUSY;
}

GSM_Error FB61_EnterPin(char *pin)
{

  unsigned char req[15] = {FB61_FRAME_HEADER, 0x0a, 0x02};
  int i=0, timeout=20;

  PINError=GE_BUSY;

  for (i=0; i<strlen(pin);i++)
    req[5+i]=pin[i];

  req[5+strlen(pin)]=0x00;

  FB61_TX_SendMessage(6+strlen(pin), 0x08, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && PINError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (PINError);
}

GSM_Error FB61_GetDateTime(GSM_DateTime *date_time)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x62};
  int timeout=5;

  CurrentDateTime=date_time;
  CurrentDateTimeError=GE_BUSY;

  FB61_TX_SendMessage(4, 0x11, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentDateTimeError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentDateTimeError);
}

GSM_Error FB61_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x6d};
  int timeout=5;

  CurrentAlarm=date_time;
  CurrentAlarmError=GE_BUSY;

  FB61_TX_SendMessage(4, 0x11, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentAlarmError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentAlarmError);
}

/* This function sends to the mobile phone a request for the SMS Center of
   rank `priority' */

GSM_Error FB61_GetSMSCenter(u8 priority)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x33, 0x64, 0x01};
  int timeout=10;

  req[5]=priority;

  CurrentSMSStatusError = GE_BUSY;

  FB61_TX_SendMessage(6, 0x02, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentSMSStatusError == GE_BUSY ) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (GE_NONE);
}

GSM_Error FB61_GetSMSStatus(GSM_SMSStatus *Status)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x36, 0x64};
  int timeout=10;

  CurrentSMSStatus = Status;
  CurrentSMSStatusError = GE_BUSY;

  FB61_TX_SendMessage(5, 0x14, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentSMSStatusError == GE_BUSY ) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (GE_NONE);
}

GSM_Error FB61_GetIMEI(char *imei)
{

  strncpy (imei, IMEI, FB61_MAX_IMEI_LENGTH);

  return (GE_NONE);
}

GSM_Error FB61_GetRevision(char *revision)
{

  strncpy (revision, Revision, FB61_MAX_REVISION_LENGTH);

  return (GE_NONE);
}

GSM_Error FB61_GetModel(char *model)
{

  strncpy (model, Model, FB61_MAX_MODEL_LENGTH);

  return (GE_NONE);
}

GSM_Error FB61_SetDateTime(GSM_DateTime *date_time)
{

  unsigned char req[] = { FB61_FRAME_HEADER,
			  0x60, /* set-time subtype */
			  0x01, 0x01, 0x07, /* unknown */
			  0x00, 0x00, /* Year (0x07cf = 1999) */
			  0x00, 0x00, /* Month Day */
			  0x00, 0x00, /* Hours Minutes */
			  0x00 /* Unknown, but not seconds - try 59 and wait 1 sec. */
			};
  int timeout=20;

  req[7] = date_time->Year / 256;
  req[8] = date_time->Year % 256;
  req[9] = date_time->Month;
  req[10] = date_time->Day;
  req[11] = date_time->Hour;
  req[12] = date_time->Minute;

  CurrentSetDateTimeError=GE_BUSY;

  FB61_TX_SendMessage(14, 0x11, req);

  while (timeout != 0 && CurrentSetDateTimeError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentSetDateTimeError);
}

/* FIXME: we should also allow to set the alarm off :-) */

GSM_Error FB61_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{

  unsigned char req[] = { FB61_FRAME_HEADER,
			  0x6b, /* set-alarm subtype */
			  0x01, 0x20, 0x03, /* unknown */
			  0x02,       /* should be alarm on/off, but it don't works */
			  0x00, 0x00, /* Hours Minutes */
			  0x00 /* Unknown, but not seconds - try 59 and wait 1 sec. */
			};
  int timeout=20;

  req[8] = date_time->Hour;
  req[9] = date_time->Minute;

  CurrentSetAlarmError=GE_BUSY;

  FB61_TX_SendMessage(11, 0x11, req);

  while (timeout != 0 && CurrentSetAlarmError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentSetAlarmError);
}

/* Routine to get specifed phone book location.  Designed to be called by
   application.  Will block until location is retrieved or a timeout/error
   occurs. */

GSM_Error FB61_GetMemoryLocation(int location, GSM_PhonebookEntry *entry) {

  unsigned char req[] = {FB61_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00};
  int timeout=20; /* 2 seconds for command to complete */

  CurrentPhonebookEntry = entry;
  CurrentPhonebookError = GE_BUSY;

  req[4] = FB61_GetMemoryType(entry->MemoryType);
  req[5] = location;

  FB61_TX_SendMessage(7, 0x03, req);

  while (timeout != 0 && CurrentPhonebookError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentPhonebookError);
}

/* Routine to write phonebook location in phone. Designed to be called by
   application code. Will block until location is written or timeout
   occurs. */

GSM_Error FB61_WritePhonebookLocation(int location, GSM_PhonebookEntry *entry)
{

  unsigned char req[128] = {FB61_FRAME_HEADER, 0x04, 0x00, 0x00};
  int i=0, current=0;
  int timeout=50;

  CurrentPhonebookError=GE_BUSY;

  req[4] = FB61_GetMemoryType(entry->MemoryType);
  req[5] = location;

  req[6] = strlen(entry->Name);

  current=7;

  for (i=0; i<strlen(entry->Name); i++)
    req[current+i] = entry->Name[i];

  current+=strlen(entry->Name);

  req[current++]=strlen(entry->Number);

  for (i=0; i<strlen(entry->Number); i++)
    req[current+i] = entry->Number[i];

  current+=strlen(entry->Number);

  req[current++]=0xff;

  FB61_TX_SendMessage(current, 3, req);

  while (timeout != 0 && CurrentPhonebookError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentPhonebookError);
}

GSM_Error FB61_GetSMSMessage(int location, GSM_SMSMessage *message)
{

  unsigned char req[] = { FB61_FRAME_HEADER,
                          0x07,
                          0x02, /* Unknown */
                          0x00, /* Location */
                          0x01, 0x64};

  int timeout = 60; /* 6 seconds for command to complete */

  /* State machine code writes data to these variables when it comes in. */

  CurrentSMSMessage = message;
  CurrentSMSMessageError = GE_BUSY;

  req[5] = location;

  /* Send request */

  FB61_TX_SendMessage(8, 0x02, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && (CurrentSMSMessageError == GE_BUSY || CurrentSMSMessageError == GE_SMSWAITING)) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentSMSMessageError);
}

GSM_Error FB61_DeleteSMSMessage(int location, GSM_SMSMessage *message)
{

  unsigned char req[] = {FB61_FRAME_HEADER, 0x0a, 0x02, 0x00};
  int timeout = 50; /* 5 seconds for command to complete */

  CurrentSMSMessageError = GE_BUSY;

  req[5] = location;

  FB61_TX_SendMessage(6, 0x14, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentSMSMessageError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentSMSMessageError);
}

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */

int SemiOctetPack(char *Number, unsigned char *Output) {

  unsigned char *IN=Number;  /* Pointer to the input number */
  unsigned char *OUT=Output; /* Pointer to the output */

  int count=0; /* This variable is used to notify us about count of already
                  packed numbers. */

  /* The first byte in the Semi-octet representation of the address field is
     the Type-of-Address. This field is described in the official GSM
     specification 03.40 version 5.3.0, section 9.1.2.5, page 33. We support
     only international and unknown number. */
  
  if (*IN == '+') {
    *OUT++=GNT_INTERNATIONAL; /* International number */
    IN++;
  }
  else
    *OUT++=GNT_UNKNOWN; /* Unknown number */

  /* The next field is the number. It is in semi-octet representation - see
     GSM scpecification 03.40 version 5.3.0, section 9.1.2.3, page 31. */

  while (*IN) {

    if (count & 0x01) {
      *OUT=*OUT | ((*IN-'0')<<4);
      OUT++;
    }
    else
      *OUT=*IN-'0';

    count++; IN++;

  }

  /* We should also fill in the most significant bits of the last byte with
     0x0f (1111 binary) if the number is represented with odd number of
     digits. */

  if (count & 0x01) {
    *OUT=*OUT | 0xf0;
    OUT++;
    return (2 * (OUT - Output - 1) - 1);
  }

  return (2 * (OUT - Output - 1));
}

GSM_Error FB61_SendSMSMessage(GSM_SMSMessage *SMS)
{

  unsigned char req[256] = {
    FB61_FRAME_HEADER,
    0x01, 0x02, 0x00, /* SMS send request*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* SMS centre, the rest is unused */
    0x11, /* TP validity period (9.2.3.1) */
    0x00, /* TP-Protocol identifier (9.2.3.9) */
    0x00, /* Type of sms: 00=text 22=fax 26=paging 2d=email */
    0x00, /* 00=normal_sms  F0=flash_sms */
    0x00, /* Message length in characters */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* destination */
    0xa9, /* SMS validity: b0=1h 47=6h a7=24h a9=72h ad=1week */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  int size=PackSevenBitsToEight(SMS->MessageText, req+42);
  int timeout=60;

  CurrentSMSMessageError=GE_BUSY;

  if (strlen(SMS->MessageText) > GSM_MAX_SMS_LENGTH)
    return(GE_SMSTOOLONG);

  req[6]=SemiOctetPack(SMS->MessageCentre, req+7);
  if (req[6] % 2) req[6]++;
  req[6] = req[6] / 2 + 1;

  req[22]=strlen(SMS->MessageText);

  req[23]=SemiOctetPack(SMS->Destination, req+24);

  /* TP-Validity Period handling */

  /* FIXME: error-checking for correct Validity - it should not be bigger then
     63 weeks and smaller then 5minutes. We should also test intervals because
     the SMS->Validity to TP-VP is not continuos. I think that the simplest
     solution will be an array of correct values. We should parse it and if we
     find the closest TP-VP value we should use it. Or is it good to take
     closest smaller TP-VP as we do now? I think it is :-) */

  /* 5 minutes intervals up to 12 hours = 720 minutes */

  if (SMS->Validity <= 720)
    req[35] = (unsigned char) (SMS->Validity/5)-1;

  /* 30 minutes intervals up to 1 day */

  else if ((SMS->Validity > 720) && (SMS->Validity <= 1440))
    req[35] = (unsigned char) ((SMS->Validity-720)/30)+143;

  /* 1 day intervals up to 30 days */

  else if ((SMS->Validity > 1440) && (SMS->Validity <= 43200))
    req[35] = (unsigned char) (SMS->Validity/1440)+166;

  /* 1 week intervals up to 63 weeks */

  else if ((SMS->Validity > 43200) && (SMS->Validity <= 635040))
    req[35] = (unsigned char) (SMS->Validity/10080)+192;

  FB61_TX_SendMessage(42+size, 0x02, req);

  /* Wait for timeout or other error. */
  while (timeout != 0 && CurrentSMSMessageError == GE_BUSY) {

    if (--timeout == 0)
      return (GE_TIMEOUT);

    usleep (100000);
  }

  return (CurrentSMSMessageError);
}

void FB61_DumpSerial(void)
{

  unsigned int Flags=0;

  ioctl(PortFD, TIOCMGET, &Flags);

#ifdef DEBUG
  printf(_("Serial flags dump:\n"));
  printf(_("DTR is %s.\n"), Flags&TIOCM_DTR?_("up"):_("down"));
  printf(_("RTS is %s.\n"), Flags&TIOCM_RTS?_("up"):_("down"));
  printf(_("CAR is %s.\n"), Flags&TIOCM_CAR?_("up"):_("down"));
  printf(_("CTS is %s.\n"), Flags&TIOCM_CTS?_("up"):_("down"));
  printf("\n");
#endif DEBUG

}

/* This functions set up the Nokia DAU-9P MBus Cable NSE-3 which is probably
   dual MBUS/FBUS to FBUS communication. If we skip this step we can
   communicate with the phone, but the phone sends us some garbagge (0x18 0x00
   ...).

   This was brought to my attention by people with original cable. My cable is
   FBUS only so I borrow the original cable from my friend and solve
   it. Thanks to Cobus, Juan and people on the mailing list.

   Colin wrote:

   The data suite cable has some electronics built into the connector. This of
   course needs a power supply of some sorts to operate properly.

   In this case power is drawn off the handshaking lines of the PC. DTR has to
   be set and RTS have to be cleared, thus if you use a terminal program (that
   does not set the handshaking lines to these conditions) you will get weird
   results. It will not set them like this since if Request To Send (RTS) is
   not set the other party will not send any data (in hardware handshaking)
   and if DTS is not set (handshaking = none) the cable will not receive
   power. */

/* FIXME: In this function we use ioctls - can people with different OSes than
   my (Linux) tell me if it works for them? */

void FB61_SetFBUS()
{

  unsigned int Flags=0;

#ifdef DEBUG
  FB61_DumpSerial();  
  printf(_("Setting FBUS communication...\n"));
#endif DEBUG

  /* clearing the RTS bit */

  Flags=TIOCM_RTS;
  ioctl(PortFD, TIOCMBIC, &Flags);

  /* setting the DTR bit */

  Flags=TIOCM_DTR;
  ioctl(PortFD, TIOCMBIS, &Flags);

#ifdef DEBUG
  FB61_DumpSerial();  
#endif DEBUG
}

/* Called by initialisation code to open comm port in asynchronous mode. */

bool FB61_OpenSerial(void)
{

  struct termios new_termios;
  struct sigaction sig_io;

  /* Open device. */

  PortFD = open (PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (PortFD < 0) {
    perror(_("Couldn't open FB61 device: "));
    return (false);
  }

  /* Set up and install handler before enabling async IO on port. */

  sig_io.sa_handler = FB61_SigHandler;
  sig_io.sa_flags = 0;
  sigaction (SIGIO, &sig_io, NULL);

  /* Allow process/thread to receive SIGIO */

  fcntl(PortFD, F_SETOWN, getpid());

  /* Make filedescriptor asynchronous. */

  fcntl(PortFD, F_SETFL, FASYNC);

  /* Save current port attributes so they can be restored on exit. */

  tcgetattr(PortFD, &OldTermios);

  /* Set port settings for canonical input processing */

  new_termios.c_cflag = FB61_BAUDRATE | CS8 | CLOCAL | CREAD;
  new_termios.c_iflag = IGNPAR;
  new_termios.c_oflag = 0;
  new_termios.c_lflag = 0;
  new_termios.c_cc[VMIN] = 1;
  new_termios.c_cc[VTIME] = 0;

  tcflush(PortFD, TCIFLUSH);
  tcsetattr(PortFD, TCSANOW, &new_termios);

  FB61_SetFBUS();

  return (true);
}

void FB61_SigHandler(int status)
{

  unsigned char buffer[255];
  int count, res;

  res = read(PortFD, buffer, 255);

  for (count = 0; count < res ; count ++)
    FB61_RX_StateMachine(buffer[count]);
}

char *FB61_GetBCDNumber(u8 *Number) {

  static char Buffer[20]="";

  /* This is the length of BCD coded number */
  int length=Number[0];
  int count;

  if (Number[1]==0x91)
	sprintf(Buffer, "+");
  else
        Buffer[0] = '\0';

  for (count=0; count <length-1; count++) {

    int Digit;

    Digit=Number[count+2]&0x0f;

    if (Digit<10)
      sprintf(Buffer, "%s%d", Buffer, Digit);
    
    Digit=Number[count+2]>>4;

    if (Digit<10)
      sprintf(Buffer, "%s%d", Buffer, Digit);
  }

  return Buffer;
}

char *FB61_GetPackedDateTime(u8 *Number) {

  static char Buffer[20]="";

  sprintf(Buffer, "%d%d-", Number[0]&0xf, Number[0]>>4);
  sprintf(Buffer, "%s%d%d-", Buffer, Number[1]&0xf, Number[1]>>4);
  sprintf(Buffer, "%s%d%d ", Buffer, Number[2]&0xf, Number[2]>>4);
  sprintf(Buffer, "%s%d%d:", Buffer, Number[3]&0xf, Number[3]>>4);
  sprintf(Buffer, "%s%d%d:", Buffer, Number[4]&0xf, Number[4]>>4);
  sprintf(Buffer, "%s%d%d",  Buffer, Number[5]&0xf, Number[5]>>4);
  
  return Buffer;

}

enum FB61_RX_States FB61_RX_DispatchMessage(void) {

  int i, tmp, count, report, offset;
  unsigned char output[160];

#ifdef DEBUG
  FB61_RX_DisplayMessage();
#endif DEBUG

  /* Switch on the basis of the message type byte */
  switch (MessageType) {
	  
    /* Call information */

  case 0x01:
	  
    switch (MessageBuffer[3]) {

      /* Unknown message - it has been seen after the 0x07 message (call
	 answered). Probably it has similar meaning. If you can solve
	 this - just mail me. Pavel Janík ml.

	 The message looks like this:

	 Msg Destination: PC
	 Msg Source: Phone
	 Msg Type: 01
	 Msg Unknown: 00
	 Msg Len: 0e

	 Phone: [01 ][08 ][00 ] is the header of the frame

	 [03 ] is the call message subtype

	 [05 ] is the call sequence number

	 [05 ] unknown 

	 [00 ][01 ][03 ][02 ][91][00] are unknown but has been
	 seen in the Incoming call message (just after the
	 caller's name from the phonebook). But never change
	 between phone calls :-(
      */

    case 0x03:

#ifdef DEBUG
      printf(_("Message: Call message, type 0x03:"));
      printf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
      printf(_("   Exact meaning not known yet, sorry :-(\n"));
#endif DEBUG

      break;

      /* Remote end has gone away before you answer the call.  Probably your
         mother-in-law or banker (which is worse?) ... */

    case 0x04:

#ifdef DEBUG
      printf(_("Message: Remote end hang up.\n"));
      printf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
#endif DEBUG

      CurrentIncomingCall[0]=0;

      break;

      /* Incoming call alert */

    case 0x05:

#ifdef DEBUG
      printf(_("Message: Incoming call alert:\n"));

      /* We can have more then one call ringing - we can distinguish between
         them */

      printf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
      printf(_("   Number: "));
      count=MessageBuffer[6];

      for (tmp=0; tmp <count; tmp++)
	printf("%c", MessageBuffer[7+tmp]);

      printf("\n");

      printf(_("   Name: "));

      for (tmp=0; tmp <MessageBuffer[7+count]; tmp++)
	printf("%c", MessageBuffer[8+count+tmp]);

      printf("\n");
#endif DEBUG

      count=MessageBuffer[6];

      for (tmp=0; tmp <count; tmp++)
	sprintf(CurrentIncomingCall, "%s%c", CurrentIncomingCall, MessageBuffer[7+tmp]);

      break;

      /* Call answered. Probably your girlfriend...*/

    case 0x07:

#ifdef DEBUG
      printf(_("Message: Call answered.\n"));
      printf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
#endif DEBUG

      break;

      /* Call ended. Girlfriend is girlfriend, but time is money :-) */

    case 0x09:

#ifdef DEBUG
      printf(_("Message: Call ended by your phone.\n"));
      printf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
#endif DEBUG

      break;

      /* This message has been seen with the message of subtype 0x09
	 after I hang the call.

	 Msg Destination: PC
	 Msg Source: Phone
	 Msg Type: 01
	 Msg Unknown: 00
	 Msg Len: 08
	 Phone: [01 ][08 ][00 ][0a ][04 ][87 ][01 ][42B][1a ][c2 ]

	 What is the meaning of 87? Can you spell some magic light into
	 this issue?

      */

    case 0x0a:

#ifdef DEBUG
      printf(_("Message: Call message, type 0x0a:"));
      printf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
      printf(_("   Exact meaning not known yet, sorry :-(\n"));
#endif DEBUG

      CurrentIncomingCall[0]=0;

      break;

    default:

#ifdef DEBUG
      printf(_("Message: Unknown message of type 0x01\n"));
#endif DEBUG

    }

    break;

    /* SMS handling */

  case 0x02:

    switch (MessageBuffer[3]) {

    case 0x02:

      /* SMS message correctly sent to the network */

#ifdef DEBUG
      printf(_("Message: SMS Message correctly sent.\n"));
#endif DEBUG

      CurrentSMSMessageError = GE_SMSSENDOK;
      break;

    case 0x03:

      /* SMS message send to the network failed */

#ifdef DEBUG
      printf(_("Message: Sending SMS Message failed.\n"));
#endif DEBUG

      CurrentSMSMessageError = GE_SMSSENDFAILED;
      break;

    case 0x10:
	  
#ifdef DEBUG
      printf(_("Message: SMS Message Received\n"));
      printf(_("   SMS center number: %s\n"), FB61_GetBCDNumber(MessageBuffer+7));

      MessageBuffer[23] = (MessageBuffer[23]+1)/2+1;

      printf(_("   Remote number: %s\n"), FB61_GetBCDNumber(MessageBuffer+23));
      printf(_("   Date: %s\n"), FB61_GetPackedDateTime(MessageBuffer+35));
      printf(_("   SMS: "));

      tmp=UnpackEightBitsToSeven(MessageLength-42-2, MessageBuffer+42, output);

      for (i=0; i<tmp;i++)
	printf("%c", GSM_Default_Alphabet[output[i]]);

      printf("\n");
#endif DEBUG

      break;

    case 0x34:

#ifdef DEBUG
      printf(_("Message: SMS Center received:\n"));
      printf(_("   %d. SMS Center name is %s\n"), MessageBuffer[4], MessageBuffer+33);
      printf(_("   %d. SMS Center number is %s\n"), MessageBuffer[4], FB61_GetBCDNumber(MessageBuffer+21));
#endif DEBUG

      CurrentSMSStatusError=GE_NONE;

      break;

    case 0x35:

      /* Nokia 6110 has support for three SMS centers with numbers 1, 2 and
	 3. Each request for SMS Center without one of these numbers
	 fail. */

      printf(_("Message: SMS Center error received:\n"));
      printf(_("   The request for SMS Center failed.\n"));

      break;  

    default:

      FB61_RX_DisplayMessage();

    }

    break;

    /* Phonebook handling */

  case 0x03:

    switch (MessageBuffer[3]) {

    case 0x02:

      CurrentPhonebookEntry->Empty = true;

      count=MessageBuffer[5];
	  
#ifdef DEBUG
      printf(_("Message: Phonebook entry received:\n"));
      printf(_("   Name: "));

      for (tmp=0; tmp <count; tmp++)
	printf("%c", MessageBuffer[6+tmp]);

      printf("\n");
#endif DEBUG

      memcpy(CurrentPhonebookEntry->Name, MessageBuffer + 6, count);
      CurrentPhonebookEntry->Name[count] = 0x00;
      CurrentPhonebookEntry->Empty = false;

      i=7+count;
      count=MessageBuffer[6+count];

#ifdef DEBUG
      printf(_("   Number: "));

      for (tmp=0; tmp <count; tmp++)
	printf("%c", MessageBuffer[i+tmp]);

      printf("\n");
#endif DEBUG

      memcpy(CurrentPhonebookEntry->Number, MessageBuffer + i, count);
      CurrentPhonebookEntry->Number[count] = 0x00;
      CurrentPhonebookEntry->Group = MessageBuffer[i+count];

      /* Signal no error to calling code. */

      CurrentPhonebookError = GE_NONE;

      break;

    case 0x03:

#ifdef DEBUG
      printf(_("Message: Phonebook read entry error received:\n"));
#endif DEBUG

      switch (MessageBuffer[4]) {

      case 0x7d:

#ifdef DEBUG
	printf(_("   Invalid memory type!\n"));
#endif DEBUG

	CurrentPhonebookError = GE_INVALIDMEMORYTYPE;

	break;

      default:

#ifdef DEBUG
	printf(_("   Unknown error!\n"));
#endif DEBUG

	CurrentPhonebookError = GE_INTERNALERROR;
      }

      break;

    case 0x05:

#ifdef DEBUG
      printf(_("Message: Phonebook written correctly.\n"));
#endif DEBUG

      CurrentPhonebookError = GE_NONE;

      break;

    case 0x06:

      switch (MessageBuffer[4]) {

	/* FIXME: other errors? When I send the phonebook with index of 350 it
           still report error 0x7d :-( */

      case 0x7d:

#ifdef DEBUG
	printf(_("Message: Phonebook not written - name is too long.\n"));
#endif DEBUG

	CurrentPhonebookError = GE_PHBOOKNAMETOOLONG;

	break;

      default:

#ifdef DEBUG
	printf(_("   Unknown error!\n"));
#endif DEBUG

	CurrentPhonebookError = GE_INTERNALERROR;
      }

      break;

    case 0x08:

#ifdef DEBUG
      printf(_("Message: Memory status received:\n"));

      printf(_("   Memory Type: %s\n"), FB61_MemoryType_String[MessageBuffer[4]]);
      printf(_("   Used: %d\n"), MessageBuffer[6]);
      printf(_("   Free: %d\n"), MessageBuffer[5]);
#endif DEBUG

      CurrentMemoryStatus->Used = MessageBuffer[6];
      CurrentMemoryStatus->Free = MessageBuffer[5];
      CurrentMemoryStatusError = GE_NONE;

      break;

    case 0x09:

	switch (MessageBuffer[4]) {

	  case 0x6f:
#ifdef DEBUG
	        printf(_("Message: Memory status error, phone is probably powered off.\n"));
#endif DEBUG
		CurrentMemoryStatusError = GE_TIMEOUT;
		break;

	  case 0x7d:
#ifdef DEBUG
	        printf(_("Message: Memory status error, memory type not supported by phone model.\n"));
#endif DEBUG
		CurrentMemoryStatusError = GE_INTERNALERROR;
		break;

	  case 0x8d:
#ifdef DEBUG
	        printf(_("Message: Memory status error, waiting for pin.\n"));
#endif DEBUG
		CurrentMemoryStatusError = GE_INVALIDPIN;
		break;

	  default:
#ifdef DEBUG
	        printf(_("Message: Unknown Memory status error, subtype (MessageBuffer[4]) = %02x\n"),MessageBuffer[4]);
#endif DEBUG
		break;
	  }

      break;

    default:

#ifdef DEBUG
      printf(_("Message: Unknown message of type 0x03\n"));
#endif DEBUG

    }

    break;

    /* Phone status */
	  
 case 0x04:
    
    switch (MessageBuffer[3]) {

    case 0x02:

#ifdef DEBUG
      printf(_("Message: Phone status received:\n"));
      printf(_("   Mode: "));

      switch (MessageBuffer[4]) {

      case 0x01:

	printf(_("registered within the network\n"));

	break;
	      
	/* I was really amazing why is there a hole in the type of 0x02, now I
	   know... */
	      
      case 0x02:

	printf(_("call in progress\n")); /* ringing or already answered call */

	break;
	      
      case 0x03:

	printf(_("waiting for pin\n"));

	break;

      case 0x04:

	printf(_("powered off\n"));

	break;

      default:

	printf(_("unknown\n"));

      }

      printf(_("   Power source: "));

      switch (MessageBuffer[7]) {

      case 0x01:

	printf(_("AC/DC\n"));

	break;
		
      case 0x02:

	printf(_("battery\n"));

	break;

      default:

	printf(_("unknown\n"));

      }

      printf(_("   Battery Level: %d\n"), MessageBuffer[8]);
      printf(_("   Signal strength: %d\n"), MessageBuffer[5]);
#endif DEBUG

      CurrentRFLevel=MessageBuffer[5];
      CurrentBatteryLevel=MessageBuffer[8];
      CurrentPowerSource=MessageBuffer[7];

      break;

    default:

#ifdef DEBUG
      printf(_("Message: Unknown message of type 0x04\n"));
#endif DEBUG

    }

    break;

    /* PIN requests */

  case 0x08:

#ifdef DEBUG
    printf(_("Message: PIN:\n"));
#endif DEBUG

    if (MessageBuffer[3] != 0x0c) {

#ifdef DEBUG
      printf(_("   Code Accepted\n"));
#endif DEBUG

      PINError = GE_NONE;
    }
    else {

#ifdef DEBUG
      printf(_("   Code Error\n   You're not my big owner :-)\n"));
#endif DEBUG

      PINError = GE_INVALIDPIN;
    }

    break;

    /* Network status */

  case 0x0a:

#ifdef DEBUG
    printf(_("Message: Network registration:\n"));

    printf(_("   CellID: %02x%2x\n"), MessageBuffer[10], MessageBuffer[11]);
    printf(_("   LAC: %02x%02x\n"), MessageBuffer[12], MessageBuffer[13]);

    sprintf(output, "%x%x%x %x%x", MessageBuffer[14] & 0x0f, MessageBuffer[14] >>4, MessageBuffer[15] & 0x0f, MessageBuffer[16] & 0x0f, MessageBuffer[16] >>4);

    printf(_("   Network code: %s\n"), output);
    printf(_("   Network name: %s\n"), GSM_GetNetworkName(output));
#endif DEBUG

    break;

    /* Keyboard lock */

    /* FIXME: there are at least two types of 0x0d messages (length 09
       and 0b(?)). The exact meaning of all bytes except keyboard
       locked/unlocked is not known yet. */

  case 0x0d:

#ifdef DEBUG
    printf(_("Message: Keyboard lock\n"));
    printf(_("   Keyboard is %s\n"), (MessageBuffer[6]==2)?_("locked"):_("unlocked"));
#endif DEBUG

    break;

    /* Phone Clock and Alarm */

  case 0x11:

    switch (MessageBuffer[3]) {

    case 0x61:

#ifdef DEBUG
      printf(_("Message: Date and time set correctly\n"));
#endif DEBUG

      CurrentSetDateTimeError=GE_NONE;

      break;

    case 0x63:

      CurrentDateTime->Year=256*MessageBuffer[8]+MessageBuffer[9];
      CurrentDateTime->Month=MessageBuffer[10];
      CurrentDateTime->Day=MessageBuffer[11];

      CurrentDateTime->Hour=MessageBuffer[12];
      CurrentDateTime->Minute=MessageBuffer[13];
      CurrentDateTime->Second=MessageBuffer[14];

#ifdef DEBUG
      printf(_("Message: Date and time\n"));
      printf(_("   Time: %02d:%02d:%02d\n"), CurrentDateTime->Hour, CurrentDateTime->Minute, CurrentDateTime->Second);
      printf(_("   Date: %4d/%02d/%02d\n"), CurrentDateTime->Year, CurrentDateTime->Month, CurrentDateTime->Day);
#endif DEBUG

      CurrentDateTimeError=GE_NONE;

      break;

    case 0x6c:

#ifdef DEBUG
      printf(_("Message: Alarm set correctly\n"));
#endif DEBUG

      CurrentSetAlarmError=GE_NONE;

      break;

    case 0x6e:

#ifdef DEBUG
      printf(_("Message: Alarm\n"));
      printf(_("   Alarm: %02d:%02d\n"), MessageBuffer[9], MessageBuffer[10]);
      printf(_("   Alarm is %s\n"), (MessageBuffer[8]==2) ? _("on"):_("off"));
#endif DEBUG

      CurrentAlarm->Hour=MessageBuffer[9];
      CurrentAlarm->Minute=MessageBuffer[10];
      CurrentAlarm->Second=0;

      CurrentAlarm->AlarmEnabled=(MessageBuffer[8]==2);

      CurrentAlarmError=GE_NONE;

      break;

    default:

      printf(_("Message: Unknown message of type 0x11\n"));

    }

    break;

    /* Calendar notes handling */

  case 0x13:

    switch (MessageBuffer[3]) {

    case 0x67:

#ifdef DEBUG
      printf(_("Message: Calendar note received.\n"));

      printf(_("   Date: %d-%02d-%02d\n"), 256*MessageBuffer[9]+MessageBuffer[10], MessageBuffer[11], MessageBuffer[12]);

      printf(_("   Time: %02d:%02d:%02d\n"), MessageBuffer[13], MessageBuffer[14], MessageBuffer[15]);

      /* Some messages do not have alarm set up */

      if (256*MessageBuffer[16]+MessageBuffer[17] != 0) {
	printf(_("   Alarm date: %d-%02d-%02d\n"), 256*MessageBuffer[16]+MessageBuffer[17], MessageBuffer[18], MessageBuffer[19]);
	printf(_("   Alarm time: %02d:%02d:%02d\n"), MessageBuffer[20], MessageBuffer[21], MessageBuffer[22]);
      }

      printf(_("   Number: %d\n"), MessageBuffer[8]);
      printf(_("   Text: "));

      tmp=MessageBuffer[23];

      for(i=0;i<tmp;i++)
	printf("%c", MessageBuffer[24+i]);

      printf("\n");
#endif DEBUG

      break;

    default:

#ifdef DEBUG
      printf(_("Message: Unknown message of type 0x13\n"));
#endif DEBUG
   
    }

    break;

    /* SMS Messages frame received */

  case 0x14:

    if (CurrentSMSMessageError == GE_SMSWAITING) {

#ifdef DEBUG
      printf(_("Message: the rest of the SMS message received.\n"));
#endif DEBUG

      tmp=UnpackEightBitsToSeven(MessageLength-2, MessageBuffer, output);

      for (i=0; i<tmp;i++) {

#ifdef DEBUG
        printf("%c", GSM_Default_Alphabet[output[i]]);
#endif DEBUG

        CurrentSMSMessage->MessageText[CurrentSMSPointer+i]=GSM_Default_Alphabet[output[i]];
      }

      CurrentSMSMessage->MessageText[CurrentSMSPointer+tmp]=0;

      CurrentSMSMessageError = GE_NONE;

#ifdef DEBUG
      printf("\n");
#endif DEBUG

      break;
      
    }
    
    switch (MessageBuffer[3]) {

    case 0x08:

      switch (MessageBuffer[20]) {

      /* Nokia delivery report */
      case 0x06:
      
        offset = 3;
        report = true;
          
        break;
      
      /* SMS Message */
      case 0x24:
      /* If it is something else let's try to handle it like simple
       * SMS message, but maybe it's an error
       */
      default:
        offset = 4;
        report = false;
      }

      MessageBuffer[20+offset] = (MessageBuffer[20+offset]+1)/2+1;

#ifdef DEBUG
      if (report) printf(_("Message: Nokia delivery report.\n"));
      else printf(_("Message: SMS Received.\n"));
      printf(_("   Number: %d\n"), MessageBuffer[6]);

      printf(_("   Date: %s\n"), FB61_GetPackedDateTime(MessageBuffer+32+offset));
      printf(_("   SMS center number: %s\n"), FB61_GetBCDNumber(MessageBuffer+8));
      printf(_("   Remote number: %s\n"), FB61_GetBCDNumber(MessageBuffer+20+offset));
#endif DEBUG

      CurrentSMSMessage->Year=10*(MessageBuffer[32+offset]&0x0f)+(MessageBuffer[32+offset]>>4);
      CurrentSMSMessage->Month=10*(MessageBuffer[33+offset]&0x0f)+(MessageBuffer[33+offset]>>4);
      CurrentSMSMessage->Day=10*(MessageBuffer[34+offset]&0x0f)+(MessageBuffer[34+offset]>>4);
      CurrentSMSMessage->Hour=10*(MessageBuffer[35+offset]&0x0f)+(MessageBuffer[35+offset]>>4);
      CurrentSMSMessage->Minute=10*(MessageBuffer[36+offset]&0x0f)+(MessageBuffer[36+offset]>>4);
      CurrentSMSMessage->Second=10*(MessageBuffer[37+offset]&0x0f)+(MessageBuffer[37+offset]>>4);

      strcpy(CurrentSMSMessage->Sender, FB61_GetBCDNumber(MessageBuffer+20+offset));

      strcpy(CurrentSMSMessage->MessageCentre, FB61_GetBCDNumber(MessageBuffer+8));

      CurrentSMSMessage->Length=MessageBuffer[23];

      /* Fixes reading SMS Outbox msgs: skips first byte if null --GR */
      if (MessageBuffer[43] == 0) {

#ifdef DEBUG
         printf(_("Note: We are probably reading SMS Outbox message\n"));
#endif DEBUG

         tmp=UnpackEightBitsToSeven(MessageLength-44-2, MessageBuffer+44, output);
      }
      else
         if (!report) tmp=UnpackEightBitsToSeven(MessageLength-39-offset-2, MessageBuffer+39+offset, output);

      if (report) {
        switch (MessageBuffer[22]) {
        
        case 0x00:
        
#ifdef DEBUG
          printf(_("Delivered"));
#endif DEBUG
          strcpy(CurrentSMSMessage->MessageText,_("Delivered"));
          tmp = 10;
          break;

        case 0x30:
        
#ifdef DEBUG
          printf(_("Pending"));
#endif DEBUG
          strcpy(CurrentSMSMessage->MessageText,_("Pending"));
          tmp = 8;
          break;

        case 0x46:
        
#ifdef DEBUG
          printf(_("Failed"));
#endif DEBUG
          strcpy(CurrentSMSMessage->MessageText,_("Failed"));
          tmp = 7;
          break;

        default:
        
#ifdef DEBUG
          printf(_("Unknown"));
#endif DEBUG
          strcpy(CurrentSMSMessage->MessageText,_("Unknown"));
          tmp = 8;
        }
      } else {
        for (i=0; i<tmp;i++) {

#ifdef DEBUG
	  printf("%c", GSM_Default_Alphabet[output[i]]);
#endif DEBUG

	  CurrentSMSMessage->MessageText[i]=GSM_Default_Alphabet[output[i]];
	}
      }

      CurrentSMSMessage->MessageText[tmp]=0;

      CurrentSMSPointer=tmp;

      CurrentSMSMessage->MemoryType = MessageBuffer[5];
      CurrentSMSMessage->MessageNumber = MessageBuffer[6];
 
      /* Signal no error to calling code. */

      /* CurrentSMSMessage->Length > tmp indicates additional frames pending */
      if (CurrentSMSMessage->Length > tmp)
	CurrentSMSMessageError = GE_SMSWAITING;
      else
	CurrentSMSMessageError = GE_NONE;

#ifdef DEBUG
      printf("\n");
#endif DEBUG

      break;

    case 0x09:

      /* We have requested invalid or empty location. */

#ifdef DEBUG
      printf(_("Message: SMS reading failed.\n"));
#endif DEBUG

      switch (MessageBuffer[4]) {

      case 0x02:

#ifdef DEBUG
	printf(_("   Invalid memory type!\n"));
#endif DEBUG

	CurrentSMSMessageError = GE_INVALIDMEMORYTYPE;

	break;

      case 0x07:

#ifdef DEBUG
	printf(_("   Empty SMS location.\n"));
#endif DEBUG

	CurrentSMSMessageError = GE_EMPTYSMSLOCATION;

	break;
      }

      break;

    case 0x0b:	/* successful delete */

#ifdef DEBUG
      printf(_("Message: SMS deleted successfully.\n"));
#endif DEBUG

      CurrentSMSMessageError = GE_NONE;	

      break;

    case 0x37:

#ifdef DEBUG
      printf(_("Message: SMS Status Received\n"));
      printf(_("   The number of messages: %d\n"), MessageBuffer[10]);
      printf(_("   Unread messages: %d\n"), MessageBuffer[11]);
#endif DEBUG

      CurrentSMSStatus->UnRead = MessageBuffer[11];
      CurrentSMSStatus->Number = MessageBuffer[10];
      CurrentSMSStatusError = GE_NONE;

      break;

    case 0x38:

#ifdef DEBUG
      printf(_("Message: SMS Status error, probably not authorized by PIN\n"));
#endif DEBUG

      CurrentSMSStatusError = GE_INTERNALERROR;

      break;
	  
    default:
    
      CurrentSMSStatusError = GE_INTERNALERROR;

      break;

    }

    break;

    /* Mobile phone identification */

  case 0x64:

    /* We should skip the string `NOKIA' */

    snprintf(IMEI, FB61_MAX_IMEI_LENGTH, "%s", MessageBuffer+4+5);
    snprintf(Model, FB61_MAX_MODEL_LENGTH, "%s", MessageBuffer+25);
    snprintf(Revision, FB61_MAX_REVISION_LENGTH, "SW%s: HW%s\n", MessageBuffer+44, MessageBuffer+39);

#ifdef DEBUG
    printf(_("Message: Mobile phone identification received:\n"));
    printf(_("   IMEI: %s\n"), IMEI);

    printf(_("   Model: %s\n"), Model);

    printf(_("   Production Code: %s\n"), MessageBuffer+31);

    printf(_("   HW: %s\n"), MessageBuffer+39);

    printf(_("   Firmware: %s\n"), MessageBuffer+44);

    /* These bytes are probably the source of the "Accessory not connected"
       messages on the phone when trying to emulate NCDS... I hope... */

    printf(_("   Magic bytes: %02x %02x %02x %02x\n"), MessageBuffer[50], MessageBuffer[51], MessageBuffer[52], MessageBuffer[53]);
#endif DEBUG

    MagicBytes[0]=MessageBuffer[50];
    MagicBytes[1]=MessageBuffer[51];
    MagicBytes[2]=MessageBuffer[52];
    MagicBytes[3]=MessageBuffer[53];

    CurrentMagicError=GE_NONE;

    break;

    /* Acknowlegment */

  case 0x7f:

#ifdef DEBUG
    printf(_("[Received Ack of type %02x, seq: %2x]\n"), MessageBuffer[0], MessageBuffer[1]);
#endif DEBUG

    FB61_LinkOK = true;

    break;

    /* Power on message */

  case 0xd0:

#ifdef DEBUG
    printf(_("Message: The phone is powered on - seq 1.\n"));
#endif DEBUG

    break;

    /* Power on message */

  case 0xf4:

#ifdef DEBUG
    printf(_("Message: The phone is powered on - seq 2.\n"));
#endif DEBUG

    break;

    /* Unknown message - if you think that you know the exact meaning of
       other messages - please let us know. */

  default:

    FB61_RX_DisplayMessage();
  }

  return FB61_RX_Sync;
}

/* RX_State machine for receive handling.  Called once for each character
   received from the phone/phone. */
void FB61_RX_StateMachine(char rx_byte) {

  static int checksum[2];

  /* XOR the byte with the current checksum */
  checksum[BufferCount&1] ^= rx_byte;

  switch (RX_State) {
	
    /* Messages from the phone start with an 0x1e.  We use this to
       "synchronise" with the incoming data stream. */

  case FB61_RX_Sync:

    if ( CurrentConnectionType == GCT_Infrared ) {
      if (rx_byte == FB61_IR_FRAME_ID) {
	BufferCount = 0;
	RX_State = FB61_RX_GetDestination;
	
	/* Initialize checksums. */
	checksum[0] = FB61_IR_FRAME_ID;
	checksum[1] = 0;
      }
    } else { /* CurrentConnectionType == GCT_Serial */
      if (rx_byte == FB61_FRAME_ID) {
	BufferCount = 0;
	RX_State = FB61_RX_GetDestination;
	
	/* Initialize checksums. */
	checksum[0] = FB61_FRAME_ID;
	checksum[1] = 0;
      }
    }
    
    break;

  case FB61_RX_GetDestination:

    MessageDestination=rx_byte;
    RX_State = FB61_RX_GetSource;

    break;

  case FB61_RX_GetSource:

    MessageSource=rx_byte;
    RX_State = FB61_RX_GetType;

    break;

  case FB61_RX_GetType:

    MessageType=rx_byte;
    RX_State = FB61_RX_GetUnknown;

    break;

  case FB61_RX_GetUnknown:

    MessageUnknown=rx_byte;
    RX_State = FB61_RX_GetLength;

    break;
    
  case FB61_RX_GetLength:

    MessageLength = rx_byte;
    RX_State = FB61_RX_GetMessage;

    break;
    
  case FB61_RX_GetMessage:

    MessageBuffer[BufferCount] = rx_byte;
    BufferCount ++;

    /* If this is the last byte, it's the checksum. */

    if (BufferCount == MessageLength+(MessageLength%2)+2) {

      /* Is the checksum correct? */

      if (checksum[0] == checksum[1]) {

      /* We do not want to send ACK of ACKs. */

	if (MessageType != FB61_FRTYPE_ACK)
	  FB61_TX_SendAck(MessageType, MessageBuffer[MessageLength-1] & 0x0f);

	FB61_RX_DispatchMessage();
      }

#ifdef DEBUG
      else
	printf("Bad checksum!\n");
#endif DEBUG

      RX_State = FB61_RX_Sync;
    }

    break;
    
  }
}

char *FB61_PrintDevice(int Device)
{

  switch (Device) {

  case FB61_DEVICE_PHONE:
    return _("Phone");

  case FB61_DEVICE_PC:
    return _("PC");

  default:
    return _("Unknown");
  }
}

/* FB61_RX_DisplayMessage is called when a message we don't know about is
   received so that the user can see what is going back and forth, and perhaps
   shed some more light/explain another message type! */
	
void FB61_RX_DisplayMessage(void)
{

#ifdef DEBUG

  int count;

  fprintf(stdout, _("Msg Dest: %s\n"), FB61_PrintDevice(MessageDestination));
  fprintf(stdout, _("Msg Source: %s\n"), FB61_PrintDevice(MessageSource));

  fprintf(stdout, _("Msg Type: %02x\n"), MessageType);

  fprintf(stdout, _("Msg Unknown: %02x\n"), MessageUnknown);
  fprintf(stdout, _("Msg Len: %02x\nPhone: "), MessageLength);

  for (count = 0; count < MessageLength+(MessageLength%2)+2; count ++)
    if (isprint(MessageBuffer[count]))
      fprintf(stdout, "[%02x%c]", MessageBuffer[count], MessageBuffer[count]);
    else
      fprintf(stdout, "[%02x ]", MessageBuffer[count]);

  fprintf(stdout, "\n");
  fflush(stdout);
#endif DEBUG
}

/* Prepares the message header and sends it, prepends the message start byte
	   (0x1e) and other values according the value specified when called.
	   Calculates checksum and then sends the lot down the pipe... */

int FB61_TX_SendMessage(u8 message_length, u8 message_type, u8 *buffer)
{
  u8 out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
  int count, current=0;
  unsigned char	checksum;

  /* FIXME - we should check for the message length ... */

  /* Now construct the message header. */

  if (CurrentConnectionType == GCT_Infrared)
    out_buffer[current++] = FB61_IR_FRAME_ID; /* Start of the IR frame indicator */
  else /* CurrentConnectionType == GCT_Serial */
    out_buffer[current++] = FB61_FRAME_ID;    /* Start of the frame indicator */

  out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
  out_buffer[current++] = FB61_DEVICE_PC;    /* Source */

  out_buffer[current++] = message_type; /* Type */

  out_buffer[current++] = 0; /* Unknown */

  out_buffer[current++] = message_length+2; /* Length + 2 for seq. nr */

  /* Copy in data if any. */	
	
  if (message_length != 0) {
    memcpy(out_buffer + current, buffer, message_length);
    current+=message_length;
  }

  out_buffer[current++]=0x01;

  out_buffer[current++]=0x40+RequestSequenceNumber;

  RequestSequenceNumber=(RequestSequenceNumber+1) & 0x07;

  /* If the message length is odd we should add pad byte 0x00 */
  if (message_length % 2)
    out_buffer[current++]=0x00;

  /* Now calculate checksums over entire message and append to message. */

  /* Odd bytes */

  checksum = 0;
  for (count = 0; count < current; count+=2)
    checksum ^= out_buffer[count];

  out_buffer[current++] = checksum;

  /* Even bytes */

  checksum = 0;
  for (count = 1; count < current; count+=2)
    checksum ^= out_buffer[count];

  out_buffer[current++] = checksum;

#ifdef DEBUG
  printf(_("PC: "));

  for (count = 0; count < current; count++)
    printf("%02x:", out_buffer[count]);

  printf("\n");
#endif DEBUG

  /* Send it out... */
  
  if (write(PortFD, out_buffer, current) != current)
    return (false);

  return (true);
}

int FB61_TX_SendAck(u8 message_type, u8 message_seq) {

  unsigned char out_buffer[10]; /* Acks are always 10 char */
  int count, current=0;
  unsigned char checksum;

#ifdef DEBUG

  printf(_("[Sending Ack of type %02x, seq: %x]\n"), message_type, message_seq);

#endif DEBUG

  /* Now construct the Ack header. */

  if (CurrentConnectionType == GCT_Infrared)
    out_buffer[current++] = FB61_IR_FRAME_ID; /* Start of the IR frame indicator */
  else /* CurrentConnectionType == GCT_Serial */
    out_buffer[current++] = FB61_FRAME_ID;    /* Start of the frame indicator */

  out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
  out_buffer[current++] = FB61_DEVICE_PC;    /* Source */

  out_buffer[current++] = FB61_FRTYPE_ACK; /* Ack */

  out_buffer[current++] = 0; /* Unknown */
  out_buffer[current++] = 2; /* Ack is always of 2 bytes */

  out_buffer[current++] = message_type; /* Type */
  out_buffer[current++] = message_seq;  /* Sequence number */

  /* Now calculate checksums over entire message and append to message. */

  /* Odd bytes */

  checksum = 0;
  for (count = 0; count < current; count+=2)
    checksum ^= out_buffer[count];

  out_buffer[current++] = checksum;

  /* Even bytes */

  checksum = 0;
  for (count = 1; count < current; count+=2)
    checksum ^= out_buffer[count];

  out_buffer[current++] = checksum;

  /* Send it out... */

  if (write(PortFD, out_buffer, current) != current)
    return (false);

  return true;  
}
