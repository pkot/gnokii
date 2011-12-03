/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.

  Header file for data pump code.

*/

#ifndef _gnokii_data_datapump_h
#define _gnokii_data_datapump_h

#include "compat.h"
#include "gnokii.h"

/* Prototypes */
bool	dp_Initialise(int read_fd, int write_fd);
void    dp_CallPassup(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data);

#endif	/* _gnokii_data_datapump_h */
