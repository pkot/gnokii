/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries.

  Warning: this code is only the test tool. It is not intented to real work -
  wait for GUI application.

  Last modification: Sun May 16 21:04:03 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "gsm-networks.h"
#include "cfgreader.h"

/* Prototypes. */
void monitormode(void);
void enterpin(void);
void getmemory(char *argv[]);
void writephonebook(void);
void getsms(char *argv[]);
void deletesms(char *argv[]);
void sendsms(int argc, char *argv[]);
void getsmsc(char *argv[]);
void setdatetime(int argc, char *argv[]);
void getdatetime(void);
void setalarm(char *argv[]);
void getalarm(void);
void dialvoice(char *argv[]);
void dialdata(char *argv[]);
void sendoplogo(char *argv[]);
void sendclicon(char *argv[]);
void getcalendarnote(char *argv[]);
void writecalendarnote(char *argv[]);
void deletecalendarnote(char *argv[]);
void foogle(char *argv[]);
void readconfig(void);

char		*Model;		/* Model from .gnokiirc file. */
char		*Port;		/* Serial port from .gnokiirc file */
char 		*Initlength;	/* Init length from .gnokiirc file */
char		*Connection;	/* Connection type from .gnokiirc file */

	/* Local variables */
char		*DefaultModel = MODEL;	/* From Makefile */
char		*DefaultPort = PORT;

char		*DefaultConnection = "serial";

