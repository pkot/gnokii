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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  This file provides a virtual modem or "AT" interface to the GSM phone by
  calling code in gsm-api.c. Inspired by and in places copied from the Linux
  kernel AT Emulator IDSN code by Fritz Elfert and others.

*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#ifndef WIN32
#  include <termios.h>
#endif

#include "config.h"
#include "misc.h"
#include "gnokii.h"
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
#define S35           6
#define REG_CTRLZ     7
#define REG_ESCAPE    8

#define REG_RESP     12
#define BIT_RESP      1
#define REG_RESPNUM  12
#define BIT_RESPNUM   2
#define REG_ECHO     12
#define BIT_ECHO      4
#define REG_DCD      12
#define BIT_DCD       8
#define REG_CTS      12
#define BIT_CTS      16
#define REG_DTRR     12
#define BIT_DTRR     32
#define REG_DSR      12
#define BIT_DSR      64
#define REG_CPPP     12
#define BIT_CPPP    128


#define	MAX_MODEM_REGISTERS	20

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
static 	char imei[64], model[64], revision[64], manufacturer[64];

/* Local variables */
static int	PtyRDFD;	/* File descriptor for reading and writing to/from */
static int	PtyWRFD;	/* pty interface - only different in debug mode. */

static u8	ModemRegisters[MAX_MODEM_REGISTERS];
static char	CmdBuffer[MAX_CMD_BUFFERS][CMD_BUFFER_LENGTH];
static int	CurrentCmdBuffer;
static int	CurrentCmdBufferIndex;
static bool	VerboseResponse; 	/* Switch betweek numeric (4) and text responses (ERROR) */
static char    IncomingCallNo;
static int     MessageFormat;          /* Message Format (text or pdu) */

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

	/* Default to verbose reponses */
	VerboseResponse = true;

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

	/* We're ready to roll... */
	gn_atem_initialised = true;
	return (true);
}


/* Initialise the "registers" used by the virtual modem. */
void	gn_atem_registers_init(void)
{

	ModemRegisters[REG_RINGATA] = 0;
	ModemRegisters[REG_RINGCNT] = 2;
	ModemRegisters[REG_ESC] = '+';
	ModemRegisters[REG_CR] = 10;
	ModemRegisters[REG_LF] = 13;
	ModemRegisters[REG_BS] = 8;
	ModemRegisters[S35]=7;
	ModemRegisters[REG_ECHO] = BIT_ECHO;
	ModemRegisters[REG_CTRLZ] = 26;
	ModemRegisters[REG_ESCAPE] = 27;
}


