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

  Copyright (C) 2007-2008 Daniele Forsi

  This file provides functions for accessing PC/SC SIM smart cards.
  Where not otherwise noted the document referenced is ETSI TS 100 977 V8.13.0 (2005-06).

*/

#include "config.h"

#ifdef HAVE_PCSC

#include "phones/pcsc.h"

/* prototypes for functions related to libgnokii */

static gn_error GetFile(gn_data *data, struct gn_statemachine *state);
static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state);
static gn_error GetNetworkInfo(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSCenter(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSFolders(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error GetSMSStatus(gn_data *data, struct gn_statemachine *state);
static gn_error DeleteSMSMessage(gn_data *data, struct gn_statemachine *state);
static gn_error Identify(gn_data *data, struct gn_statemachine *state);
static gn_error Initialise(struct gn_statemachine *state);
static gn_error ReadPhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error DeletePhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error Terminate(gn_data *data, struct gn_statemachine *state);
#ifdef SECURITY
static gn_error GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state);
static gn_error EnterSecurityCode(gn_data *data, struct gn_statemachine *state);
#endif
static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state);

/* prototypes for functions related to libpcsclite */

static LONG pcsc_change_dir(PCSC_IOSTRUCT *ios, LONG directory_id);
static LONG pcsc_close_reader();
static LONG pcsc_cmd_get_response(PCSC_IOSTRUCT *ios, BYTE length);
static LONG pcsc_cmd_read_binary(PCSC_IOSTRUCT *ios, LONG length);
static LONG pcsc_cmd_read_record(PCSC_IOSTRUCT *ios, BYTE record, BYTE length);
static LONG pcsc_cmd_update_record(PCSC_IOSTRUCT *ios, BYTE record, BYTE *data, BYTE length);
static LONG pcsc_cmd_select(PCSC_IOSTRUCT *ios, LONG file_id);
static LONG pcsc_file_get_contents(PCSC_IOSTRUCT *ios, LONG file_id);
static LONG pcsc_open_reader_name(LPCSTR reader_name);
static LONG pcsc_open_reader_number(LONG number);
static LONG pcsc_read_file(PCSC_IOSTRUCT *ios, LONG dir_id, LONG file_id);
static LONG pcsc_read_file_record(PCSC_IOSTRUCT *ios, LONG file_id, BYTE record);
static LONG pcsc_stat_file(PCSC_IOSTRUCT *ios, LONG file_id);
static LONG pcsc_update_file_record(PCSC_IOSTRUCT *ios, LONG file_id, BYTE record, BYTE *data, BYTE length);
#ifdef SECURITY
static LONG pcsc_verify_chv(PCSC_IOSTRUCT *ios, BYTE chv_id, BYTE *chv, BYTE chv_len);
#endif

/* prototypes for functions converting between the two libraries */

static gn_error get_gn_error(PCSC_IOSTRUCT *ios, LONG ret);
static LONG get_memory_type(gn_memory_type memory_type);
static gn_error ios2gn_error(PCSC_IOSTRUCT *ios);

/* higher level functions */

static gn_error get_iccid(char *result);
static gn_error get_imsi(char *result);
static gn_error get_mnc_len(int *len);
static gn_error get_phase(const char **phase);

/* Some globals */

gn_driver driver_pcsc = {
	NULL,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"pcsc|APDU",           /* Supported models */
		5,                     /* Unused */ /* Max RF Level */
		0,                     /* Unused */ /* Min RF Level */
		GN_RF_Percentage,      /* Unused */ /* RF level units */
		100,                   /* Unused */ /* Max Battery Level */
		0,                     /* Unused */ /* Min Battery Level */
		GN_BU_Percentage,      /* Unused */ /* Battery level units */
		GN_DT_None,            /* Unused */ /* Have date/time support */
		GN_DT_None,            /* Unused */ /* Alarm supports time only */
		0,                     /* Unused */ /* Alarms available */
		60, 96,                /* Unused */ /* Startup logo size */
		21, 78,                /* Unused */ /* Op logo size */
		14, 72                 /* Unused */ /* Caller logo size */
	},
	functions,
	NULL
};

SCARDCONTEXT hContext;
SCARDHANDLE hCard;
DWORD dwActiveProtocol;
SCARD_IO_REQUEST *pioSendPci, pioRecvPci;
BYTE buf[MAX_BUFFER_SIZE];
PCSC_IOSTRUCT IoStruct = { buf, sizeof(buf), 0, 0 };

#ifdef DEBUG
#define DUMP_BUFFER(ret, str, buf, len) pcsc_dump_buffer(ret, (char *) __FUNCTION__, str, buf, len)

void pcsc_dump_buffer(LONG ret, const char *func, const char *str, BYTE *buf, LONG len)
{
	if (ret != SCARD_S_SUCCESS) {
		dump("%s: %s (0x%lX)\n", func, pcsc_stringify_error(ret), ret);
	} else {
		dump("%s", str);
		sm_message_dump(gn_elog_write, 0, buf, len);
	}
}
#else
#define DUMP_BUFFER(a, b, c, d)  do { } while (0)
#endif

#define PCSC_TRANSMIT_WITH_LENGTH(ret, string, pbSendBuffer, bSendLength, ios) \
	DUMP_BUFFER(ret, "Sending " string ": ", pbSendBuffer, bSendLength); \
	ret = SCardTransmit(hCard, pioSendPci, pbSendBuffer, bSendLength, &pioRecvPci, ios->pbRecvBuffer, &ios->dwReceived); \
	DUMP_BUFFER(ret, "Received: ", ios->pbRecvBuffer, ios->dwReceived);

#define PCSC_TRANSMIT(ret, string, pbSendBuffer, ios) \
	PCSC_TRANSMIT_WITH_LENGTH(ret, string, pbSendBuffer, sizeof(pbSendBuffer), ios)


/* helper functions */

static gn_error ios2gn_error(PCSC_IOSTRUCT *ios)
{
/* Convert a card status condition code into a libgnokii error code */
	if (!ios || ios->dwReceived < 2) return GN_ERR_NONE;
	/* check for a "soft" error */
	switch (ios->pbRecvBuffer[ios->dwReceived - 2]) {
	/* 9.4.1 Responses to commands which are correctly executed */
	case 0x90: /* "normal ending of command" */
	case 0x91: /* "normal ending of command, with extra information" */
	case 0x9E: /* "SW2 is the length of response data" (SIM data donwload error) */
	case 0x9F: /* "SW2 is the length of response data" */
		return GN_ERR_NONE;
	case 0x92: /* 9.4.3 Memory management */
		if (ios->pbRecvBuffer[ios->dwReceived - 1] < 16) {
			dprintf("WARNING: command succeded after %d retries\n", ios->pbRecvBuffer[ios->dwReceived - 1]);
			return GN_ERR_NONE;
		} else {
			return GN_ERR_FAILED;
		}
	case 0x94: /* Subclause 9.4.4 Referencing management */
		switch (ios->pbRecvBuffer[ios->dwReceived - 1]) {
		case 0x02: /* out of range (invalid address) */
			return GN_ERR_INVALIDLOCATION;
		case 0x04: /* file ID (or pattern) not found */
			return GN_ERR_INVALIDMEMORYTYPE;
		}
	case 0x98: /* Subclause 9.4.5 Security management */
		dprintf("ERROR: PIN/PUK failure code 0x%02x\n", ios->pbRecvBuffer[ios->dwReceived - 1]);
		return GN_ERR_CODEREQUIRED;
	default:
		return GN_ERR_UNKNOWN;
	}
}

