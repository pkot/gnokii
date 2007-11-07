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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001      Chris Kemp, Marcin Wiacek
  Copyright (C) 2002      Pavel Machek <pavel@ucw.cz>
  Copyright (C) 2002-2004 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan  

  This file provides support for ringtones.

*/

#include "config.h"
#include "compat.h"
#include "gnokii.h"
#include "misc.h"

/* Beats-per-Minute Encoding */

int BeatsPerMinute[] = {
	25,
	28,
	31,
	35,
	40,
	45,
	50,
	56,
	63,
	70,
	80,
	90,
	100,
	112,
	125,
	140,
	160,
	180,
	200,
	225,
	250,
	285,
	320,
	355,
	400,
	450,
	500,
	565,
	635,
	715,
	800,
	900
};

int OctetAlign(unsigned char *Dest, int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) {
		gn_ringtone_clear_bit(Dest, CurrentBit+i);
		i++;
	}

	return CurrentBit+i;
}

int OctetAlignNumber(int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) {
		i++;
	}

	return CurrentBit+i;
}

int BitPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
	int i;

	for (i=0; i<Bits; i++)
		if (gn_ringtone_get_bit(Source, i))
			gn_ringtone_set_bit(Dest, CurrentBit+i);
		else
			gn_ringtone_clear_bit(Dest, CurrentBit+i);

	return CurrentBit+Bits;
}

int GetTempo(int Beats)
{
	int i=0;

	while ( i < sizeof(BeatsPerMinute)/sizeof(BeatsPerMinute[0])) {

		if (Beats<=BeatsPerMinute[i])
			break;
		i++;
	}

	return (i << 3);
}

int BitPackByte(unsigned char *Dest, int CurrentBit, unsigned char Command, int Bits)
{
	unsigned char Byte[]={Command};

	return BitPack(Dest, CurrentBit, Byte, Bits);
}



/* This is messy but saves using the math library! */

int GSM_GetDuration(int number, unsigned char *spec)
{
	int duration=0;

	switch (number) {

	case 128*3/2:
		duration=GN_RINGTONE_Duration_Full; *spec = GN_RINGTONE_DottedNote; break;
	case 128*2/3:
		duration=GN_RINGTONE_Duration_Full; *spec = GN_RINGTONE_Length_2_3; break;
	case 128:
		duration=GN_RINGTONE_Duration_Full; *spec = GN_RINGTONE_NoSpecialDuration; break;
	case 64*9/4:
		duration=GN_RINGTONE_Duration_1_2; *spec = GN_RINGTONE_DoubleDottedNote; break;
	case 64*3/2:
		duration=GN_RINGTONE_Duration_1_2; *spec = GN_RINGTONE_DottedNote; break;
	case 64*2/3:
		duration=GN_RINGTONE_Duration_1_2; *spec = GN_RINGTONE_Length_2_3; break;
	case 64:
		duration=GN_RINGTONE_Duration_1_2; *spec = GN_RINGTONE_NoSpecialDuration; break;
	case 32*9/4:
		duration=GN_RINGTONE_Duration_1_4; *spec = GN_RINGTONE_DoubleDottedNote; break;
	case 32*3/2:
		duration=GN_RINGTONE_Duration_1_4; *spec = GN_RINGTONE_DottedNote; break;
	case 32*2/3:
		duration=GN_RINGTONE_Duration_1_4; *spec = GN_RINGTONE_Length_2_3; break;
	case 32:
		duration=GN_RINGTONE_Duration_1_4; *spec = GN_RINGTONE_NoSpecialDuration; break;
	case 16*9/4:
		duration=GN_RINGTONE_Duration_1_8; *spec = GN_RINGTONE_DoubleDottedNote; break;
	case 16*3/2:
		duration=GN_RINGTONE_Duration_1_8; *spec = GN_RINGTONE_DottedNote; break;
	case 16*2/3:
		duration=GN_RINGTONE_Duration_1_8; *spec = GN_RINGTONE_Length_2_3; break;
	case 16:
		duration=GN_RINGTONE_Duration_1_8; *spec = GN_RINGTONE_NoSpecialDuration; break;
	case 8*9/4:
		duration=GN_RINGTONE_Duration_1_16; *spec = GN_RINGTONE_DoubleDottedNote; break;
	case 8*3/2:
		duration=GN_RINGTONE_Duration_1_16; *spec = GN_RINGTONE_DottedNote; break;
	case 8*2/3:
		duration=GN_RINGTONE_Duration_1_16; *spec = GN_RINGTONE_Length_2_3; break;
	case 8:
		duration=GN_RINGTONE_Duration_1_16; *spec = GN_RINGTONE_NoSpecialDuration; break;
	case 4*9/4:
		duration=GN_RINGTONE_Duration_1_32; *spec = GN_RINGTONE_DoubleDottedNote; break;
	case 4*3/2:
		duration=GN_RINGTONE_Duration_1_32; *spec = GN_RINGTONE_DottedNote; break;
	case 4*2/3:
		duration=GN_RINGTONE_Duration_1_32; *spec = GN_RINGTONE_Length_2_3; break;
	case 4:
		duration=GN_RINGTONE_Duration_1_32; *spec = GN_RINGTONE_NoSpecialDuration; break;
	}

	return duration;
}


