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
#include	<libintl.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<signal.h>
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
	unsigned char 		init_string[4] = {0x55, 0x55, 0x55, 0x55};
	unsigned char 		connect1[9] = {0x00, 0x01, 0x00, 0x0d, 0x00, 0x00, 0x02, 0x01, 0x40};
	unsigned char 		connect2[7] = {0x00, 0x01, 0x00, 0x20, 0x02, 0x01, 0x40};

	unsigned char 		connect3[9] = {0x00, 0x01, 0x00, 0x0d, 0x01, 0x00, 0x02, 0x01, 0x41};
	unsigned char 		connect4[6] = {0x00, 0x01, 0x00, 0x10, 0x01, 0x62};

	unsigned char 		connect5[0x1a] = {0x00, 0x01, 0x00, 0x42, 0x05, 0x01, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x55, 0x63, 0xa8, 0x00, 0x00, 07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80, 0x01, 0x63};

	unsigned char       connect6[0x2f] = {0x00, 0x01, 0x00, 0x12, 0x65, 0x40, 0x36, 0x76, 0x10, 0x66, 0x50, 0x14, 0xba, 0xff, 0x66, 0x62, 0x40, 0xf9, 0xde, 0xdf, 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x26, 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x20, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x79, 0x00, 0x00, 0x00, 0x00, 0x01, 0x44};

	int					count;

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
		/* Send init string to phone, this is a bunch of 0x55
		   characters.  Timing is empirical. */
	for (count = 0; count < 100; count ++) {
		usleep(1000);
		write(PortFD, init_string, 1);
	}

	FB61_TX_SendMessage(9, 0x02, connect1);

		usleep(10000);

	FB61_TX_SendMessage(7, 0x02, connect2);

		usleep(10000);

	FB61_TX_SendMessage(9, 0x02, connect3);

		usleep(10000);

	FB61_TX_SendMessage(6, 0x64, connect4);

		usleep(1000);

	FB61_TX_SendMessage(0x1a, 0x01, connect5);

		usleep(1000);

	FB61_TX_SendMessage(0x2f, 0x64, connect6);

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
		perror(_("Couldn't open FB61 device: "));
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

void 	  FB61_HandleReceived(unsigned char *buffer, int count){

  int current;

  if (buffer[count+3] != FB61_FRTYPE_ACK) {

	printf("Phone: ");

	for (current=0; current<buffer[count+5]+8; current++)
	  printf("[%2x]", buffer[count+current]);

	printf("\n");

	FB61_TX_SendAck(buffer[count+3]);

  }
  else {
	printf("Received Ack of type %2x, seq %2x\n", buffer[count+6], buffer[count+7]);
  }

}

	/* Handler called when characters received from serial port. 
	   calls state machine code to process it. */
void	FB61_SigHandler(int status)
{
	unsigned char 	buffer[255];
	int				count,res;

	res = read(PortFD, buffer, 255);
	count=0;

	while (res) {

	  int length=8+buffer[count+5]+(buffer[count+5] % 2);

	  FB61_HandleReceived(buffer, count);
	  count=count+length;
	  res=res-length;
	}

}

	/* Prepares the message header and sends it, prepends the
	   message start byte (0x1e) and other values according
	   the value specified when called.  Calculates checksum
	   and then sends the lot down the pipe... */
int		FB61_TX_SendMessage(u8 message_length, u8 message_type, u8 *buffer)
{
	u8			out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
	int			count, current=0;
	unsigned char			checksum;

		/* Check message isn't too long, once the necessary
		   header and trailer bytes are included. */
	/* FIXME */
	if ((message_length + 5) > FB61_MAX_TRANSMIT_LENGTH) {

		return (false);
	}
		/* Now construct the message header. */
	out_buffer[current++] = FB61_FRAME_ID;	/* Start of message indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC; /* Source */

	out_buffer[current++] = message_type; /* Type */

	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = message_length; /* Length */

		/* Copy in data if any. */	
	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current+=message_length;
	}

	/* If the message length is odd we should add pad byte 0x00 */
	if (message_length % 2)
	  out_buffer[current++]=0x00;

		/* Now calculate checksums over entire message 
		   and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

	printf("PC: ");

	for (count = 0; count < current; count++) {
		printf("%2x:", out_buffer[count]);
	}

	printf("\n");

		/* Send it out... */
	if (write(PortFD, out_buffer, current) != current) {
		return (false);
	}
	return (true);
}

int		FB61_TX_SendAck(u8 message_type) {

  static sequence=0;
	u8			out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
  int count, current=0;
	unsigned char			checksum;

	printf("Sending Ack of type %2x\n", message_type);

  /* Now construct the Ack header. */
    out_buffer[current++] = FB61_FRAME_ID;	/* Start of message indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC; /* Source */

	out_buffer[current++] = FB61_FRTYPE_ACK; /* Ack */

	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = 2; /* Ack is always of 2 bytes */

	out_buffer[current++] = sequence++; /* Sequence number */

	printf("sequence: %x\n", sequence);

	if (sequence == 7)
	  sequence=0;

		/* Now calculate checksums over entire message 
		   and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count+=2) {
		checksum ^= out_buffer[count];
	}
	out_buffer[current++] = checksum;

		/* Send it out... */
	if (write(PortFD, out_buffer, current) != current) {
		return (false);
	}

return true;  
}
