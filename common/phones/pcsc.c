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

#include "config.h"

#ifdef HAVE_PCSC

#include "phones/pcsc.h"

/* prototypes for functions related to libgnokii */

static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state);
static gn_error Identify(gn_data *data, struct gn_statemachine *state);
static gn_error Initialise(struct gn_statemachine *state);
static gn_error ReadPhonebook(gn_data *data, struct gn_statemachine *state);
static gn_error Terminate(gn_data *data, struct gn_statemachine *state);
static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state);

/* prototypes for functions related to libpcsclite */

static LONG pcsc_change_dir(PCSC_IOSTRUCT *ios, LONG directory_id);
static LONG pcsc_close_reader();
static LONG pcsc_cmd_get_response(PCSC_IOSTRUCT *ios, BYTE length);
static LONG pcsc_cmd_read_binary(PCSC_IOSTRUCT *ios, LONG length);
static LONG pcsc_cmd_read_record(PCSC_IOSTRUCT *ios, BYTE record, BYTE length);
static LONG pcsc_cmd_select(PCSC_IOSTRUCT *ios, LONG file_id);
static LONG pcsc_open_reader_name(LPCSTR reader_name);
static LONG pcsc_open_reader_number(LONG number);
static LONG pcsc_read_file(PCSC_IOSTRUCT *ios, LONG file_id);
static LONG pcsc_stat_file(PCSC_IOSTRUCT *ios, LONG file_id);

/* prototypes for functions converting between the two libraries */

static gn_error get_gn_error(PCSC_IOSTRUCT *ios, LONG ret);
static LONG get_memory_type(gn_memory_type memory_type);
static gn_error ios2gn_error(PCSC_IOSTRUCT *ios);

/* Some globals */

