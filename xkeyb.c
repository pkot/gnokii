#include <stdio.h>   /* for printf */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "cfgreader.h"
#include "fbus-6110.h"

/* FIXME: where we have these files located? */

#define Background_6110 "/usr/local/lib/gnokii/6110.xpm"
#define Background_6150 "/usr/local/lib/gnokii/6150.xpm"

typedef struct {
  int top_left_x, top_left_y;
  int bottom_right_x, bottom_right_y;
  int code;
} ButtonT;

ButtonT *button;

ButtonT button_6110[50] = {
  { 103,  91, 114, 107, 0x0d }, /* Power */
  {  28, 240,  54, 263, 0x19 }, /* Menu */
  {  84, 240, 110, 263, 0x1a }, /* Names */
  {  58, 245,  82, 258, 0x17 }, /* Up */
  {  55, 263,  85, 276, 0x18 }, /* Down */
  {  22, 271,  50, 289, 0x0e }, /* Green */
  {  91, 271, 115, 289, 0x0f }, /* Red */
  {  18, 294,  44, 310, 0x01 }, /* 1 */
  {  56, 294,  85, 310, 0x02 }, /* 2 */
  {  98, 294, 121, 310, 0x03 }, /* 3 */
  {  18, 317,  44, 333, 0x04 }, /* 4 */
  {  56, 317,  85, 333, 0x05 }, /* 5 */
  {  98, 317, 121, 333, 0x06 }, /* 6 */
  {  18, 342,  44, 356, 0x07 }, /* 7 */
  {  56, 342,  85, 356, 0x08 }, /* 8 */
  {  98, 342, 121, 356, 0x09 }, /* 9 */
  {  18, 365,  44, 380, 0x0c }, /* * */
  {  56, 365,  85, 380, 0x0a }, /* 0 */
  {  98, 365, 121, 380, 0x0b }, /* # */
  {   1, 138,  10, 150, 0x10 } , /* Volume + */
  {   1, 165,  10, 176, 0x11 } , /* Volume - */
  {   0,   0,   0,   0, 0x00 }
};

ButtonT button_6150[50] = {
  {  99,  78, 114,  93, 0x0d }, /* Power */
  {  20, 224,  49, 245, 0x19 }, /* Menu */
  {  90, 245, 120, 223, 0x1a }, /* Names */
  {  59, 230,  83, 247, 0x17 }, /* Up */
  {  56, 249,  84, 265, 0x18 }, /* Down */
  {  14, 254,  51, 273, 0x0e }, /* Green */
  {  90, 255, 126, 273, 0x0f }, /* Red */
  {  18, 281,  53, 299, 0x01 }, /* 1 */
  {  55, 280,  86, 299, 0x02 }, /* 2 */
  {  90, 281, 122, 299, 0x03 }, /* 3 */
  {  18, 303,  53, 323, 0x04 }, /* 4 */
  {  55, 303,  87, 323, 0x05 }, /* 5 */
  {  90, 303, 122, 323, 0x06 }, /* 6 */
  {  18, 327,  53, 346, 0x07 }, /* 7 */
  {  53, 327,  87, 346, 0x08 }, /* 8 */
  {  90, 327, 122, 346, 0x09 }, /* 9 */
  {  18, 349,  53, 370, 0x0c }, /* * */
  {  56, 349,  87, 370, 0x0a }, /* 0 */
  {  98, 349, 122, 370, 0x0b }, /* # */
  {   2, 131,  10, 147, 0x10 }, /* Volume + */
  {   2, 155,  10, 173, 0x11 }, /* Volume - */
  {   0,   0,   0,   0, 0x00 }
};

char *Model;      /* Model from .gnokiirc file. */
char *Port;       /* Serial port from .gnokiirc file */
char *Initlength; /* Init length from .gnokiirc file */
char *Connection; /* Connection type from .gnokiirc file */

/* Local variables */
char *DefaultModel      = MODEL; /* From Makefile */
char *DefaultPort       = PORT;
char *DefaultConnection = "serial";

GdkBitmap *mask;

