/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries and other stuff.

  WARNING: this code is only the test tool. It is not intented to real work -
  wait for GUI application. Well, our test tool is now really powerful and
  useful :-)

  Last modification: Mon Mar 20 21:40:04 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#ifdef WIN32

  #include <windows.h>
  #define sleep(x) Sleep((x) * 1000)
  #define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
  #include "win32/getopt.h"

#else

  #include <unistd.h>
  #include <termios.h>
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/time.h>
  #include <getopt.h>

#endif

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-networks.h"
#include "cfgreader.h"
#include "gnokii.h"
#include "gsm-filetypes.h"

char *model;      /* Model from .gnokiirc file. */
char *Port;       /* Serial port from .gnokiirc file */
char *Initlength; /* Init length from .gnokiirc file */
char *Connection; /* Connection type from .gnokiirc file */

/* Local variables */

char *DefaultModel = MODEL; /* From Makefile */
char *DefaultPort = PORT;
char *DefaultConnection = "serial";

char *GetProfileCallAlertString(int code) {

  switch (code) {
    case PROFILE_CALLALERT_RINGING:
      return "Ringing";
    case PROFILE_CALLALERT_ASCENDING:
      return "Ascending";
    case PROFILE_CALLALERT_RINGONCE:
      return "Ring once";
    case PROFILE_CALLALERT_BEEPONCE:
      return "Beep once";
    case PROFILE_CALLALERT_CALLERGROUPS:
      return "Caller groups";
    case PROFILE_CALLALERT_OFF:
      return "Off";
    default:
      return "Unknown";
  }
}

char *GetProfileVolumeString(int code) {

  switch (code) {
    case PROFILE_VOLUME_LEVEL1:
      return "Level 1";
    case PROFILE_VOLUME_LEVEL2:
      return "Level 2";
    case PROFILE_VOLUME_LEVEL3:
      return "Level 3";
    case PROFILE_VOLUME_LEVEL4:
      return "Level 4";
    case PROFILE_VOLUME_LEVEL5:
      return "Level 5";
    default:
      return "Unknown";
  }
}

char *GetProfileKeypadToneString(int code) {

  switch (code) {
    case PROFILE_KEYPAD_OFF:
      return "Off";
    case PROFILE_KEYPAD_LEVEL1:
      return "Level 1";
    case PROFILE_KEYPAD_LEVEL2:
      return "Level 2";
    case PROFILE_KEYPAD_LEVEL3:
      return "Level 3";
    default:
      return "Unknown";
  }
}

char *GetProfileMessageToneString(int code) {

  switch (code) {
    case PROFILE_MESSAGE_NOTONE:
      return "No tone";
    case PROFILE_MESSAGE_STANDARD:
      return "Standard";
    case PROFILE_MESSAGE_SPECIAL:
      return "Special";
    case PROFILE_MESSAGE_BEEPONCE:
      return "Beep once";
    case PROFILE_MESSAGE_ASCENDING:
      return "Ascending";
    default:
      return "Unknown";
  }
}

char *GetProfileWarningToneString(int code) {

  switch (code) {
    case PROFILE_WARNING_OFF:
      return "Off";
    case PROFILE_WARNING_ON:
      return "On";
    default:
      return "Unknown";
  }
}

char *GetProfileVibrationString(int code) {

  switch (code) {
    case PROFILE_VIBRATION_OFF:
      return "Off";
    case PROFILE_VIBRATION_ON:
      return "On";
    default:
      return "Unknown";
  }
}

/* FIXME: correct only for N6110, fill in other strings and change it into
   the array of structures for easy grepping through. */
char *GetRingtoneName(int code) {

  switch (code) {
    case RINGTONE_NOTSET:
      return "Not set";
    case RINGTONE_PRESET:
      return "Preset";
    case RINGTONE_UPLOAD:
      return "Uploaded tone";
    case RINGTONE_RINGRING:
      return "Ring ring";
    case RINGTONE_LOW:
      return "Low";
    case RINGTONE_FLY:
      return "Fly";
    case RINGTONE_MOSQUITO:
      return "Mosquito";
    case RINGTONE_BEE:
      return "Bee";
    case RINGTONE_INTRO:
      return "Intro";
    case RINGTONE_ETUDE:
      return "Etude";
    case RINGTONE_HUNT:
      return "Hunt";
    case RINGTONE_GOINGUP:
      return "Going up";
    case RINGTONE_CITYBIRD:
      return "City bird";
    case RINGTONE_GRANDEVALSE:
      return "Grande valse";
    case RINGTONE_ATTRACTION:
      return "Attraction";
    case RINGTONE_SAMBA:
      return "Samba";
    default:
      return "Unknown";
  }
}


/* This function shows the copyright and some informations usefull for
   debugging. */

int version(void)
{

  fprintf(stdout, _("GNOKII Version %s\n"
"Copyright (C) Hugh Blemings <hugh@linuxcare.com>, 1999\n"
"Copyright (C) Pavel Janík ml. <Pavel.Janik@linux.cz>, 1999\n"
"Built %s %s for %s on %s \n"), VERSION, __TIME__, __DATE__, model, Port);

  return 0;
}

/* The function usage is only informative - it prints this program's usage and
   command-line options. */

int usage(void)
{

  fprintf(stdout, _("   usage: gnokii [--help|--monitor|--version]\n"
"          gnokii --getmemory memory_type start end\n"
"          gnokii --writephonebook\n"
"          gnokii --getspeeddial number\n"
"          gnokii --setspeeddial number memory_type location\n"
"          gnokii --getsms memory_type start end\n"
"          gnokii --deletesms memory_type start end\n"
"          gnokii --sendsms destination [--smsc message_center_number |\n"
"                 --smscno message_center_index] [-r] [-C n] [-v n]\n"
"                 [--long n]\n"
"          gnokii --getsmsc message_center_number\n"
"          gnokii --setdatetime [YYYY [MM [DD [HH [MM]]]]]\n"
"          gnokii --getdatetime\n"
"          gnokii --setalarm HH MM\n"
"          gnokii --getalarm\n"
"          gnokii --dialvoice number\n"
"          gnokii --dialdata number\n"
"          gnokii --getcalendarnote index [-v]\n"
"          gnokii --writecalendarnote\n"
"          gnokii --deletecalendarnote index\n"
"          gnokii --getdisplaystatus\n"
"          gnokii --netmonitor {reset|off|field|devel|next|nr}\n"
"          gnokii --identify\n"
"          gnokii --senddtmf string\n"
"          gnokii --sendlogo {caller|op} destionation logofile [network code]\n"
"          gnokii --setlogo logofile [network code]\n"
"          gnokii --setlogo logofile [caller group number] [group name]\n"
"          gnokii --setlogo text [startup text]\n"
"          gnokii --setlogo dealer [dealer startup text]\n"
"          gnokii --getlogo logofile {caller|op|startup} [caller group number]\n"
"          gnokii --sendringtone destionation rtttlfile\n"
"          gnokii --reset [soft|hard]\n"
"          gnokii --getprofile [number]\n"
  ));
#ifdef SECURITY
  fprintf(stdout, _(
"          gnokii --entersecuritycode PIN|PIN2|PUK|PUK2\n"
"          gnokii --getsecuritycodestatus\n"
  ));
#endif
  fprintf(stdout, _(
"\n"
"          --help            display usage information.\n\n"

"          --monitor         continually updates phone status to stderr.\n\n"

"          --version         displays version and copyright information.\n\n"

"          --getmemory       reads specificed memory location from phone.\n"
"                            Valid memory types are:\n"
"                            ME, SM, FD, ON, EN, DC, RC, MC, LD\n\n"

"          --writephonebook  reads data from stdin and writes to phonebook.\n"
"                            Uses the same format as provided by the output of\n"
"                            the getphonebook command.\n\n"

"          --getspeeddial    reads speed dial from the specified location.\n\n"

"          --setspeeddial    specify speed dial.\n\n"

"          --getsms          gets SMS messages from specified memory type\n"
"                            starting at entry [start] and ending at [end].\n"
"                            Entries are dumped to stdout.\n\n"

"          --deletesms       deletes SMS messages from specified memory type\n"
"                            starting at entry [start] and ending at [end].\n\n"

"          --sendsms         sends an SMS message to [destination] via\n"
"                            [message_center_number] or SMSC number taken from\n"
"                            phone memory from address [message_center_index].\n"
"                            If this argument is ommited SMSC number is taken\n"
"                            from phone memory from location 1. Message text\n"
"                            is taken from stdin.  This function has had\n"
"                            limited testing and may not work at all on your\n"
"                            network. Meaning of other optional parameters:\n"
"                             [-r] - request for delivery report\n"
"                             [-C n] - Class Message n, where n can be 0..3\n"
"                             [-v n] - validity in minutes\n"
"                             [--long n] - send no more then n characters,\n"
"                                          default is 160\n\n"

"          --getsmsc         show the SMSC number from location\n"
"                            [message_center_number].\n\n"

"          --setdatetime     set the date and the time of the phone.\n\n"

"          --getdatetime     shows current date and time in the phone.\n\n"

"          --setalarm        set the alarm of the phone.\n\n"

"          --getalarm        shows current alarm.\n\n"

"          --dialvoice       initiate voice call.\n\n"

"          --dialdata        initiate data call.\n\n"

"          --getcalendarnote get the note with number index from calendar.\n"
"                             [-v] - output in vCalendar 1.0 format\n\n"

"          --writecalendarnote write the note to calendar.\n\n"

"          --deletecalendarnote  delete the note with number [index]\n"
"                                from calendar.\n\n"

"          --getdisplaystatus shows what icons are displayed.\n\n"

"          --netmonitor      setting/querying netmonitor mode.\n\n"

"          --identify        get IMEI, model and revision\n\n"

"          --senddtmf        send DTMF sequence\n\n"

"          --sendlogo        send the logofile to destination as operator\n"
"                            or CLI logo\n\n"

"          --setlogo         set caller, startup or operator logo\n\n"

"          --getlogo         get caller, startup or operator logo\n\n"

"          --sendringtone    send the rtttlfile to destination as ringtone\n\n"

"          --reset [soft|hard] resets the phone.\n\n"

"          --getprofile [number] show settings for selected(all) profile(s)\n\n"
  ));
#ifdef SECURITY
  fprintf(stdout, _(
"          --entersecuritycode asks for the code and sends it to the phone\n\n"
"          --getsecuritycodestatus show if a security code is needed\n\n"
  ));
#endif

  return 0;
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

void fbusinit(void (*rlp_handler)(RLP_F96Frame *frame))
{

  int count=0;
  GSM_Error error;
  GSM_ConnectionType connection=GCT_Serial;

  if (!strcmp(Connection, "infrared")) {
    connection=GCT_Infrared;
  }

  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise(model, Port, Initlength, connection, rlp_handler);

  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    exit(-1);
  }

  /* First (and important!) wait for GSM link to be active. We allow 10
     seconds... */

  while (count++ < 200 && *GSM_LinkOK == false)
    usleep(50000);

  if (*GSM_LinkOK == false) {
    fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting.\n"));
    exit(-1);
  }
}

