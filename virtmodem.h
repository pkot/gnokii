/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	virtmodem.h - Header file for virtmodem code in virtmodem.c


*/

#ifndef __virtmodem_h
#define __virtmodem_h

	/* Prototypes */
bool	VM_Initialise(bool debug_mode);
int		VM_PtySetup(void);
void	VM_ThreadLoop(void);
void    VM_CharHandler(void);
int		VM_GetMasterPty(char **name);
GSM_Error 	VM_GSMInitialise(void);

	/* All defines and prototypes from here down are specific to 
	   the virtual modem code and so are #ifdef out if __virtmodem_c isn't 
	   defined. */
#ifdef	__virtmodem_c









#endif	/* __virtmodem_c */

#endif	/* __virtmodem_h */
