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

  Copyright (C) 2004 Martin Goldhahn
  Copyright (C) 2004 Pawel Kot, BORBELY Zoltan

*/

#include "config.h"

#include <string.h>

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "gsm-filetypes.h"

/* We have to do this here since libical defines its own VERSION macro.
 * Define it unconditionally to have nicer code below. */
static const char gnokii_version[] = VERSION;


#ifdef HAVE_LIBICAL
#  include "ical.h"
/* mother's little helpers */
static int iterate_cal(icalcomponent *parent, int depth, int *count, icalcomponent **result, icalcomponent_kind kind);
static const char *comp_get_category(icalcomponent *comp);

/* This is a wrapper for fgets. icalparser_parse() requires a function
 * argument with different signature than fgets(). */
static char *ical_fgets(char *s, size_t size, void *d)
{
	return fgets(s, size, (FILE *)d);
}
#endif /* HAVE_LIBICAL */

/* This string is used so often that we keep it as long as the program is
 * running. Due to VERSION being defined in both libical and gnokii we need
 * to us a static string instead of a pre-compiler macro.
 */
static char vcal_prodid[64] = {0};
static inline const char *get_prodid()
{
	if (!*vcal_prodid) {
		sprintf(vcal_prodid, "//Gnokii.org//NONSGML v%s", gnokii_version);
	}
	return vcal_prodid;
}

/*
	ICALENDAR Reading functions
 */
API int gn_calnote2ical(FILE *f, gn_calnote *calnote)
{
#ifdef HAVE_LIBICAL
#  define MAX_PROP_INDEX 5
	icalcomponent *pIcal = NULL;
	struct icaltimetype stime = {0}, etime = {0}, atime = {0};
	icalproperty *properties[MAX_PROP_INDEX+1] = {0}; /* order and number of properties vary */
	int iprop = 0;
	char compuid[64];

	/* In at least some Nokia phones it is possible to skip the year of
	   birth, in this case year == 0xffff, so we set it to the arbitrary
	   value of 1800 */

	stime.year = (calnote->time.year == 0xffff ? 1800 : calnote->time.year);
	stime.month = calnote->time.month;
	stime.day = calnote->time.day;
	stime.hour = calnote->time.hour;
	stime.minute = calnote->time.minute;
	stime.second = calnote->time.second;

	stime.is_daylight = 1;

	if (calnote->end_time.year) {
		etime.year = (calnote->end_time.year == 0xffff ? 1800 : calnote->end_time.year);
		etime.month = calnote->end_time.month;
		etime.day = calnote->end_time.day;
		etime.hour = calnote->end_time.hour;
		etime.minute = calnote->end_time.minute;
		etime.second = calnote->end_time.second;
		etime.is_daylight = 1;

		properties[iprop++] = icalpropery_new_dtend(etime);
	}

	/* FIXME: how to set alarm?
	if (calnote->alarm.enabled) {
		atime.year = (calnote->alarm.timestamp.year == 0xffff ? 1800 : calnote->alarm.timestamp.year);
		atime.month = calnote->alarm.timestamp.month;
		atime.day = calnote->alarm.timestamp.day;
		atime.hour = calnote->alarm.timestamp.hour;
		atime.minute = calnote->alarm.timestamp.minute;
		atime.second = calnote->alarm.timestamp.second;
		atime.is_daylight = 1;

		properties[iprop++] = icalpropery_new_FIXME(atime);
	}
	*/

	/* TODO: should the strings be configurable? */
	switch(calnote->type) {
	case GN_CALNOTE_MEMO:
	case GN_CALNOTE_REMINDER:
		properties[iprop++] = icalproperty_new_categories("MISCELLANEOUS");
		properties[iprop++] = icalproperty_new_summary(calnote->text);
		break;
	case GN_CALNOTE_CALL:
		properties[iprop++] = icalproperty_new_categories("PHONE CALL");
		properties[iprop++] = icalproperty_new_summary(calnote->phone_number);
		properties[iprop++] = icalproperty_new_description(calnote->text);
		break;
	case GN_CALNOTE_MEETING:
		properties[iprop++] = icalproperty_new_categories("MEETING");
		properties[iprop++] = icalproperty_new_summary(calnote->text);
		if (calnote->mlocation[0])
			properties[iprop++] = icalproperty_new_location(calnote->mlocation);
		break;
	case GN_CALNOTE_BIRTHDAY:
		properties[iprop++] = icalproperty_new_categories("ANNIVERSARY");
		properties[iprop++] = icalproperty_new_summary(calnote->text);
		do {
			char rrule[64];
			sprintf(rrule, "FREQ=YEARLY;INTERVAL=1;BYMONTH=%d", stime.month);
			properties[iprop++] = icalproperty_new_rrule(icalrecurrencetype_from_string(rrule));
		} while (0);
		stime.is_date = 1;
		calnote->recurrence = GN_CALNOTE_YEARLY;
		break;
	default:
		properties[iprop++] = icalproperty_new_categories("UNKNOWN");
		properties[iprop++] = icalproperty_new_summary(calnote->text);
		break;
	}

	sprintf(compuid, "guid.gnokii.org_%d_%d", calnote->location, rand());

	pIcal = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT,
				    icalproperty_new_version("2.0"),
				    icalproperty_new_prodid(get_prodid()),
				    icalcomponent_vanew(ICAL_VEVENT_COMPONENT,
							icalproperty_new_dtstart(stime),
							icalproperty_new_uid(compuid),
							icalproperty_new_categories("GNOKII"),
							properties[0],
							properties[1],
							properties[2],
							properties[3],
							properties[4],
							0),
				    0);

	if (pIcal) {
		char *icalstrbuf = NULL;
		const char *icalstr = icalcomponent_as_ical_string(pIcal);

		/* conversion to UTF-8 */
		if (string_base64(icalstr)) {
			int icalstrlen = strlen(icalstr);
			icalstrbuf = malloc(icalstrlen * 2 + 1);
			utf8_encode(icalstrbuf, icalstrlen * 2, icalstr, icalstrlen);

			icalstr = icalstrbuf;
		}
		fprintf(f, "%s\n", icalstr);
		dprintf("%s\n", icalstr);
		if (icalstrbuf)
			free(icalstrbuf);

		icalcomponent_free(pIcal);
		pIcal = NULL;
	} else {
		fprintf(stderr, _("ERROR\n"));
	}
	return GN_ERR_NONE;