static gn_error get_gn_error(PCSC_IOSTRUCT *ios, LONG ret)
{
/* Convert a libpcsclite error code into a libgnokii error code */
	switch (ret) {
	case SCARD_S_SUCCESS:
		return ios2gn_error(ios);
	case SCARD_F_INTERNAL_ERROR:
		return GN_ERR_INTERNALERROR;
	case SCARD_E_NOT_READY:
		return GN_ERR_NOTREADY;
	case SCARD_E_NO_SMARTCARD:
	case SCARD_W_UNSUPPORTED_CARD:
	case SCARD_W_UNRESPONSIVE_CARD:
	case SCARD_W_UNPOWERED_CARD:
	case SCARD_W_RESET_CARD:
	case SCARD_W_REMOVED_CARD:
	 	return GN_ERR_SIMPROBLEM;
	case SCARD_E_NO_MEMORY:
		return GN_ERR_MEMORYFULL;
	case SCARD_F_WAITED_TOO_LONG:
	case SCARD_E_TIMEOUT:
		return GN_ERR_TIMEOUT;
	case SCARD_E_READER_UNAVAILABLE:
	case SCARD_E_READER_UNSUPPORTED:
	case SCARD_E_NO_SERVICE:
	case SCARD_E_SERVICE_STOPPED:
	case SCARD_E_UNKNOWN_READER:
		return GN_ERR_NOLINK;
	default:
		return GN_ERR_UNKNOWN;
	}
}

static LONG get_memory_type(gn_memory_type memory_type)
{
/* Convert a libgnokii memory_type into a GSM file ID */
	switch (memory_type) {
	case GN_MT_SM:
		return GN_PCSC_FILE_EF_ADN;
	case GN_MT_EN:
		return GN_PCSC_FILE_EF_ECC;
	case GN_MT_FD:
		return GN_PCSC_FILE_EF_FDN;
	case GN_MT_LD:
		return GN_PCSC_FILE_EF_LND;
	case GN_MT_ON:
		return GN_PCSC_FILE_EF_MSISDN;
	case GN_MT_SD:
		return GN_PCSC_FILE_EF_SDN;
	case GN_MT_BD:
		return GN_PCSC_FILE_EF_BDN;
	default:
		return GN_PCSC_FILE_UNKNOWN;
	}
}

static size_t alpha_tag_strlen(BYTE *buf, size_t max_length)
{
/* Calculate length of an "alpha tag" (ie the text associated to a phone number)
 according to subclause 10.5.1 and Annex B */
	size_t alpha_length;

	switch (*buf) {
	case 0x80:
		/* UCS2 alphabet */
		buf++; /* skip 0x80 */
		alpha_length = 0;
		while ((*buf++ != 0xff) && (alpha_length < max_length)) {
			alpha_length++;
		}
		break;
	case 0x81:
	case 0x82:
		/* UCS2 alphabet with different coding schemes */
		if (buf[1] > max_length) {
			alpha_length = max_length;
		} else {
			alpha_length = buf[1];
		}
		break;
	default:
		/* GSM Default Alphabet */
		alpha_length = 0;
		while ((*buf++ != 0xff) && (alpha_length < max_length)) {
			alpha_length++;
		}
	}

	return alpha_length;
}

static void alpha_tag_decode(BYTE *dest, BYTE *buf, size_t alpha_length)
{
/* Handle decoding according to Annex B */
	BYTE *temp;
	int i, ucs2_base;

	switch (*buf) {
	case 0x80:
		/* UCS2 alphabet */
		buf++;
		char_unicode_decode(dest, buf, alpha_length * 2);
		break;
	case 0x81:
	case 0x82:
		/* UCS2 alphabet mixed with GSM Default Alphabet */
		if (*buf == 0x81) {
			ucs2_base = *(buf+2) << 7;
			buf += 3;
		} else {
			ucs2_base = (*(buf+2) << 8) + *(buf+3);
			buf += 4;
		}
		dprintf("ucs2_base = 0x%04x\n", ucs2_base);
		temp = (BYTE *) malloc(alpha_length * 2);
		if (!temp) {
			dest[0] = '\0';
			return;
		}
		/* Convert to pure UCS2 data in a temporary buffer to use existing libgnokii function */
		for (i = 0; i < alpha_length; i++, buf++) {
			if (*buf & 0x80) {
				/* UCS2 alphabet */
				dprintf("ucs2 = 0x%04x\n", ucs2_base + (*buf & 0x7f));
				temp[i * 2] = ((ucs2_base + (*buf & 0x7f)) >> 8) & 0xff;
				temp[i * 2 + 1] = (ucs2_base + (*buf & 0x7f)) & 0xff;
			} else {
				/* GSM Default Alphabet */
				dprintf("GSM = 0x%02x\n", *buf & 0x7f);
				temp[i * 2] = 0;
				temp[i * 2 + 1] = char_def_alphabet_decode(*buf & 0x7f);
			}
		}
		char_unicode_decode(dest, temp, alpha_length * 2);
		free(temp);
		break;
	default:
		/* GSM Default Alphabet */
		i = 0;
		while ((*buf != 0xff) && (i < alpha_length)) {
			dest[i++] = char_def_alphabet_decode(*buf++);
		}
		dest[i] = '\0';
	}
}

/* higher level functions */

/* read ICCID and convert it into an ASCII string in the provided buffer (at least GN_PCSC_ICCID_MAX_LENGTH bytes long) */
static gn_error get_iccid(char *result)
{
	LONG ret;
	LONG result_len;
	BYTE *buf;
	gn_error error;

	ret = pcsc_read_file(&IoStruct, GN_PCSC_FILE_MF, GN_PCSC_FILE_EF_ICCID);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* Convert into ASCII digits */
	result_len = (GN_PCSC_ICCID_MAX_LENGTH - 1) / 2;
	buf = IoStruct.pbRecvBuffer;
	/* Note: the following ignores the exceptions for some Phase 1 SIM cards (see subclause 10.1.1) */
	while (result_len--) {
		if ((*buf & 0xf) == 0xf) break;
		*result++ = (*buf & 0xf) + '0';
		if ((*buf & 0xf0) == 0xf0) break;
		*result++ = ((*buf & 0xf0) >> 4) + '0';
		buf++;
	}
	*result = '\0';

	return error;
}

/*
sets *len to the number of MNC digits in IMSI from Administrative Data (subclause 10.3.18)
*len is unchanged in case of error
*/
static gn_error get_mnc_len(int *len)
{
	LONG ret;
	gn_error error;

	ret = pcsc_read_file(&IoStruct, GN_PCSC_FILE_DF_GSM, GN_PCSC_FILE_EF_AD);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* add 2 because dwRecvLength contains SW */
	if (IoStruct.dwReceived >= 4 + 2) {
		*len = IoStruct.pbRecvBuffer[3];
	}

	return error;
}

