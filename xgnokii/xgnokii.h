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
  Copyright (C) 2001      Chris Kemp
  Copyright (C) 2002      Markus Plail
  Copyright (C) 2002-2004 Pawel Kot

*/

#ifndef XGNOKII_H
#define XGNOKII_H

#include <gtk/gtk.h>
#include "config.h"
#include "misc.h"
#include "gnokii.h"

#define MAX_CALLER_GROUP_LENGTH	10
#define MAX_SMS_CENTER		10
#define MAX_BUSINESS_CARD_LENGTH	139

typedef struct {
	gchar *name;
	gchar *title;
	gchar *company;
	gchar *telephone;
	gchar *fax;
	gchar *email;
	gchar *address;
} UserInf;

typedef struct {
	gint initlength;	/* Init length from the config file */
	gchar *model;		/* Model from the config file. */
	gchar *port;		/* Serial port from the config file */
	gn_connection_type connection;	/* Connection type from the config file */
	const gchar *bindir;
	gchar *xgnokiidir;
	gchar *mailbox;		/* Mailbox, where we can save SMS's */
	gchar *maxSIMLen;	/* Max length of names on SIM card */
	gchar *maxPhoneLen;	/* Max length of names in phone */
	gchar *locale;		/* Locale for the app translations */
	gn_sms_message_center smsSetting[MAX_SMS_CENTER];
	UserInf user;
	gint allowBreakage;
	gchar *callerGroups[6];
	gint smsSets:4;
	bool alarmSupported:1;
} XgnokiiConfig;

/* Hold main configuration data for xgnokii */
extern XgnokiiConfig xgnokiiConfig;

extern gint lastfoldercount, foldercount;
extern char folders[GN_SMS_FOLDER_MAX_NUMBER][GN_SMS_MESSAGE_MAX_NUMBER];
extern gint max_phonebook_name_length;
extern gint max_phonebook_number_length;
extern gint max_phonebook_sim_name_length;
extern gint max_phonebook_sim_number_length;
extern void GUI_InitCallerGroupsInf(void);
extern void GUI_InitSMSSettings(void);
extern void GUI_ShowAbout(void);
extern void MainExit(gchar *);

#endif				/* XGNOKII_H */
