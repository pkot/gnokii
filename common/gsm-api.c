/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Provides a generic API for accessing functions on the phone, wherever
  possible hiding the model specific details.

  The underlying code should run in it's own thread to allow communications to
  the phone to be run independantly of mailing code that calls these API
  functions.

  Unless otherwise noted, all functions herein block until they complete.  The
  functions themselves are defined in a structure in gsm-common.h.

  Last modification: Sun May 16 21:05:42 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "gsm-common.h"
#include "fbus-3810.h"
#include "fbus-6110.h"

/* GSM_LinkOK is set to true once normal communications with the phone have
   been established. */

bool *GSM_LinkOK;

/* Define pointer to the GSM_Functions structure used by external code to call
   relevant API functions. This structure is defined in gsm-common.h. */

GSM_Functions *GSM;

/* Define pointer to the GSM_Information structure used by external code to
   obtain information that varies from model to model. This structure is also
   defined in gsm-common.h */

GSM_Information		*GSM_Info;

/* Initialise interface to the phone. Model number should be a string such as
   3810, 5110, 6110 etc. Device is the serial port to use e.g. /dev/ttyS0, the
   user must have write permission to the device. Finally, if
   enable_monitoring is true and the model specific code supports it,
   additional information will be output when communicating with the phone. */

GSM_Error GSM_Initialise(char *model, char *device, bool enable_monitoring)
{
  bool found_match=false;

  /* Scan through the models supported by the FB38 code and see if we get a
     match. */

  if (strstr(FB38_Information.Models, model) != NULL) {
    found_match = true;

    /* Set pointers to relevant FB38 addresses */

    GSM = &FB38_Functions;
    GSM_Info = &FB38_Information;
    GSM_LinkOK = &FB38_LinkOK;
  }
  else

  /* Scan through the models supported by the FB61 code and see if we get a
     match. */

    if (strstr(FB61_Information.Models, model) != NULL) {
      found_match = true;

      /* Set pointers to relevant FB61 addresses */

      GSM = &FB61_Functions;
      GSM_Info = &FB61_Information;
      GSM_LinkOK = &FB61_LinkOK;
    }

  /* If we didn't get a model match, return error code. */

  if (found_match == false)
    return (GE_UNKNOWNMODEL);

  /* Now call model specific initialisation code. */

  return (GSM->Initialise(device, enable_monitoring));
}