/* read IMSI and convert it into an ASCII string in the provided buffer (at least GN_PCSC_IMSI_MAX_LENGTH bytes long) */
static gn_error get_imsi(char *result)
{
	LONG ret;
	LONG result_len;
	BYTE *buf;
	gn_error error;

	ret = pcsc_read_file(&IoStruct, GN_PCSC_FILE_DF_GSM, GN_PCSC_FILE_EF_IMSI);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* Convert IMSI into ASCII digits (ETSI TS 100 940 V7.21.0 (2003-12) only talks about MCC and MNC parts) */
	result_len = IoStruct.pbRecvBuffer[0];
	buf = IoStruct.pbRecvBuffer + 1;
	/* ignore lower nibble of first octet because it is a bit field */
	*result++ = ((*buf & 0xf0) >> 4) + '0';
	while (--result_len) {
		buf++;
		*result++ = (*buf & 0xf) + '0';
		*result++ = ((*buf & 0xf0) >> 4) + '0';
	}
	*result = '\0';

	return error;
}

/*
sets *phase to a string representing Phase Identification of the SIM (subclause 10.3.19) or leaves it unchanged in case of error
phase can be "1", "2" or "2+"
*/
static gn_error get_phase(const char **phase)
{
	LONG ret;
	gn_error error;

	ret = pcsc_read_file(&IoStruct, GN_PCSC_FILE_DF_GSM, GN_PCSC_FILE_EF_PHASE);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* add 2 because dwRecvLength contains SW */
	if (IoStruct.dwReceived >= 1 + 2) {
		switch (IoStruct.pbRecvBuffer[0]) {
		case 1: *phase = "1"; break;
		case 2: *phase = "2"; break;
		case 3: *phase = "2+"; break;
		default: dprintf("Unknown Phase Identification 0x%02x\n", IoStruct.pbRecvBuffer[0]);
		}
	}

	return error;
}