/* This gets called to indicate an incoming call */
void gn_atem_call_passup(gn_call_status CallStatus, gn_call_info *CallInfo, struct gn_statemachine *state)
{
	dprintf("gn_atem_call_passup called with %d\n", CallStatus);

	if (CallStatus == GN_CALL_Incoming) {
		gn_atem_modem_result(MR_RING);
		IncomingCallNo = CallInfo->call_id;
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

		} else {
			/* Echo character if appropriate. */
			if (ModemRegisters[REG_ECHO] & BIT_ECHO) {
				out_buf[0] = buffer[count];
				out_buf[1] = 0;
				gn_atem_string_out(out_buf);
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

	if (strncasecmp (cmd_buffer, "AT", 2) != 0) {
		gn_atem_modem_result(MR_ERROR);
		return;
	}

	for (buf = &cmd_buffer[2]; *buf;) {
		switch (toupper(*buf)) {

		case 'Z':
			buf++;
			break;
		case 'A':
			buf++;
			/* For now we'll also initialise the datapump + rlp code again */
			dp_Initialise(PtyRDFD, PtyWRFD);
			data.call_notification = dp_CallPassup;
			gn_sm_functions(GN_OP_SetCallNotification, &data, sm);
			data.call_info->call_id = IncomingCallNo;
			gn_sm_functions(GN_OP_AnswerCall, &data, sm);
			CommandMode = false;
			return;
			break;
		case 'D':
			/* Dial Data :-) */
			/* FIXME - should parse this better */
			/* For now we'll also initialise the datapump + rlp code again */
			dp_Initialise(PtyRDFD, PtyWRFD);
			buf++;
			if (toupper(*buf) == 'T') buf++;
			if (*buf == ' ') buf++;
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
				dp_CallPassup(GN_CALL_RemoteHangup, NULL, NULL);
			} else {
				IncomingCallNo = data.call_info->call_id;
			}
			return;
			break;
		case 'H':
			/* Hang Up */
			buf++;
			rlp_user_request_set(Disc_Req, true);
			data.call_info->call_id = IncomingCallNo;
			gn_sm_functions(GN_OP_CancelCall, &data, sm);
			break;
		case 'S':
			/* Change registers - only no. 35 for now */
			buf++;
			if (memcmp(buf, "35=", 3) == 0) {
				buf += 3;
				ModemRegisters[S35] = *buf - '0';
				buf++;
			}
			break;
		  /* E - Turn Echo on/off */
		case 'E':
			buf++;
			switch (gn_atem_num_get(&buf)) {
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
					return;
				}
				break;

			case 'G':
				buf++;
				/* Returns true if error occured */
				if (gn_atem_command_plusg(&buf) == true) {
					return;
				}
				break;

			default:
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
		gsprintf(line, MAX_LINE_LENGTH, _("\n\rDate/time: %d/%d/%d %d:%02d:%02d Sender: %s Msg Center: %s\n\rText: %s\n\r"), message->time.day, message->time.month, message->time.year, message->time.hour, message->time.minute, message->time.second, message->remote.number, message->smsc.number, message->user_data[0].u.text);
		break;
	case TEXT_MODE:
		if ((message->dcs.type == GN_SMS_DCS_GeneralDataCoding) &&
		    (message->dcs.u.general.alphabet == GN_SMS_DCS_8bit))
			gsprintf(line, MAX_LINE_LENGTH, _("\"%s\",\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"\n\r%s"), (message->status ? _("REC READ") : _("REC UNREAD")), message->remote.number, message->time.year, message->time.month, message->time.day, message->time.hour, message->time.minute, message->time.second, message->time.timezone, _("<Not implemented>"));
		else
			gsprintf(line, MAX_LINE_LENGTH, _("\"%s\",\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"\n\r%s"), (message->status ? _("REC READ") : _("REC UNREAD")), message->remote.number, message->time.year, message->time.month, message->time.day, message->time.hour, message->time.minute, message->time.second, message->time.timezone, message->user_data[0].u.text);
		break;
	case PDU_MODE:
		gsprintf(line, MAX_LINE_LENGTH, _("<Not implemented>"));
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
		gsprintf(buffer, MAX_LINE_LENGTH, _("\n\rNo message under number %d\n\r"), SMSNumber);
		gn_atem_string_out(buffer);
		break;
	}
	return;
}

/* Parser for SMS interactive mode */
void	gn_atem_sms_parse(char *buff)
{
	if (!strcasecmp(buff, "HELP")) {
		gn_atem_string_out(_("\n\rThe following commands work...\n\r"));
		gn_atem_string_out("DIR\n\r");
		gn_atem_string_out("EXIT\n\r");
		gn_atem_string_out("HELP\n\r");
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
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGS: %d", data.sms->number);
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
	gn_atem_string_out("\n\r> ");
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
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CSQ: %0.0f, 99", *(data.rf_level));
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
		gn_atem_string_out(_("\n\rNokia Mobile Phones"));
		return (false);
	}

	/* AT+CGSN is IMEI */
	if (strncasecmp(*buf, "GSN", 3) == 0) {
		buf[0] += 3;
		strcpy(data.imei, "+CME ERROR: 0");
		if (gn_sm_functions(GN_OP_GetImei, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s", data.imei);
			gn_atem_string_out(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMR is Revision (hardware) */
	if (strncasecmp(*buf, "GMR", 3) == 0) {
		buf[0] += 3;
		strcpy(data.revision, "+CME ERROR: 0");
		if (gn_sm_functions(GN_OP_GetRevision, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s", data.revision);
			gn_atem_string_out(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMM is Model code  */
	if (strncasecmp(*buf, "GMM", 3) == 0) {
		buf[0] += 3;
		strcpy(data.model, "+CME ERROR: 0");
		if (gn_sm_functions(GN_OP_GetModel, &data, sm) == GN_ERR_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s", data.model);
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
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMS ERROR: %d\n\r", error);
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
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGF: %d", MessageFormat);
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
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGR: %s", buffer2);
				gn_atem_string_out(buffer);
				break;
			default:
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMS ERROR: %d\n\r", error);
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
				gn_atem_string_out("\n\r> ");
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
						gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGL: %d,%s", index, buffer2);
						gn_atem_string_out(buffer);
					}
					break;
				case GN_ERR_EMPTYLOCATION:
					/* don't care if this storage is empty */
					break;
				default:
					/* print other error codes and quit */
					gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMS ERROR: %d\n\r", error);
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

		gn_atem_string_out(_("\n\rHugh Blemings, Pavel Janik ml. and others..."));
		return (false);
	}

	/* AT+GMR is Revision information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MR", 3) == 0) {
		buf[0] += 2;
		gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s %s %s", VERSION, __TIME__, __DATE__);

		gn_atem_string_out(buffer);
		return (false);
	}

	/* AT+GMM is Model information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MM", 3) == 0) {
		buf[0] += 2;

		gsprintf(buffer, MAX_LINE_LENGTH, _("\n\rgnokii configured on %s for models %s"), sm->config.port_device, sm->driver.phone.models);
		gn_atem_string_out(buffer);
		return (false);
	}

	/* AT+GSN is Serial number for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "SN", 3) == 0) {
		buf[0] += 2;

		gsprintf(buffer, MAX_LINE_LENGTH, _("\n\rnone built in, choose your own"));
		gn_atem_string_out(buffer);
		return (false);
	}

	return (true);
}

/* Send a result string back.  There is much work to do here, see
   the code in the isdn driver for an idea of where it's heading... */
void	gn_atem_modem_result(int code)
{
	char	buffer[16];

	if (VerboseResponse == false) {
		sprintf(buffer, "\n\r%d\n\r", code);
		gn_atem_string_out(buffer);
	} else {
		switch (code) {
			case MR_OK:
					gn_atem_string_out("\n\rOK\n\r");
					break;

			case MR_ERROR:
					gn_atem_string_out("\n\rERROR\n\r");
					break;

			case MR_CARRIER:
					gn_atem_string_out("\n\rCARRIER\n\r");
					break;

			case MR_CONNECT:
					gn_atem_string_out("\n\rCONNECT\n\r");
					break;

			case MR_NOCARRIER:
					gn_atem_string_out("\n\rNO CARRIER\n\r");
					break;
		        case MR_RING:
					gn_atem_string_out("RING\n\r");
					break;
			default:
					gn_atem_string_out(_("\n\rUnknown Result Code!\n\r"));
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
	int		count = 0;
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

		write(PtyWRFD, &out_char, 1);
		count++;
	}
}