GNOKII_API int gn_note_get(int number)
{
	int note = 0;

	if (number != 255) {
		note = number % 14;
		switch (note) {
		case 0:
			note=GN_RINGTONE_Note_C; break;
		case 1:
			note=GN_RINGTONE_Note_Cis; break;
		case 2:
			note=GN_RINGTONE_Note_D; break;
		case 3:
			note=GN_RINGTONE_Note_Dis; break;
		case 4:
			note=GN_RINGTONE_Note_E; break;
		case 6:
			note=GN_RINGTONE_Note_F; break;
		case 7:
			note=GN_RINGTONE_Note_Fis; break;
		case 8:
			note=GN_RINGTONE_Note_G; break;
		case 9:
			note=GN_RINGTONE_Note_Gis; break;
		case 10:
			note=GN_RINGTONE_Note_A; break;
		case 11:
			note=GN_RINGTONE_Note_Ais; break;
		case 12:
			note=GN_RINGTONE_Note_H; break;
		}
	}
	else note = GN_RINGTONE_Note_Pause;

	return note;

}

int GSM_GetScale(int number)
{
	int scale=-1;

	if (number!=255) {
		scale=number/14;

		/* Ensure the scale is valid */
		scale%=4;

		scale=scale<<6;
	}
	return scale;
}


/* This function packs the ringtone from the structure, so it can be set
   or sent via sms to another phone.
   Function returns number of packed notes and changes maxlength to
   number of used chars in "package" */

