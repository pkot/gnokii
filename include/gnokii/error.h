/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Error codes.

  $Log$
  Revision 1.4  2002-01-16 12:39:21  pkot
  Dump unhandled frames

  Revision 1.3  2002/01/10 14:23:38  pkot
  Allow to read all phonebook entries easily (Simon Huggins)

  Revision 1.2  2001/11/08 16:34:20  pkot
  Updates to work with new libsms

  Revision 1.1  2001/10/24 22:33:00  pkot
  Moved error codes to a separate files


*/

#ifndef __gsm_error_h_
#define __gsm_error_h_

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
	GE_SMSSENDOK,             /* 18. SMS was send correctly. */
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
} GSM_Error;

extern char *print_error(GSM_Error e);

#endif
