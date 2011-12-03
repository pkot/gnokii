/*

  S M S D

  A Linux/Unix tool for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2011 Jan Derfinak

*/

#ifndef DB_H
#define DB_H

#include <glib.h>
#include "smsd.h"
#include "gnokii.h"

GNOKII_API void (*DB_Bye) (void);
GNOKII_API gint (*DB_ConnectInbox) (const DBConfig);
GNOKII_API gint (*DB_ConnectOutbox) (const DBConfig);
GNOKII_API gint (*DB_InsertSMS) (const gn_sms * const, const gchar * const);
GNOKII_API gint (*DB_Look) (const gchar * const);

#endif
