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
/* needs gnokii-internal.h for string_base64() and utf8_encode() */
#  include "gnokii-internal.h"
#  include <libical/ical.h>
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
 * to use a static string instead of a pre-processor macro.
 */
static char vcal_prodid[64] = {0};
static inline const char *get_prodid()
{
	if (!*vcal_prodid) {
		snprintf(vcal_prodid, sizeof(vcal_prodid), "//Gnokii.org//NONSGML v%s", gnokii_version);
	}
	return vcal_prodid;
}

typedef int (*print_func) (void *data, const char *fmt, ...);

typedef struct {
	char *str;
	char *end;
	unsigned int len;
} ical_string;

static void ical_append_printf(ical_string *str, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	int len;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (str->str == NULL) {
		str->str = strdup (buf);
		str->len = strlen (buf) + 1;
		return;
	}

	len = strlen (buf);
	str->str = realloc(str->str, len + str->len);

	memcpy (str->str + str->len - 1, buf, len);
	str->len = str->len + len;
	str->end = str->str + str->len;

	/* NULL-terminate the string now */
	*(str->end - 1) = '\0';
}

/*
	ICALENDAR Reading functions
 */
GNOKII_API char *gn_calnote2icalstr(gn_calnote *calnote)
{
	ical_string str;

#ifdef HAVE_LIBICAL
	icalcomponent *pIcal = NULL, *vevent;
	struct icaltimetype stime = {0}, etime = {0};
	char compuid[64];

	memset(&str, 0, sizeof (str));

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

	snprintf(compuid, sizeof(compuid), "guid.gnokii.org_%d_%d", calnote->location, rand());

	vevent = icalcomponent_vanew(ICAL_VEVENT_COMPONENT,
				     icalproperty_new_dtstart(stime),
				     icalproperty_new_uid(compuid),
				     icalproperty_new_categories("GNOKII"),
				     0);
	if (!vevent) {
		dprintf("ERROR in icalcomponent_vanew() creating VEVENT\n");
		return NULL;
	}

	if (calnote->end_time.year) {
		etime.year = (calnote->end_time.year == 0xffff ? 1800 : calnote->end_time.year);
		etime.month = calnote->end_time.month;
		etime.day = calnote->end_time.day;
		etime.hour = calnote->end_time.hour;
		etime.minute = calnote->end_time.minute;
		etime.second = calnote->end_time.second;
		etime.is_daylight = 1;

		icalcomponent_add_property(vevent, icalproperty_new_dtend(etime));
	}

	/* TODO: should the strings be configurable? */
	switch(calnote->type) {
	case GN_CALNOTE_MEMO:
		icalcomponent_add_property(vevent, icalproperty_new_categories("MISCELLANEOUS"));
		icalcomponent_add_property(vevent, icalproperty_new_summary(calnote->text));
		break;
	case GN_CALNOTE_REMINDER:
		icalcomponent_add_property(vevent, icalproperty_new_categories("REMINDER"));
		icalcomponent_add_property(vevent, icalproperty_new_summary(calnote->text));
		break;
	case GN_CALNOTE_CALL:
		icalcomponent_add_property(vevent, icalproperty_new_categories("PHONE CALL"));
		icalcomponent_add_property(vevent, icalproperty_new_summary(calnote->phone_number));
		icalcomponent_add_property(vevent, icalproperty_new_description(calnote->text));
		break;
	case GN_CALNOTE_MEETING:
		icalcomponent_add_property(vevent, icalproperty_new_categories("MEETING"));
		icalcomponent_add_property(vevent, icalproperty_new_summary(calnote->text));
		if (calnote->mlocation[0])
			icalcomponent_add_property(vevent, icalproperty_new_location(calnote->mlocation));
		break;
	case GN_CALNOTE_BIRTHDAY:
		icalcomponent_add_property(vevent, icalproperty_new_categories("ANNIVERSARY"));
		icalcomponent_add_property(vevent, icalproperty_new_summary(calnote->text));
		stime.is_date = 1;
		calnote->recurrence = GN_CALNOTE_YEARLY;
		break;
	default:
		icalcomponent_add_property(vevent, icalproperty_new_categories("UNKNOWN"));
		icalcomponent_add_property(vevent, icalproperty_new_summary(calnote->text));
		break;
	}

	if (calnote->recurrence) {
		char rrule[64];
		char *freq;
		int interval = 1;

		switch (calnote->recurrence) {
		case GN_CALNOTE_NEVER:
			goto norecurrence;
		case GN_CALNOTE_DAILY:
			freq = "DAILY";
			break;
		case GN_CALNOTE_WEEKLY:
			freq = "WEEKLY";
			break;
		case GN_CALNOTE_2WEEKLY:
			freq = "WEEKLY";
			interval = 2;
			break;
		case GN_CALNOTE_MONTHLY:
			freq = "MONTHLY";
			break;
		case GN_CALNOTE_YEARLY:
			freq = "YEARLY";
			break;
		default:
			freq = "HOURLY";
			interval = calnote->recurrence;
			break;
		}
		if (calnote->type == GN_CALNOTE_BIRTHDAY)
			snprintf(rrule, sizeof(rrule), "FREQ=YEARLY;INTERVAL=1;BYMONTH=%d", stime.month);
		else if (calnote->occurrences == 0)
			snprintf(rrule, sizeof(rrule), "FREQ=%s;INTERVAL=%d", freq, interval);
		else
			snprintf(rrule, sizeof(rrule), "FREQ=%s;COUNT=%d;INTERVAL=%d", freq, calnote->occurrences, interval);
		icalcomponent_add_property(vevent, icalproperty_new_rrule(icalrecurrencetype_from_string(rrule)));
	}
norecurrence:

	if (calnote->alarm.enabled) {
		icalcomponent *valarm;
		struct icaltriggertype trig;

		trig.time.year = (calnote->alarm.timestamp.year == 0xffff ? 1800 : calnote->alarm.timestamp.year);
		trig.time.month = calnote->alarm.timestamp.month;
		trig.time.day = calnote->alarm.timestamp.day;
		trig.time.hour = calnote->alarm.timestamp.hour;
		trig.time.minute = calnote->alarm.timestamp.minute;
		trig.time.second = calnote->alarm.timestamp.second;
		trig.time.is_daylight = 1;

		valarm = icalcomponent_new_valarm();

		icalcomponent_add_property(valarm, icalproperty_new_trigger(trig));
		/* FIXME: with ICAL_ACTION_DISPLAY a DESCRIPTION property is mandatory */
		icalcomponent_add_property(valarm, icalproperty_new_action(calnote->alarm.tone ? ICAL_ACTION_AUDIO : ICAL_ACTION_DISPLAY));

		icalcomponent_add_component(vevent, valarm);
	}

	pIcal = icalcomponent_vanew(ICAL_VCALENDAR_COMPONENT,
				    icalproperty_new_version("2.0"),
				    icalproperty_new_prodid(get_prodid()),
				    vevent,
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
		ical_append_printf(&str, "%s\n", icalstr);
		dprintf("%s\n", icalstr);
		if (icalstrbuf)
			free(icalstrbuf);

		icalcomponent_free(pIcal);
		pIcal = NULL;
	} else {
		dprintf("ERROR in icalcomponent_vanew() creating VCALENDAR\n");
	}
	return str.str;
#else
	memset(&str, 0, sizeof (str));

	ical_append_printf(&str, "BEGIN:VCALENDAR\r\n");
	ical_append_printf(&str, "VERSION:1.0\r\n");
	ical_append_printf(&str, "BEGIN:VEVENT\r\n");
	ical_append_printf(&str, "CATEGORIES:");
	switch (calnote->type) {
	case GN_CALNOTE_MEMO:
		ical_append_printf(&str, "MISCELLANEOUS\r\n");
		break;
	case GN_CALNOTE_REMINDER:
		ical_append_printf(&str, "REMINDER\r\n");
		break;
	case GN_CALNOTE_CALL: 
		ical_append_printf(&str, "PHONE CALL\r\n");
		ical_append_printf(&str, "SUMMARY:%s\r\n", calnote->phone_number);
		ical_append_printf(&str, "DESCRIPTION:%s\r\n", calnote->text);
		break; 
	case GN_CALNOTE_MEETING: 
		ical_append_printf(&str, "MEETING\r\n"); 
		if (calnote->mlocation[0])
			ical_append_printf(&str, "LOCATION:%s\r\n", calnote->mlocation);
		break; 
	case GN_CALNOTE_BIRTHDAY: 
		ical_append_printf(&str, "SPECIAL OCCASION\r\n"); 
		break; 
	default: 
		ical_append_printf(&str, "UNKNOWN\r\n"); 
		break; 
	} 
	if (calnote->type != GN_CALNOTE_CALL)
		ical_append_printf(&str, "SUMMARY:%s\r\n", calnote->text);
	ical_append_printf(&str, "DTSTART:%04d%02d%02dT%02d%02d%02d\r\n", calnote->time.year,
		calnote->time.month, calnote->time.day, calnote->time.hour, 
		calnote->time.minute, calnote->time.second); 
	if (calnote->end_time.year) {
		ical_append_printf(&str, "DTEND:%04d%02d%02dT%02d%02d%02d\r\n", calnote->end_time.year, 
			calnote->end_time.month, calnote->end_time.day, calnote->end_time.hour, 
			calnote->end_time.minute, calnote->end_time.second); 
	}
	if (calnote->alarm.enabled) {
		ical_append_printf(&str, "%sALARM:%04d%02d%02dT%02d%02d%02d\r\n",
		(calnote->alarm.tone ? "A" : "D"),
		calnote->alarm.timestamp.year, 
		calnote->alarm.timestamp.month, calnote->alarm.timestamp.day, calnote->alarm.timestamp.hour, 
		calnote->alarm.timestamp.minute, calnote->alarm.timestamp.second); 
	} 
	switch (calnote->recurrence) { 
	case GN_CALNOTE_NEVER: 
		break; 
	case GN_CALNOTE_DAILY: 
		calnote->occurrences ?
			ical_append_printf(&str, "RRULE:FREQ=DAILY\r\n") :
			ical_append_printf(&str, "RRULE:FREQ=DAILY;COUNT=%d\r\n", calnote->occurrences);
		break; 
	case GN_CALNOTE_WEEKLY: 
		calnote->occurrences ?
			ical_append_printf(&str, "RRULE:FREQ=WEEKLY\r\n") :
			ical_append_printf(&str, "RRULE:FREQ=WEEKLY;COUNT=%d\r\n", calnote->occurrences);
		break; 
	case GN_CALNOTE_2WEEKLY: 
		calnote->occurrences ?
			ical_append_printf(&str, "RRULE:FREQ=WEEKLY;INTERVAL=2\r\n") :
			ical_append_printf(&str, "RRULE:FREQ=WEEKLY;INTERVAL=2;COUNT=%d\r\n", calnote->occurrences);
		break; 
	case GN_CALNOTE_MONTHLY: 
		calnote->occurrences ?
			ical_append_printf(&str, "RRULE:FREQ=MONTHLY\r\n") :
			ical_append_printf(&str, "RRULE:FREQ=MONTHLY;COUNT=%d\r\n", calnote->occurrences);
		break; 
	case GN_CALNOTE_YEARLY: 
		calnote->occurrences ?
			ical_append_printf(&str, "RRULE:FREQ=YEARLY\r\n") :
			ical_append_printf(&str, "RRULE:FREQ=YEARLY;COUNT=%d\r\n", calnote->occurrences);
		break; 
	default: 
		calnote->occurrences ?
			ical_append_printf(&str, "RRULE:FREQ=HOURLY;INTERVAL=%d\r\n", calnote->recurrence) :
			ical_append_printf(&str, "RRULE:FREQ=HOURLY;INTERVAL=%d;COUNT=%d\r\n", calnote->recurrence, calnote->occurrences);
		break; 
	} 
	ical_append_printf(&str, "END:VEVENT\r\n"); 
	ical_append_printf(&str, "END:VCALENDAR\r\n");
	return str.str;
#endif /* HAVE_LIBICAL */
}

