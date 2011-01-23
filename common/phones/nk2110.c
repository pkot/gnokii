/* -*- linux-c -*-

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 2000-2002 Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2001-2003 Pawel Kot

  Notice that this code was (partly) converted to "new" structure, but it
  does not have code for bus separated. I think that separating it would
  be waste of effort...					--pavel

*/

#include "config.h"

#ifndef WIN32

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#undef DEBUG

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "device.h"
#include "phones/generic.h"
#include "phones/nk2110.h"
#include "phones/nokia.h"

#define MYID 0x78
#define ddprintf(x...)
#define eprintf(x...) fprintf(stderr, x)
#undef DEBUG

static gn_error P2110_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state);

/* Global variables used by code in gsm-api.c to expose the
   functions supported by this model of phone.  */
bool N2110_LinkOK;
static char PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];
static char *Revision = NULL,
	*RevisionDate = NULL,
	*Model = NULL,
	VersionInfo[64];

#define INFO \
{ \
	"2110|2140|6080",		/* Models */ \
	100,				/* Max RF Level */ \
	0,				/* Min RF Level */ \
	GRF_Percentage,			/* RF level units */ \
	100,				/* Max Battery Level */ \
	0,				/* Min Battery Level */ \
	GBU_Percentage,			/* Battery level units */ \
	GDT_None,			/* No date/time support */ \
	GDT_None,			/* No alarm support */ \
	0,				/* Max alarms = 0 */ \
	0, 0,                           /* Startup logo size */ \
	0, 0,                           /* Op logo size */ \
	0, 0                            /* Caller logo size */ \
}

static const SMSMessage_Layout nk2110_deliver = {
	true,						/* Is the SMS type supported */
	/* Last ASCIIZ string */ -1, false, false,	/* SMSC */
	-1, -1, -1, -1, -1, -1, -1, -1, 13, 5, -1,
	-1, -1, -1,					/* Validity */
	/* ASCIIZ second from the end */ -1, false, false,	/* Remote Number */
	 6, -1,						/* Time */
	 1,  3, 14, false				/* Nonstandart fields, User Data */
};


GSM_Information N2110_Information = INFO;

GSM_Phone phone_nokia_2110 = {
	NULL,
	NULL,
	INFO,
	P2110_Functions,
	NULL
};

/* Local variables */
static volatile bool RequestTerminate;
static u8 TXPacketNumber = 0x01;
#define MAX_MODEL_LENGTH 16
static volatile unsigned char PacketData[10240];
static volatile bool
	ACKOK    = false,
	PacketOK = false;

static volatile int SMSpos = 0;
static volatile unsigned char SMSData[10240];

static long long LastChar = 0;

static long long
GetTime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (long long) tv.tv_sec * 1000000 + tv.tv_usec;
}
static void SigHandler(int status);
#define POLLIT do { SigHandler(0); } while (0)

static void
yield(void)
{
	usleep(5000);
}

static void
Wait(long long from, int msec)
{
	while (GetTime() < from + ((long long) msec)*1000) {
		yield();
		POLLIT;
	}
}

static void
Wait2(long long from, int msec)
{
	while (GetTime() < from + ((long long) msec)*1000) {
		yield();
	}
}

#define msleep(x) do { usleep(x*1000); } while (0)
#define msleep_poll(x) do { Wait(GetTime(), x); } while (0)

#define waitfor(condition, maxtime) \
do { \
	long long limit = GetTime() + maxtime*1000; \
	if (!maxtime) limit = 0x7fffffffffffffffULL; \
	while ((!(condition)) && (limit > GetTime())) { \
		yield(); \
		POLLIT; \
	} \
	if (!(limit > GetTime())) dprintf("???? TIMEOUT!"); \
} while(0)

/* Checksum calculation */
static u8
GetChecksum( u8 * packet, int len )
{
	u8 checksum = 0;
	unsigned int i;

	for( i = 0; i < len; i++ ) checksum ^= packet[i]; /* calculate checksum */
	return checksum;
}

/* --------------- */

static int xread(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_read(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				dprintf("I/O error : %m?!\n");
				return -1;
			} else device_select(NULL);
		} else {
			d += res;
			len -= res;
			dprintf("(%d)", len);
		}
	}
	return 0;
}

