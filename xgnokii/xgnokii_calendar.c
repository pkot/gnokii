/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Nov 28 1999
  Modified by Jan Derfinak

*/

#include <time.h>
#include <gtk/gtk.h>
#include "misc.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_calendar.h"
#include "xpm/Read.xpm"
#include "xpm/Send.xpm"
#include "xpm/Open.xpm"
#include "xpm/Save.xpm"
#include "xpm/Edit.xpm"
#include "xpm/Delete.xpm"


typedef struct {
  GtkWidget *calendar;
  GtkWidget *notesClist;
} CalendarWidget;

static GtkWidget *GUI_CalendarWindow;
static ErrorDialog errorDialog = {NULL, NULL};
static CalendarWidget cal = {NULL, NULL};

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


static gint InsertCalendarEntry (GSM_CalendarNote *note)
{
  g_print ("%d: %s\n", note->Location, note->Text);
  return (GE_NONE);
}


static inline gint ReadCalendarFailed (gint i)
{
  return (0);
}

 
static void ReadCalNotes (void)
{
  PhoneEvent *e;
  D_CalendarNoteAll *cna;
  
  gtk_clist_clear (GTK_CLIST (cal.notesClist));
  
  cna = (D_CalendarNoteAll *) g_malloc (sizeof (D_CalendarNoteAll));
  cna->InsertEntry = InsertCalendarEntry;
  cna->ReadFailed = ReadCalendarFailed;
  
  e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
  e->event = Event_GetCalendarNoteAll;
  e->data = cna;
  GUI_InsertEvent (e);
}


static GtkItemFactoryEntry menu_items[] = {
  { NULL,		NULL,		NULL, 0, "<Branch>"},
  { NULL,		"<control>R",	ReadCalNotes, 0, NULL},
  { NULL,		"<control>S",	NULL, 0, NULL},
  { "/File/sep1",	NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>I",	NULL, 0, NULL},
  { NULL,		"<control>E",	NULL, 0, NULL},
  { "/File/sep2",	NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>W",	CloseCalendar, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<Branch>"},
  { NULL,		"<control>N",	NULL, 0, NULL},
  { NULL,		"<control>C",	NULL, 0, NULL},
  { NULL,		"<control>M",	NULL, 0, NULL},
  { NULL,		"<control>B",	NULL, 0, NULL},
  { NULL,		NULL,		NULL, 0, NULL},
  { NULL,		"<control>D",	NULL, 0, NULL},
  { "/Edit/sep3",	NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>A",	NULL, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<LastBranch>"},
  { NULL,		NULL,		Help1, 0, NULL},
  { NULL,		NULL,		GUI_ShowAbout, 0, NULL},
};

static void InitMainMenu (void)
{
  menu_items[0].path = g_strdup (_("/_File"));
  menu_items[1].path = g_strdup (_("/File/_Read from phone"));
  menu_items[2].path = g_strdup (_("/File/_Save to phone"));
  menu_items[4].path = g_strdup (_("/File/_Import from file"));
  menu_items[5].path = g_strdup (_("/File/_Export to file"));
  menu_items[7].path = g_strdup (_("/File/_Close"));
  menu_items[8].path = g_strdup (_("/_Edit"));
  menu_items[9].path = g_strdup (_("/_Edit/Add _reminder"));
  menu_items[10].path = g_strdup (_("/_Edit/Add _call"));
  menu_items[11].path = g_strdup (_("/_Edit/Add _meeting"));
  menu_items[12].path = g_strdup (_("/_Edit/Add _birthday"));
  menu_items[13].path = g_strdup (_("/_Edit/_Edit"));
  menu_items[14].path = g_strdup (_("/_Edit/_Delete"));
  menu_items[16].path = g_strdup (_("/_Edit/Select _all"));
  menu_items[17].path = g_strdup (_("/_Help"));
  menu_items[18].path = g_strdup (_("/Help/_Help"));
  menu_items[19].path = g_strdup (_("/Help/_About"));
}

void GUI_CreateCalendarWindow ()
{
  int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  GtkWidget *menubar, *toolbar, *scrolledWindow;
  GtkWidget *main_vbox;
  time_t t;
  struct tm *tm;
  gchar *titles[5] = { _("#"), _("Type"), _("Date / Time"), _("Text"), _("Alarm")};

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

  /* Create the toolbar */
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NORMAL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Read from phone"), NULL,
                           NewPixmap(Read_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) ReadCalNotes, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Save to phone"), NULL,
                           NewPixmap(Send_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Import from file"), NULL,
                           NewPixmap(Open_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Export to file"), NULL,
                           NewPixmap(Save_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Edit note"), NULL,
                           NewPixmap(Edit_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add reminder"), NULL,
                           NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add call"), NULL,
                           NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add meeting"), NULL,
                           NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add birthday"), NULL,
                           NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Delete note"), NULL,
                           NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  cal.calendar = gtk_calendar_new ();

  gtk_box_pack_start (GTK_BOX (main_vbox), cal.calendar, FALSE, FALSE, 0);
  t = time (NULL);
  tm = localtime (&t);
  gtk_calendar_select_month (GTK_CALENDAR (cal.calendar), tm->tm_mon, tm->tm_year + 1900);
  gtk_calendar_select_day (GTK_CALENDAR (cal.calendar), tm->tm_mday);

  gtk_widget_show (cal.calendar);  

  /* Notes list */
  cal.notesClist = gtk_clist_new_with_titles (5, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (cal.notesClist), GTK_SHADOW_OUT);
//  gtk_clist_set_compare_func (GTK_CLIST (cal.notesClist), CListCompareFunc);
//  gtk_clist_set_sort_column (GTK_CLIST (cal.notesClist), 1);
//  gtk_clist_set_sort_type (GTK_CLIST (cal.notesClist), GTK_SORT_ASCENDING);
  gtk_clist_set_auto_sort (GTK_CLIST (cal.notesClist), FALSE);
  gtk_clist_set_selection_mode (GTK_CLIST (cal.notesClist), GTK_SELECTION_EXTENDED);

//  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 0, 40);
//  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 1, 155);
//  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 2, 135);
  //gtk_clist_set_column_justification (GTK_CLIST (cal.notesClist), 2, GTK_JUSTIFY_CENTER);

//  for (i = 0; i < 5; i++)
//  {
//    if ((sColumn = g_malloc (sizeof (SortColumn))) == NULL)
//    {
//      g_print (_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
//      gtk_main_quit ();
//    }
//    sColumn->clist = SMS.smsClist;
//    sColumn->column = i;
//    gtk_signal_connect (GTK_OBJECT (GTK_CLIST (SMS.smsClist)->column[i].button), "clicked",
//                        GTK_SIGNAL_FUNC (SetSortColumn), (gpointer) sColumn);
//  }

//  gtk_signal_connect (GTK_OBJECT (notesClist), "select_row",
//                      GTK_SIGNAL_FUNC (ClickEntry), NULL);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolledWindow, 550, 100);
  gtk_container_add (GTK_CONTAINER (scrolledWindow), cal.notesClist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (main_vbox), scrolledWindow, 
                      TRUE, TRUE, 0);

  gtk_widget_show (cal.notesClist);
  gtk_widget_show (scrolledWindow);

  CreateErrorDialog (&errorDialog, GUI_CalendarWindow);
}