/* read SPN and convert it into an ASCII string in the provided buffer (at least GN_PCSC_SPN_MAX_LENGTH bytes long)
according to subclause 10.3.11 and Annex B
*/
static gn_error get_spn(char *result)
{
	LONG ret;
	BYTE *buf;
	gn_error error;

	ret = pcsc_read_file(&IoStruct, GN_PCSC_FILE_DF_GSM, GN_PCSC_FILE_EF_SPN);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* Skip first byte (it's a bit field) */
	buf = IoStruct.pbRecvBuffer + 1;
	alpha_tag_decode(result, buf, GN_PCSC_SPN_MAX_LENGTH);

	return GN_ERR_NONE;
}

/* functions for libgnokii stuff */

static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state)
{
	switch (op) {
	case GN_OP_Init:
		return Initialise(state);
	case GN_OP_Terminate:
		return Terminate(data, state);
	case GN_OP_Identify:
		return Identify(data, state);
	case GN_OP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GN_OP_GetNetworkInfo:
		return GetNetworkInfo(data, state);
	case GN_OP_GetSMSCenter:
		return GetSMSCenter(data, state);
	case GN_OP_GetSMS:
		return GetSMSMessage(data, state);
	case GN_OP_GetSMSFolders:
		return GetSMSFolders(data, state);
	case GN_OP_GetSMSFolderStatus:
		return GetSMSFolderStatus(data, state);
	case GN_OP_GetSMSStatus:
		return GetSMSStatus(data, state);
	case GN_OP_DeleteSMS:
		return DeleteSMSMessage(data, state);
	case GN_OP_ReadPhonebook:
		return ReadPhonebook(data, state);
	case GN_OP_DeletePhonebook:
		return DeletePhonebook(data, state);
	case GN_OP_GetFile:
		return GetFile(data, state);
#ifdef SECURITY
	case GN_OP_EnterSecurityCode:
		return EnterSecurityCode(data, state);
	case GN_OP_GetSecurityCodeStatus:
		return GetSecurityCodeStatus(data, state);
#endif
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_INTERNALERROR;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error Initialise(struct gn_statemachine *state)
{
	LONG ret;
	LONG reader_number;
	char *aux = NULL;

	dprintf("Initializing...\n");

	if (!strcmp("pcsc", state->config.model))
		fprintf(stderr, "WARNING: %s=%s is deprecated and will soon be removed. Use %s=%s instead.\n", "model", state->config.model, "model", "APDU");

	/* Copy in the phone info */
	memcpy(&(state->driver), &driver_pcsc, sizeof(gn_driver));

	/* Initialise */
	ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (ret != SCARD_S_SUCCESS) {
		dprintf("SCardEstablishContext error %lX: %s\n", ret, pcsc_stringify_error(ret));
		return get_gn_error(NULL, ret);
	}

	/* Now establish the link */
	reader_number = strtol(state->config.port_device, &aux, 10);
	if (*aux) {
		/* port_device doesn't look like a number, so use it as a reader name */
		ret = pcsc_open_reader_name(state->config.port_device);
	} else {
		/* use port_device as an index in the list of readers (starting from 0) */
		ret = pcsc_open_reader_number(reader_number);
	}

	return get_gn_error(NULL, ret);
}

static gn_error Terminate(gn_data *data, struct gn_statemachine *state)
{
	LONG ret;

	/* we're going to terminate even if this call returns error */
	(void) pcsc_close_reader();

	ret = SCardReleaseContext(hContext);
	if (ret != SCARD_S_SUCCESS) {
		dprintf("SCardReleaseContext error %lX: %s\n", ret, pcsc_stringify_error(ret));
	}

	return get_gn_error(NULL, ret);
}

static gn_error Identify(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	char imsi[GN_PCSC_IMSI_MAX_LENGTH], iccid[GN_PCSC_ICCID_MAX_LENGTH];
	char spn[GN_PCSC_SPN_MAX_LENGTH] = "";
	const char *phase = "??";

	/* read ICCID */
	error = get_iccid(iccid);
	if (error == GN_ERR_NONE) {
		/* showing this as IMEI would be confusing, so just print it in debug mode */
		dprintf("ICCID = %s\n", iccid);
	}

	/* read IMSI */
	error = get_imsi(imsi);
	if (error == GN_ERR_NONE) {
		char mcc[4], mnc[4], net_code[8], *network_name;
		int mnc_len = 2;

		/* extract MCC and MNC */
		snprintf(mcc, sizeof(mcc), "%s", imsi);
		/* ignore errors, set default value above */
		get_mnc_len(&mnc_len);
		snprintf(mnc, sizeof(mnc), "%.*s", mnc_len, imsi + 3);
		dprintf("IMSI = %s Home PLMN = MCC %s MNC %s\n", imsi, mcc, mnc);
		snprintf(net_code, sizeof(net_code), "%s %s", mcc, mnc);
		/* get operator and country names as strings from libgnokii */
		network_name = gn_network_name_get(net_code);
		/* read Service Provider Name */
		(void) get_spn(spn);
		if (*spn && strcmp(spn, network_name))
			snprintf(data->manufacturer, GN_MANUFACTURER_MAX_LENGTH, "%s, %s (%s)", spn, network_name, gn_country_name_get(mcc));
		else
			snprintf(data->manufacturer, GN_MANUFACTURER_MAX_LENGTH, "%s (%s)", network_name, gn_country_name_get(mcc));
	}

	snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", "APDU");

	/* read SIM "Phase Identification" */
	/* ignore errors, set default value above */
	get_phase(&phase);
	snprintf(data->revision, GN_REVISION_MAX_LENGTH, _("Phase %s SIM"), phase);

	return GN_ERR_NONE;
}

static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state)
{
	LONG file;
	LONG ret;

	if (!data || !data->memory_status) {
		return GN_ERR_INTERNALERROR;
	}
	file = get_memory_type(data->memory_status->memory_type);
	if (!file) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	if (get_gn_error(&IoStruct, ret) == GN_ERR_NONE) {
		ret = pcsc_stat_file(&IoStruct, file);
		if (get_gn_error(&IoStruct, ret) == GN_ERR_NONE) {
			/* FIXME: this is total space (should read all entries and subtract empty ones) */
			data->memory_status->used = (IoStruct.pbRecvBuffer[2] * 256 + IoStruct.pbRecvBuffer[3]) / IoStruct.pbRecvBuffer[14];
			data->memory_status->free = 0;
		}
	}

	return get_gn_error(&IoStruct, ret);
}

static gn_error GetNetworkInfo(gn_data *data, struct gn_statemachine *state)
{
/* See Subclause 10.3.17 */
	LONG ret;
	gn_error error;

	ret = pcsc_read_file(&IoStruct, GN_PCSC_FILE_DF_GSM, GN_PCSC_FILE_EF_LOCI);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* See Subclause 10.5.1.3 of 3GPP TS 04.08 version 7.21.0 Release 1998 == ETSI TS 100 940 V7.21.0 (2003-12) */
	data->network_info->cell_id[0] = 0;
	data->network_info->cell_id[1] = 0;
	data->network_info->LAC[0] = IoStruct.pbRecvBuffer[7];
	data->network_info->LAC[1] = IoStruct.pbRecvBuffer[8];
	data->network_info->network_code[0] = '0' + (IoStruct.pbRecvBuffer[4] & 0x0f);
	data->network_info->network_code[1] = '0' + (IoStruct.pbRecvBuffer[4] >> 4);
	data->network_info->network_code[2] = '0' + (IoStruct.pbRecvBuffer[5] & 0x0f);
	data->network_info->network_code[3] = ' ';
	data->network_info->network_code[4] = '0' + (IoStruct.pbRecvBuffer[6] & 0x0f);
	data->network_info->network_code[5] = '0' + (IoStruct.pbRecvBuffer[6] >> 4);
	/* 3-digits MNC */
	if ((IoStruct.pbRecvBuffer[5] & 0xf0) != 0xf0) {
		data->network_info->network_code[6] = '0' + (IoStruct.pbRecvBuffer[5] >> 4);
		data->network_info->network_code[7] = '\0';
	} else {
		data->network_info->network_code[6] = '\0';
	}

	return error;
}

static gn_error GetSMSCenter(gn_data *data, struct gn_statemachine *state)
{
/* See Subclause 10.5.6 */
	LONG file;
	LONG ret;
	int alpha_max_len, alpha_length, parameter_indicators;
	gn_error error;

	if (!data || !data->message_center) {
		return GN_ERR_INTERNALERROR;
	}
	if ((data->message_center->id < 1) || (data->message_center->id > 255)) {
		return GN_ERR_INVALIDLOCATION;
	}
	file = GN_PCSC_FILE_EF_SMSP;

	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	ret = pcsc_read_file_record(&IoStruct, file, data->message_center->id);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* Expected layout: 28+Y bytes (subtract 2 for SW) */
	alpha_max_len = IoStruct.dwReceived - 28 - 2;
	alpha_length = alpha_tag_strlen(IoStruct.pbRecvBuffer, alpha_max_len);
	dprintf("Max length of Alpha Identifier is %d\n", alpha_max_len);
	if (alpha_max_len < 0) return GN_ERR_INTERNALERROR;

	/* Alpha-Identifier */
	if (alpha_length > 0) {
		alpha_tag_decode(data->message_center->name, IoStruct.pbRecvBuffer, GNOKII_MIN(sizeof(data->message_center->name), alpha_length));
		data->message_center->default_name = -1;
	} else {
		/* Set a default name */
		snprintf(data->message_center->name, sizeof(data->message_center->name), _("Set %d"), data->message_center->id);
		/* From include/gnokii/sms.h: set this flag >= 1 if default name used, otherwise -1 */
		data->message_center->default_name = data->message_center->id;
	}

	/* Parameter Indicators */
	parameter_indicators = IoStruct.pbRecvBuffer[alpha_max_len];
	dprintf("Parameter Indicators: 0x%02x\n", parameter_indicators);
	/* Call it empty if no parameter is present */
	if (parameter_indicators == 0xff) return GN_ERR_EMPTYLOCATION;

	/* TP-Destination Address */
	if (parameter_indicators & 0x01) {
		data->message_center->recipient.number[0] = '\0';
	} else {
		snprintf(data->message_center->recipient.number, sizeof(data->message_center->recipient.number), "%s", char_bcd_number_get(IoStruct.pbRecvBuffer + alpha_max_len + 1));
		/* TON - Type of Number */
		data->message_center->recipient.type = IoStruct.pbRecvBuffer[alpha_max_len + 2];
	}

	/* TP-Service Centre Address */
	if (parameter_indicators & 0x02) {
		data->message_center->smsc.number[0] = '\0';
	} else {
		snprintf(data->message_center->smsc.number, sizeof(data->message_center->smsc.number), "%s", char_bcd_number_get(IoStruct.pbRecvBuffer + alpha_max_len + 13));
		/* TON - Type of Number */
		data->message_center->smsc.type = IoStruct.pbRecvBuffer[alpha_max_len + 14];
	}

	/* TP-Protocol Identifier */
	if (parameter_indicators & 0x04) {
		dprintf("\tTP-Protocol Identifier: parameter absent\n");
	} else {
		data->message_center->format = IoStruct.pbRecvBuffer[alpha_max_len + 25];
	}

	/* TP-Data Coding Scheme */
	if (parameter_indicators & 0x08) {
		dprintf("\tTP-Data Coding Scheme: parameter absent\n");
	} else {
		dprintf("\tTP-Data Coding Scheme: 0x%02x\n", IoStruct.pbRecvBuffer[alpha_max_len + 26]);
	}

	/* TP-Validity Period */
	if (parameter_indicators & 0x10) {
		dprintf("\tTP-Validity Period: parameter absent\n");
	} else {
		data->message_center->validity = IoStruct.pbRecvBuffer[alpha_max_len + 27];
	}

	return GN_ERR_NONE;
}

static gn_error GetSMSFolders(gn_data *data, struct gn_statemachine *state)
{
	if (!data || !data->sms_folder_list)
		return GN_ERR_INTERNALERROR;

	memset(data->sms_folder_list, 0, sizeof(gn_sms_folder_list));

	data->sms_folder_list->number = 1;
	snprintf(data->sms_folder_list->folder[0].name, sizeof(data->sms_folder_list->folder[0].name), "%s", gn_memory_type_print(GN_MT_SM));
	data->sms_folder_list->folder_id[0] = GN_MT_SM;
	data->sms_folder_list->folder[0].folder_id = GN_PCSC_FILE_EF_SMS;

	return GN_ERR_NONE;
}

static gn_error GetSMSFolderStatus(gn_data *data, struct gn_statemachine *state)
{
	gn_sms_status sms_status = {0, 0, 0, 0}, *save_sms_status;
	gn_error error = GN_ERR_NONE;

	if (!data || !data->sms_folder)
		return GN_ERR_INTERNALERROR;

	if (data->sms_folder->folder_id != GN_PCSC_FILE_EF_SMS)
		return GN_ERR_INVALIDMEMORYTYPE;

	save_sms_status = data->sms_status;
	data->sms_status = &sms_status;
	error = GetSMSStatus(data, state);
	data->sms_status = save_sms_status;
	if (error != GN_ERR_NONE) return error;

	data->sms_folder->number = sms_status.number;

	return error;
}

static gn_error GetSMSMessage(gn_data *data, struct gn_statemachine *state)
{
/* See Subclause 10.5.3 */
	LONG file;
	LONG ret;
	gn_error error = GN_ERR_NONE;
	unsigned char *tmp;
	unsigned int sms_len;

	if (!data || !data->raw_sms || !data->sms) {
		return GN_ERR_INTERNALERROR;
	}
	if (data->raw_sms->memory_type != GN_MT_SM) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}
	if ((data->raw_sms->number < 1) || (data->raw_sms->number > 255)) {
		return GN_ERR_INVALIDLOCATION;
	}
	file = GN_PCSC_FILE_EF_SMS;

	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	ret = pcsc_read_file_record(&IoStruct, file, data->raw_sms->number);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* expected layout: 1 octet status, N octets number, 177-N octets data */
	if (IoStruct.dwReceived < 178) return GN_ERR_INTERNALERROR;

	/* skip SIM entry status */
	tmp = IoStruct.pbRecvBuffer + 1;
	/* subtract 1 for SIM entry status and 2 for SW */
	sms_len = IoStruct.dwReceived - 3;

	ret = gn_sms_pdu2raw(data->raw_sms, tmp, sms_len, GN_SMS_PDU_DEFAULT);

	/* after-the-fact detection of unrecoverable deleted messages */
	if (ret != GN_ERR_NONE && (IoStruct.pbRecvBuffer[0] & 0x01) == 0)
		return GN_ERR_EMPTYLOCATION;

	switch (IoStruct.pbRecvBuffer[0] & 0x06) {
		case 0: data->raw_sms->status = GN_SMS_Read; break;
		case 2: data->raw_sms->status = GN_SMS_Unread; break;
		case 4: data->raw_sms->status = GN_SMS_Sent; break;
		case 6: data->raw_sms->status = GN_SMS_Unsent; break;
	}

	return get_gn_error(&IoStruct, ret);
}

