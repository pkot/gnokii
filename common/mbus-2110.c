/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000 Pavel Machek <pavel@ucw.cz>

  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  $Log$
  Revision 1.21  2000-12-19 16:32:28  pkot
  Lots of updates in common/mbus-2110.c. (thanks to Pavel Machek)


*/

#ifndef WIN32

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "misc.h"
#include "gsm-common.h"
#include "mbus-2110.h"

#define MYID 0x78

#undef DEBUG
#define dprintf(a...) 

/* Global variables used by code in gsm-api.c to expose the
   functions supported by this model of phone.  */
bool MB21_LinkOK;
static char PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
static char *Revision = NULL,
	*RevisionDate = NULL,
	*Model = NULL,
	VersionInfo[64];

GSM_Information MB21_Information = {
	"2110|2140|6080",		/* Models */
	4, 				/* Max RF Level */
	0,				/* Min RF Level */
	GRF_Arbitrary,			/* RF level units */
	4,    				/* Max Battery Level */
	0,				/* Min Battery Level */
	GBU_Arbitrary,			/* Battery level units */
	GDT_None,			/* No date/time support */
	GDT_None,			/* No alarm support */
	0				/* Max alarms = 0 */
};

/* Local variables */
static pthread_t Thread;
static volatile bool RequestTerminate;
static int PortFD;
static struct termios old_termios; /* old termios */
static u8 TXPacketNumber = 0x01;
#define MAX_MODEL_LENGTH 16
static bool ModelValid     = false;
static volatile unsigned char PacketData[256];
static volatile bool
	ACKOK    = false,
	PacketOK = false,
	EchoOK   = false;

static volatile int SMSpos = 0;
static volatile unsigned char SMSData[256];

/* The following functions are made visible to gsm-api.c and friends. */

/* Checksum calculation */
static u8
GetChecksum( u8 * packet, int len )
{
	u8           checksum = 0;
	unsigned int i;

	for( i = 0; i < len; i++ ) checksum ^= packet[i]; /* calculate checksum */
	return checksum;
}

static GSM_Error
SendCommand( u8 *buffer, u8 command, u8 length )
{
	u8             pkt[256];
	int            current = 0;

	pkt[current++] = 0x00;               /* Construct the header.      */
	pkt[current++] = MYID;
	pkt[current++] = length;             /* Set data size              */
	pkt[current++] = command;
	memcpy( pkt + current, buffer, length ); /* Copy in data.          */
	current += length;
	pkt[current++] = TXPacketNumber;         /* Set packet number      */
	pkt[current++] = GetChecksum( pkt, current); /* Calculate and set checksum */
#ifdef DEBUG
	{
		int i;
		u8  b;
		fprintf( stdout, _("PC   : ") );
		for( i = 0; i < current; i++ ) {
			b = pkt[i];
			fprintf( stdout, "[%02X %c]", b, b > 0x20 ? b : '.' );
		}
		fprintf( stdout, "\n" );
	}
#endif /* DEBUG */
	/* Send it out... */
	EchoOK = false;
	if( write(PortFD, pkt, current) != current ) /* BUGGY! */ {
		perror( _("Write error!\n") );
		return (GE_INTERNALERROR);
	}
	/* wait for echo */
	while( !EchoOK && current-- ) 
		usleep(1000);
	if( !EchoOK ) return (GE_TIMEOUT);
	return (GE_NONE);
}

/* Applications should call Terminate to close the serial port. */
static void
Terminate(void)
{
	/* Request termination of thread */
	RequestTerminate = true;
	/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);
	/* Close serial port. */
	if( PortFD >= 0 ) {
		tcsetattr(PortFD, TCSANOW, &old_termios);
		close( PortFD );
	}
}