/* This function checks that the argument count for a given options is withing
   an allowed range. */

int checkargs(int opt, struct gnokii_arg_len gals[], int argc)
{

  int i;

  /* Walk through the whole array with options requiring arguments. */

  for(i = 0;!(gals[i].gal_min == 0 && gals[i].gal_max == 0); i++) {

    /* Current option. */

    if(gals[i].gal_opt == opt) {

      /* Argument count checking. */

      if(gals[i].gal_flags == GAL_XOR) {
	if(gals[i].gal_min == argc || gals[i].gal_max == argc)
	  return 0;
      }
      else {
	if(gals[i].gal_min <= argc && gals[i].gal_max >= argc)
	  return 0;
      }

      return 1;

    }

  }

  /* We do not have options without arguments in the array, so check them. */

  if (argc==0)
    return 0;
  else
    return 1;
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{

  int c, i, rc = -1;
  int nargc = argc-2;
  char **nargv;

  /* Every option should be in this array. */

  static struct option long_options[] =
  {

    // Display usage.
    { "help",               no_argument,       NULL, OPT_HELP },

    // Display version and build information.
    { "version",            no_argument,       NULL, OPT_VERSION },

    // Monitor mode
    { "monitor",            no_argument,       NULL, OPT_MONITOR },

#ifdef SECURITY

    // Enter Security Code mode
    { "entersecuritycode",  required_argument, NULL, OPT_ENTERSECURITYCODE },

    // Get Security Code status
    { "getsecuritycodestatus",  no_argument,   NULL, OPT_GETSECURITYCODESTATUS },

#endif

    // Set date and time
    { "setdatetime",        optional_argument, NULL, OPT_SETDATETIME },

    // Get date and time mode
    { "getdatetime",        no_argument,       NULL, OPT_GETDATETIME },

    // Set alarm
    { "setalarm",           required_argument, NULL, OPT_SETALARM },

    // Get alarm
    { "getalarm",           no_argument,       NULL, OPT_GETALARM },

    // Voice call mode
    { "dialvoice",          required_argument, NULL, OPT_DIALVOICE },

    // Data call mode
    { "dialdata",           required_argument, NULL, OPT_DIALDATA },

    // Get calendar note mode
    { "getcalendarnote",    required_argument, NULL, OPT_GETCALENDARNOTE },

    // Write calendar note mode
    { "writecalendarnote",  no_argument,       NULL, OPT_WRITECALENDARNOTE },

    // Delete calendar note mode
    { "deletecalendarnote", required_argument, NULL, OPT_DELCALENDARNOTE },

    // Get display status mode
    { "getdisplaystatus",   no_argument,       NULL, OPT_GETDISPLAYSTATUS },

    // Get memory mode
    { "getmemory",          required_argument, NULL, OPT_GETMEMORY },

    // Write phonebook (memory) mode
    { "writephonebook",     no_argument,       NULL, OPT_WRITEPHONEBOOK },

    // Get speed dial mode
    { "getspeeddial",       required_argument, NULL, OPT_GETSPEEDDIAL },

    // Set speed dial mode
    { "setspeeddial",       required_argument, NULL, OPT_SETSPEEDDIAL },

    // Get SMS message mode
    { "getsms",             required_argument, NULL, OPT_GETSMS },

    // Delete SMS message mode
    { "deletesms",          required_argument, NULL, OPT_DELETESMS },

    // Send SMS message mode
    { "sendsms",            required_argument, NULL, OPT_SENDSMS },

    // Send logo as SMS message mode
    { "sendlogo",           required_argument, NULL, OPT_SENDLOGO },

    // Send ringtone as SMS message
    { "sendringtone",       required_argument, NULL, OPT_SENDRINGTONE },

    // Get SMS center number mode
    { "getsmsc",            required_argument, NULL, OPT_GETSMSC },

    // For development purposes: run in passive monitoring mode
    { "pmon",               no_argument,       NULL, OPT_PMON },

    // NetMonitor mode
    { "netmonitor",         required_argument, NULL, OPT_NETMONITOR },

    // Identify
    { "identify",           no_argument,       NULL, OPT_IDENTIFY },

    // Send DTMF sequence
    { "senddtmf",           required_argument, NULL, OPT_SENDDTMF },

    // Resets the phone
    { "reset",              optional_argument, NULL, OPT_RESET },

    // Set logo
    { "setlogo",            optional_argument, NULL, OPT_SETLOGO },

    // Get logo
    { "getlogo",            required_argument, NULL, OPT_GETLOGO },

    // Show profile
    { "getprofile",        optional_argument, NULL, OPT_GETPROFILE },

#ifndef WIN32
    // For development purposes: insert you function calls here
    { "foogle",             no_argument, NULL, OPT_FOOGLE },
#endif

    { 0, 0, 0, 0},
  };

  /* Every command which requires arguments should have an appropriate entry
     in this array. */
	
  struct gnokii_arg_len gals[] =
  {

#ifdef SECURITY
    { OPT_ENTERSECURITYCODE, 1, 1, 0 },
#endif

    { OPT_SETDATETIME,       0, 5, 0 },
    { OPT_SETALARM,          2, 2, 0 },
    { OPT_DIALVOICE,         1, 1, 0 },
    { OPT_DIALDATA,          1, 1, 0 },
    { OPT_GETCALENDARNOTE,   1, 2, 0 },
    { OPT_DELCALENDARNOTE,   1, 1, 0 },
    { OPT_GETMEMORY,         3, 3, 0 },
    { OPT_GETSPEEDDIAL,      1, 1, 0 },
    { OPT_SETSPEEDDIAL,      3, 3, 0 },
    { OPT_GETSMS,            3, 3, 0 },
    { OPT_DELETESMS,         3, 3, 0 },
    { OPT_SENDSMS,           1, 10, 0 },
    { OPT_SENDLOGO,          3, 4, GAL_XOR },
    { OPT_SENDRINGTONE,      2, 2, 0 },
    { OPT_GETSMSC,           1, 1, 0 },
    { OPT_GETWELCOMENOTE,    1, 1, 0 },
    { OPT_SETWELCOMENOTE,    1, 1, 0 },
    { OPT_NETMONITOR,        1, 1, 0 },
    { OPT_SENDDTMF,          1, 1, 0 },
    { OPT_SETLOGO,           1, 3, 0 },
    { OPT_GETLOGO,           2, 3, 0 },
    { OPT_RESET,             0, 1, 0 },
    { OPT_GETPROFILE,        0, 1, 0 },

    { 0, 0, 0, 0 },
  };

  opterr = 0;

  /* For GNU gettext */

#ifdef USE_NLS
  textdomain("gnokii");
#endif

  /* Read config file */
  readconfig();

  /* Handle command line arguments. */

  c = getopt_long(argc, argv, "", long_options, NULL);

  if (c == -1) {

    /* No argument given - we should display usage. */
    usage();
    exit(-1);
  }

  /* We have to build an array of the arguments which will be passed to the
     functions.  Please note that every text after the --command will be
     passed as arguments.  A syntax like gnokii --cmd1 args --cmd2 args will
     not work as expected; instead args --cmd2 args is passed as a
     parameter. */

  if((nargv = malloc(sizeof(char *) * argc)) != NULL) {

    for(i = 2; i < argc; i++)
      nargv[i-2] = argv[i];
	
    if(checkargs(c, gals, nargc)) {

      free(nargv);

      /* Wrong number of arguments - we should display usage. */
      usage();
      exit(-1);
    }

    switch(c) {

      // First, error conditions
	
    case '?':
    case ':':

      fprintf(stderr, _("Use '%s --help' for usage informations.\n"), argv[0]);
      break;
	
      // Then, options with no arguments

    case OPT_HELP:

      rc = usage();
      break;

    case OPT_VERSION:

      rc = version();
      break;

    case OPT_MONITOR:

      rc = monitormode();
      break;

#ifdef SECURITY
    case OPT_ENTERSECURITYCODE:

      rc = entersecuritycode(optarg);
      break;

    case OPT_GETSECURITYCODESTATUS:

      rc = getsecuritycodestatus();
      break;
#endif
					
    case OPT_GETDATETIME:

      rc = getdatetime();
      break;

    case OPT_GETALARM:

      rc = getalarm();
      break;

    case OPT_GETDISPLAYSTATUS:

      rc = getdisplaystatus();
      break;

    case OPT_PMON:

      rc = pmon();
      break;

    case OPT_WRITEPHONEBOOK:

      rc = writephonebook();
      break;
	
      // Now, options with arguments
	
    case OPT_SETDATETIME:

      rc = setdatetime(nargc, nargv);
      break;

    case OPT_SETALARM:

      rc = setalarm(nargv);
      break;

    case OPT_DIALVOICE:

      rc = dialvoice(optarg);
      break;

    case OPT_DIALDATA:

      rc = dialdata(optarg);
      break;

    case OPT_GETCALENDARNOTE:

      rc = getcalendarnote(nargc, nargv);
      break;

    case OPT_DELCALENDARNOTE:

      rc = deletecalendarnote(optarg);
      break;

    case OPT_WRITECALENDARNOTE:

      rc = writecalendarnote(nargv);
      break;

    case OPT_GETMEMORY:

      rc = getmemory(nargv);
      break;

    case OPT_GETSPEEDDIAL:

      rc = getspeeddial(optarg);
      break;

    case OPT_SETSPEEDDIAL:

      rc = setspeeddial(nargv);
      break;

    case OPT_GETSMS:

      rc = getsms(nargv);
      break;

    case OPT_DELETESMS:

      rc = deletesms(nargv);
      break;

    case OPT_SENDSMS:

      rc = sendsms(nargc, nargv);
      break;

    case OPT_SENDLOGO:

      rc = sendlogo(nargc, nargv);
      break;

    case OPT_SENDRINGTONE:

      rc = sendringtone(nargc, nargv);
      break;

    case OPT_GETSMSC:

      rc = getsmsc(optarg);
      break;

    case OPT_NETMONITOR:

      rc = netmonitor(optarg);
      break;

    case OPT_IDENTIFY:

      rc = identify();
      break;

    case OPT_SETLOGO:

      rc = setlogo(nargv);
      break;

    case OPT_GETLOGO:

      rc = getlogo(nargv);
      break;

    case OPT_GETPROFILE:

      rc = getprofile(nargc, nargv);
      break;

#ifndef WIN32
    case OPT_FOOGLE:

      rc = foogle(nargv);
      break;
#endif

    case OPT_SENDDTMF:

      rc = senddtmf(optarg);
      break;

    case OPT_RESET:

      rc = reset(optarg);
      break;

    default:

      fprintf(stderr, _("Unknown option: %d\n"), c);
      break;

    }

    free(nargv);

    return(rc);
  }

  fprintf(stderr, _("Wrong number of arguments\n"));

  exit(-1);
}

/* Send  SMS messages. */
int sendsms(int argc, char *argv[])
{

  GSM_SMSMessage SMS;
  GSM_Error error;
  char UDH[GSM_MAX_USER_DATA_HEADER_LENGTH];
  /* The maximum length of an uncompressed concatenated short message is
     255 * 153 = 39015 default alphabet characters */
  char message_buffer[GSM_MAX_CONCATENATED_SMS_LENGTH];
  int input_len, chars_read;
  int i, offset, nr_msg, aux;

  struct option options[] = {
             { "smsc",    required_argument, NULL, '1'},
             { "smscno",  required_argument, NULL, '2'},
             { "long",	  required_argument, NULL, '3'},
             { NULL,      0,                 NULL, 0}
  };

  input_len = GSM_MAX_SMS_LENGTH;

  /* Default settings:
      - no delivery report
      - no Class Message
      - no compression
      - 7 bit data
      - SMSC no. 1
      - message validity for 3 days
      - unset user data header indicator
  */

  SMS.Type = GST_MO;
  SMS.Class = -1;
  SMS.Compression = false;
  SMS.EightBit = false;
  SMS.MessageCenter.No = 1;
  SMS.Validity = 4320; /* 4320 minutes == 72 hours */
  SMS.UDHType = GSM_NoUDH;

  strcpy(SMS.Destination,argv[0]);

  optarg = NULL;
  optind = 0;

  while ((i = getopt_long(argc, argv, "r8cC:v:", options, NULL)) != -1) {
    switch (i) {       // -8 is for 8-bit data, -c for compression. both are not yet implemented.

      case '1': /* SMSC number */
        SMS.MessageCenter.No = 0;
        strcpy(SMS.MessageCenter.Number,optarg);
        break;

      case '2': /* SMSC number index in phone memory */
        SMS.MessageCenter.No = atoi(optarg);

	if (SMS.MessageCenter.No < 1 || SMS.MessageCenter.No > 5) {
	  usage();
          return -1;
	}

        break;

      case '3': /* we send long message */
        input_len = atoi(optarg);
        if (input_len > GSM_MAX_CONCATENATED_SMS_LENGTH) {
        	fprintf(stderr, _("Input too long!\n"));	
		exit(-1);
        }

        break;

      case 'r': /* request for delivery report */
        SMS.Type = GST_DR;
        break;

      case 'C': /* class Message */

        switch (*optarg) {

          case '0':
            SMS.Class = 0;
            break;

          case '1':
            SMS.Class = 1;
            break;

          case '2':
            SMS.Class = 2;
            break;

          case '3':
            SMS.Class = 3;
            break;

          default:
            usage();
            return -1;

        }
        break;

      case 'v':
        SMS.Validity = atoi(optarg);
        break;

      default:
        usage(); // Would be better to have an sendsms_usage() here.
        return -1;
    }
  }

  /* Get message text from stdin. */

  chars_read = fread(message_buffer, 1, input_len, stdin);

  if (chars_read == 0) {

    fprintf(stderr, _("Couldn't read from stdin!\n"));	
    return -1;

  } else if (chars_read > input_len) {

    fprintf(stderr, _("Input too long!\n"));	
    return -1;

  }

  /*  Null terminate. */

  message_buffer[chars_read] = 0x00;	

  /* We send concatenated SMS if and only if we have more then
     GSM_MAX_SMS_LENGTH characters to send */
  if (chars_read > GSM_MAX_SMS_LENGTH) {
        SMS.UDHType = GSM_ConcatenatedMessages;
  }

  /* offset - length of the UDH */
  /* nr_msg - number of the messages to send */
  offset = 0;
  nr_msg = 1;
  switch (SMS.UDHType) {
  	case GSM_NoUDH:
  	  break;
    	case GSM_ConcatenatedMessages:
    	  UDH[0] = 0x05;	// UDH length
    	  UDH[1] = 0x00;	// concatenated messages (IEI)
    	  UDH[2] = 0x03;	// IEI data length
    	  UDH[3] = 0x01;	// reference number
    	  UDH[4] = 0x00;	// number of messages
    	  UDH[5] = 0x00;	// message reference number
    	  offset = 6;
    	  /* only 153 chars in single uncompressed message */
    	  nr_msg += chars_read/153;
    	  break;
    	default:
    	  break;
  }

  UDH[4] = nr_msg;	// number of messages
  /* Initialise the GSM interface. */     

  fbusinit(NULL);

  for (i = 0; i < nr_msg; i++) {

    UDH[5] = i + 1;	// message reference number
    aux = i*153;	// aux - how many character we have already sent

    if (SMS.UDHType) {
      memcpy(SMS.UDH,UDH,offset);
      strncpy (SMS.MessageText, message_buffer+aux, 153);
      SMS.MessageText[153] = 0;
    } else {
      strncpy (SMS.MessageText,message_buffer,chars_read);
      SMS.MessageText[chars_read] = 0;
    }

    /* Send the message. */

    error = GSM->SendSMSMessage(&SMS,0);

    if (error == GE_SMSSENDOK) {
      fprintf(stdout, _("Send succeeded!\n"));
    } else {
      fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);
      break;
    }
    sleep(10);
  }

  GSM->Terminate();

  return 0;
}

