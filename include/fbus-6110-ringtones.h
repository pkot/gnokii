/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides definitions of macros from the Smart Messaging
  Specification. It is mainly rewrite of the spec to C :-) Viva Nokia!

  Last modification: Thu Apr  6 01:32:14 CEST 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __fbus_6110_ringtones_h
#define __fbus_6110_ringtones_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "misc.h"
#include "gsm-common.h"
#include "fbus-6110.h"

#define GetBit(Stream,BitNr) Stream[(BitNr)/8] & 1<<(7-((BitNr)%8))
#define SetBit(Stream,BitNr) Stream[(BitNr)/8] |= 1<<(7-((BitNr)%8))
#define ClearBit(Stream,BitNr) Stream[(BitNr)/8] &= 255 - (1 << (7-((BitNr)%8)))

/* These values are from Smart Messaging Specification Revision 2.0.0 pages
   3-23, ..., 3-29 */

/* Command-Part Encoding */

#define CancelCommand          (0x05<<1) /* binary 0000 101 */
#define RingingToneProgramming (0x25<<1) /* binary 0100 101 */
#define Sound                  (0x1d<<1) /* binary 0011 101 */
#define Unicode                (0x22<<1) /* binary 0100 010 */

/* Song-Type Encoding */

#define BasicSongType     (0x01<<5) /* binary 001 */
#define TemporarySongType (0x02<<5) /* binary 010 */
#define MidiSongType      (0x03<<5) /* binary 011 */
#define DigitizedSongType (0x04<<5) /* binary 100 */

/* Instruction ID Encoding */

#define PatternHeaderId      (0x00<<5) /* binary 000 */
#define NoteInstructionId    (0x01<<5) /* binary 001 */
#define ScaleInstructionId   (0x02<<5) /* binary 010 */
#define StyleInstructionId   (0x03<<5) /* binary 011 */
#define TempoInstructionId   (0x04<<5) /* binary 100 */
#define VolumeInstructionId (0x05<<5) /* binary 101 */

/* Style-Value Encoding*/

#define NaturalStyle    (0x00<<6) /* binary 00 */
#define ContinuousStyle (0x01<<6) /* binary 01 */
#define StaccatoStyle   (0x02<<6) /* binary 11 */

/* Note-Scale Encoding  */

#define Scale1 (0x00<<6) /* binary 00 */
#define Scale2 (0x01<<6) /* binary 01 */
#define Scale3 (0x02<<6) /* binary 10 */
#define Scale4 (0x03<<6) /* binary 11 */

/* Note-Value Encoding */

#define Note_Pause (0x00<<4) /* binary 0000 */
#define Note_C     (0x01<<4) /* binary 0001 */
#define Note_Cis   (0x02<<4) /* binary 0010 */
#define Note_D     (0x03<<4) /* binary 0011 */
#define Note_Dis   (0x04<<4) /* binary 0100 */
#define Note_E     (0x05<<4) /* binary 0101 */
#define Note_F     (0x06<<4) /* binary 0110 */
#define Note_Fis   (0x07<<4) /* binary 0111 */
#define Note_G     (0x08<<4) /* binary 1000 */
#define Note_Gis   (0x09<<4) /* binary 1001 */
#define Note_A     (0x0a<<4) /* binary 1010 */
#define Note_Ais   (0x0b<<4) /* binary 1011 */
#define Note_H     (0x0c<<4) /* binary 1100 */

/* Note-Duration Encoding */

#define Duration_Full (0x00<<5) /* binary 000 */
#define Duration_1_2  (0x01<<5) /* binary 001 */
#define Duration_1_4  (0x02<<5) /* binary 010 */
#define Duration_1_8  (0x03<<5) /* binary 011 */
#define Duration_1_16 (0x04<<5) /* binary 100 */
#define Duration_1_32 (0x05<<5) /* binary 101 */

/* Note-Duration-Specifier Encoding */

#define NoSpecialDuration (0x00<<6) /* binary 00 */
#define DottedNote        (0x01<<6) /* binary 01 */
#define DoubleDottedNote  (0x02<<6) /* binary 10 */
#define Length_2_3        (0x03<<6) /* binary 11 */

/* Command-End */

#define CommandEnd (0x00) /* binary 00000000 */

/* Definition of the Note type */

typedef struct {
  int Scale;
  int NoteID;
  int Duration;
  int DurationSpecifier;
} Note;

#define FB61_MAX_RINGTONE_PACKAGE_LENGTH 200

u8 FB61_PackRingtone(GSM_Ringtone *ringtone, char *package);
int FB61_GetDuration(int number, unsigned char *spec);
int FB61_GetNote(int number);
int FB61_GetScale(int number);

#endif	/* __fbus_6110_ringtones_h */
