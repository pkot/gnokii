/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml. & Hugh Blemings.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sat Jun 26 07:51:13 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include <stdio.h>   /* for printf */
#include <unistd.h>  /* for usleep */
#include <stdlib.h>  /* for malloc */
#include <string.h>  /* for strtok */
#include <gtk/gtk.h>

#include "../misc.h"
#include "../gsm-common.h"
#include "../gsm-api.h"
#include "../fbus-6110.h"
#include "../fbus-3810.h"
#include "../cfgreader.h"
#include "xgnokii.h"
#include "xgnokii_contacts.h"
#include "xgnokii_sms.h"

#include "../pixmaps/logo.xpm"
#include "../pixmaps/background.xpm"
#include "../pixmaps/sms.xpm"
#include "../pixmaps/alarm.xpm"

static GtkWidget *GUI_SplashWindow;
static GtkWidget *GUI_MainWindow;
static GtkWidget *GUI_CardWindow;
static GtkWidget *GUI_AboutDialog;

/* Pixmap used for drawing all the stuff. */
static GdkPixmap *Pixmap = NULL;

/* Pixmap used for background. */
static GdkPixmap *BackgroundPixmap = NULL;

/* Pixmap used for SMS picture. */
static GdkPixmap *SMSPixmap = NULL;

/* Pixmap used for alarm picture. */
static GdkPixmap *AlarmPixmap = NULL;

/* Widgets for dialogs. */
static GtkWidget *GUI_OptionsDialog;

/* Widget for popup menu */
static GtkWidget *GUI_Menu;

/* Hold main configuration data for xgnokii */
XgnokiiConfig xgnokiiConfig;

gint max_phonebook_name_length; 
gint max_phonebook_number_length;
gint max_phonebook_sim_name_length;
gint max_phonebook_sim_number_length;

/* Local variables */
static char *DefaultModel = MODEL; /* From Makefile */
static char *DefaultPort = PORT;
static char *DefaultBindir = "/usr/sbin/";
static char *DefaultConnection = "serial";

static struct ConfigDialogData
{
  GtkWidget *port;
  GtkWidget *model;
  GtkWidget *init;
  GtkWidget *bindir;
  GtkWidget *serial, *infrared;
} configDialogData;

/* Variables which contain current signal and battery. */
static float rflevel=-1, batterylevel=-1;
static GSM_PowerSource powersource=-1;
static GSM_DateTime Alarm;
static GSM_SMSStatus SMSStatus = {0, 0}; 

GSM_Error fbusinit(bool enable_monitoring)
{
  int count=0;
  static GSM_Error error=GE_NOLINK;
  GSM_ConnectionType connection=GCT_Serial;

  if (!strcmp(xgnokiiConfig.connection, "infrared"))
    connection = GCT_Infrared;
    
  /* Initialise the code for the GSM interface. */     

  if (error==GE_NOLINK)
    error = GSM_Initialise(xgnokiiConfig.model, xgnokiiConfig.port,
                           xgnokiiConfig.initlength, connection, enable_monitoring);

#ifdef XDEBUG
  g_print("fbusinit: error %d\n", error);
#endif
  
  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    /* FIXME: should popup some message... */
    exit(-1);
  }

#ifdef XDEBUG
  g_print("Pred usleep.\n");
#endif
  while (count++ < 40 && *GSM_LinkOK == false)
    usleep(50000);
#ifdef XDEBUG
  g_print("Za usleep. GSM_LinkOK: %d\n", *GSM_LinkOK);
#endif

  return *GSM_LinkOK;
}

void GUI_DrawBackground(GtkWidget *data) {

  gdk_draw_pixmap(Pixmap,
		  GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		  BackgroundPixmap,
		  0, 0,
		  0, 0,
		  261, 96);
}

int network_levels[] = {
  152, 69, 11, 3,
  138, 69, 11, 3,
  124, 69, 11, 4,
  110, 69, 11, 6
};

