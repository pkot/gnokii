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

  Copyright (C) 1999      Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak
  Copyright (C) 2002      Markus Plail, Pavel Machek
  Copyright (C) 2002-2004 Pawel Kot, BORBELY Zoltan
  Copyright (C) 2002-2003 Uli Hopp
  Copyright (C) 2003      Ladis Michl

*/


#include <gtk/gtk.h>
#include "misc.h"

#include <stdio.h>
#include <pthread.h>

#ifndef WIN32
#  include <unistd.h>
#else
#  include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "gnokii.h"
#include "xgnokii_contacts.h"
#include "xgnokii.h"
#include "xgnokii_common.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii_sms.h"
#include "xpm/Read.xpm"
#include "xpm/Send.xpm"
#include "xpm/Open.xpm"
#include "xpm/Save.xpm"
#include "xpm/New.xpm"
#include "xpm/Duplicate.xpm"
#include "xpm/Edit.xpm"
#include "xpm/Delete.xpm"
#include "xpm/Dial.xpm"
#include "xpm/sim.xpm"
#include "xpm/phone.xpm"
#include "xpm/quest.xpm"

#define strip_white(x) g_strjoinv("", g_strsplit(x, " ", 0))

typedef struct {
	GtkWidget *dialog;
	GtkWidget *entry;
} DialVoiceDialog;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *entry;
	gint row;
	GtkWidget *gbutton;
	GtkWidget *mbutton;
	GtkWidget *wbutton;
	GtkWidget *fbutton;
	GtkWidget *hbutton;
	PhonebookEntry *pbEntry;
} ExtPbkDialog;

static GtkWidget *GUI_ContactsWindow;
static bool contactsMemoryInitialized;
static MemoryStatus memoryStatus;
static ContactsMemory contactsMemory;	/* Hold contacts. */
static GtkWidget *clist;
static StatusInfo statusInfo;
static ProgressDialog progressDialog = { NULL, NULL, NULL };
static ErrorDialog errorDialog = { NULL, NULL };
static FindEntryStruct findEntryStruct = { "", 0 };
static ExportDialogData exportDialogData = { NULL };
static MemoryPixmaps memoryPixmaps;
static QuestMark questMark;
static EditEntryData newEditEntryData =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static EditEntryData editEditEntryData =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static EditEntryData duplicateEditEntryData =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static EditNumbersData editNumbersData = { NULL, NULL };

static EditEntryData editSubEntriesData =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static void EditPbEntry(PhonebookEntry * pbEntry, gint row);
static void NewPbEntry(PhonebookEntry * pbEntry);
static void DuplicatePbEntry(PhonebookEntry * pbEntry);

/* return != 0 if user has unsaved changes in contacts memory */
inline gint GUI_ContactsIsChanged(void)
{
	return statusInfo.ch_ME | statusInfo.ch_SM;
}


/* return TRUE if Contacts memory was read from phone or from file */
inline bool GUI_ContactsIsIntialized(void)
{
	return contactsMemoryInitialized;
}


static inline void Help1(GtkWidget * w, gpointer data)
{
	gchar *indx = g_strdup_printf("/help/%s/windows/contacts/index.html", xgnokiiConfig.help_locale);
	Help(w, indx);
	g_free(indx);
}


void RefreshStatusInfo(void)
{
	char p, s;

	if (statusInfo.ch_ME)
		p = '*';
	else
		p = ' ';

	if (statusInfo.ch_SM)
		s = '*';
	else
		s = ' ';
	g_snprintf(statusInfo.text, STATUS_INFO_LENGTH, _("SIM: %d/%d%c  Phone: %d/%d%c"),
		   memoryStatus.UsedSM, memoryStatus.MaxSM, s,
		   memoryStatus.UsedME, memoryStatus.MaxME, p);
	gtk_label_set_text(GTK_LABEL(statusInfo.label), statusInfo.text);
}


static inline void SetGroup0(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newGroup = 0;
}


static inline void SetGroup1(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newGroup = 1;
}


static inline void SetGroup2(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newGroup = 2;
}


static inline void SetGroup3(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newGroup = 3;
}


static inline void SetGroup4(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newGroup = 4;
}


static inline void SetGroup5(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newGroup = 5;
}


static inline void SetType0(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 0;
}
static inline void SetType1(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 1;
}
static inline void SetType2(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 2;
}
static inline void SetType3(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 3;
}
static inline void SetType4(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 4;
}
static inline void SetType5(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 5;
}
static inline void SetType6(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 6;
}
static inline void SetType7(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 7;
}
static inline void SetType8(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 8;
}
static inline void SetType9(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 9;
}
static inline void SetType10(GtkWidget * item, gpointer data)
{
	((EditEntryData *) data)->newType = 10;
}

PhonebookEntry *FindFreeEntry(gn_memory_type type)
{
	PhonebookEntry *entry;
	gint start, end;
	register gint i;

	if (type == GN_MT_ME) {
		if (memoryStatus.FreeME == 0)
			return NULL;
		start = 0;
		end = memoryStatus.MaxME;
	} else {
		if (memoryStatus.FreeSM == 0)
			return NULL;
		start = memoryStatus.MaxME;
		end = memoryStatus.MaxME + memoryStatus.MaxSM;
	}

	for (i = start; i < end; i++) {
		entry = g_ptr_array_index(contactsMemory, i);
		if (entry->status == E_Empty || entry->status == E_Deleted)
			return entry;
	}

	return NULL;
}


inline PhonebookEntry *GUI_GetEntry(gn_memory_type type, gint nr)
{
	if ((type == GN_MT_ME && (nr < 1 || nr >= memoryStatus.MaxME)) ||
	    (type == GN_MT_SM && (nr < 1 || nr >= memoryStatus.MaxSM)))
		return NULL;

	if (type == GN_MT_ME)
		return g_ptr_array_index(contactsMemory, nr - 1);
	else
		return g_ptr_array_index(contactsMemory, nr + memoryStatus.MaxME - 1);
}


static void CloseContacts(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(GUI_ContactsWindow);
}


/* I don't want to allow window to close */
static void ProgressDialogDeleteEvent(GtkWidget * w, gpointer data)
{
	return;
}


static void CancelEditDialog(GtkWidget * widget, gpointer data)
{
	((EditEntryData *) data)->pbEntry->status = E_Empty;
	gtk_widget_hide(GTK_WIDGET(((EditEntryData *) data)->dialog));
}



static void OkEditEntryDialog(GtkWidget * widget, gpointer data)
{
	gchar *clist_row[4];
	gchar **number;
	PhonebookEntry *new_entry, *current_entry = ((EditEntryData *) data)->pbEntry;

	/* Memory Type changed SM -> ME */
	if (GTK_TOGGLE_BUTTON(((EditEntryData *) data)->memoryTypePhone)->active &&
	    current_entry->entry.memory_type == GN_MT_SM) {
		if ((new_entry = FindFreeEntry(GN_MT_ME)) == NULL) {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Can't change memory type!"));
			gtk_widget_show(errorDialog.dialog);
			return;
		}
		snprintf(new_entry->entry.name, max_phonebook_name_length, "%s",
			 gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)));

		if (phoneMonitor.supported & PM_EXTPBK) {
			number = g_malloc(sizeof(char) * max_phonebook_number_length);
			gtk_label_get(GTK_LABEL(((EditEntryData *) data)->number), number);
			snprintf(new_entry->entry.number, max_phonebook_number_length, "%s", number[0]);
			snprintf(new_entry->entry.subentries[0].data.number, max_phonebook_number_length, "%s", number[0]);
			new_entry->entry.subentries[0].entry_type = GN_PHONEBOOK_ENTRY_Number;
			new_entry->entry.subentries[0].number_type = GN_PHONEBOOK_NUMBER_General;
			new_entry->entry.subentries_count = 1;
			g_free(number);
		} else {
			snprintf(new_entry->entry.number, max_phonebook_number_length, "%s",
				 gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->number)));
		}

		new_entry->entry.caller_group = ((EditEntryData *) data)->newGroup;

		new_entry->status = E_Changed;

		current_entry->status = E_Deleted;

		memoryStatus.UsedME++;
		memoryStatus.FreeME--;
		memoryStatus.UsedSM--;
		memoryStatus.FreeSM++;
		statusInfo.ch_ME = statusInfo.ch_SM = 1;

		current_entry = new_entry;

		/* Memory Type changed ME -> SM */
	} else if (GTK_TOGGLE_BUTTON(((EditEntryData *) data)->memoryTypeSIM)->active &&
		   current_entry->entry.memory_type == GN_MT_ME) {
		if ((new_entry = FindFreeEntry(GN_MT_SM)) == NULL) {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Can't change memory type!"));
			gtk_widget_show(errorDialog.dialog);
			return;
		}
		strncpy(new_entry->entry.name,
			gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
			max_phonebook_sim_name_length);
		new_entry->entry.name[max_phonebook_sim_name_length] = '\0';

		if (strlen(gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name))) >
		    max_phonebook_sim_name_length) {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Sorry, phonebook name will be truncated,\n\
because you save it into SIM memory!"));
			gtk_widget_show(errorDialog.dialog);
		}

		if (phoneMonitor.supported & PM_EXTPBK) {
			number = g_malloc(sizeof(char) * max_phonebook_number_length);
			gtk_label_get(GTK_LABEL(((EditEntryData *) data)->number), number);
			snprintf(new_entry->entry.number, max_phonebook_number_length, "%s", number[0]);
			snprintf(new_entry->entry.subentries[0].data.number, max_phonebook_number_length, "%s", number[0]);
			new_entry->entry.subentries[0].entry_type = GN_PHONEBOOK_ENTRY_Number;
			new_entry->entry.subentries[0].number_type = GN_PHONEBOOK_NUMBER_General;
			new_entry->entry.subentries_count = 1;
			g_free(number);
		} else {
			strncpy(new_entry->entry.number,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->number)),
				max_phonebook_sim_number_length);
			new_entry->entry.number[max_phonebook_sim_number_length] = '\0';
		}

		new_entry->entry.caller_group = ((EditEntryData *) data)->newGroup;

		new_entry->status = E_Changed;

		current_entry->status = E_Deleted;

		memoryStatus.UsedME--;
		memoryStatus.FreeME++;
		memoryStatus.UsedSM++;
		memoryStatus.FreeSM--;
		statusInfo.ch_ME = statusInfo.ch_SM = 1;

		current_entry = new_entry;
	} else {
		/* Memory type noct changes -> current_entry will be "edited" directly */
		/* Memory type not changed, phone memory */
		if (GTK_TOGGLE_BUTTON(((EditEntryData *) data)->memoryTypePhone)->active) {
			strncpy(current_entry->entry.name,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
				max_phonebook_name_length);
			current_entry->entry.name[max_phonebook_name_length] = '\0';

			if (phoneMonitor.supported & PM_EXTPBK) {

			} else {
				strncpy(current_entry->entry.number,
					gtk_entry_get_text(GTK_ENTRY
							   (((EditEntryData *) data)->number)),
					max_phonebook_number_length);
				current_entry->entry.number[max_phonebook_number_length] = '\0';
			}
		/* Memory type not changed, SIM memory */
		} else {
			strncpy(current_entry->entry.name,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
				max_phonebook_sim_name_length);
			current_entry->entry.name[max_phonebook_sim_name_length] = '\0';

			if (strlen(gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name))) >
			    max_phonebook_sim_name_length) {
				gtk_label_set_text(GTK_LABEL(errorDialog.text),
						   _("Sorry, phonebook name will be truncated,\n\
because you save it into SIM memory!"));
				gtk_widget_show(errorDialog.dialog);
			}

			if (phoneMonitor.supported & PM_EXTPBK) {
				number = g_malloc(sizeof(char) * max_phonebook_number_length);
				gtk_label_get(GTK_LABEL(((EditEntryData *) data)->number), number);
				snprintf(current_entry->entry.number, max_phonebook_number_length, "%s", number[0]);
				snprintf(current_entry->entry.subentries[0].data.number, max_phonebook_number_length, "%s", number[0]);
				current_entry->entry.subentries[0].entry_type = GN_PHONEBOOK_ENTRY_Number;
				current_entry->entry.subentries[0].number_type = GN_PHONEBOOK_NUMBER_General;
				current_entry->entry.subentries_count = 1;
				g_free(number);
			} else {
				strncpy(current_entry->entry.number,
					gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->number)), 
					max_phonebook_sim_number_length);
				current_entry->entry.number[max_phonebook_sim_number_length] = '\0';
			}
		}

		current_entry->entry.caller_group = ((EditEntryData *) data)->newGroup;

		current_entry->status = E_Changed;

		if (current_entry->entry.memory_type == GN_MT_ME) {
			statusInfo.ch_SM = 0;
			statusInfo.ch_ME = 1;
		} else {
			statusInfo.ch_SM = 1;
			statusInfo.ch_ME = 0;
		}
	}

	gtk_widget_hide(GTK_WIDGET(((EditEntryData *) data)->dialog));
	((EditEntryData *) data)->dialog = NULL;

	RefreshStatusInfo();

	gtk_clist_freeze(GTK_CLIST(clist));
	gtk_clist_remove(GTK_CLIST(clist), ((EditEntryData *) data)->row);

	clist_row[0] = current_entry->entry.name;

	clist_row[1] = current_entry->entry.number;

	if (current_entry->entry.memory_type == GN_MT_ME)
		clist_row[2] = "P";
	else
		clist_row[2] = "S";
	if (phoneMonitor.supported & PM_CALLERGROUP)
		clist_row[3] =
		    xgnokiiConfig.callerGroups[current_entry->entry.caller_group];
	else
		clist_row[3] = "";
	gtk_clist_insert(GTK_CLIST(clist), ((EditEntryData *) data)->row, clist_row);
	if (current_entry->entry.memory_type == GN_MT_ME)
		gtk_clist_set_pixmap(GTK_CLIST(clist), ((EditEntryData *) data)->row, 2,
				     memoryPixmaps.phoneMemPix, memoryPixmaps.mask);
	else
		gtk_clist_set_pixmap(GTK_CLIST(clist), ((EditEntryData *) data)->row, 2,
				     memoryPixmaps.simMemPix, memoryPixmaps.mask);

	gtk_clist_set_row_data(GTK_CLIST(clist), ((EditEntryData *) data)->row,
			       (gpointer) current_entry);

	gtk_clist_sort(GTK_CLIST(clist));
	gtk_clist_thaw(GTK_CLIST(clist));
}


