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

#include "gsm-common.h"

API int phonebook2vcard(FILE * f, GSM_PhonebookEntry *entry, char *addon)
{
	char buf2[1024];
	int i;

	fprintf(f, "BEGIN:VCARD\n");
	fprintf(f, "VERSION:3.0\n");
	fprintf(f, "FN:%s\n", entry->Name);
	fprintf(f, "TEL;VOICE:%s\n", entry->Number);

	fprintf(f, "X_GSM_STORE_AT:%s\n", buf2);
	fprintf(f, "X_GSM_CALLERGROUP:%d\n", entry->Group);
	fprintf(f, "%s", addon);

	/* Add ext. pbk info if required */
	for (i = 0; i < entry->SubEntriesCount; i++) {
		switch (entry->SubEntries[i].EntryType) {
		case 0x08:
			fprintf(f, "EMAIL;INTERNET:%s\n", entry->SubEntries[i].data.Number);
			break;
		case 0x09:
			fprintf(f, "ADR;HOME:%s\n", entry->SubEntries[i].data.Number);
			break;
		case 0x0a:
			fprintf(f, "NOTE:%s\n", entry->SubEntries[i].data.Number);
			break;
		case 0x0b:
			switch (entry->SubEntries[i].NumberType) {
			case 0x02:
				fprintf(f, "TEL;HOME:%s\n", entry->SubEntries[i].data.Number);
				break;
			case 0x03:
				fprintf(f, "TEL;CELL:%s\n", entry->SubEntries[i].data.Number);
				break;
			case 0x04:
				fprintf(f, "TEL;FAX:%s\n", entry->SubEntries[i].data.Number);
				break;
			case 0x06:
				fprintf(f, "TEL;WORK:%s\n", entry->SubEntries[i].data.Number);
				break;
			case 0x0a:
				fprintf(f, "TEL;PREF:%s\n", entry->SubEntries[i].data.Number);
				break;
			default:
				fprintf(f, "TEL;X_UNKNOWN_%d: %s\n", entry->SubEntries[i].NumberType, entry->SubEntries[i].data.Number);
				break;
			}
			break;
		case 0x2c:
			fprintf(f, "URL:%s\n", entry->SubEntries[i].data.Number);
			break;
		default:
			fprintf(f, "X_GNOKII_%d: %s\n", entry->SubEntries[i].EntryType, entry->SubEntries[i].data.Number);
			break;
		}
	}

	fprintf(f, "END:VCARD\n\n");
	return 0;
}

#define BEGINS(a) ( !strncmp(buf, a, strlen(a)) )
#define STORE2(a, b, c) if (BEGINS(a)) { c; strcpy(b, buf+strlen(a)+1); continue; }
#define STORE(a, b) STORE2(a, b, (void) 0)

#define STORESUB(a, c) STORE2(a, entry->SubEntries[entry->SubEntriesCount++].data.Number, entry->SubEntries[entry->SubEntriesCount].EntryType = c);
#define STORENUM(a, c) STORE2(a, entry->SubEntries[entry->SubEntriesCount++].data.Number, entry->SubEntries[entry->SubEntriesCount].EntryType = 0x0b; entry->SubEntries[entry->SubEntriesCount].NumberType = c);


#define ERROR(a) fprintf(stderr, "%s\n", a)

API int vcard2phonebook(FILE *f, GSM_PhonebookEntry *entry)
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
		STORE("FN:", entry->Name);
		STORE("TEL;VOICE:", entry->Number);

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
