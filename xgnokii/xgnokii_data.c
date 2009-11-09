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

  Copyright (C) 1999 Pavel Janik ml., Hugh Blemings
  & 1999-2005 Jan Derfinak.
  Copyright (C) 2002-2004 Pawel Kot
  Copyright (C) 2002      BORBELY Zoltan
 
*/

#include "config.h"

#include <stdio.h>
#include <gtk/gtk.h>
#include "misc.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_data.h"

static GtkWidget *GUI_DataWindow;
bool GTerminateThread = false;
bool enabled = false;
static GtkWidget *label = NULL;

static void UpdateStatus(void)
{
	gchar *buf;

	if (!enabled)
		buf = g_strdup_printf(_("Data calls are currently\nDisabled\n "));
	else
		buf = g_strdup_printf(_("Data calls are currently\nEnabled\n "));

	if (label != NULL)
		gtk_label_set_text(GTK_LABEL(label), buf);

	g_free(buf);
}


void GUI_ShowData(void)
{
	if ((!phoneMonitor.supported) & PM_DATA)
		return;

	if (GTerminateThread) {
		gn_vm_terminate();
		enabled = false;
		GTerminateThread = false;
	}
	UpdateStatus();

	gtk_window_present(GTK_WINDOW(GUI_DataWindow));
}

static void GUI_HideData(void)
{
	gtk_widget_hide(GUI_DataWindow);
}

static inline void DisableData(GtkWidget * widget, gpointer data)
{
	gn_vm_terminate();
	enabled = false;
	UpdateStatus();
}


static inline void EnableData(GtkWidget * widget, gpointer data)
{

	GTerminateThread = false;
	gn_vm_initialise("", xgnokiiConfig.bindir, false, false);
	enabled = true;
	UpdateStatus();
}


void GUI_CreateDataWindow(void)
{
	GtkWidget *button, *hbox, *vbox;

	GUI_DataWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_DataWindow), "DataWindow", "Xgnokii");
	gtk_window_set_title(GTK_WINDOW(GUI_DataWindow), _("Virtual Modem"));
	gtk_container_set_border_width(GTK_CONTAINER(GUI_DataWindow), 10);
	gtk_signal_connect(GTK_OBJECT(GUI_DataWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(vbox), 1);
	gtk_container_add(GTK_CONTAINER(GUI_DataWindow), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	button = gtk_button_new_with_label(_("Enable"));
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(EnableData), NULL);
	gtk_widget_show(button);
	button = gtk_button_new_with_label(_("Disable"));
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(DisableData), NULL);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);
}