static void OkDeleteEntryDialog(GtkWidget * widget, gpointer data)
{
	PhonebookEntry *pbEntry;
	GList *sel;
	gint row;

	sel = GTK_CLIST(clist)->selection;

	gtk_clist_freeze(GTK_CLIST(clist));

	while (sel != NULL) {
		row = GPOINTER_TO_INT(sel->data);
		pbEntry = (PhonebookEntry *) gtk_clist_get_row_data(GTK_CLIST(clist), row);
		sel = sel->next;

		pbEntry->status = E_Deleted;

		if (pbEntry->entry.memory_type == GN_MT_ME) {
			memoryStatus.UsedME--;
			memoryStatus.FreeME++;
			statusInfo.ch_ME = 1;
		} else {
			memoryStatus.UsedSM--;
			memoryStatus.FreeSM++;
			statusInfo.ch_SM = 1;
		}

		gtk_clist_remove(GTK_CLIST(clist), row);
	}

	RefreshStatusInfo();

	gtk_widget_hide(GTK_WIDGET(data));

	gtk_clist_thaw(GTK_CLIST(clist));
}


static void OkNewEntryDialog(GtkWidget * widget, gpointer data)
{
	gchar *clist_row[4];
	PhonebookEntry *entry;

	if (GTK_TOGGLE_BUTTON(((EditEntryData *) data)->memoryTypePhone)->active &&
	    ((EditEntryData *) data)->pbEntry->entry.memory_type == GN_MT_SM) {
		if ((entry = FindFreeEntry(GN_MT_ME)) == NULL) {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Can't change memory type!"));
			gtk_widget_show(errorDialog.dialog);
			return;
		}
		strncpy(entry->entry.name,
			gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
			max_phonebook_name_length);
		entry->entry.name[max_phonebook_name_length] = '\0';

		if (phoneMonitor.supported & PM_EXTPBK) {
		} else {
			strncpy(entry->entry.number,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->number)),
				max_phonebook_number_length);
			entry->entry.number[max_phonebook_number_length] = '\0';
		}

		entry->entry.caller_group = ((EditEntryData *) data)->newGroup;

		entry->status = E_Changed;

		((EditEntryData *) data)->pbEntry = entry;
	} else if (GTK_TOGGLE_BUTTON(((EditEntryData *) data)->memoryTypeSIM)->active &&
		   ((EditEntryData *) data)->pbEntry->entry.memory_type == GN_MT_ME) {
		if ((entry = FindFreeEntry(GN_MT_SM)) == NULL) {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Can't change memory type!"));
			gtk_widget_show(errorDialog.dialog);
			return;
		}
		strncpy(entry->entry.name,
			gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
			max_phonebook_sim_name_length);
		entry->entry.name[max_phonebook_sim_name_length] = '\0';

		if (strlen(gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name))) >
		    max_phonebook_sim_name_length) {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Sorry, phonebook name will be truncated,\n\
because you save it into SIM memory!"));
			gtk_widget_show(errorDialog.dialog);
		}

		if (phoneMonitor.supported & PM_EXTPBK) {
		} else {
			strncpy(entry->entry.number,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->number)),
				max_phonebook_sim_number_length);
			entry->entry.number[max_phonebook_sim_number_length] = '\0';
		}

		entry->entry.caller_group = ((EditEntryData *) data)->newGroup;

		entry->status = E_Changed;

		((EditEntryData *) data)->pbEntry = entry;
	} else {
		if (GTK_TOGGLE_BUTTON(((EditEntryData *) data)->memoryTypePhone)->active) {
			strncpy(((EditEntryData *) data)->pbEntry->entry.name,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
				max_phonebook_name_length);
			((EditEntryData *) data)->pbEntry->entry.name[max_phonebook_name_length] = '\0';

			if (phoneMonitor.supported & PM_EXTPBK) {
			} else {
				strncpy(((EditEntryData *) data)->pbEntry->entry.number,
					gtk_entry_get_text(GTK_ENTRY
							   (((EditEntryData *) data)->number)),
					max_phonebook_number_length);
				((EditEntryData *) data)->pbEntry->entry.
				    name[max_phonebook_number_length] = '\0';
			}
		} else {
			strncpy(((EditEntryData *) data)->pbEntry->entry.name,
				gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name)),
				max_phonebook_sim_name_length);
			((EditEntryData *) data)->pbEntry->entry.
			    name[max_phonebook_sim_name_length] = '\0';

			if (strlen(gtk_entry_get_text(GTK_ENTRY(((EditEntryData *) data)->name))) >
			    max_phonebook_sim_name_length) {
				gtk_label_set_text(GTK_LABEL(errorDialog.text),
						   _("Sorry, phonebook name will be truncated,\n\
because you save it into SIM memory!"));
				gtk_widget_show(errorDialog.dialog);
			}

			if (phoneMonitor.supported & PM_EXTPBK) {
			} else {
				strncpy(((EditEntryData *) data)->pbEntry->entry.number,
					gtk_entry_get_text(GTK_ENTRY
							   (((EditEntryData *) data)->number)),
					max_phonebook_sim_number_length);
				((EditEntryData *) data)->pbEntry->entry.
				    name[max_phonebook_sim_number_length] = '\0';
			}
		}

		((EditEntryData *) data)->pbEntry->entry.caller_group = ((EditEntryData *) data)->newGroup;

		((EditEntryData *) data)->pbEntry->status = E_Changed;
	}

	if (((EditEntryData *) data)->pbEntry->entry.memory_type == GN_MT_ME) {
		memoryStatus.UsedME++;
		memoryStatus.FreeME--;
		statusInfo.ch_ME = 1;
	} else {
		memoryStatus.UsedSM++;
		memoryStatus.FreeSM--;
		statusInfo.ch_SM = 1;
	}

	gtk_widget_hide(GTK_WIDGET(((EditEntryData *) data)->dialog));
	((EditEntryData *) data)->dialog = NULL;

	RefreshStatusInfo();

	gtk_clist_freeze(GTK_CLIST(clist));
	clist_row[0] = ((EditEntryData *) data)->pbEntry->entry.name;

	clist_row[1] = ((EditEntryData *) data)->pbEntry->entry.number;

	if (((EditEntryData *) data)->pbEntry->entry.memory_type == GN_MT_ME)
		clist_row[2] = "P";
	else
		clist_row[2] = "S";
	if (phoneMonitor.supported & PM_CALLERGROUP)
		clist_row[3] =
		    xgnokiiConfig.callerGroups[((EditEntryData *) data)->pbEntry->entry.caller_group];
	else
		clist_row[3] = "";
	gtk_clist_insert(GTK_CLIST(clist), 1, clist_row);

	if (((EditEntryData *) data)->pbEntry->entry.memory_type == GN_MT_ME)
		gtk_clist_set_pixmap(GTK_CLIST(clist), 1, 2,
				     memoryPixmaps.phoneMemPix, memoryPixmaps.mask);
	else
		gtk_clist_set_pixmap(GTK_CLIST(clist), 1, 2,
				     memoryPixmaps.simMemPix, memoryPixmaps.mask);

	gtk_clist_set_row_data(GTK_CLIST(clist), 1, (gpointer) ((EditEntryData *) data)->pbEntry);

	gtk_clist_sort(GTK_CLIST(clist));
	gtk_clist_thaw(GTK_CLIST(clist));
}


static void OkChangeEntryDialog(GtkWidget * widget, gpointer data)
{
	gchar *clist_row[4];
	gint row;
	PhonebookEntry *oldPbEntry, *newPbEntry;
	GList *sel;
	gchar string[100];

	sel = GTK_CLIST(clist)->selection;

	gtk_widget_hide(GTK_WIDGET(data));

	gtk_clist_freeze(GTK_CLIST(clist));

	while (sel != NULL) {
		row = GPOINTER_TO_INT(sel->data);
		oldPbEntry = (PhonebookEntry *) gtk_clist_get_row_data(GTK_CLIST(clist), row);

		sel = sel->next;

		if (oldPbEntry->entry.memory_type == GN_MT_SM) {
			if ((newPbEntry = FindFreeEntry(GN_MT_ME)) == NULL) {
				gtk_label_set_text(GTK_LABEL(errorDialog.text),
						   _("Can't change memory type!"));
				gtk_widget_show(errorDialog.dialog);
				return;
			}

			newPbEntry->entry = oldPbEntry->entry;
			newPbEntry->entry.memory_type = GN_MT_ME;

			newPbEntry->status = E_Changed;
			oldPbEntry->status = E_Deleted;

			memoryStatus.UsedME++;
			memoryStatus.FreeME--;
			memoryStatus.UsedSM--;
			memoryStatus.FreeSM++;
			statusInfo.ch_ME = statusInfo.ch_SM = 1;

		} else {
			if ((newPbEntry = FindFreeEntry(GN_MT_SM)) == NULL) {
				gtk_label_set_text(GTK_LABEL(errorDialog.text),
						   _("Can't change memory type!"));
				gtk_widget_show(errorDialog.dialog);
				return;
			}

			newPbEntry->entry = oldPbEntry->entry;
			newPbEntry->entry.name[max_phonebook_sim_name_length] = '\0';
			newPbEntry->entry.memory_type = GN_MT_SM;

			newPbEntry->status = E_Changed;
			oldPbEntry->status = E_Deleted;

			memoryStatus.UsedME--;
			memoryStatus.FreeME++;
			memoryStatus.UsedSM++;
			memoryStatus.FreeSM--;
			statusInfo.ch_ME = statusInfo.ch_SM = 1;
		}

		gtk_clist_remove(GTK_CLIST(clist), row);

		clist_row[0] = newPbEntry->entry.name;

		if (newPbEntry->entry.subentries_count > 0) {
			snprintf(string, 100, "%s *", newPbEntry->entry.number);
			clist_row[1] = string;
		} else
			clist_row[1] = newPbEntry->entry.number;

		if (newPbEntry->entry.memory_type == GN_MT_ME)
			clist_row[2] = "P";
		else
			clist_row[2] = "S";
		if (phoneMonitor.supported & PM_CALLERGROUP)
			clist_row[3] = xgnokiiConfig.callerGroups[newPbEntry->entry.caller_group];
		else
			clist_row[3] = "";
		gtk_clist_insert(GTK_CLIST(clist), row, clist_row);
		if (newPbEntry->entry.memory_type == GN_MT_ME)
			gtk_clist_set_pixmap(GTK_CLIST(clist), row, 2,
					     memoryPixmaps.phoneMemPix, memoryPixmaps.mask);
		else
			gtk_clist_set_pixmap(GTK_CLIST(clist), row, 2,
					     memoryPixmaps.simMemPix, memoryPixmaps.mask);

		gtk_clist_set_row_data(GTK_CLIST(clist), row, (gpointer) newPbEntry);
	}

	RefreshStatusInfo();
	gtk_clist_sort(GTK_CLIST(clist));
	gtk_clist_thaw(GTK_CLIST(clist));
}


static void SearchEntry(void)
{
	gchar *buf;
	gchar *entry;
	gint i;

	if (!contactsMemoryInitialized || *findEntryStruct.pattern == '\0')
		return;

	gtk_clist_unselect_all(GTK_CLIST(clist));
	g_strup(findEntryStruct.pattern);

	gtk_clist_get_text(GTK_CLIST(clist), findEntryStruct.lastRow, findEntryStruct.type, &entry);
	i = (findEntryStruct.lastRow + 1) % (memoryStatus.MaxME + memoryStatus.MaxSM);

	while (findEntryStruct.lastRow != i) {
		buf = g_strdup(entry);
		g_strup(buf);

		if (strstr(buf, findEntryStruct.pattern)) {
			g_free(buf);
			findEntryStruct.lastRow = i;
			gtk_clist_select_row(GTK_CLIST(clist),
					     (i + memoryStatus.MaxME + memoryStatus.MaxSM - 1)
					     % (memoryStatus.MaxME + memoryStatus.MaxSM),
					     findEntryStruct.type);
			gtk_clist_moveto(GTK_CLIST(clist),
					 (i + memoryStatus.MaxME + memoryStatus.MaxSM - 1)
					 % (memoryStatus.MaxME + memoryStatus.MaxSM),
					 findEntryStruct.type, 0.5, 0.5);
			return;
		}
		g_free(buf);
		gtk_clist_get_text(GTK_CLIST(clist), i, findEntryStruct.type, &entry);

		i = (i + 1) % (memoryStatus.MaxME + memoryStatus.MaxSM);
	}

	gtk_label_set_text(GTK_LABEL(errorDialog.text), _("Can't find pattern!"));
	gtk_widget_show(errorDialog.dialog);
}


static void OkFindEntryDialog(GtkWidget * widget, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(((FindEntryData *) data)->nameB)->active)
		findEntryStruct.type = FIND_NAME;
	else
		findEntryStruct.type = FIND_NUMBER;

	strncpy(findEntryStruct.pattern,
		gtk_entry_get_text(GTK_ENTRY(((FindEntryData *) data)->pattern)),
		max_phonebook_number_length);
	findEntryStruct.pattern[max_phonebook_number_length] = '\0';

	findEntryStruct.lastRow = 0;

	gtk_widget_hide(((FindEntryData *) data)->dialog);

	SearchEntry();
}


void CreateGroupMenu(EditEntryData * data)
{
	GtkWidget *item;

	if (data->groupMenu) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(data->group));
		if (GTK_IS_WIDGET(data->groupMenu))
			gtk_widget_destroy(GTK_WIDGET(data->groupMenu));
		data->groupMenu = NULL;
	}

	data->groupMenu = gtk_menu_new();

	item = gtk_menu_item_new_with_label(xgnokiiConfig.callerGroups[0]);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetGroup0), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(xgnokiiConfig.callerGroups[1]);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetGroup1), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(xgnokiiConfig.callerGroups[2]);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetGroup2), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(xgnokiiConfig.callerGroups[3]);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetGroup3), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(xgnokiiConfig.callerGroups[4]);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetGroup4), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(xgnokiiConfig.callerGroups[5]);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetGroup5), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(data->group), data->groupMenu);
}

