/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 2002, BORBELY Zoltan <bozo@andrews.hu>

  Include file for the call management library.

*/

#ifndef _gnokii_gsm_call_h
#define _gnokii_gsm_call_h

#include "gsm-error.h"

typedef enum {
	GN_CALL_Voice,		/* Voice call */
	GN_CALL_NonDigitalData,	/* Data call on non digital line */
	GN_CALL_DigitalData	/* Data call on digital line */
} gn_call_type;

typedef enum {
	GN_CALL_Never,			/* Never send my number */
	GN_CALL_Always,			/* Always send my number */
	GN_CALL_Default			/* Use the network default settings */
} gn_call_send_number;

typedef enum {
	GN_CALL_Idle,
	GN_CALL_Ringing,
	GN_CALL_Dialing,
	GN_CALL_Incoming,		/* Incoming call */
	GN_CALL_LocalHangup,		/* Local end terminated the call */
	GN_CALL_RemoteHangup,		/* Remote end terminated the call */
	GN_CALL_Established,		/* Remote end answered the call */
	GN_CALL_Held,			/* Call placed on hold */
	GN_CALL_Resumed			/* Held call resumed */
} gn_call_status;

typedef struct {
	gn_call_type type;
	char number[GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1];
	char name[GN_PHONEBOOK_NAME_MAX_LENGTH + 1];
	gn_call_send_number send_number;
	int call_id;
} gn_call_info;

/* Call functions and structs */
typedef struct {
	struct gn_statemachine *state;
	int call_id;
	gn_call_status status;
	gn_call_type type;
	char remote_number[GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1];
	char remote_name[GN_PHONEBOOK_NAME_MAX_LENGTH + 1];
	struct timeval start_time;
	struct timeval answer_time;
	bool local_originated;
} gn_call;

#define	GN_CALL_MAX_PARALLEL 2

#endif /* _gnokii_gsm_call_h */
