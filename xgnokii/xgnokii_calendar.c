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

  Copyright (C) 1999      Pavel Janik ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak
  Copyright (C) 2001-2002 Pawel Kot
  Copyright (C) 2002      BORBELY Zoltan, Markus Plail

*/

#include "config.h"

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
/* #include "xpm/Edit.xpm" */
#include "xpm/Delete.xpm"
#include "xpm/SendSMS.xpm"
#include "xpm/NewBD.xpm"
#include "xpm/NewCall.xpm"
#include "xpm/NewMeet.xpm"
#include "xpm/NewRem.xpm"
#include "xpm/quest.xpm"

/* Calendar entry columns */
#define ENTRY_COLUMNS		9
#define ENTRY_ID		0
#define ENTRY_TYPE		1
#define ENTRY_SDATE		2
#define ENTRY_EDATE		3
#define ENTRY_TEXT		4
#define ENTRY_NUMBER		5 /* Just for calls */
#define ENTRY_LOCATION		6 /* Just for meetings */
#define ENTRY_ALARM		7
#define ENTRY_RECURRENCE	8

typedef struct {
	GtkWidget *calendar;
	GtkWidget *notesClist;
	GtkWidget *noteText;
	GdkColor colour;
} CalendarWidget;


typedef struct {
	guint year;
	guint month;
	guint day;
	GtkWidget *button;
} Date;


typedef struct {
	Date date;
	guchar hours;
	guchar minutes;
	GtkWidget *button;
	GtkWidget *hButton;
	GtkWidget *mButton;
} DateTime;


typedef struct {
	GtkWidget *dialog;
	DateTime date;
	DateTime alarm;
	gint recurrence;
	GtkWidget *alarmCheck;
	GtkWidget *text;
	GtkWidget *phone;
	GtkWidget *group;
	GtkWidget *groupLabel;
	GtkWidget *groupMenu;
} AddDialogData;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *cal;
} CalendarDialog;


typedef struct {
	GtkWidget *dialog;
	GtkWidget *cal;
} CalTimeDialog;


static GtkWidget *GUI_CalendarWindow;
static ErrorDialog errorDialog = { NULL, NULL };
static CalendarWidget cal = { NULL, NULL };
static QuestMark questMark;
static AddDialogData addReminderDialogData;
static AddDialogData addCallDialogData;
static AddDialogData addMeetingDialogData;
static AddDialogData addBirthdayDialogData;
static CalendarDialog calendarDialog = { NULL, NULL };
static CalTimeDialog calTimeDialog = { NULL, NULL };

static void FunctionNotImplemented(void)
{
	gtk_label_set_text(GTK_LABEL(errorDialog.text), _("Function not implemented!"));  
	gtk_widget_show(errorDialog.dialog);
}

static inline void CloseCalendar(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(GUI_CalendarWindow);
}


inline void GUI_ShowCalendar()
{
	if (phoneMonitor.supported & PM_CALENDAR)
		gtk_window_present(GTK_WINDOW(GUI_CalendarWindow));
}


static gint InsertCalendarEntry(gn_calnote * note)
{
	gchar *row[ENTRY_COLUMNS];
	int free_recur = 0;

	row[ENTRY_ID] = g_strdup_printf("%d", note->location);

	switch (note->type) {
	case GN_CALNOTE_MEMO:
		row[ENTRY_TYPE] = _("Memo");
		row[ENTRY_NUMBER] = "";
		row[ENTRY_LOCATION] = "";
		break;

	case GN_CALNOTE_REMINDER:
		row[ENTRY_TYPE] = _("Reminder");
		row[ENTRY_NUMBER] = "";
		row[ENTRY_LOCATION] = "";
		break;

	case GN_CALNOTE_CALL:
		row[ENTRY_TYPE] = _("Call");
		row[ENTRY_NUMBER] = note->phone_number;
		row[ENTRY_LOCATION] = "";
		break;

	case GN_CALNOTE_MEETING:
		row[ENTRY_TYPE] = _("Meeting");
		row[ENTRY_NUMBER] = "";
		row[ENTRY_LOCATION] = note->mlocation;
		break;

	case GN_CALNOTE_BIRTHDAY:
		row[ENTRY_TYPE] = _("Birthday");
		row[ENTRY_NUMBER] = "";
		row[ENTRY_LOCATION] = "";
		break;

	default:
		row[ENTRY_TYPE] = _("Unknown");
		row[ENTRY_NUMBER] = "";
		row[ENTRY_LOCATION] = "";
		break;
	}

	if (note->type == GN_CALNOTE_BIRTHDAY) {
		row[ENTRY_SDATE] = g_strdup_printf(_("%04d/%02d/%02d"),
					note->time.year, note->time.month, note->time.day);
	} else {
		row[ENTRY_SDATE] = g_strdup_printf(_("%04d/%02d/%02d %02d:%02d"),
					note->time.year, note->time.month, note->time.day,
					note->time.hour, note->time.minute);
	}

	if (note->end_time.year) {
		row[ENTRY_EDATE] = g_strdup_printf(_("%04d/%02d/%02d %02d:%02d"),
					note->end_time.year, note->end_time.month, note->end_time.day,
					note->end_time.hour, note->end_time.minute);
	} else {
		row[ENTRY_EDATE] = "";
	}
	if (note->alarm.enabled) {
		if (note->type == GN_CALNOTE_BIRTHDAY) {
			row[ENTRY_ALARM] = g_strdup_printf(_("%02d/%02d %02d:%02d tone %s"),
						note->alarm.timestamp.month, note->alarm.timestamp.day,
						note->alarm.timestamp.hour, note->alarm.timestamp.minute,
						note->alarm.tone ? _("enabled") : _("disabled"));
		} else {
			row[ENTRY_ALARM] = g_strdup_printf(_("%04d/%02d/%02d %02d:%02d tone %s"),
						note->alarm.timestamp.year, note->alarm.timestamp.month,
						note->alarm.timestamp.day, note->alarm.timestamp.hour,
						note->alarm.timestamp.minute,
						note->alarm.tone ? _("enabled") : _("disabled"));
		}
	} else {
		row[ENTRY_ALARM] = "";
	}
	row[ENTRY_TEXT] = note->text;

	switch (note->recurrence) {
	case GN_CALNOTE_NEVER:
		row[ENTRY_RECURRENCE] = _("never");
		break;
	case GN_CALNOTE_DAILY:
		row[ENTRY_RECURRENCE] = _("daily");
		break;
	case GN_CALNOTE_WEEKLY:
		row[ENTRY_RECURRENCE] = _("weekly");
		break;
	case GN_CALNOTE_2WEEKLY:
		row[ENTRY_RECURRENCE] = _("every two weeks");
		break;
	case GN_CALNOTE_MONTHLY:
		row[ENTRY_RECURRENCE] = _("monthly");
		break;
	case GN_CALNOTE_YEARLY:
		row[ENTRY_RECURRENCE] = _("yearly");
		break;
	default:
		row[ENTRY_RECURRENCE] = g_strdup_printf(_("every %d minutes"), note->recurrence * 60);
		free_recur = 1;
		break;
	}

	gtk_clist_freeze(GTK_CLIST(cal.notesClist));
	gtk_clist_append(GTK_CLIST(cal.notesClist), row);
	gtk_clist_sort(GTK_CLIST(cal.notesClist));
	gtk_clist_thaw(GTK_CLIST(cal.notesClist));

	g_free(row[ENTRY_ID]);
	g_free(row[ENTRY_SDATE]);
	if (*row[ENTRY_EDATE] != '\0')
		g_free(row[ENTRY_EDATE]);
	if (*row[ENTRY_ALARM] != '\0')
		g_free(row[ENTRY_ALARM]);
	if (free_recur)
		g_free(row[ENTRY_RECURRENCE]);

	return GN_ERR_NONE;
}

