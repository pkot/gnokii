/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sun Dec 17 2000
  Modified by Jan Derfinak

*/

#ifndef SMSD_H
#define SMSD_H

#include <glib.h>

typedef struct {
  gchar *initlength; /* Init length from .gnokiirc file */
  gchar *model;      /* Model from .gnokiirc file. */
  gchar *port;       /* Serial port from .gnokiirc file */
  gchar *connection; /* Connection type from .gnokiirc file */
  gchar *bindir;
  gint   smsSets:4;
} SmsdConfig;

extern SmsdConfig smsdConfig;
extern gint WriteSMS (GSM_SMSMessage *);

#endif
