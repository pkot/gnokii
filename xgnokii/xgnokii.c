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
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2001      Chris Kemp, Marian Jancar
  Copyright (C) 2002      Markus Plail
  Copyright (C) 2003-2004 BORBELY Zoltan

*/

#include "config.h"
#ifdef HAVE_ASPRINTF
#  ifndef _GNU_SOURCE 
#    define _GNU_SOURCE 1
#  endif
#  include <stdio.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "misc.h"

#include <stdlib.h>		/* for getenv */
#include <locale.h>
#include <string.h>
#include <time.h>		/* for time   */
#include <pthread.h>

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifndef WIN32
#  include <signal.h>
#else
#  include <windows.h>
#  undef IN
#  undef OUT
#endif

#include "gnokii.h"

#include "xgnokii.h"
#include "xgnokii_common.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_contacts.h"
#include "xgnokii_sms.h"
#include "xgnokii_netmon.h"
#include "xgnokii_dtmf.h"
#include "xgnokii_speed.h"
#include "xgnokii_xkeyb.h"
#include "xgnokii_calendar.h"
#include "xgnokii_logos.h"
#include "xgnokii_xring.h"
#include "xgnokii_cfg.h"
#include "xgnokii_data.h"

#include "xpm/logo.xpm"
#include "xpm/background.xpm"
#include "xpm/sms.xpm"
#include "xpm/alarm.xpm"

/* Widgets for dialogs. */
static GtkWidget *SplashWindow;
static GtkWidget *GUI_MainWindow;
static GtkWidget *AboutDialog;
static ErrorDialog errorDialog = { NULL, NULL };
static InfoDialog infoDialog = { NULL, NULL };
static GtkWidget *OptionsDialog;
static bool optionsDialogIsOpened;

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


/* Widget for popup menu */
static GtkWidget *Menu;
static GtkWidget *netmon_menu_item;
static GtkWidget *sms_menu_item;
static GtkWidget *calendar_menu_item;
static GtkWidget *logos_menu_item;
static GtkWidget *dtmf_menu_item;
static GtkWidget *speedDial_menu_item;
static GtkWidget *xkeyb_menu_item;
static GtkWidget *xring_menu_item;
static GtkWidget *cg_names_option_frame;
static GtkWidget *sms_option_frame;
static GtkWidget *mail_option_frame;
static GtkWidget *user_option_frame;
static GtkWidget *data_menu_item;

/* Hold main configuration data for xgnokii */
XgnokiiConfig xgnokiiConfig;

gint max_phonebook_name_length;
gint max_phonebook_number_length;
gint max_phonebook_sim_name_length;
gint max_phonebook_sim_number_length;
char folders[GN_SMS_FOLDER_MAX_NUMBER][GN_SMS_MESSAGE_MAX_NUMBER];
gint foldercount = 0, lastfoldercount = 0;

/* Local variables */
static char *DefaultXGnokiiDir = PREFIX "/share/xgnokii";
static bool SMSSettingsInitialized = FALSE;
static bool CallersGroupsInitialized = FALSE;
static gint hiddenCallDialog;
static guint splashRemoveHandler;

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
	GtkWidget *serial, *infrared, *irda;
} ConnectionWidgets;

typedef struct {
	GtkWidget *model;
	GtkWidget *version;
	GtkWidget *revision;
	GtkWidget *imei;
	GtkWidget *simNameLen;
	GtkWidget *phoneNameLen;
} PhoneWidgets;

typedef struct {
	GtkWidget *set;
	GtkWidget *number;
	GtkWidget *format;
	GtkWidget *validity;
	gn_sms_message_center smsSetting[MAX_SMS_CENTER];
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
	gint max;
	gint used;
} UserWidget;

static struct ConfigDialogData {
	ConnectionWidgets connection;
	PhoneWidgets phone;
	GtkWidget *groups[GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER];
	AlarmWidgets alarm;
	SMSWidgets sms;
	UserWidget user;
	GtkWidget *mailbox;
} configDialogData;

static gn_sms_message_center tempMessageSettings;


void GUI_InitCallerGroupsInf(void)
{
	D_CallerGroup *cg;
	PhoneEvent *e;
	register gint i;

	if (CallersGroupsInitialized)
		return;

	gtk_label_set_text(GTK_LABEL(infoDialog.text), _("Reading caller groups names ..."));
	gtk_widget_show_now(infoDialog.dialog);
	GUI_Refresh();

	xgnokiiConfig.callerGroups[0] = g_strndup(_("Family"), MAX_CALLER_GROUP_LENGTH);
	xgnokiiConfig.callerGroups[1] = g_strndup(_("VIP"), MAX_CALLER_GROUP_LENGTH);
	xgnokiiConfig.callerGroups[2] = g_strndup(_("Friends"), MAX_CALLER_GROUP_LENGTH);
	xgnokiiConfig.callerGroups[3] = g_strndup(_("Colleagues"), MAX_CALLER_GROUP_LENGTH);
	xgnokiiConfig.callerGroups[4] = g_strndup(_("Other"), MAX_CALLER_GROUP_LENGTH);
	xgnokiiConfig.callerGroups[5] = g_strndup(_("No group"), MAX_CALLER_GROUP_LENGTH);

	if (phoneMonitor.supported & PM_CALLERGROUP) {
		for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER; i++) {
			cg = (D_CallerGroup *) g_malloc(sizeof(D_CallerGroup));
			cg->number = i;
			e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
			e->event = Event_GetCallerGroup;
			e->data = cg;
			GUI_InsertEvent(e);
			pthread_mutex_lock(&callerGroupMutex);
			pthread_cond_wait(&callerGroupCond, &callerGroupMutex);
			pthread_mutex_unlock(&callerGroupMutex);

			if (*cg->text != '\0' && cg->status == GN_ERR_NONE) {
				g_free(xgnokiiConfig.callerGroups[i]);
				xgnokiiConfig.callerGroups[i] =
				    g_strndup(cg->text, MAX_CALLER_GROUP_LENGTH);
			}
			g_free(cg);
		}
	}

	CallersGroupsInitialized = TRUE;
	gtk_widget_hide(infoDialog.dialog);
	GUIEventSend(GUI_EVENT_CALLERS_GROUPS_CHANGED);
}


static inline void DrawBackground(GtkWidget * data)
{
	gdk_draw_pixmap(Pixmap,
			GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
			BackgroundPixmap, 0, 0, 0, 0, 261, 96);
}


int network_levels[] = {
	152, 69, 11, 3,
	138, 69, 11, 3,
	124, 69, 11, 4,
	110, 69, 11, 6
};


static inline void DrawNetwork(GtkWidget * data, int rflevel)
{
	int i;

	if (rflevel > 100)
		rflevel = 100;
	for (i = 0; (i * 25) <= rflevel; i++) {
		float percent = ((float) rflevel - i * 25) / 25;

		if (percent > 1)
			percent = 1;
		gdk_draw_rectangle(Pixmap, GTK_WIDGET(data)->style->white_gc, TRUE,
				   network_levels[4 * i] + network_levels[4 * i + 2] * (1 -
											percent),
				   network_levels[4 * i + 1], network_levels[4 * i + 2] * percent,
				   network_levels[4 * i + 3]);
	}
}


int battery_levels[] = {
	50, 69, 11, 3,
	64, 69, 11, 3,
	78, 69, 11, 4,
	92, 69, 11, 6
};


static inline void DrawBattery(GtkWidget * data, int batterylevel)
{
	int i;

	if (batterylevel < 0)
		return;
	if (batterylevel > 100)
		batterylevel = 100;
	for (i = 0; (i * 25) <= batterylevel; i++) {
		float percent = ((float) batterylevel - i * 25) / 25;
		if (percent > 1)
			percent = 1;
		gdk_draw_rectangle(Pixmap, GTK_WIDGET(data)->style->white_gc, TRUE,
				   battery_levels[4 * i],
				   battery_levels[4 * i + 1],
				   battery_levels[4 * i + 2] * percent, battery_levels[4 * i + 3]);
	}
}


static inline void DrawSMS(GtkWidget * data)
{
	gdk_draw_pixmap(Pixmap,
			GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
			SMSPixmap, 0, 0, 25, 47, 26, 7);
}