GNOKII_API unsigned char gn_ringtone_pack(gn_ringtone *ringtone, unsigned char *package, int *maxlength)
{
	int StartBit=0;
	int i;
	unsigned char CommandLength = 0x02;
	unsigned char spec;
	int oldscale=10, newscale;
	int HowMany=0, HowLong=0, StartNote=0, EndNote=0;

	StartBit=BitPackByte(package, StartBit, CommandLength, 8);
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_Programming, 7);

	/* The page 3-23 of the specs says that <command-part> is always
	   octet-aligned. */
	StartBit=OctetAlign(package, StartBit);

	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_Sound, 7);
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_BasicSongType, 3);

	/* Packing the name of the tune. */
	StartBit=BitPackByte(package, StartBit, strlen(ringtone->name)<<4, 4);
	StartBit=BitPack(package, StartBit, ringtone->name, 8*strlen(ringtone->name));

	/* Info about song pattern */
	StartBit=BitPackByte(package, StartBit, 0x01, 8); /* One song pattern */
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_PatternHeaderId, 3);
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_A_part, 2);
	StartBit=BitPackByte(package, StartBit, 0, 4); /* No loop value */

	/* Info, how long is contents for SMS */
	HowLong=30+8*strlen(ringtone->name)+17+8+8+13;

	/* Calculate the number of instructions in the tune.
	   Each Note contains Note and (sometimes) Scale.
	   Default Tempo and Style are instructions too. */
	HowMany=2; /* Default Tempo and Style */

	for(i = 0; i < ringtone->notes_count; i++) {

		/* PC Composer 2.0.010 doesn't like, when we start ringtone from pause:
		   it displays that the format is invalid and
		   hangs, when you move mouse over place, where pause is */
		if (gn_note_get(ringtone->notes[i].note)==GN_RINGTONE_Note_Pause && oldscale==10) {
			StartNote++;
		} else {

			/* we don't write Scale info before "Pause" note - it saves space */
			if (gn_note_get(ringtone->notes[i].note)!=GN_RINGTONE_Note_Pause &&
			    oldscale!=(newscale=GSM_GetScale(ringtone->notes[i].note))) {

				/* We calculate, if we have space to add next scale instruction */
				if (((HowLong+5)/8)<=(*maxlength-1)) {
					oldscale=newscale;
					HowMany++;
					HowLong+=5;
				} else {
					break;
				}
			}

			/* We calculate, if we have space to add next note instruction */
			if (((HowLong+12)/8)<=(*maxlength-1)) {
				HowMany++;
				EndNote++;
				HowLong+=12;
			} else {
				break;
			}
		}

		/* If we are sure, we pack it for SMS or setting to phone, not for OTT file */
		if (*maxlength<1000) {
			/* Pc Composer gives this as the phone limitation */
			if ((EndNote-StartNote)==GN_RINGTONE_MAX_NOTES-1) break;
		}
	}

	StartBit=BitPackByte(package, StartBit, HowMany, 8);

	/* Style */
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_StyleInstructionId, 3);
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_ContinuousStyle, 2);

	/* Beats per minute/tempo of the tune */
	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_TempoInstructionId, 3);
	StartBit=BitPackByte(package, StartBit, GetTempo(ringtone->tempo), 5);

	/* Default scale */
	oldscale=10;

	/* Notes packing */
	for(i=StartNote; i<(EndNote+StartNote); i++) {

		/* we don't write Scale info before "Pause" note - it saves place */
		if (gn_note_get(ringtone->notes[i].note)!=GN_RINGTONE_Note_Pause &&
		    oldscale!=(newscale=GSM_GetScale(ringtone->notes[i].note))) {
			oldscale=newscale;
			StartBit=BitPackByte(package, StartBit, GN_RINGTONE_ScaleInstructionId, 3);
			StartBit=BitPackByte(package, StartBit, GSM_GetScale(ringtone->notes[i].note), 2);
		}

		/* Note */
		StartBit=BitPackByte(package, StartBit, GN_RINGTONE_NoteInstructionId, 3);
		StartBit=BitPackByte(package, StartBit, gn_note_get(ringtone->notes[i].note), 4);
		StartBit=BitPackByte(package, StartBit, GSM_GetDuration(ringtone->notes[i].duration,&spec), 3);
		StartBit=BitPackByte(package, StartBit, spec, 2);
	}

	StartBit=OctetAlign(package, StartBit);

	StartBit=BitPackByte(package, StartBit, GN_RINGTONE_CommandEnd, 8);

	if (StartBit!=OctetAlignNumber(HowLong))
		dprintf("Error in PackRingtone - StartBit different to HowLong %d - %d)\n", StartBit,OctetAlignNumber(HowLong));

	*maxlength=StartBit/8;

	return(EndNote+StartNote);
}


int BitUnPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
	int i;

	for (i=0; i<Bits; i++)
		if (gn_ringtone_get_bit(Dest, CurrentBit+i)) {
			gn_ringtone_set_bit(Source, i);
		} else {
			gn_ringtone_clear_bit(Source, i);
		}

	return CurrentBit+Bits;
}

