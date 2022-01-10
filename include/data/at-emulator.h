/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.

  Header file for AT emulator code.

*/

#ifndef _gnokii_data_at_emulator_h
#define _gnokii_data_at_emulator_h

#include "compat.h"
#include "gnokii.h"

	/* Prototypes */
bool	gn_atem_initialise(int read_fd, int write_fd, struct gn_statemachine *sm);
void	gn_atem_incoming_data_handle(char *buffer, int length);
void	gn_atem_registers_init(void);
void	gn_atem_string_out(char *buffer);
void	gn_atem_at_parse(char *cmd_buffer);
void	gn_atem_sms_parse(char *cmd_buffer);
void	gn_atem_dir_parse(char *cmd_buffer);
bool	gn_atem_command_plusc(char **buf);
bool	gn_atem_command_plusg(char **buf);
int	gn_atem_num_get(char **p);
void	gn_atem_modem_result(int code);
void    gn_atem_call_passup(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data);
void	gn_atem_cid_out(gn_call_info *callinfo);
bool	gn_atem_command_diesis(char **buf);

	/* Global variables */
extern bool gn_atem_initialised;
extern struct gn_statemachine	*sm;
extern gn_data		data;

	/* Definition of modem result codes - these are returned to "terminal"
	   numerically or as a string depending on the setting of S12 */

	/* FIX ME - Numeric values for everything except OK and ERROR
	   are guesses as I've not got an AT reference handy.   HAB */

#define	MR_OK		(0)
#define	MR_ERROR	(4)
#define	MR_NOCARRIER	(5)
#define	MR_CARRIER	(2)
#define	MR_CONNECT	(3)
#define MR_RING		(6)

#endif	/* _gnokii_data_at_emulator_h */
