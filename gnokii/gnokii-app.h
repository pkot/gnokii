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

extern int shell(gn_data *data, struct gn_statemachine *state);

/* Utils functions */
int writefile(char *filename, char *text, int mode);
int writebuffer(const char *filename, const char *buffer, size_t nitems, int mode);
extern gn_error readtext(gn_sms_user_data *udata); 
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

extern int sendsms_usage(FILE *f, int exitval);
extern gn_error sendsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int savesms_usage(FILE *f, int exitval);
extern gn_error savesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getsms_usage(FILE *f, int exitval);
extern gn_error getsms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletesms_usage(FILE *f, int exitval);
extern gn_error deletesms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

extern int getsmsc_usage(FILE *f, int exitval);
extern gn_error getsmsc(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int setsmsc_usage(FILE *f, int exitval);
extern gn_error setsmsc(gn_data *data, struct gn_statemachine *state);

extern int createsmsfolder_usage(FILE *f, int exitval);
extern gn_error createsmsfolder(char *name, gn_data *data, struct gn_statemachine *state);
extern int deletesmsfolder_usage(FILE *f, int exitval);
extern gn_error deletesmsfolder(char *number, gn_data *data, struct gn_statemachine *state);
extern int showsmsfolderstatus_usage(FILE *f, int exitval);
extern gn_error showsmsfolderstatus(gn_data *data, struct gn_statemachine *state);

gn_error smsreader(gn_data *data, struct gn_statemachine *state);

/* MMS functions */
extern void mms_usage(FILE *f);

extern int getmms_usage(FILE *f, int exitval);
extern gn_error getmms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletemms_usage(FILE *f, int exitval);
extern gn_error deletemms(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Phonebook functions */
extern void phonebook_usage(FILE *f);

extern int getphonebook_usage(FILE *f, int exitval);
extern gn_error getphonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int writephonebook_usage(FILE *f, int exitval);
extern gn_error writephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletephonebook_usage(FILE *f, int exitval);
extern gn_error deletephonebook(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Calendar functions */
extern void calendar_usage(FILE *f);

extern int getcalendarnote_usage(FILE *f, int exitval);
extern gn_error getcalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int writecalendarnote_usage(FILE *f, int exitval);
extern gn_error writecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int deletecalendarnote_usage(FILE *f, int exitval);
extern gn_error deletecalendarnote(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* ToDo functions */
extern void todo_usage(FILE *f);

extern int gettodo_usage(FILE *f, int exitval);
extern gn_error gettodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int writetodo_usage(FILE *f, int exitval);
extern gn_error writetodo(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error deletealltodos(gn_data *data, struct gn_statemachine *state);

/* Dialling and call handling functions */
extern void dial_usage(FILE *f);

extern gn_error getspeeddial(char *number, gn_data *data, struct gn_statemachine *state);
extern gn_error setspeeddial(char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error dialvoice(char *number, gn_data *data, struct gn_statemachine *state);
extern gn_error senddtmf(char *string, gn_data *data, struct gn_statemachine *state);
extern int answercall_usage(FILE *f, int exitval);
extern gn_error answercall(char *callid, gn_data *data, struct gn_statemachine *state);
extern gn_error hangup(char *callid, gn_data *data, struct gn_statemachine *state);
extern int divert_usage(FILE *f, int exitval);
extern gn_error divert(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Profile options */
extern void profile_usage(FILE *f);

extern int getprofile_usage(FILE *f, int exitval);
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
extern int writewapbookmark_usage(FILE *f, int exitval);
extern gn_error writewapbookmark(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error deletewapbookmark(char *number, gn_data *data, struct gn_statemachine *state);
extern int getwapsetting_usage(FILE *f, int exitval);
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
extern int getringtone_usage(FILE *f, int exitval);
extern gn_error sendringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int setringtone_usage(FILE *f, int exitval);
extern gn_error setringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int playringtone_usage(FILE *f, int exitval);
extern gn_error playringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int ringtoneconvert_usage(FILE *f, int exitval);
extern gn_error ringtoneconvert(int argc, char *argv[]);
extern gn_error getringtonelist(gn_data *data, struct gn_statemachine *state);
extern int deleteringtone_usage(FILE *f, int exitval);
extern gn_error deleteringtone(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Security options */
extern void security_usage(FILE *f);

extern gn_error identify(struct gn_statemachine *state);
extern gn_error getlocksinfo(gn_data *data, struct gn_statemachine *state);
extern gn_error getsecuritycode(gn_data *data, struct gn_statemachine *state);
#ifdef SECURITY
extern int entersecuritycode_usage(FILE *f, int exitval);
extern gn_error entersecuritycode(char *type, gn_data *data, struct gn_statemachine *state);
extern gn_error getsecuritycodestatus(gn_data *data, struct gn_statemachine *state);
extern int changesecuritycode_usage(FILE *f, int exitval);
extern gn_error changesecuritycode(char *type, gn_data *data, struct gn_statemachine *state);
#endif

/* File options */
extern void file_usage(FILE *f);

extern gn_error getfilelist(char *path, gn_data *data, struct gn_statemachine *state);
extern gn_error getfiledetailsbyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getfileid(char *filename, gn_data *data, struct gn_statemachine *state);
extern gn_error deletefile(char *filename, gn_data *data, struct gn_statemachine *state);
extern gn_error deletefilebyid(char *id, gn_data *data, struct gn_statemachine *state);
extern int getfile_usage(FILE *f, int exitval);
extern gn_error getfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern int getfilebyid_usage(FILE *f, int exitval);
extern gn_error getfilebyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);
extern gn_error getallfiles(char *path, gn_data *data, struct gn_statemachine *state);
extern int putfile_usage(FILE *f, int exitval);
extern gn_error putfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state);

/* Misc options */
extern void other_usage(FILE *f);

extern gn_error pmon(gn_data *data, struct gn_statemachine *state);
extern gn_error presskeysequence(gn_data *data, struct gn_statemachine *state);
extern gn_error enterchar(gn_data *data, struct gn_statemachine *state);
extern void list_gsm_networks(void);
extern gn_error getnetworkinfo(gn_data *data, struct gn_statemachine *state);
extern int gnokii_atoi(char *string);

#endif /* __gnokii_app_h_ */
