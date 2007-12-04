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

  Copyright (C) 1999 Tomi Ollila
  Copyright (C) 2003 BORBELY Zoltan
  Copyright (C) 2004 Pawel Kot
  Copyright (C) 2005 Jan Derfinak
  

*/

/*
 * Id: xring.c
 *
 * Author: Tomi Ollila <Tomi.Ollila@iki.fi>
 *
 * 	Copyright (c) 1999 Tomi Ollila
 * 	    All rights reserved
 *
 * Created: Fri Aug 13 17:11:06 1999 too
 * Last modified: Sun Aug 29 18:50:24 1999 too
 *
 * INFO
 * This program is a quickly written GTK software to interactively create
 * tunes for some purpote, like cellular phone ring tones.
 *
 * Currently the code looks like somebody is just learning (by doing) how
 * to write GTK programs, and that is just what has happened. Expect
 * more polished versions ACN
 *
 * HISTORY 
 */

#include "config.h"

#ifndef WIN32
#  include <unistd.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "misc.h"
#include "gnokii.h"
#include "xgnokii.h"
#include "xgnokii_common.h"
#include "xgnokii_lowlevel.h"

#include "xpm/New.xpm"
#include "xpm/Send.xpm"
#include "xpm/Read.xpm"
#include "xpm/Delete.xpm"
#include "xpm/Open.xpm"
#include "xpm/Save.xpm"
#include "xpm/Play.xpm"

static char *blackparts[] = {
	".+@@@@@@@@@@@@+.",
	"+.+@@@@@@@@@@+.+",
	"@+.+@@@@@@@@+.+@",
	"@@+.+@@@@@@+.+@@",
	"@@@+........+@@@",
	"@@@@........@@@@"
};

static char *whitestarts[][3] = {
	{"        ++++++++++++++++",
	 "        +..............+",
	 "+++++++++..............+"},
	{
	 "        ++++++++        ",
	 "        +......+        ",
	 "+++++++++......+++++++++"},
	{
	 "++++++++++++++++        ",
	 "+..............+        ",
	 "+..............+++++++++"}
};

static char *whiteend[] = {
	"+......................+",
	"+@....................@+",
	"+@@..................@@+",
	"++@@@..............@@@++",
	"++++++++++++++++++++++++"
};


static void writecolors(char *buf0, char *col0,
			char *buf1, char *col1,
			char *buf2, char *col2)
{
	sprintf(buf0, ".	c #%s", col0);
	sprintf(buf1, "+	c #%s", col1);
	sprintf(buf2, "@	c #%s", col2);
}

typedef struct {
	GdkPixmap *pixmap;
	GdkBitmap *mask;
} PixmapAndMask;

static int createpixmap(GtkWidget * window, PixmapAndMask * pam, char **a);

int greateWhitePixmap(GtkWidget * window, PixmapAndMask * pam, char *start[],
		      char *color0, char *color1, char *color2)
{
	int i = 0, j = 0;
	char *a[160 + 5];
	char col0[20];
	char col1[20];
	char col2[20];

	a[i++] = "24 160 4 1";
	a[i++] = " 	c None";
	writecolors(col0, color0, col1, color1, col2, color2);
	a[i++] = col0;
	a[i++] = col1;
	a[i++] = col2;

	a[i++] = start[0];
	for (j = 0; j < 99; j++)
		a[i++] = start[1];
	a[i++] = start[2];

	for (j = 0; j < 54; j++)
		a[i++] = whiteend[0];

	a[i++] = whiteend[1];
	a[i++] = whiteend[1];
	a[i++] = whiteend[2];
	a[i++] = whiteend[3];
	a[i++] = whiteend[4];

	assert(i == 165);

	return createpixmap(window, pam, a);
}