static inline void DrawAlarm(GtkWidget * data)
{
	gdk_draw_pixmap(Pixmap,
			GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL],
			AlarmPixmap, 0, 0, 163, 11, 9, 9);
}


static inline void DrawText(GtkWidget * data, int at, char *text)
{
	static GdkFont *Font;

	Font = gdk_font_load("-misc-fixed-medium-r-*-*-*-90-*-*-*-*-iso8859-*");
	gdk_draw_string(Pixmap,
			Font, GTK_WIDGET(data)->style->fg_gc[GTK_STATE_NORMAL], 33, at, text);
}


static inline void DrawSMSReceived(GtkWidget * data)
{
	DrawText(data, 25, _("Short Message received"));
}


static inline void DrawWorking(GtkWidget * data)
{
	DrawText(data, 40, _("Working..."));
}


static inline void HideCallDialog(GtkWidget * widget, gpointer data)
{
	hiddenCallDialog = 1;
	gtk_widget_hide(GTK_WIDGET(data));
}


static void CreateInCallDialog(void)
{
	GtkWidget *button, *hbox;

	inCallDialog.dialog = gtk_dialog_new();
	gtk_window_position(GTK_WINDOW(inCallDialog.dialog), GTK_WIN_POS_MOUSE);
	gtk_window_set_title(GTK_WINDOW(inCallDialog.dialog), _("Call in progress"));
	gtk_container_set_border_width(GTK_CONTAINER(inCallDialog.dialog), 5);
	gtk_signal_connect(GTK_OBJECT(inCallDialog.dialog), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(inCallDialog.dialog)->vbox), hbox, FALSE, FALSE, 5);
	gtk_widget_show(hbox);

	inCallDialog.label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), inCallDialog.label, FALSE, FALSE, 0);
	gtk_widget_show(inCallDialog.label);

	button = gtk_button_new_with_label(_("Hide"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(inCallDialog.dialog)->action_area),
			   button, TRUE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(HideCallDialog), (gpointer) inCallDialog.dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);
}


static gint Update(gpointer data)
{
	static gchar lastCallNum[20] = "";
	static gchar callBuf[80];
	static gchar timeBuf[10];
	static struct tm stm;
	static gint callTimerStart = 0;
	gint callTimer = 0;
	time_t t;
	static gchar *name;
	static bool outgoing = TRUE;


	/* The number of SMS messages before second */
	static int smsold = 0;

	/* The number of second for we should display "Short Message Received" message */
	static int smsreceived = -1;

	DrawBackground(data);

	DrawNetwork(data, phoneMonitor.rfLevel);

	DrawBattery(data, phoneMonitor.batteryLevel);

	if (phoneMonitor.alarm)
		DrawAlarm(data);

	if (phoneMonitor.working)
		DrawText(data, 25, phoneMonitor.working);

	pthread_mutex_lock(&smsMutex);

	if (phoneMonitor.sms.unRead > 0) {
		DrawSMS(data);
		if (phoneMonitor.sms.unRead > smsold && smsold != -1)
			smsreceived = 10;	/* The message "Short Message Received" is displayed for 10s */
	}

	if (phoneMonitor.sms.changed)
		GUIEventSend(GUI_EVENT_SMS_NUMBER_CHANGED);
	phoneMonitor.sms.changed = 0;
	smsold = phoneMonitor.sms.unRead;

	pthread_mutex_unlock(&smsMutex);

	if (smsreceived >= 0) {
		DrawSMSReceived(data);
		smsreceived--;
	}

	pthread_mutex_lock(&callMutex);
	if (phoneMonitor.call.callInProgress != CS_Idle) {
		if (phoneMonitor.call.callInProgress == CS_InProgress) {
			if (!callTimerStart)
				callTimerStart = callTimer = time(NULL);
			else
				callTimer = time(NULL);
		}
		if (phoneMonitor.call.callInProgress == CS_Waiting) {
			outgoing = FALSE;

			if (*phoneMonitor.call.callNum == '\0')
				name = _("anonymous");
			else if (strncmp(phoneMonitor.call.callNum, lastCallNum, 20)) {
				strncpy(lastCallNum, phoneMonitor.call.callNum, 20);
				lastCallNum[19] = '\0';
				name = GUI_GetName(phoneMonitor.call.callNum);
				if (!name)
					name = phoneMonitor.call.callNum;
			}
		}
		t = (time_t) difftime(callTimer, callTimerStart);
		(void) gmtime_r(&t, &stm);
		strftime(timeBuf, 10, "%T", &stm);
		if (outgoing)
			g_snprintf(callBuf, 80, _("Outgoing call in progress:\nTime: %s"), timeBuf);
		else
			g_snprintf(callBuf, 80, _("Incoming call from: %s\nTime: %s"), name,
				   timeBuf);

		gtk_label_set_text(GTK_LABEL(inCallDialog.label), callBuf);
		if (!GTK_WIDGET_VISIBLE(inCallDialog.dialog) && !hiddenCallDialog)
			gtk_widget_show(inCallDialog.dialog);
	} else {
		callTimerStart = callTimer = 0;
		*lastCallNum = '\0';
		outgoing = TRUE;
		if (GTK_WIDGET_VISIBLE(inCallDialog.dialog))
			gtk_widget_hide(inCallDialog.dialog);
		hiddenCallDialog = 0;
	}
	pthread_mutex_unlock(&callMutex);

	gtk_widget_draw(data, NULL);

	GUIEventSend(GUI_EVENT_NETMON_CHANGED);

	return TRUE;
}


/* Redraw the screen from the backing pixmap */
static inline gint ExposeEvent(GtkWidget * widget, GdkEventExpose * event)
{
	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			Pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y, event->area.width, event->area.height);

	return FALSE;
}


static void ParseSMSCenters(void)
{
	register gint i;
	register gint j;

	gtk_clist_freeze(GTK_CLIST(SMSClist));

	gtk_clist_clear(GTK_CLIST(SMSClist));

	for (i = 0; i < xgnokiiConfig.smsSets; i++) {
		gchar *row[4];
		if (*(configDialogData.sms.smsSetting[i].name) == '\0')
			row[0] = g_strdup_printf(_("Set %d"), i + 1);
		else
			row[0] = g_strdup(configDialogData.sms.smsSetting[i].name);

		row[1] = g_strdup(configDialogData.sms.smsSetting[i].smsc.number);

		switch (configDialogData.sms.smsSetting[i].format) {
		case GN_SMS_MF_Text:
			row[2] = g_strdup(_("Text"));
			break;

		case GN_SMS_MF_Paging:
			row[2] = g_strdup(_("Paging"));
			break;

		case GN_SMS_MF_Fax:
			row[2] = g_strdup(_("Fax"));
			break;

		case GN_SMS_MF_Email:
		case GN_SMS_MF_UCI:
			row[2] = g_strdup(_("Email"));
			break;

		case GN_SMS_MF_ERMES:
			row[2] = g_strdup(_("ERMES"));
			break;

		case GN_SMS_MF_X400:
			row[2] = g_strdup(_("X.400"));
			break;

		case GN_SMS_MF_Voice:
			row[2] = g_strdup(_("Voice"));
			break;

		default:
			row[2] = g_strdup(_("Text"));
			break;
		}

		switch (configDialogData.sms.smsSetting[i].validity) {
		case GN_SMS_VP_1H:
			row[3] = g_strdup(_("1 hour"));
			break;

		case GN_SMS_VP_6H:
			row[3] = g_strdup(_("6 hours"));
			break;

		case GN_SMS_VP_24H:
			row[3] = g_strdup(_("24 hours"));
			break;

		case GN_SMS_VP_72H:
			row[3] = g_strdup(_("72 hours"));
			break;

		case GN_SMS_VP_1W:
			row[3] = g_strdup(_("1 week"));
			break;

		case GN_SMS_VP_Max:
			row[3] = g_strdup(_("Max. Time"));
			break;

		default:
			row[3] = g_strdup(_("24 hours"));
			break;
		}

		gtk_clist_append(GTK_CLIST(SMSClist), row);

		for (j = 0; j < 4; j++)
			g_free(row[j]);
	}

	gtk_clist_thaw(GTK_CLIST(SMSClist));
}


