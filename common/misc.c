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

static PhoneModel models[] = {
	{NULL,    "", 0 },
	{"1611",  "NHE-5", 0 },
	{"2110i", "NHE-4", PM_SMS },
	{"2148i", "NHK-4", 0 },
	{"8810",  "NSE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8110i", "0423",  PM_SMS | PM_DTMF | PM_DATA }, /* Guess for NHE-6 */
	{"8110",  "0423" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-6BX */
	{"3110",  "0310" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-8 */
	{"3210",  "NSE-8", PM_SMS | PM_DTMF },
	{"3210",  "NSE-9", PM_SMS | PM_DTMF },
	{"3310",  "NHM-5", PM_SMS | PM_DTMF },
	{"3810",  "0305" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-9 */
	{"5110",  "NSE-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"5130",  "NSK-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"5160",  "NSW-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"5190",  "NSB-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"6110",  "NSE-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6120",  "NSC-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6130",  "NSK-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6150",  "NSM-1", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"616x",  "NSW-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6185",  "NSD-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6190",  "NSB-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"7110",  "NSE-5", PM_CALLERGROUP | PM_SPEEDDIAL | PM_EXTPBK },
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
		if (strcmp (num, models[i].number) == 0) {
			dprintf(_("Found model\n"));
			return (&models[i]);
		}
		else {
			dprintf(_("comparing %s and %s\n"), num, models[i].number);
		}
		i++;
	}

	return (&models[0]);
}

inline char *GetModel (const char *num)
{
	return (GetPhoneModel(num)->model);
}