void CreateTypeMenu(EditEntryData * data)
{
	GtkWidget *item;

	if (data->groupMenu) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(data->group));
		if (GTK_IS_WIDGET(data->groupMenu))
			gtk_widget_destroy(GTK_WIDGET(data->groupMenu));
		data->groupMenu = NULL;
	}

	data->groupMenu = gtk_menu_new();

	item = gtk_menu_item_new_with_label(_("General"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType0), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Mobile"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType1), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Work"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType2), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Fax"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType3), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Home"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType4), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Note"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType5), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Postal"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType6), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Email"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType7), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	item = gtk_menu_item_new_with_label(_("Name"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType8), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	if  ((g_strncasecmp(xgnokiiConfig.model, "6210", 4) != 0) &&
	     (g_strncasecmp(xgnokiiConfig.model, "7110", 4) != 0) &&
	     (g_strncasecmp(xgnokiiConfig.model, "6250", 4) != 0)) {
		item = gtk_menu_item_new_with_label(_("URL"));
		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(SetType9), (gpointer) data);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(data->groupMenu), item);
	}

	item = gtk_menu_item_new_with_label(_("Unknown"));
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(SetType10), (gpointer) data);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(data->groupMenu), item);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(data->group), data->groupMenu);
}


inline void GUI_RefreshGroupMenu(void)
{
	if (newEditEntryData.group)
		CreateGroupMenu(&newEditEntryData);

	if (editEditEntryData.group)
		CreateGroupMenu(&editEditEntryData);

	if (duplicateEditEntryData.group)
		CreateGroupMenu(&duplicateEditEntryData);
}

void inttotype(gint int_type, gchar *type)
{
	switch (int_type) {
	case 0:
		strcpy(type, _("General"));
		return;
	case 1:
		strcpy(type, _("Mobile"));
		return;
	case 2:
		strcpy(type, _("Work"));
		return;
	case 3:
		strcpy(type, _("Fax"));
		return;
	case 4:
		strcpy(type, _("Home"));
		return;
	case 5:
		strcpy(type, _("Note"));
		return;
	case 6:
		strcpy(type, _("Postal"));
		return;
	case 7:
		strcpy(type, _("Email"));
		return;
	case 8:
		strcpy(type, _("Name"));
		return;
	case 9:
		strcpy(type, _("URL"));
		return;
	case 10:
		strcpy(type, _("Unknown"));
		return;
	default:
		strcpy(type, "");
		return;
	}
}

static void OkEditSubEntriesDialog(GtkWidget * clist, gpointer data)
{
	gchar *clist_row[3], *temp_row1;
	gint row;

	row = editNumbersData.row;

	gtk_clist_get_text(GTK_CLIST(editNumbersData.clist), row, 0, &temp_row1);
	clist_row[0] = malloc(sizeof(gchar) * 2);
	strncpy(clist_row[0], temp_row1, 1);
	gtk_clist_remove(GTK_CLIST(editNumbersData.clist), row);

	clist_row[1] = strip_white(gtk_entry_get_text(GTK_ENTRY(editSubEntriesData.number)));
	clist_row[2] = malloc(sizeof(gchar) * 15);
	inttotype(editSubEntriesData.newType, clist_row[2]);
	gtk_clist_insert(GTK_CLIST(editNumbersData.clist), row, clist_row);

	gtk_widget_hide(GTK_WIDGET(editSubEntriesData.dialog));
}

static void DeleteEditSubEntriesDialog(GtkWidget * clist, gpointer data)
{
	gint row;

	row = editNumbersData.row;
	/* Do not allow to remove the 'main' entry */
	if (row != 0) gtk_clist_remove(GTK_CLIST(editNumbersData.clist), row);
	gtk_widget_hide(GTK_WIDGET(editSubEntriesData.dialog));
}

static void CancelEditSubEntriesDialog(GtkWidget * clist, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(editSubEntriesData.dialog));
}

static void CreateSubEntriesDialog(EditEntryData * editSubEntriesData, gchar * title, GtkSignalFunc okFunc, gint row)
{
	GtkWidget *button, *label, *hbox;

	editSubEntriesData->dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(editSubEntriesData->dialog), _("Edit subentries"));
	gtk_window_set_modal(GTK_WINDOW(editSubEntriesData->dialog), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(editSubEntriesData->dialog), 10);
	gtk_signal_connect(GTK_OBJECT(editSubEntriesData->dialog), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	button = gtk_button_new_with_label(_("Ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editSubEntriesData->dialog)->action_area),
			   button, TRUE, TRUE, 10);

	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(okFunc), GINT_TO_POINTER(row));

	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editSubEntriesData->dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(CancelEditSubEntriesDialog), NULL);

	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Delete"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editSubEntriesData->dialog)->action_area),
			   button, TRUE, TRUE, 10);

	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(DeleteEditSubEntriesDialog), NULL);

	gtk_widget_show(button);

	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(editSubEntriesData->dialog)->vbox), 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editSubEntriesData->dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editSubEntriesData->dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Number/Text:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	editSubEntriesData->number = gtk_entry_new_with_max_length(max_phonebook_number_length);
	gtk_box_pack_end(GTK_BOX(hbox), editSubEntriesData->number, FALSE, FALSE, 2);
	gtk_widget_show(editSubEntriesData->number);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editSubEntriesData->dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	editSubEntriesData->groupLabel = gtk_label_new(_("Type:"));
	gtk_box_pack_start(GTK_BOX(hbox), editSubEntriesData->groupLabel, FALSE, FALSE, 2);

	editSubEntriesData->group = gtk_option_menu_new();

	CreateTypeMenu(editSubEntriesData);

	gtk_box_pack_start(GTK_BOX(hbox), editSubEntriesData->group, TRUE, TRUE, 0);

}

gint typetoint(gchar *type)
{
	if (g_strcasecmp(type, _("General")) == 0) return (0);
	if (g_strcasecmp(type, _("Mobile")) == 0) return (1);
	if (g_strcasecmp(type, _("Work")) == 0) return (2);
	if (g_strcasecmp(type, _("Fax")) == 0) return (3);
	if (g_strcasecmp(type, _("Home")) == 0) return (4);
	if (g_strcasecmp(type, _("Note")) == 0) return (5);
	if (g_strcasecmp(type, _("Postal")) == 0) return (6);
	if (g_strcasecmp(type, _("Email")) == 0) return (7);
	if (g_strcasecmp(type, _("Name")) == 0) return (8);
	if (g_strcasecmp(type, _("URL")) == 0) return (9);
	if (g_strcasecmp(type, _("Unknown")) == 0) return (10);
	return(-1);
}


static void EditSubEntries(GtkWidget * clist,
			   gint row, gint column, GdkEventButton * event, gpointer data)
{
	gchar *buf;
	gint type;

	if (editSubEntriesData.dialog == NULL)
		CreateSubEntriesDialog(&editSubEntriesData, _("Edit entry"), (GtkSignalFunc) OkEditSubEntriesDialog, row);

	gtk_clist_get_text(GTK_CLIST(clist), row, 1, &buf);
	editNumbersData.row = row;
	
       	gtk_entry_set_text(GTK_ENTRY(editSubEntriesData.number), buf);

	gtk_clist_get_text(GTK_CLIST(clist), row, 2, &buf);
	type = typetoint(buf);
	editSubEntriesData.newType = type;
	gtk_option_menu_set_history(GTK_OPTION_MENU(editSubEntriesData.group), type);

	gtk_widget_show(editSubEntriesData.group);
	gtk_widget_show(editSubEntriesData.groupLabel);
	
	gtk_widget_show(GTK_WIDGET(editSubEntriesData.dialog));
}

gint typetohex(gchar *type)
{
	if (g_strcasecmp(type, _("General")) == 0) return (0x0a);
	if (g_strcasecmp(type, _("Mobile")) == 0) return (0x03);
	if (g_strcasecmp(type, _("Work")) == 0) return (0x06);
	if (g_strcasecmp(type, _("Fax")) == 0) return (0x04);
	if (g_strcasecmp(type, _("Home")) == 0) return (0x02);
	if (g_strcasecmp(type, _("Note")) == 0) return (0x0a);
	if (g_strcasecmp(type, _("Postal")) == 0) return (0x09);
	if (g_strcasecmp(type, _("Email")) == 0) return (0x08);
	if (g_strcasecmp(type, _("Name")) == 0) return (0x07);
	if (g_strcasecmp(type, _("URL")) == 0) return (0x2c);
	if (g_strcasecmp(type, _("Unknown")) == 0) return (0x00);
	return(-1);
}

static void OkEditNumbersDialog(GtkWidget * c_list, EditEntryData * editEntryData)
{
	gchar *row_data;
	gint found, i, type;

	editEntryData->pbEntry->entry.subentries_count = 0;

	editEntryData->pbEntry->status = E_Changed;

	for (i = 0; i < GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER; i++) {
		found = gtk_clist_get_text(GTK_CLIST(editNumbersData.clist), i, 0, &row_data);
		if (found != 1) break;
		if (g_strncasecmp(row_data, "*", 1) == 0) {
			found = gtk_clist_get_text(GTK_CLIST(editNumbersData.clist), i, 1, &row_data);
			memcpy(editEntryData->pbEntry->entry.number, row_data, strlen(row_data) + 1);
		} else {
			found = gtk_clist_get_text(GTK_CLIST(editNumbersData.clist), i, 1, &row_data);
		}
		memcpy(&editEntryData->pbEntry->entry.subentries[editEntryData->pbEntry->entry.subentries_count].data,
		       row_data,
		       strlen(row_data) + 1);
		found = gtk_clist_get_text(GTK_CLIST(editNumbersData.clist), i, 2, &row_data);
		if (found != 1) break;
		type = typetohex(row_data);
		switch (type) {
			/* FIXME: replace constants with the macros */
		case 0x0a:
			if (g_strcasecmp(row_data, _("General")) == 0) {
				editEntryData->pbEntry->entry.subentries
					[editEntryData->pbEntry->entry.subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number;
				editEntryData->pbEntry->entry.subentries
					[editEntryData->pbEntry->entry.subentries_count].number_type = type;
			} else if (g_strcasecmp(row_data, _("Note")) == 0) {
				editEntryData->pbEntry->entry.subentries
					[editEntryData->pbEntry->entry.subentries_count].entry_type = type;
				editEntryData->pbEntry->entry.subentries
					[editEntryData->pbEntry->entry.subentries_count].number_type = 0x00;
			}
			break;
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x06:
			editEntryData->pbEntry->entry.subentries
				[editEntryData->pbEntry->entry.subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number;
			editEntryData->pbEntry->entry.subentries
				[editEntryData->pbEntry->entry.subentries_count].number_type = type;
			break;
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x2c:
			editEntryData->pbEntry->entry.subentries
				[editEntryData->pbEntry->entry.subentries_count].entry_type = type;
			editEntryData->pbEntry->entry.subentries
				[editEntryData->pbEntry->entry.subentries_count].number_type = 0x00;
			break;
		default:
			break;
		}
				
		editEntryData->pbEntry->entry.subentries_count++;
	}
	gtk_clist_clear(GTK_CLIST(editNumbersData.clist));
	gtk_widget_hide(GTK_WIDGET(editNumbersData.dialog));
	snprintf(editEntryData->pbEntry->entry.name, max_phonebook_name_length,
		 "%s", gtk_entry_get_text(GTK_ENTRY(editEntryData->name)));
	editEntryData->pbEntry->entry.caller_group = editEntryData->newGroup;

	switch (editEntryData->row) {
	case -1:
		NewPbEntry(editEntryData->pbEntry);
		break;
	case -2:
		DuplicatePbEntry(editEntryData->pbEntry);
		break;
	default:
		EditPbEntry(editEntryData->pbEntry , editEntryData->row);
		break;
	}
}

static void NewEditNumbersDialog(GtkWidget * c_list, EditEntryData * editEntryData)
{
	gchar *row[3];

	switch (editNumbersData.pbEntry->entry.subentries_count) {
	case 0:
		strcpy(editNumbersData.pbEntry->entry.number, "");
		row[0] = "*";
		break;
	case GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER:
		return;
		break;
	default:
		row[0] = "";
		break;
	}

	row[1] = "";
	row[2] = _("General");
	gtk_clist_append(GTK_CLIST(editNumbersData.clist), row);
	strcpy(editNumbersData.pbEntry->entry.subentries[editNumbersData.pbEntry->entry.subentries_count].data.number, "");
	editNumbersData.pbEntry->entry.subentries[editNumbersData.pbEntry->entry.subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number;
	editNumbersData.pbEntry->entry.subentries[editNumbersData.pbEntry->entry.subentries_count].number_type = GN_PHONEBOOK_NUMBER_General;
	editNumbersData.pbEntry->entry.subentries_count++;
}

static void EditNumbers(GtkWidget * widget, EditEntryData *editEntryData)
{
	gchar *row[3];
	register gint i;
	GtkWidget *button, *clistScrolledWindow;

	editNumbersData.dialog = NULL;
	if (editNumbersData.dialog == NULL) {
		editNumbersData.dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(editNumbersData.dialog), _("Numbers"));
		gtk_window_set_modal(GTK_WINDOW(editNumbersData.dialog), TRUE);
		gtk_window_set_default_size(GTK_WINDOW(editNumbersData.dialog), 350, 200);

		gtk_container_set_border_width(GTK_CONTAINER(editNumbersData.dialog), 10);
		gtk_signal_connect(GTK_OBJECT(editNumbersData.dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editNumbersData.dialog)->action_area),
				   button, TRUE, TRUE, 10);

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    GTK_SIGNAL_FUNC (OkEditNumbersDialog), 
				    editEntryData);

		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);

		button = gtk_button_new_with_label(_("New"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editNumbersData.dialog)->action_area),
				   button, TRUE, TRUE, 10);

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    GTK_SIGNAL_FUNC (NewEditNumbersDialog), 
				    (gpointer) editEntryData);

		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER
					       (GTK_DIALOG(editNumbersData.dialog)->vbox), 5);

		editNumbersData.clist = gtk_clist_new(3);
		gtk_clist_set_shadow_type(GTK_CLIST(editNumbersData.clist), GTK_SHADOW_OUT);

		gtk_clist_set_column_width(GTK_CLIST(editNumbersData.clist), 0, 4);
		gtk_clist_set_column_width(GTK_CLIST(editNumbersData.clist), 1, 115);
		gtk_clist_set_column_width(GTK_CLIST(editNumbersData.clist), 2, 10);
		/*
		gtk_clist_set_column_justification (GTK_CLIST (editNumbersData.clist), 
						    2, GTK_JUSTIFY_CENTER);
		*/
		gtk_signal_connect(GTK_OBJECT(editNumbersData.clist), "select_row",
				   GTK_SIGNAL_FUNC(EditSubEntries), NULL);

		clistScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(clistScrolledWindow), editNumbersData.clist);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(clistScrolledWindow),
					       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editNumbersData.dialog)->vbox),
				  clistScrolledWindow);

		gtk_widget_show(editNumbersData.clist);
		gtk_widget_show(clistScrolledWindow);
	}
	gtk_clist_freeze(GTK_CLIST(editNumbersData.clist));
	gtk_clist_clear(GTK_CLIST(editNumbersData.clist));

	for (i = 0; i < editNumbersData.pbEntry->entry.subentries_count; i++) {
		if (strcmp(editNumbersData.pbEntry->entry.number,
			   editNumbersData.pbEntry->entry.subentries[i].data.number) == 0)
			row[0] = "*";
		else
			row[0] = "";

		row[1] = editNumbersData.pbEntry->entry.subentries[i].data.number;

		switch (editNumbersData.pbEntry->entry.subentries[i].entry_type) {
		case GN_PHONEBOOK_ENTRY_Number:
			switch (editNumbersData.pbEntry->entry.subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_General:
				row[2] = _("General");
				break;

			case GN_PHONEBOOK_NUMBER_Mobile:
				row[2] = _("Mobile");
				break;

			case GN_PHONEBOOK_NUMBER_Work:
				row[2] = _("Work");
				break;

			case GN_PHONEBOOK_NUMBER_Fax:
				row[2] = _("Fax");
				break;

			case GN_PHONEBOOK_NUMBER_Home:
				row[2] = _("Home");
				break;

			default:
				row[2] = _("Unknown");
				break;
			}
			break;

		case GN_PHONEBOOK_ENTRY_Note:
			row[2] = _("Note");
			break;

		case GN_PHONEBOOK_ENTRY_Postal:
			row[2] = _("Postal");
			break;

		case GN_PHONEBOOK_ENTRY_Email:
			row[2] = _("Email");
			break;

		case GN_PHONEBOOK_ENTRY_Name:
			row[2] = _("Name");
			break;
		case GN_PHONEBOOK_ENTRY_URL:
			row[2] = _("URL");
			break;
		default:
			row[2] = _("Unknown");
			break;
		}

		gtk_clist_append(GTK_CLIST(editNumbersData.clist), row);
	}

	gtk_clist_thaw(GTK_CLIST(editNumbersData.clist));

	gtk_widget_show(editNumbersData.dialog);

}