int greateBlackPixmap(GtkWidget * window, PixmapAndMask * pam,
		      char *color0, char *color1, char *color2)
{
	int i = 0, j = 0;
	char *a[160 + 4];
	char col0[20];
	char col1[20];
	char col2[20];

	a[i++] = "16 100 3 1";
	writecolors(col0, color0, col1, color1, col2, color2);
	a[i++] = col0;
	a[i++] = col1;
	a[i++] = col2;

	for (j = 0; j < 5; j++)
		a[i++] = blackparts[j];
	for (j = 0; j < 90; j++)
		a[i++] = blackparts[5];
	for (j = 4; j >= 0; j--)
		a[i++] = blackparts[j];

	assert(i == 104);

	return createpixmap(window, pam, a);
}

static int createpixmap(GtkWidget * window, PixmapAndMask * pam, char **a)
{
	GtkStyle *style = gtk_widget_get_default_style();
	pam->pixmap = gdk_pixmap_create_from_xpm_d(window->window,
						   &pam->mask,
						   &style->
						   bg[GTK_STATE_NORMAL],
						   a);
	return (int) pam->pixmap;	/* (to) NULL or not (to) NULL */
}

enum {
	KIE_BLACK, KIE_BLACKSEL,
	KIE_WHITEL, KIE_WHITELSEL,
	KIE_WHITEM, KIE_WHITEMSEL,
	KIE_WHITER, KIE_WHITERSEL,
	KIE_COUNT
};

#define WHITE_COUNT 28		/* mit? on pianonkosketin englanniksi */

struct GUI {
	GtkWidget *w;
	GtkWidget *f;
	GtkAccelGroup *accel;
	GtkItemFactory *item_factory;
	GtkWidget *menu;
	GtkWidget *toolbar;
	GtkWidget *vbox;
	GtkWidget *rlist_combo;
	GtkWidget *name;
	GtkWidget *tempo_combo;
	ErrorDialog error_dialog;
	PixmapAndMask pam[KIE_COUNT];
	GtkWidget *blacks[WHITE_COUNT - 1];
	GtkWidget *whites[WHITE_COUNT];
	int focus;
	int pressed;
	gint32 presstime;
	gint32 releasetime;
	gchar *file_name;
	int volume;
	gn_ringtone ringtone;
	gn_ringtone_list ringtone_list;
};

static struct GUI gi = {0};
static char xwhi[] = {6, 4, 2, 6, 4, 4, 2};
static char *beats_per_minute[] = {
	" 25", " 28", " 31", " 35", " 40", " 45", " 50", " 56", " 63", " 70",
	" 80", " 90", "100", "112", "125", "140", "160", "180", "200", "225",
	"250", "285", "320", "355", "400", "450", "500", "565", "635", "715",
	"800", "900", NULL};

#define BLACK_PRESSED 64
#define WHITE_PRESSED 128


static void play_tone(int frequency, int volume, int usec)
{
	PhoneEvent *e = g_malloc(sizeof(PhoneEvent));
	D_PlayTone data;

	/* prepare data for event */

	data.frequency = frequency;
	data.volume = volume;
	data.usec = usec;
	e->event = Event_PlayTone;
	e->data = &data;

	/* launch event and wait for completition */
	GUI_InsertEvent(e);

	if (usec > 0)
		while (gtk_events_pending()) gtk_main_iteration_do(FALSE);

	pthread_mutex_lock(&ringtoneMutex);
	pthread_cond_wait(&ringtoneCond, &ringtoneMutex);
	pthread_mutex_unlock(&ringtoneMutex);
}

static gn_error ringtone_event(int event, gn_ringtone *ringtone)
{
	PhoneEvent *e = g_malloc(sizeof(PhoneEvent));
	D_Ringtone data;

	/* prepare data for event */

	data.ringtone = ringtone;
	e->event = event;
	e->data = &data;

	/* launch event and wait for completition */
	GUI_InsertEvent(e);

	pthread_mutex_lock(&ringtoneMutex);
	pthread_cond_wait(&ringtoneCond, &ringtoneMutex);
	pthread_mutex_unlock(&ringtoneMutex);

	return data.status;
}

static void get_ringtone_list(gn_ringtone_list *rlist)
{
	PhoneEvent *e = g_malloc(sizeof(PhoneEvent));

	/* prepare data for event */

	e->event = Event_GetRingtoneList;
	e->data = rlist;

	/* launch event and wait for completition */
	GUI_InsertEvent(e);

	pthread_mutex_lock(&ringtoneMutex);
	pthread_cond_wait(&ringtoneCond, &ringtoneMutex);
	pthread_mutex_unlock(&ringtoneMutex);
}