static void ClickEntry(GtkWidget * clist,
		       gint row, gint column, GdkEventButton * event, gpointer data)
{
	gchar *buf;
	int birthday = 0;
	int year, month, day;

	gtk_text_freeze(GTK_TEXT(cal.noteText));

	gtk_text_set_point(GTK_TEXT(cal.noteText), 0);
	gtk_text_forward_delete(GTK_TEXT(cal.noteText),
				gtk_text_get_length(GTK_TEXT(cal.noteText)));

	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL, _("Type: "), -1);
	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_TYPE, &buf);
	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL, buf, -1);
	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
			"\n", -1);

	if (!strcmp(buf, _("Birthday"))) {
		birthday = 1;
	}

	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL, _("Start date: "), -1);
	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_SDATE, &buf);
	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL, buf, -1);
	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
			"\n", -1);

	year = atoi(buf);
	month = atoi(buf + 5) - 1;
	day = atoi(buf + 8);
	if (birthday) {
		time_t t;
		struct tm *tm;
		
		t = time(NULL);
		tm = localtime(&t);
		if ((tm->tm_mon > month) ||
		    ((tm->tm_mon == month) &&
		     (tm->tm_mday > day))) {
			year = tm->tm_year + 1;
		} else {
			year = tm->tm_year;
		}
		year += 1900;
	}
	gtk_calendar_select_month(GTK_CALENDAR(cal.calendar), month, year);
	gtk_calendar_select_day(GTK_CALENDAR(cal.calendar), day);

	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_EDATE, &buf);
	if (*buf != '\0') {
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL,
				_("End date: "), -1);

		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				buf, -1);
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				"\n", -1);
	}

	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_ALARM, &buf);
	if (*buf != '\0') {
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL,
				_("Alarm: "), -1);

		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				buf, -1);
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				"\n", -1);
	}

	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_NUMBER, &buf);
	if (*buf != '\0') {
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL,
				_("Number: "), -1);

		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				buf, -1);
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				"\n", -1);
	}

	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_LOCATION, &buf);
	if (*buf != '\0') {
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL,
				_("Location: "), -1);

		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				buf, -1);
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				"\n", -1);
	}

	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_RECURRENCE, &buf);
	if (*buf != '\0') {
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL,
				_("Recurrence: "), -1);

		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				buf, -1);
		gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL,
				"\n", -1);
	}

	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.colour), NULL, _("Text: "), -1);
	gtk_clist_get_text(GTK_CLIST(clist), row, ENTRY_TEXT, &buf);
	gtk_text_insert(GTK_TEXT(cal.noteText), NULL, &(cal.noteText->style->black), NULL, buf, -1);

	gtk_text_thaw(GTK_TEXT(cal.noteText));
}

static inline gint ReadCalendarFailed(gint i)
{
	return (0);
}


static void ReadCalNotes(void)
{
	PhoneEvent *e;
	D_CalendarNoteAll *cna;

	gtk_clist_clear(GTK_CLIST(cal.notesClist));

	cna = (D_CalendarNoteAll *) g_malloc(sizeof(D_CalendarNoteAll));
	cna->InsertEntry = InsertCalendarEntry;
	cna->ReadFailed = ReadCalendarFailed;

	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetCalendarNoteAll;
	e->data = cna;
	GUI_InsertEvent(e);
}


static gint CListCompareFunc(GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
	char *text1 = NULL;
	char *text2 = NULL;

	GtkCListRow *row1 = (GtkCListRow *) ptr1;
	GtkCListRow *row2 = (GtkCListRow *) ptr2;

	switch (row1->cell[clist->sort_column].type) {
	case GTK_CELL_TEXT:
		text1 = GTK_CELL_TEXT(row1->cell[clist->sort_column])->text;
		break;
	case GTK_CELL_PIXTEXT:
		text1 = GTK_CELL_PIXTEXT(row1->cell[clist->sort_column])->text;
		break;
	default:
		break;
	}
	switch (row2->cell[clist->sort_column].type) {
	case GTK_CELL_TEXT:
		text2 = GTK_CELL_TEXT(row2->cell[clist->sort_column])->text;
		break;
	case GTK_CELL_PIXTEXT:
		text2 = GTK_CELL_PIXTEXT(row2->cell[clist->sort_column])->text;
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

	if (clist->sort_column == ENTRY_ID) {
		gint n1 = atoi(text1);
		gint n2 = atoi(text2);

		if (n1 > n2)
			return (1);
		else if (n1 < n2)
			return (-1);
		else
			return 0;
	}

	return (g_strcasecmp(text1, text2));
}


static gint ReverseSelection(gconstpointer a, gconstpointer b)
{
	gchar *buf1, *buf2;
	gint index1, index2;
	gint row1 = GPOINTER_TO_INT(a);
	gint row2 = GPOINTER_TO_INT(b);

	gtk_clist_get_text(GTK_CLIST(cal.notesClist), row1, 0, &buf1);
	gtk_clist_get_text(GTK_CLIST(cal.notesClist), row2, 0, &buf2);

	index1 = atoi(buf1);
	index2 = atoi(buf2);

	if (index1 < index2)
		return (1);
	else if (index1 > index2)
		return (-1);
	else
		return (0);
}


static gint AddCalendarNote(gn_calnote * cnote)
{
	gn_error error;
	PhoneEvent *e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	D_CalendarNote *dnote = (D_CalendarNote *) g_malloc(sizeof(D_CalendarNote));

	dnote->entry = cnote;
	e->event = Event_WriteCalendarNote;
	e->data = dnote;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&calendarMutex);
	pthread_cond_wait(&calendarCond, &calendarMutex);
	pthread_mutex_unlock(&calendarMutex);

	gn_log_xdebug("Saving calendar note\n");

	error = dnote->status;
	g_free(dnote);

	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error saving calendar note\n%s"), gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
	} else {
		gn_log_xdebug("Note saved\n");
		ReadCalNotes();
	}

	return (error);
}

