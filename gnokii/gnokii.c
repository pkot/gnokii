/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) Hugh Blemings, 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Mainline code for gnokii utility.  Handles command line parsing and
  reading/writing phonebook entries.

  Warning: this code is only the test tool. It is not intented to real work -
  wait for GUI application.

  Last modification: Thu May  6 00:51:48 CEST 1999
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

#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"

/* Prototypes. */
void monitormode(void);
void enterpin(void);
void getphonebook(char *argv[]);
void writephonebook(void);
void getsms(char *argv[]);
void deletesms(char *argv[]);
void sendsms(char *argv[]);
void getdatetime(void);
void getalarm(void);
void dialvoice(char *argv[]);

void version(void)
{

  fprintf(stdout, _("GNOKII Version 0.2.4
Copyright (C) Hugh Blemings <hugh@vsb.com.au>, 1999
Copyright (C) Pavel Janík ml. <Pavel.Janik@linux.cz>, 1999
Built %s %s for %s on %s \n"), __TIME__, __DATE__, MODEL, PORT);
}

/* The function usage is only informative - it prints this program's usage and
   command-line options.*/

void usage(void)
{

  fprintf(stdout, _("   usage: gnokii {--help|--monitor|--version}
          gnokii [--getphonebook] [memory type] [start] [end]
          gnokii [--writephonebook]
          gnokii [--getsms] [memory type] [start] [end]
          gnokii [--deletesms] [memory type] [start] [end]
          gnokii [--sendsms] [destination] [message centre]
          gnokii [--getdatetime]
          gnokii [--getalarm]
          gnokii [--dialvoice] [number]

          --help            display usage information.

          --monitor         continually updates phone status to stderr.

          --version         displays version and copyright information.

          --enterpin        sends the entered PIN to the mobile phone.

          --getphonebook    gets phonebook entries from specified memory type
                            ('int' or 'sim') starting at entry [start] and
                            ending at [end].  Entries are dumped to stdout in
                            format suitable for editing and passing back to
                            writephonebook command.

          --writephonebook  reads data from stdin and writes to phonebook.
                            Uses the same format as provided by the output of
                            the getphonebook command.

          --getsms          gets SMS messages from specified memory type
                            ('int' or 'sim') starting at entry [start] and
                            ending at [end].  Entries are dumped to stdout.

          --deletesms       deletes SMS messages from specified memory type
                            ('int' or 'sim') starting at entry [start] and
                            ending at [end].

          --sendsms         sends an SMS message to [destination] via
                            [message centre].  Message text is taken from
                            stdin.  This function has had limited testing
                            and may not work at all on your network.

          --getdatetime     shows current date and time in the phone.

          --getalarm        shows current alarm.

          --dialvoice       initiate voice call.\n"));
}

/* fbusinit is the generic function which waits for the FBUS link. The limit
   is 10 seconds. After 10 seconds we quit. */

void fbusinit(bool enable_monitoring)
{
  int count=0;
  GSM_Error error;

  /* Initialise the code for the GSM interface. */     

  error = GSM_Initialise(MODEL, PORT, enable_monitoring);

  if (error != GE_NONE) {
    fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    exit(-1);
  }

  /* First (and important!) wait for GSM link to be active. We allow 10
     seconds... */

  while (count++ < 200 && *GSM_LinkOK == false)
    usleep(50000);

  if (GSM_LinkOK == false) {
    fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting. \n"));
    exit(-1);
  }
}

/* Main function - handles command line arguments, passes them to separate
   functions accordingly. */

int main(int argc, char *argv[])
{

  /* For GNU gettext */

#ifdef GNOKII_GETTEXT
  textdomain("gnokii");
#endif

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

  /* Get date/time mode. */
  if (strcmp(argv[1], "--getdatetime") == 0) {
    getdatetime();
  }

  /* Get alarm mode. */
  if (strcmp(argv[1], "--getalarm") == 0) {
    getalarm();
  }

  /* Voice call mode. */
  if (strcmp(argv[1], "--dialvoice") == 0) {
    dialvoice(argv);
  }

		/* Get phonebook command. */
	if (argc == 5 && strcmp(argv[1], "--getphonebook") == 0) {
		getphonebook(argv);
	}

		/* Write phonebook command. */
	if (strcmp(argv[1], "--writephonebook") == 0) {
		writephonebook();
	}

		/* Get sms message mode. */
	if (argc == 5 && strcmp(argv[1], "--getsms") == 0) {
		getsms(argv);
	}

		/* Delete sms message mode. */
	if (argc == 5 && strcmp(argv[1], "--deletesms") == 0) {
		deletesms(argv);
	}

		/* Send sms message mode. */
	if (argc == 4 && strcmp(argv[1], "--sendsms") == 0) {
		sendsms(argv);
	}

		/* Either an unknown command or wrong number of args! */
	usage();
	exit (-1);
}

	/* Send  SMS messages. */
void	sendsms(char *argv[])
{
	int		error;
	char	message_buffer[200];	
	int		chars_read;

		/* Get message text from stdin. */
	chars_read = fread(message_buffer, 1, 160, stdin);

	if (chars_read == 0) {
		fprintf(stderr, _("Couldn't read from stdin!\n"));	
		exit (1);
	}
		/*  Null terminate. */
	message_buffer[chars_read] = 0x00;	

	fprintf(stdout, _("Sending SMS to %s via message centre %s\n"), argv[2], argv[3]);

		/* Initialise the GSM interface. */     
	fbusinit(true);

		/* Send the message. */
	error = GSM->SendSMSMessage(argv[3], argv[2], message_buffer);

	if (error == GE_NOTIMPLEMENTED) {
		fprintf(stderr, _("Function not implemented in %s model!\n"), MODEL);
		GSM->Terminate();
		exit(-1);
	}

	GSM->Terminate();
	exit(-1);

}

	/* Get SMS messages. */
void	getsms(char *argv[])
{
	GSM_SMSMessage		message;
	GSM_MemoryType		memory_type;
	char				*memory_type_string;
	int					start_message, end_message, count;
	GSM_Error			error;


		/* Handle command line args that set type, start and end locations. */
	if (strcmp(argv[2], "int") == 0) {
		memory_type = GMT_INTERNAL;
		memory_type_string = "int";
	}
	else {
		if (strcmp(argv[2], "sim") == 0) {
			memory_type = GMT_SIM;
			memory_type_string = "sim";
		}
		else {
			fprintf(stderr, _("Unknown memory type %s!\n"), argv[2]);
			exit (-1);
		}
	}

	start_message = atoi (argv[3]);
	end_message = atoi (argv[4]);

		/* Initialise the code for the GSM interface. */     
	fbusinit(true);

		/* Now retrieve the requested entries. */
	for (count = start_message; count <= end_message; count ++) {
	
		error = GSM->GetSMSMessage(memory_type, count, &message);

		if (error == GE_NONE) {

			fprintf(stdout, _("Date/time: %d/%d/%d %d:%02d:%02d Sender: %s Msg Centre: %s\n"), message.Day, message.Month, message.Year, message.Hour, message.Minute, message.Second, message.Sender, message.MessageCentre);


			fprintf(stdout, _("Text: %s\n\n"), message.MessageText); 

		}
		else {

			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), MODEL);
				GSM->Terminate();
				exit(-1);	
			}

			fprintf(stdout, _("GetSMS %s %d failed!(%d)\n\n"), memory_type_string, count, error);
		}
	}

	GSM->Terminate();
	exit(0);
}

	/* Delete SMS messages. */
void	deletesms(char *argv[])
{
	GSM_SMSMessage		message;
	GSM_MemoryType		memory_type;
	char				*memory_type_string;
	int					start_message, end_message, count;
	GSM_Error			error;


		/* Handle command line args that set type, start and end locations. */
	if (strcmp(argv[2], "int") == 0) {
		memory_type = GMT_INTERNAL;
		memory_type_string = "int";
	}
	else {
		if (strcmp(argv[2], "sim") == 0) {
			memory_type = GMT_SIM;
			memory_type_string = "sim";
		}
		else {
			fprintf(stderr, _("Unknown memory type %s!\n"), argv[2]);
			exit (-1);
		}
	}

	start_message = atoi (argv[3]);
	end_message = atoi (argv[4]);

		/* Initialise the code for the GSM interface. */     
	fbusinit(true);

		/* Now delete the requested entries. */
	for (count = start_message; count <= end_message; count ++) {
	
		error = GSM->DeleteSMSMessage(memory_type, count, &message);

		if (error == GE_NONE) {

			fprintf(stdout, _("Deleted SMS %s %d\n"), memory_type_string, count);

		}
		else {

			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), MODEL);
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

	/* In this mode we get the pin from the keyboard and send it to the
	   mobile phone */

void	enterpin(void)
{
	char *pin=getpass("Enter your PIN: ");

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

	/* In this mode we receive the date and time from mobile phone */

void	getdatetime(void) {

  GSM_DateTime date_time;

	fbusinit(false);

	if (GSM->GetDateTime(&date_time)==GE_NONE) {
		fprintf(stdout, "Date: %4d/%02d/%02d\n", date_time.Year, date_time.Month, date_time.Day);
		fprintf(stdout, "Time: %02d:%02d:%02d\n", date_time.Hour, date_time.Minute, date_time.Second);
	}

	GSM->Terminate(); exit(0);
}

void	getalarm(void) {

  GSM_DateTime date_time;

	fbusinit(false);

	if (GSM->GetAlarm(0, &date_time)==GE_NONE) {
		fprintf(stdout, "Alarm: %s\n", (date_time.AlarmEnabled==0)?"off":"on");
		fprintf(stdout, "Time: %02d:%02d\n", date_time.Hour, date_time.Minute);
	}

	GSM->Terminate(); exit(0);
}

/* In monitor mode we don't do much, we just initialise the fbus code with
   monitoring enabled and print status informations. */

void monitormode(void)
{

  float rflevel=-1, batterylevel=-1;
  GSM_PowerSource powersource=-1;

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

    if (GSM->GetRFLevel(&rflevel)==GE_NONE)
      fprintf(stdout, "RFLevel: %d\n", (int)rflevel);

    if (GSM->GetBatteryLevel(&batterylevel) == GE_NONE)
      fprintf(stdout, "Battery: %d\n", (int)batterylevel);

    if (GSM->GetPowerSource(&powersource) == GE_NONE)
      fprintf(stdout, "Power Source: %s\n", (powersource==GPS_ACDC)?"AC/DC":"battery");

    sleep(1);
  }

  fprintf (stderr, _("Leaving monitor mode...\n"));

  GSM->Terminate();
  exit(0);
}

	/* Get requested range of phonebook entries and output to stdout
	   in format suitable for editing and subsequent writing back
	   to phone with writephonebook. */
void	getphonebook(char *argv[])
{
		/* Temporary entry for phonebook code. */
	GSM_PhonebookEntry		entry;
	int						count;
	GSM_MemoryType			memory_type;
	GSM_Error				error;
	char					*memory_type_string;
	int						start_entry;
	int						end_entry;


		/* Handle command line args that set type, start and end locations. */
	if (strcmp(argv[2], "int") == 0) {
		memory_type = GMT_INTERNAL;
		memory_type_string = "int";
	}
	else {
		if (strcmp(argv[2], "sim") == 0) {
			memory_type = GMT_SIM;
			memory_type_string = "sim";
		}
		else {
			fprintf(stderr, _("Unknown memory type %s!\n"), argv[2]);
			exit (-1);
		}
	}

	start_entry = atoi (argv[3]);
	end_entry = atoi (argv[4]);

		/* Do generic initialisation routine, monitoring disabled. */
	fbusinit(false);

		/* Now retrieve the requested entries. */
	for (count = start_entry; count <= end_entry; count ++) {
		if (GSM->GetPhonebookLocation(memory_type, count, &entry) == 0) {
			fprintf(stdout, "%s|%d|%s|%s|%d\n", memory_type_string, count, entry.Name, entry.Number, entry.Group);
		}
		else {

			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), MODEL);
				GSM->Terminate();
				exit(-1);
			}

			fprintf(stdout, _("%s|%d|Bad location or other error!\n"), memory_type_string, count);
		}
	}
	
	GSM->Terminate();
	exit(0);
}

	/* Read data from stdin, parse and write to phone.  The parsing
	   is relatively crude and doesn't allow for much variation
	   from the stipulated format. */

