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

  Copyright (C) 2004 BORBELY Zoltan <bozo@andrews.hu>

  This file provides functions specific to the gnapplet series.
  See README for more details on supported mobile phones.

*/

#ifndef _gnokii_phones_gnapplet_h
#define _gnokii_phones_gnapplet_h

#include "gnokii.h"


#define	GNAPPLET_MAJOR_VERSION	0
#define	GNAPPLET_MINOR_VERSION	2

#define	GNAPPLET_MSG_INFO	1
#define	GNAPPLET_MSG_PHONEBOOK	2

#define	GNAPPLET_MSG_INFO_ID_REQ	1
#define	GNAPPLET_MSG_INFO_ID_RESP	2

#define	GNAPPLET_MSG_PHONEBOOK_READ_REQ		1
#define	GNAPPLET_MSG_PHONEBOOK_READ_RESP	2
#define	GNAPPLET_MSG_PHONEBOOK_WRITE_REQ	3
#define	GNAPPLET_MSG_PHONEBOOK_WRITE_RESP	4
#define	GNAPPLET_MSG_PHONEBOOK_DELETE_REQ	5
#define	GNAPPLET_MSG_PHONEBOOK_DELETE_RESP	6


typedef struct {
//	char model[GN_MODEL_MAX_LENGTH];
//	char imei[GN_IMEI_MAX_LENGTH];
//	char sw_version[10];
//	char hw_version[10];
//	gn_phone_model *pm;
} gnapplet_driver_instance;

#endif  /* #ifndef _gnokii_phones_gnapplet_h */