static int xwrite(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_write(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				dprintf("I/O error : %m?!\n");
				return -1;
			}
		} else {
			d += res;
			len -= res;
			dprintf("<%d>", len);
		}
	}
	return 0;
}

/* --------------------------- */

static gn_error
SendFrame( u8 *buffer, u8 command, u8 length )
{
	u8  pkt[10240], pkt2[10240];
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
		dprintf( "PC   : " );
		for( i = 0; i < current; i++ ) {
			b = pkt[i];
			dprintf( "[%02X %c]", b, b > 0x20 ? b : '.' );
		}
		dprintf( "\n" );
	}
#endif /* DEBUG */
	/* Send it out... */
	ddprintf("(");
	Wait2(LastChar, 3);
	/* I should put my messages at least 2msec apart... */
	ddprintf(")");
	dprintf("writing...");
	LastChar = GetTime();
	if (xwrite(pkt, current) == -1)
		return GN_ERR_INTERNALERROR;
	if (xread(pkt2, current) == -1)
		return GN_ERR_INTERNALERROR;
	dprintf("echook");
	if (memcmp(pkt, pkt2, current)) {
		dprintf("Bad echo?!");
		msleep(1000);
		return GN_ERR_TIMEOUT;
	}
	return GN_ERR_NONE;
}

static gn_error
SendCommand( u8 *buffer, u8 command, u8 length )
{
	int time, retries = 10;
	char pkt[10240];

//	msleep(2);
	while ((time = device_read(pkt, 10240)) != -1) {
		int j;
		char b;
		dprintf("Spurious? (%d)", time);
					dprintf( "Phone: " );
					for( j = 0; j < time; j++ ) {
						b = pkt[j];
						dprintf( "[%02X %c]", b, b >= 0x20 ? b : '.' );
					}
		msleep(2);
	}

	ACKOK = false;
	time = 30;
	while (retries--) {
		long long now;
		SendFrame(buffer, command, length);
		now = GetTime();
		while ((GetTime() - now) < time*1000) {
			if (ACKOK)
				return GN_ERR_NONE;
			yield();
			POLLIT;
		}
		time = 50;		/* 5 seems to be enough */
		dprintf("[resend]");
	}
	dprintf("Command not okay after 10 retries!\n");
	return GN_ERR_BUSY;
}

/* Applications should call Terminate to close the serial port. */
static void
Terminate(void)
{
	/* Request termination of thread */
	RequestTerminate = true;
	device_close();
}

static gn_error
SMS(GSM_SMSMessage *message, int command)
{
	u8 pkt[] = { 0x10, 0x02, 0, 0 };

	SMSpos = 0;
	memset((void *) &SMSData[0], 0, 255);
	PacketOK = false;
	pkt[1] = command;
	pkt[2] = 1; /* == LM_SMS_MEM_TYPE_DEFAULT or LM_SMS_MEM_TYPE_SIM or LM_SMS_MEM_TYPE_ME */
	pkt[3] = message->Number;

	SendCommand(pkt, LM_SMS_COMMAND, sizeof(pkt));
	msleep_poll(300);	/* We have to keep acknowledning phone's data */
	waitfor(PacketOK, 1000);
	if (!PacketOK) {
		dprintf("SMS: No reply came within second!\n");
	}
	if (PacketData[3] != LM_SMS_EVENT) {
		dprintf("Something is very wrong with SMS\n");
		return GN_ERR_BUSY; /* FIXME */
	}
	if ((SMSData[2]) && (SMSData[2] != message->Number)) {
		dprintf("Wanted message @%d, got message at @%d!\n", message->Number, SMSData[2]);
		return GN_ERR_BUSY;
	}
	SMSpos = 0;
	return GN_ERR_NONE;
}