static void CreateEditDialog(EditEntryData * editEntryData, gchar * title, GtkSignalFunc okFunc)
{
	GtkWidget *button, *label, *hbox;

	editEntryData->dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(editEntryData->dialog), title);
	gtk_window_set_modal(GTK_WINDOW(editEntryData->dialog), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(editEntryData->dialog), 10);
	gtk_signal_connect(GTK_OBJECT(editEntryData->dialog), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	button = gtk_button_new_with_label(_("Ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editEntryData->dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(okFunc), editEntryData);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);
	button = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(editEntryData->dialog)->action_area),
			   button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(CancelEditDialog), editEntryData);
	gtk_widget_show(button);

	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(editEntryData->dialog)->vbox), 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editEntryData->dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Name:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	editEntryData->name = gtk_entry_new_with_max_length(100);

	gtk_box_pack_end(GTK_BOX(hbox), editEntryData->name, FALSE, FALSE, 2);
	gtk_widget_show(editEntryData->name);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editEntryData->dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	label = gtk_label_new(_("Number:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	if (phoneMonitor.supported & PM_EXTPBK) {
		button = gtk_button_new();
		editEntryData->number = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(button), editEntryData->number);

		gtk_box_pack_end(GTK_BOX(hbox), button, TRUE, TRUE, 2);

		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(EditNumbers), editEntryData);
		gtk_widget_show_all(button);
	} else {
		editEntryData->number = gtk_entry_new_with_max_length(max_phonebook_number_length);
		gtk_box_pack_end(GTK_BOX(hbox), editEntryData->number, FALSE, FALSE, 2);
		gtk_widget_show(editEntryData->number);
	}


	editEntryData->memoryBox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editEntryData->dialog)->vbox),
			  editEntryData->memoryBox);

	label = gtk_label_new(_("Memory:"));
	gtk_box_pack_start(GTK_BOX(editEntryData->memoryBox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	editEntryData->memoryTypePhone = gtk_radio_button_new_with_label(NULL, _("phone"));
	gtk_box_pack_end(GTK_BOX(editEntryData->memoryBox), editEntryData->memoryTypePhone, TRUE,
			 FALSE, 2);
	gtk_widget_show(editEntryData->memoryTypePhone);

	editEntryData->memoryTypeSIM =
	    gtk_radio_button_new_with_label(gtk_radio_button_group
					    (GTK_RADIO_BUTTON(editEntryData->memoryTypePhone)),
					    "SIM");
	gtk_box_pack_end(GTK_BOX(editEntryData->memoryBox), editEntryData->memoryTypeSIM, TRUE,
			 FALSE, 2);
	gtk_widget_show(editEntryData->memoryTypeSIM);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(editEntryData->dialog)->vbox), hbox);
	gtk_widget_show(hbox);

	editEntryData->groupLabel = gtk_label_new(_("Caller group:"));
	gtk_box_pack_start(GTK_BOX(hbox), editEntryData->groupLabel, FALSE, FALSE, 2);

	editEntryData->group = gtk_option_menu_new();

	CreateGroupMenu(editEntryData);

	gtk_box_pack_start(GTK_BOX(hbox), editEntryData->group, TRUE, TRUE, 0);
}


static void EditPbEntry(PhonebookEntry * pbEntry, gint row)
{
	editEditEntryData.pbEntry = pbEntry;
	editEditEntryData.newGroup = pbEntry->entry.caller_group;
	editEditEntryData.row = row;
	editNumbersData.pbEntry = pbEntry;
	editSubEntriesData.pbEntry = pbEntry;

	if (editEditEntryData.dialog == NULL)
		CreateEditDialog(&editEditEntryData, _("Edit entry"), (GtkSignalFunc) OkEditEntryDialog);


	gtk_entry_set_text(GTK_ENTRY(editEditEntryData.name), pbEntry->entry.name);

	if (phoneMonitor.supported & PM_EXTPBK)
		gtk_label_set_text(GTK_LABEL(editEditEntryData.number), pbEntry->entry.number);
	else
		gtk_entry_set_text(GTK_ENTRY(editEditEntryData.number), pbEntry->entry.number);

	if (pbEntry->entry.memory_type == GN_MT_ME)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editEditEntryData.memoryTypePhone),
					     TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editEditEntryData.memoryTypeSIM),
					     TRUE);

	gtk_option_menu_set_history(GTK_OPTION_MENU(editEditEntryData.group), pbEntry->entry.caller_group);

	if (phoneMonitor.supported & PM_CALLERGROUP) {
		gtk_widget_show(editEditEntryData.group);
		gtk_widget_show(editEditEntryData.groupLabel);
	} else {
		gtk_widget_hide(editEditEntryData.group);
		gtk_widget_hide(editEditEntryData.groupLabel);
	}

	if (memoryStatus.MaxME > 0)
		gtk_widget_show(GTK_WIDGET(editEditEntryData.memoryBox));
	else
		gtk_widget_hide(GTK_WIDGET(editEditEntryData.memoryBox));

	gtk_widget_show(GTK_WIDGET(editEditEntryData.dialog));
}


void DeletePbEntry(void)
{
	static GtkWidget *dialog = NULL;
	GtkWidget *button, *hbox, *label, *pixmap;

	if (dialog == NULL) {
		dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(dialog), _("Delete entries"));
		gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
		gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkDeleteEntryDialog), (gpointer) dialog);
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

		label = gtk_label_new(_("Do you want to delete selected entries?"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
		gtk_widget_show(label);
	}

	gtk_widget_show(GTK_WIDGET(dialog));
}


void NewPbEntry(PhonebookEntry * pbEntry)
{
	newEditEntryData.pbEntry = pbEntry;
	newEditEntryData.newGroup = pbEntry->entry.caller_group;
	newEditEntryData.row = -1;
	editNumbersData.pbEntry = pbEntry;
	editSubEntriesData.pbEntry = pbEntry;

	if (newEditEntryData.dialog == NULL)
		CreateEditDialog(&newEditEntryData, _("New entry"), (GtkSignalFunc) OkNewEntryDialog);

	gtk_entry_set_text(GTK_ENTRY(newEditEntryData.name), pbEntry->entry.name);

	if (phoneMonitor.supported & PM_EXTPBK)
		gtk_label_set_text(GTK_LABEL(newEditEntryData.number), pbEntry->entry.number);
	else
		gtk_entry_set_text(GTK_ENTRY(newEditEntryData.number), pbEntry->entry.number);

	if (pbEntry->entry.memory_type == GN_MT_ME)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(newEditEntryData.memoryTypePhone),
					     TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(newEditEntryData.memoryTypeSIM),
					     TRUE);

	gtk_option_menu_set_history(GTK_OPTION_MENU(newEditEntryData.group), pbEntry->entry.caller_group);

	if (phoneMonitor.supported & PM_CALLERGROUP) {
		gtk_widget_show(newEditEntryData.group);
		gtk_widget_show(newEditEntryData.groupLabel);
	} else {
		gtk_widget_hide(newEditEntryData.group);
		gtk_widget_hide(newEditEntryData.groupLabel);
	}

	if (memoryStatus.MaxME > 0)
		gtk_widget_show(GTK_WIDGET(newEditEntryData.memoryBox));
	else
		gtk_widget_hide(GTK_WIDGET(newEditEntryData.memoryBox));

	gtk_widget_show(GTK_WIDGET(newEditEntryData.dialog));
}


void DuplicatePbEntry(PhonebookEntry * pbEntry)
{
	duplicateEditEntryData.pbEntry = pbEntry;
	duplicateEditEntryData.newGroup = pbEntry->entry.caller_group;
	duplicateEditEntryData.row = -2;
	editNumbersData.pbEntry = pbEntry;
	editSubEntriesData.pbEntry = pbEntry;

	if (duplicateEditEntryData.dialog == NULL)
		CreateEditDialog(&duplicateEditEntryData, _("Duplicate entry"), (GtkSignalFunc) OkNewEntryDialog);

	gtk_entry_set_text(GTK_ENTRY(duplicateEditEntryData.name), pbEntry->entry.name);

	if (phoneMonitor.supported & PM_EXTPBK)
		gtk_label_set_text(GTK_LABEL(duplicateEditEntryData.number), pbEntry->entry.number);
	else
		gtk_entry_set_text(GTK_ENTRY(duplicateEditEntryData.number), pbEntry->entry.number);

	if (pbEntry->entry.memory_type == GN_MT_ME)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (duplicateEditEntryData.memoryTypePhone), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
					     (duplicateEditEntryData.memoryTypeSIM), TRUE);

	gtk_option_menu_set_history(GTK_OPTION_MENU(duplicateEditEntryData.group),
				    pbEntry->entry.caller_group);

	if (phoneMonitor.supported & PM_CALLERGROUP) {
		gtk_widget_show(duplicateEditEntryData.group);
		gtk_widget_show(duplicateEditEntryData.groupLabel);
	} else {
		gtk_widget_hide(duplicateEditEntryData.group);
		gtk_widget_hide(duplicateEditEntryData.groupLabel);
	}

	if (memoryStatus.MaxME > 0)
		gtk_widget_show(GTK_WIDGET(duplicateEditEntryData.memoryBox));
	else
		gtk_widget_hide(GTK_WIDGET(duplicateEditEntryData.memoryBox));

	gtk_widget_show(GTK_WIDGET(duplicateEditEntryData.dialog));
}


static void EditEntry(void)
{
	if (contactsMemoryInitialized) {
		if (GTK_CLIST(clist)->selection == NULL)
			return;

		EditPbEntry((PhonebookEntry *) gtk_clist_get_row_data(GTK_CLIST(clist),
								      GPOINTER_TO_INT(GTK_CLIST
										      (clist)->
										      selection->
										      data)),
			    GPOINTER_TO_INT(GTK_CLIST(clist)->selection->data));
	}
}


static void DuplicateEntry(void)
{
	PhonebookEntry *new, *old;

	if (contactsMemoryInitialized) {
		if (GTK_CLIST(clist)->selection == NULL)
			return;

		old =
		    (PhonebookEntry *) gtk_clist_get_row_data(GTK_CLIST(clist),
							      GPOINTER_TO_INT(GTK_CLIST(clist)->
									      selection->data));

		if (old->entry.memory_type == GN_MT_ME) {
			if ((new = FindFreeEntry(GN_MT_ME)) == NULL)
				if ((new = FindFreeEntry(GN_MT_SM)) == NULL) {
					gtk_label_set_text(GTK_LABEL(errorDialog.text),
							   _("Can't find free memory."));
					gtk_widget_show(errorDialog.dialog);
					return;
				}
		} else {
			if ((new = FindFreeEntry(GN_MT_SM)) == NULL)
				if ((new = FindFreeEntry(GN_MT_ME)) == NULL) {
					gtk_label_set_text(GTK_LABEL(errorDialog.text),
							   _("Can't find free memory."));
					gtk_widget_show(errorDialog.dialog);
					return;
				}
		}

		new->entry = old->entry;

		DuplicatePbEntry(new);
	}
}


