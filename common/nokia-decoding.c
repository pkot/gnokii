/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 2002 Markus Plail

  This file provides decoding functions specific to the newer 
  Nokia 7110/6510 series.

*/

#include <time.h>

#include "gsm-common.h"
#include "nokia-decoding.h"

gn_error phonebook_decode(unsigned char *blockstart, int length, gn_data *data,
			  int blocks, int memtype, int speeddial_pos)
{
	int subblock_count = 0, i;
	gn_phonebook_subentry* subentry = NULL;

	if (!data->phonebook_entry) return GN_ERR_INTERNALERROR;

	for (i = 0; i < blocks; i++) {
		subentry = &data->phonebook_entry->subentries[subblock_count];

		dprintf("Blockstart: %i\n", blockstart[0]);  /* FIXME ? */

		switch ((gn_phonebook_entry_type)blockstart[0]) {
		case GN_PHONEBOOK_ENTRY_Pointer:  /* Pointer */
			switch (memtype) {	/* Memory type */
			case GN_NOKIA_MEMORY_SPEEDDIALS:  /* Speed dial numbers */
				if (data && data->speed_dial) {
					unsigned char *str;
					data->speed_dial->location = (((unsigned int)blockstart[6]) << 8) + blockstart[7];
					switch(blockstart[speeddial_pos]) {
					case 0x05:
						data->speed_dial->memory_type = GN_MT_ME;
						str = "phone";
						break;
					case 0x06:
						str = "SIM";
						data->speed_dial->memory_type = GN_MT_SM;
						break;
					default:
						str = "unknown";
						data->speed_dial->memory_type = GN_MT_XX;
						break;
					}
					dprintf("Speed dial pointer: %i in %s\n", data->speed_dial->location, str);
				} else {
					dprintf("NULL entry?\n");
				}
				break;
			default:
				/* FIXME: is it possible? */
				dprintf("Wrong memory type(?)\n");
				break;
			}
			break;
		case GN_PHONEBOOK_ENTRY_Name:	/* Name */
			if (data->bitmap) {
				char_decode_unicode(data->bitmap->text, (blockstart + 6), blockstart[5]);
				dprintf("Bitmap Name: %s\n", data->bitmap->text);
			}
			char_decode_unicode(data->phonebook_entry->name, (blockstart + 6), blockstart[5]);
			data->phonebook_entry->empty = false;
			dprintf("   Name: %s\n", data->phonebook_entry->name);
			break;
		case GN_PHONEBOOK_ENTRY_Email:
		case GN_PHONEBOOK_ENTRY_URL:
		case GN_PHONEBOOK_ENTRY_Postal:
		case GN_PHONEBOOK_ENTRY_Note:
			subentry->entry_type  = blockstart[0];
			subentry->number_type = 0;
			subentry->id          = blockstart[4];
			char_decode_unicode(subentry->data.number, (blockstart + 6), blockstart[5]);
			dprintf("   Type: %d (%02x)\n", subentry->entry_type, subentry->entry_type);
			dprintf("   Text: %s\n", subentry->data.number);
			subblock_count++;
			data->phonebook_entry->subentries_count++;
			break;
		case GN_PHONEBOOK_ENTRY_Number:
			subentry->entry_type  = blockstart[0];
			subentry->number_type = blockstart[5];
			subentry->id          = blockstart[4];
			char_decode_unicode(subentry->data.number, (blockstart + 10), blockstart[9]);
			if (!subblock_count) strcpy(data->phonebook_entry->number, subentry->data.number);
			dprintf("   Type: %d (%02x)\n", subentry->number_type, subentry->number_type);
			dprintf("   Number: %s\n", subentry->data.number);
			subblock_count++;
			data->phonebook_entry->subentries_count++;
			break;
		case GN_PHONEBOOK_ENTRY_Ringtone:  /* Ringtone */
			if (data->bitmap) {
				data->bitmap->ringtone = blockstart[5];
				dprintf("Ringtone no. %d\n", data->bitmap->ringtone);
			}
			break;
		case GN_PHONEBOOK_ENTRY_Date:
			subentry->entry_type       = blockstart[0];
			subentry->number_type      = blockstart[5];
			subentry->id		   = blockstart[4];
			subentry->data.date.year   = (blockstart[6] << 8) + blockstart[7];
			subentry->data.date.month  = blockstart[8];
			subentry->data.date.day    = blockstart[9];
			subentry->data.date.hour   = blockstart[10];
			subentry->data.date.minute = blockstart[11];
			subentry->data.date.second = blockstart[12];
			dprintf("   Date: %04u.%02u.%04u\n", subentry->data.date.year,
				subentry->data.date.month, subentry->data.date.day);
			dprintf("   Time: %02u:%02u:%02u\n", subentry->data.date.hour,
				subentry->data.date.minute, subentry->data.date.second);
			subblock_count++;
			break;
		case GN_PHONEBOOK_ENTRY_Logo:   /* Caller group logo */
			if (data->bitmap) {
				dprintf("Caller logo received (h: %i, w: %i)!\n", blockstart[5], blockstart[6]);
				data->bitmap->width = blockstart[5];
				data->bitmap->height = blockstart[6];
				data->bitmap->size = (data->bitmap->width * data->bitmap->height) / 8;
				memcpy(data->bitmap->bitmap, blockstart + 10, data->bitmap->size);
				dprintf("Bitmap: width: %i, height: %i\n", blockstart[5], blockstart[6]);
			}
			break;
		case GN_PHONEBOOK_ENTRY_LogoSwitch:   /* Logo on/off */
			break;
		case GN_PHONEBOOK_ENTRY_Group:   /* Caller group number */
			if (data->phonebook_entry) {
				data->phonebook_entry->caller_group = blockstart[5] - 1;
				dprintf("   Group: %d\n", data->phonebook_entry->caller_group);
			}
			break;
		default:
			dprintf("Unknown phonebook block %02x\n", blockstart[0]);
			break;
		}
		blockstart += blockstart[3];
	}
	return GN_ERR_NONE;
}