/* Get SMSC number */

int getsmsc(char *MessageCenterNumber)
{

  GSM_MessageCenter MessageCenter;

  MessageCenter.No=atoi(MessageCenterNumber);

  fbusinit(NULL);

  if (GSM->GetSMSCenter(&MessageCenter) == GE_NONE) {

    fprintf(stdout, _("%d. SMS center (%s) number is %s\n"), MessageCenter.No, MessageCenter.Name, MessageCenter.Number);

    fprintf(stdout, _("Messages sent as "));

    switch (MessageCenter.Format) {

    case GSMF_Text:
      fprintf(stdout, _("Text"));
      break;

    case GSMF_Paging:
      fprintf(stdout, _("Paging"));
      break;

    case GSMF_Fax:
      fprintf(stdout, _("Fax"));
      break;

    case GSMF_Email:
    case GSMF_UCI:
      fprintf(stdout, _("Email"));
      break;

    case GSMF_ERMES:
      fprintf(stdout, _("ERMES"));
      break;

    case GSMF_X400:
      fprintf(stdout, _("X.400"));
      break;

    default:
      fprintf(stdout, _("Unknown"));
    }

    printf("\n");

    fprintf(stdout, _("Message validity is "));

    switch (MessageCenter.Validity) {

    case GSMV_1_Hour:
      fprintf(stdout, _("1 hour"));
      break;

    case GSMV_6_Hours:
      fprintf(stdout, _("6 hours"));
      break;

    case GSMV_24_Hours:
      fprintf(stdout, _("24 hours"));
      break;

    case GSMV_72_Hours:
      fprintf(stdout, _("72 hours"));
      break;

    case GSMV_1_Week:
      fprintf(stdout, _("1 week"));
	break;

    case GSMV_Max_Time:
      fprintf(stdout, _("Maximum time"));
      break;

    default:
      fprintf(stdout, _("Unknown"));
    }

    fprintf(stdout, "\n");

  }
  else
    fprintf(stdout, _("SMS center can not be found :-(\n"));

  GSM->Terminate();

  return 0;
}