GNOKII_API int gn_calnote2ical(FILE *f, gn_calnote *calnote)
{
	char *vcal;
	int retval;

	vcal = gn_calnote2icalstr(calnote);
	if (vcal == NULL)
		return -1;
	retval = fputs(vcal, f);
	free (vcal);
	return retval;
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
	} else if (!strncasecmp("MISCELLANEOUS", str, 13)) {
		return GN_CALNOTE_MEMO;
	}
	return GN_CALNOTE_REMINDER;
}

#ifdef HAVE_LIBICAL
static int gn_ical2calnote_real(icalcomponent *comp, gn_calnote *calnote, int id)
{
	int retval = GN_ERR_FAILED;
	struct icaltimetype dtstart = {0}, dtend = {0};
	icalcomponent *compresult = NULL;

	if (id < 1)
		id = 1;

	/* iterate through the component */
	iterate_cal(comp, 0, &id, &compresult, ICAL_VEVENT_COMPONENT);

	if (!compresult) {
		dprintf("No component found.\n");
		retval = GN_ERR_EMPTYLOCATION;
	} else {
		const char *str;
		icalcomponent *alarm = {0};
		icalproperty *rrule;
		struct icalrecurrencetype recur;
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
		alarm = icalcomponent_get_first_component(compresult, ICAL_VALARM_COMPONENT);
		if (alarm) {
			icalproperty *trigger = NULL;

			trigger = icalcomponent_get_first_property(alarm, ICAL_TRIGGER_PROPERTY);
			if (trigger) {
				struct icaltriggertype trigger_value;
				struct icaltimetype alarm_start;
				icalproperty *property;

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

					property = icalcomponent_get_first_property(alarm, ICAL_ACTION_PROPERTY);
					calnote->alarm.tone = icalproperty_get_action(property) == ICAL_ACTION_AUDIO ? 1 : 0;
				}
			}
		}

		/* recurrence */
		rrule = icalcomponent_get_first_property(compresult, ICAL_RRULE_PROPERTY);
		if (rrule) {
			recur = icalproperty_get_rrule(rrule);
			calnote->occurrences = recur.count;
			switch (recur.freq) {
			case ICAL_SECONDLY_RECURRENCE:
				dprintf("Not supported recurrence type. Approximating recurrence\n");
				calnote->recurrence = recur.interval / 3600;
				if (!calnote->recurrence)
					calnote->recurrence = 1;
				break;
			case ICAL_MINUTELY_RECURRENCE:
				dprintf("Not supported recurrence type. Approximating recurrence\n");
				calnote->recurrence = recur.interval / 60;
				if (!calnote->recurrence)
					calnote->recurrence = 1;
				break;
			case ICAL_HOURLY_RECURRENCE:
				calnote->recurrence = recur.interval;
				break;
			case ICAL_DAILY_RECURRENCE:
				calnote->recurrence = recur.interval * 24;
				break;
			case ICAL_WEEKLY_RECURRENCE:
				calnote->recurrence = recur.interval * 24 * 7;
				break;
			case ICAL_MONTHLY_RECURRENCE:
				calnote->recurrence = GN_CALNOTE_MONTHLY;
				break;
			case ICAL_YEARLY_RECURRENCE:
				calnote->recurrence = GN_CALNOTE_YEARLY;
				break;
			case ICAL_NO_RECURRENCE:
				calnote->recurrence = GN_CALNOTE_NEVER;
				break;
			default:
				dprintf("Not supported recurrence type. Assuming never\n");
				calnote->recurrence = GN_CALNOTE_NEVER;
				break;
			}
		}

		str = icalcomponent_get_location(compresult);
		if (!str)
			str = "";
		snprintf(calnote->mlocation, sizeof(calnote->mlocation), "%s", str);
	}
	if (compresult) {
#ifdef DEBUG
		char *temp;

		temp = icalcomponent_as_ical_string(compresult);
		dprintf("Component found\n%s\n", temp);
		free(temp);
#endif
		icalcomponent_free(compresult);
	}

	return retval;
}
#endif /* HAVE_LIBICAL */

