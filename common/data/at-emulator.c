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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001-2004  Pawel Kot
  Copyright (C) 2002-2004  BORBELY Zoltan

  This file provides a virtual modem or "AT" interface to the GSM phone by
  calling code in gsm-api.c. Inspired by and in places copied from the Linux
  kernel AT Emulator IDSN code by Fritz Elfert and others.

*/

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#ifndef WIN32
#  include <termios.h>
#endif

#include "misc.h"
#include "gnokii.h"
#include "compat.h"
#include "data/at-emulator.h"
#include "data/datapump.h"

#define MAX_LINE_LENGTH 256

#define	MAX_CMD_BUFFERS	(2)
#define	CMD_BUFFER_LENGTH (100)

/* Definition of some special Registers of AT-Emulator, pinched in
   part from ISDN driver in Linux kernel */
#define REG_RINGATA   0
#define REG_RINGCNT   1
#define REG_ESC       2
#define REG_CR        3
#define REG_LF        4
#define REG_BS        5
#define	S22          22
#define S35          35
#define REG_CTRLZ   100
#define REG_ESCAPE  101

#define REG_QUIET    14
#define BIT_QUIET     4
#define REG_VERBOSE  14
#define BIT_VERBOSE   8
#define REG_ECHO     14
#define BIT_ECHO      2


#define	MAX_MODEM_REGISTERS	102

/* Message format definitions */
#define PDU_MODE      0
#define TEXT_MODE     1
#define INTERACT_MODE 2

/* Global variables */
bool gn_atem_initialised = false;	/* Set to true once initialised */
extern bool CommandMode;
extern int ConnectCount;

struct gn_statemachine *sm;
gn_data data;

static gn_sms sms;
static gn_call_info callinfo;
static char imei[GN_IMEI_MAX_LENGTH], model[GN_MODEL_MAX_LENGTH], revision[GN_REVISION_MAX_LENGTH], manufacturer[GN_MANUFACTURER_MAX_LENGTH];

/* Local variables */
static int	PtyRDFD;	/* File descriptor for reading and writing to/from */
static int	PtyWRFD;	/* pty interface - only different in debug mode. */

static u8	ModemRegisters[MAX_MODEM_REGISTERS];
static char	CmdBuffer[MAX_CMD_BUFFERS][CMD_BUFFER_LENGTH];
static int	CurrentCmdBuffer;
static int	CurrentCmdBufferIndex;
static int	IncomingCallNo;
static int     MessageFormat;          /* Message Format (text or pdu) */
static int	CallerIDMode;

	/* Current command parser */
static void 	(*Parser)(char *); /* Current command parser */
/* void 	(*Parser)(char *) = gn_atem_at_parse; */

static gn_memory_type 	SMSType;
static int 	SMSNumber;

/* If initialised in debug mode, stdin/out is used instead
   of ptys for interface. */
bool gn_atem_initialise(int read_fd, int write_fd, struct gn_statemachine *vmsm)
{
	PtyRDFD = read_fd;
	PtyWRFD = write_fd;

	gn_data_clear(&data);
	memset(&sms, 0, sizeof(sms));
	memset(&callinfo, 0, sizeof(callinfo));

	data.sms = &sms;
	data.call_info = &callinfo;
	data.manufacturer = manufacturer;
	data.model = model;
	data.revision = revision;
	data.imei = imei;

	sm = vmsm;

	/* Initialise command buffer variables */
	CurrentCmdBuffer = 0;
	CurrentCmdBufferIndex = 0;

	/* Initialise registers */
	gn_atem_registers_init();

	/* Initial parser is AT routine */
	Parser = gn_atem_at_parse;

	/* Setup defaults for AT*C interpreter. */
	SMSNumber = 1;
	SMSType = GN_MT_ME;

	/* Default message format is PDU */
	MessageFormat = PDU_MODE;

	/* Set the call passup so that we get notified of incoming calls */
	data.call_notification = gn_atem_call_passup;
	gn_sm_functions(GN_OP_SetCallNotification, &data, sm);

	/* query model, revision and imei */
	if (gn_sm_functions(GN_OP_Identify, &data, sm) != GN_ERR_NONE)
		return false;

	/* We're ready to roll... */
	gn_atem_initialised = true;
	return (true);
}


