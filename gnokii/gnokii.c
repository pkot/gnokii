	/* G N O K I I
	   A Linux/Unix toolset and driver for the Nokia 3110/3810/8110 mobiles.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  gnokii.c  Version 0.2.3

	   Mainline code for gnokii utility.  Handles command line parsing
	   and reading/writing phonebook entries. */


#include	<termios.h>
#include	<stdio.h>
#include	<libintl.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<string.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"gsm-api.h"

	/* Prototypes. */
void	usage(void);
void	monitormode(void);
void	enterpin(void);
void	getphonebook(char *argv[]);
void	writephonebook(void);
void	fbusinit(bool enable_monitoring);
void	getsms(char *argv[]);
void	sendsms(char *argv[]);


	/* Main function - handles command line args then passes to 
	   separate function accordingly. */
int	main(int argc, char *argv[])
{

		/* For GNU gettext */
	textdomain("gnokii");

		/* Handle command line arguments. */
	if (argc == 1 || strcmp(argv[1], "--help") == 0) {
		usage();
		exit (0);
	}

		/* Display version,  copyright and build information. */
	if (strcmp(argv[1], "--version") == 0) {
		fprintf(stdout, _("GNOKII Version 0.2.3 Copyright (C) Hugh Blemings 1999. <hugh@vsb.com.au>\n"));
		fprintf(stdout, _("       Built %s %s for %s on %s \n"), __TIME__, __DATE__, MODEL, PORT);
		exit (0);
	}

		/* Enter monitor mode. */
	if (strcmp(argv[1], "--monitor") == 0) {
		monitormode();
	}

		/* Enter pin. */
	if (strcmp(argv[1], "--enterpin") == 0) {
		enterpin();
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
	u8		c1, c2;
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
	error = GSM->SendSMSMessage(argv[3], argv[2], message_buffer, &c1, &c2);

		/* Report success. */
	if (error == GE_SMSSENDOK) {
		fprintf(stdout, _("SMS Send OK. Result code  0x%02x\n"), c1);
		GSM->Terminate();
		exit(0);
	}

	if (error == GE_NOTIMPLEMENTED) {
		fprintf(stderr, _("Function not implemented in %s model!\n"), MODEL);
		GSM->Terminate();
		exit(-1);
	}

		/* ...or failure, lookup GE_ codes in fbus.h */
	fprintf (stderr, _("SMS Send failed, gnokii error code was %d, network returned 0x%02x 0x%02x\n"), error, c1, c2);

	/* PJ: This should be moved somewhere under 3810... */
	if (c1 == 0x65 && c2 == 0x15) {
		fprintf(stderr, _("0x65 0x15 means SMS sending not enabled by network (probably...)\n"));
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

			fprintf(stdout, _("SMS %s %d Unknown bytes: %02x %02x %02x %02x %02x %02x Length: %d\n"), memory_type_string, count, message.Unk2, message.Unk3, message.Unk4, message.Unk5, message.Unk9, message.UnkEnd, message.Length);

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


static volatile bool shutdown = false;
static void interrupted(int sig)
{
    signal(sig, SIG_IGN);
    fprintf(stdout, _("Interrupted\n"));
    shutdown = true;
}

	/* In this mode we get the pin from the keyboard and send it to the
	   mobile phone */

void	enterpin(void)
{
	char *pin=getpass("Enter your PIN: ");

	fbusinit(true);

	sleep(1);

	GSM->EnterPin(pin);

	sleep(2);

	GSM->Terminate();
	exit(0);
}

	/* In monitor mode we don't do much, just initialise the fbus code
	   with monitoring enabled. */
void	monitormode(void)
{
	signal(SIGINT, interrupted);

	fprintf (stdout, _("Monitor mode...\n"));
	fprintf (stdout, _("Initialising GSM interface...\n"));

		/* Initialise the code for the GSM interface. */     
	fbusinit(true);

		/* loop here indefinitely - allows you to see messages from GSM
		   code in response to unknown messages etc. */
	while (!shutdown) {
		sleep(1);
	}

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
			fprintf(stdout, "%s|%d|%s|%s\n", memory_type_string, count, entry.Name, entry.Number);
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


void	usage(void)
{

  /* PJ: Uff, this should be one string -> easy gettextization :-) */
	fprintf(stdout, "usage: gnokii {--help|--monitor|--version}\n");
	fprintf(stdout, "       gnokii [--getphonebook] [memory type] [start] [end]\n");
	fprintf(stdout, "       gnokii [--writephonebook]\n");
	
	fprintf(stdout, "       gnokii [--getsms] [memory type] [start] [end]\n");

	fprintf(stdout, "       gnokii [--sendsms] [destination] [message centre]\n\n");

	fprintf(stdout, "          --help            display usage information.\n\n");
	fprintf(stdout, "          --monitor         continually updates phone status to stderr.\n\n");
	fprintf(stdout, "          --version         displays version and copyright information.\n\n");

	fprintf(stdout, "          --enterpin        send the entered PIN to the phone.\n\n");
	fprintf(stdout, "          --getphonebook    gets phonebook entries from specified memory type\n");
 	fprintf(stdout, "                            ('int' or 'sim') starting at entry [start] and\n");
	fprintf(stdout, "                            ending at [end].  Entries are dumped to stdout in\n");
	fprintf(stdout, "                            format suitable for editing and passing back to\n");
	fprintf(stdout, "                            writephonebook command.\n\n");

	fprintf(stdout, "          --writephonebook  reads data from stdin in and writes to phonebook.\n");
	fprintf(stdout, "                            Uses the same format as provided by the output of\n");
	fprintf(stdout, "                            the getphonebook command.\n\n");


	fprintf(stdout, "          --getsms          gets SMS messages from specified memory type\n");
 	fprintf(stdout, "                            ('int' or 'sim') starting at entry [start] and\n");
	fprintf(stdout, "                            ending at [end].  Entries are dumped to stdout.\n\n");

	fprintf(stdout, "          --sendsms         sends an SMS message to [destination] via\n");
 	fprintf(stdout, "                            [message centre].  Message text is taken from\n");
	fprintf(stdout, "                            stdin.  This function has had limited testing\n");
	fprintf(stdout, "                            and may not work at all on your network.\n\n");
}

void	fbusinit(bool enable_monitoring)
{
	int			count;
	GSM_Error	error;

		/* Initialise the code for the GSM interface. */     
	error = GSM_Initialise(MODEL, PORT, enable_monitoring);

	if (error != GE_NONE) {
		fprintf(stderr, _("GSM/fbus init failed! (Unknown model ?).  Quitting. \n"));
		exit (-1);
	}

		/* First (and important!) wait for GSM link to be active. 
		   we allow 10 seconds... */
	count = 0;

	while (count < 200 && *GSM_LinkOK == false) {
		count ++;
		usleep (50000);
	}

	if (GSM_LinkOK == false) {
		fprintf (stderr, _("Hmmm... GSM_LinkOK never went true.  Quitting. \n"));
		exit (-1);
	}
}
