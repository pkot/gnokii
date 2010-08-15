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
  Copyright (C) 1999-2005 Jan Derfinak
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan
  Copyright (C) 2002      Markus Plail
  Copyright (C) 2003      Uli Hopp

*/

#include "config.h"

#ifndef WIN32
#  include <unistd.h>
#endif
#include <locale.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "misc.h"
#include "gnokii.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_contacts.h"
#include "xgnokii_sms.h"
#include "xpm/Edit.xpm"
#include "xpm/Delete.xpm"
#include "xpm/Forward.xpm"
#include "xpm/Reply.xpm"
#include "xpm/Send.xpm"
#include "xpm/SendSMS.xpm"
#include "xpm/Check.xpm"
#include "xpm/Names.xpm"
#include "xpm/BCard.xpm"
#include "xpm/quest.xpm"

typedef struct {
	gint count;		/* Messages count */
	gint number;		/* Number of tail in Long SMS */
	gint *msgPtr;		/* Array of MessageNumber */
	gint validity;
	gint class;
	gint memory;
	gchar sender[GN_BCD_STRING_MAX_LENGTH + 1];
} MessagePointers;

typedef struct {
	gchar *number;
	gchar *name;
	gint used:1;
} AddressPar;

typedef struct {
	GtkWidget *smsClist;
	GtkWidget *smsText;
	GSList *messages;
	GdkColor colour;
	gint row_i;
	gint currentBox;
} SMSWidget;

typedef struct {
	GtkWidget *SMSSendWindow;
	GtkWidget *smsSendText;
	GtkWidget *addr;
	GtkWidget *status;
	GtkWidget *report;
	GtkWidget *longSMS;
	GtkWidget *smscOptionMenu;
	GtkTooltips *addrTip;
	gint center;
	GSList *addressLine;
} SendSMSWidget;

  static gchar *mailbox_name=NULL;


static GtkWidget *GUI_SMSWindow;
static GtkWidget* create_SaveSMStoMailbox (void);
static SMSWidget SMS = { NULL, NULL, NULL };
static SendSMSWidget sendSMS = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL };
static ErrorDialog errorDialog = { NULL, NULL };
static InfoDialog infoDialog = { NULL, NULL };
static QuestMark questMark;
static GtkWidget *treeFolderItem[GN_SMS_FOLDER_MAX_NUMBER], *subTree;

static inline void CloseSMS(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(GUI_SMSWindow);
}


static inline void CloseSMSSend(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(sendSMS.SMSSendWindow);
}


static gint CListCompareFunc(GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
	gchar *text1 = NULL;
	gchar *text2 = NULL;

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

	if (clist->sort_column == 1 && !SMS.currentBox) {
		struct tm bdTime;
		time_t time1, time2;

		bdTime.tm_sec = atoi(text1 + 17);
		bdTime.tm_min = atoi(text1 + 14);
		bdTime.tm_hour = atoi(text1 + 11);
		bdTime.tm_mday = atoi(text1);
		bdTime.tm_mon = atoi(text1 + 3) - 1;
		bdTime.tm_year = atoi(text1 + 6) - 1900;
#ifdef HAVE_TM_GMTON
		if (text1[19] != '\0')
			bdTime.tm_gmtoff = atoi(text1 + 20) * 3600;
#endif
		bdTime.tm_isdst = -1;

		time1 = mktime(&bdTime);

		bdTime.tm_sec = atoi(text2 + 17);
		bdTime.tm_min = atoi(text2 + 14);
		bdTime.tm_hour = atoi(text2 + 11);
		bdTime.tm_mday = atoi(text2);
		bdTime.tm_mon = atoi(text2 + 3) - 1;
		bdTime.tm_year = atoi(text2 + 6) - 1900;
#ifdef HAVE_TM_GMTON
		if (text2[19] != '\0')
			bdTime.tm_gmtoff = atoi(text2 + 20) * 3600;
#endif
		bdTime.tm_isdst = -1;

		time2 = mktime(&bdTime);

		if (time1 < time2)
			return (1);
		else if (time1 > time2)
			return (-1);
		else
			return 0;
	}

	return (g_strcasecmp(text1, text2));
}


static inline void DestroyMsgPtrs(gpointer data)
{
	g_free(((MessagePointers *) data)->msgPtr);
	g_free((MessagePointers *) data);
}

static int IsValidSMSforFolder(gpointer d, gpointer userData)
{
	gn_sms *data = (gn_sms *) d;
	int folder_idx;

	if (phoneMonitor.supported & PM_FOLDERS) {
		folder_idx = data->memory_type - 12;
		if (folder_idx == SMS.currentBox)
			return 1;
	} else {
		if ((data->type == GN_SMS_MT_Deliver && !SMS.currentBox) ||
		    (data->type == GN_SMS_MT_StatusReport && !SMS.currentBox) ||
		    (data->type == GN_SMS_MT_Submit && SMS.currentBox))
			return 1;
	}
	return 0;
}


/* This function works only if phoneMonitor.sms.messages.number 
   ( = data->number) is the number of the SMS in the current folder
   ( = row + 1, row = 0 is the first row in the SMS list )
*/
static void MarkSMSasRead(gpointer d, gpointer userData)
{
	gn_sms *data = (gn_sms *) d;
	gint row = *(gint *) userData;
#ifdef XDEBUG
	gchar *dprint_status1;
#endif
	gint valid;

	row = row + 1 ; 
	if ((valid = IsValidSMSforFolder(d, userData))) {
#ifdef XDEBUG
		switch (data->status) {
		case GN_SMS_Read:
			dprint_status1 = g_strdup(_("read"));
			break;
		case GN_SMS_Unread:
			dprint_status1 = g_strdup(_("unread"));
			break;
		case GN_SMS_Sent:
			dprint_status1 = g_strdup(_("sent"));
			break;
		case GN_SMS_Unsent:
			dprint_status1 = g_strdup(_("not sent"));
			break;
		default:
			dprint_status1 = g_strdup(_("unknown"));
			break;
		}
		gn_log_xdebug("In MarkSMSasRead : data->status : %s \n", dprint_status1);
		gn_log_xdebug("In MarkSMSasRead : data->number : %i row : %i\n", data->number, row);
#endif
		if (data->number == row) data->status = GN_SMS_Read;
	};
}


