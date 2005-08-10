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

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak.
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002      BORBELY Zoltan, Markus Plail

*/

#ifndef XGNOKII_CONTACTS_H
#define XGNOKII_CONTACTS_H

#include <gtk/gtk.h>
#include "misc.h"
#include "gnokii.h"

#define IO_BUF_LEN	1024

/* Structure to keep memory status information */
typedef struct {
	int MaxME;		/* Maximum Phone memory entries. */
	int UsedME;		/* Actualy used Phone memory entries. */
	int FreeME;		/* FreeME = MaxME - UsedME */
	int MaxSM;		/* Maximum SIM memory entries. */
	int UsedSM;
	int FreeSM;
} MemoryStatus;

/* Array to hold contacts entry */
typedef GPtrArray *ContactsMemory;

/* Structure to keep contacts memory entry status */
typedef enum {
	E_Unchanged,		/* Entry is not empty and is unchanged. */
	E_Changed,		/* Entry is not empty and is changed. */
	E_Deleted,		/* Entry was deleted. */
	E_Empty			/* Entry is empty. */
} EntryStatus;

/* Memory entry data */
typedef struct {
	gn_phonebook_entry entry;	/* Phonebook entry self. */
	EntryStatus status;	/* Entry status. */
} PhonebookEntry;

/* Structure to hold information of Edit and New dialogs */
typedef struct {
	PhonebookEntry *pbEntry;
	GtkWidget *dialog;
	GtkWidget *name;
	GtkWidget *number;
	GtkWidget *extended;
	GtkWidget *memoryBox;
	GtkWidget *memoryTypePhone;
	GtkWidget *memoryTypeSIM;
	GtkWidget *group;
	GtkWidget *groupLabel;
	GtkWidget *groupMenu;
	gint newGroup;
	gint newType;
	gint row;
} EditEntryData;


typedef struct {
	PhonebookEntry *pbEntry;
	GtkWidget *dialog;
	GtkWidget *clist;
	gint row;
} EditNumbersData;


/* Structure to hold information for FindEntry dialog. */
typedef struct {
	GtkWidget *dialog;
	GtkWidget *pattern;
	GtkWidget *nameB;
	GtkWidget *numberB;
} FindEntryData;


/* Contains fileName for Export dialog. */
typedef struct {
	gchar *fileName;
} ExportDialogData;


/* Hold widgets for SelectContactDialog */
typedef struct {
	GtkWidget *dialog;
	GtkWidget *clist;	/* list of contacts */
	GtkWidget *clistScrolledWindow;
	GtkWidget *okButton;	/* Ok and Cancel button widgets */
	GtkWidget *cancelButton;
} SelectContactData;


/* Max length for status line. (Line that shows used/max information for
   memories). */
#define STATUS_INFO_LENGTH	40


/* Structure to hold information for status line (bottom line of window) */
typedef struct {
	GtkWidget *label;
	gchar text[STATUS_INFO_LENGTH];	/* Status line text. */
	gint ch_ME:1;		/* 1 if phone memory was changed */
	gint ch_SM:1;		/* 1 if phone SIM was changed */
} StatusInfo;


/* Structure to hold information for progress dialog */
typedef struct {
	GtkWidget *dialog;
	GtkWidget *pbarME;
	GtkWidget *pbarSM;
} ProgressDialog;


/* Search type. */
typedef enum {
	FIND_NAME = 0,
	FIND_NUMBER
} FindType;


typedef struct {
	gchar pattern[GN_PHONEBOOK_NAME_MAX_LENGTH + 1];
	gint lastRow;
	FindType type;
} FindEntryStruct;


typedef struct {
	GdkPixmap *simMemPix, *phoneMemPix;
	GdkBitmap *mask;
} MemoryPixmaps;


extern void GUI_CreateContactsWindow(void);

extern void GUI_ShowContacts(void);

/* return != 0 if user has unsaved changes in contacts memory */
extern gint GUI_ContactsIsChanged(void);

/* return TRUE if Contacts memory was read from phone or from file */
extern bool GUI_ContactsIsIntialized(void);

/* Read contacts from phone */
extern void GUI_ReadContacts(void);

/* Save contacts to phone */
extern void GUI_SaveContacts(void);

/* Create save question dialog and can end application */
extern void GUI_QuitSaveContacts(void);

extern void GUI_RefreshContacts(void);

/* Function take number and return name belonged to number.
   If no name is found, return NULL;
   Do not modify returned name!					*/
extern gchar *GUI_GetName(gchar * number);

extern gchar *GUI_GetNameExact(gchar * number);

extern gchar *GUI_GetNumber(gchar * name);
/* Function show dialog with contacts and let select entries.
   See xgnokii_contacts.c for sample of use.			*/
extern SelectContactData *GUI_SelectContactDialog(void);

extern void GUI_RefreshGroupMenu(void);

extern PhonebookEntry *GUI_GetEntry(gn_memory_type, gint);

void ExtPbkChanged(GtkWidget * widget, gpointer data);

#endif