/* read a vcalendar event given by id from file f and store the data in calnote */
GNOKII_API gn_error gn_ical2calnote(FILE *f, gn_calnote *calnote, int id)
{
#ifdef HAVE_LIBICAL
	icalparser *parser = NULL;
	icalcomponent *comp = NULL;
	gn_error retval;

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

	retval = gn_ical2calnote_real(comp, calnote, id);

	icalcomponent_free(comp);
	icalparser_free(parser);

	return retval;
#else
	return GN_ERR_NOTIMPLEMENTED;
#endif /* HAVE_LIBICAL */
}

/* read a vcalendar event given by id from string ical and store the data in calnote */
GNOKII_API gn_error gn_icalstr2calnote(const char * ical, gn_calnote *calnote, int id)
{
#ifdef HAVE_LIBICAL
	icalparser *parser = NULL;
	icalcomponent *comp = NULL;
	gn_error retval;

	parser = icalparser_new();
	if (!parser) {
		return GN_ERR_FAILED;
	}

	comp = icalparser_parse_string (ical);
	if (!comp) {
		icalparser_free(parser);
		return GN_ERR_FAILED;
	}

	retval = gn_ical2calnote_real(comp, calnote, id);

	icalcomponent_free(comp);
	icalparser_free(parser);

	return retval;
#else
	return GN_ERR_NOTIMPLEMENTED;
#endif /* HAVE_LIBICAL */
}

