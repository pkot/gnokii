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

*/

#include <string.h>
#include "gsm-data.h"

gn_error Unimplemented(void)
{
	return GN_ERR_NOTIMPLEMENTED;
}

API gn_memory_type StrToMemoryType(const char *s)
{
#define X(a) if (!strcmp(s, #a)) return GN_MT_##a;
	X(ME);
	X(SM);
	X(FD);
	X(ON);
	X(EN);
	X(DC);
	X(RC);
	X(MC);
	X(LD);
	X(MT);
	X(IN);
	X(OU);
	X(AR);
	X(TE);
	X(F1);
	X(F2);
	X(F3);
	X(F4);
	X(F5);
	X(F6);
	X(F7);
	X(F8);
	X(F9);
	X(F10);
	X(F11);
	X(F12);
	X(F13);
	X(F14);
	X(F15);
	X(F16);
	X(F17);
	X(F18);
	X(F19);
	X(F20);
	return GN_MT_XX;
#undef X
}

/* This very small function is just to make it */
/* easier to clear the data struct every time one is created */
API void GSM_DataClear(gn_data *data)
{
	memset(data, 0, sizeof(gn_data));
}
