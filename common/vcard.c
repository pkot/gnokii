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

typedef int (*print_func) (void *data, const char *fmt, ...);

/* Write a string to a file doing folding when needed (see RFC 2425) */
static int vcard_fprintf(FILE *f, const char *fmt, ...)
{
	char buf[1024], *current;
	size_t pos;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	for (pos = 1, current = buf; *current; pos++) {
		if (pos == 76) {
			fprintf(f, "\r\n ");
			pos = 0;
		}
		fputc(*current++, f);
	}
	fprintf(f, "\r\n");

	/* FIXME: count also end of line chars? */
	return current - buf;
}

typedef struct {
	char *str;
	char *end;
	unsigned int len;
} vcard_string;

static void vcard_append_printf(vcard_string *str, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	int len, lines, l;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	/* Number of lines needed */
	lines = strlen(buf) / 76 + 1;
	/* 3 characters for each line beyond the first one,
	 * plus the length of the buffer
	 * plus the line feed and the nul byte to finish it off */
	len = (lines - 1) * 3 + strlen (buf) + 3;

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

		to_copy = GNOKII_MIN(76, strlen (buf) - 76 * l);
		memcpy (str->end,  buf + 76 * l, to_copy);
		str->end = str->end + to_copy;
		if (l != lines - 1) {
			char *s = "\r\n ";
			memcpy (str->end, s, 3);
			str->end += 3;
		}
	}

	{
		char *s = "\r\n";
		memcpy (str->end, s, 2);
		str->end += 2;
		str->end[0] = '\0';
	}

	str->len = str->end - str->str;
}

static int phonebook2vcard_real(void *data, print_func func, gn_phonebook_entry *entry)
{
	int i;
	char name[2 * GN_PHONEBOOK_NAME_MAX_LENGTH];

	func(data, "BEGIN:VCARD");
	func(data, "VERSION:3.0");
	add_slashes(name, entry->name, sizeof(name), strlen(entry->name));
	func(data, "FN:%s", name);

	if (entry->person.has_person)
		func(data, "N:%s;%s;%s;%s;%s",
			entry->person.family_name[0]        ? entry->person.family_name        : "",
			entry->person.given_name[0]         ? entry->person.given_name         : "",
			entry->person.additional_names[0]   ? entry->person.additional_names   : "",
			entry->person.honorific_prefixes[0] ? entry->person.honorific_prefixes : "",
			entry->person.honorific_suffixes[0] ? entry->person.honorific_suffixes : "");
	else
		func(data, "N:%s", name);

	func(data, "TEL;TYPE=PREF,VOICE:%s", entry->number);
	func(data, "X-GSM-MEMORY:%s", gn_memory_type2str(entry->memory_type));
	func(data, "X-GSM-LOCATION:%d", entry->location);
	func(data, "X-GSM-CALLERGROUP:%d", entry->caller_group);
	func(data, "CATEGORIES:%s", gn_phonebook_group_type2str(entry->caller_group));

	if (entry->address.has_address)
		func(data, "ADR;TYPE=HOME,PREF:%s;%s;%s;%s;%s;%s;%s",
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
			func(data, "EMAIL;TYPE=INTERNET:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Postal:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			func(data, "ADR;TYPE=HOME:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Note:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			func(data, "NOTE:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Number:
			switch (entry->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				func(data, "TEL;TYPE=HOME:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				func(data, "TEL;TYPE=CELL:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				func(data, "TEL;TYPE=FAX:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				func(data, "TEL;TYPE=WORK:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_None:
			case GN_PHONEBOOK_NUMBER_Common:
			case GN_PHONEBOOK_NUMBER_General:
				func(data, "TEL;TYPE=VOICE:%s", entry->subentries[i].data.number);
				break;
			default:
				func(data, "TEL;TYPE=X-UNKNOWN-%d: %s", entry->subentries[i].number_type, entry->subentries[i].data.number);
				break;
			}
			break;
		case GN_PHONEBOOK_ENTRY_URL:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			func(data, "URL:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_JobTitle:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			func(data, "TITLE:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Company:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			func(data, "ORG:%s", name);
			break;
		case GN_PHONEBOOK_ENTRY_Nickname:
			func(data, "NICKNAME:%s", entry->subentries[i].data.number);
			break;
		case GN_PHONEBOOK_ENTRY_Birthday:
			func(data, "BDAY:%s", entry->subentries[i].data.number);
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
			func(data, "X-GNOKII-%d: %s", entry->subentries[i].entry_type, name);
			break;
		}
	}

	func(data, "END:VCARD");
	/* output an empty line */
	func(data, "");

	return 0;
}

GNOKII_API int gn_phonebook2vcard(FILE *f, gn_phonebook_entry *entry, char *location)
{
	return phonebook2vcard_real((void *) f, (print_func) vcard_fprintf, entry);
}

GNOKII_API char * gn_phonebook2vcardstr(gn_phonebook_entry *entry)
{
	vcard_string str;

	str.str = NULL;
	str.end = NULL;
	str.len = 0;
	phonebook2vcard_real(&str, (print_func) vcard_append_printf, entry);
	return str.str;
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

/* We assume gn_phonebook_entry is ready for writing in, ie. no rubbish inside */
GNOKII_API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry)
{
	char buf[1024];
	char memloc[10];
	char memory_name[10];

	memset(memloc, 0, sizeof(memloc));
	memset(memory_name, 0, sizeof(memory_name));
	while (1) {
		if (!fgets(buf, 1024, f))
			return -1;
		if (BEGINS("BEGIN:VCARD"))
			break;
	}

	while (1) {
		int line_len = 0;
		int c = 0;

		do {
			if (!fgets(buf + line_len, 1024 - line_len, f)) {
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
			/* perform line unfolding (see RFC2425) */
			c = fgetc(f);
		} while ((c == ' ') || (c == 0x09));
		/* ungetc() is EOF safe */
		ungetc(c, f);

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
	v = strdup (vcard);
	fold = strstr (v, "\r\n");
	while (fold == NULL) {
		memmove (fold, fold + 2, strlen (fold) - 2);
		fold = strstr (v, "\r\n");
	}

	/* Count the number of lines */
	s = strchr (v, '\n');
	for (num_lines = 0; s != NULL; num_lines++) {
		num_lines++;
	}

	lines = gnokii_strsplit (v, "\n", num_lines);

	for (i = 0; i < num_lines; i++) {
		const char *buf;
		int line_len;

		if (lines[i] == NULL || *lines[i] == '\0')
			continue;

		buf = lines[i];
		line_len = strlen (buf);

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

