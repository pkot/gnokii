/*

  S M S D

  A Linux/Unix tool for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2011 Jan Derfinak

*/

#ifndef SMSD_H
#define SMSD_H

#include "compat.h"

#include "gnokii.h"

#include <glib.h>

/* error definition */
#define SMSD_OK		0
#define SMSD_NOK	1
#define SMSD_DUPLICATE	2
#define SMSD_WAITING	3
#define SMSD_OUTBOXEMPTY	4

typedef enum {
  SMSD_READ_REPORTS = 1
} SMSDSettings;

typedef struct {
  gchar *bindir;
  gchar *dbMod;
  gchar *libDir;
  gchar *configFile;
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
  gchar *clientEncoding;
} DBConfig;

extern SmsdConfig smsdConfig;
GNOKII_API gint WriteSMS (gn_sms *);

#endif