static gn_error
DecodeIncomingSMS(GSM_SMSMessage *m)
{
	gn_error error;
	int len, i;

	error = GN_ERR_NONE;
/*	Should be moved to gsm-sms.c */

	/* SMSData[0] == Memory type */

	ddprintf("Status: " );
	switch (SMSData[3]) {
	case 7: m->Type = SMS_Submit;  /* m->Status = GSS_NOTSENTREAD; */ ddprintf("not sent\n"); break;
	case 5: m->Type = SMS_Submit;  /* m->Status = GSS_SENTREAD;    */ ddprintf("sent\n"); break;
	case 3: m->Type = SMS_Deliver; /* m->Status = GSS_NOTSENTREAD; */ ddprintf("not read\n"); break;
	case 1: m->Type = SMS_Deliver; /* m->Status = GSS_SENTREAD;    */ ddprintf("read\n"); break;
	}

	UnpackDateTime((u8 *)SMSData+7, &m->Time);

	m->Length = len = SMSData[14];
	ddprintf("%d bytes: ", len );
	for (i = 0; i<len; i++)
		ddprintf("%c", SMSData[15+i]);
	ddprintf("\n");

	if (len>160)
		dprintf("Magic not allowed\n");
	memset(m->UserData[0].u.Text, 0, 161);
	strncpy(m->UserData[0].u.Text, (void *) &SMSData[15], len);

	ddprintf("Text is %s\n", m->UserData[0].u.Text);

	/*
	Originator address is at 15+i,
	   followed by message center addres (?)
	*/
	{
		char *s = (char *) &SMSData[15+i];	/* We discard volatile. Make compiler quiet. */
		strcpy(m->RemoteNumber.number, s);
		s+=strlen(s)+1;
		strcpy(m->MessageCenter.Number, s);
		ddprintf("Sender = %s, MessageCenter = %s\n", m->Sender, m->MessageCenter.Name);
	}

	m->MessageCenter.No = 0;
	strcpy(m->MessageCenter.Name, "(unknown)");
	m->UDH_No = 0;

	return error;
}

static gn_error
GetSMSMessage(GSM_Data *data)
{
	GSM_SMSMessage *m = data->SMSMessage;
	if (m->Number > 10)
		return GN_ERR_INVALIDSMSLOCATION;

	if (SMS(m, LM_SMS_READ_STORED_DATA) != GN_ERR_NONE)
		return GN_ERR_BUSY; /* FIXME */
	ddprintf("Have message?\n");

	if (SMSData[0] != LM_SMS_FORWARD_STORED_DATA) {
		ddprintf("No sms there? (%x/%d)\n", SMSData[0], SMSpos);
		return GN_ERR_EMPTYSMSLOCATION;
	}

	data->RawData->Length = 180;
	data->RawData->Data = calloc(data->RawData->Length, 1);
	memcpy(data->RawData->Data, (void *)SMSData+1, 180);
	if (ParseSMS(data, 0))
		dprintf("Error in parsesms?\n");

	msleep_poll(1000);		/* If phone lost our ack, it might retransmit data */
	return GN_ERR_NONE;
}

#if 0
static gn_error
SendSMSMessage(GSM_SMSMessage *m)
{
	if (m->Number > 10)
		return GN_ERR_INVALIDSMSLOCATION;

	if (SMSData[0] != 0x0b) {
		ddprintf("No sms there? (%x/%d)\n", SMSData[0], SMSpos);
		return GN_ERR_EMPTYSMSLOCATION;
	}
	ddprintf("Status: " );
	switch (SMSData[3]) {
	case 7: m->Type = GST_MO; m->Status = GSS_NOTSENTREAD; ddprintf("not sent\n"); break;
	case 5: m->Type = GST_MO; m->Status = GSS_SENTREAD; ddprintf("sent\n"); break;
	case 3: m->Type = GST_MT; m->Status = GSS_NOTSENTREAD; ddprintf("not read\n"); break;
	case 1: m->Type = GST_MT; m->Status = GSS_SENTREAD; ddprintf("read\n"); break;
	}
	return  GN_ERR_NONE);
}
#endif

static gn_error DeleteSMSMessage(GSM_SMSMessage *message)
{
	ddprintf("deleting...");
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

	ddprintf("\nRequesting value(%d)", index);
	SendCommand(pkt, 0xe5, 3);

	waitfor(PacketOK, 0);
	if ((PacketData[3] != 0xe6) ||
	    (PacketData[4] != index) ||
	    (PacketData[5] != type))
		dprintf("Something is very wrong with GetValue\n");
	val = PacketData[7];
	ddprintf( "Value = %d\n", val );
	return (val);
}