static gn_error GetSMSStatus(gn_data *data, struct gn_statemachine *state)
{
	LONG file;
	LONG ret;

	if (!data || !data->sms_status) {
		return GN_ERR_INTERNALERROR;
	}
	file = GN_PCSC_FILE_EF_SMS;

	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	if (get_gn_error(&IoStruct, ret) == GN_ERR_NONE) {
		ret = pcsc_stat_file(&IoStruct, file);
		if (get_gn_error(&IoStruct, ret) == GN_ERR_NONE) {
			/* FIXME: this is total space (should read all SMS to check which are empty or unread) */
			data->sms_status->number = (IoStruct.pbRecvBuffer[2] * 256 + IoStruct.pbRecvBuffer[3]) / IoStruct.pbRecvBuffer[14];
			data->sms_status->unread = 0;
		}
	}

	return get_gn_error(&IoStruct, ret);
}

static gn_error DeleteSMSMessage(gn_data *data, struct gn_statemachine *state)
{
	LONG file;
	LONG ret;
	BYTE buf[256];
	int record_length;
	gn_sms_raw *rs;
	gn_error error;
 
	if (!data || !data->raw_sms || !data->sms) {
		return GN_ERR_INTERNALERROR;
	}

	rs = data->raw_sms;
	file = GN_PCSC_FILE_EF_SMS;
	if (data->raw_sms->memory_type != GN_MT_SM) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}
	if (rs->number < 1 || rs->number > 255) {
		return GN_ERR_INVALIDLOCATION;
	}
 
	dprintf("Erasing SMS storage number (%d/%d)\n", rs->memory_type, rs->number);
	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
 
	ret = pcsc_read_file_record(&IoStruct, file, rs->number);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
	if (IoStruct.dwReceived < 178) return GN_ERR_INTERNALERROR;
 
	/* do not check whether the record is already empty; this is
	   intentional to make is possible to wipe deleted records */
 
	record_length = IoStruct.dwReceived - 2;
	if (record_length > 255) return GN_ERR_INTERNALERROR;
	buf[0] = 0x00;
	memset(buf+1, 0xff, record_length-1);
	ret = pcsc_update_file_record(&IoStruct, file, rs->number, buf, record_length);
 
	return get_gn_error(&IoStruct, ret);
}

static gn_error GetFile(gn_data *data, struct gn_statemachine *state)
{
	LONG file_id;
	LONG ret;
	gn_file *file = NULL;
	unsigned char *pos, buf[4];
	gn_error error = GN_ERR_NONE;

	if (!data || !data->file) {
		return GN_ERR_INTERNALERROR;
	}
	file = data->file;

	/* emulated path starts with B: or b: */
	pos = file->name;
	if (*pos != 'B' && *pos != 'b') return GN_ERR_INVALIDLOCATION;
	pos++;
	if (*pos != ':') return GN_ERR_INVALIDLOCATION;
	pos++;
	/* the rest of the path is made of groups of 4 hex digits with a slash or backslash as separator */
	if (strlen(pos) % 5) return GN_ERR_INVALIDLOCATION;

	/* the last group of digits is the file id, the previous groups, if any, are the directories
	   eg: b:/7f10/6f3c */
	while (*pos && error == GN_ERR_NONE) {
		if (*pos != '\\' && *pos != '/') return GN_ERR_INVALIDLOCATION;
		pos++;
		hex2bin(buf, pos, 2);
		file_id = buf[0] * 256 + buf[1];
		pos += 4;
		if (*pos) {
			/* intermediate IDs are treated as directories */
			ret = pcsc_change_dir(&IoStruct, file_id);
			error = get_gn_error(&IoStruct, ret);
		} else {
			PCSC_IOSTRUCT ios;
			/* the last ID is read as a file */
			ret = pcsc_file_get_contents(&ios, file_id);
			error = get_gn_error(&ios, ret);
			/* fill in libgnokii fields */
			snprintf(file->name, sizeof(file->name), "%04lx", file_id);
			file->file = ios.pbRecvBuffer;
			file->file_length = ios.dwReceived - 2;
		}
	}

	return error;
}

