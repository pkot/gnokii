	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	
	   This file:  fbus-6110.c  Version 0.2.4
	
	   Copyright (C) 1999 Pavel Janik .ml & Hugh Blemings. 
	   Released under the terms of the GNU GPL, see file COPYING
	   for more details.

	   Provides an API for accessing functions on the 6110 and
	   similar phones. 

	   The various routines are called FB61 (whatever) as a
	   concatenation of FBUS and 6110.

	   */

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
		FB61_DeleteSMSMessage,
		FB61_SendSMSMessage,
		FB61_GetRFLevel,
		FB61_GetBatteryLevel,
		FB61_EnterPin,
		FB61_GetIMEIAndCode,
		FB61_GetDateTime,
		FB61_SetDateTime,
		FB61_GetAlarm,
		FB61_SetAlarm
};

	/* These are all guesses for the 6110 at this stage (HAB) */
GSM_Information			FB61_Information = {
		"6110",					/* Models */
		4, 						/* Max RF Level */
		0,						/* Min RF Level */
		GRF_Arbitrary,			/* RF level units */
		4, 						/* Max Battery Level */
		0,						/* Min Battery Level */
		GBU_Arbitrary,			/* Battery level units */
		GDT_DateTime,			/* Have date/time support */
		GDT_TimeOnly,			/* Alarm supports time only ? */
		1						/* Only one alarm available. */
};

	/* Local variables */
int						PortFD;
int BufferCount;
u8						MessageBuffer[FB61_MAX_RECEIVE_LENGTH];
unsigned char MessageLength, MessageType, MessageDestination, MessageSource, MessageUnknown;
char					PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
u8						RequestSequenceNumber=0x00;
pthread_t				Thread;
bool					RequestTerminate;
bool					DisableKeepalive=false;
struct termios			OldTermios;	/* To restore termio on close. */
bool					EnableMonitoringOutput;

	/* Initialise variables and state machine. */
GSM_Error	FB61_Initialise(char *port_device, bool enable_monitoring)
{
	int		rtn;

	RequestTerminate = false;
	FB61_LinkOK = true;
	EnableMonitoringOutput = enable_monitoring;

	strncpy (PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) FB61_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (GE_INTERNALERROR);
    }

	return (GE_NONE);

}

GSM_Error FB61_TX_Send0x04Message()
{

	unsigned char request[] = {0x00, 0x01, 0x00, 0x01, 0x01};

	FB61_TX_SendMessage(0x05, 0x04, request);

	return (GE_NONE);
}

	/* This is the main loop for the FB61 functions.  When FB61_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the FB61_Terminate function. */
void	FB61_ThreadLoop(void)
{
		/* Initialize things... */
	unsigned char 		init_char[1] = {0x55};
	unsigned char 		connect1[] = {0x00, 0x01, 0x00, 0x0d, 0x00, 0x00, 0x02, 0x01};
	unsigned char 		connect2[] = {0x00, 0x01, 0x00, 0x20, 0x02, 0x01};
 
	unsigned char 		connect3[] = {0x00, 0x01, 0x00, 0x0d, 0x01, 0x00, 0x02, 0x01};
	unsigned char 		connect4[] = {0x00, 0x01, 0x00, 0x10, 0x01};
 
	//	unsigned char 		connect5[] = {0x00, 0x01, 0x00, 0x42, 0x05, 0x01, 0x07, 0xa2, 0x88, 0x81, 0x21, 0x55, 0x63, 0xa8, 0x00, 0x00, 0x07, 0xa3, 0xb8, 0x81, 0x20, 0x15, 0x63, 0x80, 0x01};
 
	//	unsigned char       connect6[] = {0x00, 0x01, 0x00, 0x12, 0x65, 0x40, 0x36, 0x76, 0x10, 0x66, 0x50, 0x14, 0xba, 0xff, 0x66, 0x62, 0x40, 0xf9, 0xde, 0xdf, 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x26, 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x20, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x79, 0x00, 0x00, 0x00, 0x00, 0x01};

	int					count, idle_timer;

	GSM_DateTime 		date_time;		/* "bogus" variable for calling GetAlarm */

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
	for (count = 0; count < 250; count ++) {
		usleep(100);
		write(PortFD, init_char, 1);
	}

	FB61_TX_Send0x04Message();

		usleep(1000);

	FB61_TX_SendMessage(8, 0x02, connect1);

		usleep(1000);

	FB61_TX_SendMessage(6, 0x02, connect2);

		usleep(1000);

	FB61_TX_SendMessage(8, 0x02, connect3);

		usleep(1000);

	FB61_TX_SendMessage(5, 0x64, connect4);

	usleep(10000);


	/* This function sends the Clock request to the phone. Value
	   returned isn't used yet  */

	FB61_GetDateTime(&date_time);

	/* This function sends the Alarm request to the phone.  Values
	   passed are ignored at present */

	FB61_GetAlarm(0, &date_time);

	usleep(10000);

	/* Get the primary SMS Center */

	/* It's very strange that if I send this request again (with different
       number) it fails. But if I send it alone it succeds. It seems that 6110
       is refusing to tell you all the SMS Centers information at once :-( */

	FB61_GetSMSCenter(1);

	/* This doesn't work now, but will in the near future, do you believe? */
	//		FB61_GetPhonebookLocation(GMT_SIM,1,NULL);

		idle_timer=0;

		/* Now enter main loop */
	 	while (!RequestTerminate) {

		  if (idle_timer == 0) {
			/* Dont send keepalive packets when doing other transactions. */
			if (!DisableKeepalive)
			  FB61_TX_Send0x04Message();
			
			idle_timer = 20;
		  }
		  else
			idle_timer--;

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
	unsigned char pin_req[] = {0x00, 0x01, 0x00, 0x0a, 0x02, '0', '0', '0', '0', 0x00, 0x00, 0x01};

	pin_req[5]=pin[0];
	pin_req[6]=pin[1];
	pin_req[7]=pin[2];
	pin_req[8]=pin[3];

	FB61_TX_SendMessage(0x0c, 0x08, pin_req);

	return (GE_NONE);
}

GSM_Error	FB61_GetDateTime(GSM_DateTime *date_time)
{
	unsigned char clock_req[] = {0x00, 0x01, 0x00, 0x62, 0x01};

	FB61_TX_SendMessage(0x05, 0x11, clock_req);

	return (GE_NONE);
}

GSM_Error	FB61_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	unsigned char alarm_req[] = {0x00, 0x01, 0x00, 0x6d, 0x01};

	FB61_TX_SendMessage(0x05, 0x11, alarm_req);

	return (GE_NONE);
}

	/* This function sends to the mobile phone a request for the SMS Center
	   of rank `priority' */

