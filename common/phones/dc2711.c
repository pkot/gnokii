/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright 2001 Pavel Machek <pavel@ucw.cz>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to the dancall 2711.
  See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "phones/generic.h"
#include "links/cbus.h"

/* Mobile phone information */

static GSM_Link link;
static GSM_IncomingFunctionType D2711_IncomingFunctions[];

#define INFO \
{ \
	"dancall|2711|2713",	/* Supported models */ \
	7,			/* Max RF Level */ \
	0,			/* Min RF Level */ \
	GRF_Percentage,		/* RF level units */ \
	7,			/* Max Battery Level */ \
	0,			/* Min Battery Level */ \
	GBU_Percentage,		/* Battery level units */ \
	0,			/* Have date/time support */ \
	0,			/* Alarm supports time only */ \
	1,			/* Alarms available - FIXME */ \
	60, 96,			/* Startup logo size */ \
	21, 78,			/* Op logo size */ \
	14, 72			/* Caller logo size */ \
}

GSM_Information D2711_Information = INFO;


static GSM_Phone phone = {
	D2711_IncomingFunctions,
	PGEN_IncomingDefault,
	INFO
};

/* LinkOK is always true for now... */
bool D2711_LinkOK = true;
char reply_buf[10240];

static void Terminate()
{
	return;
};

/* ----------------------------------------------------------------------------------- */

static GSM_Error Reply(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	printf("[ack]");
	return GE_NONE;
}

extern int seen_okay;
extern char reply_buf[];

static char *Request(char *c)
{
	link.SendMessage(strlen(c), 0, c);
	while(!seen_okay)
		link.Loop(NULL);
	return reply_buf;
}

/* ----------------------------------- SMS stuff ------------------------------------- */


GSM_Error ATGSM_GetSMSMessage(GSM_SMSMessage * m)
{
	GSM_Error test = GE_NONE;
	char writecmd[128];
	char *s, *t;

	// Set memory
	m->MemoryType = GMT_SM;     // Type of memory message is stored in.
	// Send get request
	sprintf(writecmd, "AT+CMGR=%d\r", m->Location);
	s = Request(writecmd);
	if (!s)
		return GE_BUSY;
	t = strchr(s, '\n')+1;
	if (!strncmp(s, "+CMS ERROR: 321", 15))
		return GE_EMPTYSMSLOCATION;
	if (!strncmp(s, "+CMS ERROR: ", 11))
		return GE_INTERNALERROR;

	printf("Got %s [%s] as reply for cmgr\n", s, t);
	{
		m->Time.Year=0;
		m->Time.Month=0;
		m->Time.Day=0;
		m->Time.Hour=0;
		m->Time.Minute=0;
		m->Time.Second=0;
		m->Time.Timezone=0;
	}
	memset(m->UserData[0].u.Text, 0, 161);
	strncpy(m->UserData[0].u.Text, (void *) t, 161);
	m->Length = strlen(t);
	strcpy(m->MessageCenter.Number, "(unknown)");
	strcpy(m->MessageCenter.Name, "(unknown)");
	m->MessageCenter.No = 0;
	strcpy(m->Sender, "(sender unknown)");
	m->UDHType = GSM_NoUDH;

	return test;
}


GSM_Error ATGSM_DeleteSMSMessage(GSM_SMSMessage * message)
{
	char writecmd[128];

	sprintf(writecmd, "AT+CMGD=%d\r", message->Location);

	Request(writecmd);
	return GE_NONE;
}


GSM_Error ATGSM_SendSMSMessage(GSM_SMSMessage * SMS, int size)
{
	return (GE_NOTIMPLEMENTED);
}

/* ----------------------------------------------------------------------------------- */

static GSM_Error Initialise(char *port_device, char *initlength,
			   GSM_ConnectionType connection,
			   void (*rlp_callback)(RLP_F96Frame *frame))
{
	/* char model[10]; */

	strncpy(link.PortDevice, port_device, 20);
	link.InitLength = atoi(initlength);
	link.ConnectionType = connection;

	fprintf(stderr, "Initializing dancall...\n");
	switch (connection) {
	case GCT_Serial:
		CBUS_Initialise(&link, &phone);
		break;
	default:
		return GE_NOTSUPPORTED;
		break;
	}
	sendat("AT+CPMS=\"SM\",\"SM\"\r");
	printf("Waiting spurious...\n");
	if (strncmp(reply_buf, "+CPMS", 5)) {
		while (strncmp(reply_buf, "+CPMS", 5))
			link.Loop(NULL);
		seen_okay = 0;
		printf("Waiting OKAY\n");
		while (!seen_okay)
			link.Loop(NULL);
		printf("Link UP\n");
	}

	return GE_NONE;
}