static gn_error ReadPhonebook(gn_data *data, struct gn_statemachine *state)
{
	LONG file;
	LONG ret;
	int number_start, alpha_length;
	gn_phonebook_entry *pe;
	gn_error error;

	if (!data || !data->phonebook_entry) return GN_ERR_INTERNALERROR;

	pe = data->phonebook_entry;
	file = get_memory_type(pe->memory_type);
	if (!file) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}
	if (pe->location < 1 || pe->location > 255) {
		return GN_ERR_INVALIDLOCATION;
	}

	dprintf("Reading phonebook location (%d/%d)\n", pe->memory_type, pe->location);
	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	ret = pcsc_read_file_record(&IoStruct, file, pe->location);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* FIXME: number format for EF_ECC is different (BCD without leading length, max 3 octects)
	   this is so previous lines will print useful debug information */
	if (file == GN_PCSC_FILE_EF_ECC) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	/* layout of buffer is: 0..241 octects with alpha tag", 14 octects for number, 2 octects for SW
		(so for EF_ADN subtract 14 for number and 2 for SW) */
		/* FIXME: "14" is not right for all types of numbers (it's 15 for EF_BDN, it's completely different for EF_ECC) */
	if (IoStruct.dwReceived < 16) return GN_ERR_INTERNALERROR;
	number_start = IoStruct.dwReceived - 14 - 2;
	/* mark the entry as empty only if both name and number are missing */
	if ((IoStruct.pbRecvBuffer[0] == 0xff) && (IoStruct.pbRecvBuffer[number_start] == 0xff)) {
		pe->empty = true;
		/* xgnokii checks for empty name and number */
		*pe->name = '\0';
		*pe->number = '\0';
		return GN_ERR_EMPTYLOCATION;
	}

	/* decode and copy the name */
	/* calculate length of the "alpha tag" (ie the text associated to this number) because existing libgnokii functions need it */
	alpha_length = alpha_tag_strlen(IoStruct.pbRecvBuffer, number_start);
	/* better check, since subclause 10.5.1 says length of alpha tag is 0..241 */
	if (alpha_length > GN_PHONEBOOK_NAME_MAX_LENGTH) {
		dprintf("WARNING: Phonebook name too long, %d will be truncated to %d\n", alpha_length, GN_PHONEBOOK_NAME_MAX_LENGTH);
		alpha_length = GN_PHONEBOOK_NAME_MAX_LENGTH;
	}
	alpha_tag_decode(pe->name, IoStruct.pbRecvBuffer, alpha_length);

	/* decode and copy the number */
	snprintf(pe->number, GN_PHONEBOOK_NUMBER_MAX_LENGTH, "%s", char_bcd_number_get((u8 *)&IoStruct.pbRecvBuffer[number_start]));
	if (IoStruct.pbRecvBuffer[IoStruct.dwReceived - 3] != 0xff) {
		/* TODO: handle the extension identifier field for numbers or SSCs longer than 20 digits (do they really exist in the wild?) */
		dprintf("WARNING: Ignoring extension identifier field\n");
	}
	/* fill in other libgnokii fields */
	pe->subentries_count = 0;
	pe->caller_group = GN_PHONEBOOK_GROUP_None;

	return get_gn_error(&IoStruct, ret);
}

static gn_error DeletePhonebook(gn_data *data, struct gn_statemachine *state)
{
	LONG file;
	LONG ret;
	BYTE buf[256];
	int record_length;
	gn_phonebook_entry *pe;
	gn_error error;
 
	if (!data || !data->phonebook_entry) return GN_ERR_INTERNALERROR;
 
	pe = data->phonebook_entry;
	file = get_memory_type(pe->memory_type);
	if (!file) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}
	if (pe->location < 1 || pe->location > 255) {
		return GN_ERR_INVALIDLOCATION;
	}
 
	dprintf("Erasing phonebook location (%d/%d)\n", pe->memory_type, pe->location);
	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
 
	ret = pcsc_read_file_record(&IoStruct, file, pe->location);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
	if (IoStruct.dwReceived < 16) return GN_ERR_INTERNALERROR;
 
	/* do not check whether the record is already empty; this is
	   intentional to make is possible to wipe deleted records */
 
	if (IoStruct.pbRecvBuffer[IoStruct.dwReceived - 3] != 0xff) {
		dprintf("Cannot erase a record with an extension field\n");
		return GN_ERR_INTERNALERROR;
	}
 
	record_length = IoStruct.dwReceived - 2;
	if (record_length > 255) return GN_ERR_INTERNALERROR;
	memset(buf, 0xff, record_length);
	ret = pcsc_update_file_record(&IoStruct, file, pe->location, buf, record_length);
 
	return get_gn_error(&IoStruct, ret);
}

#ifdef SECURITY

static gn_error GetSecurityCodeStatus(gn_data *data, struct gn_statemachine *state)
{
	LONG ret;
	gn_error error;
	BYTE fchars, chv1st;
 
	data->security_code->type = GN_SCT_None;
 
	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
	ret = pcsc_stat_file(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
 
	if (IoStruct.dwReceived < 22) return GN_ERR_INTERNALERROR;
	fchars = IoStruct.pbRecvBuffer[13];
	chv1st = IoStruct.pbRecvBuffer[18];
	dprintf("File chars 0x%02x CHV1 status 0x%02x\n", fchars, chv1st);
	if ((fchars & 0x80) == 0) {
		/* CHV1 enabled */
		if ((chv1st & 0x0f) == 0) {
			/* CHV1 blocked */
			data->security_code->type = GN_SCT_Puk;
		}
		else {
			ret = pcsc_read_file_record(&IoStruct, GN_PCSC_FILE_EF_FDN, 1);
			error = get_gn_error(&IoStruct, ret);
			if (error == GN_ERR_CODEREQUIRED)
				/* CHV1 required */
				data->security_code->type = GN_SCT_Pin;
			else if (error != GN_ERR_NONE)
				return error;
		}
	}
 
	return GN_ERR_NONE;
}
 
static gn_error EnterSecurityCode(gn_data *data, struct gn_statemachine *state)
{
	LONG ret;
	gn_error error;
	BYTE chv_id;
	size_t chv_len;
 
	switch (data->security_code->type) {
	case GN_SCT_Pin:
		chv_id = 1;
		break;
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
 
	chv_len = strlen(data->security_code->code);
	if (chv_len > 8)
		return GN_ERR_INVALIDSECURITYCODE;
 
	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;
 
	ret = pcsc_verify_chv(&IoStruct, chv_id,
			      (BYTE *)data->security_code->code, chv_len);
	error = get_gn_error(&IoStruct, ret);
	if (error == GN_ERR_CODEREQUIRED)
		error = GN_ERR_INVALIDSECURITYCODE;
	return error;
}

#endif /* SECURITY */


/* functions for libpcsc stuff */

static LONG pcsc_cmd_get_response(PCSC_IOSTRUCT *ios, BYTE length)
{
/* fetch the data that the card has prepared */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_GET_RESPONSE, 0x00, 0x00, 0x00 };

	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P3] = length;
	ios->dwReceived = ios->dwRecvLength;

	PCSC_TRANSMIT(ret, "cmd_get_response", pbSendBuffer, ios);

	return ret;
}

static LONG pcsc_cmd_select(PCSC_IOSTRUCT *ios, LONG file_id)
{
/* this is similar to open() or chdir(), depending on the file id */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_SELECT, 0x00, 0x00, 0x02, 0x00, 0x00 };

	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_FILEID_HI] = (file_id >> 8) & 0xff;
	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_FILEID_LO] = file_id & 0xff;
	ios->dwReceived = ios->dwRecvLength;

	PCSC_TRANSMIT(ret, "cmd_select", pbSendBuffer, ios);

	return ret;
}

static LONG pcsc_change_dir(PCSC_IOSTRUCT *ios, LONG directory_id)
{
/* select the specified directory */

	/* checking if it is really a "Directory File" would require getting the response to check the type of file e.g. using pcsc_stat_file() */
	return pcsc_cmd_select(ios, directory_id);
}

static LONG pcsc_stat_file(PCSC_IOSTRUCT *ios, LONG file_id)
{
/* fill the buffer with file information */
	LONG ret;

	ret = pcsc_cmd_select(ios, file_id);
	if (ret != SCARD_S_SUCCESS) return ret;

	/* "soft" failure that needs to be catched by the caller */
	if (ios->pbRecvBuffer[ios->dwReceived - 2] != 0x9f) {
		return SCARD_S_SUCCESS;
	}
	/* get file structure and length */
	ret = pcsc_cmd_get_response(ios, ios->pbRecvBuffer[1]);
	if (ret != SCARD_S_SUCCESS) return ret;

	/* short reads should never happen (minimum lengths taken from subclause 9.2.1) */
	switch (ios->pbRecvBuffer[6]) {
	case GN_PCSC_FILE_TYPE_MF:
	case GN_PCSC_FILE_TYPE_DF:
		if (ios->dwReceived < 22) ret = SCARD_F_UNKNOWN_ERROR;
		break;
	case GN_PCSC_FILE_TYPE_EF:
		if (ios->dwReceived < 14) ret = SCARD_F_UNKNOWN_ERROR;
		break;
	default:
		ret = SCARD_E_INVALID_PARAMETER;
	}
	return ret;
}