#else
	fprintf(f, "BEGIN:VCALENDAR\r\n");
	fprintf(f, "VERSION:1.0\r\n");
	fprintf(f, "BEGIN:VEVENT\r\n");
	fprintf(f, "CATEGORIES:");
	switch (calnote->type) {
	case GN_CALNOTE_MEMO:
	case GN_CALNOTE_REMINDER:
		fprintf(f, "MISCELLANEOUS\r\n");
		break;
	case GN_CALNOTE_CALL: 
		fprintf(f, "PHONE CALL\r\n");
		fprintf(f, "SUMMARY:%s\r\n", calnote->phone_number);
		fprintf(f, "DESCRIPTION:%s\r\n", calnote->text);
		break; 
	case GN_CALNOTE_MEETING: 
		fprintf(f, "MEETING\r\n"); 
		if (calnote->mlocation[0])
			fprintf(f, "LOCATION:%s\r\n", calnote->mlocation);
		break; 
	case GN_CALNOTE_BIRTHDAY: 
		fprintf(f, "SPECIAL OCCASION\r\n"); 
		break; 
	default: 
		fprintf(f, "UNKNOWN\r\n"); 
		break; 
	} 
	if (calnote->type != GN_CALNOTE_CALL)
		fprintf(f, "SUMMARY:%s\r\n", calnote->text);
	fprintf(f, "DTSTART:%04d%02d%02dT%02d%02d%02d\r\n", calnote->time.year,
		calnote->time.month, calnote->time.day, calnote->time.hour, 
		calnote->time.minute, calnote->time.second); 
	if (calnote->end_time.year) {
		fprintf(f, "DTEND:%04d%02d%02dT%02d%02d%02d\r\n", calnote->end_time.year, 
			calnote->end_time.month, calnote->end_time.day, calnote->end_time.hour, 
			calnote->end_time.minute, calnote->end_time.second); 
	}
	if (calnote->alarm.enabled) {
		fprintf(f, "AALARM:%04d%02d%02dT%02d%02d%02d\r\n", calnote->alarm.timestamp.year, 
		calnote->alarm.timestamp.month, calnote->alarm.timestamp.day, calnote->alarm.timestamp.hour, 
		calnote->alarm.timestamp.minute, calnote->alarm.timestamp.second); 
	} 
	switch (calnote->recurrence) { 
	case GN_CALNOTE_NEVER: 
		break; 
	case GN_CALNOTE_DAILY: 
		fprintf(f, "RRULE:FREQ=DAILY\r\n"); 
		break; 
	case GN_CALNOTE_WEEKLY: 
		fprintf(f, "RRULE:FREQ=WEEKLY\r\n"); 
		break; 
	case GN_CALNOTE_2WEEKLY: 
		fprintf(f, "RRULE:FREQ=WEEKLY;INTERVAL=2\r\n"); 
		break; 
	case GN_CALNOTE_MONTHLY: 
		fprintf(f, "RRULE:FREQ=MONTHLY\r\n"); 
		break; 
	case GN_CALNOTE_YEARLY: 
		fprintf(f, "RRULE:FREQ=YEARLY\r\n"); 
		break; 
	default: 
		fprintf(f, "RRULE:FREQ=HOURLY;INTERVAL=%d\r\n", calnote->recurrence); 
		break; 
	} 
	fprintf(f, "END:VEVENT\r\n"); 
	fprintf(f, "END:VCALENDAR\r\n");
	return GN_ERR_NONE;
