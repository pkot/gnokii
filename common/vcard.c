/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2002 Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2003-2004 Pawel Kot

 */

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

/*
  Copies at most @maxcount strings, each of @maxsize length to the given
  pointers from a vCard property, such as
  ADR;TYPE=HOME,PREF:PO Box;Ext Add;Street;Town;State;ZIP;Country

  Returns: the number of strings copied
 */
int copy_fields(const char *str, int maxcount, size_t maxsize, ...)
{
	va_list ap;
	int count;
	size_t size;
	char *dest;

	va_start(ap, maxsize);

	for (count = maxcount; count && *str; count--) {
		dest = va_arg(ap, char *);

		size = maxsize;
		while (size && *str) {
			if (*str == ';') {
				str++;
				break;
			}
			*dest++ = *str++;
			size--;
		}
		*dest = '\0';
	}

	va_end(ap);

	return maxcount - count;
}

/* Write a string to a file doing folding when needed (see RFC 2425) */
static void vcard_append_printf(vcard_string *str, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	size_t len, to_copy;
	int lines, l;
	char *s = "\r\n";

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	/* Split lines according to sections 5.8.1 and 5.8.2 of http://www.ietf.org/rfc/rfc2425.txt */
	len = strlen(buf);
	/* Number of lines needed after the first one:
	 * at most 75 characters in the first line
	 * at most space+74 characters in each line beyond the first */
	if (len < 2)
		lines = 0;
	else
		lines = (len - 75 + 73) / 74;
	/* Current string length
	 * plus length of the string to be appended
	 * plus 2 characters for \r\n at the end of the first line,
	 * plus 3 characters for space at the beginning and \r\n at the end of each line beyond the first
	 * and a NUL byte to finish it off */
	str->str = realloc(str->str, str->len + len + 2 + lines * 3 + 1);
	str->end = str->str + str->len;

	to_copy = GNOKII_MIN(75, len);
	memcpy(str->end, buf, to_copy);
	str->end += to_copy;
	len -= to_copy;

	for (l = 0; l < lines; l++) {
		char *s = "\r\n ";

		memcpy(str->end, s, 3);
		str->end += 3;
		to_copy = GNOKII_MIN(74, len);
		memcpy(str->end, buf + 75 + 74 * l, to_copy);
		str->end += to_copy;
		len -= to_copy;
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

	if (*entry->number)
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
				vcard_append_printf(&str, "TEL;TYPE=VOICE:%s", entry->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_NUMBER_General:
				vcard_append_printf(&str, "TEL;TYPE=PREF:%s", entry->subentries[i].data.number);
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
			vcard_append_printf(&str, "BDAY:%04u%02u%02uT%02u%02u%02u",
				entry->subentries[i].data.date.year, entry->subentries[i].data.date.month, entry->subentries[i].data.date.day,
				entry->subentries[i].data.date.hour, entry->subentries[i].data.date.minute, entry->subentries[i].data.date.second);
			break;
		case GN_PHONEBOOK_ENTRY_Date:
			/* This is used when reading DC, MC, RC so the vcard *reader* won't read it back */
			vcard_append_printf(&str, "REV:%04u%02u%02uT%02u%02u%02u",
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
		case GN_PHONEBOOK_ENTRY_FirstName:
		case GN_PHONEBOOK_ENTRY_Group:
		case GN_PHONEBOOK_ENTRY_Image:
		case GN_PHONEBOOK_ENTRY_LastName:
		case GN_PHONEBOOK_ENTRY_Location:
		case GN_PHONEBOOK_ENTRY_Logo:
		case GN_PHONEBOOK_ENTRY_LogoSwitch:
		case GN_PHONEBOOK_ENTRY_Pointer:
		case GN_PHONEBOOK_ENTRY_Ringtone:
		case GN_PHONEBOOK_ENTRY_RingtoneAdv:
		case GN_PHONEBOOK_ENTRY_Video:
		case GN_PHONEBOOK_ENTRY_CallDuration:
			/* Ignore */
			break;
		case GN_PHONEBOOK_ENTRY_City:
		case GN_PHONEBOOK_ENTRY_Country:
		case GN_PHONEBOOK_ENTRY_ExtendedAddress:
		case GN_PHONEBOOK_ENTRY_FormalName:
		case GN_PHONEBOOK_ENTRY_Name:
		case GN_PHONEBOOK_ENTRY_PostalAddress:
		case GN_PHONEBOOK_ENTRY_StateProvince:
		case GN_PHONEBOOK_ENTRY_Street:
		case GN_PHONEBOOK_ENTRY_ZipCode:
			add_slashes(name, entry->subentries[i].data.number, sizeof(name), strlen(entry->subentries[i].data.number));
			vcard_append_printf(&str, "X-GNOKII-%d:%s", entry->subentries[i].entry_type, name);
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

#define STORENUM(a, b) if (BEGINS(a)) { strip_slashes(b, buf + strlen(a), line_len - strlen(a), GN_PHONEBOOK_NUMBER_MAX_LENGTH); }
#define STORESUB(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = c);
#define STORESUBNUM(a, c) if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				STORE2(a, entry->subentries[entry->subentries_count++].data.number, \
				entry->subentries[entry->subentries_count].entry_type = GN_PHONEBOOK_ENTRY_Number; \
				entry->subentries[entry->subentries_count].number_type = c);
#define STOREMEMTYPE(a, memory_type) if (BEGINS(a)) { STRIP(a, memory_name); memory_type = gn_str2memory_type(memory_name); }
#define STOREDATE(a, c) if (BEGINS(a)) { \
				if (entry->subentries_count == GN_PHONEBOOK_SUBENTRIES_MAX_NUMBER) return -1; \
				memset(&entry->subentries[entry->subentries_count].data.date, 0, sizeof(entry->subentries[entry->subentries_count].data.date)); \
				sscanf(buf+strlen(a), "%4u%2u%2uT%2u%2u%2u", \
					&entry->subentries[entry->subentries_count].data.date.year, \
					&entry->subentries[entry->subentries_count].data.date.month, \
					&entry->subentries[entry->subentries_count].data.date.day, \
					&entry->subentries[entry->subentries_count].data.date.hour, \
					&entry->subentries[entry->subentries_count].data.date.minute, \
					&entry->subentries[entry->subentries_count].data.date.second); \
				entry->subentries[entry->subentries_count].entry_type = c; \
				entry->subentries_count++; \
			} while (0)

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
	str_append_printf(&str, buf);

	retval = -1;
	while (fgets(buf, 1024, f)) {
		str_append_printf(&str, buf);
		if (BEGINS("END:VCARD")) {
			retval = gn_vcardstr2phonebook(str.str, entry);
			break;
		}
	}

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
	fold = strstr(v, "\r\n");
	while (fold != NULL) {
		memmove(fold, fold + 1, strlen(fold));
		fold = strstr(fold, "\r\n");
	}
	fold = strstr(v, "\n ");
	while (fold != NULL) {
		memmove(fold, fold + 2, strlen(fold) - 1);
		fold = strstr(fold, "\n ");
	}
	fold = strstr(v, "\n\t");
	while (fold != NULL) {
		memmove(fold, fold + 2, strlen(fold) - 1);
		fold = strstr(fold, "\n\t");
	}

	/* Count the number of lines */
	s = strstr(v, "\n");
	for (num_lines = 0; s != NULL; num_lines++) {
		s = strstr(s + 1, "\n");
	}

	/* FIXME on error STORESUB() and STORESUBNUM() leak memory allocated by gnokii_strsplit() */
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
			if (0 < copy_fields(buf +2 , 5, GN_PHONEBOOK_PERSON_MAX_LENGTH,
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
		STORENUM("TEL;TYPE=PREF,VOICE:", entry->number);

		if (BEGINS("ADR;TYPE=HOME,PREF:")) {
			if (0 < copy_fields(buf + 19, 7, GN_PHONEBOOK_ADDRESS_MAX_LENGTH,
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

		STORESUB("TITLE:", GN_PHONEBOOK_ENTRY_JobTitle);
		STORESUB("ORG:", GN_PHONEBOOK_ENTRY_Company);
		STORESUB("NICKNAME:", GN_PHONEBOOK_ENTRY_Nickname);
		STORESUB("X-SIP;POC:", GN_PHONEBOOK_ENTRY_PTTAddress);
		STORESUB("X-WV-ID:", GN_PHONEBOOK_ENTRY_UserID);

		STOREDATE("BDAY:", GN_PHONEBOOK_ENTRY_Birthday);

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

		STORESUBNUM("TEL;TYPE=HOME:", GN_PHONEBOOK_NUMBER_Home);
		STORESUBNUM("TEL;TYPE=CELL:", GN_PHONEBOOK_NUMBER_Mobile);
		STORESUBNUM("TEL;TYPE=FAX:", GN_PHONEBOOK_NUMBER_Fax);
		STORESUBNUM("TEL;TYPE=WORK:", GN_PHONEBOOK_NUMBER_Work);
		STORESUBNUM("TEL;TYPE=PREF:", GN_PHONEBOOK_NUMBER_General);
		STORESUBNUM("TEL;TYPE=VOICE:", GN_PHONEBOOK_NUMBER_Common);
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

