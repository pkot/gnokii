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

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "cfgreader.h"

#include "pixmaps/logo.xpm"
#include "pixmaps/background.xpm"
#include "pixmaps/sms.xpm"
#include "pixmaps/alarm.xpm"

GtkWidget *GUI_SplashWindow;
GtkWidget *GUI_MainWindow;

/* Pixmap used for drawing all the stuff. */
static GdkPixmap *Pixmap = NULL;

/* Pixmap used for background. */
static GdkPixmap *BackgroundPixmap = NULL;

/* Pixmap used for SMS picture. */
static GdkPixmap *SMSPixmap = NULL;

/* Pixmap used for alarm picture. */
static GdkPixmap *AlarmPixmap = NULL;

char *Model;      /* Model from .gnokiirc file. */
char *Port;       /* Serial port from .gnokiirc file */
char *Connection; /* Connection type from .gnokiirc file */

/* Local variables */
char *DefaultModel = MODEL; /* From Makefile */
char *DefaultPort = PORT;

char *DefaultConnection = "serial";

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

  if (!strcmp(Connection, "infrared"))
    connection=GCT_Infrared;
    
  /* Initialise the code for the GSM interface. */     

  if (error==GE_NOLINK)
    error = GSM_Initialise(Model, Port, connection, enable_monitoring);

  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    /* FIXME: should popup some message... */
    exit(-1);
  }

  while (count++ < 40 && *GSM_LinkOK == false)
    usleep(50000);

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

  if (!initialized && (fbusinit(true)==GE_NONE))
    initialized++;

  GUI_DrawBackground(data);

  if (GSM->GetRFLevel(&rf_units, &rflevel) == GE_NONE)
    GUI_DrawNetwork(data, rflevel);

  if (GSM->GetPowerSource(&powersource) == GE_NONE && powersource == GPS_ACDC) {
    batterylevel=((int) batterylevel+1)%5;
  }
  else if (GSM->GetBatteryLevel(&batt_units, &batterylevel) != GE_NONE)
    batterylevel=-1;

  GUI_DrawBattery(data, batterylevel);

  if (GSM->GetAlarm(0, &Alarm) == GE_NONE && Alarm.AlarmEnabled!=0)
    GUI_DrawAlarm(data);

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

static gint GUI_ButtonPressEvent (GtkWidget *widget, GdkEventButton *event)
{

  /* Left button */
  if (event->button == 1) {

    /* Close */
    if (event->x >= 206 && event->x <= 221 &&
	event->y >=  42 && event->y <= 55)
      exit(0);

  }

  //  printf ("event->x: %f\n", event->x);
  //  printf ("event->y: %f\n", event->y);

  return TRUE;
}

void GUI_Refresh() {
  while (gtk_events_pending())
    gtk_main_iteration();
}

void GUI_TopLevelWindow() {

  GtkWidget *pixmap, *drawing_area;
  GdkBitmap *mask;
  GtkStyle *style;
  GdkGC *gc;

  // GUI_MainWindow = gtk_window_new(GTK_WINDOW_POPUP);
  GUI_MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (GUI_MainWindow);

  BackgroundPixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_background);

  SMSPixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_sms);

  AlarmPixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_alarm);

  Pixmap = gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_background);

  style = gtk_widget_get_default_style();
  gc = style->black_gc;
  pixmap = gtk_pixmap_new(Pixmap, mask);

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
    
  gtk_widget_show_all(GUI_MainWindow);
  GUI_Refresh();

  gtk_timeout_add(1200, (GtkFunction) GUI_Update, GUI_MainWindow);
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

  if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
    fprintf(stderr, "error opening %s, using default config\n", 
	    rcfile);

  Model = CFG_Get(cfg_info, "global", "model");
  if (Model == NULL)
    Model = DefaultModel;

  Port = CFG_Get(cfg_info, "global", "port");
  if (Port == NULL)
    Port = DefaultPort;

  Connection = CFG_Get(cfg_info, "global", "connection");
  if (Connection == NULL)
    Connection = DefaultConnection;
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