/* Get SMS messages. */
int getsms(char *argv[])
{

  GSM_SMSMessage message;
  char *memory_type_string;
  int start_message, end_message, count;
  GSM_Error error;

  /* Handle command line args that set type, start and end locations. */


  if (strcmp(argv[0], "ME") == 0) {
    message.MemoryType = GMT_ME;
    memory_type_string = "ME";
  }
  else
    if (strcmp(argv[0], "SM") == 0) {
      message.MemoryType = GMT_SM;
      memory_type_string = "SM";
    }
  else
    if (strcmp(argv[0], "FD") == 0) {
      message.MemoryType = GMT_FD;
      memory_type_string = "FD";
    }
  else
    if (strcmp(argv[0], "ON") == 0) {
      message.MemoryType = GMT_ON;
      memory_type_string = "ON";
    }
  else
    if (strcmp(argv[0], "EN") == 0) {
      message.MemoryType = GMT_EN;
      memory_type_string = "EN";
    }
  else
    if (strcmp(argv[0], "DC") == 0) {
      message.MemoryType = GMT_DC;
      memory_type_string = "DC";
    }
  else
    if (strcmp(argv[0], "RC") == 0) {
      message.MemoryType = GMT_RC;
      memory_type_string = "RC";
     }
  else
    if (strcmp(argv[0], "MC") == 0) {
      message.MemoryType = GMT_MC;
      memory_type_string = "MC";
    }
  else
    if (strcmp(argv[0], "LD") == 0) {
      message.MemoryType = GMT_LD;
      memory_type_string = "LD";
    }
  else
    if (strcmp(argv[0], "MT") == 0) {
      message.MemoryType = GMT_MT;
      memory_type_string = "MT";
    }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[0]);
    exit (-1);
  }

  start_message = atoi(argv[1]);
  end_message = atoi(argv[2]);

  /* Initialise the code for the GSM interface. */     

  fbusinit(NULL);

  /* Now retrieve the requested entries. */

  for (count = start_message; count <= end_message; count ++) {

    message.Location = count;
	
    error = GSM->GetSMSMessage(&message);

    switch (error) {

    case GE_NONE:

      switch (message.Type) {

        case GST_MO:

          fprintf(stdout, _("%d. Outbox Message "), message.MessageNumber);

          if (message.Status)
            fprintf(stdout, _("(sent)\n"));
          else
            fprintf(stdout, _("(not sent)\n"));

          fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 

          break;

        case GST_DR:

          fprintf(stdout, _("%d. Delivery Report "), message.MessageNumber);
          if (message.Status)
            fprintf(stdout, _("(read)\n"));
          else
            fprintf(stdout, _("(not read)\n"));

          fprintf(stdout, _("Sending date/time: %d/%d/%d %d:%02d:%02d "), \
                  message.Time.Day, message.Time.Month, message.Time.Year, \
                  message.Time.Hour, message.Time.Minute, message.Time.Second);

          if (message.Time.Timezone) {
            if (message.Time.Timezone > 0)
              fprintf(stdout,_("+%02d00"), message.Time.Timezone);
            else
              fprintf(stdout,_("%02d00"), message.Time.Timezone);
          }

          fprintf(stdout, "\n");

          fprintf(stdout, _("Response date/time: %d/%d/%d %d:%02d:%02d "), \
                  message.SMSCTime.Day, message.SMSCTime.Month, message.SMSCTime.Year, \
                  message.SMSCTime.Hour, message.SMSCTime.Minute, message.SMSCTime.Second);

          if (message.SMSCTime.Timezone) {
            if (message.SMSCTime.Timezone > 0)
              fprintf(stdout,_("+%02d00"),message.SMSCTime.Timezone);
            else
              fprintf(stdout,_("%02d00"),message.SMSCTime.Timezone);
          }

          fprintf(stdout, "\n");

          fprintf(stdout, _("Receiver: %s Msg Center: %s\n"), message.Sender, message.MessageCenter.Number);
          fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 

          break;

        default:

          fprintf(stdout, _("%d. Inbox Message "), message.MessageNumber);

          if (message.Status)
            fprintf(stdout, _("(read)\n"));
          else
            fprintf(stdout, _("(not read)\n"));

          fprintf(stdout, _("Date/time: %d/%d/%d %d:%02d:%02d "), \
                  message.Time.Day, message.Time.Month, message.Time.Year, \
                  message.Time.Hour, message.Time.Minute, message.Time.Second);

          if (message.Time.Timezone) {
            if (message.Time.Timezone > 0)
              fprintf(stdout,_("+%02d00"),message.Time.Timezone);
            else
              fprintf(stdout,_("%02d00"),message.Time.Timezone);
          }

          fprintf(stdout, "\n");
          fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.Sender, message.MessageCenter.Number);

	  switch (message.UDHType) {
	  		case GSM_NoUDH:
	  			break;
	  		case GSM_ConcatenatedMessages:
	  			fprintf(stdout,_("Linked (%d/%d):\n"),message.UDH[5],message.UDH[4]);
	  			break;
	  		default:
	  			break;
	  }

          fprintf(stdout, _("Text:\n%s\n\n"), message.MessageText); 

          break;
      }

      break;

    case GE_NOTIMPLEMENTED:

      fprintf(stderr, _("Function not implemented in %s model!\n"), model);
      GSM->Terminate();
      return -1;	

    case GE_INVALIDSMSLOCATION:

      fprintf(stderr, _("Invalid location: %s %d\n"), memory_type_string, count);

      break;

    case GE_EMPTYSMSLOCATION:

      fprintf(stderr, _("SMS location %s %d empty.\n"), memory_type_string, count);

      break;

    default:

      fprintf(stdout, _("GetSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
    }
  }

  GSM->Terminate();

  return 0;
}