GSM_Error	FB61_GetSMSCenter(u8 priority)
{
	unsigned char smsc_req[] = {0x00, 0x01, 0x00, 0x33, 0x64, 0x01, 0x01};

	smsc_req[5]=priority;

	FB61_TX_SendMessage(0x07, 0x02, smsc_req);

	return (GE_NONE);
}

GSM_Error	FB61_GetIMEIAndCode(char *imei, char *code)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	FB61_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	FB61_GetPhonebookLocation(GSM_MemoryType memory_type, int location, GSM_PhonebookEntry *entry) {

	unsigned char entry_req[] = {0x00, 0x01, 0x00, 0x01, 0x02, 0x01, 0x00, 0x01};

	FB61_TX_SendMessage(0x09, 0x03, entry_req);

	return (GE_NONE);
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

GSM_Error	FB61_DeleteSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message)
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

void	FB61_SigHandler(int status)
{
	unsigned char 	buffer[255];
	int				count,res;

	res = read(PortFD, buffer, 255);

	for (count = 0; count < res ; count ++) {
		FB61_RX_StateMachine(buffer[count]);
	}
}

char *FB61_GetBCDNumber(u8 *Number) {
  static char Buffer[20]="";
  /* This is the length of BCD coded number */
  int length=Number[0];
  int count;

  if (Number[1]==0x91)
	sprintf(Buffer, "+");

  for (count=0; count <length-1; count++)
	sprintf(Buffer, "%s%d%d", Buffer, Number[count+2]&0x0f, Number[count+2]>>4);

  return Buffer;
}

enum FB61_RX_States		FB61_RX_DispatchMessage(void)
{

	int tmp, count;

	/* Uncomment this if you want all messages in raw form. */
	// FB61_RX_DisplayMessage();

