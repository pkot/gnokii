/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for data pump code.
	
  Last modification: Mon May 15th
  Modified by Chris Kemp <ck231@cam.ac.uk>

*/

#ifndef __datapump_h
#define __datapump_h 

	/* Prototypes */

void    DP_CallFinished(void);
bool	DP_Initialise(int read_fd, int write_fd);
int     DP_CallBack(RLP_UserInds ind, u8 *buffer, int length);
void    DP_CallPassup(char c);

	/* All defines and prototypes from here down are specific to 
	   the datapump code and so are #ifdef out if __datapump_c isn't 
	   defined. */
#ifdef	__datapump_c







#endif	/* __datapump_c */

#endif	/* __datapump_h */
