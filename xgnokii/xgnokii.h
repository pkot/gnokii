/*

  X G N O K I I
  
  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.
  
  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  Last modification: Mon Oct 10 1999
  Modified by Jan Derfinak
              
*/

#ifndef XGNOKII_H
#define XGNOKII_H

#include <gtk/gtk.h>
#include "../misc.h"
#include "../gsm-common.h"

#define MAX_CALLER_GROUP_LENGTH	10
#define MAX_SMS_SET_LENGTH	15
#define MAX_SMS_CENTER		10
#define MAX_BUSINESS_CARD_LENGTH	139

typedef enum {
  SMSFormat_Text,
  SMSFormat_Fax,
  SMSFormat_Paging,
  SMSFormat_E_Mail
} SMSFormat;

typedef enum {
  SMSValidity_Max,
  SMSValidity_1h,
  SMSValidity_6h,
  SMSValidity_24h,
  SMSValidity_72h,
  SMSValidity_1week
} SMSValidity;

typedef struct {
  gchar       name[MAX_SMS_SET_LENGTH + 1];
  gchar       number[GSM_MAX_SMS_CENTER_LENGTH + 1]; /* SMS center number */
  SMSFormat   format;	/* SMS sending format */
  SMSValidity validity;	/* Validity period */
} SMSSetting;

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
  GtkWidget *alarmSwitch;
  GtkWidget *alarmHour;
  GtkWidget *alarmMin;
} AlarmWidgets;

typedef struct {
  GtkWidget *port;
  GtkWidget *model;
  GtkWidget *init;
  GtkWidget *bindir;
  GtkWidget *serial, *infrared;
} ConnectionWidgets;

typedef struct {
  GtkWidget *set;
  GtkWidget *number;
  GtkWidget *format;
  GtkWidget *validity;
  SMSSetting smsSetting[MAX_SMS_CENTER];
  gint       currentSet:4;
} SMSWidgets;

typedef struct {
  GtkWidget *name;
  GtkWidget *title;
  GtkWidget *company;
  GtkWidget *telephone;
  GtkWidget *fax;
  GtkWidget *email;
  GtkWidget *address;
  GtkWidget *status;
  gint   max;
  gint   used;
} UserWidget;

typedef struct {
  gchar *initlength; /* Init length from .gnokiirc file */
  gchar *model;      /* Model from .gnokiirc file. */
  gchar *port;       /* Serial port from .gnokiirc file */
  gchar *connection; /* Connection type from .gnokiirc file */
  gchar *bindir;
  SMSSetting smsSetting[MAX_SMS_CENTER];
  UserInf user;
  gchar *callerGroups[6];
  gint   smsSets:4;
  bool   callerGroupsSupported:1;
  bool   alarmSupported:1;
} XgnokiiConfig;
  
/* Hold main configuration data for xgnokii */
extern XgnokiiConfig xgnokiiConfig;

extern gint max_phonebook_name_length;
extern gint max_phonebook_number_length;
extern gint max_phonebook_sim_name_length;
extern gint max_phonebook_sim_number_length;

void GUI_ShowAbout();

#endif XGNOKII_H
