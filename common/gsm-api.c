/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Provides a generic API for accessing functions on the phone, wherever
  possible hiding the model specific details.

  The underlying code should run in it's own thread to allow communications to
  the phone to be run independantly of mailing code that calls these API
  functions.

  Unless otherwise noted, all functions herein block until they complete.  The
  functions themselves are defined in a structure in gsm-common.h.

  $Log$
  Revision 1.29  2001-07-27 00:02:20  pkot
  Generic AT support for the new structure (Manfred Jonsson)

  Revision 1.28  2001/06/06 09:05:56  machek
  Convert Grab/Release display to new structure.

  Revision 1.27  2001/05/07 14:13:06  machek
  nokia-2110 module converted to suit new API better. --identify now works.

  Revision 1.26  2001/04/25 12:54:47  machek
  Partly converted nokia 2110 to "new" form, and moved it to phone
  directory.

  Revision 1.25  2001/03/26 23:39:21  pkot
  Minor updates:
   - Windows INLINE patch (Manfred Jonsson)
   - patch to configure.in to compile under FreeBSD (Panagiotis Astithas)
   - other cleanups (me)

  
*/

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "gsm-common.h"
#include "data/rlp-common.h"
#include "gsm-statemachine.h"
#include "fbus-3810.h"
#include "fbus-6110.h"
#include "mbus-2110.h"
#include "mbus-6160.h"
#include "mbus-640.h"
#include "phones/nk7110.h"

GSM_Statemachine GSM_SM;
GSM_Error (*GSM_F)(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);


/* GSM_LinkOK is set to true once normal communications with the phone have
   been established. */

bool *GSM_LinkOK;
bool LinkAlwaysOK = true;

/* Define pointer to the GSM_Functions structure used by external code to call
   relevant API functions. This structure is defined in gsm-common.h. */

GSM_Functions *GSM;

/* Define pointer to the GSM_Information structure used by external code to
   obtain information that varies from model to model. This structure is also
   defined in gsm-common.h */

GSM_Information		*GSM_Info;

/* Initialise interface to the phone. Model number should be a string such as
   3810, 5110, 6110 etc. Device is the serial port to use e.g. /dev/ttyS0, the
   user must have write permission to the device. */

static GSM_Error register_phone(GSM_Phone *phone, char *model, GSM_Statemachine *sm)
{
       if (strstr(phone->Info.Models, model) != NULL)
               return phone->Functions(GOP_Init, NULL, sm);
       return GE_UNKNOWNMODEL;
}

#define MODULE(x) { \
	extern GSM_Functions x##_Functions; \
	extern GSM_Information x##_Information; \
	extern bool x##_LinkOK; \
	if (strstr(x##_Information.Models, model) != NULL) { \
		GSM = & x##_Functions; \
	        GSM_Info = & x##_Information; \
		GSM_LinkOK = & x##_LinkOK; \
		return (GSM->Initialise(device, initlength, connection, rlp_callback)); \
	} \
}

#define REGISTER_PHONE(x) { \
        extern GSM_Phone phone_##x; \
        if ((ret = register_phone(&phone_##x, model, sm)) != GE_UNKNOWNMODEL) \
                return ret; \
 } 

 
GSM_Error GSM_Initialise(char *model, char *device, char *initlength, GSM_ConnectionType connection, void (*rlp_callback)(RLP_F96Frame *frame), GSM_Statemachine *sm)
{
        GSM_Error ret;
        MODULE(FB38);
        MODULE(FB61);
#ifndef WIN32  /* MB21 not supported in win32 */
        MODULE(MB61);
        MODULE(MB640);
        MODULE(D2711);
	if (strstr("2110", model)) {
		extern GSM_Phone phone_nokia_2110;
		memcpy(&(sm->Phone), &phone_nokia_2110, sizeof(GSM_Phone));
		sm->Phone.Functions(GOP_Init, NULL, sm);
	}
	MODULE(N2110);
 
        GSM_LinkOK = &LinkAlwaysOK;
        sm->Link.ConnectionType=connection;
        sm->Link.InitLength=atoi(initlength);
        strcpy(sm->Link.PortDevice,device);
 
        REGISTER_PHONE(nokia_7110);
        REGISTER_PHONE(at);

#endif /* WIN32 */ 
        return (GE_UNKNOWNMODEL);
}
