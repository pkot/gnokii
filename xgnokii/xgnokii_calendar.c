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
#include "xpm/SendSMS.xpm"

typedef struct {
  GtkWidget *calendar;
  GtkWidget *notesClist;
  GtkWidget *noteText;
  GdkColor   colour;
} CalendarWidget;

static GtkWidget *GUI_CalendarWindow;
static ErrorDialog errorDialog = {NULL, NULL};
static CalendarWidget cal = {NULL, NULL};

static inline void Help1 (GtkWidget *w, gpointer data)
{
  gchar *indx = g_strdup_printf ("/help/%s/windows/calendar/index.html", xgnokiiConfig.locale);
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
  gchar *row[6];

  row[0] = g_strdup_printf ("%d", note->Location);

  switch (note->Type)
  {
    case GCN_REMINDER:
    	row[1] = _("Reminder");
    	row[2] = g_strdup_printf ("%d-%02d-%02d", note->Time.Year,
                                  note->Time.Month, note->Time.Day);
    	row[5] = "";
    	break;

    case GCN_CALL:
    	row[1] = _("Call");
    	row[2] = g_strdup_printf ("%d-%02d-%02d  %02d:%02d", note->Time.Year,
                                  note->Time.Month, note->Time.Day,
                                  note->Time.Hour, note->Time.Minute);
    	row[5] = note->Phone;
    	break;

    case GCN_MEETING:
    	row[1] = _("Meeting");
    	row[2] = g_strdup_printf ("%d-%02d-%02d  %02d:%02d", note->Time.Year,
                                  note->Time.Month, note->Time.Day,
                                  note->Time.Hour, note->Time.Minute);
    	row[5] = "";
    	break;

    case GCN_BIRTHDAY:
    	row[1] = _("Birthday");
    	row[2] = g_strdup_printf ("%d-%02d-%02d", note->Time.Year,
                                  note->Time.Month, note->Time.Day);
    	row[5] = "";
    	break;

    default:
    	row[1] = _("Unknown");
    	row[5] = "";
    	break;
  }

  row[3] = note->Text;

  if (note->Alarm.Year == 0)
    row[4] = "";
  else
    row[4] = g_strdup_printf ("%d-%02d-%02d  %02d:%02d", note->Alarm.Year,
                               note->Alarm.Month, note->Alarm.Day,
                               note->Alarm.Hour, note->Alarm.Minute);

  gtk_clist_freeze (GTK_CLIST (cal.notesClist));
  gtk_clist_append (GTK_CLIST (cal.notesClist), row);
  gtk_clist_sort (GTK_CLIST (cal.notesClist));
  gtk_clist_thaw (GTK_CLIST (cal.notesClist));
  
  g_free (row[0]);
  g_free (row[2]);
  if (*row[4] != '\0')
    g_free (row[4]);

  return (GE_NONE);
}