static void InsertFolderElement(gpointer d, gpointer userData)
{
	gn_sms *data = (gn_sms *) d;
	MessagePointers *msgPtrs;
	gn_timestamp *dt = NULL;
	gint valid;

	dt = &data->smsc_time;

	if ((valid = IsValidSMSforFolder(d, userData))) {

		/*
		if (data->type == GST_MT && data->UDHtype == GSM_ConcatenatedMessages) {
			msgPtrs = (MessagePointers *) g_malloc (sizeof (MessagePointers));
			msgPtrs->count = data->UDH[4];
			msgPtrs->number = data->UDH[5];
			msgPtrs->validity = data->Validity;
			msgPtrs->class = data->Class;
			strcpy (msgPtrs->sender, data->Sender);
			msgPtrs->msgPtr = (gint *) g_malloc (msgPtrs->count * sizeof (gint));
			*(msgPtrs->msgPtr + msgPtrs->number - 1) = data->MessageNumber;
		}
		else { 
		*/
		gchar *row[4];
		switch (data->type) {
		case GN_SMS_MT_StatusReport:
			switch (data->status) {
			case GN_SMS_Read:
				row[0] = g_strdup(_("read report"));
				break;
			default:
				row[0] = g_strdup(_("unread report"));
				break;
			}
		case GN_SMS_MT_Picture:
		case GN_SMS_MT_PictureTemplate:
			switch (data->status) {
			case GN_SMS_Read:
				row[0] = g_strdup(_("seen picture"));
				break;
			case GN_SMS_Unread:
				row[0] = g_strdup(_("unseen picture"));
				break;
			case GN_SMS_Sent:
				row[0] = g_strdup(_("sent picture"));
				break;
			case GN_SMS_Unsent:
				row[0] = g_strdup(_("not sent picture"));
				break;
			default:
				row[0] = g_strdup(_("unknown picture"));
				break;
			}
		default:
			switch (data->status) {
			case GN_SMS_Read:
				row[0] = g_strdup(_("read"));
				break;
			case GN_SMS_Unread:
				row[0] = g_strdup(_("unread"));
				break;
			case GN_SMS_Sent:
				row[0] = g_strdup(_("sent"));
				break;
			case GN_SMS_Unsent:
				row[0] = g_strdup(_("not sent"));
				break;
			default:
				row[0] = g_strdup(_("unknown"));
				break;
			}
		}

		if (dt) {
			if (dt->timezone)
				row[1] = g_strdup_printf(_("%02d/%02d/%04d %02d:%02d:%02d %c%02d00"),
							 dt->day, dt->month, dt->year,
							 dt->hour, dt->minute, dt->second,
							 dt->timezone > 0 ? '+' : '-',
							 abs(dt->timezone));
			else
				row[1] = g_strdup_printf(_("%02d/%02d/%04d %02d:%02d:%02d"),
							 dt->day, dt->month, dt->year,
							 dt->hour, dt->minute, dt->second);
		} else {
			row[1] =
			    g_strdup_printf(_("%02d/%02d/%04d %02d:%02d:%02d"), 01, 01, 0001, 01, 01,
					    01);
		}

		row[2] = GUI_GetName(data->remote.number);
		if (row[2] == NULL)
			row[2] = data->remote.number;
		if ((data->type == GN_SMS_MT_Picture) || (data->type == GN_SMS_MT_PictureTemplate))
			row[3] = g_strdup_printf(_("Picture Message: %s"), data->user_data[1].u.text);
		else
			row[3] = data->user_data[0].u.text;

		gtk_clist_append(GTK_CLIST(SMS.smsClist), row);

		msgPtrs = (MessagePointers *) g_malloc(sizeof(MessagePointers));
		msgPtrs->count = msgPtrs->number = 1;
		msgPtrs->validity = data->validity;
/*		msgPtrs->class = data->Class; */
		strcpy(msgPtrs->sender, data->remote.number);
		msgPtrs->msgPtr = (gint *) g_malloc(sizeof(gint));
		*(msgPtrs->msgPtr) = (int) data->number;
		msgPtrs->memory = data->memory_type;

		gtk_clist_set_row_data_full(GTK_CLIST(SMS.smsClist), SMS.row_i++,
					    msgPtrs, DestroyMsgPtrs);
		g_free(row[0]);
		g_free(row[1]);
	}
}

static inline void RefreshFolder(void)
{
	GtkTextBuffer *buffer;

	gn_log_xdebug("RefreshFolder\n\n\n");

	/* Clear the SMS text window */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(SMS.smsText));
	gtk_text_buffer_set_text(buffer, "", 0);

	gtk_clist_freeze(GTK_CLIST(SMS.smsClist));
	gtk_clist_clear(GTK_CLIST(SMS.smsClist));

	SMS.row_i = 0;
	g_slist_foreach(phoneMonitor.sms.messages, InsertFolderElement, (gpointer) NULL);

	gtk_clist_sort(GTK_CLIST(SMS.smsClist));
	gtk_clist_thaw(GTK_CLIST(SMS.smsClist));
}

inline void GUI_RefreshMessageWindow(void)
{
	if (!GTK_WIDGET_VISIBLE(GUI_SMSWindow))
		return;
	RefreshFolder();
}


static void SelectTreeItem(GtkWidget * item, gpointer data)
{
	SMS.currentBox = GPOINTER_TO_INT(data);
	GUI_RefreshMessageWindow();
}


static void ClickEntry(GtkWidget * clist,
		       gint row, gint column, GdkEventButton * event, gpointer data)
{
	gchar *buf;
	gchar *text1, *text2, *text3;
	text2 = g_strdup(_("unread"));
	GtkTextBuffer *buffer;

	     /* Mark SMS as read */

	if (gtk_clist_get_text(GTK_CLIST(clist), row, 0, &(text1))) {
		gn_log_xdebug("*text1: %s *text2: %s \n", text1, text2);

		/* strcpm(text1,text2) = 0 if text1 is the same string as text2 */
		if (!(strcmp(text1, text2) )) {

			/* Store the read status in phoneMonitor.sms.messages */

			g_slist_foreach(phoneMonitor.sms.messages, (GFunc) MarkSMSasRead, (gpointer) &(row));
			text3 = g_strdup(_("read"));

			/* Change the status now in the clist window and don't wait for */
			/* the next refresh command */

			gtk_clist_freeze(GTK_CLIST(clist));

			gtk_clist_set_text(GTK_CLIST(clist), row, 0, text3); 

			gtk_clist_thaw(GTK_CLIST(clist));
		};
	};

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(SMS.smsText));

	gtk_text_buffer_set_text(buffer, "", 0);

	gtk_text_buffer_insert_at_cursor(buffer, _("From: "), -1);
	gtk_clist_get_text(GTK_CLIST(clist), row, 2, &buf);
	gtk_text_buffer_insert_at_cursor(buffer, buf, -1);
	gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);

	gtk_text_buffer_insert_at_cursor(buffer, _("Date: "), -1);
	gtk_clist_get_text(GTK_CLIST(clist), row, 1, &buf);
	gtk_text_buffer_insert_at_cursor(buffer, buf, -1);
	gtk_text_buffer_insert_at_cursor(buffer, "\n\n", -1);

	gtk_clist_get_text(GTK_CLIST(clist), row, 3, &buf);
	gtk_text_buffer_insert_at_cursor(buffer, buf, -1);
}


inline void GUI_ShowSMS(void)
{
	gint i, j;
	gn_error error;

	while ((error = GUI_InitSMSFolders()) != GN_ERR_NONE)
		sleep(1);

	for (i = 0; i < foldercount; i++) {
		if (i > lastfoldercount - 1) {
			treeFolderItem[i] = gtk_tree_item_new_with_label(_(folders[i]));
			gtk_tree_append(GTK_TREE(subTree), treeFolderItem[i]);
			gtk_signal_connect(GTK_OBJECT(treeFolderItem[i]), "select",
					   GTK_SIGNAL_FUNC(SelectTreeItem), GINT_TO_POINTER(i));
			gtk_widget_show(treeFolderItem[i]);
		} else {
			gtk_widget_set_name(treeFolderItem[i], _(folders[i]));
		}
	}
	for (j = i + 1; j < lastfoldercount; j++)
		gtk_widget_hide(treeFolderItem[j - 1]);
	lastfoldercount = foldercount;
	gtk_window_present(GTK_WINDOW(GUI_SMSWindow));
	GUI_RefreshMessageWindow();
}