void GUI_DrawNetwork(GtkWidget *data, int rflevel) {

  int i;

  for (i=1; i<=rflevel; i++)
    gdk_draw_rectangle(Pixmap,
		       GTK_WIDGET(data)->style->white_gc,
		       TRUE,
		       network_levels[4*(i-1)], network_levels[4*(i-1)+1],
		       network_levels[4*(i-1)+2], network_levels[4*(i-1)+3]);
}

int battery_levels[] = {
  50, 69, 11, 3,
  64, 69, 11, 3,
  78, 69, 11, 4,
  92, 69, 11, 6
};

void GUI_DrawBattery(GtkWidget *data, int batterylevel) {

  int i;

  if (batterylevel>=0) {
    for (i=1; i<=batterylevel; i++)
      gdk_draw_rectangle(Pixmap,
			 GTK_WIDGET(data)->style->white_gc,
			 TRUE,
			 battery_levels[4*(i-1)], battery_levels[4*(i-1)+1],
			 battery_levels[4*(i-1)+2], battery_levels[4*(i-1)+3]);

  }
}

void GUI_DrawSMS(GtkWidget *data) {

    gdk_draw_pixmap(Pixmap,
		    GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		    SMSPixmap,
		    0, 0,
		    25, 47,
		    26, 7);
}

void GUI_DrawAlarm(GtkWidget *data) {

    gdk_draw_pixmap(Pixmap,
		    GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		    AlarmPixmap,
		    0, 0,
		    163, 11,
		    9, 9);
}

void GUI_DrawSMSReceived(GtkWidget *data) {

  static GdkFont *Font;

  Font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-100-*-*-*-*-*-*");
  gdk_draw_string(Pixmap,
		  Font,
		  GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		  33, 25, "Short Message received");
}

gint GUI_Update(gpointer data) {

  static int initialized=0;

  /* Define required unit types for RF and Battery level meters. */
  GSM_RFUnits rf_units = GRF_Arbitrary;
  GSM_BatteryUnits batt_units = GBU_Arbitrary;

  /* The number of SMS messages before second */
  static int smsold=0;

  /* The number of second for we should display "Short Message Received" message */
  static int smsreceived=-1;

#ifdef XDEBUG
  g_print("GUI_Update enter.\n");
#endif

  if (!initialized && (fbusinit(true)==true))
    initialized++;

  GUI_DrawBackground(data);

#ifdef XDEBUG
  g_print("Point 1. GSM_LinkOK: %d\n", *GSM_LinkOK);
#endif

  if (GSM->GetRFLevel(&rf_units, &rflevel) == GE_NONE)
    GUI_DrawNetwork(data, rflevel);


#ifdef XDEBUG
  g_print("Point 2.\n");
#endif

  if (GSM->GetPowerSource(&powersource) == GE_NONE && powersource == GPS_ACDC) {
    batterylevel=((int) batterylevel+1)%5;
  }
  else if (GSM->GetBatteryLevel(&batt_units, &batterylevel) != GE_NONE)
    batterylevel=-1;

  GUI_DrawBattery(data, batterylevel);

#ifdef XDEBUG
  g_print("Point 3.\n");
#endif

  if (GSM->GetAlarm(0, &Alarm) == GE_NONE && Alarm.AlarmEnabled!=0)
    GUI_DrawAlarm(data);

#ifdef XDEBUG
  g_print("Point 4.\n");
#endif

  if (GSM->GetSMSStatus(&SMSStatus) == GE_NONE && SMSStatus.UnRead>0) {
    GUI_DrawSMS(data);

    if (SMSStatus.UnRead > smsold && smsold != -1)
      smsreceived=10; /* The message "Short Message Received" is displayed for 10s */
    smsold=SMSStatus.UnRead;
  }

  if (smsreceived >= 0) {
    GUI_DrawSMSReceived(data);
    smsreceived--;
  }

  gtk_widget_draw(data,NULL);

#ifdef XDEBUG
  g_print("GUI_Update leave.\n");
#endif

  return TRUE;
}

