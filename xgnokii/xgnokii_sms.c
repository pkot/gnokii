/*
** Copyright (C) 1999 Andri Saar <andri@marknet.ee>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include "../misc.h"
#include "../gsm-common.h"
#include "../gsm-api.h"
#include "xgnokii_sms.h"
#include "xgnokii.h"


static GtkWidget *GUI_SMSWindow;

/* Some required declarations 
*/
void SMSWindowRefresh();


/*  SMS Window Menus
*/
static GtkItemFactoryEntry SMSWindowMenu[] = {
 {"/_Messages",			NULL,		NULL,	0,	"<Branch>" },
 {"/Messages/_New Message",	"<control>N",	NULL,	0,	NULL },
 {"/Messages/_Forward Message",	"<control>F",	NULL,	0,	NULL },
 {"/Messages/_Delete Message",	"<control>D",	NULL,	0,	NULL },
 {"/Messages/separator1",	NULL,		NULL,	0,	"<Separator>" },
 {"/Messages/_Refresh",		NULL,		SMSWindowRefresh,	0,	NULL },
 {"/Messages/separator2",	NULL,		NULL,	0,	"<Separator>" },
 {"/Messages/_Close Window",	"<control>C",	GTK_SIGNAL_FUNC(delete_event),	0,	NULL },
 {"/_Help",			NULL,		NULL,	0,	"<Branch>" },
 {"/Help/About",		NULL,		NULL,	0,	NULL }
};

/* SMS list widget
*/
GtkWidget *SMSWindowCList;

/* List of SMS Messages
*/
GSList *SMSMessages;

/* Folder ComboBox
*/
GtkWidget *SMSWindowCombo;



void SMS_FetchSMSMessages()
{
    GSM_Error SMSError;
    gint i;
    
    g_slist_free (SMSMessages);
    SMSMessages = NULL;
    
    for (i = 1; i <= 20; i++)
    {
         GSM_SMSMessage *SMSMessage;
         
         SMSMessage = (GSM_SMSMessage *)g_malloc(sizeof(GSM_SMSMessage));
         
         SMSMessage->MemoryType = GMT_SM;
         
         SMSError = GSM->GetSMSMessage (i, SMSMessage);
         
         if (SMSError != GE_NONE && SMSError != GE_EMPTYSMSLOCATION)
         {
             g_warning ("FetchSMSMessages(): Whoops, an error! I'm not going to continue!\n");
             g_free (SMSMessage);
             return;
         }
         
         if (SMSError == GE_EMPTYSMSLOCATION)
         {
             g_free (SMSMessage);
             continue;
         }
         
         SMSMessages = g_slist_append (SMSMessages, SMSMessage);
    }
    
    return;
}

void SMSWindowInsertSMS(gpointer GSMSMessage, gpointer SMSType)
{
    GSM_SMSMessage *SMSMessage;
    
    SMSMessage = (GSM_SMSMessage *)GSMSMessage;
    
    if (SMSMessage->Type != GPOINTER_TO_INT(SMSType))
       return;
    
    switch (GPOINTER_TO_INT(SMSType))
    {
        case GST_MO:
        {
            /* Outbox */
            gchar *buffer[4];
            
            buffer[0] = NULL;
            buffer[1] = NULL;
            if (SMSMessage->Status)
                buffer[2] = g_strdup ("Sent");
            else
                buffer[2] = g_strdup ("Not sent");
            buffer[3] = g_strdup (SMSMessage->MessageText);
           
            gtk_clist_append (GTK_CLIST(SMSWindowCList), buffer);
           
            break;
       }
       
       default:
       {
            /* All else to inbox */
            gchar *buffer[4];
           
            buffer[0] = g_strdup (SMSMessage->Sender);
            buffer[1] = g_strdup_printf ("%d/%d/%d %d:%02d:%02d",
            			SMSMessage->Time.Day, SMSMessage->Time.Month, SMSMessage->Time.Year,
            			SMSMessage->Time.Hour, SMSMessage->Time.Minute, SMSMessage->Time.Second);
            			
            if (SMSMessage->Status)
                buffer[2] = g_strdup ("Read");
            else
                buffer[2] = g_strdup ("Not read");
           
            buffer[3] = g_strdup (SMSMessage->MessageText);
           
            gtk_clist_append (GTK_CLIST(SMSWindowCList), buffer);
           
            break;
        }
    }
}

void SMSWindowRefresh()
{
    gchar *SMSTypeString;
    gint SMSType;
    
    /* First, update SMS list. */
    SMS_FetchSMSMessages();
    
    /* Then go and update CList. */
    
    SMSTypeString = gtk_entry_get_text (GTK_ENTRY(GTK_COMBO(SMSWindowCombo)->entry));
    
    if (!strcmp (SMSTypeString, "Outbox"))
        SMSType = GST_MO;
    else
        SMSType = GST_MT;
    
    gtk_clist_freeze(GTK_CLIST(SMSWindowCList));
    gtk_clist_clear(GTK_CLIST(SMSWindowCList));
    g_slist_foreach (SMSMessages, (GFunc)SMSWindowInsertSMS, GINT_TO_POINTER(SMSType));
    gtk_clist_thaw (GTK_CLIST(SMSWindowCList));
}