static void OkDeleteSMSDialog(GtkWidget * widget, gpointer data)
{
	gn_sms *message;
	PhoneEvent *e;
	GList *sel;
	gint row, count, number;


	sel = GTK_CLIST(SMS.smsClist)->selection;

	gtk_clist_freeze(GTK_CLIST(SMS.smsClist));

	while (sel != NULL) {
		row = GPOINTER_TO_INT(sel->data);
		sel = sel->next;
		for (count = 0; 
		     count < ((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST(SMS.smsClist), row))->count; 
		     count++) {
			message = (gn_sms *) g_malloc(sizeof(gn_sms));
			number = *(((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST(SMS.smsClist), row))->msgPtr + count);
			if (number == -1) {
				g_free(message);
				continue;
			}
			message->number = number;
			message->memory_type = ((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST(SMS.smsClist), row))->memory;

			e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
			e->event = Event_DeleteSMSMessage;
			e->data = message;
			GUI_InsertEvent(e);
			/*
			if (e->status != GN_ERR_NONE) {
				if (e->status == GN_ERR_NOTIMPLEMENTED) {
					gtk_label_set_text(GTK_LABEL(errorDialog.text), _("Function not implemented!"));  
					gtk_widget_show(errorDialog.dialog);
				}
				else
					{
						gtk_label_set_text(GTK_LABEL(errorDialog.text), _("Delete SMS failed!"));  
						gtk_widget_show(errorDialog.dialog);
					}
			}
			*/
		}
	}

	gtk_widget_hide(GTK_WIDGET(data));

	gtk_clist_thaw(GTK_CLIST(SMS.smsClist));
}


static void DelSMS(void)
{
	static GtkWidget *dialog = NULL;
	GtkWidget *button, *hbox, *label, *pixmap;

	if (dialog == NULL) {
		dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(dialog), _("Delete SMS"));
		gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
		gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkDeleteSMSDialog), (gpointer) dialog);
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

		label = gtk_label_new(_("Do you want to delete selected SMS?"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
		gtk_widget_show(label);
	}

	gtk_widget_show(GTK_WIDGET(dialog));
}

static void SaveToMailbox(gchar *mailbox_name)
{
	gchar buf[255];
	FILE *f;
	gint fd;
	GList *sel;
	struct tm t, *loctime;
	struct flock lock;
	time_t caltime;
	gint row;
	gchar *number, *text, *loc, dummy[6];


	if ((f = fopen(mailbox_name, "a")) == NULL) {
		snprintf(buf, 255, _("Cannot open mailbox %s for appending!"),
			 mailbox_name);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		return;
	}

	fd = fileno(f);
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(fd, F_GETLK, &lock) != -1 && lock.l_type != F_UNLCK) {
		snprintf(buf, 255, _("Cannot save to mailbox %s.\n\%s is locked for process %d!"),
			mailbox_name, mailbox_name, lock.l_pid);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		fclose(f);
		return;
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(fd, F_SETLK, &lock) == -1) {
		snprintf(buf, 255, _("Cannot lock mailbox %s!"), mailbox_name);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		fclose(f);
		return;
	}

	sel = GTK_CLIST(SMS.smsClist)->selection;

	while (sel != NULL) {
		row = GPOINTER_TO_INT(sel->data);
		sel = sel->next;
		gtk_clist_get_text(GTK_CLIST(SMS.smsClist), row, 1, &text);
		dummy[3] = 0;
		snprintf(dummy, 3, "%s", text + 17);
		t.tm_sec = atoi(dummy);
		snprintf(dummy, 3, "%s", text + 14);
		t.tm_min = atoi(dummy);
		snprintf(dummy, 3, "%s", text + 11);
		t.tm_hour = atoi(dummy);
		snprintf(dummy, 3, "%s", text);
		t.tm_mday = atoi(dummy);
		snprintf(dummy, 3, "%s", text + 3);
		t.tm_mon = atoi(dummy) - 1;
		dummy[5] = 0;
		snprintf(dummy, 5, "%s", text + 6);
		t.tm_year = atoi(dummy) - 1900;
#ifdef HAVE_TM_GMTON
		if (text[19] != '\0')
			t.tm_gmtoff = atoi(text + 18) * 3600;
#endif

		caltime = mktime(&t);
		loctime = localtime(&caltime);

		gtk_clist_get_text(GTK_CLIST(SMS.smsClist), row, 2, &number);
		gtk_clist_get_text(GTK_CLIST(SMS.smsClist), row, 3, &text);

		fprintf(f, "From %s@xgnokii %s", number, asctime(loctime));
		loc = setlocale(LC_ALL, "C");
		strftime(buf, 255, "Date: %a, %d %b %Y %H:%M:%S %z (%Z)\n", loctime);
		setlocale(LC_ALL, loc);
		fprintf(f, "%s", buf);
		fprintf(f, "From: %s@xgnokii\n", number);
		strncpy(buf, text, 20);
		buf[20] = '\0';
		fprintf(f, "Subject: %s\n\n", buf);
		fprintf(f, "%s\n\n", text);
	}

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(fd, F_SETLK, &lock) == -1) {
		snprintf(buf, 255, _("Cannot unlock mailbox %s!"), mailbox_name);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
	}

	fclose(f);
}

static void OKSaveSMStoMailbox(GtkWidget * widget, gpointer data)
{
  mailbox_name = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
  SaveToMailbox(mailbox_name);
  gtk_widget_hide(GTK_WIDGET(data));
}


static void SaveSMStoMailbox_dialog (void)
{
  static GtkWidget *SaveSMS_dialog = NULL;
  
  if ( SaveSMS_dialog == NULL) SaveSMS_dialog = create_SaveSMStoMailbox();
  
  if (mailbox_name == NULL) {
    mailbox_name = g_malloc (strlen(xgnokiiConfig.mailbox)+1);
    mailbox_name = strcpy ( mailbox_name, xgnokiiConfig.mailbox);
  };
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(SaveSMS_dialog), mailbox_name);
  
  gtk_widget_show(GTK_WIDGET(SaveSMS_dialog));

}

static void SaveSMStoMailbox (void)
{

SaveToMailbox(xgnokiiConfig.mailbox);

}

/* created with the help of glade */

static GtkWidget*
create_SaveSMStoMailbox (void)
{
  GtkWidget *SaveSMStoMailbox;
  GtkWidget *ok_button1;
  GtkWidget *cancel_button1;

  SaveSMStoMailbox = gtk_file_selection_new (_("Choose Mailbox File"));
  gtk_object_set_data (GTK_OBJECT (SaveSMStoMailbox), "SaveSMStoMailbox", SaveSMStoMailbox);
  gtk_container_set_border_width (GTK_CONTAINER (SaveSMStoMailbox), 10);
  GTK_WINDOW (SaveSMStoMailbox)->type = GTK_WINDOW_TOPLEVEL;

  ok_button1 = GTK_FILE_SELECTION (SaveSMStoMailbox)->ok_button;
  gtk_object_set_data (GTK_OBJECT (SaveSMStoMailbox), "ok_button1", ok_button1);
  gtk_widget_show (ok_button1);
  GTK_WIDGET_SET_FLAGS (ok_button1, GTK_CAN_DEFAULT);

  cancel_button1 = GTK_FILE_SELECTION (SaveSMStoMailbox)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (SaveSMStoMailbox), "cancel_button1", cancel_button1);
  gtk_widget_show (cancel_button1);
  GTK_WIDGET_SET_FLAGS (cancel_button1, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (ok_button1), "clicked",
                      GTK_SIGNAL_FUNC (OKSaveSMStoMailbox),
                      (gpointer) SaveSMStoMailbox);
  gtk_signal_connect(GTK_OBJECT(SaveSMStoMailbox), "delete_event",
		     GTK_SIGNAL_FUNC(DeleteEvent), NULL);
  gtk_signal_connect (GTK_OBJECT (cancel_button1), "clicked",
                      GTK_SIGNAL_FUNC (CancelDialog),
                      (gpointer) SaveSMStoMailbox );

  return SaveSMStoMailbox;
}