static void NewEntry(void)
{
	PhonebookEntry *entry;

	if (contactsMemoryInitialized) {
		if ((entry = FindFreeEntry(GN_MT_ME)) != NULL) {
			memset(entry, 0, sizeof(PhonebookEntry));
			entry->entry.memory_type = GN_MT_ME;
			NewPbEntry(entry);
		} else if ((entry = FindFreeEntry(GN_MT_SM)) != NULL) {
			memset(entry, 0, sizeof(PhonebookEntry));
			entry->entry.memory_type = GN_MT_SM;
			NewPbEntry(entry);
		} else {
			gtk_label_set_text(GTK_LABEL(errorDialog.text),
					   _("Can't find free memory."));
			gtk_widget_show(errorDialog.dialog);
		}
	}
}


static inline void ClickEntry(GtkWidget * clist,
			      gint row, gint column, GdkEventButton * event, gpointer data)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		EditPbEntry((PhonebookEntry *) gtk_clist_get_row_data(GTK_CLIST(clist), row), row);
}


static inline void DeleteEntry(void)
{
	if (contactsMemoryInitialized) {
		if (GTK_CLIST(clist)->selection == NULL)
			return;

		DeletePbEntry();
	}
}


static void ChMemType(void)
{
	static GtkWidget *dialog = NULL;
	GtkWidget *button, *hbox, *label;

	if (contactsMemoryInitialized) {
		if (GTK_CLIST(clist)->selection == NULL)
			return;

		if (dialog == NULL) {
			dialog = gtk_dialog_new();
			gtk_window_set_title(GTK_WINDOW(dialog), _("Changing memory type"));
			gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
			gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
			gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
			gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
					   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

			button = gtk_button_new_with_label(_("Continue"));
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
					   button, FALSE, FALSE, 0);
			gtk_signal_connect(GTK_OBJECT(button), "clicked",
					   GTK_SIGNAL_FUNC(OkChangeEntryDialog), (gpointer) dialog);
			GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
			gtk_widget_grab_default(button);
			gtk_widget_show(button);

			button = gtk_button_new_with_label(_("Cancel"));
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
					   button, FALSE, FALSE, 0);
			gtk_signal_connect(GTK_OBJECT(button), "clicked",
					   GTK_SIGNAL_FUNC(CancelDialog), (gpointer) dialog);
			gtk_widget_show(button);

			hbox = gtk_hbox_new(FALSE, 0);
			gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
			gtk_widget_show(hbox);

			label =
			    gtk_label_new(_
					  ("If you change from phone memory to SIM memory\nsome entries may be truncated."));
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
			gtk_widget_show(label);
		}

		gtk_widget_show(dialog);
	}
}


static void FindFirstEntry(void)
{
	static FindEntryData findEntryData = { NULL };
	GtkWidget *button, *label, *hbox;

	if (contactsMemoryInitialized) {
		if (findEntryData.dialog == NULL) {
			findEntryData.dialog = gtk_dialog_new();
			gtk_window_set_title(GTK_WINDOW(findEntryData.dialog), _("Find"));
			gtk_window_set_modal(GTK_WINDOW(findEntryData.dialog), TRUE);
			gtk_container_set_border_width(GTK_CONTAINER(findEntryData.dialog), 10);
			gtk_signal_connect(GTK_OBJECT(findEntryData.dialog), "delete_event",
					   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

			button = gtk_button_new_with_label(_("Find"));
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(findEntryData.dialog)->action_area),
					   button, TRUE, TRUE, 10);
			gtk_signal_connect(GTK_OBJECT(button), "clicked",
					   GTK_SIGNAL_FUNC(OkFindEntryDialog),
					   (gpointer) & findEntryData);
			GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
			gtk_widget_grab_default(button);
			gtk_widget_show(button);

			button = gtk_button_new_with_label(_("Cancel"));
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(findEntryData.dialog)->action_area),
					   button, TRUE, TRUE, 10);
			gtk_signal_connect(GTK_OBJECT(button), "clicked",
					   GTK_SIGNAL_FUNC(CancelEditDialog),
					   (gpointer) & findEntryData);
			gtk_widget_show(button);

			gtk_container_set_border_width(GTK_CONTAINER
						       (GTK_DIALOG(findEntryData.dialog)->vbox), 5);

			hbox = gtk_hbox_new(FALSE, 0);
			gtk_container_add(GTK_CONTAINER(GTK_DIALOG(findEntryData.dialog)->vbox),
					  hbox);
			gtk_widget_show(hbox);

			label = gtk_label_new(_("Pattern:"));
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
			gtk_widget_show(label);

			findEntryData.pattern =
			    gtk_entry_new_with_max_length(max_phonebook_name_length);

			gtk_box_pack_end(GTK_BOX(hbox), findEntryData.pattern, FALSE, FALSE, 2);
			gtk_widget_show(findEntryData.pattern);

			hbox = gtk_hbox_new(FALSE, 0);
			gtk_container_add(GTK_CONTAINER(GTK_DIALOG(findEntryData.dialog)->vbox),
					  hbox);
			gtk_widget_show(hbox);

			findEntryData.nameB = gtk_radio_button_new_with_label(NULL, _("Name"));
			gtk_box_pack_start(GTK_BOX(hbox), findEntryData.nameB, TRUE, FALSE, 2);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(findEntryData.nameB), TRUE);
			gtk_widget_show(findEntryData.nameB);

			findEntryData.numberB =
			    gtk_radio_button_new_with_label(gtk_radio_button_group
							    (GTK_RADIO_BUTTON(findEntryData.nameB)),
							    _("Number"));
			gtk_box_pack_start(GTK_BOX(hbox), findEntryData.numberB, TRUE, FALSE, 2);
			gtk_widget_show(findEntryData.numberB);
		}

		gtk_widget_show(findEntryData.dialog);
	}
}


static inline void SelectAll(void)
{
	gtk_clist_select_all(GTK_CLIST(clist));
}


static inline void OkDialVoiceDialog(GtkWidget * w, gpointer data)
{
	PhoneEvent *e;
	gchar *buf = g_strdup(gtk_entry_get_text(GTK_ENTRY(((DialVoiceDialog *) data)->entry)));

	if (strlen(buf) > 0) {
		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_DialVoice;
		e->data = buf;
		GUI_InsertEvent(e);
	} else
		g_free(buf);

	gtk_widget_hide(((DialVoiceDialog *) data)->dialog);
}


static void DialVoice(void)
{
	static DialVoiceDialog dialVoiceDialog = { NULL, NULL };
	GtkWidget *button, *label;
	PhonebookEntry *pbEntry;

	if (dialVoiceDialog.dialog == NULL) {
		dialVoiceDialog.dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(dialVoiceDialog.dialog), _("Dial voice"));
		gtk_window_set_modal(GTK_WINDOW(dialVoiceDialog.dialog), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(dialVoiceDialog.dialog), 10);
		gtk_signal_connect(GTK_OBJECT(dialVoiceDialog.dialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);

		button = gtk_button_new_with_label(_("Ok"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialVoiceDialog.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(OkDialVoiceDialog),
				   (gpointer) & dialVoiceDialog);
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_widget_show(button);

		button = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialVoiceDialog.dialog)->action_area),
				   button, TRUE, TRUE, 10);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
				   GTK_SIGNAL_FUNC(CancelDialog),
				   (gpointer) dialVoiceDialog.dialog);
		gtk_widget_show(button);

		gtk_container_set_border_width(GTK_CONTAINER
					       (GTK_DIALOG(dialVoiceDialog.dialog)->vbox), 5);

		label = gtk_label_new(_("Number:"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialVoiceDialog.dialog)->vbox), label, FALSE,
				   FALSE, 5);
		gtk_widget_show(label);

		dialVoiceDialog.entry =
		    gtk_entry_new_with_max_length(GN_PHONEBOOK_NUMBER_MAX_LENGTH);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialVoiceDialog.dialog)->vbox),
				   dialVoiceDialog.entry, FALSE, FALSE, 5);
		gtk_widget_show(dialVoiceDialog.entry);
	}

	if (GTK_CLIST(clist)->selection != NULL) {
		pbEntry = (PhonebookEntry *) gtk_clist_get_row_data(GTK_CLIST(clist),
								    GPOINTER_TO_INT(GTK_CLIST
										    (clist)->
										    selection->
										    data));

		gtk_entry_set_text(GTK_ENTRY(dialVoiceDialog.entry), pbEntry->entry.number);
	}

	gtk_widget_show(dialVoiceDialog.dialog);
}


static gint CListCompareFunc(GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
	static gchar phoneText[] = "ME";
	static gchar simText[] = "SM";
	char *text1 = NULL;
	char *text2 = NULL;
	gint ret;

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

	if (clist->sort_column == 2) {
		if (((PhonebookEntry *) row1->data)->entry.memory_type == GN_MT_ME)
			text1 = phoneText;
		else
			text1 = simText;
		if (((PhonebookEntry *) row2->data)->entry.memory_type == GN_MT_ME)
			text2 = phoneText;
		else
			text2 = simText;
	}

	if (!text2)
		return (text1 != NULL);

	if (!text1)
		return -1;

	if ((ret = g_strcasecmp(text1, text2)) == 0) {
		if (((PhonebookEntry *) row1->data)->entry.memory_type <
		    ((PhonebookEntry *) row2->data)->entry.memory_type)
			ret = -1;
		else if (((PhonebookEntry *) row1->data)->entry.memory_type >
			 ((PhonebookEntry *) row2->data)->entry.memory_type)
			ret = 1;
	}

	return ret;
}


static void CreateProgressDialog(gint maxME, gint maxSM)
{
	GtkWidget *vbox, *label;
	GtkAdjustment *adj;

	progressDialog.dialog = gtk_dialog_new();
	gtk_window_position(GTK_WINDOW(progressDialog.dialog), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal(GTK_WINDOW(progressDialog.dialog), TRUE);
	gtk_signal_connect(GTK_OBJECT(progressDialog.dialog), "delete_event",
			   GTK_SIGNAL_FUNC(ProgressDialogDeleteEvent), NULL);

	vbox = GTK_DIALOG(progressDialog.dialog)->vbox;

	label = gtk_label_new(_("Phone memory..."));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(0, 1, maxME, 0, 0, 0);
	progressDialog.pbarME = gtk_progress_bar_new_with_adjustment(adj);

	gtk_box_pack_start(GTK_BOX(vbox), progressDialog.pbarME, FALSE, FALSE, 0);
	gtk_widget_show(progressDialog.pbarME);

	label = gtk_label_new(_("SIM memory..."));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	gtk_widget_show(label);

	adj = (GtkAdjustment *) gtk_adjustment_new(0, 1, maxSM, 0, 0, 0);
	progressDialog.pbarSM = gtk_progress_bar_new_with_adjustment(adj);

	gtk_box_pack_start(GTK_BOX(vbox), progressDialog.pbarSM, FALSE, FALSE, 0);
	gtk_widget_show(progressDialog.pbarSM);
}


static void SaveContacts(void)
{
	register gint i;
	PhonebookEntry *pbEntry;
	PhoneEvent *e;
	D_MemoryLocation *ml;

	if (progressDialog.dialog == NULL) {
		CreateProgressDialog(memoryStatus.MaxME, memoryStatus.MaxSM);
	}

	if (contactsMemoryInitialized && progressDialog.dialog) {
		gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarME), 0);
		gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarSM), 0);
		gtk_window_set_title(GTK_WINDOW(progressDialog.dialog), _("Saving entries"));
		gtk_widget_show_now(progressDialog.dialog);

		/* Save Phone memory */
		for (i = 0; i < memoryStatus.MaxME; i++) {
			pbEntry = g_ptr_array_index(contactsMemory, i);
			gn_log_xdebug("%d;%s;%s;%d;%d;%d\n", pbEntry->entry.empty, pbEntry->entry.name,
				pbEntry->entry.number, (int) pbEntry->entry.memory_type,
				pbEntry->entry.caller_group, (int) pbEntry->status);
			if (pbEntry->status == E_Changed || pbEntry->status == E_Deleted) {
				if (pbEntry->status == E_Deleted) {
					pbEntry->entry.name[0] = '\0';
					pbEntry->entry.number[0] = '\0';
					pbEntry->entry.caller_group = 5;
				}

				pbEntry->entry.location = i + 1;
				ml = (D_MemoryLocation *) g_malloc(sizeof(D_MemoryLocation));
				ml->entry = &(pbEntry->entry);
				e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
				e->event = Event_WriteMemoryLocation;
				e->data = ml;
				GUI_InsertEvent(e);
				pthread_mutex_lock(&memoryMutex);
				pthread_cond_wait(&memoryCond, &memoryMutex);
				pthread_mutex_unlock(&memoryMutex);

				if (ml->status != GN_ERR_NONE) {
					g_print(_
						("%s: line %d: Can't write ME memory entry number %d! Error: %d\n"),
						__FILE__, __LINE__, i + 1, ml->status);
				}
				g_free(ml);
			}
			gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarME), i + 1);
			GUI_Refresh();
		}

		/* Save SIM memory */
		for (i = memoryStatus.MaxME; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
			pbEntry = g_ptr_array_index(contactsMemory, i);

			gn_log_xdebug("%d;%s;%s;%d;%d;%d\n", pbEntry->entry.empty, pbEntry->entry.name,
				pbEntry->entry.number, (int) pbEntry->entry.memory_type,
				pbEntry->entry.caller_group, (int) pbEntry->status);

			if (pbEntry->status == E_Changed || pbEntry->status == E_Deleted) {
				if (pbEntry->status == E_Deleted) {
					pbEntry->entry.name[0] = '\0';
					pbEntry->entry.number[0] = '\0';
					pbEntry->entry.caller_group = 5;
				}

				pbEntry->entry.location = i - memoryStatus.MaxME + 1;
				ml = (D_MemoryLocation *) g_malloc(sizeof(D_MemoryLocation));
				ml->entry = &(pbEntry->entry);
				e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
				e->event = Event_WriteMemoryLocation;
				e->data = ml;
				GUI_InsertEvent(e);
				pthread_mutex_lock(&memoryMutex);
				pthread_cond_wait(&memoryCond, &memoryMutex);
				pthread_mutex_unlock(&memoryMutex);

				if (ml->status != GN_ERR_NONE) {
					g_print(_
						("%s: line %d: Can't write SM memory entry number %d! Error: %d\n"),
						__FILE__, __LINE__, i - memoryStatus.MaxME + 1,
						ml->status);
				}
				g_free(ml);
			}
			gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarSM),
					       i - memoryStatus.MaxME + 1);
			GUI_Refresh();
		}

		statusInfo.ch_ME = statusInfo.ch_SM = 0;
		RefreshStatusInfo();
		gtk_widget_hide(progressDialog.dialog);
	}
}


