/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

*/

#include <string.h>
#include "gsm-common.h"

GSM_Error Unimplemented(void)
{
	return GE_NOTIMPLEMENTED;
}

GSM_MemoryType StrToMemoryType(const char *s)
{
#define X(a) if (!strcmp(s, #a)) return GMT_##a;
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
	return GMT_XX;
#undef X
}

/* This very small function is just to make it */
/* easier to clear the data struct every time one is created */
inline void GSM_DataClear(GSM_Data *data)
{
	memset(data, 0, sizeof(GSM_Data));
}
