/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Jul  4 23:26:47 CEST 2000
  Modified by Pawe³ Kot <pkot@linuxnews.pl>

*/

#include <string.h>
#include "misc.h"

int GetLine(FILE *File, char *Line, int count)
{
	char *ptr;

	if (fgets(Line, count, File)) {
		ptr = Line + strlen(Line) - 1;

		while ( (*ptr == '\n' || *ptr == '\r') && ptr>=Line)
			*ptr--='\0';

		return strlen(Line);
	}
	else
		return 0;
}

/* jano: Functions for detecting phone capabiltes. For explanation how  */
/* to use these functions look into xgnokii_lowlevel.c                  */

static PhoneModel models[] = {
	{NULL,    "", 0 },
	{"1611",  "NHE-5", 0 },
	{"2110i", "NHE-4", 0 },
	{"2148i", "NHK-4", 0 },
	{"8810",  "NSE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8110i", "NHE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"3110",  "0310" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-8 */
	{"3210",  "NSE-8", PM_SMS | PM_DTMF },
	{"3810",  "NHE-9", PM_SMS | PM_DTMF | PM_DATA },
	{"5110",  "NSE-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"5130",  "NSK-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"5160",  "NSW-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"5190",  "NSB-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6110",  "NSE-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6120",  "NSC-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6130",  "NSK-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6150",  "NSM-1", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"616x",  "NSW-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6185",  "NSD-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6190",  "NSB-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"7110",  "NSE-5", PM_DATA },
	{"9000i", "RAE-4", 0 },
	{"9110",  "RAE-2", 0 },
	{"550",	  "THF-10", 0 },
	{"540",   "THF-11", 0 },
	{"650",   "THF-12", 0 },
	{"640",   "THF-13", 0 },
	{NULL,    NULL, 0}
};

PhoneModel *GetPhoneModel (const char *num)
{
	register int i = 0;

	while (models[i].number != NULL) {
		if (strcmp (num, models[i].number) == 0)
			return (&models[i]);
		i++;
	}

	return (&models[0]);
}

char *GetModel (const char *num)
{
	return GetPhoneModel(num)->model;
}

int CallerGroupSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_CALLERGROUP;
}

int CalendarSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_CALENDAR;
}

int NetmonitorSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_NETMONITOR;
}

int KeyboardSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_KEYBOARD;
}

int SMSSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_SMS;
}

int DTMFSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_DTMF;
}

int DataSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_DATA;
}

int SpeedDialSupported (const char *num)
{
	return GetPhoneModel(num)->flags & PM_SPEEDDIAL;
}
