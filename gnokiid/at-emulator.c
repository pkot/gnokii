/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	at-emulator.c - provides a virtual modem or "AT" interface to the GSM
	phone by calling code in gsm-api.c.  Inspired by and in places
	copied from the linux kernel AT Emulator IDSN code by Fritz Elfert and
	others.


*/

#define		__at_emulator_c


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
#include "gsm-api.h"
#include "at-emulator.h"

	/* Global variables */
bool	ATEM_Initialised = false;	/* Set to true once initialised */

	/* Local variables */

int		PtyRDFD;	/* File descriptor for reading and writing to/from */
int		PtyWRFD;	/* pty interface - only different in debug mode. */ 

u8		ModemRegisters[MAX_MODEM_REGISTERS];
char	CmdBuffer[MAX_CMD_BUFFERS][CMD_BUFFER_LENGTH];
int		CurrentCmdBuffer;
int		CurrentCmdBufferIndex;



	/* If initialised in debug mode, stdin/out is used instead
	   of ptys for interface. */
bool	ATEM_Initialise(int read_fd, int write_fd)
{
	PtyRDFD = read_fd;
	PtyWRFD = write_fd;

		/* Initialise command buffer variables */
	CurrentCmdBuffer = 0;
	CurrentCmdBufferIndex = 0;

		/* Initialise registers */
	ATEM_InitRegisters();

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

	ModemRegisters[REG_ECHO] = 0x00;

}



    /* Handler called when characters received from serial port.
       calls state machine code to process it. */

void	ATEM_HandleIncomingData(char *buffer, int length)
{
    int             count;
	unsigned char	out_buf[3];	

    for (count = 0; count < length ; count ++) {
			/* Echo character if appropriate. */
		if (ModemRegisters[REG_ECHO] & BIT_ECHO) {
			out_buf[0] = buffer[count];
			out_buf[1] = 0;
			ATEM_StringOut(out_buf);
		}

			/* If it's a command terminator character, go to next
			   buffer. */
		if (buffer[count] == ModemRegisters[REG_CR] ||
		    buffer[count] == ModemRegisters[REG_LF]) {

			CmdBuffer[CurrentCmdBuffer][CurrentCmdBufferIndex] = 0x00;
			ATEM_ParseAT(CmdBuffer[CurrentCmdBuffer]);

			CurrentCmdBuffer ++;
			if (CurrentCmdBuffer >= MAX_CMD_BUFFERS) {
				CurrentCmdBuffer = 0;
			}
			CurrentCmdBufferIndex = 0;
		}
		else {
			CmdBuffer[CurrentCmdBuffer][CurrentCmdBufferIndex] = buffer[count];
			CurrentCmdBufferIndex ++;
			if (CurrentCmdBufferIndex >= CMD_BUFFER_LENGTH) {
				CurrentCmdBufferIndex = CMD_BUFFER_LENGTH;
			}
		}
    }
}     


	/* Parse commands in buffer, cmd_buffer must be null terminated. */
void	ATEM_ParseAT(char *cmd_buffer)
{
	char *buf;
	char dial_string[40];

	if (strncmp (cmd_buffer, "AT", 2) != 0) {
		ATEM_ModemResult(4);
		return;
	}

	for (buf = &cmd_buffer[2]; *buf;) {
		switch (*buf) {
			case 'E':
				/* E - Turn Echo on/off */
				buf++;
				switch (ATEM_GetNum(&buf)) {
					case 0:
						ModemRegisters[REG_ECHO] &= ~BIT_ECHO;
						break;

					case 1:
						ModemRegisters[REG_ECHO] |= BIT_ECHO;
						break;

					default:
						ATEM_ModemResult(4);
						return;
				}
				break;

			case '+':
				/* + is the precursor to another set of commands +CSQ, +FCLASS etc. */
				buf++;
				switch (*buf) {
					case 'C':
						buf++;
							/* Returns true if error occured */
						if (ATEM_CommandPlusC(&buf) == true) {
							return;	
						}
						break;

					default:
						ATEM_ModemResult(4);
						return;
				}
				break;

			default: 
				ATEM_ModemResult(4);
				return;
		}
	}

	ATEM_ModemResult(0);
}


	/* Handle AT+C commands, this is a quick hack together at this
	   stage. */
bool	ATEM_CommandPlusC(char **buf)
{
	float		rflevel;
	char		buffer[20];

	if (strncmp(*buf, "SQ", 2) == 0) {
		buf[0] ++;
		buf[0] ++;

    	if (GSM->GetRFLevel(&rflevel) == GE_NONE) {
			sprintf(buffer, "%f, 99", rflevel);
			ATEM_StringOut(buffer);
			return (false);
		}
		else {
			return (true);
		}
			
	}
	return (true);
	


}

	/* Send a result string back.  There is much work to do here, see
	   the code in the isdn driver for an idea of where it's heading... */
void	ATEM_ModemResult(int code) 
{
	switch (code) {
		case 0:		ATEM_StringOut("\n\rOK\n\r");
					break;

		case 4:		ATEM_StringOut("\n\rERROR\n\r");
					break;

		default:	ATEM_StringOut("\n\rUnknown Result Code!\n\r");
					break;
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

	while (count <= strlen(buffer)) {

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
		count ++;
	}

}


