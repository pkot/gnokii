/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Wed Sep 08 1999
  Modified by Jan Derfinak

*/
#include "../misc.h"  /* for _() */
#include <gtk/gtk.h>
#include "xgnokii_common.h"
#include "../pixmaps/quest.xpm"
#include "../pixmaps/stop.xpm"

inline void DeleteEvent (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (widget));
}

inline void CancelDialog (GtkWidget *widget, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (data));
}

void CreateErrorDialog (ErrorDialog *errorDialog, GtkWidget *window)
{
  GtkWidget *button, *hbox, *pixmap;
  
  errorDialog->dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (errorDialog->dialog), _("Error"));
  gtk_window_set_modal (GTK_WINDOW (errorDialog->dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (errorDialog->dialog), 5);
  gtk_signal_connect (GTK_OBJECT (errorDialog->dialog), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
    
  button = gtk_button_new_with_label (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (errorDialog->dialog)->action_area),
                      button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (CancelDialog), (gpointer) errorDialog->dialog);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (errorDialog->dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  if (window)
  {
    pixmap = NewPixmap (stop_xpm, window->window,
                        &window->style->bg[GTK_STATE_NORMAL]);
    gtk_box_pack_start (GTK_BOX(hbox), pixmap, FALSE, FALSE, 10);
    gtk_widget_show (pixmap);
  }
  
  errorDialog->text = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX(hbox), errorDialog->text, FALSE, FALSE, 10);
  gtk_widget_show (errorDialog->text);
}

void CreateYesNoDialog (YesNoDialog *yesNoDialog, GtkSignalFunc yesFunc,
                        GtkSignalFunc noFunc, GtkWidget *window)
{
  GtkWidget *button, *hbox, *pixmap;
  
  yesNoDialog->dialog = gtk_dialog_new ();
  gtk_window_set_modal (GTK_WINDOW (yesNoDialog->dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (yesNoDialog->dialog), 5);
  gtk_signal_connect (GTK_OBJECT (yesNoDialog->dialog), "delete_event",
                      GTK_SIGNAL_FUNC (DeleteEvent), NULL);
    
  
  button = gtk_button_new_with_label (_("Yes"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (yesNoDialog->dialog)->action_area),
                      button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (yesFunc), (gpointer) yesNoDialog->dialog);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);                               
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("No"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (yesNoDialog->dialog)->action_area),
                      button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (noFunc), (gpointer) yesNoDialog->dialog);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (yesNoDialog->dialog)->action_area),
                      button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (CancelDialog), (gpointer) yesNoDialog->dialog);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (yesNoDialog->dialog)->vbox), hbox);
  gtk_widget_show (hbox);
  
  if (window)
  {
    pixmap = NewPixmap (quest_xpm, window->window,
                        &window->style->bg[GTK_STATE_NORMAL]);
    gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 10);
    gtk_widget_show (pixmap);
  }
  
  yesNoDialog->text = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), yesNoDialog->text, FALSE, FALSE, 10);
  gtk_widget_show (yesNoDialog->text);
}

GtkWidget* NewPixmap (gchar **data, GdkWindow *window, GdkColor *background)
{
  GtkWidget *wpixmap;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
                              
  pixmap = gdk_pixmap_create_from_xpm_d (window, &mask, background, data);
  
  wpixmap = gtk_pixmap_new (pixmap, mask);
  
  return wpixmap;
}
