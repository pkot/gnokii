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
  Copyright (C) 2001-2002 Pawe³ Kot
  
  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See gsm-api.c for more details.

*/

#ifndef _gnokii_gsm_api_h
#define _gnokii_gsm_api_h

#include "gsm-data.h"
#include "data/rlp-common.h"
#include "gsm-statemachine.h"

/* Define these as externs so that app code can pick them up. */
extern API GSM_Information *gn_gsm_info;
extern API GSM_Error (*gn_gsm_f)(GSM_Operation op, GSM_Data *data,
				 GSM_Statemachine *state);

/* Prototype for the functions actually provided by gsm-api.c. */
API GSM_Error gn_gsm_initialise(char *model, char *device, char *initlength,
			        GSM_ConnectionType connection,
			        void (*rlp_handler)(RLP_F96Frame *frame),
			        GSM_Statemachine *sm);

/* SMS Functions */
API GSM_Error gn_sms_send(GSM_Data *data, GSM_Statemachine *state);
API GSM_Error gn_sms_save(GSM_Data *data, GSM_Statemachine *state);
API GSM_Error gn_sms_get(GSM_Data *data, GSM_Statemachine *state);
API GSM_Error gn_sms_get_no_validate(GSM_Data *data, GSM_Statemachine *state);
API GSM_Error gn_sms_get_folder_changes(GSM_Data *data, GSM_Statemachine *state,
					int has_folders);
API GSM_Error gn_sms_delete(GSM_Data *data, GSM_Statemachine *state);
API GSM_Error gn_sms_delete_no_validate(GSM_Data *data, GSM_Statemachine *state);
API void gn_sms_default_submit(GSM_API_SMS *sms);
API void gn_sms_default_deliver(GSM_API_SMS *sms);

/* Not exported */
GSM_Error sms_parse(GSM_Data *data, int offset);
GSM_Error sms_request(GSM_Data *data, GSM_Statemachine *state);

#endif	/* _gnokii_gsm_api_h */
