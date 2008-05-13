/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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
  Copyright (C) 2003-2004 Pawel Kot

 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "gnokii.h"
#include "gnokii-internal.h"

GNOKII_API int gn_phonebook2vcard(FILE *f, gn_phonebook_entry *entry, char *location)
{
	int i;
	char name[2 * GN_PHONEBOOK_NAME_MAX_LENGTH];

	fprintf(f, "BEGIN:VCARD\n");
	fprintf(f, "VERSION:3.0\n");
	add_slashes(name, entry->name, sizeof(name), strlen(entry->name));
	fprintf(f, "FN:%s\n", name);

	if (entry->person.has_person)
		fprintf(f, "N:%s;%s;%s;%s;%s\n",
			entry->person.family_name[0]        ? entry->person.family_name        : "",
			entry->person.given_name[0]         ? entry->person.given_name         : "",
			entry->person.additional_names[0]   ? entry->person.additional_names   : "",
			entry->person.honorific_prefixes[0] ? entry->person.honorific_prefixes : "",
			entry->person.honorific_suffixes[0] ? entry->person.honorific_suffixes : "");
	else
		fprintf(f, "N:%s\n", name);

	fprintf(f, "TEL;TYPE=PREF,VOICE:%s\n", entry->number);
	fprintf(f, "X-GSM-MEMORY:%s\n", gn_memory_type2str(entry->memory_type));
	fprintf(f, "X-GSM-LOCATION:%d\n", entry->location);
	fprintf(f, "X-GSM-CALLERGROUP:%d\n", entry->caller_group);
	fprintf(f, "CATEGORIES:%s\n", gn_phonebook_group_type2str(entry->caller_group));

	if (entry->address.has_address)
		fprintf(f, "ADR;TYPE=HOME,PREF:%s;%s;%s;%s;%s;%s;%s\n",
			entry->address.post_office_box[0]  ? entry->address.post_office_box  : "",
			entry->address.extended_address[0] ? entry->address.extended_address : "",
			entry->address.street[0]           ? entry->address.street           : "",
			entry->address.city[0]             ? entry->address.city             : "",
			entry->address.state_province[0]   ? entry->address.state_province   : "",
			entry->address.zipcode[0]          ? entry->address.zipcode          : "",
			entry->address.country[0]          ? entry->address.country          : "");

	/* Add ext. pbk info if required */
	for (i = 0; i < entry->subentries_count; i++) {
		switch (entry->subentries[i].entry_type) {
		case GN_PHONEBOOK_ENTRY_Email:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "EMAIL;TYPE=INTERNET:%s\n", name);
			break;
		case GN_PHONEBOOK_ENTRY_Postal:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "ADR;TYPE=HOME:%s\n", name);
			break;
		case GN_PHONEBOOK_ENTRY_Note:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "NOTE:%s\n", name);
			break;
		case GN_PHONEBOOK_ENTRY_Number:
			switch (entry->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				fprintf(f, "TEL;TYPE=HOME:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				fprintf(f, "TEL;TYPE=CELL:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				fprintf(f, "TEL;TYPE=FAX:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				fprintf(f, "TEL;TYPE=WORK:%s\n", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_None:
			case GN_PHONEBOOK_NUMBER_Common:
			case GN_PHONEBOOK_NUMBER_General:
				fprintf(f, "TEL;TYPE=VOICE:%s\n", entry->subentries[i].data.number);
				break;
			default:
				fprintf(f, "TEL;TYPE=X-UNKNOWN-%d: %s\n", entry->subentries[i].number_type, entry->subentries[i].data.number);
				break;
			}
			break;
		case GN_PHONEBOOK_ENTRY_URL:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "URL:%s\n", name);
			break;
		case GN_PHONEBOOK_ENTRY_JobTitle:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "TITLE:%s\n", name);
			break;
		case GN_PHONEBOOK_ENTRY_Company:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "ORG:%s\n", name);
			break;
		case GN_PHONEBOOK_ENTRY_Nickname:
			fprintf(f, "NICKNAME:%s\n", entry->subentries[i].data.number);
			break;
		case GN_PHONEBOOK_ENTRY_Birthday:
			fprintf(f, "BDAY:%s\n", entry->subentries[i].data.number);
			break;
		case GN_PHONEBOOK_ENTRY_Ringtone:
		case GN_PHONEBOOK_ENTRY_Pointer:
		case GN_PHONEBOOK_ENTRY_Logo:
		case GN_PHONEBOOK_ENTRY_LogoSwitch:
		case GN_PHONEBOOK_ENTRY_Group:
		case GN_PHONEBOOK_ENTRY_Location:
		case GN_PHONEBOOK_ENTRY_Image:
		case GN_PHONEBOOK_ENTRY_RingtoneAdv:
			/* Ignore */
			break;
		default:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			fprintf(f, "X-GNOKII-%d: %s\n", entry->subentries[i].entry_type, name);
			break;
		}
	}

	fprintf(f, "END:VCARD\n\n");
	return 0;
}