static void set_pixmap(struct GUI *gui, int flag)
{
	int i = gui->pressed & ~(BLACK_PRESSED | WHITE_PRESSED);
	int j;

	if (gui->pressed & BLACK_PRESSED) {
		j = flag ? KIE_BLACKSEL : KIE_BLACK;

		gtk_pixmap_set(GTK_PIXMAP(gui->blacks[i]),
			       gui->pam[j].pixmap, gui->pam[j].mask);
	} else {
		j = xwhi[i % 7] + (flag ? 1 : 0);
		gtk_pixmap_set(GTK_PIXMAP(gui->whites[i]),
			       gui->pam[j].pixmap, gui->pam[j].mask);
	}
}

static void tone_stop(struct GUI *gui)
{
	int usec;

	if (!gui->pressed) return;

	usec = 1000 * (gui->releasetime - gui->presstime);

	set_pixmap(gui, FALSE);
	gui->pressed = 0;

	play_tone(0, 0, -1);

	if (gui->releasetime < gui->presstime) {
		if (gui->ringtone.notes_count) gui->ringtone.notes_count--;
		return;
	}

	gn_ringtone_set_duration(&gui->ringtone, gui->ringtone.notes_count ? gui->ringtone.notes_count - 1 : 0, usec);

#if 0
	printf("%d %d %3d\n", gui->ringtone.notes[gui->ringtone.notes_count - 1].note / 14,
		gui->ringtone.notes[gui->ringtone.notes_count - 1].note % 14,
		gui->ringtone.notes[gui->ringtone.notes_count - 1].duration);
#endif
}

static void tone_start(struct GUI *gui)
{
	int note, pressed, freq, usec;

	if (!gui->pressed) return;

	set_pixmap(gui, TRUE);

	pressed = gui->pressed & ~(BLACK_PRESSED | WHITE_PRESSED);
	if (gui->pressed & WHITE_PRESSED) {
		note = 14 * (pressed / 7) + 2 * (pressed % 7);
	} else {
		note = 14 * (pressed / 7) + 2 * (pressed % 7) + 1;
	}

	gui->ringtone.notes[gui->ringtone.notes_count].note = note;
	gui->ringtone.notes[gui->ringtone.notes_count].duration = 0;
	gui->ringtone.notes_count++;

	gn_ringtone_get_tone(&gui->ringtone, gui->ringtone.notes_count - 1, &freq, &usec);
	play_tone(freq, gui->volume, -1);

	if (gui->ringtone.notes_count >= GN_RINGTONE_MAX_NOTES - 1) {
		gui->pressed = 0;
		return;
	}
}

static void load_ringtone_list(void)
{
	GList *list;
	int i, uidx;
	char **userdef;
	gn_ringtone_info *ri;

	get_ringtone_list(&gi.ringtone_list);
	userdef = g_new0(char *, gi.ringtone_list.userdef_count);

	list = NULL;
	i = 0;
	while (i < gi.ringtone_list.count) {
		if (!gi.ringtone_list.ringtone[i].readable && !gi.ringtone_list.ringtone[i].writable) {
			memmove(gi.ringtone_list.ringtone + i,
				gi.ringtone_list.ringtone + i + 1,
				(GN_RINGTONE_MAX_COUNT - i - 1) * sizeof(gn_ringtone_info));
			gi.ringtone_list.count--;
			continue;
		}
		uidx = gi.ringtone_list.ringtone[i].location - gi.ringtone_list.userdef_location;
		if (uidx >= 0 && uidx < gi.ringtone_list.userdef_count) {
			userdef[uidx] = g_strdup_printf(_("User #%d (%s)"), uidx, gi.ringtone_list.ringtone[i].name);
			list = g_list_append(list, userdef[uidx]);
		} else {
			list = g_list_append(list, gi.ringtone_list.ringtone[i].name);
		}
		i++;
	}

	for (i = 0; i < gi.ringtone_list.userdef_count; i++) {
		if (gi.ringtone_list.count >= GN_RINGTONE_MAX_COUNT) break;
		if (userdef[i]) continue;

		ri = gi.ringtone_list.ringtone + gi.ringtone_list.count++;
		ri->location = gi.ringtone_list.userdef_location + i;
		ri->name[0] = '\0';
		ri->user_defined = 0;
		ri->readable = 0;
		ri->writable = 1;

		userdef[i] = g_strdup_printf(_("User #%d (*EMPTY*)"), i);
		list = g_list_append(list, userdef[i]);
	}

	gtk_combo_set_popdown_strings(GTK_COMBO(gi.rlist_combo), list);

	for (i = 0; i < gi.ringtone_list.userdef_count; i++)
		if (userdef[i]) g_free(userdef[i]);
}