/* Delete SMS messages. */
int deletesms(char *argv[])
{

  GSM_SMSMessage message;
  char *memory_type_string;
  int start_message, end_message, count;
  GSM_Error error;

  /* Handle command line args that set type, start and end locations. */

  if (strcmp(argv[0], "ME") == 0) {
    message.MemoryType = GMT_ME;
    memory_type_string = "ME";
  }
  else
    if (strcmp(argv[0], "SM") == 0) {
      message.MemoryType = GMT_SM;
      memory_type_string = "SM";
    }
  else
    if (strcmp(argv[0], "FD") == 0) {
      message.MemoryType = GMT_FD;
      memory_type_string = "FD";
    }
  else
    if (strcmp(argv[0], "ON") == 0) {
      message.MemoryType = GMT_ON;
      memory_type_string = "ON";
    }
  else
    if (strcmp(argv[0], "EN") == 0) {
      message.MemoryType = GMT_EN;
      memory_type_string = "EN";
    }
  else
    if (strcmp(argv[0], "DC") == 0) {
      message.MemoryType = GMT_DC;
      memory_type_string = "DC";
    }
  else
    if (strcmp(argv[0], "RC") == 0) {
      message.MemoryType = GMT_RC;
      memory_type_string = "RC";
    }
  else
    if (strcmp(argv[0], "MC") == 0) {
      message.MemoryType = GMT_MC;
      memory_type_string = "MC";
    }
  else
    if (strcmp(argv[0], "LD") == 0) {
      message.MemoryType = GMT_LD;
      memory_type_string = "LD";
    }
  else
    if (strcmp(argv[0], "MT") == 0) {
      message.MemoryType = GMT_MT;
      memory_type_string = "MT";
    }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[0]);
    return -1;
  }

  start_message = atoi (argv[1]);
  end_message = atoi (argv[2]);

  /* Initialise the code for the GSM interface. */     

  fbusinit(NULL);

  /* Now delete the requested entries. */

  for (count = start_message; count <= end_message; count ++) {

    message.Location = count;

    error = GSM->DeleteSMSMessage(&message);

    if (error == GE_NONE)
      fprintf(stdout, _("Deleted SMS %s %d\n"), memory_type_string, count);
    else {
      if (error == GE_NOTIMPLEMENTED) {
	fprintf(stderr, _("Function not implemented in %s model!\n"), model);
	GSM->Terminate();
	return -1;	
      }
      fprintf(stdout, _("DeleteSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
    }
  }

  GSM->Terminate();

  return 0;
}

static volatile bool bshutdown = false;

/* SIGINT signal handler. */

static void interrupted(int sig)
{

  signal(sig, SIG_IGN);
  bshutdown = true;

}

#ifdef SECURITY

/* In this mode we get the code from the keyboard and send it to the mobile
   phone. */

int entersecuritycode(char *type)
{
  GSM_Error test;
  GSM_SecurityCode SecurityCode;

  if (!strcmp(type,"PIN"))
    SecurityCode.Type=GSCT_Pin;
  else if (!strcmp(type,"PUK"))
    SecurityCode.Type=GSCT_Puk;
  else if (!strcmp(type,"PIN2"))
    SecurityCode.Type=GSCT_Pin2;
  else if (!strcmp(type,"PUK2"))
    SecurityCode.Type=GSCT_Puk2;
  // FIXME: Entering of SecurityCode does not work :-(
  //  else if (!strcmp(type,"SecurityCode"))
  //    SecurityCode.Type=GSCT_SecurityCode;
  else {
    usage();
    return -1;
  }

#ifdef WIN32
  printf("Enter your code: ");
  gets(SecurityCode.Code);
#else
  strcpy(SecurityCode.Code,getpass(_("Enter your code: ")));
#endif

  fbusinit(NULL);

  test = GSM->EnterSecurityCode(SecurityCode);
  if (test == GE_INVALIDSECURITYCODE)
    fprintf(stdout, _("Error: invalid code.\n"));
  else if (test == GE_NONE)
    fprintf(stdout, _("Code ok.\n"));
  else if (test == GE_NOTIMPLEMENTED)
    fprintf(stderr, _("Function not implemented in %s model!\n"), model);
  else
    fprintf(stdout, _("Other error.\n"));

  GSM->Terminate();

  return 0;
}

int getsecuritycodestatus(void)
{

  int Status;

  fbusinit(NULL);

  if (GSM->GetSecurityCodeStatus(&Status) == GE_NONE) {

    fprintf(stdout, _("Security code status: "));

      switch(Status) {

      case GSCT_SecurityCode:

	fprintf(stdout, _("waiting for Security Code.\n"));
	break;

      case GSCT_Pin:

	fprintf(stdout, _("waiting for PIN.\n"));
	break;

      case GSCT_Pin2:

	fprintf(stdout, _("waiting for PIN2.\n"));
	break;

      case GSCT_Puk:

	fprintf(stdout, _("waiting for PUK.\n"));
	break;

      case GSCT_Puk2:

	fprintf(stdout, _("waiting for PUK2.\n"));
	break;

      case GSCT_None:

	fprintf(stdout, _("nothing to enter.\n"));
	break;

      default:

	fprintf(stdout, _("Unknown!\n"));
      }
  }

  GSM->Terminate();

  return 0;
}


#endif

/* Voice dialing mode. */

int dialvoice(char *Number)
{

  fbusinit(NULL);

  GSM->DialVoice(Number);

  GSM->Terminate();

  return 0;
}

/* The following function allows to send logos using SMS */
int sendlogo(int argc, char *argv[])
{
  GSM_SMSMessage SMS;
  GSM_Bitmap bitmap;
  GSM_Error error;

  char UserDataHeader[7] = {	0x06, /* UDH Length */
		  		0x05, /* IEI: application port addressing scheme, 16 bit address */
		  		0x04, /* IEI length */
                		0x15, /* destination address: high byte */
				0x00, /* destination address: low byte */
                		0x00, /* originator address */
				0x00};

  char Data[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  int current=0;

  /* Default settings for SMS message:
      - no delivery report
      - Class Message 1
      - no compression
      - 8 bit data
      - SMSC no. 1
      - validity 3 days
      - set UserDataHeaderIndicator
  */

  SMS.Type = GST_MO;
  SMS.Class = 1;
  SMS.Compression = false;
  SMS.EightBit = true;
  SMS.MessageCenter.No = 1;
  SMS.Validity = 4320; /* 4320 minutes == 72 hours */

  /* The first argument is the type of the logo. */
  if (!strcmp(argv[0], "op")) {
    SMS.UDHType = GSM_OpLogo;
    UserDataHeader[4] = 0x82; /* NBS port 0x1582 */
    fprintf(stdout, _("Sending operator logo.\n"));
  } else if (!strcmp(argv[0], "caller")) {
    SMS.UDHType = GSM_CallerIDLogo;
    UserDataHeader[4] = 0x83; /* NBS port 0x1583 */
    fprintf(stdout, _("Sending caller line identification logo.\n"));
  } else {
    fprintf(stderr, _("You should specify what kind of logo to send!\n"));
    return (-1);
  }

  /* The second argument is the destionation, ie the phone number of recipient. */
  strcpy(SMS.Destination,argv[1]);

  /* The third argument is the bitmap file. */
  GSM_ReadBitmapFile(argv[2], &bitmap);

  /* If we are sending op logo we can rewrite network code. */
  if (!strcmp(argv[0], "op")){
    /*
     * The fourth argument, if present, is the Network code of the operator.
     * Network code is in this format: "xxx yy".
     */
    if (argc > 3) {
      strcpy(bitmap.netcode, argv[3]);
#ifdef DEBUG
      fprintf(stdout, _("Operator code: %s\n"), argv[3]);
#endif
    }

    /* Set the network code */
    Data[current++] = ((bitmap.netcode[1] & 0x0f) << 4) | (bitmap.netcode[0] & 0xf);
    Data[current++] = 0xf0 | (bitmap.netcode[2] & 0x0f);
    Data[current++] = ((bitmap.netcode[5] & 0x0f) << 4) | (bitmap.netcode[4] & 0xf);
  }

  /* Set the logo size */
  current++;
  Data[current++] = bitmap.width;
  Data[current++] = bitmap.height;

  Data[current++] = 0x01;

  memcpy(SMS.UDH,UserDataHeader,7);
  memcpy(SMS.MessageText,Data,current);
  memcpy(SMS.MessageText+current,bitmap.bitmap,bitmap.size);

  /* Initialise the GSM interface. */
  fbusinit(NULL);

  /* Send the message. */
  error = GSM->SendSMSMessage(&SMS,current+bitmap.size);

  if (error == GE_SMSSENDOK)
    fprintf(stdout, _("Send succeeded!\n"));
  else
    fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

  GSM->Terminate();
  return 0;
}

/* Data dialing mode. */

int dialdata(char *Number)
{

  fbusinit(NULL);

  sleep(10);

  GSM->DialData(Number, 0);

  GSM->Terminate();

  return 0;
}

/* Getting logos. */

int getlogo(char *argv[])
{
  GSM_Bitmap bitmap;

  fbusinit(NULL);

  bitmap.type=GSM_None;

  if (strcmp(argv[1],"op")==0)
    bitmap.type=GSM_OperatorLogo;
  if (strcmp(argv[1],"caller")==0) {
    /* There is caller group number missing in argument list. */
    if (!argv[2]) {
      usage();
      exit(-1);
    }
    bitmap.number=argv[2][0]-'0';
    bitmap.type=GSM_CallerLogo;
    if ((bitmap.type<0)||(bitmap.type>9)) bitmap.type=0;
    }
  if (strcmp(argv[1],"startup")==0)
    bitmap.type=GSM_StartupLogo;

  if (bitmap.type!=GSM_None) {
    fprintf(stdout, _("Getting Logo.\n"));
    if (GSM->GetBitmap(&bitmap)==GE_NONE && bitmap.width!=0){
      GSM_SaveBitmapFile(argv[0], &bitmap);
    }
  }

  GSM->Terminate();

  return 0;
}


/* Sending logos. */

int setlogo(char *argv[])
{

  GSM_Bitmap bitmap;
  GSM_Bitmap oldbit;

  fbusinit(NULL);

  if (!GSM_ReadBitmapFile(argv[0], &bitmap)) {      
    if (argv[1] && (bitmap.type==GSM_OperatorLogo))
      strncpy(bitmap.netcode,argv[1],7);
    if (argv[1] && (bitmap.type==GSM_CallerLogo)) {
      bitmap.number=argv[1][0]-'0';
      oldbit.type=GSM_CallerLogo;
      oldbit.number=bitmap.number;
      if (GSM->GetBitmap(&oldbit)==0) {
	/* We have to get the old name and ringtone!! */
	bitmap.ringtone=oldbit.ringtone;
	strncpy(bitmap.text,oldbit.text,255);
	if ((bitmap.number<0)||(bitmap.number>9)) bitmap.number=0;
	if (argv[2]) strncpy(bitmap.text,argv[2],255);
      }
    }
  }
  else {
    if (strcmp(argv[0],"text")==0 || strcmp(argv[0],"dealer")==0){
      bitmap.type=GSM_StartupLogo;
      bitmap.text[0]=0x00;
      bitmap.dealertext[0]=0x00;
      bitmap.dealerset=(strcmp(argv[0],"dealer")==0);
      if (argv[1]) {
        if (strcmp(argv[0],"text")==0) {
	  strncpy(bitmap.text,argv[1],255);
	} else {
          strncpy(bitmap.dealertext,argv[1],255);
	}
      }
      bitmap.size=0;
    } else {
      fprintf(stdout, _("Logo file error.\n"));
      GSM->Terminate();
      return (-1);
    }
  }

  fprintf(stdout, _("Sending Logo.\n"));
  GSM->SetBitmap(&bitmap);
  GSM->Terminate();

  return 0;
}



/* Calendar notes receiving. */

int getcalendarnote(int argc, char *argv[])
{
  GSM_CalendarNote CalendarNote;
  int i;
  bool vCal=false;

  struct option options[] = {
             { "vCard",    optional_argument, NULL, '1'},
             { NULL,      0,                 NULL, 0}
  };

  optarg = NULL;
  optind = 0;
  CalendarNote.Location=atoi(argv[0]);

  while ((i = getopt_long(argc, argv, "v", options, NULL)) != -1) {
    switch (i) {       
      case 'v':
        vCal=true;
        break;
      default:
        usage(); // Would be better to have an calendar_usage() here.
        return -1;
    }
  }

  fbusinit(NULL);

  if (GSM->GetCalendarNote(&CalendarNote) == GE_NONE) {

    if (vCal) {
        fprintf(stdout, _("BEGIN:VCALENDAR\n"));
        fprintf(stdout, _("VERSION:1.0\n"));
        fprintf(stdout, _("BEGIN:VEVENT\n"));
        fprintf(stdout, _("CATEGORIES:"));
        switch (CalendarNote.Type) {
         case GCN_REMINDER:
           fprintf(stdout, _("MISCELLANEOUS\n"));
           break;
         case GCN_CALL:
           fprintf(stdout, _("PHONE CALL\n"));
           break;
         case GCN_MEETING:
           fprintf(stdout, _("MEETING\n"));
           break;
         case GCN_BIRTHDAY:
           fprintf(stdout, _("SPECIAL OCCASION\n"));
           break;
         default:
           fprintf(stdout, _("UNKNOWN\n"));
           break;
        }
        fprintf(stdout, _("SUMMARY:%s\n"),CalendarNote.Text);
        fprintf(stdout, _("DTSTART:%04d%02d%02dT%02d%02d%02d\n"), CalendarNote.Time.Year,
           CalendarNote.Time.Month, CalendarNote.Time.Day, CalendarNote.Time.Hour,
           CalendarNote.Time.Minute, CalendarNote.Time.Second);
        if (CalendarNote.Alarm.Year!=0) {
           fprintf(stdout, _("DALARM:%04d%02d%02dT%02d%02d%02d\n"), CalendarNote.Alarm.Year,
              CalendarNote.Alarm.Month, CalendarNote.Alarm.Day, CalendarNote.Alarm.Hour,
              CalendarNote.Alarm.Minute, CalendarNote.Alarm.Second);
        }
        fprintf(stdout, _("END:VEVENT\n"));
        fprintf(stdout, _("END:VCALENDAR\n"));

    } else {  /* not vCal */
	fprintf(stdout, _("   Type of the note: "));

	switch (CalendarNote.Type) {

	case GCN_REMINDER:
	  fprintf(stdout, _("Reminder\n"));
	  break;

	case GCN_CALL:
	  fprintf(stdout, _("Call\n"));
	  break;

	case GCN_MEETING:
	  fprintf(stdout, _("Meeting\n"));
	  break;

	case GCN_BIRTHDAY:
	  fprintf(stdout, _("Birthday\n"));
	  break;

	default:
	  fprintf(stdout, _("Unknown\n"));

	}

        fprintf(stdout, _("   Date: %d-%02d-%02d\n"), CalendarNote.Time.Year,
                                                      CalendarNote.Time.Month,
                                                      CalendarNote.Time.Day);

        fprintf(stdout, _("   Time: %02d:%02d:%02d\n"), CalendarNote.Time.Hour,
                                                        CalendarNote.Time.Minute,
                                                        CalendarNote.Time.Second);

        if (CalendarNote.Alarm.Year!=0) {
          fprintf(stdout, _("   Alarm date: %d-%02d-%02d\n"), CalendarNote.Alarm.Year,
                                                            CalendarNote.Alarm.Month,
                                                            CalendarNote.Alarm.Day);

          fprintf(stdout, _("   Alarm time: %02d:%02d:%02d\n"), CalendarNote.Alarm.Hour,
                                                              CalendarNote.Alarm.Minute,
                                                              CalendarNote.Alarm.Second);
        }

        fprintf(stdout, _("   Text: %s\n"), CalendarNote.Text);

        if (CalendarNote.Type == GCN_CALL)
          fprintf(stdout, _("   Phone: %s\n"), CalendarNote.Phone);
    }
  }
  else {
    fprintf(stderr, _("The calendar note can not be read\n"));

    GSM->Terminate();
    return -1;
  }

  GSM->Terminate();

  return 0;
}

/* Writing calendar notes. */

int writecalendarnote(char *argv[])
{

  GSM_CalendarNote CalendarNote;

  CalendarNote.Type=GCN_REMINDER;

  CalendarNote.Time.Year=1999;
  CalendarNote.Time.Month=12;
  CalendarNote.Time.Day=31;
  CalendarNote.Time.Hour=23;
  CalendarNote.Time.Minute=59;

  CalendarNote.Alarm.Year=1999;
  CalendarNote.Alarm.Month=12;
  CalendarNote.Alarm.Day=31;
  CalendarNote.Alarm.Hour=23;
  CalendarNote.Alarm.Minute=58;

  /* FIXME: Hey Pavel, fix this :-)) It works... */
  sprintf(CalendarNote.Text, "Big day ...");
  sprintf(CalendarNote.Phone, "jhgjhgjhgjhgj");

  fbusinit(NULL);

  if (GSM->WriteCalendarNote(&CalendarNote) == GE_NONE) {
    fprintf(stdout, _("Succesfully written!\n"));
  }

  GSM->Terminate();

  return 0;
}

/* Calendar note deleting. */

int deletecalendarnote(char *Index)
{

  GSM_CalendarNote CalendarNote;

  CalendarNote.Location=atoi(Index);

  fbusinit(NULL);

  if (GSM->DeleteCalendarNote(&CalendarNote) == GE_NONE) {
    fprintf(stdout, _("   Calendar note deleted.\n"));
  }
  else {
    fprintf(stderr, _("The calendar note can not be deleted\n"));

    GSM->Terminate();
    return -1;
  }

  GSM->Terminate();

  return 0;
}

/* Setting the date and time. */

int setdatetime(int argc, char *argv[])
{
  struct tm *now;
  time_t nowh;
  GSM_DateTime Date;

  fbusinit(NULL);

  nowh=time(NULL);
  now=localtime(&nowh);

  Date.Year = now->tm_year;
  Date.Month = now->tm_mon+1;
  Date.Day = now->tm_mday;
  Date.Hour = now->tm_hour;
  Date.Minute = now->tm_min;
  Date.Second = now->tm_sec;

  if (argc>0) Date.Year = atoi (argv[0]);
  if (argc>1) Date.Month = atoi (argv[1]);
  if (argc>2) Date.Day = atoi (argv[2]);
  if (argc>3) Date.Hour = atoi (argv[3]);
  if (argc>4) Date.Minute = atoi (argv[4]);

  if (Date.Year<1900)
  {

    /* Well, this thing is copyrighted in U.S. This technique is known as
       Windowing and you can read something about it in LinuxWeekly News:
       http://lwn.net/1999/features/Windowing.phtml. This thing is beeing
       written in Czech republic and Poland where algorhitms are not allowed
       to be patented. */

    if (Date.Year>90)
      Date.Year = Date.Year+1900;
    else
      Date.Year = Date.Year+2000;
  }

  /* FIXME: Error checking should be here. */
  GSM->SetDateTime(&Date);

  GSM->Terminate();

  return 0;
}

/* In this mode we receive the date and time from mobile phone. */

int getdatetime(void) {

  GSM_DateTime date_time;

  fbusinit(NULL);

  if (GSM->GetDateTime(&date_time)==GE_NONE) {
    fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.Year, date_time.Month, date_time.Day);
    fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.Hour, date_time.Minute, date_time.Second);
  }

  GSM->Terminate();

  return 0;
}

