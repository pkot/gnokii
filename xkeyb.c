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

#define Background_6110 "/usr/lib/gnokii/6110.xpm"
#define Background_6150 "/usr/lib/gnokii/6150.xpm"

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
  { 99,  83, 112, 97, 0x0d }, /* Power */
  {  26, 233,  44, 248, 0x19 }, /* Menu */
  {  94, 233, 110, 247, 0x1a }, /* Names */
  {  58, 236,  82, 253, 0x17 }, /* Up */
  {  54, 256,  83, 273, 0x18 }, /* Down */
  {  12, 261,  50, 281, 0x0e }, /* Green */
  {  90, 263, 125, 280, 0x0f }, /* Red */
  {  18, 290,  50, 305, 0x01 }, /* 1 */
  {  53, 290,  85, 305, 0x02 }, /* 2 */
  {  90, 290, 121, 305, 0x03 }, /* 3 */
  {  18, 313,  50, 330, 0x04 }, /* 4 */
  {  53, 313,  85, 330, 0x05 }, /* 5 */
  {  90, 313, 121, 330, 0x06 }, /* 6 */
  {  18, 335,  50, 353, 0x07 }, /* 7 */
  {  53, 335,  85, 353, 0x08 }, /* 8 */
  {  90, 335, 121, 353, 0x09 }, /* 9 */
  {  18, 365,  50, 377, 0x0c }, /* * */
  {  56, 365,  85, 377, 0x0a }, /* 0 */
  {  98, 365, 121, 377, 0x0b }, /* # */
  {   1, 138,  10, 150, 0x10 } , /* Volume + */
  {   1, 162,  10, 177, 0x11 } , /* Volume - */
  {   0,   0,   0,   0, 0x00 }
};


char		*Model;		/* Model from .gnokiirc file. */
char		*Port;		/* Serial port from .gnokiirc file */
char 		*Initlength;	/* Init length from .gnokiirc file */
char		*Connection;	/* Connection type from .gnokiirc file */

	/* Local variables */
char		*DefaultModel = MODEL;	/* From Makefile */
char		*DefaultPort = PORT;

char		*DefaultConnection = "serial";

void	readconfig(void)
{
    struct CFG_Header 	*cfg_info;
	char				*homedir;
	char				rcfile[200];

	homedir = getenv("HOME");

	strncpy(rcfile, homedir, 200);
	strncat(rcfile, "/.gnokiirc", 200);

    if ((cfg_info = CFG_ReadFile(rcfile)) == NULL) {
		fprintf(stderr, "error opening %s, using default config\n", 
		  rcfile);
    }

    Model = CFG_Get(cfg_info, "global", "model");
    if (Model == NULL) {
		Model = DefaultModel;
    }

    Port = CFG_Get(cfg_info, "global", "port");
    if (Port == NULL) {
		Port = DefaultPort;
    }

    Initlength = CFG_Get(cfg_info, "global", "initlength");
    if (Initlength == NULL) {
		Initlength = "default";
    }

    Connection = CFG_Get(cfg_info, "global", "connection");
    if (Connection == NULL) {
		Connection = DefaultConnection;
    }
}

static GdkPixmap *Pixmap = NULL;

/* Create a new backing pixmap of the appropriate size */
static gint configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  GdkBitmap *mask;

  if (!strcmp(Model, "6150")) {
    Pixmap = gdk_pixmap_create_from_xpm(widget->window,&mask, &widget->style->white, Background_6150);
    button=button_6150;
  } else {
    Pixmap = gdk_pixmap_create_from_xpm(widget->window,&mask, &widget->style->white, Background_6110);
    button=button_6110;
  }
  return TRUE;
}

static gint
button_press_event (GtkWidget *widget, GdkEventButton *event)
{

  int tmp=0;
  unsigned char req[] = {0x00, 0x01, 0x00, 0x42, 0x01, 0x00, 0x01};
  unsigned char req1[] = {0x00, 0x01, 0x00, 0x42, 0x02, 0x00, 0x01};

  if (event->button != 1)
    return 0;

  while (button[tmp].top_left_x != 0) {
    if (button[tmp].top_left_x <= event->x &&
	event->x <= button[tmp].bottom_right_x &&
	button[tmp].top_left_y <= event->y &&
	event->y <= button[tmp].bottom_right_y) {

      req[5]=button[tmp].code;
      req1[5]=button[tmp].code;
      FB61_TX_SendMessage(0x07, 0x0c, req);
      FB61_TX_SendMessage(0x07, 0x0c, req1);

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
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);

  gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), 138,420);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, FALSE, FALSE, 3);

  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
		      (GtkSignalFunc) button_press_event, NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      (GtkSignalFunc) expose_event, NULL);
  gtk_signal_connect (GTK_OBJECT(drawing_area),"configure_event",
		      (GtkSignalFunc) configure_event, NULL);


  readconfig();

  fbusinit();

  gtk_widget_show_all (window);

  gtk_main ();
}
