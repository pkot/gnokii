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
#include "config.h"
#include "compat.h"
#include "gnokii.h"

gn_error unimplemented(void)
{
	return GN_ERR_NOTIMPLEMENTED;
}

API gn_memory_type gn_str2memory_type(const char *s)
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

API char *gn_memory_type2str(gn_memory_type mt)
{
	switch (mt) {
	case GN_MT_ME: return _("Internal memory");
	case GN_MT_SM: return _("SIM card");
	case GN_MT_FD: return _("Fixed dial numbers");
	case GN_MT_ON: return _("Own numbers");
	case GN_MT_EN: return _("Emergency numbers");
	case GN_MT_DC: return _("Dialed numbers");
	case GN_MT_RC: return _("Received numbers");
	case GN_MT_MC: return _("Missed numbers");
	case GN_MT_LD: return _("Last dialed");
	case GN_MT_MT: return _("Combined ME and SIM phonebook");
	case GN_MT_TA: return _("Computer memory");
	case GN_MT_CB: return _("Currently selected memory");
	case GN_MT_IN: return _("SMS Inbox");
	case GN_MT_OU: return _("SMS Outbox");
	case GN_MT_AR: return _("SMS Archive");
	case GN_MT_TE: return _("SMS Templates");
	case GN_MT_F1: return _("SMS Folder 1");
	case GN_MT_F2: return _("SMS Folder 2");
	case GN_MT_F3: return _("SMS Folder 3");
	case GN_MT_F4: return _("SMS Folder 4");
	case GN_MT_F5: return _("SMS Folder 5");
	case GN_MT_F6: return _("SMS Folder 6");
	case GN_MT_F7: return _("SMS Folder 7");
	case GN_MT_F8: return _("SMS Folder 8");
	case GN_MT_F9: return _("SMS Folder 9");
	case GN_MT_F10: return _("SMS Folder 10");
	case GN_MT_F11: return _("SMS Folder 11");
	case GN_MT_F12: return _("SMS Folder 12");
	case GN_MT_F13: return _("SMS Folder 13");
	case GN_MT_F14: return _("SMS Folder 14");
	case GN_MT_F15: return _("SMS Folder 15");
	case GN_MT_F16: return _("SMS Folder 16");
	case GN_MT_F17: return _("SMS Folder 17");
	case GN_MT_F18: return _("SMS Folder 18");
	case GN_MT_F19: return _("SMS Folder 19");
	case GN_MT_F20: return _("SMS Folder 20");
	default: return _("Unknown");
	}
}

/**
 * gn_number_sanitize - Remove all white charactes from the string
 * @number: input/output number string
 * @maxlen: maximal number length
 *
 * Use this function to sanitize GSM phone number format. It changes
 * number argument.
 */
API void gn_number_sanitize(char *number, int maxlen)
{
	char *iter, *e;

	iter = e = number;
	while (*iter && *e && (e - number < maxlen)) {
		*iter = *e;
		if (isspace(*iter)) {
			while (*e && isspace(*e) && (e - number < maxlen)) {
				e++;
			}
		}
		*iter = *e;
		iter++;
		e++;
	}
	*iter = 0;
}

/**
 * gn_phonebook_entry_sanitize - Remove all white charactes from the string
 * @entry: phonebook entry to sanitize
 *
 * Use this function before any attempt to write an entry to the phone.
 */
API void gn_phonebook_entry_sanitize(gn_phonebook_entry *entry)
{
	int i;

	gn_number_sanitize(entry->number, GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1);
	for (i = 0; i < entry->subentries_count; i++) {
		if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number)
			gn_number_sanitize(entry->subentries[i].data.number, GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1);
	}
}

/* 
 * This very small function is just to make it easier to clear
 * the data struct every time one is created 
 */
API void gn_data_clear(gn_data *data)
{
	memset(data, 0, sizeof(gn_data));
}