static inline void RefreshSMSStatus(void)
{
	gchar *buf;
	guint l, m;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sendSMS.smsSendText));
	/* FIXME: char count is not the same as octects to be sent */
	l = gtk_text_buffer_get_char_count(buffer);

	if (l <= GN_SMS_MAX_LENGTH)
		buf = g_strdup_printf("%d/1", l);
	else {
		m = l % 153;
		buf = g_strdup_printf("%d/%d", l > 0 &&
				      !m ? 153 : m, l == 0 ? 1 : ((l - 1) / 153) + 1);
	}

	gtk_frame_set_label(GTK_FRAME(sendSMS.status), buf);
	g_free(buf);
}


static inline gint RefreshSMSLength(GtkWidget * widget, gpointer callback_data)
{
	RefreshSMSStatus();

	return (FALSE);
}


static inline void SetActiveCenter(GtkWidget * item, gpointer data)
{
	sendSMS.center = GPOINTER_TO_INT(data);
}


void GUI_RefreshsmscenterMenu(void)
{
	static GtkWidget *smscMenu = NULL;
	GtkWidget *item;
	register gint i;

	if (!sendSMS.smscOptionMenu)
		return;

	if (smscMenu) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(sendSMS.smscOptionMenu));
		if (GTK_IS_WIDGET(smscMenu))
			gtk_widget_destroy(GTK_WIDGET(smscMenu));
		smscMenu = NULL;
	}

	smscMenu = gtk_menu_new();

	for (i = 0; i < xgnokiiConfig.smsSets; i++) {
		if (*(xgnokiiConfig.smsSetting[i].name) == '\0') {
			gchar *buf = g_strdup_printf(_("Set %d"), i + 1);
			item = gtk_menu_item_new_with_label(xgnokiiConfig.smsSetting[i].smsc.number);
			g_free(buf);
		} else
			item = gtk_menu_item_new_with_label(xgnokiiConfig.smsSetting[i].name);

		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetActiveCenter), (gpointer) i);

		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(smscMenu), item);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(sendSMS.smscOptionMenu), smscMenu);
}


static inline void InitAddressLine(gpointer d, gpointer userData)
{
	((AddressPar *) d)->used = 0;
}


#ifdef XDEBUG
static inline void PrintAddressLine(gpointer d, gpointer userData)
{
	g_print(_("Name: %s\nNumber: %s\nUsed: %d\n"),
		((AddressPar *) d)->name, ((AddressPar *) d)->number, ((AddressPar *) d)->used);
}
#endif


static inline gint CompareAddressLineName(gconstpointer a, gconstpointer b)
{
	return strcmp(((AddressPar *) a)->name, ((AddressPar *) b)->name);
}


static inline gint CompareAddressLineUsed(gconstpointer a, gconstpointer b)
{
	return !(((AddressPar *) a)->used == ((AddressPar *) b)->used);
}


static gint CheckAddressMain(void)
{
	AddressPar aps;
	GSList *r;
	register gint i = 0;
	gint ret = 0;
	gchar *buf;
	GString *tooltipBuf;
	gchar **strings = g_strsplit(gtk_entry_get_text(GTK_ENTRY(sendSMS.addr)), ",", 0);

	tooltipBuf = g_string_new("");
	gtk_entry_set_text(GTK_ENTRY(sendSMS.addr), "");
	g_slist_foreach(sendSMS.addressLine, InitAddressLine, (gpointer) NULL);

	while (strings[i]) {
		g_strstrip(strings[i]);
		if (*strings[i] == '\0') {
			i++;
			continue;
		}
		if ((buf = GUI_GetName(strings[i])) != NULL) {
			AddressPar *ap = g_malloc(sizeof(AddressPar));
			ap->number = g_strdup(strings[i]);
			ap->name = g_strdup(buf);
			ap->used = 1;
			sendSMS.addressLine = g_slist_append(sendSMS.addressLine, ap);
			gtk_entry_append_text(GTK_ENTRY(sendSMS.addr), buf);
			g_string_append(tooltipBuf, ap->number);
		} else if ((buf = GUI_GetNumber(strings[i])) != NULL) {
			aps.name = strings[i];
			r = g_slist_find_custom(sendSMS.addressLine, &aps, CompareAddressLineName);
			if (r) {
				((AddressPar *) r->data)->used = 1;
				g_string_append(tooltipBuf, ((AddressPar *) r->data)->number);
			} else
				g_string_append(tooltipBuf, buf);
			gtk_entry_append_text(GTK_ENTRY(sendSMS.addr), strings[i]);
		} else {
			gint len = strlen(strings[i]);
			gint j;

			for (j = 0; j < len; j++)
				if (strings[i][j] != '*' && strings[i][j] != '#' &&
				    strings[i][j] != '+' && (strings[i][j] < '0' ||
							     strings[i][j] > '9'))
					ret = 1;
			gtk_entry_append_text(GTK_ENTRY(sendSMS.addr), strings[i]);
			g_string_append(tooltipBuf, strings[i]);
		}
		if (strings[i + 1] != NULL) {
			gtk_entry_append_text(GTK_ENTRY(sendSMS.addr), ", ");
			g_string_append(tooltipBuf, ", ");
		}
		i++;
	}

	aps.used = 0;
	while ((r = g_slist_find_custom(sendSMS.addressLine, &aps, CompareAddressLineUsed)))
		sendSMS.addressLine = g_slist_remove(sendSMS.addressLine, r->data);

	if (tooltipBuf->len > 0) {
		gtk_tooltips_set_tip(sendSMS.addrTip, sendSMS.addr, tooltipBuf->str, NULL);
		gtk_tooltips_enable(sendSMS.addrTip);
	} else
		gtk_tooltips_disable(sendSMS.addrTip);

#ifdef XDEBUG
	g_slist_foreach(sendSMS.addressLine, PrintAddressLine, (gpointer) NULL);
#endif

	g_strfreev(strings);
	g_string_free(tooltipBuf, TRUE);

	return ret;
}


static inline void CheckAddress(void)
{
	if (CheckAddressMain()) {
		gtk_label_set_text(GTK_LABEL(errorDialog.text),
				   _("Address line contains illegal address!"));
		gtk_widget_show(errorDialog.dialog);
	}
}


static void DeleteSelectContactDialog(GtkWidget * widget, GdkEvent * event,
				      SelectContactData * data)
{
	gtk_widget_destroy(GTK_WIDGET(data->clist));
	gtk_widget_destroy(GTK_WIDGET(data->clistScrolledWindow));
	gtk_widget_destroy(GTK_WIDGET(widget));
}


