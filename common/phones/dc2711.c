/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright 2001 Pavel Machek <pavel@ucw.cz>

  This file provides functions specific to the dancall 2711.
  See README for more details on supported mobile phones.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"
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


GSM_Phone phone_dancall_2711 = {
	D2711_IncomingFunctions,
	PGEN_IncomingDefault,
	INFO
};

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


#if 0
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
#endif

/* ----------------------------------------------------------------------------------- */

static GSM_Error Initialise(GSM_Statemachine *state)
{
	/* char model[10]; */

	memcpy(&(state->Phone), &phone_dancall_2711, sizeof(GSM_Phone));

	fprintf(stderr, "Initializing dancall...\n");
	switch (state->Link.ConnectionType) {
	case GCT_Serial:
		CBUS_Initialise(state);
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

#if 0
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
#endif

/* Here we initialise model specific functions called by 'gnokii'. */
/* This too needs fixing .. perhaps pass the link a 'request' of certain */
/* type and the link then searches the phone functions.... */

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
