/* -*- linux-c -*-

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2000 Pavel Machek <pavel@ucw.cz>

  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  $Log$
  Revision 1.32  2001-02-17 22:40:49  chris
  ATA support

  Revision 1.31  2001/02/17 14:39:19  machek
  Make writing/reading phonebook less verbose (as it should be).

  Revision 1.30  2001/02/01 15:17:30  pkot
  Fixed --identify and added Manfred's manufacturer patch

  Revision 1.29  2001/01/31 11:46:50  machek
  Comment out SendSMSMessage to kill warning.

  Revision 1.28  2001/01/29 17:14:42  chris
  dprintf now in misc.h (and fiddling with 7110 code)

  Revision 1.27  2001/01/23 18:58:51  machek
  Implement collision protocol slightly better. It is still not
  completely okay -- we should check if echo is exactly same characters
  as we sent.

  Revision 1.26  2001/01/23 15:32:38  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.25  2001/01/12 14:09:12  pkot
  More cleanups. This time mainly in the code.

  Revision 1.24  2001/01/10 16:32:17  pkot
  Documentation updates.
  FreeBSD fix for 3810 code.
  Added possibility for deleting SMS just after reading it in gnokii.
  2110 code updates.
  Many cleanups.

  Revision 1.23  2001/01/02 09:09:08  pkot
  Misc fixes and updates.

  Revision 1.22  2000/12/20 09:11:19  pkot
  Fixes to mbus-2110.c to let it compile (by Pavel Machek)

  Revision 1.21  2000/12/19 16:32:28  pkot
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

#undef DEBUG
#include "misc.h"
#include "gsm-common.h"
#include "mbus-2110.h"
#include "phone-nokia.h"

#define MYID 0x78

#undef VELO
#ifdef VELO
#define usleep(x) do { usleep(x); SigHandler(0); } while (0)
#endif

#define msleep(x) do { usleep(x*1000); } while (0)

#define THREAD

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
	0,				/* Max alarms = 0 */
	0, 0,                           /* Startup logo size */
	0, 0,                           /* Op logo size */
	0, 0                            /* Caller logo size */
};

/* Local variables */
#ifdef THREAD
static pthread_t Thread;
#endif
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

static long long LastChar = 0;

long long
GetTime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void
Wait(long long from, int msec)
{
	while (GetTime() < from + ((long long) msec)*1000)
		usleep(5000);
}

/* Checksum calculation */
static u8
GetChecksum( u8 * packet, int len )
{
	u8 checksum = 0;
	unsigned int i;

	for( i = 0; i < len; i++ ) checksum ^= packet[i]; /* calculate checksum */
	return checksum;
}

static GSM_Error
SendFrame( u8 *buffer, u8 command, u8 length )
{
	u8  pkt[256];
	int current = 0;

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
		fprintf( stderr, _("PC   : ") );
		for( i = 0; i < current; i++ ) {
			b = pkt[i];
			fprintf( stderr, "[%02X %c]", b, b > 0x20 ? b : '.' );
		}
		fprintf( stderr, "\n" );
	}
#endif /* DEBUG */
	/* Send it out... */
	EchoOK = false;
	dprintf("(");
	Wait(LastChar, 3);
	/* I should put my messages at least 2msec apart... */
	dprintf(")");
	if (write(PortFD, pkt, current) != current) /* BUGGY, we should handle -EINTR and short writes! */ {
		perror( _("Write error!\n") );
		return (GE_INTERNALERROR);
	}
	/* wait for echo */
	while( !EchoOK && current-- ) 
		usleep(1300);
	if( !EchoOK ) {
		fprintf(stderr, "no echo?!");
		return (GE_TIMEOUT);
	}
	return (GE_NONE);
}

static GSM_Error
SendCommand( u8 *buffer, u8 command, u8 length )
{
	int time, retries = 10;

	ACKOK = false;
	while (retries--) {
		SendFrame(buffer, command, length);
		time = 20;
		while (time--) {
			if (ACKOK)
				return GE_NONE;
			msleep(10);
		}
		fprintf(stderr, "[resend]");
	}
	fprintf(stderr, "Command not okay after 10 retries!\n");
	return GE_NONE;
}

/* Applications should call Terminate to close the serial port. */
static void
Terminate(void)
{
	/* Request termination of thread */
	RequestTerminate = true;
#ifdef THREAD
	/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);