static void RefreshUserStatus(void)
{
	gchar buf[8];
	configDialogData.user.used = GTK_ENTRY(configDialogData.user.name)->text_length
	    + GTK_ENTRY(configDialogData.user.title)->text_length
	    + GTK_ENTRY(configDialogData.user.company)->text_length
	    + GTK_ENTRY(configDialogData.user.telephone)->text_length
	    + GTK_ENTRY(configDialogData.user.fax)->text_length
	    + GTK_ENTRY(configDialogData.user.email)->text_length
	    + GTK_ENTRY(configDialogData.user.address)->text_length;
	configDialogData.user.max = MAX_BUSINESS_CARD_LENGTH;
	if (GTK_ENTRY(configDialogData.user.telephone)->text_length > 0)
		configDialogData.user.max -= 4;
	if (GTK_ENTRY(configDialogData.user.fax)->text_length > 0)
		configDialogData.user.max -= 4;
	g_snprintf(buf, 8, "%d/%d", configDialogData.user.used, configDialogData.user.max);
	gtk_label_set_text(GTK_LABEL(configDialogData.user.status), buf);
}


void GUI_InitSMSSettings(void)
{
	PhoneEvent *e;
	D_SMSCenter *c;
	register gint i;

	if (SMSSettingsInitialized)
		return;

	gtk_label_set_text(GTK_LABEL(infoDialog.text), _("Reading SMS centers ..."));
	gtk_widget_show_now(infoDialog.dialog);
	GUI_Refresh();

	for (i = 1; i <= MAX_SMS_CENTER; i++) {
		xgnokiiConfig.smsSetting[i - 1].id = i;
		c = (D_SMSCenter *) g_malloc(sizeof(D_SMSCenter));
		c->center = &(xgnokiiConfig.smsSetting[i - 1]);

		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_GetSMSCenter;
		e->data = c;
		GUI_InsertEvent(e);
		pthread_mutex_lock(&smsCenterMutex);
		pthread_cond_wait(&smsCenterCond, &smsCenterMutex);
		pthread_mutex_unlock(&smsCenterMutex);

		if (c->status != GN_ERR_NONE)
			break;

		g_free(c);

		configDialogData.sms.smsSetting[i - 1] = xgnokiiConfig.smsSetting[i - 1];
	}

	xgnokiiConfig.smsSets = i - 1;

	ParseSMSCenters();

	SMSSettingsInitialized = TRUE;

	gtk_widget_hide(infoDialog.dialog);
}


void GUI_ShowOptions(void)
{
	PhoneEvent *e;
	D_Alarm *alarm;
	register gint i;

	if (optionsDialogIsOpened) {
		gtk_window_present(GTK_WINDOW(OptionsDialog));
		return;
	}

	gtk_entry_set_text(GTK_ENTRY(configDialogData.connection.port), xgnokiiConfig.port);

	gtk_entry_set_text(GTK_ENTRY(configDialogData.connection.model), xgnokiiConfig.model);

	gtk_entry_set_text(GTK_ENTRY(configDialogData.connection.bindir), xgnokiiConfig.bindir);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.connection.init),
				  (gdouble) xgnokiiConfig.initlength);

	if (xgnokiiConfig.connection == GN_CT_Irda) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialogData.connection.irda),
					     TRUE);
	} else if (xgnokiiConfig.connection == GN_CT_Infrared || xgnokiiConfig.connection == GN_CT_Tekram) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (configDialogData.connection.infrared), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialogData.connection.serial),
					     TRUE);
	}

	/* Phone */
	gtk_entry_set_text(GTK_ENTRY(configDialogData.phone.model), phoneMonitor.phone.model);

	gtk_entry_set_text(GTK_ENTRY(configDialogData.phone.version), phoneMonitor.phone.product_name);

	gtk_entry_set_text(GTK_ENTRY(configDialogData.phone.revision), phoneMonitor.phone.revision);

	gtk_entry_set_text(GTK_ENTRY(configDialogData.phone.imei), phoneMonitor.phone.imei);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.phone.simNameLen),
				  atof(xgnokiiConfig.maxSIMLen));

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.phone.phoneNameLen),
				  atof(xgnokiiConfig.maxPhoneLen));

	/* Alarm */
	alarm = (D_Alarm *) g_malloc(sizeof(D_Alarm));
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetAlarm;
	e->data = alarm;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&alarmMutex);
	pthread_cond_wait(&alarmCond, &alarmMutex);
	pthread_mutex_unlock(&alarmMutex);

	if (alarm->status != GN_ERR_NONE) {
		xgnokiiConfig.alarmSupported = FALSE;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialogData.alarm.alarmSwitch),
					     FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.alarm.alarmHour), 0.0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.alarm.alarmMin), 0.0);
	} else {
		xgnokiiConfig.alarmSupported = TRUE;
		if (alarm->alarm.enabled)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
						     (configDialogData.alarm.alarmSwitch), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
						     (configDialogData.alarm.alarmSwitch), FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.alarm.alarmHour),
					  alarm->alarm.timestamp.hour);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(configDialogData.alarm.alarmMin),
					  alarm->alarm.timestamp.minute);
	}
	g_free(alarm);

	/* SMS */
	if (phoneMonitor.supported & PM_SMS) {
		gtk_widget_show(sms_option_frame);
		GUI_InitSMSSettings();
	} else
		gtk_widget_hide(sms_option_frame);


	/* BUSINESS CARD */
	if (phoneMonitor.supported & PM_SMS) {
		gtk_widget_show(user_option_frame);

		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.name), xgnokiiConfig.user.name);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.title),
				   xgnokiiConfig.user.title);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.company),
				   xgnokiiConfig.user.company);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.telephone),
				   xgnokiiConfig.user.telephone);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.fax), xgnokiiConfig.user.fax);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.email),
				   xgnokiiConfig.user.email);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.user.address),
				   xgnokiiConfig.user.address);

		RefreshUserStatus();
	} else
		gtk_widget_hide(user_option_frame);


	/* Groups */
	if (phoneMonitor.supported & PM_CALLERGROUP) {
		gtk_widget_show(cg_names_option_frame);
		GUI_InitCallerGroupsInf();
		for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER; i++)
			gtk_entry_set_text(GTK_ENTRY(configDialogData.groups[i]),
					   xgnokiiConfig.callerGroups[i]);
	} else
		gtk_widget_hide(cg_names_option_frame);

	/* Mail */
	if (phoneMonitor.supported & PM_SMS) {
		gtk_widget_show(mail_option_frame);
		gtk_entry_set_text(GTK_ENTRY(configDialogData.mailbox), xgnokiiConfig.mailbox);;
	} else
		gtk_widget_hide(mail_option_frame);

	optionsDialogIsOpened = TRUE;
	gtk_widget_show(OptionsDialog);
}


inline void GUI_ShowAbout(void)
{
	gtk_window_present(GTK_WINDOW(AboutDialog));
}


inline void GUI_HideAbout(void)
{
	gtk_widget_hide(AboutDialog);
}


static void _MainExit(void)
{
	PhoneEvent *e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));

	e->event = Event_Exit;
	e->data = NULL;
	GUI_InsertEvent(e);
	pthread_join(monitor_th, NULL);
	gtk_main_quit();
}

void MainExit(gchar *ermsg)
{
	if (ermsg) {
		/* Nasty workaround -- that's some race condition that makes sometimes xgnokii to hang on exit */
		sleep(1);
		gtk_label_set_text(GTK_LABEL(infoDialog.text), ermsg);
		gtk_widget_show_now(infoDialog.dialog);
		GUI_Refresh();
		sleep(10);
	}
	_MainExit();
}


