/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2000 Pavel Machek

  This file provides functions specific to the Nokia 2110 series.
  See README for more details on supported mobile phones.

  The various routines are called P2110_(whatever).

*/

#ifndef __phones_nk2110_h
#define __phones_nk2110_h

#define LM_SMS_EVENT	0x37
#define LM_SMS_COMMAND	0x38
#define   LM_SMS_READ_STORED_DATA	0x02		/* These are really subcommands of LM_SMS_COMMAND */
#define   LM_SMS_RECEIVED_PP_DATA	0x06
#define   LM_SMS_FORWARD_STORED_DATA 	0x0b
#define   LM_SMS_RESERVE_PP 		0x12
#define	    LN_SMS_NORMAL_RESERVE 1
#define     LN_SMS_NEW_MSG_INDICATION 2
#define   LM_SMS_UNRESERVE_PP		0x14

#define   LM_SMS_NEW_MESSAGE_INDICATION 0x1a
#define   LM_SMS_ALIVE_TEST		0x1e
#define   LM_SMS_ALIVE_ACK		0x1f
#define	  LM_SMS_PP_RESERVE_COMPLETE      22


#define LN_LOC_COMMAND	0x1f

#endif  /* #ifndef __phones_nk2110_h */