GNOKII_API int gn_todo2ical(FILE *f, gn_todo *ctodo)
{
	char *icalstr;

	icalstr = gn_todo2icalstr(ctodo);
	if (icalstr) {
		fprintf(f, "%s\n", icalstr);
		dprintf("%s\n", icalstr);
		free(icalstr);
	}

	return GN_ERR_NONE;
}

GNOKII_API char * gn_todo2icalstr(gn_todo *ctodo)
{
#ifdef HAVE_LIBICAL
	icalcomponent *pIcal = NULL;
	char compuid[64];

	snprintf(compuid, sizeof(compuid), "guid.gnokii.org_%d_%d", ctodo->location, rand());

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
		} else {
			icalstrbuf = strdup (icalstr);
		}
		icalcomponent_free(pIcal);
		return icalstrbuf;
	}
	return NULL;
#else
	ical_string str;

	memset(&str, 0, sizeof(str));
	ical_append_printf(&str, "BEGIN:VCALENDAR\r\n");
	ical_append_printf(&str, "VERSION:1.0\r\n"); 
	ical_append_printf(&str, "BEGIN:VTODO\r\n"); 
	ical_append_printf(&str, "PRIORITY:%i\r\n", ctodo->priority); 
	ical_append_printf(&str, "SUMMARY:%s\r\n", ctodo->text); 
	ical_append_printf(&str, "END:VTODO\r\n"); 
	ical_append_printf(&str, "END:VCALENDAR\r\n");

	return str.str;
