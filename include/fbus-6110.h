	/* G N O K I I
	   A Linux/Unix toolset and driver for the Nokia 3110/6110/8110 mobiles.
	   Copyright (C) Hugh Blemings ?????, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  fbus-6110.h   Version 0.2.3

	   Header file for the various functions, definitions etc. used
	   to implement the handset interface.  See fbus-6110.c for more details. */

#ifndef		__fbus_6110_h
#define		__fbus_6110_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

	/* Global variables */
extern bool					FB61_LinkOK;
extern GSM_Functions		FB61_Functions;
extern GSM_Information		FB61_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error	FB61_Initialise(char *port_device, bool enable_monitoring);
bool		FB61_OpenSerial(void);
void		FB61_Terminate(void);

GSM_Error	FB61_GetPhonebookLocation(GSM_MemoryType memory_type, int location,
				GSM_PhonebookEntry *entry);

GSM_Error	FB61_WritePhonebookLocation(GSM_MemoryType memory_type, 
				int location, GSM_PhonebookEntry *entry);

GSM_Error	FB61_GetSMSMessage(GSM_MemoryType memory_type, int location,
				 GSM_SMSMessage *message);

GSM_Error	FB61_SendSMSMessage(char *message_centre, char *destination,
				 char *text, u8 *return_code1, u8 *return_code2);

GSM_Error	FB61_GetRFLevel(float *level);
GSM_Error	FB61_GetBatteryLevel(float *level);
GSM_Error	FB61_EnterPin(char *pin);
GSM_Error	FB61_GetIMEIAndCode(char *imei, char *code);

	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __fbus_6110_c isn't 
	   defined. */
#ifdef	__fbus_6110_c

#define		FB61_BAUDRATE		(B115200)

void		FB61_Terminate(void);
void		FB61_ThreadLoop(void);
bool		FB61_OpenSerial(void);
void		FB61_SigHandler(int status);

	/* Insert code here as required ? */


#endif	/* __fbus_6110_c */

#endif	/* __fbus_6110_h */
