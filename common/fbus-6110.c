/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides an API for accessing functions on the 6110 and similar
  phones. See README for more details on supported mobile phones.

  The various routines are called FB61 (whatever) as a concatenation of FBUS
  and 6110.

  $Log$
  Revision 1.123  2001-01-31 12:49:00  pkot
  Many cleanups in fbus-6110 code.
  3210/3310 really work now.

  Revision 1.122  2001/01/23 15:32:35  chris
  Pavel's 'break' and 'static' corrections.
  Work on logos for 7110.

  Revision 1.121  2001/01/15 17:00:45  pkot
  Initial keypress sequence support. Disable compilation warning

  Revision 1.120  2001/01/14 22:46:56  chris
  Preliminary 7110 support (dlr9 only) and the beginnings of a new structure

  Revision 1.119  2001/01/10 16:32:16  pkot
  Documentation updates.
  FreeBSD fix for 3810 code.
  Added possibility for deleting SMS just after reading it in gnokii.
  2110 code updates.
  Many cleanups.

  Revision 1.118  2001/01/08 15:11:36  pkot
  Documentation updates.
  Fixed some bugs and removed FIXMEs.
  We need to move some stuff from configure.in to aclocal.m4

  Revision 1.117  2000/12/21 15:13:46  pkot
  Fixed functions converting ascii to and from PDU

  Revision 1.116  2000/12/19 16:27:16  pkot
  Added 'static' word to variable declarations in common/fbus-6110.c and Makefile fix. (thanks to Pavel Machek)

*/

/* "Turn on" prototypes in fbus-6110.h */

#define __fbus_6110_c 

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
#ifdef WIN32

#include <windows.h>
#include "win32/winserial.h"

#undef IN
#undef OUT

#define WRITEPHONE(a, b, c) WriteCommBlock(b, c)
#define sleep(x) Sleep((x) * 1000)
#define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
extern HANDLE hPhone;

#else

#define WRITEPHONE(a, b, c) device_write(b, c)
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include "device.h"
#include "unixserial.h"

#endif

/* Various header file */

#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "fbus-6110.h"
#include "fbus-6110-auth.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "phone-generic.h"

/* Global variables used by code in gsm-api.c to expose the functions
   supported by this model of phone. */

bool FB61_LinkOK;

#if defined(__svr4__) || defined(__FreeBSD__)
/* fd opened in device.c */
extern int device_portfd;
#endif

#ifdef WIN32

void FB61_InitializeLink();

#endif

/* Here we initialise model specific functions. */

GSM_Functions FB61_Functions = {
	FB61_Initialise,
	FB61_Terminate,
	FB61_GetMemoryLocation,
	FB61_WritePhonebookLocation,
	FB61_GetSpeedDial,
	FB61_SetSpeedDial,
	FB61_GetMemoryStatus,
	FB61_GetSMSStatus,
	FB61_GetSMSCenter,
	FB61_SetSMSCenter,
	FB61_GetSMSMessage,
	FB61_DeleteSMSMessage,
	FB61_SendSMSMessage,
	FB61_SaveSMSMessage,
	FB61_GetRFLevel,
	FB61_GetBatteryLevel,
	FB61_GetPowerSource,
	FB61_GetDisplayStatus,
	FB61_EnterSecurityCode,
	FB61_GetSecurityCodeStatus,
	FB61_GetIMEI,
	FB61_GetRevision,
	FB61_GetModel,
	FB61_GetDateTime,
	FB61_SetDateTime,
	FB61_GetAlarm,
	FB61_SetAlarm,
	FB61_DialVoice,
	FB61_DialData,
	FB61_GetIncomingCallNr,
	FB61_GetNetworkInfo,
	FB61_GetCalendarNote,
	FB61_WriteCalendarNote,
	FB61_DeleteCalendarNote,
	FB61_NetMonitor,
	FB61_SendDTMF,
	FB61_GetBitmap,
	FB61_SetBitmap,
	FB61_SetRingTone,
	FB61_SendRingTone,
	FB61_Reset,
	FB61_GetProfile,
	FB61_SetProfile,
	FB61_SendRLPFrame,
	FB61_CancelCall,
	FB61_EnableDisplayOutput,
	FB61_DisableDisplayOutput,
	FB61_EnableCellBroadcast,
	FB61_DisableCellBroadcast,
	FB61_ReadCellBroadcast,
	UNIMPLEMENTED,
	UNIMPLEMENTED
};

/* Mobile phone information */

GSM_Information FB61_Information = {
	"6110|6130|6150|6190|5110|5130|5190|3210|3310", /* Supported models */
	4,                     /* Max RF Level */
	0,                     /* Min RF Level */
	GRF_Arbitrary,         /* RF level units */
	4,                     /* Max Battery Level */
	0,                     /* Min Battery Level */
	GBU_Arbitrary,         /* Battery level units */
	GDT_DateTime,          /* Have date/time support */
	GDT_TimeOnly,	         /* Alarm supports time only */
	1,                     /* Only one alarm available */
	48, 84,                /* Startup logo size */
	14, 72,                /* Op logo size */
	14, 72                 /* Caller logo size */
};

unsigned char GSM_Default_Alphabet[] = {

	/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
	/* Characters in hex position 10, [12 to 1a] and 24 are not present on
	   latin1 charset, so we cannot reproduce on the screen, however they are
	   greek symbol not present even on my Nokia */

	'@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec, 
	0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
	'?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
	' ',  '!',  '\"', '#',  0xa4,  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
	0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

const char *FB61_MemoryType_String [] = {
	"", 	/* 0x00 */
	"MT", /* 0x01 */
	"ME", /* 0x02 */
	"SM", /* 0x03 */
	"FD", /* 0x04 */
	"ON", /* 0x05 */
	"EN", /* 0x06 */
	"DC", /* 0x07 */
	"RC", /* 0x08 */
	"MC", /* 0x09 */
};

/* Local variables */


char PortDevice[GSM_MAX_DEVICE_NAME_LENGTH];

/* This is the connection type used in gnokii. */

GSM_ConnectionType CurrentConnectionType;

int BufferCount;

u8 MessageBuffer[FB61_MAX_RECEIVE_LENGTH * 6];

u16 MessageLength;
u8 MessageType, MessageDestination, MessageSource, MessageUnknown;
u8 MessagesSent=0, AcksReceived=0;

/* Magic bytes from the phone. */

unsigned char MagicBytes[4] = { 0x00, 0x00, 0x00, 0x00 };
GSM_Error CurrentMagicError = GE_BUSY;

enum FB61_RX_States RX_State;
bool RX_Multiple = false;

u8        RequestSequenceNumber = 0x00;
bool      RequestTerminate;
bool      DisableKeepalive = false;
int       InitLength;
u8        CallSequenceNumber; /* Used to disconnect the call */

#ifndef WIN32

pthread_t Thread;
# if defined(__svr4__) || defined(__FreeBSD__)
pthread_t selThread;
# endif

#endif

/* Local variables used by get/set phonebook entry code. Buffer is used as a
   source or destination for phonebook data and other functions... Error is
   set to GE_NONE by calling function, set to GE_COMPLETE or an error code by
   handler routines as appropriate. */
		   	   	   
static GSM_PhonebookEntry *CurrentPhonebookEntry;
static GSM_Error          CurrentPhonebookError;

static GSM_SpeedDial      *CurrentSpeedDialEntry;
static GSM_Error          CurrentSpeedDialError;

static GSM_SMSMessage     *CurrentSMSMessage;
static GSM_Error          CurrentSMSMessageError;
static int                CurrentSMSPointer;

static GSM_MemoryStatus   *CurrentMemoryStatus;
static GSM_Error          CurrentMemoryStatusError;

static GSM_NetworkInfo    *CurrentNetworkInfo = NULL;
static GSM_Error          CurrentNetworkInfoError;

static GSM_SMSStatus      *CurrentSMSStatus;
static GSM_Error          CurrentSMSStatusError;

static GSM_MessageCenter  *CurrentMessageCenter;
static GSM_Error          CurrentMessageCenterError;

static int                *CurrentSecurityCodeStatus;
static GSM_Error          CurrentSecurityCodeError;

static GSM_DateTime       *CurrentDateTime;
static GSM_Error          CurrentDateTimeError;

static GSM_DateTime       *CurrentAlarm;
static GSM_Error          CurrentAlarmError;

static GSM_CalendarNote   *CurrentCalendarNote;
static GSM_Error          CurrentCalendarNoteError;

static GSM_Error          CurrentSetDateTimeError;
static GSM_Error          CurrentSetAlarmError;

static int                CurrentRFLevel,
	CurrentBatteryLevel,
	CurrentPowerSource;

static int                DisplayStatus;
static GSM_Error          DisplayStatusError;

static char               *CurrentNetmonitor;
static GSM_Error          CurrentNetmonitorError;

static GSM_Bitmap         *GetBitmap=NULL;
static GSM_Error          GetBitmapError;

static GSM_Error          SetBitmapError;

static GSM_Profile        *CurrentProfile;
static GSM_Error          CurrentProfileError;

static GSM_Error          CurrentDisplayOutputError;

static GSM_CBMessage      *CurrentCBMessage;
static GSM_Error          CurrentCBError;

static GSM_Error          CurrentPhoneInfoError;

static unsigned char      IMEI[FB61_MAX_IMEI_LENGTH];
static unsigned char      Revision[FB61_MAX_REVISION_LENGTH];
static unsigned char      Model[FB61_MAX_MODEL_LENGTH];

static char               CurrentIncomingCall[20] = " ";

/* Pointer to callback function in user code to be called when RLP frames
   are received. */

void               (*RLP_RXCallback)(RLP_F96Frame *frame);

/* Pointer to a callback function used to return changes to a calls status */
/* This saves unreliable polling */
void (*CallPassup)(char c);

#ifdef WIN32
/* called repeatedly from a separate thread */
void FB61_KeepAliveProc()
{
	if (!DisableKeepalive)
		FB61_TX_SendStatusRequest();
	Sleep(2000);
}
#endif

/* Initialise variables and state machine. */

GSM_Error FB61_Initialise(char *port_device, char *initlength,
                          GSM_ConnectionType connection,
                          void (*rlp_callback)(RLP_F96Frame *frame))
{
	int rtn;

	RequestTerminate = false;
	FB61_LinkOK = false;
	RLP_RXCallback = rlp_callback;
	CallPassup = NULL;

	strncpy(PortDevice, port_device, GSM_MAX_DEVICE_NAME_LENGTH);

	InitLength = atoi(initlength);
	if (!strcmp(initlength, "default") || (InitLength == 0)) {
		InitLength = 250;	/* This is the usual value, lower may work. */
	}

	CurrentConnectionType = connection;

	/* Create and start main thread. */

#ifdef WIN32
	DisableKeepalive = true;
	rtn = ! OpenConnection(PortDevice, FB61_RX_StateMachine, FB61_KeepAliveProc);
	if (rtn == 0) {
		FB61_InitializeLink(); /* makes more sense to do this in 'this' thread */
		DisableKeepalive = false;
	}
#else
	rtn = pthread_create(&Thread, NULL, (void *)FB61_ThreadLoop, (void *)NULL);
#endif

	if (rtn != 0)
		return (GE_INTERNALERROR);

	return (GE_NONE);
}

#if defined(__svr4__) || defined(__FreeBSD__)
/* thread for handling incoming data */
void FB61_SelectLoop()
{
	int err;
	fd_set readfds;
	struct timeval timeout;

	FD_ZERO(&readfds);
	FD_SET(device_portfd, &readfds);
	/* set timeout to 15 seconds */
	timeout.tv_sec=15;
	timeout.tv_usec=0;
	while (!RequestTerminate) {
		err = select(device_portfd + 1, &readfds, NULL, NULL, &timeout);
		/* call singal handler to process incoming data */
		if ( err > 0 ) FB61_SigHandler(0);
		else if (err == -1) perror("Error in SelectLoop");
	}
}
#endif

  
/* This function send the status request to the phone. */

GSM_Error FB61_TX_SendStatusRequest(void)
{
	/* The status request is of the type 0x04. It's subtype is 0x01. If you have
	   another subtypes and it's meaning - just inform Pavel, please. */
	unsigned char request[] = {FB61_FRAME_HEADER, 0x01};
	FB61_TX_SendMessage(4, 0x04, request);
	return (GE_NONE);
}

/* This function translates GMT_MemoryType to FB61_MEMORY_xx */

int FB61_GetMemoryType(GSM_MemoryType memory_type)
{
	int result;
	switch (memory_type) {
	case GMT_MT: result = FB61_MEMORY_MT; break;
	case GMT_ME: result = FB61_MEMORY_ME; break;
	case GMT_SM: result = FB61_MEMORY_SM; break;
	case GMT_FD: result = FB61_MEMORY_FD; break;
	case GMT_ON: result = FB61_MEMORY_ON; break;
	case GMT_EN: result = FB61_MEMORY_EN; break;
	case GMT_DC: result = FB61_MEMORY_DC; break;
	case GMT_RC: result = FB61_MEMORY_RC; break;
	case GMT_MC: result = FB61_MEMORY_MC; break;
	default:     result = FB61_MEMORY_XX; break;
	}
	return (result);
}

/* This function is used to get storage status from the phone. It currently
   supports two different memory areas - internal and SIM. */

static GSM_Error 
wait_on(volatile GSM_Error *what, int timeout)
{
	*what = GE_BUSY;
	while (timeout && (*what == GE_BUSY)) {
		if (!--timeout)
			return GE_TIMEOUT;
		usleep(100000);
	}
	return *what;
}

#define WAIT_ON(x, y) \
	{ \
		GSM_Error res = wait_on(x, y); \
		if (res != GE_NONE) \
			return res; \
        }

#define WAIT_ON1(x, y) \
	{ \
		GSM_Error res = wait_on(x, y); \
		if (res != GE_NONE) \
			return; \
        }

static GSM_Error FB61_GetPhoneInfo()
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x03, 0x00 };
	FB61_TX_SendMessage(5, 0xd1, req);
	return (wait_on(&CurrentPhoneInfoError, 20));
}

GSM_Error FB61_GetMemoryStatus(GSM_MemoryStatus *Status)
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x07 /* MemoryStatus request */, 0x00 /* MemoryType */ };
	CurrentMemoryStatus = Status;
	req[4] = FB61_GetMemoryType(Status->MemoryType);
	FB61_TX_SendMessage(5, 0x03, req);
	return wait_on(&CurrentMemoryStatusError, 20);
}

GSM_Error FB61_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo)
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x70 };
	CurrentNetworkInfo = NetworkInfo;
	FB61_TX_SendMessage(4, 0x0a, req);
	return wait_on(&CurrentNetworkInfoError, 20);
}

GSM_Error FB61_EnableDisplayOutput(void)
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x53, 0x01};
	FB61_TX_SendMessage(5, 0x0d, req);
	return wait_on(&CurrentDisplayOutputError, 20);
}
 
GSM_Error FB61_DisableDisplayOutput(void)
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x53, 0x02};
	FB61_TX_SendMessage(5, 0x0d, req);
	return wait_on(&CurrentDisplayOutputError, 20);
}

GSM_Error FB61_GetProfile(GSM_Profile *Profile)
{
	int i;
	/* Hopefully is 64 larger as FB38_MAX* / FB61_MAX* */
	char model[64];
	unsigned char name_req[] = { FB61_FRAME_HEADER, 0x1a, 0x00};
	unsigned char feat_req[] = { FB61_FRAME_HEADER, 0x13, 0x01, 0x00, 0x00};

	CurrentProfile = Profile;

	name_req[4] = Profile->Number;
	feat_req[5] = Profile->Number;
	FB61_TX_SendMessage(5, 0x05, name_req);

	WAIT_ON(&CurrentProfileError, 20);

	for (i = 0x00; i <= 0x09; i++) {
		CurrentProfileError = GE_BUSY;
		feat_req[6] = i;
		FB61_TX_SendMessage(7, 0x05, feat_req);
		WAIT_ON(&CurrentProfileError, 20);
	}

	if (Profile->DefaultName > -1)
	{
		while (FB61_GetModel(model)  != GE_NONE)
			sleep(1);

		/*For N5110*/
		/*FIX ME: It should be set for N5130 and 3210 too*/
		if (!strcmp(model,"NSE-1")) {
			switch (Profile->DefaultName) {
			case 0x00: sprintf(Profile->Name, "Personal"); break;
			case 0x01: sprintf(Profile->Name, "Car"); break;
			case 0x02: sprintf(Profile->Name, "Headset"); break;
			default:   sprintf(Profile->Name, "Unknown (%i)", Profile->DefaultName); break;
			}
		} else {
			switch (Profile->DefaultName) {
			case 0x00: sprintf(Profile->Name, "General"); break;
			case 0x01: sprintf(Profile->Name, "Silent"); break;
			case 0x02: sprintf(Profile->Name, "Meeting"); break;
			case 0x03: sprintf(Profile->Name, "Outdoor"); break;
			case 0x04: sprintf(Profile->Name, "Pager"); break;
			case 0x05: sprintf(Profile->Name, "Car"); break;
			case 0x06: sprintf(Profile->Name, "Headset"); break;
			default: sprintf(Profile->Name, "Unknown (%i)", Profile->DefaultName); break;
			}
		}
	}
	return (GE_NONE);
}

