/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

*/

/* System header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Various header file */
#include "config.h"
#include "misc.h"
#include "gsm-statemachine.h"
#include "links/utils.h"
#include "device.h"


GSM_Error LINK_Terminate(GSM_Statemachine *state)
{
	/* device_close(&(state->Device)); */
	device_close();
	return GE_NONE; /* FIXME */
}
