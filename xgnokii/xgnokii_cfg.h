/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Mon Oct 10 1999
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_CFG_H
#define XGNOKII_CFG_H

#include <gtk/gtk.h>
#include "xgnokii.h"

#define		HTMLVIEWER_LENGTH	200

typedef struct {
  gchar key[10];
  gchar **value;
} ConfigEntry;

extern void GUI_ReadXConfig();
extern gint GUI_SaveXConfig();

#endif