GSM_Error FB61_SetProfile(GSM_Profile *Profile)
{
	int i;
	unsigned char name_req[40] = { FB61_FRAME_HEADER, 0x1c, 0x01, 0x03, 0x00, 0x00, 0x00};
	unsigned char feat_req[] = { FB61_FRAME_HEADER, 0x10, 0x01, 0x00, 0x00, 0x00};

	name_req[7] = Profile->Number;
	name_req[8] = strlen(Profile->Name);
	name_req[6] = name_req[8] + 2;

	for (i = 0; i < name_req[8]; i++)
		name_req[9 + i] = Profile->Name[i];

	FB61_TX_SendMessage(name_req[8] + 9, 0x05, name_req);
	WAIT_ON(&CurrentProfileError, 30);

	feat_req[5] = Profile->Number;
	for (i = 0x00; i <= 0x09; i++) {
		feat_req[6] = i;
		switch (feat_req[6]) {
		case 0x00: feat_req[7] = Profile->KeypadTone; break;
		case 0x01: feat_req[7] = Profile->Lights; break;
		case 0x02: feat_req[7] = Profile->CallAlert; break;
		case 0x03: feat_req[7] = Profile->Ringtone; break;
		case 0x04: feat_req[7] = Profile->Volume; break;
		case 0x05: feat_req[7] = Profile->MessageTone; break;
		case 0x06: feat_req[7] = Profile->Vibration; break;
		case 0x07: feat_req[7] = Profile->WarningTone; break;
		case 0x08: feat_req[7] = Profile->CallerGroups; break;
		case 0x09: feat_req[7] = Profile->AutomaticAnswer; break;
		}

		FB61_TX_SendMessage(8, 0x05, feat_req);
		WAIT_ON(&CurrentProfileError, 20);
	}

	return (GE_NONE);
}

bool FB61_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	u8 req[60] = { 0x00, 0xd9 };
	/* Discontinuos transmission (DTX).  See section 5.6 of GSM 04.22 version 7.0.1. */
       
	if (out_dtx) req[1] = 0x01;
	memcpy(req + 2, (u8 *) frame, 32);
	return (FB61_TX_SendFrame(32, 0xf0, req));
}


GSM_Error FB61_GetCalendarNote(GSM_CalendarNote *CalendarNote)
{
	unsigned char req[5] = { FB61_FRAME_HEADER, 0x66, 0x00 };
	req[4] = CalendarNote->Location;
	CurrentCalendarNote = CalendarNote;
	CurrentCalendarNoteError = GE_BUSY;
	FB61_TX_SendMessage(5, 0x13, req);
	return wait_on(&CurrentCalendarNoteError, 20);
}

GSM_Error FB61_WriteCalendarNote(GSM_CalendarNote *CalendarNote)
{
	unsigned char req[200] = { FB61_FRAME_HEADER,
				   0x64, 0x01, 0x10,
				   0x00, /* Length of the rest of the frame. */
				   0x00, /* The type of calendar note */
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	int i, current;

	req[7] = CalendarNote->Type;

	req[8]  = CalendarNote->Time.Year / 256;
	req[9]  = CalendarNote->Time.Year % 256;
	req[10] = CalendarNote->Time.Month;
	req[11] = CalendarNote->Time.Day;
	req[12] = CalendarNote->Time.Hour;
	req[13] = CalendarNote->Time.Minute;
	req[14] = CalendarNote->Time.Timezone;

	if (CalendarNote->Alarm.Year) {
		req[15] = CalendarNote->Alarm.Year / 256;
		req[16] = CalendarNote->Alarm.Year % 256;
		req[17] = CalendarNote->Alarm.Month;
		req[18] = CalendarNote->Alarm.Day;
		req[19] = CalendarNote->Alarm.Hour;
		req[20] = CalendarNote->Alarm.Minute;
		req[21] = CalendarNote->Alarm.Timezone;
	}

	req[22] = strlen(CalendarNote->Text);

	current = 23;

	for (i=0; i<strlen(CalendarNote->Text); i++)
		req[current++] = CalendarNote->Text[i];

	req[current++] = strlen(CalendarNote->Phone);

	for (i=0; i<strlen(CalendarNote->Phone); i++)
		req[current++] = CalendarNote->Phone[i];

	CurrentCalendarNote = CalendarNote;
	CurrentCalendarNoteError = GE_BUSY;

	FB61_TX_SendMessage(current, 0x13, req);
	return wait_on(&CurrentCalendarNoteError, 20);
}

GSM_Error FB61_DeleteCalendarNote(GSM_CalendarNote *CalendarNote)
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x68, 0x00 };
	req[4] = CalendarNote->Location;
	CurrentCalendarNoteError = GE_BUSY;
	FB61_TX_SendMessage(5, 0x13, req);
	return wait_on(&CurrentCalendarNoteError, 20);
}

/* Init ir for win32 is made in winserial.c */
#ifndef WIN32

void FB61_InitIR(void)
{
	int i;
	unsigned char init_char     = FB61_SYNC_BYTE;
	unsigned char end_init_char = FB61_IR_END_INIT_BYTE;
  
	for ( i = 0; i < 32; i++ )
		device_write(&init_char, 1);

	device_write(&end_init_char, 1);
	usleep(100000);
}

bool FB61_InitIR115200(void)
{
	int PortFD;
	u8 connect_seq[] = {FB61_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};

	bool ret         = true;
	u8 nr_read       = 0;
	u8 in_buffer[255];
	struct timeval timeout;
	fd_set ready;
	int no_timeout   = 0;
	int i;
	int done         = 0;

	/* send the connection sequence to phone */
	FB61_TX_SendMessage(7, 0x02, connect_seq);

	/* Wait for 1 sec. */
	timeout.tv_sec  = 1;
	timeout.tv_usec = 0;

	PortFD = device_getfd();

	do {
		FD_ZERO(&ready);
		FD_SET(PortFD, &ready);
		no_timeout = select(PortFD + 1, &ready, NULL, NULL, &timeout);
		if ( FD_ISSET(PortFD, &ready) ) {
			nr_read = read(PortFD, in_buffer, 1);
			if ( nr_read >= 1 ) {
				for (i = 0; i < nr_read; i++) {
					if ( in_buffer[i] == FB61_IR_FRAME_ID ) {
						done = 1;
						ret = true;
						break;
					}
				}
			} else {
				done = 1;
				ret = false;
			}
		}

		if (!no_timeout) {
			dprintf(_("Timeout in IR-mode\n"));
			done = 1;
			ret = false;
		}
	} while (!done);
  
	return (ret);
}

/* This function is used to open the IR connection with the phone. */

bool FB61_OpenIR(void)
{
	int result;
	bool ret = false;
	u8 i = 0;

#if defined(__svr4__) || defined(__FreeBSD__)
	int rtn;
#else
	struct sigaction sig_io;  

	/* Set up and install handler before enabling async IO on port. */
  
	sig_io.sa_handler = FB61_SigHandler;
	sig_io.sa_flags = 0;
	sigaction (SIGIO, &sig_io, NULL);
#endif

	/* Open device. */
  
	result = device_open(PortDevice, false, true);

	if (!result) {
		perror(_("Couldn't open FB61 infrared device"));
		return false;
	}

#if defined(__svr4__) || defined(__FreeBSD__)
	/* create a thread to handle incoming data from mobile phone */
	rtn = pthread_create(&selThread, NULL, (void*)FB61_SelectLoop, (void*)NULL);
	if (rtn != 0) return false;
#endif

	device_changespeed(9600);
	FB61_InitIR();
	device_changespeed(115200);
	ret = FB61_InitIR115200();
	if (!ret) {
		for ( i = 0; i < 4 ; i++) {
			usleep (500000);
			device_changespeed(9600);
			FB61_InitIR();
			device_changespeed(115200);
			ret = FB61_InitIR115200();
			if (ret) break;
		}
	}
  
	return (ret);
}

static void FB61_Authentication()
{
	unsigned char connect1[] = {FB61_FRAME_HEADER, 0x0d, 0x00, 0x00, 0x02};
	unsigned char connect2[] = {FB61_FRAME_HEADER, 0x20, 0x02};
	unsigned char connect3[] = {FB61_FRAME_HEADER, 0x0d, 0x01, 0x00, 0x02};
	unsigned char connect4[] = {FB61_FRAME_HEADER, 0x10};

	unsigned char magic_connect[] = {FB61_FRAME_HEADER,
					 0x12,

					 /* The real magic goes here ... These bytes are filled in with the
					    external function FB61_GetNokiaAuth(). */

					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

					 /* NOKIA&GNOKII Accessory */

					 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x26, 0x4e, 0x4f, 0x4b, 0x49, 0x41, 0x20,
					 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x79,
  
					 0x00, 0x00, 0x00, 0x00};

	usleep(100); FB61_GetPhoneInfo();

	if (*Model && (GetPhoneModel(Model)->flags & PM_AUTHENTICATION)) {
		usleep(100); FB61_TX_SendMessage(7, 0x02, connect1);
		usleep(100); FB61_TX_SendMessage(5, 0x02, connect2);
		usleep(100); FB61_TX_SendMessage(7, 0x02, connect3);
		usleep(100); FB61_TX_SendMessage(4, 0x64, connect4);

		WAIT_ON1(&CurrentMagicError, 50);

		FB61_GetNokiaAuth(IMEI, MagicBytes, magic_connect+4);

		FB61_TX_SendMessage(45, 0x64, magic_connect);
	}

	return;
}

/* This is the main loop for the FB61 functions. When FB61_Initialise is
   called a thread is created to run this loop. This loop is exited when the
   application calls the FB61_Terminate function. */
void FB61_ThreadLoop(void)
{
	unsigned char init_char = 0x55;
	int count, idle_timer;

	CurrentPhonebookEntry = NULL;  

	if ( CurrentConnectionType == GCT_Infrared ) {
		dprintf(_("Starting IR mode...!\n"));
		if (FB61_OpenIR() != true) {
			FB61_LinkOK = false;
			while (!RequestTerminate)
				usleep (100000);
			return;
		}

	} else { /* CurrentConnectionType == GCT_Serial */

		/* Try to open serial port, if we fail we sit here and don't proceed to the
		   main loop. */
   
		if (FB61_OpenSerial() != true) {
			FB61_LinkOK = false;
      
			/* Fail so sit here till calling code works out there is a problem. */
      
			while (!RequestTerminate)
				usleep (100000);
      
			return;
		}
	}

	/* Initialise link with phone or what have you */

	/* Send init string to phone, this is a bunch of 0x55 characters. Timing is
	   empirical. */

	for (count = 0; count < InitLength; count ++) {
		usleep(100);
		WRITEPHONE(PortFD, &init_char, 1);
	}

	FB61_TX_SendStatusRequest();

	FB61_Authentication();

	/* FIXME: we should implement better support for ringtones and the utility
	   to set ringtones. */

	// FB61_SendRingtoneRTTTL("/tmp/barbie.txt");

	idle_timer = 0;

	/* Now enter main loop */

	while (!RequestTerminate) {
		if (idle_timer == 0) {
			/* Dont send keepalive and status packets when doing other transactions. */
			if (!DisableKeepalive) FB61_TX_SendStatusRequest();
			idle_timer = 20;
		} else
			idle_timer--;

		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}
}
#endif /* WIN32 */

#ifdef WIN32
void FB61_InitializeLink()
{
	int count;
	DCB        dcb;

	unsigned char init_char = 0x55;
	/* Daxer */
	unsigned char end_init_char = 0xc1;

	/* Daxer Added for infared support */
	if ( CurrentConnectionType == GCT_Infrared ) {
		dcb.DCBlength = sizeof(DCB);

		GetCommState(hPhone, &dcb);
		dcb.BaudRate = CBR_9600;
		SetCommState(hPhone, &dcb);
	}

	/* Send init string to phone, this is a bunch of 0x55 characters. Timing is
	   empirical. */

	for (count = 0; count < 32; count ++) {
		usleep(100);
		WRITEPHONE(PortFD, &init_char, 1);
	}

	WRITEPHONE(PortFD, &end_init_char, 1);

	if ( CurrentConnectionType == GCT_Infrared ) {
		dcb.BaudRate = CBR_115200;
		SetCommState(hPhone, &dcb);
	}
 
	FB61_TX_SendStatusRequest();

	FB61_Authentication();
}
#endif

/* Applications should call FB61_Terminate to shut down the FB61 thread and
   close the serial port. */

void FB61_Terminate(void)
{
	/* Request termination of thread */
	RequestTerminate = true;

#ifndef WIN32
	/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

	/* Close serial port. */
	device_close();

#else
	CloseConnection();
#endif
}

#define ByteMask ((1 << Bits) - 1)

int UnpackEightBitsToSeven(int offset, int in_length, int out_length,
                           unsigned char *input, unsigned char *output)
{
        unsigned char *OUT = output; /* Current pointer to the output buffer */
        unsigned char *IN  = input;  /* Current pointer to the input buffer */
        unsigned char Rest = 0x00;
        int Bits;

        Bits = offset ? offset : 7;

        while ((IN - input) < in_length) {

                *OUT = ((*IN & ByteMask) << (7 - Bits)) | Rest;
                Rest = *IN >> Bits;

                /* If we don't start from 0th bit, we shouldn't go to the
                   next char. Under *OUT we have now 0 and under Rest -
                   _first_ part of the char. */
                if ((IN != input) || (Bits == 7)) OUT++;
                IN++;

                if ((OUT - output) >= out_length) break;

                /* After reading 7 octets we have read 7 full characters but
                   we have 7 bits as well. This is the next character */
                if (Bits == 1) {
                        *OUT = Rest;
                        OUT++;
                        Bits = 7;
                        Rest = 0x00;
                } else {
                        Bits--;
                }
        }

        return OUT - output;
}

unsigned char GetAlphabetValue(unsigned char value)
{
        unsigned char i;
  
        if (value == '?') return 0x3f;
  
        for (i = 0 ; i < 128 ; i++)
                if (GSM_Default_Alphabet[i] == value)
                        return i;

        return 0x3f; /* '?' */
}

int PackSevenBitsToEight(int offset, unsigned char *input, unsigned char *output)
{
        unsigned char *OUT = output; /* Current pointer to the output buffer */
        unsigned char *IN  = input;  /* Current pointer to the input buffer */
        int Bits;                    /* Number of bits directly copied to
                                        the output buffer */
        Bits = (7 + offset) % 8;

        /* If we don't begin with 0th bit, we will write only a part of the
           first octet */
        if (offset) {
                *OUT = 0x00;
                OUT++;
        }

        while ((IN - input) < strlen(input)) {
                unsigned char Byte = GetAlphabetValue(*IN);

                *OUT = Byte >> (7 - Bits);
                /* If we don't write at 0th bit of the octet, we should write
                   a second part of the previous octet */
                if (Bits != 7)
                        *(OUT-1) |= (Byte & ((1 << (7-Bits)) - 1)) << (Bits+1);

                Bits--;

                if (Bits == -1) Bits = 7;
                else OUT++;

                IN++;
        }
        return (OUT - output);
}

GSM_Error FB61_GetRFLevel(GSM_RFUnits *units, float *level)
{
	/* FIXME - these values are from 3810 code, may be incorrect.  Map from
	   values returned in status packet to the the values returned by the AT+CSQ
	   command. */
	float	csq_map[5] = {0, 8, 16, 24, 31};
	int timeout = 10;
	int rf_level;

	CurrentRFLevel = -1;

	FB61_TX_SendStatusRequest();

	/* Wait for timeout or other error. */

	while (timeout && (CurrentRFLevel == -1)) {
		if (!--timeout)
			return (GE_TIMEOUT);
		usleep (100000);
	}

	/* Make copy in case it changes. */
	rf_level = CurrentRFLevel;

	if (rf_level == -1)
		return (GE_NOLINK);

  /* Now convert between the different units we support. */

  /* Arbitrary units. */
	if (*units == GRF_Arbitrary) {
		*level = rf_level;
		return (GE_NONE);
	}

	/* CSQ units. */
	if (*units == GRF_CSQ) {

		if (rf_level <= 4)
			*level = csq_map[rf_level];
		else
			*level = 99; /* Unknown/undefined */

		return (GE_NONE);
	}

	/* Unit type is one we don't handle so return error */
	return (GE_INTERNALERROR);
}

GSM_Error FB61_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	int timeout = 10;
	int batt_level;

	CurrentBatteryLevel = -1;

	FB61_TX_SendStatusRequest();

	/* Wait for timeout or other error. */
	while (timeout && (CurrentBatteryLevel == -1)) {
		if (!--timeout)
			return (GE_TIMEOUT);
		usleep (100000);
	}

	/* Take copy in case it changes. */
	batt_level = CurrentBatteryLevel;

	if (batt_level != -1) {
		/* Only units we handle at present are GBU_Arbitrary */
		if (*units == GBU_Arbitrary) {
			*level = batt_level;
			return (GE_NONE);
		}
		return (GE_INTERNALERROR);
	} else return (GE_NOLINK);
}

GSM_Error FB61_GetPowerSource(GSM_PowerSource *source)
{
	int timeout = 10;
	CurrentPowerSource=-1;

	FB61_TX_SendStatusRequest();

	/* Wait for timeout or other error. */
	while (timeout && (CurrentPowerSource == -1)) {
		if (!--timeout)
			return (GE_TIMEOUT);
		usleep (100000);
	}

	if (CurrentPowerSource != -1) {
		*source = CurrentPowerSource;
		return (GE_NONE);
	} else return (GE_NOLINK);
}

GSM_Error FB61_GetDisplayStatus(int *Status)
{
	unsigned char req[4]={ FB61_FRAME_HEADER, 0x51 };
	FB61_TX_SendMessage(4, 0x0d, req);
	WAIT_ON(&DisplayStatusError, 10);
	*Status = DisplayStatus;
	return (GE_NONE);
}