static void ShowMenu(GdkEventButton * event)
{
	GdkEventButton *bevent = (GdkEventButton *) event;

	if (phoneMonitor.supported & PM_KEYBOARD)
		gtk_widget_show(xkeyb_menu_item);
	else
		gtk_widget_hide(xkeyb_menu_item);

	if (phoneMonitor.supported & PM_NETMONITOR)
		gtk_widget_show(netmon_menu_item);
	else
		gtk_widget_hide(netmon_menu_item);

	if (phoneMonitor.supported & PM_SMS)
		gtk_widget_show(sms_menu_item);
	else
		gtk_widget_hide(sms_menu_item);

	if (phoneMonitor.supported & PM_CALENDAR)
		gtk_widget_show(calendar_menu_item);
	else
		gtk_widget_hide(calendar_menu_item);

	if (phoneMonitor.supported & PM_DTMF)
		gtk_widget_show(dtmf_menu_item);
	else
		gtk_widget_hide(dtmf_menu_item);

	if (phoneMonitor.supported & PM_SPEEDDIAL)
		gtk_widget_show(speedDial_menu_item);
	else
		gtk_widget_hide(speedDial_menu_item);

	if (phoneMonitor.supported & PM_DATA)
		gtk_widget_show(data_menu_item);
	else
		gtk_widget_hide(data_menu_item);

	gtk_widget_show(xring_menu_item);

	gtk_menu_popup(GTK_MENU(Menu), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
}


static gint ButtonPressEvent(GtkWidget * widget, GdkEventButton * event)
{
	/* Left button */
	if (event->button == 1) {

		/* Close */
		if (event->x >= 206 && event->x <= 221 && event->y >= 42 && event->y <= 55) {
			if (GUI_ContactsIsChanged())
				GUI_QuitSaveContacts();
			else
				MainExit(NULL);
		} else if ((event->x >= 180 && event->x <= 195 &&
			    event->y >= 30 && event->y <= 50) ||
			   (event->x >= 185 && event->x <= 215 &&
			    event->y >= 15 && event->y <= 30)) {
			GUI_ShowContacts();
		} else if (event->x >= 190 && event->x <= 210 && event->y >= 70 && event->y <= 85) {
			if ((!phoneMonitor.supported) & PM_SMS)
				phoneMonitor.working = _("SMS not supported!");
			else
				GUI_ShowSMS();
		} else if (event->x >= 235 && event->x <= 248 && event->y >= 27 && event->y <= 75) {
			if ((!phoneMonitor.supported) & PM_CALENDAR)
				phoneMonitor.working = _("Calendar not supported!");
			else
				GUI_ShowCalendar();
		} else if (event->x >= 245 && event->x <= 258 && event->y >= 83 && event->y <= 93) {
			GUI_ShowOptions();
		}
	} /* Right button */
	else if (event->button == 3)
		ShowMenu(event);

	// g_print ("event->x: %f\n", event->x);
	// g_print ("event->y: %f\n", event->y);

	return TRUE;
}


static void OptionsApplyCallback(GtkWidget * widget, gpointer data)
{
	PhoneEvent *e;
	D_Alarm *alarm;
	register gint i;

	/* Phone */
	max_phonebook_sim_name_length = GNOKII_MIN(GN_PHONEBOOK_NAME_MAX_LENGTH,
	    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(configDialogData.phone.simNameLen))) + 1;
	max_phonebook_name_length = GNOKII_MIN(GN_PHONEBOOK_NAME_MAX_LENGTH,
	    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(configDialogData.phone.phoneNameLen))) + 1;
	g_free(xgnokiiConfig.maxSIMLen);
	g_free(xgnokiiConfig.maxPhoneLen);
	xgnokiiConfig.maxSIMLen = g_strdup_printf("%d", max_phonebook_sim_name_length - 1);
	xgnokiiConfig.maxPhoneLen = g_strdup_printf("%d", max_phonebook_name_length - 1);

	/* ALARM */
	/* From fbus-6110.c 
	   FIXME: we should also allow to set the alarm off :-) */
	if (xgnokiiConfig.alarmSupported
	    && GTK_TOGGLE_BUTTON(configDialogData.alarm.alarmSwitch)->active) {
		alarm = (D_Alarm *) g_malloc(sizeof(D_Alarm));
		alarm->alarm.timestamp.hour =
		    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						     (configDialogData.alarm.alarmHour));
		alarm->alarm.timestamp.minute =
		    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						     (configDialogData.alarm.alarmMin));
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SetAlarm;
		e->data = alarm;
		GUI_InsertEvent(e);
	}

	/* SMS */
	if (phoneMonitor.supported & PM_SMS) {
		for (i = 0; i < xgnokiiConfig.smsSets; i++)
			xgnokiiConfig.smsSetting[i] = configDialogData.sms.smsSetting[i];
		GUIEventSend(GUI_EVENT_SMS_CENTERS_CHANGED);
	}

	/* BUSINESS CARD */
	if (phoneMonitor.supported & PM_SMS) {
		g_free(xgnokiiConfig.user.name);
		xgnokiiConfig.user.name =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.name)));
		g_free(xgnokiiConfig.user.title);
		xgnokiiConfig.user.title =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.title)));
		g_free(xgnokiiConfig.user.company);
		xgnokiiConfig.user.company =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.company)));
		g_free(xgnokiiConfig.user.telephone);
		xgnokiiConfig.user.telephone =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.telephone)));
		g_free(xgnokiiConfig.user.fax);
		xgnokiiConfig.user.fax =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.fax)));
		g_free(xgnokiiConfig.user.email);
		xgnokiiConfig.user.email =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.email)));
		g_free(xgnokiiConfig.user.address);
		xgnokiiConfig.user.address =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.user.address)));
	}

	/* GROUPS */
	if (phoneMonitor.supported & PM_CALLERGROUP) {
		for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER; i++) {
			strncpy(xgnokiiConfig.callerGroups[i],
				gtk_entry_get_text(GTK_ENTRY(configDialogData.groups[i])),
				MAX_CALLER_GROUP_LENGTH);
			xgnokiiConfig.callerGroups[i][MAX_CALLER_GROUP_LENGTH] = '\0';
		}
		GUIEventSend(GUI_EVENT_CALLERS_GROUPS_CHANGED);
		GUIEventSend(GUI_EVENT_CONTACTS_CHANGED);
	}

	/* Mail */
	if (phoneMonitor.supported & PM_SMS) {
		g_free(xgnokiiConfig.mailbox);
		xgnokiiConfig.mailbox =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(configDialogData.mailbox)));
	}
}


static void OptionsSaveCallback(GtkWidget * widget, gpointer data)
{
	D_CallerGroup *cg;
	D_SMSCenter *c;
	PhoneEvent *e;
	register gint i;

	//gtk_widget_hide(GTK_WIDGET(data));
	OptionsApplyCallback(widget, data);
	for (i = 0; i < xgnokiiConfig.smsSets; i++) {
		xgnokiiConfig.smsSetting[i].id = i + 1;
		c = (D_SMSCenter *) g_malloc(sizeof(D_SMSCenter));
		c->center = &(xgnokiiConfig.smsSetting[i]);
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SetSMSCenter;
		e->data = c;
		GUI_InsertEvent(e);
	}

	if (phoneMonitor.supported & PM_CALLERGROUP) {
		cg = (D_CallerGroup *) g_malloc(sizeof(D_CallerGroup));
		cg->number = 0;
		if (strcmp(xgnokiiConfig.callerGroups[0], _("Family")) == 0)
			*cg->text = '\0';
		else
			strncpy(cg->text, xgnokiiConfig.callerGroups[0], 256);
		cg->text[255] = '\0';
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SendCallerGroup;
		e->data = cg;
		GUI_InsertEvent(e);

		cg = (D_CallerGroup *) g_malloc(sizeof(D_CallerGroup));
		cg->number = 1;
		if (strcmp(xgnokiiConfig.callerGroups[1], _("VIP")) == 0)
			*cg->text = '\0';
		else
			strncpy(cg->text, xgnokiiConfig.callerGroups[1], 256);
		cg->text[255] = '\0';
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SendCallerGroup;
		e->data = cg;
		GUI_InsertEvent(e);

		cg = (D_CallerGroup *) g_malloc(sizeof(D_CallerGroup));
		cg->number = 2;
		if (strcmp(xgnokiiConfig.callerGroups[2], _("Friends")) == 0)
			*cg->text = '\0';
		else
			strncpy(cg->text, xgnokiiConfig.callerGroups[2], 256);
		cg->text[255] = '\0';
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SendCallerGroup;
		e->data = cg;
		GUI_InsertEvent(e);

		cg = (D_CallerGroup *) g_malloc(sizeof(D_CallerGroup));
		cg->number = 3;
		if (strcmp(xgnokiiConfig.callerGroups[3], _("Colleagues")) == 0)
			*cg->text = '\0';
		else
			strncpy(cg->text, xgnokiiConfig.callerGroups[3], 256);
		cg->text[255] = '\0';
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SendCallerGroup;
		e->data = cg;
		GUI_InsertEvent(e);

		cg = (D_CallerGroup *) g_malloc(sizeof(D_CallerGroup));
		cg->number = 4;
		if (strcmp(xgnokiiConfig.callerGroups[4], _("Other")) == 0)
			*cg->text = '\0';
		else
			strncpy(cg->text, xgnokiiConfig.callerGroups[4], 256);
		cg->text[255] = '\0';
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_SendCallerGroup;
		e->data = cg;
		GUI_InsertEvent(e);
	}

	if (GUI_SaveXConfig()) {
		gtk_label_set_text(GTK_LABEL(errorDialog.text),
				   _("Error writing configuration file!"));
		gtk_widget_show(errorDialog.dialog);
	}
}