#define BEGINS(a) ( !strncmp(buf, a, strlen(a)) )
#define STRIP(a, b) strip_slashes(b, buf+strlen(a), line_len - strlen(a), line_len - strlen(a))
#define STORE3(a, b) if (BEGINS(a)) { STRIP(a, b); }
#define STORE2(a, b, c) if (BEGINS(a)) { c; STRIP(a, b); continue; }
#define STORE(a, b) STORE2(a, b, (void) 0)
#define STOREINT(a, b) if (BEGINS(a)) { b = atoi(buf+strlen(a)); continue; }

#define STORESUB(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORENUM(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
				entry->subentries[entry->subentries_count].number_type = c);
#define STOREMEMTYPE(a, memory_type) if (BEGINS(a)) { STRIP(a, memory_name); memory_type = gn_str2memory_type(memory_name); }

#undef ERROR
#define ERROR(a) fprintf(stderr, "%s\n", a)

/* We assume gn_phonebook_entry is ready for writing in, ie. no rubbish inside */
GNOKII_API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[1024];
	char memloc[10];
	char memory_name[10];

	memset(memloc, 0, 10);
	memset(memory_name, 0, 10);
	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("BEGIN:VCARD"))
			break;
	}

	while (1) {
		int line_len;
		if (!fgets(buf, 1024, f)) {
			ERROR(_("Vcard began but not ended?"));
			return -1;
		}
		/* There's either "\n" or "\r\n' sequence at the
		 * end of the line */
		line_len = strlen(buf);
		if (buf[line_len - 1] == '\n') {
			buf[--line_len] = 0;
		}
		if (!line_len)
			continue;
		if (buf[line_len - 1] == '\r') {
			buf[--line_len] = 0;
		}
		if (!line_len)
			continue; 

		if (BEGINS("N:")) {
			if (0 < sscanf(buf +2 , "%64[^;];%64[^;];%64[^;];%64[^;];%64[^;]\n",
				entry->person.family_name,
				entry->person.given_name,
				entry->person.additional_names,
				entry->person.honorific_prefixes,
				entry->person.honorific_suffixes
			)) {
				entry->person.has_person = 1;
			}
		}
		STORE("FN:", entry->name);
		STORE("TEL;TYPE=PREF,VOICE:", entry->number);
		STORE("TEL;TYPE=PREF:", entry->number);

		STORESUB("URL:", GN_PHONEBOOK_ENTRY_URL);
		STORESUB("EMAIL;TYPE=INTERNET:", GN_PHONEBOOK_ENTRY_Email);
		STORESUB("ADR;TYPE=HOME:", GN_PHONEBOOK_ENTRY_Postal);
		STORESUB("NOTE:", GN_PHONEBOOK_ENTRY_Note);

#if 1
		/* libgnokii 0.6.25 deprecates X_GSM_STORE_AT in favour of X-GSM-MEMORY and X-GSM-LOCATION and X_GSM_CALLERGROUP in favour of X-GSM-CALLERGROUP */
		STORE3("X_GSM_STORE_AT:", memloc);
		/* if the field is present and correctly formed */
		if (strlen(memloc) > 2) {
			entry->location = atoi(memloc + 2);
			memloc[2] = 0;
			entry->memory_type = gn_str2memory_type(memloc);
			continue;
		}
		STOREINT("X_GSM_CALLERGROUP:", entry->caller_group);
#endif
		STOREMEMTYPE("X-GSM-MEMORY:", entry->memory_type);
		STOREINT("X-GSM-LOCATION:", entry->location);
		STOREINT("X-GSM-CALLERGROUP:", entry->caller_group);

		STORENUM("TEL;TYPE=HOME:", GN_PHONEBOOK_NUMBER_Home);
		STORENUM("TEL;TYPE=CELL:", GN_PHONEBOOK_NUMBER_Mobile);
		STORENUM("TEL;TYPE=FAX:", GN_PHONEBOOK_NUMBER_Fax);
		STORENUM("TEL;TYPE=WORK:", GN_PHONEBOOK_NUMBER_Work);
		STORENUM("TEL;TYPE=PREF:", GN_PHONEBOOK_NUMBER_General);
		STORENUM("TEL;TYPE=VOICE:", GN_PHONEBOOK_NUMBER_Common);

		if (BEGINS("END:VCARD"))
			break;
	}
	return 0;
}