static LONG pcsc_cmd_read_binary(PCSC_IOSTRUCT *ios, LONG length)
{
/* copy a smart card "transparent file" contents to the provided buffer */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_READ_BINARY, 0x00, 0x00, 0x00 };

	/* TODO: read files longer than 255 bytes (not needed for phonebook or SMS) */
	if ((length > 255) || (length > ios->dwRecvLength)) {
		ret = SCARD_E_INVALID_PARAMETER;
		dprintf("%s error %lX: %s\n", __FUNCTION__, ret, pcsc_stringify_error(ret));
		return ret;
	}

	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P3] = length;
	ios->dwReceived = ios->dwRecvLength;

	PCSC_TRANSMIT(ret, "cmd_read_binary", pbSendBuffer, ios);

	return ret;
}

static LONG pcsc_cmd_read_record(PCSC_IOSTRUCT *ios, BYTE record, BYTE length)
{
/* copy a smart card "linear fixed file" contents to the provided buffer */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_READ_RECORD, 0x00, GN_PCSC_FILE_READ_ABS, 0x00 };

	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P1] = record;
	/* pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P2] = GN_PCSC_FILE_READ_ABS; */ /* always use absolute/current mode */
	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P3] = length;
	ios->dwReceived = ios->dwRecvLength;

	PCSC_TRANSMIT(ret, "cmd_read_record", pbSendBuffer, ios);

	return ret;
}

static LONG pcsc_file_get_contents(PCSC_IOSTRUCT *ios, LONG file_id)
{
/* copy file contents in a newly allocated buffer which must be freed by the caller
Note that this function overwrites ios->pbRecvBuffer and ios->dwRecvLength
*/
	LONG ret;
	BYTE bRecordLength, bRecordCount, *pbFileBuffer, bFileStructure;
	DWORD dwFileLength;

	/* allocate a buffer for the "stat" command */
	ios->dwRecvLength = MAX_BUFFER_SIZE;
	ios->pbRecvBuffer = malloc(ios->dwRecvLength);  /* FIXME this memory is leaked after the following errors */
	if (!ios->pbRecvBuffer) return SCARD_E_NO_MEMORY;

	ret = pcsc_stat_file(ios, file_id);
	if (ret != SCARD_S_SUCCESS) return ret;
	/* "soft" failure that needs to be catched by the caller */
	if (ios->pbRecvBuffer[ios->dwReceived - 2] != 0x90) return SCARD_S_SUCCESS;

	switch (ios->pbRecvBuffer[6]) {
	case GN_PCSC_FILE_TYPE_MF:
	case GN_PCSC_FILE_TYPE_DF:
		/* reuse the buffer even if it's bigger than needed */
		dprintf("file %04x length %d\n", file_id, ios->dwReceived - 2);
		return SCARD_S_SUCCESS;
	case GN_PCSC_FILE_TYPE_EF:
		break;
	default:
		return SCARD_E_INVALID_PARAMETER;
	}

	dwFileLength = ios->pbRecvBuffer[2] * 256 + ios->pbRecvBuffer[3];
	bFileStructure = ios->pbRecvBuffer[13];
	if (ios->dwReceived <= 15) return SCARD_F_UNKNOWN_ERROR;
	bRecordLength = ios->pbRecvBuffer[14];
	dprintf("file %04x length %d\n", file_id, dwFileLength);
	/* 2 extra bytes are needed to store the last SW (the previous SWs are overwritten by the following reads)
	   having the SW after file contents is consistent with the other functions */
	free(ios->pbRecvBuffer);
	ios->pbRecvBuffer = malloc(dwFileLength + 2);
	if (!ios->pbRecvBuffer) return SCARD_E_NO_MEMORY;

	switch (bFileStructure) {
	case GN_PCSC_FILE_STRUCTURE_TRANSPARENT:
		ret = pcsc_cmd_read_binary(ios, dwFileLength);
		break;
	case GN_PCSC_FILE_STRUCTURE_CYCLIC:
	case GN_PCSC_FILE_STRUCTURE_LINEAR_FIXED:
		bRecordCount = dwFileLength / bRecordLength;
		pbFileBuffer = ios->pbRecvBuffer;
		ios->bRecordNumber = 1;
		while (bRecordCount-- && ret == SCARD_S_SUCCESS) {
			ret = pcsc_cmd_read_record(ios, ios->bRecordNumber++, bRecordLength);
			ios->pbRecvBuffer += bRecordLength;
		}
		/* pretend we filled the buffer in one pass */
		ios->pbRecvBuffer = pbFileBuffer;
		/* use the actual number of records read */
		ios->dwReceived = (ios->bRecordNumber - 1) * bRecordLength + 2;
		break;
	default:
		ret = SCARD_E_INVALID_PARAMETER;
	}

	if (ret != SCARD_S_SUCCESS) {
		dprintf("%s error %lX: %s\n", __FUNCTION__, ret, pcsc_stringify_error(ret));
	}

	return ret;
}

static LONG pcsc_read_file_record(PCSC_IOSTRUCT *ios, LONG file_id, BYTE record)
{
/* copy contents of a record from a file to the provided buffer */
	LONG ret;

	ret = pcsc_stat_file(ios, file_id);
	if (ret != SCARD_S_SUCCESS) return ret;
	/* "soft" failure that needs to be catched by the caller */
	if (ios->pbRecvBuffer[ios->dwReceived - 2] != 0x90) return SCARD_S_SUCCESS;

	/* records can only be read from EFs */
	if (ios->pbRecvBuffer[6] != GN_PCSC_FILE_TYPE_EF) return SCARD_E_INVALID_PARAMETER;

	switch (ios->pbRecvBuffer[13]) {
	case GN_PCSC_FILE_STRUCTURE_CYCLIC:
	case GN_PCSC_FILE_STRUCTURE_LINEAR_FIXED:
		if (ios->dwReceived <= 14) return SCARD_F_UNKNOWN_ERROR;
		ret = pcsc_cmd_read_record(ios, record, ios->pbRecvBuffer[14]);
		break;
	default:
		ret = SCARD_E_INVALID_PARAMETER;
	}

	if (ret != SCARD_S_SUCCESS) {
		dprintf("%s error %lX: %s\n", __FUNCTION__, ret, pcsc_stringify_error(ret));
	}

	return ret;
}

