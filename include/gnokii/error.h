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

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Error codes.

*/

#ifndef __gsm_error_h_
#define __gsm_error_h_

#include "compat.h"

/* Define standard GSM error/return code values. These codes are also used for
   some internal functions such as SIM read/write in the model specific code. */

typedef enum {
	GE_NONE = 0,              /* 0. No error. */
	GE_DEVICEOPENFAILED,	  /* 1. Couldn't open specified serial device. */
	GE_UNKNOWNMODEL,          /* 2. Model specified isn't known/supported. */
	GE_NOLINK,                /* 3. Couldn't establish link with phone. */
	GE_TIMEOUT,               /* 4. Command timed out. */
	GE_TRYAGAIN,              /* 5. Try again. */
	GE_INVALIDSECURITYCODE,   /* 6. Invalid Security code. */
	GE_NOTIMPLEMENTED,        /* 7. Command called isn't implemented in model. */
	GE_INVALIDSMSLOCATION,    /* 8. Invalid SMS location. */
	GE_INVALIDPHBOOKLOCATION, /* 9. Invalid phonebook location. */
	GE_INVALIDMEMORYTYPE,     /* 10. Invalid type of memory. */
	GE_INVALIDSPEEDDIALLOCATION, /* 11. Invalid speed dial location. */
	GE_INVALIDCALNOTELOCATION,/* 12. Invalid calendar note location. */
	GE_INVALIDDATETIME,       /* 13. Invalid date, time or alarm specification. */
	GE_EMPTYSMSLOCATION,      /* 14. SMS location is empty. */
	GE_PHBOOKNAMETOOLONG,     /* 15. Phonebook name is too long. */
	GE_PHBOOKNUMBERTOOLONG,   /* 16. Phonebook number is too long. */
	GE_PHBOOKWRITEFAILED,     /* 17. Phonebook write failed. */
	GE_RESERVED_1,            /* 18. SMS was send correctly. */
	GE_SMSSENDFAILED,         /* 19. SMS send fail. */
	GE_SMSWAITING,            /* 20. Waiting for the next part of SMS. */
	GE_SMSTOOLONG,            /* 21. SMS message too long. */
	GE_SMSWRONGFORMAT,        /* 22. Wrong format of the SMS */
	GE_NONEWCBRECEIVED,       /* 23. Attempt to read CB when no new CB received */
	GE_INTERNALERROR,         /* 24. Problem occured internal to model specific code. */
	GE_CANTOPENFILE,          /* 25. Can't open file with bitmap/ringtone */
	GE_WRONGNUMBEROFCOLORS,   /* 26. Wrong number of colors in specified bitmap file */
	GE_WRONGCOLORS,           /* 27. Wrong colors in bitmap file */
	GE_INVALIDFILEFORMAT,     /* 28. Invalid format of file */
	GE_SUBFORMATNOTSUPPORTED, /* 29. Subformat of file not supported */
	GE_FILETOOSHORT,          /* 30. Too short file to read */
	GE_FILETOOLONG,           /* 31. Too long file to read */
	GE_INVALIDIMAGESIZE,      /* 32. Invalid size of bitmap (in file, sms etc.) */
	GE_NOTSUPPORTED,          /* 33. Function not supported by the phone */
	GE_BUSY,                  /* 34. Command is still being executed. */
	GE_USERCANCELED,          /* 35. */
	GE_UNKNOWN,               /* 36. Unknown error - well better than nothing!! */
	GE_MEMORYFULL,            /* 37. */
	GE_NOTWAITING,            /* 38. Not waiting for a response from the phone */
	GE_NOTREADY,              /* 39. */
	GE_EMPTYMEMORYLOCATION,   /* 40. */

	/* The following are here in anticipation of data call requirements. */

	GE_LINEBUSY,              /* 41. Outgoing call requested reported line busy */
	GE_NOCARRIER,             /* 42. No Carrier error during data call setup ? */

	/* The following value signals the current frame is unhandled */

	GE_UNHANDLEDFRAME,        /* 43. The current frame isn't handled by the incoming function */
	GE_UNSOLICITED            /* 44. Unsolicited message received. */
} GSM_Error;

API char *print_error(GSM_Error e);

#endif