/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
static GSM_Error
GetMemoryLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
static GSM_Error	WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error
SMS(GSM_SMSMessage *message, int command)
{
	u8 pkt[] = { 0x10, 0x02, 0, 0 };
	int timeout = 2000000;

	SMSpos = 0;
	memset(&SMSData[0], 0, 160);
	ACKOK    = false;
	PacketOK = false;
	timeout  = 10;
	pkt[1] = command;
	pkt[2] = 1; /* == LM_SMS_MEM_TYPE_DEFAULT or LM_SMS_MEM_TYPE_SIM or LM_SMS_MEM_TYPE_ME */
	pkt[3] = message->Location;
	message->MessageNumber = message->Location;

	while (!ACKOK) {
		fprintf(stderr, ":");
		usleep(100000);
		SendCommand(pkt, 0x38 /* LN_SMS_COMMAND */, sizeof(pkt));
	}
	while(!PacketOK) {
		fprintf(stderr, ".");
		usleep(100000);
		if(0) {
			fprintf(stderr, "Impossible timeout?\n");
			return GE_BUSY;
		}
	}
	if (PacketData[3] != 0x37 /* LN_SMS_EVENT */) {
		fprintf(stderr, "Something is very wrong with GetValue\n");
		return GE_BUSY; /* FIXME */
	}
	return (GE_NONE);
}

static GSM_Error
GetSMSMessage(GSM_SMSMessage *m)
{
	int i, len;
	if (SMS(m, 2) != GE_NONE)
		return GE_BUSY; /* FIXME */
	dprintf("Have message?\n");

	if (SMSData[0] != 0x0b) {
		printf("No sms there? (%x/%d)\n", SMSData[0], SMSpos);
		return GE_BUSY;
	}
	dprintf("Status: " );
	switch (SMSData[3]) {
	case 7: m->Type = GST_MO; m->Status = GSS_NOTSENTREAD; dprintf("not sent\n"); break;
	case 5: m->Type = GST_MO; m->Status = GSS_SENTREAD; dprintf("sent\n"); break;
	case 3: m->Type = GST_MT; m->Status = GSS_NOTSENTREAD; dprintf("not read\n"); break;
	case 1: m->Type = GST_MT; m->Status = GSS_SENTREAD; dprintf("read\n"); break;
	}
	len = SMSData[14];
	dprintf("%d bytes: ", len );
	for (i = 0; i<len; i++)
		dprintf("%c", SMSData[15+i]);
	dprintf("\n");

	if (len>160)
		fprintf(stderr, "Magic not allowed\n");
	memset(m->MessageText, 0, 161);
	strncpy(m->MessageText, &SMSData[15], len);
	m->Length = len;
	dprintf("Text is %s\n", m->MessageText);

	m->MessageCenter.No = 0;
	strcpy(m->MessageCenter.Name, "(unknown)");
	strcpy(m->MessageCenter.Number, "(unknown)");
	m->UDHType = GSM_NoUDH;

	return (GE_NONE);
}

static GSM_Error	DeleteSMSMessage(GSM_SMSMessage *message)
{
	printf("deleting..."); fflush(stdout);
	return SMS(message, 3);
}

static GSM_Error	SendSMSMessage(GSM_SMSMessage *SMS, int data_size)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	CancelCall(void)
{
	return (GE_NOTIMPLEMENTED);
}

/* GetRFLevel */

static int
GetValue(u8 index, u8 type)
{
	u8  pkt[] = {0x84, 2, 0};	/* Sending 4 at pkt[0] makes phone crash */
	int timeout, val;
	pkt[0] = index;
	pkt[1] = type;

	PacketOK = false;
	ACKOK    = false;
	timeout        = 10;
	while(!PacketOK) {
		fprintf(stderr, "\nRequesting value");
		usleep(1000000);
		if(!ACKOK) SendCommand(pkt, 0xe5, 3);
		usleep(1000000);
		if(!--timeout || RequestTerminate)
			return(-1);
	}
	if ((PacketData[3] != 0xe6) ||
	    (PacketData[4] != index) || 
	    (PacketData[5] != type))
		fprintf(stderr, "Something is very wrong with GetValue\n");
	val = PacketData[7];
	fprintf(stderr, "Value = %d\n", val );
	return (GE_NONE);
}