static gn_error calnote_get_alarm(int alarmdiff, gn_timestamp *time, gn_timestamp *alarm)
{
	time_t t_alarm;
	struct tm tm_time, *tm_alarm;

	if (!time || !alarm) return GN_ERR_INTERNALERROR;

	memset(&tm_time, 0, sizeof(tm_time));
	tm_time.tm_year = time->year - 1900;
	tm_time.tm_mon = time->month - 1;
	tm_time.tm_mday = time->day;
	tm_time.tm_hour = time->hour;
	tm_time.tm_min = time->minute;

	tzset();
	t_alarm = mktime(&tm_time);
	t_alarm -= alarmdiff;
	t_alarm += timezone;

	tm_alarm = localtime(&t_alarm);

	alarm->year = tm_alarm->tm_year + 1900;
	alarm->month = tm_alarm->tm_mon + 1;
	alarm->day = tm_alarm->tm_mday;
	alarm->hour = tm_alarm->tm_hour;
	alarm->minute = tm_alarm->tm_min;
	alarm->second = tm_alarm->tm_sec;

	return GN_ERR_NONE;
}

static gn_error calnote_get_times(unsigned char *block, gn_calnote *c)
{
	time_t alarmdiff;
	gn_error e = GN_ERR_NONE;

	if (!c) return GN_ERR_INTERNALERROR;

	c->time.hour = block[0];
	c->time.minute = block[1];
	c->recurrence = ((((unsigned int)block[4]) << 8) + block[5]) * 60;
	alarmdiff = (((unsigned int)block[2]) << 8) + block[3];

	if (alarmdiff != 0xffff) {
		e = calnote_get_alarm(alarmdiff * 60, &(c->time), &(c->alarm.timestamp));
		c->alarm.enabled = true;
	} else {
		c->alarm.enabled = false;
	}

	return e;
}

gn_error calnote_decode(unsigned char *message, int length, gn_data *data)
{
	unsigned char *block;
	int alarm;
	gn_error e = GN_ERR_NONE;

	if (!data->calnote) return GN_ERR_INTERNALERROR;

	block = message + 12;

	data->calnote->location = (((unsigned int)message[4]) << 8) + message[5];
	data->calnote->time.year = (((unsigned int)message[8]) << 8) + message[9];
	data->calnote->time.month = message[10];
	data->calnote->time.day = message[11];
	data->calnote->time.second = 0;

	dprintf("Year: %i\n", data->calnote->time.year);

	switch (data->calnote->type = message[6]) {
	case GN_CALNOTE_MEETING:
		e = calnote_get_times(block, data->calnote);
		if (e != GN_ERR_NONE) return e;
		char_decode_unicode(data->calnote->text, (block + 8), block[6] << 1);
		break;
	case GN_CALNOTE_CALL:
		e = calnote_get_times(block, data->calnote);
		if (e != GN_ERR_NONE) return e;
		char_decode_unicode(data->calnote->text, (block + 8), block[6] << 1);
		char_decode_unicode(data->calnote->phone_number, (block + 8 + block[6] * 2), block[7] << 1);
		break;
	case GN_CALNOTE_REMINDER:
		data->calnote->recurrence = ((((unsigned int)block[0]) << 8) + block[1]) * 60;
		char_decode_unicode(data->calnote->text, (block + 4), block[2] << 1);
		break;
	case GN_CALNOTE_BIRTHDAY:
		data->calnote->time.hour = 23;
		data->calnote->time.minute = 59;
		data->calnote->time.second = 58;

		alarm = ((unsigned int)block[2]) << 24;
		alarm += ((unsigned int)block[3]) << 16;
		alarm += ((unsigned int)block[4]) << 8;
		alarm += block[5];

		dprintf("alarm: %i\n", alarm);

		if (alarm == 0xffff) {
			data->calnote->alarm.enabled = false;
		} else {
			data->calnote->alarm.enabled = true;
		}

		e = calnote_get_alarm(alarm, &(data->calnote->time), &(data->calnote->alarm.timestamp));
		if (e != GN_ERR_NONE) return e;

		data->calnote->time.hour = 0;
		data->calnote->time.minute = 0;
		data->calnote->time.second = 0;
		data->calnote->time.year = (((unsigned int)block[6]) << 8) + block[7];

		char_decode_unicode(data->calnote->text, (block + 10), block[9] << 1);
		break;
	default:
		return GN_ERR_UNKNOWN;
	}
	return GN_ERR_NONE;
}