static gn_error
GetRFLevel(GSM_RFUnits *units, float *level)
{
	int val = GetValue(0x84, 2);
	float res;
	if (*units == GRF_Arbitrary) {
		res = (100* (float) val) / 60.0;	/* This should be / 99.0 for some models other than nokia-2110 */
		*level = 0;
		if (res > 10)
			*level = 1;
		if (res > 30)
			*level = 2;
		if (res > 50)
			*level = 3;
		if (res > 70)
			*level = 4;
	} else {
		*level = (100* (float) val) / 60.0;	/* This should be / 99.0 for some models other than nokia-2110 */
		*units = GRF_Percentage;
	}
	return GN_ERR_NONE;
}

static gn_error
GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	int val = GetValue(0x85, 2);
	*level = 0;
	if (val >= 5)
		*level = 1;
	if (val >= 10)
		*level = 2;
	if (val >= 90)
		*level = 3;
	if (*units == GBU_Arbitrary) {
	} else {
/*		*level = (100 * (float) val) / 90.0;*/	/* 5..first bar, 10..second bar, 90..third bar */
		*level = *level * 33;
		*units = GBU_Percentage;
	}

	return GN_ERR_NONE;
}

static gn_error GetVersionInfo(void)
{
	char *s = VersionInfo;
	ddprintf("Getting version info...\n");
	if (GetValue(0x11, 0x03) == -1)
		return GN_ERR_TIMEOUT;

	strncpy( s, (void *) &PacketData[6], sizeof(VersionInfo) );

	for( Revision     = s; *s != 0x0A; s++ ) if( !*s ) return GN_ERR_NONE;
	*s++ = 0;
	for( RevisionDate = s; *s != 0x0A; s++ ) if( !*s ) return GN_ERR_NONE;
	*s++ = 0;
	for( Model        = s; *s != 0x0A; s++ ) if( !*s ) return GN_ERR_NONE;
	*s++ = 0;
	ddprintf("Revision %s, Date %s, Model %s\n", Revision, RevisionDate, Model );
	return GN_ERR_NONE;
}

/* Our "Not implemented" functions */
static gn_error
GetMemoryStatus(GSM_MemoryStatus *Status)
{
	switch(Status->MemoryType) {
	case GMT_ME:
		Status->Used = 0;
		Status->Free = 150;
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
		Status->Free = 150;
		break;
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_NONE;
}

static bool
SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	return false;
}

static char *
Display(u8 b, int shift, char *s, char *buf)
{
	b >>= shift;
	b &= 0x03;
	switch (b) {
	case 0: break;
	case 1: case 2: *buf++ = '!';
	case 3: strcpy(buf, s); buf += strlen(s); *buf++ = ' ';
	}
	return buf;
}

static void (*OutputFn)(GSM_DrawMessage *DrawMessage);
static gn_error (*OnSMSFn)(GSM_SMSMessage *m);
int SMSReady = 0;

static int
CheckIncomingSMS(int at)
{
	GSM_SMSMessage m;
	dprintf("Check message at %d\n", at);
	memset(&m, 0, sizeof(m));
	m.Number = at;
	m.MemoryType = GMT_ME;
#if 0
	if (GetSMSMessage(&m) != GN_ERR_NONE) {
		return 0;
	}
#endif
#warning Need to do something with data... somehow.
	if (OnSMSFn)
		OnSMSFn(&m);
	DeleteSMSMessage(&m);
	return 1;
}