static void CancelSelectContactDialog(GtkWidget * widget, SelectContactData * data)
{
	gtk_widget_destroy(GTK_WIDGET(data->clist));
	gtk_widget_destroy(GTK_WIDGET(data->clistScrolledWindow));
	gtk_widget_destroy(GTK_WIDGET(data->dialog));
}


static void OkSelectContactDialog(GtkWidget * widget, SelectContactData * data)
{
	GList *sel;
	PhonebookEntry *pbEntry;

	if ((sel = GTK_CLIST(data->clist)->selection) != NULL)
		while (sel != NULL) {
			AddressPar *ap = g_malloc(sizeof(AddressPar));

			pbEntry = gtk_clist_get_row_data(GTK_CLIST(data->clist),
							 GPOINTER_TO_INT(sel->data));
			ap->number = g_strdup(pbEntry->entry.number);
			ap->name = g_strdup(pbEntry->entry.name);
			ap->used = 1;
			sendSMS.addressLine = g_slist_append(sendSMS.addressLine, ap);
			if (strlen(gtk_entry_get_text(GTK_ENTRY(sendSMS.addr))) > 0)
				gtk_entry_append_text(GTK_ENTRY(sendSMS.addr), ", ");
			gtk_entry_append_text(GTK_ENTRY(sendSMS.addr), ap->name);

			sel = sel->next;
		}

	gtk_widget_destroy(GTK_WIDGET(data->clist));
	gtk_widget_destroy(GTK_WIDGET(data->clistScrolledWindow));
	gtk_widget_destroy(GTK_WIDGET(data->dialog));

	CheckAddressMain();
}


static void ShowSelectContactsDialog(void)
{
	SelectContactData *r;

	if (!GUI_ContactsIsIntialized())
		GUI_ReadContacts();

	if ((r = GUI_SelectContactDialog()) == NULL)
		return;

	gtk_signal_connect(GTK_OBJECT(r->dialog), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteSelectContactDialog), (gpointer) r);

	gtk_signal_connect(GTK_OBJECT(r->okButton), "clicked",
			   GTK_SIGNAL_FUNC(OkSelectContactDialog), (gpointer) r);
	gtk_signal_connect(GTK_OBJECT(r->cancelButton), "clicked",
			   GTK_SIGNAL_FUNC(CancelSelectContactDialog), (gpointer) r);
}


static gint Sendsmscore(gn_sms * sms)
{
	gn_error error;
	PhoneEvent *e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	D_SMSMessage *m = (D_SMSMessage *) g_malloc(sizeof(D_SMSMessage));
	unsigned int i = 0;

	while (sms->user_data[i].type != GN_SMS_DATA_None) {
		if ((sms->user_data[i].type == GN_SMS_DATA_Text ||
		     sms->user_data[i].type == GN_SMS_DATA_NokiaText ||
		     sms->user_data[i].type == GN_SMS_DATA_iMelody) &&
		     !gn_char_def_alphabet(sms->user_data[i].u.text))
			sms->dcs.u.general.alphabet = GN_SMS_DCS_UCS2;
		i++;
	}
	m->sms = sms;
	e->event = Event_SendSMSMessage;
	e->data = m;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&sendSMSMutex);
	pthread_cond_wait(&sendSMSCond, &sendSMSMutex);
	pthread_mutex_unlock(&sendSMSMutex);

	gn_log_xdebug("Address: %s\nText: %s\nDelivery report: %d\nSMS Center: %d\n",
		sms->remote.number, sms->user_data[0].u.text,
		GTK_TOGGLE_BUTTON(sendSMS.report)->active, sendSMS.center);

	error = m->status;
	g_free(m);

	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error sending SMS to %s\n%s"),
					     sms->remote.number, gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
	} else
		g_print(_("Message sent to: %s\n"), sms->remote.number);

	return (error);
}


static void DoSendSMS(void)
{
	gn_sms sms;
	AddressPar aps;
	char udh[256];
	GSList *r;
	gchar *text, *number;
	gchar **addresses;
	gchar *buf;
	gint offset, nr_msg, l;
	gint longSMS;
	register gint i = 0, j;
	GtkTextBuffer *buffer;
	GtkTextIter startIter, endIter;

	if (CheckAddressMain()) {
		gtk_label_set_text(GTK_LABEL(errorDialog.text),
				   _("Address line contains illegal address!"));
		gtk_widget_show(errorDialog.dialog);
		return;
	}

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sendSMS.smsSendText));
	/* FIXME: char count is not the same as octects to be sent */
	l = gtk_text_buffer_get_char_count(buffer);

	gtk_text_buffer_get_start_iter(buffer, &startIter);
	gtk_text_buffer_get_end_iter(buffer, &endIter);
	text = gtk_text_buffer_get_text(buffer, &startIter, &endIter, FALSE);

	addresses = g_strsplit(gtk_entry_get_text(GTK_ENTRY(sendSMS.addr)), ",", 0);

	longSMS = GTK_TOGGLE_BUTTON(sendSMS.longSMS)->active;

	while (addresses[i]) {
		g_strstrip(addresses[i]);
		if ((number = GUI_GetNumber(addresses[i]))) {
			aps.name = addresses[i];
			if ((r =
			     g_slist_find_custom(sendSMS.addressLine, &aps,
						 CompareAddressLineName)))
				number = ((AddressPar *) r->data)->number;
		} else
			number = addresses[i];

		gn_sms_default_submit(&sms);
		strcpy(sms.smsc.number, xgnokiiConfig.smsSetting[sendSMS.center].smsc.number);
		sms.smsc.type = xgnokiiConfig.smsSetting[sendSMS.center].smsc.type;

		if (GTK_TOGGLE_BUTTON(sendSMS.report)->active)
			sms.delivery_report = true;
		else
			sms.delivery_report = false;
		sms.type = GN_SMS_MT_Submit;

		strncpy(sms.remote.number, number, GN_BCD_STRING_MAX_LENGTH);
		sms.remote.number[GN_BCD_STRING_MAX_LENGTH - 1] = '\0';
		if (sms.remote.number[0] == '+')
			sms.remote.type = GN_GSM_NUMBER_International;
		else 
			sms.remote.type = GN_GSM_NUMBER_Unknown;

		if (l > GN_SMS_MAX_LENGTH) {
			if (longSMS) {
				sms.user_data[0].type = GN_SMS_DATA_Concat;
				nr_msg = ((l - 1) / 153) + 1;
				udh[0] = 0x05;	/* UDH length */
				udh[1] = 0x00;	/* concatenated messages (IEI) */
				udh[2] = 0x03;	/* IEI data length */
				udh[3] = 0x01;	/* reference number */
				udh[4] = nr_msg;	/* number of messages */
				udh[5] = 0x00;	/* message reference number */
				offset = 6;

				for (j = 0; j < nr_msg; j++) {
					udh[5] = j + 1;
					memcpy(sms.user_data[0].u.text, udh, offset);
					strncpy(sms.user_data[0].u.text + offset, text + (j * 153),
						153);
					sms.user_data[0].u.text[153] = '\0';

					buf = g_strdup_printf(_("Sending SMS to %s (%d/%d) ...\n"),
							      sms.remote.number, j + 1,
							      nr_msg);
					gtk_label_set_text(GTK_LABEL(infoDialog.text), buf);
					gtk_widget_show_now(infoDialog.dialog);
					g_free(buf);
					GUI_Refresh();

					if (Sendsmscore(&sms) != GN_ERR_NONE) {
						gtk_widget_hide(infoDialog.dialog);
						GUI_Refresh();
						break;
					}
					gtk_widget_hide(infoDialog.dialog);
					GUI_Refresh();
					sleep(1);
				}
			} else {
				sms.udh.length = 0;
				sms.user_data[0].type = GN_SMS_DATA_Text;
				nr_msg = ((l - 1) / 153) + 1;
				if (nr_msg > 99)	/* We have place only for 99 messages in header. */
					nr_msg = 99;
				for (j = 0; j < nr_msg; j++) {
					gchar header[8];
					g_snprintf(header, 8, "%2d/%-2d: ", j + 1, nr_msg);
					header[7] = '\0';

					strcpy(sms.user_data[0].u.text, header);
					strncat(sms.user_data[0].u.text, text + (j * 153), 153);
					sms.user_data[0].u.text[160] = '\0';

					buf = g_strdup_printf(_("Sending SMS to %s (%d/%d) ...\n"),
							      sms.remote.number, j + 1,
							      nr_msg);
					gtk_label_set_text(GTK_LABEL(infoDialog.text), buf);
					gtk_widget_show_now(infoDialog.dialog);
					g_free(buf);
					GUI_Refresh();

					if (Sendsmscore(&sms) != GN_ERR_NONE) {
						gtk_widget_hide(infoDialog.dialog);
						GUI_Refresh();
						break;
					}

					gtk_widget_hide(infoDialog.dialog);
					GUI_Refresh();

					sleep(1);
				}
			}
		} else {
			sms.udh.length = 0;
			sms.user_data[0].type = GN_SMS_DATA_Text;
			strncpy(sms.user_data[0].u.text, text, GN_SMS_MAX_LENGTH + 1);
			sms.user_data[0].u.text[GN_SMS_MAX_LENGTH] = '\0';

			buf = g_strdup_printf(_("Sending SMS to %s (%d/%d) ...\n"), sms.remote.number, 1, 1);
			gtk_label_set_text(GTK_LABEL(infoDialog.text), buf);
			gtk_widget_show_now(infoDialog.dialog);
			g_free(buf);
			GUI_Refresh();

			(void) Sendsmscore(&sms);
			gtk_widget_hide(infoDialog.dialog);
			GUI_Refresh();
		}
		i++;
	}

	g_strfreev(addresses);
	g_free(text);
}


