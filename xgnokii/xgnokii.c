/*

  X G N O K I I
  
  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.
  
  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  Last modification: Mon Oct 10 1999
  Modified by Jan Derfinak
              
*/

#include <stdio.h>   /* for printf */
#include <stdlib.h>  /* for getenv */
#include <string.h>  /* for strtok */
#include <time.h>    /* for time   */

#ifndef WIN32
# include <unistd.h>  /* for usleep */
# include <signal.h>
#else
# include <windows.h>
# include "../win32/winserial.h"
# define WRITEPHONE(a, b, c) WriteCommBlock(b, c)
# undef IN
# undef OUT
# define sleep(x) Sleep((x) * 1000)
# define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "fbus-6110.h"
#include "fbus-3810.h"
#include "cfgreader.h"
#include "xgnokii.h"
#include "xgnokii_common.h"
#include "xgnokii_contacts.h"
#include "xgnokii_sms.h"
#include "xgnokii_netmon.h"
#include "xgnokii_dtmf.h"
#include "xgnokii_speed.h"
#include "xgnokii_xkeyb.h"
#include "xgnokii_cfg.h"

#include "xpm/logo.xpm"
#include "xpm/background.xpm"
#include "xpm/sms.xpm"
#include "xpm/alarm.xpm"


static GtkWidget *GUI_SplashWindow;
static GtkWidget *GUI_MainWindow;
static GtkWidget *GUI_AboutDialog;
static ErrorDialog errorDialog = {NULL, NULL};
/* SMS sets list */
static GtkWidget *SMSClist;

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
static bool optionsDialogIsOpened;

/* Widget for popup menu */
static GtkWidget *GUI_Menu;
static GtkWidget *netmon_menu_item;
static GtkWidget *xkeyb_menu_item;

static GtkWidget *cg_names_option_frame;

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
static char *DefaultXGnokiiDir = XGNOKIIDIR;
static bool SMSSettingsInitialized = FALSE;
static gint hiddenCallDialog;

static struct CallDialog {
  GtkWidget *dialog;
  GtkWidget *label;
} inCallDialog;

typedef struct {
  GtkWidget *alarmSwitch;
  GtkWidget *alarmHour;
  GtkWidget *alarmMin;
} AlarmWidgets;

typedef struct {
  GtkWidget *port;
  GtkWidget *model;
  GtkWidget *init;
  GtkWidget *bindir;
  GtkWidget *serial, *infrared;
} ConnectionWidgets;

typedef struct {
  GtkWidget *model;
  GtkWidget *version;
  GtkWidget *revision;
  GtkWidget *imei;
} PhoneWidgets;

typedef struct {
  GtkWidget *set;
  GtkWidget *number;
  GtkWidget *format;
  GtkWidget *validity;
  GSM_MessageCenter smsSetting[MAX_SMS_CENTER];
} SMSWidgets;

typedef struct {
  GtkWidget *name;
  GtkWidget *title;
  GtkWidget *company;
  GtkWidget *telephone;
  GtkWidget *fax;
  GtkWidget *email;
  GtkWidget *address;
  GtkWidget *status;
  gint   max;
  gint   used;
} UserWidget;

static struct ConfigDialogData
{
  ConnectionWidgets connection;
  PhoneWidgets phone;
  GtkWidget *groups[6];
  AlarmWidgets alarm;
  SMSWidgets sms;
  UserWidget user;
  GtkWidget *help;
} configDialogData;

static GSM_MessageCenter tempMessageSettings;

/* Variables which contain current signal and battery. */
static float rflevel=-1, batterylevel=-1;
static GSM_PowerSource powersource=-1;
static GSM_DateTime Alarm;
static GSM_SMSStatus SMSStatus = {0, 0}; 


static inline void Help1 (GtkWidget *w, gpointer data)
{
  Help (w, _("/help/index.html"));
}

void InitModelInf (void)
{
  gchar buf[64];
  GSM_Error error;
  register gint i = 0;
  
  while ((error = GSM->GetModel(buf)) != GE_NONE && i++ < 5)
    sleep(1);
  
  if (error != GE_NONE)
  {
    xgnokiiConfig.phoneModel = g_strdup (_("unknown"));
    xgnokiiConfig.phoneVer = xgnokiiConfig.phoneModel;
    xgnokiiConfig.callerGroupsSupported = FALSE;
    xgnokiiConfig.netMonitorSupported = FALSE;
  }
  else
  {
    xgnokiiConfig.phoneVer = g_strdup (buf);
    xgnokiiConfig.phoneModel = GetModel (buf);
    if (xgnokiiConfig.phoneModel == NULL)
      xgnokiiConfig.phoneModel = g_strdup (_("unknown"));

    xgnokiiConfig.callerGroupsSupported = CallerGroupSupported (buf);
    xgnokiiConfig.netMonitorSupported = NetmonitorSupported (buf);
  }
  
  i = 0;
  while ((error = GSM->GetRevision (buf)) != GE_NONE && i++ < 5)
    sleep(1);
  
  if (error != GE_NONE)
    xgnokiiConfig.phoneRevision = g_strdup (_("unknown"));
  else
    xgnokiiConfig.phoneRevision = g_strdup (buf);
    
  i = 0;
  while ((error = GSM->GetIMEI (buf)) != GE_NONE && i++ < 5)
    sleep(1);
  
  if (error != GE_NONE)
    xgnokiiConfig.phoneImei = g_strdup (_("unknown"));
  else
    xgnokiiConfig.phoneImei = g_strdup (buf);
    
  
#ifdef XDEBUG
  g_print ("Version: %s\n", xgnokiiConfig.phoneVer);
  g_print ("Model: %s\n", xgnokiiConfig.phoneModel);
  g_print ("Caller Groups: %d\n", xgnokiiConfig.callerGroupsSupported);
  g_print ("Netmonitor: %d\n", xgnokiiConfig.netMonitorSupported);
  g_print ("IMEI: %s\n", xgnokiiConfig.phoneImei);
  g_print ("Revision: %s\n", xgnokiiConfig.phoneRevision);
#endif
}

void GUI_InitCallerGroupsInf (void)
{
  GSM_Bitmap bitmap;
  register gint i;
  
  xgnokiiConfig.callerGroups[0] = g_strndup( _("Familly"), MAX_CALLER_GROUP_LENGTH);
  xgnokiiConfig.callerGroups[1] = g_strndup( _("VIP"), MAX_CALLER_GROUP_LENGTH);
  xgnokiiConfig.callerGroups[2] = g_strndup( _("Friends"), MAX_CALLER_GROUP_LENGTH);
  xgnokiiConfig.callerGroups[3] = g_strndup( _("Colleagues"), MAX_CALLER_GROUP_LENGTH);
  xgnokiiConfig.callerGroups[4] = g_strndup( _("Other"), MAX_CALLER_GROUP_LENGTH);
  xgnokiiConfig.callerGroups[5] = g_strndup( _("No group"), MAX_CALLER_GROUP_LENGTH);

  bitmap.type = GSM_CallerLogo;
  
  if (xgnokiiConfig.callerGroupsSupported)
    for (i = 0; i < 5; i++)
    {
      bitmap.number = i;
      GSM->GetBitmap (&bitmap);
      if (*bitmap.text != '\0')
      {
        g_free (xgnokiiConfig.callerGroups[i]);
        xgnokiiConfig.callerGroups[i] = g_strndup (bitmap.text, MAX_CALLER_GROUP_LENGTH);
      }
    }
}