static int
HandlePacket(void)
{
	GSM_DrawMessage drawmsg;
	int i;
	int lsize[5] = {10, 10, 10, 3, 12};
	char *t;

	dprintf("[%x]", PacketData[3]);
	switch(PacketData[3]) {
	case 0x12: {			/* Text from display */
		drawmsg.Command = GSM_Draw_ClearScreen;
		if (OutputFn)
			OutputFn(&drawmsg);
		t = (char *)&PacketData[8];
		for (i = 0; i < 5; i++) {
			drawmsg.Command = GSM_Draw_DisplayText;
			drawmsg.Data.DisplayText.x = 0;
			drawmsg.Data.DisplayText.y = ((i+1)*48)/DRAW_MAX_SCREEN_HEIGHT;
			strncpy(drawmsg.Data.DisplayText.text, t, lsize[i]);
			drawmsg.Data.DisplayText.text[ lsize[i] ] = 0;
			if (OutputFn)
				OutputFn(&drawmsg);
			t += lsize[i];
		}
		return 1;
	}
	case 0x2f: {			/* Display lights */
#define	COPYFLAG(x, y, z) if (PacketData[x] & (1 << (y))) drawmsg.Data.DisplayStatus |= (1 << (z))
		drawmsg.Data.DisplayStatus = 0;
		drawmsg.Command = GSM_Draw_DisplayStatus;
		COPYFLAG(7, 2, DS_Unread_SMS);
		if (OutputFn)
			OutputFn(&drawmsg);
#if 0
/* Please convert it. I didn't managed to convert the rest... :-( - bozo */
		char buf[10240], *s = buf;
#undef COPY
#define COPY(x, y, z) s = Display(PacketData[x], y, z, s)
		/* Valid for 2110 */
		COPY(4, 0, "d"); COPY(4, 2, "b"); COPY(4, 4, "a"); COPY(4, 6, "lights");
		COPY(5, 0, "service"); COPY(5, 2, "scroll_up"); COPY(5, 4, "scroll_down"); COPY(5, 6, "ABC");
		COPY(6, 0, "2.>"); COPY(6, 2, "1.>"); COPY(6, 4, "roam"); COPY(6, 6, "handset");
		COPY(7, 0, "vmail"); COPY(7, 2, "envelope"); COPY(7, 4, "battbar"); COPY(7, 6, "3.>");
		COPY(8, 0, "?1"); COPY(8, 2, "?2"); COPY(8, 4, "fieldbar"); COPY(8, 6, "ring");
		*s++ = 0;
		if (OutputFn)
			(*OutputFn)(NULL, buf);
		return 1;
#undef COPY
#endif
	}
	case LM_SMS_EVENT:		/* SMS Data */
		/* copy bytes 5+ to smsbuf */
		ddprintf("SMSdata:");
		{
			int i;
			for (i=5; i<PacketData[2]+4; i++) {
				SMSData[SMSpos++] = PacketData[i];
				ddprintf("%c", PacketData[i]);
			}
			fflush(stdout);
		}

		/* Handle incoming sms notifications */
		{
			switch (SMSData[0]) {
			case LM_SMS_RECEIVED_PP_DATA:
				dprintf("Data came!\n");
				SMSpos = 0;
				return 1;
			case LM_SMS_ALIVE_TEST:
				dprintf("Am I alive?\n");
				SMSpos = 0;
				return 1;
			case LM_SMS_NEW_MESSAGE_INDICATION:
			{
				int i, at;
				SMSpos = 0;
				at = SMSData[2];
				dprintf("New message indicated @%d\n", at);
				for (i=0; i<sizeof(SMSData); i++)
					SMSData[i] = 0;
				SMSReady = at;
				return 1;
			}
			}
		}

		return ((PacketData[4] & 0xf) != 0);
		/* Make all but last fragment "secret" */

	default: dprintf("Unknown response %dx\n", PacketData[3]);
	         return 0;
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
	int                  j;

	res = device_read(buffer, 256);
	if( res < 0 ) return;
	for(i = 0; i < res ; i++) {
		b = buffer[i];
/*	 dprintf("(%x)", b, Index); */
		if (!Index && b != MYID && b != 0xf8 && b != 0x00) /* MYID is code of computer */ {
			/* something strange goes from phone. Just ignore it */
			ddprintf("Get [%02X %c]\n", b, b >= 0x20 ? b : '.' );
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
					ddprintf( "Phone: " );
					for( j = 0; j < Length; j++ ) {
						b = pkt[j];
						ddprintf( "[%02X %c]", b, b >= 0x20 ? b : '.' );
					}
					ddprintf( "\n" );
					/* ensure that we received valid packet */
					if(pkt[Length - 1] != GetChecksum(pkt, Length-1)) {
						dprintf( "***bad checksum***");
					} else {
						if((pkt[2] == 0x7F) || (pkt[2] == 0x7E)) /* acknowledge by phone */ {
							if (pkt[2] == 0x7F) {
								dprintf( "[ack]" );
								/* Set ACKOK flag */
								ACKOK    = true;
								/* Increase TX packet number */
							} else {
								dprintf( "[registration ack]" );
								N2110_LinkOK = true;
							}
							TXPacketNumber++;
						} else {
							/* Copy packet data  */
							dprintf( "[data]" );
							memcpy((void *) PacketData,pkt,Length);
							/* send acknowledge packet to phone */
							msleep(10);
							ack[0] = 0x00;                     /* Construct the header.   */
							ack[1] = pkt[0];                   /* Send back id            */
							ack[2] = 0x7F;                     /* Set special size value  */
							ack[3] = pkt[Length - 2];          /* Send back packet number */
							ack[4] = GetChecksum( ack, 4); /* Set checksum            */
							ddprintf("PC   : ");
							for( j = 0; j < 5; j++ ) {
								b = ack[j];
								ddprintf( "[%02X %c]", b, b >= 0x20 ? b : '.' );
							}
							ddprintf( "\n" );
							LastChar = GetTime();
							if( xwrite( ack, 5 ) == -1 )
								perror( _("Write error!\n") );
							if( xread( ack, 5 ) == -1 )
								perror( _("Read ack error!\n") );

							/* Set validity flag */
							if (!HandlePacket())
								PacketOK = true;
						}
					}
				} else
					dprintf("Got my own echo? That should not be possible!\n");
				/* Look for new packet */
				Index  = 0;
				Length = 5;
			}
		}
	}
	if (SMSReady) {
		int at = SMSReady;
		SMSReady = 0;
		if (!CheckIncomingSMS(at))
			dprintf("Could not find promissed message?\n");
	}
}