static GtkWidget *CreateSaveQuestionDialog(GtkSignalFunc SaveFunc, GtkSignalFunc DontSaveFunc)
{
	GtkWidget *dialog;
	GtkWidget *button, *label, *hbox, *pixmap;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Save changes?"));
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
	gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", GTK_SIGNAL_FUNC(DeleteEvent), NULL);

	button = gtk_button_new_with_label(_("Save"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(SaveFunc), (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Don't save"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 10);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(DontSaveFunc), (gpointer) dialog);
	gtk_widget_show(button);

	button = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 10);
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

	label = gtk_label_new(_("You have made changes in your\ncontacts directory.\n\
\n\nDo you want save these changes into phone?\n"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_widget_show(label);

	return dialog;
}

void GUI_RefreshContacts(void)
{
	PhonebookEntry *pbEntry;
	gint row_i = 0;
	register gint i;

	if (contactsMemoryInitialized == FALSE)
		return;

	gtk_clist_freeze(GTK_CLIST(clist));
	gtk_clist_clear(GTK_CLIST(clist));

	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		gchar *row[4];

		pbEntry = g_ptr_array_index(contactsMemory, i);
		if (pbEntry->status != E_Empty && pbEntry->status != E_Deleted) {
			row[0] = pbEntry->entry.name;
			row[1] = pbEntry->entry.number;

			if (pbEntry->entry.memory_type == GN_MT_ME)
				row[2] = "P";
			else
				row[2] = "S";
			if (phoneMonitor.supported & PM_CALLERGROUP)
				row[3] = xgnokiiConfig.callerGroups[pbEntry->entry.caller_group];
			else
				row[3] = "";
			gtk_clist_append(GTK_CLIST(clist), row);
			if (pbEntry->entry.memory_type == GN_MT_ME)
				gtk_clist_set_pixmap(GTK_CLIST(clist), row_i, 2,
						     memoryPixmaps.phoneMemPix, memoryPixmaps.mask);
			else
				gtk_clist_set_pixmap(GTK_CLIST(clist), row_i, 2,
						     memoryPixmaps.simMemPix, memoryPixmaps.mask);

			gtk_clist_set_row_data(GTK_CLIST(clist), row_i++, (gpointer) pbEntry);
		}
		gn_log_xdebug("%d;%s;%s;%d;%d;%d\n", pbEntry->entry.empty, pbEntry->entry.name,
			pbEntry->entry.number, (int) pbEntry->entry.memory_type,
			pbEntry->entry.caller_group, (int) pbEntry->status);
	}

	gtk_clist_sort(GTK_CLIST(clist));
	gtk_clist_thaw(GTK_CLIST(clist));
}


static gint InsertPBEntryME(gn_phonebook_entry * entry)
{
	PhonebookEntry *pbEntry;

	if ((pbEntry = (PhonebookEntry *) g_malloc(sizeof(PhonebookEntry))) == NULL) {
		g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
		g_ptr_array_free(contactsMemory, TRUE);
		gtk_widget_hide(progressDialog.dialog);
		return (-1);
	}

	pbEntry->entry = *entry;

	if (*pbEntry->entry.name == '\0' && *pbEntry->entry.number == '\0')
		pbEntry->status = E_Empty;
	else
		pbEntry->status = E_Unchanged;

	g_ptr_array_add(contactsMemory, (gpointer) pbEntry);
	gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarME), entry->location);
	GUI_Refresh();

	return GN_ERR_NONE;
}


static gint InsertPBEntrySM(gn_phonebook_entry * entry)
{
	PhonebookEntry *pbEntry;

	if ((pbEntry = (PhonebookEntry *) g_malloc(sizeof(PhonebookEntry))) == NULL) {
		g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
		g_ptr_array_free(contactsMemory, TRUE);
		gtk_widget_hide(progressDialog.dialog);
		return (-1);
	}

	pbEntry->entry = *entry;

	if (*pbEntry->entry.name == '\0' && *pbEntry->entry.number == '\0')
		pbEntry->status = E_Empty;
	else {
		pbEntry->status = E_Unchanged;
	}

	g_ptr_array_add(contactsMemory, (gpointer) pbEntry);
	gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarSM), entry->location);
	GUI_Refresh();

	return GN_ERR_NONE;
}


static inline gint ReadFailedPBEntry(gint i)
{
	g_ptr_array_free(contactsMemory, TRUE);
	gtk_widget_hide(progressDialog.dialog);
	return (0);
}


static void ReadContactsMain(void)
{
	PhoneEvent *e;
	D_MemoryStatus *ms;
	D_MemoryLocationAll *mla;
	PhonebookEntry *pbEntry;
	register gint i;
	bool has_memory_stats = true;

	if (contactsMemoryInitialized == TRUE) {
		for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
			pbEntry = g_ptr_array_index(contactsMemory, i);
			g_free(pbEntry);
		}
		g_ptr_array_free(contactsMemory, TRUE);
		contactsMemory = NULL;
		gtk_clist_clear(GTK_CLIST(clist));
		contactsMemoryInitialized = FALSE;
		memoryStatus.MaxME = memoryStatus.UsedME = memoryStatus.FreeME =
		    memoryStatus.MaxSM = memoryStatus.UsedSM = memoryStatus.FreeSM = 0;
		statusInfo.ch_ME = statusInfo.ch_SM = 0;
		RefreshStatusInfo();
	}

	ms = (D_MemoryStatus *) g_malloc(sizeof(D_MemoryStatus));
	ms->memoryStatus.memory_type = GN_MT_ME;
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetMemoryStatus;
	e->data = ms;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&memoryMutex);
	pthread_cond_wait(&memoryCond, &memoryMutex);
	pthread_mutex_unlock(&memoryMutex);

	if (ms->status == GN_ERR_NONE || ms->status == GN_ERR_NOTSUPPORTED) {
		memoryStatus.MaxME = ms->memoryStatus.used + ms->memoryStatus.free;
		memoryStatus.UsedME = ms->memoryStatus.used;
		memoryStatus.FreeME = ms->memoryStatus.free;
	} else {
		/* Phone doesn't support ME (5110) */
		memoryStatus.MaxME = memoryStatus.UsedME = memoryStatus.FreeME = 0;
	}

	ms->memoryStatus.memory_type = GN_MT_SM;
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetMemoryStatus;
	e->data = ms;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&memoryMutex);
	pthread_cond_wait(&memoryCond, &memoryMutex);
	pthread_mutex_unlock(&memoryMutex);

	has_memory_stats = ms->status == GN_ERR_NONE;
	if (ms->status == GN_ERR_NONE || ms->status == GN_ERR_NOTSUPPORTED) {
		memoryStatus.MaxSM = ms->memoryStatus.used + ms->memoryStatus.free;
		memoryStatus.UsedSM = ms->memoryStatus.used;
		memoryStatus.FreeSM = ms->memoryStatus.free;
	} else {
		/* guess that there are 100 entries and sort out UsedSM later */
		memoryStatus.MaxSM = memoryStatus.FreeSM = 100;
		memoryStatus.UsedSM = 0;
	}
	g_free(ms);

	statusInfo.ch_ME = statusInfo.ch_SM = 0;

	RefreshStatusInfo();

	if (progressDialog.dialog == NULL) {
		CreateProgressDialog(memoryStatus.MaxME, memoryStatus.MaxSM);
	}

	gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarME), 0);
	gtk_progress_set_value(GTK_PROGRESS(progressDialog.pbarSM), 0);
	gtk_window_set_title(GTK_WINDOW(progressDialog.dialog), _("Getting entries"));
	gtk_widget_show_now(progressDialog.dialog);

	contactsMemory = g_ptr_array_new();

	mla = (D_MemoryLocationAll *) g_malloc(sizeof(D_MemoryLocationAll));
	if (memoryStatus.MaxME > 0) {
		mla->min = 1;
		mla->max = memoryStatus.MaxME;
		mla->used = memoryStatus.UsedME;
		mla->type = GN_MT_ME;
		mla->InsertEntry = InsertPBEntryME;
		mla->ReadFailed = ReadFailedPBEntry;

		e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
		e->event = Event_GetMemoryLocationAll;
		e->data = mla;
		GUI_InsertEvent(e);
		pthread_mutex_lock(&memoryMutex);
		pthread_cond_wait(&memoryCond, &memoryMutex);
		pthread_mutex_unlock(&memoryMutex);

		if (mla->status != GN_ERR_NONE) {
			g_free(mla);
			gtk_widget_hide(progressDialog.dialog);
			return;
		}

		if (!has_memory_stats) {
			memoryStatus.FreeME = memoryStatus.UsedME = 0;
			for (i = 0; i < memoryStatus.MaxME; i++) {
				pbEntry = g_ptr_array_index(contactsMemory, i);

				if (pbEntry->status == E_Empty)
					memoryStatus.FreeME++;
				else
					memoryStatus.UsedME++;
			}
			RefreshStatusInfo();
		}
	}

	mla->min = 1;
	mla->max = memoryStatus.MaxSM;
	mla->used = memoryStatus.UsedSM;
	mla->type = GN_MT_SM;
	mla->InsertEntry = InsertPBEntrySM;
	mla->ReadFailed = ReadFailedPBEntry;

	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetMemoryLocationAll;
	e->data = mla;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&memoryMutex);
	pthread_cond_wait(&memoryCond, &memoryMutex);
	pthread_mutex_unlock(&memoryMutex);

	if (mla->status != GN_ERR_NONE) {
		g_free(mla);
		gtk_widget_hide(progressDialog.dialog);
		return;
	}

	if (!has_memory_stats) {
		memoryStatus.FreeSM = memoryStatus.UsedSM = 0;
		for (i = memoryStatus.MaxME; i < memoryStatus.MaxME+memoryStatus.MaxSM; i++) {
			pbEntry = g_ptr_array_index(contactsMemory, i);

			if (pbEntry->status == E_Empty)
				memoryStatus.FreeSM++;
			else
				memoryStatus.UsedSM++;
		}
		RefreshStatusInfo();
	}

	g_free(mla);

	gtk_widget_hide(progressDialog.dialog);

	contactsMemoryInitialized = TRUE;
	statusInfo.ch_ME = statusInfo.ch_SM = 0;
	GUIEventSend(GUI_EVENT_CONTACTS_CHANGED);
	GUIEventSend(GUI_EVENT_SMS_NUMBER_CHANGED);
}


static void ReadSaveCallback(GtkWidget * widget, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	SaveContacts();
	ReadContactsMain();
}


static void ReadDontSaveCallback(GtkWidget * widget, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	ReadContactsMain();
}


static void ReadSaveContacts(void)
{
	static GtkWidget *dialog = NULL;

	if (dialog == NULL)
		dialog = CreateSaveQuestionDialog((GtkSignalFunc) ReadSaveCallback, (GtkSignalFunc) ReadDontSaveCallback);

	gtk_widget_show(dialog);
}


static void ReadContacts(void)
{
	if (contactsMemoryInitialized == TRUE && (statusInfo.ch_ME || statusInfo.ch_SM))
		ReadSaveContacts();
	else
		ReadContactsMain();
}


inline void GUI_ReadContacts(void)
{
	ReadContacts();
}


inline void GUI_SaveContacts(void)
{
	SaveContacts();
}


inline void GUI_ShowContacts(void)
{
	if (xgnokiiConfig.callerGroups[0] == NULL) {
		GUI_Refresh();
		GUI_InitCallerGroupsInf();
	}
	gtk_clist_set_column_visibility(GTK_CLIST(clist), 3,
					phoneMonitor.supported & PM_CALLERGROUP);
	GUI_RefreshContacts();
	gtk_widget_show(GUI_ContactsWindow);
	/*
	if (!contactsMemoryInitialized)
		ReadContacts ();
	*/
}

static void ExportVCARD(FILE * f)
{
	gchar location[32];
	register gint i;
	PhonebookEntry *pbEntry;

	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		pbEntry = g_ptr_array_index(contactsMemory, i);

		if (pbEntry->status == E_Deleted || pbEntry->status == E_Empty)
			continue;

		if (pbEntry->entry.memory_type == GN_MT_ME)
			sprintf(location, "ME%d", i + 1);
		else
			sprintf(location, "SM%d", i - memoryStatus.MaxME + 1);

		gn_phonebook2vcard(f, &pbEntry->entry, location);
	}

	fclose(f);
}

static char *escape_semicolon(char *src)
{
	int i, j, len = strlen(src);
	char *dest;

	dest = calloc(2 * len, sizeof(char));
	for (i = 0, j = 0; i < len; i++, j++) {
		if (src[i] == ';')
			dest[j++] = '\\';
		dest[j] = src[i];
	}
	dest[j] = 0;
	return dest;
}

