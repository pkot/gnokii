/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for test utility.	

  Last modification: Wed Nov  3 21:47:35 CET 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

/* Prototypes */

int monitormode(void);

#ifdef SECURITY
  int entersecuritycode(char *type);
  int getsecuritycodestatus(void);
#endif

int getmemory(char *argv[]);
int writephonebook(void);
int getspeeddial(char *number);
int setspeeddial(char *argv[]);
int getsms(char *argv[]);
int deletesms(char *argv[]);
int sendsms(int argc, char *argv[]);
int sendoplogoviasms(int argc, char *argv[]);
int getsmsc(char *mcn);
int setdatetime(int argc, char *argv[]);
int getdatetime(void);
int setalarm(char *argv[]);
int getalarm(void);
int dialvoice(char *number);
int dialdata(char *number);
int getcalendarnote(char *index);
int writecalendarnote(char *argv[]);
int deletecalendarnote(char *index);
int getdisplaystatus();
int netmonitor(char *_mode);
int identify(void);
int senddtmf(char *String);
int foogle(char *argv[]);
int pmon(void);
int setlogo(char *argv[]);
int getlogo(char *argv[]);
int reset(char *type);
void readconfig(void);

typedef enum {
  OPT_HELP,
  OPT_VERSION,
  OPT_MONITOR,
  OPT_ENTERSECURITYCODE,
  OPT_GETSECURITYCODESTATUS,
  OPT_SETDATETIME,
  OPT_GETDATETIME,
  OPT_SETALARM,
  OPT_GETALARM,
  OPT_DIALVOICE,
  OPT_DIALDATA, 
  OPT_GETCALENDARNOTE,
  OPT_WRITECALENDARNOTE,
  OPT_DELCALENDARNOTE,
  OPT_GETDISPLAYSTATUS,
  OPT_GETMEMORY,
  OPT_WRITEPHONEBOOK,
  OPT_GETSPEEDDIAL,
  OPT_SETSPEEDDIAL,
  OPT_GETSMS,
  OPT_DELETESMS,
  OPT_SENDSMS,
  OPT_SENDOPLOGOVIASMS,
  OPT_GETSMSC,
  OPT_GETWELCOMENOTE,
  OPT_SETWELCOMENOTE,
  OPT_PMON,
  OPT_NETMONITOR,
  OPT_IDENTIFY,
  OPT_SENDDTMF,
  OPT_RESET,
  OPT_SETLOGO,
  OPT_GETLOGO,
  OPT_FOOGLE
} opt_index;

struct gnokii_arg_len {
  int gal_opt;
  int gal_min;
  int gal_max;
  int gal_flags;
};

/* This is used for checking correct argument count. If it is used then if
   the user specifies some argument, their count should be equivalent to the
   count the programmer expects. */

#define GAL_XOR 0x01