/* Initialise the "registers" used by the virtual modem. */
void	gn_atem_registers_init(void)
{
	memset(ModemRegisters, 0, sizeof(ModemRegisters));

	ModemRegisters[REG_RINGATA] = 0;
	ModemRegisters[REG_RINGCNT] = 0;
	ModemRegisters[REG_ESC] = '+';
	ModemRegisters[REG_CR] = 13;
	ModemRegisters[REG_LF] = 10;
	ModemRegisters[REG_BS] = 8;
	ModemRegisters[S35] = 7;
	ModemRegisters[REG_ECHO] |= BIT_ECHO;
	ModemRegisters[REG_VERBOSE] |= BIT_VERBOSE;
	ModemRegisters[REG_CTRLZ] = 26;
	ModemRegisters[REG_ESCAPE] = 27;

	CallerIDMode = 0;
}


static void  gn_atem_hangup_phone(void)
{
	if (IncomingCallNo > 0) {
		rlp_user_request_set(Disc_Req, true);
		gn_sm_loop(10, sm);
	}
	if (IncomingCallNo > 0) {
		data.call_info->call_id = IncomingCallNo;
		gn_sm_functions(GN_OP_CancelCall, &data, sm);
		IncomingCallNo = -1;
	}
	dp_Initialise(PtyRDFD, PtyWRFD);
}


static void  gn_atem_answer_phone(void)
{
	/* For now we'll also initialise the datapump + rlp code again */
	dp_Initialise(PtyRDFD, PtyWRFD);
	data.call_notification = dp_CallPassup;
	gn_sm_functions(GN_OP_SetCallNotification, &data, sm);
	data.call_info->call_id = IncomingCallNo;
	gn_sm_functions(GN_OP_AnswerCall, &data, sm);
	CommandMode = false;
}


/* This gets called to indicate an incoming call */
void gn_atem_call_passup(gn_call_status CallStatus, gn_call_info *CallInfo, struct gn_statemachine *state, void *callback_data)
{
	dprintf("gn_atem_call_passup called with %d\n", CallStatus);

	switch (CallStatus) {
	case GN_CALL_Incoming:
		gn_atem_modem_result(MR_RING);
		IncomingCallNo = CallInfo->call_id;
		ModemRegisters[REG_RINGCNT]++;
		gn_atem_cid_out(CallInfo);
		if (ModemRegisters[REG_RINGATA] != 0) gn_atem_answer_phone();
		break;
	case GN_CALL_LocalHangup:
	case GN_CALL_RemoteHangup:
		IncomingCallNo = -1;
		break;
	default:
		break;
	}
}

/* This gets called to output caller id info of incoming call */
void gn_atem_cid_out(gn_call_info *CallInfo)
{
	struct tm *now;
	time_t nowh;
	char buf[14]; /* 7 for "DATE = " + 4 digits + \n + \r + \0 */

	nowh = time(NULL);
	now = localtime(&nowh);

	switch (CallerIDMode) {
	case 0:	/* no output */
		break;
	case 1: /* formatted CID */
		snprintf(buf, sizeof(buf), "DATE = %02d%02d\r\n", now->tm_mon + 1, now->tm_mday);
		gn_atem_string_out(buf);

		snprintf(buf, sizeof(buf), "TIME = %02d%02d\r\n", now->tm_hour, now->tm_min);
		gn_atem_string_out(buf);

		/* TO DO: handle P and O numbers */
		gn_atem_string_out("NMBR = ");
		gn_atem_string_out(1 + CallInfo->number); /* skip leading "+" */
		gn_atem_string_out("\r\nNAME = ");
		gn_atem_string_out(CallInfo->name);
		gn_atem_string_out("\r\n");

		/* FIX ME: do a real emulation of rings after the first one (at a lower level than this) */
		gn_atem_modem_result(MR_RING);

		break;

	}
}

/* Handler called when characters received from serial port.
   calls state machine code to process it. */
