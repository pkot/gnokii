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

extern gn_error monitormode(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error displayoutput(gn_data *data, struct gn_statemachine *state);
extern gn_error getdisplaystatus(gn_data *data, struct gn_statemachine *state);
extern gn_error netmonitor(char *m, gn_data *data, struct gn_statemachine *state);

/* SMS functions */
extern void sms_usage(FILE *f);

extern void sendsms_usage(FILE *f, int exitval);
extern gn_error sendsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void savesms_usage(FILE *f, int exitval);
extern gn_error savesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void getsms_usage(FILE *f, int exitval);
extern gn_error getsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void deletesms_usage(FILE *f, int exitval);
extern gn_error deletesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

extern void getsmsc_usage(FILE *f, int exitval);
extern gn_error getsmsc(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void setsmsc_usage(FILE *f, int exitval);
extern gn_error setsmsc(gn_data *data, struct gn_statemachine *state);

extern void createsmsfolder_usage(FILE *f, int exitval);
extern gn_error createsmsfolder(char *name, gn_data *data, struct gn_statemachine *state);
extern void deletesmsfolder_usage(FILE *f, int exitval);
extern gn_error deletesmsfolder(char *number, gn_data *data, struct gn_statemachine *state);
extern void showsmsfolderstatus_usage(FILE *f, int exitval);
extern gn_error showsmsfolderstatus(gn_data *data, struct gn_statemachine *state);

gn_error smsreader(gn_data *data, struct gn_statemachine *state);

/* Phonebook functions */
extern void phonebook_usage(FILE *f);

extern void getphonebook_usage(FILE *f, int exitval);
extern gn_error getphonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void writephonebook_usage(FILE *f, int exitval);
extern gn_error writephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void deletephonebook_usage(FILE *f, int exitval);
extern gn_error deletephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Calendar functions */
extern void calendar_usage(FILE *f);

extern void getcalendarnote_usage(FILE *f, int exitval);
extern gn_error getcalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error writecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error deletecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* ToDo functions */
extern void todo_usage(FILE *f);

extern void gettodo_usage(FILE *f, int exitval);
extern gn_error gettodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error writetodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error deletealltodos(gn_data *data, struct gn_statemachine *state);

/* Dialling and call handling functions */
extern void dial_usage(FILE *f);

extern gn_error getspeeddial(char *number, gn_data *data, struct gn_statemachine *state);
extern gn_error setspeeddial(char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error dialvoice(char *number, gn_data *data, struct gn_statemachine *state);
extern gn_error senddtmf(char *string, gn_data *data, struct gn_statemachine *state);
extern gn_error answercall(char *callid, gn_data *data, struct gn_statemachine *state);
extern gn_error hangup(char *callid, gn_data *data, struct gn_statemachine *state);
extern void divert_usage(FILE *f, int exitval);
extern gn_error divert(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Profile options */
extern void profile_usage(FILE *f);

extern void getprofile_usage(FILE *f, int exitval);
extern gn_error getprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error setprofile(gn_data *data, struct gn_statemachine *state);
extern gn_error getactiveprofile(gn_data *data, struct gn_statemachine *state);
extern gn_error setactiveprofile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Phone settings options */
extern void settings_usage(FILE *f);

extern gn_error setdatetime(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getdatetime(gn_data *data, struct gn_statemachine *state);
extern gn_error setalarm(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getalarm(gn_data *data, struct gn_statemachine *state);
extern gn_error reset(char *type, gn_data *data, struct gn_statemachine *state);

/* WAP options */
extern void wap_usage(FILE *f);

extern gn_error getwapbookmark(char *number, gn_data *data, struct gn_statemachine *state);
extern void writewapbookmark_usage(FILE *f, int exitval);
extern gn_error writewapbookmark(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error deletewapbookmark(char *number, gn_data *data, struct gn_statemachine *state);
extern void getwapsetting_usage(FILE *f, int exitval);
extern gn_error getwapsetting(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error writewapsetting(gn_data *data, struct gn_statemachine *state);
extern gn_error activatewapsetting(char *number, gn_data *data, struct gn_statemachine *state);

/* Logo options */
extern void logo_usage(FILE *f);

extern gn_error sendlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error setlogo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error viewlogo(char *filename);

/* Ringtone options */
extern void ringtone_usage(FILE *f);

extern char *get_ringtone_name(int id, gn_data *data, struct gn_statemachine *state);
extern void getringtone_usage(FILE *f, int exitval);
extern gn_error sendringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void setringtone_usage(FILE *f, int exitval);
extern gn_error setringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void playringtone_usage(FILE *f, int exitval);
extern gn_error playringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void ringtoneconvert_usage(FILE *f, int exitval);
extern gn_error ringtoneconvert(int argc, char *argv[]);
extern gn_error getringtonelist(gn_data *data, struct gn_statemachine *state);
extern void deleteringtone_usage(FILE *f, int exitval);
extern gn_error deleteringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Security options */
extern void security_usage(FILE *f);

extern gn_error identify(struct gn_statemachine *state);
extern gn_error getlocksinfo(gn_data *data, struct gn_statemachine *state);
extern gn_error getsecuritycode(gn_data *data, struct gn_statemachine *state);
#ifdef SECURITY
extern void entersecuritycode_usage(FILE *f, int exitval);
extern gn_error entersecuritycode(char *type, gn_data *data, struct gn_statemachine *state);
extern gn_error getsecuritycodestatus(gn_data *data, struct gn_statemachine *state);
extern void changesecuritycode_usage(FILE *f, int exitval);
extern gn_error changesecuritycode(char *type, gn_data *data, struct gn_statemachine *state);
#endif

/* File options */
extern void file_usage(FILE *f);

extern gn_error getfilelist(char *path, gn_data *data, struct gn_statemachine *state);
extern gn_error getfiledetailsbyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getfileid(char *filename, gn_data *data, struct gn_statemachine *state);
extern gn_error deletefile(char *filename, gn_data *data, struct gn_statemachine *state);
extern gn_error deletefilebyid(char *id, gn_data *data, struct gn_statemachine *state);
extern void getfile_usage(FILE *f, int exitval);
extern gn_error getfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern void getfilebyid_usage(FILE *f, int exitval);
extern gn_error getfilebyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getallfiles(char *path, gn_data *data, struct gn_statemachine *state);
extern void putfile_usage(FILE *f, int exitval);
extern gn_error putfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Misc options */
extern void other_usage(FILE *f);

extern gn_error pmon(gn_data *data, struct gn_statemachine *state);
extern gn_error presskeysequence(gn_data *data, struct gn_statemachine *state);
extern gn_error enterchar(gn_data *data, struct gn_statemachine *state);
extern void list_gsm_networks(void);
extern gn_error getnetworkinfo(gn_data *data, struct gn_statemachine *state);
extern int gnokii_atoi(char *string);

/* Compatibility functions */
#ifndef HAVE_GETLINE
#  ifdef HAVE_SYS_PARAM_H
#    include <sys/param.h>
#  endif
int getline(char **line, size_t *len, FILE *stream);
#endif

#endif /* __gnokii_app_h_ */
