/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolkit and driver for the mobile phones.

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

  Copyright (C) 1999 Hugh Blemings, Pavel Janik
  Copyright (C) 2002 Pavel Machek
  Copyright (C) 2003 Pawel Kot, Martin Goldhahn, BORBELY Zoltan

 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "gnokii.h"
#include "gnokii-internal.h"

/* write the string value to the file and convert it to base64 if necessary
   convertToUTF8 is necessary for Mozilla, but might not be appropriate for
   other programs that use the LDIF format, base64 encoding is compliant with the LDIF format
*/
static int ldif_entry_write(FILE *f, const char *parameter, const char *value, int convertToUTF8)
{
	char *buf = NULL;
	int n, inlen, buflen;

	if (string_base64(value)) {
		inlen = strlen(value);
		n = 2 * inlen * 4 / 3; /* * 2   -> UTF8
					* * 4/3 -> base 64
					*/
		if (n % 4)
			n += (4 - (n % 4)); /* base 64 padds with '=' to length dividable by 4 */

		buflen = n;
		buf = malloc(buflen + 1);
		if (convertToUTF8)
			n = utf8_base64_encode(buf, buflen, value, inlen);
		else
			n = base64_encode(buf, buflen, value, inlen);

		fprintf(f, "%s:: %s\n", parameter, buf);

		free(buf);
	} else {
		fprintf(f, "%s: %s\n", parameter, value);
	}
	return 1;
}

GNOKII_API int gn_phonebook2ldif(FILE *f, gn_phonebook_entry *entry)
{
	char *aux;
	int i;

	/* this works for the import, but mozilla exports strings that contain chars >= 0x80 as
		dn:: <utf-8+base64 encoded: cn=entryname>
	*/
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
	if (entry->caller_group) {
		char aux2[10];

		snprintf(aux2, sizeof(aux2), "%d", entry->caller_group);
		ldif_entry_write(f, "businessCategory", aux2, 1);
	}

	if (entry->subentries_count == 0) {
		ldif_entry_write(f, "telephoneNumber", entry->number, 1);
	}

	if (entry->address.has_address) {
		ldif_entry_write(f, "homePostalAddress", entry->address.post_office_box, 1);
	}

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
			case GN_PHONEBOOK_NUMBER_None:
			case GN_PHONEBOOK_NUMBER_Common:
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

#define BEGINS(a) ( !strncasecmp(buf, a, strlen(a)) )
#define STOREINT(a, b) if (BEGINS(a)) { b = atoi(buf+strlen(a)); continue; }
#define STORE2(a, b, c) if (BEGINS(a)) { c; strncpy(b, buf+strlen(a), strlen(buf)-strlen(a)-1); continue; }
#define STORE2_BASE64(a, b, c) if (BEGINS(a)) { c; utf8_base64_decode(b, GN_PHONEBOOK_NAME_MAX_LENGTH, buf+strlen(a), strlen(buf)-strlen(a)-1); continue; }

#define STORE(a, b) STORE2(a, b, (void) 0)
#define STORE_BASE64(a, b) STORE2_BASE64(a, b, (void) 0)

#define STORESUB(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORESUB_BASE64(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2_BASE64(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORENUM(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
					entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
					entry->subentries[entry->subentries_count].number_type = c);
#define STORENUM_BASE64(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2_BASE64(a, entry->subentries[entry->subentries_count++].data.number, \
					entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
					entry->subentries[entry->subentries_count].number_type = c);
#undef ERROR
#define ERROR(a) fprintf(stderr, "%s\n", a)

/* We assume gn_phonebook_entry is ready for writing in, ie. no rubbish inside */
GNOKII_API int gn_ldif2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[10240];
	int i;

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
		STORENUM("cellphone: ", GN_PHONEBOOK_NUMBER_Mobile);
		STORENUM_BASE64("cellphone:: ", GN_PHONEBOOK_NUMBER_Mobile);
		STORENUM("fax: ", GN_PHONEBOOK_NUMBER_Fax);
		STORENUM_BASE64("fax:: ", GN_PHONEBOOK_NUMBER_Fax);
		STORENUM("workPhone: ", GN_PHONEBOOK_NUMBER_Work);
		STORENUM_BASE64("workPhone:: ", GN_PHONEBOOK_NUMBER_Work);
		STORENUM("telephoneNumber: ", GN_PHONEBOOK_NUMBER_General);
		STORENUM_BASE64("telephoneNumber:: ", GN_PHONEBOOK_NUMBER_General);
		
		STOREINT("businessCategory: ", entry->caller_group);

		if (BEGINS("\n"))
			break;
	}
	/* set entry->number from the first sub-entry that is a number */
	for (i = 0; i < entry->subentries_count && entry->number[0] == 0; i++) {
		if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number) {
			switch (entry->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_None:
			case GN_PHONEBOOK_NUMBER_Common:
			case GN_PHONEBOOK_NUMBER_General:
			case GN_PHONEBOOK_NUMBER_Work: 
			case GN_PHONEBOOK_NUMBER_Home:
			case GN_PHONEBOOK_NUMBER_Mobile:
				dprintf("setting default number to %s\n", entry->subentries[i].data.number);
				snprintf(entry->number, sizeof(entry->number), "%s", entry->subentries[i].data.number);
				break;
			default:
				dprintf("unknown type\n");
				break;
			}
		}
	}
	return 0;
}