		/* Switch on the basis of the message type byte */
	switch (MessageType) {

	  /* Incoming call alert message */

	case 0x01:
	  printf("Message: Incoming call alert:\n");
	  printf("   Number: ");
	  count=MessageBuffer[6];
	  
	  for (tmp=0; tmp <count; tmp++)
	  	printf("%c", MessageBuffer[7+tmp]);

	  printf("\n");

	  printf("   Name: ");
	  for (tmp=0; tmp <MessageBuffer[7+count]; tmp++)
	  	printf("%c", MessageBuffer[8+count+tmp]);

	  printf("\n");
	break;

	/* General phone control */

	case 0x02:

	  switch (MessageBuffer[3]) {

	  case 0x34:

		printf(_("Message: SMS Center received:\n"));
		printf("   %d. SMS Center name is %s\n", MessageBuffer[4], MessageBuffer+33);
		printf("   %d. SMS Center number is %s\n", MessageBuffer[4], FB61_GetBCDNumber(MessageBuffer+21));

		break;

	  case 0x35:

		/* Nokia 6110 has support for three SMS centers with numbers 1, 2 and
           3. Each request for SMS Center without one of these numbers fail. */

		printf(_("Message: SMS Center error received:\n"));
		printf(_("   The request for SMS Center failed.\n"));

		break;  

	  default:
			  FB61_RX_DisplayMessage();
	  }

	  break;

	  /* Phone status */
	  
	case 0x04:
	  printf(_("Message: Phone status received:\n"));

	  printf("   Mode: ");

	  switch (MessageBuffer[4]) {

	  case 0x01:
		printf("registered within the network\n");
		break;

	  case 0x03:
		printf("waiting for pin\n");
		break;

	  case 0x04:
		printf("powered off\n");
		break;

	  default:
		printf("unknown\n");
	  }

	  printf("   Power source: ");

	  switch (MessageBuffer[7]) {

	  case 0x01:
		printf("AC/DC\n");
		break;
		
	  case 0x02:
		printf("battery\n");
		break;

	  default:
		printf("unknown\n");
	  }


	  printf("   Battery Level: %d\n", MessageBuffer[8]);

   	  printf("   Signal strength: %d\n", MessageBuffer[5]);
	  break;

	  /* PIN requests */

	case 0x08:

	  printf("Message: PIN:\n");

	  if (MessageBuffer[3] != 0x0c)
		printf("   Code Accepted\n");
	  else
	  	printf("   Code Error\n   You're not my big owner :-)\n");
	break;

	/* Network status */

	case 0x0a:

	  printf("Message: Network registration:\n");

	  printf("   CellID: %02x%2x\n", MessageBuffer[10], MessageBuffer[11]);
	  printf("   LAC: %02x%02x\n", MessageBuffer[12], MessageBuffer[13]);
	  printf("   Network: %x%x%x-%x%x\n", MessageBuffer[14] & 0x0f, MessageBuffer[14] >>4, MessageBuffer[15] & 0x0f, MessageBuffer[16] & 0x0f, MessageBuffer[16] >>4);
	  break;

	  /* Phone Clock and Alarm */

	case 0x11:

	  switch (MessageBuffer[3]) {

	  case 0x63:
	    printf("Message: Clock\n");

	    printf("   Time: %02d:%02d:%02d\n", MessageBuffer[12], MessageBuffer[13], MessageBuffer[14]);
	    printf("   Date: %4d/%02d/%02d\n", 256*MessageBuffer[8]+MessageBuffer[9], MessageBuffer[10], MessageBuffer[11]);

	    break;

	  case 0x6e:
	    printf("Message: Alarm\n");

	    printf("   Alarm: %02d:%02d\n", MessageBuffer[9], MessageBuffer[10]);
	    printf("   Alarm is %s\n", (MessageBuffer[8]==2) ? "on":"off");

	    break;

	  default:
	    printf("Message: Unknown message of type 0x11\n");

	  }
	  break;

	  /* Mobile phone identification */

	case 0x64:
	  printf("Message: Mobile phone identification received:\n");

	  printf("   Serial No. %s\n", MessageBuffer+4);

	  /* What is this? My phone reports: NSE-3 */
	  printf("   %s\n", MessageBuffer+25);

	  /* What is this? My phone reports: 0501505 */
	  printf("   %s\n", MessageBuffer+31);

	  /* What is this? My phone reports: 4200. This could be the internetional number of the country - I;m in the Czech republic - our code is +420*/
	  printf("   %s\n", MessageBuffer+39);

	  printf("   Firmware: %s\n", MessageBuffer+44);

	  /* These bytes are probably the source of the "Accessory not connected"
         messages on the phone when trying to emulate NCDS... I hope... */
   	  printf("   Magic byte1: %02x\n", MessageBuffer[50]);
   	  printf("   Magic byte2: %02x\n", MessageBuffer[51]);
   	  printf("   Magic byte3: %02x\n", MessageBuffer[52]);
   	  printf("   Magic byte4: %02x\n", MessageBuffer[53]);

	  break;

	  /* Acknowlegment */
	  
	case 0x7f:
	  printf(_("[Received Ack of type %02x, seq: %2x]\n"), MessageBuffer[0], MessageBuffer[1]);
	  break;

	  /* Power on message */

	case 0xd0:
	  printf(_("Message: The phone is powered on - seq 1.\n"));
	  FB61_RX_DisplayMessage();
	  break;

	  /* Power on message */

	case 0xf4:
	  printf(_("Message: The phone is powered on - seq 2.\n"));
	  FB61_RX_DisplayMessage();
	  break;

	  /* Unknown message - if you think that you know the exact meaning of
         other messages - please let us know. */

	default:
	  FB61_RX_DisplayMessage();

}

