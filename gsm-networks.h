/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Pavel Janík ml. & Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Header file for GSM networks.

  Last modification: Sun May  2 12:42:56 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __gsm_networks_h
#define __gsm_networks_h

/* This type is used to hold information about various GSM networks. */

typedef struct {
  char *Code; /* GSM network code */
  char *Name; /* GSM network name */
} GSM_Network;

/* GSM networks data. */

/* These functions are used to search the structure defined above.*/
char *GSM_GetNetworkName(char *NetworkCode);
char *GSM_GetNetworkCode(char *NetworkName);

#endif	/* __gsm_networks_h */
