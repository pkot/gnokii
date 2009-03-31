/*

  $Id$

  X G N O K I I

  A Linux/Unix GUI for the mobile phones.

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
  & 1999-2005 Jan Derfinak.

*/

#ifndef XGNOKII_COMMON_H
#define XGNOKII_COMMON_H

#include <gtk/gtk.h>
#include <stdlib.h>		/* for size_t */

typedef struct {
	GtkWidget *clist;
	gint column;
} SortColumn;

/* typedef struct {
  gchar *model;
  gchar *number;
} Model;
*/

typedef struct {
	GtkWidget *dialog;
	GtkWidget *text;
} ErrorDialog;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *text;
} InfoDialog;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *text;
} YesNoDialog;

typedef struct {
	GdkPixmap *pixmap;
	GdkBitmap *mask;
} QuestMark;

typedef enum {
	GUI_EVENT_CONTACTS_CHANGED,
	GUI_EVENT_CALLERS_GROUPS_CHANGED,
	GUI_EVENT_SMS_NUMBER_CHANGED,
	GUI_EVENT_SMS_CENTERS_CHANGED,
	GUI_EVENT_NETMON_CHANGED,
} GUIEventType;

extern void CancelDialog(const GtkWidget *, const gpointer);
extern void CreateErrorDialog(ErrorDialog *, GtkWidget *);
extern void CreateInfoDialog(InfoDialog *, GtkWidget *);
extern void CreateYesNoDialog(YesNoDialog *, const GtkSignalFunc, const GtkSignalFunc, GtkWidget *);
extern GtkWidget *NewPixmap(gchar **, GdkWindow *, GdkColor *);
extern void DeleteEvent(const GtkWidget *, const GdkEvent *, const gpointer);
extern gint LaunchProcess(const gchar *, const gchar *, const gint, const gint, const gint);
extern void RemoveZombie(const gint);
extern gint strrncmp(const gchar * const, const gchar * const, size_t);
extern void GUI_Refresh(void);
extern void SetSortColumn(GtkWidget *, SortColumn *);
extern void GUIEventAdd(GUIEventType, void (*)(void));
extern bool GUIEventRemove(GUIEventType, void (*)(void));
extern void GUIEventSend(GUIEventType);
#endif
