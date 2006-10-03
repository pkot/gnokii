/*

  $Id$

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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2000      Jan Derfinak
  Copyright (C) 2000-2004 Pawel Kot
  Copyright (C) 2001      Jan Kratochvil, Ladis Michl, Chris Kemp
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2002      Manfred Jonsson, Markus Plail

  Some parts of the gn_lock_device() function are derived from minicom
  communications package, main.c file written by Miquel van Smoorenburg.
  These parts are copyrighted under GNU GPL version 2.

  Copyright 1991-1995 Miquel van Smoorenburg.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "compat.h"

#include "misc.h"
#include "gnokii.h"

GNOKII_API gn_log_target gn_log_debug_mask = GN_LOG_T_NONE;
GNOKII_API gn_log_target gn_log_rlpdebug_mask = GN_LOG_T_NONE;
GNOKII_API gn_log_target gn_log_xdebug_mask = GN_LOG_T_NONE;
GNOKII_API void (*gn_elog_handler)(const char *fmt, va_list ap) = NULL;

GNOKII_API int gn_line_get(FILE *file, char *line, int count)
{
	char *ptr;

	if (fgets(line, count, file)) {
		ptr = line + strlen(line) - 1;

		while ((*ptr == '\n' || *ptr == '\r') && ptr >= line)
			*ptr-- = '\0';

		return strlen(line);
	} else {
		return 0;
	}
}

static gn_phone_model models[] = {
	/* model, product_name, ... */
	{"2711",  "?????", PM_SMS },		/* Dancall */
	{"2731",  "?????", PM_SMS },
	{"11",    "TFF-3", 0 },
	{"12",    "RX-2",  0 },
	{"20",    "TME-2", 0 },
	{"22",    "TME-1", 0 },
	{"30",    "TME-3", 0 },
	{"100",   "THX-9L", 0 },
	{"450",   "THF-9", 0 },
	{"505",   "NHX-8", 0 },
	{"540",   "THF-11", 0 },
	{"550",	  "THF-10", 0 },
	{"640",   "THF-13", 0 },
	{"650",   "THF-12", 0 },
	{"810",   "TFE-4R", 0 },
	{"1011",  "NHE-2", 0 },
	{"1100",  "RH-18", 0 },
	{"1100",  "RH-38", 0 },
	{"1100b", "RH-36", 0 },
	{"1220",  "NKC-1", 0 },
	{"1260",  "NKW-1", 0 },
	{"1261",  "NKW-1C", 0 },
	{"1610",  "NHE-5", 0 },
	{"1610",  "NHE-5NX", 0 },
	{"1611",  "NHE-5", 0 },
	{"1630",  "NHE-5NA", 0 },
	{"1630",  "NHE-5NX", 0 },
	{"1631",  "NHE-5SA", 0 },
	{"2010",  "NHE-3", 0 },
	{"2100",  "NAM-2", 0 },
	{"2110",  "NHE-1", 0 },
	{"2110i", "NHE-4", PM_SMS },
	{"2118",  "NHE-4", 0 },
	{"2140",  "NHK-1XA", 0 },
	{"2148",  "NHK-1", 0 },
	{"2148i", "NHK-4", 0 },
	{"2160",  "NHC-4NE/HE", 0 },
	{"2160i", "NHC-4NE/HE", 0 },
	{"2170",  "NHP-4", 0 },
	{"2180",  "NHD-4X", 0 },
	{"2190",  "NHB-3NB", 0 },
	{"2220",  "RH-40", 0 },
	{"2220",  "RH-42", 0 },
	{"2260",  "RH-39", 0 },
	{"2260",  "RH-41", 0 },
	{"2270",  "RH-3P", 0 },
	{"2275",  "RH-3DNG", 0 },
	{"2280",  "RH-17", 0 },
	{"2285",  "RH-3", 0 },
	{"2300",  "RM-4", 0 },
	{"2300b", "RM-5", 0 },
	{"3100",  "RH-19", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3100b", "RH-50", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3105",  "RH-48", 0 },
	{"3108",  "RH-6", 0 },
	{"3110",  "NHE-8", PM_SMS | PM_DTMF | PM_DATA },
	{"3110",  "0310" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-8 */
	{"3120",  "RH-19", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3120b", "RH-50", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3200",  "RH-30", 0 },
	{"3200b", "RH-31", 0 },
	{"3210",  "NSE-8", PM_NETMONITOR | PM_SPEEDDIAL | PM_SMS | PM_DTMF | PM_KEYBOARD },
	{"3210",  "NSE-9", PM_SMS | PM_DTMF | PM_KEYBOARD },
	{"3220",  "RH-37", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3285",  "NSD-1A", 0 },
	{"3300",  "NEM-1", 0 },
	{"3300",  "NEM-2", 0 },
	{"3310",  "NHM-5", PM_NETMONITOR | PM_SPEEDDIAL | PM_SMS | PM_DTMF },
	{"3310",  "NHM-5NX", PM_SMS | PM_DTMF },
	{"3315",  "NHM-5NY", PM_SMS | PM_DTMF },
	{"3320",  "NPC-1", 0 },
	{"3330",  "NHM-6", PM_NETMONITOR | PM_SPEEDDIAL | PM_SMS | PM_DTMF },
	{"3350",  "NHM-9", PM_SMS | PM_DTMF },
	{"3360",  "NPW-6", PM_SMS | PM_DTMF },
	{"3360",  "NPW-1", PM_SMS | PM_DTMF },
	{"3361",  "NPW-1", PM_SMS | PM_DTMF },
	{"3390",  "NPB-1", PM_SMS | PM_DTMF | PM_KEYBOARD },
	{"3395",  "NPB-1B", PM_SMS | PM_DTMF | PM_KEYBOARD },
	{"3410",  "NHM-2", PM_SMS | PM_DTMF | PM_NETMONITOR },
	{"3510",  "NHM-8", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3510i", "RH-9", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3520",  "RH-21", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3530",  "RH-9", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3560",  "RH-14", PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3570",  "NPD-1FW", 0 },
	{"3585",  "NPD-1AW", 0 },
	{"3585i", "NPD-4AW", 0 },
	{"3586i", "RH-44", 0 },
	{"3590",  "NPM-8", 0 },
	{"3595",  "NPM-10", 0 },
	{"3600",  "NHM-10", 0 },
	{"3610",  "NAM-1", 0 },
	{"3620",  "NHM-10X", 0 },
	{"3650",  "NHL-8", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3660",  "NHL-8X", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"3810",  "0305" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-9 */
	{"3810",  "NHE-9", PM_SMS | PM_DTMF | PM_DATA },
	{"5100",  "NPM-6", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"5100",  "NPM-6X", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"5100",  "NMP-6", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"5110",  "NSE-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5110",  "NSE-1NX", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5110i", "NSE-1NY", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5110i", "NSE-2NY", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5120",  "NSC-1", PM_KEYBOARD  },
	{"5125",  "NSC-1", PM_KEYBOARD },
	{"5130",  "NSK-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5140",  "NPL-5", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"5140i", "RM-104", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"5160",  "NSW-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5170",  "NSD-1F", 0 },
	{"5180",  "NSD-1G", 0 },
	{"5185",  "NSD-1A", 0 },
	{"5190",  "NSB-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5210",  "NSM-5", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"5510",  "NPM-5", 0 },
	{"6010",  "NPM-10", 0 },
	{"6010",  "NPM-10X", 0 },
	{"6011",  "RTE-2RH", 0 },
	{"6015i", "RH-55", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6020",  "RM-30", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6021",  "RM-94", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6050",  "NME-1", 0 },
	{"6080",  "NME-2", 0 },
	{"6081",  "NME-2A", 0 },
	{"6081",  "NME-2E", 0 },
	{"6090",  "NME-3", 0 },
	{"6100",  "NPL-2", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6108",  "RH-4", 0 },
	{"6110",  "NSE-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6120",  "NSC-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6130",  "NSK-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6136",  "RM-199", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6138",  "NSK-3", 0 },
	{"6150",  "NSM-1", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"616x",  "NSW-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6160",  "NSW-3AX", 0 },
	{"6161",  "NSW-3ND", 0 },
	{"6162",  "NSW-3AF", 0 },
	{"6170",  "RM47_-48", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6185",  "NSD-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6185i", "NSD-3AW", 0 },
	{"6188",  "NSD-3AX", 0 },
	{"6190",  "NSB-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6200",  "NPL-3", 0 },
	{"6210",  "NPE-3", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6220",  "RH-20", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6225",  "RH-27", 0 },
	{"6230",  "RH-12_28_-", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6230",  "RH12_28_-", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6230i", "RM72_73_-", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6250",  "NHM-3", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6280",  "RH-78", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6310",  "NPE-4", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS | PM_KEYBOARD | PM_DTMF },
	{"6310i", "NPL-1", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS | PM_KEYBOARD | PM_DTMF },
	{"6340",  "NPM-2", 0 },
	{"6340i", "RH-13", 0 },
	{"6360",  "NPW-2", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6370",  "NHP-2FX", 0 },
	{"6385",  "NHP-2AX", 0 },
	{"6500",  "NHM-7", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6510",  "NPM-9", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6560",  "RH-25", 0 },
	{"6585",  "RH-34", 0 },
	{"6590",  "NSM-9", 0 },
	{"6590i", "NSM-9", 0 },
	{"6600",  "NHL-10", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6610",  "NHL-4U", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6610i", "RM-37", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6650",  "NHM-1", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6680",  "RM-36", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6800",  "NHL-6", 0 },
	{"6800",  "NSB-9", 0 },
	{"6810",  "RM-2", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6820",  "NHL-9", 0 },
	{"6820b", "RH-26", 0 },
	{"7110",  "NSE-5", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7160",  "NSW-5", 0 },
	{"7190",  "NSB-5", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7210",  "NHL-4", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7250",  "NHL-4J", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7250i", "NHL-4JX", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7600",  "NMM-3", PM_CALLERGROUP | PM_EXTPBK | PM_SMS | PM_FOLDERS | PM_SPEEDDIAL | PM_CALENDAR | PM_DTMF },
	{"7650",  "NHL-2NA", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"8110",  "0423" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-6BX */
	{"8110",  "2501", PM_SMS | PM_DTMF | PM_DATA },
	{"8110",  "NHE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8110",  "NHE-6BX", PM_SMS | PM_DTMF | PM_DATA },
	{"8110i", "0423",  PM_SMS | PM_DTMF | PM_DATA }, /* Guess for NHE-6 */
	{"8110i", "NHE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8110i", "NHE-6BM", PM_SMS | PM_DTMF | PM_DATA },
	{"8146",  "NHK-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8148",  "NHK-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8148i", "NHK-6V", PM_SMS | PM_DTMF | PM_DATA },
	{"8210",  "NSM-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"8250",  "NSM-3D", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"8260",  "NSW-4", 0 },
	{"8265",  "NPW-3", 0 },
	{"8270",  "NSD-5FX", 0 },
	{"8280",  "RH-10", 0 },
	{"8290",  "NSB-7", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL },
	{"8310",  "NHM-7", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"8390",  "NSB-8", 0 },
	{"8810",  "NSE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8850",  "NSM-2", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_SMS | PM_DTMF | PM_DATA | PM_KEYBOARD},
	{"8855",  "NSM-4", 0 },
	{"8860",  "NSW-6", 0 },
	{"8890",  "NSB-6", 0 },
	{"8910",  "NHM-4NX", 0 },
	{"8910i", "NHM-4NX", 0 },
	{"9000",  "RAE-1", 0 },
	{"9000i", "RAE-4", 0 },
	{"9110",  "RAE-2", 0 },
	{"9110i", "RAE-2i", 0 },
	{"9210",  "RAE-2", 0 },
	{"9210c", "RAE-3", 0 },
	{"9210i", "RAE-5N", 0 },
	{"9290",  "RAE-3N", 0 },
	{"RPM-1", "RPM-1", PM_CALLERGROUP | PM_SMS | PM_DTMF | PM_DATA },
	{"Card Phone 1.0", "RPE-1", PM_CALLERGROUP | PM_SMS | PM_DTMF | PM_DATA },
	{"Card Phone 2.0", "RPM-1", PM_CALLERGROUP | PM_SMS | PM_DTMF | PM_DATA },
	{"C110 Wireless LAN Card", "DTN-10", 0 },
	{"C111 Wireless LAN Card", "DTN-11", 0 },
	{"D211",  "DTE-1", 0},
	{"N-Gage", "NEM-4", 0},
	{"RinGo", "NHX-7", 0},
	{"sx1",  "SX1", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{NULL,    NULL, 0 } /* keep last one as NULL */
};

#define MODELS_NUM_ENTRIES (sizeof(models)/sizeof(models[0]))

/* this should be in libfunctions - will move it there with next version of libgnokii */
GNOKII_API const char *gn_lib_get_supported_phone_model( const int num )
{
	if (num < 0 || num >= MODELS_NUM_ENTRIES)
		return NULL;
	return models[num].model;
}


GNOKII_API gn_phone_model *gn_phone_model_get(const char *product_name)
{
	int i = 0;

	while (models[i].product_name != NULL) {
		if (strcmp(product_name, models[i].product_name) == 0) {
			dprintf("Found model \"%s\"\n", product_name);
			return (&models[i]);
		}
		i++;
	}

	return (&models[MODELS_NUM_ENTRIES-1]); /* NULL entry */
}

GNOKII_API const char *gn_model_get(const char *product_name)
{
	return (gn_phone_model_get(product_name)->model);
}

static void log_printf(gn_log_target mask, const char *fmt, va_list ap)
{
	if (mask & GN_LOG_T_STDERR) {
		vfprintf(stderr, fmt, ap);
		fflush(stderr);
	}
}

GNOKII_API void gn_log_debug(const char *fmt, ...)
{
#ifdef DEBUG
	va_list ap;

	va_start(ap, fmt);

	log_printf(gn_log_debug_mask, fmt, ap);

	va_end(ap);
#endif
}

GNOKII_API void gn_log_rlpdebug(const char *fmt, ...)
{
#ifdef RLP_DEBUG
	va_list ap;

	va_start(ap, fmt);

	log_printf(gn_log_rlpdebug_mask, fmt, ap);

	va_end(ap);
#endif
}

GNOKII_API void gn_log_xdebug(const char *fmt, ...)
{
#ifdef XDEBUG
	va_list ap;

	va_start(ap, fmt);

	log_printf(gn_log_xdebug_mask, fmt, ap);

	va_end(ap);
#endif
}

GNOKII_API void gn_elog_write(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	log_printf(gn_log_debug_mask, fmt, ap);

	if (gn_elog_handler) {
		gn_elog_handler(fmt, ap);
	} else {
#ifndef	DEBUG
		if (!(gn_log_debug_mask & GN_LOG_T_STDERR))
			log_printf(GN_LOG_T_STDERR, fmt, ap);
#endif
	}

	va_end(ap);
}

#define BUFFER_MAX_LENGTH 128
#if defined (__svr4__)
#  define lock_path "/var/run/LCK.."
#else
#  define lock_path "/var/lock/LCK.."
#endif

/* Lock the device. Return allocated string with a lock name */
GNOKII_API char *gn_device_lock(const char* port)
{
#ifndef WIN32
	char *lock_file = NULL;
	char buffer[BUFFER_MAX_LENGTH];
	const char *aux = strrchr(port, '/');
	int fd, len;

	if (!port) {
		fprintf(stderr, _("Cannot lock NULL device.\n"));
		return NULL;
	}

	/* Remove leading '/' */
	if (aux)
		aux++;
	else
		aux = port;

	len = strlen(aux) + strlen(lock_path);

	memset(buffer, 0, sizeof(buffer));
	lock_file = calloc(len + 1, 1);
	if (!lock_file) {
		fprintf(stderr, _("Out of memory error while locking device.\n"));
		return NULL;
	}
	/* I think we don't need to use strncpy, as we should have enough
	 * buffer due to strlen results
	 */
	strcpy(lock_file, lock_path);
	strcat(lock_file, aux);

	/* Check for the stale lockfile.
	 * The code taken from minicom by Miquel van Smoorenburg */
	if ((fd = open(lock_file, O_RDONLY)) >= 0) {
		char buf[BUFFER_MAX_LENGTH];
		int pid, n = 0;

		n = read(fd, buf, sizeof(buf) - 1);
		close(fd);
		if (n > 0) {
			pid = -1;
			if (n == 4)
				/* Kermit-style lockfile. */
				pid = *(int *)buf;
			else {
				/* Ascii lockfile. */
				buf[n] = 0;
				sscanf(buf, "%d", &pid);
			}
			if (pid > 0 && kill((pid_t)pid, 0) < 0 && errno == ESRCH) {
				fprintf(stderr, _("Lockfile %s is stale. Overriding it..\n"), lock_file);
				sleep(1);
				if (unlink(lock_file) == -1) {
					fprintf(stderr, _("Overriding failed, please check the permissions.\n"));
					fprintf(stderr, _("Cannot lock device.\n"));
					goto failed;
				}
			} else {
				fprintf(stderr, _("Device already locked.\n"));
				goto failed;
			}
		}
		/* This must not happen. Because we could open the file    */
		/* no wrong permissions are set. Only reason could be      */
		/* flock/lockf or an empty lockfile due to a broken binary */
		/* which is more likely (like gnokii 0.4.0pre11 ;-).       */
		if (n == 0) {
			fprintf(stderr, _("Unable to read lockfile %s.\n"), lock_file);
			fprintf(stderr, _("Please check for reason and remove the lockfile by hand.\n"));
			fprintf(stderr, _("Cannot lock device.\n"));
			goto failed;
		}
	}

	/* Try to create a new file, with 0644 mode */
	fd = open(lock_file, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (fd == -1) {
		if (errno == EEXIST)
			fprintf(stderr, _("Device seems to be locked by unknown process.\n"));
		else if (errno == EACCES)
			fprintf(stderr, _("Please check permission on lock directory.\n"));
		else if (errno == ENOENT)
			fprintf(stderr, _("Cannot create lockfile %s. Please check for existence of the path."), lock_file);
		goto failed;
	}
	sprintf(buffer, "%10ld gnokii\n", (long)getpid());
	/* Probably we should add some error checking in here */
	write(fd, buffer, strlen(buffer));
	close(fd);
	return lock_file;
failed:
	free(lock_file);
	return NULL;
#else
	return 1;
#endif /* WIN32 */
}

/* Removes lock and frees memory */
GNOKII_API bool gn_device_unlock(char *lock_file)
{
#ifndef WIN32
	int err;

	if (!lock_file) {
		fprintf(stderr, _("Cannot unlock device\n"));
		return false;
	}
	err = unlink(lock_file);
	free(lock_file);
	return (err + 1);
#else
	return true;
#endif /* WIN32 */
}

/*
 * Splits string into NULL-terminated string array
 */
char **gnokii_strsplit(const char *string, const char *delimiter, int tokens)
{
	const char *left = string;
	char *tmp, *str;
	int count = 0;
	char **strings;

	if (!string || !delimiter || !tokens)
		return NULL;

	strings = calloc(tokens + 1, sizeof(char *));

	while ((tmp = strstr(left, delimiter)) != NULL && (count < tokens)) {
		str = malloc((tmp - left) + 1);
		memset(str, 0, (tmp - left) + 1);
		memcpy(str, left, tmp - left);
		strings[count] = str;
		left = tmp + strlen(delimiter);
		count++;
	}

	strings[count] = strdup(left);

	for (count = 0; count < tokens; count++) {
		dprintf("strings[%d] = %s\n", count, strings[count]);
	}

	return strings;
}

/*
 * frees NULL-terminated array of strings
 */
void gnokii_strfreev(char **str_array)
{
	char **tmp = str_array;

	if (!str_array)
		return;

	while (*tmp) {
		free(*tmp);
		tmp++;
	}
	free(str_array);
}

/*
 * check if the timestamp in dt has valid date and time
 */
GNOKII_API int gn_timestamp_isvalid(gn_timestamp dt)
{
#define BETWEEN(a, x, y)	((a >= x) && (a <= y))
	int daynum;

	/* assume that year is OK */
	switch (dt.month) {
	case 2:
		if (((dt.year % 4) == 0) &&
		    (((dt.year % 100) != 0) ||
		     ((dt.year % 1000) == 0)))
			daynum = 29;
		else
			daynum = 28;
		break;
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		daynum = 31;
		break;
	default:
		daynum = 30;
		break;
	}
	return (BETWEEN(dt.month, 1, 12) && BETWEEN(dt.day, 1, daynum) &&
		BETWEEN(dt.hour, 0, 24) && BETWEEN(dt.minute, 0, 59) &&
		BETWEEN(dt.second, 0, 59));
}