gn_ringtone_info *get_selected_ringtone(void)
{
	GtkList *list;
	int pos = -1;

	list = GTK_LIST(GTK_COMBO(gi.rlist_combo)->list);
	if (list && list->selection)
		pos = gtk_list_child_position(list, list->selection->data);

	if (pos < 0 || pos > gi.ringtone_list.count) return NULL;

	return gi.ringtone_list.ringtone + pos;
}

static void get_ringtone_info(gn_ringtone *ringtone)
{
	gn_ringtone_info *ri;
	char term;
	int x;

	if (!(ri = get_selected_ringtone())) return;
	ringtone->location = ri->location;

	snprintf(ringtone->name, sizeof(ringtone->name), "%s", gtk_entry_get_text(GTK_ENTRY(gi.name)));

	if (sscanf(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gi.tempo_combo)->entry)), " %d %c", &x, &term) == 1)
		ringtone->tempo = x;
}

static void set_ringtone_info(gn_ringtone *ringtone)
{
	char buf[256];

	gtk_entry_set_text(GTK_ENTRY(gi.name), ringtone->name);

	snprintf(buf, sizeof(buf), "%3d", ringtone->tempo);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(gi.tempo_combo)->entry), buf);
}

/* When invoked (via signal delete_event), terminates the application */
static void close_application(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	if (gi.file_name) {
		g_free(gi.file_name);
		gi.file_name = NULL;
	}
	play_tone(0, 0, -1);

//	gdk_key_repeat_restore();
	gtk_widget_hide(gi.w);
}

static int key_map(gint keysym)
{
	switch (keysym) {
	case GDK_q:				/* h-1 */
	case GDK_a: return  6 | WHITE_PRESSED;	/* h-1 in awerty -keyboard */
	case GDK_w: return  7 | WHITE_PRESSED;	/* c */
	case GDK_e: return  8 | WHITE_PRESSED;	/* d */
	case GDK_r: return  9 | WHITE_PRESSED;	/* e */
	case GDK_t: return 10 | WHITE_PRESSED;	/* f */
	case GDK_y:				/* g */
	case GDK_z: return 11 | WHITE_PRESSED;	/* g in qwertz -keyboard */
	case GDK_u: return 12 | WHITE_PRESSED;	/* a */
	case GDK_i: return 13 | WHITE_PRESSED;	/* h */
	case GDK_o: return 14 | WHITE_PRESSED;	/* c+1 */
	case GDK_p: return 15 | WHITE_PRESSED;	/* d+1 */

	case GDK_1: return  5 | BLACK_PRESSED;	/* a-1# */
	case GDK_3: return  7 | BLACK_PRESSED;	/* c# */
	case GDK_4: return  8 | BLACK_PRESSED;	/* d# */
	case GDK_6: return 10 | BLACK_PRESSED;	/* f# */
	case GDK_7: return 11 | BLACK_PRESSED;	/* g# */
	case GDK_8: return 12 | BLACK_PRESSED;	/* a# */
	case GDK_0: return 14 | BLACK_PRESSED;	/* c+1# */
	}
	return 0;
}

