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

  This file is based on vcard.c written by Pavel Machek.

  Copyright (C) 2003 Pawel Kot <pkot@linuxnews.pl>

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "compat.h"
#include "gnokii.h"

API int gn_phonebook2ldif(FILE *f, gn_phonebook_entry *entry)
{
	char *aux;
	int i;

	fprintf(f, "dn: cn=%s\n", entry->name);
	fprintf(f, "objectclass: top\n");
	fprintf(f, "objectclass: person\n");
	fprintf(f, "objectclass: organizationalPerson\n");
	fprintf(f, "objectclass: inetOrgPerson\n");
	fprintf(f, "objectclass: mozillaAbPersonObsolete\n");

	aux = rindex(entry->name, ' ');
	if (aux) *aux = 0;
	fprintf(f, "givenName: %s\n", entry->name);
	fprintf(f, "sn: %s\n", aux+1);
	if (aux) *aux = ' ';
	fprintf(f, "cn: %s\n", entry->name);

	/* Add ext. pbk info if required */
	for (i = 0; i < entry->subentries_count; i++) {
		switch (entry->subentries[i].entry_type) {
		case 0x08:
			fprintf(f, "mail: %s\n", entry->subentries[i].data.number);
			break;
		case 0x09:
			fprintf(f, "homePostalAddress: %s\n", entry->subentries[i].data.number);
			break;
		case 0x0a:
			fprintf(f, "Description: %s\n", entry->subentries[i].data.number);
			break;
		case 0x0b:
			switch (entry->subentries[i].number_type) {
			case 0x02:
				fprintf(f, "homePhone: %s\n", entry->subentries[i].data.number);
				break;
			case 0x03:
				fprintf(f, "mobile: %s\n", entry->subentries[i].data.number);
				break;
			case 0x04:
				fprintf(f, "fax: %s\n", entry->subentries[i].data.number);
				break;
			case 0x06:
				fprintf(f, "workPhone: %s\n", entry->subentries[i].data.number);
				break;
			case 0x0a:
				fprintf(f, "telephoneNumber: %s\n", entry->subentries[i].data.number);
				break;
			default:
				break;
			}
			break;
		case 0x2c:
			fprintf(f, "homeurl: %s\n", entry->subentries[i].data.number);
			break;
		default:
			fprintf(f, "custom%d: %s\n", entry->subentries[i].entry_type, entry->subentries[i].data.number);
			break;
		}
	}

	fprintf(f, "\n");
	return 0;
}

#define BEGINS(a) ( !strncmp(buf, a, strlen(a)) )
#define STORE2(a, b, c) if (BEGINS(a)) { c; strcpy(b, buf+strlen(a)+1); continue; }
#define STORE(a, b) STORE2(a, b, (void) 0)

#define STORESUB(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, entry->subentries[entry->subentries_count].entry_type = c);
#define STORENUM(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, entry->subentries[entry->subentries_count].entry_type = 0x0b; entry->subentries[entry->subentries_count].number_type = c);

#undef ERROR
#define ERROR(a) fprintf(stderr, "%s\n", a)

API int gn_ldif2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[10240];

	memset(entry, 0, sizeof(*entry));

	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("dn: "))
			break;
	}

	while (1) {
		if (!fgets(buf, 1024, f)) {
			ERROR("LDIF began but not ended?");
			return -1;
		}
		STORE("cn: ", entry->name);
		STORE("telephoneNumber: ", entry->number);

		STORESUB("homeurl: ", 0x2c);
		STORESUB("mail: ", 0x08);
		STORESUB("homePostalAddress: ", 0x09);
		STORESUB("Description: ", 0x0a);

		STORENUM("homePhone: ", 0x02);
		STORENUM("mobile: ", 0x03);
		STORENUM("fax: ", 0x04);
		STORENUM("workPhone: ", 0x06);
		STORENUM("telephoneNumber: ", 0x0a);

		if (BEGINS("\n"))
			break;
	}
	return 0;
}