/* Setting the alarm. */

int setalarm(char *argv[])
{

  GSM_DateTime Date;

  fbusinit(NULL);

  Date.Hour = atoi(argv[0]);
  Date.Minute = atoi(argv[1]);

  GSM->SetAlarm(1, &Date);

  GSM->Terminate();

  return 0;
}

/* Getting the alarm. */

int getalarm(void) {

  GSM_DateTime date_time;

  fbusinit(NULL);

  if (GSM->GetAlarm(0, &date_time)==GE_NONE) {
    fprintf(stdout, _("Alarm: %s\n"), (date_time.AlarmEnabled==0)?"off":"on");
    fprintf(stdout, _("Time: %02d:%02d\n"), date_time.Hour, date_time.Minute);
  }

  GSM->Terminate();

  return 0;
}

/* In monitor mode we don't do much, we just initialise the fbus code.
   Note that the fbus code no longer has an internal monitor mode switch,
   instead compile with DEBUG enabled to get all the gumpf. */

int monitormode(void)
{

  float rflevel=-1, batterylevel=-1;
  GSM_PowerSource powersource=-1;
  GSM_RFUnits rf_units = GRF_Arbitrary;
  GSM_BatteryUnits batt_units = GBU_Arbitrary;

  GSM_NetworkInfo NetworkInfo;

  GSM_MemoryStatus SIMMemoryStatus = {GMT_SM, 0, 0};
  GSM_MemoryStatus PhoneMemoryStatus = {GMT_ME, 0, 0};
  GSM_MemoryStatus DC_MemoryStatus = {GMT_DC, 0, 0};
  GSM_MemoryStatus EN_MemoryStatus = {GMT_EN, 0, 0};
  GSM_MemoryStatus FD_MemoryStatus = {GMT_FD, 0, 0};
  GSM_MemoryStatus LD_MemoryStatus = {GMT_LD, 0, 0};
  GSM_MemoryStatus MC_MemoryStatus = {GMT_MC, 0, 0};
  GSM_MemoryStatus ON_MemoryStatus = {GMT_ON, 0, 0};
  GSM_MemoryStatus RC_MemoryStatus = {GMT_RC, 0, 0};

  GSM_SMSStatus SMSStatus = {0, 0};

  char Number[20];

  /* We do not want to monitor serial line forever - press Ctrl+C to stop the
     monitoring mode. */

  signal(SIGINT, interrupted);

  fprintf (stderr, _("Entering monitor mode...\n"));
  fprintf (stderr, _("Initialising GSM interface...\n"));

  /* Initialise the code for the GSM interface. */     

  fbusinit(NULL);

  /* Loop here indefinitely - allows you to see messages from GSM code in
     response to unknown messages etc. The loops ends after pressing the
     Ctrl+C. */
  while (!bshutdown) {
    if (GSM->GetRFLevel(&rf_units, &rflevel) == GE_NONE)
      fprintf(stdout, _("RFLevel: %d\n"), (int)rflevel);

    if (GSM->GetBatteryLevel(&batt_units, &batterylevel) == GE_NONE)
      fprintf(stdout, _("Battery: %d\n"), (int)batterylevel);

    if (GSM->GetPowerSource(&powersource) == GE_NONE)
      fprintf(stdout, _("Power Source: %s\n"), (powersource==GPS_ACDC)?_("AC/DC"):_("battery"));

    if (GSM->GetMemoryStatus(&SIMMemoryStatus) == GE_NONE)
      fprintf(stdout, _("SIM: Used %d, Free %d\n"), SIMMemoryStatus.Used, SIMMemoryStatus.Free);

    if (GSM->GetMemoryStatus(&PhoneMemoryStatus) == GE_NONE)
      fprintf(stdout, _("Phone: Used %d, Free %d\n"), PhoneMemoryStatus.Used, PhoneMemoryStatus.Free);

    if (GSM->GetMemoryStatus(&DC_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("DC: Used %d, Free %d\n"), DC_MemoryStatus.Used, DC_MemoryStatus.Free);

    if (GSM->GetMemoryStatus(&EN_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("EN: Used %d, Free %d\n"), EN_MemoryStatus.Used, EN_MemoryStatus.Free);

    if (GSM->GetMemoryStatus(&FD_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("FD: Used %d, Free %d\n"), FD_MemoryStatus.Used, FD_MemoryStatus.Free);

    if (GSM->GetMemoryStatus(&LD_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("LD: Used %d, Free %d\n"), LD_MemoryStatus.Used, LD_MemoryStatus.Free);

    if (GSM->GetMemoryStatus(&MC_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("MC: Used %d, Free %d\n"), MC_MemoryStatus.Used, MC_MemoryStatus.Free);

    if (GSM->GetMemoryStatus(&ON_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("ON: Used %d, Free %d\n"), ON_MemoryStatus.Used, ON_MemoryStatus.Free);

    if (GSM->GetMemoryStatus(&RC_MemoryStatus) == GE_NONE)
      fprintf(stdout, _("RC: Used %d, Free %d\n"), RC_MemoryStatus.Used, RC_MemoryStatus.Free);

    if (GSM->GetSMSStatus(&SMSStatus) == GE_NONE)
      fprintf(stdout, _("SMS Messages: UnRead %d, Number %d\n"), SMSStatus.UnRead, SMSStatus.Number);

    if (GSM->GetIncomingCallNr(Number) == GE_NONE)
      fprintf(stdout, _("Incoming call: %s\n"), Number);

    if (GSM->GetNetworkInfo(&NetworkInfo) == GE_NONE)
      fprintf(stdout, _("Network: %s (%s), LAC: %s, CellID: %s\n"), GSM_GetNetworkName (NetworkInfo.NetworkCode), GSM_GetCountryName(NetworkInfo.NetworkCode), NetworkInfo.LAC, NetworkInfo.CellID);
	    
    sleep(1);
  }

  fprintf (stderr, _("Leaving monitor mode...\n"));

  GSM->Terminate();

  return 0;
}

/* Reads profile from phone and displays its' settings */

int getprofile(int argc, char *argv[])
{

  int max_profiles;
  int start, stop, i;
  GSM_Profile profile;
  GSM_Error error;

  /* Initialise the code for the GSM interface. */     

  fbusinit(NULL);

  profile.Number = 0;
  error=GSM->GetProfile(&profile);

  if (error == GE_NONE)
  {
    max_profiles=7; /* This is correct for 6110 (at least my). How do we get
                       the number of profiles? */

    /* FIXME: what does this mean? Why 0x30 ie 48? */
    if (profile.Ringtone==48)
      max_profiles=3;

    if (argc>0)
    {
      profile.Number=atoi(argv[0])-1;
      start=profile.Number;
      stop=start+1;

      if (profile.Number < 0)
      {
         fprintf(stderr, _("Profile number must be value from 1 to %i!\n"), max_profiles);
         GSM->Terminate();
         return -1;
      }

      if (profile.Number >= max_profiles)
      {
         fprintf(stderr, _("This phone supports only %i profiles!\n"), max_profiles);
         GSM->Terminate();
         return -1;
      }
    } else {
      start=0;
      stop=max_profiles;
    }

    i=start;
    while (i<stop)
    {
      profile.Number=i;

      if (profile.Number!=0)
        GSM->GetProfile(&profile);

      printf("%i. \"%s\"\n", (profile.Number+1), profile.Name);
      if (profile.DefaultName==-1) printf(" (name defined)\n");

      printf("Incoming call alert: %s\n", GetProfileCallAlertString(profile.CallAlert));

      if (profile.Ringtone!=48)
      {
        /* For different phones different ringtones names */

        if (!strcmp(model,"6110"))
          printf("Ringing tone: %s\n", GetRingtoneName(profile.Ringtone));
        else
          printf("Ringtone number: %i\n", profile.Ringtone);
      }

      printf("Ringing volume: %s\n", GetProfileVolumeString(profile.Volume));

      printf("Message alert tone: %s\n", GetProfileMessageToneString(profile.MessageTone));

      printf("Keypad tones: %s\n", GetProfileKeypadToneString(profile.KeypadTone));

      printf("Warning and game tones: %s\n", GetProfileWarningToneString(profile.WarningTone));

      /* FIXME: Light settings is only used for Car */
      if (profile.Number==(max_profiles-2)) printf("Lights: %s\n", profile.Lights ? "on" : "off");

      printf("Vibration: %s\n", GetProfileVibrationString(profile.Vibration));

      /* FIXME: it will be nice to add here reading caller group name. */
      if (profile.Ringtone!=48) printf("Caller groups: 0x%02x\n", profile.CallerGroups);

      /* FIXME: Automatic answer is only used for Car and Headset. */
      if (profile.Number>=(max_profiles-2)) printf("Automatic answer: %s\n", profile.AutomaticAnswer ? "On" : "Off");

      printf("\n");

      i++;
    }
  } else {
    if (error == GE_NOTIMPLEMENTED) {
       fprintf(stderr, _("Function not implemented in %s model!\n"), model);
       GSM->Terminate();
       return -1;
    } else
    {
      fprintf(stderr, _("Unspecified error\n"));
      GSM->Terminate();
      return -1;
    }
  }

  GSM->Terminate();

  return 0;

}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */

int getmemory(char *argv[])
{

  GSM_PhonebookEntry entry;
  int count;
  GSM_Error error;
  char *memory_type_string;
  int start_entry;
  int end_entry;

  /* Handle command line args that set type, start and end locations. */

  if (strcmp(argv[0], "ME") == 0) {
    entry.MemoryType = GMT_ME;
    memory_type_string = "ME";
  }
  else
    if (strcmp(argv[0], "SM") == 0) {
      entry.MemoryType = GMT_SM;
      memory_type_string = "SM";
    }
  else
    if (strcmp(argv[0], "FD") == 0) {
      entry.MemoryType = GMT_FD;
      memory_type_string = "FD";
    }
  else
    if (strcmp(argv[0], "ON") == 0) {
      entry.MemoryType = GMT_ON;
      memory_type_string = "ON";
    }
  else
    if (strcmp(argv[0], "EN") == 0) {
      entry.MemoryType = GMT_EN;
      memory_type_string = "EN";
    }
  else
    if (strcmp(argv[0], "DC") == 0) {
      entry.MemoryType = GMT_DC;
      memory_type_string = "DC";
    }
  else
    if (strcmp(argv[0], "RC") == 0) {
      entry.MemoryType = GMT_RC;
      memory_type_string = "RC";
    }
  else
    if (strcmp(argv[0], "MC") == 0) {
      entry.MemoryType = GMT_MC;
      memory_type_string = "MC";
    }
  else
    if (strcmp(argv[0], "LD") == 0) {
      entry.MemoryType = GMT_LD;
      memory_type_string = "LD";
    }
  else
    if (strcmp(argv[0], "MT") == 0) {
      entry.MemoryType = GMT_MT;
      memory_type_string = "MT";
    }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[0]);
    return -1;
  }

  start_entry = atoi (argv[1]);
  end_entry = atoi (argv[2]);

  /* Do generic initialisation routine */

  fbusinit(NULL);

  /* Now retrieve the requested entries. */

  for (count = start_entry; count <= end_entry; count ++) {

    entry.Location=count;

    error=GSM->GetMemoryLocation(&entry);

    if (error == GE_NONE)
    {
      fprintf(stdout, "%s;%s;%s;%d;%d\n", entry.Name, entry.Number, memory_type_string, entry.Location, entry.Group);
      if (entry.MemoryType == GMT_MC || entry.MemoryType == GMT_DC || entry.MemoryType == GMT_RC)
        fprintf(stdout, "%02u.%02u.%04u %02u:%02u:%02u\n", entry.Date.Day, entry.Date.Month, entry.Date.Year, entry.Date.Hour, entry.Date.Minute, entry.Date.Second);
    }  
    else {
      if (error == GE_NOTIMPLEMENTED) {
	fprintf(stderr, _("Function not implemented in %s model!\n"), model);
	GSM->Terminate();
	return -1;
      }
      else if (error == GE_INVALIDMEMORYTYPE) {
	fprintf(stderr, _("Memory type %s not supported!\n"), memory_type_string);
	GSM->Terminate();
	return -1;
      }

      fprintf(stdout, _("%s|%d|Bad location or other error!(%d)\n"), memory_type_string, count, error);
    }
  }
	
  GSM->Terminate();

  return 0;
}

int GetLine(FILE *File, char *Line) {

  char *ptr;

  if (fgets(Line, 99, File)) {
    ptr=Line+strlen(Line)-1;

    while ( (*ptr == '\n' || *ptr == '\r') && ptr>=Line)
      *ptr--='\0';

      return strlen(Line);
  }
  else
    return 0;
}

/* Read data from stdin, parse and write to phone.  The parsing is relatively
   crude and doesn't allow for much variation from the stipulated format. */

int writephonebook(void)
{

  GSM_PhonebookEntry entry;
  GSM_Error error;
  char *memory_type_string;
  int line_count=0;

  char *Line, OLine[100], BackLine[100];
  char *ptr;

  /* Initialise fbus code */

  fbusinit(NULL);

  Line = OLine;

  /* Go through data from stdin. */

  while (GetLine(stdin, Line)) {

    strcpy(BackLine, Line);

    line_count++;

    ptr=strsep(&Line, ";"); strcpy(entry.Name, ptr);

    ptr=strsep(&Line, ";"); strcpy(entry.Number, ptr);

    ptr=strsep(&Line, ";");

    if (!strncmp(ptr,"ME", 2)) {
      memory_type_string = "int";
      entry.MemoryType = GMT_ME;
    }
    else {
      if (!strncmp(ptr,"SM", 2)) {
	memory_type_string = "sim";
	entry.MemoryType = GMT_SM;
      }
      else {
	fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, BackLine);
        break;
      }
    }

    ptr=strsep(&Line, ";"); entry.Location=atoi(ptr);

    ptr=strsep(&Line, ";"); entry.Group=atoi(ptr);
    Line = OLine;

    /* Do write and report success/failure. */

    error = GSM->WritePhonebookLocation(&entry);

    if (error == GE_NONE)
      fprintf (stdout, _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), memory_type_string, entry.Location, entry.Name, entry.Number);
    else
      fprintf (stdout, _("Write FAILED(%d): memory type: %s, loc: %d, name: %s, number: %s\n"), error, memory_type_string, entry.Location, entry.Name, entry.Number);

  }

  GSM->Terminate();

  return 0;
}

/* Getting speed dials. */

int getspeeddial(char *Number) {

  GSM_SpeedDial entry;

  entry.Number = atoi(Number);

  fbusinit(NULL);

  if (GSM->GetSpeedDial(&entry)==GE_NONE) {
    fprintf(stdout, _("SpeedDial nr. %d: %d:%d\n"), entry.Number, entry.MemoryType, entry.Location);
  }

  GSM->Terminate();

  return 0;
}

/* Setting speed dials. */

int setspeeddial(char *argv[]) {

  GSM_SpeedDial entry;

  char *memory_type_string;

  /* Handle command line args that set type, start and end locations. */

  if (strcmp(argv[1], "ME") == 0) {
    entry.MemoryType = 0x02;
    memory_type_string = "ME";
  }
  else if (strcmp(argv[1], "SM") == 0) {
    entry.MemoryType = 0x03;
    memory_type_string = "SM";
  }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[1]);

    return -1;
  }

  entry.Number = atoi(argv[0]);
  entry.Location = atoi(argv[2]);

  fbusinit(NULL);

  if (GSM->SetSpeedDial(&entry) == GE_NONE) {
    fprintf(stdout, _("Succesfully written!\n"));
  }

  GSM->Terminate();

  return 0;
}

