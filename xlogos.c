#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>

#ifndef WIN32

  #include <unistd.h>  /* for usleep */
  #include <termios.h>
  #include <sys/time.h>

#else

  #include <windows.h>
  #include "win32/winserial.h"

  #undef IN
  #undef OUT

  #define WRITEPHONE(a, b, c) WriteCommBlock(b, c)
  #define sleep(x) Sleep((x) * 1000)
  #define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))

#endif

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "cfgreader.h"
#include "gsm-networks.h"
#include "gsm-filetypes.h"

extern GSM_Network GSM_Networks[];

#include "pixmaps/New.xpm"
#include "pixmaps/Invert.xpm"
#include "pixmaps/Flip.xpm"
#include "pixmaps/Open.xpm"
#include "pixmaps/Save.xpm"
#include "pixmaps/Send.xpm"

#include <gtk/gtk.h>

/* Backing pixmap for drawing area */
static GdkPixmap *pixmap = NULL;

GtkWidget *FileSelection;
GtkWidget *drawing_area;
GtkWidget *Combo;
GtkWidget *Combo2;

unsigned char *logo;
char groupnames[6][255];

char *Model;      /* Model from .gnokiirc file. */
char *Port;       /* Serial port from .gnokiirc file */
char *Initlength; /* Init length from .gnokiirc file */
char *Connection; /* Connection type from .gnokiirc file */

void mycallback()
{
  printf("Not implemented yet, sorry!\n");
}

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

  if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
    fprintf(stderr, "error opening %s, using default config\n", rcfile);

  Initlength = CFG_Get(cfg_info, "global", "initlength");
  if (Initlength == NULL)
    Initlength = "default";

  Model = CFG_Get(cfg_info, "global", "model");
  Port = CFG_Get(cfg_info, "global", "port");
  Connection = CFG_Get(cfg_info, "global", "connection");
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

