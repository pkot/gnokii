/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Chris Kemp

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions via fbus over irda. 
  See README for more details on supported mobile phones.

  The various routines are called PHONET_(whatever).

  $Log$
  Revision 1.1  2001-02-21 19:57:12  chris
  More fiddling with the directory layout

  Revision 1.1  2001/02/06 21:15:37  chris
  Preliminary irda support for 7110 etc.  Not well tested!


*/

#ifndef __links_fbus_phonet_h
#define __links_fbus_phonet_h


#define PHONET_MAX_FRAME_LENGTH    1010
#define PHONET_MAX_TRANSMIT_LENGTH 1010
#define PHONET_MAX_CONTENT_LENGTH  1000


/* This byte is at the beginning of all GSM Frames sent over PhoNet. */
#define FBUS_PHONET_FRAME_ID 0x14


GSM_Error PHONET_Initialise(GSM_Link *newlink, GSM_Phone *newphone);


#ifdef __links_fbus_phonet_c  /* Prototype functions for fbus-phonet.c only */

typedef struct{
  int BufferCount;
  enum FBUS_RX_States state;
  int MessageSource;
  int MessageDestination;
  int MessageType;
  int MessageLength;
  char MessageBuffer[PHONET_MAX_FRAME_LENGTH];
} PHONET_IncomingMessage;


bool PHONET_OpenSerial();
void PHONET_RX_StateMachine(unsigned char rx_byte);
int PHONET_TX_SendFrame(u8 message_length, u8 message_type, u8 *buffer);
GSM_Error PHONET_SendMessage(u16 messagesize, u8 messagetype, void *message);
int PHONET_TX_SendAck(u8 message_type, u8 message_seq);

#endif   /* #ifdef __links_fbus_phonet_c */

#endif   /* #ifndef __links_fbus_phonet_h */