static gboolean button_press(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	GdkEventButton *e = (GdkEventButton *) event;
	int i;

	if (!gi.focus) {
		gtk_widget_grab_focus(gi.f);
		gi.focus = TRUE;
	}

	if (!gi.pressed) {
		guint x = e->x;
		guint y = e->y;

		gi.presstime = e->time;

		if (y < 100)	/* a possible black */
			for (i = 0; i < WHITE_COUNT - 1; i++)
				if (gi.blacks[i] && x - 16 - 24 * i < 16) {
					gi.pressed = i | BLACK_PRESSED;
					tone_start(&gi);
					return TRUE;	/* this a subroutine and we can do more... */
				}
		for (i = 0; i < WHITE_COUNT; i++)	/* whites ? */
			if (x - 24 * i < 24) {
				gi.pressed = i | WHITE_PRESSED;
				tone_start(&gi);
				return TRUE;
			}
	}
	return TRUE;
}

static gboolean button_release(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gi.releasetime = event->button.time;

	tone_stop(&gi);

	return TRUE;
}

static gboolean focus_in(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gi.focus = TRUE;
//	gdk_key_repeat_disable();

	return TRUE;
}

static gboolean focus_out(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	tone_stop(&gi);

	gi.focus = FALSE;
//	gdk_key_repeat_restore();

	return TRUE;
}

static gboolean key_press(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	int press = key_map(event->key.keyval);
	/* printf("key press %d\n", press); */

	if (press && !gi.pressed && gi.focus) {
		gi.presstime = event->key.time;
		gi.pressed = press;
		tone_start(&gi);
	}

	return TRUE;
}

static gboolean key_release(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	int press = key_map(event->key.keyval);
	/* printf("key release %d\n", press); */

	if (press && gi.pressed == press) {
		gi.releasetime = event->key.time;
		tone_stop(&gi);
	}

	return TRUE;
}

static void read_ringtone(GtkWidget *w, GtkFileSelection *fs)
{
	gchar *file_name;
	gn_error err;

	if (gi.file_name) {
		g_free(gi.file_name);
		gi.file_name = NULL;
	}

	file_name = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	gtk_widget_hide(GTK_WIDGET(fs));

	if ((err = gn_file_ringtone_read(file_name, &gi.ringtone)) != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error reading file\n%s"), gn_error_print(err));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}

	gi.file_name = g_strdup(file_name);
	set_ringtone_info(&gi.ringtone);
}

static void open_ringtone(GtkWidget *w)
{
	GtkWidget *file_select;

	file_select = gtk_file_selection_new(_("Open ringtone..."));

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_select)->ok_button),
			"clicked", (GtkSignalFunc)read_ringtone, file_select);

	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(file_select)->cancel_button),
			"clicked", (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT(file_select));

	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_select));

	gtk_widget_show(file_select);
}

static void write_ringtone(GtkWidget *w, GtkFileSelection *fs)
{
	gchar *file_name;
	gn_error err;

	file_name = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	gtk_widget_hide(GTK_WIDGET(fs));

	if ((err = gn_file_ringtone_save(file_name, &gi.ringtone)) != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error writing file\n%s"), gn_error_print(err));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}

	if (gi.file_name) {
		g_free(gi.file_name);
		gi.file_name = NULL;
	}
	gi.file_name = g_strdup(file_name);
}

static void save_ringtone_as(GtkWidget *w)
{
	GtkWidget *file_select;

	get_ringtone_info(&gi.ringtone);

	file_select = gtk_file_selection_new(_("Save ringtone as..."));

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_select)->ok_button),
			"clicked", (GtkSignalFunc)write_ringtone, file_select);

	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(file_select)->cancel_button),
			"clicked", (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT(file_select));

	gtk_widget_show(file_select);
}