GSM_Error fbusinit(bool enable_monitoring)
{
  int count=0;
  static GSM_Error error=GE_NOLINK;
  GSM_ConnectionType connection=GCT_Serial;

  if (!strcmp(xgnokiiConfig.connection, "infrared"))
    connection = GCT_Infrared;
    
  /* Initialise the code for the GSM interface. */     

  if (error==GE_NOLINK)
    error = GSM_Initialise (xgnokiiConfig.model, xgnokiiConfig.port,
                            xgnokiiConfig.initlength, connection, NULL);

#ifdef XDEBUG
  g_print ("fbusinit: error %d\n", error);
#endif
  
  if (error != GE_NONE) {
    fprintf (stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
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

  InitModelInf ();
  
  return *GSM_LinkOK;
}

void GUI_DrawBackground (GtkWidget *data) {

  gdk_draw_pixmap (Pixmap,
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

void GUI_DrawNetwork (GtkWidget *data, int rflevel) {

  int i;

  if (rflevel > 4)
    rflevel = 4;
  for (i=1; i<=rflevel; i++)
    gdk_draw_rectangle (Pixmap,
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

void GUI_DrawBattery (GtkWidget *data, int batterylevel) {

  int i;

  if (batterylevel >= 0) {
    if (batterylevel > 4)
      batterylevel = 4;
    for (i = 1; i <= batterylevel; i++)
      gdk_draw_rectangle (Pixmap,
			  GTK_WIDGET (data)->style->white_gc,
			  TRUE,
			  battery_levels[4*(i-1)], battery_levels[4*(i-1)+1],
			  battery_levels[4*(i-1)+2], battery_levels[4*(i-1)+3]);

  }
}

void GUI_DrawSMS (GtkWidget *data) {

    gdk_draw_pixmap (Pixmap,
		    GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		    SMSPixmap,
		    0, 0,
		    25, 47,
		    26, 7);
}

void GUI_DrawAlarm (GtkWidget *data) {

    gdk_draw_pixmap (Pixmap,
		     GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		     AlarmPixmap,
		     0, 0,
		     163, 11,
		     9, 9);
}

void GUI_DrawSMSReceived (GtkWidget *data) {

  static GdkFont *Font;

  Font = gdk_font_load ("-misc-fixed-medium-r-*-*-*-100-*-*-*-*-*-*");
  gdk_draw_string(Pixmap,
		  Font,
		  GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
		  33, 25, _("Short Message received"));
}

inline void HideCallDialog (GtkWidget *widget, gpointer data)
{
  hiddenCallDialog = 1;
  gtk_widget_hide (GTK_WIDGET (data));
}

void CreateInCallDialog ()
{
  GtkWidget *button, *hbox;
  
  inCallDialog.dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (inCallDialog.dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (inCallDialog.dialog), _("Call in progress"));
  gtk_container_set_border_width (GTK_CONTAINER (inCallDialog.dialog), 5);
  gtk_signal_connect (GTK_OBJECT (inCallDialog.dialog), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (inCallDialog.dialog)->vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);
  
  inCallDialog.label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), inCallDialog.label, FALSE, FALSE, 0);
  gtk_widget_show (inCallDialog.label);
  
  button = gtk_button_new_with_label (_("Hide"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (inCallDialog.dialog)->action_area),
                      button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (HideCallDialog), (gpointer) inCallDialog.dialog);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
}

gint GUI_Update (gpointer data)
{
  static gchar callNum[20];
  static gchar lastCallNum[20] = "";
  static gchar callBuf[80];
  static gchar timeBuf[10];
  static gchar anonym[] = "anonymous";
  static struct tm stm;
  static gint initialized = 0;
  static gint smsNumber = 0;
  static gint callTimerStart = 0;
  gint callTimer = 0;
  time_t t;
  static gchar *name;
  gint status;
    

  /* Define required unit types for RF and Battery level meters. */
  GSM_RFUnits rf_units = GRF_Arbitrary;
  GSM_BatteryUnits batt_units = GBU_Arbitrary;

  /* The number of SMS messages before second */
  static int smsold=0;

  /* The number of second for we should display "Short Message Received" message */
  static int smsreceived=-1;

/*#ifdef XDEBUG
  g_print("GUI_Update enter.\n");
#endif*/

  if (!initialized && (fbusinit (true)==true))
    initialized++;

  GUI_DrawBackground (data);

/*#ifdef XDEBUG
  g_print("Point 1. GSM_LinkOK: %d\n", *GSM_LinkOK);
#endif
*/
  if (GSM->GetRFLevel(&rf_units, &rflevel) == GE_NONE)
    GUI_DrawNetwork(data, rflevel);


/*#ifdef XDEBUG
  g_print("Point 2.\n");
#endif*/

  if (GSM->GetPowerSource(&powersource) == GE_NONE && powersource == GPS_ACDC) {
    batterylevel=((int) batterylevel+1)%5;
  }
  else if (GSM->GetBatteryLevel(&batt_units, &batterylevel) != GE_NONE)
    batterylevel=-1;

  GUI_DrawBattery(data, batterylevel);

/*#ifdef XDEBUG
  g_print("Point 3.\n");
#endif*/

  if (GSM->GetAlarm (0, &Alarm) == GE_NONE && Alarm.AlarmEnabled != 0)
    GUI_DrawAlarm (data);

/*#ifdef XDEBUG
  g_print("Point 4.\n");
#endif*/

  if (GSM->GetSMSStatus (&SMSStatus) == GE_NONE)
  {
    if (SMSStatus.UnRead > 0)
    {
      GUI_DrawSMS (data);
  
      if (SMSStatus.UnRead > smsold && smsold != -1)
      {
        GUI_RefreshSMS ();
        smsreceived = 10; /* The message "Short Message Received" is displayed for 10s */
      }
      else if (smsNumber != SMSStatus.Number)
        GUI_RefreshSMS ();
        
      smsold=SMSStatus.UnRead;
    }
    if (smsNumber != SMSStatus.Number)
      GUI_RefreshSMS ();
    
    smsNumber = SMSStatus.Number;
  }

  if (smsreceived >= 0) {
    GUI_DrawSMSReceived (data);
    smsreceived--;
  }

  if (GSM->GetIncomingCallNr (callNum) == GE_NONE)
  {
    GSM->GetDisplayStatus (&status);
    if (status & (1<<DS_Call_In_Progress))
    {
      if (!callTimerStart)
        callTimerStart = callTimer = time (NULL);
      else
        callTimer = time (NULL);
    }
    
    if (*callNum == '\0')
      name = anonym;
    else if (strncmp (callNum, lastCallNum, 20))
    {
      strncpy (lastCallNum, callNum, 20);
      lastCallNum[19] = '\0';
      name = GUI_GetName (callNum);
      if (!name)
        name = callNum;
    }
    t = (time_t) difftime (callTimer, callTimerStart);
    (void) gmtime_r (&t, &stm);
    strftime (timeBuf, 10, "%T", &stm);
    g_snprintf (callBuf, 80, _("Incomming call from: %s\nTime: %s"),
                name, timeBuf);
    
    gtk_label_set_text (GTK_LABEL (inCallDialog.label), callBuf);
    if (!GTK_WIDGET_VISIBLE (inCallDialog.dialog) && !hiddenCallDialog)
      gtk_widget_show (inCallDialog.dialog);
  }
  else
  {  
    callTimerStart = callTimer = 0;
    *lastCallNum = '\0';
    if (GTK_WIDGET_VISIBLE (inCallDialog.dialog))
      gtk_widget_hide (inCallDialog.dialog);
    hiddenCallDialog = 0;
  }
  
  gtk_widget_draw (data,NULL);

  
/*#ifdef XDEBUG
  g_print("GUI_Update leave.\n");
#endif*/
  
  GUI_RefreshNetmon ();
  
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

static void ParseSMSCenters ()
{
  register gint i;
  register gint j;
  
  gtk_clist_freeze (GTK_CLIST (SMSClist));
  
  gtk_clist_clear (GTK_CLIST (SMSClist));
  
  for (i = 0; i < xgnokiiConfig.smsSets; i++)
  {
    gchar *row[4];
    if (*(configDialogData.sms.smsSetting[i].Name) == '\0')
      row[0] = g_strdup_printf (_("Set %d"), i + 1);
    else
      row[0] = g_strdup (configDialogData.sms.smsSetting[i].Name);
    
    row[1] = g_strdup (configDialogData.sms.smsSetting[i].Number);
  
    switch (configDialogData.sms.smsSetting[i].Format)
    {
      case GSMF_Text:
        row[2] = g_strdup (_("Text"));
        break;
      
    case GSMF_Paging:
        row[2] = g_strdup (_("Paging"));
        break;
      
    case GSMF_Fax:
        row[2] = g_strdup (_("Fax"));
        break;
    
    case GSMF_Email:
    case GSMF_UCI:
        row[2] = g_strdup (_("E-Mail"));
        break;
    
    case GSMF_ERMES:
        row[2] = g_strdup (_("ERMES"));
        break;
        
    case GSMF_X400:
        row[2] = g_strdup (_("X.400"));
        break;
    
    case GSMF_Voice:
        row[2] = g_strdup (_("Voice"));
        break;
    
    default:
        row[2] = g_strdup (_("Text"));
        break;
    }
  
    switch (configDialogData.sms.smsSetting[i].Validity)
    {
      case GSMV_1_Hour:
        row[3] = g_strdup (_("1 h"));
        break;
      
      case GSMV_6_Hours:
        row[3] = g_strdup (_("6 h"));
        break;
      
      case GSMV_24_Hours:
        row[3] = g_strdup (_("24 h"));
        break;
      
      case GSMV_72_Hours:
        row[3] = g_strdup (_("72 h"));
        break;
      
      case GSMV_1_Week:
        row[3] = g_strdup (_("1 week"));
        break;
      
      case GSMV_Max_Time:
        row[3] = g_strdup (_("Max. time"));
        break;
      
      default:
        row[3] = g_strdup (_("24 h"));
        break;
    }
    
    gtk_clist_append( GTK_CLIST (SMSClist), row); 
    
    for (j = 0; j < 4; j++)
      g_free (row[j]);
  }
  
  gtk_clist_thaw (GTK_CLIST (SMSClist));
}
  
static void RefreshUserStatus (void)
{
  gchar buf[8];
  configDialogData.user.used = GTK_ENTRY (configDialogData.user.name)->text_length
                             + GTK_ENTRY (configDialogData.user.title)->text_length
                             + GTK_ENTRY (configDialogData.user.company)->text_length
                             + GTK_ENTRY (configDialogData.user.telephone)->text_length
                             + GTK_ENTRY (configDialogData.user.fax)->text_length
                             + GTK_ENTRY (configDialogData.user.email)->text_length
                             + GTK_ENTRY (configDialogData.user.address)->text_length;
  configDialogData.user.max = MAX_BUSINESS_CARD_LENGTH;
  if (GTK_ENTRY (configDialogData.user.telephone)->text_length > 0)
    configDialogData.user.max -= 4;
  if (GTK_ENTRY (configDialogData.user.fax)->text_length > 0)
    configDialogData.user.max -= 4;
  g_snprintf (buf, 8, "%d/%d", configDialogData.user.used, configDialogData.user.max);
  gtk_label_set_text (GTK_LABEL (configDialogData.user.status), buf);
}

void GUI_InitSMSSettings (void)
{
  register gint i;
  
  if (SMSSettingsInitialized)
    return;
    
  for (i = 1; i <= MAX_SMS_CENTER; i++)
  {
    xgnokiiConfig.smsSetting[i - 1].No = i;
    if (GSM->GetSMSCenter (&(xgnokiiConfig.smsSetting[i - 1])) != GE_NONE)
      break;
    
    configDialogData.sms.smsSetting[i - 1] = xgnokiiConfig.smsSetting[i - 1];
  }
  
  xgnokiiConfig.smsSets = i - 1;
  
  ParseSMSCenters ();
  
  SMSSettingsInitialized = TRUE;
}

static void GUI_ShowOptions ()
{
  GSM_DateTime date_time;
  register gint i;
  
  if (optionsDialogIsOpened)
    return;
    
  gtk_entry_set_text (GTK_ENTRY (configDialogData.connection.port), xgnokiiConfig.port);
  
  gtk_entry_set_text (GTK_ENTRY (configDialogData.connection.model), xgnokiiConfig.model);

  gtk_entry_set_text (GTK_ENTRY (configDialogData.connection.init), xgnokiiConfig.initlength);

  gtk_entry_set_text (GTK_ENTRY (configDialogData.connection.bindir), xgnokiiConfig.bindir);

  if (!strcmp(xgnokiiConfig.connection, "serial"))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.connection.serial), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.connection.infrared), TRUE);
  
  /* Phone */
  gtk_entry_set_text (GTK_ENTRY (configDialogData.phone.model), xgnokiiConfig.phoneModel);
  
  gtk_entry_set_text (GTK_ENTRY (configDialogData.phone.version), xgnokiiConfig.phoneVer);

  gtk_entry_set_text (GTK_ENTRY (configDialogData.phone.revision), xgnokiiConfig.phoneRevision);

  gtk_entry_set_text (GTK_ENTRY (configDialogData.phone.imei), xgnokiiConfig.phoneImei);

  /* Alarm */
  if (GSM->GetAlarm(0, &date_time) != GE_NONE)
  {
    xgnokiiConfig.alarmSupported = FALSE;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.alarm.alarmSwitch), FALSE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (configDialogData.alarm.alarmHour), 0.0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (configDialogData.alarm.alarmMin), 0.0);
  }
  else
  {
    xgnokiiConfig.alarmSupported = TRUE;
    if (date_time.AlarmEnabled)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.alarm.alarmSwitch), TRUE);
    else
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (configDialogData.alarm.alarmSwitch), FALSE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (configDialogData.alarm.alarmHour), date_time.Hour);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (configDialogData.alarm.alarmMin), date_time.Minute);
  }
  

  /* SMS */
  GUI_InitSMSSettings ();
  
  
  /* BUSINESS CARD */
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.name),
                      xgnokiiConfig.user.name);
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.title),
                      xgnokiiConfig.user.title);
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.company),
                      xgnokiiConfig.user.company);
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.telephone),
                      xgnokiiConfig.user.telephone);
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.fax),
                      xgnokiiConfig.user.fax);
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.email),
                      xgnokiiConfig.user.email);
  gtk_entry_set_text (GTK_ENTRY (configDialogData.user.address),
                      xgnokiiConfig.user.address);
  
  RefreshUserStatus();
  
  /* Groups */
  if (xgnokiiConfig.callerGroupsSupported)
  {
    gtk_widget_show (cg_names_option_frame);
    GUI_InitCallerGroupsInf ();
    for ( i = 0; i < 6; i++)
      gtk_entry_set_text (GTK_ENTRY (configDialogData.groups[i]), xgnokiiConfig.callerGroups[i]);
  }
  else
    gtk_widget_hide (cg_names_option_frame);

  /* Help */
  gtk_entry_set_text (GTK_ENTRY (configDialogData.help),
                      xgnokiiConfig.helpviewer);
                      
  optionsDialogIsOpened = TRUE;
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
  
  if (KeyboardSupported (xgnokiiConfig.phoneVer))
    gtk_widget_show (xkeyb_menu_item);
  else
    gtk_widget_hide (xkeyb_menu_item);
    
  if (xgnokiiConfig.netMonitorSupported)
    gtk_widget_show (netmon_menu_item);
  else
    gtk_widget_hide (netmon_menu_item);
  
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
      // gtk_widget_show(GUI_CardWindow);
    }
    else if (event->x >= 245 && event->x <= 258 &&
             event->y >=  83 && event->y <= 93)
    {
      GUI_ShowOptions();
    }
  } /* Right button */
  else if (event->button == 3)
    GUI_ShowMenu (event); 

  // g_print ("event->x: %f\n", event->x);
  // g_print ("event->y: %f\n", event->y);

  return TRUE;
}

