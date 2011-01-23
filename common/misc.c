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
#include "gnokii-internal.h"

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
	{"2711",  "?????",	PM_OLD_DEFAULT },		/* Dancall */
	{"2731",  "?????",	PM_OLD_DEFAULT },
	{"11",    "TFF-3",	PM_OLD_DEFAULT },
	{"12",    "RX-2",	PM_OLD_DEFAULT },
	{"20",    "TME-2",	PM_OLD_DEFAULT },
	{"22",    "TME-1",	PM_OLD_DEFAULT },
	{"30",    "TME-3",	PM_OLD_DEFAULT },
	{"100",   "THX-9L",	PM_OLD_DEFAULT },
	{"450",   "THF-9",	PM_OLD_DEFAULT },
	{"505",   "NHX-8",	PM_OLD_DEFAULT },
	{"540",   "THF-11",	PM_OLD_DEFAULT },
	{"550",	  "THF-10",	PM_OLD_DEFAULT },
	{"640",   "THF-13",	PM_OLD_DEFAULT },
	{"650",   "THF-12",	PM_OLD_DEFAULT },
	{"810",   "TFE-4R",	PM_OLD_DEFAULT },
	{"1011",  "NHE-2",	PM_OLD_DEFAULT },
	{"1100",  "RH-18",	PM_OLD_DEFAULT },
	{"1100",  "RH-38",	PM_OLD_DEFAULT },
	{"1100b", "RH-36",	PM_OLD_DEFAULT },
	{"1101",  "RH-75",	PM_OLD_DEFAULT }, /* Yet another 1100 variant, untested */
	{"1110",  "RH-93",	PM_DEFAULT },     /* Series 30, phone supports a very limited command set */
	{"1209",  "RH-105",	PM_OLD_DEFAULT }, /* Series 30, phone supports a very limited command set */
	{"1220",  "NKC-1",	PM_OLD_DEFAULT },
	{"1260",  "NKW-1",	PM_OLD_DEFAULT },
	{"1261",  "NKW-1C",	PM_OLD_DEFAULT },
	{"1610",  "NHE-5",	PM_OLD_DEFAULT },
	{"1610",  "NHE-5NX",	PM_OLD_DEFAULT },
	{"1611",  "NHE-5",	PM_OLD_DEFAULT },
	{"1630",  "NHE-5NA",	PM_OLD_DEFAULT },
	{"1630",  "NHE-5NX",	PM_OLD_DEFAULT },
	{"1631",  "NHE-5SA",	PM_OLD_DEFAULT },
	{"2010",  "NHE-3",	PM_OLD_DEFAULT },
	{"2100",  "NAM-2",	PM_OLD_DEFAULT },
	{"2110",  "NHE-1",	PM_OLD_DEFAULT },
	{"2110i", "NHE-4",	PM_OLD_DEFAULT },
	{"2118",  "NHE-4",	PM_OLD_DEFAULT },
	{"2140",  "NHK-1XA",	PM_OLD_DEFAULT },
	{"2148",  "NHK-1",	PM_OLD_DEFAULT },
	{"2148i", "NHK-4",	PM_OLD_DEFAULT },
	{"2160",  "NHC-4NE/HE",	PM_OLD_DEFAULT },
	{"2160i", "NHC-4NE/HE",	PM_OLD_DEFAULT },
	{"2170",  "NHP-4",	PM_OLD_DEFAULT },
	{"2180",  "NHD-4X",	PM_OLD_DEFAULT },
	{"2190",  "NHB-3NB",	PM_OLD_DEFAULT },
	{"2220",  "RH-40",	PM_OLD_DEFAULT },
	{"2220",  "RH-42",	PM_OLD_DEFAULT },
	{"2260",  "RH-39",	PM_OLD_DEFAULT },
	{"2260",  "RH-41",	PM_OLD_DEFAULT },
	{"2270",  "RH-3P",	PM_OLD_DEFAULT },
	{"2275",  "RH-3DNG",	PM_OLD_DEFAULT },
	{"2280",  "RH-17",	PM_OLD_DEFAULT },
	{"2285",  "RH-3",	PM_OLD_DEFAULT },
	{"2300",  "RM-4",	PM_OLD_DEFAULT },
	{"2300b", "RM-5",	PM_OLD_DEFAULT },
	{"2330 classic", "RM-512",	PM_DEFAULT_S40_3RD }, /* Series 40 5th Edition FP 1 Lite */
	{"2600",  "RH-59",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"2630",  "RM-298",	PM_DEFAULT },
	{"2650",  "RH-53",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"2730 classic",  "RM-578",	PM_DEFAULT_S40_3RD }, /* Series 40 5th Edition FP 1 */
	{"2760",  "RM-258",	PM_DEFAULT_S40_3RD }, /* Series 40 5th Edition */
	{"3100",  "RH-19",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"3100b", "RH-50",	PM_DEFAULT },
	{"3105",  "RH-48",	PM_OLD_DEFAULT },
	{"3108",  "RH-6",	PM_OLD_DEFAULT },
	{"3110",  "NHE-8",	PM_OLD_DEFAULT | PM_DATA },
	{"3110",  "0310" ,	PM_OLD_DEFAULT | PM_DATA }, /* NHE-8 */
	{"3110c", "RM-237",     PM_DEFAULT_S40_3RD }, /* Nokia 3110 classic */
	{"3120",  "RH-19",	PM_DEFAULT },
	{"3120b", "RH-50",	PM_DEFAULT },
	{"3200",  "RH-30",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"3200",  "RH-30/31",	PM_DEFAULT },
	{"3200b", "RH-31",	PM_DEFAULT },
	{"3210",  "NSE-8",	PM_OLD_DEFAULT | PM_NETMONITOR },
	{"3210",  "NSE-9",	PM_OLD_DEFAULT },
	{"3220",  "RH-37",	PM_DEFAULT },
	{"3285",  "NSD-1A",	PM_OLD_DEFAULT },
	{"3300",  "NEM-1",	PM_OLD_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"3300",  "NEM-2",	PM_OLD_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"3310",  "NHM-5",	PM_OLD_DEFAULT | PM_NETMONITOR },
	{"3310",  "NHM-5NX",	PM_OLD_DEFAULT },
	{"3315",  "NHM-5NY",	PM_OLD_DEFAULT },
	{"3320",  "NPC-1",	PM_OLD_DEFAULT },
	{"3330",  "NHM-6",	PM_OLD_DEFAULT | PM_NETMONITOR },
	{"3350",  "NHM-9",	PM_OLD_DEFAULT },
	{"3360",  "NPW-6",	PM_OLD_DEFAULT },
	{"3360",  "NPW-1",	PM_OLD_DEFAULT },
	{"3361",  "NPW-1",	PM_OLD_DEFAULT },
	{"3390",  "NPB-1",	PM_OLD_DEFAULT },
	{"3395",  "NPB-1B",	PM_OLD_DEFAULT },
	{"3410",  "NHM-2",	PM_OLD_DEFAULT | PM_NETMONITOR },
	{"3500c", "RM-272",	PM_DEFAULT },
	{"3510",  "NHM-8",	PM_DEFAULT },
	{"3510i", "RH-9",	PM_DEFAULT },
	{"3520",  "RH-21",	PM_DEFAULT },
	{"3530",  "RH-9",	PM_DEFAULT },
	{"3560",  "RH-14",	PM_DEFAULT },
	{"3570",  "NPD-1FW",	PM_OLD_DEFAULT },
	{"3585",  "NPD-1AW",	PM_OLD_DEFAULT },
	{"3585i", "NPD-4AW",	PM_OLD_DEFAULT },
	{"3586i", "RH-44",	PM_OLD_DEFAULT },
	{"3590",  "NPM-8",	PM_OLD_DEFAULT },
	{"3595",  "NPM-10",	PM_OLD_DEFAULT },
	{"3600",  "NHM-10",	PM_OLD_DEFAULT },
	{"3610",  "NAM-1",	PM_OLD_DEFAULT },
	{"3620",  "NHM-10X",	PM_OLD_DEFAULT },
	{"3650",  "NHL-8",	PM_DEFAULT | PM_NETMONITOR },
	{"3660",  "NHL-8X",	PM_DEFAULT | PM_NETMONITOR },
	{"3810",  "0305" ,	PM_OLD_DEFAULT | PM_DATA }, /* NHE-9 */
	{"3810",  "NHE-9",	PM_OLD_DEFAULT | PM_DATA },
	{"5100",  "NPM-6",	PM_DEFAULT },
	{"5100",  "NPM-6X",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"5100",  "NMP-6",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"5110",  "NSE-1",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5110",  "NSE-1NX",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5110i", "NSE-1NY",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5110i", "NSE-2NY",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5120",  "NSC-1",	PM_OLD_DEFAULT  },
	{"5125",  "NSC-1",	PM_OLD_DEFAULT },
	{"5130",  "NSK-1",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5130 XpressMusic",  "RM-495",	PM_DEFAULT_S40_3RD },
	{"5140",  "NPL-5",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"5140i", "RM-104",	PM_DEFAULT },
	{"5160",  "NSW-1",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5170",  "NSD-1F",	PM_OLD_DEFAULT },
	{"5180",  "NSD-1G",	PM_OLD_DEFAULT },
	{"5185",  "NSD-1A",	PM_OLD_DEFAULT },
	{"5190",  "NSB-1",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"5210",  "NSM-5",	PM_DEFAULT },
	{"5300 XpressMusic",  "RM-146",	PM_DEFAULT_S40_3RD },
	{"5310 XpressMusic",  "RM-303",	PM_DEFAULT_S40_3RD },
	{"5510",  "NPM-5",	PM_OLD_DEFAULT },
	{"6010",  "NPM-10",	PM_OLD_DEFAULT },
	{"6010",  "NPM-10X",	PM_OLD_DEFAULT },
	{"6011",  "RTE-2RH",	PM_OLD_DEFAULT },
	{"6015i", "RH-55",	PM_DEFAULT | PM_NETMONITOR },
	{"6020",  "RM-30",	PM_DEFAULT | PM_NETMONITOR },
	{"6021",  "RM-94",	PM_DEFAULT | PM_NETMONITOR | PM_XGNOKIIBREAKAGE},
	{"6030",  "RM-74",      PM_DEFAULT },
	{"6050",  "NME-1",	PM_OLD_DEFAULT },
	{"6070",  "RM-166",	PM_DEFAULT },
	{"6080",  "NME-2",	PM_OLD_DEFAULT },
	{"6081",  "NME-2A",	PM_OLD_DEFAULT },
	{"6081",  "NME-2E",	PM_OLD_DEFAULT },
	{"6085",  "RM-260",     PM_DEFAULT },
	{"6090",  "NME-3",	PM_OLD_DEFAULT },
	{"6100",  "NPL-2",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6108",  "RH-4",	PM_OLD_DEFAULT },
	{"6110",  "NSE-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"6120",  "NSC-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"6130",  "NSK-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"6133",  "RM-126",	PM_DEFAULT_S40_3RD },
	{"6136",  "RM-199",	PM_DEFAULT | PM_FULLPBK },
	{"6138",  "NSK-3",	PM_OLD_DEFAULT },
	{"6150",  "NSM-1",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"616x",  "NSW-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"6160",  "NSW-3AX",	PM_OLD_DEFAULT },
	{"6161",  "NSW-3ND",	PM_OLD_DEFAULT },
	{"6162",  "NSW-3AF",	PM_OLD_DEFAULT },
	{"6170",  "RM47_-48",	PM_DEFAULT },
	{"6185",  "NSD-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"6185i", "NSD-3AW",	PM_OLD_DEFAULT },
	{"6188",  "NSD-3AX",	PM_OLD_DEFAULT },
	{"6190",  "NSB-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA | PM_AUTHENTICATION },
	{"6200",  "NPL-3",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6210",  "NPE-3",	PM_DEFAULT | PM_NETMONITOR },
	{"6220",  "RH-20",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6225",  "RH-27",	PM_OLD_DEFAULT },
	{"6230",  "RH-12_28_-",	PM_DEFAULT | PM_NETMONITOR },
	{"6230",  "RH12_28_-",	PM_DEFAULT | PM_NETMONITOR },
	{"6230i", "RM72_73_-",	PM_DEFAULT | PM_NETMONITOR },
	{"6233",  "RM123_145_-", PM_DEFAULT | PM_NETMONITOR },
	{"6250",  "NHM-3",	PM_DEFAULT | PM_NETMONITOR },
	{"6280",  "RH-78",	PM_DEFAULT | PM_NETMONITOR },
	{"6300",  "RM-217",     PM_DEFAULT_S40_3RD | PM_FULLPBK },
	{"6303",  "RM-443",     PM_DEFAULT_S40_3RD | PM_FULLPBK }, /* Nokia 6303 classic */
	{"6310",  "NPE-4",	PM_DEFAULT },
	{"6310i", "NPL-1",	PM_DEFAULT },
	{"6340",  "NPM-2",	PM_OLD_DEFAULT },
	{"6340i", "RH-13",	PM_OLD_DEFAULT },
	{"6360",  "NPW-2",	PM_DEFAULT },
	{"6370",  "NHP-2FX",	PM_OLD_DEFAULT },
	{"6385",  "NHP-2AX",	PM_OLD_DEFAULT },
	{"6500",  "NHM-7",	PM_DEFAULT | PM_NETMONITOR },
	{"6500c", "RM-265",	PM_DEFAULT_S40_3RD | PM_FULLPBK }, /* Series 40 5th Edition */
	{"6510",  "NPM-9",	PM_DEFAULT },
	{"6560",  "RH-25",	PM_OLD_DEFAULT },
	{"6585",  "RH-34",	PM_OLD_DEFAULT },
	{"6590",  "NSM-9",	PM_OLD_DEFAULT },
	{"6590i", "NSM-9",	PM_OLD_DEFAULT },
	{"6600",  "NHL-10",	PM_DEFAULT | PM_NETMONITOR },
	{"6610",  "NHL-4U",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6610i", "RM-37",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6650",  "NHM-1",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6680",  "RM-36",	PM_DEFAULT | PM_NETMONITOR },
	{"6700 classic",  "RM-470",	PM_DEFAULT_S40_3RD },  /* Series 40 6th Edition */
	{"6800",  "NHL-6",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6800",  "NSB-9",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6810",  "RM-2",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6820",  "NHL-9",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6820b", "RH-26",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"6822",  "RM-69",	PM_DEFAULT },
	{"7110",  "NSE-5",	PM_DEFAULT | PM_NETMONITOR },
	{"7160",  "NSW-5",	PM_OLD_DEFAULT },
	{"7190",  "NSB-5",	PM_DEFAULT | PM_NETMONITOR },
	{"7200",  "RH-23",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7210",  "NHL-4",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7250",  "NHL-4J",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7250i", "NHL-4JX",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7260",  "RM-17",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7270",  "RM-8",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7280",  "RM-14",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7373",  "RM-209",	PM_DEFAULT },
	{"7600",  "NMM-3",	PM_DEFAULT | PM_XGNOKIIBREAKAGE },
	{"7650",  "NHL-2NA",	PM_DEFAULT },
	{"8110",  "0423" ,	PM_OLD_DEFAULT | PM_DATA }, /* NHE-6BX */
	{"8110",  "2501",	PM_OLD_DEFAULT | PM_DATA },
	{"8110",  "NHE-6",	PM_OLD_DEFAULT | PM_DATA },
	{"8110",  "NHE-6BX",	PM_OLD_DEFAULT | PM_DATA },
	{"8110i", "0423",	PM_OLD_DEFAULT | PM_DATA }, /* Guess for NHE-6 */
	{"8110i", "NHE-6",	PM_OLD_DEFAULT | PM_DATA },
	{"8110i", "NHE-6BM",	PM_OLD_DEFAULT | PM_DATA },
	{"8146",  "NHK-6",	PM_OLD_DEFAULT | PM_DATA },
	{"8148",  "NHK-6",	PM_OLD_DEFAULT | PM_DATA },
	{"8148i", "NHK-6V",	PM_OLD_DEFAULT | PM_DATA },
	{"8210",  "NSM-3",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA },
	{"8250",  "NSM-3D",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA },
	{"8260",  "NSW-4",	PM_OLD_DEFAULT },
	{"8265",  "NPW-3",	PM_OLD_DEFAULT },
	{"8270",  "NSD-5FX",	PM_OLD_DEFAULT },
	{"8280",  "RH-10",	PM_OLD_DEFAULT },
	{"8290",  "NSB-7",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA },
	{"8310",  "NHM-7",	PM_DEFAULT | PM_NETMONITOR },
	{"8390",  "NSB-8",	PM_OLD_DEFAULT },
	{"8810",  "NSE-6",	PM_OLD_DEFAULT | PM_DATA },
	{"8850",  "NSM-2",	PM_OLD_DEFAULT | PM_NETMONITOR | PM_DATA },
	{"8855",  "NSM-4",	PM_OLD_DEFAULT },
	{"8860",  "NSW-6",	PM_OLD_DEFAULT },
	{"8890",  "NSB-6",	PM_OLD_DEFAULT },
	{"8910",  "NHM-4NX",	PM_OLD_DEFAULT },
	{"8910i", "NHM-4NX",	PM_OLD_DEFAULT },
	{"9000",  "RAE-1",	PM_OLD_DEFAULT },
	{"9000i", "RAE-4",	PM_OLD_DEFAULT },
	{"9110",  "RAE-2",	PM_OLD_DEFAULT },
	{"9110i", "RAE-2i",	PM_OLD_DEFAULT },
	{"9210",  "RAE-2",	PM_OLD_DEFAULT },
	{"9210c", "RAE-3",	PM_OLD_DEFAULT },
	{"9210i", "RAE-5N",	PM_OLD_DEFAULT },
	{"9290",  "RAE-3N",	PM_OLD_DEFAULT },
	{"N70",   "RM-84",      PM_DEFAULT },
	{"N73",   "RM-133",	PM_DEFAULT },
	{"RPM-1", "RPM-1",	PM_OLD_DEFAULT | PM_DATA },
	{"Card Phone 1.0", "RPE-1",	PM_OLD_DEFAULT | PM_DATA },
	{"Card Phone 2.0", "RPM-1",	PM_OLD_DEFAULT | PM_DATA },
	{"C110 Wireless LAN Card", "DTN-10",	PM_OLD_DEFAULT },
	{"C111 Wireless LAN Card", "DTN-11",	PM_OLD_DEFAULT },
	{"D211",  "DTE-1",	PM_OLD_DEFAULT },
	{"N-Gage", "NEM-4",	PM_OLD_DEFAULT },
	{"RinGo", "NHX-7",	PM_OLD_DEFAULT },
	{"sx1",  "SX1",		PM_DEFAULT },
	{"SIM/USIM", "APDU",	0 },
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
	return gn_cfg_get_phone_model(gn_cfg_info, product_name)->model;
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
	va_list ap, ap1;

	va_start(ap, fmt);

	va_copy(ap1, ap);
	log_printf(gn_log_debug_mask, fmt, ap1);
	va_end(ap1);

	if (gn_elog_handler) {
		va_copy(ap1, ap);
		gn_elog_handler(fmt, ap1);
		va_end(ap1);
	} else {
#ifndef	DEBUG
		if (!(gn_log_debug_mask & GN_LOG_T_STDERR)) {
			va_copy(ap1, ap);
			log_printf(GN_LOG_T_STDERR, fmt, ap1);
			va_end(ap1);
		}
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
		fprintf(stderr, _("Cannot lock NULL device. Set port config parameter correctly.\n"));
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
	/*
	 * I think we don't need to use strncpy, as we should have enough
	 * buffer due to strlen results, but it's safer to do so...
	 */
	strncpy(lock_file, lock_path, len);
	strncat(lock_file, aux, len - strlen(lock_file));

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
				fprintf(stderr, _("Lockfile %s is stale. Overriding it...\n"), lock_file);
				sleep(1);
				if (unlink(lock_file) == -1) {
					fprintf(stderr, _("Overriding file %s failed, please check the permissions.\n"), lock_file);
					fprintf(stderr, _("Cannot lock device.\n"));
					goto failed;
				}
			} else {
				fprintf(stderr, _("Device already locked with %s.\n"), lock_file);
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
			fprintf(stderr, _("Cannot create lockfile %s. Please check for existence of the path.\n"), lock_file);
		goto failed;
	}
	snprintf(buffer, sizeof(buffer), "%10ld gnokii\n", (long)getpid());
	if (write(fd, buffer, strlen(buffer)) < 0) {
		fprintf(stderr, _("Failed to write to the lockfile %s.\n"), lock_file);
		goto failed;
	}
	close(fd);
	return lock_file;
failed:
	if (fd > -1)
		close(fd);
	free(lock_file);
	return NULL;
#else
	return (char *)1;
#endif /* WIN32 */
}

/* Removes lock and frees memory */
GNOKII_API int gn_device_unlock(char *lock_file)
{
#ifndef WIN32
	int err;

	if (lock_file) {
		err = unlink(lock_file);
		free(lock_file);
		if (err) {
			fprintf(stderr, _("Cannot unlock device: %s\n"), strerror(errno));
			return false;
		}
	}
	return true;
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

	/*
	 * Last two elements are:
	 *  - leftover from the input line
	 *  - NULL (required by gnokii_strfreev())
	 */
	strings = calloc(tokens + 2, sizeof(char *));

	while ((tmp = strstr(left, delimiter)) != NULL && (count < tokens)) {
		str = malloc((tmp - left) + 1);
		memset(str, 0, (tmp - left) + 1);
		memcpy(str, left, tmp - left);
		strings[count] = str;
		left = tmp + strlen(delimiter);
		count++;
	}

	strings[count] = strdup(left);
	/*
	 * Make NULL termination explicit even if it is done via calloc().
	 * gnokii_strfreev() requires it to be NULL-terminated.
	 */
	strings[count + 1] = NULL;

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

/**
 * gnokii_strcmpsep:
 * @s1: a string, NUL terminated, may NOT contain @sep
 * @s2: a string, NUL terminated, may contain @sep
 * @sep: comparison stops if this char is found in @s2
 *
 * Returns: < 0 if @s1 < @s2
 *            0 if @s1 == @s2
 *          > 0 if @s1 > @s2
 *
 * Compares two strings up to a NUL terminator or a separator char.
 * Leading and trailing white space in @s2 is ignored.
 */
int gnokii_strcmpsep(const char *s1, const char *s2, char sep)
{
	while (isblank(*s2))
		s2++;
	while (*s1 && *s1 == *s2) {
		s1++;
		s2++;
	}
	while (isblank(*s2))
		s2++;
	if (!*s1 && *s2 == sep)
		return 0;

	return *s1 - *s2;
}

/*
 * check if the timestamp in dt has valid date and time
 */
GNOKII_API int gn_timestamp_isvalid(const gn_timestamp dt)
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