void fbusinit(bool enable_monitoring)
{
  int count=0;
  GSM_Error error;
  GSM_ConnectionType connection=GCT_Serial;

    if (!strcmp(Connection, "infrared"))
		connection = GCT_Infrared;
    
  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise(Model, Port, Initlength, connection,
                         enable_monitoring,NULL);

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

void newlogo();


/* Create a new backing pixmap of the appropriate size */
static gint
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{

  logo=(unsigned char *)calloc(128,1);

  if (pixmap)
    gdk_pixmap_unref(pixmap);

  pixmap = gdk_pixmap_new(widget->window,
			  widget->allocation.width,
			  widget->allocation.height,
			  -1);
  gdk_draw_rectangle (pixmap,
                      widget->style->white_gc,
                      TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

  newlogo();

  return TRUE;
}

/* Redraw the screen from the backing pixmap */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

void clear_point(GtkWidget *widget, int x, int y){

  GdkRectangle update_rect;

  update_rect.width = 4;
  update_rect.height = 4;
  update_rect.x = x * 5;
  update_rect.y = y * 5;

  logo[9*y + (x/8)] &= 255 - (1<<(7-(x%8)));
    
  gdk_draw_rectangle (pixmap,
  	widget->style->white_gc,
	TRUE,
	update_rect.x, update_rect.y,
	update_rect.width, update_rect.height);
  gtk_widget_draw (widget, &update_rect);
  
}

void set_point(GtkWidget *widget, int x, int y){

  GdkRectangle update_rect;

  update_rect.width = 4;
  update_rect.height = 4;
  update_rect.x = x * 5;
  update_rect.y = y * 5;

  logo[9*y + (x/8)]|=1<<(7-(x%8));
  gdk_draw_rectangle (pixmap,
			widget->style->black_gc,
			TRUE,
			update_rect.x, update_rect.y,
			update_rect.width, update_rect.height);
  gtk_widget_draw (widget, &update_rect);
  
}

int is_point(int x, int y)
{
	return (logo[9*y + (x/8)] & 1<<(7-(x%8)));
}

void update_points(GtkWidget *widget)
{
  int i,j;
  
  for (i=0;i<74;i++) {
    for (j=0;j<14;j++) {
      if (is_point(i,j)) {
	set_point(widget,i,j);
      } else clear_point(widget,i,j);
    }
  }
}


/* Draw a rectangle on the screen */
static void
draw_brush (GtkWidget *widget, gdouble x, gdouble y, int button)
{
  int row = y/5, column = x/5;

  if (button > 1)
    clear_point(widget, column, row);
  else
    set_point(widget, column, row);
}

void newlogo()
{
	int column,row;

	for (column=0; column<72; column++)
	  for (row=0; row<14; row++)
	    clear_point(drawing_area, column, row);
}

void invertlogo()
{
	int column,row;

	for (column=0; column<72; column++)
	  for (row=0; row<14; row++)
            if (is_point(column, row))
	      clear_point(drawing_area, column, row);
	    else
	      set_point(drawing_area, column, row);
}

void downlogo()
{

  int column,row;

  for (row=13; row>0; row--)
    for (column=0; column<72; column++)
      if (is_point(column, row-1))
        set_point(drawing_area, column, row);
      else
        clear_point(drawing_area, column, row);

    for (column=0; column<72; column++)
    	clear_point(drawing_area, column, 0);
}

void uplogo(){

  int column,row;

  for (row=0; row<13; row++)
    for (column=0; column<72; column++)
      if (is_point(column, row+1))
        set_point(drawing_area, column, row);
      else
        clear_point(drawing_area, column, row);

    for (column=0; column<72; column++)
    	clear_point(drawing_area, column, 13);
}

void rightlogo(){

  int column,row;

  for (column=71; column>0; column--)
    for (row=0; row<14; row++)
      if (is_point(column-1, row))
        set_point(drawing_area, column, row);
      else
        clear_point(drawing_area, column, row);

    for (row=0; row<14; row++)
    	clear_point(drawing_area, 0, row);
}

void leftlogo(){

  int column,row;

  for (column=0; column<71; column++)
    for (row=0; row<14; row++)
      if (is_point(column+1, row))
        set_point(drawing_area, column, row);
      else
        clear_point(drawing_area, column, row);

    for (row=0; row<14; row++)
    	clear_point(drawing_area, 71, row);
}

void flipvertlogo()
{
  int row, column, temp;

  for(row=0; row<7; row++)
    for(column=0; column<72; column++)
    {
      temp = is_point(column, row);

      if (is_point(column,13-row))
        set_point(drawing_area, column,row);
      else
        clear_point(drawing_area, column,row);

      if (temp)
        set_point(drawing_area, column,13-row);
      else
        clear_point(drawing_area, column,13-row);
    }
}

void fliphorizlogo()
{
  int row, column, temp;

  for(row=0; row<14; row++)
    for(column=0; column<36; column++)
    {
      temp = is_point(column, row);

      if (is_point(71-column,row))
        set_point(drawing_area, column,row);
      else
        clear_point(drawing_area, column,row);

      if (temp)
        set_point(drawing_area, 71-column,row);
      else
        clear_point(drawing_area, 71-column, row);
    }
}

void ExportFileSelected(GtkWidget *w, GtkFileSelection *fs)
{

  GSM_Bitmap bitmap;
  char *File=(char *)
    malloc(strlen(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs))+1));
  char *operator = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Combo)->entry));


  strcpy(File, gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs)));

  gtk_widget_destroy(FileSelection);

 
  if (strcmp(operator, "Group Graphics Logo")) {
    strncpy(bitmap.netcode,GSM_GetNetworkCode(operator),7);
    bitmap.type=GSM_OperatorLogo;
  }
  else { /* Group Graphics file */
    bitmap.type=GSM_CallerLogo;
  }
  bitmap.width=72;
  bitmap.height=14;
  bitmap.size=14*72/8;
  memcpy(bitmap.bitmap,logo,bitmap.size);
  GSM_SaveBitmapFile(File,&bitmap);
}

void ImportFileSelected(GtkWidget *w, GtkFileSelection *fs)
{
  char *File=(char *)
    malloc(strlen(gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs))+1));
 
  GSM_Bitmap bitmap;

  strcpy(File, gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs)));

  gtk_widget_destroy(FileSelection);

  GSM_ReadBitmapFile(File,&bitmap);

  if (bitmap.type==GSM_OperatorLogo) {
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Combo)->entry), GSM_GetNetworkName(bitmap.netcode));
  }
  memcpy(logo,bitmap.bitmap,bitmap.size);  
  update_points(drawing_area);
}


void savelogo()
{
  FileSelection=gtk_file_selection_new ("Save logo...");

  gtk_signal_connect (
	GTK_OBJECT (GTK_FILE_SELECTION (FileSelection)->ok_button),
	"clicked", (GtkSignalFunc) ExportFileSelected, FileSelection);
    
  gtk_signal_connect_object (
	GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->cancel_button),
	"clicked", (GtkSignalFunc) gtk_widget_destroy,
        GTK_OBJECT (FileSelection));
    
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(FileSelection));

  gtk_widget_show(FileSelection);
}