static GtkWidget *CreateMenu(void)
{
	GtkWidget *menu, *menu_items;

	menu = gtk_menu_new();

	menu_items = gtk_menu_item_new_with_label(_("Contacts"));
	gtk_menu_append(GTK_MENU(menu), menu_items);
	gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowContacts), NULL);
	gtk_widget_show(menu_items);

	sms_menu_item = gtk_menu_item_new_with_label(_("SMS"));
	gtk_menu_append(GTK_MENU(menu), sms_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(sms_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowSMS), NULL);

	calendar_menu_item = gtk_menu_item_new_with_label(_("Calendar"));
	gtk_menu_append(GTK_MENU(menu), calendar_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(calendar_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowCalendar), NULL);

	logos_menu_item = gtk_menu_item_new_with_label(_("Logos"));
	gtk_menu_append(GTK_MENU(menu), logos_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(logos_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowLogosWindow), NULL);
	gtk_widget_show(logos_menu_item);

	dtmf_menu_item = gtk_menu_item_new_with_label(_("DTMF"));
	gtk_menu_append(GTK_MENU(menu), dtmf_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(dtmf_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowDTMF), NULL);

	speedDial_menu_item = gtk_menu_item_new_with_label(_("Speed Dial"));
	gtk_menu_append(GTK_MENU(menu), speedDial_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(speedDial_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowSpeedDial), NULL);

	xkeyb_menu_item = gtk_menu_item_new_with_label(_("Keyboard"));
	gtk_menu_append(GTK_MENU(menu), xkeyb_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(xkeyb_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowXkeyb), NULL);

	netmon_menu_item = gtk_menu_item_new_with_label(_("Net Monitor"));
	gtk_menu_append(GTK_MENU(menu), netmon_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(netmon_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowNetmon), NULL);

	xring_menu_item = gtk_menu_item_new_with_label(_("Ringtone"));
	gtk_menu_append(GTK_MENU(menu), xring_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(xring_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowXring), NULL);

	data_menu_item = gtk_menu_item_new_with_label(_("Data calls"));
	gtk_menu_append(GTK_MENU(menu), data_menu_item);
	gtk_signal_connect_object(GTK_OBJECT(data_menu_item), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowData), NULL);

	menu_items = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menu_items);
	gtk_widget_show(menu_items);

	menu_items = gtk_menu_item_new_with_label(_("Options"));
	gtk_menu_append(GTK_MENU(menu), menu_items);
	gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowOptions), NULL);
	gtk_widget_show(menu_items);

	menu_items = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menu_items);
	gtk_widget_show(menu_items);

	menu_items = gtk_menu_item_new_with_label(_("About"));
	gtk_menu_append(GTK_MENU(menu), menu_items);
	gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate",
				  GTK_SIGNAL_FUNC(GUI_ShowAbout), NULL);
	gtk_widget_show(menu_items);

	return menu;
}


static GtkWidget *CreateAboutDialog(void)
{
	GtkWidget *dialog;
	GtkWidget *button, *hbox, *label;
	gchar buf[2000];

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("About"));
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
	gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	button = gtk_button_new_with_label(_("Ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(GUI_HideAbout), NULL);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	g_snprintf(buf, 2000, _("xgnokii version: %s\ngnokii version: %s\n\n\
Copyright (C) 1999-2004 Pavel Janik ml.,\nHugh Blemings, Jan Derfinak,\n\
Pawel Kot and others\n\
xgnokii is free software, covered by the GNU General Public License,\n\
and you are welcome to change it and/or distribute copies of it under\n\
certain conditions. There is absolutely no warranty for xgnokii.\n\
See GPL for details.\n"), XVERSION, VERSION);
	label = gtk_label_new((gchar *) buf);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return dialog;
}


static inline void SetFormat(GtkWidget * item, gpointer data)
{
	tempMessageSettings.format = GPOINTER_TO_INT(data);
}


static inline void SetValidity(GtkWidget * item, gpointer data)
{
	tempMessageSettings.validity = GPOINTER_TO_INT(data);
}


static inline void OptionsDeleteEvent(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(widget));
	optionsDialogIsOpened = FALSE;
}


static inline void OptionsCloseCallback(GtkWidget * widget, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	optionsDialogIsOpened = FALSE;
}


static gint CheckInUserDataLength(GtkWidget * widget, GdkEvent * event, gpointer callback_data)
{
	register gint len;

	len = configDialogData.user.max - (GTK_ENTRY(configDialogData.user.name)->text_length
					   + GTK_ENTRY(configDialogData.user.title)->text_length
					   + GTK_ENTRY(configDialogData.user.company)->text_length
					   + GTK_ENTRY(configDialogData.user.telephone)->text_length
					   + GTK_ENTRY(configDialogData.user.fax)->text_length
					   + GTK_ENTRY(configDialogData.user.email)->text_length
					   + GTK_ENTRY(configDialogData.user.address)->text_length
					   - GTK_ENTRY(widget)->text_length);

	if (len < 1) {
		gtk_entry_set_editable(GTK_ENTRY(widget), FALSE);
		return (FALSE);
	} else
		gtk_entry_set_editable(GTK_ENTRY(widget), TRUE);
	if (GPOINTER_TO_INT(callback_data) == 3 || GPOINTER_TO_INT(callback_data) == 4) {
		if ((GPOINTER_TO_INT(callback_data) == 3
		     && GTK_ENTRY(configDialogData.user.telephone)->text_length == 0)
		    || (GPOINTER_TO_INT(callback_data) == 4
			&& GTK_ENTRY(configDialogData.user.fax)->text_length == 0))
			len -= 4;

		if (len < 1) {
			gtk_entry_set_editable(GTK_ENTRY(widget), FALSE);
			return (FALSE);
		}

		if (len > max_phonebook_number_length)
			len = max_phonebook_number_length;
	}

	gtk_entry_set_max_length(GTK_ENTRY(widget), len);
	return (FALSE);
}


static inline gint CheckOutUserDataLength(GtkWidget * widget,
					  GdkEvent * event, gpointer callback_data)
{
	gtk_entry_set_max_length(GTK_ENTRY(widget), GPOINTER_TO_INT(callback_data));
	return (FALSE);
}


static inline gint RefreshUserStatusCallBack(GtkWidget * widget,
					     GdkEventKey * event, gpointer callback_data)
{
	RefreshUserStatus();
	if (gtk_editable_get_editable (GTK_EDITABLE(widget)) == FALSE)
		return (FALSE);
	if (event->keyval == GDK_BackSpace || event->keyval == GDK_Clear ||
	    event->keyval == GDK_Insert || event->keyval == GDK_Delete ||
	    event->keyval == GDK_Home || event->keyval == GDK_End ||
	    event->keyval == GDK_Left || event->keyval == GDK_Right ||
	    event->keyval == GDK_Return || (event->keyval >= 0x20 && event->keyval <= 0xFF))
		return (TRUE);

	return (FALSE);
}


