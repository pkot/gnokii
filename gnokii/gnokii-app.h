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

int getmemory(int argc, char *argv[]);
int writephonebook(int argc, char *argv[]);
int getspeeddial(char *number);
int setspeeddial(char *argv[]);
int getsms(int argc, char *argv[]);
int deletesms(int argc, char *argv[]);
int sendsms(int argc, char *argv[]);
int sendlogo(int argc, char *argv[]);
int sendringtone(int argc, char *argv[]);
int getsmsc(char *mcn);
int setdatetime(int argc, char *argv[]);
int getdatetime(void);
int setalarm(char *argv[]);
int getalarm(void);
int dialvoice(char *number);
int getcalendarnote(int argc, char *argv[]);
int writecalendarnote(char *argv[]);
int deletecalendarnote(char *index);
int getdisplaystatus();
int netmonitor(char *_mode);
int identify(void);
int senddtmf(char *String);
int foogle(char *argv[]);
int pmon(void);
int setlogo(int argc, char *argv[]);
int getlogo(char *argv[]);
int setringtone(int argc, char *argv[]);
int reset(char *type);
int getprofile(int argc, char *argv[]);
int displayoutput();
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
  OPT_SETRINGTONE,
  OPT_GETPROFILE,
  OPT_DISPLAYOUTPUT,
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

/* Nokia ringtones codes. */

char *RingingTones[] = {
/*  0 */ "Unknown",
/*  1 */ "Unknown",                 /* FIXME: probably not set. */
/*  2 */ "Unknown",
/*  3 */ "Unknown",
/*  4 */ "Unknown",
/*  5 */ "Unknown",
/*  6 */ "Unknown",
/*  7 */ "Unknown",
/*  8 */ "Unknown",
/*  9 */ "Unknown",
/* 10 */ "Unknown",                 /* FIXME: probably pre set. */
/* 11 */ "Unknown",
/* 12 */ "Unknown",
/* 13 */ "Unknown",
/* 14 */ "Unknown",
/* 15 */ "Unknown",
/* 16 */ "Unknown",
/* 17 */ "Uploaded",
/* 18 */ "Ring ring",
/* 19 */ "Low",
/* 20 */ "Fly",
/* 21 */ "Mosquito",
/* 22 */ "Bee",
/* 23 */ "Intro",
/* 24 */ "Etude",
/* 25 */ "Hunt",
/* 26 */ "Going up",
/* 27 */ "City Bird",
/* 28 */ "Unknown",
/* 29 */ "Unknown",
/* 30 */ "Chase",
/* 31 */ "Unknown",
/* 32 */ "Scifi",
/* 33 */ "Unknown",
/* 34 */ "Kick",
/* 35 */ "Do-mi-so",
/* 36 */ "Robo N1X",
/* 37 */ "Dizzy",
/* 38 */ "Unknown",
/* 39 */ "Playground",
/* 40 */ "Unknown",
/* 41 */ "Unknown",
/* 42 */ "Unknown",
/* 43 */ "That's it!",
/* 44 */ "Unknown",
/* 45 */ "Unknown",
/* 46 */ "Unknown",
/* 47 */ "Grande valse",   /* FIXME: Knock knock (Knock again). */
/* 48 */ "Helan",          /* FIXME: Grand valse on 5110. */
/* 49 */ "Fuga",           /* FIXME: Helan on 5110. */
/* 50 */ "Menuet",         /* FIXME: Fuga on 5110. */
/* 51 */ "Ode to Joy",
/* 52 */ "Elise",
/* 53 */ "Mozart 40",
/* 54 */ "Piano Concerto", /* FIXME: Mozart 40 on 5110. */
/* 55 */ "William Tell",
/* 56 */ "Badinerie",      /* FIXME: William Tell on 5110. */
/* 57 */ "Polka",          /* FIXME: Badinerie on 5110. */
/* 58 */ "Attraction",     /* FIXME: Polka on 5110. */
/* 59 */ "Unknown",        /* FIXME: Attraction on 5110. */
/* 60 */ "Polite",         /* FIXME: Down on 5110. */
/* 61 */ "Persuasion",
/* 62 */ "Unknown",        /* FIXME: Persuasion on 5110. */
/* 63 */ "Unknown",
/* 64 */ "Unknown",
/* 65 */ "Unknown",
/* 66 */ "Unknown",
/* 67 */ "Tick tick",
/* 68 */ "Samba",
/* 69 */ "Unknown",        /* FIXME: Samba on 5110. */
/* 70 */ "Orient",
/* 71 */ "Charleston",     /* FIXME: Orient on 5110. */
/* 72 */ "Unknown",        /* FIXME: Charleston on 5110. */
/* 73 */ "Jumping",        /* FIXME: Songette on 5110. */
/* 74 */ "Unknown",        /* FIXME: Jumping on 5110. */
/* 75 */ "Unknown",        /* FIXME: Lamb (Marry) on 5110. */
/* 76 */ "Unknown",
/* 77 */ "Unknown",
/* 78 */ "Unknown",
/* 79 */ "Unknown",
/* 80 */ "Unknown"         /* FIXME: Tango (Tangoed) on 5110. */
};