#endif /* HAVE_LIBICAL */
}

static inline gn_calnote_type str2calnote_type(const char *str)
{
	if (!str) {
		return GN_CALNOTE_REMINDER;
	} else if (!strncasecmp("PHONE CALL", str, 10)) {
		return GN_CALNOTE_CALL;
	} else if (!strncasecmp("MEETING", str, 7)) {
		return GN_CALNOTE_MEETING;
	} else if (!strncasecmp("ANNIVERSARY", str, 11)) {
		return GN_CALNOTE_BIRTHDAY;
	}
	return GN_CALNOTE_REMINDER;
}

/* read a vcalendar event given by id from file f and store the data in calnote */
API int gn_ical2calnote(FILE *f, gn_calnote *calnote, int id)
{
	int retval = GN_ERR_FAILED;
#ifdef HAVE_LIBICAL
	icalparser *parser = NULL;
	icalcomponent *comp = NULL, *compresult = NULL;
	struct icaltimetype dtstart = {0}, dtend = {0};

	parser = icalparser_new();
	if (!parser) {
		return GN_ERR_FAILED;
	}

	icalparser_set_gen_data(parser, f);

	comp = icalparser_parse(parser, ical_fgets);
	if (!comp) {
		icalparser_free(parser);
		return GN_ERR_FAILED;
	}

	if (id < 1)
		id = 1;

	/* interate through the component */
	iterate_cal(comp, 0, &id, &compresult, ICAL_VEVENT_COMPONENT);

	if (!compresult) {
		fprintf(stderr, _("No component found.\n"));
	} else {
		const char *str;
		icalcomponent *alarm = {0};
		retval = GN_ERR_NONE;
		calnote->phone_number[0] = 0;

		/* type */
		str = comp_get_category(compresult);
		calnote->type = str2calnote_type(str);
		switch (calnote->type) {
		case GN_CALNOTE_CALL:
			/* description goes into text */
			/* TODO: UTF8 --> ascii */
			snprintf(calnote->phone_number, GN_CALNOTE_NUMBER_MAX_LENGTH-1, "%s", icalcomponent_get_summary(compresult));
			str = icalcomponent_get_description(compresult);
			break;
		default:
			/* summary goes into text */
			/* TODO: UTF8 --> ascii */
			str = icalcomponent_get_summary(compresult);
			break;
		}
		if (str) {
			snprintf(calnote->text, GN_CALNOTE_MAX_LENGTH - 1, "%s", str);
		}

		/* start time */
		dtstart = icalcomponent_get_dtstart(compresult);
		if (!icaltime_is_null_time(dtstart)) {
			calnote->time.year = dtstart.year;
			calnote->time.month = dtstart.month;
			calnote->time.day = dtstart.day;
			if (!icaltime_is_date(dtstart)) {
				calnote->time.hour = dtstart.hour;
				calnote->time.minute = dtstart.minute;
				calnote->time.second = dtstart.second;
				/* TODO: what about time zones? */
			}
		}

		/* end time */
		memset(&calnote->end_time, 0, sizeof(calnote->end_time));
		dtend = icalcomponent_get_dtend(compresult);
		if (!icaltime_is_null_time(dtend)) {
			calnote->end_time.year = dtend.year;
			calnote->end_time.month = dtend.month;
			calnote->end_time.day = dtend.day;
			if (!icaltime_is_date(dtend)) {
				calnote->end_time.hour = dtend.hour;
				calnote->end_time.minute = dtend.minute;
				calnote->end_time.second = dtend.second;
				/* TODO: what about time zones? */
			}
		}

		/* alarm */
		alarm = icalcomponent_get_first_component(comp, ICAL_VALARM_COMPONENT);
		if (alarm) {
			struct icaldurationtype trigger_duration = {0};
			struct icaltriggertype trigger_value = {0};
			icalproperty *trigger = NULL;
			struct icaltimetype alarm_start = {0};

			trigger = icalcomponent_get_first_property(alarm, ICAL_TRIGGER_PROPERTY);
			if (trigger) {
				trigger_value = icalvalue_get_trigger(icalproperty_get_value(trigger));
				if (icaltriggertype_is_null_trigger(trigger_value) ||
						icaltriggertype_is_bad_trigger(trigger_value)) {
					calnote->alarm.enabled = 0;
				} else {
					if (icaltime_is_null_time(trigger_value.time)) {
						memcpy(&alarm_start, &dtstart, sizeof(struct icaltimetype));
					} else {
						memcpy(&alarm_start, &trigger_value.time, sizeof(struct icaltimetype));
					}
					if (!(icaldurationtype_is_bad_duration(trigger_value.duration) ||
							icaldurationtype_is_null_duration(trigger_value.duration))) {
						alarm_start = icaltime_add(alarm_start, trigger_value.duration);
					}
					calnote->alarm.enabled = 1;
					calnote->alarm.timestamp.year = alarm_start.year;
					calnote->alarm.timestamp.month = alarm_start.month;
					calnote->alarm.timestamp.day = alarm_start.day;
					calnote->alarm.timestamp.hour = alarm_start.hour;
					calnote->alarm.timestamp.minute = alarm_start.minute;
					calnote->alarm.timestamp.second = alarm_start.second;
					/* TODO: time zone? */
				}
			}
		}

		str = icalcomponent_get_location(compresult);
		if (!str) str = "";
		snprintf(calnote->mlocation, sizeof(calnote->mlocation), "%s", str);

		fprintf(stderr, _("Component found\n%s\n"), icalcomponent_as_ical_string(compresult));

	}
	if (compresult)
		icalcomponent_free(compresult);
	if (comp)
		icalcomponent_free(comp);
	if (parser)
		icalparser_free(parser);
#else
	retval = GN_ERR_NOTIMPLEMENTED;
#endif /* HAVE_LIBICAL */
	return retval;
}

