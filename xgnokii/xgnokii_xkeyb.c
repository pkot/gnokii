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
  Copyright (C) 2002 BORBELY Zoltan

*/

#include "config.h"

#include <string.h>
#include <gtk/gtk.h>
#include "misc.h"
#include "gnokii.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_xkeyb.h"

typedef struct {
	int top_left_x, top_left_y;
	int bottom_right_x, bottom_right_y;
	int code;
} ButtonT;


static GtkWidget *GUI_XkeybWindow;
static GtkWidget *pixArea;
static GtkWidget *phonePixmap = NULL;
static ErrorDialog errorDialog = { NULL, NULL };
static ButtonT *button = NULL;

static ButtonT button_6110[30] = {
	{103, 91, 114, 107, GN_KEY_POWER},		/* Power */
	{28, 240, 54, 263, GN_KEY_MENU},		/* Menu */
	{84, 240, 110, 263, GN_KEY_NAMES},		/* Names */
	{58, 245, 82, 258, GN_KEY_UP},			/* Up */
	{55, 263, 85, 276, GN_KEY_DOWN},		/* Down */
	{22, 271, 50, 289, GN_KEY_GREEN},		/* Green */
	{91, 271, 115, 289, GN_KEY_RED},		/* Red */
	{18, 294, 44, 310, GN_KEY_1},			/* 1 */
	{56, 294, 85, 310, GN_KEY_2},			/* 2 */
	{98, 294, 121, 310, GN_KEY_3},			/* 3 */
	{18, 317, 44, 333, GN_KEY_4},			/* 4 */
	{56, 317, 85, 333, GN_KEY_5},			/* 5 */
	{98, 317, 121, 333, GN_KEY_6},			/* 6 */
	{18, 342, 44, 356, GN_KEY_7},			/* 7 */
	{56, 342, 85, 356, GN_KEY_8},			/* 8 */
	{98, 342, 121, 356, GN_KEY_9},			/* 9 */
	{18, 365, 44, 380, GN_KEY_ASTERISK},		/* * */
	{56, 365, 85, 380, GN_KEY_0},			/* 0 */
	{98, 365, 121, 380, GN_KEY_HASH},		/* # */
	{1, 138, 10, 150, GN_KEY_INCREASEVOLUME},	/* Volume + */
	{1, 165, 10, 176, GN_KEY_DECREASEVOLUME},	/* Volume - */
	{0, 0, 0, 0, 0x00}
};

static ButtonT button_6150[30] = {
	{99, 78, 114, 93, GN_KEY_POWER},		/* Power */
	{20, 223, 49, 245, GN_KEY_MENU},		/* Menu */
	{90, 223, 120, 245, GN_KEY_NAMES},		/* Names */
	{59, 230, 83, 247, GN_KEY_UP},			/* Up */
	{56, 249, 84, 265, GN_KEY_DOWN},		/* Down */
	{14, 254, 51, 273, GN_KEY_GREEN},		/* Green */
	{90, 255, 126, 273, GN_KEY_RED},		/* Red */
	{18, 281, 53, 299, GN_KEY_1},			/* 1 */
	{55, 280, 86, 299, GN_KEY_2},			/* 2 */
	{90, 281, 122, 299, GN_KEY_3},			/* 3 */
	{18, 303, 53, 323, GN_KEY_4},			/* 4 */
	{55, 303, 87, 323, GN_KEY_5},			/* 5 */
	{90, 303, 122, 323, GN_KEY_6},			/* 6 */
	{18, 327, 53, 346, GN_KEY_7},			/* 7 */
	{53, 327, 87, 346, GN_KEY_8},			/* 8 */
	{90, 327, 122, 346, GN_KEY_9},			/* 9 */
	{18, 349, 53, 370, GN_KEY_ASTERISK},		/* * */
	{56, 349, 87, 370, GN_KEY_0},			/* 0 */
	{98, 349, 122, 370, GN_KEY_HASH},		/* # */
	{2, 131, 10, 147, GN_KEY_INCREASEVOLUME},	/* Volume + */
	{2, 155, 10, 173, GN_KEY_DECREASEVOLUME},	/* Volume - */
	{0, 0, 0, 0, 0x00}
};

static ButtonT button_5110[30] = {
	{100, 85, 114, 99, GN_KEY_POWER},	/* Power */
	{50, 240, 85, 265, GN_KEY_MENU},	/* Menu */
	{20, 240, 45, 260, GN_KEY_NAMES},	/* Names */
	{100, 240, 117, 258, GN_KEY_UP},	/* Up */
	{93, 267, 112, 287, GN_KEY_DOWN},	/* Down */
	{14, 294, 44, 312, GN_KEY_1},		/* 1 */
	{54, 294, 83, 312, GN_KEY_2},		/* 2 */
	{94, 294, 122, 312, GN_KEY_3},		/* 3 */
	{14, 320, 44, 338, GN_KEY_4},		/* 4 */
	{54, 320, 83, 338, GN_KEY_5},		/* 5 */
	{94, 320, 122, 338, GN_KEY_6},		/* 6 */
	{14, 345, 44, 363, GN_KEY_7},		/* 7 */
	{54, 345, 83, 363, GN_KEY_8},		/* 8 */
	{94, 345, 122, 363, GN_KEY_9},		/* 9 */
	{18, 374, 49, 389, GN_KEY_ASTERISK},	/* * */
	{53, 371, 82, 387, GN_KEY_0},		/* 0 */
	{96, 374, 119, 389, GN_KEY_HASH},	/* # */
	{0, 0, 0, 0, 0x00}
};

