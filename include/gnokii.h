/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See gsm-api.c for more details.

  Last modification: Mon Mar 20 21:51:59 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __gsm_api_h
#define __gsm_api_h

/* If gsm-common.h isn't included at this point, little in this file will make
   sense so we include it here if required. */

#ifndef __gsm_common_h
  #include "gsm-common.h"
#endif

/* Ditto rlp_common.h... */
#ifndef __data_rlp_common_h
  #include "data/rlp-common.h"
#endif

#include "gsm-statemachine.h"

/* Define these as externs so that app code can pick them up. */

extern bool *GSM_LinkOK;
extern GSM_Information *GSM_Info;
extern GSM_Functions *GSM;
extern GSM_Error (*GSM_F)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);

/* Prototype for the functions actually provided by gsm-api.c. */

GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, void (*rlp_handler)(RLP_F96Frame *frame), GSM_Statemachine *sm);

/* All the rest of the API functions are contained in the GSM_Function
   structure which ultimately points into the model specific code. */

#endif	/* __gsm_api_h */
