/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id$
  
  $Log$
  Revision 1.21  2001-12-28 16:00:31  pkot
  Misc cleanup. Some usefull functions

  Revision 1.20  2001/11/26 18:06:08  pkot
  Checking for *printf functions, N_(x) for localization, generic ARRAY_LEN, SAFE_STRNCPY, G_GNUC_PRINTF (Jan Kratochvil)

  Revision 1.19  2001/11/22 17:56:53  pkot
  smslib update. sms sending

  Revision 1.18  2001/09/09 21:45:49  machek
  Cleanups from Ladislav Michl <ladis@psi.cz>:

  *) do *not* internationalize debug messages

  *) some whitespace fixes, do not use //

  *) break is unneccessary after return

  Revision 1.17  2001/08/09 12:34:34  pkot
  3330 and 6250 support - I have no idea if it does work (mygnokii)

  Revision 1.16  2001/03/21 23:36:04  chris
  Added the statemachine
  This will break gnokii --identify and --monitor except for 6210/7110

  Revision 1.15  2001/03/06 10:38:52  machek
  Dancall models added to the global list.

  Revision 1.14  2001/02/06 13:55:23  pkot
  Enabled authentication in 51xx models

  Revision 1.13  2001/02/02 08:09:56  ja
  New dialogs for 6210/7110 in xgnokii. Fixed the smsd for new capabilty code.


*/

#include <string.h>
#include <stdlib.h>
#include "misc.h"

int GetLine(FILE *File, char *Line, int count)
{
	char *ptr;

	if (fgets(Line, count, File)) {
		ptr = Line + strlen(Line) - 1;

		while ( (*ptr == '\n' || *ptr == '\r') && ptr >= Line)
			*ptr-- = '\0';

		return strlen(Line);
	}
	else
		return 0;
}

static PhoneModel models[] = {
	{NULL,    "", 0 },
	{"2711",  "?????", PM_SMS },		/* Dancall */
	{"2731",  "?????", PM_SMS },
	{"1611",  "NHE-5", 0 },
	{"2110i", "NHE-4", PM_SMS },
	{"2148i", "NHK-4", 0 },
	{"3110",  "0310" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-8 */
	{"3210",  "NSE-8", PM_SMS | PM_DTMF },
	{"3210",  "NSE-9", PM_SMS | PM_DTMF },
	{"3310",  "NHM-5", PM_SMS | PM_DTMF },
	{"3330",  "NHM-6", PM_SMS | PM_DTMF },
	{"3810",  "0305" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-9 */
	{"5110",  "NSE-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5130",  "NSK-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5160",  "NSW-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5190",  "NSB-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6110",  "NSE-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6120",  "NSC-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6130",  "NSK-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6150",  "NSM-1", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"616x",  "NSW-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6185",  "NSD-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6190",  "NSB-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6210",  "NPE-3", PM_CALLERGROUP | PM_CALENDAR | PM_EXTPBK | PM_SMS},
	{"6250",  "NHM-3", PM_CALLERGROUP | PM_CALENDAR | PM_EXTPBK },
	{"7110",  "NSE-5", PM_CALLERGROUP | PM_SPEEDDIAL | PM_EXTPBK },
	{"8210",  "NSM-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"8810",  "NSE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8110i", "0423",  PM_SMS | PM_DTMF | PM_DATA }, /* Guess for NHE-6 */
	{"8110",  "0423" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-6BX */
	{"9000i", "RAE-4", 0 },
	{"9110",  "RAE-2", 0 },
	{"550",	  "THF-10", 0 },
	{"540",   "THF-11", 0 },
	{"650",   "THF-12", 0 },
	{"640",   "THF-13", 0 },
	{NULL,    NULL, 0 }
};

PhoneModel *GetPhoneModel (const char *num)
{
	register int i = 0;

	while (models[i].number != NULL) {
		if (strcmp (num, models[i].number) == 0) {
			dprintf("Found model\n");
			return (&models[i]);
		}
		else {
			dprintf("comparing %s and %s\n", num, models[i].number);
		}
		i++;
	}

	return (&models[0]);
}

inline char *GetModel (const char *num)
{
	return (GetPhoneModel(num)->model);
}

#ifndef HAVE_VASPRINTF
/* Adapted from snprintf(3) man page: */
int gvasprintf(char **destp, const char *fmt, va_list ap)
{
	int n, size = 0x100;
	char *p, *pnew;

	if (!(p = malloc(size))) {
		*destp = NULL;
		return(-1);
	}
	for (;;) {
		/* Try to print in the allocated space. */
		n = gvsprintf(p, size, fmt, ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size) {
			*destp = p;
			return(n);
		}
		/* Else try again with more space. */
		if (n > -1)	/* glibc 2.1 */
			size = n + 1;	/* precisely what is needed */
		else		/* glibc 2.0 */
			size *= 2;	/* twice the old size */
		if (!(pnew = realloc(p, size))) {
			free(p);
			*destp = NULL;
			return(-1);
		}
		p = pnew;
	}
}
#endif

#ifndef HAVE_ASPRINTF
int gasprintf(char **destp, const char *fmt,...)
{
	va_list ap;
	int r;

	va_start(ap,fmt);
	r = gvasprintf(destp, fmt, ap);
	va_end(ap);
	return(r);
}
#endif
