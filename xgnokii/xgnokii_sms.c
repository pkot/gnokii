/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Nov 28 1999
  Modified by Jan Derfinak

*/

#include <gtk/gtk.h>
#include "../misc.h"
#include "../gsm-api.h"
#include "xgnokii_common.h"
#include "xgnokii.h"
#include "xgnokii_contacts.h"
#include "xgnokii_sms.h"
#include "../pixmaps/Edit.xpm"
#include "../pixmaps/Delete.xpm"

static GtkWidget *GUI_SMSWindow;
static GtkWidget *smsClist;
static GtkWidget *smsText;
static GSList* messages = NULL;
static ErrorDialog errorDialog = {NULL, NULL};
static gint row_i;
static gint currentBox;
static GdkColormap *cmap;
static GdkColor colour;

static inline void Help1 (GtkWidget *w, gpointer data)
{
  Help (w, _("/help/sms.html"));
}

static inline void CloseSMS (GtkWidget *w, gpointer data)
{
  gtk_widget_hide (GUI_SMSWindow);
}

static inline void FreeElement (gpointer data, gpointer userData)
{
  g_free ((GSM_SMSMessage *) data);
}

static inline void FreeArray (GSList *array)
{
  if (array)
  {
    g_slist_foreach (array, FreeElement, NULL);
    g_slist_free (array);
  }
}

static void InsertInboxElement (gpointer d, gpointer userData)
{
  GSM_SMSMessage *data = (GSM_SMSMessage *) d;
  
  if (data->Type == GST_MT || data->Type == GST_DR)
  {
    gchar *row[4];

    if (data->Type == GST_DR)
      row[0] = g_strdup (_("report"));
    else if (data->Status)
      row[0] = g_strdup (_("read"));
    else
      row[0] = g_strdup (_("unread"));

    if (data->Time.Timezone)
      row[1] = g_strdup_printf (_("%02d/%02d/%02d %02d:%02d:%02d GMT%+dh"),
                                data->Time.Day, data->Time.Month, data->Time.Year,
                                data->Time.Hour, data->Time.Minute, data->Time.Second,
                                data->Time.Timezone);
    else
      row[1] = g_strdup_printf (_("%02d/%02d/%02d %02d:%02d:%02d GMT"),
                                data->Time.Day, data->Time.Month, data->Time.Year,
                                data->Time.Hour, data->Time.Minute, data->Time.Second);
      
    row[2] = GUI_GetName(data->Sender);
    if (row[2] == NULL)
      row[2] = data->Sender;
    row[3] = data->MessageText;
      
    gtk_clist_append( GTK_CLIST (smsClist), row);
    gtk_clist_set_row_data (GTK_CLIST (smsClist), row_i++,
                            GINT_TO_POINTER (data->MessageNumber));
    g_free (row[0]);
    g_free (row[1]);
  }
}
                              
static inline void RefreshInbox ()
{
  gtk_clist_freeze (GTK_CLIST (smsClist));
  gtk_clist_clear (GTK_CLIST (smsClist));
  
  row_i = 0;
  g_slist_foreach (messages, InsertInboxElement, (gpointer) NULL);
  
  gtk_clist_sort (GTK_CLIST (smsClist));
  gtk_clist_thaw (GTK_CLIST (smsClist));
}

static void InsertOutboxElement (gpointer d, gpointer userData)
{
  GSM_SMSMessage *data = (GSM_SMSMessage *) d;
  
  if (data->Type == GST_MO)
  {
    gchar *row[4];

    if (data->Status)
      row[0] = g_strdup (_("sent"));
    else
      row[0] = g_strdup (_("unsent"));

    
    row[1] = row[2] = g_strdup ("");
    row[3] = data->MessageText;
      
    gtk_clist_append( GTK_CLIST (smsClist), row);
    gtk_clist_set_row_data (GTK_CLIST (smsClist), row_i++,
                            GINT_TO_POINTER (data->MessageNumber));
    g_free (row[0]);
    g_free (row[1]);
  }
}
                              
static inline void RefreshOutbox ()
{
  gtk_clist_freeze (GTK_CLIST (smsClist));
  gtk_clist_clear (GTK_CLIST (smsClist));
  
  row_i = 0;
  g_slist_foreach (messages, InsertOutboxElement, (gpointer) NULL);
  
  gtk_clist_sort (GTK_CLIST (smsClist));
  gtk_clist_thaw (GTK_CLIST (smsClist));
}

inline void GUI_RefreshMessageWindow ()
{
  if (!GTK_WIDGET_VISIBLE (GUI_SMSWindow))
    return;
    
  if (currentBox)
    RefreshOutbox ();
  else
    RefreshInbox ();
}

static void SelectTreeItem (GtkWidget *item, gpointer data)
{
  currentBox = GPOINTER_TO_INT (data);
  GUI_RefreshMessageWindow ();
}

void GUI_RefreshSMS ()
{
  GSM_SMSMessage *msg;
  register gint i;
  
  FreeArray (messages);
  
  for (i = 1; i <= 10; i++)
  {
    msg = g_malloc (sizeof (GSM_SMSMessage));
    msg->MemoryType = GMT_SM;
    msg->Location = i;
    
    if (GSM->GetSMSMessage(msg) == GE_NONE)
      messages = g_slist_append (messages, msg);
  }
}

