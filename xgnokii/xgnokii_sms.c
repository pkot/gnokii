/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sun Apr 30 2000
  Modified by Jan Derfinak

*/
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "misc.h"
#include "gsm-api.h"
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
  gint  count;		// Messages count
  gint  number;		// Number of tail in Long SMS;
  gint *msgPtr;		// Array of MessageNumber;
  gint  validity;
  gint  class;
  gchar sender[GSM_MAX_SENDER_LENGTH + 1];
} MessagePointers;

typedef struct {
  gchar *number;
  gchar *name;
  gint used:1;
} AddressPar;

typedef struct {
  GtkWidget *smsClist;
  GtkWidget *smsText;
  GSList    *messages;
  GdkColor   colour;
  gint       row_i;
  gint       currentBox;
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
  gint       center;
  GSList    *addressLine;
} SendSMSWidget;

static GtkWidget *GUI_SMSWindow;
static SMSWidget SMS = {NULL, NULL, NULL};
static SendSMSWidget sendSMS = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL};
static ErrorDialog errorDialog = {NULL, NULL};
static InfoDialog infoDialog = {NULL, NULL};
static QuestMark questMark;


static inline void Help1 (GtkWidget *w, gpointer data)
{
  gchar *indx = g_strdup_printf ("/help/%s/sms.html", xgnokiiConfig.locale);
  Help (w, indx);
  g_free (indx);
}


static inline void Help2 (GtkWidget *w, gpointer data)
{
  gchar *indx = g_strdup_printf ("/help/%s/sms_send.html", xgnokiiConfig.locale);
  Help (w, indx);
  g_free (indx);
}


static inline void CloseSMS (GtkWidget *w, gpointer data)
{
  gtk_widget_hide (GUI_SMSWindow);
}


static inline void CloseSMSSend (GtkWidget *w, gpointer data)
{
  gtk_widget_hide (sendSMS.SMSSendWindow);
}


static gint CListCompareFunc (GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
  char *text1 = NULL;
  char *text2 = NULL;

  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  switch (row1->cell[clist->sort_column].type)
  {
    case GTK_CELL_TEXT:
      text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
      break;
    case GTK_CELL_PIXTEXT:
      text1 = GTK_CELL_PIXTEXT (row1->cell[clist->sort_column])->text;
      break;
    default:
      break;
  }
  switch (row2->cell[clist->sort_column].type)
  {
    case GTK_CELL_TEXT:
      text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;
      break;
    case GTK_CELL_PIXTEXT:
      text2 = GTK_CELL_PIXTEXT (row2->cell[clist->sort_column])->text;
      break;
    default:
      break;
  }

  if (!text2)
    return (text1 != NULL);

  if (!text1)
    return -1;

  if (clist->sort_column == 1 && !SMS.currentBox)
  {
    struct tm bdTime;
    time_t time1, time2;

    bdTime.tm_sec  = atoi (text1 + 15);
    bdTime.tm_min  = atoi (text1 + 12);
    bdTime.tm_hour = atoi (text1 + 9);
    bdTime.tm_mday = atoi (text1);
    bdTime.tm_mon  = atoi (text1 + 3);
    bdTime.tm_year = atoi (text1 + 6);
    if (bdTime.tm_year < 70)
      bdTime.tm_year += 100;
    bdTime.tm_gmtoff = atoi (text1 + 21);
    bdTime.tm_isdst = -1;

    time1 = mktime (&bdTime);

    bdTime.tm_sec  = atoi (text2 + 15);
    bdTime.tm_min  = atoi (text2 + 12);
    bdTime.tm_hour = atoi (text2 + 9);
    bdTime.tm_mday = atoi (text2);
    bdTime.tm_mon  = atoi (text2 + 3);
    bdTime.tm_year = atoi (text2 + 6);
    if (bdTime.tm_year < 70)
      bdTime.tm_year += 100;
    bdTime.tm_gmtoff = atoi (text2 + 21);
    bdTime.tm_isdst = -1;

    time2 = mktime (&bdTime);

    if (time1 < time2)
      return (1);
    else if (time1 > time2)
      return (-1);
    else 
      return 0;
  }

  return (g_strcasecmp (text1, text2));
}


static inline void SetSortColumn (GtkWidget *widget, SortColumn *data)
{
  gtk_clist_set_sort_column (GTK_CLIST (data->clist), data->column);
  gtk_clist_sort (GTK_CLIST (data->clist));
}


static inline void DestroyMsgPtrs (gpointer data)
{
  g_free (((MessagePointers *) data)->msgPtr);
  g_free ((MessagePointers *) data);
}


static void InsertInboxElement (gpointer d, gpointer userData)
{
  GSM_SMSMessage *data = (GSM_SMSMessage *) d;
  MessagePointers *msgPtrs;

  if (data->Type == GST_MT || data->Type == GST_DR)
  {
/*    if (data->Type == GST_MT && data->UDHType == GSM_ConcatenatedMessages)
    {
      //FIX ME

      msgPtrs = (MessagePointers *) g_malloc (sizeof (MessagePointers));
      msgPtrs->count = data->UDH[4];
      msgPtrs->number = data->UDH[5];
      msgPtrs->validity = data->Validity;
      msgPtrs->class = data->Class;
      strcpy (msgPtrs->sender, data->Sender);
      msgPtrs->msgPtr = (gint *) g_malloc (msgPtrs->count * sizeof (gint));
      *(msgPtrs->msgPtr + msgPtrs->number - 1) = data->MessageNumber;
    }
    else */
    {
      gchar *row[4];

      if (data->Type == GST_DR)
        row[0] = g_strdup (_("report"));
      else if (data->Status)
        row[0] = g_strdup (_("read"));
      else
        row[0] = g_strdup (_("unread"));

      if (data->Time.Timezone)
        row[1] = g_strdup_printf ("%02d/%02d/%02d %02d:%02d:%02d GMT%+dh",
                                  data->Time.Day, data->Time.Month, data->Time.Year,
                                  data->Time.Hour, data->Time.Minute, data->Time.Second,
                                  data->Time.Timezone);
      else
        row[1] = g_strdup_printf ("%02d/%02d/%02d %02d:%02d:%02d GMT",
                                  data->Time.Day, data->Time.Month, data->Time.Year,
                                  data->Time.Hour, data->Time.Minute, data->Time.Second);

      row[2] = GUI_GetName (data->Sender);
      if (row[2] == NULL)
        row[2] = data->Sender;
      row[3] = data->MessageText;

      gtk_clist_append (GTK_CLIST (SMS.smsClist), row);
      msgPtrs = (MessagePointers *) g_malloc (sizeof (MessagePointers));
      msgPtrs->count = msgPtrs->number = 1;
      msgPtrs->validity = data->Validity;
      msgPtrs->class = data->Class;
      strcpy (msgPtrs->sender, data->Sender);
      msgPtrs->msgPtr = (gint *) g_malloc (sizeof (gint));
      *(msgPtrs->msgPtr) = data->MessageNumber;
      gtk_clist_set_row_data_full (GTK_CLIST (SMS.smsClist), SMS.row_i++,
                                   msgPtrs, DestroyMsgPtrs);
      g_free (row[0]);
      g_free (row[1]);
    }
  }
}