void	gn_atem_incoming_data_handle(char *buffer, int length)
{
	int count;
	unsigned char out_buf[3];

	for (count = 0; count < length ; count++) {

		/* If it's a command terminator character, parse what
		   we have so far then go to next buffer. */
		if (buffer[count] == ModemRegisters[REG_CR] ||
		    buffer[count] == ModemRegisters[REG_LF] ||
		    buffer[count] == ModemRegisters[REG_CTRLZ] ||
		    buffer[count] == ModemRegisters[REG_ESCAPE]) {

			/* Echo character if appropriate. */
			if (buffer[count] == ModemRegisters[REG_CR] &&
				(ModemRegisters[REG_ECHO] & BIT_ECHO)) {
				gn_atem_string_out("\r\n");
			}

			/* Save CTRL-Z and ESCAPE for the parser */
			if (buffer[count] == ModemRegisters[REG_CTRLZ] ||
			    buffer[count] == ModemRegisters[REG_ESCAPE])
				CmdBuffer[CurrentCmdBuffer][CurrentCmdBufferIndex++] = buffer[count];

			CmdBuffer[CurrentCmdBuffer][CurrentCmdBufferIndex] = 0x00;

			Parser(CmdBuffer[CurrentCmdBuffer]);

			CurrentCmdBuffer++;
			if (CurrentCmdBuffer >= MAX_CMD_BUFFERS) {
				CurrentCmdBuffer = 0;
			}
			CurrentCmdBufferIndex = 0;

		} else if (buffer[count] == ModemRegisters[REG_BS]) {
			if (CurrentCmdBufferIndex > 0) {
				/* Echo character if appropriate. */
				if (ModemRegisters[REG_ECHO] & BIT_ECHO) {
					gn_atem_string_out("\b \b");
				}

				CurrentCmdBufferIndex--;
			}
		} else {
			/* Echo character if appropriate. */
			if (ModemRegisters[REG_ECHO] & BIT_ECHO) {
				out_buf[0] = buffer[count];
				out_buf[1] = 0;
				gn_atem_string_out((char *)out_buf);
			}

			/* Collect it to command buffer */
			CmdBuffer[CurrentCmdBuffer][CurrentCmdBufferIndex++] = buffer[count];
			if (CurrentCmdBufferIndex >= CMD_BUFFER_LENGTH) {
				CurrentCmdBufferIndex = CMD_BUFFER_LENGTH;
			}
		}
	}
}