static void OkAddReminderDialog(GtkWidget * widget, gpointer data)
{
	gn_calnote note;

	note.type = GN_CALNOTE_REMINDER;
	note.location = 0;
	strncpy(note.text, gtk_entry_get_text(GTK_ENTRY(((AddDialogData *) data)->text)), 20);
	note.time.year = ((AddDialogData *) data)->date.date.year;
	note.time.month = ((AddDialogData *) data)->date.date.month;
	note.time.day = ((AddDialogData *) data)->date.date.day;
	note.time.hour = note.time.minute = note.time.second = note.time.timezone = 0;
	if (GTK_TOGGLE_BUTTON(((AddDialogData *) data)->alarmCheck)->active) {
		note.alarm.timestamp.year = ((AddDialogData *) data)->alarm.date.year;
		note.alarm.timestamp.month = ((AddDialogData *) data)->alarm.date.month;
		note.alarm.timestamp.day = ((AddDialogData *) data)->alarm.date.day;
		note.alarm.timestamp.hour = ((AddDialogData *) data)->alarm.hours;
		note.alarm.timestamp.minute = ((AddDialogData *) data)->alarm.minutes;
		note.alarm.timestamp.second = note.alarm.timestamp.timezone = 0;
	} else {
		note.alarm.timestamp.year = 0;
	}
	note.recurrence = ((AddDialogData *) data)->recurrence;
	AddCalendarNote(&note);
	gtk_entry_set_text(GTK_ENTRY(((AddDialogData *) data)->text), "");
	gtk_widget_hide(((AddDialogData *) data)->dialog);
}

static void OkAddCallDialog(GtkWidget * widget, gpointer data)
{
	gn_calnote note;

	note.type = GN_CALNOTE_CALL;
	note.location = 0;
	strncpy(note.phone_number, gtk_entry_get_text(GTK_ENTRY(((AddDialogData *) data)->phone)), 20);
	strncpy(note.text, gtk_entry_get_text(GTK_ENTRY(((AddDialogData *) data)->text)), 20);
	note.time.year = ((AddDialogData *) data)->date.date.year;
	note.time.month = ((AddDialogData *) data)->date.date.month;
	note.time.day = ((AddDialogData *) data)->date.date.day;
	note.time.hour = ((AddDialogData *) data)->date.hours;
	note.time.minute = ((AddDialogData *) data)->date.minutes;
	note.time.second = note.time.timezone = 0;
	if (GTK_TOGGLE_BUTTON(((AddDialogData *) data)->alarmCheck)->active) {
		note.alarm.timestamp.year = ((AddDialogData *) data)->alarm.date.year;
		note.alarm.timestamp.month = ((AddDialogData *) data)->alarm.date.month;
		note.alarm.timestamp.day = ((AddDialogData *) data)->alarm.date.day;
		note.alarm.timestamp.hour = ((AddDialogData *) data)->alarm.hours;
		note.alarm.timestamp.minute = ((AddDialogData *) data)->alarm.minutes;
		note.alarm.timestamp.second = note.alarm.timestamp.timezone = 0;
	} else {
		note.alarm.timestamp.year = 0;
	}
	note.recurrence = ((AddDialogData *) data)->recurrence;
	AddCalendarNote(&note);
	gtk_entry_set_text(GTK_ENTRY(((AddDialogData *) data)->text), "");
	gtk_entry_set_text(GTK_ENTRY(((AddDialogData *) data)->phone), "");
	gtk_widget_hide(((AddDialogData *) data)->dialog);
}

static void OkAddMeetingDialog(GtkWidget * widget, gpointer data)
{
	gn_calnote note;

	note.type = GN_CALNOTE_MEETING;
	note.location = 0;
	strncpy(note.text, gtk_entry_get_text(GTK_ENTRY(((AddDialogData *) data)->text)), 20);
	note.time.year = ((AddDialogData *) data)->date.date.year;
	note.time.month = ((AddDialogData *) data)->date.date.month;
	note.time.day = ((AddDialogData *) data)->date.date.day;
	note.time.hour = ((AddDialogData *) data)->date.hours;
	note.time.minute = ((AddDialogData *) data)->date.minutes;
	note.time.second = note.time.timezone = 0;
	if (GTK_TOGGLE_BUTTON(((AddDialogData *) data)->alarmCheck)->active) {
		note.alarm.timestamp.year = ((AddDialogData *) data)->alarm.date.year;
		note.alarm.timestamp.month = ((AddDialogData *) data)->alarm.date.month;
		note.alarm.timestamp.day = ((AddDialogData *) data)->alarm.date.day;
		note.alarm.timestamp.hour = ((AddDialogData *) data)->alarm.hours;
		note.alarm.timestamp.minute = ((AddDialogData *) data)->alarm.minutes;
		note.alarm.timestamp.second = note.alarm.timestamp.timezone = 0;
	} else {
		note.alarm.timestamp.year = 0;
	}
	note.recurrence = ((AddDialogData *) data)->recurrence;
	AddCalendarNote(&note);
	gtk_entry_set_text(GTK_ENTRY(((AddDialogData *) data)->text), "");
	gtk_widget_hide(((AddDialogData *) data)->dialog);
}

