/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus. 
  See README for more details on supported mobile phones.

  The various routines are called FBUS_(whatever).

  $Log$
  Revision 1.3  2001-02-06 21:15:36  chris
  Preliminary irda support for 7110 etc.  Not well tested!

  Revision 1.2  2001/01/17 02:54:55  chris
  More 7110 work.  Use with care! (eg it is not possible to delete phonebook entries)
  I can now edit my phonebook in xgnokii but it is 'work in progress'.

  Revision 1.1  2001/01/14 22:47:01  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure


*/

#ifndef __fbus_generic_h
#define __fbus_generic_h

#include <sys/time.h>

#define FBUS_MAX_FRAME_LENGTH 256
#define FBUS_MAX_MESSAGE_TYPES 128
#define FBUS_MAX_TRANSMIT_LENGTH 256
#define FBUS_MAX_CONTENT_LENGTH 120

/* Nokia mobile phone. */
#define FBUS_DEVICE_PHONE 0x00

/* Our PC. */
#define FBUS_DEVICE_PC 0x0c


/* This byte is at the beginning of all GSM Frames sent over FBUS to Nokia
   phones.  This may have to become a phone dependant parameter... */
#define FBUS_FRAME_ID 0x1e

/* This byte is at the beginning of all GSM Frames sent over IR to Nokia phones. */
#define FBUS_IR_FRAME_ID 0x1c


/* Every (well, almost every) frame from the computer starts with this
   sequence. */

#define FBUS_FRAME_HEADER 0x00, 0x01, 0x00


/* States for receive code. */

enum FBUS_RX_States {
  FBUS_RX_Sync,
  FBUS_RX_Discarding,
  FBUS_RX_GetDestination,
  FBUS_RX_GetSource,
  FBUS_RX_GetType,
  FBUS_RX_GetLength1,
  FBUS_RX_GetLength2,
  FBUS_RX_GetMessage
};


typedef struct{
  int checksum[2];
  int BufferCount;
  struct timeval time_now;
  struct timeval time_last;
  enum FBUS_RX_States state;
  int MessageSource;
  int MessageDestination;
  int MessageType;
  int FrameLength;
  char MessageBuffer[FBUS_MAX_FRAME_LENGTH];
} FBUS_IncomingFrame;

typedef struct{
  int MessageLength;
  unsigned char *MessageBuffer;
  char FramesToGo;
  int Malloced;
} FBUS_IncomingMessage;

typedef struct {
  u16 message_length;
  u8 message_type;
  u8 *buffer;
} FBUS_OutgoingMessage;


typedef struct{
  FBUS_IncomingFrame i;
  FBUS_IncomingMessage messages[FBUS_MAX_MESSAGE_TYPES];
  u8 RequestSequenceNumber;
} FBUS_Link;

GSM_Error FBUS_Initialise(GSM_Link *newlink, GSM_Phone *newphone);



#ifdef __fbus_generic_c  /* Prototype functions for fbus-generic.c only */

bool FBUS_OpenSerial();
void FBUS_RX_StateMachine(unsigned char rx_byte);
int FBUS_TX_SendFrame(u8 message_length, u8 message_type, u8 *buffer);
GSM_Error FBUS_SendMessage(u16 messagesize, u8 messagetype, void *message);
int FBUS_TX_SendAck(u8 message_type, u8 message_seq);

#endif   /* #ifdef __fbus_generic_c */

#endif   /* #ifndef __fbus_generic_h */