#endif
	/* Close serial port. */
	if( PortFD >= 0 ) {
		tcsetattr(PortFD, TCSANOW, &old_termios);
		close( PortFD );
	}
}

static GSM_Error
SMS(GSM_SMSMessage *message, int command)
{
	u8 pkt[] = { 0x10, 0x02, 0, 0 };

	SMSpos = 0;
	memset((void *) &SMSData[0], 0, 160);
	PacketOK = false;
	pkt[1] = command;
	pkt[2] = 1; /* == LM_SMS_MEM_TYPE_DEFAULT or LM_SMS_MEM_TYPE_SIM or LM_SMS_MEM_TYPE_ME */
	pkt[3] = message->Location;
	message->MessageNumber = message->Location;

	SendCommand(pkt, 0x38 /* LN_SMS_COMMAND */, sizeof(pkt));
	while(!PacketOK) {
		dprintf(".");
		usleep(100000);
		if(0) {
			fprintf(stderr, _("Impossible timeout?\n"));
			return GE_BUSY;
		}
	}
	if (PacketData[3] != 0x37 /* LN_SMS_EVENT */) {
		fprintf(stderr, _("Something is very wrong with GetValue\n"));
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
	if (m->Location > 10)
		return GE_INVALIDSMSLOCATION;

	if (SMSData[0] != 0x0b) {
		dprintf("No sms there? (%x/%d)\n", SMSData[0], SMSpos);
		return GE_EMPTYSMSLOCATION;
	}
	dprintf("Status: " );
	switch (SMSData[3]) {
	case 7: m->Type = GST_MO; m->Status = GSS_NOTSENTREAD; dprintf("not sent\n"); break;
	case 5: m->Type = GST_MO; m->Status = GSS_SENTREAD; dprintf("sent\n"); break;
	case 3: m->Type = GST_MT; m->Status = GSS_NOTSENTREAD; dprintf("not read\n"); break;
	case 1: m->Type = GST_MT; m->Status = GSS_SENTREAD; dprintf("read\n"); break;
	}

	/* Date is at SMSData[7]; this code is copied from fbus-6110.c*/
	{
		int offset = -32 + 7;
		m->Time.Year=10*(SMSData[32+offset]&0x0f)+(SMSData[32+offset]>>4);
		m->Time.Month=10*(SMSData[33+offset]&0x0f)+(SMSData[33+offset]>>4);
		m->Time.Day=10*(SMSData[34+offset]&0x0f)+(SMSData[34+offset]>>4);
		m->Time.Hour=10*(SMSData[35+offset]&0x0f)+(SMSData[35+offset]>>4);
		m->Time.Minute=10*(SMSData[36+offset]&0x0f)+(SMSData[36+offset]>>4);
		m->Time.Second=10*(SMSData[37+offset]&0x0f)+(SMSData[37+offset]>>4);
		m->Time.Timezone=(10*(SMSData[38+offset]&0x07)+(SMSData[38+offset]>>4))/4;
	}

	len = SMSData[14];
	dprintf("%d bytes: ", len );
	for (i = 0; i<len; i++)
		dprintf("%c", SMSData[15+i]);
	dprintf("\n");

	if (len>160)
		fprintf(stderr, "Magic not allowed\n");
	memset(m->MessageText, 0, 161);
	strncpy(m->MessageText, (void *) &SMSData[15], len);
	m->Length = len;
	dprintf("Text is %s\n", m->MessageText);

	/* Originator address is at 15+i,
	   followed by message center addres (?) */
	{
		char *s = (char *) &SMSData[15+i];	/* We discard volatile. Make compiler quiet. */
		strcpy(m->Sender, s);
		s+=strlen(s)+1;
		strcpy(m->MessageCenter.Number, s);
		dprintf("Sender = %s, MessageCenter = %s\n", m->Sender, m->MessageCenter.Name);
	}

	m->MessageCenter.No = 0;
	strcpy(m->MessageCenter.Name, "(unknown)");
	m->UDHType = GSM_NoUDH;

	fprintf(stderr, "(spurious?");
	usleep(1000000);
	fprintf(stderr, "(okay)");

	return (GE_NONE);
}

#if 0
static GSM_Error
SendSMSMessage(GSM_SMSMessage *m)
{
	if (m->Location > 10)
		return GE_INVALIDSMSLOCATION;

	if (SMSData[0] != 0x0b) {
		dprintf("No sms there? (%x/%d)\n", SMSData[0], SMSpos);
		return GE_EMPTYSMSLOCATION;
	}
	dprintf("Status: " );
	switch (SMSData[3]) {
	case 7: m->Type = GST_MO; m->Status = GSS_NOTSENTREAD; dprintf("not sent\n"); break;
	case 5: m->Type = GST_MO; m->Status = GSS_SENTREAD; dprintf("sent\n"); break;
	case 3: m->Type = GST_MT; m->Status = GSS_NOTSENTREAD; dprintf("not read\n"); break;
	case 1: m->Type = GST_MT; m->Status = GSS_SENTREAD; dprintf("read\n"); break;
	}
	return (GE_NONE);
}
#endif

static GSM_Error	DeleteSMSMessage(GSM_SMSMessage *message)
{
	dprintf("deleting...");
	return SMS(message, 3);
}

/* GetRFLevel */

static int
GetValue(u8 index, u8 type)
{
	u8  pkt[] = {0x84, 2, 0};	/* Sending 4 at pkt[0] makes phone crash */
	int val;
	pkt[0] = index;
	pkt[1] = type;

	PacketOK = false;

	dprintf("\nRequesting value(%d)", index);
	SendCommand(pkt, 0xe5, 3);

	while(!PacketOK) {
		usleep(1000000);
	}
	if ((PacketData[3] != 0xe6) ||
	    (PacketData[4] != index) || 
	    (PacketData[5] != type))
		fprintf(stderr, "Something is very wrong with GetValue\n");
	val = PacketData[7];
	dprintf( "Value = %d\n", val );
	return (val);
}

static GSM_Error
GetRFLevel(GSM_RFUnits *units, float *level)
{
	int val = GetValue(0x84, 2);
	*level = (100* (float) val) / 60.0;	/* This should be / 99.0 for some models other than nokia-2110 */
	*units = GRF_Arbitrary;
	return (GE_NONE);
}

static GSM_Error
GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	int val = GetValue(0x85, 2);
	*level = (100 * (float) val) / 90.0;	/* 5..first bar, 10..second bar, 90..third bar */
	*units = GBU_Arbitrary;
	return (GE_NONE);
}