static void OkEditSMSSetDialog(GtkWidget * w, gpointer data)
{

	strncpy(configDialogData.sms.smsSetting
		[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].name,
		gtk_entry_get_text(GTK_ENTRY(configDialogData.sms.set)),
		GN_SMS_CENTER_NAME_MAX_LENGTH);
	configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].
	    name[GN_SMS_CENTER_NAME_MAX_LENGTH - 1]
	    = '\0';

	strncpy(configDialogData.sms.smsSetting
		[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].smsc.number,
		gtk_entry_get_text(GTK_ENTRY(configDialogData.sms.number)),
		GN_BCD_STRING_MAX_LENGTH);
	configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].
	    smsc.number[GN_BCD_STRING_MAX_LENGTH - 1]
	    = '\0';

	configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].
	    format = tempMessageSettings.format;

	configDialogData.sms.smsSetting[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].
	    validity = tempMessageSettings.validity;

	ParseSMSCenters();

	gtk_widget_hide(GTK_WIDGET(data));
}


static inline void EditSMSSetDialogClick(GtkWidget * clist,
					 gint row,
					 gint column, GdkEventButton * event, GtkWidget * data)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		gtk_signal_emit_by_name(GTK_OBJECT(data), "clicked");
}


static void ShowEditSMSSetDialog(GtkWidget * w, gpointer data)
{
	static GtkWidget *dialog = NULL;
	GtkWidget *button, *label, *hbox, *menu, *item;

	if (GTK_CLIST(SMSClist)->selection == NULL)
		return;

	if (dialog == NULL) {
		dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(dialog), _("Edit SMS Setting"));
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
		gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkEditSMSSetDialog), (gpointer) dialog);
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

		label = gtk_label_new(_("Set's name:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		configDialogData.sms.set =
		    gtk_entry_new_with_max_length(GN_SMS_CENTER_NAME_MAX_LENGTH - 1);
		gtk_widget_set_usize(configDialogData.sms.set, 110, 22);
		gtk_box_pack_end(GTK_BOX(hbox), configDialogData.sms.set, FALSE, FALSE, 2);
		gtk_widget_show(configDialogData.sms.set);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Center:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		configDialogData.sms.number =
		    gtk_entry_new_with_max_length(GN_BCD_STRING_MAX_LENGTH - 1);
		gtk_widget_set_usize(configDialogData.sms.number, 110, 22);
		gtk_box_pack_end(GTK_BOX(hbox), configDialogData.sms.number, FALSE, FALSE, 2);
		gtk_widget_show(configDialogData.sms.number);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 9);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Sending Format:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		configDialogData.sms.format = gtk_option_menu_new();
		menu = gtk_menu_new();
		gtk_widget_set_usize(configDialogData.sms.format, 110, 28);

		item = gtk_menu_item_new_with_label(_("Text"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetFormat), (gpointer) GN_SMS_MF_Text);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("Fax"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetFormat), (gpointer) GN_SMS_MF_Fax);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("Paging"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetFormat), (gpointer) GN_SMS_MF_Paging);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("Email"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetFormat), (gpointer) GN_SMS_MF_Email);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		gtk_option_menu_set_menu(GTK_OPTION_MENU(configDialogData.sms.format), menu);
		gtk_box_pack_end(GTK_BOX(hbox), configDialogData.sms.format, FALSE, FALSE, 2);
		gtk_widget_show(configDialogData.sms.format);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 9);
		gtk_widget_show(hbox);

		label = gtk_label_new(_("Validity Period:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		configDialogData.sms.validity = gtk_option_menu_new();
		menu = gtk_menu_new();
		gtk_widget_set_usize(configDialogData.sms.validity, 110, 28);

		item = gtk_menu_item_new_with_label(_("Max. Time"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetValidity), (gpointer) GN_SMS_VP_Max);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("1 hour"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetValidity), (gpointer) GN_SMS_VP_1H);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("6 hours"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetValidity), (gpointer) GN_SMS_VP_6H);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("24 hours"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetValidity), (gpointer) GN_SMS_VP_24H);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("72 hours"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetValidity), (gpointer) GN_SMS_VP_72H);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		item = gtk_menu_item_new_with_label(_("1 week"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetValidity), (gpointer) GN_SMS_VP_1W);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		gtk_option_menu_set_menu(GTK_OPTION_MENU(configDialogData.sms.validity), menu);
		gtk_box_pack_end(GTK_BOX(hbox), configDialogData.sms.validity, FALSE, FALSE, 2);
		gtk_widget_show(configDialogData.sms.validity);
	}

	gtk_entry_set_text(GTK_ENTRY(configDialogData.sms.set),
			   configDialogData.sms.smsSetting
			   [GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].name);

	gtk_entry_set_text(GTK_ENTRY(configDialogData.sms.number),
			   configDialogData.sms.smsSetting
			   [GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].smsc.number);

	switch (configDialogData.sms.smsSetting
		[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].format) {
	case GN_SMS_MF_Text:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.format), 0);
		break;

	case GN_SMS_MF_Paging:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.format), 2);
		break;

	case GN_SMS_MF_Fax:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.format), 1);
		break;

	case GN_SMS_MF_Email:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.format), 3);
		break;

	default:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.format), 0);
	}

	switch (configDialogData.sms.smsSetting
		[GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].validity) {
	case GN_SMS_VP_1H:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 1);
		break;

	case GN_SMS_VP_6H:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 2);
		break;

	case GN_SMS_VP_24H:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 3);
		break;

	case GN_SMS_VP_72H:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 4);
		break;

	case GN_SMS_VP_1W:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 5);
		break;

	case GN_SMS_VP_Max:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 0);
		break;

	default:
		gtk_option_menu_set_history(GTK_OPTION_MENU(configDialogData.sms.validity), 3);
	}

	tempMessageSettings.format = configDialogData.sms.smsSetting
	    [GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].format;
	tempMessageSettings.validity = configDialogData.sms.smsSetting
	    [GPOINTER_TO_INT(GTK_CLIST(SMSClist)->selection->data)].validity;

	gtk_widget_show(dialog);
}