static GtkWidget *GetPixmap(void)
{
	GtkWidget *wpixmap;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	gchar *file;

	if (!strcmp(phoneMonitor.phone.model, "6110") || !strcmp(phoneMonitor.phone.model, "6120")) {
		button = button_6110;
		file = g_strdup_printf("%s%s", xgnokiiConfig.xgnokiidir, "/xpm/keyb_6110.xpm");
	} else if (!strcmp(phoneMonitor.phone.model, "6130") ||
		   !strcmp(phoneMonitor.phone.model, "6150") ||
		   !strcmp(phoneMonitor.phone.model, "616x") ||
		   !strcmp(phoneMonitor.phone.model, "6185") ||
		   !strcmp(phoneMonitor.phone.model, "6190")) {
		button = button_6150;
		file = g_strdup_printf("%s%s", xgnokiiConfig.xgnokiidir, "/xpm/keyb_6150.xpm");
	} else if (!strcmp(phoneMonitor.phone.model, "5110") ||
		   !strcmp(phoneMonitor.phone.model, "5130") ||
		   !strcmp(phoneMonitor.phone.model, "5160") ||
		   !strcmp(phoneMonitor.phone.model, "5190")) {
		button = button_5110;
		file = g_strdup_printf("%s%s", xgnokiiConfig.xgnokiidir, "/xpm/keyb_5110.xpm");
	} else
		return NULL;

	pixmap = gdk_pixmap_create_from_xpm(pixArea->window, &mask,
					    &pixArea->style->bg[GTK_STATE_NORMAL], file);
	g_free(file);

	if (pixmap == NULL)
		return NULL;

	wpixmap = gtk_pixmap_new(pixmap, mask);

	return wpixmap;
}


static inline void CloseXkeyb(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(GUI_XkeybWindow);
}


void GUI_ShowXkeyb(void)
{
	if (phonePixmap == NULL) {
		phonePixmap = GetPixmap();
		if (phonePixmap != NULL) {
			gtk_fixed_put(GTK_FIXED(pixArea), phonePixmap, 0, 0);
			gtk_widget_show(phonePixmap);
		} else {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Cannot load background pixmap!"));
			gtk_widget_show(errorDialog.dialog);
		}
	}
	gtk_window_present(GTK_WINDOW(GUI_XkeybWindow));
}


static gint ButtonEvent(GtkWidget * widget, GdkEventButton * event)
{
	register gint i = 0;
	int action;

	if (button == NULL)
		return TRUE;

	if (event->button != 1)
		return TRUE;

	if (event->type == GDK_BUTTON_PRESS)
		action = Event_PressKey;
	else if (event->type == GDK_BUTTON_RELEASE)
		action = Event_ReleaseKey;
	else
		return TRUE;

//  g_print ("%f %f\n", event->x, event->y);

	while (button[i].top_left_x != 0) {
		if (button[i].top_left_x <= event->x &&
		    event->x <= button[i].bottom_right_x &&
		    button[i].top_left_y <= event->y && event->y <= button[i].bottom_right_y) {
			PhoneEvent *e = g_malloc(sizeof(PhoneEvent));

			e->event = action;
			e->data = GINT_TO_POINTER(button[i].code);
			GUI_InsertEvent(e);
		}

		i++;
	}

	return TRUE;
}


static GtkItemFactoryEntry menu_items[] = {
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>W", CloseXkeyb, 0, NULL},
	{NULL, NULL, NULL, 0, "<LastBranch>"},
	{NULL, NULL, GUI_ShowAbout, 0, NULL},
};

static void InitMainMenu(void)
{
	menu_items[0].path = _("/_File");
	menu_items[1].path = _("/File/_Close");
	menu_items[2].path = _("/_Help");
	menu_items[3].path = _("/Help/_About");
}


void GUI_CreateXkeybWindow(void)
{
	int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar;
	GtkWidget *main_vbox;

	InitMainMenu();
	GUI_XkeybWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_XkeybWindow), "XkeybWindow", "Xgnokii");
	gtk_window_set_title(GTK_WINDOW(GUI_XkeybWindow), _("XGnokii Keyboard"));
	//gtk_widget_set_usize (GTK_WIDGET (GUI_XkeybWindow), 436, 220);
	gtk_signal_connect(GTK_OBJECT(GUI_XkeybWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	gtk_widget_realize(GUI_XkeybWindow);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

	gtk_window_add_accel_group(GTK_WINDOW(GUI_XkeybWindow), accel_group);
	
	/* Finally, return the actual menu bar created by the item factory. */
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
	gtk_container_add(GTK_CONTAINER(GUI_XkeybWindow), main_vbox);
	gtk_widget_show(main_vbox);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);

	pixArea = gtk_fixed_new();
	gtk_fixed_set_has_window(GTK_FIXED(pixArea), TRUE);
	gtk_signal_connect(GTK_OBJECT(pixArea), "button_press_event",
			   (GtkSignalFunc) ButtonEvent, NULL);
	gtk_signal_connect(GTK_OBJECT(pixArea), "button_release_event",
			   (GtkSignalFunc) ButtonEvent, NULL);
	gtk_widget_set_events(pixArea, GDK_EXPOSURE_MASK
			      | GDK_LEAVE_NOTIFY_MASK
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_BUTTON_RELEASE_MASK
			      | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	gtk_box_pack_start(GTK_BOX(main_vbox), pixArea, FALSE, FALSE, 3);
	gtk_widget_show(pixArea);

	CreateErrorDialog(&errorDialog, GUI_XkeybWindow);
}
