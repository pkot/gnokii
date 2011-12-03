/*

  X G N O K I I

  A Linux/Unix GUI for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & 1999-2005 Jan Derfinak.

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
