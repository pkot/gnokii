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

typedef struct {
	char *str;
	char *end;
	unsigned int len;
} vcard_string;

/* Write a string to a file doing folding when needed (see RFC 2425) */
static void vcard_append_printf(vcard_string *str, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	int len, lines, l;
	char *s = "\r\n";

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	/* Number of lines needed */
	lines = strlen(buf) / 76 + 1;
	/* 3 characters for each line beyond the first one,
	 * plus the length of the buffer
	 * plus the line feed and the nul byte to finish it off */
	len = (lines - 1) * 3 + strlen(buf) + 3;

	/* The first malloc must have a nul byte at the end */
	if (str->str)
		str->str = realloc(str->str, len + str->len);
	else
		str->str = realloc(str->str, len + 1);
	if (str->end == NULL)
		str->end = str->str;
	else
		str->end = str->str + str->len;

	for (l = 0; l < lines; l++) {
		int to_copy;

		to_copy = GNOKII_MIN(76, strlen(buf) - 76 * l);
		memcpy(str->end,  buf + 76 * l, to_copy);
		str->end = str->end + to_copy;
		if (l != lines - 1) {
			char *s = "\r\n ";
			memcpy(str->end, s, 3);
			str->end += 3;
		}
	}

	memcpy(str->end, s, 2);
	str->end += 2;
	str->end[0] = '\0';

	str->len = str->end - str->str;
}

GNOKII_API char * gn_phonebook2vcardstr(gn_phonebook_entry *entry)
{
	vcard_string str;
	int i;
	char name[2 * GN_PHONEBOOK_NAME_MAX_LENGTH];

	memset(&str, 0, sizeof(str));

	vcard_append_printf(&str, "BEGIN:VCARD");
	vcard_append_printf(&str, "VERSION:3.0");
	add_slashes(name, entry->name, sizeof(name), strlen(entry->name));
	vcard_append_printf(&str, "FN:%s", name);

	if (entry->person.has_person)
		vcard_append_printf(&str, "N:%s;%s;%s;%s;%s",
			entry->person.family_name[0]        ? entry->person.family_name        : "",
			entry->person.given_name[0]         ? entry->person.given_name         : "",
			entry->person.additional_names[0]   ? entry->person.additional_names   : "",
			entry->person.honorific_prefixes[0] ? entry->person.honorific_prefixes : "",
			entry->person.honorific_suffixes[0] ? entry->person.honorific_suffixes : "");
	else
		vcard_append_printf(&str, "N:%s", name);

	vcard_append_printf(&str, "TEL;TYPE=PREF,VOICE:%s", entry->number);
	vcard_append_printf(&str, "X-GSM-MEMORY:%s", gn_memory_type2str(entry->memory_type));
	vcard_append_printf(&str, "X-GSM-LOCATION:%d", entry->location);
	vcard_append_printf(&str, "X-GSM-CALLERGROUP:%d", entry->caller_group);
	vcard_append_printf(&str, "CATEGORIES:%s", gn_phonebook_group_type2str(entry->caller_group));

	if (entry->address.has_address)
		vcard_append_printf(&str, "ADR;TYPE=HOME,PREF:%s;%s;%s;%s;%s;%s;%s",
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
			vcard_append_printf(&str, "EMAIL;TYPE=INTERNET:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Postal:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "ADR;TYPE=HOME:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Note:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "NOTE:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Number:
			switch (entry->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				vcard_append_printf(&str, "TEL;TYPE=HOME:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				vcard_append_printf(&str, "TEL;TYPE=CELL:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				vcard_append_printf(&str, "TEL;TYPE=FAX:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				vcard_append_printf(&str, "TEL;TYPE=WORK:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_None:
			case GN_PHONEBOOK_NUMBER_Common:
			case GN_PHONEBOOK_NUMBER_General:
				vcard_append_printf(&str, "TEL;TYPE=VOICE:%s", entry->subentries[i].data.number);
				break;
			default:
				vcard_append_printf(&str, "TEL;TYPE=X-UNKNOWN-%d: %s", entry->subentries[i].number_type, entry->subentries[i].data.number);
				break;
			}
			break;
		case GN_PHONEBOOK_ENTRY_URL:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "URL:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_JobTitle:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "TITLE:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Company:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "ORG:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Nickname:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "NICKNAME:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Birthday:
			vcard_append_printf(&str, "BDAY:%04u%02u%02u%02uT%02u%02u",
				entry->subentries[i].data.date.year, entry->subentries[i].data.date.month, entry->subentries[i].data.date.day,
				entry->subentries[i].data.date.hour, entry->subentries[i].data.date.minute, entry->subentries[i].data.date.second);
			break;
		case GN_PHONEBOOK_ENTRY_ExtGroup:
			vcard_append_printf(&str, "X-GSM-CALLERGROUPID:%d", entry->subentries[i].data.id);
			break;
		case GN_PHONEBOOK_ENTRY_PTTAddress:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "X-SIP;POC:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_UserID:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "X-WV-ID:%s", name);
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
			vcard_append_printf(&str, "X-GNOKII-%d: %s", entry->subentries[i].entry_type, name);
			break;
		}
	}

	vcard_append_printf(&str, "END:VCARD");
	/* output an empty line */
	vcard_append_printf(&str, "");

	return str.str;
}

GNOKII_API int gn_phonebook2vcard(FILE *f, gn_phonebook_entry *entry, char *location)
{
	char *vcard;
	int retval;

	vcard = gn_phonebook2vcardstr(entry);
	if (vcard == NULL)
		return -1;
	retval = fputs(vcard, f);
	free(vcard);

	return retval;
}

#define BEGINS(a) ( !strncmp(buf, a, strlen(a)) )
#define STRIP(a, b) strip_slashes(b, buf+strlen(a), line_len - strlen(a), GN_PHONEBOOK_NAME_MAX_LENGTH)
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

static void str_append_printf(vcard_string *str, const char *s)
{
	int len;

	if (str->str == NULL) {
		str->str = strdup(s);
		str->len = strlen(s) + 1;
		return;
	}

	len = strlen(s);
	str->str = realloc(str->str, len + str->len);

	memcpy(str->str + str->len - 1, s, len);
	str->len = str->len + len;
	str->end = str->str + str->len;

	/* NULL-terminate the string now */
	*(str->end - 1) = '\0';
}

GNOKII_API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[1024];
	vcard_string str;
	int retval;

	memset(&str, 0, sizeof(str));

	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("BEGIN:VCARD"))
			break;
	}

	str_append_printf(&str, "BEGIN:VCARD\r\n");
	while (fgets(buf, 1024, f)) {
		str_append_printf(&str, buf);
		if (BEGINS("END:VCARD"))
			break;
	}

	retval = gn_vcardstr2phonebook(str.str, entry);
	free(str.str);

	return retval;
}

