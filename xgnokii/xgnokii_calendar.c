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
#include <string.h>
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
#include "xpm/NewBD.xpm"
#include "xpm/NewCall.xpm"
#include "xpm/NewMeet.xpm"
#include "xpm/NewRem.xpm"
#include "xpm/quest.xpm"

typedef struct {
  GtkWidget *calendar;
  GtkWidget *notesClist;
  GtkWidget *noteText;
  GdkColor   colour;
} CalendarWidget;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *date;
  GtkWidget *text;
  GtkWidget *alarm;
  GtkWidget *alarmFrame;
  GtkWidget *alarmCal;
  GtkWidget *alarmHour;
  GtkWidget *alarmMin;
} AddDialogData;

static GtkWidget *GUI_CalendarWindow;
static ErrorDialog errorDialog = {NULL, NULL};
static CalendarWidget cal = {NULL, NULL};
static QuestMark questMark;
static AddDialogData addReminderDialogData;

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
  if (phoneMonitor.supported.calendar)
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
    	row[2] = g_strdup_printf ("%02d/%02d/%04d", note->Time.Day,
                                  note->Time.Month, note->Time.Year);
    	row[5] = "";
    	break;

    case GCN_CALL:
    	row[1] = _("Call");
    	row[2] = g_strdup_printf ("%02d/%02d/%04d  %02d:%02d", note->Time.Day,
                                  note->Time.Month, note->Time.Year,
                                  note->Time.Hour, note->Time.Minute);
    	row[5] = note->Phone;
    	break;

    case GCN_MEETING:
    	row[1] = _("Meeting");
    	row[2] = g_strdup_printf ("%02d/%02d/%04d  %02d:%02d", note->Time.Day,
                                  note->Time.Month, note->Time.Year,
                                  note->Time.Hour, note->Time.Minute);
    	row[5] = "";
    	break;

    case GCN_BIRTHDAY:
    	row[1] = _("Birthday");
    	row[2] = g_strdup_printf ("%02d/%02d/%04d", note->Time.Day,
                                  note->Time.Month, note->Time.Year);
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
    row[4] = g_strdup_printf ("%02d/%02d/%04d  %02d:%02d", note->Alarm.Day,
                               note->Alarm.Month, note->Alarm.Year,
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

  gtk_calendar_select_month (GTK_CALENDAR (cal.calendar),
                             atoi (buf + 3) - 1, atoi (buf + 6));
  gtk_calendar_select_day (GTK_CALENDAR (cal.calendar), atoi (buf));

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


static gint CListCompareFunc (GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
  char *text1 = NULL;
  char *text2 = NULL;

  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  switch (row1->cell[clist->sort_column].type)
  {
    case GTK_CELL_TEXT:
      text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
      break;
    case GTK_CELL_PIXTEXT:
      text1 = GTK_CELL_PIXTEXT (row1->cell[clist->sort_column])->text;
      break;
    default:
      break;
  }
  switch (row2->cell[clist->sort_column].type)
  {
    case GTK_CELL_TEXT:
      text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;
      break;
    case GTK_CELL_PIXTEXT:
      text2 = GTK_CELL_PIXTEXT (row2->cell[clist->sort_column])->text;
      break;
    default:
      break;
  }

  if (!text2)
    return (text1 != NULL);

  if (!text1)
    return -1;

  if (*text2 == '\0')
    return (*text1 != '\0');

  if (*text1 == '\0')
    return (-1);

  if (clist->sort_column == 0)
  {
    gint n1 = atoi (text1);
    gint n2 = atoi (text2);
    
    if (n1 > n2)
      return (1);
    else if (n1 < n2)
      return (-1);
    else 
      return 0;
  }

  if (clist->sort_column == 2 || clist->sort_column == 4)
  {
    GDate *date1, *date2;
    gint time1, time2;
    gint ret;

    date1 = g_date_new_dmy (atoi (text1), atoi (text1 + 3), atoi (text1 + 6));
    date2 = g_date_new_dmy (atoi (text2), atoi (text2 + 3), atoi (text2 + 6));

    ret = g_date_compare (date1, date2);

    g_date_free (date1);
    g_date_free (date2);

    if (ret)
      return (ret);

    if (strlen (text1) > 10)
      time1 = atoi (text1 + 11) * 60 + atoi (text1 + 14);
    else
      time1 = 0;

    if (strlen (text2) > 10)
      time2 = atoi (text2 + 11) * 60 + atoi (text2 + 14);
    else
      time2 = 0;

    if (time1 > time2)
      return (1);
    else if (time1 < time2)
      return (-1);
    else 
      return 0;
    
/*    struct tm bdTime;
    time_t time1, time2;

    bdTime.tm_sec  = 0;
    if (strlen (text1) > 10)
    {
      bdTime.tm_min  = atoi (text1 + 14);
      bdTime.tm_hour = atoi (text1 + 11);
    }
    else
      bdTime.tm_min  = bdTime.tm_hour = 0;
    bdTime.tm_mday = atoi (text1);
    bdTime.tm_mon  = atoi (text1 + 3);
    bdTime.tm_year = atoi (text1 + 6) - 1900;
    bdTime.tm_isdst = -1;

    time1 = mktime (&bdTime);

    bdTime.tm_sec  = 0;
    if (strlen (text2) > 10)
    {
      bdTime.tm_min  = atoi (text2 + 14);
      bdTime.tm_hour = atoi (text2 + 11);
    }
    else
      bdTime.tm_min  = bdTime.tm_hour = 0;
    bdTime.tm_mday = atoi (text2);
    bdTime.tm_mon  = atoi (text2 + 3);
    bdTime.tm_year = atoi (text2 + 6) - 1900;
    bdTime.tm_isdst = -1;

    time2 = mktime (&bdTime);

    g_print ("Cas1: %s - %d, Cas2: %s - %d\n", text1, time1, text2, time2);

    if (time1 > time2)
      return (1);
    else if (time1 < time2)
      return (-1);
    else 
      return 0; */
  }

  return (g_strcasecmp (text1, text2));
}


static gint ReverseSelection (gconstpointer a, gconstpointer b)
{
  gchar *buf1, *buf2;
  gint index1, index2;
  gint row1 = GPOINTER_TO_INT (a);
  gint row2 = GPOINTER_TO_INT (b);
  
  gtk_clist_get_text (GTK_CLIST (cal.notesClist), row1, 0, &buf1);
  gtk_clist_get_text (GTK_CLIST (cal.notesClist), row2, 0, &buf2);
  
  index1 = atoi (buf1);
  index2 = atoi (buf2);
  
  if (index1 < index2)
    return (1);
  else if (index1 > index2)
    return (-1);
  else
    return (0);
}

  
static void OkAddReminderDialog (GtkWidget *widget, gpointer data)
{
}

static void ShowAlarmVBox (GtkToggleButton *button, AddDialogData *data)
{
  if (gtk_toggle_button_get_active (button))
  {
    gtk_widget_show (data->alarmFrame);
  }
  else
  {
    gtk_widget_hide (data->alarmFrame);
    gtk_widget_set_usize (GTK_WIDGET (GTK_DIALOG (data->dialog)->vbox), 0, 0);
//    gtk_widget_map (data->dialog);
  }
}

static void AddReminder (void)
{
  GtkWidget *button, *hbox, *vbox, *vbox2, *label;
  GtkAdjustment *adj;
  time_t t;
  struct tm *tm;
  
  if (addReminderDialogData.dialog == NULL)
  {
    addReminderDialogData.dialog = gtk_dialog_new();
    gtk_window_set_title (GTK_WINDOW (addReminderDialogData.dialog), _("Add reminder"));
    gtk_window_position (GTK_WINDOW (addReminderDialogData.dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal(GTK_WINDOW (addReminderDialogData.dialog), TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (addReminderDialogData.dialog), 10);
    gtk_signal_connect (GTK_OBJECT (addReminderDialogData.dialog), "delete_event",
                        GTK_SIGNAL_FUNC (DeleteEvent), NULL);

    button = gtk_button_new_with_label (_("Ok"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (addReminderDialogData.dialog)->action_area),
                        button, TRUE, TRUE, 10);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (OkAddReminderDialog), (gpointer) addReminderDialogData.dialog);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    button = gtk_button_new_with_label (_("Cancel"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (addReminderDialogData.dialog)->action_area),
                        button, TRUE, TRUE, 10);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (CancelDialog), (gpointer) addReminderDialogData.dialog);
    gtk_widget_show (button);

    gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (addReminderDialogData.dialog)->vbox), 5);

    vbox = gtk_vbox_new (FALSE, 10);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (addReminderDialogData.dialog)->vbox), vbox);
    gtk_widget_show (vbox);

    label = gtk_label_new (_("Date:"));
    gtk_container_add (GTK_CONTAINER (vbox), label);
    gtk_widget_show (label);
    
    addReminderDialogData.date = gtk_calendar_new ();

    t = time (NULL);
    tm = localtime (&t);
    gtk_calendar_select_month (GTK_CALENDAR (addReminderDialogData.date), tm->tm_mon, tm->tm_year + 1900);
    gtk_calendar_select_day (GTK_CALENDAR (addReminderDialogData.date), tm->tm_mday);

    gtk_container_add (GTK_CONTAINER (vbox), addReminderDialogData.date);
    gtk_widget_show (addReminderDialogData.date);  

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (vbox), hbox);
    gtk_widget_show (hbox);
    
    label = gtk_label_new (_("Subject:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);

    addReminderDialogData.text = gtk_entry_new_with_max_length (30);
    gtk_box_pack_end (GTK_BOX (hbox), addReminderDialogData.text, FALSE, FALSE, 2);
    gtk_widget_show (addReminderDialogData.text);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (vbox), hbox);
    gtk_widget_show (hbox);
    
    addReminderDialogData.alarm = gtk_check_button_new_with_label (_("Alarm"));
    gtk_box_pack_start (GTK_BOX(hbox), addReminderDialogData.alarm, FALSE, FALSE, 10);
    gtk_signal_connect (GTK_OBJECT (addReminderDialogData.alarm), "toggled",
                        GTK_SIGNAL_FUNC (ShowAlarmVBox), &addReminderDialogData);
    gtk_widget_show (addReminderDialogData.alarm);

    
    addReminderDialogData.alarmFrame = gtk_frame_new (_("Alarm"));
    gtk_container_add (GTK_CONTAINER (vbox), addReminderDialogData.alarmFrame);
    
    vbox2 = gtk_vbox_new (FALSE, 3);
    gtk_container_add (GTK_CONTAINER (addReminderDialogData.alarmFrame), vbox2);
    gtk_widget_show (vbox2);

    label = gtk_label_new (_("Alarm date:"));
    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 2);
    gtk_widget_show (label);

    addReminderDialogData.alarmCal = gtk_calendar_new ();
    gtk_container_add (GTK_CONTAINER (vbox2), addReminderDialogData.alarmCal);
    gtk_widget_show (addReminderDialogData.alarmCal);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (vbox2), hbox);
    gtk_widget_show (hbox);
    
    label = gtk_label_new (_("Alarm time:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);

    adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 23.0, 1.0, 4.0, 0.0);
    addReminderDialogData.alarmHour = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (addReminderDialogData.alarmHour), TRUE);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (addReminderDialogData.alarmHour), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), addReminderDialogData.alarmHour, FALSE, FALSE, 0);
    gtk_widget_show (addReminderDialogData.alarmHour);

    label = gtk_label_new (":");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);

    adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 59.0, 1.0, 10.0, 0.0);
    addReminderDialogData.alarmMin = gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (addReminderDialogData.alarmMin), TRUE);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (addReminderDialogData.alarmMin), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), addReminderDialogData.alarmMin, FALSE, FALSE, 0);
    gtk_widget_show (addReminderDialogData.alarmMin);

  }
  
  gtk_widget_show (GTK_WIDGET (addReminderDialogData.dialog));
}