static GSM_Error GetVersionInfo()
{
	char *s = VersionInfo;
	if (GetValue(0x11, 0x03) == -1)
		return GE_TIMEOUT;

	strncpy( s, (void *) &PacketData[6], sizeof(VersionInfo) );

	for( Revision     = s; *s != 0x0A; s++ ) if( !*s ) return (GE_NONE);
	*s++ = 0;
	for( RevisionDate = s; *s != 0x0A; s++ ) if( !*s ) return (GE_NONE);
	*s++ = 0;
	for( Model        = s; *s != 0x0A; s++ ) if( !*s ) return (GE_NONE);
	*s++ = 0;
	dprintf("Revision %s, Date %s, Model %s\n", Revision, RevisionDate, Model );
	ModelValid = true;
	return (GE_NONE);
}

static GSM_Error
GetRevision(char *revision)
{
	GSM_Error err = GE_NONE;

	if(!Revision) err = GetVersionInfo();
	if(err == GE_NONE) strncpy(revision, Revision, 64);

	return err;
}

static GSM_Error
GetModel2110(char *model)
{
	GSM_Error err = GE_NONE;

	if(!Model) err = GetVersionInfo();
	if(err == GE_NONE) strncpy(model, Model, 64);

	return err;
}

/* Our "Not implemented" functions */
static GSM_Error
GetMemoryStatus(GSM_MemoryStatus *Status)
{
	switch(Status->MemoryType) {
	case GMT_ME:
		Status->Used = 0;
		Status->Free = 10;
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
		Status->Free = 10;
		break;
	default:
		return (GE_NOTIMPLEMENTED);
	}
	return (GE_NONE);
}

static bool
SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	return (false);
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
	unsigned char        buffer[256], ack[5], b;
	int                  i, res;
	static unsigned int  Index = 0, Length = 5;
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
							memcpy((void *) PacketData,pkt,Length);
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
#ifndef VELO
	/* Make filedescriptor asynchronous. */
	fcntl(PortFD, F_SETFL, FASYNC);
#endif
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