static GdkPixmap *Pixmap = NULL;

void readconfig(void)
{

  struct CFG_Header *cfg_info;
  char *homedir;
  char rcfile[200];

#ifdef WIN32
  homedir = getenv("HOMEDRIVE");
  strncpy(rcfile, homedir, 200);
  homedir = getenv("HOMEPATH");
  strncat(rcfile, homedir, 200);
  strncat(rcfile, "\\_gnokiirc", 200);
#else
  homedir = getenv("HOME");

  strncpy(rcfile, homedir, 200);
  strncat(rcfile, "/.gnokiirc", 200);
#endif

  if ( (cfg_info = CFG_ReadFile("/etc/gnokiirc")) == NULL )
    if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
      fprintf(stderr, _("error opening %s, using default config\n"), rcfile);

  Model = CFG_Get(cfg_info, "global", "model");
  if (!Model)
    Model = DefaultModel;

  Port = CFG_Get(cfg_info, "global", "port");
  if (!Port)
    Port = DefaultPort;

  Initlength = CFG_Get(cfg_info, "global", "initlength");
  if (!Initlength)
    Initlength = "default";

  Connection = CFG_Get(cfg_info, "global", "connection");
  if (!Connection)
    Connection = DefaultConnection;
}

static gint button_event (GtkWidget *widget, GdkEventButton *event)
{

  int tmp=0;
  unsigned char req[] = { FB61_FRAME_HEADER, 0x42, 0x01, 0x00, 0x01 };

  if (event->button != 1)
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS) 
    req[4] = 0x01;
  else if (event->type == GDK_BUTTON_RELEASE)
    req[4] = 0x02;
  else
    return TRUE;

  while (button[tmp].top_left_x != 0) {
    if (button[tmp].top_left_x <= event->x &&
	event->x <= button[tmp].bottom_right_x &&
	button[tmp].top_left_y <= event->y &&
	event->y <= button[tmp].bottom_right_y) {

      req[5]=button[tmp].code;
      FB61_TX_SendMessage(0x07, 0x0c, req);

    }

    tmp++;
  }

  return TRUE;
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

void fbusinit(void)
{
  int count=0;
  GSM_Error error;
  GSM_ConnectionType connection=GCT_Serial;

  if (!strcmp(Connection, "infrared"))
    connection=GCT_Infrared;
    
  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise(Model, Port, Initlength, connection, NULL);

  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    exit(-1);
  }

  /* First (and important!) wait for GSM link to be active. We allow 10
     seconds... */

  while (count++ < 200 && *GSM_LinkOK == false)
    usleep(50000);

  if (*GSM_LinkOK == false) {
    fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting. \n"));
    exit(-1);
  }
}

/* Redraw the screen from the backing pixmap */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  Pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

void GUIRefresh() {
  while (gtk_events_pending())
    gtk_main_iteration();
}

void main (int argc, char *argv[])
{

  /* Meta widgets */

  GtkWidget *window, *vbox; /* Top level window */

  GtkWidget *drawing_area;    /* Working area */

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "Gnokii's xkeyb");

  gtk_container_set_border_width (GTK_CONTAINER (window), 1);
  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /* Create the drawing area */

  drawing_area = gtk_drawing_area_new ();

  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);

  gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), 138,420);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, FALSE, FALSE, 3);

  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
		      (GtkSignalFunc) button_event, NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_release_event",
		      (GtkSignalFunc) button_event, NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      (GtkSignalFunc) expose_event, NULL);

  readconfig();

  if (!strcmp(Model, "6150")) {
    Pixmap = gdk_pixmap_create_from_xpm(GTK_WIDGET(drawing_area)->window,&mask, &drawing_area->style->white, Background_6150);
    button=button_6150;
  } else {
    Pixmap = gdk_pixmap_create_from_xpm(GTK_WIDGET(drawing_area)->window,&mask, &drawing_area->style->white, Background_6110);
    button=button_6110;
  }

  fbusinit();

  gtk_widget_show_all (window);

  gtk_main ();
}