static void ExportNative(FILE * f)
{
	gchar buf[IO_BUF_LEN], buf2[16];
	register gint i, j;
	PhonebookEntry *pbEntry;


	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		pbEntry = g_ptr_array_index(contactsMemory, i);

		if (pbEntry->status != E_Deleted && pbEntry->status != E_Empty) {
			char *aux;
			aux = escape_semicolon(pbEntry->entry.name);
			strcpy(buf, aux);
			strcat(buf, ";");
			free(aux);

			strcat(buf, pbEntry->entry.number);
			strcat(buf, ";");

			if (pbEntry->entry.memory_type == GN_MT_ME)
				sprintf(buf2, "ME;%d;%d", i + 1, pbEntry->entry.caller_group);
			else
				sprintf(buf2, "SM;%d;%d", i - memoryStatus.MaxME + 1,
					pbEntry->entry.caller_group);
			strcat(buf, buf2);

			/* Add ext. pbk info if required */
			if (phoneMonitor.supported & PM_EXTPBK) {
				for (j = 0; j < pbEntry->entry.subentries_count; j++)
					switch (pbEntry->entry.subentries[j].entry_type) {
					case GN_PHONEBOOK_ENTRY_Date:
						break;
					default:
						sprintf(buf2, ";%d;%d;%d;",
							pbEntry->entry.subentries[j].entry_type,
							pbEntry->entry.subentries[j].number_type,
							pbEntry->entry.subentries[j].id);
						strcat(buf, buf2);

						aux = escape_semicolon(pbEntry->entry.subentries[j].data.number);
						strcat(buf,
						       pbEntry->entry.subentries[j].data.number);
						free(aux);
						break;
					}
			}
			fprintf(f, "%s\n", buf);
		}
	}

	fclose(f);
}


static void ExportContactsMain(gchar * name)
{
	FILE *f;
	gchar buf[IO_BUF_LEN];

	if ((f = fopen(name, "w")) == NULL) {
		g_snprintf(buf, IO_BUF_LEN, _("Can't open file %s for writing!\n"), name);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		return;
	}

	if (strstr(name, ".vcard") || strstr(name, ".gcrd"))
		ExportVCARD(f);
	else
		ExportNative(f);
}


static void YesExportDialog(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	ExportContactsMain(exportDialogData.fileName);
}


static void OkExportDialog(GtkWidget * w, GtkFileSelection * fs)
{
	static YesNoDialog dialog = { NULL, NULL };
	FILE *f;
	gchar err[255];

	exportDialogData.fileName = (gchar *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	gtk_widget_hide(GTK_WIDGET(fs));

	if ((f = fopen(exportDialogData.fileName, "r")) != NULL) {
		fclose(f);
		if (dialog.dialog == NULL) {
			CreateYesNoDialog(&dialog, (GtkSignalFunc) YesExportDialog, (GtkSignalFunc) CancelDialog,
					  GUI_ContactsWindow);
			gtk_window_set_title(GTK_WINDOW(dialog.dialog), _("Overwrite file?"));
			g_snprintf(err, 255, _("File %s already exists.\nOverwrite?"),
				   exportDialogData.fileName);
			gtk_label_set_text(GTK_LABEL(dialog.text), err);
		}
		gtk_widget_show(dialog.dialog);
	} else
		ExportContactsMain(exportDialogData.fileName);
}


static void ExportContacts(void)
{
	static GtkWidget *fileDialog = NULL;

	if (contactsMemoryInitialized) {
		if (fileDialog == NULL) {
			fileDialog = gtk_file_selection_new(_("Export to file"));
			gtk_signal_connect(GTK_OBJECT(fileDialog), "delete_event",
					   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileDialog)->ok_button),
					   "clicked", GTK_SIGNAL_FUNC(OkExportDialog),
					   (gpointer) fileDialog);
			gtk_signal_connect(GTK_OBJECT
					   (GTK_FILE_SELECTION(fileDialog)->cancel_button),
					   "clicked", GTK_SIGNAL_FUNC(CancelDialog),
					   (gpointer) fileDialog);
		}

		gtk_widget_show(fileDialog);
	}
}

static void OkImportDialog(GtkWidget * w, GtkFileSelection * fs)
{
	FILE *f;
	PhoneEvent *e;
	D_MemoryStatus *ms;
	PhonebookEntry *pbEntry;
	gn_phonebook_entry gsmEntry;
	gchar buf[IO_BUF_LEN];
	gchar *fileName;
	gint i;

	fileName = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	gtk_widget_hide(GTK_WIDGET(fs));

	if ((f = fopen(fileName, "r")) == NULL) {
		g_snprintf(buf, IO_BUF_LEN, _("Can't open file %s for reading!\n"), fileName);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		return;
	}

	if (contactsMemoryInitialized == TRUE) {
		for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
			pbEntry = g_ptr_array_index(contactsMemory, i);
			g_free(pbEntry);
		}
		g_ptr_array_free(contactsMemory, TRUE);
		contactsMemory = NULL;
		gtk_clist_clear(GTK_CLIST(clist));
		contactsMemoryInitialized = FALSE;
		memoryStatus.MaxME = memoryStatus.UsedME = memoryStatus.FreeME =
		    memoryStatus.MaxSM = memoryStatus.UsedSM = memoryStatus.FreeSM = 0;
		statusInfo.ch_ME = statusInfo.ch_SM = 0;
		RefreshStatusInfo();
	}

	ms = (D_MemoryStatus *) g_malloc(sizeof(D_MemoryStatus));
	ms->memoryStatus.memory_type = GN_MT_ME;
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetMemoryStatus;
	e->data = ms;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&memoryMutex);
	pthread_cond_wait(&memoryCond, &memoryMutex);
	pthread_mutex_unlock(&memoryMutex);

	if (ms->status == GN_ERR_NONE || ms->status == GN_ERR_NOTSUPPORTED) {
		memoryStatus.MaxME = ms->memoryStatus.used + ms->memoryStatus.free;
		memoryStatus.UsedME = 0;
		memoryStatus.FreeME = memoryStatus.MaxME;
	} else {
		/* Phone don't support ME (5110) */
		memoryStatus.MaxME = memoryStatus.UsedME = memoryStatus.FreeME = 0;
	}

	ms->memoryStatus.memory_type = GN_MT_SM;
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_GetMemoryStatus;
	e->data = ms;
	GUI_InsertEvent(e);
	pthread_mutex_lock(&memoryMutex);
	pthread_cond_wait(&memoryCond, &memoryMutex);
	pthread_mutex_unlock(&memoryMutex);

	if (ms->status == GN_ERR_NONE || ms->status == GN_ERR_NOTSUPPORTED) {
		memoryStatus.MaxSM = ms->memoryStatus.used + ms->memoryStatus.free;
		memoryStatus.UsedSM = 0;
		memoryStatus.FreeSM = memoryStatus.MaxSM;
	} else {
		/* guess that there are 100 entries */
		memoryStatus.MaxSM = memoryStatus.FreeSM = 100;
		memoryStatus.UsedSM = 0;
	}
	g_free(ms);

	statusInfo.ch_ME = statusInfo.ch_SM = 0;

	RefreshStatusInfo();

	contactsMemory = g_ptr_array_new();

	for (i = 1; i <= memoryStatus.MaxME; i++) {
		if ((pbEntry = (PhonebookEntry *) g_malloc(sizeof(PhonebookEntry))) == NULL) {
			fclose(f);
			g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
			g_ptr_array_free(contactsMemory, TRUE);
			return;
		}

		pbEntry->entry.empty = FALSE;
		pbEntry->entry.name[0] = '\0';
		pbEntry->entry.number[0] = '\0';
		pbEntry->entry.memory_type = GN_MT_ME;
		pbEntry->entry.caller_group = 5;
		pbEntry->status = E_Deleted;	/* I must set status to E_Deleted, since I   */
		/* I want clear empty entries when saving to */
		/* to phone                                  */
		pbEntry->entry.subentries_count = 0;

		g_ptr_array_add(contactsMemory, (gpointer) pbEntry);
		pbEntry = NULL;
	}

	for (i = 1; i <= memoryStatus.MaxSM; i++) {
		if ((pbEntry = (PhonebookEntry *) g_malloc(sizeof(PhonebookEntry))) == NULL) {
			fclose(f);
			g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__, __LINE__);
			g_ptr_array_free(contactsMemory, TRUE);
			return;
		}

		pbEntry->entry.empty = FALSE;
		pbEntry->entry.name[0] = '\0';
		pbEntry->entry.number[0] = '\0';
		pbEntry->entry.memory_type = GN_MT_SM;
		pbEntry->entry.caller_group = 5;
		pbEntry->status = E_Deleted;
		pbEntry->entry.subentries_count = 0;

		g_ptr_array_add(contactsMemory, (gpointer) pbEntry);
		pbEntry = NULL;
	}


	while (fgets(buf, IO_BUF_LEN, f)) {
		if (gn_file_phonebook_raw_parse(&gsmEntry, buf) == GN_ERR_NONE) {
			i = gsmEntry.location;
			if (gsmEntry.memory_type == GN_MT_ME && memoryStatus.FreeME > 0
			    && i > 0 && i <= memoryStatus.MaxME) {
				pbEntry = g_ptr_array_index(contactsMemory, i - 1);

				if (pbEntry->status == E_Deleted) {
					pbEntry->entry = gsmEntry;
					pbEntry->status = E_Changed;
					memoryStatus.UsedME++;
					memoryStatus.FreeME--;
					statusInfo.ch_ME = 1;
				}
			} else if (gsmEntry.memory_type == GN_MT_SM && memoryStatus.FreeSM > 0
				   && i > 0 && i <= memoryStatus.MaxSM) {
				pbEntry =
				    g_ptr_array_index(contactsMemory, memoryStatus.MaxME + i - 1);

				if (pbEntry->status == E_Deleted) {
					pbEntry->entry = gsmEntry;
					pbEntry->status = E_Changed;
					memoryStatus.UsedSM++;
					memoryStatus.FreeSM--;
					statusInfo.ch_SM = 1;
				}
			}
		}
	}

	contactsMemoryInitialized = TRUE;
	RefreshStatusInfo();
	GUIEventSend(GUI_EVENT_CONTACTS_CHANGED);
	GUIEventSend(GUI_EVENT_SMS_NUMBER_CHANGED);
}


static void ImportContactsFileDialog()
{
	static GtkWidget *fileDialog = NULL;

	if (fileDialog == NULL) {
		fileDialog = gtk_file_selection_new(_("Import from file"));
		gtk_signal_connect(GTK_OBJECT(fileDialog), "delete_event",
				   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileDialog)->ok_button),
				   "clicked", GTK_SIGNAL_FUNC(OkImportDialog),
				   (gpointer) fileDialog);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileDialog)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC(CancelDialog), (gpointer) fileDialog);
	}

	gtk_widget_show(fileDialog);
}


static void ImportSaveCallback(GtkWidget * widget, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	SaveContacts();
	ImportContactsFileDialog();
}


static void ImportDontSaveCallback(GtkWidget * widget, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	ImportContactsFileDialog();
}


void static ImportSaveContacts(void)
{
	static GtkWidget *dialog = NULL;

	if (dialog == NULL)
		dialog = CreateSaveQuestionDialog((GtkSignalFunc) ImportSaveCallback, (GtkSignalFunc) ImportDontSaveCallback);

	gtk_widget_show(dialog);
}


static void ImportContacts(void)
{
	if (contactsMemoryInitialized == TRUE && (statusInfo.ch_ME || statusInfo.ch_SM))
		ImportSaveContacts();
	else
		ImportContactsFileDialog();
}


static void QuitSaveCallback(GtkWidget * widget, gpointer data)
{
	PhoneEvent *e;

	gtk_widget_hide(GTK_WIDGET(data));
	SaveContacts();
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_Exit;
	e->data = NULL;
	GUI_InsertEvent(e);
	pthread_join(monitor_th, NULL);
	gtk_main_quit();
}


static void QuitDontSaveCallback(GtkWidget * widget, gpointer data)
{
	PhoneEvent *e;

	gtk_widget_hide(GTK_WIDGET(data));
	e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	e->event = Event_Exit;
	e->data = NULL;
	GUI_InsertEvent(e);
	pthread_join(monitor_th, NULL);
	gtk_main_quit();
}


void GUI_QuitSaveContacts(void)
{
	static GtkWidget *dialog = NULL;

	if (dialog == NULL)
		dialog = CreateSaveQuestionDialog((GtkSignalFunc) QuitSaveCallback, (GtkSignalFunc) QuitDontSaveCallback);

	gtk_widget_show(dialog);
}


/* Function take number and return name belonged to number.
   If no name is found, return NULL;
   Do not modify returned name!
*/
gchar *GUI_GetName(gchar * number)
{
	PhonebookEntry *pbEntry;
	register gint i;

	if (contactsMemoryInitialized == FALSE || number == NULL)
		return (gchar *) NULL;

	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		pbEntry = g_ptr_array_index(contactsMemory, i);

		if (pbEntry->status == E_Empty || pbEntry->status == E_Deleted)
			continue;

		if (strcmp(pbEntry->entry.number, number) == 0)
			return pbEntry->entry.name;
	}

	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		pbEntry = g_ptr_array_index(contactsMemory, i);

		if (pbEntry->status == E_Empty || pbEntry->status == E_Deleted)
			continue;

		if (strrncmp(pbEntry->entry.number, number, 9) == 0)
			return pbEntry->entry.name;
	}

	return NULL;
}


gchar *GUI_GetNameExact(gchar * number)
{
	PhonebookEntry *pbEntry;
	register gint i;

	if (contactsMemoryInitialized == FALSE || number == NULL)
		return (gchar *) NULL;

	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		pbEntry = g_ptr_array_index(contactsMemory, i);

		if (pbEntry->status == E_Empty || pbEntry->status == E_Deleted)
			continue;

		if (strcmp(pbEntry->entry.number, number) == 0)
			return pbEntry->entry.name;
	}

	return NULL;
}


gchar *GUI_GetNumber(gchar * name)
{
	PhonebookEntry *pbEntry;
	register gint i;

	if (contactsMemoryInitialized == FALSE || name == NULL || *name == '\0')
		return (gchar *) NULL;

	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		pbEntry = g_ptr_array_index(contactsMemory, i);

		if (pbEntry->status == E_Empty || pbEntry->status == E_Deleted)
			continue;

		if (strcmp(g_strstrip(pbEntry->entry.name), g_strstrip(name)) == 0)
			return pbEntry->entry.number;
	}

	return NULL;
}


static void SelectDialogClickEntry(GtkWidget * clist,
				   gint row,
				   gint column, GdkEventButton * event, SelectContactData * data)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		gtk_signal_emit_by_name(GTK_OBJECT(data->okButton), "clicked");
}