/* Redraw the screen from the backing pixmap */

static gint GUI_ExposeEvent (GtkWidget *widget, GdkEventExpose *event)
{

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  Pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

static void GUI_ShowOptions()
{
  gtk_entry_set_text (GTK_ENTRY (configDialogData.port), xgnokiiConfig.port);
//  gtk_entry_select_region (GTK_ENTRY (configDialogData.port),
//                               0, GTK_ENTRY(configDialogData.port)->text_length);
  
  gtk_entry_set_text (GTK_ENTRY (configDialogData.model), xgnokiiConfig.model);
//  gtk_entry_select_region (GTK_ENTRY (configDialogData.model),
//                               0, GTK_ENTRY(configDialogData.model)->text_length);

  gtk_entry_set_text (GTK_ENTRY (configDialogData.init), xgnokiiConfig.initlength);
//  gtk_entry_select_region (GTK_ENTRY (configDialogData.init),
//                               0, GTK_ENTRY(configDialogData.init)->text_length);

  gtk_entry_set_text (GTK_ENTRY (configDialogData.bindir), xgnokiiConfig.bindir);
//  gtk_entry_select_region (GTK_ENTRY (configDialogData.bindir),
//                               0, GTK_ENTRY(configDialogData.bindir)->text_length);

  if (!strcmp(xgnokiiConfig.connection, "serial"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.serial), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.infrared), TRUE);
  
  gtk_widget_show(GUI_OptionsDialog);
}

void GUI_ShowAbout()
{
  gtk_widget_show(GUI_AboutDialog);
}

void GUI_HideAbout()
{
  gtk_widget_hide(GUI_AboutDialog);
}

void GUI_ShowMenu(GdkEventButton *event)
{
  GdkEventButton *bevent = (GdkEventButton *) event;
  gtk_menu_popup (GTK_MENU(GUI_Menu), NULL, NULL, NULL, NULL,
                          bevent->button, bevent->time);
}

static gint GUI_ButtonPressEvent (GtkWidget *widget, GdkEventButton *event)
{

  /* Left button */
  if (event->button == 1) {

    /* Close */
    if (event->x >= 206 && event->x <= 221 &&
	event->y >=  42 && event->y <= 55)
    {
      if (GUI_ContactsIsChanged())
        GUI_QuitSaveContacts();
      else
        gtk_main_quit();
    }
    else if ((event->x >= 180 && event->x <= 210 &&
             event->y >=  15 && event->y <= 30) || 
             (event->x >= 175 && event->x <= 195 &&
             event->y >=  30 && event->y <= 50))
    {
      GUI_ShowContacts();
    }
    else if (event->x >= 180 && event->x <= 205 &&
             event->y >=  60 && event->y <= 80)
    {
      GUI_ShowSMS();
    }
    else if ((event->x >= 225 && event->x <= 250 &&
             event->y >=  15 && event->y <= 35) || 
             (event->x >= 235 && event->x <= 250 &&
             event->y >=  35 && event->y <= 65) ||
             (event->x >= 210 && event->x <= 245 &&
             event->y >=  65 && event->y <= 80))
    {
      gtk_widget_show(GUI_CardWindow);
    }
    else if (event->x >= 245 && event->x <= 258 &&
             event->y >=  83 && event->y <= 93)
    {
      GUI_ShowOptions();
    }
  } /* Right button */
  else if (event->button == 3)
    GUI_ShowMenu(event); 

  // g_print ("event->x: %f\n", event->x);
  // g_print ("event->y: %f\n", event->y);

  return TRUE;
}

void GUI_Refresh() {
  while (gtk_events_pending())
    gtk_main_iteration();
}

void delete_event( GtkWidget *widget, GdkEvent *event, gpointer data )
{
  gtk_widget_hide(widget);
}