static void save_ringtone(GtkWidget *w)
{
	gn_error err;

	get_ringtone_info(&gi.ringtone);

	if (!gi.file_name) {
		save_ringtone_as(w);
		return;
	}

	if ((err = gn_file_ringtone_save(gi.file_name, &gi.ringtone)) != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error writing file\n%s"), gn_error_print(err));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}
}

static void play_ringtone(GtkWidget *w)
{
	int i, freq, usec;

	get_ringtone_info(&gi.ringtone);

	set_pixmap(&gi, FALSE);
	for (i = 0; i < gi.ringtone.notes_count; i++) {
		if (gi.ringtone.notes[i].note == 255) {
			gn_ringtone_get_tone(&gi.ringtone, i, &freq, &usec);
			play_tone(freq, gi.volume, usec - 10000);
			continue;
		}
		if (gi.ringtone.notes[i].note & 1)
			gi.pressed = BLACK_PRESSED + gi.ringtone.notes[i].note / 2;
		else
			gi.pressed = WHITE_PRESSED + gi.ringtone.notes[i].note / 2;
		set_pixmap(&gi, TRUE);
		gn_ringtone_get_tone(&gi.ringtone, i, &freq, &usec);
		play_tone(freq, gi.volume, usec - 30000);
		set_pixmap(&gi, FALSE);
		while (gtk_events_pending()) gtk_main_iteration_do(FALSE);
		usleep(20000);
	}

	play_tone(0, 0, 0);
}

static void get_ringtone(GtkWidget *w)
{
	gn_error err;
	gn_ringtone_info *ri;

	ri = get_selected_ringtone();
	if (!ri) {
		gchar *buf = g_strdup_printf(_("Empty ringtone list"));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}

	gi.ringtone.location = ri->location;

	if ((err = ringtone_event(Event_GetRingtone, &gi.ringtone)) != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error getting ringtone\n%s"), gn_error_print(err));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}

	set_ringtone_info(&gi.ringtone);
}

static void set_ringtone(GtkWidget *w)
{
	gn_error err;

	get_ringtone_info(&gi.ringtone);

	if ((err = ringtone_event(Event_SetRingtone, &gi.ringtone)) != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error setting ringtone\n%s"), gn_error_print(err));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}

	load_ringtone_list();
}

static void delete_ringtone(GtkWidget *w)
{
	gn_error err;
	gn_ringtone ringtone;

	memset(&ringtone, 0, sizeof(ringtone));
	get_ringtone_info(&ringtone);

	if ((err = ringtone_event(Event_DeleteRingtone, &ringtone)) != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error deleting ringtone\n%s"), gn_error_print(err));
		gtk_label_set_text(GTK_LABEL(gi.error_dialog.text), buf);
		gtk_widget_show(gi.error_dialog.dialog);
		g_free(buf);
		return;
	}

	load_ringtone_list();
}

static void clear_ringtone(GtkWidget *w)
{
	gi.volume = 1;
	if (gi.file_name) {
		g_free(gi.file_name);
		gi.file_name = NULL;
	}
	memset(&gi.ringtone, 0, sizeof(gn_ringtone));
	gi.ringtone.tempo = 120;

	set_ringtone_info(&gi.ringtone);
}


void GUI_CreateXringWindow(void)
{
	int i;
	GList *list;
	GtkItemFactoryEntry menu_items[] = {
		{NULL, NULL,		NULL,		  0, "<Branch>"},
		{NULL, "<control>O",	open_ringtone,	  0, NULL},
		{NULL, "<control>S",	save_ringtone,	  0, NULL},
		{NULL, NULL,		save_ringtone_as, 0, NULL},
		{NULL, NULL,		NULL,		  0, "<Separator>"},
		{NULL, NULL,		get_ringtone,	  0, NULL},
		{NULL, NULL,		set_ringtone,	  0, NULL},
		{NULL, NULL,		delete_ringtone,  0, NULL},
		{NULL, NULL,		play_ringtone,	  0, NULL},
		{NULL, NULL,		NULL,		  0, "<Separator>"},
		{NULL, "<control>W",	close_application,0, NULL},
		{NULL, NULL,		NULL,		  0, "<Branch>"},
		{NULL, NULL,		clear_ringtone,	  0, NULL}
	};

	menu_items[0].path = _("/_File");
	menu_items[1].path = _("/File/_Open");
	menu_items[2].path = _("/File/_Save");
	menu_items[3].path = _("/File/Save _as ...");
	menu_items[4].path = _("/File/S1");
	menu_items[5].path = _("/File/_Get ringtone");
	menu_items[6].path = _("/File/Se_t ringtone");
	menu_items[7].path = _("/File/Delete ringtone");
	menu_items[8].path = _("/File/_Play");
	menu_items[9].path = _("/File/S2");
	menu_items[10].path = _("/File/_Close");
	menu_items[11].path = _("/_Edit");
	menu_items[12].path = _("/Edit/_Clear");

	/* create toplevel window */

	gi.w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(gi.w), "RingtoneWindow", "Xgnokii");
	gtk_window_set_policy(GTK_WINDOW(gi.w), FALSE, FALSE, TRUE);
	gtk_window_set_title(GTK_WINDOW(gi.w), _("Ringtone"));
	gtk_container_set_border_width(GTK_CONTAINER(gi.w), 4);

	gtk_signal_connect(GTK_OBJECT(gi.w), "delete_event",
			   GTK_SIGNAL_FUNC(close_application), &gi);

	gtk_widget_realize(gi.w);

	/* create vbox */

	gi.vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(gi.w), gi.vbox);

	/* create menubar */

	gi.accel = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(gi.w), gi.accel);
	gi.item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", gi.accel);
	gtk_item_factory_create_items(gi.item_factory, sizeof(menu_items) / sizeof(menu_items[0]), menu_items, NULL);
	gi.menu = gtk_item_factory_get_widget(gi.item_factory, "<main>");

	gtk_box_pack_start(GTK_BOX(gi.vbox), gi.menu, FALSE, FALSE, 0);

	/* create toolbar */

	gi.toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(gi.toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(gi.toolbar), GTK_ORIENTATION_HORIZONTAL);

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Clear ringtone"), NULL,
			NewPixmap(New_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)clear_ringtone, gi.toolbar);

	gtk_toolbar_append_space(GTK_TOOLBAR(gi.toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Get ringtone"), NULL,
			NewPixmap(Read_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)get_ringtone, gi.toolbar);

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Set ringtone"), NULL,
			NewPixmap(Send_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)set_ringtone, gi.toolbar);

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Delete ringtone"), NULL,
			NewPixmap(Delete_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)delete_ringtone, gi.toolbar);

	gtk_toolbar_append_space(GTK_TOOLBAR(gi.toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Import from file"), NULL,
			NewPixmap(Open_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)open_ringtone, gi.toolbar);

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Export to file"), NULL,
			NewPixmap(Save_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)save_ringtone, gi.toolbar);

	gtk_toolbar_append_space(GTK_TOOLBAR(gi.toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(gi.toolbar), NULL, _("Play"), NULL,
			NewPixmap(Play_xpm, gi.w->window, &gi.w->style->bg[GTK_STATE_NORMAL]),
			(GtkSignalFunc)play_ringtone, gi.toolbar);

	gtk_toolbar_append_space(GTK_TOOLBAR(gi.toolbar));

	gi.rlist_combo = gtk_combo_new();
	gtk_combo_set_use_arrows_always(GTK_COMBO(gi.rlist_combo), 1);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(gi.rlist_combo)->entry), FALSE);
	gtk_toolbar_append_widget(GTK_TOOLBAR(gi.toolbar), gi.rlist_combo, _("Select ringtone"), NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(gi.toolbar));

	gi.name = gtk_entry_new();
	gtk_toolbar_append_widget(GTK_TOOLBAR(gi.toolbar), gi.name, _("Ringtone name"), NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(gi.toolbar));

	gi.tempo_combo = gtk_combo_new();
	gtk_combo_set_use_arrows_always(GTK_COMBO(gi.tempo_combo), 1);
	gtk_widget_set_usize(gi.tempo_combo, 60, -1);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(gi.tempo_combo)->entry), TRUE);
	gtk_entry_set_max_length(GTK_ENTRY(GTK_COMBO(gi.tempo_combo)->entry), 3);
	for (list = NULL, i = 0; beats_per_minute[i]; i++)
		list = g_list_append(list, beats_per_minute[i]);
	gtk_combo_set_popdown_strings(GTK_COMBO(gi.tempo_combo), list);
	gtk_toolbar_append_widget(GTK_TOOLBAR(gi.toolbar), gi.tempo_combo, _("Set tempo"), NULL);

	gtk_box_pack_start(GTK_BOX(gi.vbox), gi.toolbar, FALSE, FALSE, 0);

	/* create fixed */

	gi.f = gtk_fixed_new();
	gtk_fixed_set_has_window(GTK_FIXED(gi.f), TRUE); 
	gtk_widget_set_usize(gi.f, 672, 160);
	gtk_box_pack_start(GTK_BOX(gi.vbox), gi.f, FALSE, FALSE, 0);

	gtk_widget_add_events(gi.f, GDK_FOCUS_CHANGE_MASK |
			      GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
			      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
			      GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK);

	GTK_WIDGET_SET_FLAGS(gi.f, GTK_CAN_FOCUS);

	gtk_signal_connect(GTK_OBJECT(gi.f), "key_press_event",
			   GTK_SIGNAL_FUNC(key_press), &gi);
	gtk_signal_connect(GTK_OBJECT(gi.f), "key_release_event",
			   GTK_SIGNAL_FUNC(key_release), &gi);

	/*
	gtk_signal_connect(GTK_OBJECT(gi.f), "focus_in_event",
			   GTK_SIGNAL_FUNC(focus_in), &gi);
	gtk_signal_connect(GTK_OBJECT(gi.f), "focus_out_event",
			   GTK_SIGNAL_FUNC(focus_out), &gi);
	*/
	gtk_signal_connect(GTK_OBJECT(gi.f), "enter_notify_event",
			   GTK_SIGNAL_FUNC(focus_in), &gi);
	gtk_signal_connect(GTK_OBJECT(gi.f), "leave_notify_event",
			   GTK_SIGNAL_FUNC(focus_out), &gi);

	gtk_signal_connect(GTK_OBJECT(gi.f), "button_press_event",
			   GTK_SIGNAL_FUNC(button_press), &gi);
	gtk_signal_connect(GTK_OBJECT(gi.f), "button_release_event",
			   GTK_SIGNAL_FUNC(button_release), &gi);

	greateBlackPixmap(gi.w, &gi.pam[KIE_BLACK], "333333", "666666",
			  "999999");
	greateBlackPixmap(gi.w, &gi.pam[KIE_BLACKSEL], "000000", "333333",
			  "666666");

	for (i = 0; i < 6; i += 2) {
		greateWhitePixmap(gi.w, &gi.pam[KIE_WHITEL + i],
				  whitestarts[i / 2], "FFFFFF", "999999",
				  "CCCCCC");
		greateWhitePixmap(gi.w, &gi.pam[KIE_WHITELSEL + i],
				  whitestarts[i / 2], "CCCCCC", "666666",
				  "999999");
	}

	for (i = 0; i < WHITE_COUNT - 1; i++) {
		PixmapAndMask *b = &gi.pam[KIE_BLACK];
		if (xwhi[i % 7] == 2)
			continue;
		gi.blacks[i] = gtk_pixmap_new(b->pixmap, b->mask);
		gtk_fixed_put(GTK_FIXED(gi.f), gi.blacks[i], 16 + 24 * i,
			      0);
	}

	for (i = 0; i < WHITE_COUNT; i++) {
		int j = xwhi[i % 7];
		gi.whites[i] =
		    gtk_pixmap_new(gi.pam[j].pixmap, gi.pam[j].mask);
		gtk_fixed_put(GTK_FIXED(gi.f), gi.whites[i], 24 * i, 0);
	}

	CreateErrorDialog(&gi.error_dialog, gi.w);
}

void GUI_ShowXring(void)
{
	clear_ringtone(NULL);

	load_ringtone_list();

	gi.focus = FALSE;
	gi.pressed = 0;

	gtk_widget_show_all(gi.w);
	gtk_window_present(GTK_WINDOW(gi.w));
	gtk_widget_grab_focus(gi.f);
}