static inline void RefreshInbox (void)
{
  gtk_clist_freeze (GTK_CLIST (SMS.smsClist));

  gtk_clist_clear (GTK_CLIST (SMS.smsClist));

  SMS.row_i = 0;
  g_slist_foreach (phoneMonitor.sms.messages, InsertInboxElement, (gpointer) NULL);

  gtk_clist_sort (GTK_CLIST (SMS.smsClist));
  gtk_clist_thaw (GTK_CLIST (SMS.smsClist));
}


static void InsertOutboxElement (gpointer d, gpointer userData)
{
  GSM_SMSMessage *data = (GSM_SMSMessage *) d;
  MessagePointers *msgPtrs;

  if (data->Type == GST_MO)
  {
    gchar *row[4];

    if (data->Status)
      row[0] = g_strdup (_("sent"));
    else
      row[0] = g_strdup (_("unsent"));

    row[1] = row[2] = g_strdup ("");
    row[3] = data->MessageText;

    gtk_clist_append( GTK_CLIST (SMS.smsClist), row);
    msgPtrs = (MessagePointers *) g_malloc (sizeof (MessagePointers));
    msgPtrs->count = msgPtrs->number = 1;
    msgPtrs->validity = data->Validity;
    msgPtrs->class = data->Class;
    strcpy (msgPtrs->sender, data->Sender);
    msgPtrs->msgPtr = (gint *) g_malloc (sizeof (gint));
    *(msgPtrs->msgPtr) = data->MessageNumber;
    gtk_clist_set_row_data_full (GTK_CLIST (SMS.smsClist), SMS.row_i++,
                                 msgPtrs, DestroyMsgPtrs);
    g_free (row[0]);
    g_free (row[1]);
  }
}


static inline void RefreshOutbox (void)
{
  gtk_clist_freeze (GTK_CLIST (SMS.smsClist));
  gtk_clist_clear (GTK_CLIST (SMS.smsClist));

  SMS.row_i = 0;
  g_slist_foreach (phoneMonitor.sms.messages, InsertOutboxElement, (gpointer) NULL);

  gtk_clist_sort (GTK_CLIST (SMS.smsClist));
  gtk_clist_thaw (GTK_CLIST (SMS.smsClist));
}


inline void GUI_RefreshMessageWindow (void)
{
  if (!GTK_WIDGET_VISIBLE (GUI_SMSWindow))
    return;

  if (SMS.currentBox)
    RefreshOutbox ();
  else
    RefreshInbox ();
}


static void SelectTreeItem (GtkWidget *item, gpointer data)
{
  SMS.currentBox = GPOINTER_TO_INT (data);
  GUI_RefreshMessageWindow ();
}


static void ClickEntry (GtkWidget      *clist,
                        gint            row,
                        gint            column,
                        GdkEventButton *event,
                        gpointer        data )
{
  gchar *buf;

  /* FIXME - We must mark SMS as readed */
  gtk_text_freeze (GTK_TEXT (SMS.smsText));

  gtk_text_set_point (GTK_TEXT (SMS.smsText), 0);
  gtk_text_forward_delete (GTK_TEXT (SMS.smsText), gtk_text_get_length (GTK_TEXT (SMS.smsText)));

  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.colour), NULL,
                   _("From: "), -1);
  gtk_clist_get_text (GTK_CLIST (clist), row, 2, &buf);
  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.smsText->style->black), NULL,
                   buf, -1);
  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.smsText->style->black), NULL,
                   "\n", -1);

  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.colour), NULL,
                   _("Date: "), -1);
  gtk_clist_get_text (GTK_CLIST (clist), row, 1, &buf);
  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.smsText->style->black), NULL,
                   buf, -1);
  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.smsText->style->black), NULL,
                   "\n\n", -1);

  gtk_clist_get_text (GTK_CLIST (clist), row, 3, &buf);
  gtk_text_insert (GTK_TEXT (SMS.smsText), NULL, &(SMS.smsText->style->black), NULL,
                   buf, -1);

  gtk_text_thaw (GTK_TEXT (SMS.smsText));
}


inline void GUI_ShowSMS (void)
{
  gtk_widget_show (GUI_SMSWindow);
  GUI_RefreshMessageWindow ();
}