void optionsSaveCallback( GtkWidget *widget, gpointer data )
{
  gtk_widget_hide(GTK_WIDGET(data));
}

void cancelCallback( GtkWidget *widget, gpointer data )
{
  gtk_widget_hide(GTK_WIDGET(data));
}

GtkWidget *GUI_CreateMenu()
{
  GtkWidget *menu, *menu_items;
  
  menu = gtk_menu_new();
  
  menu_items = gtk_menu_item_new_with_label("Contacts");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
      GTK_SIGNAL_FUNC(GUI_ShowContacts), NULL);
  gtk_widget_show(menu_items);

  menu_items = gtk_menu_item_new_with_label("SMS");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
      GTK_SIGNAL_FUNC(GUI_ShowSMS), NULL);
  gtk_widget_show(menu_items);
/*  
  menu_items = gtk_menu_item_new_with_label("Bussiness Cards");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
      GTK_SIGNAL_FUNC(GUI_ShowCards), NULL);
  gtk_widget_show(menu_items);
*/  
  menu_items = gtk_menu_item_new_with_label("--------");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_widget_show(menu_items);
  
  menu_items = gtk_menu_item_new_with_label("Options");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
      GTK_SIGNAL_FUNC(GUI_ShowOptions), NULL);
  gtk_widget_show(menu_items);
  
  menu_items = gtk_menu_item_new_with_label("--------");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_widget_show(menu_items);
  
  menu_items = gtk_menu_item_new_with_label("About");
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
      GTK_SIGNAL_FUNC(GUI_ShowAbout), NULL);
  gtk_widget_show(menu_items);
  
  return menu;
}

GtkWidget *GUI_CreateAboutDialog()
{
  GtkWidget *dialog;
  GtkWidget *button, *hbox, *label;
  gchar buf[200];
  
  dialog = gtk_dialog_new();
  gtk_window_set_title (GTK_WINDOW (dialog), "About");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (delete_event), NULL);
  button = gtk_button_new_with_label ("Ok");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                      button, TRUE, FALSE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (GUI_HideAbout), NULL);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  g_snprintf(buf, 200, "xgnokii version: %s\ngnokii version: %s\n
Copyright (C) 1999 Pavel Janík ml.,\nHugh Blemings & Jan Derfinak\n", XVERSION, VERSION);
  label = gtk_label_new ((gchar *) buf);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  return dialog;
}

