/*

  $Id$
  
  G N O K I I

  A Linux/Unix GUI for Nokia mobile phones.

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

  Copyright (C) 2002 Pavel Machek <pavel@ucw.cz>

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "gnokii/common.h"

API int gn_phonebook2vcard(FILE * f, gn_phonebook_entry *entry, char *addon)
{
	char buf2[1024];
	int i;

	fprintf(f, "BEGIN:VCARD\n");
	fprintf(f, "VERSION:3.0\n");
	fprintf(f, "FN:%s\n", entry->name);
	fprintf(f, "TEL;VOICE:%s\n", entry->number);

	fprintf(f, "X_GSM_STORE_AT:%s\n", buf2);
	fprintf(f, "X_GSM_CALLERGROUP:%d\n", entry->caller_group);
	fprintf(f, "%s", addon);

	/* Add ext. pbk info if required */
	for (i = 0; i < entry->subentries_count; i++) {
		switch (entry->subentries[i].entry_type) {
		case 0x08:
			fprintf(f, "EMAIL;INTERNET:%s\n", entry->subentries[i].data.number);
			break;
		case 0x09:
			fprintf(f, "ADR;HOME:%s\n", entry->subentries[i].data.number);
			break;
		case 0x0a:
			fprintf(f, "NOTE:%s\n", entry->subentries[i].data.number);
			break;
		case 0x0b:
			switch (entry->subentries[i].number_type) {
			case 0x02:
				fprintf(f, "TEL;HOME:%s\n", entry->subentries[i].data.number);
				break;
			case 0x03:
				fprintf(f, "TEL;CELL:%s\n", entry->subentries[i].data.number);
				break;
			case 0x04:
				fprintf(f, "TEL;FAX:%s\n", entry->subentries[i].data.number);
				break;
			case 0x06:
				fprintf(f, "TEL;WORK:%s\n", entry->subentries[i].data.number);
				break;
			case 0x0a:
				fprintf(f, "TEL;PREF:%s\n", entry->subentries[i].data.number);
				break;
			default:
				fprintf(f, "TEL;X_UNKNOWN_%d: %s\n", entry->subentries[i].number_type, entry->subentries[i].data.number);
				break;
			}
			break;
		case 0x2c:
			fprintf(f, "URL:%s\n", entry->subentries[i].data.number);
			break;
		default:
			fprintf(f, "X_GNOKII_%d: %s\n", entry->subentries[i].entry_type, entry->subentries[i].data.number);
			break;
		}
	}

	fprintf(f, "END:VCARD\n\n");
	return 0;
}

#define BEGINS(a) ( !strncmp(buf, a, strlen(a)) )
#define STORE2(a, b, c) if (BEGINS(a)) { c; strcpy(b, buf+strlen(a)+1); continue; }
#define STORE(a, b) STORE2(a, b, (void) 0)

#define STORESUB(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, entry->subentries[entry->subentries_count].entry_type = c);
#define STORENUM(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, entry->subentries[entry->subentries_count].entry_type = 0x0b; entry->subentries[entry->subentries_count].number_type = c);

#undef ERROR
#define ERROR(a) fprintf(stderr, "%s\n", a)

API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[10240];

	memset(entry, 0, sizeof(*entry));

	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("BEGIN:VCARD"))
			break;
	}

	while (1) {
		if (!fgets(buf, 1024, f)) {
			ERROR("Vcard began but not ended?");
			return -1;
		}
		STORE("FN:", entry->name);
		STORE("TEL;VOICE:", entry->number);

		STORESUB("URL:", 0x2c);
		STORESUB("EMAIL;INTERNET:", 0x08);
		STORESUB("ADR;HOME:", 0x09);
		STORESUB("NOTE:", 0x0a);

		STORENUM("TEL;HOME:", 0x02);
		STORENUM("TEL;CELL:", 0x03);
		STORENUM("TEL;FAX:", 0x04);
		STORENUM("TEL;WORK:", 0x06);
		STORENUM("TEL;PREF:", 0x0a);

		if (BEGINS("END:VCARD"))
			break;
	}
	return 0;
}