static GtkWidget *CreateOptionsDialog(void)
{
	gchar labelBuffer[10];
	GtkWidget *dialog;
	GtkWidget *button, *hbox, *vbox, *label, *notebook, *frame, *clistScrolledWindow;
	register gint i;
	GtkAdjustment *adj;
	gchar *titles[4] = { _("Set's name"), _("Center number"), _("Format"), _("Validity") };

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Options"));
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
	gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
			   GTK_SIGNAL_FUNC(OptionsDeleteEvent), NULL);

	button = gtk_button_new_with_label(_("Apply"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(OptionsApplyCallback), (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Save"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(OptionsSaveCallback), (gpointer) dialog);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Close"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(OptionsCloseCallback), (gpointer) dialog);
	gtk_widget_show(button);

	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 5);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), notebook);
	gtk_widget_show(notebook);

  /***  Connection notebook  ***/
	frame = gtk_frame_new(_("Phone and connection type"));
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("Connection"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Port:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.connection.port = gtk_entry_new_with_max_length(GN_DEVICE_NAME_MAX_LENGTH);
	gtk_widget_set_usize(configDialogData.connection.port, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.connection.port), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.connection.port, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.port);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Model:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.connection.model = gtk_entry_new_with_max_length(GN_MODEL_MAX_LENGTH);
	gtk_widget_set_usize(configDialogData.connection.model, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.connection.model), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.connection.model, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.model);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Bindir:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.connection.bindir = gtk_entry_new_with_max_length(100);
	gtk_widget_set_usize(configDialogData.connection.bindir, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.connection.bindir), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.connection.bindir, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.bindir);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Init length:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1000, 1.0, 10.0, 0.0);
	configDialogData.connection.init = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(configDialogData.connection.init), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(configDialogData.connection.init), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), configDialogData.connection.init, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.init);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Connection:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.connection.infrared = gtk_radio_button_new_with_label(NULL, _("infrared"));
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.connection.infrared, TRUE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.infrared);

	configDialogData.connection.serial =
	    gtk_radio_button_new_with_label(gtk_radio_button_group
					    (GTK_RADIO_BUTTON
					     (configDialogData.connection.infrared)), _("serial"));
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.connection.serial, TRUE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.serial);

	configDialogData.connection.irda =
	    gtk_radio_button_new_with_label(gtk_radio_button_group
					    (GTK_RADIO_BUTTON
					     (configDialogData.connection.infrared)), _("irda"));
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.connection.irda, TRUE, FALSE, 2);
	gtk_widget_show(configDialogData.connection.irda);

  /***  Phone notebook  ***/
	frame = gtk_frame_new(_("Phone information"));
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("Phone"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Model:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.phone.model = gtk_entry_new_with_max_length(GN_MODEL_MAX_LENGTH);
	gtk_widget_set_usize(configDialogData.phone.model, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.phone.model), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.phone.model, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.phone.model);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Version:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.phone.version = gtk_entry_new_with_max_length(GN_MODEL_MAX_LENGTH);
	gtk_widget_set_usize(configDialogData.phone.version, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.phone.version), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.phone.version, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.phone.version);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Revision:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.phone.revision = gtk_entry_new_with_max_length(GN_REVISION_MAX_LENGTH);
	gtk_widget_set_usize(configDialogData.phone.revision, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.phone.revision), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.phone.revision, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.phone.revision);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("IMEI:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.phone.imei = gtk_entry_new_with_max_length(GN_IMEI_MAX_LENGTH);
	gtk_widget_set_usize(configDialogData.phone.imei, 220, 22);
	gtk_entry_set_editable(GTK_ENTRY(configDialogData.phone.imei), FALSE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.phone.imei, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.phone.imei);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Names length:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("SIM:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 1.0, GN_PHONEBOOK_NAME_MAX_LENGTH, 1.0, 10.0, 0.0);
	configDialogData.phone.simNameLen = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(configDialogData.phone.simNameLen), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(configDialogData.phone.simNameLen), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), configDialogData.phone.simNameLen, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.phone.simNameLen);

	adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 1.0, GN_PHONEBOOK_NAME_MAX_LENGTH, 1.0, 10.0, 0.0);
	configDialogData.phone.phoneNameLen = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(configDialogData.phone.phoneNameLen), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(configDialogData.phone.phoneNameLen), TRUE);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.phone.phoneNameLen, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.phone.phoneNameLen);

	label = gtk_label_new(_("Phone:"));
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

  /***  Alarm notebook  ***/
	xgnokiiConfig.alarmSupported = TRUE;

	frame = gtk_frame_new(_("Alarm setting"));
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("Alarm"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
	gtk_widget_show(hbox);

	configDialogData.alarm.alarmSwitch = gtk_check_button_new_with_label(_("Alarm"));
	gtk_box_pack_start(GTK_BOX(hbox), configDialogData.alarm.alarmSwitch, FALSE, FALSE, 10);
	gtk_widget_show(configDialogData.alarm.alarmSwitch);

	adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 23.0, 1.0, 4.0, 0.0);
	configDialogData.alarm.alarmHour = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(configDialogData.alarm.alarmHour), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(configDialogData.alarm.alarmHour), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), configDialogData.alarm.alarmHour, FALSE, FALSE, 0);
	gtk_widget_show(configDialogData.alarm.alarmHour);

	label = gtk_label_new(":");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 59.0, 1.0, 10.0, 0.0);
	configDialogData.alarm.alarmMin = gtk_spin_button_new(adj, 0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(configDialogData.alarm.alarmMin), TRUE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(configDialogData.alarm.alarmMin), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), configDialogData.alarm.alarmMin, FALSE, FALSE, 0);
	gtk_widget_show(configDialogData.alarm.alarmMin);

  /***  SMS notebook     ***/
	sms_option_frame = gtk_frame_new(_("Short Message Service"));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(sms_option_frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("SMS"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sms_option_frame, label);

	SMSClist = gtk_clist_new_with_titles(4, titles);
	gtk_clist_set_shadow_type(GTK_CLIST(SMSClist), GTK_SHADOW_OUT);
	gtk_clist_column_titles_passive(GTK_CLIST(SMSClist));
	gtk_clist_set_auto_sort(GTK_CLIST(SMSClist), FALSE);

	gtk_clist_set_column_width(GTK_CLIST(SMSClist), 0, 70);
	gtk_clist_set_column_width(GTK_CLIST(SMSClist), 1, 115);
	gtk_clist_set_column_width(GTK_CLIST(SMSClist), 2, 40);
	gtk_clist_set_column_width(GTK_CLIST(SMSClist), 3, 55);
//  gtk_clist_set_column_justification (GTK_CLIST (SMSClist), 1, GTK_JUSTIFY_RIGHT);

	clistScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(clistScrolledWindow), SMSClist);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(clistScrolledWindow),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), clistScrolledWindow, TRUE, TRUE, 10);

	gtk_widget_show(SMSClist);
	gtk_widget_show(clistScrolledWindow);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 9);
	gtk_widget_show(hbox);

	button = gtk_button_new_with_label(_("Edit"));
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(ShowEditSMSSetDialog), (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	gtk_signal_connect(GTK_OBJECT(SMSClist), "select_row",
			   GTK_SIGNAL_FUNC(EditSMSSetDialogClick), (gpointer) button);

  /***  Business notebook  ***/
	user_option_frame = gtk_frame_new(_("Business Card"));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(user_option_frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("User"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), user_option_frame, label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	configDialogData.user.status = gtk_label_new("");
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.status, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.status);

	configDialogData.user.max = MAX_BUSINESS_CARD_LENGTH;
	configDialogData.user.used = 0;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Name:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.name = gtk_entry_new_with_max_length(configDialogData.user.max);
	gtk_widget_set_usize(configDialogData.user.name, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.name),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.name),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.name),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 0);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.name),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) configDialogData.user.max);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.name, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.name);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Title:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.title = gtk_entry_new_with_max_length(configDialogData.user.max);
	gtk_widget_set_usize(configDialogData.user.title, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.title),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.title),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.title),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 1);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.title),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) configDialogData.user.max);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.title, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.title);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Company:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.company = gtk_entry_new_with_max_length(configDialogData.user.max);
	gtk_widget_set_usize(configDialogData.user.company, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.company),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.company),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.company),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 2);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.company),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) configDialogData.user.max);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.company, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.company);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Telephone:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.telephone =
	    gtk_entry_new_with_max_length(max_phonebook_number_length);
	gtk_widget_set_usize(configDialogData.user.telephone, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.telephone),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.telephone),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.telephone),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 3);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.telephone),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) max_phonebook_number_length);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.telephone, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.telephone);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Fax:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.fax = gtk_entry_new_with_max_length(max_phonebook_number_length);
	gtk_widget_set_usize(configDialogData.user.fax, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.fax),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.fax),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.fax),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 4);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.fax),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) max_phonebook_number_length);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.fax, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.fax);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Email:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.email = gtk_entry_new_with_max_length(configDialogData.user.max);
	gtk_widget_set_usize(configDialogData.user.email, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.email),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.email),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.email),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 5);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.email),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) configDialogData.user.max);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.email, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.email);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Address:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.user.address = gtk_entry_new_with_max_length(configDialogData.user.max);
	gtk_widget_set_usize(configDialogData.user.address, 220, 22);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.address),
				 "key_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect_after(GTK_OBJECT(configDialogData.user.address),
				 "button_press_event",
				 GTK_SIGNAL_FUNC(RefreshUserStatusCallBack), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.address),
			   "focus_in_event", GTK_SIGNAL_FUNC(CheckInUserDataLength), (gpointer) 6);
	gtk_signal_connect(GTK_OBJECT(configDialogData.user.address),
			   "focus_out_event",
			   GTK_SIGNAL_FUNC(CheckOutUserDataLength),
			   (gpointer) configDialogData.user.max);
	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.user.address, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.user.address);


  /***  Groups notebook  ***/
	cg_names_option_frame = gtk_frame_new(_("Caller groups names"));


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(cg_names_option_frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("Groups"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cg_names_option_frame, label);

	for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER; i++) {
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 3);
		gtk_widget_show(hbox);

		g_snprintf(labelBuffer, 10, _("Group %d:"), i + 1);
		label = gtk_label_new(labelBuffer);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);

		configDialogData.groups[i] = gtk_entry_new_with_max_length(MAX_CALLER_GROUP_LENGTH);
		gtk_widget_set_usize(configDialogData.groups[i], 220, 22);

		gtk_box_pack_end(GTK_BOX(hbox), configDialogData.groups[i], FALSE, FALSE, 2);
		gtk_widget_show(configDialogData.groups[i]);
	}

	/* Mail */
	mail_option_frame = gtk_frame_new(_("Mailbox"));
	gtk_widget_show(mail_option_frame);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(mail_option_frame), vbox);
	gtk_widget_show(vbox);

	label = gtk_label_new(_("Mail"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mail_option_frame, label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Path to mailbox:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	configDialogData.mailbox = gtk_entry_new_with_max_length(MAILBOX_LENGTH - 1);
	gtk_widget_set_usize(configDialogData.mailbox, 220, 22);

	gtk_box_pack_end(GTK_BOX(hbox), configDialogData.mailbox, FALSE, FALSE, 2);
	gtk_widget_show(configDialogData.mailbox);

	optionsDialogIsOpened = FALSE;
	return dialog;
}


static void TopLevelWindow(void)
{
	GtkWidget *drawing_area;
	GdkBitmap *mask;
	GtkStyle *style;
	GdkGC *gc;
	struct sigaction act;

	GUI_MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_MainWindow), "MainWindow", "Xgnokii");