API int gn_todo2ical(FILE *f, gn_todo *ctodo)
{
#ifdef HAVE_LIBICAL
	icalcomponent *pIcal = NULL;
	char compuid[64];

	sprintf(compuid, "guid.gnokii.org_%d_%d", ctodo->location, rand());

	pIcal = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT,
				    icalproperty_new_version("2.0"),
				    icalproperty_new_prodid(get_prodid()),
				    icalcomponent_vanew(ICAL_VTODO_COMPONENT,
							icalproperty_new_categories("GNOKII"),
							icalproperty_new_priority(ctodo->priority),
							icalproperty_new_summary(ctodo->text),
							0),
				    0);

	if (pIcal) {
		char *icalstrbuf = NULL;
		const char *icalstr = icalcomponent_as_ical_string(pIcal);
		/* conversion to UTF-8 */
		if (string_base64(icalstr)) {
			int icalstrlen = strlen(icalstr);
			icalstrbuf = malloc(icalstrlen * 2 + 1);
			utf8_encode(icalstrbuf, icalstrlen * 2, icalstr, icalstrlen);
			icalstr = icalstrbuf;
		}
		fprintf(f, "%s\n", icalstr);
		dprintf("%s\n", icalstr);
		if (icalstrbuf)
			free(icalstrbuf);

		icalcomponent_free(pIcal);
		pIcal = NULL;
	}
	return GN_ERR_NONE;