static void OkDeleteSMSDialog (GtkWidget *widget, gpointer data)
{
  GSM_SMSMessage *message;
  PhoneEvent *e;
  GList *sel;
//  GSM_Error error;
  gint row;
  gint count;


  sel = GTK_CLIST (SMS.smsClist)->selection;

  gtk_clist_freeze(GTK_CLIST (SMS.smsClist));

  while (sel != NULL)
  {
    row = GPOINTER_TO_INT(sel->data);
    sel = sel->next;
    for (count = 0; count < ((MessagePointers *) gtk_clist_get_row_data (GTK_CLIST (SMS.smsClist), row))->count; count++)
    {
      message = (GSM_SMSMessage *) g_malloc (sizeof (GSM_SMSMessage));
      message->Location = *(((MessagePointers *) gtk_clist_get_row_data (GTK_CLIST (SMS.smsClist), row))->msgPtr + count);
      if (message->Location == -1)
      {
        g_free (message);
        continue;
      }
      message->MemoryType = GMT_SM;

      e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
      e->event = Event_DeleteSMSMessage;
      e->data = message;
      GUI_InsertEvent (e);

/*      error = GSM->DeleteSMSMessage(&message);

      if (error != GE_NONE)
      {
        if (error == GE_NOTIMPLEMENTED)
        {
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

  gtk_widget_hide(GTK_WIDGET (data));

  gtk_clist_thaw(GTK_CLIST (SMS.smsClist));
}


void DeleteSMS(void)
{
  static GtkWidget *dialog = NULL;
  GtkWidget *button, *hbox, *label, *pixmap;

  if (dialog == NULL)
  {
    dialog = gtk_dialog_new();
    gtk_window_set_title (GTK_WINDOW (dialog), _("Delete SMS"));
    gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal(GTK_WINDOW (dialog), TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                        GTK_SIGNAL_FUNC (DeleteEvent), NULL);

    button = gtk_button_new_with_label (_("Ok"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
                        button, TRUE, TRUE, 10);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (OkDeleteSMSDialog), (gpointer) dialog);
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

    pixmap = gtk_pixmap_new (questMark.pixmap, questMark.mask);
    gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 10);
    gtk_widget_show(pixmap);

    label = gtk_label_new(_("Do you want delete selected SMS?"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
    gtk_widget_show(label);
  }

  gtk_widget_show(GTK_WIDGET (dialog));
}


static inline void RefreshSMSStatus (void)
{
  gchar *buf;
  guint l, m;

  l = gtk_text_get_length (GTK_TEXT (sendSMS.smsSendText));

  if (l <= GSM_MAX_SMS_LENGTH)
    buf = g_strdup_printf ("%d/1", l);
  else
  {
    m = l % 153;
    buf = g_strdup_printf ("%d/%d", l > 0 && !m ? 153 : m, l == 0 ? 1 : ((l - 1) / 153) + 1);
  }

  gtk_frame_set_label (GTK_FRAME (sendSMS.status), buf);
  g_free (buf);
}


static inline gint RefreshSMSLength (GtkWidget   *widget,
                                     GdkEventKey *event,
                                     gpointer     callback_data)
{
  RefreshSMSStatus ();
  if (GTK_EDITABLE (widget)->editable == FALSE)
    return (FALSE);
  if (event->keyval == GDK_BackSpace || event->keyval == GDK_Clear ||
      event->keyval == GDK_Insert || event->keyval == GDK_Delete ||
      event->keyval == GDK_Home || event->keyval == GDK_End ||
      event->keyval == GDK_Left || event->keyval == GDK_Right ||
      event->keyval == GDK_Up || event->keyval == GDK_Down ||
      event->keyval == GDK_Return ||
      (event->keyval >= 0x20 && event->keyval <= 0xFF))
    return (TRUE);
  
  return (FALSE);
}


static inline void SetActiveCenter (GtkWidget *item, gpointer data)
{
  sendSMS.center = GPOINTER_TO_INT (data);
}


void GUI_RefreshSMSCenterMenu (void)
{
  static GtkWidget *smscMenu = NULL;
  GtkWidget *item;
  register gint i;

  if (!sendSMS.smscOptionMenu)
    return;

  if (smscMenu)
  {
    gtk_option_menu_remove_menu (GTK_OPTION_MENU (sendSMS.smscOptionMenu));
    if (GTK_IS_WIDGET (smscMenu))
      gtk_widget_destroy (GTK_WIDGET (smscMenu));
    smscMenu = NULL;
  }

  smscMenu = gtk_menu_new();

  for (i = 0; i < xgnokiiConfig.smsSets; i++)
  {
    if (*(xgnokiiConfig.smsSetting[i].Name) == '\0')
    {
      gchar *buf = g_strdup_printf (_("Set %d"), i + 1);
      item = gtk_menu_item_new_with_label (buf);
      g_free (buf);
    }
    else
      item = gtk_menu_item_new_with_label (xgnokiiConfig.smsSetting[i].Name);

    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC (SetActiveCenter),
                        (gpointer) i);

    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (smscMenu), item);
  }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (sendSMS.smscOptionMenu), smscMenu);
}


static inline void InitAddressLine (gpointer d, gpointer userData)
{
  ((AddressPar *) d)->used = 0;
}


#ifdef XDEBUG
static inline void PrintAddressLine (gpointer d, gpointer userData)
{
  g_print ("Name: %s\nNumber: %s\nUsed: %d\n",
           ((AddressPar *) d)->name,
           ((AddressPar *) d)->number,
           ((AddressPar *) d)->used);
}
#endif


static inline gint CompareAddressLineName (gconstpointer a, gconstpointer b)
{
  return strcmp (((AddressPar *) a)->name, ((AddressPar *) b)->name);
}


static inline gint CompareAddressLineUsed (gconstpointer a, gconstpointer b)
{
  return !(((AddressPar *) a)->used == ((AddressPar *) b)->used);
}


static gint CheckAddressMain (void)
{
  AddressPar aps;
  GSList *r;
  register gint i = 0;
  gint ret = 0;
  gchar *buf;
  GString *tooltipBuf;
  gchar **strings = g_strsplit (gtk_entry_get_text (GTK_ENTRY (sendSMS.addr)), ",", 0);

  tooltipBuf = g_string_new ("");
  gtk_entry_set_text (GTK_ENTRY (sendSMS.addr), "");
  g_slist_foreach (sendSMS.addressLine, InitAddressLine, (gpointer) NULL);

  while (strings[i])
  {
    g_strstrip (strings[i]);
    if (*strings[i] == '\0')
    { 
      i++;
      continue;
    }
    if ((buf = GUI_GetName (strings[i])) != NULL)
    {
      AddressPar *ap = g_malloc (sizeof (AddressPar));
      ap->number = g_strdup (strings[i]);
      ap->name = g_strdup (buf);
      ap->used = 1;
      sendSMS.addressLine = g_slist_append (sendSMS.addressLine, ap);
      gtk_entry_append_text (GTK_ENTRY (sendSMS.addr), buf);
      g_string_append (tooltipBuf, ap->number);
    }
    else 
    if ((buf = GUI_GetNumber (strings[i])) != NULL)
    {
      aps.name = strings[i];
      r = g_slist_find_custom (sendSMS.addressLine, &aps, CompareAddressLineName);
      if (r)
      {
        ((AddressPar *) r->data)->used = 1;
        g_string_append (tooltipBuf, ((AddressPar *) r->data)->number);
      }
      else
        g_string_append (tooltipBuf, buf);
      gtk_entry_append_text (GTK_ENTRY (sendSMS.addr), strings[i]);
    }
    else
    {
      gint len = strlen (strings[i]);
      gint j;

      for (j = 0; j < len; j++)
        if (strings[i][j] != '*' && strings[i][j] != '#' && strings[i][j] != '+'
            && (strings[i][j] < '0' || strings[i][j] > '9'))
          ret = 1;
      gtk_entry_append_text (GTK_ENTRY (sendSMS.addr), strings[i]);
      g_string_append (tooltipBuf, strings[i]);
    }
    if (strings[i + 1] != NULL)
    {
      gtk_entry_append_text (GTK_ENTRY (sendSMS.addr), ", ");
      g_string_append (tooltipBuf, ", ");
    }
    i++;
  }

  aps.used = 0;
  while ((r = g_slist_find_custom (sendSMS.addressLine, &aps, CompareAddressLineUsed)))
    sendSMS.addressLine = g_slist_remove (sendSMS.addressLine, r->data);
  
  if (tooltipBuf->len > 0)
  {
    gtk_tooltips_set_tip (sendSMS.addrTip, sendSMS.addr, tooltipBuf->str, NULL);
    gtk_tooltips_enable (sendSMS.addrTip);
  }
  else
    gtk_tooltips_disable (sendSMS.addrTip);

#ifdef XDEBUG  
  g_slist_foreach (sendSMS.addressLine, PrintAddressLine, (gpointer) NULL);
#endif

  g_strfreev (strings);
  g_string_free (tooltipBuf, TRUE);

  return ret;
}


static inline void CheckAddress (void)
{
  if (CheckAddressMain ())
  {
    gtk_label_set_text(GTK_LABEL(errorDialog.text), _("Address line contains illegal address!"));
    gtk_widget_show(errorDialog.dialog);
  }
}


static void DeleteSelectContactDialog (GtkWidget *widget, GdkEvent *event,
                                       SelectContactData *data)
{
  gtk_widget_destroy (GTK_WIDGET (data->clist));
  gtk_widget_destroy (GTK_WIDGET (data->clistScrolledWindow));
  gtk_widget_destroy (GTK_WIDGET (widget));
}


static void CancelSelectContactDialog (GtkWidget *widget,
                                       SelectContactData *data)
{
  gtk_widget_destroy (GTK_WIDGET (data->clist));
  gtk_widget_destroy (GTK_WIDGET (data->clistScrolledWindow));
  gtk_widget_destroy (GTK_WIDGET (data->dialog));
}


static void OkSelectContactDialog (GtkWidget *widget,
                                   SelectContactData *data)
{
  GList *sel;
  PhonebookEntry *pbEntry;

  if ((sel = GTK_CLIST (data->clist)->selection) != NULL)
    while (sel != NULL)
    {
      AddressPar *ap = g_malloc (sizeof (AddressPar));
      
      pbEntry = gtk_clist_get_row_data (GTK_CLIST (data->clist),
                                        GPOINTER_TO_INT (sel->data));
      ap->number = g_strdup (pbEntry->entry.Number);
      ap->name = g_strdup (pbEntry->entry.Name);
      ap->used = 1;
      sendSMS.addressLine = g_slist_append (sendSMS.addressLine, ap);
      if (strlen (gtk_entry_get_text (GTK_ENTRY (sendSMS.addr))) > 0)
        gtk_entry_append_text (GTK_ENTRY (sendSMS.addr), ", ");
      gtk_entry_append_text (GTK_ENTRY (sendSMS.addr), ap->name);

      sel = sel->next;
    }

  gtk_widget_destroy (GTK_WIDGET (data->clist));
  gtk_widget_destroy (GTK_WIDGET (data->clistScrolledWindow));
  gtk_widget_destroy (GTK_WIDGET (data->dialog));

  CheckAddressMain ();
}


static void ShowSelectContactsDialog (void)
{
  SelectContactData *r;

  if (!GUI_ContactsIsIntialized ())
    GUI_ReadContacts ();

  if ((r = GUI_SelectContactDialog ()) == NULL)
    return;

  gtk_signal_connect (GTK_OBJECT (r->dialog), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteSelectContactDialog), (gpointer) r);

  gtk_signal_connect (GTK_OBJECT (r->okButton), "clicked",
                      GTK_SIGNAL_FUNC (OkSelectContactDialog), (gpointer) r);
  gtk_signal_connect (GTK_OBJECT (r->cancelButton), "clicked",
                      GTK_SIGNAL_FUNC (CancelSelectContactDialog), (gpointer) r);
}


static gint SendSMSCore (GSM_SMSMessage *sms)
{
  GSM_Error error;
  PhoneEvent *e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
  D_SMSMessage *m = (D_SMSMessage *) g_malloc (sizeof (D_SMSMessage));
  
  m->sms = sms;
  e->event = Event_SendSMSMessage;
  e->data = m;
  GUI_InsertEvent (e);
  pthread_mutex_lock (&sendSMSMutex);
  pthread_cond_wait (&sendSMSCond, &sendSMSMutex);
  pthread_mutex_unlock (&sendSMSMutex);

#ifdef XDEBUG
  g_print ("Address: %s\nText: %s\nDelivery report: %d\nSMS Center: %d\n",
           sms->Destination,
           sms->MessageText,
           GTK_TOGGLE_BUTTON (sendSMS.report)->active,
           sendSMS.center);
#endif

  error = m->status;
  g_free (m);

  if (error != GE_SMSSENDOK)
  {
    gchar *buf = g_strdup_printf (_("SMS send to %s failed\n(error=%d)"),
                                  sms->Destination, error);
    gtk_label_set_text (GTK_LABEL(errorDialog.text), buf);
    gtk_widget_show (errorDialog.dialog);
    g_free (buf);
  }
  else
    g_print ("Message sent to: %s\n", sms->Destination);

  return (error);
}


static void SendSMS (void)
{
  GSM_SMSMessage sms;
  AddressPar aps;
  char udh[GSM_MAX_USER_DATA_HEADER_LENGTH];
  GSList *r;
  gchar *text, *number;
  gchar **addresses;
  gchar *buf;
  gint offset, nr_msg, l;
  gint longSMS;
  register gint i = 0, j;

  if (CheckAddressMain ())
  {
    gtk_label_set_text(GTK_LABEL(errorDialog.text), _("Address line contains illegal address!"));
    gtk_widget_show(errorDialog.dialog);
    return;
  }

  text = gtk_editable_get_chars (GTK_EDITABLE (sendSMS.smsSendText), 0, -1);
  l = strlen (text);

  addresses = g_strsplit (gtk_entry_get_text (GTK_ENTRY (sendSMS.addr)), ",", 0);

  longSMS = GTK_TOGGLE_BUTTON (sendSMS.longSMS)->active;

  while (addresses[i])
  {
    g_strstrip (addresses[i]);
    if ((number = GUI_GetNumber (addresses[i])))
    {
      aps.name = addresses[i];
      if ((r = g_slist_find_custom (sendSMS.addressLine, &aps, CompareAddressLineName)))
        number = ((AddressPar *) r->data)->number;
    }
    else
      number = addresses[i];

    sms.MessageCenter = xgnokiiConfig.smsSetting[sendSMS.center];
    sms.MessageCenter.No = 0;

    if (GTK_TOGGLE_BUTTON (sendSMS.report)->active)
      sms.Type = GST_DR;
    else
      sms.Type = GST_MO;

    sms.Class = -1;
    sms.Compression = false;
    sms.EightBit = false;
    sms.Validity = sms.MessageCenter.Validity;

    strncpy (sms.Destination, number, GSM_MAX_DESTINATION_LENGTH + 1);
    sms.Destination[GSM_MAX_DESTINATION_LENGTH] = '\0';

    if (l > GSM_MAX_SMS_LENGTH)
    {
      if (longSMS)
      {
        sms.UDHType = GSM_ConcatenatedMessages;
        nr_msg = ((l - 1) / 153) + 1;
        udh[0] = 0x05;	// UDH length
        udh[1] = 0x00;	// concatenated messages (IEI)
        udh[2] = 0x03;	// IEI data length
        udh[3] = 0x01;	// reference number
        udh[4] = nr_msg;	// number of messages
        udh[5] = 0x00;	// message reference number
        offset = 6;

        for (j = 0; j < nr_msg; j++)
        {
          udh[5] = j + 1;

          memcpy(sms.UDH,udh,offset);
          strncpy (sms.MessageText, text + (j * 153), 153);
          sms.MessageText[153] = '\0';

          buf = g_strdup_printf (_("Sending SMS to %s (%d/%d) ...\n"),
                                 sms.Destination, j + 1, nr_msg);
          gtk_label_set_text (GTK_LABEL (infoDialog.text), buf);
          gtk_widget_show_now (infoDialog.dialog);
          g_free (buf);
          GUI_Refresh ();

          if (SendSMSCore (&sms) != GE_SMSSENDOK)
          {
            gtk_widget_hide (infoDialog.dialog);
            GUI_Refresh ();
            break;
          }

          gtk_widget_hide (infoDialog.dialog);
          GUI_Refresh ();

          sleep (1);
        }
      }
      else
      {
        sms.UDHType = GSM_NoUDH;
        nr_msg = ((l - 1) / 153) + 1;
        if (nr_msg > 99) // We have place only for 99 messages in header.
          nr_msg = 99;
        for (j = 0; j < nr_msg; j++)
        {
          gchar header[8];

          g_snprintf (header, 8, "%2d/%-2d: ", j + 1, nr_msg);
          header[7] = '\0';

          strcpy (sms.MessageText, header);
          strncat (sms.MessageText, text + (j * 153), 153);
          sms.MessageText[160] = '\0';

          buf = g_strdup_printf (_("Sending SMS to %s (%d/%d) ...\n"),
                                 sms.Destination, j + 1, nr_msg);
          gtk_label_set_text (GTK_LABEL (infoDialog.text), buf);
          gtk_widget_show_now (infoDialog.dialog);
          g_free (buf);
          GUI_Refresh ();

          if (SendSMSCore (&sms) != GE_SMSSENDOK)
          {
            gtk_widget_hide (infoDialog.dialog);
            GUI_Refresh ();
            break;
          }

          gtk_widget_hide (infoDialog.dialog);
          GUI_Refresh ();

          sleep (1);
        }
      }
    }
    else
    {
      sms.UDHType = GSM_NoUDH;
      strncpy (sms.MessageText, text, GSM_MAX_SMS_LENGTH + 1);
      sms.MessageText[GSM_MAX_SMS_LENGTH] = '\0';

      buf = g_strdup_printf (_("Sending SMS to %s ...\n"), sms.Destination);
      gtk_label_set_text (GTK_LABEL (infoDialog.text), buf);
      gtk_widget_show_now (infoDialog.dialog);
      g_free (buf);
      GUI_Refresh ();

      (void) SendSMSCore (&sms);
      gtk_widget_hide (infoDialog.dialog);
      GUI_Refresh ();
    }

    i++;
  }

  g_strfreev (addresses);

  g_free (text);
}


static GtkItemFactoryEntry send_menu_items[] = {
  { NULL,		NULL,		NULL,		0, "<Branch>"},
  { NULL,		"<control>X",	SendSMS,	0, NULL},
  { NULL,		"<control>S",	NULL,		0, NULL},
  { "/File/sep1",	NULL,		NULL,		0, "<Separator>"},
  { NULL,		"<control>N",	CheckAddress,	0, NULL},
  { NULL,		"<control>C",	ShowSelectContactsDialog, 0, NULL},
  { "/File/sep2",	NULL,		NULL,		0, "<Separator>"},
  { NULL,		"<control>W",	CloseSMSSend,	0, NULL},
  { NULL,		NULL,		NULL,		0, "<LastBranch>"},
  { NULL,		NULL,		Help2,		0, NULL},
  { NULL,		NULL,		GUI_ShowAbout,	0, NULL},
};


static void InitSendMenu (void)
{
  send_menu_items[0].path = g_strdup (_("/_File"));
  send_menu_items[1].path = g_strdup (_("/File/Sen_d"));
  send_menu_items[2].path = g_strdup (_("/File/_Save"));
  send_menu_items[4].path = g_strdup (_("/File/Check _Names"));
  send_menu_items[5].path = g_strdup (_("/File/C_ontacts"));
  send_menu_items[7].path = g_strdup (_("/File/_Close"));
  send_menu_items[8].path = g_strdup (_("/_Help"));
  send_menu_items[9].path = g_strdup (_("/Help/_Help"));
  send_menu_items[10].path = g_strdup (_("/Help/_About"));
}


static void CreateSMSSendWindow (void)
{
  int nmenu_items = sizeof (send_menu_items) / sizeof (send_menu_items[0]);
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

  InitSendMenu ();
  sendSMS.SMSSendWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  //gtk_widget_set_usize (GTK_WIDGET (sendSMS.SMSSendWindow), 436, 220);
  gtk_signal_connect (GTK_OBJECT (sendSMS.SMSSendWindow), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
  gtk_widget_realize (sendSMS.SMSSendWindow);

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
                                       accel_group);

  gtk_item_factory_create_items (item_factory, nmenu_items, send_menu_items, NULL);

  gtk_accel_group_attach (accel_group, GTK_OBJECT (sendSMS.SMSSendWindow));

  /* Finally, return the actual menu bar created by the item factory. */ 
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (sendSMS.SMSSendWindow), main_vbox);
  gtk_widget_show (main_vbox);

  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);

  /* Create the toolbar */

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NORMAL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Send message"), NULL,
                           NewPixmap(SendSMS_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) SendSMS, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Save message to outbox"), NULL,
                           NewPixmap(Send_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Check names"), NULL,
                           NewPixmap(Check_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) CheckAddress, NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /* Address line */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("To:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  sendSMS.addr = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), sendSMS.addr, TRUE, TRUE, 1);

  sendSMS.addrTip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (sendSMS.addrTip, sendSMS.addr, "", NULL);
  gtk_tooltips_disable (sendSMS.addrTip);

  gtk_widget_show (sendSMS.addr);

  button = gtk_button_new ();
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (ShowSelectContactsDialog), (gpointer) NULL);

  pixmap = NewPixmap(Names_xpm, sendSMS.SMSSendWindow->window,
                     &sendSMS.SMSSendWindow->style->bg[GTK_STATE_NORMAL]);
  gtk_container_add (GTK_CONTAINER (button), pixmap);
  gtk_widget_show (pixmap);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 5);

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltips, button, _("Select contacts"), NULL);

  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 3);
  gtk_widget_show (hbox);

  /* Edit SMS widget */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 3);
  gtk_widget_show (vbox);

  sendSMS.status = gtk_frame_new ("0/1");
  gtk_frame_set_label_align (GTK_FRAME (sendSMS.status), 1.0, 0.0);
  gtk_frame_set_shadow_type (GTK_FRAME (sendSMS.status), GTK_SHADOW_OUT);

  gtk_box_pack_end (GTK_BOX (vbox), sendSMS.status, TRUE, TRUE, 5);
  gtk_widget_show (sendSMS.status);

  sendSMS.smsSendText = gtk_text_new (NULL, NULL);
  gtk_widget_set_usize (GTK_WIDGET (sendSMS.smsSendText), 0, 120);
  gtk_text_set_editable (GTK_TEXT (sendSMS.smsSendText), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT (sendSMS.smsSendText), TRUE);
  gtk_signal_connect_after (GTK_OBJECT (sendSMS.smsSendText),
                      "key_press_event",
                      GTK_SIGNAL_FUNC(RefreshSMSLength), (gpointer) NULL);
  gtk_signal_connect_after (GTK_OBJECT (sendSMS.smsSendText),
                      "button_press_event",
                      GTK_SIGNAL_FUNC(RefreshSMSLength), (gpointer) NULL);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledWindow), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolledWindow), sendSMS.smsSendText);
  gtk_container_add (GTK_CONTAINER (sendSMS.status), scrolledWindow);

  gtk_widget_show (sendSMS.smsSendText);
  gtk_widget_show (scrolledWindow);

  /* Options widget */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 5);
  gtk_widget_show (vbox);

  sendSMS.report = gtk_check_button_new_with_label (_("Delivery report"));
  gtk_box_pack_start (GTK_BOX (vbox), sendSMS.report, FALSE, FALSE, 3);
  gtk_widget_show (sendSMS.report);

  sendSMS.longSMS = gtk_check_button_new_with_label (_("Send as Long SMS"));
  gtk_box_pack_start (GTK_BOX (vbox), sendSMS.longSMS, FALSE, FALSE, 3);
  gtk_widget_show (sendSMS.longSMS);

  label = gtk_label_new (_("SMS Center:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 1);
  gtk_widget_show (label);

  GUI_InitSMSSettings ();
  sendSMS.smscOptionMenu = gtk_option_menu_new();

  GUI_RefreshSMSCenterMenu ();

  gtk_box_pack_start (GTK_BOX (vbox), sendSMS.smscOptionMenu, FALSE, FALSE, 1);
  gtk_widget_show (sendSMS.smscOptionMenu);
}


static void NewSMS (void)
{
  if (!sendSMS.SMSSendWindow)
    CreateSMSSendWindow ();

  gtk_window_set_title (GTK_WINDOW (sendSMS.SMSSendWindow), _("New Message"));

  gtk_tooltips_disable (sendSMS.addrTip);

  gtk_text_freeze (GTK_TEXT (sendSMS.smsSendText));
  gtk_text_set_point (GTK_TEXT (sendSMS.smsSendText), 0);
  gtk_text_forward_delete (GTK_TEXT (sendSMS.smsSendText), gtk_text_get_length (GTK_TEXT (sendSMS.smsSendText)));
  gtk_text_thaw (GTK_TEXT (sendSMS.smsSendText));

  gtk_entry_set_text (GTK_ENTRY (sendSMS.addr), "");

  RefreshSMSStatus ();

  gtk_widget_show (sendSMS.SMSSendWindow);
}


static void ForwardSMS (void)
{
  gchar *buf;

  if (GTK_CLIST (SMS.smsClist)->selection == NULL)
    return;

  if (!sendSMS.SMSSendWindow)
    CreateSMSSendWindow ();

  gtk_window_set_title (GTK_WINDOW (sendSMS.SMSSendWindow), _("Forward Message"));

  gtk_tooltips_disable (sendSMS.addrTip);

  gtk_text_freeze (GTK_TEXT (sendSMS.smsSendText));
  gtk_text_set_point (GTK_TEXT (sendSMS.smsSendText), 0);
  gtk_text_forward_delete (GTK_TEXT (sendSMS.smsSendText), gtk_text_get_length (GTK_TEXT (sendSMS.smsSendText)));
  gtk_clist_get_text (GTK_CLIST (SMS.smsClist),
                      GPOINTER_TO_INT(GTK_CLIST (SMS.smsClist)->selection->data),
                      3, &buf);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   buf, -1);

  gtk_text_thaw (GTK_TEXT (sendSMS.smsSendText));

  gtk_entry_set_text (GTK_ENTRY (sendSMS.addr), "");

  RefreshSMSStatus ();

  gtk_widget_show (sendSMS.SMSSendWindow);
}