void version(void)
{

  fprintf(stdout, _("GNOKII Version %s
Copyright (C) Hugh Blemings <hugh@vsb.com.au>, 1999
Copyright (C) Pavel Janík ml. <Pavel.Janik@linux.cz>, 1999
Built %s %s for %s on %s \n"), VERSION, __TIME__, __DATE__, Model, Port);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options.*/

void usage(void)
{

  fprintf(stdout, _("   usage: gnokii {--help|--monitor|--version}
          gnokii [--getmemory] [memory type] [start] [end]
          gnokii [--writephonebook]
          gnokii [--getsms] [memory type] [start] [end]
          gnokii [--deletesms] [memory type] [start] [end]
          gnokii [--sendsms] [destination] [--smsc message_center_number |
                  --smscno message_center_index] [-r] [-C n] [-v n]
          gnokii [--getsmsc] [message_center_number]
          gnokii [--setdatetime] [YYYY MM DD HH MM]]
          gnokii [--getdatetime]
          gnokii [--setalarm] [HH] [MM]
          gnokii [--getalarm]
          gnokii [--dialvoice] [number]
          gnokii [--dialdata] [number]
          gnokii [--sendoplogo] [logofile] [network code]
          gnokii [--sendclicon] [logofile]
          gnokii [--getcalendarnote] [index]
          gnokii [--writecalendarnote]
          gnokii [--deletecalendarnote] [index]

          --help            display usage information.

          --monitor         continually updates phone status to stderr.

          --version         displays version and copyright information.

          --enterpin        sends the entered PIN to the mobile phone.

          --getmemory       reads specificed memory location from phone.
                            Valid memory types are:
                            ME, SM, FD, ON, EN, DC, RC, MC, LD

          --writephonebook  reads data from stdin and writes to phonebook.
                            Uses the same format as provided by the output of
                            the getphonebook command.

          --getsms          gets SMS messages from specified memory type
                            starting at entry [start] and ending at [end].
                            Entries are dumped to stdout.

          --deletesms       deletes SMS messages from specified memory type
                            starting at entry [start] and ending at [end].

          --sendsms         sends an SMS message to [destination] via
                            [message_center_number] or SMSC number taken from
                            phone memory from address [message_center_index].
                            If this argument is ommited SMSC number is taken
                            from phone memory from location 1. Message text
                            is taken from stdin.  This function has had
                            limited testing and may not work at all on your
                            network. Meaning of other optional parameters:
                             [-r] - request for delivery report
                             [-C n] - Class Message n, where n can be 0..3
                             [-v n] - validity in minutes

          --getsmsc         show the SMSC number from location
                            [message_center_number].

          --setdatetime     set the date and the time of the phone.

          --getdatetime     shows current date and time in the phone.

          --setalarm        set the alarm of the phone.

          --getalarm        shows current alarm.

          --dialvoice       initiate voice call.

          --dialdata        initiate data call.

          --sendoplogo      send operator logo.

          --sendclicon      send CL icon.

          --getcalendarnote get the note with number [index] from calendar.

          --writecalendarnote write the note to calendar.

          --deletecalendarnote  delete the note with number [index]
                                from calendar.
"));
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

void fbusinit(bool enable_monitoring)
{
  int count=0;
  GSM_Error error;
  GSM_ConnectionType connection=GCT_Serial;

  if (!strcmp(Connection, "infrared")) {
    connection=GCT_Infrared;
  }
    
  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise(Model, Port, Initlength, connection, enable_monitoring);

  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    exit(-1);
  }

  /* First (and important!) wait for GSM link to be active. We allow 10
     seconds... */

  while (count++ < 200 && *GSM_LinkOK == false)
    usleep(50000);

  if (*GSM_LinkOK == false) {
    fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting. \n"));
    exit(-1);
  }
}

int ReadBitmapFile(char *FileName, char *NetworkCode, unsigned char *logo, int *width, int *height)
{

  FILE *file;
  unsigned char buffer[2000];
  int i,j;

  file = fopen(FileName, "r");

  if (!file)
    return(-1);
  
  fread(buffer, 1, 6, file); /* Read the header of the file. */

  if (!strncmp(buffer, "NOL", 3)) { /* NOL files have network code in the header. */
    fread(buffer, 1, 4, file);
    if (NetworkCode)
      sprintf(NetworkCode, "%d %02d", buffer[0]+256*buffer[1], buffer[2]);
  }

  fread(buffer, 1, 4, file); /* Width and height of the icon. */
  *width=buffer[0];
  *height=buffer[2];

  fread(buffer, 1, 6, file); /* Unknown bytes. */

  for (i=0; i<( (*width * *height) /8); i++) {
    fread(buffer, 1, 8, file);

    for (j=7; j>=0;j--)
      if (buffer[7-j] == '1') logo[i]|=(1<<j);
  }

  fclose(file);

  return 0;
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{

  /* For GNU gettext */

#ifdef GNOKII_GETTEXT
  textdomain("gnokii");
#endif

  /* Read config file */
  readconfig();

  /* Handle command line arguments. */

  if (argc == 1 || !strcmp(argv[1], "--help")) {
    usage();
    exit(0);
  }

  /* Display version, copyright and build information. */

  if (strcmp(argv[1], "--version") == 0) {
    version();
    exit(0);
  }

  /* Enter monitor mode. */

  if (strcmp(argv[1], "--monitor") == 0) {
    monitormode();
  }

  /* Enter pin. */

  if (strcmp(argv[1], "--enterpin") == 0) {
    enterpin();
  }

  /* Set Date and Time. */
  if (strcmp(argv[1], "--setdatetime") == 0 && (argc==7 || argc==2)) {
    setdatetime(argc, argv);
  }

  /* Get date/time mode. */
  if (strcmp(argv[1], "--getdatetime") == 0) {
    getdatetime();
  }

  /* Set Alarm. */
  if (strcmp(argv[1], "--setalarm") == 0 && argc==4) {
    setalarm(argv);
  }

  /* Get alarm mode. */
  if (strcmp(argv[1], "--getalarm") == 0) {
    getalarm();
  }

  /* Voice call mode. */
  if (strcmp(argv[1], "--dialvoice") == 0) {
    dialvoice(argv);
  }

  /* Data call mode. */
  if (strcmp(argv[1], "--dialdata") == 0) {
    dialdata(argv);
  }

  /* Send operator log mode. */
  if (strcmp(argv[1], "--sendoplogo") == 0) {
    sendoplogo(argv);
  }

  /* Send CL icon mode. */
  if (strcmp(argv[1], "--sendclicon") == 0) {
    sendclicon(argv);
  }

  /* Get calendar note mode. */
  if (argc == 3 && strcmp(argv[1], "--getcalendarnote") == 0) {
    getcalendarnote(argv);
  }

  /* Write calendar note mode. */
  if (strcmp(argv[1], "--writecalendarnote") == 0) {
    writecalendarnote(argv);
  }

  /* Delete calendar note mode. */
  if (argc == 3 && strcmp(argv[1], "--deletecalendarnote") == 0) {
    deletecalendarnote(argv);
  }

  /* Get memory command. */
  if (argc == 5 && strcmp(argv[1], "--getmemory") == 0)
    getmemory(argv);

  /* Write phonebook command. */
  if (strcmp(argv[1], "--writephonebook") == 0)
    writephonebook();

  /* Get sms message mode. */
  if (argc == 5 && strcmp(argv[1], "--getsms") == 0)
    getsms(argv);

  /* Delete sms message mode. */
  if (argc == 5 && strcmp(argv[1], "--deletesms") == 0)
    deletesms(argv);

  /* Send sms message mode. */
  if (argc >=3 && strcmp(argv[1], "--sendsms") == 0)
    sendsms(argc, argv);

  /* Get smsc number mode. */
  if (argc == 3 && strcmp(argv[1], "--getsmsc") == 0)
    getsmsc(argv);

    /* Foogle function - insert you own function calls here 
     when testing stuff.  This is for the developer/hackers
     convenience only :) */
  if (strcmp(argv[1], "--foogle") == 0) {
	  foogle(argv);
  }

  /* Either an unknown command or wrong number of args! */
  usage();

  exit (-1);
}

/* Send  SMS messages. */
void sendsms(int argc, char *argv[])
{

  GSM_SMSMessage SMS;
  GSM_Error error;
  char message_buffer[200];	
  int chars_read;
  int i;

  struct option options[] = {
             { "sendsms", required_argument, NULL, '1'},
             { "smsc",    required_argument, NULL, '2'},
             { "smscno",  required_argument, NULL, '3'},
             { NULL,      0,                 NULL, 0}
  };

  /* Get message text from stdin. */

  chars_read = fread(message_buffer, 1, 160, stdin);

  if (chars_read == 0) {
    fprintf(stderr, _("Couldn't read from stdin!\n"));	
    exit (1);
  }

  /*  Null terminate. */

  message_buffer[chars_read] = 0x00;	

  /* Default settings:
      - no delivery report
      - no Class Message
      - no compression
      - 7 bit data
      - SMSC no. 1
  */

  SMS.Type = GST_MO;
  SMS.Class = -1;
  SMS.Compression = false;
  SMS.EightBit = false;
  SMS.MessageCenter.No = 1;
  SMS.Validity = 4320; /* 4320 minutes == 72 hours */

  while ((i = getopt_long(argc, argv, "r8cC:v:", options, NULL)) != EOF) {
    switch (i) {

      case '1': /* Remote number */
        strcpy(SMS.Destination,argv[2]);
        break;

      case '2': /* SMSC number */
        SMS.MessageCenter.No = 0;
        strcpy(SMS.MessageCenter.Number,optarg);
        break;

      case '3': /* SMSC number index in phone memory */
        SMS.MessageCenter.No = atoi(optarg);
        if (SMS.MessageCenter.No < 1 || SMS.MessageCenter.No > 5) {
          usage();
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
            exit(-1);

        }
        break;
      
      case 'v':
        SMS.Validity = atoi(optarg);
        break;

      default:
        usage();
        exit(-1);
    }
  }

  /* Initialise the GSM interface. */     

  fbusinit(true);

  strcpy (SMS.MessageText, message_buffer);

  /* Send the message. */

  error = GSM->SendSMSMessage(&SMS);

  if (error == GE_SMSSENDOK)
    fprintf(stdout, _("Send succeeded!\n"));
  else
    fprintf(stdout, _("SMS Send failed (error=%d)\n"), error);

  GSM->Terminate();
  exit(0);
}

/* Get SMSC number */

void getsmsc(char *argv[])
{

  GSM_MessageCenter MessageCenter;
  
  MessageCenter.No=atoi(argv[2]);

  fbusinit(false);

  if (GSM->GetSMSCenter(&MessageCenter) == GE_NONE)
    fprintf(stdout, _("%d. SMS center number is %s\n"), MessageCenter.No, MessageCenter.Number);
  else
    fprintf(stdout, _("SMS center can not be found :-(\n"));

  GSM->Terminate();
  exit(0);
}

/* Get SMS messages. */
void getsms(char *argv[])
{

  GSM_SMSMessage message;
  char *memory_type_string;
  int start_message, end_message, count;
  GSM_Error error;

  /* Handle command line args that set type, start and end locations. */

  /* FIXME: This is done more than once in gnokii, should be in some function
     or should not be here at all if we can not store SMS in different types
     of memory */

  if (strcmp(argv[2], "ME") == 0) {
    message.MemoryType = GMT_ME;
    memory_type_string = "ME";
  }
  else
    if (strcmp(argv[2], "SM") == 0) {
      message.MemoryType = GMT_SM;
      memory_type_string = "SM";
    }
  else
    if (strcmp(argv[2], "FD") == 0) {
      message.MemoryType = GMT_FD;
      memory_type_string = "FD";
    }
  else
    if (strcmp(argv[2], "ON") == 0) {
      message.MemoryType = GMT_ON;
      memory_type_string = "ON";
    }
  else
    if (strcmp(argv[2], "EN") == 0) {
      message.MemoryType = GMT_EN;
      memory_type_string = "EN";
    }
  else
    if (strcmp(argv[2], "DC") == 0) {
      message.MemoryType = GMT_DC;
      memory_type_string = "DC";
    }
  else
    if (strcmp(argv[2], "RC") == 0) {
      message.MemoryType = GMT_RC;
      memory_type_string = "RC";
     }
  else
    if (strcmp(argv[2], "MC") == 0) {
      message.MemoryType = GMT_MC;
      memory_type_string = "MC";
    }
  else
    if (strcmp(argv[2], "LD") == 0) {
      message.MemoryType = GMT_LD;
      memory_type_string = "LD";
    }
  else
    if (strcmp(argv[2], "MT") == 0) {
      message.MemoryType = GMT_MT;
      memory_type_string = "MT";
    }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[2]);
    exit (-1);
  }

  start_message = atoi(argv[3]);
  end_message = atoi(argv[4]);

  /* Initialise the code for the GSM interface. */     

  fbusinit(true);

  /* Now retrieve the requested entries. */

  for (count = start_message; count <= end_message; count ++) {
	
    error = GSM->GetSMSMessage(count, &message);

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

          fprintf(stdout, _("Sending date/time: %d/%d/%d %d:%02d:%02d GMT"), \
                  message.Time.Day, message.Time.Month, message.Time.Year, \
                  message.Time.Hour, message.Time.Minute, message.Time.Second);

          if (message.Time.Timezone) {
            if (message.Time.Timezone > 0)
              fprintf(stdout,_("+%dh"), message.Time.Timezone);
            else
              fprintf(stdout,_("%d"), message.Time.Timezone);
          }

          fprintf(stdout, _("\n"));

          fprintf(stdout, _("Response date/time: %d/%d/%d %d:%02d:%02d GMT"), \
                  message.Time.Day, message.Time.Month, message.Time.Year, \
                  message.Time.Hour, message.Time.Minute, message.Time.Second);

          if (message.Time.Timezone) {
            if (message.Time.Timezone > 0)
              fprintf(stdout,_("+%dh"),message.Time.Timezone);
            else
              fprintf(stdout,_("%d"),message.Time.Timezone);
          }

          fprintf(stdout, _("\n"));

          fprintf(stdout, _("Receiver: %s Msg Center: %s\n"), message.Sender, message.MessageCenter.Number);
          fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 

          break;

        default:

          fprintf(stdout, _("%d. Inbox Message "), message.MessageNumber);

          if (message.Status)
            fprintf(stdout, _("(read)\n"));
          else
            fprintf(stdout, _("(not read)\n"));

          fprintf(stdout, _("Date/time: %d/%d/%d %d:%02d:%02d GMT"), \
                  message.Time.Day, message.Time.Month, message.Time.Year, \
                  message.Time.Hour, message.Time.Minute, message.Time.Second);

          if (message.Time.Timezone) {
            if (message.Time.Timezone > 0)
              fprintf(stdout,_("+%dh"),message.Time.Timezone);
            else
              fprintf(stdout,_("%d"),message.Time.Timezone);
          }

          fprintf(stdout, _("\n"));

          fprintf(stdout, _("Sender: %s Msg Center: %s\n"), message.Sender, message.MessageCenter.Number);
          fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 

          break;
      }

      break;

    case GE_NOTIMPLEMENTED:

      fprintf(stderr, _("Function not implemented in %s model!\n"), Model);
      GSM->Terminate();
      exit(-1);	

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
  exit(0);
}

/* Delete SMS messages. */
void deletesms(char *argv[])
{

  GSM_SMSMessage message;
  char *memory_type_string;
  int start_message, end_message, count;
  GSM_Error error;

  /* Handle command line args that set type, start and end locations. */

  if (strcmp(argv[2], "ME") == 0) {
    message.MemoryType = GMT_ME;
    memory_type_string = "ME";
  }
  else
    if (strcmp(argv[2], "SM") == 0) {
      message.MemoryType = GMT_SM;
      memory_type_string = "SM";
    }
  else
    if (strcmp(argv[2], "FD") == 0) {
      message.MemoryType = GMT_FD;
      memory_type_string = "FD";
    }
  else
    if (strcmp(argv[2], "ON") == 0) {
      message.MemoryType = GMT_ON;
      memory_type_string = "ON";
    }
  else
    if (strcmp(argv[2], "EN") == 0) {
      message.MemoryType = GMT_EN;
      memory_type_string = "EN";
    }
  else
    if (strcmp(argv[2], "DC") == 0) {
      message.MemoryType = GMT_DC;
      memory_type_string = "DC";
    }
  else
    if (strcmp(argv[2], "RC") == 0) {
      message.MemoryType = GMT_RC;
      memory_type_string = "RC";
     }
  else
    if (strcmp(argv[2], "MC") == 0) {
      message.MemoryType = GMT_MC;
      memory_type_string = "MC";
    }
  else
    if (strcmp(argv[2], "LD") == 0) {
      message.MemoryType = GMT_LD;
      memory_type_string = "LD";
    }
  else
    if (strcmp(argv[2], "MT") == 0) {
      message.MemoryType = GMT_MT;
      memory_type_string = "MT";
    }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[2]);
    exit (-1);
  }

  start_message = atoi (argv[3]);
  end_message = atoi (argv[4]);

  /* Initialise the code for the GSM interface. */     

  fbusinit(true);

  /* Now delete the requested entries. */

  for (count = start_message; count <= end_message; count ++) {

    error = GSM->DeleteSMSMessage(count, &message);

    if (error == GE_NONE)
      fprintf(stdout, _("Deleted SMS %s %d\n"), memory_type_string, count);
    else {
      if (error == GE_NOTIMPLEMENTED) {
	fprintf(stderr, _("Function not implemented in %s model!\n"), Model);
	GSM->Terminate();
	exit(-1);	
      }
      fprintf(stdout, _("DeleteSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
    }
  }

  GSM->Terminate();
  exit(0);
}

static volatile bool shutdown = false;

/* SIGINT signal handler. */

static void interrupted(int sig)
{

  signal(sig, SIG_IGN);
  shutdown = true;
}

/* In this mode we get the pin from the keyboard and send it to the mobile
   phone */

void enterpin(void)
{

  char *pin=getpass(_("Enter your PIN: "));

  fbusinit(false);

  if (GSM->EnterPin(pin) == GE_INVALIDPIN)
    fprintf(stdout, _("Error: invalid PIN\n"));
  else
    fprintf(stdout, _("PIN ok.\n"));

  GSM->Terminate();
  exit(0);
}

void dialvoice(char *argv[])
{

  fbusinit(false);

  if (argv[2])
    GSM->DialVoice(argv[2]);

  GSM->Terminate();
  exit(0);
}

void dialdata(char *argv[])
{

  fbusinit(false);

  sleep(10);

  if (argv[2])
    GSM->DialData(argv[2]);

  sleep(10);

  GSM->Terminate();
  exit(0);
}

void sendoplogo(char *argv[])
{

  char *NetworkCode=(char *)malloc(10);
  unsigned char *logo=(unsigned char *)calloc(1,256);
  int width, height;

  if (argv[2] && !ReadBitmapFile(argv[2], NetworkCode, logo, &width, &height)) {
    fbusinit(false);

    if (argv[3])
      NetworkCode=argv[3];

    fprintf(stdout, _("Sending operator logo for %s\n"), NetworkCode);
    GSM->SendBitmap(NetworkCode, width, height, logo);
  }
  else {
    fprintf(stdout, _("File does not exist or can not be opened\n"));
    exit(-1);
  }

  GSM->Terminate();
  exit(0);
}

void sendclicon(char *argv[])
{

  unsigned char *logo=(unsigned char *)calloc(1,256);
  int width, height;

  if (argv[2] && !ReadBitmapFile(argv[2], NULL, logo, &width, &height)) {
    fbusinit(false);
    GSM->SendBitmap(NULL, width, height, logo);
  }
  else {
    fprintf(stdout, _("File does not exist or can not be opened\n"));
    exit(-1);
  }

  GSM->Terminate();
  exit(0);
}

void getcalendarnote(char *argv[])
{

  GSM_CalendarNote CalendarNote;

  CalendarNote.Location=atoi(argv[2]);

  fbusinit(false);

  if (GSM->GetCalendarNote(&CalendarNote) == GE_NONE) {

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
  else {
    fprintf(stderr, _("The calendar note can not be read\n"));

    GSM->Terminate();
    exit(-1);
  }

  GSM->Terminate();
  exit(0);
}

void writecalendarnote(char *argv[])
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

  sprintf(CalendarNote.Text, "Big day ...");
  sprintf(CalendarNote.Phone, "jhgjhgjhgjhgj");

  fbusinit(false);

  if (GSM->WriteCalendarNote(&CalendarNote) == GE_NONE) {
    fprintf(stdout, _("Succesfully written!"));
  }

  GSM->Terminate();
  exit(0);
}

void deletecalendarnote(char *argv[])
{

  GSM_CalendarNote CalendarNote;

  CalendarNote.Location=atoi(argv[2]);

  fbusinit(false);

  if (GSM->DeleteCalendarNote(&CalendarNote) == GE_NONE) {
    fprintf(stdout, _("   Calendar note deleted.\n"));
  }
  else {
    fprintf(stderr, _("The calendar note can not be deleted\n"));

    GSM->Terminate();
    exit(-1);
  }

  GSM->Terminate();
  exit(0);
}


void setdatetime(int argc, char *argv[])
{
  struct tm *now;
  time_t nowh;
  GSM_DateTime Date;

  fbusinit(false);

  if (argc==7) {
    Date.Year = atoi (argv[2]);
    Date.Month = atoi (argv[3]);
    Date.Day = atoi (argv[4]);
    Date.Hour = atoi (argv[5]);
    Date.Minute = atoi (argv[6]);
    }
  else {
    nowh=time(NULL);
    now=localtime(&nowh);
    if (now->tm_year>90) {
      Date.Year = now->tm_year+1900;
      }
    else {
      Date.Year = now->tm_year+2000;
      } 
    Date.Month = now->tm_mon+1;
    Date.Day = now->tm_mday;
    Date.Hour = now->tm_hour;
    Date.Minute = now->tm_min;
    Date.Second = now->tm_sec;
    }

  GSM->SetDateTime(&Date);

  GSM->Terminate();
  exit(0);
}

/* In this mode we receive the date and time from mobile phone */

void getdatetime(void) {

  GSM_DateTime date_time;

  fbusinit(false);

  if (GSM->GetDateTime(&date_time)==GE_NONE) {
    fprintf(stdout, _("Date: %4d/%02d/%02d\n"), date_time.Year, date_time.Month, date_time.Day);
    fprintf(stdout, _("Time: %02d:%02d:%02d\n"), date_time.Hour, date_time.Minute, date_time.Second);
  }

  GSM->Terminate();
  exit(0);
}

void setalarm(char *argv[])
{

  GSM_DateTime Date;

  fbusinit(false);

  Date.Hour = atoi(argv[2]);
  Date.Minute = atoi(argv[3]);

  GSM->SetAlarm(1, &Date);

  GSM->Terminate();
  exit(0);
}

void getalarm(void) {

  GSM_DateTime date_time;

  fbusinit(false);

  if (GSM->GetAlarm(0, &date_time)==GE_NONE) {
    fprintf(stdout, _("Alarm: %s\n"), (date_time.AlarmEnabled==0)?"off":"on");
    fprintf(stdout, _("Time: %02d:%02d\n"), date_time.Hour, date_time.Minute);
  }

  GSM->Terminate();
  exit(0);
}

/* In monitor mode we don't do much, we just initialise the fbus code with
   monitoring enabled and print status informations. */

void monitormode(void)
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

  fbusinit(true);

  /* Loop here indefinitely - allows you to see messages from GSM code in
     response to unknown messages etc. The loops ends after pressing the
     Ctrl+C. */

  while (!shutdown) {

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
  exit(0);
}

/* Get requested range of memory storage entries and output to stdout in
   easy-to-parse format */

void getmemory(char *argv[])
{

  GSM_PhonebookEntry entry;
  int count;
  GSM_Error error;
  char *memory_type_string;
  int start_entry;
  int end_entry;

  /* Handle command line args that set type, start and end locations. */

  if (strcmp(argv[2], "ME") == 0) {
    entry.MemoryType = GMT_ME;
    memory_type_string = "ME";
  }
  else
    if (strcmp(argv[2], "SM") == 0) {
      entry.MemoryType = GMT_SM;
      memory_type_string = "SM";
    }
  else
    if (strcmp(argv[2], "FD") == 0) {
      entry.MemoryType = GMT_FD;
      memory_type_string = "FD";
    }
  else
    if (strcmp(argv[2], "ON") == 0) {
      entry.MemoryType = GMT_ON;
      memory_type_string = "ON";
    }
  else
    if (strcmp(argv[2], "EN") == 0) {
      entry.MemoryType = GMT_EN;
      memory_type_string = "EN";
    }
  else
    if (strcmp(argv[2], "DC") == 0) {
      entry.MemoryType = GMT_DC;
      memory_type_string = "DC";
    }
  else
    if (strcmp(argv[2], "RC") == 0) {
      entry.MemoryType = GMT_RC;
      memory_type_string = "RC";
     }
  else
    if (strcmp(argv[2], "MC") == 0) {
      entry.MemoryType = GMT_MC;
      memory_type_string = "MC";
    }
  else
    if (strcmp(argv[2], "LD") == 0) {
      entry.MemoryType = GMT_LD;
      memory_type_string = "LD";
    }
  else
    if (strcmp(argv[2], "MT") == 0) {
      entry.MemoryType = GMT_MT;
      memory_type_string = "MT";
    }
  else {
    fprintf(stderr, _("Unknown memory type %s!\n"), argv[2]);
    exit (-1);
  }

  start_entry = atoi (argv[3]);
  end_entry = atoi (argv[4]);

  /* Do generic initialisation routine, monitoring disabled. */

  fbusinit(false);

  /* Now retrieve the requested entries. */

  for (count = start_entry; count <= end_entry; count ++) {

    error=GSM->GetMemoryLocation(count, &entry);
 
    if (error == GE_NONE)
      fprintf(stdout, "%s;%s;%s;%d;%d\n", entry.Name, entry.Number, memory_type_string, count, entry.Group);
    else {
      if (error == GE_NOTIMPLEMENTED) {
	fprintf(stderr, _("Function not implemented in %s model!\n"), Model);
	GSM->Terminate();
	exit(-1);
      }
      else if (error == GE_INVALIDMEMORYTYPE) {
	fprintf(stderr, _("Memory type %s not supported!\n"), memory_type_string);
	GSM->Terminate();
	exit(-1);
      }

      fprintf(stdout, _("%s|%d|Bad location or other error!(%d)\n"), memory_type_string, count, error);
    }
  }
	
  GSM->Terminate();
  exit(0);
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

void writephonebook(void)
{

  GSM_PhonebookEntry entry;
  GSM_Error error;
  char *memory_type_string;
  int entry_number;
  int line_count=0;

  char Line[100], BackLine[100];
  char *ptr;

  /* Initialise fbus code, enable monitoring for debugging purposes... */

  fbusinit(true);

  /* Go through data from stdin. */

  while (GetLine(stdin, Line)) {

    strcpy(BackLine, Line);

    line_count++;

    ptr=strtok(Line, ";"); strcpy(entry.Name, ptr);

    ptr=strtok(NULL, ";"); strcpy(entry.Number, ptr);

    ptr=strtok(NULL, ";");

    if (!strncmp(ptr,"ME", 2)) {
      memory_type_string = "int";
      entry.MemoryType = GMT_ME;
    }
    else {
      if (!strncmp(ptr,"SM", 2)) {
	memory_type_string = "sim";
	entry.MemoryType = GMT_SM;
      }
      else
	fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, BackLine);
    }

    ptr=strtok(NULL, ";"); entry_number=atoi(ptr);

    ptr=strtok(NULL, ";"); entry.Group=atoi(ptr);

    /* Do write and report success/failure. */

    error = GSM->WritePhonebookLocation(entry_number, &entry);

    if (error == GE_NONE)
      fprintf (stdout, _("Write Succeeded: memory type: %s, loc: %d, name: %s, number: %s\n"), memory_type_string, entry_number, entry.Name, entry.Number);
    else
      fprintf (stdout, _("Write FAILED(%d): memory type: %s, loc: %d, name: %s, number: %s\n"), error, memory_type_string, entry_number, entry.Name, entry.Number);

  }

  GSM->Terminate();
  exit(0);
}

void	readconfig(void)
{
    struct CFG_Header 	*cfg_info;
	char				*homedir;
	char				rcfile[200];

	homedir = getenv("HOME");

	strncpy(rcfile, homedir, 200);
	strncat(rcfile, "/.gnokiirc", 200);

    if ( (cfg_info = CFG_ReadFile("/etc/gnokiirc")) == NULL )
       if ((cfg_info = CFG_ReadFile(rcfile)) == NULL)
		fprintf(stderr, _("error opening %s, using default config\n"), 
		  rcfile);

    Model = CFG_Get(cfg_info, "global", "model");
    if (Model == NULL) {
		Model = DefaultModel;
    }

    Port = CFG_Get(cfg_info, "global", "port");
    if (Port == NULL) {
		Port = DefaultPort;
    }

    Initlength = CFG_Get(cfg_info, "global", "initlength");
    if (Initlength == NULL) {
		Initlength = "default";
    }

    Connection = CFG_Get(cfg_info, "global", "connection");
    if (Connection == NULL) {
		Connection = DefaultConnection;
    }
}

	/* This is a "convenience" function to allow quick test of new
	   API stuff which doesn't warrant a "proper" command line
	   function.  Most recently used by Hugh to test 3810
	   GetSMSCenter function... */
extern GSM_Error	FB38_GetSMSCenter(GSM_MessageCenter *MessageCenter);
void foogle(char *argv[])
{ 
  GSM_MessageCenter msg_center;

  fbusinit(false);

  if (FB38_GetSMSCenter(&msg_center) != GE_NONE) {
    fprintf(stderr,"Bzzt.  Failed.\n");
  }
  else {
  	fprintf(stdout, "No: %d, Name:%s, Number:%s\n\r", 
	  	msg_center.No, msg_center.Name, msg_center.Number);		
  }

  GSM->Terminate();
  exit(0);
}

