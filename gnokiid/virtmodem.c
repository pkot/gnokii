/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	virtmodem.c - provides a virtual modem interface to the GSM
	phone by calling code in gsm-api.c, at-emulator.c and datapump.c

	The code here provides the overall framework and coordinates
	switching between command mode (AT-emulator) and "online" mode
	where the data pump code translates data from/to the GSM handset
	and the modem data/fax stream.

*/

#define		__virtmodem_c


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>


#include "misc.h"
#include "gsm-api.h"
#include "gsm-common.h"
#include "at-emulator.h"
#include "datapump.h"
#include "virtmodem.h"


	/* Global variables */

	/* Local variables */

int		PtyRDFD;	/* File descriptor for reading and writing to/from */
int		PtyWRFD;	/* pty interface - only different in debug mode. */ 

bool	UseSTDIO;	/* Use STDIO for debugging purposes instead of pty */
bool	CommandMode;

pthread_t		Thread;
bool			RequestTerminate;

	/* If initialised in debug mode, stdin/out is used instead
	   of ptys for interface. */
bool	VM_Initialise(char *model, char *port, GSM_ConnectionType connection, bool debug_mode)
{
	int		rtn;

	CommandMode = true;

	RequestTerminate = false;

	if (debug_mode == true) {
		UseSTDIO = true;
	}
	else {
		UseSTDIO = false;
	}

	if (VM_GSMInitialise(model, port, connection) != GE_NONE) {
		fprintf (stderr, _("VM_Initialise - VM_GSMInitialise failed!\n\r"));
		return (false);
	}

	if (VM_PtySetup() < 0) {
		fprintf (stderr, _("VM_Initialise - VM_PtySetup failed!\n\r"));
		return (false);
	}

	if (ATEM_Initialise(PtyRDFD, PtyWRFD) != true) {
		fprintf (stderr, _("VM_Initialise - ATEM_Initialise failed!\n\r"));
		return (false);
	}

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) VM_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (false);
    }

	return (true);
}

void	VM_ThreadLoop(void)
{
	int				res;
	fd_set			input_fds, test_fds;
	struct timeval	timeout;	

		/* Note we can't use signals here as they are already used
		   in the FBUS code.  This may warrant changing the FBUS
		   code around sometime to use select instead to free up
		   the SIGIO handler for mainline code. */


		/* Clear then set file descriptor set to point to PtyRDFD */
	FD_ZERO(&input_fds);
	FD_SET(PtyRDFD, &input_fds);


	while (!RequestTerminate) {
		test_fds = input_fds;
		timeout.tv_usec =0;
		timeout.tv_usec = 500000;	/* Timeout after 500mS */	

		res = select(FD_SETSIZE, &test_fds, (fd_set *)0, (fd_set *)0, &timeout);

		switch (res) {
			case 0:
				break;

			case -1:
				perror("VM_ThreadLoop - select");
				exit (-1);

			default:
				if (FD_ISSET(PtyRDFD, &test_fds)) {
					VM_CharHandler();
				}
				break;
		}
	}

}

	/* Application should call VM_Terminate to shut down
	   the virtual modem thread */
void		VM_Terminate(void)
{
		/* Request termination of thread */
	RequestTerminate = true;

		/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

	if (!UseSTDIO) {
		close (PtyRDFD);
		close (PtyWRFD);
	}
}

	/* Open pseudo tty interface and (in due course create a symlink
	   to be /dev/gnokii etc. ) */