static void OkAddBdayDialog(GtkWidget * widget, gpointer data)
{
	gn_calnote note;

	note.type = GN_CALNOTE_BIRTHDAY;
	note.location = 0;
	strncpy(note.text, gtk_entry_get_text(GTK_ENTRY(((AddDialogData *) data)->text)), 20);
	note.time.year = ((AddDialogData *) data)->date.date.year;
	note.time.month = ((AddDialogData *) data)->date.date.month;
	note.time.day = ((AddDialogData *) data)->date.date.day;
	note.time.hour = note.time.minute = note.time.second = note.time.timezone = 0;
	if (GTK_TOGGLE_BUTTON(((AddDialogData *) data)->alarmCheck)->active) {
		note.alarm.timestamp.year = ((AddDialogData *) data)->alarm.date.year;
		note.alarm.timestamp.month = ((AddDialogData *) data)->alarm.date.month;
		note.alarm.timestamp.day = ((AddDialogData *) data)->alarm.date.day;
		note.alarm.timestamp.hour = ((AddDialogData *) data)->alarm.hours;
		note.alarm.timestamp.minute = ((AddDialogData *) data)->alarm.minutes;
		note.alarm.timestamp.second = note.alarm.timestamp.timezone = 0;
	} else {
		note.alarm.timestamp.year = 0;
	}
	AddCalendarNote(&note);
	gtk_entry_set_text(GTK_ENTRY(((AddDialogData *) data)->text), "");
	gtk_widget_hide(((AddDialogData *) data)->dialog);
}


static inline void SetDateButton(Date * date)
{
	gchar *buf = g_strdup_printf(_("%d-%02d-%02d"), date->year,
				     date->month, date->day);
	gtk_label_set_text(GTK_LABEL(date->button), buf);
	g_free(buf);
}


static inline void SetDateTimeButton(DateTime * date)
{
	gchar *buf = g_strdup_printf(_("%d-%02d-%02d  %02d:%02d"), date->date.year,
				     date->date.month, date->date.day,
				     date->hours, date->minutes);
	gtk_label_set_text(GTK_LABEL(date->button), buf);
	g_free(buf);
}


static void OkCalendarDialog(GtkWidget * widget, Date * date)
{
	gtk_calendar_get_date(GTK_CALENDAR(calendarDialog.cal), &(date->year),
			      &(date->month), &(date->day));
	date->month += 1;

	SetDateButton(date);

	gtk_widget_hide(calendarDialog.dialog);
}


static void OkCalTimeDialog(GtkWidget * widget, DateTime * date)
{
	gtk_calendar_get_date(GTK_CALENDAR(calTimeDialog.cal), &(date->date.year),
			      &(date->date.month), &(date->date.day));
	date->date.month += 1;

	date->hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(date->hButton));
	date->minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(date->mButton));

	SetDateTimeButton(date);

	gtk_widget_hide(calTimeDialog.dialog);
}


static void ShowCalendar(GtkWidget * widget, Date * date)
{
	GtkWidget *button;

	calendarDialog.dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(calendarDialog.dialog), _("Choose date"));
	gtk_window_position(GTK_WINDOW(calendarDialog.dialog), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal(GTK_WINDOW(calendarDialog.dialog), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(calendarDialog.dialog), 10);
	gtk_signal_connect(GTK_OBJECT(calendarDialog.dialog), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	button = gtk_button_new_with_label(_("Ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(calendarDialog.dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(OkCalendarDialog), (gpointer) date);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(calendarDialog.dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(CancelDialog), (gpointer) calendarDialog.dialog);
	gtk_widget_show(button);

	gtk_container_set_border_width(GTK_CONTAINER
				       (GTK_DIALOG(calendarDialog.dialog)->vbox), 5);

	calendarDialog.cal = gtk_calendar_new();

	gtk_calendar_select_month(GTK_CALENDAR(calendarDialog.cal), date->month - 1,
				  date->year);
	gtk_calendar_select_day(GTK_CALENDAR(calendarDialog.cal), date->day);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(calendarDialog.dialog)->vbox),
			  calendarDialog.cal);
	gtk_widget_show(calendarDialog.cal);

	gtk_widget_show(calendarDialog.dialog);
}


static void ShowCalTime(GtkWidget * widget, DateTime * date)
{
	GtkWidget *button, *hbox, *label;
	GtkAdjustment *adj;

	calTimeDialog.dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(calTimeDialog.dialog), _("Choose date"));
	gtk_window_position(GTK_WINDOW(calTimeDialog.dialog), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal(GTK_WINDOW(calTimeDialog.dialog), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(calTimeDialog.dialog), 10);
	gtk_signal_connect(GTK_OBJECT(calTimeDialog.dialog), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	button = gtk_button_new_with_label(_("Ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(calTimeDialog.dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(OkCalTimeDialog), (gpointer) date);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(calTimeDialog.dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(CancelDialog), (gpointer) calTimeDialog.dialog);
	gtk_widget_show(button);

	gtk_container_set_border_width(GTK_CONTAINER
				       (GTK_DIALOG(calTimeDialog.dialog)->vbox), 5);

	calTimeDialog.cal = gtk_calendar_new();

	gtk_calendar_select_month(GTK_CALENDAR(calTimeDialog.cal), date->date.month - 1,
				  date->date.year);
	gtk_calendar_select_day(GTK_CALENDAR(calTimeDialog.cal), date->date.day);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(calTimeDialog.dialog)->vbox),
			  calTimeDialog.cal);
	gtk_widget_show(calTimeDialog.cal);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(calTimeDialog.dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Alarm time:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(date->hours, 0.0, 23.0, 1.0, 4.0, 0.0);
	date->hButton = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(date->hButton), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(date->hButton), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), date->hButton, FALSE, FALSE, 0);
	gtk_widget_show(date->hButton);

	label = gtk_label_new(":");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(date->minutes,
						   0.0, 59.0, 1.0, 10.0, 0.0);
	date->mButton = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(date->mButton), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(date->mButton), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), date->mButton, FALSE, FALSE, 0);
	gtk_widget_show(date->mButton);

	gtk_widget_show(calTimeDialog.dialog);
}

static inline void SetRecurrence0(GtkWidget * item, gpointer data)
{
	((AddDialogData *) data)->recurrence = 0x0000;
}

static inline void SetRecurrence1(GtkWidget * item, gpointer data)
{
	((AddDialogData *) data)->recurrence = 0x0018;
}

static inline void SetRecurrence2(GtkWidget * item, gpointer data)
{
	((AddDialogData *) data)->recurrence = 0x00A8;
}

static inline void SetRecurrence3(GtkWidget * item, gpointer data)
{
	((AddDialogData *) data)->recurrence = 0x0150;
}

