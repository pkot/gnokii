/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Log$
  Revision 1.2  2001-08-20 23:27:37  pkot
  Add hardware shakehand to the link layer (Manfred Jonsson)

  Revision 1.1  2001/07/27 00:02:22  pkot
  Generic AT support for the new structure (Manfred Jonsson)


*/
                
#ifndef __atbus_h
#define __atbus_h

GSM_Error ATBUS_Initialise(GSM_Statemachine *state, int hw_handshake);

#ifdef __atbus_c  /* Prototype functions for atbus.c only */

GSM_Error ATBUS_Loop(struct timeval *timeout);
bool ATBUS_OpenSerial(int hw_handshake);
void ATBUS_RX_StateMachine(unsigned char rx_char);

#endif   /* #ifdef __atbus_c */

#endif   /* #ifndef __atbus_h */
