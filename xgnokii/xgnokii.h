#ifndef XGNOKII_H
#define XGNOKII_H

#include <gtk/gtk.h>

#define MAX_CALLER_GROUP_LENGTH	10

typedef struct {
  gchar *initlength; /* Init length from .gnokiirc file */
  gchar *model;      /* Model from .gnokiirc file. */
  gchar *port;       /* Serial port from .gnokiirc file */
  gchar *connection; /* Connection type from .gnokiirc file */
  gchar *bindir;
  bool   callerGroupsSupported;
  gchar *callerGroups[6];
} XgnokiiConfig;
  
/* Hold main configuration data for xgnokii */
extern XgnokiiConfig xgnokiiConfig;

extern gint max_phonebook_name_length;
extern gint max_phonebook_number_length;
extern gint max_phonebook_sim_name_length;
extern gint max_phonebook_sim_number_length;

void GUI_ShowAbout();

void delete_event( GtkWidget *, GdkEvent *, gpointer );

#endif XGNOKII_H
