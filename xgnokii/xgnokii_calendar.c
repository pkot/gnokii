/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Nov 28 1999
  Modified by Jan Derfinak

*/

#include <gtk/gtk.h>
#include "misc.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_calendar.h"

static GtkWidget *GUI_CalendarWindow;
static ErrorDialog errorDialog = {NULL, NULL};

static inline void Help1 (GtkWidget *w, gpointer data)
{
  gchar *indx = g_strdup_printf ("/help/%s/calendar.html", xgnokiiConfig.locale);
  Help (w, indx);
  g_free (indx);
}

static inline void CloseCalendar (GtkWidget *w, gpointer data)
{
  gtk_widget_hide (GUI_CalendarWindow);
}

inline void GUI_ShowCalendar ()
{
  gtk_widget_show (GUI_CalendarWindow);
}

static GtkItemFactoryEntry menu_items[] = {
  { NULL,		NULL,		NULL, 0, "<Branch>"},
  { NULL,		"<control>O",	NULL, 0, NULL},
  { NULL,		"<control>S",	NULL, 0, NULL},
  {"/File/sep1",	NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>W",	CloseCalendar, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<LastBranch>"},
  { NULL,		NULL,		Help1, 0, NULL},
  { NULL,		NULL,		GUI_ShowAbout, 0, NULL},
};

static void InitMainMenu (void)
{
  menu_items[0].path = g_strdup (_("/_File"));
  menu_items[1].path = g_strdup (_("/File/_Open"));
  menu_items[2].path = g_strdup (_("/File/_Save"));
  menu_items[4].path = g_strdup (_("/File/_Close"));
  menu_items[5].path = g_strdup (_("/_Help"));
  menu_items[6].path = g_strdup (_("/Help/_Help"));
  menu_items[7].path = g_strdup (_("/Help/_About"));
}

void GUI_CreateCalendarWindow ()
{
  int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  GtkWidget *menubar;
  GtkWidget *main_vbox;
  GtkWidget *calendar;


  InitMainMenu ();
  GUI_CalendarWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (GUI_CalendarWindow), _("Calendar"));
  //gtk_widget_set_usize (GTK_WIDGET (GUI_CalendarWindow), 436, 220);
  gtk_signal_connect (GTK_OBJECT (GUI_CalendarWindow), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
  gtk_widget_realize (GUI_CalendarWindow);
  
  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
                                       accel_group);

  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  gtk_accel_group_attach (accel_group, GTK_OBJECT (GUI_CalendarWindow));

  /* Finally, return the actual menu bar created by the item factory. */ 
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (GUI_CalendarWindow), main_vbox);
  gtk_widget_show (main_vbox);

  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);

  calendar = gtk_calendar_new ();

  gtk_box_pack_start (GTK_BOX (main_vbox), calendar, FALSE, FALSE, 0);
  gtk_widget_show (calendar);  

  CreateErrorDialog (&errorDialog, GUI_CalendarWindow);
}