static LONG pcsc_read_file(PCSC_IOSTRUCT *ios, LONG dir_id, LONG file_id)
{
/* copy contents of a file to the provided buffer */
	LONG ret;
	DWORD dwFileLength;
	BYTE bRecordLength, bRecordCount, *pbFileBuffer, bFileStructure;

	ret = pcsc_change_dir(ios, dir_id);
	if (ret != SCARD_S_SUCCESS) return ret;

	ret = pcsc_stat_file(ios, file_id);
	if (ret != SCARD_S_SUCCESS) return ret;

	dwFileLength = ios->pbRecvBuffer[2] * 256 + ios->pbRecvBuffer[3];
	bFileStructure = ios->pbRecvBuffer[13];

	switch (bFileStructure) {
	case GN_PCSC_FILE_STRUCTURE_TRANSPARENT:
		ret = pcsc_cmd_read_binary(ios, dwFileLength);
		break;
	case GN_PCSC_FILE_STRUCTURE_CYCLIC:
	case GN_PCSC_FILE_STRUCTURE_LINEAR_FIXED:
		bRecordLength = ios->pbRecvBuffer[14];
		bRecordCount = dwFileLength / bRecordLength;
		pbFileBuffer = ios->pbRecvBuffer;
		ios->bRecordNumber = 1;
		while (bRecordCount-- && ret == SCARD_S_SUCCESS) {
			ret = pcsc_cmd_read_record(ios, ios->bRecordNumber++, bRecordLength);
			ios->pbRecvBuffer += bRecordLength;
		}
		/* pretend we filled the buffer in one pass */
		ios->pbRecvBuffer = pbFileBuffer;
		/* use the actual number of records read */
		ios->dwReceived = (ios->bRecordNumber - 1) * bRecordLength + 2;
		break;
	default:
		ret = SCARD_E_INVALID_PARAMETER;
	}

	if (ret != SCARD_S_SUCCESS) {
		dprintf("%s error %lX: %s\n", __FUNCTION__, ret, pcsc_stringify_error(ret));
	}

	return ret;
}

static LONG pcsc_cmd_update_record(PCSC_IOSTRUCT *ios, BYTE record, BYTE *data, BYTE length)
{
/* update a smart card "linear fixed file" contents from the provided buffer */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[GN_PCSC_DATA_OFFSET + 256] = { GN_PCSC_CMD_UPDATE_RECORD, 0x00, GN_PCSC_FILE_READ_ABS, 0x00 };
 
	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P1] = record;
	/* pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P2] = GN_PCSC_FILE_READ_ABS; */ /* always
use absolute/current mode */
	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P3] = length;
	memcpy(&pbSendBuffer[GN_PCSC_DATA_OFFSET], data, length);
	ios->dwReceived = ios->dwRecvLength;
 
	PCSC_TRANSMIT_WITH_LENGTH(ret, "cmd_update_record", pbSendBuffer, GN_PCSC_DATA_OFFSET + length, ios);
 
	return ret;
}

static LONG pcsc_update_file_record(PCSC_IOSTRUCT *ios, LONG file_id, BYTE record, BYTE *data, BYTE length)
{
/* copy contents of a record from the provided buffer to a file */
	LONG ret;
 
	ret = pcsc_stat_file(ios, file_id);
	if (ret != SCARD_S_SUCCESS) return ret;
	if (ios->dwReceived < 2) return SCARD_F_UNKNOWN_ERROR;
	/* "soft" failure that needs to be catched by the caller */
	if (ios->pbRecvBuffer[ios->dwReceived - 2] != 0x90) return SCARD_S_SUCCESS;
 
	/* records can only be updated in EFs */
	if (ios->dwReceived <= 6) return SCARD_F_UNKNOWN_ERROR;
	if (ios->pbRecvBuffer[6] != GN_PCSC_FILE_TYPE_EF) return SCARD_E_INVALID_PARAMETER;
 
	if (ios->dwReceived <= 13) return SCARD_F_UNKNOWN_ERROR;
	switch (ios->pbRecvBuffer[13]) {
	case GN_PCSC_FILE_STRUCTURE_CYCLIC:
	case GN_PCSC_FILE_STRUCTURE_LINEAR_FIXED:
		if (length != ios->pbRecvBuffer[14]) {
			ret = SCARD_E_INVALID_PARAMETER;
			break;
		}
		ret = pcsc_cmd_update_record(ios, record, data, length);
		break;
	default:
		ret = SCARD_E_INVALID_PARAMETER;
	}
 
	if (ret != SCARD_S_SUCCESS) {
		dprintf("%s error %lX: %s\n", __FUNCTION__, ret, pcsc_stringify_error(ret));
	}
 
	return ret;
}

#ifdef SECURITY

static LONG pcsc_verify_chv(PCSC_IOSTRUCT *ios, BYTE chv_id, BYTE *chv, BYTE chv_len)
{
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_VERIFY_CHV, 0x00, 0x00, 0x08,
				 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
 
	if (chv_len > 8)
		return SCARD_E_INVALID_PARAMETER;
 
	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P2] = chv_id;
	memcpy(&pbSendBuffer[GN_PCSC_DATA_OFFSET], chv, chv_len);
	ios->dwReceived = ios->dwRecvLength;
 
	PCSC_TRANSMIT(ret, "cmd_verify_chv", pbSendBuffer, ios);
 
	return ret;
}

#endif /* SECURITY */

static LONG pcsc_close_reader()
{
/* disconnect from reader */

	/* if PIN has been successfully entered, SCARD_LEAVE_CARD leaves the card unprotected
	   until SCARD_UNPOWER_CARD or SCARD_RESET_CARD are used or it is removed from reader
	*/
	return SCardDisconnect(hCard, SCARD_LEAVE_CARD);
}

static LONG pcsc_open_reader_number(LONG number)
{
/* connect to a reader by number, 0 is the first reader (as configured in /etc/reader.conf) */
	LONG ret;
	LONG len;
	BYTE *buf;
	LPCSTR reader_name;

	/* first retrieve buffer length */
	ret = SCardListReaders(hContext, NULL, NULL, &len);
	buf = (BYTE *) malloc(len);
	if (!buf) {
		return SCARD_E_NO_MEMORY;
	}
	/* then fill in the buffer just allocated */
	ret = SCardListReaders(hContext, NULL, buf, &len);
	if (ret == SCARD_S_SUCCESS) {
		reader_name = buf;
		/* print names because libpcsclite appends some figures to the names in the config file */
		dprintf("pcsc_reader_name = \"%s\"\n", reader_name);
		while (--len && number) {
			if (!*reader_name++) {
				number--;
				dprintf("pcsc_reader_name = \"%s\"\n", reader_name);
			}
		}
		/* reader_name points to the n-th reader name or to '\0' if there aren't enough names */
		if (*reader_name) {
			ret = pcsc_open_reader_name(reader_name);
		} else {
			ret = SCARD_E_UNKNOWN_READER;
		}
	}
	free(buf);

	return ret;
}

static LONG pcsc_open_reader_name(LPCSTR reader_name)
{
/* connect to a reader by name, case sensitive (as configured in /etc/reader.conf) */
	LONG ret;

	dprintf("Connecting to reader \"%s\"\n", reader_name);
	dwActiveProtocol = -1;
	ret = SCardConnect(hContext, reader_name, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
	if (ret != SCARD_S_SUCCESS) {
		dprintf("SCardConnect error %lX: %s\n", ret, pcsc_stringify_error(ret));
		pcsc_close_reader();
		return ret;
	}

	switch(dwActiveProtocol) {
	case SCARD_PROTOCOL_T0:
		pioSendPci = SCARD_PCI_T0;
		break;
	case SCARD_PROTOCOL_T1:
		pioSendPci = SCARD_PCI_T1;
		break;
	default:
		/* Unknown protocol */
		pcsc_close_reader();
		return SCARD_E_PROTO_MISMATCH;
	}

	return ret;
}

#endif /* HAVE_PCSC */