/* Called by initialisation code to open comm port in asynchronous mode. */
bool OpenSerial(void)
{
	int result;

	ddprintf("Setting MBUS communication with 2110...\n");

	result = device_open(PortDevice, true, false, false, GCT_Serial);
	if (!result) {
		dprintf( "Failed to open %s ...\n", PortDevice);
		return false;
	}

	ddprintf("%s opened...\n", PortDevice);

	device_changespeed(9600);
	device_setdtrrts(1, 1);
	return true;
}

static gn_error
SetKey(int c, int up)
{
	u8 reg[] = { 0x7a /* RPC_UI_KEY_PRESS or RPC_UI_KEY_RELEASE */, 0, 1, 0 /* key code */ };
	reg[0] += up;
	reg[3] = c;
	dprintf("\n Pressing %d\n", c );
	PacketOK = false;
	SendCommand( reg, 0xe5, 4 );
	waitfor(PacketOK, 0);
	return GN_ERR_NONE;
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
	default: dprintf("Unknown key %d\n", c);
	}
#undef X
}


static void
PressString(char *s, int upcase)
{
	static int lastchar;
	static int lastupcase = 1;

	if (lastchar == *s) {
		dprintf("***collision");
		PressKey(ALPHA, 0);
		PressKey(ALPHA, 0);
		lastupcase = 1;
	}

	while (*s) {
		lastchar = *s;
		PressKey(*s, 0);
		if (upcase != lastupcase) {
			dprintf("***size change");
			msleep_poll(1500);
			PressKey(*s, 1);
			lastupcase = upcase;
		}
		s++;
	}
}

/*
 * This is able to press keys at 62letters/36seconds, tested on message
 * "Tohle je testovaci zprava schvalne za jak dlouho ji to napise"
 * Well, it is possible to write that message in 17seconds...
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

static gn_error
HandleString(char *s)
{
	while (*s) {
		HandleKey(*s);
		s++;
	}
	dprintf("***end of input");
	PressKey(lastkey, 1);
	return GN_ERR_NONE;
}

static void
Register(void)
{
	u8 reg[] = { 1, 1, 0x0f, 1, 0x0f };
	SendFrame( reg, 0xe9, 5 );
}

static gn_error
EnableDisplayOutput(GSM_Statemachine *sm)
{
	/* LN_UC_SHARE, LN_UC_SHARE, LN_UC_RELEASE, LN_UC_RELEASE, LN_UC_KEEP */
	u8  pkt[] = {3, 3, 0, 0, 1};

	msleep_poll(500);
	dprintf("\nShould display output\n");
	if (!OutputFn) {
		pkt[0] = 0;
		pkt[1] = 0;
	}
	PacketOK = false;
	SendCommand(pkt, 0x19, 5);
	dprintf("\nGrabbing display");
	waitfor(PacketOK, 0);
	if ((PacketData[3] != 0xcd) ||
	    (PacketData[2] != 1) ||
	    (PacketData[4] != 1 /* LN_UC_REQUEST_OK */))
		dprintf("Something is very wrong with GrabDisplay\n");
	dprintf("Display grabbed okay (waiting)\n");
	msleep_poll(500);
	dprintf("Okay\n");
	return GN_ERR_NONE;
}

