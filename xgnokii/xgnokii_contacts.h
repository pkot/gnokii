/*

  X G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Mon Aug 23 1999
  Modified by Jan Derfinak

*/

#ifndef XGNOKII_CONTACTS_H
#define XGNOKII_CONTACTS_H

#include <gtk/gtk.h>
#include "../misc.h"
#include "../gsm-common.h"
#include "../gsm-api.h"
#include "xgnokii.h"

#define IO_BUF_LEN	160

/* Structure to keep memory status information */
typedef struct {
  int MaxME;
  int UsedME;
  int FreeME;
  int MaxSM;
  int UsedSM;
  int FreeSM;
} MemoryStatus;

/* Array to hold contacts entry */
typedef GPtrArray* ContactsMemory;

/* Structure to keep contacts memory entry status */
typedef enum {
  E_Unchanged,
  E_Changed,
  E_Deleted,
  E_Empty
} EntryStatus;

/* Structure of status memory entry */
typedef struct {
  GSM_PhonebookEntry entry;
  EntryStatus status;
} PhonebookEntry;

/* Structure to hold information of Edit and New dialogs */
typedef struct {
  PhonebookEntry *pbEntry;
  GtkWidget *dialog;
  GtkWidget *name;
  GtkWidget *number;
  GtkWidget *memoryTypePhone;
  GtkWidget *memoryTypeSIM;
  GtkWidget *group;
  gint      newGroup;
  gint      row;
} EditEntryData;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *pattern;
  GtkWidget *nameB;
  GtkWidget *numberB;
} FindEntryData;

typedef struct {
  gchar *fileName;
} ExportDialogData;

#define STATUS_INFO_LENGTH	40

/* Structure to hold information for status line (bottom line of window) */
typedef struct {
  GtkWidget *label;
  gchar text[STATUS_INFO_LENGTH];
  gint ch_ME:1;
  gint ch_SM:1;
} StatusInfo;

/* Structure to hold information for progress dialog */
typedef struct {
  GtkWidget *dialog;
  GtkWidget *pbarME;
  GtkWidget *pbarSM;
} ProgressDialog;

typedef enum {
  FIND_NAME = 0,
  FIND_NUMBER
} FindType;

typedef struct {
  gchar pattern[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1];
  gint lastRow;
  FindType type;
} FindEntryStruct;

typedef struct {
  GdkPixmap *simMemPix, *phoneMemPix;
  GdkBitmap *mask;
} MemoryPixmaps;

typedef struct {
  GdkPixmap *pixmap;
  GdkBitmap *mask;
} QuestMark;

extern void GUI_CreateContactsWindow();

extern void GUI_ShowContacts();

/* return != 0 if user has unsaved changes in contacts memory */ 
extern gint GUI_ContactsIsChanged();

/* return TRUE is user had read memory from phone */
extern bool GUI_ContactsIsIntialized();

/* Read contacts from phone */
extern void GUI_ReadContacts();

/* Save contacts to phone */
extern void GUI_SaveContacts();

extern void GUI_QuitSaveContacts();
#endif
