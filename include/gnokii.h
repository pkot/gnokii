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

  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See gsm-api.c for more details.

*/

#ifndef __gsm_api_h
#define __gsm_api_h

#include "gsm-data.h"
#include "data/rlp-common.h"
#include "gsm-statemachine.h"

/* Define these as externs so that app code can pick them up. */

extern GSM_Information *GSM_Info;
extern GSM_Error (*GSM_F)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);

/* Prototype for the functions actually provided by gsm-api.c. */

GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, void (*rlp_handler)(RLP_F96Frame *frame), GSM_Statemachine *sm);

/* SMS Functions */
/* Sending */
GSM_Error SendSMS(GSM_Data *data, GSM_Statemachine *state);
GSM_Error SaveSMS(GSM_Data *data, GSM_Statemachine *state);
/* Reading */
GSM_Error ParseSMS(GSM_Data *data, int offset);
GSM_Error RequestSMS(GSM_Data *data, GSM_Statemachine *state);
GSM_Error GetSMS(GSM_Data *data, GSM_Statemachine *state);
GSM_Error GetSMSnoValidate(GSM_Data *data, GSM_Statemachine *state);
GSM_Error GetFolderChanges(GSM_Data *data, GSM_Statemachine *state, int has_folders);
/* Default values */
void DefaultSMS(GSM_SMSMessage *SMS);
void DefaultSubmitSMS(GSM_SMSMessage *SMS);
void DefaultDeliverSMS(GSM_SMSMessage *SMS);

/* All the rest of the API functions are contained in the GSM_Function
   structure which ultimately points into the model specific code. */

#endif	/* __gsm_api_h */
