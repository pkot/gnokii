/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000-2006  Pawel Kot
  Copyright (C) 2001       Jan Kratochvil
  Copyright (C) 2003       BORBELY Zoltan
  
  Header file for gnokii utility.

*/

#ifndef __gnokii_app_h_
#define __gnokii_app_h_

#include <errno.h>
#include "gnokii-internal.h"

#define MAX_INPUT_LINE_LEN 512

/* shared globals */
extern volatile bool bshutdown;

/* Utils functions */
int writefile(char *filename, char *text, int mode);
extern gn_error readtext(gn_sms_user_data *udata, int input_len); 
extern gn_error loadbitmap(gn_bmp *bitmap, char *s, int type, struct gn_statemachine *state);
extern int parse_end_value_option(int argc, char *argv[], int pos, int start_value);
extern void interrupted(int sig);
extern void console_raw(void);

/* Monitoring functions */
extern void monitor_usage(FILE *f);

extern int monitormode(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int displayoutput(gn_data *data, struct gn_statemachine *state);
extern int getdisplaystatus(gn_data *data, struct gn_statemachine *state);
extern int netmonitor(char *m, gn_data *data, struct gn_statemachine *state);

/* SMS functions */
extern void sms_usage(FILE *f);

extern void sendsms_usage(FILE *f, int exitval);
extern int sendsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void savesms_usage(FILE *f, int exitval);
extern int savesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void getsms_usage(FILE *f, int exitval);
extern int getsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void deletesms_usage(FILE *f, int exitval);
extern int deletesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

extern void getsmsc_usage(FILE *f, int exitval);
extern int getsmsc(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void setsmsc_usage(FILE *f, int exitval);
extern int setsmsc(gn_data *data, struct gn_statemachine *state);

extern void createsmsfolder_usage(FILE *f, int exitval);
extern int createsmsfolder(char *name, gn_data *data, struct gn_statemachine *state);
extern void deletesmsfolder_usage(FILE *f, int exitval);
extern int deletesmsfolder(char *number, gn_data *data, struct gn_statemachine *state);
extern void showsmsfolderstatus_usage(FILE *f, int exitval);
extern int showsmsfolderstatus(gn_data *data, struct gn_statemachine *state);

int smsreader(gn_data *data, struct gn_statemachine *state);

/* Phonebook functions */
extern void phonebook_usage(FILE *f);

extern void getphonebook_usage(FILE *f, int exitval);
extern int getphonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void writephonebook_usage(FILE *f, int exitval);
extern int writephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void deletephonebook_usage(FILE *f, int exitval);
extern int deletephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Calendar functions */
extern void calendar_usage(FILE *f);

extern void getcalendarnote_usage(FILE *f, int exitval);
extern int getcalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int writecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* ToDo functions */
extern void todo_usage(FILE *f);

extern void gettodo_usage(FILE *f, int exitval);
extern int gettodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int writetodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletealltodos(gn_data *data, struct gn_statemachine *state);

/* Dialling and call handling functions */
extern void dial_usage(FILE *f);

extern int getspeeddial(char *number, gn_data *data, struct gn_statemachine *state);
extern int setspeeddial(char *argv[], gn_data *data, struct gn_statemachine *state);
extern int dialvoice(char *number, gn_data *data, struct gn_statemachine *state);
extern int senddtmf(char *string, gn_data *data, struct gn_statemachine *state);
extern int answercall(char *callid, gn_data *data, struct gn_statemachine *state);
extern int hangup(char *callid, gn_data *data, struct gn_statemachine *state);
extern void divert_usage(FILE *f, int exitval);
extern int divert(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Profile options */
extern void profile_usage(FILE *f);

extern void getprofile_usage(FILE *f, int exitval);
extern int getprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int setprofile(gn_data *data, struct gn_statemachine *state);
extern int getactiveprofile(gn_data *data, struct gn_statemachine *state);
extern int setactiveprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Phone settings options */
extern void settings_usage(FILE *f);

extern int setdatetime(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getdatetime(gn_data *data, struct gn_statemachine *state);
extern int setalarm(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getalarm(gn_data *data, struct gn_statemachine *state);
extern int reset(char *type, gn_data *data, struct gn_statemachine *state);

/* WAP options */
extern void wap_usage(FILE *f);

extern int getwapbookmark(char *number, gn_data *data, struct gn_statemachine *state);
extern void writewapbookmark_usage(FILE *f, int exitval);
extern int writewapbookmark(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletewapbookmark(char *number, gn_data *data, struct gn_statemachine *state);
extern void getwapsetting_usage(FILE *f, int exitval);
extern int getwapsetting(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int writewapsetting(gn_data *data, struct gn_statemachine *state);
extern int activatewapsetting(char *number, gn_data *data, struct gn_statemachine *state);

/* Logo options */
extern void logo_usage(FILE *f);

extern int sendlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int setlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int viewlogo(char *filename);

/* Ringtone options */
extern void ringtone_usage(FILE *f);

extern char *get_ringtone_name(int id, gn_data *data, struct gn_statemachine *state);
extern void getringtone_usage(FILE *f, int exitval);
extern int sendringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void setringtone_usage(FILE *f, int exitval);
extern int setringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void playringtone_usage(FILE *f, int exitval);
extern int playringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void ringtoneconvert_usage(FILE *f, int exitval);
extern int ringtoneconvert(int argc, char *argv[]);
extern int getringtonelist(gn_data *data, struct gn_statemachine *state);
extern void deleteringtone_usage(FILE *f, int exitval);
extern gn_error deleteringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Security options */
extern void security_usage(FILE *f);

extern int identify(struct gn_statemachine *state);
extern int getlocksinfo(gn_data *data, struct gn_statemachine *state);
extern int getsecuritycode(gn_data *data, struct gn_statemachine *state);
#ifdef SECURITY
extern void entersecuritycode_usage(FILE *f, int exitval);
extern int entersecuritycode(char *type, gn_data *data, struct gn_statemachine *state);
extern int getsecuritycodestatus(gn_data *data, struct gn_statemachine *state);
extern void changesecuritycode_usage(FILE *f, int exitval);
extern int changesecuritycode(char *type, gn_data *data, struct gn_statemachine *state);
#endif

/* File options */
extern void file_usage(FILE *f);

extern int getfilelist(char *path, gn_data *data, struct gn_statemachine *state);
extern int getfiledetailsbyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getfileid(char *filename, gn_data *data, struct gn_statemachine *state);
extern int deletefile(char *filename, gn_data *data, struct gn_statemachine *state);
extern int deletefilebyid(char *id, gn_data *data, struct gn_statemachine *state);
extern void getfile_usage(FILE *f, int exitval);
extern int getfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void getfilebyid_usage(FILE *f, int exitval);
extern int getfilebyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getallfiles(char *path, gn_data *data, struct gn_statemachine *state);
extern void putfile_usage(FILE *f, int exitval);
extern int putfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Misc options */
extern void other_usage(FILE *f);

extern int pmon(gn_data *data, struct gn_statemachine *state);
extern int presskeysequence(gn_data *data, struct gn_statemachine *state);
extern int enterchar(gn_data *data, struct gn_statemachine *state);
extern void list_gsm_networks(void);
extern int getnetworkinfo(gn_data *data, struct gn_statemachine *state);
extern int gnokii_atoi(char *string);

/* Compatibility functions */
#ifndef HAVE_GETLINE
#  ifdef HAVE_SYS_PARAM_H
#    include <sys/param.h>
#  endif
int getline(char **line, size_t *len, FILE *stream);
#endif

#endif /* __gnokii_app_h_ */