GtkWidget *GUI_CreateOptionsDialog()
{
  GtkWidget *dialog;
  GtkWidget *button, *hbox, *label;
  
  dialog = gtk_dialog_new();
  gtk_window_set_title (GTK_WINDOW (dialog), "Options");
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (delete_event), NULL);
  button = gtk_button_new_with_label ("Save");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                      button, TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (optionsSaveCallback), (gpointer)dialog);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                      button, TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (cancelCallback), (gpointer)dialog);
  gtk_widget_show (button);

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 5);
   
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new ("Port:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.port = gtk_entry_new_with_max_length(10);

  gtk_box_pack_end(GTK_BOX(hbox), configDialogData.port, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.port);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new ("Model:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.model = gtk_entry_new_with_max_length(5);
  gtk_box_pack_end(GTK_BOX(hbox), configDialogData.model, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.model);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new ("Init length:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.init = gtk_entry_new_with_max_length(100);
  gtk_box_pack_end(GTK_BOX(hbox), configDialogData.init, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.init);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new ("Bindir:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.bindir = gtk_entry_new_with_max_length(100);
  gtk_box_pack_end(GTK_BOX(hbox), configDialogData.bindir, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.bindir);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new ("Connection:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  configDialogData.infrared = gtk_radio_button_new_with_label (NULL, "infrared");
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.infrared, TRUE, FALSE, 2);
  gtk_widget_show (configDialogData.infrared);
  configDialogData.serial = gtk_radio_button_new_with_label( 
            gtk_radio_button_group (GTK_RADIO_BUTTON (configDialogData.infrared)), "serial");
  gtk_box_pack_end(GTK_BOX(hbox), configDialogData.serial, TRUE, FALSE, 2);
  gtk_widget_show (configDialogData.serial);

  
  return dialog;
}

GtkWidget *GUI_CreateCardWindow()
{
  GtkWidget *window;
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Busines Cards");
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC (delete_event), NULL);
  
  return window;
}
  
void GUI_TopLevelWindow() {

  GtkWidget *drawing_area;
  GdkBitmap *mask;
  GtkStyle *style;
  GdkGC *gc;

  GUI_MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (GUI_MainWindow);

  BackgroundPixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_background);

  SMSPixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_sms);

  AlarmPixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_alarm);

  Pixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_background);

  style = gtk_widget_get_default_style();
  gc = style->black_gc;

  /* Create the drawing area */

  drawing_area = gtk_drawing_area_new ();

  /* Signals used to handle backing pixmap */

  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      (GtkSignalFunc) GUI_ExposeEvent, NULL);

  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
		      (GtkSignalFunc) GUI_ButtonPressEvent, NULL);

  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_BUTTON_PRESS_MASK);

  gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), 261, 96);
  gtk_container_add(GTK_CONTAINER(GUI_MainWindow), drawing_area);

  gdk_draw_pixmap(drawing_area->window,
		  drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
		  Pixmap,
		  0, 0,
		  0, 0,
		  261, 96);

  gtk_widget_shape_combine_mask(GUI_MainWindow, mask, 0, 0);
  
  gtk_signal_connect (GTK_OBJECT (GUI_MainWindow), "destroy",
                      GTK_SIGNAL_FUNC(gtk_main_quit),
                      NULL);
                      
  GUI_Menu = GUI_CreateMenu();
  GUI_OptionsDialog = GUI_CreateOptionsDialog();
  GUI_AboutDialog = GUI_CreateAboutDialog();
  GUI_CreateSMSWindow();
  GUI_CreateContactsWindow();
  GUI_CardWindow = GUI_CreateCardWindow();
  
  gtk_widget_show_all(GUI_MainWindow);
  GUI_Refresh();

  gtk_timeout_add(1800, (GtkFunction) GUI_Update, GUI_MainWindow);
}

void GUI_SplashScreen() {

  GtkWidget *pixmap, *fixed;
  GdkPixmap *gdk_pixmap;
  GdkBitmap *mask;
  GtkStyle *style;
  GdkGC *gc;

  GUI_SplashWindow = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_realize (GUI_SplashWindow);

  gtk_widget_set_usize (GUI_SplashWindow, 475, 160);
  gtk_window_position (GTK_WINDOW (GUI_SplashWindow), GTK_WIN_POS_CENTER);

  style = gtk_widget_get_default_style();
  gc = style->black_gc;
  gdk_pixmap = gdk_pixmap_create_from_xpm_d(GUI_SplashWindow->window, &mask,
					    &style->bg[GTK_STATE_NORMAL],
					    XPM_logo);
  pixmap = gtk_pixmap_new(gdk_pixmap, mask);

  fixed = gtk_fixed_new();
  gtk_widget_set_usize(fixed, 261, 96);
  gtk_fixed_put(GTK_FIXED(fixed), pixmap, 0, 0);
  gtk_container_add(GTK_CONTAINER(GUI_SplashWindow), fixed);

  gtk_widget_shape_combine_mask(GUI_SplashWindow, mask, 0, 0);

  gtk_widget_show_all (GUI_SplashWindow);
}

int GUI_RemoveSplash(GtkWidget *Win) {

  static int was_here=0;

  if (was_here==0) {
    was_here=1;
    gtk_widget_hide(GUI_SplashWindow);

    GUI_TopLevelWindow();

    return TRUE;
  }

  return FALSE;
}