void GUI_Refresh() {
  while (gtk_events_pending())
    gtk_main_iteration();
}


static void optionsApplyCallback (GtkWidget *widget, gpointer data )
{
  GSM_DateTime date_time;
  register gint i;
  
  /* ALARM */
  /* From fbus-6110.c 
     FIXME: we should also allow to set the alarm off :-) */  
  if (xgnokiiConfig.alarmSupported 
      && GTK_TOGGLE_BUTTON (configDialogData.alarm.alarmSwitch)->active) 
  {
    date_time.Hour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (configDialogData.alarm.alarmHour));
    date_time.Minute = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (configDialogData.alarm.alarmMin));
    GSM->SetAlarm (0, &date_time);
  }
  
  /* SMS */
  for (i = 0; i < xgnokiiConfig.smsSets; i++)
    xgnokiiConfig.smsSetting[i] = configDialogData.sms.smsSetting[i];
  GUI_RefreshSMSCenterMenu ();
  
  /* BUSINESS CARD */
  g_free(xgnokiiConfig.user.name);
  xgnokiiConfig.user.name = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.name)));
  g_free(xgnokiiConfig.user.title);
  xgnokiiConfig.user.title = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.title)));
  g_free(xgnokiiConfig.user.company);
  xgnokiiConfig.user.company = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.company)));
  g_free(xgnokiiConfig.user.telephone);
  xgnokiiConfig.user.telephone = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.telephone)));
  g_free(xgnokiiConfig.user.fax);
  xgnokiiConfig.user.fax = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.fax)));
  g_free(xgnokiiConfig.user.email);
  xgnokiiConfig.user.email = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.email)));
  g_free(xgnokiiConfig.user.address);
  xgnokiiConfig.user.address = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.user.address)));
  
  /* GROUPS */
  if (xgnokiiConfig.callerGroupsSupported)
    for ( i = 0; i < 6; i++)
    {
      strncpy(xgnokiiConfig.callerGroups[i], 
              gtk_entry_get_text(GTK_ENTRY (configDialogData.groups[i])),
              MAX_CALLER_GROUP_LENGTH);
      xgnokiiConfig.callerGroups[i][MAX_CALLER_GROUP_LENGTH] = '\0';
    }
  GUI_RefreshGroupMenu ();
  
  /* Help */
  g_free(xgnokiiConfig.helpviewer);
  xgnokiiConfig.helpviewer = g_strdup (gtk_entry_get_text(GTK_ENTRY (configDialogData.help)));
  
  GUI_RefreshContacts();
}