#else
	fprintf(f, "BEGIN:VCALENDAR\r\n");
	fprintf(f, "VERSION:1.0\r\n"); 
	fprintf(f, "BEGIN:VTODO\r\n"); 
	fprintf(f, "PRIORITY:%i\r\n", ctodo->priority); 
	fprintf(f, "SUMMARY:%s\r\n", ctodo->text); 
	fprintf(f, "END:VTODO\r\n"); 
	fprintf(f, "END:VCALENDAR\r\n");
	return GN_ERR_NONE;
#endif /* HAVE_LIBICAL */
}

/* read the entry identified by id from the vcal file f and write it to the phone */
API int gn_ical2todo(FILE *f, gn_todo *ctodo, int id)
{
#ifdef HAVE_LIBICAL
	icalparser *parser = NULL;
	icalcomponent *comp = NULL, *compresult = NULL;
	struct icaltimetype dtstart = {0};

	parser = icalparser_new();
	if (!parser) {
		return GN_ERR_FAILED;
	}

	icalparser_set_gen_data(parser, f);

	comp = icalparser_parse(parser, ical_fgets);
	if (!comp) {
		icalparser_free(parser);
		return GN_ERR_FAILED;
	}

	if (id < 1)
		id = 1;

	/* interate through the component */
	iterate_cal(comp, 0, &id, &compresult, ICAL_VTODO_COMPONENT);

	if (!compresult) {
		fprintf(stderr, _("No component found.\n"));
	} else {
		const char *priostr = NULL;
		icalproperty *priority = icalcomponent_get_first_property(compresult, ICAL_PRIORITY_PROPERTY);

		/* summary goes into text */
		/* TODO: UTF8 --> ascii */
		snprintf(ctodo->text, GN_CALNOTE_MAX_LENGTH-1, "%s", icalcomponent_get_summary(compresult));

		/* priority */
		ctodo->priority = icalproperty_get_priority((const icalproperty *)priority);

		fprintf(stderr, _("Component found\n%s\n"), icalcomponent_as_ical_string(compresult));
	}
	icalcomponent_free(compresult);
	icalcomponent_free(comp);
	icalparser_free(parser);

	return GN_ERR_NONE;
#else
	return GN_ERR_NOTIMPLEMENTED;
#endif /* HAVE_LIBICAL */
}

#ifdef HAVE_LIBICAL
/* This function is called recursively, since objects may be embedded in a
 * single VCALENDAR component or each in its own. Returns 0 on an empty set,
 * 1 -- otherwise.
 */
static int iterate_cal(icalcomponent *parent, int depth, int *count, icalcomponent **result, icalcomponent_kind kind)
{
	icalcomponent *comp = NULL;
	icalcompiter compiter;

	if (!parent)
		return 0;

	/* we need to use interators because we're calling iterate_cal recursively */
	compiter = icalcomponent_begin_component(parent, ICAL_ANY_COMPONENT);
	comp = icalcompiter_deref(&compiter);
	for (; comp && *count; comp = icalcompiter_next(&compiter)) {
		if (icalcomponent_isa(comp) == kind) {
			/* FIXME: should we only consider items with category GNOKII? */
			(*count)--;
		}
		if (!*count) {
			*result = comp;
			dprintf("Found component at depth %d\n", depth);
			break;
		}
		iterate_cal(comp, depth+1, count, result, kind);
	}

	return 1;
}

/* Return the first category that is not equal to GNOKII */
static const char *comp_get_category(icalcomponent *comp)
{
	icalproperty *cat = NULL;
	const char *str;

	for (cat = icalcomponent_get_first_property(comp, ICAL_CATEGORIES_PROPERTY);
	     cat;
	     cat = icalcomponent_get_next_property(comp, ICAL_CATEGORIES_PROPERTY)) {
		str = icalproperty_get_categories(cat);
		if (strncasecmp("GNOKII", str, 6) != 0)
			return str;
	}
	return NULL;
}
#endif /* HAVE_LIBICAL */
