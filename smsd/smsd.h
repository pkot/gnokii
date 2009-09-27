/*

  $Id$

  S M S D

  A Linux/Unix tool for the mobile phones.

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

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak

*/

#ifndef SMSD_H
#define SMSD_H

#include "config.h"
#include "compat.h"

#include "gnokii.h"

#include <glib.h>

/* error definition */
#define SMSD_OK		0
#define SMSD_NOK	1
#define SMSD_DUPLICATE	2
#define SMSD_WAITING	3

typedef enum {
  SMSD_READ_REPORTS = 1
} SMSDSettings;

typedef struct {
  gchar *bindir;
  gchar *dbMod;
  gchar *libDir;
  gchar *logFile;
  gchar *phone;
  gint   refreshInt;
  gint   maxSMS;
  gint   firstSMS;
//  gint   smsSets:4;
  gn_memory_type memoryType;
} SmsdConfig;

typedef struct {
  gchar *user;
  gchar *password;
  gchar *db;
  gchar *host;
  gchar *schema;
} DBConfig;

extern SmsdConfig smsdConfig;
GNOKII_API gint WriteSMS (gn_sms *);

#endif
