/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	at-emulator.h - Header file for AT emulator code in at-emulator.c


*/

#ifndef __at_emulator_h
#define __at_emulator_h

	/* Prototypes */
bool	ATEM_Initialise(int read_fd, int write_fd);
void	ATEM_HandleIncomingData(char *buffer, int length);
void	ATEM_InitRegisters(void);
void	ATEM_StringOut(char *buffer);
void	ATEM_ParseAT(char *cmd_buffer);
void	ATEM_ParseSMS(char *cmd_buffer);
void	ATEM_ParseDIR(char *cmd_buffer);
bool	ATEM_CommandPlusC(char **buf);
bool	ATEM_CommandPlusG(char **buf);
int		ATEM_GetNum(char **p);
void	ATEM_ModemResult(int code);

	/* Global variables */
bool	ATEM_Initialised;


	/* All defines and prototypes from here down are specific to 
	   the at-emulator code and so are #ifdef out if __at_emulator_c isn't 
	   defined. */
#ifdef	__at_emulator_c


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

#endif	/* __at_emulator_c */

#endif	/* __at_emulator_h */