GSM_Error FB61_DialVoice(char *Number)
{
	unsigned char req[64] = {FB61_FRAME_HEADER, 0x01};
	unsigned char req_end[] = {0x05, 0x01, 0x01, 0x05, 0x81, 0x01, 0x00, 0x00, 0x01};
	int i = 0;

	req[4] = strlen(Number);
	for(i = 0; i < strlen(Number) ; i++)
		req[5+i] = Number[i];
	memcpy(req + 5 + strlen(Number), req_end, 10);
	FB61_TX_SendMessage(13 + strlen(Number), 0x01, req);
	return(GE_NONE);
}

/* Dial a data call - type specifies request to use: 
     type 0 should normally be used
     type 1 should be used when calling a digital line - corresponds to ats35=0
     Maybe one day we'll know what they mean!
*/

GSM_Error FB61_DialData(char *Number, char type, void (* callpassup)(char c))
{
	unsigned char req[100]   = { FB61_FRAME_HEADER, 0x01 };
	unsigned char *req_end;
	unsigned char req_end0[] = { 0x01,  /* make a data call = type 0x01 */
				     0x02,0x01,0x05,0x81,0x01,0x00,0x00,0x01,0x02,0x0a,
				     0x07,0xa2,0x88,0x81,0x21,0x15,0x63,0xa8,0x00,0x00 };
	unsigned char req_end1[] = { 0x01,
				     0x02,0x01,0x05,0x81,0x01,0x00,0x00,0x01,0x02,0x0a,
				     0x07,0xa1,0x88,0x89,0x21,0x15,0x63,0xa0,0x00,0x06,
				     0x88,0x90,0x21,0x48,0x40,0xbb };
	unsigned char req2[]     = { FB61_FRAME_HEADER, 0x42,0x05,0x01,
				     0x07,0xa2,0xc8,0x81,0x21,0x15,0x63,0xa8,0x00,0x00,
				     0x07,0xa3,0xb8,0x81,0x20,0x15,0x63,0x80,0x01,0x60 };
	unsigned char req3[]     = { FB61_FRAME_HEADER, 0x42,0x05,0x01,
				     0x07,0xa2,0x88,0x81,0x21,0x15,0x63,0xa8,0x00,0x00,
				     0x07,0xa3,0xb8,0x81,0x20,0x15,0x63,0x80 };
	unsigned char req4[]     = { FB61_FRAME_HEADER, 0x42,0x05,0x81,
				     0x07,0xa1,0x88,0x89,0x21,0x15,0x63,0xa0,0x00,0x06,
				     0x88,0x90,0x21,0x48,0x40,0xbb,0x07,0xa3,0xb8,0x81,
				     0x20,0x15,0x63,0x80 };

	int i = 0;
	u8 size;

	CallPassup=callpassup;

	switch (type) {
	case 0:
		req_end = req_end0;
		size = sizeof(req_end0);
		break;
	case 1:
		FB61_TX_SendMessage(sizeof(req3), 0x01, req3);
		FB61_TX_SendMessage(sizeof(req4), 0x01, req4);
		req_end = req_end1;
		size = sizeof(req_end1);
		break;
	default:
		req_end = req_end0;
		size = sizeof(req_end0);
		break;
	}

	req[4] = strlen(Number);

	for(i = 0; i < strlen(Number) ; i++)
		req[5+i] = Number[i];

	memcpy(req + 5 + strlen(Number), req_end, size);

	FB61_TX_SendMessage(5 + size + strlen(Number), 0x01, req);
	if (type != 1) FB61_TX_SendMessage(26, 0x01, req2);

	return (GE_NONE);
}

GSM_Error FB61_GetIncomingCallNr(char *Number)
{
	if (*CurrentIncomingCall != ' ') {
		strcpy(Number, CurrentIncomingCall);
		return GE_NONE;
	} else return GE_BUSY;
}

GSM_Error FB61_CancelCall(void)
{
	unsigned char req[] = { FB61_FRAME_HEADER, 0x08, 0x00, 0x85};
  
	req[4] = CallSequenceNumber;
	FB61_TX_SendMessage(6, 0x01, req);
  
	return GE_NONE;
}  
  
GSM_Error FB61_EnterSecurityCode(GSM_SecurityCode SecurityCode)
{
	unsigned char req[15] = { FB61_FRAME_HEADER, 0x0a /* Enter code request. */, 0x00 /* Type of the entered code. */ };
	int i = 0;
	req[4] = SecurityCode.Type;
	for (i = 0; i < strlen(SecurityCode.Code); i++)
		req[5+i] = SecurityCode.Code[i];
	req[5+strlen(SecurityCode.Code)] = 0x00;
	req[6+strlen(SecurityCode.Code)] = 0x00;
	FB61_TX_SendMessage(7+strlen(SecurityCode.Code), 0x08, req);
	return wait_on(&CurrentSecurityCodeError, 20);
}

GSM_Error FB61_GetSecurityCodeStatus(int *Status)
{
	unsigned char req[4] = { FB61_FRAME_HEADER, 0x07 };
	CurrentSecurityCodeStatus=Status;
	FB61_TX_SendMessage(4, 0x08, req);
	return wait_on(&CurrentSecurityCodeError, 20);
}

GSM_Error FB61_GetDateTime(GSM_DateTime *date_time)
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x62};
	CurrentDateTime = date_time;
	FB61_TX_SendMessage(4, 0x11, req);
	return wait_on(&CurrentDateTimeError, 5);
}

GSM_Error FB61_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x6d};
	CurrentAlarm = date_time;
	FB61_TX_SendMessage(4, 0x11, req);
	return wait_on(&CurrentAlarmError, 5);
}

/* This function sends to the mobile phone a request for the SMS Center */

GSM_Error FB61_GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	unsigned char req[10] = { FB61_FRAME_HEADER, 0x33, 0x64, 0x00 /* SMS Center Number. */ };
	req[5] = MessageCenter->No;
	CurrentMessageCenter = MessageCenter;
	FB61_TX_SendMessage(6, 0x02, req);
	return wait_on(&CurrentMessageCenterError, 10);
}

/* This function set the SMS Center profile on the phone. */

GSM_Error FB61_SetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	unsigned char req[64] = { FB61_FRAME_HEADER, 0x30, 0x64,
				  0x00, /* SMS Center Number. */
				  0x00, /* Unknown. */
				  0x00, /* SMS Message Format. */
				  0x00, /* Unknown. */
				  0x00, /* Validity. */
				  0,0,0,0,0,0,0,0,0,0,0,0, /* Unknown. */
				  0,0,0,0,0,0,0,0,0,0,0,0 /* Message Center Number. */
				  /* Message Center Name. */
	};
	req[5] = MessageCenter->No;
	req[7] = MessageCenter->Format;
	req[9] = MessageCenter->Validity;

	req[22] = SemiOctetPack(MessageCenter->Number, req+23);
	if (req[22] % 2) req[22]++;
	req[22] = req[22] / 2 + 1;

	sprintf(req+34, "%s", MessageCenter->Name);

	CurrentMessageCenter = MessageCenter;
	CurrentMessageCenterError = GE_BUSY;

	FB61_TX_SendMessage(35+strlen(MessageCenter->Name), 0x02, req);
	return wait_on(&CurrentMessageCenterError,20);
}

GSM_Error FB61_GetSMSStatus(GSM_SMSStatus *Status)
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x36, 0x64};
	CurrentSMSStatus = Status;
	FB61_TX_SendMessage(5, 0x14, req);
	return wait_on(&CurrentSMSStatusError,20);
}

GSM_Error FB61_GetIMEI(char *imei)
{
	if (*IMEI) {
		strncpy (imei, IMEI, FB61_MAX_IMEI_LENGTH);
		return (GE_NONE);
	} else return (GE_TRYAGAIN);
}

GSM_Error FB61_GetRevision(char *revision)
{
	if (*Revision) {
		strncpy (revision, Revision, FB61_MAX_REVISION_LENGTH);
		return (GE_NONE);
	} else return (GE_TRYAGAIN);
}

GSM_Error FB61_GetModel(char *model)
{
	if (*Model) {
		strncpy (model, Model, FB61_MAX_MODEL_LENGTH);
		return (GE_NONE);
	} else return (GE_TRYAGAIN);
}

GSM_Error FB61_SetDateTime(GSM_DateTime *date_time)
{
	unsigned char req[] = { FB61_FRAME_HEADER,
				0x60, /* set-time subtype */
				0x01, 0x01, 0x07, /* unknown */
				0x00, 0x00, /* Year (0x07cf = 1999) */
				0x00, 0x00, /* Month Day */
				0x00, 0x00, /* Hours Minutes */
				0x00 /* Unknown, but not seconds - try 59 and wait 1 sec. */
	};
	req[7] = date_time->Year / 256;
	req[8] = date_time->Year % 256;
	req[9] = date_time->Month;
	req[10] = date_time->Day;
	req[11] = date_time->Hour;
	req[12] = date_time->Minute;

	FB61_TX_SendMessage(14, 0x11, req);
	return wait_on(&CurrentSetDateTimeError, 20);
}

/* FIXME: we should also allow to set the alarm off :-) */

GSM_Error FB61_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	unsigned char req[] = { FB61_FRAME_HEADER,
				0x6b, /* set-alarm subtype */
				0x01, 0x20, 0x03, /* unknown */
				0x02,       /* should be alarm on/off, but it don't works */
				0x00, 0x00, /* Hours Minutes */
				0x00 /* Unknown, but not seconds - try 59 and wait 1 sec. */
	};
	req[8] = date_time->Hour;
	req[9] = date_time->Minute;

	FB61_TX_SendMessage(11, 0x11, req);
	return wait_on(&CurrentSetAlarmError, 20);
}

/* Routine to get specifed phone book location.  Designed to be called by
   application.  Will block until location is retrieved or a timeout/error
   occurs. */

GSM_Error FB61_GetMemoryLocation(GSM_PhonebookEntry *entry)
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x01, 0x00, 0x00, 0x00};

	CurrentPhonebookEntry = entry;
	req[4] = FB61_GetMemoryType(entry->MemoryType);
	req[5] = entry->Location;
	FB61_TX_SendMessage(7, 0x03, req);
	return wait_on(&CurrentPhonebookError, 20);
}

/* Routine to write phonebook location in phone. Designed to be called by
   application code. Will block until location is written or timeout
   occurs. */

GSM_Error FB61_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	unsigned char req[128] = { FB61_FRAME_HEADER, 0x04, 0x00, 0x00 };
	int i = 0, current = 0;

	req[4] = FB61_GetMemoryType(entry->MemoryType);
	req[5] = entry->Location;

	req[6] = strlen(entry->Name);

	current = 7;

	for (i = 0; i<strlen(entry->Name); i++)
		req[current+i] = entry->Name[i];

	current += strlen(entry->Name);

	req[current++] = strlen(entry->Number);

	for (i = 0; i < strlen(entry->Number); i++)
		req[current+i] = entry->Number[i];

	current += strlen(entry->Number);

	/* Jano: This allow to save 14 characters name into SIM memory, when
	   No Group is selected. */

	if (entry->Group == 5)
		req[current++] = 0xff;
	else
		req[current++] = entry->Group;

	FB61_TX_SendMessage(current, 3, req);
	return wait_on(&CurrentPhonebookError, 50);
}

GSM_Error FB61_NetMonitor(unsigned char mode, char *Screen)
{
	unsigned char req1[] = { 0x00, 0x01, 0x64, 0x01 };
	unsigned char req2[] = { 0x00, 0x01, 0x7e, 0x00 };

	CurrentNetmonitor = Screen;
	FB61_TX_SendMessage(4, 0x40, req1);
	req2[3] = mode;
	FB61_TX_SendMessage(4, 0x40, req2);
	return wait_on(&CurrentNetmonitorError, 20);
}

GSM_Error FB61_SendDTMF(char *String)
{
	unsigned char req[64] = { FB61_FRAME_HEADER, 0x50, 0x00 /* Length of DTMF string. */ };
	u8 length=strlen(String);

	req[4] = length;
	sprintf(req+5, "%s", String);
	FB61_TX_SendMessage(5+length, 0x01, req);
	return (GE_NONE);
}

GSM_Error FB61_GetSpeedDial(GSM_SpeedDial *entry)
{
	unsigned char req[10] = { FB61_FRAME_HEADER, 0x16, 0x00  /* The number of speed dial. */ };
	CurrentSpeedDialEntry = entry;
	req[4] = entry->Number;
	FB61_TX_SendMessage(5, 0x03, req);
	return wait_on(&CurrentSpeedDialError, 20);
}

GSM_Error FB61_SetSpeedDial(GSM_SpeedDial *entry)
{
	unsigned char req[7] = { FB61_FRAME_HEADER, 0x19, 0x00 /* Number */, 0x00 /* Memory Type */, 0x00  /* Location */ };
	req[4] = entry->Number;
	req[5] = entry->MemoryType;
	req[6] = entry->Location;
	FB61_TX_SendMessage(7, 0x03, req);
	return wait_on(&CurrentSpeedDialError, 20);
}

GSM_Error FB61_GetSMSMessage(GSM_SMSMessage *message)
{
	unsigned char req[10] = { FB61_FRAME_HEADER, 0x07, 0x02 /* Unknown */, 0x00 /* Location */, 0x01, 0x64};
	int timeout = 60;

	/* State machine code writes data to these variables when it comes in. */

	CurrentSMSMessage = message;
	CurrentSMSMessageError = GE_BUSY;
	req[5] = message->Location;
	FB61_TX_SendMessage(8, 0x02, req);

	while (timeout && (CurrentSMSMessageError == GE_BUSY || CurrentSMSMessageError == GE_SMSWAITING)) {
		if (!--timeout)
			return (GE_TIMEOUT);
		usleep (100000);
	}
	return (CurrentSMSMessageError);
}

GSM_Error FB61_DeleteSMSMessage(GSM_SMSMessage *message)
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x0a, 0x02, 0x00};
	CurrentSMSMessageError = GE_BUSY;
	req[5] = message->Location;
	FB61_TX_SendMessage(6, 0x14, req);
	return wait_on(&CurrentSMSMessageError, 50);
}

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */

int SemiOctetPack(char *Number, unsigned char *Output)
{
	unsigned char *IN = Number;  /* Pointer to the input number */
	unsigned char *OUT = Output; /* Pointer to the output */
	int count = 0; /* This variable is used to notify us about count of already
			  packed numbers. */

	/* The first byte in the Semi-octet representation of the address field is
	   the Type-of-Address. This field is described in the official GSM
	   specification 03.40 version 5.3.0, section 9.1.2.5, page 33. We support
	   only international and unknown number. */
  
	if (*IN == '+') {
		*OUT++ = GNT_INTERNATIONAL; /* International number */
		IN++;
	} else *OUT++ = GNT_UNKNOWN; /* Unknown number */

	/* The next field is the number. It is in semi-octet representation - see
	   GSM scpecification 03.40 version 5.3.0, section 9.1.2.3, page 31. */

	while (*IN) {
		if (count & 0x01) {
			*OUT = *OUT | ((*IN - '0') << 4);
			OUT++;
		}
		else
			*OUT = *IN - '0';
		count++; IN++;
	}

	/* We should also fill in the most significant bits of the last byte with
	   0x0f (1111 binary) if the number is represented with odd number of
	   digits. */

	if (count & 0x01) {
		*OUT=*OUT | 0xf0;
		OUT++;
		return (2 * (OUT - Output - 1) - 1);
	}

	return (2 * (OUT - Output - 1));
}

/* The second argument is the size of the data in octets,
   excluding User Data Header - important only for 8bit data */