void readconfig(void)
{

  struct CFG_Header *cfg_info;
  char *homedir;
  char rcfile[200];

#ifdef WIN32
  homedir = getenv("HOMEDRIVE");
  strncpy(rcfile, homedir ? homedir : "", 200);
  homedir = getenv("HOMEPATH");
  strncat(rcfile, homedir ? homedir : "", 200);
  strncat(rcfile, "\\_gnokiirc", 200);
#else
  homedir = getenv("HOME");

  if (homedir)
    strncpy(rcfile, homedir, 200);
  strncat(rcfile, "/.gnokiirc", 200);
#endif

  if ( (cfg_info = CFG_ReadFile("/etc/gnokiirc")) == NULL )
    if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
      fprintf(stderr, _("error opening %s, using default config\n"), rcfile);

  model = CFG_Get(cfg_info, "global", "model");
  if (!model)
    model = DefaultModel;

  Port = CFG_Get(cfg_info, "global", "port");
  if (!Port)
    Port = DefaultPort;

  Initlength = CFG_Get(cfg_info, "global", "initlength");
  if (!Initlength)
    Initlength = "default";

  Connection = CFG_Get(cfg_info, "global", "connection");
  if (!Connection)
    Connection = DefaultConnection;
}

/* Getting the status of the display. */

