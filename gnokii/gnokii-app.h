/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for test utility.

  Last modification: Mon Mar 20 21:51:59 CET 2000
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
int sendlogo(int argc, char *argv[]);
int sendringtone(int argc, char *argv[]);
int getsmsc(char *mcn);
int setdatetime(int argc, char *argv[]);
int getdatetime(void);
int setalarm(char *argv[]);
int getalarm(void);
int dialvoice(char *number);
int dialdata(char *number);
int getcalendarnote(int argc, char *argv[]);
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
int getprofile(int argc, char *argv[]);
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
  OPT_SENDLOGO,
  OPT_SENDRINGTONE,
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
  OPT_GETPROFILE,
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

/* Constants for Profiles. */

#define PROFILE_OFF  0x00
#define PROFILE_ON   0x01

#define PROFILE_MESSAGE_NOTONE     0x00
#define PROFILE_MESSAGE_STANDARD   0x01
#define PROFILE_MESSAGE_SPECIAL    0x02
#define PROFILE_MESSAGE_BEEPONCE   0x03
#define PROFILE_MESSAGE_ASCENDING  0x04

#define PROFILE_WARNING_OFF  0xff
#define PROFILE_WARNING_ON   0x04

/* FIXME: Are these values correct or not? */

#define PROFILE_VIBRATION_OFF  0x00
#define PROFILE_VIBRATION_ON   0x01

#define PROFILE_CALLALERT_RINGING       0x01
#define PROFILE_CALLALERT_BEEPONCE      0x02
#define PROFILE_CALLALERT_OFF           0x04
#define PROFILE_CALLALERT_RINGONCE      0x05
#define PROFILE_CALLALERT_ASCENDING     0x06
#define PROFILE_CALLALERT_CALLERGROUPS  0x07

#define PROFILE_KEYPAD_OFF     0xff
#define PROFILE_KEYPAD_LEVEL1  0x00
#define PROFILE_KEYPAD_LEVEL2  0x01
#define PROFILE_KEYPAD_LEVEL3  0x02
//in 5110 I had also once 0x03

#define PROFILE_VOLUME_LEVEL1  0x06
#define PROFILE_VOLUME_LEVEL2  0x07
#define PROFILE_VOLUME_LEVEL3  0x08
#define PROFILE_VOLUME_LEVEL4  0x09
#define PROFILE_VOLUME_LEVEL5  0x0a

#define RINGTONE_NOTSET       0x01
#define RINGTONE_PRESET       0x10
#define RINGTONE_UPLOAD       0x11
#define RINGTONE_RINGRING     0x12
#define RINGTONE_LOW          0x13
#define RINGTONE_FLY          0x14
#define RINGTONE_MOSQUITO     0x15
#define RINGTONE_BEE          0x16
#define RINGTONE_INTRO        0x17
#define RINGTONE_ETUDE        0x18
#define RINGTONE_HUNT         0x19
#define RINGTONE_GOINGUP      0x1a
#define RINGTONE_CITYBIRD     0x1b
#define RINGTONE_GRANDEVALSE  0x2f
#define RINGTONE_ATTRACTION   0x3a
#define RINGTONE_SAMBA        0x44