int BitUnPackInt(unsigned char *Src, int CurrentBit, int *integer, int Bits)
{
	int l=0,z=128,i;

	for (i=0; i<Bits; i++) {
		if (gn_ringtone_get_bit(Src, CurrentBit+i)) l=l+z;
		z=z/2;
	}

	*integer=l;

	return CurrentBit+i;
}

int OctetUnAlign(int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) i++;

	return CurrentBit+i;
}


/* TODO: better checking, if contents of ringtone is OK */

GNOKII_API gn_error gn_ringtone_unpack(gn_ringtone *ringtone, unsigned char *package, int maxlength)
{
	int StartBit = 0;
	int spec, duration, scale;
	int HowMany;
	int l, q, i;

	StartBit = BitUnPackInt(package, StartBit, &l, 8);
	if (l != 0x02) {
		dprintf("Not header\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

	StartBit = BitUnPackInt(package, StartBit, &l, 7);
	if (l != GN_RINGTONE_Programming) {
		dprintf("Not RingingToneProgramming\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

/* The page 3-23 of the specs says that <command-part> is always
   octet-aligned. */
	StartBit = OctetUnAlign(StartBit);

	StartBit = BitUnPackInt(package, StartBit, &l, 7);
	if (l != GN_RINGTONE_Sound) {
		dprintf("Not Sound\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

	StartBit = BitUnPackInt(package, StartBit, &l, 3);
	if (l != GN_RINGTONE_BasicSongType) {
		dprintf("Not BasicSongType\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

/* Getting length of the tune name */
	StartBit = BitUnPackInt(package, StartBit, &l, 4);
	l = l >> 4;

/* Unpacking the name of the tune. */
	StartBit = BitUnPack(package, StartBit, ringtone->name, 8*l);
	ringtone->name[l] = 0;

	StartBit = BitUnPackInt(package, StartBit, &l, 8);
	if (l != 1) return GN_ERR_WRONGDATAFORMAT;

	StartBit = BitUnPackInt(package, StartBit, &l, 3);
	if (l != GN_RINGTONE_PatternHeaderId) {
		dprintf("Not PatternHeaderId\n");
		return GN_ERR_WRONGDATAFORMAT;
	}

	StartBit += 2; //Pattern ID - we ignore it

	StartBit = BitUnPackInt(package, StartBit, &l, 4);

	HowMany = 0;
	StartBit = BitUnPackInt(package, StartBit, &HowMany, 8);

	scale = 0;
	ringtone->notes_count = 0;

	for (i = 0; i < HowMany; i++) {

		StartBit = BitUnPackInt(package, StartBit, &q, 3);
		switch (q) {
		case GN_RINGTONE_VolumeInstructionId:
			StartBit += 4;
			break;
		case GN_RINGTONE_StyleInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &l, 2);
			l = l >> 3;
			break;
		case GN_RINGTONE_TempoInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &l, 5);
			l = l >> 3;
			ringtone->tempo = BeatsPerMinute[l];
			break;
		case GN_RINGTONE_ScaleInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &scale, 2);
			scale = scale >> 6;
			break;
		case GN_RINGTONE_NoteInstructionId:
			StartBit = BitUnPackInt(package, StartBit, &l, 4);

			switch (l) {
			case GN_RINGTONE_Note_C   : ringtone->notes[ringtone->notes_count].note = 0;   break;
			case GN_RINGTONE_Note_Cis : ringtone->notes[ringtone->notes_count].note = 1;   break;
			case GN_RINGTONE_Note_D   : ringtone->notes[ringtone->notes_count].note = 2;   break;
			case GN_RINGTONE_Note_Dis : ringtone->notes[ringtone->notes_count].note = 3;   break;
			case GN_RINGTONE_Note_E   : ringtone->notes[ringtone->notes_count].note = 4;   break;
			case GN_RINGTONE_Note_F   : ringtone->notes[ringtone->notes_count].note = 6;   break;
			case GN_RINGTONE_Note_Fis : ringtone->notes[ringtone->notes_count].note = 7;   break;
			case GN_RINGTONE_Note_G   : ringtone->notes[ringtone->notes_count].note = 8;   break;
			case GN_RINGTONE_Note_Gis : ringtone->notes[ringtone->notes_count].note = 9;   break;
			case GN_RINGTONE_Note_A   : ringtone->notes[ringtone->notes_count].note = 10;  break;
			case GN_RINGTONE_Note_Ais : ringtone->notes[ringtone->notes_count].note = 11;  break;
			case GN_RINGTONE_Note_H   : ringtone->notes[ringtone->notes_count].note = 12;  break;
			default                   : ringtone->notes[ringtone->notes_count].note = 255; break; /* Pause ? */
			}

			if (ringtone->notes[ringtone->notes_count].note != 255)
				ringtone->notes[ringtone->notes_count].note = ringtone->notes[ringtone->notes_count].note + scale*14;

			StartBit = BitUnPackInt(package, StartBit, &duration, 3);

			StartBit = BitUnPackInt(package, StartBit, &spec, 2);

			if (duration==GN_RINGTONE_Duration_Full && spec == GN_RINGTONE_DottedNote)
				ringtone->notes[ringtone->notes_count].duration=128*3/2;
			if (duration==GN_RINGTONE_Duration_Full && spec == GN_RINGTONE_Length_2_3)
				ringtone->notes[ringtone->notes_count].duration=128*2/3;
			if (duration==GN_RINGTONE_Duration_Full && spec == GN_RINGTONE_NoSpecialDuration)
				ringtone->notes[ringtone->notes_count].duration=128;
			if (duration==GN_RINGTONE_Duration_1_2 && spec == GN_RINGTONE_DottedNote)
				ringtone->notes[ringtone->notes_count].duration=64*3/2;
			if (duration==GN_RINGTONE_Duration_1_2 && spec == GN_RINGTONE_Length_2_3)
				ringtone->notes[ringtone->notes_count].duration=64*2/3;
			if (duration==GN_RINGTONE_Duration_1_2 && spec == GN_RINGTONE_NoSpecialDuration)
				ringtone->notes[ringtone->notes_count].duration=64;
			if (duration==GN_RINGTONE_Duration_1_4 && spec == GN_RINGTONE_DottedNote)
				ringtone->notes[ringtone->notes_count].duration=32*3/2;
			if (duration==GN_RINGTONE_Duration_1_4 && spec == GN_RINGTONE_Length_2_3)
				ringtone->notes[ringtone->notes_count].duration=32*2/3;
			if (duration==GN_RINGTONE_Duration_1_4 && spec == GN_RINGTONE_NoSpecialDuration)
				ringtone->notes[ringtone->notes_count].duration=32;
			if (duration==GN_RINGTONE_Duration_1_8 && spec == GN_RINGTONE_DottedNote)
				ringtone->notes[ringtone->notes_count].duration=16*3/2;
			if (duration==GN_RINGTONE_Duration_1_8 && spec == GN_RINGTONE_Length_2_3)
				ringtone->notes[ringtone->notes_count].duration=16*2/3;
			if (duration==GN_RINGTONE_Duration_1_8 && spec == GN_RINGTONE_NoSpecialDuration)
				ringtone->notes[ringtone->notes_count].duration=16;
			if (duration==GN_RINGTONE_Duration_1_16 && spec == GN_RINGTONE_DottedNote)
				ringtone->notes[ringtone->notes_count].duration=8*3/2;
			if (duration==GN_RINGTONE_Duration_1_16 && spec == GN_RINGTONE_Length_2_3)
				ringtone->notes[ringtone->notes_count].duration=8*2/3;
			if (duration==GN_RINGTONE_Duration_1_16 && spec == GN_RINGTONE_NoSpecialDuration)
				ringtone->notes[ringtone->notes_count].duration=8;
			if (duration==GN_RINGTONE_Duration_1_32 && spec == GN_RINGTONE_DottedNote)
				ringtone->notes[ringtone->notes_count].duration=4*3/2;
			if (duration==GN_RINGTONE_Duration_1_32 && spec == GN_RINGTONE_Length_2_3)
				ringtone->notes[ringtone->notes_count].duration=4*2/3;
			if (duration==GN_RINGTONE_Duration_1_32 && spec == GN_RINGTONE_NoSpecialDuration)
				ringtone->notes[ringtone->notes_count].duration=4;

			if (ringtone->notes_count == GN_RINGTONE_MAX_NOTES-1) break;

			ringtone->notes_count++;
			break;
		default:
			dprintf("Unsupported block\n");
			return GN_ERR_WRONGDATAFORMAT;
		}
	}

	return GN_ERR_NONE;
}


gn_error GSM_ReadRingtoneFromSMS(gn_sms *message, gn_ringtone *ringtone)
{
	if (message->udh.udh[0].type == GN_SMS_UDH_Ringtone) {
		return gn_ringtone_unpack(ringtone, message->user_data[0].u.text, message->user_data[0].length);
	} else return GN_ERR_WRONGDATAFORMAT;
}

int ringtone_sms_encode(unsigned char *message, gn_ringtone *ringtone)
{
	int j = GN_SMS_8BIT_MAX_LENGTH;
	gn_ringtone_pack(ringtone, message, &j);
	return j;
}

/* Returns message length */
int imelody_sms_encode(unsigned char *imelody, unsigned char *message)
{
	unsigned int current = 0;

	dprintf("EMS iMelody\n");
	message[current++] = strlen(imelody) + 3;
	message[current++] = 0x0c; 	/* iMelody code */
	message[current++] = strlen(imelody) + 1;
	message[current++] = 0;		      /* Position in text this melody is at */
	/* FIXME: check the overflow */
	strcpy(message + current, imelody);

	return (current + strlen(imelody));
}

GNOKII_API void gn_ringtone_get_tone(const gn_ringtone *ringtone, int n, int *freq, int *ulen)
{
	float f;

	*freq = 0;
	*ulen = 0;

	if (n >= ringtone->notes_count) return;

	if (ringtone->notes[n].note != 255) {
		switch (ringtone->notes[n].note % 14) {
		case  0: f = 261.625565; break;
		case  1: f = 277.182631; break;
		case  2: f = 293.664768; break;
		case  3: f = 311.126984; break;
		case  4: f = 329.627557; break;
		case  5: f = 329.627557; break;
		case  6: f = 349.228231; break;
		case  7: f = 369.994423; break;
		case  8: f = 391.995436; break;
		case  9: f = 415.304698; break;
		case 10: f = 440.000000; break;
		case 11: f = 466.163762; break;
		case 12: f = 493.883301; break;
		case 13: f = 493.883301; break;
		default: f = 0; break;
		}
		switch (ringtone->notes[n].note / 14) {
		case 0: *freq = f; break;
		case 1: *freq = f * 2; break;
		case 2: *freq = f * 4; break;
		case 3: *freq = f * 8; break;
		default: *freq = 0; break;
		}
	}

	*ulen = 1875000 * ringtone->notes[n].duration / ringtone->tempo;
}

GNOKII_API void gn_ringtone_set_duration(gn_ringtone *ringtone, int n, int ulen)
{
	int l = ulen * ringtone->tempo / 240;
	gn_ringtone_note *note = ringtone->notes + n;

	if (l < 156250) {
		if (l < 54687) {
			if (l < 15625)
				note->duration = 0;
			else if (l < 39062)
				note->duration = 4;
			else
				note->duration = 4 * 3/2;
		} else {
			if (l < 78125)
				note->duration = 8;
			else if (l < 109375)
				note->duration = 8 * 3/2;
			else
				note->duration = 16;
		}
	} else {
		if (l < 437500) {
			if (l < 218750)
				note->duration = 16 * 3/2;
			else if (l < 312500)
				note->duration = 32;
			else
				note->duration = 32 * 3/2;
		} else {
			if (l < 625000)
				note->duration = 64;
			else if (l < 875000)
				note->duration = 64 * 3/2;
			else if (l < 1250000)
				note->duration = 128;
			else
				note->duration = 128 * 3/2;
		}
	}
}