/*  gtk_window_set_decorated (GTK_WINDOW (GUI_MainWindow), GTK_FALSE); */
	gtk_widget_realize(GUI_MainWindow);

	BackgroundPixmap =
	    gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window, &mask,
					 &GUI_MainWindow->style->white, (gchar **) XPM_background);

	SMSPixmap =
	    gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window, &mask,
					 &GUI_MainWindow->style->white, (gchar **) XPM_sms);

	AlarmPixmap =
	    gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window, &mask,
					 &GUI_MainWindow->style->white, (gchar **) XPM_alarm);

	Pixmap =
	    gdk_pixmap_create_from_xpm_d(GUI_MainWindow->window, &mask,
					 &GUI_MainWindow->style->white, (gchar **) XPM_background);

//  gdk_window_set_icon_name (GUI_MainWindow->window, "XXX");
	style = gtk_widget_get_default_style();
	gc = style->black_gc;

	/* Create the drawing area */
	drawing_area = gtk_drawing_area_new();

	/* Signals used to handle backing pixmap */
	gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
			   (GtkSignalFunc) ExposeEvent, NULL);

	gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
			   (GtkSignalFunc) ButtonPressEvent, NULL);

	gtk_widget_set_events(drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

	gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), 261, 96);
	gtk_container_add(GTK_CONTAINER(GUI_MainWindow), drawing_area);

	gdk_draw_pixmap(drawing_area->window,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE(drawing_area)],
			Pixmap, 0, 0, 0, 0, 261, 96);

	gtk_widget_shape_combine_mask(GUI_MainWindow, mask, 0, 0);

	gtk_signal_connect(GTK_OBJECT(GUI_MainWindow), "destroy", GTK_SIGNAL_FUNC(_MainExit), NULL);

	Menu = CreateMenu();
	OptionsDialog = CreateOptionsDialog();
	AboutDialog = CreateAboutDialog();
	GUI_CreateSMSWindow();
	GUI_CreateContactsWindow();
	GUI_CreateNetmonWindow();
	GUI_CreateDTMFWindow();
	GUI_CreateSpeedDialWindow();
	GUI_CreateXkeybWindow();
	GUI_CreateCalendarWindow();
	GUI_CreateLogosWindow();
	GUI_CreateXringWindow();
	GUI_CreateDataWindow();
	CreateErrorDialog(&errorDialog, GUI_MainWindow);
	CreateInfoDialog(&infoDialog, GUI_MainWindow);
	CreateInCallDialog();

	act.sa_handler = RemoveZombie;
	sigemptyset(&(act.sa_mask));
	act.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &act, NULL);

#if __unices__
	act.sa_handler = SIG_IGN;
	sigemptyset(&(act.sa_mask));
	sigaction(SIGALRM, &act, NULL);
#endif

	gtk_widget_show_all(GUI_MainWindow);
	GUI_Refresh();

	GUI_InitPhoneMonitor();
	pthread_create(&monitor_th, NULL, GUI_Connect, NULL);

	gtk_timeout_add(1000, (GtkFunction) Update, GUI_MainWindow);

	hiddenCallDialog = 0;
}


static void SplashScreen(void)
{
	GtkWidget *pixmap, *fixed;
	GdkPixmap *gdk_pixmap;
	GdkBitmap *mask;
	GtkStyle *style;
	GdkGC *gc;

	SplashWindow = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_realize(SplashWindow);

	gtk_widget_set_usize(SplashWindow, 475, 160);
	gtk_window_position(GTK_WINDOW(SplashWindow), GTK_WIN_POS_CENTER);

	style = gtk_widget_get_default_style();
	gc = style->black_gc;
	gdk_pixmap = gdk_pixmap_create_from_xpm_d(SplashWindow->window, &mask,
						  &style->bg[GTK_STATE_NORMAL], XPM_logo);
	pixmap = gtk_pixmap_new(gdk_pixmap, mask);

	fixed = gtk_fixed_new();
	gtk_widget_set_usize(fixed, 261, 96);
	gtk_fixed_put(GTK_FIXED(fixed), pixmap, 0, 0);
	gtk_container_add(GTK_CONTAINER(SplashWindow), fixed);

	gtk_widget_shape_combine_mask(SplashWindow, mask, 0, 0);

	gtk_widget_show_all(SplashWindow);
}


static gint RemoveSplash(GtkWidget * Win)
{
	if (GTK_WIDGET_VISIBLE(SplashWindow)) {
		gtk_timeout_remove(splashRemoveHandler);
		gtk_widget_hide(SplashWindow);
		gtk_widget_destroy(SplashWindow);
		return TRUE;
	}

	return FALSE;
}

static void ReadConfig(void)
{
	if (GN_ERR_NONE != gn_lib_phoneprofile_load( NULL, &statemachine ))
		exit(-1);

	xgnokiiConfig.model = statemachine->config.model;
	xgnokiiConfig.port = statemachine->config.port_device;
	xgnokiiConfig.connection = statemachine->config.connection_type;
	xgnokiiConfig.initlength = statemachine->config.init_length;
	xgnokiiConfig.bindir = gn_lib_cfg_get("global", "bindir");
	if (!xgnokiiConfig.bindir)
		xgnokiiConfig.bindir = gn_lib_cfg_get("gnokiid", "bindir");
	if (!xgnokiiConfig.bindir)
		xgnokiiConfig.bindir = g_strdup(SBINDIR);
	if (gn_lib_cfg_get("xgnokii", "allow_breakage"))
		xgnokiiConfig.allowBreakage = atoi(gn_lib_cfg_get("xgnokii", "allow_breakage"));
	else
		xgnokiiConfig.allowBreakage = 0;

	max_phonebook_number_length = max_phonebook_sim_number_length =
	    GN_PHONEBOOK_NUMBER_MAX_LENGTH;
	GUI_ReadXConfig();
	max_phonebook_name_length = GNOKII_MIN(GN_PHONEBOOK_NAME_MAX_LENGTH, atoi(xgnokiiConfig.maxPhoneLen) + 1);
	max_phonebook_sim_name_length = GNOKII_MIN(GN_PHONEBOOK_NAME_MAX_LENGTH, atoi(xgnokiiConfig.maxSIMLen) + 1);

#ifndef WIN32
	xgnokiiConfig.xgnokiidir = DefaultXGnokiiDir;
#endif

	xgnokiiConfig.callerGroups[0] = xgnokiiConfig.callerGroups[1] =
	    xgnokiiConfig.callerGroups[2] = xgnokiiConfig.callerGroups[3] =
	    xgnokiiConfig.callerGroups[4] = xgnokiiConfig.callerGroups[5] = NULL;
	xgnokiiConfig.smsSets = 0;
}


int main(int argc, char *argv[])
{
	/* For GNU gettext */
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	gtk_init(&argc, &argv);

	gn_elog_handler = NULL;

/* Show the splash screen. */

	SplashScreen();

/* Remove it after a while. */

	ReadConfig();
	TopLevelWindow();

	splashRemoveHandler =
	    gtk_timeout_add(2000, (GtkFunction) RemoveSplash, (gpointer) SplashWindow);

	gtk_main();

	return (0);
}
