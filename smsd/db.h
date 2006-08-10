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

#ifndef DB_H
#define DB_H

#include <glib.h>
#include "smsd.h"
#include "gnokii.h"

GNOKII_API void (*DB_Bye) (void);
GNOKII_API gint (*DB_ConnectInbox) (const DBConfig);
GNOKII_API gint (*DB_ConnectOutbox) (const DBConfig);
GNOKII_API gint (*DB_InsertSMS) (const gn_sms * const, const gchar * const);
GNOKII_API void (*DB_Look) (const gchar * const);

#endif