SelectContactData *GUI_SelectContactDialog(void)
{
	PhonebookEntry *pbEntry;
	static SelectContactData selectContactData;
	SortColumn *sColumn;
	gchar *titles[4] = { _("Name"), _("Number"), _("Memory"), _("Group") };
	gint row_i = 0;
	register gint i;
	gchar string[100];

	if (contactsMemoryInitialized == FALSE)
		return NULL;

	selectContactData.dialog = gtk_dialog_new();
	gtk_widget_set_usize(GTK_WIDGET(selectContactData.dialog), 436, 200);
	gtk_window_set_title(GTK_WINDOW(selectContactData.dialog), _("Select contacts"));
	gtk_window_set_modal(GTK_WINDOW(selectContactData.dialog), TRUE);

	selectContactData.okButton = gtk_button_new_with_label(_("Ok"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(selectContactData.dialog)->action_area),
			   selectContactData.okButton, TRUE, TRUE, 10);
	GTK_WIDGET_SET_FLAGS(selectContactData.okButton, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(selectContactData.okButton);
	gtk_widget_show(selectContactData.okButton);

	selectContactData.cancelButton = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(selectContactData.dialog)->action_area),
			   selectContactData.cancelButton, TRUE, TRUE, 10);
	gtk_widget_show(selectContactData.cancelButton);

	selectContactData.clist = gtk_clist_new_with_titles(4, titles);
	gtk_clist_set_shadow_type(GTK_CLIST(selectContactData.clist), GTK_SHADOW_OUT);
	gtk_clist_set_compare_func(GTK_CLIST(selectContactData.clist), CListCompareFunc);
	gtk_clist_set_sort_column(GTK_CLIST(selectContactData.clist), 0);
	gtk_clist_set_sort_type(GTK_CLIST(selectContactData.clist), GTK_SORT_ASCENDING);
	gtk_clist_set_auto_sort(GTK_CLIST(selectContactData.clist), FALSE);
	gtk_clist_set_selection_mode(GTK_CLIST(selectContactData.clist), GTK_SELECTION_EXTENDED);

	gtk_clist_set_column_width(GTK_CLIST(selectContactData.clist), 0, 150);
	gtk_clist_set_column_width(GTK_CLIST(selectContactData.clist), 1, 115);
	gtk_clist_set_column_width(GTK_CLIST(selectContactData.clist), 3, 70);
	gtk_clist_set_column_justification(GTK_CLIST(selectContactData.clist), 2,
					   GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_visibility(GTK_CLIST(selectContactData.clist), 3,
					phoneMonitor.supported & PM_CALLERGROUP);

	for (i = 0; i < 4; i++) {
		if ((sColumn = g_malloc(sizeof(SortColumn))) == NULL) {
			g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__,
				__LINE__);
			return NULL;
		}
		sColumn->clist = selectContactData.clist;
		sColumn->column = i;
		gtk_signal_connect(GTK_OBJECT(GTK_CLIST(selectContactData.clist)->column[i].button),
				   "clicked", GTK_SIGNAL_FUNC(SetSortColumn), (gpointer) sColumn);
	}

	gtk_signal_connect(GTK_OBJECT(selectContactData.clist), "select_row",
			   GTK_SIGNAL_FUNC(SelectDialogClickEntry), (gpointer) & selectContactData);

	selectContactData.clistScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(selectContactData.clistScrolledWindow),
			  selectContactData.clist);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(selectContactData.clistScrolledWindow),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(selectContactData.dialog)->vbox),
			   selectContactData.clistScrolledWindow, TRUE, TRUE, 0);

	gtk_widget_show(selectContactData.clist);
	gtk_widget_show(selectContactData.clistScrolledWindow);

	if (xgnokiiConfig.callerGroups[0] == NULL)
		GUI_InitCallerGroupsInf();

	gtk_clist_freeze(GTK_CLIST(selectContactData.clist));
	for (i = 0; i < memoryStatus.MaxME + memoryStatus.MaxSM; i++) {
		gchar *row[4];

		pbEntry = g_ptr_array_index(contactsMemory, i);
		if (pbEntry->status != E_Empty && pbEntry->status != E_Deleted) {
			row[0] = pbEntry->entry.name;

			if (pbEntry->entry.subentries_count > 0) {
				snprintf(string, 100, "%s *", pbEntry->entry.number);
				row[1] = string;
			} else
				row[1] = pbEntry->entry.number;


			if (pbEntry->entry.memory_type == GN_MT_ME)
				row[2] = "P";
			else
				row[2] = "S";
			if (phoneMonitor.supported & PM_CALLERGROUP)
				row[3] = xgnokiiConfig.callerGroups[pbEntry->entry.caller_group];
			else
				row[3] = "";
			gtk_clist_append(GTK_CLIST(selectContactData.clist), row);
			if (pbEntry->entry.memory_type == GN_MT_ME)
				gtk_clist_set_pixmap(GTK_CLIST(selectContactData.clist), row_i, 2,
						     memoryPixmaps.phoneMemPix, memoryPixmaps.mask);
			else
				gtk_clist_set_pixmap(GTK_CLIST(selectContactData.clist), row_i, 2,
						     memoryPixmaps.simMemPix, memoryPixmaps.mask);

			gtk_clist_set_row_data(GTK_CLIST(selectContactData.clist), row_i++,
					       (gpointer) pbEntry);
		}
	}

	gtk_clist_sort(GTK_CLIST(selectContactData.clist));
	gtk_clist_thaw(GTK_CLIST(selectContactData.clist));

	gtk_widget_show(selectContactData.dialog);

	return &selectContactData;
}


static GtkItemFactoryEntry menu_items[] = {
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>R", ReadContacts, 0, NULL},
	{NULL, "<control>S", SaveContacts, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>I", ImportContacts, 0, NULL},
	{NULL, "<control>E", ExportContacts, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>W", CloseContacts, 0, NULL},
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>N", NewEntry, 0, NULL},
	{NULL, "<control>U", DuplicateEntry, 0, NULL},
	{NULL, NULL, EditEntry, 0, NULL},
	{NULL, "<control>D", DeleteEntry, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>C", ChMemType, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>F", FindFirstEntry, 0, NULL},
	{NULL, "<control>L", SearchEntry, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>A", SelectAll, 0, NULL},
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>V", DialVoice, 0, NULL},
	{NULL, NULL, NULL, 0, "<LastBranch>"},
	{NULL, NULL, Help1, 0, NULL},
	{NULL, NULL, GUI_ShowAbout, 0, NULL},
};


static void InitMainMenu(void)
{
	menu_items[0].path = g_strdup(_("/_File"));
	menu_items[1].path = g_strdup(_("/File/_Read from phone"));
	menu_items[2].path = g_strdup(_("/File/_Save to phone"));
	menu_items[3].path = g_strdup(_("/File/Sep1"));
	menu_items[4].path = g_strdup(_("/File/_Import from file"));
	menu_items[5].path = g_strdup(_("/File/_Export to file"));
	menu_items[6].path = g_strdup(_("/File/Sep2"));
	menu_items[7].path = g_strdup(_("/File/_Close"));
	menu_items[8].path = g_strdup(_("/_Edit"));
	menu_items[9].path = g_strdup(_("/Edit/_New"));
	menu_items[10].path = g_strdup(_("/Edit/D_uplicate"));
	menu_items[11].path = g_strdup(_("/Edit/_Edit"));
	menu_items[12].path = g_strdup(_("/Edit/_Delete"));
	menu_items[13].path = g_strdup(_("/Edit/Sep3"));
	menu_items[14].path = g_strdup(_("/Edit/_Change memory type"));
	menu_items[15].path = g_strdup(_("/Edit/Sep4"));
	menu_items[16].path = g_strdup(_("/Edit/_Find"));
	menu_items[17].path = g_strdup(_("/Edit/Find ne_xt"));
	menu_items[18].path = g_strdup(_("/Edit/Sep5"));
	menu_items[19].path = g_strdup(_("/Edit/Select _all"));
	menu_items[20].path = g_strdup(_("/_Dial"));
	menu_items[21].path = g_strdup(_("/Dial/Dial _voice"));
	menu_items[22].path = g_strdup(_("/_Help"));
	menu_items[23].path = g_strdup(_("/Help/_Help"));
	menu_items[24].path = g_strdup(_("/Help/_About"));
}


void GUI_CreateContactsWindow(void)
{
	int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	SortColumn *sColumn;
	GtkWidget *menubar;
	GtkWidget *main_vbox;
	GtkWidget *toolbar;
	GtkWidget *clistScrolledWindow;
	GtkWidget *status_hbox;
	register gint i;
	gchar *titles[4] = { _("Name"), _("Number"), _("Memory"), _("Group") };

	InitMainMenu();
	contactsMemoryInitialized = FALSE;
	GUI_ContactsWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_ContactsWindow), "ContactsWindow", "Xgnokii");
	gtk_window_set_title(GTK_WINDOW(GUI_ContactsWindow), _("Contacts"));
	gtk_widget_set_usize(GTK_WIDGET(GUI_ContactsWindow), 436, 220);
	//gtk_container_set_border_width (GTK_CONTAINER (GUI_ContactsWindow), 10);
	gtk_signal_connect(GTK_OBJECT(GUI_ContactsWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	gtk_widget_realize(GUI_ContactsWindow);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

	gtk_window_add_accel_group(GTK_WINDOW(GUI_ContactsWindow), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
	gtk_container_add(GTK_CONTAINER(GUI_ContactsWindow), main_vbox);
	gtk_widget_show(main_vbox);

	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);

	/* Create the toolbar */

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Read from phone"), NULL,
				NewPixmap(Read_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ReadContacts, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Save to phone"), NULL,
				NewPixmap(Send_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) SaveContacts, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Import from file"), NULL,
				NewPixmap(Open_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ImportContacts, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Export to file"), NULL,
				NewPixmap(Save_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ExportContacts, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("New entry"), NULL,
				NewPixmap(New_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) NewEntry, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Duplicate entry"), NULL,
				NewPixmap(Duplicate_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) DuplicateEntry, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Edit entry"), NULL,
				NewPixmap(Edit_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) EditEntry, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Delete entry"), NULL,
				NewPixmap(Delete_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) DeleteEntry, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Dial voice"), NULL,
				NewPixmap(Dial_xpm, GUI_ContactsWindow->window,
					  &GUI_ContactsWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) DialVoice, NULL);

//  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_widget_show(toolbar);


	clist = gtk_clist_new_with_titles(4, titles);
	gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_OUT);
	gtk_clist_set_compare_func(GTK_CLIST(clist), CListCompareFunc);
	gtk_clist_set_sort_column(GTK_CLIST(clist), 0);
	gtk_clist_set_sort_type(GTK_CLIST(clist), GTK_SORT_ASCENDING);
	gtk_clist_set_auto_sort(GTK_CLIST(clist), FALSE);
	gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_EXTENDED);

	gtk_clist_set_column_width(GTK_CLIST(clist), 0, 150);
	gtk_clist_set_column_width(GTK_CLIST(clist), 1, 115);
	gtk_clist_set_column_width(GTK_CLIST(clist), 3, 70);
	gtk_clist_set_column_justification(GTK_CLIST(clist), 2, GTK_JUSTIFY_CENTER);
//  gtk_clist_set_column_visibility (GTK_CLIST (clist), 3, phoneMonitor.supported & PM_CALLERGROUP);

	for (i = 0; i < 4; i++) {
		if ((sColumn = g_malloc(sizeof(SortColumn))) == NULL) {
			g_print(_("Error: %s: line %d: Can't allocate memory!\n"), __FILE__,
				__LINE__);
			gtk_main_quit();
		}
		sColumn->clist = clist;
		sColumn->column = i;
		gtk_signal_connect(GTK_OBJECT(GTK_CLIST(clist)->column[i].button), "clicked",
				   GTK_SIGNAL_FUNC(SetSortColumn), (gpointer) sColumn);
	}

	gtk_signal_connect(GTK_OBJECT(clist), "select_row", GTK_SIGNAL_FUNC(ClickEntry), NULL);

	clistScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(clistScrolledWindow), clist);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(clistScrolledWindow),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(main_vbox), clistScrolledWindow, TRUE, TRUE, 0);

	gtk_widget_show(clist);
	gtk_widget_show(clistScrolledWindow);

	status_hbox = gtk_hbox_new(FALSE, 20);
	gtk_box_pack_start(GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 0);
	gtk_widget_show(status_hbox);

	memoryStatus.MaxME = memoryStatus.UsedME = memoryStatus.FreeME =
	    memoryStatus.MaxSM = memoryStatus.UsedSM = memoryStatus.FreeSM = 0;
	statusInfo.ch_ME = statusInfo.ch_SM = 0;

	statusInfo.label = gtk_label_new("");
	RefreshStatusInfo();
	gtk_box_pack_start(GTK_BOX(status_hbox), statusInfo.label, FALSE, FALSE, 10);
	gtk_widget_show(statusInfo.label);

	memoryPixmaps.simMemPix = gdk_pixmap_create_from_xpm_d(GUI_ContactsWindow->window,
							       &memoryPixmaps.mask,
							       &GUI_ContactsWindow->style->
							       bg[GTK_STATE_NORMAL], sim_xpm);

	memoryPixmaps.phoneMemPix = gdk_pixmap_create_from_xpm_d(GUI_ContactsWindow->window,
								 &memoryPixmaps.mask,
								 &GUI_ContactsWindow->style->
								 bg[GTK_STATE_NORMAL], phone_xpm);

	questMark.pixmap = gdk_pixmap_create_from_xpm_d(GUI_ContactsWindow->window,
							&questMark.mask,
							&GUI_ContactsWindow->style->
							bg[GTK_STATE_NORMAL], quest_xpm);

	CreateErrorDialog(&errorDialog, GUI_ContactsWindow);
	GUIEventAdd(GUI_EVENT_CONTACTS_CHANGED, GUI_RefreshContacts);
	GUIEventAdd(GUI_EVENT_CALLERS_GROUPS_CHANGED, GUI_RefreshGroupMenu);
}
