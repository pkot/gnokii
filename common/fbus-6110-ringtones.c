/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides support for ringtones.

  Last modification: Thu Apr  6 01:32:14 CEST 2000
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



/* This is messy but saves using the math library! */

int FB61_GetDuration(int number, unsigned char *spec) {

  int duration=0;

  switch (number) {

  case 128*3/2:
    duration=Duration_Full; *spec=DottedNote; break;  
  case 128*2/3:
    duration=Duration_Full; *spec=Length_2_3; break;  
  case 128:
    duration=Duration_Full; *spec=NoSpecialDuration; break;  
  case 64*9/4:
    duration=Duration_1_2; *spec=DoubleDottedNote; break;    
  case 64*3/2:
    duration=Duration_1_2; *spec=DottedNote; break;  
  case 64*2/3:
    duration=Duration_1_2; *spec=Length_2_3; break;  
  case 64:
    duration=Duration_1_2; *spec=NoSpecialDuration; break;  
  case 32*9/4:
    duration=Duration_1_4; *spec=DoubleDottedNote; break;    
  case 32*3/2:
    duration=Duration_1_4; *spec=DottedNote; break;  
  case 32*2/3:
    duration=Duration_1_4; *spec=Length_2_3; break;  
  case 32:
    duration=Duration_1_4; *spec=NoSpecialDuration; break;  
  case 16*9/4:
    duration=Duration_1_8; *spec=DoubleDottedNote; break;    
  case 16*3/2:
    duration=Duration_1_8; *spec=DottedNote; break;  
  case 16*2/3:
    duration=Duration_1_8; *spec=Length_2_3; break;  
  case 16:
    duration=Duration_1_8; *spec=NoSpecialDuration; break;  
  case 8*9/4:
    duration=Duration_1_16; *spec=DoubleDottedNote; break;    
  case 8*3/2:
    duration=Duration_1_16; *spec=DottedNote; break;  
  case 8*2/3:
    duration=Duration_1_16; *spec=Length_2_3; break;  
  case 8:
    duration=Duration_1_16; *spec=NoSpecialDuration; break;  
  case 4*9/4:
    duration=Duration_1_32; *spec=DoubleDottedNote; break;    
  case 4*3/2:
    duration=Duration_1_32; *spec=DottedNote; break;  
  case 4*2/3:
    duration=Duration_1_32; *spec=Length_2_3; break;  
  case 4:
    duration=Duration_1_32; *spec=NoSpecialDuration; break;  
  }

  return duration;
}


int FB61_GetNote(int number) {
  
  int note=0;
 
  if (number!=255) {
    note=number%14;
    switch (note) {

    case 0:
      note=Note_C; break;
    case 1:
      note=Note_Cis; break;
    case 2:
      note=Note_D; break;
    case 3:
      note=Note_Dis; break;
    case 4:
      note=Note_E; break;
    case 6:
      note=Note_F; break;
    case 7:
      note=Note_Fis; break;
    case 8:
      note=Note_G; break;
    case 9:
      note=Note_Gis; break;
    case 10:
      note=Note_A; break;
    case 11:
      note=Note_Ais; break;
    case 12:
      note=Note_H; break;
    }
  }
  else note = Note_Pause;

  return note;

}

int FB61_GetScale(int number) {

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
   or sent via sms to another phone. */

u8 FB61_PackRingtone(GSM_Ringtone *ringtone, char *package)
{
  int StartBit=0;
  int i;
  unsigned char CommandLength = 0x02;
  unsigned char spec;
  int oldscale=-1, newscale;

  StartBit=FB61_BitPackByte(package, StartBit, CommandLength, 8);
  StartBit=FB61_BitPackByte(package, StartBit, RingingToneProgramming, 7);

  /* The page 3-23 of the specs says that <command-part> is always
     octet-aligned. */
  StartBit=FB61_OctetAlign(package, StartBit);

  StartBit=FB61_BitPackByte(package, StartBit, Sound, 7);
  StartBit=FB61_BitPackByte(package, StartBit, BasicSongType, 3);

  /* Packing the name of the tune. */
  StartBit=FB61_BitPackByte(package, StartBit, strlen(ringtone->name)<<4, 4);
  StartBit=FB61_BitPack(package, StartBit, ringtone->name, 8*strlen(ringtone->name));

  /* FIXME: unknown */
  StartBit=FB61_BitPackByte(package, StartBit, 0x01, 8);
  StartBit=FB61_BitPackByte(package, StartBit, 0x00, 8);
  StartBit=FB61_BitPackByte(package, StartBit, 0x00, 1);

  /* Number of instructions in the tune. Each Note contains two instructions -
     Scale and Note. Default Tempo and Style are instructions too.  */
  StartBit=FB61_BitPackByte(package, StartBit, 2*(ringtone->NrNotes)+2, 8);

  /* Style */
  StartBit=FB61_BitPackByte(package, StartBit, StyleInstructionId, 3);
  StartBit=FB61_BitPackByte(package, StartBit, ContinuousStyle, 2);

  /* Beats per minute/tempo of the tune */
  StartBit=FB61_BitPackByte(package, StartBit, TempoInstructionId, 3);
  StartBit=FB61_BitPackByte(package, StartBit, FB61_GetTempo(ringtone->tempo), 5);

  /* Notes packing */
  for(i=0; i<ringtone->NrNotes; i++) {
    /* Scale */
    if (oldscale!=(newscale=FB61_GetScale(ringtone->notes[i].note))) {
      oldscale=newscale;
      StartBit=FB61_BitPackByte(package, StartBit, ScaleInstructionId, 3);
      StartBit=FB61_BitPackByte(package, StartBit, FB61_GetScale(ringtone->notes[i].note), 2);
    }
    /* Note */
    StartBit=FB61_BitPackByte(package, StartBit, NoteInstructionId, 3);
    StartBit=FB61_BitPackByte(package, StartBit, FB61_GetNote(ringtone->notes[i].note), 4);
    StartBit=FB61_BitPackByte(package, StartBit, FB61_GetDuration(ringtone->notes[i].duration,&spec), 3);
    StartBit=FB61_BitPackByte(package, StartBit, spec, 2);
  }

  StartBit=FB61_OctetAlign(package, StartBit);

  StartBit=FB61_BitPackByte(package, StartBit, CommandEnd, 8);

  return(StartBit/8);
}
