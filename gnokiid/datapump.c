/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	datapump.c - provides routines to handle processing of data
	when connected in fax or data mode.  Converts data from/to
	GSM phone to virtual modem interface.

*/

#define		__datapump_c


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "misc.h"
#include "gsm-common.h"
#include "at-emulator.h"
#include "virtmodem.h"
#include "datapump.h"

	/* Global variables */

	/* Local variables */