#endif /* HAVE_LIBICAL */
}

#ifdef HAVE_LIBICAL
/* read the entry identified by id from the vcal file f and write it to the phone */
static int gn_ical2todo_real(icalcomponent *comp, gn_todo *ctodo, int id)
{
	icalcomponent *compresult = NULL;

	if (id < 1)
		id = 1;

	/* iterate through the component */
	iterate_cal(comp, 0, &id, &compresult, ICAL_VTODO_COMPONENT);

	if (!compresult) {
		dprintf("No component found.\n");
		return GN_ERR_EMPTYLOCATION;
	} else {
		icalproperty *priority = icalcomponent_get_first_property(compresult, ICAL_PRIORITY_PROPERTY);

		/* summary goes into text */
		/* TODO: UTF8 --> ascii */
		snprintf(ctodo->text, GN_TODO_MAX_LENGTH-1, "%s", icalcomponent_get_summary(compresult));

		/* priority */
		ctodo->priority = icalproperty_get_priority((const icalproperty *)priority);

		dprintf("Component found\n%s\n", icalcomponent_as_ical_string(compresult));
	}
	icalcomponent_free(compresult);

	return GN_ERR_NONE;
}
#endif /* HAVE_LIBICAL */

GNOKII_API int gn_ical2todo(FILE *f, gn_todo *ctodo, int id)
{
#ifdef HAVE_LIBICAL
	icalparser *parser = NULL;
	icalcomponent *comp = NULL;
	gn_error error;

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

	error = gn_ical2todo_real(comp, ctodo, id);

	icalcomponent_free(comp);
	icalparser_free(parser);

	return error;
#else
	return GN_ERR_NOTIMPLEMENTED;
#endif /* HAVE_LIBICAL */
}

GNOKII_API int gn_icalstr2todo(const char * ical, gn_todo *ctodo, int id)
{
#ifdef HAVE_LIBICAL
	icalparser *parser = NULL;
	icalcomponent *comp = NULL;
	gn_error error;

	parser = icalparser_new();
	if (!parser) {
		return GN_ERR_FAILED;
	}

	comp = icalparser_parse_string (ical);
	if (!comp) {
		icalparser_free(parser);
		return GN_ERR_FAILED;
	}

	error = gn_ical2todo_real(comp, ctodo, id);

	icalcomponent_free(comp);
	icalparser_free(parser);

	return error;
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

	/* we need to use iterators because we're calling iterate_cal recursively */
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