static inline void SetRecurrence4(GtkWidget * item, gpointer data)
{
	((AddDialogData *) data)->recurrence = 0xffff;
}

void CreateRecurrenceMenu(AddDialogData * data)
{
	GtkWidget *item;

	if (data->groupMenu) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(data->group));
		if (GTK_IS_WIDGET(data->groupMenu))
			gtk_widget_destroy(GTK_WIDGET(data->groupMenu));
		data->groupMenu = NULL;
	}

	data->groupMenu = gtk_menu_new();

	item = gtk_menu_item_new_with_label(_("Never"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetRecurrence0), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Every Day"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetRecurrence1), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Every Week"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetRecurrence2), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Every 2 Weeks"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetRecurrence3), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Every Year"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetRecurrence4), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(data->group), data->groupMenu);
}



static void AddReminder(void)
{
	GtkWidget *button, *hbox, *vbox, *label;
	time_t t;
	struct tm *tm;

	if (addReminderDialogData.dialog == NULL) {
		addReminderDialogData.dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(addReminderDialogData.dialog), _("Add reminder"));
		gtk_window_position(GTK_WINDOW(addReminderDialogData.dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(addReminderDialogData.dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(addReminderDialogData.dialog), 10);
		gtk_signal_connect(GTK_OBJECT(addReminderDialogData.dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addReminderDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkAddReminderDialog),
				   (gpointer) & addReminderDialogData);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);
		button = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addReminderDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(CancelDialog),
				   (gpointer) addReminderDialogData.dialog);
		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER
					       (GTK_DIALOG(addReminderDialogData.dialog)->vbox), 5);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(addReminderDialogData.dialog)->vbox),
				  vbox);
		gtk_widget_show(vbox);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Date:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		t = time(NULL);
		tm = localtime(&t);

		addReminderDialogData.date.date.year = addReminderDialogData.alarm.date.year =
		    tm->tm_year + 1900;
		addReminderDialogData.date.date.month = addReminderDialogData.alarm.date.month =
		    tm->tm_mon + 1;
		addReminderDialogData.date.date.day = addReminderDialogData.alarm.date.day = tm->tm_mday;

		addReminderDialogData.alarm.hours = tm->tm_hour;
		addReminderDialogData.alarm.minutes = tm->tm_min;

		button = gtk_button_new();

		addReminderDialogData.date.date.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addReminderDialogData.date.date.button);
		SetDateButton(&(addReminderDialogData.date.date));

		gtk_widget_show(addReminderDialogData.date.date.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalendar),
				   (gpointer) & (addReminderDialogData.date));
		gtk_widget_show(button);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Subject:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		addReminderDialogData.text = gtk_entry_new_with_max_length(30);
		gtk_box_pack_end(GTK_BOX(hbox), addReminderDialogData.text, FALSE, FALSE, 2);
		gtk_widget_show(addReminderDialogData.text);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		addReminderDialogData.alarmCheck = gtk_check_button_new_with_label(_("Alarm"));
		gtk_box_pack_start(GTK_BOX(hbox), addReminderDialogData.alarmCheck, FALSE, FALSE,
				   2);
		/*
		gtk_signal_connect (GTK_OBJECT (addReminderDialogData.alarmCheck), "toggled",
				    GTK_SIGNAL_FUNC (TogleAlarm), &addReminderDialogData);
		*/
		gtk_widget_show(addReminderDialogData.alarmCheck);

		button = gtk_button_new();

		addReminderDialogData.alarm.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addReminderDialogData.alarm.button);
		SetDateTimeButton(&(addReminderDialogData.alarm));

		gtk_widget_show(addReminderDialogData.alarm.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalTime),
				   (gpointer) & (addReminderDialogData.alarm));
		gtk_widget_show(button);
	}

	gtk_widget_show(GTK_WIDGET(addReminderDialogData.dialog));
}


static void AddCall(void)
{
	GtkWidget *button, *hbox, *vbox, *label;
	time_t t;
	struct tm *tm;

	if (addCallDialogData.dialog == NULL) {
		addCallDialogData.dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(addCallDialogData.dialog), _("Add Call"));
		gtk_window_position(GTK_WINDOW(addCallDialogData.dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(addCallDialogData.dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(addCallDialogData.dialog), 10);
		gtk_signal_connect(GTK_OBJECT(addCallDialogData.dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addCallDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkAddCallDialog),
				   (gpointer) & addCallDialogData);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);
		button = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addCallDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(CancelDialog),
				   (gpointer) addCallDialogData.dialog);
		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER
					       (GTK_DIALOG(addCallDialogData.dialog)->vbox), 5);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(addCallDialogData.dialog)->vbox), vbox);
		gtk_widget_show(vbox);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Date:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		t = time(NULL);
		tm = localtime(&t);

		addCallDialogData.date.date.year = addCallDialogData.alarm.date.year =
		    tm->tm_year + 1900;
		addCallDialogData.date.date.month = addCallDialogData.alarm.date.month =
		    tm->tm_mon + 1;
		addCallDialogData.date.date.day = addCallDialogData.alarm.date.day = tm->tm_mday;

		addCallDialogData.date.hours = addCallDialogData.alarm.hours = tm->tm_hour;
		addCallDialogData.date.minutes = addCallDialogData.alarm.minutes = tm->tm_min;

		button = gtk_button_new();

		addCallDialogData.date.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addCallDialogData.date.button);
		SetDateTimeButton(&(addCallDialogData.date));

		gtk_widget_show(addCallDialogData.date.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalTime),
				   (gpointer) & (addCallDialogData.date));
		gtk_widget_show(button);
		/* Phone */
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Number:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		addCallDialogData.phone = gtk_entry_new_with_max_length(30);
		gtk_box_pack_end(GTK_BOX(hbox), addCallDialogData.phone, FALSE, FALSE, 2);
		gtk_widget_show(addCallDialogData.phone);

		/* Text */
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Text:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		addCallDialogData.text = gtk_entry_new_with_max_length(30);
		gtk_box_pack_end(GTK_BOX(hbox), addCallDialogData.text, FALSE, FALSE, 2);
		gtk_widget_show(addCallDialogData.text);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		addCallDialogData.alarmCheck = gtk_check_button_new_with_label(_("Alarm"));
		gtk_box_pack_start(GTK_BOX(hbox), addCallDialogData.alarmCheck, FALSE, FALSE, 2);

		gtk_widget_show(addCallDialogData.alarmCheck);

		button = gtk_button_new();

		addCallDialogData.alarm.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addCallDialogData.alarm.button);
		SetDateTimeButton(&(addCallDialogData.alarm));

		gtk_widget_show(addCallDialogData.alarm.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalTime),
				   (gpointer) & (addCallDialogData.alarm));
		gtk_widget_show(button);

		addCallDialogData.groupLabel = gtk_label_new(_("Repeat:"));
		gtk_box_pack_start(GTK_BOX(hbox), addCallDialogData.groupLabel, FALSE, FALSE, 2);

		addCallDialogData.group = gtk_option_menu_new();
		
		CreateRecurrenceMenu(&addCallDialogData);

		gtk_box_pack_start(GTK_BOX(hbox), addCallDialogData.group, TRUE, TRUE, 0);

	}

	gtk_widget_show(addCallDialogData.group);
	gtk_widget_show(addCallDialogData.groupLabel);
	gtk_widget_show(GTK_WIDGET(addCallDialogData.dialog));
}