int		VM_PtySetup(void)
{
	int			err;
	char		*slave_name;
	//char		*mgnokiidev = "/home/hugh/work/gnokii/mgnokiidev";			
	char		*mgnokiidev = "./mgnokiidev";			
	char		cmdline[200];
	int			pty_number;

	if (UseSTDIO) {
		PtyRDFD = STDIN_FILENO;
		PtyWRFD = STDOUT_FILENO;
		return (0);
	}
	
	PtyRDFD = VM_GetMasterPty(&slave_name);
	if (PtyRDFD < 0) {
		fprintf (stderr, "Couldn't open pty!\n\r");
		return(-1);
	}
	PtyWRFD = PtyRDFD;

		/* Check we haven't been installed setuid root for some reason
		   if so, don't create /dev/gnokii */
	if (getuid() != geteuid()) {
		fprintf(stderr, "gnokiid should not be installed setuid root!\n\r");
		return (0);
	}

	fprintf (stderr, "Slave pty is %s, calling %s to create /dev/gnokii.\n\r", slave_name, mgnokiidev);

		/* Get pty number */
	pty_number = atoi(slave_name + 9);

		/* Create command line, something line ./mkgnokiidev ttyp0 */
	sprintf(cmdline, "%s %d", mgnokiidev, pty_number);

		/* And use system to call it. */	
	err = system (cmdline);
	
	return (err);

}

    /* Handler called when characters received from serial port.
       calls state machine code to process it. */
void    VM_CharHandler(void)
{
    unsigned char   buffer[255];
    int             res;

    res = read(PtyRDFD, buffer, 255);

		/* FIXME - need something more elegant here - a -1 return
		   indicates that the slave pty has closed... */
	if (res < 0) {
		return;
	}

		/* If we're in command mode and the AT emulator is initialised,
		   pass byte to it for processing. */
	if (CommandMode && ATEM_Initialised) {
		ATEM_HandleIncomingData(buffer, res);
	}
	
	//if (!CommandModem && VM_Initialised) {
	//	VM_HandleIncomingData(&buffer, res);
	//}
}     

	/* Initialise GSM interface, returning GSM_Error as appropriate  */
GSM_Error 	VM_GSMInitialise(char *model, char *port, GSM_ConnectionType connection)
{
	int 		count=0;
	GSM_Error 	error;

		/* Initialise the code for the GSM interface. */     

	error = GSM_Initialise(model, port, connection, false);

	if (error != GE_NONE) {
		fprintf(stderr, _("GSM/FBUS init failed! (Unknown model ?). Quitting.\n"));
    	return (error);
	}

		/* First (and important!) wait for GSM link to be active. We allow 10
		   seconds... */

	while (count++ < 200 && *GSM_LinkOK == false) {
    	usleep(50000);
	}

	if (*GSM_LinkOK == false) {
		fprintf (stderr, _("Hmmm... GSM_LinkOK never went true. Quitting. \n"));
   		return (GE_NOLINK); 
	}

	return (GE_NONE);
}

/* VM_GetMasterPty()
   Takes a double-indirect character pointer in which to put a slave
   name, and returns an integer file descriptor.  If it returns < 0, an
   error has occurred.  Otherwise, it has returned the master pty
   file descriptor, and fills in *name with the name of the
   corresponding slave pty.  Once the slave pty has been opened,
   you are responsible to free *name.  Code is from Developling Linux
   Applications by Troan and Johnson */


int	VM_GetMasterPty(char **name) { 

   int i, j;
   /* default to returning error */
   int master = -1;

   /* create a dummy name to fill in */
   *name = strdup("/dev/ptyXX");

   /* search for an unused pty */
   for (i=0; i<16 && master <= 0; i++) {
      for (j=0; j<16 && master <= 0; j++) {
         (*name)[8] = "pqrstuvwxyzPQRST"[i];
         (*name)[9] = "0123456789abcdef"[j];
         /* open the master pty */
         if ((master = open(*name, O_RDWR | O_NOCTTY | O_NONBLOCK )) < 0) {
            if (errno == ENOENT) {
               /* we are out of pty devices */
               free (*name);
               return (master);
            }
         }
      }
   }
   if ((master < 0) && (i == 16) && (j == 16)) {
      /* must have tried every pty unsuccessfully */
      free (*name);
      return (master);
   }

   /* By substituting a letter, we change the master pty
    * name into the slave pty name.
    */
   (*name)[5] = 't';

   return (master);
}
