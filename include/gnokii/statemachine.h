/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Header file for the statemachine.

*/

#ifndef __gsm_statemachine_h
#define __gsm_statemachine_h

#include "gsm-data.h"

gn_error SM_Initialise(GSM_Statemachine *state);
API GSM_State SM_Loop(GSM_Statemachine *state, int timeout);
gn_error SM_SendMessage(GSM_Statemachine *state, u16 messagesize, u8 messagetype, void *message);
gn_error SM_WaitFor(GSM_Statemachine *state, GSM_Data *data, unsigned char messagetype);
void SM_IncomingFunction(GSM_Statemachine *state, u8 messagetype, void *message, u16 messagesize);
void SM_Reset(GSM_Statemachine *state);
gn_error SM_GetError(GSM_Statemachine *state, unsigned char messagetype);
gn_error SM_BlockTimeout(GSM_Statemachine *state, GSM_Data *data, int waitfor, int t);
gn_error SM_Block(GSM_Statemachine *state, GSM_Data *data, int waitfor);
gn_error SM_BlockNoRetryTimeout(GSM_Statemachine *state, GSM_Data *data, int waitfor, int t);
gn_error SM_BlockNoRetry(GSM_Statemachine *state, GSM_Data *data, int waitfor);
API gn_error SM_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *sm);
void SM_DumpMessage(int messagetype, unsigned char *message, int length);
void SM_DumpUnhandledFrame(GSM_Statemachine *state, int messagetype, unsigned char *message, int length);

#endif	/* __gsm_statemachine_h */
