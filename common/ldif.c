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
#include "gnokii-internal.h"

/* a utility function to reuse code */
static int ldif_entry_write(FILE *f, const char *parameter, const char *value, int convertToUTF8)
{
	char *buf = NULL;

	if (string_base64(value)) {
		base64_encode(buf, value, convertToUTF8);
		fprintf(f, "%s:: %s\n", parameter, buf);
	} else {
		fprintf(f, "%s: %s\n", parameter, value);
	}
	return 1;
}

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

	aux = strrchr(entry->name, ' ');
	if (aux) *aux = 0;
	ldif_entry_write(f, "givenName", entry->name, 1);
	if (aux) ldif_entry_write(f, "sn", aux + 1, 1);
	if (aux) *aux = ' ';
	ldif_entry_write(f, "cn", entry->name, 1);

	/* Add ext. pbk info if required */
	for (i = 0; i < entry->subentries_count; i++) {
		switch (entry->subentries[i].entry_type) {
		case GN_PHONEBOOK_ENTRY_Email:
			ldif_entry_write(f, "mail", entry->subentries[i].data.number, 1);
			break;
		case GN_PHONEBOOK_ENTRY_Postal:
			ldif_entry_write(f, "homePostalAddress", entry->subentries[i].data.number, 1);
			break;
		case GN_PHONEBOOK_ENTRY_Note:
			ldif_entry_write(f, "Description", entry->subentries[i].data.number, 1);
			break;
		case GN_PHONEBOOK_ENTRY_Number:
			switch (entry->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				ldif_entry_write(f, "homePhone", entry->subentries[i].data.number, 1);
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				ldif_entry_write(f, "mobile", entry->subentries[i].data.number, 1);
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				ldif_entry_write(f, "fax", entry->subentries[i].data.number, 1);
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				ldif_entry_write(f, "workPhone", entry->subentries[i].data.number, 1);
				break;
			case GN_PHONEBOOK_NUMBER_General:
				ldif_entry_write(f, "telephoneNumber", entry->subentries[i].data.number, 1);
				break;
			default:
				break;
			}
			break;
		case GN_PHONEBOOK_ENTRY_URL:
			ldif_entry_write(f, "homeurl", entry->subentries[i].data.number, 1);
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
#define STORE2(a, b, c) if (BEGINS(a)) { c; strncpy(b, buf+strlen(a), strlen(buf)-strlen(a)-1); continue; }
#define STORE2_BASE64(a, b, c) if (BEGINS(a)) { c; base64_decode(b, buf+strlen(a), strlen(buf)-strlen(a)-1); continue; }

#define STORE(a, b) STORE2(a, b, (void) 0)
#define STORE_BASE64(a, b) STORE2_BASE64(a, b, (void) 0)

#define STORESUB(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORESUB_BASE64(a, c) STORE2_BASE64(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORENUM(a, c) STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
					entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
					entry->subentries[entry->subentries_count].number_type = c);
#define STORENUM_BASE64(a, c) STORE2_BASE64(a, entry->subentries[entry->subentries_count++].data.number, \
					entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
					entry->subentries[entry->subentries_count].number_type = c);

#undef ERROR
#define ERROR(a) fprintf(stderr, "%s\n", a)

/* We assume gn_phonebook_entry is ready for writing in, ie. no rubbish inside */
API int gn_ldif2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[10240];

	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("dn:"))
			break;
	}

	while (1) {
		if (!fgets(buf, 1024, f)) {
			ERROR("LDIF began but not ended?");
			return -1;
		}
		STORE("cn: ", entry->name);
		STORE_BASE64("cn:: ", entry->name);
		STORE("telephoneNumber: ", entry->number);
		STORE_BASE64("telephoneNumber:: ", entry->number);

		STORESUB("homeurl: ", GN_PHONEBOOK_ENTRY_URL);
		STORESUB_BASE64("homeurl:: ", GN_PHONEBOOK_ENTRY_URL);
		STORESUB("mail: ", GN_PHONEBOOK_ENTRY_Email);
		STORESUB_BASE64("mail:: ", GN_PHONEBOOK_ENTRY_Email);
		STORESUB("homePostalAddress: ", GN_PHONEBOOK_ENTRY_Postal);
		STORESUB_BASE64("homePostalAddress:: ", GN_PHONEBOOK_ENTRY_Postal);
		STORESUB("Description: ", GN_PHONEBOOK_ENTRY_Note);
		STORESUB_BASE64("Description:: ", GN_PHONEBOOK_ENTRY_Note);

		STORENUM("homePhone: ", GN_PHONEBOOK_NUMBER_Home);
		STORENUM_BASE64("homePhone:: ", GN_PHONEBOOK_NUMBER_Home);
		STORENUM("mobile: ", GN_PHONEBOOK_NUMBER_Mobile);
		STORENUM_BASE64("mobile:: ", GN_PHONEBOOK_NUMBER_Mobile);
		STORENUM("fax: ", GN_PHONEBOOK_NUMBER_Fax);
		STORENUM_BASE64("fax:: ", GN_PHONEBOOK_NUMBER_Fax);
		STORENUM("workPhone: ", GN_PHONEBOOK_NUMBER_Work);
		STORENUM_BASE64("workPhone:: ", GN_PHONEBOOK_NUMBER_Work);
		STORENUM("telephoneNumber: ", GN_PHONEBOOK_NUMBER_General);
		STORENUM_BASE64("telephoneNumber:: ", GN_PHONEBOOK_NUMBER_General);

		if (BEGINS("\n"))
			break;
	}
	return 0;
}