static void ClickEntry (GtkWidget      *clist,
                        gint            row,
                        gint            column,
                        GdkEventButton *event,
                        gpointer        data )
{
  gchar *buf;

  /* FIXME - We must mark SMS as readed */
  gtk_text_freeze (GTK_TEXT (cal.noteText));

  gtk_text_set_point (GTK_TEXT (cal.noteText), 0);
  gtk_text_forward_delete (GTK_TEXT (cal.noteText), gtk_text_get_length (GTK_TEXT (cal.noteText)));

  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.colour), NULL,
                   _("Type: "), -1);
  gtk_clist_get_text (GTK_CLIST (clist), row, 1, &buf);
  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                   buf, -1);
  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                   "\n", -1);

  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.colour), NULL,
                   _("Date: "), -1);
  gtk_clist_get_text (GTK_CLIST (clist), row, 2, &buf);
  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                   buf, -1);
  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                   "\n", -1);

  gtk_clist_get_text (GTK_CLIST (clist), row, 4, &buf);
  if (*buf != '\0')
  {
    gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.colour), NULL,
                     _("Alarm: "), -1);
  
    gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                     buf, -1);
    gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                     "\n", -1);
  }
  
  gtk_clist_get_text (GTK_CLIST (clist), row, 5, &buf);
  if (*buf != '\0')
  {
    gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.colour), NULL,
                     _("Number: "), -1);
  
    gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                     buf, -1);
    gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                     "\n", -1);
  }
  
  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.colour), NULL,
                   _("Text: "), -1);
  gtk_clist_get_text (GTK_CLIST (clist), row, 3, &buf);
  gtk_text_insert (GTK_TEXT (cal.noteText), NULL, &(cal.noteText->style->black), NULL,
                   buf, -1);

  gtk_text_thaw (GTK_TEXT (cal.noteText));
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
  { NULL,		NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>X",	NULL, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>I",	NULL, 0, NULL},
  { NULL,		"<control>E",	NULL, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<Separator>"},
  { NULL,		"<control>W",	CloseCalendar, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<Branch>"},
  { NULL,		"<control>N",	NULL, 0, NULL},
  { NULL,		"<control>C",	NULL, 0, NULL},
  { NULL,		"<control>M",	NULL, 0, NULL},
  { NULL,		"<control>B",	NULL, 0, NULL},
  { NULL,		NULL,		NULL, 0, NULL},
  { NULL,		"<control>D",	NULL, 0, NULL},
  { NULL,		NULL,		NULL, 0, "<Separator>"},
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
  menu_items[3].path = g_strdup (_("/File/Sep1"));
  menu_items[4].path = g_strdup (_("/File/Send via S_MS"));
  menu_items[5].path = g_strdup (_("/File/Sep2"));
  menu_items[6].path = g_strdup (_("/File/_Import from file"));
  menu_items[7].path = g_strdup (_("/File/_Export to file"));
  menu_items[8].path = g_strdup (_("/File/Sep3"));
  menu_items[9].path = g_strdup (_("/File/_Close"));
  menu_items[10].path = g_strdup (_("/_Edit"));
  menu_items[11].path = g_strdup (_("/Edit/Add _reminder"));
  menu_items[12].path = g_strdup (_("/Edit/Add _call"));
  menu_items[13].path = g_strdup (_("/Edit/Add _meeting"));
  menu_items[14].path = g_strdup (_("/Edit/Add _birthday"));
  menu_items[15].path = g_strdup (_("/Edit/_Edit"));
  menu_items[16].path = g_strdup (_("/Edit/_Delete"));
  menu_items[17].path = g_strdup (_("/Edit/Sep4"));
  menu_items[18].path = g_strdup (_("/Edit/Select _all"));
  menu_items[19].path = g_strdup (_("/_Help"));
  menu_items[20].path = g_strdup (_("/Help/_Help"));
  menu_items[21].path = g_strdup (_("/Help/_About"));
}

void GUI_CreateCalendarWindow ()
{
  int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  GtkWidget *menubar, *toolbar, *scrolledWindow, *hpaned;
  GtkWidget *main_vbox;
  GdkColormap *cmap;
  time_t t;
  struct tm *tm;
  gchar *titles[6] = { _("#"), _("Type"), _("Date"), _("Text"),
                       _("Alarm"), _("Number")};

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

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Send via SMS"), NULL,
                           NewPixmap(SendSMS_xpm, GUI_CalendarWindow->window,
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

  hpaned = gtk_hpaned_new ();
  //gtk_paned_set_handle_size (GTK_PANED (hpaned), 10);
  //gtk_paned_set_gutter_size (GTK_PANED (hpaned), 15);
  gtk_box_pack_start (GTK_BOX (main_vbox), hpaned, TRUE, TRUE, 0);
  gtk_widget_show (hpaned);

  /* Note viewer */
  cal.noteText = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (cal.noteText), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (cal.noteText), TRUE);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_paned_add1 (GTK_PANED (hpaned), scrolledWindow);

  gtk_container_add (GTK_CONTAINER (scrolledWindow), cal.noteText);
  gtk_widget_show_all (scrolledWindow);

  /* Calendar */
  cal.calendar = gtk_calendar_new ();

  t = time (NULL);
  tm = localtime (&t);
  gtk_calendar_select_month (GTK_CALENDAR (cal.calendar), tm->tm_mon, tm->tm_year + 1900);
  gtk_calendar_select_day (GTK_CALENDAR (cal.calendar), tm->tm_mday);

  gtk_paned_add2 (GTK_PANED (hpaned), cal.calendar);
  gtk_widget_show (cal.calendar);  

  /* Notes list */
  cal.notesClist = gtk_clist_new_with_titles (6, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (cal.notesClist), GTK_SHADOW_OUT);
//  gtk_clist_set_compare_func (GTK_CLIST (cal.notesClist), CListCompareFunc);
  gtk_clist_set_sort_column (GTK_CLIST (cal.notesClist), 0);
  gtk_clist_set_sort_type (GTK_CLIST (cal.notesClist), GTK_SORT_ASCENDING);
  gtk_clist_set_auto_sort (GTK_CLIST (cal.notesClist), FALSE);
  gtk_clist_set_selection_mode (GTK_CLIST (cal.notesClist), GTK_SELECTION_EXTENDED);

  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 0, 15);
  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 1, 52);
  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 2, 110);
  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 3, 130);
  gtk_clist_set_column_width (GTK_CLIST (cal.notesClist), 4, 110);
  gtk_clist_set_column_justification (GTK_CLIST (cal.notesClist), 0, GTK_JUSTIFY_RIGHT);

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

  gtk_signal_connect (GTK_OBJECT (cal.notesClist), "select_row",
                      GTK_SIGNAL_FUNC (ClickEntry), NULL);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolledWindow, 550, 100);
  gtk_container_add (GTK_CONTAINER (scrolledWindow), cal.notesClist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_end (GTK_BOX (main_vbox), scrolledWindow, 
                      TRUE, TRUE, 0);

  gtk_widget_show (cal.notesClist);
  gtk_widget_show (scrolledWindow);

  cmap = gdk_colormap_get_system();
  cal.colour.red = 0xffff;
  cal.colour.green = 0;
  cal.colour.blue = 0;
  if (!gdk_color_alloc (cmap, &(cal.colour)))
    g_error (_("couldn't allocate colour"));

  CreateErrorDialog (&errorDialog, GUI_CalendarWindow);
}