static void AddMeeting(void)
{
	GtkWidget *button, *hbox, *vbox, *label;
	time_t t;
	struct tm *tm;

	if (addMeetingDialogData.dialog == NULL) {
		addMeetingDialogData.dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(addMeetingDialogData.dialog), _("Add meeting"));
		gtk_window_position(GTK_WINDOW(addMeetingDialogData.dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(addMeetingDialogData.dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(addMeetingDialogData.dialog), 10);
		gtk_signal_connect(GTK_OBJECT(addMeetingDialogData.dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addMeetingDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkAddMeetingDialog),
				   (gpointer) & addMeetingDialogData);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);
		button = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addMeetingDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(CancelDialog),
				   (gpointer) addMeetingDialogData.dialog);
		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER
					       (GTK_DIALOG(addMeetingDialogData.dialog)->vbox), 5);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(addMeetingDialogData.dialog)->vbox),
				  vbox);
		gtk_widget_show(vbox);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Date:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		t = time(NULL);
		tm = localtime(&t);

		addMeetingDialogData.date.date.year = addMeetingDialogData.alarm.date.year =
		    tm->tm_year + 1900;
		addMeetingDialogData.date.date.month = addMeetingDialogData.alarm.date.month =
		    tm->tm_mon + 1;
		addMeetingDialogData.date.date.day = addMeetingDialogData.alarm.date.day =
		    tm->tm_mday;

		addMeetingDialogData.date.hours = addMeetingDialogData.alarm.hours = tm->tm_hour;
		addMeetingDialogData.date.minutes = addMeetingDialogData.alarm.minutes = tm->tm_min;

		button = gtk_button_new();

		addMeetingDialogData.date.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addMeetingDialogData.date.button);
		SetDateTimeButton(&(addMeetingDialogData.date));

		gtk_widget_show(addMeetingDialogData.date.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalTime),
				   (gpointer) & (addMeetingDialogData.date));
		gtk_widget_show(button);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Name:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		addMeetingDialogData.text = gtk_entry_new_with_max_length(30);
		gtk_box_pack_end(GTK_BOX(hbox), addMeetingDialogData.text, FALSE, FALSE, 2);
		gtk_widget_show(addMeetingDialogData.text);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		addMeetingDialogData.alarmCheck = gtk_check_button_new_with_label(_("Alarm"));
		gtk_box_pack_start(GTK_BOX(hbox), addMeetingDialogData.alarmCheck, FALSE, FALSE, 2);
		/*
		gtk_signal_connect (GTK_OBJECT (addMeetingDialogData.alarmCheck), "toggled",
				    GTK_SIGNAL_FUNC (TogleAlarm), &addMeetingDialogData);
		*/
		gtk_widget_show(addMeetingDialogData.alarmCheck);

		button = gtk_button_new();

		addMeetingDialogData.alarm.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addMeetingDialogData.alarm.button);
		SetDateTimeButton(&(addMeetingDialogData.alarm));

		gtk_widget_show(addMeetingDialogData.alarm.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalTime),
				   (gpointer) & (addMeetingDialogData.alarm));
		gtk_widget_show(button);

		addMeetingDialogData.groupLabel = gtk_label_new(_("Repeat:"));
		gtk_box_pack_start(GTK_BOX(hbox), addMeetingDialogData.groupLabel, FALSE, FALSE, 2);

		addMeetingDialogData.group = gtk_option_menu_new();
		
		CreateRecurrenceMenu(&addMeetingDialogData);

		gtk_box_pack_start(GTK_BOX(hbox), addMeetingDialogData.group, TRUE, TRUE, 0);

	}

	gtk_widget_show(addMeetingDialogData.group);
	gtk_widget_show(addMeetingDialogData.groupLabel);
	gtk_widget_show(GTK_WIDGET(addMeetingDialogData.dialog));
}