/*
static inline gint CompareSMSMessageLocation (gconstpointer a, gconstpointer b)
{
  return !(((GSM_SMSMessage *) a)->Location == ((GSM_SMSMessage *) b)->Location);
}
*/


static void ReplySMS (void)
{
  gchar *buf;
//  GSList *r;
//  GSM_SMSMessage msg;

  if (GTK_CLIST (SMS.smsClist)->selection == NULL)
    return;

  if (!sendSMS.SMSSendWindow)
    CreateSMSSendWindow ();

  gtk_window_set_title (GTK_WINDOW (sendSMS.SMSSendWindow), _("Reply Message"));

  gtk_text_freeze (GTK_TEXT (sendSMS.smsSendText));
  gtk_text_set_point (GTK_TEXT (sendSMS.smsSendText), 0);
  gtk_text_forward_delete (GTK_TEXT (sendSMS.smsSendText),
                           gtk_text_get_length (GTK_TEXT (sendSMS.smsSendText)));
  gtk_clist_get_text (GTK_CLIST (SMS.smsClist),
                      GPOINTER_TO_INT(GTK_CLIST (SMS.smsClist)->selection->data),
                      3, &buf);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL,
                   &(sendSMS.smsSendText->style->black), NULL, buf, -1);

  gtk_text_thaw (GTK_TEXT (sendSMS.smsSendText));

  //msg.Location = *(((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST (SMS.smsClist),
  //               GPOINTER_TO_INT (GTK_CLIST (SMS.smsClist)->selection->data)))->msgPtr);

  //r = g_slist_find_custom (SMS.messages, &msg, CompareSMSMessageLocation);
  //if (r)
  gtk_entry_set_text (GTK_ENTRY (sendSMS.addr),
                      ((MessagePointers *) gtk_clist_get_row_data(GTK_CLIST (SMS.smsClist),
                      GPOINTER_TO_INT (GTK_CLIST (SMS.smsClist)->selection->data)))->sender);

  CheckAddressMain ();
  RefreshSMSStatus ();

  gtk_widget_show (sendSMS.SMSSendWindow);
}


