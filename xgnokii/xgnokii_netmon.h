/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Nov 16 1999
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_NETMON_H
#define XGNOKII_NETMON_H

#include <gtk/gtk.h>

typedef struct {
	GtkWidget *number;
	GtkWidget *label;
	gint curDisp;
} DisplayData;

extern void GUI_CreateNetmonWindow();

extern void GUI_ShowNetmon();

extern void GUI_RefreshNetmon();

#endif