GSM_Error FB61_SendSMSMessage(GSM_SMSMessage *SMS, int data_size)
{
	GSM_Error error;

	unsigned char req[256] = {
		FB61_FRAME_HEADER,
		0x01, 0x02, 0x00, /* SMS send request*/
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* SMS center, the rest is unused */
		0x11, /*  0 - TP-Reply-Path (9.2.3.17)
			  0 - TP-TP-User-Data-Header-Indicator (9.2.3.23)
			  x - TP-Status-Report-Request (9.2.3.5)
			  0 - no delivery report (default for gnokii)
			  1 - request for delivry report
			  xx - TP validity period (9.2.3.3, see also 9.2.3.12)
			  00 - not present
			  10 - relative format (default for gnokii)
			  01 - enhanced format
			  11 - absolute format
			  no support for this field yet
			  0 - TP-Reject-Duplicates (9.2.3.25)
			  01 - TP-Message-Type-Indicator (9.2.3.1) - SMS_SUBMIT */
		0x00, /* TP-Message-Reference (9.2.3.6) */
		0x00, /* TP-Protocol-Identifier (9.2.3.9) */
		0x00, /* TP-Data-Coding-Scheme (9.2.3.10, GSM 03.38) */
		0x00, /* TP-User-Data-Length (9.2.3.16) */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* destination */
		0xa9, /* SMS validity: b0=1h 47=6h a7=24h a9=72h ad=1week */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	int size, offset;

	/* First of all we should get SMSC number */
	if (SMS->MessageCenter.No) {
		error = FB61_GetSMSCenter(&SMS->MessageCenter);
		if (error != GE_NONE)
			return error;
		SMS->MessageCenter.No = 0;
	}
	dprintf(_("Sending SMS to %s via message center %s\n"), SMS->Destination, SMS->MessageCenter.Number);

	if (SMS->UDHType) {
		/* offset - length of the user data header */
		offset = 1 + SMS->UDH[0];
		/* we copy the udh and set the mask for the indicator */
		memcpy(req + 42, SMS->UDH, offset);
		req[18] |= 0x40;
	} else {
		offset = 0;
		/* such messages should be sent as concatenated */
		if (strlen(SMS->MessageText) > GSM_MAX_SMS_LENGTH)
			return(GE_SMSTOOLONG);
	}

	/* size is the length of the data in octets including udh */
	/* SMS->Length is:
	   - integer representation of the number od octets within the user data when UD is coded using 8bit data
	   - the sum of the number of septets in UDH including any padding and number of septets in UD in other case
	*/
  
	/* offset now denotes UDH length */
	if (SMS->EightBit) {
		memcpy(req + 42 + offset, SMS->MessageText, data_size);
		SMS->Length = size = data_size + offset;
		/* the mask for the 8-bit data */
		req[21] |= 0xf4;
	} else {
		size = PackSevenBitsToEight((7-offset)%7, SMS->MessageText, req + 42 + offset);
		size += offset;
		SMS->Length = (offset*8 + ((7-offset)%7)) / 7 + strlen(SMS->MessageText);
	}

	req[22] = SMS->Length;

	CurrentSMSMessageError=GE_BUSY;

	req[6] = SemiOctetPack(SMS->MessageCenter.Number, req+7);
	if (req[6] % 2) req[6]++;

	req[6] = req[6] / 2 + 1;

	/* Mask for request for delivery report from SMSC */
	if (SMS->Type == GST_DR) req[18] |= 0x20;
 
	/* Message Class */
	switch (SMS->Class) {
	case 0: req[21] |= 0xf0; break;
	case 1: req[21] |= 0xf1; break;
	case 2: req[21] |= 0xf2; break;
	case 3:	req[21] |= 0xf3; break;
	default: break;
	}
  
	/* Mask for compression */
	/* FIXME: support for compression */
	/* See GSM 03.42 */
/*  if (SMS->Compression) req[21] = req[21] | 0x20; */

	req[23] = SemiOctetPack(SMS->Destination, req+24);

	/* TP-Validity Period handling */

	/* FIXME: error-checking for correct Validity - it should not be bigger then
	   63 weeks and smaller then 5minutes. We should also test intervals because
	   the SMS->Validity to TP-VP is not continuos. I think that the simplest
	   solution will be an array of correct values. We should parse it and if we
	   find the closest TP-VP value we should use it. Or is it good to take
	   closest smaller TP-VP as we do now? I think it is :-) */

	/* 5 minutes intervals up to 12 hours = 720 minutes */
	if (SMS->Validity <= 720)
		req[35] = (unsigned char) (SMS->Validity/5)-1;

	/* 30 minutes intervals up to 1 day */
	else if ((SMS->Validity > 720) && (SMS->Validity <= 1440))
		req[35] = (unsigned char) ((SMS->Validity-720)/30)+143;

	/* 1 day intervals up to 30 days */
	else if ((SMS->Validity > 1440) && (SMS->Validity <= 43200))
		req[35] = (unsigned char) (SMS->Validity/1440)+166;

	/* 1 week intervals up to 63 weeks */
	else if ((SMS->Validity > 43200) && (SMS->Validity <= 635040))
		req[35] = (unsigned char) (SMS->Validity/10080)+192;

	FB61_TX_SendMessage(42+size, 0x02, req);
	return wait_on(&CurrentSMSMessageError, 70);
}

GSM_Error FB61_SaveSMSMessage(GSM_SMSMessage *SMS)
{
	unsigned char req[256] = {
		FB61_FRAME_HEADER,
		0x04, 0x05, 0x02, 0x00, 0x02, /* SMS save request*/
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* SMS center, the rest is unused */
		0x11, /*  0 - TP-Reply-Path (9.2.3.17)
			  0 - TP-TP-User-Data-Header-Indicator (9.2.3.23)
			  x - TP-Status-Report-Request (9.2.3.5)
			  0 - no delivery report (default for gnokii)
			  1 - request for delivry report
			  xx - TP validity period (9.2.3.3, see also 9.2.3.12)
			  00 - not present
			  10 - relative format (default for gnokii)
			  01 - enhanced format
			  11 - absolute format
			  no support for this field yet
			  0 - TP-Reject-Duplicates (9.2.3.25)
			  01 - TP-Message-Type-Indicator (9.2.3.1) - SMS_SUBMIT */
		0x00, /* TP-Message-Reference (9.2.3.6) */
		0x00, /* TP-Protocol-Identifier (9.2.3.9) */
		0x00, /* TP-Data-Coding-Scheme (9.2.3.10, GSM 03.38) */
		0x00, /* TP-User-Data-Length (9.2.3.16) */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* destination */
		0xa9, /* SMS validity: b0=1h 47=6h a7=24h a9=72h ad=1week */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	int size, offset;

	if (SMS->Status == GSS_NOTSENTREAD)
		req[4] |= 0x02;

	if (SMS->Location)
		req[6] = SMS->Location;

	offset = 0;
	/* such messages should be sent as concatenated */
	if (strlen(SMS->MessageText) > GSM_MAX_SMS_LENGTH)
		return(GE_SMSTOOLONG);

	/* size is the length of the data in octets including udh */
	/* SMS->Length is:
	   - integer representation of the number od octets within the user data when UD is coded using 8bit data
	   - the sum of the number of septets in UDH including any padding and number of septets in UD in other case
	*/
  
	/* offset now denotes UDH length */
	size = PackSevenBitsToEight((7-offset)%7, SMS->MessageText, req + 44 + offset);
	size += offset;
	SMS->Length = (offset*8 + ((7-offset)%7)) / 7 + strlen(SMS->MessageText);

	req[24] = SMS->Length;
	FB61_TX_SendMessage(44+size, 0x14, req);
	return wait_on(&CurrentSMSMessageError, 70);
}

/* Enable and disable Cell Broadcasting */

GSM_Error FB61_EnableCellBroadcast(void)
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x20, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01};
	CurrentCBError = GE_BUSY;

	CurrentCBMessage = (GSM_CBMessage *)malloc(sizeof (GSM_CBMessage));
	CurrentCBMessage->Channel = 0;
	CurrentCBMessage->New = false;
	strcpy (CurrentCBMessage->Message,"");
	FB61_TX_SendMessage(10, 0x02, req);
	return wait_on(&CurrentCBError, 20);
}