static void NewBC (void)
{
  if (!sendSMS.SMSSendWindow)
    CreateSMSSendWindow ();

  gtk_window_set_title (GTK_WINDOW (sendSMS.SMSSendWindow), _("Bussiness Card"));

  gtk_tooltips_disable (sendSMS.addrTip);

  gtk_text_freeze (GTK_TEXT (sendSMS.smsSendText));
  gtk_text_set_point (GTK_TEXT (sendSMS.smsSendText), 0);
  gtk_text_forward_delete (GTK_TEXT (sendSMS.smsSendText), gtk_text_get_length (GTK_TEXT (sendSMS.smsSendText)));

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(SMS.colour), NULL,
                   "Business Card\n", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.name, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   ", ", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.title, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   "\n", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.company, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   "\n\n", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(SMS.colour), NULL,
                   "tel ", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.telephone, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   "\n", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(SMS.colour), NULL,
                   "fax ", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.fax, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   "\n", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.email, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   "\n", -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   xgnokiiConfig.user.address, -1);

  gtk_text_insert (GTK_TEXT (sendSMS.smsSendText), NULL, &(sendSMS.smsSendText->style->black), NULL,
                   "\n", -1);

  gtk_text_thaw (GTK_TEXT (sendSMS.smsSendText));

  gtk_entry_set_text (GTK_ENTRY (sendSMS.addr), "");

  RefreshSMSStatus ();

  gtk_widget_show (sendSMS.SMSSendWindow);
}