static GSM_Error
SetKey(int c, int up)
{
	u8 reg[] = { 0x7a /* RPC_UI_KEY_PRESS or RPC_UI_KEY_RELEASE */, 0, 1, 0 /* key code */ };
	reg[0] += up;
	reg[3] = c;
	fprintf(stderr, "\n Pressing %d\n", c );
	PacketOK = false;
	SendCommand( reg, 0xe5, 4 );
	while (!PacketOK)
		usleep(20000);
	return GE_NONE;
}

#define XCTRL(a) (a&0x1f)
#define POWER XCTRL('o')
#define SEND XCTRL('t')
#define END XCTRL('s')
#define CLR XCTRL('h')
#define MENU XCTRL('d')
#define ALPHA XCTRL('a')
#define PLUS XCTRL('b')
#define MINUS XCTRL('e')
#define PREV XCTRL('p')
#define NEXT XCTRL('n')
#define SOFTA XCTRL('x')
#define SOFTB XCTRL('q')

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
	X(MENU, 21)
	X(ALPHA, 22)
	X(PREV, 23)
	X(NEXT, 24)
	X(SOFTA, 25)
	X(SOFTB, 26)
#if 0
	X(STO, 19)	/* These are not present on 2110, so I can't test this. Enable once tested. */
	X(RCL, 20)
	X(MUTE, 28)
#endif
	default: fprintf(stderr, "Unknown key %d\n", c);
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

/*
 * This is able to press keys at 62letters/36seconds, tested on message
 * "Tohle je testovaci zprava schvalne za jak dlouho ji to napise"
 */
static void
HandleKey(char c)
{
	switch(c) {
#define X(a, b) case a: PressString(b, 0); break;
	X('-', "1");
	X('?', "11");
	X('!', "111");
	X(',', "1111");
	X('.', "11111");
	X(':', "111111");
	X('"', "1111111");
	X('\'', "11111111");
	X('&', "111111111");
	X('$', "1111111111");
/*	X('$', "11111111111"); libra, not in ascii */
	X('(', "111111111111");
	X(')', "1111111111111");
	X('/', "11111111111111");
	X('%', "111111111111111");
	X('@', "1111111111111111");
	X('_', "11111111111111111");
	X('=', "111111111111111111");
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
#undef X
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
	case '+': PressKey(ALPHA, 0); PressKey('*', 0); PressKey('*', 0);  PressKey(ALPHA, 0); break;
	case '*': case '#':
	case '0' ... '9': PressKey(ALPHA, 0); PressKey(c, 0); PressKey(ALPHA, 0); break;
	default: PressKey(c, 0);
	}
}

static GSM_Error
HandleString(char *s)
{
	while (*s) {
		HandleKey(*s);
		s++;
	}
	fprintf(stderr,"***end of input");
	PressKey(lastkey, 1);
	return GE_NONE;
}

static void
Register(void)
{
	u8 reg[] = { 1, 1, 0x0f, 1, 0x0f };
	SendFrame( reg, 0xe9, 5 );
}

/* Fixme: implement grabdisplay properly */
static GSM_Error
EnableDisplayOutput(void)
{
	/* LN_UC_SHARE, LN_UC_SHARE, LN_UC_RELEASE, LN_UC_RELEASE, LN_UC_KEEP */
	u8  pkt[] = {3, 3, 0, 0, 1};

	return GE_NOTIMPLEMENTED;	/* We can do grab display, but we do not know how to ungrab it and with
					   display grabbed, we can't even initialize connection. */
	PacketOK = false;

	SendCommand(pkt, 0x19, 5);
	while(!PacketOK) {
		fprintf(stderr, "\nGrabbing display");
		usleep(1000000);
	}
	if ((PacketData[3] != 0xcd) ||
	    (PacketData[2] != 1) || 
	    (PacketData[4] != 1 /* LN_UC_REQUEST_OK */))
		fprintf(stderr, "Something is very wrong with GrabDisplay\n");
	fprintf(stderr, "Display grabbed okay\n");
	return GE_NONE;
}


/* This is the main loop for the MB21 functions.  When MB21_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB21_Terminate function. */
static void
ThreadLoop(void)
{
	fprintf(stderr, "Initializing... ");
	/* Do initialisation stuff */
	LastChar = GetTime();
	if (OpenSerial() != true) {
		MB21_LinkOK = false;
#ifdef THREAD
		while (!RequestTerminate) {
			usleep (100000);
		}
#endif
		return;
	}

	usleep(100000);
	while(!MB21_LinkOK) {
		fprintf(stderr, "registration... ");
		Register();
		msleep(100);
		msleep(100);
		msleep(100);
		msleep(100);
	}
	fprintf(stderr, "okay\n");
#ifdef THREAD
	while (!RequestTerminate)
		msleep(100);		/* Avoid becoming a "busy" loop. */
#endif
}