void openlogo()
{
  FileSelection=gtk_file_selection_new ("Open logo...");

  gtk_signal_connect (
	GTK_OBJECT (GTK_FILE_SELECTION (FileSelection)->ok_button),
	"clicked", (GtkSignalFunc) ImportFileSelected, FileSelection);
    
  gtk_signal_connect_object (
	GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->cancel_button),
	"clicked", (GtkSignalFunc) gtk_widget_destroy,
        GTK_OBJECT (FileSelection));
    
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(FileSelection));

  gtk_widget_show(FileSelection);
}

static gint
button_press_event (GtkWidget *widget, GdkEventButton *event)
{

  if (pixmap != NULL) {
    draw_brush (widget, event->x, event->y, event->button);
  }

  return TRUE;
}

static gint
motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
  int x, y;
  GdkModifierType state;

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &x, &y, &state);
  else
    {
      x = event->x;
      y = event->y;
      state = event->state;
    }
    
  if (state & GDK_BUTTON1_MASK && pixmap != NULL)
    draw_brush (widget, x, y, 1);
  if (state & GDK_BUTTON2_MASK && pixmap != NULL)
    draw_brush (widget, x, y, 2);
  
  return TRUE;
}

void
quit ()
{
  gtk_exit (0);
}

void show_logo()
{
  GSM_Bitmap bitmap;
  char *operator = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Combo)->entry));
  int i;

  readconfig();
  fbusinit(false);

  bitmap.height=14;
  bitmap.width=72;
  bitmap.size=72*14/8;
  if (strcmp(operator, "Group Graphics Logo")) {
    strncpy(bitmap.netcode,GSM_GetNetworkCode(operator),7);
    bitmap.type=GSM_OperatorLogo;
  } else {
    bitmap.type=GSM_CallerLogo;
    /* Is there a better way to do this? */
    for (i=0;i<6;i++) {
      if (strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Combo2)->entry)),groupnames[i])==0) bitmap.number=i;
    }    
    if (GSM->GetBitmap(&bitmap)!=GE_NONE) return; /* Get the old name */
  }
  memcpy(bitmap.bitmap,logo,bitmap.size);
  GSM->SetBitmap(&bitmap);
}

void get_logo()
{
  GSM_Bitmap bitmap;
  char *operator = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Combo)->entry));
  int i;

  readconfig();
  fbusinit(false);

  if (strcmp(operator, "Group Graphics Logo")) {
    strncpy(bitmap.netcode,GSM_GetNetworkCode(operator),7);
    bitmap.type=GSM_OperatorLogo;
  } else {
    bitmap.type=GSM_CallerLogo;
    /* Is there a better way to do this? */
    for (i=0;i<6;i++) {
      if (strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Combo2)->entry)),groupnames[i])==0) bitmap.number=i;
    }    
  }
  if (GSM->GetBitmap(&bitmap)==GE_NONE){
    memcpy(logo,bitmap.bitmap,bitmap.size);
    update_points(drawing_area);
  }
}


void get_operator()
{

  GSM_NetworkInfo NetworkInfo;

  readconfig();
  fbusinit(true);

  if (GSM->GetNetworkInfo(&NetworkInfo) == GE_NONE)
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Combo)->entry), GSM_GetNetworkName(NetworkInfo.NetworkCode));

}


static GtkWidget*
new_pixmap (gchar      **data,
	    GdkWindow *window,
	    GdkColor  *background)
{
  GtkWidget *wpixmap;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  pixmap = gdk_pixmap_create_from_xpm_d (window, &mask,
				       background,
				       data);
  wpixmap = gtk_pixmap_new (pixmap, mask);

  return wpixmap;
}

int
main (int argc, char *argv[])
{
  GSM_Bitmap bitmap;

  GtkWidget *window;
  GtkWidget *toolbar;
  GtkWidget *vbox;
  GtkWidget *menubar;
  
  /* File menu */
    
  GtkWidget *FileMenu, *FileMenuItem;
  GtkWidget *FileMenu_Open;
  GtkWidget *FileMenu_Save;
  GtkWidget *FileMenu_GetOperator;
  GtkWidget *FileMenu_Send;
  GtkWidget *FileMenu_Get;
  GtkWidget *FileMenu_Exit;

  GtkWidget *Menu_Empty;

  GtkWidget *EditMenu, *EditMenuItem;
  GtkWidget *EditMenu_Clear;
  GtkWidget *EditMenu_Invert;
  GtkWidget *EditMenu_FlipHoriz;
  GtkWidget *EditMenu_FlipVert;

  GtkWidget *EditMenu_Up;
  GtkWidget *EditMenu_Down;
  GtkWidget *EditMenu_Right;
  GtkWidget *EditMenu_Left;
  
  GtkWidget *separator;

  GtkAccelGroup *accel_group=gtk_accel_group_get_default();

  GList *glist = NULL;
  GList *glist2 = NULL;
  int i=0;

  gtk_init (&argc, &argv);

  separator=gtk_hseparator_new();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW(window), "Gnokii - Xlogos");