static GtkItemFactoryEntry send_menu_items[] = {
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>X", DoSendSMS, 0, NULL},
	{NULL, "<control>S", NULL, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>N", CheckAddress, 0, NULL},
	{NULL, "<control>C", ShowSelectContactsDialog, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>W", CloseSMSSend, 0, NULL},
	{NULL, NULL, NULL, 0, "<LastBranch>"},
	{NULL, NULL, GUI_ShowAbout, 0, NULL},
};


static void InitSendMenu(void)
{
	send_menu_items[0].path = _("/_File");
	send_menu_items[1].path = _("/File/Sen_d");
	send_menu_items[2].path = _("/File/_Save");
	send_menu_items[3].path = _("/File/Sep1");
	send_menu_items[4].path = _("/File/Check _Names");
	send_menu_items[5].path = _("/File/C_ontacts");
	send_menu_items[6].path = _("/File/Sep2");
	send_menu_items[7].path = _("/File/_Close");
	send_menu_items[8].path = _("/_Help");
	send_menu_items[9].path = _("/Help/_About");
}


static void CreateSMSSendWindow(void)
{
	int nmenu_items = sizeof(send_menu_items) / sizeof(send_menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar;
	GtkWidget *main_vbox;
	GtkWidget *hbox, *vbox;
	GtkWidget *button;
	GtkWidget *pixmap;
	GtkWidget *label;
	GtkWidget *toolbar;
	GtkWidget *scrolledWindow;
	GtkTooltips *tooltips;

	if (sendSMS.SMSSendWindow)
		return;

	InitSendMenu();
	sendSMS.SMSSendWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(sendSMS.SMSSendWindow), "SMSSendWindow", "Xgnokii");

	gtk_widget_set_usize (GTK_WIDGET (sendSMS.SMSSendWindow), 436, 220);

	gtk_signal_connect(GTK_OBJECT(sendSMS.SMSSendWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	gtk_widget_realize(sendSMS.SMSSendWindow);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, send_menu_items, NULL);

	gtk_window_add_accel_group(GTK_WINDOW(sendSMS.SMSSendWindow), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
	gtk_container_add(GTK_CONTAINER(sendSMS.SMSSendWindow), main_vbox);
	gtk_widget_show(main_vbox);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);

	/* Create the toolbar */

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Send message"), NULL,
				NewPixmap(SendSMS_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) DoSendSMS, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Save message to outbox"), NULL,
				NewPixmap(Send_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) NULL, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Check names"), NULL,
				NewPixmap(Check_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) CheckAddress, NULL);

	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_widget_show(toolbar);

	/* Address line */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 3);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("To:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 3);
	gtk_widget_show(label);

	sendSMS.addr = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), sendSMS.addr, TRUE, TRUE, 1);

	sendSMS.addrTip = gtk_tooltips_new();
	gtk_tooltips_set_tip(sendSMS.addrTip, sendSMS.addr, "", NULL);
	gtk_tooltips_disable(sendSMS.addrTip);

	gtk_widget_show(sendSMS.addr);

	button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(ShowSelectContactsDialog), (gpointer) NULL);

	pixmap = NewPixmap(Names_xpm, sendSMS.SMSSendWindow->window,
			   &sendSMS.SMSSendWindow->style->bg[GTK_STATE_NORMAL]);
	gtk_container_add(GTK_CONTAINER(button), pixmap);
	gtk_widget_show(pixmap);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);

	tooltips = gtk_tooltips_new();
	gtk_tooltips_set_tip(tooltips, button, _("Select contacts"), NULL);

	gtk_widget_show(button);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_end(GTK_BOX(main_vbox), hbox, TRUE, TRUE, 3);
	gtk_widget_show(hbox);

	/* Edit SMS widget */
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 3);
	gtk_widget_show(vbox);

	sendSMS.status = gtk_frame_new("0/1");
	gtk_frame_set_label_align(GTK_FRAME(sendSMS.status), 1.0, 0.0);
	gtk_frame_set_shadow_type(GTK_FRAME(sendSMS.status), GTK_SHADOW_OUT);

	gtk_box_pack_end(GTK_BOX(vbox), sendSMS.status, TRUE, TRUE, 5);
	gtk_widget_show(sendSMS.status);

	sendSMS.smsSendText = gtk_text_view_new();
	gtk_widget_set_usize(GTK_WIDGET(sendSMS.smsSendText), 0, 120);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(sendSMS.smsSendText), TRUE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(sendSMS.smsSendText), GTK_WRAP_WORD);
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(sendSMS.smsSendText))),
				 "changed",
				 G_CALLBACK(RefreshSMSLength), (gpointer) NULL);

	scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledWindow), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolledWindow), sendSMS.smsSendText);
	gtk_container_add(GTK_CONTAINER(sendSMS.status), scrolledWindow);

	gtk_widget_show(sendSMS.smsSendText);
	gtk_widget_show(scrolledWindow);

	/* Options widget */
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_end(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);
	gtk_widget_show(vbox);

	sendSMS.report = gtk_check_button_new_with_label(_("Delivery report"));
	gtk_box_pack_start(GTK_BOX(vbox), sendSMS.report, FALSE, FALSE, 3);
	gtk_widget_show(sendSMS.report);

	sendSMS.longSMS = gtk_check_button_new_with_label(_("Send as Long SMS"));
	gtk_box_pack_start(GTK_BOX(vbox), sendSMS.longSMS, FALSE, FALSE, 3);
	gtk_widget_show(sendSMS.longSMS);

	label = gtk_label_new(_("SMS Center:"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 1);
	gtk_widget_show(label);

	GUI_InitSMSSettings();
	sendSMS.smscOptionMenu = gtk_option_menu_new();

	GUIEventSend(GUI_EVENT_SMS_CENTERS_CHANGED);

	gtk_box_pack_start(GTK_BOX(vbox), sendSMS.smscOptionMenu, FALSE, FALSE, 1);
	gtk_widget_show(sendSMS.smscOptionMenu);
}