/* Initialise variables and state machine. */
static GSM_Error   Initialise(char *port_device, char *initlength,
		       GSM_ConnectionType connection,
		       void (*rlp_callback)(RLP_F96Frame *frame))
{
	RequestTerminate = false;
	MB21_LinkOK     = false;
	memset(VersionInfo,0,sizeof(VersionInfo));
	strncpy(PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);
#ifdef THREAD
	{
		int rtn;
		rtn = pthread_create(&Thread, NULL, (void *) ThreadLoop, (void *)NULL);
		if(rtn == EAGAIN || rtn == EINVAL)
			return (GE_INTERNALERROR);
	}
#else
	ThreadLoop();
#endif
	return (GE_NONE);
}

/* Routine to get specifed phone book location.  Designed to be called by
   application.  Will block until location is retrieved or a timeout/error
   occurs. */

GSM_Error GetPhonebookLocation(GSM_PhonebookEntry *entry)
{
	u8  pkt[] = {0x1a, 0 /* 1 == phone */, 0};
	int i;

	pkt[1] = 3 + (entry->MemoryType != GMT_ME);
	pkt[2] = entry->Location;
	
	PacketOK = false;
	SendCommand(pkt, /* LN_LOC_COMMAND */ 0x1f, 3);
	while (!PacketOK)
		msleep(100);
	if ((PacketData[3] != 0xc9) ||
	    (PacketData[4] != 0x1a)) {
		fprintf(stderr, "Something is very wrong with GetPhonebookLocation\n");
		return GE_BUSY;
	}
	dprintf("type= %x\n", PacketData[5]);
	dprintf("location= %x\n", PacketData[6]);
	dprintf("status= %x\n", PacketData[7]);
	for (i=8; PacketData[i]; i++) {
		dprintf("%c", PacketData[i]);
	}
	strcpy(entry->Name, (void *)&PacketData[8]);
	i++;
	strcpy(entry->Number, (void *)&PacketData[i]);
	for (; PacketData[i]; i++) {
		dprintf("%c", PacketData[i]);
	}
	dprintf("\n");
	entry->Empty = false;
	entry->Group = 0;

	return (GE_NONE);
}

/* Routine to write phonebook location in phone. Designed to be called by
   application code. Will block until location is written or timeout
   occurs. */

GSM_Error WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	u8  pkt[999] = {0x1b, 0 /* 1 == phone */, 0};

	pkt[1] = 3 + (entry->MemoryType != GMT_ME);
	pkt[2] = entry->Location;
	strcpy(&pkt[3], entry->Name);
	strcpy(&pkt[3+strlen(entry->Name)+1], entry->Number);
	
	PacketOK = false;
	SendCommand(pkt, /* LN_LOC_COMMAND */ 0x1f, 3+strlen(entry->Number)+strlen(entry->Name)+2);
	while(!PacketOK) {
		usleep(1000000);
	}
	printf("okay?\n");
	if ((PacketData[3] != 0xc9) ||
	    (PacketData[4] != 0x1b)) {
		fprintf(stderr, "Something is very wrong with WritePhonebookLocation\n");
		return GE_BUSY;
	}
	printf("type= %x\n", PacketData[5]);
	printf("location= %x\n", PacketData[6]);
	printf("status= %x\n", PacketData[7]);
	return (GE_NONE);
}

GSM_Error GetSMSStatus(GSM_SMSStatus *Status)
{
	Status->UnRead = 0;
	Status->Number = 5;
	return GE_NONE;
}


GSM_Functions MB21_Functions = {
	Initialise,
	Terminate,
	GetPhonebookLocation,
	WritePhonebookLocation,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	GetMemoryStatus,
	GetSMSStatus,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	GetSMSMessage,
	DeleteSMSMessage,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	GetRFLevel,
	GetBatteryLevel,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	GetRevision,
	GetModel2110,
	PNOK_GetManufacturer,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	SendRLPFrame,
	UNIMPLEMENTED,
	EnableDisplayOutput,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	UNIMPLEMENTED,
	SetKey,
	HandleString,
	UNIMPLEMENTED
};

#endif

