	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobiles.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  gsm-api.c  Version 0.2.3
	
	   Provides a generic API for accessing functions on the
	   phone, wherever possible hiding the model specific
	   details.

	   The underlying code should run in it's own thread to allow
	   communications to the phone to be run independantly of
	   mailine code that calls these API functions.

	   Unless otherwise noted, all functions herein block
	   until they complete.  The functions themselves are defined
	   in a structure in gsm-common.h */

#include	<stdio.h>
#include	<string.h>
#include	<libintl.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"fbus-3810.h"
#include	"fbus-6110.h"

	/* GSM_LinkOK is set to true once normal communications with
	   the phone have been established. */
bool				*GSM_LinkOK;

	/* Define pointer to the GSM_Functions structure used by external 
	   code to call relevant API functions.  This structure is defined
	   in gsm-common.h */
GSM_Functions		*GSM;

	/* Define pointer to the GSM_Information structure used by external
	   code to obtain information that varies from model to model.  This
	   structure is also defined in gsm-common.h */
GSM_Information		*GSM_Info;

	/* GSM_Initialise
	   Initialise interface to phone.  Model number should be a 
	   string such as 3810, 6110 etc.  Device is the serial
	   port to use eg /dev/ttyS0, the user must have write permission
	   to the device.  Finally, if enable_monitoring is true and
	   the model specific code supports it, additional information
	   will be output when communicating with the phone. */
GSM_Error	GSM_Initialise(char *model, char *device, bool enable_monitoring)
{
	bool			found_match;

	found_match = false;

		/* Scan through the models supported by the FB38 code and see
	 	   if we get a match. */
	if (strstr(FB38_Information.Models, model) != NULL) {
		found_match = true;

			/* Set pointers to relevant FB38 addresses */
		GSM = &FB38_Functions;
		GSM_Info = &FB38_Information;
		GSM_LinkOK = &FB38_LinkOK;
	}

		/* Try 6110 models... */
	if (strstr(FB61_Information.Models, model) != NULL) {
		found_match = true;

			/* Set pointers to relevant FB61 addresses */
		GSM = &FB61_Functions;
		GSM_Info = &FB61_Information;
		GSM_LinkOK = &FB61_LinkOK;
	}

		/* If we didn't get a model match, return error code. */
	if (found_match == false) {
		return (GE_UNKNOWNMODEL);
	}
		/* Now call model specific Initialisation code. */
	return (GSM->Initialise(device, enable_monitoring));
}


	/* GSM_TranslateError
	   This function will provide a string corresponding to the supplied
	   GSM_Error code.  For example, passed GE_UNKNOWNMODEL it would
	   return a pointer to the static string "Unknown Model!" */

/* PJ: Will be great :-) */