/* FIXME: can not parse group now...*/

void	writephonebook(void)
{
	GSM_PhonebookEntry		entry;
	GSM_Error				error;
	char					line_buffer[100];
	int						buffer_count;
	GSM_MemoryType			memory_type;
	char					*memory_type_string;
	int						entry_number, count, sub_count;
	char					name_buffer[40], number_buffer[60];
	int						line_count;
	int						separator_count;
	int						error_code;

		/* Initialise fbus code, enable monitoring for debugging
		   purposes... */
	fbusinit(true);

		/* Initialise line count. */
	line_count = 0;

		/* Go through data from stdin. */
	while(!feof(stdin)) {
		buffer_count = 0;
	
		while ((line_buffer[buffer_count] = fgetc(stdin)) != '\n' && (buffer_count < 99) && !feof(stdin)) {
			buffer_count ++;
		}
		line_buffer[buffer_count] = 0;

		if (strlen(line_buffer) == 0) {
			break;
		}

		line_count ++;

			/* Do initial parseo... Arrgh - gimmie PERL! :) */
		if (strncmp(line_buffer, "int", 3) == 0) {
			memory_type_string = "int";
			memory_type = GMT_INTERNAL;
		}
		else {
			if (strncmp(line_buffer, "sim", 3) == 0) {
				memory_type_string = "sim";
				memory_type = GMT_SIM;
			}
			else {
				fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, line_buffer);
			}
		}

		if (sscanf(line_buffer + 4, "%d|", &entry_number) != 1) {
			fprintf(stderr, _("Format problem on line %d [%s]\n"), line_count, line_buffer);
		} 
		else {
			separator_count = 0;
			count = 0;

				/* Skip first two separators. */
			while (count < strlen(line_buffer) && separator_count < 2) {
				if (line_buffer[count] == '|') {
					separator_count ++;
				}
				count ++;
			}
				/* Get next field (name) */
			sub_count = 0;	
			while ((count < strlen(line_buffer)) && (line_buffer[count] != '|')) {
				name_buffer[sub_count] = line_buffer[count];

				count ++;
				sub_count ++;
			}
			name_buffer[sub_count] = 0;
			count++;

				/* Get last field (number) */
			sub_count = 0;	
			while ((count < strlen(line_buffer)) && (line_buffer[count] != '|')) {
				number_buffer[sub_count] = line_buffer[count];

				count ++;
				sub_count ++;
			}
			number_buffer[sub_count] = 0;

		}

			/* Copy into entry. */	
		strcpy (entry.Name, name_buffer);
		strcpy (entry.Number, number_buffer);

			/* Do write and report success/failure. */
		error_code = GSM->WritePhonebookLocation(memory_type, entry_number, &entry);

		if (error_code == GE_NONE) {
			fprintf (stdout, _("Write Succeeded: memory type:%s, loc:%d, name: %s, number: %s\n"), memory_type_string, entry_number, entry.Name, entry.Number);
		}
		else {

			if (error == GE_NOTIMPLEMENTED) {
				fprintf(stderr, _("Function not implemented in %s model!\n"), MODEL);
				GSM->Terminate();
				exit(-1);
			}

			fprintf (stdout, _("Write FAILED(%d): memory type:%s, loc:%d, name: %s, number: %s\n"), error_code, memory_type_string, entry_number, entry.Name, entry.Number);
		}
	}

	GSM->Terminate();
	exit(0);
}


