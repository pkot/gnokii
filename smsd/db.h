/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id$
  
*/

#ifndef DB_H
#define DB_H

#include <glib.h>
#include "smsd.h"
#include "gsm-sms.h"

/* extern void DB_Bye (void);
extern gint DB_Connect (const DBConfig);
extern gint DB_InsertSMS (const GSM_SMSMessage * const);
extern void DB_Look (void);
*/

extern void (*DB_Bye) (void);
extern gint (*DB_Connect) (const DBConfig);
extern gint (*DB_InsertSMS) (const GSM_SMSMessage * const);
extern void (*DB_Look) (void);

#endif