gn_driver driver_pcsc = {
	NULL,
	pgen_incoming_default,
	/* Mobile phone information */
	{
		"pcsc",                /* Supported models */
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
BYTE buf[256];
PCSC_IOSTRUCT IoStruct = { buf, sizeof(buf), 0, 0 };

#ifdef DEBUG
#define DUMP_BUFFER(ret, str, buf, len) pcsc_dump_buffer(ret, __FUNCTION__, str, buf, len)

void pcsc_dump_buffer(LONG ret, char *func, char *str, BYTE *buf, LONG len) {
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

#define PCSC_TRANSMIT(ret, string, pbSendBuffer, ios) DUMP_BUFFER(ret, "Sending " string ": ", pbSendBuffer, sizeof(pbSendBuffer)); \
	ret = SCardTransmit(hCard, pioSendPci, pbSendBuffer, sizeof(pbSendBuffer), &pioRecvPci, ios->pbRecvBuffer, &ios->dwReceived); \
	DUMP_BUFFER(ret, "Received: ", ios->pbRecvBuffer, ios->dwReceived);

/* helper functions */

static gn_error ios2gn_error(PCSC_IOSTRUCT *ios) {
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

static gn_error get_gn_error(PCSC_IOSTRUCT *ios, LONG ret) {
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
	case SCARD_W_INSERTED_CARD:
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

static LONG get_memory_type(gn_memory_type memory_type) {
/* Convert a libgnokii memory_type into a GSM file ID */
	switch (memory_type) {
	case GN_MT_SM:
		return GN_PCSC_FILE_EF_ADN;
	case GN_MT_EN:
		return GN_PCSC_FILE_EF_ECC;
	case GN_MT_FD:
		return GN_PCSC_FILE_EF_FDN;
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

/* functions for libgnokii stuff */

static gn_error functions(gn_operation op, gn_data *data, struct gn_statemachine *state) {
	switch (op) {
	case GN_OP_Init:
		return Initialise(state);
	case GN_OP_Terminate:
		return Terminate(data, state);
	case GN_OP_Identify:
		return Identify(data, state);
	case GN_OP_GetMemoryStatus:
		return GetMemoryStatus(data, state);
	case GN_OP_ReadPhonebook:
		return ReadPhonebook(data, state);
	default:
		return GN_ERR_NOTIMPLEMENTED;
	}
	return GN_ERR_INTERNALERROR;
}

/* Initialise is the only function allowed to 'use' state */
static gn_error Initialise(struct gn_statemachine *state) {
	LONG ret;
	LONG reader_number;
	char *aux = NULL;

	dprintf("Initializing...\n");

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

static gn_error Terminate(gn_data *data, struct gn_statemachine *state) {
	LONG ret;

	/* we're going to terminate even if this call returns error */
	(void) pcsc_close_reader();

	ret = SCardReleaseContext(hContext);
	if (ret != SCARD_S_SUCCESS) {
		dprintf("SCardReleaseContext error %lX: %s\n", ret, pcsc_stringify_error(ret));
	}

	return get_gn_error(NULL, ret);
}

static gn_error Identify(gn_data *data, struct gn_statemachine *state) {
	SAFE_STRNCPY(data->model, "pcsc", GN_MODEL_MAX_LENGTH);

	return GN_ERR_NONE;
}

static gn_error GetMemoryStatus(gn_data *data, struct gn_statemachine *state) {
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

static gn_error ReadPhonebook(gn_data *data, struct gn_statemachine *state) {
	LONG file;
	LONG ret;
	BYTE *buf, *temp;
	int i, number_start, alpha_length, ucs2_base;
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
	IoStruct.bRecordNumber = pe->location;

	dprintf("Reading phonebook location (%d/%d)\n", pe->memory_type, pe->location);
	ret = pcsc_change_dir(&IoStruct, GN_PCSC_FILE_DF_TELECOM);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	ret = pcsc_read_file(&IoStruct, file);
	error = get_gn_error(&IoStruct, ret);
	if (error != GN_ERR_NONE) return error;

	/* FIXME: number format for EF_ECC is different (BCD without leading length, max 3 octects)
	   this is so previous lines will print useful debug information */
	if (file == GN_PCSC_FILE_EF_ECC) {
		return GN_ERR_INVALIDMEMORYTYPE;
	}

	/* layout of buffer is: 0..241 octects with alpha tag", 14 octects for number, 2 octects for SW
		(so for EF_ADN subtract 14 for number and 2 for SW) */
	/* FIXME: "14" is not right for all types of numbers (eg EF_ECC) */
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
	/* calculate length of "alpha tag" (ie the text associated to this number) */
	buf = IoStruct.pbRecvBuffer;
	switch (*buf) {
	case 0x80:
		/* UCS2 alphabet */
		buf++; /* skip 0x80 */
		alpha_length = 0;
		while ((*buf++ != 0xff) && (alpha_length < number_start)) {
			alpha_length++;
		}
		break;
	case 0x81:
	case 0x82:
		/* UCS2 alphabet with different coding schemes */
		alpha_length = buf[1];
		break;
	default:
		/* GSM Default Alphabet */
		alpha_length = 0;
		while ((*buf++ != 0xff) && (alpha_length < number_start)) {
			alpha_length++;
		}
	}
	/* better check, since subclause 10.5.1 says length of alpha tag is 0..241 */
	if (alpha_length > GN_PHONEBOOK_NAME_MAX_LENGTH) {
		dprintf("WARNING: Phonebook name too long, %d will be truncated to %d\n", alpha_length, GN_PHONEBOOK_NAME_MAX_LENGTH);
		alpha_length = GN_PHONEBOOK_NAME_MAX_LENGTH;
	}
	/* handle the encoding according to Annex B of ETSI TS 100 977 */
	buf = IoStruct.pbRecvBuffer;
	switch (*buf) {
	case 0x80:
		/* UCS2 alphabet */
		buf++;
		char_unicode_decode(pe->name, buf, alpha_length * 2);
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
			return GN_ERR_MEMORYFULL;
		}
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
		char_unicode_decode(pe->name, temp, alpha_length * 2);
		free(temp);
		break;
	default:
		/* GSM Default Alphabet */
		i = 0;
		while ((*buf != 0xff) && (i < alpha_length)) {
			pe->name[i++] = char_def_alphabet_decode(*buf++);
		}
		pe->name[i] = '\0';
	}
	/* decode and copy the number */
	SAFE_STRNCPY(pe->number, char_bcd_number_get((u8 *)&IoStruct.pbRecvBuffer[number_start]), GN_PHONEBOOK_NUMBER_MAX_LENGTH);
	if (IoStruct.pbRecvBuffer[IoStruct.dwReceived - 3] != 0xff) {
		/* TODO: handle the extension identifier field for numbers or SSCs longer than 20 digits (do they really exist in the wild?) */
		dprintf("WARNING: Ignoring extension identifier field\n");
	}
	/* fill in other libgnokii fields */
	pe->subentries_count = 0;
	pe->caller_group = GN_PHONEBOOK_GROUP_None;

	return get_gn_error(&IoStruct, ret);
}

/* functions for libpcsc stuff */

static LONG pcsc_cmd_get_response(PCSC_IOSTRUCT *ios, BYTE length) {
/* fetch the data that the card has prepared */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_GET_RESPONSE, 0x00, 0x00, 0x00 };

	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_P3] = length;
	ios->dwReceived = ios->dwRecvLength;

	PCSC_TRANSMIT(ret, "cmd_get_response", pbSendBuffer, ios);

	return ret;
}

static LONG pcsc_cmd_select(PCSC_IOSTRUCT *ios, LONG file_id) {
/* this is similar to open() or chdir(), depending on the file id */
	LONG ret = SCARD_S_SUCCESS;
	BYTE pbSendBuffer[] = { GN_PCSC_CMD_SELECT, 0x00, 0x00, 0x02, 0x00, 0x00 };

	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_FILEID_HI] = (file_id >> 8) & 0xff;
	pbSendBuffer[GN_PCSC_PARAMETER_OFFSET_FILEID_LO] = file_id & 0xff;
	ios->dwReceived = ios->dwRecvLength;

	PCSC_TRANSMIT(ret, "cmd_select", pbSendBuffer, ios);

	/* this should never happen */
	if ((ret == SCARD_S_SUCCESS) && (ios->dwReceived < 2)) ret = SCARD_F_UNKNOWN_ERROR;

	return ret;
}

static LONG pcsc_change_dir(PCSC_IOSTRUCT *ios, LONG directory_id) {
/* select the specified directory */

	/* checking if it is really a "Directory File" would require getting the response to check the type of file e.g. using pcsc_stat_file() */
	return pcsc_cmd_select(ios, directory_id);
}

static LONG pcsc_stat_file(PCSC_IOSTRUCT *ios, LONG file_id) {
/* fill the buffer with file information */
	LONG ret;

	ret = pcsc_cmd_select(ios, file_id);
	if (ret != SCARD_S_SUCCESS) {
		return ret;
	}
	/* "soft" failure that needs to be catched by the caller */
	if (ios->pbRecvBuffer[ios->dwReceived - 2] != 0x9f) {
		return SCARD_S_SUCCESS;
	}
	/* get file structure and length */
	return pcsc_cmd_get_response(ios, ios->pbRecvBuffer[1]);
}

static LONG pcsc_cmd_read_binary(PCSC_IOSTRUCT *ios, LONG length) {
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

static LONG pcsc_cmd_read_record(PCSC_IOSTRUCT *ios, BYTE record, BYTE length) {
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

static LONG pcsc_read_file(PCSC_IOSTRUCT *ios, LONG file_id) {
/* copy file contents to the provided buffer */
	LONG ret;

	ret = pcsc_stat_file(ios, file_id);
	if (ret != SCARD_S_SUCCESS) return ret;
	if (ios->dwReceived < 2) return SCARD_F_UNKNOWN_ERROR;
	/* "soft" failure that needs to be catched by the caller */
	if (ios->pbRecvBuffer[ios->dwReceived - 2] != 0x90) return SCARD_S_SUCCESS;

	/* TODO: handle GN_PCSC_FILE_TYPE_MF and GN_PCSC_FILE_TYPE_DF */
	if (ios->dwReceived <= 6) return SCARD_F_UNKNOWN_ERROR;
	if (ios->pbRecvBuffer[6] != GN_PCSC_FILE_TYPE_EF) return SCARD_E_INVALID_PARAMETER;

	if (ios->dwReceived <= 13) return SCARD_F_UNKNOWN_ERROR;
	switch (ios->pbRecvBuffer[13]) {
	case GN_PCSC_FILE_STRUCTURE_TRANSPARENT:
		ret = pcsc_cmd_read_binary(ios, ios->pbRecvBuffer[2] * 256 + ios->pbRecvBuffer[3]);
		break;
	case GN_PCSC_FILE_STRUCTURE_LINEAR_FIXED:
		ret = pcsc_cmd_read_record(ios, ios->bRecordNumber, ios->pbRecvBuffer[14]);
		break;
	default:
		ret = SCARD_E_INVALID_PARAMETER;
	}

	if (ret != SCARD_S_SUCCESS) {
		dprintf("%s error %lX: %s\n", __FUNCTION__, ret, pcsc_stringify_error(ret));
	}

	return ret;
}

static LONG pcsc_close_reader() {
/* disconnect from reader */
	return SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
}

static LONG pcsc_open_reader_number(LONG number) {
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

static LONG pcsc_open_reader_name(LPCSTR reader_name) {
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