static void OkDeleteNoteDialog (GtkWidget *widget, gpointer data)
{
  GSM_CalendarNote *note;
  PhoneEvent *e;
  GList *sel;
  gint row;
  gchar *buf;


  sel = GTK_CLIST (cal.notesClist)->selection;

  gtk_clist_freeze (GTK_CLIST (cal.notesClist));

  sel = g_list_sort (sel, ReverseSelection);
  
  while (sel != NULL)
  {
    row = GPOINTER_TO_INT (sel->data);
    sel = sel->next;

    note = (GSM_CalendarNote *) g_malloc (sizeof (GSM_CalendarNote));
    gtk_clist_get_text (GTK_CLIST (cal.notesClist), row, 0, &buf);
    note->Location = atoi (buf);

    e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
    e->event = Event_DeleteCalendarNote;
    e->data = note;
    GUI_InsertEvent (e);
  }

  gtk_widget_hide (GTK_WIDGET (data));

  gtk_clist_thaw (GTK_CLIST (cal.notesClist));

  ReadCalNotes ();
}


static void DeleteNote (void)
{
  static GtkWidget *dialog = NULL;
  GtkWidget *button, *hbox, *label, *pixmap;

  if (dialog == NULL)
  {
    dialog = gtk_dialog_new();
    gtk_window_set_title (GTK_WINDOW (dialog), _("Delete calendar note"));
    gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal(GTK_WINDOW (dialog), TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                        GTK_SIGNAL_FUNC (DeleteEvent), NULL);

    button = gtk_button_new_with_label (_("Ok"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                        button, TRUE, TRUE, 10);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (OkDeleteNoteDialog), (gpointer) dialog);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    button = gtk_button_new_with_label (_("Cancel"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                        button, TRUE, TRUE, 10);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (CancelDialog), (gpointer) dialog);
    gtk_widget_show (button);

    gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 5);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
    gtk_widget_show (hbox);

    pixmap = gtk_pixmap_new (questMark.pixmap, questMark.mask);
    gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 10);
    gtk_widget_show (pixmap);

    label = gtk_label_new (_("Do you want to delete selected note(s)?"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 10);
    gtk_widget_show (label);
  }

  gtk_widget_show (GTK_WIDGET (dialog));
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
  { NULL,		"<control>D",	DeleteNote, 0, NULL},
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
  SortColumn *sColumn;
  register gint i;
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
                           NewPixmap(NewRem_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) AddReminder, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add call"), NULL,
                           NewPixmap(NewCall_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add meeting"), NULL,
                           NewPixmap(NewMeet_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Add birthday"), NULL,
                           NewPixmap(NewBD_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Delete note"), NULL,
                           NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
                           &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) DeleteNote, NULL);

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
  gtk_clist_set_compare_func (GTK_CLIST (cal.notesClist), CListCompareFunc);
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

  for (i = 0; i < 6; i++)
  {
    if ((sColumn = g_malloc (sizeof (SortColumn))) == NULL)
    {
      g_print (_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
      gtk_main_quit ();
    }
    sColumn->clist = cal.notesClist;
    sColumn->column = i;
    gtk_signal_connect (GTK_OBJECT (GTK_CLIST (cal.notesClist)->column[i].button), "clicked",
                        GTK_SIGNAL_FUNC (SetSortColumn), (gpointer) sColumn);
  }

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

  questMark.pixmap = gdk_pixmap_create_from_xpm_d (GUI_CalendarWindow->window,
                         &questMark.mask,
                         &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL],
                         quest_xpm);

  CreateErrorDialog (&errorDialog, GUI_CalendarWindow);
}