static void optionsSaveCallback( GtkWidget *widget, gpointer data )
{
  GSM_Bitmap bitmap;
  register gint i;
  
  //gtk_widget_hide(GTK_WIDGET(data));
  optionsApplyCallback (widget, data);
  for (i = 0; i < xgnokiiConfig.smsSets; i++)
  {
    xgnokiiConfig.smsSetting[i].No = i + 1;
    if (GSM->SetSMSCenter (&(xgnokiiConfig.smsSetting[i])) != GE_NONE)
    {
      gtk_label_set_text (GTK_LABEL(errorDialog.text), _("Error saving SMS centers!"));
      gtk_widget_show (errorDialog.dialog);
    }
  }

  if (xgnokiiConfig.callerGroupsSupported)
  {
    bitmap.type = GSM_CallerLogo;
    
    bitmap.number = 0;
    GSM->GetBitmap (&bitmap);
    if (strcmp (xgnokiiConfig.callerGroups[0], _("Familly")) == 0)
      *bitmap.text = '\0';
    else
      strncpy (bitmap.text, xgnokiiConfig.callerGroups[0], 256);
    bitmap.text[255] = '\0';
    GSM->SetBitmap (&bitmap);
    
    bitmap.number = 1;
    GSM->GetBitmap (&bitmap);
    if (strcmp (xgnokiiConfig.callerGroups[1], _("VIP")) == 0)
      *bitmap.text = '\0';
    else
      strncpy (bitmap.text, xgnokiiConfig.callerGroups[1], 256);
    bitmap.text[255] = '\0';
    GSM->SetBitmap (&bitmap);
    
    bitmap.number = 2;
    GSM->GetBitmap (&bitmap);
    if (strcmp (xgnokiiConfig.callerGroups[2], _("Friends")) == 0)
      *bitmap.text = '\0';
    else
      strncpy (bitmap.text, xgnokiiConfig.callerGroups[2], 256);
    bitmap.text[255] = '\0';
    GSM->SetBitmap (&bitmap);
    
    bitmap.number = 3;
    GSM->GetBitmap (&bitmap);
    if (strcmp (xgnokiiConfig.callerGroups[3], _("Colleagues")) == 0)
      *bitmap.text = '\0';
    else
      strncpy (bitmap.text, xgnokiiConfig.callerGroups[3], 256);
    bitmap.text[255] = '\0';
    GSM->SetBitmap (&bitmap);
    
    bitmap.number = 4;
    GSM->GetBitmap (&bitmap);
    if (strcmp (xgnokiiConfig.callerGroups[4], _("Other")) == 0)
      *bitmap.text = '\0';
    else
      strncpy (bitmap.text, xgnokiiConfig.callerGroups[4], 256);
    bitmap.text[255] = '\0';
    GSM->SetBitmap (&bitmap);
  }

  
  if (GUI_SaveXConfig())
  {
    gtk_label_set_text (GTK_LABEL(errorDialog.text), _("Error writing configuration file!"));
    gtk_widget_show (errorDialog.dialog);
  }
}