static void NewSMS(void)
{
	GtkTextBuffer *buffer;

	if (!sendSMS.SMSSendWindow)
		CreateSMSSendWindow();

	gtk_window_set_title(GTK_WINDOW(sendSMS.SMSSendWindow), _("New Message"));

	gtk_tooltips_disable(sendSMS.addrTip);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sendSMS.smsSendText));
	gtk_text_buffer_set_text(buffer, "", 0);

	gtk_entry_set_text(GTK_ENTRY(sendSMS.addr), "");

	RefreshSMSStatus();

	gtk_window_present(GTK_WINDOW(sendSMS.SMSSendWindow));
}


static void ForwardSMS(void)
{
	gchar *buf;
	GtkTextBuffer *buffer;

	if (GTK_CLIST(SMS.smsClist)->selection == NULL)
		return;

	if (!sendSMS.SMSSendWindow)
		CreateSMSSendWindow();

	gtk_window_set_title(GTK_WINDOW(sendSMS.SMSSendWindow), _("Forward Message"));

	gtk_tooltips_disable(sendSMS.addrTip);

	gtk_clist_get_text(GTK_CLIST(SMS.smsClist),
			   GPOINTER_TO_INT(GTK_CLIST(SMS.smsClist)->selection->data), 3, &buf);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sendSMS.smsSendText));
	gtk_text_buffer_set_text(buffer, buf, -1);

	gtk_entry_set_text(GTK_ENTRY(sendSMS.addr), "");

	RefreshSMSStatus();

	gtk_window_present(GTK_WINDOW(sendSMS.SMSSendWindow));
}


/*
static inline gint CompareSMSMessageLocation (gconstpointer a, gconstpointer b)
{
  return !(((gn_sms *) a)->Number == ((gn_sms *) b)->Number);
}
*/


static void ReplySMS(void)
{
	gchar *buf;
	GtkTextBuffer *buffer;
	/*
	GSList *r;
	gn_sms msg;
	*/

	if (GTK_CLIST(SMS.smsClist)->selection == NULL)
		return;

	if (!sendSMS.SMSSendWindow)
		CreateSMSSendWindow();

	gtk_window_set_title(GTK_WINDOW(sendSMS.SMSSendWindow), _("Reply Message"));

	gtk_clist_get_text(GTK_CLIST(SMS.smsClist),
			   GPOINTER_TO_INT(GTK_CLIST(SMS.smsClist)->selection->data), 3, &buf);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sendSMS.smsSendText));
	gtk_text_buffer_set_text(buffer, buf, -1);

	/*
	msg.Number = *(((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST (SMS.smsClist),
	               GPOINTER_TO_INT (GTK_CLIST (SMS.smsClist)->selection->data)))->msgPtr);

	r = g_slist_find_custom (SMS.messages, &msg, CompareSMSMessageLocation);
	if (r)
	*/
	gtk_entry_set_text(GTK_ENTRY(sendSMS.addr),
			   ((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST(SMS.smsClist),
								       GPOINTER_TO_INT(GTK_CLIST
										       (SMS.
											smsClist)->
										       selection->
										       data)))->
			   sender);

	CheckAddressMain();
	RefreshSMSStatus();

	gtk_window_present(GTK_WINDOW(sendSMS.SMSSendWindow));
}

static void ActivateSMS(void)
{
	if (isSMSactivated)
		isSMSactivated = 0;
	else
		isSMSactivated = 1;
}

static void NewBC(void)
{
	if (!sendSMS.SMSSendWindow)
		CreateSMSSendWindow();

	gtk_window_set_title(GTK_WINDOW(sendSMS.SMSSendWindow), _("Business Card"));

	gtk_tooltips_disable(sendSMS.addrTip);

	gtk_text_freeze(GTK_TEXT(sendSMS.smsSendText));
	gtk_text_set_point(GTK_TEXT(sendSMS.smsSendText), 0);
	gtk_text_forward_delete(GTK_TEXT(sendSMS.smsSendText),
				gtk_text_get_length(GTK_TEXT(sendSMS.smsSendText)));

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(SMS.colour), NULL,
			"Business Card\n", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.name, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, ", ", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.title, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, "\n", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.company, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, "\n\n", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(SMS.colour), NULL, "tel ", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.telephone, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, "\n", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(SMS.colour), NULL, "fax ", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.fax, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, "\n", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.email, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, "\n", -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, xgnokiiConfig.user.address, -1);

	gtk_text_insert(GTK_TEXT(sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black),
			NULL, "\n", -1);

	gtk_text_thaw(GTK_TEXT(sendSMS.smsSendText));

	gtk_entry_set_text(GTK_ENTRY(sendSMS.addr), "");

	RefreshSMSStatus();

	gtk_window_present(GTK_WINDOW(sendSMS.SMSSendWindow));
}


