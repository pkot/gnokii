/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides a virtual modem or "AT" interface to the GSM phone by
  calling code in gsm-api.c. Inspired by and in places copied from the Linux
  kernel AT Emulator IDSN code by Fritz Elfert and others.
  
*/

#define		__data_at_emulator_c


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
#include "gsm-data.h"
#include "gsm-api.h"
#include "gsm-sms.h"
#include "data/at-emulator.h"
#include "data/virtmodem.h"
#include "data/datapump.h"

#define MAX_LINE_LENGTH 256

/* Global variables */
bool ATEM_Initialised = false;	/* Set to true once initialised */
extern bool CommandMode;
extern int ConnectCount;

static GSM_Statemachine *sm;

static GSM_Data data;
static GSM_SMSMessage sms;
static 	char imei[64], model[64], revision[64], manufacturer[64];

/* Local variables */
int	PtyRDFD;	/* File descriptor for reading and writing to/from */
int	PtyWRFD;	/* pty interface - only different in debug mode. */ 

u8	ModemRegisters[MAX_MODEM_REGISTERS];
char	CmdBuffer[MAX_CMD_BUFFERS][CMD_BUFFER_LENGTH];
int	CurrentCmdBuffer;
int	CurrentCmdBufferIndex;
bool	VerboseResponse; 	/* Switch betweek numeric (4) and text responses (ERROR) */
char    IncomingCallNo;
int     MessageFormat;          /* Message Format (text or pdu) */

	/* Current command parser */
void 	(*Parser)(char *); /* Current command parser */
/* void 	(*Parser)(char *) = ATEM_ParseAT; */

GSM_MemoryType 	SMSType;
int 	SMSNumber;

/* If initialised in debug mode, stdin/out is used instead
   of ptys for interface. */
bool ATEM_Initialise(int read_fd, int write_fd, GSM_Statemachine *vmsm)
{
	PtyRDFD = read_fd;
	PtyWRFD = write_fd;

	GSM_DataClear(&data);
	memset(&sms, 0, sizeof(GSM_SMSMessage));

	data.SMSMessage = &sms;
	data.Manufacturer = manufacturer;
	data.Model = model;
	data.Revision = revision;
	data.Imei = imei;

	sm = vmsm;

	/* Initialise command buffer variables */
	CurrentCmdBuffer = 0;
	CurrentCmdBufferIndex = 0;

	/* Default to verbose reponses */
	VerboseResponse = true;

	/* Initialise registers */
	ATEM_InitRegisters();

	/* Initial parser is AT routine */
	Parser = ATEM_ParseAT;

	/* Setup defaults for AT*C interpreter. */
	SMSNumber = 1;
	SMSType = GMT_ME;

	/* Default message format is PDU */
	MessageFormat = PDU_MODE;

	/* Set the call passup so that we get notified of incoming calls */
	/*GSM->DialData(NULL,-1,&ATEM_CallPassup);*/

	/* We're ready to roll... */
	ATEM_Initialised = true;
	return (true);
}


/* Initialise the "registers" used by the virtual modem. */
void	ATEM_InitRegisters(void) 
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
void ATEM_CallPassup(char c)
{
	if ((c >= 0) && (c < 9)) {
		ATEM_ModemResult(MR_RING);		
		IncomingCallNo = c;
	}
}


/* Handler called when characters received from serial port.
   calls state machine code to process it. */