static GSM_Error
GetRFLevel(GSM_RFUnits *units, float *level)
{
	int val = GetValue(0x84, 2);
	*level = (float) val / 99.0;	/* This should be / 60.0 for 2110 */
	*units = GRF_Arbitrary;
}

static GSM_Error
GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	int val = GetValue(0x85, 2);
	*level = (float) val / 90.0;	/* 5..first bar, 10..second bar, 90..third bar */
	*units = GBU_Arbitrary;
}

/* Do not know how to fetch IMEI */
static GSM_Error	GetIMEI(char *imei)
{
	return GE_NOTIMPLEMENTED;
}

static GSM_Error GetVersionInfo()
{
	char *s = VersionInfo;
	if (GetValue(0x11, 0x03) == -1)
		return GE_TIMEOUT;

	strncpy( s, &PacketData[6], sizeof(VersionInfo) );

	for( Revision     = s; *s != 0x0A; s++ ) if( !*s ) goto out;
	*s++ = 0;
	for( RevisionDate = s; *s != 0x0A; s++ ) if( !*s ) goto out;
	*s++ = 0;
	for( Model        = s; *s != 0x0A; s++ ) if( !*s ) goto out;
	*s++ = 0;
	dprintf("Revision %s, Date %s, Model %s\n", Revision, RevisionDate, Model );
	ModelValid = true;
out:
	return (GE_NONE);
}

static GSM_Error	GetRevision(char *revision)
{
	GSM_Error err = GE_NONE;

	if(!Revision) err = GetVersionInfo();
	if(err == GE_NONE) strncpy(revision, Revision, 64);

	return err;
}

static GSM_Error	GetModel(char *model)
{
	GSM_Error err = GE_NONE;

	if(!Model) err = GetVersionInfo();
	if(err == GE_NONE) strncpy(model, Model, 64);

	return err;
}

/* This function sends to the mobile phone a request for the SMS Center */

static GSM_Error	GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

/* Our "Not implemented" functions */
static GSM_Error	GetMemoryStatus(GSM_MemoryStatus *Status)
{
	switch(Status->MemoryType) {
	case GMT_ME:
		Status->Used = 0;
		Status->Free = 100;
		break;
	case GMT_LD:
		Status->Used = 5;
		Status->Free = 0;
		break;
	case GMT_ON:
		Status->Used = 1;
		Status->Free = 0;
		break;
	case GMT_SM:
		Status->Used = 0;
		Status->Free = 0;
		break;
	default: return (GE_NOTIMPLEMENTED);
	}
	return (GE_NONE);
}