GSM_Error FB61_DisableCellBroadcast(void) /* Should work, but not tested fully */
{
	unsigned char req[] = {FB61_FRAME_HEADER, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /*VERIFY*/
	FB61_TX_SendMessage(10, 0x02, req);
	return wait_on(&CurrentCBError, 20);
}

GSM_Error FB61_ReadCellBroadcast(GSM_CBMessage *Message)
{
	if (CurrentCBMessage != NULL) {
		if (CurrentCBMessage->New == true) {
			Message->Channel = CurrentCBMessage->Channel;
			strcpy(Message->Message,CurrentCBMessage->Message);
			CurrentCBMessage->New = false;
			return (GE_NONE);
		}
	}
	return (GE_NONEWCBRECEIVED);
}

/* Send a bitmap or welcome-note */

GSM_Error FB61_SetBitmap(GSM_Bitmap *Bitmap)
{
	unsigned char req[600] = { FB61_FRAME_HEADER };
	u16 count = 3;
	u8 textlen;

	switch (Bitmap->type) {
	case GSM_WelcomeNoteText:
		req[count++] = 0x18;
		req[count++] = 0x01; /* Only one block */
		req[count++] = 0x02; /* Welcome text */
		req[count++] = textlen = strlen(Bitmap->text);
		memcpy(req+count, Bitmap->text, textlen);
      
		count += textlen;
		FB61_TX_SendMessage(count, 0x05, req);
		break;

	case GSM_DealerNoteText:
		req[count++] = 0x18;
		req[count++] = 0x01; /* Only one block */
		req[count++] = 0x03; /* Dealer Welcome Note */
		req[count++] = textlen = strlen(Bitmap->text);
		memcpy(req+count, Bitmap->text, textlen);
      
		count += textlen;
		FB61_TX_SendMessage(count, 0x05, req);
		break;

	case GSM_StartupLogo:
		req[count++] = 0x18;
		req[count++] = 0x01; /* Only one block */
		req[count++] = 0x01;
		req[count++] = Bitmap->height;
		req[count++] = Bitmap->width;
		memcpy(req+count, Bitmap->bitmap, Bitmap->size);
		count += Bitmap->size;
		FB61_TX_SendMessage(count, 0x05, req);
		break;

	case GSM_OperatorLogo:
		req[count++] = 0x30;  /* Store Op Logo */
		req[count++] = 0x01;  /* Location */
		req[count++] = ((Bitmap->netcode[1] & 0x0f) << 4) | (Bitmap->netcode[0] & 0x0f);
		req[count++] = 0xf0 | (Bitmap->netcode[2] & 0x0f);
		req[count++] = ((Bitmap->netcode[5] & 0x0f) << 4) | (Bitmap->netcode[4] & 0x0f);
		req[count++] = (Bitmap->size+4) >> 8;
		req[count++] = (Bitmap->size+4)%0xff;
		req[count++] = 0x00;  /* Infofield */
		req[count++] = Bitmap->width;
		req[count++] = Bitmap->height;
		req[count++] = 0x01;  /* Just BW */    
		memcpy(req+count, Bitmap->bitmap, Bitmap->size);
		FB61_TX_SendMessage(count+Bitmap->size, 0x05, req);
		break;

	case GSM_CallerLogo:
		req[count++] = 0x13;
		req[count++] = Bitmap->number;
		req[count++] = strlen(Bitmap->text);
		memcpy(req+count, Bitmap->text, req[count-1]);
		count += req[count-1];
		req[count++]= Bitmap->ringtone;
		req[count++]= 0x01;  /* Graphic on. You can use other values as well:
				        0x00 - Off
				        0x01 - On
				        0x02 - View Graphics
				        0x03 - Send Graphics
				        0x04 - Send via IR
				        You can even set it higher but Nokia phones (my
				        6110 at least) will not show you the name of this
					item in menu ;-)) Nokia is really joking here. */
		req[count++] = (Bitmap->size + 4) >> 8;
		req[count++] = (Bitmap->size + 4) % 0xff;
		req[count++] = 0x00;  /* Future extensions! */
		req[count++] = Bitmap->width;
		req[count++] = Bitmap->height;
		req[count++] = 0x01;  /* Just BW */
		memcpy(req+count, Bitmap->bitmap, Bitmap->size);
		FB61_TX_SendMessage(count+Bitmap->size, 0x03, req);
		break;

	case GSM_PictureImage:
		return (GE_NOTIMPLEMENTED);
		break;

	case GSM_None:
		break;
	}
	return wait_on(&SetBitmapError, 70);
}

/* Get a bitmap from the phone */

GSM_Error FB61_GetBitmap(GSM_Bitmap *Bitmap)
{
	unsigned char req[10] = { FB61_FRAME_HEADER };
	u8 count = 3;
	int timeout = 10;

	GetBitmap = Bitmap;
	GetBitmapError = GE_BUSY;
 
	/* This is needed to avoid the packet being interrupted */
	/* Remove when multipacket code is implemented fully */

	DisableKeepalive = true;

	while (timeout && (MessagesSent != AcksReceived)) {
		usleep(100000);
		timeout--;
	}

	/* We'll assume that nothing more will be received after 1 sec */

	MessagesSent = AcksReceived;

	switch (GetBitmap->type) {
	case GSM_StartupLogo:
		req[count++] = 0x16;
		FB61_TX_SendMessage(count, 0x05, req);
		break;
	case GSM_WelcomeNoteText:
		req[count++] = 0x16;
		FB61_TX_SendMessage(count, 0x05, req);
		break;
	case GSM_DealerNoteText:
		req[count++] = 0x16;
		FB61_TX_SendMessage(count, 0x05, req);
		break;  
	case GSM_OperatorLogo:
		req[count++] = 0x33;
		req[count++] = 0x01; /* Location 1 */
		FB61_TX_SendMessage(count, 0x05, req);
		break;
	case GSM_CallerLogo:
		req[count++] = 0x10;
		req[count++] = Bitmap->number;
		FB61_TX_SendMessage(count, 0x03, req);
		break;
	default:
		break;
	}

	/* 5secs for the command to complete */
  
	WAIT_ON(&GetBitmapError, 50);

	DisableKeepalive = false;
	GetBitmap = NULL;
	return (GetBitmapError);
}


GSM_Error FB61_SetRingTone(GSM_Ringtone *ringtone)
{
	char package[GSM_MAX_RINGTONE_PACKAGE_LENGTH+10] =
	{ 0x0c, 0x01, /* FBUS RingTone header */
	  /* Next bytes are from Smart Messaging Specification version 2.0.0 */
	  0x06,       /* User Data Header Length */
	  0x05,       /* IEI FIXME: What is this? */
	  0x04,       /* IEDL FIXME: What is this? */
	  0x15, 0x81, /* Destination port */
	  0x15, 0x81  /* Originator port, only
			 to fill in the two
			 bytes :-) */
	};
	int size = GSM_MAX_RINGTONE_PACKAGE_LENGTH;
	GSM_PackRingtone(ringtone, package+9, &size);
	package[size+9] = 0x01;
	FB61_TX_SendMessage((size+10), 0x12, package);
	return (GE_NONE);
}

GSM_Error FB61_SendRingTone(GSM_Ringtone *ringtone, char *dest)
{
	GSM_SMSMessage SMS;
	GSM_Error error;

	int size = GSM_MAX_RINGTONE_PACKAGE_LENGTH;
	char Package[GSM_MAX_RINGTONE_PACKAGE_LENGTH];
	char udh[] = {
		0x06,       /* User Data Header Length */
		0x05,       /* IEI */
		0x04,       /* IEDL */
		0x15, 0x81, /* Destination port */
		0x15, 0x81  /* Originator port, only
			       to fill in the two
			       bytes :-) */
	};

	/* Default settings for SMS message:
	   - no delivery report
	   - Class Message 1
	   - no compression
	   - 8 bit data
	   - SMSC no. 1
	   - validity 3 days
	   - set UserDataHeaderIndicator
	*/

	SMS.Type = GST_MO;
	SMS.Class = 1;
	SMS.Compression = false;
	SMS.EightBit = true;
	SMS.MessageCenter.No = 1;
	SMS.Validity = 4320; /* 4320 minutes == 72 hours */

	SMS.UDHType = GSM_RingtoneUDH;

	strcpy(SMS.Destination, dest);
	GSM_PackRingtone(ringtone, Package, &size);
	memcpy(SMS.UDH, udh, 7);
	memcpy(SMS.MessageText, Package, size);

	/* Send the message. */
	error = FB61_SendSMSMessage(&SMS, size);

	if (error == GE_SMSSENDOK) dprintf(_("Send succeeded!\n"));
	else dprintf(_("SMS Send failed (error=%d)\n"), error);

	return (GE_NONE);
}

GSM_Error FB61_Reset(unsigned char type)
{
	unsigned char req[4] = { 0x00,0x01,0x64,0x03 };
	req[3] = type;
	FB61_TX_SendMessage(4, 0x40, req);
	return (GE_NONE);
}

#ifndef WIN32

void FB61_DumpSerial(void)
{
	int PortFD;
	unsigned int Flags=0;

	PortFD = device_getfd();
	ioctl(PortFD, TIOCMGET, &Flags);

	dprintf(_("Serial flags dump:\n"));
	dprintf(_("DTR is %s.\n"), Flags&TIOCM_DTR?_("up"):_("down"));
	dprintf(_("RTS is %s.\n"), Flags&TIOCM_RTS?_("up"):_("down"));
	dprintf(_("CAR is %s.\n"), Flags&TIOCM_CAR?_("up"):_("down"));
	dprintf(_("CTS is %s.\n"), Flags&TIOCM_CTS?_("up"):_("down"));
	dprintf("\n");
}

/* This functions set up the Nokia DAU-9P MBus Cable NSE-3 which is probably
   dual MBUS/FBUS to FBUS communication. If we skip this step we can
   communicate with the phone, but the phone sends us some garbagge (0x18 0x00
   ...).

   This was brought to my attention by people with original cable. My cable is
   FBUS only so I borrow the original cable from my friend and solve
   it. Thanks to Cobus, Juan and people on the mailing list.

   Colin wrote:

   The data suite cable has some electronics built into the connector. This of
   course needs a power supply of some sorts to operate properly.

   In this case power is drawn off the handshaking lines of the PC. DTR has to
   be set and RTS have to be cleared, thus if you use a terminal program (that
   does not set the handshaking lines to these conditions) you will get weird
   results. It will not set them like this since if Request To Send (RTS) is
   not set the other party will not send any data (in hardware handshaking)
   and if DTS is not set (handshaking = none) the cable will not receive
   power. */

/* FIXME: In this function we use ioctls - can people with different OSes than
   my (Linux) tell me if it works for them? */

void FB61_SetFBUS(void)
{
#ifdef DEBUG
	FB61_DumpSerial();  
	dprintf(_("Setting FBUS communication...\n"));
#endif

	/* clearing the RTS bit and setting the DTR bit*/
	device_setdtrrts(1, 0);

#ifdef DEBUG
	FB61_DumpSerial();  
#endif /* DEBUG */
}

/* Called by initialisation code to open comm port in asynchronous mode. */

bool FB61_OpenSerial(void)
{
	int result;
  
#if defined(__svr4__) || defined(__FreeBSD__)
	int rtn;
#else
	struct sigaction sig_io;

	/* Set up and install handler before enabling async IO on port. */

	sig_io.sa_handler = FB61_SigHandler;
	sig_io.sa_flags = 0;
	sigaction (SIGIO, &sig_io, NULL);
#endif

	/* Open device. */

	result = device_open(PortDevice, false, true);

	if (!result) {
		perror(_("Couldn't open FB61 device"));
		return false;
	}

#if defined(__svr4__) || defined(__FreeBSD__)
	/* create a thread to handle incoming data from mobile phone */
	rtn = pthread_create(&selThread, NULL, (void*)FB61_SelectLoop, (void*)NULL);
	if (rtn != 0) return false;
#endif

	device_changespeed(115200);
	FB61_SetFBUS();
	return (true);
}

void FB61_SigHandler(int status)
{
	unsigned char buffer[255];
	int count, res;
	res = device_read(buffer, 255);
	for (count = 0; count < res ; count ++)
		FB61_RX_StateMachine(buffer[count]);
}
#endif /* WIN32 */

char *FB61_GetBCDNumber(u8 *Number) 
{
	static char Buffer[20] = "";

	/* This is the length of BCD coded number */
	int length = Number[0];
	int count;

	if (Number[1] == 0x91) sprintf(Buffer, "+");
	else Buffer[0] = '\0';

	for (count = 0; count < length-1; count++) {
		int Digit;
		Digit = Number[count+2] & 0x0f;
		if (Digit < 10) sprintf(Buffer, "%s%d", Buffer, Digit);
		Digit = Number[count+2] >> 4;
		if (Digit < 10) sprintf(Buffer, "%s%d", Buffer, Digit);
	}

	return Buffer;
}

char *FB61_GetPackedDateTime(u8 *Number)
{
	static char Buffer[20] = "";

	sprintf(Buffer, "%d%d-", Number[0] & 0xf, Number[0] >> 4);
	sprintf(Buffer, "%s%d%d-", Buffer, Number[1] & 0xf, Number[1] >> 4);
	sprintf(Buffer, "%s%d%d ", Buffer, Number[2] & 0xf, Number[2] >> 4);
	sprintf(Buffer, "%s%d%d:", Buffer, Number[3] & 0xf, Number[3] >> 4);
	sprintf(Buffer, "%s%d%d:", Buffer, Number[4] & 0xf, Number[4] >> 4);
	sprintf(Buffer, "%s%d%d",  Buffer, Number[5] & 0xf, Number[5] >> 4);
  
	return Buffer;
}

enum FB61_RX_States FB61_RX_DispatchMessage(void)
{
	int i, tmp, count, offset, off, length;
	unsigned char output[160];
	bool supported;

	if (RX_Multiple) return FB61_RX_Sync;

#ifdef DEBUG
	/* Do not debug Ack and RLP frames to detail. */

	if (MessageType != FB61_FRTYPE_ACK && MessageType != 0xf1)
		FB61_RX_DisplayMessage();
#endif /* DEBUG */

	/* Switch on the basis of the message type byte */
	switch (MessageType) {
	case 0x01:    /* Call information */
		switch (MessageBuffer[3]) {
			/* Unknown message - it has been seen after the 0x07 message (call
			   answered). Probably it has similar meaning. If you can solve
			   this - just mail me. Pavel Janík ml.

			   The message looks like this:

			   Msg Destination: PC
			   Msg Source: Phone
			   Msg Type: 01
			   Msg Unknown: 00
			   Msg Len: 0e

			   Phone: [01 ][08 ][00 ] is the header of the frame

			   [03 ] is the call message subtype

			   [05 ] is the call sequence number

			   [05 ] unknown 

			   [00 ][01 ][03 ][02 ][91][00] are unknown but has been
			   seen in the Incoming call message (just after the
			   caller's name from the phonebook). But never change
			   between phone calls :-(
			*/

		case 0x02: /* This may mean sequence number of 'just made' call - CK */
			dprintf(_("Message: Call message, type 0x02:"));
			dprintf(_("   Exact meaning not known yet, sorry :-(\n"));
			break;

		case 0x03: /* Possibly call OK */
/* JD: I think that this means "call in progress" (incomming or outgoing) */
			dprintf(_("Message: Call message, type 0x03:"));
			dprintf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
			dprintf(_("   Exact meaning not known yet, sorry :-(\n"));
			CallSequenceNumber = MessageBuffer[4];
			CurrentIncomingCall[0] = 'D';
			if (CallPassup) CallPassup('D');
			break;

			/* Remote end has gone away before you answer the call.  Probably your
			   mother-in-law or banker (which is worse?) ... */
		case 0x04:
			dprintf(_("Message: Remote end hang up.\n"));
			dprintf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
			CurrentIncomingCall[0] = ' ';
			if (CallPassup) CallPassup(' ');
			break;

		case 0x05: /* Incoming call alert */
			dprintf(_("Message: Incoming call alert:\n"));
			/* We can have more then one call ringing - we can distinguish between
			   them */
			dprintf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
			dprintf(_("   Number: "));
			count = MessageBuffer[6];
			for (tmp = 0; tmp < count; tmp++)
				dprintf("%c", MessageBuffer[7+tmp]);
			dprintf("\n");
			dprintf(_("   Name: "));
			for (tmp = 0; tmp < MessageBuffer[7+count]; tmp++)
				dprintf("%c", MessageBuffer[8+count+tmp]);
			dprintf("\n");
			count = MessageBuffer[6];
			CurrentIncomingCall[0] = 0;
			for (tmp = 0; tmp < count; tmp++)
				sprintf(CurrentIncomingCall, "%s%c", CurrentIncomingCall, MessageBuffer[7+tmp]);
			break;

			/* Call answered. Probably your girlfriend...*/
		case 0x07:
			dprintf(_("Message: Call answered.\n"));
			dprintf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
			break;

			/* Call ended. Girlfriend is girlfriend, but time is money :-) */
		case 0x09:
			dprintf(_("Message: Call ended by your phone.\n"));
			dprintf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
			break;

			/* This message has been seen with the message of subtype 0x09
			   after I hang the call.

			   Msg Destination: PC
			   Msg Source: Phone
			   Msg Type: 01
			   Msg Unknown: 00
			   Msg Len: 08
			   Phone: [01 ][08 ][00 ][0a ][04 ][87 ][01 ][42B][1a ][c2 ]

			   What is the meaning of 87? Can you spell some magic light into
			   this issue?

			*/

		case 0x0a: /* Probably means call over - CK */
			dprintf(_("Message: Call message, type 0x0a:"));
			dprintf(_("   Sequence nr. of the call: %d\n"), MessageBuffer[4]);
			dprintf(_("   Exact meaning not known yet, sorry :-(\n"));

			CurrentIncomingCall[0] = ' ';
			if (CallPassup) CallPassup(' ');
			break;

		default:
			dprintf(_("Message: Unknown message of type 0x01\n"));
			break;
		}
		break;

		/* SMS handling */

	case 0x02:
		switch (MessageBuffer[3]) {
		case 0x02: /* SMS message correctly sent to the network */
			dprintf(_("Message: SMS Message correctly sent.\n"));
			CurrentSMSMessageError = GE_SMSSENDOK;
			break;

		case 0x03: /* SMS message send to the network failed */
			dprintf(_("Message: Sending SMS Message failed.\n"));
			CurrentSMSMessageError = GE_SMSSENDFAILED;
			break;

		case 0x10:
			dprintf(_("Message: SMS Message Received\n"));
			dprintf(_("   SMS center number: %s\n"), FB61_GetBCDNumber(MessageBuffer+7));

			MessageBuffer[23] = (MessageBuffer[23]+1)/2+1;

			dprintf(_("   Remote number: %s\n"), FB61_GetBCDNumber(MessageBuffer+23));
			dprintf(_("   Date: %s\n"), FB61_GetPackedDateTime(MessageBuffer+35));
			dprintf(_("   SMS: "));

			tmp = UnpackEightBitsToSeven(0, MessageLength - 42 - 2, MessageBuffer[22], MessageBuffer + 42, output);

			for (i = 0; i < tmp; i++) dprintf("%c", GSM_Default_Alphabet[output[i]]);

			dprintf("\n");
			break;

		case 0x21:
			dprintf(_("Message: Cell Broadcast enabled/disabled successfully.\n"));
			CurrentCBError = GE_NONE;
			break;

		case 0x23:
			CurrentCBMessage->Channel = MessageBuffer[7];
			CurrentCBMessage->New = true;
			tmp=UnpackEightBitsToSeven(0, 82, 82, MessageBuffer+10, output);

			dprintf(_("Message: CB received.\n")); fflush (stdout);
			dprintf(_("Message: channel number %i\n"),MessageBuffer[7]);
			for (i=0; i<tmp;i++)
				dprintf("%c", GSM_Default_Alphabet[output[i]]);

			dprintf("\n");
			for (i = 0; i < tmp; i++)
				CurrentCBMessage->Message[i] = GSM_Default_Alphabet[output[i]];

			break;

  
		case 0x31:
			dprintf(_("Message: SMS Center correctly set.\n"));
			CurrentMessageCenterError = GE_NONE;
			break;

		case 0x34:

			CurrentMessageCenter->No = MessageBuffer[4];
			CurrentMessageCenter->Format = MessageBuffer[6];
			CurrentMessageCenter->Validity = MessageBuffer[8];
			sprintf(CurrentMessageCenter->Name, "%s", MessageBuffer+33);
			sprintf(CurrentMessageCenter->Number, "%s", FB61_GetBCDNumber(MessageBuffer+21));

			dprintf(_("Message: SMS Center received:\n"));
			dprintf(_("   %d. SMS Center name is %s\n"), CurrentMessageCenter->No, CurrentMessageCenter->Name);
			dprintf(_("   SMS Center number is %s\n"), CurrentMessageCenter->Number);
			dprintf(_("   SMS Center message format is "));

			switch (CurrentMessageCenter->Format) {
			case GSMF_Text:   dprintf(_("Text"));    break;
			case GSMF_Paging: dprintf(_("Paging"));  break;
			case GSMF_Fax:	  dprintf(_("Fax"));     break;
			case GSMF_Email:  dprintf(_("Email"));   break;
			default:          dprintf(_("Unknown")); break;
			}

			dprintf("\n");

			dprintf(_("   SMS Center message validity is "));

			switch (CurrentMessageCenter->Validity) {
			case GSMV_1_Hour:    dprintf(_("1 hour"));       break;
			case GSMV_6_Hours:   dprintf(_("6 hours"));      break;
			case GSMV_24_Hours:  dprintf(_("24 hours"));     break;
			case GSMV_72_Hours:  dprintf(_("72 hours"));     break;
			case GSMV_1_Week:    dprintf(_("1 week"));       break;
			case GSMV_Max_Time:  dprintf(_("Maximum time")); break;
			default:             dprintf(_("Unknown"));      break;
			}
			dprintf("\n");
			CurrentMessageCenterError = GE_NONE;
			break;

		case 0x35:
			/* Nokia 6110 has support for three SMS centers with numbers 1, 2 and
			   3. Each request for SMS Center without one of these numbers
			   fail. */
			dprintf(_("Message: SMS Center error received:\n"));
			dprintf(_("   The request for SMS Center failed.\n"));

			/* FIXME: appropriate error. */
			CurrentMessageCenterError = GE_INTERNALERROR;
			break;  

		default:
			dprintf(_("Unknown message!\n"));
			break;
		}

		break;

		/* Phonebook handling */

	case 0x03:

		switch (MessageBuffer[3]) {

		case 0x02:

			CurrentPhonebookEntry->Empty = true;

			count = MessageBuffer[5];
			dprintf(_("Message: Phonebook entry received:\n"));
			dprintf(_("   Name: "));
			for (tmp = 0; tmp < count; tmp++)
				dprintf("%c", MessageBuffer[6+tmp]);
			dprintf("\n");

			memcpy(CurrentPhonebookEntry->Name, MessageBuffer + 6, count);
			CurrentPhonebookEntry->Name[count] = 0x00;
			CurrentPhonebookEntry->Empty = false;

			i = 7 + count;
			count = MessageBuffer[6+count];

			dprintf(_("   Number: "));
			for (tmp = 0; tmp < count; tmp++)
				dprintf("%c", MessageBuffer[i+tmp]);
			dprintf("\n");

			memcpy(CurrentPhonebookEntry->Number, MessageBuffer + i, count);
			CurrentPhonebookEntry->Number[count] = 0x00;
			CurrentPhonebookEntry->Group = MessageBuffer[i+count];
      
			CurrentPhonebookEntry->Date.Year = MessageBuffer[i+count+2] * 256 + MessageBuffer[i+count+3];
			CurrentPhonebookEntry->Date.Month = MessageBuffer[i+count+4];
			CurrentPhonebookEntry->Date.Day = MessageBuffer[i+count+5];
			CurrentPhonebookEntry->Date.Hour = MessageBuffer[i+count+6];
			CurrentPhonebookEntry->Date.Minute = MessageBuffer[i+count+7];
			CurrentPhonebookEntry->Date.Second = MessageBuffer[i+count+8];

			dprintf(_("   Date: "));
			dprintf("%02u-%02u-%04u\n", CurrentPhonebookEntry->Date.Day, CurrentPhonebookEntry->Date.Month, CurrentPhonebookEntry->Date.Year);
			dprintf(_("   Time: "));
			dprintf("%02u-%02u-%02u\n", CurrentPhonebookEntry->Date.Hour, CurrentPhonebookEntry->Date.Minute, CurrentPhonebookEntry->Date.Second);

			/* Signal no error to calling code. */

			CurrentPhonebookError = GE_NONE;

			break;

		case 0x03:
			dprintf(_("Message: Phonebook read entry error received:\n"));
			switch (MessageBuffer[4]) {
			case 0x7d:
				dprintf(_("   Invalid memory type!\n"));
				CurrentPhonebookError = GE_INVALIDMEMORYTYPE;
				break;
			default:
				dprintf(_("   Unknown error!\n"));
				CurrentPhonebookError = GE_INTERNALERROR;
				break;
			}
			break;

		case 0x05:
			dprintf(_("Message: Phonebook written correctly.\n"));
			CurrentPhonebookError = GE_NONE;
			break;

		case 0x06:
			switch (MessageBuffer[4]) {

				/* FIXME: other errors? When I send the phonebook with index of 350 it
				   still report error 0x7d :-( */

			case 0x7d:
				dprintf(_("Message: Phonebook not written - name is too long.\n"));
				CurrentPhonebookError = GE_PHBOOKNAMETOOLONG;
				break;

			default:
				dprintf(_("   Unknown error!\n"));
				CurrentPhonebookError = GE_INTERNALERROR;
				break;
			}

			break;

		case 0x08:
			dprintf(_("Message: Memory status received:\n"));
			dprintf(_("   Memory Type: %s\n"), FB61_MemoryType_String[MessageBuffer[4]]);
			dprintf(_("   Used: %d\n"), MessageBuffer[6]);
			dprintf(_("   Free: %d\n"), MessageBuffer[5]);
			CurrentMemoryStatus->Used = MessageBuffer[6];
			CurrentMemoryStatus->Free = MessageBuffer[5];
			CurrentMemoryStatusError = GE_NONE;
			break;

		case 0x09:
			switch (MessageBuffer[4]) {
			case 0x6f:
				dprintf(_("Message: Memory status error, phone is probably powered off.\n"));
				CurrentMemoryStatusError = GE_TIMEOUT;
				break;

			case 0x7d:
				dprintf(_("Message: Memory status error, memory type not supported by phone model.\n"));
				CurrentMemoryStatusError = GE_INTERNALERROR;
				break;

			case 0x8d:
				dprintf(_("Message: Memory status error, waiting for security code.\n"));
				CurrentMemoryStatusError = GE_INVALIDSECURITYCODE;
				break;

			default:
				dprintf(_("Message: Unknown Memory status error, subtype (MessageBuffer[4]) = %02x\n"),MessageBuffer[4]);
				break;
			}

			break;
    
		case 0x11:   /* Get group data */
      
			/* [ID],[name_len],[name].,[ringtone],[graphicon],[lenhi],[lenlo],[bitmap] */
 
			if (GetBitmap != NULL) {
				count = MessageBuffer[5];
				memcpy(GetBitmap->text, MessageBuffer+6, count);
				GetBitmap->text[count] = 0;

				dprintf(_("Message: Caller group logo etc.\n"));
				dprintf(_("Caller group name: %s\n"), GetBitmap->text);

				count += 6;
				GetBitmap->ringtone = MessageBuffer[count++];
				count++;
				GetBitmap->size = MessageBuffer[count++]<<8;
				GetBitmap->size += MessageBuffer[count++];
				count++;
				GetBitmap->width = MessageBuffer[count++];
				GetBitmap->height = MessageBuffer[count++];
				count++;
				tmp = GetBitmap->height * GetBitmap->width / 8;
				if (GetBitmap->size > tmp) GetBitmap->size = tmp;
				memcpy(GetBitmap->bitmap, MessageBuffer+count, GetBitmap->size);
				GetBitmapError = GE_NONE;
			}
			else {
				dprintf(_("Message: Caller group data received but not requested!\n"));
			}
			break;

		case 0x12:   /* Get group data error */
			GetBitmapError = GE_UNKNOWN;   
			dprintf(_("Message: Error attempting to get caller group data.\n"));
			break;
      
		case 0x14:   /* Set group data OK */
			SetBitmapError = GE_NONE;      
			dprintf(_("Message: Caller group data set correctly.\n"));
			break;

		case 0x15:   /* Set group data error */
			SetBitmapError = GE_UNKNOWN;      
			dprintf(_("Message: Error attempting to set caller group data\n"));
			break;  

		case 0x17:
			CurrentSpeedDialEntry->MemoryType = MessageBuffer[4];
			CurrentSpeedDialEntry->Location = MessageBuffer[5];
			dprintf(_("Message: Speed dial entry received:\n"));
			dprintf(_("   Location: %d\n"), CurrentSpeedDialEntry->Location);
			dprintf(_("   MemoryType: %s\n"), FB61_MemoryType_String[CurrentSpeedDialEntry->MemoryType]);
			dprintf(_("   Number: %d\n"), CurrentSpeedDialEntry->Number);
			CurrentSpeedDialError = GE_NONE;
			break;

		case 0x18:
			dprintf(_("Message: Speed dial entry error\n"));
			CurrentSpeedDialError = GE_INVALIDSPEEDDIALLOCATION;
			break;

		case 0x1a:
			dprintf(_("Message: Speed dial entry set.\n"));
			CurrentSpeedDialError = GE_NONE;
			break;

		case 0x1b:
			dprintf(_("Message: Speed dial entry setting error.\n"));
			CurrentSpeedDialError = GE_INVALIDSPEEDDIALLOCATION;
			break;

		default:
			dprintf(_("Message: Unknown message of type 0x03\n"));
			break;
		}

		break;

		/* Phone status */
	  
	case 0x04:
		switch (MessageBuffer[3]) {
		case 0x02:
			dprintf(_("Message: Phone status received:\n"));
			dprintf(_("   Mode: "));
			switch (MessageBuffer[4]) {
			case 0x01: dprintf(_("registered within the network\n")); break;
				/* I was really amazing why is there a hole in the type of 0x02, now I
				   know... */
			case 0x02: dprintf(_("call in progress\n")); /* ringing or already answered call */ break;
			case 0x03: dprintf(_("waiting for security code\n")); break;
			case 0x04: dprintf(_("powered off\n")); break;
			default: dprintf(_("unknown\n")); break;
			}
			dprintf(_("   Power source: "));
			switch (MessageBuffer[7]) {
			case 0x01: dprintf(_("AC/DC\n")); break;
			case 0x02: dprintf(_("battery\n")); break;
			default: dprintf(_("unknown\n")); break;
			}
			dprintf(_("   Battery Level: %d\n"), MessageBuffer[8]);
			dprintf(_("   Signal strength: %d\n"), MessageBuffer[5]);

			CurrentRFLevel = MessageBuffer[5];
			CurrentBatteryLevel = MessageBuffer[8];
			CurrentPowerSource = MessageBuffer[7];
			break;

		default:
			dprintf(_("Message: Unknown message of type 0x04\n"));
			break;
		}

		break;
  
		/* Startup Logo, Operator Logo and Profiles. */

	case 0x05:
		switch (MessageBuffer[3]) {
		case 0x11:   /* Profile feature change result */
			dprintf(_("Message: Profile feature change result.\n"));
			CurrentProfileError = GE_NONE;
			break;

		case 0x14:   /* Profile feature */
			switch (MessageBuffer[6]) {
			case 0x00:
				CurrentProfile->KeypadTone = MessageBuffer[8];
				break;
			case 0x01:
				CurrentProfile->Lights = MessageBuffer[8];
				break;
			case 0x02:
				CurrentProfile->CallAlert = MessageBuffer[8];
				break;
			case 0x03:
				CurrentProfile->Ringtone = MessageBuffer[8];
				break;
			case 0x04:
				CurrentProfile->Volume = MessageBuffer[8];
				break;
			case 0x05:
				CurrentProfile->MessageTone = MessageBuffer[8];
				break;
			case 0x06:
				CurrentProfile->Vibration = MessageBuffer[8];
				break;
			case 0x07:
				CurrentProfile->WarningTone = MessageBuffer[8];
				break;
			case 0x08:
				CurrentProfile->CallerGroups = MessageBuffer[8];
				break;
			case 0x09:
				CurrentProfile->AutomaticAnswer = MessageBuffer[8];
				break;
			}

			CurrentProfileError = GE_NONE;
			break;

		case 0x17:   /* Startup Logo */
			dprintf(_("Message: Startup Logo, welcome note and dealer welcome note received.\n"));
			if (GetBitmap != NULL) {
				supported = false;
				count = 5;
				for (tmp = 0; tmp < MessageBuffer[4]; tmp++){
					switch (MessageBuffer[count++]) {
					case 0x01:
						if (GetBitmap->type == GSM_StartupLogo) {
							GetBitmap->height = MessageBuffer[count++];
							GetBitmap->width = MessageBuffer[count++];
							GetBitmap->size = GetBitmap->height * GetBitmap->width / 8;
							length = GetBitmap->size;
							memcpy(GetBitmap->bitmap, MessageBuffer + count, length);
						} else {
							length = MessageBuffer[count++];
							length = length * MessageBuffer[count++] / 8;
						}
						count += length;
						dprintf(_("Startup logo supported - "));
						if (length != 0) dprintf(_("currently set\n"));
						else dprintf(_("currently empty\n"));
						if (GetBitmap->type == GSM_StartupLogo) supported = true;
						break;
					case 0x02:
						length = MessageBuffer[count];
						if (GetBitmap->type == GSM_WelcomeNoteText) {
							memcpy(GetBitmap->text, MessageBuffer + count + 1, length);
							GetBitmap->text[length] = 0;
						}
						dprintf(_("Startup Text supported - "));
						if (length != 0) {
							dprintf(_("currently set to \""));
							for (i = 0; i < length; i++) dprintf(_("%c"), MessageBuffer[count+1+i]);
							dprintf(_("\"\n"));
						} else {
							dprintf(_("currently empty\n"));
						}
						count += length + 1;
						if (GetBitmap->type == GSM_WelcomeNoteText) supported = true;
						break;
					case 0x03:
						length = MessageBuffer[count];
						if (GetBitmap->type == GSM_DealerNoteText) {
							memcpy(GetBitmap->text, MessageBuffer + count + 1, length);
							GetBitmap->text[length] = 0;
						}
						dprintf(_("Dealer Welcome supported - "));
						if (length != 0) {
							dprintf(_("currently set to \""));
							for (i = 0;i < length; i++) dprintf(_("%c"), MessageBuffer[count+1+i]);
							dprintf(_("\"\n"));
						} else dprintf(_("currently empty\n"));
						count += length + 1;
						if (GetBitmap->type==GSM_DealerNoteText) supported = true;
						break;
					}
				}
				if (supported) GetBitmapError = GE_NONE;
				else GetBitmapError = GE_NOTSUPPORTED;
			} else dprintf(_("Message: Startup logo received but not requested!\n"));
			break;

		case 0x19:   /* Set startup OK */
			SetBitmapError = GE_NONE;
			dprintf(_("Message: Startup logo, welcome note or dealer welcome note correctly set.\n"));
			break;      

		case 0x1b:   /* Incoming profile name */
			if (MessageBuffer[9] == 0x00)
				CurrentProfile->DefaultName = MessageBuffer[8];
			else {
				CurrentProfile->DefaultName = -1;
				sprintf(CurrentProfile->Name, MessageBuffer + 10, MessageBuffer[9]);
				CurrentProfile->Name[MessageBuffer[9]] = '\0';
			}
			CurrentProfileError = GE_NONE;
			break;

		case 0x1d:   /* Profile name set result */
			CurrentProfileError = GE_NONE;
			break;

		case 0x31:   /* Set Operator Logo OK */
			dprintf(_("Message: Operator logo correctly set.\n"));
			SetBitmapError = GE_NONE;      
			break;
      
		case 0x32:   /* Set Operator Logo Error */
			SetBitmapError = GE_UNKNOWN;
			dprintf(_("Message: Error setting operator logo!\n"));
			break;

		case 0x34:  /* Operator Logo */
			/* [location],[netcode x 3],[lenhi],[lenlo],[bitmap] */
			if (GetBitmap != NULL) {
				count = 5;  /* Location ignored. */
				/* Network code is stored as 0xBA 0xXC 0xED ("ABC DE"). */
				GetBitmap->netcode[0] = '0' + (MessageBuffer[count] & 0x0f);
				GetBitmap->netcode[1] = '0' + (MessageBuffer[count++] >> 4);
				GetBitmap->netcode[2] = '0' + (MessageBuffer[count++] & 0x0f);
				GetBitmap->netcode[3] = ' ';
				GetBitmap->netcode[4] = '0' + (MessageBuffer[count] & 0x0f);
				GetBitmap->netcode[5] = '0' + (MessageBuffer[count++] >> 4);
				GetBitmap->netcode[6] = 0;
				dprintf(_("Message: Operator Logo for %s (%s) network received.\n"),
					GetBitmap->netcode,
					GSM_GetNetworkName(GetBitmap->netcode));
				GetBitmap->size = MessageBuffer[count++] << 8;
				GetBitmap->size += MessageBuffer[count++];
				count++;
				GetBitmap->width = MessageBuffer[count++];
				GetBitmap->height = MessageBuffer[count++];
				count++;
				tmp = GetBitmap->height * GetBitmap->width / 8;
				if (GetBitmap->size > tmp) GetBitmap->size = tmp;
				memcpy(GetBitmap->bitmap, MessageBuffer + count, GetBitmap->size);
				GetBitmapError = GE_NONE;
			}
			else dprintf(_("Message: Operator logo received but not requested!\n"));
			break;
      
		case 0x35:  /* Get op logo error */
			dprintf(_("Message: Error getting operator logo!\n"));
			GetBitmapError = GE_UNKNOWN; 
			break;
		}
		break;

		/* Security code requests */

	case 0x08:
		switch(MessageBuffer[3]) {
		case 0x08:
			*CurrentSecurityCodeStatus = MessageBuffer[4];
			dprintf(_("Message: Security Code status received: "));
			switch(*CurrentSecurityCodeStatus) {
			case GSCT_SecurityCode: dprintf(_("waiting for Security Code.\n")); break;
			case GSCT_Pin: dprintf(_("waiting for PIN.\n")); break;
			case GSCT_Pin2: dprintf(_("waiting for PIN2.\n")); break;
			case GSCT_Puk: dprintf(_("waiting for PUK.\n")); break;
			case GSCT_Puk2: dprintf(_("waiting for PUK2.\n")); break;
			case GSCT_None: dprintf(_("nothing to enter.\n")); break;
			default: dprintf(_("Unknown!\n")); break;
			}
			CurrentSecurityCodeError = GE_NONE;
			break;

		case 0x0b:
			dprintf(_("Message: Security code accepted.\n"));
			CurrentSecurityCodeError = GE_NONE;
			break;

		default:
			dprintf(_("Message: Security code is wrong. You're not my big owner :-)\n"));
			CurrentSecurityCodeError = GE_INVALIDSECURITYCODE;
			break;
		}

		break;

		/* Network Info */

	case 0x0a:

		switch (MessageBuffer[3]) {
		case 0x71:
			/* Make sure we are expecting NetworkInfo frame */
			if (CurrentNetworkInfo && CurrentNetworkInfoError == GE_BUSY) {
				sprintf(CurrentNetworkInfo->NetworkCode, "%x%x%x %x%x", MessageBuffer[14] & 0x0f, MessageBuffer[14] >>4, MessageBuffer[15] & 0x0f, MessageBuffer[16] & 0x0f, MessageBuffer[16] >> 4);
				sprintf(CurrentNetworkInfo->CellID, "%02x%02x", MessageBuffer[10], MessageBuffer[11]);
				sprintf(CurrentNetworkInfo->LAC, "%02x%02x", MessageBuffer[12], MessageBuffer[13]);

				dprintf(_("Message: Network information:\n"));

				dprintf(_("   CellID: %s\n"), CurrentNetworkInfo->CellID);
				dprintf(_("   LAC: %s\n"), CurrentNetworkInfo->LAC);
				dprintf(_("   Network code: %s\n"), CurrentNetworkInfo->NetworkCode);
				dprintf(_("   Network name: %s (%s)\n"),
					GSM_GetNetworkName(CurrentNetworkInfo->NetworkCode),
					GSM_GetCountryName(CurrentNetworkInfo->NetworkCode));
				dprintf(_("   Status: "));

				switch (MessageBuffer[8]) {
				case 0x01: dprintf(_("home network selected")); break;
				case 0x02: dprintf(_("roaming network")); break;
				case 0x03: dprintf(_("requesting network")); break;
				case 0x04: dprintf(_("not registered in the network")); break;
				default: dprintf(_("unknown")); break;
				}

				dprintf("\n");

				dprintf(_("   Network selection: %s\n"), MessageBuffer[9] == 1 ? _("manual") : _("automatic"));
			}
			CurrentNetworkInfoError = GE_NONE;
			break;

		default:
			dprintf(_("Message: Unknown message of type 0x0a\n"));
			break;
		}

		break;

		/* Display status. */

	case 0x0d:
		switch(MessageBuffer[3]) {
		case 0x50:
			dprintf("%i %i ", MessageBuffer[6], MessageBuffer[5]);
			for (i = 0; i < MessageBuffer[7]; i++)
				dprintf("%c", MessageBuffer[9+(i*2)]);
			dprintf("\n");
			break;
 
		case 0x52:
			for (i = 0; i < MessageBuffer[4]; i++)
				if (MessageBuffer[2*i+6] == 2)
					DisplayStatus |= 1 << (MessageBuffer[2*i+5] - 1);
				else
					DisplayStatus &= (0xff - (1 << (MessageBuffer[2*i+5] - 1)));
			dprintf(_("Call in progress: %s\n"), DisplayStatus & (1 << DS_Call_In_Progress) ? "on" : "off");
			dprintf(_("Unknown: %s\n"),          DisplayStatus & (1 << DS_Unknown) ? "on" : "off");
			dprintf(_("Unread SMS: %s\n"),       DisplayStatus & (1 << DS_Unread_SMS) ? "on" : "off");
			dprintf(_("Voice call: %s\n"),       DisplayStatus & (1 << DS_Voice_Call) ? "on" : "off");
			dprintf(_("Fax call active: %s\n"),  DisplayStatus & (1 << DS_Fax_Call) ? "on" : "off");
			dprintf(_("Data call active: %s\n"), DisplayStatus & (1 << DS_Data_Call) ? "on" : "off");
			dprintf(_("Keyboard lock: %s\n"),    DisplayStatus & (1 << DS_Keyboard_Lock) ? "on" : "off");
			dprintf(_("SMS storage full: %s\n"), DisplayStatus & (1 << DS_SMS_Storage_Full) ? "on" : "off");
			DisplayStatusError = GE_NONE;
			break;
      
		case 0x54:
			if (MessageBuffer[5] == 1) {
				dprintf(_("Display output successfully disabled/enabled.\n"));
				CurrentDisplayOutputError = GE_NONE;
			}
			break;
      
		default:
			dprintf(_("Unknown message of type 0x0d.\n"));
			break;
		}
		break;

		/* Phone Clock and Alarm */

	case 0x11:
		switch (MessageBuffer[3]) {
		case 0x61:
			switch (MessageBuffer[4]) {
			case 0x01:
				dprintf(_("Message: Date and time set correctly\n"));
				CurrentSetDateTimeError = GE_NONE;
				break;
      
			default:
				dprintf(_("Message: Date and time set error\n"));
				CurrentSetDateTimeError = GE_INVALIDDATETIME;
				break;
			}
			break;

		case 0x63:
			CurrentDateTime->Year = 256 * MessageBuffer[8] + MessageBuffer[9];
			CurrentDateTime->Month = MessageBuffer[10];
			CurrentDateTime->Day = MessageBuffer[11];

			CurrentDateTime->Hour = MessageBuffer[12];
			CurrentDateTime->Minute = MessageBuffer[13];
			CurrentDateTime->Second = MessageBuffer[14];

			dprintf(_("Message: Date and time\n"));
			dprintf(_("   Time: %02d:%02d:%02d\n"), CurrentDateTime->Hour, CurrentDateTime->Minute, CurrentDateTime->Second);
			dprintf(_("   Date: %4d/%02d/%02d\n"), CurrentDateTime->Year, CurrentDateTime->Month, CurrentDateTime->Day);

			CurrentDateTimeError = GE_NONE;
			break;

		case 0x6c:
			switch (MessageBuffer[4]) {
			case 0x01:
				dprintf(_("Message: Alarm set correctly\n"));
				CurrentSetAlarmError = GE_NONE;
				break;
      
			default:
				dprintf(_("Message: Date and time set error\n"));
				CurrentSetAlarmError = GE_INVALIDDATETIME;
				break;
			}
			break;

		case 0x6e:
			dprintf(_("Message: Alarm\n"));
			dprintf(_("   Alarm: %02d:%02d\n"), MessageBuffer[9], MessageBuffer[10]);
			dprintf(_("   Alarm is %s\n"), (MessageBuffer[8] == 2) ? _("on") : _("off"));

			CurrentAlarm->Hour = MessageBuffer[9];
			CurrentAlarm->Minute = MessageBuffer[10];
			CurrentAlarm->Second = 0;
			CurrentAlarm->AlarmEnabled = (MessageBuffer[8] == 2);
			CurrentAlarmError = GE_NONE;
			break;

		default:
			dprintf(_("Message: Unknown message of type 0x11\n"));
			break;
		}
		break;

		/* Calendar notes handling */

	case 0x13:
		switch (MessageBuffer[3]) {
		case 0x65:
			switch(MessageBuffer[4]) {
			case 0x01: /* This message is also sent when the user enters the new entry on keypad */
				dprintf(_("Message: Calendar note write succesfull!\n"));
				CurrentCalendarNoteError = GE_NONE;
				break;
			case 0x73:
				dprintf(_("Message: Calendar note write failed!\n"));
				CurrentCalendarNoteError = GE_INTERNALERROR;
				break;
			case 0x7d:
				dprintf(_("Message: Calendar note write failed!\n"));
				CurrentCalendarNoteError = GE_INTERNALERROR;
				break;
			default:
				dprintf(_("Unknown message of type 0x13 and subtype 0x65\n"));
				break;
			}
    
			break;

		case 0x67:

			switch (MessageBuffer[4]) {

			case 0x01:
      
				CurrentCalendarNote->Type = MessageBuffer[8];

				CurrentCalendarNote->Time.Year = 256 * MessageBuffer[9] + MessageBuffer[10];
				CurrentCalendarNote->Time.Month = MessageBuffer[11];
				CurrentCalendarNote->Time.Day = MessageBuffer[12];

				CurrentCalendarNote->Time.Hour = MessageBuffer[13];
				CurrentCalendarNote->Time.Minute = MessageBuffer[14];
				CurrentCalendarNote->Time.Second = MessageBuffer[15];

				CurrentCalendarNote->Alarm.Year = 256 * MessageBuffer[16] + MessageBuffer[17];
				CurrentCalendarNote->Alarm.Month = MessageBuffer[18];
				CurrentCalendarNote->Alarm.Day = MessageBuffer[19];

				CurrentCalendarNote->Alarm.Hour = MessageBuffer[20];
				CurrentCalendarNote->Alarm.Minute = MessageBuffer[21];
				CurrentCalendarNote->Alarm.Second = MessageBuffer[22];

				memcpy(CurrentCalendarNote->Text, MessageBuffer + 24, MessageBuffer[23]);
				CurrentCalendarNote->Text[MessageBuffer[23]] = 0;

				if (CurrentCalendarNote->Type == GCN_CALL) {
					memcpy(CurrentCalendarNote->Phone,MessageBuffer + 24 + MessageBuffer[23] + 1,MessageBuffer[24+MessageBuffer[23]]);
					CurrentCalendarNote->Phone[MessageBuffer[24+MessageBuffer[23]]] = 0;
				}

				dprintf(_("Message: Calendar note received.\n"));
				dprintf(_("   Date: %d-%02d-%02d\n"), CurrentCalendarNote->Time.Year,
					CurrentCalendarNote->Time.Month,
					CurrentCalendarNote->Time.Day);
				dprintf(_("   Time: %02d:%02d:%02d\n"), CurrentCalendarNote->Time.Hour,
					CurrentCalendarNote->Time.Minute,
					CurrentCalendarNote->Time.Second);

				/* Some messages do not have alarm set up */

				if (CurrentCalendarNote->Alarm.Year != 0) {
					dprintf(_("   Alarm date: %d-%02d-%02d\n"), CurrentCalendarNote->Alarm.Year,
						CurrentCalendarNote->Alarm.Month,
						CurrentCalendarNote->Alarm.Day);
					dprintf(_("   Alarm time: %02d:%02d:%02d\n"), CurrentCalendarNote->Alarm.Hour,
						CurrentCalendarNote->Alarm.Minute,
						CurrentCalendarNote->Alarm.Second);
				}

				dprintf(_("   Type: %d\n"), CurrentCalendarNote->Type);
				dprintf(_("   Text: %s\n"), CurrentCalendarNote->Text);
				if (CurrentCalendarNote->Type == GCN_CALL)
					dprintf(_("   Phone: %s\n"), CurrentCalendarNote->Phone);
				CurrentCalendarNoteError = GE_NONE;
				break;

			case 0x93:
				dprintf(_("Message: Calendar note not available\n"));
				CurrentCalendarNoteError = GE_INVALIDCALNOTELOCATION;
				break;

			default:
				dprintf(_("Message: Calendar note error\n"));
				CurrentCalendarNoteError = GE_INTERNALERROR;
				break;
			}
			break;

		case 0x69:
			switch (MessageBuffer[4]) {
				/* This message is also sent when the user deletes an old entry on
				   keypad or moves an old entry somewhere (there is also `write'
				   message). */
			case 0x01:
				dprintf(_("Message: Calendar note deleted\n"));
				CurrentCalendarNoteError = GE_NONE;
				break;
			case 0x93:
				dprintf(_("Message: Calendar note can't be deleted\n"));
				CurrentCalendarNoteError = GE_INVALIDCALNOTELOCATION;
				break;
			default:
				dprintf(_("Message: Calendar note deleting error\n"));
				CurrentCalendarNoteError = GE_INTERNALERROR;
				break;
			}

			break;

		case 0x6a:
			/* Jano will probably implement something similar to IncomingCall
			   notification for easy integration into xgnokii */
			dprintf(_("Message: Calendar Alarm active\n"));
			dprintf(_("   Item number: %d\n"), MessageBuffer[4]);
			break;

		default:
			dprintf(_("Message: Unknown message of type 0x13\n"));
			break;
		}

		break;

		/* SMS Messages frame received */

	case 0x14:
		switch (MessageBuffer[3]) {
		case 0x08:
			switch (MessageBuffer[7]) {
			case 0x00:
				CurrentSMSMessage->Type = GST_MT;
				offset = 4;
				break;
			case 0x01:
				CurrentSMSMessage->Type = GST_DR;
				offset = 3;
				break;
			case 0x02:
				CurrentSMSMessage->Type = GST_MO;
				offset = 5;
				break;
			default:
				CurrentSMSMessage->Type = GST_UN;
				offset = 4; /* ??? */
				break;
			}

			/* Field Short Message Status - MessageBuffer[4] seems not to be
			   compliant with GSM 07.05 spec.
			   Meaning	Nokia protocol		GMS spec
			   ----------------------------------------------------
			   MO Sent	0x05			0x07 or 0x01
			   MO Not sent	0x07			0x06 or 0x00
			   MT Read	0x01			0x05 or 0x01
			   MT Not read	0x03			0x04 or 0x00
			   ----------------------------------------------------
			   See GSM 07.05 section 2.5.2.6 and correct me if I'm wrong.
         
			   Pawel Kot */

			if (MessageBuffer[4] & 0x02)
				CurrentSMSMessage->Status = GSS_NOTSENTREAD;
			else
				CurrentSMSMessage->Status = GSS_SENTREAD;

			off = 0;
			if (MessageBuffer[20] & 0x40) {
				switch (MessageBuffer[40+offset]) {
				case 0x00: /* concatenated messages */
					dprintf(_("Concatenated message!!!\n"));
					CurrentSMSMessage->UDHType = GSM_ConcatenatedMessages;
					if (MessageBuffer[41+offset] != 0x03) {
				/* should be some error */
					}
					break;
				case 0x05: /* logos */
					switch (MessageBuffer[43+offset]) {
					case 0x82:
						CurrentSMSMessage->UDHType = GSM_OpLogo;
						break;
					case 0x83:
						CurrentSMSMessage->UDHType = GSM_CallerIDLogo;
						break;
					}
					break;
				default:
					break;
				}
				/* Skip user data header when reading data */
				off = (MessageBuffer[39+offset] + 1);
				for (i = 0; i < off; i++)
					CurrentSMSMessage->UDH[i] = MessageBuffer[39+offset+i];
			} else {
				CurrentSMSMessage->UDHType = GSM_NoUDH;
			}
      
			MessageBuffer[20+offset] = (MessageBuffer[20+offset]+1)/2+1;

			dprintf(_("Number: %d\n"), MessageBuffer[6]);
			switch (CurrentSMSMessage->Type) {
			case GST_MO:
				dprintf(_("Message: Outbox message (mobile originated)\n"));
				if (CurrentSMSMessage->Status)
					dprintf(_("Sent\n"));
				else
					dprintf(_("Not sent\n"));
				break;

			default:
				dprintf(_("Message: Received SMS (mobile terminated)\n"));
				if (CurrentSMSMessage->Type == GST_DR)
					dprintf(_("Delivery Report\n"));
				if (CurrentSMSMessage->Type == GST_UN)
					dprintf(_("Unknown type\n"));

				if (CurrentSMSMessage->Status)
					dprintf(_("Read\n"));
				else
					dprintf(_("Not read\n"));

				dprintf(_("   Date: %s "), FB61_GetPackedDateTime(MessageBuffer + 32 + offset));

				if (MessageBuffer[38+offset]) {
					if (MessageBuffer[38+offset] & 0x08)
						dprintf("-");
					else
						dprintf("+");

					dprintf(_("%02d00"), (10 * (MessageBuffer[38+offset] & 0x07) + (MessageBuffer[38+offset] >> 4)) / 4);
				}
				dprintf("\n");

				if (CurrentSMSMessage->Type == GST_DR) {
					dprintf(_("   SMSC response date: %s "), FB61_GetPackedDateTime(MessageBuffer + 39 + offset));
					if (MessageBuffer[45+offset]) {
						if (MessageBuffer[45+offset] & 0x08)
							dprintf("-");
						else
							dprintf("+");

						dprintf(_("%02d00"),(10 * (MessageBuffer[45+offset] & 0x07) + (MessageBuffer[45+offset] >> 4)) / 4);
					}
					dprintf("\n");
				}
				dprintf(_("   SMS center number: %s\n"), FB61_GetBCDNumber(MessageBuffer + 8));
				dprintf(_("   Remote number: %s\n"), FB61_GetBCDNumber(MessageBuffer + 20 + offset));
				break;
			}

			/* In Outbox messages fields:
			   - datetime
			   - sender
			   - message center
			   are not filled in, so we ignore it.
			*/
			if (CurrentSMSMessage->Type != GST_MO) {
				CurrentSMSMessage->Time.Year = 10 * (MessageBuffer[32+offset] & 0x0f) + (MessageBuffer[32+offset] >> 4);
				CurrentSMSMessage->Time.Month = 10 * (MessageBuffer[33+offset] & 0x0f) + (MessageBuffer[33+offset] >> 4);
				CurrentSMSMessage->Time.Day = 10 * (MessageBuffer[34+offset] & 0x0f) + (MessageBuffer[34+offset] >> 4);
				CurrentSMSMessage->Time.Hour = 10 * (MessageBuffer[35+offset] & 0x0f) + (MessageBuffer[35+offset] >> 4);
				CurrentSMSMessage->Time.Minute = 10 * (MessageBuffer[36+offset] & 0x0f) + (MessageBuffer[36+offset] >> 4);
				CurrentSMSMessage->Time.Second = 10 * (MessageBuffer[37+offset] & 0x0f) + (MessageBuffer[37+offset] >> 4);
				CurrentSMSMessage->Time.Timezone = (10 * (MessageBuffer[38+offset] & 0x07) + (MessageBuffer[38+offset] >> 4)) / 4;
				if (MessageBuffer[38+offset] & 0x08)
					CurrentSMSMessage->Time.Timezone = -CurrentSMSMessage->Time.Timezone;
				strcpy(CurrentSMSMessage->Sender, FB61_GetBCDNumber(MessageBuffer + 20 + offset));
				strcpy(CurrentSMSMessage->MessageCenter.Number, FB61_GetBCDNumber(MessageBuffer + 8));
			}

			if (CurrentSMSMessage->Type != GST_DR) {
				/* 8bit SMS */
				if ((MessageBuffer[18+offset] & 0xf4) == 0xf4) {
					CurrentSMSMessage->EightBit = true;
					tmp=CurrentSMSMessage->Length=MessageBuffer[19+offset];
					offset += off;
					memcpy(output, MessageBuffer - 39 - offset - 2, tmp - offset);
					/* 7bit SMS */
				} else {
					CurrentSMSMessage->EightBit = false;
					CurrentSMSMessage->Length=MessageBuffer[19+offset] - (off * 8 + ((7-off)%7)) / 7;
					offset += off;
					tmp=UnpackEightBitsToSeven((7-off)%7, MessageLength - 39 - offset - 2, CurrentSMSMessage->Length, MessageBuffer + 39 + offset, output);
				}
				for (i = 0; i < tmp;i++) {
					dprintf("%c", GSM_Default_Alphabet[output[i]]);
					CurrentSMSMessage->MessageText[i] = GSM_Default_Alphabet[output[i]];
				}
			} else { /* CurrentSMSMessage->Type == GST_DR (Delivery Report) */
				/* SMSC Response time */
				CurrentSMSMessage->SMSCTime.Year = 10 * (MessageBuffer[39+offset] & 0x0f) + (MessageBuffer[39+offset] >> 4);
				CurrentSMSMessage->SMSCTime.Month = 10 * (MessageBuffer[40+offset] & 0x0f) + (MessageBuffer[40+offset] >> 4);
				CurrentSMSMessage->SMSCTime.Day = 10 * (MessageBuffer[41+offset] & 0x0f) + (MessageBuffer[41+offset] >> 4);
				CurrentSMSMessage->SMSCTime.Hour = 10 * (MessageBuffer[42+offset] & 0x0f) + (MessageBuffer[42+offset] >> 4);
				CurrentSMSMessage->SMSCTime.Minute = 10 * (MessageBuffer[43+offset] & 0x0f) + (MessageBuffer[43+offset] >> 4);
				CurrentSMSMessage->SMSCTime.Second = 10 * (MessageBuffer[44+offset] & 0x0f) + (MessageBuffer[44+offset] >> 4);
				CurrentSMSMessage->SMSCTime.Timezone = (10 * (MessageBuffer[45+offset] & 0x07) + (MessageBuffer[45+offset] >> 4)) / 4;
				if (MessageBuffer[45+offset] & 0x08) CurrentSMSMessage->SMSCTime.Timezone = -CurrentSMSMessage->SMSCTime.Timezone;
				if (MessageBuffer[22] < 0x03) {
					strcpy(CurrentSMSMessage->MessageText, _("Delivered"));
					switch (MessageBuffer[22]) {
					case 0x00:
						dprintf(_("SM received by the SME"));
						break;
					case 0x01:
						dprintf(_("SM forwarded by the SC to the SME but the SC is unable to confirm delivery"));
						break;
					case 0x02:
						dprintf(_("SM replaced by the SC"));
						break;
					}
					CurrentSMSMessage->Length = tmp = 10;
				} else if (MessageBuffer[22] & 0x40) {

					strcpy(CurrentSMSMessage->MessageText, _("Failed"));

					/* more detailed reason only for debug */

					if (MessageBuffer[22] & 0x20) {
						dprintf(_("Temporary error, SC is not making any more transfer attempts\n"));

						switch (MessageBuffer[22]) {
						case 0x60:
							dprintf(_("Congestion"));
							break;
						case 0x61:
							dprintf(_("SME busy"));
							break;
						case 0x62:
							dprintf(_("No response from SME"));
							break;
						case 0x63:
							dprintf(_("Service rejected"));
							break;
						case 0x64:
							dprintf(_("Quality of service not aviable"));
							break;
						case 0x65:
							dprintf(_("Error in SME"));
							break;
						default:
							dprintf(_("Reserved/Specific to SC: %x"), MessageBuffer[22]);
							break;
						}
					} else {
						dprintf(_("Permanent error, SC is not making any more transfer attempts\n"));
						switch (MessageBuffer[22]) {
						case 0x40:
							dprintf(_("Remote procedure error"));
							break;
						case 0x41:
							dprintf(_("Incompatibile destination"));
							break;
						case 0x42:
							dprintf(_("Connection rejected by SME"));
							break;
						case 0x43:
							dprintf(_("Not obtainable"));
							break;
						case 0x44:
							dprintf(_("Quality of service not aviable"));
							break;
						case 0x45:
							dprintf(_("No internetworking available"));
							break;
						case 0x46:
							dprintf(_("SM Validity Period Expired"));
							break;
						case 0x47:
							dprintf(_("SM deleted by originating SME"));
							break;
						case 0x48:
							dprintf(_("SM Deleted by SC Administration"));
							break;
						case 0x49:
							dprintf(_("SM does not exist"));
							break;
						default:
							dprintf(_("Reserved/Specific to SC: %x"), MessageBuffer[22]);
							break;
						}
					}
					CurrentSMSMessage->Length = tmp = 6;
				} else if (MessageBuffer[22] & 0x20) {
					strcpy(CurrentSMSMessage->MessageText, _("Pending"));

					/* more detailed reason only for debug */
					dprintf(_("Temporary error, SC still trying to transfer SM\n"));
					switch (MessageBuffer[22]) {
					case 0x20:
						dprintf(_("Congestion"));
						break;
					case 0x21:
						dprintf(_("SME busy"));
						break;
					case 0x22:
						dprintf(_("No response from SME"));
						break;
					case 0x23:
						dprintf(_("Service rejected"));
						break;
					case 0x24:
						dprintf(_("Quality of service not aviable"));
						break;
					case 0x25:
						dprintf(_("Error in SME"));
						break;
					default:
						dprintf(_("Reserved/Specific to SC: %x"), MessageBuffer[22]);
						break;
					}
					CurrentSMSMessage->Length = tmp = 7;
				} else {
					strcpy(CurrentSMSMessage->MessageText, _("Unknown"));

					/* more detailed reason only for debug */
					dprintf(_("Reserved/Specific to SC: %x"), MessageBuffer[22]);
					CurrentSMSMessage->Length = tmp = 8;
				}
			}

			CurrentSMSMessage->MessageText[CurrentSMSMessage->Length] = 0;

			CurrentSMSPointer = tmp;
      
			CurrentSMSMessage->MemoryType = MessageBuffer[5];
			CurrentSMSMessage->MessageNumber = MessageBuffer[6];
 
			/* Signal no error to calling code. */

			CurrentSMSMessageError = GE_NONE;
			dprintf("\n");
			break;
    
		case 0x05:
			dprintf(_("Message stored at %d\n"), MessageBuffer[5]);
			CurrentSMSMessageError = GE_NONE;
			break;

		case 0x06:
			dprintf(_("SMS saving failed\n"));
			switch (MessageBuffer[4]) {
			case 0x02:
				dprintf(_("   All locations busy.\n"));
				CurrentSMSMessageError = GE_MEMORYFULL;
				break;
			case 0x03:
				dprintf(_("   Invalid location!\n"));
				CurrentSMSMessageError = GE_INVALIDSMSLOCATION;
				break;
			default:
				dprintf(_("   Unknown error.\n"));
				CurrentSMSMessageError = GE_UNKNOWN;
				break;
			}
			break;

		case 0x09:
			/* We have requested invalid or empty location. */
			dprintf(_("Message: SMS reading failed.\n"));
			switch (MessageBuffer[4]) {
			case 0x02:
				dprintf(_("   Invalid location!\n"));
				CurrentSMSMessageError = GE_INVALIDSMSLOCATION;
				break;
			case 0x07:
				dprintf(_("   Empty SMS location.\n"));
				CurrentSMSMessageError = GE_EMPTYSMSLOCATION;
				break;
			}
			break;

		case 0x0b:	/* successful delete */
			dprintf(_("Message: SMS deleted successfully.\n"));
			CurrentSMSMessageError = GE_NONE;	
			break;

		case 0x37:
			dprintf(_("Message: SMS Status Received\n"));
			dprintf(_("   The number of messages: %d\n"), MessageBuffer[10]);
			dprintf(_("   Unread messages: %d\n"), MessageBuffer[11]);
			CurrentSMSStatus->UnRead = MessageBuffer[11];
			CurrentSMSStatus->Number = MessageBuffer[10];
			CurrentSMSStatusError = GE_NONE;
			break;

		case 0x38:
			dprintf(_("Message: SMS Status error, probably not authorized by PIN\n"));
			CurrentSMSStatusError = GE_INTERNALERROR;
			break;
	  
		default:
			CurrentSMSStatusError = GE_INTERNALERROR;
			break;
      
		}
		break;


		/* Internal phone functions? */

	case 0x40:

		switch(MessageBuffer[2]) {

		case 0x7e:

			switch(MessageBuffer[3]) {
			case 0x00:
				dprintf(_("Message: Netmonitor correctly set.\n"));
				CurrentNetmonitorError = GE_NONE;
				break;
			default:
				dprintf(_("Message: Netmonitor menu %d received:\n"), MessageBuffer[3]);
				dprintf("%s\n", MessageBuffer + 4);
				strcpy(CurrentNetmonitor, MessageBuffer + 4);
				CurrentNetmonitorError = GE_NONE;
				break;
			}

			break;

		default:
			dprintf(_("Unknown message of type 0x40.\n"));
			break;
		}
		break;

		/* Mobile phone identification */

	case 0x64:
#if defined WIN32 || !defined HAVE_SNPRINTF
		sprintf(IMEI, "%s", MessageBuffer + 9);
		sprintf(Model, "%s", MessageBuffer + 25);
		sprintf(Revision, "SW%s, HW%s", MessageBuffer + 44, MessageBuffer + 39);
#else
		snprintf(IMEI, FB61_MAX_IMEI_LENGTH, "%s", MessageBuffer + 9);
		snprintf(Model, FB61_MAX_MODEL_LENGTH, "%s", MessageBuffer + 25);
		snprintf(Revision, FB61_MAX_REVISION_LENGTH, "SW%s, HW%s", MessageBuffer + 44, MessageBuffer + 39);
#endif

		dprintf(_("Message: Mobile phone identification received:\n"));
		dprintf(_("   IMEI: %s\n"), IMEI);
		dprintf(_("   Model: %s\n"), Model);
		dprintf(_("   Production Code: %s\n"), MessageBuffer + 31);
		dprintf(_("   HW: %s\n"), MessageBuffer + 39);
		dprintf(_("   Firmware: %s\n"), MessageBuffer + 44);

		/* These bytes are probably the source of the "Accessory not connected"
		   messages on the phone when trying to emulate NCDS... I hope....
		   UPDATE: of course, now we have the authentication algorithm. */

		dprintf(_("   Magic bytes: %02x %02x %02x %02x\n"), MessageBuffer[50], MessageBuffer[51], MessageBuffer[52], MessageBuffer[53]);

		MagicBytes[0] = MessageBuffer[50];
		MagicBytes[1] = MessageBuffer[51];
		MagicBytes[2] = MessageBuffer[52];
		MagicBytes[3] = MessageBuffer[53];
		CurrentMagicError = GE_NONE;
		break;

		/***** Acknowlegment of our frames. *****/
	case 0x7f:
		dprintf(_("[Received Ack of type %02x, seq: %2x]\n"), MessageBuffer[0], MessageBuffer[1]);
		AcksReceived++;
		FB61_LinkOK = true;
		break;

		/***** Power on message. *****/
	case 0xd0:
		dprintf(_("Message: The phone is powered on - seq 1.\n"));
		break;

		/***** Phone info. *****/
	case 0xd2:
#if defined WIN32 || !defined HAVE_SNPRINTF
		sprintf(Model, "%s", MessageBuffer + 21);
		sprintf(Revision, "SW%s", MessageBuffer + 5);
#else
		snprintf(Model, FB61_MAX_MODEL_LENGTH, "%s", MessageBuffer + 21);
		Model[FB61_MAX_MODEL_LENGTH-1] = 0;
		snprintf(Revision, FB61_MAX_SW_LENGTH, "SW%s", MessageBuffer + 5);
		Revision[FB61_MAX_SW_LENGTH-1] = 0;
#endif
		dprintf(_("Phone info:\n%s\n"), MessageBuffer + 4);
		break;

		/***** RLP frame received. *****/
	case 0xf1:
		FB61_RX_HandleRLPMessage();
		break;

		/***** Power on message. *****/
	case 0xf4:
		dprintf(_("Message: The phone is powered on - seq 2.\n"));
		break;

		/***** Unknown message *****/
		/* If you think that you know the exact meaning of other messages - please
		   let us know. */
	default:
		dprintf(_("Message: Unknown message.\n"));
		break;
	}
	return FB61_RX_Sync;
}

/* RX_State machine for receive handling.  Called once for each character
   received from the phone/phone. */

void FB61_RX_StateMachine(char rx_byte)
{
	static int checksum[2];
	static struct timeval time_now, time_last, time_diff;

	/* XOR the byte with the current checksum */
	checksum[BufferCount&1] ^= rx_byte;

	switch (RX_State) {
	
		/* Messages from the phone start with an 0x1e (cable) or 0x1c (IR).
		   We use this to "synchronise" with the incoming data stream. However,
		   if we see something else, we assume we have lost sync and we require
		   a gap of at least 5ms before we start looking again. This is because
		   the data part of the frame could contain a byte which looks like the
		   sync byte */

	case FB61_RX_Discarding:
		gettimeofday(&time_now, NULL);
		timersub(&time_now, &time_last, &time_diff);
		if (time_diff.tv_sec == 0 && time_diff.tv_usec < 5000) {
			time_last = time_now;  /* no gap seen, continue discarding */
			break;
		}
		/* else fall through to... */

	case FB61_RX_Sync:

		if ( CurrentConnectionType == GCT_Infrared ) {
			if (rx_byte == FB61_IR_FRAME_ID) {

				if (RX_Multiple == false) BufferCount = 0;
				else BufferCount = MessageLength - 2;
				RX_State = FB61_RX_GetDestination;
	
				/* Initialize checksums. */
				checksum[0] = FB61_IR_FRAME_ID;
				checksum[1] = 0;
			} else {
				/* Lost frame sync */
				RX_State = FB61_RX_Discarding;
				gettimeofday(&time_last, NULL);
			}
		} else { /* CurrentConnectionType == GCT_Serial */
			if (rx_byte == FB61_FRAME_ID) {

				if (RX_Multiple == false) BufferCount = 0;
				else BufferCount = MessageLength - 2;
				RX_State = FB61_RX_GetDestination;
	
				/* Initialize checksums. */
				checksum[0] = FB61_FRAME_ID;
				checksum[1] = 0;
			} else {
				/* Lost frame sync */
				RX_State = FB61_RX_Discarding;
				gettimeofday(&time_last, NULL);
			}
		}
    
		break;

	case FB61_RX_GetDestination:

		MessageDestination = rx_byte;
		RX_State = FB61_RX_GetSource;

		/* When there is a checksum error and things get out of sync we have to manage to resync */
		/* If doing a data call at the time, finding a 0x1e etc is really quite likely in the data stream */
		/* Then all sorts of horrible things happen because the packet length etc is wrong... */
		/* Therefore we test here for a destination of 0x0c and return to the top if it is not */
    
		if (rx_byte != 0x0c) {
			RX_State = FB61_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x0c, got %2x\n",rx_byte);
		}
		break;

	case FB61_RX_GetSource:
		MessageSource = rx_byte;
		RX_State = FB61_RX_GetType;

		/* Source should be 0x00 */
    
		if (rx_byte != 0x00)  {
			RX_State = FB61_RX_Sync;
			dprintf("The fbus stream is out of sync - expected 0x00, got %2x\n",rx_byte);
		}
		break;

	case FB61_RX_GetType:
		if ((RX_Multiple == true) && (MessageType != rx_byte)) {
			dprintf(_("Interrupted MultiFrame-Message - Ignoring it !!!\n"));
			dprintf(_("Please report it ...\n"));
			BufferCount = 0;
			RX_Multiple = false;
		}

		MessageType = rx_byte;
		RX_State = FB61_RX_GetUnknown;
		break;

	case FB61_RX_GetUnknown:
		MessageUnknown = rx_byte;
		RX_State = FB61_RX_GetLength;
		break;
    
	case FB61_RX_GetLength:
		if (RX_Multiple == true)
			MessageLength = MessageLength - 2 + rx_byte;
		else
			MessageLength = rx_byte;
		RX_State = FB61_RX_GetMessage;
		break;
    
	case FB61_RX_GetMessage:
		MessageBuffer[BufferCount] = rx_byte;
		BufferCount++;
		if (BufferCount > FB61_MAX_RECEIVE_LENGTH*6) {
			dprintf(_("FB61: Message buffer overun - resetting\n"));
			RX_Multiple = false;
			RX_State = FB61_RX_Sync;
			break;
		}

		/* If this is the last byte, it's the checksum. */

		if (BufferCount == MessageLength + (MessageLength % 2) + 2) {
			/* Is the checksum correct? */
			if (checksum[0] == checksum[1]) {
				/* We do not want to send ACK of ACKs and ACK of RLP frames. */

				if (MessageType != FB61_FRTYPE_ACK && MessageType != 0xf1) {
					FB61_TX_SendAck(MessageType, MessageBuffer[MessageLength-1] & 0x0f);
					if ((MessageLength > 1) && (MessageBuffer[MessageLength-2] != 0x01))
						RX_Multiple = true;
					else
						RX_Multiple = false;
				}

				FB61_RX_DispatchMessage();

			}
			else {
				dprintf(_("Bad checksum!\n"));
				RX_Multiple = false;	/* Just to be sure! */
			}

			RX_State = FB61_RX_Sync;
		}
		break;
	}
}

/* This function is used for parsing the RLP frame into fields. */

enum FB61_RX_States FB61_RX_HandleRLPMessage(void)
{
	RLP_F96Frame frame;
	int count;
	int valid = true;

	/* We do not need RLP frame parsing to be done when we do not have callback
	   specified. */
    
	if (RLP_RXCallback == NULL)
		return (FB61_RX_Sync);

	/* Anybody know the official meaning of the first two bytes?
	   Nokia 6150 sends junk frames starting D9 01, and real frames starting
	   D9 00. We'd drop the junk frames anyway because the FCS is bad, but
	   it's tidier to do it here. We still need to call the callback function
	   to give it a chance to handle timeouts and/or transmit a frame */

	if (MessageBuffer[0] == 0xd9 && MessageBuffer[1] == 0x01)
		valid = false;

	/* Nokia uses 240 bit frame size of RLP frames as per GSM 04.22
	   specification, so Header consists of 16 bits (2 bytes). See section 4.1
	   of the specification. */
    
	frame.Header[0] = MessageBuffer[2];
	frame.Header[1] = MessageBuffer[3];

	/* Next 200 bits (25 bytes) contain the Information. We store the
	   information in the Data array. */

	for (count = 0; count < 25; count ++)
		frame.Data[count] = MessageBuffer[4 + count];

	/* The last 24 bits (3 bytes) contain FCS. */

	frame.FCS[0] = MessageBuffer[29];
	frame.FCS[1] = MessageBuffer[30];
	frame.FCS[2] = MessageBuffer[31];

	/* Here we pass the frame down in the input stream. */
    
	RLP_RXCallback(valid ? &frame : NULL);

	return (FB61_RX_Sync);
}

char *FB61_PrintDevice(int Device)
{

	switch (Device) {

	case FB61_DEVICE_PHONE:
		return _("Phone");

	case FB61_DEVICE_PC:
		return _("PC");

	default:
		return _("Unknown");
		break;
	}
}

/* FB61_RX_DisplayMessage is called when a message we don't know about is
   received so that the user can see what is going back and forth, and perhaps
   shed some more light/explain another message type! */
	
void FB61_RX_DisplayMessage(void)
{
#ifdef DEBUG

	int count;
	dprintf(_("Msg Dest: %s\n"), FB61_PrintDevice(MessageDestination));
	dprintf(_("Msg Source: %s\n"), FB61_PrintDevice(MessageSource));
	dprintf(_("Msg Type: %02x\n"), MessageType);
	dprintf(_("Msg Unknown: %02x\n"), MessageUnknown);
	dprintf(_("Msg Len: %02x\nPhone: "), MessageLength);

	for (count = 0; count < MessageLength + (MessageLength % 2) + 2; count ++)
		if (isprint(MessageBuffer[count]))
			dprintf("[%02x%c]", MessageBuffer[count], MessageBuffer[count]);
		else
			dprintf("[%02x ]", MessageBuffer[count]);

	dprintf("\n");
#endif /* DEBUG */
}

/* Prepares the message header and sends it, prepends the message start byte
	   (0x1e) and other values according the value specified when called.
	   Calculates checksum and then sends the lot down the pipe... */

int FB61_TX_SendFrame(u8 message_length, u8 message_type, u8 *buffer)
{
	u8 out_buffer[FB61_MAX_TRANSMIT_LENGTH + 5];
	int count, current = 0;
	unsigned char	checksum;

	/* FIXME - we should check for the message length ... */

	/* Now construct the message header. */

	if (CurrentConnectionType == GCT_Infrared)
		out_buffer[current++] = FB61_IR_FRAME_ID; /* Start of the IR frame indicator */
	else /* CurrentConnectionType == GCT_Serial */
		out_buffer[current++] = FB61_FRAME_ID;    /* Start of the frame indicator */

	out_buffer[current++] = FB61_DEVICE_PHONE; /* Destination */
	out_buffer[current++] = FB61_DEVICE_PC;    /* Source */
	out_buffer[current++] = message_type; /* Type */
	out_buffer[current++] = 0; /* Unknown */
	out_buffer[current++] = message_length; /* Length */

	/* Copy in data if any. */	
	
	if (message_length != 0) {
		memcpy(out_buffer + current, buffer, message_length);
		current += message_length;
	}

	/* If the message length is odd we should add pad byte 0x00 */
	if (message_length % 2)
		out_buffer[current++] = 0x00;

	/* Now calculate checksums over entire message and append to message. */

	/* Odd bytes */

	checksum = 0;
	for (count = 0; count < current; count += 2)
		checksum ^= out_buffer[count];

	out_buffer[current++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < current; count += 2)
		checksum ^= out_buffer[count];

	out_buffer[current++] = checksum;

	dprintf(_("PC: "));
	for (count = 0; count < current; count++)
		dprintf("%02x:", out_buffer[count]);
	dprintf("\n");

	/* Send it out... */
  
	if (WRITEPHONE(PortFD, out_buffer, current) != current)
		return (false);

	if(message_type != 0x7f)
		MessagesSent++;

	return (true);
}

int FB61_TX_SendMessage(u16 message_length, u8 message_type, u8 *buffer)
{
	u8 seqnum, frame_buffer[FB61_MAX_CONTENT_LENGTH + 2];
	u8 nom, lml;  /* number of messages, last message len */
	int i;

	seqnum = 0x40 + RequestSequenceNumber;
	RequestSequenceNumber = (RequestSequenceNumber + 1) & 0x07;

	if (message_length > FB61_MAX_CONTENT_LENGTH) {

		nom = (message_length + FB61_MAX_CONTENT_LENGTH - 1)
			/ FB61_MAX_CONTENT_LENGTH;
		lml = message_length - ((nom - 1) * FB61_MAX_CONTENT_LENGTH);

		for (i = 0; i < nom - 1; i++) {

			memcpy(frame_buffer, buffer + (i * FB61_MAX_CONTENT_LENGTH),
			       FB61_MAX_CONTENT_LENGTH);
			frame_buffer[FB61_MAX_CONTENT_LENGTH] = nom - i;
			frame_buffer[FB61_MAX_CONTENT_LENGTH + 1] = seqnum;

			FB61_TX_SendFrame(FB61_MAX_CONTENT_LENGTH + 2, message_type,
					  frame_buffer);

			seqnum = RequestSequenceNumber;
			RequestSequenceNumber = (RequestSequenceNumber + 1) & 0x07;    
		}

		memcpy(frame_buffer, buffer + ((nom - 1) * FB61_MAX_CONTENT_LENGTH), lml);
		frame_buffer[lml] = 0x01;
		frame_buffer[lml + 1] = seqnum;
		FB61_TX_SendFrame(lml + 2, message_type, frame_buffer);

	}
	else {
		memcpy(frame_buffer, buffer, message_length);
		frame_buffer[message_length] = 0x01;
		frame_buffer[message_length + 1] = seqnum;
		FB61_TX_SendFrame(message_length + 2, message_type, frame_buffer);
	}

	return (true);
}

int FB61_TX_SendAck(u8 message_type, u8 message_seq)
{
	unsigned char request[2];
	request[0] = message_type;
	request[1] = message_seq;
	dprintf(_("[Sending Ack of type %02x, seq: %x]\n"), message_type, message_seq);
	return FB61_TX_SendFrame(2, FB61_FRTYPE_ACK, request);
}