void	ATEM_HandleIncomingData(char *buffer, int length)
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
				ATEM_StringOut(out_buf);
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
void	ATEM_ParseAT(char *cmd_buffer)
{
	char *buf;
	char number[30];

	if (strncasecmp (cmd_buffer, "AT", 2) != 0) {
		ATEM_ModemResult(MR_ERROR);
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
			DP_Initialise(PtyRDFD, PtyWRFD);
			GSM->DialData(NULL, -1, &DP_CallPassup);
			GSM->AnswerCall(IncomingCallNo);
			CommandMode = false;
			return;
			break;
		case 'D':
			/* Dial Data :-) */
			/* FIXME - should parse this better */
			/* For now we'll also initialise the datapump + rlp code again */
			DP_Initialise(PtyRDFD, PtyWRFD);
			buf++;
			if (toupper(*buf) == 'T') buf++;
			if (*buf == ' ') buf++;
			strncpy(number, buf, 30);
			if (ModemRegisters[S35] == 0) GSM->DialData(number, 1, &DP_CallPassup);
			else GSM->DialData(number, 0, &DP_CallPassup);
			ATEM_StringOut("\n\r");
			CommandMode = false;
			return;
			break;
		case 'H':
			/* Hang Up */
			buf++;
			RLP_SetUserRequest(Disc_Req, true);
			GSM->CancelCall();
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
			switch (ATEM_GetNum(&buf)) {
			case 0:
				ModemRegisters[REG_ECHO] &= ~BIT_ECHO;
				break;
			case 1:
				ModemRegisters[REG_ECHO] |= BIT_ECHO;
				break;
			default:
				ATEM_ModemResult(MR_ERROR);
				return;
			}
			break;
			
		  /* Handle AT* commands (Nokia proprietary I think) */
		case '*':
			buf++;
			if (!strcasecmp(buf, "NOKIATEST")) {
				ATEM_ModemResult(MR_OK); /* FIXME? */
				return;
			} else {
				if (!strcasecmp(buf, "C")) {
					ATEM_ModemResult(MR_OK);
					Parser = ATEM_ParseSMS;
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
				if (ATEM_CommandPlusC(&buf) == true) {
					return;	
				}
				break;
				
			case 'G':
				buf++;
				/* Returns true if error occured */
				if (ATEM_CommandPlusG(&buf) == true) {
					return;	
				}
				break;
				
			default:
				ATEM_ModemResult(MR_ERROR);
				return;
			}
			break;
			
		default: 
			ATEM_ModemResult(MR_ERROR);
			return;
		}
	}
	
	ATEM_ModemResult(MR_OK);
}