/* Parser for standard AT commands.  cmd_buffer must be null terminated. */
void	gn_atem_at_parse(char *cmd_buffer)
{
	char *buf;
	int regno, val;
	char str[256];

	if (!cmd_buffer[0]) return;

	if (strncasecmp (cmd_buffer, "AT", 2) != 0) {
		gn_atem_modem_result(MR_ERROR);
		return;
	}

	for (buf = &cmd_buffer[2]; *buf;) {
		switch (toupper(*buf)) {

		case 'Z':
			/* Reset modem */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0:	/* reset and load stored profile 0 */
			case 1:	/* reset and load stored profile 1 */
				gn_atem_hangup_phone();
				gn_atem_registers_init();
				break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		case 'A':
		        /* Answer call */
			buf++;
			gn_atem_answer_phone();
			return;
			break;

		case 'D':
			/* Dial Data :-) */
			/* FIXME - should parse this better */
			/* For now we'll also initialise the datapump + rlp code again */
			dp_Initialise(PtyRDFD, PtyWRFD);
			buf++;
			if (toupper(*buf) == 'T' || toupper(*buf) == 'P') buf++;
			while (*buf == ' ') buf++;
			data.call_notification = dp_CallPassup;
			gn_sm_functions(GN_OP_SetCallNotification, &data, sm);
			snprintf(data.call_info->number, sizeof(data.call_info->number), "%s", buf);
			if (ModemRegisters[S35] == 0)
				data.call_info->type = GN_CALL_DigitalData;
			else
				data.call_info->type = GN_CALL_NonDigitalData;
			data.call_info->send_number = GN_CALL_Default;
			CommandMode = false;
			if (gn_sm_functions(GN_OP_MakeCall, &data, sm) != GN_ERR_NONE) {
				CommandMode = true;
				dp_CallPassup(GN_CALL_RemoteHangup, NULL, NULL, NULL);
			} else {
				IncomingCallNo = data.call_info->call_id;
				gn_sm_loop(10, sm);
			}
			return;
			break;

		case 'H':
			/* Hang Up */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0:	/* hook off the phone */
				gn_atem_hangup_phone();
				break;
			case 1:	/* hook on the phone */
				break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		case 'S':
			/* Change registers */
			buf++;
			regno = gn_atem_num_get(&buf);
			if (regno < 0 || regno >= MAX_MODEM_REGISTERS) {
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			if (*buf == '=') {
				buf++;
				val = gn_atem_num_get(&buf);
				if (val < 0 || val > 255) {
					gn_atem_modem_result(MR_ERROR);
					return;
				}
				ModemRegisters[regno] = val;
			} else if (*buf == '?') {
				buf++;
				snprintf(str, sizeof(str), "%d\r\n", ModemRegisters[regno]);
				gn_atem_string_out(str);
			} else {
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		case 'E':
		        /* E - Turn Echo on/off */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0:
				ModemRegisters[REG_ECHO] &= ~BIT_ECHO;
				break;
			case 1:
				ModemRegisters[REG_ECHO] |= BIT_ECHO;
				break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		case 'Q':
		        /* Q - Turn Quiet on/off */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0:
				ModemRegisters[REG_QUIET] &= ~BIT_QUIET;
				break;
			case 1:
				ModemRegisters[REG_QUIET] |= BIT_QUIET;
				break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		case 'V':
		        /* V - Turn Verbose on/off */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0:
				ModemRegisters[REG_VERBOSE] &= ~BIT_VERBOSE;
				break;
			case 1:
				ModemRegisters[REG_VERBOSE] |= BIT_VERBOSE;
				break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		case 'X':
		        /* X - Set verbosity of the result messages */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0: val = 0x00; break;
			case 1: val = 0x40; break;
			case 2: val = 0x50; break;
			case 3: val = 0x60; break;
			case 4: val = 0x70; break;
			case 5: val = 0x10; break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			ModemRegisters[S22] = (ModemRegisters[S22] & 0x8f) | val;
			break;

		case 'I':
		        /* I - info */
			buf++;
			switch (gn_atem_num_get(&buf)) {
			case -1:
			case 0:	/* terminal id */
				snprintf(str, sizeof(str), "%d\r\n", ModemRegisters[39]);
				gn_atem_string_out(str);
				break;
			case 1:	/* serial number (IMEI) */
				snprintf(str, sizeof(str), "%s\r\n", imei);
				gn_atem_string_out(str);
				break;
			case 2: /* phone revision */
				snprintf(str, sizeof(str), "%s\r\n", revision);
				gn_atem_string_out(str);
				break;
			case 3: /* modem revision */
				gn_atem_string_out("gnokiid " VERSION "\r\n");
				break;
			case 4: /* OEM string */
				snprintf(str, sizeof(str), "%s %s\r\n", manufacturer, model);
				gn_atem_string_out(str);
				break;
			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		  /* Handle AT* commands (Nokia proprietary I think) */
		case '*':
			buf++;
			if (!strcasecmp(buf, "NOKIATEST")) {
				gn_atem_modem_result(MR_OK); /* FIXME? */
				return;
			} else {
				if (!strcasecmp(buf, "C")) {
					gn_atem_modem_result(MR_OK);
					Parser = gn_atem_sms_parse;
					return;
				}
			}
			break;

		/* + is the precursor to another set of commands */
		case '+':
			buf++;
			switch (toupper(*buf)) {
			case 'C':
				buf++;
				/* Returns true if error occured */
				if (gn_atem_command_plusc(&buf) == true) {
					gn_atem_modem_result(MR_ERROR);
					return;
				}
				break;

			case 'G':
				buf++;
				/* Returns true if error occured */
				if (gn_atem_command_plusg(&buf) == true) {
					gn_atem_modem_result(MR_ERROR);
					return;
				}
				break;

			default:
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		/* # is the precursor to another set of commands */
		case '#':
			buf++;
			/* Returns true if error occured */
			if (gn_atem_command_diesis(&buf) == true) {
				gn_atem_modem_result(MR_ERROR);
				return;
			}
			break;

		default:
			gn_atem_modem_result(MR_ERROR);
			return;
		}
	}

	gn_atem_modem_result(MR_OK);
}


static void gn_atem_sms_print(char *line, gn_sms *message, int mode)
{
	switch (mode) {
	case INTERACT_MODE:
		gsprintf(line, MAX_LINE_LENGTH,
			_("\r\nDate/time: %d/%d/%d %d:%02d:%02d Sender: %s Msg Center: %s\r\nText: %s\r\n"),
			message->smsc_time.day, message->smsc_time.month, message->smsc_time.year,
			message->smsc_time.hour, message->smsc_time.minute, message->smsc_time.second,
			message->remote.number, message->smsc.number, message->user_data[0].u.text);
		break;
	case TEXT_MODE:
		if ((message->dcs.type == GN_SMS_DCS_GeneralDataCoding) &&
		    (message->dcs.u.general.alphabet == GN_SMS_DCS_8bit))
			gsprintf(line, MAX_LINE_LENGTH,
				_("\"%s\",\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"\r\n%s"),
				(message->status ? _("REC READ") : _("REC UNREAD")),
				message->remote.number,
				message->smsc_time.year, message->smsc_time.month, message->smsc_time.day,
				message->smsc_time.hour, message->smsc_time.minute, message->smsc_time.second,
				message->time.timezone, _("<Not implemented>"));
		else
			gsprintf(line, MAX_LINE_LENGTH,
				_("\"%s\",\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"\r\n%s"),
				(message->status ? _("REC READ") : _("REC UNREAD")),
				message->remote.number,
				message->smsc_time.year, message->smsc_time.month, message->smsc_time.day,
				message->smsc_time.hour, message->smsc_time.minute, message->smsc_time.second,
				message->time.timezone, message->user_data[0].u.text);
		break;
	case PDU_MODE:
		gsprintf(line, MAX_LINE_LENGTH, _("0,<Not implemented>"));
		break;
	default:
		gsprintf(line, MAX_LINE_LENGTH, _("<Unknown mode>"));
		break;
	}
}


static void gn_atem_sms_handle()
{
	gn_error	error;
	char		buffer[MAX_LINE_LENGTH];

	data.sms->memory_type = SMSType;
	data.sms->number = SMSNumber;
	error = gn_sms_get(&data, sm);

	switch (error) {
	case GN_ERR_NONE:
		gn_atem_sms_print(buffer, data.sms, INTERACT_MODE);
		gn_atem_string_out(buffer);
		break;
	default:
		gsprintf(buffer, MAX_LINE_LENGTH, _("\r\nNo message under number %d\r\n"), SMSNumber);
		gn_atem_string_out(buffer);
		break;
	}
	return;
}

/* Parser for SMS interactive mode */
void	gn_atem_sms_parse(char *buff)
{
	if (!strcasecmp(buff, "HELP")) {
		gn_atem_string_out(_("\r\nThe following commands work...\r\n"));
		gn_atem_string_out("DIR\r\n");
		gn_atem_string_out("EXIT\r\n");
		gn_atem_string_out("HELP\r\n");
		return;
	}

	if (!strcasecmp(buff, "DIR")) {
		SMSNumber = 1;
		gn_atem_sms_handle();
		Parser = gn_atem_dir_parse;
		return;
	}
	if (!strcasecmp(buff, "EXIT")) {
		Parser = gn_atem_at_parse;
		gn_atem_modem_result(MR_OK);
		return;
	}
	gn_atem_modem_result(MR_ERROR);
}

/* Parser for DIR sub mode of SMS interactive mode. */
void	gn_atem_dir_parse(char *buff)
{
	switch (toupper(*buff)) {
		case 'P':
			SMSNumber--;
			gn_atem_sms_handle();
			return;
		case 'N':
			SMSNumber++;
			gn_atem_sms_handle();
			return;
		case 'D':
			data.sms->memory_type = SMSType;
			data.sms->number = SMSNumber;
			if (gn_sm_functions(GN_OP_DeleteSMS, &data, sm) == GN_ERR_NONE) {
				gn_atem_modem_result(MR_OK);
			} else {
				gn_atem_modem_result(MR_ERROR);
			}
			return;
		case 'Q':
			Parser= gn_atem_sms_parse;
			gn_atem_modem_result(MR_OK);
			return;
	}
	gn_atem_modem_result(MR_ERROR);
}

/* Parser for entering message content (+CMGS) */
void	gn_atem_sms_parseText(char *buff)
{
	static int index = 0;
	int i, length;
	char buffer[MAX_LINE_LENGTH];
	gn_error error;

	length = strlen(buff);

	sms.user_data[0].type = GN_SMS_DATA_Text;

	for (i = 0; i < length; i++) {

		if (buff[i] == ModemRegisters[REG_CTRLZ]) {
			/* Exit SMS text mode with sending */
			sms.user_data[0].u.text[index] = 0;
			sms.user_data[0].length = index;
			index = 0;
			Parser = gn_atem_at_parse;
			dprintf("Sending SMS to %s (text: %s)\n", data.sms->remote.number, data.sms->user_data[0].u.text);

			/* FIXME: set more SMS fields before sending */
			error = gn_sms_send(&data, sm);

			if (error == GN_ERR_NONE) {
				gsprintf(buffer, MAX_LINE_LENGTH, "+CMGS: %d\r\n", data.sms->number);
				gn_atem_string_out(buffer);
				gn_atem_modem_result(MR_OK);
			} else {
				gn_atem_modem_result(MR_ERROR);
			}
			return;
		} else if (buff[i] == ModemRegisters[REG_ESCAPE]) {
			/* Exit SMS text mode without sending */
			sms.user_data[0].u.text[index] = 0;
			sms.user_data[0].length = index;
			index = 0;
			Parser = gn_atem_at_parse;
			gn_atem_modem_result(MR_OK);
			return;
		} else {
			/* Appent next char to message text */
			sms.user_data[0].u.text[index++] = buff[i];
		}
	}

	/* reached the end of line so insert \n and wait for more */
	sms.user_data[0].u.text[index++] = '\n';
	gn_atem_string_out("\r\n> ");
}

/* Handle AT+C commands, this is a quick hack together at this
   stage. */
bool	gn_atem_command_plusc(char **buf)
{
	float		rflevel = -1;
	gn_rf_unit	rfunits = GN_RF_CSQ;
	char		buffer[MAX_LINE_LENGTH], buffer2[MAX_LINE_LENGTH];
	int		status, index;
	gn_error	error;

	if (strncasecmp(*buf, "SQ", 2) == 0) {
		buf[0] += 2;

		data.rf_unit = &rfunits;
		data.rf_level = &rflevel;
		if (gn_sm_functions(GN_OP_GetRFLevel, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "+CSQ: %0.0f, 99\r\n", *(data.rf_level));
			gn_atem_string_out(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMI is Manufacturer information for the ME (phone) so
	   it should be Nokia rather than gnokii... */
	if (strncasecmp(*buf, "GMI", 3) == 0) {
		buf[0] += 3;
		gn_atem_string_out(_("Nokia Mobile Phones\r\n"));
		return (false);
	}

	/* AT+CGSN is IMEI */
	if (strncasecmp(*buf, "GSN", 3) == 0) {
		buf[0] += 3;
		snprintf(data.imei, GN_IMEI_MAX_LENGTH, "+CME ERROR: 0");
		if (gn_sm_functions(GN_OP_GetImei, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "%s\r\n", data.imei);
			gn_atem_string_out(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMR is Revision (hardware) */
	if (strncasecmp(*buf, "GMR", 3) == 0) {
		buf[0] += 3;
		snprintf(data.revision, GN_REVISION_MAX_LENGTH, "+CME ERROR: 0");
		if (gn_sm_functions(GN_OP_GetRevision, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "%s\r\n", data.revision);
			gn_atem_string_out(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMM is Model code  */
	if (strncasecmp(*buf, "GMM", 3) == 0) {
		buf[0] += 3;
		snprintf(data.model, GN_MODEL_MAX_LENGTH, "+CME ERROR: 0");
		if (gn_sm_functions(GN_OP_GetModel, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "%s\r\n", data.model);
			gn_atem_string_out(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CMGD is deleting a message */
	if (strncasecmp(*buf, "MGD", 3) == 0) {
		buf[0] += 3;
		switch (**buf) {
		case '=':
			buf[0]++;
			index = atoi(*buf);
			buf[0] += strlen(*buf);

			data.sms->memory_type = SMSType;
			data.sms->number = index;
			error = gn_sm_functions(GN_OP_DeleteSMS, &data, sm);

			switch (error) {
			case GN_ERR_NONE:
				break;
			default:
				gsprintf(buffer, MAX_LINE_LENGTH, "\r\n+CMS ERROR: %d\r\n", error);
				gn_atem_string_out(buffer);
				return (true);
			}
			break;
		default:
			return (true);
		}
		return (false);
	}

	/* AT+CMGF is mode selection for message format  */
	if (strncasecmp(*buf, "MGF", 3) == 0) {
		buf[0] += 3;
		switch (**buf) {
		case '=':
			buf[0]++;
			switch (**buf) {
			case '0':
				buf[0]++;
				MessageFormat = PDU_MODE;
				break;
			case '1':
				buf[0]++;
				MessageFormat = TEXT_MODE;
				break;
			default:
				return (true);
			}
			break;
		case '?':
			buf[0]++;
			gsprintf(buffer, MAX_LINE_LENGTH, "+CMGF: %d\r\n", MessageFormat);
			gn_atem_string_out(buffer);
			break;
		default:
			return (true);
		}
		return (false);
	}

	/* AT+CMGR is reading a message */
	if (strncasecmp(*buf, "MGR", 3) == 0) {
		buf[0] += 3;
		switch (**buf) {
		case '=':
			buf[0]++;
			index = atoi(*buf);
			buf[0] += strlen(*buf);

			data.sms->memory_type = SMSType;
			data.sms->number = index;
			error = gn_sms_get(&data, sm);

			switch (error) {
			case GN_ERR_NONE:
				gn_atem_sms_print(buffer2, data.sms, MessageFormat);
				gsprintf(buffer, MAX_LINE_LENGTH, "+CMGR: %s\r\n", buffer2);
				gn_atem_string_out(buffer);
				break;
			default:
				gsprintf(buffer, MAX_LINE_LENGTH, "\r\n+CMS ERROR: %d\r\n", error);
				gn_atem_string_out(buffer);
				return (true);
			}
			break;
		default:
			return (true);
		}
		return (false);
	}

	/* AT+CMGS is sending a message */
	if (strncasecmp(*buf, "MGS", 3) == 0) {
		buf[0] += 3;
		switch (**buf) {
		case '=':
			buf[0]++;
			if (sscanf(*buf, "\"%[+0-9a-zA-Z ]\"", sms.remote.number)) {
				Parser = gn_atem_sms_parseText;
				buf[0] += strlen(*buf);
				gn_atem_string_out("\r\n> ");
			}
			return (true);
		default:
			return (true);
		}
		return (false);
	}

	/* AT+CMGL is listing messages */
	if (strncasecmp(*buf, "MGL", 3) == 0) {
		buf[0] += 3;
		status = -1;

		switch (**buf) {
		case 0:
		case '=':
			buf[0]++;
			/* process <stat> parameter */
			if (*(*buf-1) == 0 || /* i.e. no parameter given */
				strcasecmp(*buf, "1") == 0 ||
				strcasecmp(*buf, "3") == 0 ||
				strcasecmp(*buf, "\"REC READ\"") == 0 ||
				strcasecmp(*buf, "\"STO SENT\"") == 0) {
				status = GN_SMS_Sent;
			} else if (strcasecmp(*buf, "0") == 0 ||
				strcasecmp(*buf, "2") == 0 ||
				strcasecmp(*buf, "\"REC UNREAD\"") == 0 ||
				strcasecmp(*buf, "\"STO UNSENT\"") == 0) {
				status = GN_SMS_Unsent;
			} else if (strcasecmp(*buf, "4") == 0 ||
				strcasecmp(*buf, "\"ALL\"") == 0) {
				status = 4; /* ALL */
			} else {
				return true;
			}
			buf[0] += strlen(*buf);

			/* check all message storages */
			for (index = 1; index <= 20; index++) {

				data.sms->memory_type = SMSType;
				data.sms->number = index;
				error = gn_sms_get(&data, sm);

				switch (error) {
				case GN_ERR_NONE:
					/* print messsage if it has the required status */
					if (data.sms->status == status || status == 4 /* ALL */) {
						gn_atem_sms_print(buffer2, data.sms, MessageFormat);
						gsprintf(buffer, MAX_LINE_LENGTH, "+CMGL: %d,%s\r\n", index, buffer2);
						gn_atem_string_out(buffer);
					}
					break;
				case GN_ERR_EMPTYLOCATION:
					/* don't care if this storage is empty */
					break;
				default:
					/* print other error codes and quit */
					gsprintf(buffer, MAX_LINE_LENGTH, "\r\n+CMS ERROR: %d\r\n", error);
					gn_atem_string_out(buffer);
					return (true);
				}
			}
			break;
		default:
			return (true);
		}
		return (false);
	}

	return (true);
}

/* AT+G commands.  Some of these responses are a bit tongue in cheek... */
bool	gn_atem_command_plusg(char **buf)
{
	char		buffer[MAX_LINE_LENGTH];

	/* AT+GMI is Manufacturer information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MI", 3) == 0) {
		buf[0] += 2;

		gn_atem_string_out(_("Hugh Blemings, Pavel Janik ml. and others...\r\n"));
		return (false);
	}

	/* AT+GMR is Revision information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MR", 3) == 0) {
		buf[0] += 2;
		gsprintf(buffer, MAX_LINE_LENGTH, "%s %s %s\r\n", VERSION, __TIME__, __DATE__);

		gn_atem_string_out(buffer);
		return (false);
	}

	/* AT+GMM is Model information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MM", 3) == 0) {
		buf[0] += 2;

		gsprintf(buffer, MAX_LINE_LENGTH, _("gnokii configured on %s for models %s\r\n"), sm->config.port_device, sm->driver.phone.models);
		gn_atem_string_out(buffer);
		return (false);
	}

	/* AT+GSN is Serial number for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "SN", 3) == 0) {
		buf[0] += 2;

		gsprintf(buffer, MAX_LINE_LENGTH, _("none built in, choose your own\r\n"));
		gn_atem_string_out(buffer);
		return (false);
	}

	return (true);
}

/* Handle AT# commands */
bool	gn_atem_command_diesis(char **buf)
{
	int	number;
	char	buffer[MAX_LINE_LENGTH];

	if (strncasecmp(*buf, "CID", 3) == 0) {
		buf[0] += 3;
		switch (**buf) {
		case '?':
			buf[0]++;
			gsprintf(buffer, MAX_LINE_LENGTH, "%d\r\n", CallerIDMode);
			gn_atem_string_out(buffer);
			return (false);
		case '=':
			buf[0]++;
			if (**buf == '?') {
				buf[0]++;
				gn_atem_string_out("0,1\r\n");
				return (false);
			} else {
				number = gn_atem_num_get(buf);
				if ( number == 0 || number == 1 ) {
					CallerIDMode = number;
					return (false);
				}
			}
		}
	}
	return (true);
}

/* Send a result string back.  There is much work to do here, see
   the code in the isdn driver for an idea of where it's heading... */
void	gn_atem_modem_result(int code)
{
	char	buffer[16];

	if (!(ModemRegisters[REG_VERBOSE] & BIT_VERBOSE)) {
		snprintf(buffer, sizeof(buffer), "%d\r\n", code);
		gn_atem_string_out(buffer);
	} else {
		switch (code) {
			case MR_OK:
					gn_atem_string_out("OK\r\n");
					break;

			case MR_ERROR:
					gn_atem_string_out("ERROR\r\n");
					break;

			case MR_CARRIER:
					gn_atem_string_out("CARRIER\r\n");
					break;

			case MR_CONNECT:
					gn_atem_string_out("CONNECT\r\n");
					break;

			case MR_NOCARRIER:
					gn_atem_string_out("NO CARRIER\r\n");
					break;
		        case MR_RING:
					gn_atem_string_out("RING\r\n");
					break;
			default:
					gn_atem_string_out(_("\r\nUnknown Result Code!\r\n"));
					break;
		}
	}

}


/* Get integer from char-pointer, set pointer to end of number
   stolen basically verbatim from ISDN code.  */
int gn_atem_num_get(char **p)
{
	int v = -1;

	while (*p[0] >= '0' && *p[0] <= '9') {
		v = ((v < 0) ? 0 : (v * 10)) + (int) ((*p[0]++) - '0');
	}

	return v;
}

/* Write string to virtual modem port, either pty or
   STDOUT as appropriate.  This function is only used during
   command mode - data pump is used when connected.  */
void	gn_atem_string_out(char *buffer)
{
	int	count = 0;
	char	out_char;

	while (count < strlen(buffer)) {

		/* Translate CR/LF/BS as appropriate */
		switch (buffer[count]) {
			case '\r':
				out_char = ModemRegisters[REG_CR];
				break;
			case '\n':
				out_char = ModemRegisters[REG_LF];
				break;
			case '\b':
				out_char = ModemRegisters[REG_BS];
				break;
			default:
				out_char = buffer[count];
				break;
		}

		if (write(PtyWRFD, &out_char, 1) < 0) {
			fprintf(stderr, _("Failed to output string.\n"));
			perror("write");
			return;
		}
		count++;
	}
}
