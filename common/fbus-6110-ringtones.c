/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml. & Hugh Blemings.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides support for ringtones.

  Last modification: Sat Jul 10 17:26:23 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include "fbus-6110-ringtones.h"

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

int FB61_OctetAlign(unsigned char *Dest, int CurrentBit)
{
  int i=0;

  while((CurrentBit+i)%8) {
    ClearBit(Dest, CurrentBit+i);
    i++;
  }

  return CurrentBit+i;
}

int FB61_BitPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{

  int i;

  for (i=0; i<Bits; i++)
    if (GetBit(Source, i))
      SetBit(Dest, CurrentBit+i);
    else
      ClearBit(Dest, CurrentBit+i);

  return CurrentBit+Bits;
}

int FB61_GetTempo(int Beats) {

  int i=0;

  while ( i < sizeof(BeatsPerMinute)/sizeof(BeatsPerMinute[0])) {

    if (Beats<=BeatsPerMinute[i])
      break;
    i++;
  }

  return i<<3;
}    

int FB61_BitPackByte(unsigned char *Dest, int CurrentBit, unsigned char Command, int Bits) {

  unsigned char Byte[]={Command};

  return FB61_BitPack(Dest, CurrentBit, Byte, Bits);
}

/* This function is used to send the ringtone to the phone. The name of the
   tune as displayed on the Nokia is Name, beats per minute (aka tempo in
   NCDS) is stored in the variable Beats. Enjoy! */

void FB61_Ringtone(char *Name, int Beats, int NrNotes, Note *Notes)
{

  unsigned char req[255];
  int StartBit=0, i=0;
  unsigned char FBUSRingtuneHeader[] = { 0x0c, 0x01, /* FBUS RingTone header*/

					 /* Next bytes are from Smart Messaging
					    Specification version 2.0.0 */

					 0x06,       /* User Data Header Length */
					 0x05,       /* IEI FIXME: What is this? */
					 0x04,       /* IEDL FIXME: What is this? */
					 0x15, 0x81, /* Destination port */
					 0x15, 0x81  /* Originator port, only to fill in the two bytes :-) */
  };

  unsigned char CommandLength = 0x02;

  StartBit=FB61_BitPack(req, StartBit, FBUSRingtuneHeader, 72);
  StartBit=FB61_BitPackByte(req, StartBit, CommandLength, 8);
  StartBit=FB61_BitPackByte(req, StartBit, RingingToneProgramming, 7);

  /* The page 3-23 of the specs says that <command-part> is always octet-aligned */
  StartBit=FB61_OctetAlign(req, StartBit);

  StartBit=FB61_BitPackByte(req, StartBit, Sound, 7);
  StartBit=FB61_BitPackByte(req, StartBit, BasicSongType, 3);

  /* Packing the name of the tune */
  StartBit=FB61_BitPackByte(req, StartBit, strlen(Name)<<4, 4);
  StartBit=FB61_BitPack(req, StartBit, Name, 8*strlen(Name));

  /* FIXME: unknown */
  StartBit=FB61_BitPackByte(req, StartBit, 0x01, 8);
  StartBit=FB61_BitPackByte(req, StartBit, 0x00, 8);
  StartBit=FB61_BitPackByte(req, StartBit, 0x00, 1);

  /* Number of notes in the tune (+3). FIXME: It works, but why +3? :-) */
  StartBit=FB61_BitPackByte(req, StartBit, NrNotes+3, 8);

  /* Style */
  StartBit=FB61_BitPackByte(req, StartBit, StyleInstructionId, 3);
  StartBit=FB61_BitPackByte(req, StartBit, ContinuousStyle, 2);

  /* Beats per minute/tempo of the tune */
  StartBit=FB61_BitPackByte(req, StartBit, TempoInstructionId, 3);
  StartBit=FB61_BitPackByte(req, StartBit, FB61_GetTempo(Beats), 5);

  /* Note Scale */
  StartBit=FB61_BitPackByte(req, StartBit, ScaleInstructionId, 3);
  StartBit=FB61_BitPackByte(req, StartBit, Scale1, 2);

  /* Notes packing */
  for(i=0; i<NrNotes; i++) {
    StartBit=FB61_BitPackByte(req, StartBit, NoteInstructionId, 3);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].NoteID, 4);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].Duration, 3);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].DurationSpecifier, 2);
  }

  StartBit=FB61_OctetAlign(req, StartBit);

  StartBit=FB61_BitPackByte(req, StartBit, CommandEnd, 8);

  /* 0x01 because we have only one frame... */
  StartBit=FB61_BitPackByte(req, StartBit, 1, 8);

  FB61_TX_SendMessage(StartBit/8, 0x12, req);
}

void FB61_SendRingtone(char *Name, int Beats)
{
  int NrNotes=14;
  Note Notes[14] = {
    { Note_A, Duration_Full, NoSpecialDuration},
    { Note_H, Duration_Full, NoSpecialDuration},
    { Note_C, Duration_Full, NoSpecialDuration},
    { Note_D, Duration_Full, NoSpecialDuration},
    { Note_E, Duration_Full, NoSpecialDuration},
    { Note_F, Duration_Full, NoSpecialDuration},
    { Note_G, Duration_Full, NoSpecialDuration}
  };

  FB61_Ringtone(Name, Beats, NrNotes, Notes);
}
