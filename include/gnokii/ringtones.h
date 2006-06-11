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
  Copyright (C) 2000      Marcin Wiacek, Chris Kemp
  Copyright (C) 2001-2003 Pawel Kot
  Copyright (C) 2002-2003 BORBELY Zoltan

  This file provides definitions of macros from the Smart Messaging
  Specification. It is mainly rewrite of the spec to C :-) Viva Nokia!

*/

#ifndef _gnokii_ringtones_h
#define _gnokii_ringtones_h

#include <gnokii/error.h>

/* NoteValue is encoded as octave(scale)*14 + note */
/* where for note: c=0, d=2, e=4 .... */
/* ie. c#=1 and 5 and 13 are invalid */
/* note=255 means a pause */

#define GN_RINGTONE_MAX_NOTES 1024

/* Structure to hold note of ringtone. */

typedef struct {
	unsigned char duration;
	unsigned char note;
} gn_ringtone_note;

/* Structure to hold ringtones. */
typedef struct {
	int location;
	char name[20];
	unsigned char tempo;
	unsigned int notes_count;
	gn_ringtone_note notes[GN_RINGTONE_MAX_NOTES];
} gn_ringtone;

#define gn_ringtone_get_bit(stream, bitno) stream[(bitno) / 8]   &  1   << (7 - ((bitno) % 8))
#define gn_ringtone_set_bit(stream, bitno) stream[(bitno) / 8]   |= 1   << (7 - ((bitno) % 8))
#define gn_ringtone_clear_bit(stream, bitno) stream[(bitno) / 8] &= 255 -  (1 << (7 - ((bitno) % 8)))

/* These values are from Smart Messaging Specification Revision 2.0.0 pages
   3-23, ..., 3-29 */

/* Command-Part Encoding */
#define GN_RINGTONE_CancelCommand          (0x05 << 1) /* binary 0000 101 */
#define GN_RINGTONE_Programming            (0x25 << 1) /* binary 0100 101 */
#define GN_RINGTONE_Sound                  (0x1d << 1) /* binary 0011 101 */
#define GN_RINGTONE_Unicode                (0x22 << 1) /* binary 0100 010 */

/* Song-Type Encoding */
#define GN_RINGTONE_BasicSongType     (0x01 << 5) /* binary 001 */
#define GN_RINGTONE_TemporarySongType (0x02 << 5) /* binary 010 */
#define GN_RINGTONE_MidiSongType      (0x03 << 5) /* binary 011 */
#define GN_RINGTONE_DigitizedSongType (0x04 << 5) /* binary 100 */

/* Instruction ID Encoding */
#define GN_RINGTONE_PatternHeaderId      (0x00 << 5) /* binary 000 */
#define GN_RINGTONE_NoteInstructionId    (0x01 << 5) /* binary 001 */
#define GN_RINGTONE_ScaleInstructionId   (0x02 << 5) /* binary 010 */
#define GN_RINGTONE_StyleInstructionId   (0x03 << 5) /* binary 011 */
#define GN_RINGTONE_TempoInstructionId   (0x04 << 5) /* binary 100 */
#define GN_RINGTONE_VolumeInstructionId  (0x05 << 5) /* binary 101 */

/* Style-Value Encoding*/
#define GN_RINGTONE_NaturalStyle    (0x00 << 6) /* binary 00 */
#define GN_RINGTONE_ContinuousStyle (0x01 << 6) /* binary 01 */
#define GN_RINGTONE_StaccatoStyle   (0x02 << 6) /* binary 11 */

/* Note-Scale Encoding  */
#define GN_RINGTONE_Scale1 (0x00 << 6) /* binary 00 */
#define GN_RINGTONE_Scale2 (0x01 << 6) /* binary 01 */
#define GN_RINGTONE_Scale3 (0x02 << 6) /* binary 10 */
#define GN_RINGTONE_Scale4 (0x03 << 6) /* binary 11 */