static void ClickEntry (GtkWidget      *clist,
                        gint            row,
                        gint            column,
                        GdkEventButton *event,
                        gpointer        data )
{
  if(event)
  {
    gchar *buf;
    
    gtk_text_freeze (GTK_TEXT (smsText));
    
    gtk_text_set_point (GTK_TEXT (smsText), 0);
    gtk_text_forward_delete (GTK_TEXT (smsText), gtk_text_get_length (GTK_TEXT (smsText)));
    
    gtk_text_insert (GTK_TEXT (smsText), NULL, &colour, NULL,
                     _("From: "), -1);
    gtk_clist_get_text (GTK_CLIST (clist), row, 2, &buf);
    gtk_text_insert (GTK_TEXT (smsText), NULL, &smsText->style->black, NULL,
                     buf, -1);
    gtk_text_insert (GTK_TEXT (smsText), NULL, &smsText->style->black, NULL,
                     "\n", -1);
    
    gtk_text_insert (GTK_TEXT (smsText), NULL, &colour, NULL,
                     _("Date: "), -1);
    gtk_clist_get_text (GTK_CLIST (clist), row, 1, &buf);
    gtk_text_insert (GTK_TEXT (smsText), NULL, &smsText->style->black, NULL,
                     buf, -1);
    gtk_text_insert (GTK_TEXT (smsText), NULL, &smsText->style->black, NULL,
                     "\n\n", -1);                 
    
    gtk_clist_get_text (GTK_CLIST (clist), row, 3, &buf);
    gtk_text_insert (GTK_TEXT (smsText), NULL, &smsText->style->black, NULL,
                     buf, -1);
    
    gtk_text_thaw (GTK_TEXT (smsText));
  }
}

inline void GUI_ShowSMS ()
{
  gtk_widget_show (GUI_SMSWindow);
  GUI_RefreshMessageWindow ();
}

static GtkItemFactoryEntry menu_items[] = {
  {"/_File",		NULL,		NULL, 0, "<Branch>"},
  {"/File/_Save",	"<control>S",	NULL, 0, NULL},
  {"/File/sep1",	NULL,		NULL, 0, "<Separator>"},
  {"/File/_Close",	"<control>W",	CloseSMS, 0, NULL},
  {"/_Messages",		NULL,		NULL, 0, "<Branch>"},
  {"/Messages/_New",	NULL,		NULL, 0, NULL},
  {"/Messages/_Forward",NULL,		NULL, 0, NULL},
  {"/Messages/_Reply",	NULL,		NULL, 0, NULL},
  {"/Messages/_Delete",	NULL,		NULL, 0, NULL},
  {"/_Help",		NULL,		NULL, 0, "<LastBranch>"},
  {"/Help/_Help",	NULL,		Help1, 0, NULL},
  {"/Help/_About",	NULL,		GUI_ShowAbout, 0, NULL},
};

void GUI_CreateSMSWindow ()
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
  gchar *titles[4] = { _("Status"), _("Date"), _("Sender"), _("Message")};
  
  
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
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Forward Message"), NULL,
                           NewPixmap(Delete_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Reply message"), NULL,
                           NewPixmap(Delete_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL, _("Delete message"), NULL,
                           NewPixmap(Delete_xpm, GUI_SMSWindow->window,
                           &GUI_SMSWindow->style->bg[GTK_STATE_NORMAL]),
                           (GtkSignalFunc) NULL, NULL);
  
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
  smsText = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (smsText), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (smsText), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (smsText), TRUE);
 
  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  
  gtk_paned_add2 (GTK_PANED (hpaned), scrolledWindow);
  
  gtk_container_add (GTK_CONTAINER (scrolledWindow), smsText);
  gtk_widget_show_all (scrolledWindow);
  
  /* Messages list */
  smsClist = gtk_clist_new_with_titles (4, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (smsClist), GTK_SHADOW_OUT);
  //gtk_clist_set_compare_func (GTK_CLIST (smsClist), CListCompareFunc);
  gtk_clist_set_sort_column (GTK_CLIST (smsClist), 1);
  gtk_clist_set_sort_type (GTK_CLIST (smsClist), GTK_SORT_ASCENDING);
  gtk_clist_set_auto_sort (GTK_CLIST (smsClist), FALSE);
  //gtk_clist_set_selection_mode (GTK_CLIST (smsClist), GTK_SELECTION_EXTENDED);
  
  //gtk_clist_set_column_width (GTK_CLIST (smsClist), 0, 150);
  gtk_clist_set_column_width (GTK_CLIST (smsClist), 1, 155);
  gtk_clist_set_column_width (GTK_CLIST (smsClist), 2, 135);
  //gtk_clist_set_column_justification (GTK_CLIST (smsClist), 2, GTK_JUSTIFY_CENTER);
  
/*  for (i = 0; i < 4; i++)
  {
    if ((sColumn = g_malloc (sizeof (SortColumn))) == NULL)
    {
      g_print (_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
      gtk_main_quit ();
    }
    sColumn->clist = smsClist;
    sColumn->column = i;
    gtk_signal_connect (GTK_OBJECT (GTK_CLIST (smsClist)->column[i].button), "clicked",
                        GTK_SIGNAL_FUNC (SetSortColumn), (gpointer) sColumn);
  }
  */                      
  gtk_signal_connect (GTK_OBJECT (smsClist), "select_row",
                      GTK_SIGNAL_FUNC (ClickEntry), NULL);
  
  scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolledWindow, 550, 100);
  gtk_container_add (GTK_CONTAINER (scrolledWindow), smsClist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledWindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_paned_add2 (GTK_PANED (vpaned), scrolledWindow);
  
  gtk_widget_show (smsClist);
  gtk_widget_show (scrolledWindow);
  
  CreateErrorDialog (&errorDialog, GUI_SMSWindow);
  
  gtk_signal_emit_by_name(GTK_OBJECT (treeSMSItem), "expand");
  
  cmap = gdk_colormap_get_system();
  colour.red = 0xffff;
  colour.green = 0;
  colour.blue = 0;
  if (!gdk_color_alloc (cmap, &colour)) {
    g_error ("couldn't allocate colour");
  }
}