static void AddBirthday(void)
{
	GtkWidget *button, *hbox, *vbox, *label;
	time_t t;
	struct tm *tm;

	if (addBirthdayDialogData.dialog == NULL) {
		addBirthdayDialogData.dialog = gtk_dialog_new();

		gtk_window_set_title(GTK_WINDOW(addBirthdayDialogData.dialog), _("Add birthday"));
		gtk_window_position(GTK_WINDOW(addBirthdayDialogData.dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(addBirthdayDialogData.dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(addBirthdayDialogData.dialog), 10);
		gtk_signal_connect(GTK_OBJECT(addBirthdayDialogData.dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addBirthdayDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkAddBdayDialog),
				   (gpointer) & addBirthdayDialogData);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);
		button = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(addBirthdayDialogData.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(CancelDialog),
				   (gpointer) addBirthdayDialogData.dialog);
		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER
					       (GTK_DIALOG(addBirthdayDialogData.dialog)->vbox), 5);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(addBirthdayDialogData.dialog)->vbox),
				  vbox);
		gtk_widget_show(vbox);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Date:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		t = time(NULL);
		tm = localtime(&t);

		addBirthdayDialogData.date.date.year = addBirthdayDialogData.alarm.date.year =
		    tm->tm_year + 1900;
		addBirthdayDialogData.date.date.month = addBirthdayDialogData.alarm.date.month =
		    tm->tm_mon + 1;
		addBirthdayDialogData.date.date.day = addBirthdayDialogData.alarm.date.day = tm->tm_mday;

		addBirthdayDialogData.alarm.hours = tm->tm_hour;
		addBirthdayDialogData.alarm.minutes = tm->tm_min;

		button = gtk_button_new();

		addBirthdayDialogData.date.date.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addBirthdayDialogData.date.date.button);
		SetDateButton(&(addBirthdayDialogData.date.date));

		gtk_widget_show(addBirthdayDialogData.date.date.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalendar),
				   (gpointer) & (addBirthdayDialogData.date));
		gtk_widget_show(button);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Name:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		addBirthdayDialogData.text = gtk_entry_new_with_max_length(30);
		gtk_box_pack_end(GTK_BOX(hbox), addBirthdayDialogData.text, FALSE, FALSE, 2);
		gtk_widget_show(addBirthdayDialogData.text);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		gtk_widget_show(hbox);

		addBirthdayDialogData.alarmCheck = gtk_check_button_new_with_label(_("Alarm"));
		gtk_box_pack_start(GTK_BOX(hbox), addBirthdayDialogData.alarmCheck, FALSE, FALSE,
				   2);
		/*
		  gtk_signal_connect (GTK_OBJECT (addBirthdayDialogData.alarmCheck), "toggled",
		  GTK_SIGNAL_FUNC (TogleAlarm), &addBirthdayDialogData);
		*/
		gtk_widget_show(addBirthdayDialogData.alarmCheck);

		button = gtk_button_new();

		addBirthdayDialogData.alarm.button = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), addBirthdayDialogData.alarm.button);
		SetDateTimeButton(&(addBirthdayDialogData.alarm));

		gtk_widget_show(addBirthdayDialogData.alarm.button);

		gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(ShowCalTime),
				   (gpointer) & (addBirthdayDialogData.alarm));
		gtk_widget_show(button);
		gtk_box_pack_start(GTK_BOX(hbox), addBirthdayDialogData.group, TRUE, TRUE, 0);

	}
	gtk_widget_show(GTK_WIDGET(addBirthdayDialogData.dialog));
}


static void OkDeleteNoteDialog(GtkWidget * widget, gpointer data)
{
	gn_calnote *note;
	PhoneEvent *e;
	GList *sel;
	gint row;
	gchar *buf;


	sel = GTK_CLIST(cal.notesClist)->selection;

	gtk_clist_freeze(GTK_CLIST(cal.notesClist));

	sel = g_list_sort(sel, ReverseSelection);

	while (sel != NULL) {
		row = GPOINTER_TO_INT(sel->data);
		sel = sel->next;

		note = (gn_calnote *) g_malloc(sizeof(gn_calnote));
		gtk_clist_get_text(GTK_CLIST(cal.notesClist), row, 0, &buf);
		note->location = atoi(buf);

		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_DeleteCalendarNote;
		e->data = note;
		GUI_InsertEvent(e);
	}

	gtk_text_set_point(GTK_TEXT(cal.noteText), 0);
	gtk_text_forward_delete(GTK_TEXT(cal.noteText),
				gtk_text_get_length(GTK_TEXT(cal.noteText)));

	gtk_widget_hide(GTK_WIDGET(data));

	gtk_clist_thaw(GTK_CLIST(cal.notesClist));

	ReadCalNotes();
}


static void DeleteNote(void)
{
	static GtkWidget *dialog = NULL;
	GtkWidget *button, *hbox, *label, *pixmap;

	if (dialog == NULL) {
		dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(dialog), _("Delete calendar note"));
		gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
		gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkDeleteNoteDialog), (gpointer) dialog);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);
		button = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(CancelDialog), (gpointer) dialog);
		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 5);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
		gtk_widget_show(hbox);

		pixmap = gtk_pixmap_new(questMark.pixmap, questMark.mask);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 10);
		gtk_widget_show(pixmap);

		label = gtk_label_new(_("Do you want to delete selected note(s)?"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
		gtk_widget_show(label);
	}

	gtk_widget_show(GTK_WIDGET(dialog));
}