/* We assume gn_phonebook_entry is ready for writing in, ie. no rubbish inside */
GNOKII_API int gn_vcardstr2phonebook(const char *vcard, gn_phonebook_entry *entry)
{
	char memloc[10];
	char memory_name[10];
	char *v, *fold, *s, **lines;
	int num_lines, i;

	if (vcard == NULL)
		return -1;

	memset(memloc, 0, sizeof(memloc));
	memset(memory_name, 0, sizeof(memory_name));

	/* Remove folding */
	v = strdup(vcard);
	fold = strstr(v, "\n ");
	while (fold != NULL) {
		memmove(fold, fold + 2, strlen(fold) - 1);
		fold = strstr(fold, "\n ");
	}
	fold = strstr(v, "\n\t");
	while (fold != NULL) {
		memmove(fold, fold + 2, strlen(fold) - 1);
		fold = strstr(v, "\n\t");
	}

	/* Count the number of lines */
	s = strstr(v, "\n");
	for (num_lines = 0; s != NULL; num_lines++) {
		s = strstr(s + 1, "\n");
	}

	/* FIXME on error STORESUB() and STORENUM() leak memory allocated by gnokii_strsplit() */
	lines = gnokii_strsplit(v, "\n", num_lines);

	for (i = 0; i < num_lines; i++) {
		char *buf;
		int line_len;

		if (lines[i] == NULL || *lines[i] == '\0')
			continue;

		buf = lines[i];
		line_len = strlen(buf);

		/* Strip traling '\r's */
		while (line_len > 0 && buf[line_len-1] == '\r')
			buf[--line_len] = '\0';

		if (BEGINS("N:")) {
			/* 64 is the value of GN_PHONEBOOK_PERSON_MAX_LENGTH */
			/* FIXME sscanf() doesn't accept empty fields */
			if (0 < sscanf(buf +2 , "%64[^;];%64[^;];%64[^;];%64[^;];%64[^;]\n",
				entry->person.family_name,
				entry->person.given_name,
				entry->person.additional_names,
				entry->person.honorific_prefixes,
				entry->person.honorific_suffixes
			)) {
				entry->person.has_person = 1;
				/* Temporary solution. In the phone driver we should handle person struct */
				if (entry->person.family_name[0]) {
					strcpy(entry->subentries[entry->subentries_count].data.number, entry->person.family_name);
					entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_LastName;
					entry->subentries_count++;
				}
				if (entry->person.given_name[0]) {
					strcpy(entry->subentries[entry->subentries_count].data.number, entry->person.given_name);
					entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_FirstName;
					entry->subentries_count++;
				}
			}
			continue;
		}
		STORE("FN:", entry->name);
		STORE("TEL;TYPE=PREF,VOICE:", entry->number);
		STORE("TEL;TYPE=PREF:", entry->number);

		if (BEGINS("ADR;TYPE=HOME,PREF:")) {
			/* 64 is the value of GN_PHONEBOOK_ADDRESS_MAX_LENGTH */
			/* FIXME sscanf() doesn't accept empty fields */
			if (0 < sscanf(buf + 19, "%64[^;];%64[^;];%64[^;];%64[^;];%64[^;];%64[^;];%64[^;]\n",
				entry->address.post_office_box,
				entry->address.extended_address,
				entry->address.street,
				entry->address.city,
				entry->address.state_province,
				entry->address.zipcode,
				entry->address.country
			)) {
				entry->address.has_address = 1;
			}
			continue;
		}

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
		if (BEGINS("X-GSM-CALLERGROUPID:")) {
			entry->subentries[entry->subentries_count].data.id = atoi(buf + strlen("X-GSM-CALLERGROUPID:"));
			entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_ExtGroup;
			entry->subentries_count++;
        		continue;
		}

		if (BEGINS("END:VCARD"))
			break;
	}
	free(v);
	gnokii_strfreev(lines);

	return 0;
}