GtkWidget *GUI_CreateMenu ()
{
  GtkWidget *menu, *menu_items;
  
  menu = gtk_menu_new ();
  
  menu_items = gtk_menu_item_new_with_label (_("Contacts"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC (GUI_ShowContacts), NULL);
  gtk_widget_show (menu_items);

  menu_items = gtk_menu_item_new_with_label (_("SMS"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC (GUI_ShowSMS), NULL);
  gtk_widget_show (menu_items);
  
  menu_items = gtk_menu_item_new_with_label (_("DTMF"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC (GUI_ShowDTMF), NULL);
  gtk_widget_show (menu_items);
  
  menu_items = gtk_menu_item_new_with_label (_("Speed Dial"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC (GUI_ShowSpeedDial), NULL);
  gtk_widget_show (menu_items);


  xkeyb_menu_item = gtk_menu_item_new_with_label (_("Keyboard"));
  gtk_menu_append (GTK_MENU (menu), xkeyb_menu_item);
  gtk_signal_connect_object (GTK_OBJECT(xkeyb_menu_item), "activate",
                             GTK_SIGNAL_FUNC (GUI_ShowXkeyb), NULL);
                             
  netmon_menu_item = gtk_menu_item_new_with_label (_("Net Monitor"));
  gtk_menu_append (GTK_MENU (menu), netmon_menu_item);
  gtk_signal_connect_object (GTK_OBJECT(netmon_menu_item), "activate",
                             GTK_SIGNAL_FUNC (GUI_ShowNetmon), NULL);
  
  menu_items = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_widget_show (menu_items);
  
  menu_items = gtk_menu_item_new_with_label (_("Options"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC(GUI_ShowOptions), NULL);
  gtk_widget_show (menu_items);
  
  menu_items = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_widget_show (menu_items);
  
  menu_items = gtk_menu_item_new_with_label (_("Help"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC(Help1), NULL);
  gtk_widget_show (menu_items);
  
  menu_items = gtk_menu_item_new_with_label (_("About"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect_object (GTK_OBJECT(menu_items), "activate",
                             GTK_SIGNAL_FUNC(GUI_ShowAbout), NULL);
  gtk_widget_show (menu_items);
  
  return menu;
}

GtkWidget *GUI_CreateAboutDialog ()
{
  GtkWidget *dialog;
  GtkWidget *button, *hbox, *label;
  gchar buf[200];
  
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("About"));
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
  button = gtk_button_new_with_label (_("Ok"));
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
  
  g_snprintf (buf, 200, _("xgnokii version: %s\ngnokii version: %s\n\n\
Copyright (C) 1999 Pavel Janík ml.,\nHugh Blemings & Jan Derfinak\n"), XVERSION, VERSION);
  label = gtk_label_new ((gchar *) buf);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  return dialog;
}

static inline void SetFormat( GtkWidget *item, gpointer data)
{
  tempMessageSettings.Format = GPOINTER_TO_INT (data);
}

static inline void SetValidity( GtkWidget *item, gpointer data)
{
  tempMessageSettings.Validity = GPOINTER_TO_INT (data);
}

static inline void OptionsDeleteEvent( GtkWidget *widget, GdkEvent *event, gpointer data )
{
  gtk_widget_hide( GTK_WIDGET (widget));
  optionsDialogIsOpened = FALSE;
}

static inline void OptionsCloseCallback( GtkWidget *widget, gpointer data )
{
  gtk_widget_hide(GTK_WIDGET(data));
  optionsDialogIsOpened = FALSE;
}

static gint CheckInUserDataLength(GtkWidget *widget,
                                  GdkEvent  *event,
                                  gpointer   callback_data)
{
  register gint len;
  
  len = configDialogData.user.max - (GTK_ENTRY (configDialogData.user.name)->text_length
      + GTK_ENTRY (configDialogData.user.title)->text_length
      + GTK_ENTRY (configDialogData.user.company)->text_length
      + GTK_ENTRY (configDialogData.user.telephone)->text_length
      + GTK_ENTRY (configDialogData.user.fax)->text_length
      + GTK_ENTRY (configDialogData.user.email)->text_length
      + GTK_ENTRY (configDialogData.user.address)->text_length
      - GTK_ENTRY (widget)->text_length);

  if (len < 1)
  {
    gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
    return (FALSE);
  }
  else 
    gtk_entry_set_editable (GTK_ENTRY (widget), TRUE);
  if (GPOINTER_TO_INT (callback_data) == 3
      || GPOINTER_TO_INT (callback_data) == 4)
  {
    if ((GPOINTER_TO_INT (callback_data) == 3
        && GTK_ENTRY (configDialogData.user.telephone)->text_length == 0)
        || (GPOINTER_TO_INT (callback_data) == 4
        && GTK_ENTRY (configDialogData.user.fax)->text_length == 0))
      len -= 4;
    
    if (len < 1)
    {
      gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
      return (FALSE);
    }
    
    if (len > max_phonebook_number_length)
      len = max_phonebook_number_length;
  }
  
  gtk_entry_set_max_length (GTK_ENTRY (widget), len);
  return (FALSE);
}

static inline gint CheckOutUserDataLength(GtkWidget *widget,
                                          GdkEvent  *event,
                                          gpointer   callback_data)
{
  gtk_entry_set_max_length (GTK_ENTRY (widget), GPOINTER_TO_INT (callback_data));
  return (FALSE);  
}



static inline gint RefreshUserStatusCallBack(GtkWidget   *widget,
                                             GdkEventKey *event,
                                             gpointer     callback_data)
{
  RefreshUserStatus ();
  if (GTK_EDITABLE (widget)->editable == FALSE)
    return (FALSE);
  if (event->keyval == GDK_BackSpace || event->keyval == GDK_Clear ||
      event->keyval == GDK_Insert || event->keyval == GDK_Delete ||
      event->keyval == GDK_Home || event->keyval == GDK_End ||
      event->keyval == GDK_Left || event->keyval == GDK_Right ||
      event->keyval == GDK_Return ||
      (event->keyval >= 0x20 && event->keyval <= 0xFF))
    return (TRUE);
  
  return (FALSE);
}

static void OkEditSMSSetDialog (GtkWidget *w, gpointer data)
{
  
  strncpy(configDialogData.sms.smsSetting
          [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Name,
          gtk_entry_get_text(GTK_ENTRY (configDialogData.sms.set)),
          GSM_MAX_SMS_CENTER_NAME_LENGTH);
  configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Name[GSM_MAX_SMS_CENTER_NAME_LENGTH - 1]
    = '\0';
  
  strncpy(configDialogData.sms.smsSetting
          [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Number,
          gtk_entry_get_text(GTK_ENTRY (configDialogData.sms.number)),
          GSM_MAX_SMS_CENTER_LENGTH);
  configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Number[GSM_MAX_SMS_CENTER_LENGTH]
    = '\0';

  configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Format
    = tempMessageSettings.Format;
    
  configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Validity
    = tempMessageSettings.Validity;
  
  ParseSMSCenters ();
  
  gtk_widget_hide (GTK_WIDGET (data));  
}

static void EditSMSSetDialogClick (GtkWidget        *clist,
                                   gint              row,
                                   gint              column,
                                   GdkEventButton   *event,
                                   GtkWidget        *data )
{
  if(event && event->type == GDK_2BUTTON_PRESS)
    gtk_signal_emit_by_name(GTK_OBJECT (data), "clicked");
}

static void ShowEditSMSSetDialog (GtkWidget *w, gpointer data)
{
  static GtkWidget *dialog = NULL;
  GtkWidget *button, *label, *hbox, *menu, *item;
  
  if (GTK_CLIST (SMSClist)->selection == NULL)
    return;
    
  
  if (dialog == NULL)
  {
    dialog = gtk_dialog_new();
    gtk_window_set_title (GTK_WINDOW (dialog), _("Edit SMS Setting"));
    gtk_window_set_modal(GTK_WINDOW (dialog), TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
    
    button = gtk_button_new_with_label (_("Ok"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                        button, TRUE, TRUE, 10);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (OkEditSMSSetDialog), (gpointer) dialog);
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
  
    label = gtk_label_new (_("Set name:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);
  
    configDialogData.sms.set = gtk_entry_new_with_max_length(GSM_MAX_SMS_CENTER_NAME_LENGTH - 1);
    gtk_widget_set_usize (configDialogData.sms.set, 110, 22);
    gtk_box_pack_end(GTK_BOX(hbox), configDialogData.sms.set, FALSE, FALSE, 2);
    gtk_widget_show (configDialogData.sms.set);
  
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
    gtk_widget_show (hbox);
  
    label = gtk_label_new (_("Center:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);
  
    configDialogData.sms.number = gtk_entry_new_with_max_length(GSM_MAX_SMS_CENTER_LENGTH - 1);
    gtk_widget_set_usize (configDialogData.sms.number, 110, 22);
    gtk_box_pack_end(GTK_BOX(hbox), configDialogData.sms.number, FALSE, FALSE, 2);
    gtk_widget_show (configDialogData.sms.number);
  
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 9);
    gtk_widget_show (hbox);
  
    label = gtk_label_new (_("Sending Format:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);
  
    configDialogData.sms.format = gtk_option_menu_new ();
    menu = gtk_menu_new ();
    gtk_widget_set_usize (configDialogData.sms.format, 110, 28);
  
    item = gtk_menu_item_new_with_label (_("Text"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetFormat),
                        (gpointer) GSMF_Text);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("Fax"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetFormat),
                        (gpointer) GSMF_Fax);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("Paging"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetFormat),
                        (gpointer) GSMF_Paging);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("E-Mail"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetFormat),
                        (gpointer) GSMF_Email);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    gtk_option_menu_set_menu (GTK_OPTION_MENU (configDialogData.sms.format), menu);
    gtk_box_pack_end (GTK_BOX (hbox), configDialogData.sms.format, FALSE, FALSE, 2);
    gtk_widget_show (configDialogData.sms.format);
  
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 9);
    gtk_widget_show (hbox);
  
    label = gtk_label_new (_("Validity Period:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);
  
    configDialogData.sms.validity = gtk_option_menu_new ();
    menu = gtk_menu_new ();
    gtk_widget_set_usize (configDialogData.sms.validity, 110, 28);
  
    item = gtk_menu_item_new_with_label (_("Max. Time"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetValidity),
                        (gpointer) GSMV_Max_Time);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("1 h"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetValidity),
                        (gpointer) GSMV_1_Hour);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("6 h"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetValidity),
                        (gpointer) GSMV_6_Hours);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("24 h"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetValidity),
                        (gpointer) GSMV_24_Hours);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("72 h"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetValidity),
                        (gpointer) GSMV_72_Hours);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    item = gtk_menu_item_new_with_label (_("1 week"));
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC(SetValidity),
                        (gpointer) GSMV_1_Week);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  
    gtk_option_menu_set_menu (GTK_OPTION_MENU (configDialogData.sms.validity), menu);
    gtk_box_pack_end (GTK_BOX (hbox), configDialogData.sms.validity, FALSE, FALSE, 2);
    gtk_widget_show (configDialogData.sms.validity);
  }
  
  gtk_entry_set_text (GTK_ENTRY (configDialogData.sms.set),
                      configDialogData.sms.smsSetting
                      [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Name);
  
  gtk_entry_set_text (GTK_ENTRY (configDialogData.sms.number),
                      configDialogData.sms.smsSetting
                      [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Number);
                        
  switch (configDialogData.sms.smsSetting
          [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Format)
  {
    case GSMF_Text:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.format),
                                   0);
      break;
      
    case GSMF_Paging:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.format),
                                   2);
      break;
      
    case GSMF_Fax:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.format),
                                   1);
      break;
    
    case GSMF_Email:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.format),
                                   3);
      break;
      
    default:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.format),
                                   0);
  }
  
  switch (configDialogData.sms.smsSetting
          [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Validity)
  {
    case GSMV_1_Hour:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   1);
      break;
      
    case GSMV_6_Hours:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   2);
      break;
      
    case GSMV_24_Hours:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   3);
      break;
      
    case GSMV_72_Hours:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   4);
      break;
      
    case GSMV_1_Week:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   5);
      break;
      
    case GSMV_Max_Time:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   0);
      break;
      
    default:
      gtk_option_menu_set_history (GTK_OPTION_MENU (configDialogData.sms.validity),
                                   3);
  }
  
  tempMessageSettings.Format = configDialogData.sms.smsSetting
               [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Format;
  tempMessageSettings.Validity = configDialogData.sms.smsSetting
               [GPOINTER_TO_INT(GTK_CLIST (SMSClist)->selection->data)].Validity;
  
  gtk_widget_show (dialog);
}
                              
