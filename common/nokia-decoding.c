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

gn_error DecodePhonebook(unsigned char *blockstart, int length, GSM_Data *data, int blocks, int memtype, int speeddialpos)
{
	int subblockcount = 0, i;
	char *str;
	GSM_SubPhonebookEntry* subEntry = NULL;

	for (i = 0; i < blocks; i++) {
		if (data->PhonebookEntry)
			subEntry = &data->PhonebookEntry->SubEntries[subblockcount];
		dprintf("Blockstart: %i\n", blockstart[0]);
		switch(blockstart[0]) {
		case PNOKIA_ENTRYTYPE_POINTER:	/* Pointer */
			switch (memtype) {	/* Memory type */
			case PNOKIA_MEMORY_SPEEDDIALS:	/* Speed dial numbers */
				if ((data != NULL) && (data->SpeedDial != NULL)) {
					data->SpeedDial->Location = (((unsigned int)blockstart[6]) << 8) + blockstart[7];
					switch(blockstart[speeddialpos]) {
					case 0x05:
						data->SpeedDial->MemoryType = GMT_ME;
						str = "phone";
						break;
					case 0x06:
						str = "SIM";
						data->SpeedDial->MemoryType = GMT_SM;
						break;
					default:
						str = "unknown";
						data->SpeedDial->MemoryType = GMT_XX;
						break;
					}
					dprintf("Speed dial pointer: %i in %s\n", data->SpeedDial->Location, str);
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
		case PNOKIA_ENTRYTYPE_NAME:	/* Name */
			if (data->Bitmap) {
				char_decode_unicode(data->Bitmap->text, (blockstart + 6), blockstart[5]);
				dprintf("Bitmap Name: %s\n", data->Bitmap->text);
			}
			if (data->PhonebookEntry) {
				char_decode_unicode(data->PhonebookEntry->Name, (blockstart + 6), blockstart[5]);
				data->PhonebookEntry->Empty = false;
				dprintf("   Name: %s\n", data->PhonebookEntry->Name);
			}
			break;
		case PNOKIA_ENTRYTYPE_EMAIL:
		case PNOKIA_ENTRYTYPE_URL:
		case PNOKIA_ENTRYTYPE_POSTAL:
		case PNOKIA_ENTRYTYPE_NOTE:
			if (data->PhonebookEntry) {
				subEntry->EntryType   = blockstart[0];
				subEntry->NumberType  = 0;
				subEntry->BlockNumber = blockstart[4];
				char_decode_unicode(subEntry->data.Number, (blockstart + 6), blockstart[5]);
				dprintf("   Type: %d (%02x)\n", subEntry->EntryType, subEntry->EntryType);
				dprintf("   Text: %s\n", subEntry->data.Number);
				subblockcount++;
				data->PhonebookEntry->SubEntriesCount++;
			}
			break;
		case PNOKIA_ENTRYTYPE_NUMBER:
			if (data->PhonebookEntry) {
				subEntry->EntryType   = blockstart[0];
				subEntry->NumberType  = blockstart[5];
				subEntry->BlockNumber = blockstart[4];
				char_decode_unicode(subEntry->data.Number, (blockstart + 10), blockstart[9]);
				if (!subblockcount) strcpy(data->PhonebookEntry->Number, subEntry->data.Number);
				dprintf("   Type: %d (%02x)\n", subEntry->NumberType, subEntry->NumberType);
				dprintf("   Number: %s\n", subEntry->data.Number);
				subblockcount++;
				data->PhonebookEntry->SubEntriesCount++;
			}
			break;
		case PNOKIA_ENTRYTYPE_RINGTONE:	/* Ringtone */
			if (data->Bitmap) {
				data->Bitmap->ringtone = blockstart[5];
				dprintf("Ringtone no. %d\n", data->Bitmap->ringtone);
			}
			break;
		case PNOKIA_ENTRYTYPE_DATE:
			if (data->PhonebookEntry) {
				subEntry->EntryType=blockstart[0];
				subEntry->NumberType=blockstart[5];
				subEntry->BlockNumber=blockstart[4];
				subEntry->data.Date.Year=(blockstart[6] << 8) + blockstart[7];
				subEntry->data.Date.Month  = blockstart[8];
				subEntry->data.Date.Day    = blockstart[9];
				subEntry->data.Date.Hour   = blockstart[10];
				subEntry->data.Date.Minute = blockstart[11];
				subEntry->data.Date.Second = blockstart[12];
				dprintf("   Date: %02u.%02u.%04u\n", subEntry->data.Date.Day,
					subEntry->data.Date.Month, subEntry->data.Date.Year);
				dprintf("   Time: %02u:%02u:%02u\n", subEntry->data.Date.Hour,
					subEntry->data.Date.Minute, subEntry->data.Date.Second);
				subblockcount++;
			}
			break;
		case PNOKIA_ENTRYTYPE_LOGO:	 /* Caller group logo */
			if (data->Bitmap) {
				dprintf("Caller logo received (h: %i, w: %i)!\n", blockstart[5], blockstart[6]);
				data->Bitmap->width = blockstart[5];
				data->Bitmap->height = blockstart[6];
				data->Bitmap->size = (data->Bitmap->width * data->Bitmap->height) / 8;
				memcpy(data->Bitmap->bitmap, blockstart + 10, data->Bitmap->size);
				dprintf("Bitmap: width: %i, height: %i\n", blockstart[5], blockstart[6]);
			}
			break;
		case PNOKIA_ENTRYTYPE_LOGOSWITCH:/* Logo on/off */
			break;
		case PNOKIA_ENTRYTYPE_GROUP:	/* Caller group number */
			if (data->PhonebookEntry) {
				data->PhonebookEntry->Group = blockstart[5] - 1;
				dprintf("   Group: %d\n", data->PhonebookEntry->Group);
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

static gn_error GetNoteAlarm(int alarmdiff, GSM_DateTime *time, GSM_DateTime *alarm)
{
	time_t				t_alarm;
	struct tm			tm_time;
	struct tm			*tm_alarm;
	gn_error			e = GN_ERR_NONE;

	if (!time || !alarm) return GN_ERR_INTERNALERROR;

	memset(&tm_time, 0, sizeof(tm_time));
	tm_time.tm_year = time->Year - 1900;
	tm_time.tm_mon = time->Month - 1;
	tm_time.tm_mday = time->Day;
	tm_time.tm_hour = time->Hour;
	tm_time.tm_min = time->Minute;

	tzset();
	t_alarm = mktime(&tm_time);
	t_alarm -= alarmdiff;
	t_alarm += timezone;

	tm_alarm = localtime(&t_alarm);

	alarm->Year = tm_alarm->tm_year + 1900;
	alarm->Month = tm_alarm->tm_mon + 1;
	alarm->Day = tm_alarm->tm_mday;
	alarm->Hour = tm_alarm->tm_hour;
	alarm->Minute = tm_alarm->tm_min;
	alarm->Second = tm_alarm->tm_sec;

	return e;
}


static gn_error GetNoteTimes(unsigned char *block, GSM_CalendarNote *c)
{
	time_t		alarmdiff;
	gn_error	e = GN_ERR_NONE;

	if (!c) return GN_ERR_INTERNALERROR;

	c->Time.Hour = block[0];
	c->Time.Minute = block[1];
	c->Recurrence = ((((unsigned int)block[4]) << 8) + block[5]) * 60;
	alarmdiff = (((unsigned int)block[2]) << 8) + block[3];

	if (alarmdiff != 0xffff) {
		e = GetNoteAlarm(alarmdiff * 60, &(c->Time), &(c->Alarm));
		c->Alarm.AlarmEnabled = 1;
	} else {
		c->Alarm.AlarmEnabled = 0;
	}

	return e;
}

gn_error DecodeCalendar(unsigned char *message, int length, GSM_Data *data)
{
	unsigned char *block;
	int alarm;

	block = message + 12;

	data->CalendarNote->Location = (((unsigned int)message[4]) << 8) + message[5];
	data->CalendarNote->Time.Year = (((unsigned int)message[8]) << 8) + message[9];
	data->CalendarNote->Time.Month = message[10];
	data->CalendarNote->Time.Day = message[11];
	data->CalendarNote->Time.Second = 0;

	dprintf("Year: %i\n", data->CalendarNote->Time.Year);

	switch (message[6]) {
	case PNOKIA_NOTE_MEETING:
		data->CalendarNote->Type = GCN_MEETING;
		GetNoteTimes(block, data->CalendarNote);
		char_decode_unicode(data->CalendarNote->Text, (block + 8), block[6] << 1);
		break;
	case PNOKIA_NOTE_CALL:
		data->CalendarNote->Type = GCN_CALL;
		GetNoteTimes(block, data->CalendarNote);
		char_decode_unicode(data->CalendarNote->Text, (block + 8), block[6] << 1);
		char_decode_unicode(data->CalendarNote->Phone, (block + 8 + block[6] * 2), block[7] << 1);
		break;
	case PNOKIA_NOTE_REMINDER:
		data->CalendarNote->Type = GCN_REMINDER;
		data->CalendarNote->Recurrence = ((((unsigned int)block[0]) << 8) + block[1]) * 60;
		char_decode_unicode(data->CalendarNote->Text, (block + 4), block[2] << 1);
		break;
	case PNOKIA_NOTE_BIRTHDAY:
		data->CalendarNote->Type = GCN_BIRTHDAY;
		data->CalendarNote->Time.Hour = 23;
		data->CalendarNote->Time.Minute = 59;
		data->CalendarNote->Time.Second = 58;

		alarm = ((unsigned int)block[2]) << 24;
		alarm += ((unsigned int)block[3]) << 16;
		alarm += ((unsigned int)block[4]) << 8;
		alarm += block[5];

		dprintf("alarm: %i\n", alarm);

		if (alarm == 0xffff) {
			data->CalendarNote->Alarm.AlarmEnabled = 0;
		} else {
			data->CalendarNote->Alarm.AlarmEnabled = 1;
		}

		GetNoteAlarm(alarm, &(data->CalendarNote->Time), &(data->CalendarNote->Alarm));

		data->CalendarNote->Time.Hour = 0;
		data->CalendarNote->Time.Minute = 0;
		data->CalendarNote->Time.Second = 0;
		data->CalendarNote->Time.Year = (((unsigned int)block[6]) << 8) + block[7];

		char_decode_unicode(data->CalendarNote->Text, (block + 10), block[9] << 1);
		break;
	default:
		data->CalendarNote->Type = -1;
		return GN_ERR_UNKNOWN;
	}
	return GN_ERR_NONE;
}
