/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for virtmodem code in virtmodem.c

  Last modification: Mon Mar 20 21:40:04 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __virtmodem_h
#define __virtmodem_h

/* Prototypes */

bool VM_Initialise(char *model,
		   char *port,
		   char *initlength,
		   GSM_ConnectionType connection,
		   char *bindir,
		   bool debug_mode,
		   bool GSM_Init);
int  VM_PtySetup(char *bindir);
void VM_ThreadLoop(void);
void VM_CharHandler(void);
int  VM_GetMasterPty(char **name);
void VM_Terminate(void);
GSM_Error VM_GSMInitialise(char *model,
			   char *port,
			   char *initlength,
			   GSM_ConnectionType connection);

#endif	/* __virtmodem_h */