	return FB61_RX_Sync;
}

enum FB61_RX_States         RX_State;

	/* RX_State machine for receive handling.  Called once for each
	   character received from the phone/phone. */
void	FB61_RX_StateMachine(char rx_byte)
{

	switch (RX_State) {
	
					/* Messages from the phone start with an 0x1e.  We
					   use this to "synchronise" with the incoming data
					   stream. */		
		case FB61_RX_Sync:
				if (rx_byte == FB61_FRAME_ID) {
					BufferCount = 0;
					RX_State = FB61_RX_GetDestination;
				}
				break;

		case FB61_RX_GetDestination:
		  MessageDestination=rx_byte;
		  RX_State = FB61_RX_GetSource;
		  break;

		case FB61_RX_GetSource:
		  MessageSource=rx_byte;
		  RX_State = FB61_RX_GetType;
		  break;

		case FB61_RX_GetType:
		  MessageType=rx_byte;
		  RX_State = FB61_RX_GetUnknown;
		  break;

		case FB61_RX_GetUnknown:
		  MessageUnknown=rx_byte;
		  RX_State = FB61_RX_GetLength;
		  break;
		
		case FB61_RX_GetLength:
				MessageLength = rx_byte;
				RX_State = FB61_RX_GetMessage;
				break;

		case FB61_RX_GetMessage:
				MessageBuffer[BufferCount] = rx_byte;
				BufferCount ++;

				/* If this is the last byte, it's the checksum */

				if (BufferCount == MessageLength+(MessageLength%2)+2) {

				  if (MessageType != FB61_FRTYPE_ACK)
					FB61_TX_SendAck(MessageType, MessageBuffer[MessageLength-1] & 0x0f);

		  FB61_RX_DispatchMessage();

		  RX_State = FB61_RX_Sync;
		}

					
				break;

		default:
	}
}

char *FB61_PrintDevice(int Device)
{
    switch (Device) {

	case FB61_DEVICE_PHONE:
	  return "Phone";

	case FB61_DEVICE_PC:
	  return "PC";

	default:
	  return "Unknown";
	}
}

	/* FB61_RX_DisplayMessage
	   Called when a message we don't know about is received so that
	   the user can see what is going back and forth, and perhaps shed
	   some more light/explain another message type! */
	
void	FB61_RX_DisplayMessage(void)
{
	int		count;

	fprintf(stdout, _("Msg Destination: %s\n"), FB61_PrintDevice(MessageDestination));
	fprintf(stdout, _("Msg Source: %s\n"), FB61_PrintDevice(MessageSource));

	fprintf(stdout, _("Msg Type: %02x\n"), MessageType);

	fprintf(stdout, _("Msg Unknown: %02x\n"), MessageUnknown);
	fprintf(stdout, _("Msg Len: %02x\nPhone: "), MessageLength);

	for (count = 0; count < MessageLength+(MessageLength%2)+2; count ++) {
	  if (isprint(MessageBuffer[count]))
		fprintf(stdout, "[%02x%c]", MessageBuffer[count], MessageBuffer[count]);
	  else
		fprintf(stdout, "[%02x ]", MessageBuffer[count]);
	}

	fprintf(stdout, "\n");
	fflush(stdout);
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

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */
	out_buffer[current++] = FB61_FRAME_ID;	/* Start of message indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC; /* Source */

	out_buffer[current++] = message_type; /* Type */

	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = message_length+1; /* Length + 1 for seq. nr*/

		/* Copy in data if any. */	
	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current+=message_length;
	}

	out_buffer[current++]=0x40+RequestSequenceNumber;

	RequestSequenceNumber=(RequestSequenceNumber+1) & 0x07;

	/* If the message length is odd we should add pad byte 0x00 */
	if ( (message_length+1) % 2)
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

	/* Uncoment this to see what is PC sending to the phone

	printf("PC: ");

	for (count = 0; count < current; count++) {
		printf("%2x:", out_buffer[count]);
	}

	printf("\n");

	*/

		/* Send it out... */
	if (write(PortFD, out_buffer, current) != current) {
		return (false);
	}
	return (true);
}

int		FB61_TX_SendAck(u8 message_type, u8 message_seq) {

	u8			out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
  int count, current=0;
	unsigned char			checksum;

	printf("[Sending Ack of type %02x, seq: %x]\n", message_type, message_seq);

  /* Now construct the Ack header. */
    out_buffer[current++] = FB61_FRAME_ID;	/* Start of message indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC; /* Source */

	out_buffer[current++] = FB61_FRTYPE_ACK; /* Ack */

	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = 2; /* Ack is always of 2 bytes */

	out_buffer[current++] = message_type; /* Type */
	out_buffer[current++] = message_seq; /* Sequence number */

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