static gn_error SMS_Reserve(GSM_Statemachine *sm)
{
	u8 pkt[] = { 0x10, LM_SMS_RESERVE_PP, LN_SMS_NORMAL_RESERVE };
	PacketOK = false;
	msleep_poll(3000);
	SendCommand(pkt, LM_SMS_COMMAND, sizeof(pkt));
	PacketOK = 0;
	waitfor(PacketOK, 100);
	if (!PacketOK)
		dprintf("No reply trying to reserve SMS-es\n");
	if (PacketData[3] != LM_SMS_EVENT)
		dprintf("Bad reply trying to reserve SMS-es\n");
	if (SMSData[0] != LM_SMS_PP_RESERVE_COMPLETE)
		dprintf("Not okay trying to reserve SMS-es (%d)\n", SMSData[0]);
	SMSpos = 0;
	return GN_ERR_NONE;
}

static gn_error SMS_UnReserve(GSM_Statemachine *sm)
{
	u8 pkt[] = {0x10, LM_SMS_UNRESERVE_PP };
	PacketOK = false;
	msleep_poll(3000);
	SendCommand(pkt, LM_SMS_COMMAND, sizeof(pkt));
	SMSpos = 0;
	return GN_ERR_NONE;
}

/* This is the main loop for the MB21 functions.  When N2110_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the N2110_Terminate function. */
static void
RegisterMe(void)
{
	dprintf("Initializing... ");
	/* Do initialisation stuff */
	LastChar = GetTime();
	if (OpenSerial() != true) {
		N2110_LinkOK = false;
		return;
	}

	msleep(100);
	while(!N2110_LinkOK) {
		dprintf("registration... ");
		Register();
		msleep_poll(100);
	}
	dprintf("okay\n");
}

/* Initialise variables and state machine. */
static gn_error
Initialise(GSM_Statemachine *state)
{
	GSM_Data data;

	/* Copy in the phone info */
	memcpy(&(state->Phone), &phone_nokia_2110, sizeof(GSM_Phone));

	layout.Type = 8;
	layout.SendHeader = 6;
	layout.ReadHeader = 4;
	layout.Deliver = nk2110_deliver;
	RequestTerminate = false;
	N2110_LinkOK     = false;
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	memset(VersionInfo, 0, sizeof(VersionInfo));
	strcpy(PortDevice, state->Link.PortDevice);
	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		RegisterMe();
		break;
	default:
		return GN_ERR_NOTSUPPORTED;
		break;
	}

	SM_Initialise(state);
	GSM_DataClear(&data);

	return GN_ERR_NONE;
}

/* Routine to get specifed phone book location.  Designed to be called by
   application.  Will block until location is retrieved or a timeout/error
   occurs. */

static gn_error
GetPhonebookLocation(GSM_PhonebookEntry *entry)
{
	u8  pkt[] = {0x1a, 0 /* 1 == phone */, 0};
	int i;

	pkt[1] = 3 + (entry->MemoryType != GMT_ME);
	pkt[2] = entry->Location;

	PacketOK = false;
	SendCommand(pkt, LN_LOC_COMMAND, 3);
	waitfor(PacketOK, 0);
	if ((PacketData[3] != 0xc9) ||
	    (PacketData[4] != 0x1a)) {
		dprintf("Something is very wrong with GetPhonebookLocation\n");
		return GN_ERR_BUSY;
	}
	ddprintf("type= %x\n", PacketData[5]);
	ddprintf("location= %x\n", PacketData[6]);
	ddprintf("status= %x\n", PacketData[7]);
	for (i=8; PacketData[i]; i++) {
		ddprintf("%c", PacketData[i]);
	}
	strcpy(entry->Name, (void *)&PacketData[8]);
	i++;
	strcpy(entry->Number, (void *)&PacketData[i]);
	for (; PacketData[i]; i++) {
		ddprintf("%c", PacketData[i]);
	}
	ddprintf("\n");
	entry->Empty = false;
	entry->Group = 0;

	return GN_ERR_NONE;
}