GtkWidget *SMSWindow_CreateMenus(GtkWidget *SMSWindow)
{
    GtkItemFactory *SMSWindowMenuBar;  
    GtkAccelGroup *SMSWindowAccel;
    gint SMSWindowMenuNo;

    SMSWindowAccel = gtk_accel_group_new();
    SMSWindowMenuBar = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
  					     SMSWindowAccel);
    SMSWindowMenuNo = sizeof (SMSWindowMenu) / sizeof (SMSWindowMenu[0]);
    gtk_item_factory_create_items(SMSWindowMenuBar, SMSWindowMenuNo, SMSWindowMenu, NULL);
    gtk_accel_group_attach (SMSWindowAccel, GTK_OBJECT (SMSWindow));
    
    return gtk_item_factory_get_widget (SMSWindowMenuBar, "<main>");
}

GtkWidget *SMSWindow_CreateToolbar(GtkWidget *SMSWindow)
{
    GtkWidget *SMSWindowToolbar;
    

    GList *SMSWindowComboItems = NULL;
    
    GtkWidget *SMSWindowNew;
    GtkWidget *SMSWindowForward;
    GtkWidget *SMSWindowDelete;
    GtkWidget *SMSWindowRefresh;
    GtkWidget *SMSWindowClose;
    
    SMSWindowToolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
    			GTK_TOOLBAR_TEXT);
    
    SMSWindowComboItems = g_list_append (SMSWindowComboItems, "Inbox");
    SMSWindowComboItems = g_list_append (SMSWindowComboItems, "Outbox");
    SMSWindowCombo = gtk_combo_new();
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(SMSWindowCombo)->entry), 
    			"Inbox");
    gtk_combo_set_popdown_strings(GTK_COMBO(SMSWindowCombo),
    			SMSWindowComboItems);
    gtk_combo_set_use_arrows(GTK_COMBO(SMSWindowCombo), TRUE);
    gtk_toolbar_append_element (GTK_TOOLBAR(SMSWindowToolbar),
    			GTK_TOOLBAR_CHILD_WIDGET,
    			SMSWindowCombo,
    			"Choose a folder",
    			"Choose a folder",
    			"SMSWindowToolbarCombo",
    			NULL,
    			NULL,
    			NULL);
     
    gtk_widget_show (SMSWindowCombo);

    gtk_toolbar_append_space (GTK_TOOLBAR(SMSWindowToolbar));
    
    SMSWindowNew = gtk_toolbar_append_item (GTK_TOOLBAR(SMSWindowToolbar),
    			"N",
    			"New Message",
    			"Private",
    			NULL,
    			NULL,
    			NULL);
    
    SMSWindowForward = gtk_toolbar_append_item (GTK_TOOLBAR(SMSWindowToolbar),
    			"F",
    			"Forward Message",
    			"Private",
    			NULL,
    			NULL,
    			NULL);
    
    SMSWindowDelete = gtk_toolbar_append_item (GTK_TOOLBAR(SMSWindowToolbar),
    			"D",
    			"Delete Message",
    			"Private",
    			NULL,
    			NULL,
    			NULL);
    
    gtk_toolbar_append_space (GTK_TOOLBAR(SMSWindowToolbar));
    
    SMSWindowRefresh = gtk_toolbar_append_item (GTK_TOOLBAR(SMSWindowToolbar),
    			"R",
    			"Refresh Folder",
    			"Private",
    			NULL,
    			NULL,
    			NULL);
    			
    gtk_toolbar_append_space (GTK_TOOLBAR(SMSWindowToolbar));
    
    SMSWindowClose = gtk_toolbar_append_item (GTK_TOOLBAR(SMSWindowToolbar),
    			"C",
    			"Close Window",
    			"Private",
    			NULL,
    			GTK_SIGNAL_FUNC(delete_event),
    			NULL);
    
    
    return SMSWindowToolbar;
}

inline void GUI_ShowSMS()
{
  gtk_widget_show(GUI_SMSWindow);
}

void GUI_CreateSMSWindow()
{
 GtkWidget *vbox;
 GtkWidget *SMSWindowMenuBar;
 GtkWidget *SMSWindowToolbar;
 gchar *SMSWindowListTitles[4] = { "Sender", "Time", "Status", "Message" };
 
 
 /* Create window and vertical pack box
 */
 GUI_SMSWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
 gtk_window_set_title (GTK_WINDOW (GUI_SMSWindow), "SMS");
 gtk_signal_connect (GTK_OBJECT(GUI_SMSWindow), "delete_event",
                     GTK_SIGNAL_FUNC(delete_event), NULL);
 vbox = gtk_vbox_new (FALSE, 1);
 gtk_container_add (GTK_CONTAINER (GUI_SMSWindow), vbox);
 gtk_widget_show (vbox);
 
 /* Create && attach main menu.
 */
 SMSWindowMenuBar = SMSWindow_CreateMenus (GUI_SMSWindow);
 gtk_box_pack_start (GTK_BOX (vbox),SMSWindowMenuBar, FALSE, TRUE, 0);
 gtk_widget_show (SMSWindowMenuBar);

 /* Create && attach toolbar.
 */ 
 SMSWindowToolbar = SMSWindow_CreateToolbar (GUI_SMSWindow);
 gtk_box_pack_start (GTK_BOX (vbox),SMSWindowToolbar, FALSE, TRUE, 0);
 gtk_widget_show (SMSWindowToolbar);
 
 /* Create && add SMS list.
 */
 SMSWindowCList = gtk_clist_new_with_titles (4, SMSWindowListTitles);
 gtk_clist_set_selection_mode (GTK_CLIST(SMSWindowCList), GTK_SELECTION_SINGLE);
 gtk_clist_column_titles_passive (GTK_CLIST(SMSWindowCList));
 gtk_box_pack_start (GTK_BOX (vbox), SMSWindowCList, FALSE, TRUE, 0);
 gtk_widget_show (SMSWindowCList);
}
