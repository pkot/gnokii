/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the statemachine.

  $Log$
  Revision 1.2  2001-06-27 23:52:52  pkot
  7110/6210 updates (Marian Jancar)

  Revision 1.1  2001/03/21 23:36:07  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110


*/

#ifndef __gsm_statemachine_h
#define __gsm_statemachine_h

#include "gsm-common.h"

GSM_Error SM_Initialise(GSM_Statemachine *state);
GSM_State SM_Loop(GSM_Statemachine *state, int timeout);
GSM_Error SM_SendMessage(GSM_Statemachine *state, u16 messagesize, u8 messagetype, void *message);
GSM_Error SM_WaitFor(GSM_Statemachine *state, GSM_Data *data, unsigned char messagetype);
void SM_IncomingFunction(GSM_Statemachine *state, u8 messagetype, void *message, u16 messagesize);
void SM_Reset(GSM_Statemachine *state);
GSM_Error SM_GetError(GSM_Statemachine *state, unsigned char messagetype);
GSM_Error SM_Block(GSM_Statemachine *state, GSM_Data *data, int waitfor);
GSM_Error SM_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *sm);

#endif	/* __gsm_statemachine_h */