void GUI_ReadConfig(void)
{

  struct CFG_Header *cfg_info;
  char *homedir;
  char rcfile[200];

  homedir = getenv("HOME");

  strncpy(rcfile, homedir, 200);
  strncat(rcfile, "/.gnokiirc", 200);

  if ((cfg_info = CFG_ReadFile("/etc/gnokiirc")) == NULL)
    if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
      fprintf(stderr, "error opening %s, using default config\n", rcfile);

  xgnokiiConfig.model = CFG_Get(cfg_info, "global", "model");
  if (xgnokiiConfig.model == NULL)
    xgnokiiConfig.model = DefaultModel;
  
  xgnokiiConfig.port = CFG_Get(cfg_info, "global", "port");
  if (xgnokiiConfig.port == NULL)
    xgnokiiConfig.port = DefaultPort;
  
  xgnokiiConfig.initlength = CFG_Get(cfg_info, "global", "initlength");
  if (xgnokiiConfig.initlength == NULL) {
       xgnokiiConfig.initlength = "default";
  }
  
  xgnokiiConfig.connection = CFG_Get(cfg_info, "global", "connection");
  if (xgnokiiConfig.connection == NULL)
    xgnokiiConfig.connection = DefaultConnection;
  
  xgnokiiConfig.bindir = CFG_Get(cfg_info, "global", "bindir");
    if (xgnokiiConfig.bindir == NULL)
        xgnokiiConfig.bindir = DefaultBindir;
        
  

  if (strstr(FB38_Information.Models, xgnokiiConfig.model) != NULL)
  {
    xgnokiiConfig.callerGroupsSupported = FALSE;
    max_phonebook_name_length = 20;
    max_phonebook_number_length = 30;
    max_phonebook_sim_name_length = 10;
    max_phonebook_sim_number_length = 30;
  }
  else if (strstr(FB61_Information.Models, xgnokiiConfig.model) != NULL)
  {
    xgnokiiConfig.callerGroupsSupported = TRUE;
    /* FIX ME: Can I read this from phone */
    xgnokiiConfig.callerGroups[0] = g_strndup( "Familly", MAX_CALLER_GROUP_LENGTH); 
    xgnokiiConfig.callerGroups[1] = g_strndup( "VIP", MAX_CALLER_GROUP_LENGTH);
    xgnokiiConfig.callerGroups[2] = g_strndup( "Friends", MAX_CALLER_GROUP_LENGTH);
    xgnokiiConfig.callerGroups[3] = g_strndup( "Colleagues", MAX_CALLER_GROUP_LENGTH);
    xgnokiiConfig.callerGroups[4] = g_strndup( "Other", MAX_CALLER_GROUP_LENGTH);
    xgnokiiConfig.callerGroups[5] = g_strndup( "No group", MAX_CALLER_GROUP_LENGTH);
    max_phonebook_name_length = FB61_MAX_PHONEBOOK_NAME_LENGTH;
    max_phonebook_number_length = FB61_MAX_PHONEBOOK_NUMBER_LENGTH;
    max_phonebook_sim_name_length = 14;
    max_phonebook_sim_number_length = FB61_MAX_PHONEBOOK_NUMBER_LENGTH;
  }
  else
  {
    xgnokiiConfig.callerGroupsSupported = FALSE;
    max_phonebook_name_length = max_phonebook_sim_name_length = GSM_MAX_PHONEBOOK_NAME_LENGTH;
    max_phonebook_number_length = max_phonebook_sim_number_length = GSM_MAX_PHONEBOOK_NUMBER_LENGTH;
  }
}

int main (int argc, char *argv[])
{

  gtk_init (&argc, &argv);

  /* Show the splash screen. */

  GUI_SplashScreen();

  /* Remove it after a while. */
	
  GUI_ReadConfig();

  (void) gtk_timeout_add(3000, (GtkFunction)GUI_RemoveSplash, (gpointer) GUI_SplashWindow);
  
  gtk_main ();
          
  return(0);
}