static GSM_Error
GetSMSStatus(GSM_SMSStatus *Status)
{
	int i,j,k,l;
	char *message = Request("AT+CPMS=\"SM\",\"SM\"\r");
	if (sscanf(message, "+CPMS: \"SM\",%d,%d,\"SM\",%d,%d", &i, &j, &k, &l)!=4)
		return GE_BUSY;
	Status->UnRead = i;
	Status->Number = k;
	return GE_NONE;
}

/* Here we initialise model specific functions called by 'gnokii'. */
/* This too needs fixing .. perhaps pass the link a 'request' of certain */
/* type and the link then searches the phone functions.... */

GSM_Functions D2711_Functions = {
	Initialise,
	Terminate,
	UNIMPLEMENTED, /* GetMemoryLocation */
	UNIMPLEMENTED, /* WritePhonebookLocation */
	UNIMPLEMENTED, /* GetSpeedDial */
	UNIMPLEMENTED, /* SetSpeedDial */
	UNIMPLEMENTED, /* GetMemoryStatus */
	GetSMSStatus, /* GetSMSStatus */
	UNIMPLEMENTED, /* GetSMSCentre */
	UNIMPLEMENTED, /* SetSMSCentre */
	ATGSM_GetSMSMessage, /* GetSMSMessage */
	ATGSM_DeleteSMSMessage, /* DeleteSMSMessage */
	ATGSM_SendSMSMessage, /* SendSMSMessage */
	UNIMPLEMENTED, /* SaveSMSMessage */
	UNIMPLEMENTED, /* GetRFLevel */
	UNIMPLEMENTED, /* GetBatteryLevel */
	UNIMPLEMENTED, /* GetPowerSource */
	UNIMPLEMENTED, /* GetDisplayStatus */
	UNIMPLEMENTED, /* EnterSecurityCode */
	UNIMPLEMENTED, /* GetSecurityCodeStatus */
	UNIMPLEMENTED,        /* GetIMEI */
	UNIMPLEMENTED,    /* GetRevision */
	UNIMPLEMENTED,       /* GetModel */
	UNIMPLEMENTED, /* GetManufacturer */
	UNIMPLEMENTED, /* GetDateTime */
	UNIMPLEMENTED, /* SetDateTime */
	UNIMPLEMENTED, /* GetAlarm */
	UNIMPLEMENTED, /* SetAlarm */
	UNIMPLEMENTED, /* DialVoice */
	UNIMPLEMENTED, /* DialData */
	UNIMPLEMENTED, /* GetIncomingCallNr */
	UNIMPLEMENTED, /* GetNetworkInfo */
	UNIMPLEMENTED, /* GetCalendarNote */
	UNIMPLEMENTED, /* WriteCalendarNote */
	UNIMPLEMENTED, /* DeleteCalendarNote */
	UNIMPLEMENTED, /* NetMonitor */
	UNIMPLEMENTED, /* SendDTMF */
	UNIMPLEMENTED, /* GetBitmap */
	UNIMPLEMENTED, /* SetBitmap */
	UNIMPLEMENTED, /* SetRingtone */
	UNIMPLEMENTED, /* SendRingtone */
	UNIMPLEMENTED, /* Reset */
	UNIMPLEMENTED, /* GetProfile */
	UNIMPLEMENTED, /* SetProfile */
	UNIMPLEMENTED, /* SendRLPFrame */
	UNIMPLEMENTED, /* CancelCall */
	UNIMPLEMENTED, /* EnableDisplayOutput */
	UNIMPLEMENTED, /* DisableDisplayOutput */
	UNIMPLEMENTED, /* EnableCellBroadcast */
	UNIMPLEMENTED, /* DisableCellBroadcast */
	UNIMPLEMENTED  /* ReadCellBroadcast */
};

static GSM_IncomingFunctionType D2711_IncomingFunctions[] = {
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, Reply },
	{ 0, NULL }
};