static GtkItemFactoryEntry menu_items[] = {
  { NULL,		NULL,		NULL,		0, "<Branch>"},
  { NULL,		"<control>S",	NULL,		0, NULL},
  { "/File/sep1",	NULL,		NULL,		0, "<Separator>"},
//  { NULL,		"<control>X",	GUI_RefreshSMS,	0, NULL},
//  { "/File/sep2",	NULL,		NULL,		0, "<Separator>"},
  { NULL,		"<control>W",	CloseSMS,	0, NULL},
  { NULL,		NULL,		NULL,		0, "<Branch>"},
  { NULL,		"<control>N",	NewSMS,		0, NULL},
  { NULL,		"<control>F",	ForwardSMS,	0, NULL},
  { NULL,		"<control>R",	ReplySMS,	0, NULL},
  { NULL,		"<control>D",	DeleteSMS,	0, NULL},
  { "/Messages/sep3",	NULL,		NULL,		0, "<Separator>"},
  { NULL,		"<control>B",	NewBC,		0, NULL},
  { NULL,		NULL,		NULL,		0, "<LastBranch>"},
  { NULL,		NULL,		Help1,		0, NULL},
  { NULL,		NULL,		GUI_ShowAbout,	0, NULL},
};


static void InitMainMenu (void)
{
  menu_items[0].path = g_strdup (_("/_File"));
  menu_items[1].path = g_strdup (_("/File/_Save"));
 // menu_items[3].path = g_strdup (_("/File/_Refresh"));
  menu_items[3].path = g_strdup (_("/File/_Close"));
  menu_items[4].path = g_strdup (_("/_Messages"));
  menu_items[5].path = g_strdup (_("/_Messages/_New"));
  menu_items[6].path = g_strdup (_("/_Messages/_Forward"));
  menu_items[7].path = g_strdup (_("/_Messages/_Reply"));
  menu_items[8].path = g_strdup (_("/_Messages/_Delete"));
  menu_items[10].path = g_strdup (_("/_Messages/_Bussiness card"));
  menu_items[11].path = g_strdup (_("/_Help"));
  menu_items[12].path = g_strdup (_("/Help/_Help"));
  menu_items[13].path = g_strdup (_("/Help/_About"));
}