static GSM_Error	GetSMSStatus(GSM_SMSStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetPowerSource(GSM_PowerSource *source)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetDisplayStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	EnterSecurityCode(GSM_SecurityCode SecurityCode)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetSecurityCodeStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	DialVoice(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	DialData(char *Number, char type, void (* callpassup)(char c))
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetIncomingCallNr(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetNetworkInfo (GSM_NetworkInfo *NetworkInfo)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	WriteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	DeleteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	Netmonitor(unsigned char mode, char *Screen)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SendDTMF(char *String)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetBitmap(GSM_Bitmap *Bitmap)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SetBitmap(GSM_Bitmap *Bitmap)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	Reset(unsigned char type)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	GetProfile(GSM_Profile *Profile)
{
	return (GE_NOTIMPLEMENTED);
}

static GSM_Error	SetProfile(GSM_Profile *Profile)
{
	return (GE_NOTIMPLEMENTED);
}

static bool
SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	return (false);
}

static GSM_Error Unimplemented(void)
{
	return GE_NOTIMPLEMENTED;
}

static void
Display(u8 b, int shift, char *s)
{
	b >>= shift;
	b &= 0x03;
	switch (b) {
	case 0: return;
	case 1: case 2: printf( "!!!" );
	case 3: printf("%s ", s);
	}
}

static int
HandlePacket(void)
{
	switch(PacketData[3]) {
	case 0x12: 
		printf("Display text at (%d,%d: %s)\n", PacketData[6], PacketData[7], &PacketData[8]);
		return 1;
	case 0x2f:
		printf("Display indicators: ");	/* Valid for 2110 */
		Display(PacketData[4], 0, "d");
		Display(PacketData[4], 2, "b");
		Display(PacketData[4], 4, "a");
		Display(PacketData[4], 6, "lights");

		Display(PacketData[5], 0, "service");
		Display(PacketData[5], 2, "scroll_up");
		Display(PacketData[5], 4, "scroll_down");
		Display(PacketData[5], 6, "ABC");

		Display(PacketData[6], 0, "2.>");
		Display(PacketData[6], 2, "1.>");
		Display(PacketData[6], 4, "roam");
		Display(PacketData[6], 6, "handset");

		Display(PacketData[7], 0, "vmail");
		Display(PacketData[7], 2, "envelope");
		Display(PacketData[7], 4, "battbar");
		Display(PacketData[7], 6, "3.>");

		Display(PacketData[8], 0, "?1");
		Display(PacketData[8], 2, "?2");
		Display(PacketData[8], 4, "fieldbar");
		Display(PacketData[8], 6, "ring");
		printf("\n");
		return 1;

	case 0x37:
		/* copy bytes 5+ to smsbuf */
		dprintf("SMSdata:");
		{
			int i;
			for (i=5; i<PacketData[2]+4; i++) {
				SMSData[SMSpos++] = PacketData[i];
				dprintf("%c", PacketData[i]);
			}
			fflush(stdout);
		}
		return ((PacketData[4] & 0xf) != 0);
		/* Make all but last fragment "secret" */

	default: return 0;
	}	
}

/* Handler called when characters received from serial port. 
 * and process them. */
static void
SigHandler(int status)
{
	unsigned char        buffer[256],ack[5],b;
	int                  i,res;
	static unsigned int  Index = 0,
		Length = 5;
	static unsigned char pkt[256];
#ifdef DEBUG
	int                  j;
#endif

	res = read(PortFD, buffer, 256);
	if( res < 0 ) return;
	for(i = 0; i < res ; i++) {
		b = buffer[i];
//	 fprintf(stderr, "(%x)", b, Index);
		if (!Index && b != MYID && b != 0xf8 && b != 0x00) /* MYID is code of computer */ {
			/* something strange goes from phone. Just ignore it */
#ifdef DEBUG
			fprintf( stdout, "Get [%02X %c]\n", b, b >= 0x20 ? b : '.' );
#endif /* DEBUG */
			continue;
		} else {
			pkt[Index++] = b;
			if(Index == 3) {
				Length = b + 6;
				if (b == 0x7f) Length = 5;
				if (b == 0x7e) Length = 8;
			}
			if(Index >= Length) {
				if((pkt[0] == MYID || pkt[0]==0xf8) && pkt[1] == 0x00) /* packet from phone */ {
#ifdef DEBUG
					fprintf( stdout, _("Phone: ") );
					for( j = 0; j < Length; j++ ) {
						b = pkt[j];
						fprintf( stdout, "[%02X %c]", b, b >= 0x20 ? b : '.' );
					}
					fprintf( stdout, "\n" );
#endif /* DEBUG */
					/* ensure that we received valid packet */
					if(pkt[Length - 1] != GetChecksum(pkt, Length-1)) {
						fprintf( stderr, "***bad checksum***");
					} else {
						if((pkt[2] == 0x7F) || (pkt[2] == 0x7E)) /* acknowledge by phone */ {
							if (pkt[2] == 0x7F) {
								fprintf( stderr, "[ack]" );
								/* Set ACKOK flag */
								ACKOK    = true;
								/* Increase TX packet number */
							} else {
								fprintf( stderr, "[registration ack]" );
								MB21_LinkOK = true;
							}
							TXPacketNumber++;
						} else {
							/* Copy packet data  */
							fprintf( stderr, "[data]" );
							memcpy(PacketData,pkt,Length);
							/* send acknowledge packet to phone */
							usleep(100000);
							ack[0] = 0x00;                     /* Construct the header.   */
							ack[1] = pkt[0];                   /* Send back id            */
							ack[2] = 0x7F;                     /* Set special size value  */
							ack[3] = pkt[Length - 2];          /* Send back packet number */
							ack[4] = GetChecksum( ack, 4); /* Set checksum            */
#ifdef DEBUG
							fprintf( stdout, _("PC   : ") );
							for( j = 0; j < 5; j++ ) {
								b = ack[j];
								fprintf( stdout, "[%02X %c]", b, b >= 0x20 ? b : '.' );
							}
							fprintf( stdout, "\n" );
#endif /* DEBUG */
							if( write( PortFD, ack, 5 ) != 5 )
								perror( _("Write error!\n") );
							/* Set validity flag */
							if (!HandlePacket())
								PacketOK = true;
						}
					}
				} else {
					fprintf(stderr, "[echo]\n");
					EchoOK = true;
				}
				/* Look for new packet */
				Index  = 0;
				Length = 5;
			}
		}
	}
}

/* Called by initialisation code to open comm port in asynchronous mode. */
bool OpenSerial(void)
{
	struct termios    new_termios;
	struct sigaction  sig_io;
	unsigned int      flags;

#ifdef DEBUG
	fprintf(stdout, _("Setting MBUS communication with 2110...\n"));
#endif /* DEBUG */
 
	PortFD = open(PortDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if ( PortFD < 0 ) { 
		fprintf(stderr, "Failed to open %s ...\n", PortDevice);
		return (false);
	}

#ifdef DEBUG
	fprintf(stdout, "%s opened...\n", PortDevice);
#endif /* DEBUG */

	sig_io.sa_handler = SigHandler;
	sig_io.sa_flags = 0;
	sigaction (SIGIO, &sig_io, NULL);
	/* Allow process/thread to receive SIGIO */
	fcntl(PortFD, F_SETOWN, getpid());
	/* Make filedescriptor asynchronous. */
	fcntl(PortFD, F_SETFL, FASYNC);
	/* Save old termios */
	tcgetattr(PortFD, &old_termios);
	/* set speed , 8bit, odd parity */
	memset( &new_termios, 0, sizeof(new_termios) );
	new_termios.c_cflag = B9600 | CS8 | CLOCAL | CREAD | PARODD | PARENB; 
	new_termios.c_iflag = 0;
	new_termios.c_lflag = 0;
	new_termios.c_oflag = 0;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;
	tcflush(PortFD, TCIFLUSH);
	tcsetattr(PortFD, TCSANOW, &new_termios);
	/* setting the RTS & DTR bit */
	flags = TIOCM_DTR;
	ioctl(PortFD, TIOCMBIS, &flags);
	flags = TIOCM_RTS;
	ioctl(PortFD, TIOCMBIS, &flags);
#ifdef DEBUG
	ioctl(PortFD, TIOCMGET, &flags);
	fprintf(stdout, _("DTR is %s.\n"), flags & TIOCM_DTR ? _("up") : _("down"));
	fprintf(stdout, _("RTS is %s.\n"), flags & TIOCM_RTS ? _("up") : _("down"));
	fprintf(stdout, "\n");
#endif /* DEBUG */
	return (true);
}

static void SetKey(int c, int up)
{
	u8 reg[] = { 0x7a /* RPC_UI_KEY_PRESS or RPC_UI_KEY_RELEASE */, 0, 1, 0 /* key code */ };
	reg[0] += up;
	reg[3] = c;
	fprintf(stderr, "\n Pressing %d\n", c );
	ACKOK = false;
	PacketOK = false;
	while (!ACKOK) {
		usleep(20000);
		SendCommand( reg, 0xe5, 4 );
	}
	while (!PacketOK)
		usleep(20000);
}

#define CTRL(a) (a&0x1f)
#define POWER CTRL('o')
#define SEND CTRL('t')
#define END CTRL('s')
#define CLR CTRL('h')
#define MENU CTRL('d')
#define ALPHA CTRL('a')
#define PLUS CTRL('b')
#define MINUS CTRL('e')
#define PREV CTRL('p')
#define NEXT CTRL('n')
#define SOFTA CTRL('x')
#define SOFTB CTRL('q')

static char lastkey;

static void PressKey(char c, int i)
{
	lastkey = c;
#define X( a, b ) case a: SetKey(b, i); break;
	switch (c) {
	case '1' ... '9': SetKey(c-'0',i); break;
	X('0', 10)
	X('#', 11)
	X('*', 12)
	X(POWER, 13)
	X(SEND, 14)
	X(END, 15)
	X(PLUS, 16)
	X(MINUS, 17)
	X(CLR, 18)

#if 0
	X(STO, 19)
	X(RCL, 20)
#endif
	X(MENU, 21)
	X(ALPHA, 22)
	X(PREV, 23)
	X(NEXT, 24)
	X(SOFTA, 25)
	X(SOFTB, 26)
#if 0
	X(MUTE, 28)
#endif
	default: printf("Unknown key %d\n", c);
	}
#undef X
}


static void
PressString(char *s, int upcase)
{
	static int lastchar;
	static int lastupcase = 1;

	if (lastchar == *s) {
		fprintf(stderr, "***collision");
		PressKey(ALPHA, 0);
		PressKey(ALPHA, 0);
		lastupcase = 1;
	}

	while (*s) {
		lastchar = *s;
		PressKey(*s, 0);
		if (upcase != lastupcase) {
			fprintf(stderr, "***size change");
			usleep(1500000);
			PressKey(*s, 1);
			lastupcase = upcase;
		}
		s++;
	}
}

static void
HandleKey(char c)
{
	switch(c) {
#define X(a, b) case a: PressString(b, 0); break;
	X('a', "2");
	X('b', "22");
	X('c', "222");
	X('d', "3");
	X('e', "33");
	X('f', "333");
	X('g', "4");
	X('h', "44");
	X('i', "444");
	X('j', "5");
	X('k', "55");
	X('l', "555");
	X('m', "6");
	X('n', "66");
	X('o', "666");
	X('p', "7");
	X('q', "77");
	X('r', "777");
	X('s', "7777");
	X('t', "8");
	X('u', "88");
	X('v', "888");
	X('w', "9");
	X('x', "99");
	X('y', "999");
	X('z', "9999");
#define X(a, b) case a: PressString(b, 1); break;
	X('A', "2");
	X('B', "22");
	X('C', "222");
	X('D', "3");
	X('E', "33");
	X('F', "333");
	X('G', "4");
	X('H', "44");
	X('I', "444");
	X('J', "5");
	X('K', "55");
	X('L', "555");
	X('M', "6");
	X('N', "66");
	X('O', "666");
	X('P', "7");
	X('Q', "77");
	X('R', "777");
	X('S', "7777");
	X('T', "8");
	X('U', "88");
	X('V', "888");
	X('W', "9");
	X('X', "99");
	X('Y', "999");
	X('Z', "9999");
#undef X
	case ' ': PressKey('#', 0); break;
	case '*': case '#':
	case '0' ... '9': PressKey(ALPHA, 0); PressKey(c, 0); PressKey(ALPHA, 0); break;
	default: PressKey(c, 0);
	}
}

static void
HandleString(char *s)
{
	while (*s) {
		HandleKey(*s);
		s++;
	}
	fprintf(stderr,"***end of input");
	PressKey(lastkey, 1);
}

static void
Register(void)
{
	u8 reg[] = { 1, 1, 0x0f, 1, 0x0f };
	SendCommand( reg, 0xe9, 5 );
}

static void
GrabDisplay(void)
{
	/* LN_UC_SHARE, LN_UC_SHARE, LN_UC_RELEASE, LN_UC_RELEASE, LN_UC_KEEP */
	u8  pkt[] = {3, 3, 0, 0, 1};
	int timeout, val;

	PacketOK = false;
	ACKOK    = false;
	timeout        = 10;
	while(!PacketOK) {
		fprintf(stderr, "\nGrabbing display");
		usleep(1000000);
		if(!ACKOK) SendCommand(pkt, 0x19, 5);
		usleep(1000000);
		if(!--timeout || RequestTerminate)
			return(-1);
	}
	if ((PacketData[3] != 0xcd) ||
	    (PacketData[2] != 1) || 
	    (PacketData[4] != 1 /* LN_UC_REQUEST_OK */))
		fprintf(stderr, "Something is very wrong with GrabDisplay\n");
	fprintf(stderr, "Display grabbed okay\n");
	return (GE_NONE);
}


/* This is the main loop for the MB21 functions.  When MB21_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB21_Terminate function. */
static void
ThreadLoop(void)
{
	fprintf(stderr, "Initializing... ");
	/* Do initialisation stuff */
	if (OpenSerial() != true) {
		MB21_LinkOK = false;
		while (!RequestTerminate) {
			usleep (100000);
		}
		return;
	}

	usleep(100000);
	while(!MB21_LinkOK) {
		fprintf(stderr, "registration... ");
		Register();
		usleep(400000);
	}
	fprintf(stderr, "okay\n");
/*
	while(1) {
		char c;
		read(0, &c, 1);
		HandleKey(c);
	}
*/
/*
	while(1) {
		float a,b;
		GetVersionInfo();
		GetRFLevel(&a,&b);
		GetBatteryLevel(&a,&b);
		sleep(5);
	}
*/
/*	GrabDisplay(); */
/*
	{
		GSM_SMSMessage msg;

		msg.Location = 1;
		GetSMSMessage(&msg);
		msg.Location = 2;
		GetSMSMessage(&msg);
		msg.Location = 3;
		GetSMSMessage(&msg);
		msg.Location = 4;
		GetSMSMessage(&msg);
		msg.Location = 5;
		GetSMSMessage(&msg);

	}*/
	while (!RequestTerminate)
		usleep(100000);		/* Avoid becoming a "busy" loop. */
}

/* Initialise variables and state machine. */
static GSM_Error   Initialise(char *port_device, char *initlength,
		       GSM_ConnectionType connection,
		       void (*rlp_callback)(RLP_F96Frame *frame))
{
	int rtn;

	/* ConnectionType is ignored in 640 code. */
	RequestTerminate = false;
	MB21_LinkOK     = false;
	memset(VersionInfo,0,sizeof(VersionInfo));
	strncpy(PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);
	rtn = pthread_create(&Thread, NULL, (void *) ThreadLoop, (void *)NULL);
	if(rtn == EAGAIN || rtn == EINVAL) {
		return (GE_INTERNALERROR);
	}
	return (GE_NONE);
}

GSM_Functions MB21_Functions = {
	Initialise,
	Terminate,
	GetMemoryLocation,
	WritePhonebookLocation,
	GetSpeedDial,
	SetSpeedDial,
	GetMemoryStatus,
	GetSMSStatus,
	GetSMSCenter,
	SetSMSCenter,
	GetSMSMessage,
	DeleteSMSMessage,
	SendSMSMessage,
	Unimplemented,
	GetRFLevel,
	GetBatteryLevel,
	GetPowerSource,
	GetDisplayStatus,
	EnterSecurityCode,
	GetSecurityCodeStatus,
	GetIMEI,
	GetRevision,
	GetModel,
	GetDateTime,
	SetDateTime,
	GetAlarm,
	SetAlarm,
	DialVoice,
	DialData,
	GetIncomingCallNr,
	GetNetworkInfo,
	GetCalendarNote,
	WriteCalendarNote,
	DeleteCalendarNote,
	Netmonitor,
	SendDTMF,
	GetBitmap,
	SetBitmap,
	Unimplemented,
	Unimplemented,
	Reset,
	GetProfile,
	SetProfile,
	SendRLPFrame,
        CancelCall,
	Unimplemented,
	Unimplemented,
	Unimplemented,
	Unimplemented,
	Unimplemented,
	SetKey,
	HandleString
};

#endif

