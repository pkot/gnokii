	/* G N O K I I
	   A Linux/Unix toolset and driver for the Nokia 3110/6110/8110 mobiles.
	   Copyright (C) Hugh Blemings ??? , 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  fbus-6110.c  Version 0.2.3
	
	   Provides an API for accessing functions on the 6110 and
	   similar phones. 

	   The various routines are called FB61 (whatever) as a
	   concatenation of FBUS and 6110.

	   This source file (and fbus-6110.h) are basically empty and
	   are hacked from the 3810 code.  You may or may not find the
	   3810 code a useful reference for handling 6110 functionality,
	   I would imagine the protocol handling, function dispatching etc.
	   would be pretty similar.  */

#define		__fbus_6110_c	/* "Turn on" prototypes in fbus-6110.h */

#include	<termios.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<sys/signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<string.h>
#include	<pthread.h>
#include	<errno.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"fbus-6110.h"

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					FB61_LinkOK;

GSM_Functions			FB61_Functions = {
		FB61_Initialise,
		FB61_Terminate,
		FB61_GetPhonebookLocation,
		FB61_WritePhonebookLocation,
		FB61_GetSMSMessage,
		FB61_SendSMSMessage,
		FB61_GetRFLevel,
		FB61_GetBatteryLevel,
		FB61_EnterPin,
		FB61_GetIMEIAndCode
};

	/* These are all guesses for the 6110 at this stage (HAB) */
GSM_Information			FB61_Information = {
		"6110",					/* Models */
		4, 						/* Max RF Level */
		0,						/* Min RF Level */
		GRF_Arbitrary,			/* RF level units */
		4, 						/* Max Battery Level */
		0,						/* Min Battery Level */
		GBU_Arbitrary			/* Battery level units */
};

	/* Local variables */
int						PortFD;
char					PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
pthread_t				Thread;
bool					RequestTerminate;
struct termios			OldTermios;	/* To restore termio on close. */
bool					EnableMonitoringOutput;

	/* Initialise variables and state machine. */
GSM_Error	FB61_Initialise(char *port_device, bool enable_monitoring)
{
	int		rtn;

	RequestTerminate = false;
	FB61_LinkOK = false;
	EnableMonitoringOutput = enable_monitoring;

	strncpy (PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);


		/* REMOVE THIS BEFORE TESTING YOUR CODE !!!!!!! */
	return (GE_NOTIMPLEMENTED);


		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) FB61_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (GE_INTERNALERROR);
    }

	return (GE_NONE);

}

	/* This is the main loop for the FB61 functions.  When FB61_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the FB61_Terminate function. */
void	FB61_ThreadLoop(void)
{
		/* Initialise things... */





		/* Try to open serial port, if we fail we sit here and don't proceed
		   to the main loop. */
	if (FB61_OpenSerial() != true) {
		FB61_LinkOK = false;
			/* Fail so sit here till calling code works out there is a
			   problem. */		
		while (!RequestTerminate) {
			usleep (100000);
		}
		return;
	}

		/* Initialise link with phone or what have you */



		/* Now enter main loop */
	while (!RequestTerminate) {

			/* Do things... */
		

		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}

}

	/* Applications should call FB61_Terminate to shut down
	   the FB61 thread and close the serial port. */
void		FB61_Terminate(void)
{
		/* Request termination of thread */
	RequestTerminate = true;

		/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

		/* Close serial port. */
	tcsetattr(PortFD, TCSANOW, &OldTermios);
	
	close (PortFD);
}

	/* These are the actual functions used by the higher level code, 
	   accessed by the GSM_Functions structure.  For now these are
	   all return GE_NOTIMPLEMENTED! */
GSM_Error	FB61_GetRFLevel(float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_GetBatteryLevel(float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_EnterPin(char *pin)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_GetIMEIAndCode(char *imei, char *code)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	FB61_GetPhonebookLocation(GSM_MemoryType memory_type, int location, GSM_PhonebookEntry *entry) {

	return (GE_NOTIMPLEMENTED);
}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	FB61_WritePhonebookLocation(GSM_MemoryType memory_type, int location, GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_GetSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_SendSMSMessage(char *message_centre, char *destination, char *text, u8 *return_code1, u8 *return_code2)
{
	return (GE_NOTIMPLEMENTED);
}
	/* Called by initialisation code to open comm port in
	   asynchronous mode. */
bool		FB61_OpenSerial(void)
{
	struct termios			new_termios;
	struct sigaction		sig_io;

		/* Open device. */
	PortFD = open (PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (PortFD < 0) {
		perror("Couldn't open FB61 device: ");
		return (false);
	}

		/* Set up and install handler before enabling async IO on port. */
	sig_io.sa_handler = FB61_SigHandler;
	sig_io.sa_flags = 0;
	sig_io.sa_restorer = NULL;
	sigaction (SIGIO, &sig_io, NULL);

		/* Allow process/thread to receive SIGIO */
	fcntl(PortFD, F_SETOWN, getpid());

		/* Make filedescriptor asynchronous. */
	fcntl(PortFD, F_SETFL, FASYNC);

		/* Save current port attributes so they can be restored on exit. */
	tcgetattr(PortFD, &OldTermios);

		/* Set port settings for canonical input processing */
	new_termios.c_cflag = FB61_BAUDRATE | CS8 | CLOCAL | CREAD;
	new_termios.c_iflag = IGNPAR;
	new_termios.c_oflag = 0;
	new_termios.c_lflag = 0;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;

	tcflush(PortFD, TCIFLUSH);
	tcsetattr(PortFD, TCSANOW, &new_termios);

	return (true);
}

	/* Handler called when characters received from serial port. 
	   calls state machine code to process it. */
void	FB61_SigHandler(int status)
{
	unsigned char 	buffer[255];
	int				count,res;

	res = read(PortFD, buffer, 255);

	for (count = 0; count < res ; count ++) {
		/* For the 3810 code, the RX state machine is called once
		   for each character received.  By no means the only approach
		   but seems to work OK in the 3810 case at least! */
		/*FB61_RX_StateMachine(buffer[count]);*/
	}
}