static GtkItemFactoryEntry menu_items[] = {
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>R", ReadCalNotes, 0, NULL},
	{NULL, "<control>S", FunctionNotImplemented, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>X", FunctionNotImplemented, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>I", FunctionNotImplemented, 0, NULL},
	{NULL, "<control>E", FunctionNotImplemented, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>W", CloseCalendar, 0, NULL},
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>N", AddReminder, 0, NULL},
	{NULL, "<control>C", AddCall, 0, NULL},
	{NULL, "<control>M", AddMeeting, 0, NULL},
	{NULL, "<control>B", AddBirthday, 0, NULL},
	/*
	{NULL, NULL, NULL, 0, NULL},
	*/
	{NULL, "<control>D", DeleteNote, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>A", NULL, 0, NULL},
	{NULL, NULL, NULL, 0, "<LastBranch>"},
	{NULL, NULL, GUI_ShowAbout, 0, NULL},
};

static void InitMainMenu(void)
{
	menu_items[0].path = _("/_File");
	menu_items[1].path = _("/File/_Read from phone");
	menu_items[2].path = _("/File/_Save to phone");
	menu_items[3].path = _("/File/Sep1");
	menu_items[4].path = _("/File/Send via S_MS");
	menu_items[5].path = _("/File/Sep2");
	menu_items[6].path = _("/File/_Import from file");
	menu_items[7].path = _("/File/_Export to file");
	menu_items[8].path = _("/File/Sep3");
	menu_items[9].path = _("/File/_Close");
	menu_items[10].path = _("/_Edit");
	menu_items[11].path = _("/Edit/Add _reminder");
	menu_items[12].path = _("/Edit/Add _call");
	menu_items[13].path = _("/Edit/Add _meeting");
	menu_items[14].path = _("/Edit/Add _birthday");
	/*
	menu_items[15].path = _("/Edit/_Edit");
	*/
	menu_items[15].path = _("/Edit/_Delete");
	menu_items[16].path = _("/Edit/Sep4");
	menu_items[17].path = _("/Edit/Select _all");
	menu_items[18].path = _("/_Help");
	menu_items[19].path = _("/Help/_About");
}

void GUI_CreateCalendarWindow()
{
	int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar, *toolbar, *scrolledWindow, *hpaned;
	GtkWidget *main_vbox;
	GdkColormap *cmap;
	time_t t;
	struct tm *tm;
	SortColumn *sColumn;
	register gint i;
	gchar *titles[ENTRY_COLUMNS];

	titles[ENTRY_ID]		= _("#");
	titles[ENTRY_TYPE]		= _("Type");
	titles[ENTRY_SDATE]		= _("Start date");
	titles[ENTRY_EDATE]		= _("End date");
	titles[ENTRY_TEXT]		= _("Text");
	titles[ENTRY_NUMBER]		= _("Number");
	titles[ENTRY_LOCATION]		= _("Location");
	titles[ENTRY_ALARM]		= _("Alarm");
	titles[ENTRY_RECURRENCE]	= _("Recurrence");
	InitMainMenu();
	GUI_CalendarWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_CalendarWindow), "CalendarWindow", "Xgnokii");
	gtk_window_set_title(GTK_WINDOW(GUI_CalendarWindow), _("Calendar"));
	gtk_widget_set_usize (GTK_WIDGET (GUI_CalendarWindow), 1000, 400);
	gtk_signal_connect(GTK_OBJECT(GUI_CalendarWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	gtk_widget_realize(GUI_CalendarWindow);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

	gtk_window_add_accel_group (GTK_WINDOW(GUI_CalendarWindow), accel_group);
	
	/* Finally, return the actual menu bar created by the item factory. */
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
	gtk_container_add(GTK_CONTAINER(GUI_CalendarWindow), main_vbox);
	gtk_widget_show(main_vbox);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);

	/* Create the toolbar */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Read from phone"), NULL,
				NewPixmap(Read_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ReadCalNotes, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Save to phone"), NULL,
				NewPixmap(Send_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) FunctionNotImplemented, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Send via SMS"), NULL,
				NewPixmap(SendSMS_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) FunctionNotImplemented, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Import from file"), NULL,
				NewPixmap(Open_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) FunctionNotImplemented, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Export to file"), NULL,
				NewPixmap(Save_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) FunctionNotImplemented, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	/*
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Edit note"), NULL,
				NewPixmap(Edit_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) NULL, NULL);
	*/
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Add reminder"), NULL,
				NewPixmap(NewRem_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) AddReminder, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Add call"), NULL,
				NewPixmap(NewCall_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) AddCall, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Add meeting"), NULL,
				NewPixmap(NewMeet_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) AddMeeting, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Add birthday"), NULL,
				NewPixmap(NewBD_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) AddBirthday, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Delete note"), NULL,
				NewPixmap(Delete_xpm, GUI_CalendarWindow->window,
					  &GUI_CalendarWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) DeleteNote, NULL);

	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_widget_show(toolbar);

	hpaned = gtk_hpaned_new();
	/*
	gtk_paned_set_handle_size (GTK_PANED (hpaned), 10);
	gtk_paned_set_gutter_size (GTK_PANED (hpaned), 15);
	*/
	gtk_box_pack_start(GTK_BOX(main_vbox), hpaned, TRUE, TRUE, 0);
	gtk_widget_show(hpaned);

	/* Note viewer */
	cal.noteText = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(cal.noteText), FALSE);
	gtk_text_set_word_wrap(GTK_TEXT(cal.noteText), TRUE);

	scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize (GTK_WIDGET (scrolledWindow), 250, 120);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_paned_add1(GTK_PANED(hpaned), scrolledWindow);

	gtk_container_add(GTK_CONTAINER(scrolledWindow), cal.noteText);
	gtk_widget_show_all(scrolledWindow);

	/* Calendar */
	cal.calendar = gtk_calendar_new();

	t = time(NULL);
	tm = localtime(&t);
	gtk_calendar_select_month(GTK_CALENDAR(cal.calendar), tm->tm_mon, tm->tm_year + 1900);
	gtk_calendar_select_day(GTK_CALENDAR(cal.calendar), tm->tm_mday);

	gtk_paned_add2(GTK_PANED(hpaned), cal.calendar);
	gtk_widget_show(cal.calendar);

	/* Notes list */
	cal.notesClist = gtk_clist_new_with_titles(ENTRY_COLUMNS, titles);
	gtk_clist_set_shadow_type(GTK_CLIST(cal.notesClist), GTK_SHADOW_OUT);
	gtk_clist_set_compare_func(GTK_CLIST(cal.notesClist), CListCompareFunc);
	gtk_clist_set_sort_column(GTK_CLIST(cal.notesClist), 0);
	gtk_clist_set_sort_type(GTK_CLIST(cal.notesClist), GTK_SORT_ASCENDING);
	gtk_clist_set_auto_sort(GTK_CLIST(cal.notesClist), FALSE);
	gtk_clist_set_selection_mode(GTK_CLIST(cal.notesClist), GTK_SELECTION_EXTENDED);

	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_ID, 17);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_TYPE, 55);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_SDATE, 115);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_EDATE, 115);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_TEXT, 130);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_LOCATION, 80);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_NUMBER, 80);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_ALARM, 210);
	gtk_clist_set_column_width(GTK_CLIST(cal.notesClist), ENTRY_RECURRENCE, 120);
	gtk_clist_set_column_justification(GTK_CLIST(cal.notesClist), 0, GTK_JUSTIFY_RIGHT);

	for (i = 0; i < ENTRY_COLUMNS; i++) {
		if ((sColumn = g_malloc(sizeof(SortColumn))) == NULL) {
			g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__,
				__LINE__);
			gtk_main_quit();
		}
		sColumn->clist = cal.notesClist;
		sColumn->column = i;
		gtk_signal_connect(GTK_OBJECT(GTK_CLIST(cal.notesClist)->column[i].button),
				   "clicked", GTK_SIGNAL_FUNC(SetSortColumn), (gpointer) sColumn);
	}

	gtk_signal_connect(GTK_OBJECT(cal.notesClist), "select_row",
			   GTK_SIGNAL_FUNC(ClickEntry), NULL);

	scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolledWindow, 900, 200);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), cal.notesClist);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_end(GTK_BOX(main_vbox), scrolledWindow, TRUE, TRUE, 0);

	gtk_widget_show(cal.notesClist);
	gtk_widget_show(scrolledWindow);

	cmap = gdk_colormap_get_system();
	cal.colour.red = 0xffff;
	cal.colour.green = 0;
	cal.colour.blue = 0;
	if (!gdk_color_alloc(cmap, &(cal.colour)))
		g_error(_("couldn't allocate colour"));

	questMark.pixmap = gdk_pixmap_create_from_xpm_d(GUI_CalendarWindow->window,
							&questMark.mask,
							&GUI_CalendarWindow->style->
							bg[GTK_STATE_NORMAL], quest_xpm);

	CreateErrorDialog(&errorDialog, GUI_CalendarWindow);
}
