/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Wed Sep 08 1999
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_COMMON_H
#define XGNOKII_COMMON_H

#include <gtk/gtk.h>

typedef struct {
  GtkWidget *dialog;
  GtkWidget *text;
} ErrorDialog;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *text;
} YesNoDialog;

extern void CancelDialog( GtkWidget *, gpointer );
extern void CreateErrorDialog( ErrorDialog *, GtkWidget * );
extern void CreateYesNoDialog(YesNoDialog *, GtkSignalFunc , GtkSignalFunc, GtkWidget * );
extern GtkWidget* NewPixmap( gchar **, GdkWindow *, GdkColor *);
extern void DeleteEvent( GtkWidget *, GdkEvent *, gpointer );

#endif