GtkWidget *GUI_CreateOptionsDialog ()
{
  gchar labelBuffer[10];
  GtkWidget *dialog;
  GtkWidget *button, *hbox, *vbox, *label, *notebook, *frame, *clistScrolledWindow;
  register gint i;
  GtkAdjustment *adj;
  gchar *titles[4] = { _("Set name"), _("Center number"), _("Format"), _("Validity")};
  
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Options"));
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC (OptionsDeleteEvent), NULL);
  
  button = gtk_button_new_with_label (_("Apply"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                      button, TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (optionsApplyCallback), (gpointer)dialog);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label (_("Save"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                      button, TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (optionsSaveCallback), (gpointer)dialog);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                      button, TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (OptionsCloseCallback), (gpointer)dialog);
  gtk_widget_show (button);

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 5);
   
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), notebook);
  gtk_widget_show(notebook);
  
  /***  Connection notebook  ***/
  frame = gtk_frame_new (_("Phone and connection type"));
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new( FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Connection"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Port:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.connection.port = gtk_entry_new_with_max_length (10);
  gtk_widget_set_usize (configDialogData.connection.port, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.connection.port),
                          FALSE); 
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.connection.port, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.connection.port);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Model:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.connection.model = gtk_entry_new_with_max_length (5);
  gtk_widget_set_usize (configDialogData.connection.model, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.connection.model),
                          FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.connection.model, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.connection.model);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Init length:"));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.connection.init = gtk_entry_new_with_max_length (100);
  gtk_widget_set_usize (configDialogData.connection.init, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.connection.init),
                          FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.connection.init, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.connection.init);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Bindir:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.connection.bindir = gtk_entry_new_with_max_length (100);
  gtk_widget_set_usize (configDialogData.connection.bindir, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.connection.bindir),
                          FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.connection.bindir, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.connection.bindir);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Connection:"));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  configDialogData.connection.infrared = gtk_radio_button_new_with_label (NULL, _("infrared"));
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.connection.infrared, TRUE, FALSE, 2);
  gtk_widget_show (configDialogData.connection.infrared);
  configDialogData.connection.serial = gtk_radio_button_new_with_label ( 
            gtk_radio_button_group (GTK_RADIO_BUTTON (configDialogData.connection.infrared)), _("serial"));
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.connection.serial, TRUE, FALSE, 2);
  gtk_widget_show (configDialogData.connection.serial);
  
  /***  Phone notebook  ***/
  frame = gtk_frame_new (_("Phone information"));
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new( FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Phone"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Model:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.phone.model = gtk_entry_new_with_max_length (5);
  gtk_widget_set_usize (configDialogData.phone.model, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.phone.model),
                          FALSE); 
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.phone.model, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.phone.model);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Version:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.phone.version = gtk_entry_new_with_max_length (5);
  gtk_widget_set_usize (configDialogData.phone.version, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.phone.version),
                          FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.phone.version, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.phone.version);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Revision:"));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.phone.revision = gtk_entry_new_with_max_length (64);
  gtk_widget_set_usize (configDialogData.phone.revision, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.phone.revision),
                          FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.phone.revision, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.phone.revision);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("IMEI:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.phone.imei = gtk_entry_new_with_max_length (64);
  gtk_widget_set_usize (configDialogData.phone.imei, 220, 22);
  gtk_entry_set_editable (GTK_ENTRY (configDialogData.phone.imei),
                          FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.phone.imei, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.phone.imei);

  /***  Alarm notebook  ***/
  
  xgnokiiConfig.alarmSupported = TRUE;
  
  frame = gtk_frame_new (_("Alarm setting"));
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new( FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(vbox);

  label = gtk_label_new (_("Alarm"));
  gtk_notebook_append_page( GTK_NOTEBOOK (notebook), frame, label);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);
  
  configDialogData.alarm.alarmSwitch = gtk_check_button_new_with_label (_("Alarm"));
  gtk_box_pack_start(GTK_BOX(hbox), configDialogData.alarm.alarmSwitch, FALSE, FALSE, 10);
  gtk_widget_show(configDialogData.alarm.alarmSwitch);
  
  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 23.0, 1.0, 4.0, 0.0);
  configDialogData.alarm.alarmHour = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (configDialogData.alarm.alarmHour), TRUE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (configDialogData.alarm.alarmHour), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), configDialogData.alarm.alarmHour, FALSE, FALSE, 0);
  gtk_widget_show (configDialogData.alarm.alarmHour);
  
  label = gtk_label_new (":");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 59.0, 1.0, 10.0, 0.0);
  configDialogData.alarm.alarmMin = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (configDialogData.alarm.alarmMin), TRUE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (configDialogData.alarm.alarmMin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), configDialogData.alarm.alarmMin, FALSE, FALSE, 0);
  gtk_widget_show (configDialogData.alarm.alarmMin);

  /***  SMS notebook     ***/
  
  frame = gtk_frame_new (_("Short Message Service"));
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("SMS"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
  
  SMSClist = gtk_clist_new_with_titles (4, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (SMSClist), GTK_SHADOW_OUT);
  gtk_clist_column_titles_passive (GTK_CLIST (SMSClist));
  gtk_clist_set_auto_sort (GTK_CLIST (SMSClist), FALSE);
  
  gtk_clist_set_column_width (GTK_CLIST (SMSClist), 0, 70);
  gtk_clist_set_column_width (GTK_CLIST (SMSClist), 1, 115);
  gtk_clist_set_column_width (GTK_CLIST (SMSClist), 2, 40);
  gtk_clist_set_column_width (GTK_CLIST (SMSClist), 3, 55);
//  gtk_clist_set_column_justification (GTK_CLIST (SMSClist), 1, GTK_JUSTIFY_RIGHT);
  
    
  clistScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (clistScrolledWindow), SMSClist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (clistScrolledWindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), clistScrolledWindow, 
                      TRUE, TRUE, 10);
  
  gtk_widget_show (SMSClist);
  gtk_widget_show (clistScrolledWindow);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 9);
  gtk_widget_show (hbox);
  
  button = gtk_button_new_with_label (_("Edit"));
  gtk_box_pack_start (GTK_BOX (hbox),
                      button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (ShowEditSMSSetDialog), (gpointer)dialog);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  
  gtk_signal_connect (GTK_OBJECT (SMSClist), "select_row",
                      GTK_SIGNAL_FUNC (EditSMSSetDialogClick),
                      (gpointer) button);
  
  /***  Business notebook  ***/
  frame = gtk_frame_new (_("Business Card"));
  gtk_widget_show (frame);
  
  vbox = gtk_vbox_new( FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(vbox);
  
  label = gtk_label_new(_("User"));
  gtk_notebook_append_page( GTK_NOTEBOOK (notebook), frame, label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  configDialogData.user.status = gtk_label_new ("");
  gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.status, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.status);
  
  configDialogData.user.max = MAX_BUSINESS_CARD_LENGTH;
  configDialogData.user.used = 0;
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Name:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.name = gtk_entry_new_with_max_length(configDialogData.user.max);
  gtk_widget_set_usize (configDialogData.user.name, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.name),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.name),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.name),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 0);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.name),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength),
                      (gpointer) configDialogData.user.max);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.name, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.name);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Title:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.title = gtk_entry_new_with_max_length(configDialogData.user.max);
  gtk_widget_set_usize (configDialogData.user.title, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.title),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.title),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.title),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 1);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.title),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength),
                      (gpointer) configDialogData.user.max);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.title, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.title);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Company:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.company = gtk_entry_new_with_max_length(configDialogData.user.max);
  gtk_widget_set_usize (configDialogData.user.company, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.company),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.company),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.company),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 2);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.company),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength),
                      (gpointer) configDialogData.user.max);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.company, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.company);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Telephone:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.telephone = gtk_entry_new_with_max_length(max_phonebook_number_length);
  gtk_widget_set_usize (configDialogData.user.telephone, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.telephone),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.telephone),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.telephone),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 3);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.telephone),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength), (gpointer) max_phonebook_number_length);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.telephone, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.telephone);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Fax:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.fax = gtk_entry_new_with_max_length(max_phonebook_number_length);
  gtk_widget_set_usize (configDialogData.user.fax, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.fax),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.fax),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.fax),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 4);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.fax),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength), (gpointer) max_phonebook_number_length);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.fax, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.fax);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("E-Mail:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.email = gtk_entry_new_with_max_length(configDialogData.user.max);
  gtk_widget_set_usize (configDialogData.user.email, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.email),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.email),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.email),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 5);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.email),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength),
                      (gpointer) configDialogData.user.max);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.email, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.email);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Address:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.user.address = gtk_entry_new_with_max_length(configDialogData.user.max);
  gtk_widget_set_usize (configDialogData.user.address, 220, 22);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.address),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (configDialogData.user.address),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.address),
                      "focus_in_event",
                      GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 6);
  gtk_signal_connect (GTK_OBJECT (configDialogData.user.address),
                      "focus_out_event",
                      GTK_SIGNAL_FUNC(CheckOutUserDataLength),
                      (gpointer) configDialogData.user.max);
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.user.address, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.user.address);
  

  /***  Groups notebook  ***/
  cg_names_option_frame = gtk_frame_new (_("Caller groups names"));
  

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (cg_names_option_frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Groups"));
  gtk_notebook_append_page( GTK_NOTEBOOK (notebook), cg_names_option_frame, label);
  
  for ( i = 0; i < 6; i++)
  {
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 3);
    gtk_widget_show (hbox);
  
    g_snprintf (labelBuffer, 10, _("Group %d:"), i + 1);
    label = gtk_label_new (labelBuffer);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_widget_show (label);
  
    configDialogData.groups[i] = gtk_entry_new_with_max_length (MAX_CALLER_GROUP_LENGTH);
    gtk_widget_set_usize (configDialogData.groups[i], 220, 22);

    gtk_box_pack_end (GTK_BOX (hbox), configDialogData.groups[i], FALSE, FALSE, 2);
    gtk_widget_show (configDialogData.groups[i]);
  }
  
  /* Help */
  frame = gtk_frame_new (_("Help viewer"));
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Help"));
  gtk_notebook_append_page( GTK_NOTEBOOK (notebook), frame, label);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
  
  label = gtk_label_new (_("Viewer:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  configDialogData.help = gtk_entry_new_with_max_length (HTMLVIEWER_LENGTH - 1);
  gtk_widget_set_usize (configDialogData.help, 220, 22);
  
  gtk_box_pack_end (GTK_BOX (hbox), configDialogData.help, FALSE, FALSE, 2);
  gtk_widget_show (configDialogData.help);

    
  optionsDialogIsOpened = FALSE;
  return dialog;
}

void GUI_TopLevelWindow () {

  GtkWidget *drawing_area;
  GdkBitmap *mask;
  GtkStyle *style;
  GdkGC *gc;
  struct sigaction act;

  GUI_MainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (GUI_MainWindow);

  BackgroundPixmap = gdk_pixmap_create_from_xpm_d (GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_background);

  SMSPixmap = gdk_pixmap_create_from_xpm_d (GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_sms);

  AlarmPixmap = gdk_pixmap_create_from_xpm_d (GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_alarm);

  Pixmap = gdk_pixmap_create_from_xpm_d (GUI_MainWindow->window,&mask, &GUI_MainWindow->style->white, (gchar **) XPM_background);

  style = gtk_widget_get_default_style ();
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
  gtk_container_add (GTK_CONTAINER(GUI_MainWindow), drawing_area);

  gdk_draw_pixmap (drawing_area->window,
		   drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
		   Pixmap,
		   0, 0,
		   0, 0,
		   261, 96);

  gtk_widget_shape_combine_mask (GUI_MainWindow, mask, 0, 0);
  
  gtk_signal_connect (GTK_OBJECT (GUI_MainWindow), "destroy",
                      GTK_SIGNAL_FUNC(gtk_main_quit),
                      NULL);
                      
  GUI_Menu = GUI_CreateMenu ();
  GUI_OptionsDialog = GUI_CreateOptionsDialog ();
  GUI_AboutDialog = GUI_CreateAboutDialog ();
  GUI_CreateSMSWindow ();
  GUI_CreateContactsWindow ();
  GUI_CreateNetmonWindow ();
  GUI_CreateDTMFWindow ();
  GUI_CreateSpeedDialWindow ();
  GUI_CreateXkeybWindow ();
  CreateErrorDialog (&errorDialog, GUI_MainWindow);
  CreateInCallDialog ();
  
  act.sa_handler = RemoveZombie;
  sigemptyset (&(act.sa_mask));
  act.sa_flags = SA_NOCLDSTOP;
  sigaction (SIGCHLD, &act, NULL);
  
  gtk_widget_show_all (GUI_MainWindow);
  GUI_Refresh ();

  gtk_timeout_add (1800, (GtkFunction) GUI_Update, GUI_MainWindow);
  
  hiddenCallDialog = 0;
}

void GUI_SplashScreen () {

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

int GUI_RemoveSplash (GtkWidget *Win)
{
  if (GTK_WIDGET_VISIBLE (GUI_SplashWindow))
  {
    gtk_widget_hide (GUI_SplashWindow);
    return TRUE;
  }

  return FALSE;
}

void GUI_ReadConfig (void)
{

  struct CFG_Header *cfg_info;
  gchar *homedir;
  gchar *rcfile;

#ifdef WIN32
/*  homedir = getenv("HOMEDRIVE"); */
/*  g_strconcat(homedir, getenv("HOMEPATH"), NULL); */
  homedir = g_get_home_dir ();
  rcfile=g_strconcat(homedir, "\\_gnokiirc", NULL);
#else
/*  if ((homedir = getenv ("HOME")) == NULL) */
  if ((homedir = g_get_home_dir ()) == NULL)
  {
    g_print (_("WARNING: Can't find HOME enviroment variable!\n"));
    exit (-1);
  }
  else if ((rcfile = g_strconcat (homedir, "/.gnokiirc", NULL)) == NULL)
  {
    g_print (_("WARNING: Can't allocate memory for config reading!\n"));
    exit (-1);
  }
#endif
  
  if ((cfg_info = CFG_ReadFile("/etc/gnokiirc")) == NULL)
    if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
      fprintf(stderr, _("error opening %s, using default config\n"), rcfile);
  
  g_free (rcfile);
  
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
        
#ifndef WIN32
  xgnokiiConfig.xgnokiidir = DefaultXGnokiiDir;
  
  if (strstr(FB38_Information.Models, xgnokiiConfig.model) != NULL)
  {
    max_phonebook_name_length = 20;
    max_phonebook_number_length = 30;
    max_phonebook_sim_name_length = 14;
    max_phonebook_sim_number_length = 30;
  }
  else 
#endif
  if (strstr(FB61_Information.Models, xgnokiiConfig.model) != NULL)
  {
    max_phonebook_name_length = FB61_MAX_PHONEBOOK_NAME_LENGTH;
    max_phonebook_number_length = FB61_MAX_PHONEBOOK_NUMBER_LENGTH;
    max_phonebook_sim_name_length = 14;
    max_phonebook_sim_number_length = FB61_MAX_PHONEBOOK_NUMBER_LENGTH;
  }
  else
  {
    max_phonebook_name_length = max_phonebook_sim_name_length = GSM_MAX_PHONEBOOK_NAME_LENGTH;
    max_phonebook_number_length = max_phonebook_sim_number_length = GSM_MAX_PHONEBOOK_NUMBER_LENGTH;
  }
  
  xgnokiiConfig.callerGroups[0] = xgnokiiConfig.callerGroups[1] =
  xgnokiiConfig.callerGroups[2] = xgnokiiConfig.callerGroups[3] =
  xgnokiiConfig.callerGroups[4] = xgnokiiConfig.callerGroups[5] = NULL;
  xgnokiiConfig.smsSets = 0;
  GUI_ReadXConfig();
}

int main (int argc, char *argv[])
{

#ifdef USE_NLS
  textdomain("xgnokii");
#endif

  (void) gtk_set_locale ();
  
  gtk_init (&argc, &argv);

  /* Show the splash screen. */

  GUI_SplashScreen ();

  /* Remove it after a while. */
	
  GUI_ReadConfig ();
  GUI_TopLevelWindow (); 

  (void) gtk_timeout_add(3000, (GtkFunction)GUI_RemoveSplash, (gpointer) GUI_SplashWindow);
  
  gtk_main ();
          
  return(0);
}