//  gtk_widget_realize(GTK_WIDGET(window));

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (quit), NULL);

  Combo = gtk_combo_new();
  gtk_combo_set_use_arrows_always(GTK_COMBO(Combo),1);

  while (strcmp(GSM_Networks[i].Name, "unknown"))
    glist=g_list_insert_sorted(glist, GSM_Networks[i++].Name, (GCompareFunc) strcmp);

  gtk_combo_set_popdown_strings(GTK_COMBO(Combo), glist);

  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Combo)->entry), "Group Graphics Logo");

  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(Combo)->entry), FALSE);

  /* Caller Group List */

  readconfig();
  fbusinit(false);

  Combo2 = gtk_combo_new();
  gtk_combo_set_use_arrows_always(GTK_COMBO(Combo2),1);
  bitmap.type=GSM_CallerLogo;
  
  /* Default group names */
  strcpy(groupnames[0],"Family");
  strcpy(groupnames[1],"VIP");
  strcpy(groupnames[2],"Friends");
  strcpy(groupnames[3],"Colleagues");
  strcpy(groupnames[4],"Other");

  /* Get actual group names */
  for (i=0;i<5;i++) {
     bitmap.number=i;
     GSM->GetBitmap(&bitmap);
     if (strlen(bitmap.text)>0) strncpy(groupnames[i],bitmap.text,255);
     glist2=g_list_insert(glist2,groupnames[i],i);
  }
  gtk_combo_set_popdown_strings(GTK_COMBO(Combo2), glist2);

  /* Now we create the menubar */
  
  menubar=gtk_menu_bar_new();
    
  /* File menu */
      
  FileMenu=gtk_menu_new();
  FileMenuItem=gtk_menu_item_new_with_label("File");
  FileMenu_Open=gtk_menu_item_new_with_label("Open");
  FileMenu_Save=gtk_menu_item_new_with_label("Save");
  FileMenu_GetOperator=gtk_menu_item_new_with_label("Get Operator");
  FileMenu_Send=gtk_menu_item_new_with_label("Send Logo");
  FileMenu_Get=gtk_menu_item_new_with_label("Get Logo");
  FileMenu_Exit=gtk_menu_item_new_with_label("Exit");

  Menu_Empty=gtk_menu_item_new();
  
  gtk_menu_append(GTK_MENU(FileMenu), FileMenu_Open);
  gtk_menu_append(GTK_MENU(FileMenu), FileMenu_Save);
  gtk_menu_append(GTK_MENU(FileMenu), FileMenu_GetOperator);
  gtk_menu_append(GTK_MENU(FileMenu), FileMenu_Send);
  gtk_menu_append(GTK_MENU(FileMenu), FileMenu_Get);
  gtk_menu_append(GTK_MENU(FileMenu), Menu_Empty);
  gtk_menu_append(GTK_MENU(FileMenu), FileMenu_Exit);

  gtk_signal_connect_object (GTK_OBJECT (FileMenu_Open), "activate",
  			GTK_SIGNAL_FUNC (openlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (FileMenu_Save), "activate",
  			GTK_SIGNAL_FUNC (savelogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (FileMenu_GetOperator), "activate",
  			GTK_SIGNAL_FUNC (get_operator), NULL);
  gtk_signal_connect_object (GTK_OBJECT (FileMenu_Send), "activate",
  			GTK_SIGNAL_FUNC (show_logo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (FileMenu_Get), "activate",
  			GTK_SIGNAL_FUNC (get_logo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (FileMenu_Exit), "activate",
  			GTK_SIGNAL_FUNC (quit), NULL);

  gtk_widget_add_accelerator (FileMenu_Open, "activate", accel_group,
     'O', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (FileMenu_Save, "activate", accel_group,
     'S', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (FileMenu_Send, "activate", accel_group,
     'T', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (FileMenu_Get, "activate", accel_group,
     'R', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (FileMenu_GetOperator, "activate", accel_group,
     'G', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
          
  gtk_widget_add_accelerator (FileMenu_Exit, "activate", accel_group,
     'X', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
     

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(FileMenuItem), FileMenu);
  gtk_menu_bar_append(GTK_MENU_BAR(menubar), FileMenuItem);

  /* Edit menu */
      
  EditMenu=gtk_menu_new();
  EditMenuItem=gtk_menu_item_new_with_label("Edit");
  EditMenu_Clear=gtk_menu_item_new_with_label("Clear");
  EditMenu_Invert=gtk_menu_item_new_with_label("Invert");
  EditMenu_FlipHoriz=gtk_menu_item_new_with_label("Flip horizontal");
  EditMenu_FlipVert=gtk_menu_item_new_with_label("Flip vertical");
  EditMenu_Up=gtk_menu_item_new_with_label("Up");
  EditMenu_Down=gtk_menu_item_new_with_label("Down");
  EditMenu_Right=gtk_menu_item_new_with_label("Right");
  EditMenu_Left=gtk_menu_item_new_with_label("Left");

  Menu_Empty=gtk_menu_item_new();
  
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_Clear);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_Invert);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_FlipHoriz);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_FlipVert);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_Up);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_Down);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_Right);
  gtk_menu_append(GTK_MENU(EditMenu), EditMenu_Left);

  gtk_signal_connect_object (GTK_OBJECT (EditMenu_Clear), "activate",
  			GTK_SIGNAL_FUNC (newlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_Invert), "activate",
  			GTK_SIGNAL_FUNC (invertlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_FlipHoriz), "activate",
  			GTK_SIGNAL_FUNC (fliphorizlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_FlipVert), "activate",
  			GTK_SIGNAL_FUNC (flipvertlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_Up), "activate",
  			GTK_SIGNAL_FUNC (uplogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_Down), "activate",
  			GTK_SIGNAL_FUNC (downlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_Right), "activate",
  			GTK_SIGNAL_FUNC (rightlogo), NULL);
  gtk_signal_connect_object (GTK_OBJECT (EditMenu_Left), "activate",
  			GTK_SIGNAL_FUNC (leftlogo), NULL);

  gtk_widget_add_accelerator (EditMenu_Clear, "activate", accel_group,
     'C', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (EditMenu_Invert, "activate", accel_group,
     'I', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (EditMenu_FlipHoriz, "activate", accel_group,
     'H', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (EditMenu_FlipVert, "activate", accel_group,
     'V', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (EditMenu_Up, "activate", accel_group,
     'U', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (EditMenu_Down, "activate", accel_group,
     'D', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (EditMenu_Right, "activate", accel_group,
     'R', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (EditMenu_Left, "activate", accel_group,
     'L', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(EditMenuItem), EditMenu);
  gtk_menu_bar_append(GTK_MENU_BAR(menubar), EditMenuItem);

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  /* Create the toolbar */

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NORMAL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, "New logo", NULL, new_pixmap (New_xpm, window->window, &window->style->bg[GTK_STATE_NORMAL]), (GtkSignalFunc) newlogo, toolbar);

  gtk_toolbar_append_space (GTK_TOOLBAR(toolbar));

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, "Open", NULL, new_pixmap (Open_xpm, window->window, &window->style->bg[GTK_STATE_NORMAL]), (GtkSignalFunc) openlogo, toolbar);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, "Save", NULL, new_pixmap (Save_xpm, window->window, &window->style->bg[GTK_STATE_NORMAL]), (GtkSignalFunc) savelogo, toolbar);

  gtk_toolbar_append_space (GTK_TOOLBAR(toolbar));

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, "Send logo", NULL, new_pixmap (Send_xpm, window->window, &window->style->bg[GTK_STATE_NORMAL]), (GtkSignalFunc) show_logo, toolbar);

  gtk_toolbar_append_space (GTK_TOOLBAR(toolbar));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, "Invert logo", NULL, new_pixmap (Invert_xpm, window->window, &window->style->bg[GTK_STATE_NORMAL]), (GtkSignalFunc) invertlogo, toolbar);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, "Flip Horizontal", NULL, new_pixmap (Flip_xpm, window->window, &window->style->bg[GTK_STATE_NORMAL]), (GtkSignalFunc) fliphorizlogo, toolbar);
  
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), Combo, "", "");

  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), Combo2, "", "");

  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(separator), FALSE, FALSE, 0);
  

  /* Create the drawing area */

  drawing_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), 360, 70);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

  /* Signals used to handle backing pixmap */

  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      (GtkSignalFunc) expose_event, NULL);
  gtk_signal_connect (GTK_OBJECT(drawing_area),"configure_event",
		      (GtkSignalFunc) configure_event, NULL);

  /* Event signals */

  gtk_signal_connect (GTK_OBJECT (drawing_area), "motion_notify_event",
		      (GtkSignalFunc) motion_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
		      (GtkSignalFunc) button_press_event, NULL);

  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
