/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See gsm-api.c for more details.

  Last modification: Sun May 16 21:05:42 CEST 1999
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
#ifndef __rlp_common_h
  #include "rlp-common.h"
#endif

/* Define these as externs so that app code can pick them up. */

extern bool *GSM_LinkOK;
extern GSM_Information *GSM_Info;
extern GSM_Functions *GSM;

/* Prototype for the functions actually provided by gsm-api.c. */

/*GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, bool enable_monitoring);*/
GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, bool enable_monitoring, void (*rlp_handler)(RLP_F96Frame *frame));


/* All the rest of the API functions are contained in the GSM_Function
   structure which ultimately points into the model specific code. */

#endif	/* __gsm_api_h */