/* Routine to write phonebook location in phone. Designed to be called by
   application code. Will block until location is written or timeout
   occurs. */

static gn_error
WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	u8  pkt[999] = {0x1b, 0 /* 1 == phone */, 0};

	pkt[1] = 3 + (entry->MemoryType != GMT_ME);
	pkt[2] = entry->Location;
	strcpy(&pkt[3], entry->Name);
	strcpy(&pkt[3+strlen(entry->Name)+1], entry->Number);

	PacketOK = false;
	SendCommand(pkt, LN_LOC_COMMAND, 3+strlen(entry->Number)+strlen(entry->Name)+2);
	waitfor(PacketOK, 0);
	printf("okay?\n");
	if ((PacketData[3] != 0xc9) ||
	    (PacketData[4] != 0x1b)) {
		dprintf("Something is very wrong with WritePhonebookLocation\n");
		return GN_ERR_BUSY;
	}
	printf("type= %x\n", PacketData[5]);
	printf("location= %x\n", PacketData[6]);
	printf("status= %x\n", PacketData[7]);
	return GN_ERR_NONE;
}

static gn_error
GetSMSStatus(GSM_SMSMemoryStatus *Status)
{
	Status->Unread = 0;
	Status->Number = 5;
	return GN_ERR_NONE;
}

static gn_error link_Loop(struct timeval *tm)
{
	POLLIT;
	return GN_ERR_NONE;
}

gn_error P2110_Functions(GSM_Operation op, GSM_Data *data, GSM_Statemachine *state)
{
	gn_error err = GN_ERR_NONE;

//	printf("Asked for %d\n", op);
	switch (op) {
	case GOP_Init:
		state->Link.Loop = link_Loop;
		err = Initialise(state);
		break;
	case GOP_Terminate:
		/* Request termination of thread */
		RequestTerminate = true;
		return PGEN_Terminate(data, state);
	case GOP_Identify:
	case GOP_GetModel:
	case GOP_GetRevision:
		if (!Model) err = GetVersionInfo();
		if (err) break;
		if (data->Model) strncpy(data->Model, Model, 64);
		if (data->Revision) strncpy(data->Revision, Revision, 64);
		break;
	case GOP_GetBatteryLevel:
		err = GetBatteryLevel(data->BatteryUnits, data->BatteryLevel);
		break;
	case GOP_GetRFLevel:
		err = GetRFLevel(data->RFUnits, data->RFLevel);
		break;
#if 0
	case GOP_GetMemoryStatus:
		err = N2110_GetMemoryStatus(data, state);
		break;
#endif
	case GOP_DisplayOutput:
		OutputFn = data->DisplayOutput->OutputFn;
		err = EnableDisplayOutput(state);
		break;
	case GOP_GetSMS:
		msleep(100);
		err = GetSMSMessage(data);
		break;
	case GOP_DeleteSMS:
		err = SMS(data->SMSMessage, 3);
		break;
	case GOP_ReadPhonebook:
		err = GetPhonebookLocation(data->PhonebookEntry);
		break;
	case GOP_WritePhonebook:
		err = WritePhonebookLocation(data->PhonebookEntry);
		break;
	case GOP_OnSMS:
		OnSMSFn = data->OnSMS;
		if (OnSMSFn) {
			CheckIncomingSMS(1);
			CheckIncomingSMS(2);
			CheckIncomingSMS(3);
			CheckIncomingSMS(4);
			CheckIncomingSMS(5);
			err = SMS_Reserve(state);
		}
		else	err = SMS_UnReserve(state);
		break;
	case GOP_PollSMS:			/* Our phone is able to notify us... but we do not want to burn 100% CPU polling */
		{
			struct timeval tm;
			static long long lastpoll = 0;
			tm.tv_sec = 0;
			tm.tv_usec = 50000;
			device_select(&tm);

			if (lastpoll + 10000000 < GetTime()) {
				dprintf("Trying to capture leftover messages\n");
				CheckIncomingSMS(1);
				CheckIncomingSMS(2);
				CheckIncomingSMS(3);
				CheckIncomingSMS(4);
				CheckIncomingSMS(5);
			}
			sleep(4);
		}
		break;
	case GOP_PollDisplay:
		break;
	default:
		err = GN_ERR_NOTIMPLEMENTED;
	}
	return err;
}

#endif /* WIN32 */