/* Note-Value Encoding */
#define GN_RINGTONE_Note_Pause (0x00 << 4) /* binary 0000 */
#define GN_RINGTONE_Note_C     (0x01 << 4) /* binary 0001 */
#define GN_RINGTONE_Note_Cis   (0x02 << 4) /* binary 0010 */
#define GN_RINGTONE_Note_D     (0x03 << 4) /* binary 0011 */
#define GN_RINGTONE_Note_Dis   (0x04 << 4) /* binary 0100 */
#define GN_RINGTONE_Note_E     (0x05 << 4) /* binary 0101 */
#define GN_RINGTONE_Note_F     (0x06 << 4) /* binary 0110 */
#define GN_RINGTONE_Note_Fis   (0x07 << 4) /* binary 0111 */
#define GN_RINGTONE_Note_G     (0x08 << 4) /* binary 1000 */
#define GN_RINGTONE_Note_Gis   (0x09 << 4) /* binary 1001 */
#define GN_RINGTONE_Note_A     (0x0a << 4) /* binary 1010 */
#define GN_RINGTONE_Note_Ais   (0x0b << 4) /* binary 1011 */
#define GN_RINGTONE_Note_H     (0x0c << 4) /* binary 1100 */

/* Note-Duration Encoding */
#define GN_RINGTONE_Duration_Full (0x00 << 5) /* binary 000 */
#define GN_RINGTONE_Duration_1_2  (0x01 << 5) /* binary 001 */
#define GN_RINGTONE_Duration_1_4  (0x02 << 5) /* binary 010 */
#define GN_RINGTONE_Duration_1_8  (0x03 << 5) /* binary 011 */
#define GN_RINGTONE_Duration_1_16 (0x04 << 5) /* binary 100 */
#define GN_RINGTONE_Duration_1_32 (0x05 << 5) /* binary 101 */

/* Note-Duration-Specifier Encoding */
#define GN_RINGTONE_NoSpecialDuration (0x00 << 6) /* binary 00 */
#define GN_RINGTONE_DottedNote        (0x01 << 6) /* binary 01 */
#define GN_RINGTONE_DoubleDottedNote  (0x02 << 6) /* binary 10 */
#define GN_RINGTONE_Length_2_3        (0x03 << 6) /* binary 11 */

/* Pattern ID Encoding */
#define GN_RINGTONE_A_part (0x00 << 6) /* binary 00 */
#define GN_RINGTONE_B_part (0x01 << 6) /* binary 01 */
#define GN_RINGTONE_C_part (0x02 << 6) /* binary 10 */
#define GN_RINGTONE_D_part (0x03 << 6) /* binary 11 */

/* Command-End */
#define GN_RINGTONE_CommandEnd (0x00) /* binary 00000000 */

/* Definition of the Note type */
typedef struct {
	int scale;
	int note_id;
	int duration;
	int duration_specifier;
} gn_note;

#define GN_RINGTONE_PACKAGE_MAX_LENGTH 200

/* From PC Composer help */
#define GN_RINGTONE_NOTES_MAX_NUMBER 130

GNOKII_API gn_error gn_file_ringtone_read(char *filename, gn_ringtone *ringtone);
GNOKII_API gn_error gn_file_ringtone_save(char *filename, gn_ringtone *ringtone);

GNOKII_API unsigned char gn_ringtone_pack(gn_ringtone *ringtone, unsigned char *package, int *maxlength);
GNOKII_API gn_error gn_ringtone_unpack(gn_ringtone *ringtone, unsigned char *package, int maxlength);
GNOKII_API int gn_note_get(int number);
GNOKII_API void gn_ringtone_get_tone(const gn_ringtone *ringtone, int n, int *freq, int *ulen);
GNOKII_API void gn_ringtone_set_duration(gn_ringtone *ringtone, int n, int ulen);

#endif	/* _gnokii_ringtones_h */