static void ATEM_PrintSMS(char *line, GSM_SMSMessage *message, int mode)
{
	switch (mode) {
	case INTERACT_MODE:
		gsprintf(line, MAX_LINE_LENGTH, _("\n\rDate/time: %d/%d/%d %d:%02d:%02d Sender: %s Msg Center: %s\n\rText: %s\n\r"), message->Time.Day, message->Time.Month, message->Time.Year, message->Time.Hour, message->Time.Minute, message->Time.Second, message->RemoteNumber.number, message->MessageCenter.Number, message->UserData[0].u.Text);
		break;
	case TEXT_MODE:
		if ((message->DCS.Type == SMS_GeneralDataCoding) &&
		    (message->DCS.u.General.Alphabet == SMS_8bit))
			gsprintf(line, MAX_LINE_LENGTH, _("\"%s\",\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"\n\r%s"), (message->Status ? _("REC READ") : _("REC UNREAD")), message->RemoteNumber.number, message->Time.Year, message->Time.Month, message->Time.Day, message->Time.Hour, message->Time.Minute, message->Time.Second, message->Time.Timezone, _("<Not implemented>"));
		else
			gsprintf(line, MAX_LINE_LENGTH, _("\"%s\",\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d+%02d\"\n\r%s"), (message->Status ? _("REC READ") : _("REC UNREAD")), message->RemoteNumber.number, message->Time.Year, message->Time.Month, message->Time.Day, message->Time.Hour, message->Time.Minute, message->Time.Second, message->Time.Timezone, message->UserData[0].u.Text);
		break;
	case PDU_MODE:
		gsprintf(line, MAX_LINE_LENGTH, _("<Not implemented>"));
		break;
	default:
		gsprintf(line, MAX_LINE_LENGTH, _("<Unknown mode>"));
		break;
	}
}


static void ATEM_HandleSMS()
{
	GSM_Error	error;
	char		buffer[MAX_LINE_LENGTH];

	data.SMSMessage->MemoryType = SMSType;
	data.SMSMessage->Number = SMSNumber;
	error = GetSMS(&data, sm);

	switch (error) {
	case GE_NONE:
		ATEM_PrintSMS(buffer, data.SMSMessage, INTERACT_MODE);
		ATEM_StringOut(buffer);
		break;
	default:
		gsprintf(buffer, MAX_LINE_LENGTH, _("\n\rNo message under number %d\n\r"), SMSNumber);
		ATEM_StringOut(buffer);
		break;
	}
	return;
}

/* Parser for SMS interactive mode */
void	ATEM_ParseSMS(char *buff)
{
	if (!strcasecmp(buff, "HELP")) {
		ATEM_StringOut(_("\n\rThe following commands work...\n\r"));
		ATEM_StringOut("DIR\n\r");
		ATEM_StringOut("EXIT\n\r");
		ATEM_StringOut("HELP\n\r");
		return;
	}

	if (!strcasecmp(buff, "DIR")) {
		SMSNumber = 1;
		ATEM_HandleSMS();
		Parser = ATEM_ParseDIR;
		return;
	}
	if (!strcasecmp(buff, "EXIT")) {
		Parser = ATEM_ParseAT;
		ATEM_ModemResult(MR_OK);
		return;
	} 
	ATEM_ModemResult(MR_ERROR);
}

/* Parser for DIR sub mode of SMS interactive mode. */
void	ATEM_ParseDIR(char *buff)
{
	switch (toupper(*buff)) {
		case 'P':
			SMSNumber--;
			ATEM_HandleSMS();
			return;
		case 'N':
			SMSNumber++;
			ATEM_HandleSMS();
			return;
		case 'D':
			data.SMSMessage->MemoryType = SMSType;
			data.SMSMessage->Number = SMSNumber;
			if (SM_Functions(GOP_DeleteSMS, &data, sm) == GE_NONE) {
				ATEM_ModemResult(MR_OK);
			} else {
				ATEM_ModemResult(MR_ERROR);
			}
			return;
		case 'Q':
			Parser= ATEM_ParseSMS;
			ATEM_ModemResult(MR_OK);
			return;
	}
	ATEM_ModemResult(MR_ERROR);
}

/* Parser for entering message content (+CMGS) */
void	ATEM_ParseSMSText(char *buff)
{
	static int index = 0;
	int i, length;
	char buffer[MAX_LINE_LENGTH];
	GSM_Error error;

	length = strlen(buff);

	sms.UserData[0].Type = SMS_PlainText;

	for (i = 0; i < length; i++) {

		if (buff[i] == ModemRegisters[REG_CTRLZ]) {
			/* Exit SMS text mode with sending */
			sms.UserData[0].u.Text[index] = 0;
			sms.UserData[0].Length = index;
			index = 0;
			Parser = ATEM_ParseAT;
			dprintf("Sending SMS to %s (text: %s)\n", data.SMSMessage->RemoteNumber.number, data.SMSMessage->UserData[0].u.Text);

			/* FIXME: set more SMS fields before sending */
			error = SendSMS(&data, sm);

			if (error == GE_NONE || error == GE_SMSSENDOK) {
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGS: %d", data.SMSMessage->Number);
				ATEM_StringOut(buffer);
				ATEM_ModemResult(MR_OK);
			} else {
				ATEM_ModemResult(MR_ERROR);
			}
			return;
		} else if (buff[i] == ModemRegisters[REG_ESCAPE]) {
			/* Exit SMS text mode without sending */
			sms.UserData[0].u.Text[index] = 0;
			sms.UserData[0].Length = index;
			index = 0;
			Parser = ATEM_ParseAT;
			ATEM_ModemResult(MR_OK);
			return;
		} else {
			/* Appent next char to message text */
			sms.UserData[0].u.Text[index++] = buff[i];
		}
	}

	/* reached the end of line so insert \n and wait for more */
	sms.UserData[0].u.Text[index++] = '\n';
	ATEM_StringOut("\n\r> ");
}

/* Handle AT+C commands, this is a quick hack together at this
   stage. */
bool	ATEM_CommandPlusC(char **buf)
{
	float		rflevel = -1;
	GSM_RFUnits	rfunits = GRF_CSQ;
	char		buffer[MAX_LINE_LENGTH], buffer2[MAX_LINE_LENGTH];
	int		status, index;
	GSM_Error	error;

	if (strncasecmp(*buf, "SQ", 2) == 0) {
		buf[0] += 2;

		data.RFUnits = &rfunits;
		data.RFLevel = &rflevel;
		if (SM_Functions(GOP_GetRFLevel, &data, sm) == GE_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CSQ: %0.0f, 99", *(data.RFLevel));
			ATEM_StringOut(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMI is Manufacturer information for the ME (phone) so
	   it should be Nokia rather than gnokii... */
	if (strncasecmp(*buf, "GMI", 3) == 0) {
		buf[0] += 3;
		ATEM_StringOut(_("\n\rNokia Mobile Phones"));
		return (false);
	}

	/* AT+CGSN is IMEI */
	if (strncasecmp(*buf, "GSN", 3) == 0) {
		buf[0] += 3;
		strcpy(data.Imei, "+CME ERROR: 0");
		if (SM_Functions(GOP_GetImei, &data, sm) == GE_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s", data.Imei);
			ATEM_StringOut(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMR is Revision (hardware) */
	if (strncasecmp(*buf, "GMR", 3) == 0) {
		buf[0] += 3;
		strcpy(data.Revision, "+CME ERROR: 0");
		if (SM_Functions(GOP_GetRevision, &data, sm) == GE_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s", data.Revision);
			ATEM_StringOut(buffer);
			return (false);
		} else {
			return (true);
		}
	}

	/* AT+CGMM is Model code  */
	if (strncasecmp(*buf, "GMM", 3) == 0) {
		buf[0] += 3;
		strcpy(data.Model, "+CME ERROR: 0");
		if (SM_Functions(GOP_GetModel, &data, sm) == GE_NONE) {
			gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s", data.Model);
			ATEM_StringOut(buffer);
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
			sscanf(*buf, "%d", &index);
			buf[0] += strlen(*buf);

			data.SMSMessage->MemoryType = SMSType;
			data.SMSMessage->Number = index;
			error = SM_Functions(GOP_DeleteSMS, &data, sm);

			switch (error) {
			case GE_NONE:
				break;
			default:
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMS ERROR: %d\n\r", error);
				ATEM_StringOut(buffer);
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
			ATEM_StringOut(buffer);
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
			sscanf(*buf, "%d", &index);
			buf[0] += strlen(*buf);

			data.SMSMessage->MemoryType = SMSType;
			data.SMSMessage->Number = index;
			error = GetSMS(&data, sm);

			switch (error) {
			case GE_NONE:
				ATEM_PrintSMS(buffer2, data.SMSMessage, MessageFormat);
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGR: %s", buffer2);
				ATEM_StringOut(buffer);
				break;
			default:
				gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMS ERROR: %d\n\r", error);
				ATEM_StringOut(buffer);
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
			if (sscanf(*buf, "\"%[+0-9a-zA-Z ]\"", sms.RemoteNumber.number)) {
				Parser = ATEM_ParseSMSText;
				buf[0] += strlen(*buf);
				ATEM_StringOut("\n\r> ");
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
				status = SMS_Sent;
			} else if (strcasecmp(*buf, "0") == 0 ||
				strcasecmp(*buf, "2") == 0 ||
				strcasecmp(*buf, "\"REC UNREAD\"") == 0 ||
				strcasecmp(*buf, "\"STO UNSENT\"") == 0) {
				status = SMS_Unsent;
			} else if (strcasecmp(*buf, "4") == 0 ||
				strcasecmp(*buf, "\"ALL\"") == 0) {
				status = 4; /* ALL */
			} else {
				return true;
			}
			buf[0] += strlen(*buf);

			/* check all message storages */
			for (index = 1; index <= 20; index++) {

				data.SMSMessage->MemoryType = SMSType;
				data.SMSMessage->Number = index;
				error = GetSMS(&data, sm);

				switch (error) {
				case GE_NONE:
					/* print messsage if it has the required status */
					if (data.SMSMessage->Status == status || status == 4 /* ALL */) {
						ATEM_PrintSMS(buffer2, data.SMSMessage, MessageFormat);
						gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMGL: %d,%s", index, buffer2);
						ATEM_StringOut(buffer);
					}
					break;
				case GE_EMPTYSMSLOCATION:
					/* don't care if this storage is empty */
					break;
				default:
					/* print other error codes and quit */
					gsprintf(buffer, MAX_LINE_LENGTH, "\n\r+CMS ERROR: %d\n\r", error);
					ATEM_StringOut(buffer);
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
bool	ATEM_CommandPlusG(char **buf)
{
	char		buffer[MAX_LINE_LENGTH];

	/* AT+GMI is Manufacturer information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MI", 3) == 0) {
		buf[0] += 2;

		ATEM_StringOut(_("\n\rHugh Blemings, Pavel Janík ml. and others..."));
		return (false);
	}

	/* AT+GMR is Revision information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MR", 3) == 0) {
		buf[0] += 2;
		gsprintf(buffer, MAX_LINE_LENGTH, "\n\r%s %s %s", VERSION, __TIME__, __DATE__);

		ATEM_StringOut(buffer);
		return (false);
	}

	/* AT+GMM is Model information for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "MM", 3) == 0) {
		buf[0] += 2;

		gsprintf(buffer, MAX_LINE_LENGTH, _("\n\rgnokii configured on %s for models %s"), sm->Link.PortDevice, sm->Phone.Info.Models);
		ATEM_StringOut(buffer);
		return (false);
	}

	/* AT+GSN is Serial number for the TA (Terminal Adaptor) */
	if (strncasecmp(*buf, "SN", 3) == 0) {
		buf[0] += 2;

		gsprintf(buffer, MAX_LINE_LENGTH, _("\n\rnone built in, choose your own"));
		ATEM_StringOut(buffer);
		return (false);
	}

	return (true);
}

/* Send a result string back.  There is much work to do here, see
   the code in the isdn driver for an idea of where it's heading... */
void	ATEM_ModemResult(int code) 
{
	char	buffer[16];

	if (VerboseResponse == false) {
		sprintf(buffer, "\n\r%d\n\r", code);
		ATEM_StringOut(buffer);
	} else {
		switch (code) {
			case MR_OK:	
					ATEM_StringOut("\n\rOK\n\r");
					break;

			case MR_ERROR:
					ATEM_StringOut("\n\rERROR\n\r");
					break;

			case MR_CARRIER:
					ATEM_StringOut("\n\rCARRIER\n\r");
					break;

			case MR_CONNECT:
					ATEM_StringOut("\n\rCONNECT\n\r");
					break;

			case MR_NOCARRIER:
					ATEM_StringOut("\n\rNO CARRIER\n\r");
					break;
		        case MR_RING:
					ATEM_StringOut("RING\n\r");
					break;
			default:
					ATEM_StringOut(_("\n\rUnknown Result Code!\n\r"));
					break;
		}
	}

}


/* Get integer from char-pointer, set pointer to end of number
   stolen basically verbatim from ISDN code.  */
int ATEM_GetNum(char **p)
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
void	ATEM_StringOut(char *buffer)
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