int getdisplaystatus()
{ 

  int Status;

  /* Initialise the code for the GSM interface. */     

  fbusinit(NULL);

  GSM->GetDisplayStatus(&Status);

  printf(_("Call in progress: %s\n"), Status & (1<<DS_Call_In_Progress)?_("on"):_("off"));
  printf(_("Unknown: %s\n"),          Status & (1<<DS_Unknown)?_("on"):_("off"));
  printf(_("Unread SMS: %s\n"),       Status & (1<<DS_Unread_SMS)?_("on"):_("off"));
  printf(_("Voice call: %s\n"),       Status & (1<<DS_Voice_Call)?_("on"):_("off"));
  printf(_("Fax call active: %s\n"),  Status & (1<<DS_Fax_Call)?_("on"):_("off"));
  printf(_("Data call active: %s\n"), Status & (1<<DS_Data_Call)?_("on"):_("off"));
  printf(_("Keyboard lock: %s\n"),    Status & (1<<DS_Keyboard_Lock)?_("on"):_("off"));
  printf(_("SMS storage full: %s\n"), Status & (1<<DS_SMS_Storage_Full)?_("on"):_("off"));

  GSM->Terminate();

  return 0;
}

int netmonitor(char *Mode)
{

  unsigned char mode=atoi(Mode);
  char Screen[50];

  fbusinit(NULL);

  if (!strcmp(Mode,"reset"))
    mode=0xf0;
  else if (!strcmp(Mode,"off"))
         mode=0xf1;
  else if (!strcmp(Mode,"field"))
         mode=0xf2;
  else if (!strcmp(Mode,"devel"))
         mode=0xf3;
  else if (!strcmp(Mode,"next"))
         mode=0x00;

  GSM->NetMonitor(mode, Screen);

  if (Screen)
    printf("%s\n", Screen);

  GSM->Terminate();

  return 0;
}

int identify( void )
{
  /* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
  char imei[64], model[64], rev[64];

  fbusinit(NULL);

  while (GSM->GetIMEI(imei)    != GE_NONE)
    sleep(1);

  while (GSM->GetRevision(rev) != GE_NONE)
    sleep(1);

  while (GSM->GetModel(model)  != GE_NONE)
    sleep(1);

  fprintf(stdout, _("IMEI:     %s\n"), imei);
  fprintf(stdout, _("Model:    %s\n"), model);
  fprintf(stdout, _("Revision: %s\n"), rev);

  GSM->Terminate();

  return 0;
}

int senddtmf(char *String)
{

  fbusinit(NULL);

  GSM->SendDTMF(String);

  GSM->Terminate();

  return 0;
}

/* Resets the phone */
int reset( char *type)
{

  unsigned char _type = 0x03;

  if(type) {
    if(!strcmp(type,"soft"))
      _type = 0x03;
    else
      if(!strcmp(type,"hard"))
	_type = 0x04;
      else {
	fprintf(stderr, _("What kind of reset do you want??\n"));
	return -1;
      }
  }

  fbusinit(NULL);

  GSM->Reset(_type);
  GSM->Terminate();

  return 0;
}

/* This is a "convenience" function to allow quick test of new API stuff which
   doesn't warrant a "proper" command line function. */

#ifndef WIN32
int foogle(char *argv[])
{

  /* Initialise the code for the GSM interface. */     

  fbusinit(NULL);

//  Uncomment this for testing RTTTLsending... rename it to rTTTl...
//  FB61_SendRingtoneRTTL("/tmp/q.txt");

  sleep(5);

  GSM->Terminate();

  return 0;
}
#endif

/* pmon allows fbus code to run in a passive state - it doesn't worry about
   whether comms are established with the phone.  A debugging/development
   tool. */

int pmon()
{ 

  GSM_Error error;
  GSM_ConnectionType connection=GCT_Serial;

  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise(model, Port, Initlength, connection, RLP_DisplayF96Frame);

  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    return -1;
  }


  while (1) {
    usleep(50000);
  }

  return 0;
}

/* FIXME: this can not be here... */
int FB61_PackRingtoneRTTL(unsigned char *req, char *FileName);

int sendringtone(int argc, char *argv[])
{
  GSM_SMSMessage SMS;
  GSM_Error error;

  int size;
  unsigned char req[255];

  /* Default settings for SMS message:
      - no delivery report
      - Class Message 1
      - no compression
      - 8 bit data
      - SMSC no. 1
      - validity 3 days
      - set UserDataHeaderIndicator
  */

  SMS.Type = GST_MO;
  SMS.Class = 1;
  SMS.Compression = false;
  SMS.EightBit = true;
  SMS.MessageCenter.No = 1;
  SMS.Validity = 4320; /* 4320 minutes == 72 hours */

  SMS.UDHType = GSM_Ringtone;

  /* The first argument is the destionation, ie the phone number of recipient. */
  strcpy(SMS.Destination,argv[0]);

  /* The second argument is the RTTTL file. */
  size=FB61_PackRingtoneRTTL(req, argv[1]);

  memcpy(SMS.UDH,req+2,7);
  memcpy(SMS.MessageText,req+9,size-9);

  /* Initialise the GSM interface. */
  fbusinit(NULL);

  /* Send the message. */
  error = GSM->SendSMSMessage(&SMS,size-9);

  if (error == GE_SMSSENDOK)
    fprintf(stdout, _("Send succeeded!\n"));
  else
    fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

  GSM->Terminate();
  return 0;
}
