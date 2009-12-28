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

  Copyright (C) 2007 Daniele Forsi

  This file provides functions for accessing PC/SC SIM smart cards.

*/

#ifndef _gnokii_phones_pcsc_h
#define _gnokii_phones_pcsc_h

#include "compat.h"
#include "gnokii.h"
#include "phones/generic.h"
#include "gnokii-internal.h"

#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>

#if !HAVE_LPCSTR
typedef const char *LPCSTR;
#endif

/*
some CLAss, INStruction pairs defined by GSM specs
taken from ETSI TS 100 977 V8.13.0 (2005-06)
subclause : command
   9.2.1  : SELECT
   9.2.3  : READ BINARY
   9.2.4  : UPDATE BINARY
   9.2.5  : READ RECORD
   9.2.6  : UPDATE RECORD
   9.2.9  : VERIFY CHV
   9.2.18 : GET RESPONSE
*/
#define GN_PCSC_CMD_SELECT       0xA0, 0xA4
#define GN_PCSC_CMD_READ_BINARY  0xA0, 0xB0
#define GN_PCSC_CMD_UPDATE_BINARY  0xA0, 0xD6
#define GN_PCSC_CMD_READ_RECORD  0xA0, 0xB2
#define GN_PCSC_CMD_UPDATE_RECORD  0xA0, 0xDC
#define GN_PCSC_CMD_VERIFY_CHV   0xA0, 0x20
#define GN_PCSC_CMD_GET_RESPONSE 0xA0, 0xC0

/* useful defines from subclause 9.2.5 */

#define GN_PCSC_FILE_READ_NEXT 2
#define GN_PCSC_FILE_READ_PREV 3
#define GN_PCSC_FILE_READ_ABS  4

/* useful defines from subclause 9.3 */

#define GN_PCSC_FILE_STRUCTURE_TRANSPARENT 0
#define GN_PCSC_FILE_STRUCTURE_LINEAR_FIXED 1
#define GN_PCSC_FILE_STRUCTURE_CYCLIC 3

#define GN_PCSC_FILE_TYPE_MF 1
#define GN_PCSC_FILE_TYPE_DF 2
#define GN_PCSC_FILE_TYPE_EF 4

#define GN_PCSC_PARAMETER_OFFSET_P1 2
#define GN_PCSC_PARAMETER_OFFSET_P2 3
#define GN_PCSC_PARAMETER_OFFSET_P3 4
#define GN_PCSC_PARAMETER_OFFSET_FILEID_HI 5
#define GN_PCSC_PARAMETER_OFFSET_FILEID_LO 6
#define GN_PCSC_DATA_OFFSET 5

/* misc defines */
#define GN_PCSC_ICCID_MAX_LENGTH 21
#define GN_PCSC_IMSI_MAX_LENGTH 16
#define GN_PCSC_SPN_MAX_LENGTH 17

/* some file identifiers (full list is in subclause 10.7) */

/* This is the top level container */
#define GN_PCSC_FILE_MF 0x3F00
/* ICC Identification */
#define GN_PCSC_FILE_EF_ICCID 0x2FE2
/* This is the container of phonebooks and SMS */
#define GN_PCSC_FILE_DF_TELECOM 0x7F10
/* This is the container of GSM specific information */
#define GN_PCSC_FILE_DF_GSM 0x7F20
/* International Mobile Subscriber Identity (IMSI) */
#define GN_PCSC_FILE_EF_IMSI 0x6F07
/* Abbreviated Dialing Numbers */
#define GN_PCSC_FILE_EF_ADN 0x6F3A
/* Fixed Dialing Numbers */
#define GN_PCSC_FILE_EF_FDN 0x6F3B
/* Short Messages */
#define GN_PCSC_FILE_EF_SMS 0x6F3C
/* Mobile Subscriber Integrated Services Digital Network Number */
#define GN_PCSC_FILE_EF_MSISDN 0x6F40
/* Short Message Service Parameters */
#define GN_PCSC_FILE_EF_SMSP 0x6f42
/* Last Numbers Dialed */
#define GN_PCSC_FILE_EF_LND 0x6F44
/* Service Provider Name */
#define GN_PCSC_FILE_EF_SPN 0x6F46
/* Service Dialing Numbers */
#define GN_PCSC_FILE_EF_SDN 0x6F49
/* Location Information */
#define GN_PCSC_FILE_EF_LOCI 0x6F7E
/* Barred Dialing Numbers */
#define GN_PCSC_FILE_EF_BDN 0x6F4D
/* Administrative data */
#define GN_PCSC_FILE_EF_AD 0x6FAD
/* Phase Identification */
#define GN_PCSC_FILE_EF_PHASE 0x6FAE
/* Emergency Call Codes */
#define GN_PCSC_FILE_EF_ECC 0x6FB7
/* a convenience define */
#define GN_PCSC_FILE_UNKNOWN 0x0000

typedef struct {
	BYTE *pbRecvBuffer;
	DWORD dwRecvLength;
	DWORD dwReceived;
	BYTE bRecordNumber;
} PCSC_IOSTRUCT;

#endif /* #ifndef _gnokii_phones_pcsc_h */