static GtkItemFactoryEntry menu_items[] = {
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>S", NULL, 0, NULL},
	{NULL, "<control>M", SaveSMStoMailbox, 0, NULL},
	{NULL, "<control>T", SaveSMStoMailbox_dialog, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>W", CloseSMS, 0, NULL},
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>A", ActivateSMS, 0, "<CheckItem>"},
	{NULL, "<control>N", NewSMS, 0, NULL},
	{NULL, "<control>F", ForwardSMS, 0, NULL},
	{NULL, "<control>R", ReplySMS, 0, NULL},
	{NULL, "<control>D", DelSMS, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>B", NewBC, 0, NULL},
	{NULL, NULL, NULL, 0, "<LastBranch>"},
	{NULL, NULL, GUI_ShowAbout, 0, NULL},
};


static void InitMainMenu(void)
{
	register gint i = 0;

	menu_items[i++].path = _("/_File");
	menu_items[i++].path = _("/File/_Save");
	menu_items[i++].path = _("/File/Save to mailbo_x");
	menu_items[i++].path = _("/File/Save to _file");
	menu_items[i++].path = _("/File/Sep1");
	menu_items[i++].path = _("/File/_Close");
	menu_items[i++].path = _("/_Messages");
	menu_items[i++].path = _("/_Messages/_Activate SMS reading");
	menu_items[i++].path = _("/_Messages/_New");
	menu_items[i++].path = _("/_Messages/_Forward");
	menu_items[i++].path = _("/_Messages/_Reply");
	menu_items[i++].path = _("/_Messages/_Delete");
	menu_items[i++].path = _("/Messages/Sep3");
	menu_items[i++].path = _("/_Messages/_Business card");
	menu_items[i++].path = _("/_Help");
	menu_items[i++].path = _("/Help/_About");
}


void GUI_CreateSMSWindow(void)
{
	int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar;
	GtkWidget *main_vbox;
	GtkWidget *toolbar;
	GtkWidget *scrolledWindow;
	GtkWidget *vpaned, *hpaned;
	GtkWidget *tree, *treeSMSItem;	/* , *subTree; */
	SortColumn *sColumn;
	GdkColormap *cmap;
	register gint i;
	gchar *titles[4] = { _("Status"), _("Date / Time"), _("Sender"), _("Message") };


	InitMainMenu();
	GUI_SMSWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_SMSWindow), "SMSWindow", "Xgnokii");
	gtk_window_set_title(GTK_WINDOW(GUI_SMSWindow), _("Short Message Service"));
	/*
	gtk_widget_set_usize (GTK_WIDGET (GUI_SMSWindow), 436, 220);
	*/
	gtk_signal_connect(GTK_OBJECT(GUI_SMSWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	gtk_widget_realize(GUI_SMSWindow);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

	gtk_window_add_accel_group(GTK_WINDOW(GUI_SMSWindow), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
	gtk_container_add(GTK_CONTAINER(GUI_SMSWindow), main_vbox);
	gtk_widget_show(main_vbox);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);

	/* Create the toolbar */
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("New message"), NULL,
				NewPixmap(Edit_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) NewSMS, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Forward message"), NULL,
				NewPixmap(Forward_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ForwardSMS, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Reply message"), NULL,
				NewPixmap(Reply_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ReplySMS, NULL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Business Card"), NULL,
				NewPixmap(BCard_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) NewBC, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Delete message"), NULL,
				NewPixmap(Delete_xpm, GUI_SMSWindow->window,
					  &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) DelSMS, NULL);

	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_widget_show(toolbar);

	vpaned = gtk_vpaned_new();
//	gtk_paned_set_handle_size(GTK_PANED(vpaned), 10);
//	gtk_paned_set_gutter_size(GTK_PANED(vpaned), 15);
	gtk_box_pack_end(GTK_BOX(main_vbox), vpaned, TRUE, TRUE, 0);
	gtk_widget_show(vpaned);

	hpaned = gtk_hpaned_new();
//	gtk_paned_set_handle_size(GTK_PANED(hpaned), 8);
//	gtk_paned_set_gutter_size(GTK_PANED(hpaned), 10);
	gtk_paned_add1(GTK_PANED(vpaned), hpaned);
	gtk_widget_show(hpaned);

	/* Navigation tree */
	tree = gtk_tree_new();
	gtk_tree_set_selection_mode(GTK_TREE(tree), GTK_SELECTION_SINGLE);
	gtk_widget_show(tree);

	treeSMSItem = gtk_tree_item_new_with_label(_("SMS's"));
	gtk_tree_append(GTK_TREE(tree), treeSMSItem);
	gtk_widget_show(treeSMSItem);

	subTree = gtk_tree_new();
	gtk_tree_set_selection_mode(GTK_TREE(subTree), GTK_SELECTION_SINGLE);
	gtk_tree_set_view_mode(GTK_TREE(subTree), GTK_TREE_VIEW_ITEM);
	gtk_tree_item_set_subtree(GTK_TREE_ITEM(treeSMSItem), subTree);

	scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolledWindow, 175, 180);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_paned_add1(GTK_PANED(hpaned), scrolledWindow);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledWindow), tree);
	gtk_widget_show(scrolledWindow);


	/* Message viewer */
	SMS.smsText = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(SMS.smsText), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(SMS.smsText), GTK_WRAP_WORD);

	scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_paned_add2(GTK_PANED(hpaned), scrolledWindow);

	gtk_container_add(GTK_CONTAINER(scrolledWindow), SMS.smsText);
	gtk_widget_show_all(scrolledWindow);

	/* Messages list */
	SMS.smsClist = gtk_clist_new_with_titles(4, titles);
	gtk_clist_set_shadow_type(GTK_CLIST(SMS.smsClist), GTK_SHADOW_OUT);
	gtk_clist_set_compare_func(GTK_CLIST(SMS.smsClist), CListCompareFunc);
	gtk_clist_set_sort_column(GTK_CLIST(SMS.smsClist), 1);
	gtk_clist_set_sort_type(GTK_CLIST(SMS.smsClist), GTK_SORT_ASCENDING);
	gtk_clist_set_auto_sort(GTK_CLIST(SMS.smsClist), FALSE);
	gtk_clist_set_selection_mode(GTK_CLIST(SMS.smsClist), GTK_SELECTION_EXTENDED);

	gtk_clist_set_column_width(GTK_CLIST(SMS.smsClist), 0, 70);
	gtk_clist_set_column_width(GTK_CLIST(SMS.smsClist), 1, 142);
	gtk_clist_set_column_width(GTK_CLIST(SMS.smsClist), 2, 135);
	/*
	gtk_clist_set_column_justification (GTK_CLIST (SMS.smsClist), 2, GTK_JUSTIFY_CENTER);
	*/
	for (i = 0; i < 4; i++) {
		if ((sColumn = g_malloc(sizeof(SortColumn))) == NULL) {
			g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__,
				__LINE__);
			gtk_main_quit();
		}
		sColumn->clist = SMS.smsClist;
		sColumn->column = i;
		gtk_signal_connect(GTK_OBJECT(GTK_CLIST(SMS.smsClist)->column[i].button), "clicked",
				   GTK_SIGNAL_FUNC(SetSortColumn), (gpointer) sColumn);
	}

	gtk_signal_connect(GTK_OBJECT(SMS.smsClist), "select_row",
			   GTK_SIGNAL_FUNC(ClickEntry), NULL);

	scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolledWindow, 550, 100);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), SMS.smsClist);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_paned_add2(GTK_PANED(vpaned), scrolledWindow);

	gtk_widget_show(SMS.smsClist);
	gtk_widget_show(scrolledWindow);

	CreateErrorDialog(&errorDialog, GUI_SMSWindow);
	CreateInfoDialog(&infoDialog, GUI_SMSWindow);

	gtk_signal_emit_by_name(GTK_OBJECT(treeSMSItem), "expand");

	cmap = gdk_colormap_get_system();
	SMS.colour.red = 0xffff;
	SMS.colour.green = 0;
	SMS.colour.blue = 0;
	if (!gdk_color_alloc(cmap, &(SMS.colour)))
		g_error(_("couldn't allocate colour"));

	questMark.pixmap = gdk_pixmap_create_from_xpm_d(GUI_SMSWindow->window,
							&questMark.mask,
							&GUI_SMSWindow->style->bg[GTK_STATE_NORMAL],
							quest_xpm);

	GUIEventAdd(GUI_EVENT_SMS_CENTERS_CHANGED, GUI_RefreshsmscenterMenu);
	GUIEventAdd(GUI_EVENT_SMS_NUMBER_CHANGED, GUI_RefreshMessageWindow);
}
