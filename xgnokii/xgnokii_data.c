/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tues 13 June 2000
  Modified by Chris Kemp

*/

#include <stdio.h>
#include <gtk/gtk.h>
#include "misc.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_data.h"
#include "data/virtmodem.h"

static GtkWidget *GUI_DataWindow;
bool TerminateThread = false;
bool enabled = false;
static GtkWidget *label = NULL;

static inline void Help1(GtkWidget * w, gpointer data)
{
	gchar *indx = g_strdup_printf("/help/%s/windows/data/index.html", xgnokiiConfig.locale);
	Help(w, indx);
	g_free(indx);
}

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


inline void GUI_ShowData(void)
{
	if (!phoneMonitor.supported & PM_DATA)
		return;

	if (TerminateThread) {
		VM_Terminate();
		enabled = false;
		TerminateThread = false;
	}
	UpdateStatus();

	gtk_widget_show(GUI_DataWindow);
}


inline void GUI_HideData(void)
{
	gtk_widget_hide(GUI_DataWindow);
}

static inline void DisableData(GtkWidget * widget, gpointer data)
{
	VM_Terminate();
	enabled = false;
	UpdateStatus();
}


static inline void EnableData(GtkWidget * widget, gpointer data)
{

	TerminateThread = false;
	VM_Initialise(xgnokiiConfig.model, xgnokiiConfig.port,
		      0, 0, xgnokiiConfig.bindir, false, false);
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