void GUI_CreateSMSWindow (void)
{
  int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  GtkWidget *menubar;
  GtkWidget *main_vbox;
  GtkWidget *toolbar;
  GtkWidget *scrolledWindow;
  GtkWidget *vpaned, *hpaned;
  GtkWidget *tree, *treeSMSItem, *treeInboxItem, *treeOutboxItem, *subTree;
  SortColumn *sColumn;
  GdkColormap *cmap;
  register gint i;
  gchar *titles[4] = { _("Status"), _("Date / Time"), _("Sender"), _("Message")};


  InitMainMenu ();
  GUI_SMSWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (GUI_SMSWindow), _("Short Message Service"));
  //gtk_widget_set_usize (GTK_WIDGET (GUI_SMSWindow), 436, 220);
  gtk_signal_connect (GTK_OBJECT (GUI_SMSWindow), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
  gtk_widget_realize (GUI_SMSWindow);

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
                                       accel_group);

  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  gtk_accel_group_attach (accel_group, GTK_OBJECT (GUI_SMSWindow));

  /* Finally, return the actual menu bar created by the item factory. */ 
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (GUI_SMSWindow), main_vbox);
  gtk_widget_show (main_vbox);

  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);

  /* Create the toolbar */

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NORMAL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("New message"), NULL,
                           NewPixmap(Edit_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NewSMS, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Forward Message"), NULL,
                           NewPixmap(Forward_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) ForwardSMS, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Reply message"), NULL,
                           NewPixmap(Reply_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) ReplySMS, NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Bussiness Card"), NULL,
                           NewPixmap(BCard_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NewBC, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Delete message"), NULL,
                           NewPixmap(Delete_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) DeleteSMS, NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  vpaned = gtk_vpaned_new ();
  gtk_paned_set_handle_size (GTK_PANED (vpaned), 10);
  gtk_paned_set_gutter_size (GTK_PANED (vpaned), 15);
  gtk_box_pack_end (GTK_BOX (main_vbox), vpaned, TRUE, TRUE, 0);
  gtk_widget_show (vpaned);

  hpaned = gtk_hpaned_new ();
  gtk_paned_set_handle_size (GTK_PANED (hpaned), 8);
  gtk_paned_set_gutter_size (GTK_PANED (hpaned), 10);                       
  gtk_paned_add1 (GTK_PANED (vpaned), hpaned);
  gtk_widget_show (hpaned);

  /* Navigation tree */
  tree = gtk_tree_new ();
  gtk_tree_set_selection_mode (GTK_TREE (tree), GTK_SELECTION_SINGLE);
  gtk_widget_show (tree);

  treeSMSItem = gtk_tree_item_new_with_label (_("SMS's"));
  gtk_tree_append (GTK_TREE (tree), treeSMSItem);
  gtk_widget_show (treeSMSItem);

  subTree = gtk_tree_new ();
  gtk_tree_set_selection_mode (GTK_TREE (subTree), GTK_SELECTION_SINGLE);
  gtk_tree_set_view_mode (GTK_TREE (subTree), GTK_TREE_VIEW_ITEM);
  gtk_tree_item_set_subtree (GTK_TREE_ITEM (treeSMSItem), subTree);

  treeInboxItem = gtk_tree_item_new_with_label (_("Inbox"));
  gtk_tree_append (GTK_TREE (subTree), treeInboxItem);
  gtk_signal_connect (GTK_OBJECT (treeInboxItem), "select",
                      GTK_SIGNAL_FUNC (SelectTreeItem), GINT_TO_POINTER (0));
  gtk_widget_show (treeInboxItem);

  treeOutboxItem = gtk_tree_item_new_with_label (_("Outbox"));
  gtk_tree_append (GTK_TREE (subTree), treeOutboxItem);
  gtk_signal_connect (GTK_OBJECT (treeOutboxItem), "select",
                      GTK_SIGNAL_FUNC (SelectTreeItem), GINT_TO_POINTER (1));
  gtk_widget_show (treeOutboxItem);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolledWindow, 75, 80);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_paned_add1 (GTK_PANED (hpaned), scrolledWindow);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledWindow),
                                         tree);
  gtk_widget_show (scrolledWindow);


  /* Message viewer */
  SMS.smsText = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (SMS.smsText), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (SMS.smsText), TRUE);
  //gtk_text_set_line_wrap (GTK_TEXT (SMS.smsText), TRUE);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_paned_add2 (GTK_PANED (hpaned), scrolledWindow);

  gtk_container_add (GTK_CONTAINER (scrolledWindow), SMS.smsText);
  gtk_widget_show_all (scrolledWindow);

  /* Messages list */
  SMS.smsClist = gtk_clist_new_with_titles (4, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (SMS.smsClist), GTK_SHADOW_OUT);
  gtk_clist_set_compare_func (GTK_CLIST (SMS.smsClist), CListCompareFunc);
  gtk_clist_set_sort_column (GTK_CLIST (SMS.smsClist), 1);
  gtk_clist_set_sort_type (GTK_CLIST (SMS.smsClist), GTK_SORT_ASCENDING);
  gtk_clist_set_auto_sort (GTK_CLIST (SMS.smsClist), FALSE);
  gtk_clist_set_selection_mode (GTK_CLIST (SMS.smsClist), GTK_SELECTION_EXTENDED);

  gtk_clist_set_column_width (GTK_CLIST (SMS.smsClist), 0, 40);
  gtk_clist_set_column_width (GTK_CLIST (SMS.smsClist), 1, 155);
  gtk_clist_set_column_width (GTK_CLIST (SMS.smsClist), 2, 135);
  //gtk_clist_set_column_justification (GTK_CLIST (SMS.smsClist), 2, GTK_JUSTIFY_CENTER);

  for (i = 0; i < 4; i++)
  {
    if ((sColumn = g_malloc (sizeof (SortColumn))) == NULL)
    {
      g_print (_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
      gtk_main_quit ();
    }
    sColumn->clist = SMS.smsClist;
    sColumn->column = i;
    gtk_signal_connect (GTK_OBJECT (GTK_CLIST (SMS.smsClist)->column[i].button), "clicked",
                        GTK_SIGNAL_FUNC (SetSortColumn), (gpointer) sColumn);
  }

  gtk_signal_connect (GTK_OBJECT (SMS.smsClist), "select_row",
                      GTK_SIGNAL_FUNC (ClickEntry), NULL);

  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolledWindow, 550, 100);
  gtk_container_add (GTK_CONTAINER (scrolledWindow), SMS.smsClist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_paned_add2 (GTK_PANED (vpaned), scrolledWindow);

  gtk_widget_show (SMS.smsClist);
  gtk_widget_show (scrolledWindow);

  CreateErrorDialog (&errorDialog, GUI_SMSWindow);
  CreateInfoDialog (&infoDialog, GUI_SMSWindow);

  gtk_signal_emit_by_name(GTK_OBJECT (treeSMSItem), "expand");

  cmap = gdk_colormap_get_system();
  SMS.colour.red = 0xffff;
  SMS.colour.green = 0;
  SMS.colour.blue = 0;
  if (!gdk_color_alloc (cmap, &(SMS.colour)))
    g_error ("couldn't allocate colour");

  questMark.pixmap = gdk_pixmap_create_from_xpm_d (GUI_SMSWindow->window,
                         &questMark.mask,
                         &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL],
                         quest_xpm);
}
