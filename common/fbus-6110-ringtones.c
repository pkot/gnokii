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

/* This function is used to send the ringtone to the phone. The name of the
   tune as displayed on the Nokia is Name, beats per minute (aka tempo in
   NCDS) is stored in the variable Beats. Enjoy! */

int FB61_PackRingtone(unsigned char *req, char *Name, int Beats, int NrNotes, Note *Notes)
{

  int StartBit=0, i=0;
  unsigned char FBUSRingtuneHeader[] = { 0x0c, 0x01, /* FBUS RingTone header*/

					 /* Next bytes are from Smart Messaging
					    Specification version 2.0.0 */

					 0x06,       /* User Data Header Length */
					 0x05,       /* IEI FIXME: What is this? */
					 0x04,       /* IEDL FIXME: What is this? */
					 0x15, 0x81, /* Destination port */
					 0x15, 0x81  /* Originator port, only
                                                        to fill in the two
                                                        bytes :-) */
  };

  unsigned char CommandLength = 0x02;

  StartBit=FB61_BitPack(req, StartBit, FBUSRingtuneHeader, 72);
  StartBit=FB61_BitPackByte(req, StartBit, CommandLength, 8);
  StartBit=FB61_BitPackByte(req, StartBit, RingingToneProgramming, 7);

  /* The page 3-23 of the specs says that <command-part> is always
     octet-aligned. */
  StartBit=FB61_OctetAlign(req, StartBit);

  StartBit=FB61_BitPackByte(req, StartBit, Sound, 7);
  StartBit=FB61_BitPackByte(req, StartBit, BasicSongType, 3);

  /* Packing the name of the tune. */
  StartBit=FB61_BitPackByte(req, StartBit, strlen(Name)<<4, 4);
  StartBit=FB61_BitPack(req, StartBit, Name, 8*strlen(Name));

  /* FIXME: unknown */
  StartBit=FB61_BitPackByte(req, StartBit, 0x01, 8);
  StartBit=FB61_BitPackByte(req, StartBit, 0x00, 8);
  StartBit=FB61_BitPackByte(req, StartBit, 0x00, 1);

  /* Number of instructions in the tune. Each Note contains two instructions -
     Scale and Note. Default Tempo and Style are instructions too.  */
  StartBit=FB61_BitPackByte(req, StartBit, 2*NrNotes+2, 8);

  /* Style */
  StartBit=FB61_BitPackByte(req, StartBit, StyleInstructionId, 3);
  StartBit=FB61_BitPackByte(req, StartBit, ContinuousStyle, 2);

  /* Beats per minute/tempo of the tune */
  StartBit=FB61_BitPackByte(req, StartBit, TempoInstructionId, 3);
  StartBit=FB61_BitPackByte(req, StartBit, FB61_GetTempo(Beats), 5);

  /* Notes packing */
  for(i=0; i<NrNotes; i++) {
    /* Scale */
    StartBit=FB61_BitPackByte(req, StartBit, ScaleInstructionId, 3);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].Scale, 2);
    /* Note */
    StartBit=FB61_BitPackByte(req, StartBit, NoteInstructionId, 3);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].NoteID, 4);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].Duration, 3);
    StartBit=FB61_BitPackByte(req, StartBit, Notes[i].DurationSpecifier, 2);
  }

  StartBit=FB61_OctetAlign(req, StartBit);

  StartBit=FB61_BitPackByte(req, StartBit, CommandEnd, 8);

  /* 0x01 because we have only one frame... */
  StartBit=FB61_BitPackByte(req, StartBit, 1, 8);

  return StartBit/8;
}

int GetDuration(int number) {

  int duration=0;

  switch (number) {
  case 1:
    duration=Duration_Full; break;
  case 2:
    duration=Duration_1_2; break;
  case 4:
    duration=Duration_1_4; break;
  case 8:
    duration=Duration_1_8; break;
  case 16:
    duration=Duration_1_16; break;
  case 32:
    duration=Duration_1_32; break;
  }

  return duration;
}

int GetScale(int number) {

  int scale=0;

  switch (number) {

    /* Some files on the net seem to use 1-3 */
    /* But Scale1 is not complete on my 6130 so start at Scale2 */
    /* Perhaps we need some intelligence here? */

  case 0:
    scale=Scale1; break;
  case 1:
    scale=Scale2; break;
  case 2:
    scale=Scale3; break;
  case 3:
    scale=Scale4; break;

  case 5:
    scale=Scale1; break;
  case 6:
    scale=Scale2; break;
  case 7:
    scale=Scale3; break;
  case 8:
    scale=Scale4; break;
  }

  return scale;
}

int FB61_PackRingtoneRTTTL(unsigned char *req, char *FileName)
{

  int size;

  int NrNotes=0;
  Note Notes[100];

  char Name[10];

  int DefNoteDuration=Duration_1_4;
  int DefNoteScale=Scale2;
  int DefBeats=63;

  unsigned char buffer[2000];
  unsigned char *def, *notes, *ptr;
  FILE *fd;

  fd=fopen(FileName, "r");
  if (!fd) {
    fprintf(stderr, _("File can not be opened!\n"));
    return 0;
  }

  fread(buffer, 2000, 1, fd);

#define RTTTL_SEP ":"

  /* This is for buggy RTTTL ringtones without name. */
  if (buffer[0] != RTTTL_SEP[0]) {
    strtok(buffer, RTTTL_SEP);
    sprintf(Name, "%s", buffer);
    def=strtok(NULL, RTTTL_SEP);
    notes=strtok(NULL, RTTTL_SEP);
  }
  else {
    sprintf(Name, "GNOKII");
    def=strtok(buffer, RTTTL_SEP);
    notes=strtok(NULL, RTTTL_SEP);
  }

  ptr=strtok(def, ", ");
  /* Parsing the <defaults> section. */
  while (ptr) {

    switch(*ptr) {
    case 'd':
    case 'D':
      DefNoteDuration=GetDuration(atoi(ptr+2));
      break;
    case 'o':
    case 'O':
      DefNoteScale=GetScale(atoi(ptr+2));
      break;
    case 'b':
    case 'B':
      DefBeats=atoi(ptr+2);
      break;
    }

    ptr=strtok(NULL,", ");
  }

#ifdef DEBUG
  printf("DefNoteDuration=%d\n", DefNoteDuration);
  printf("DefNoteScale=%d\n", DefNoteScale);
  printf("DefBeats=%d\n", DefBeats);
#endif

  /* Parsing the <note-command>+ section. */
  ptr=strtok(notes, ", ");
  while (ptr && NrNotes<100) {

    /* [<duration>] */
    Notes[NrNotes].Duration=GetDuration(atoi(ptr));
    if (Notes[NrNotes].Duration==0)
      Notes[NrNotes].Duration=DefNoteDuration;

    /* Skip all numbers in duration specification. */
    while(isdigit(*ptr))
      ptr++;

    /* <note> */

    switch(*ptr) {
    case 'p':
    case 'P':
      Notes[NrNotes].NoteID=Note_Pause;
      break;
    case 'c':
    case 'C':
      Notes[NrNotes].NoteID=Note_C;
      if (*(ptr+1)=='#') {
	Notes[NrNotes].NoteID=Note_Cis;
	ptr++;
      }
      break;
    case 'd':
    case 'D':
      Notes[NrNotes].NoteID=Note_D;
      if (*(ptr+1)=='#') {
	Notes[NrNotes].NoteID=Note_Dis;
	ptr++;
      }
      break;
    case 'e':
    case 'E':
      Notes[NrNotes].NoteID=Note_E;
      break;
    case 'f':
    case 'F':
      Notes[NrNotes].NoteID=Note_F;
      if (*(ptr+1)=='#') {
	Notes[NrNotes].NoteID=Note_Fis;
	ptr++;
      }
      break;
    case 'g':
    case 'G':
      Notes[NrNotes].NoteID=Note_G;
      if (*(ptr+1)=='#') {
	Notes[NrNotes].NoteID=Note_Gis;
	ptr++;
      }
      break;
    case 'a':
    case 'A':
      Notes[NrNotes].NoteID=Note_A;
      if (*(ptr+1)=='#') {
	Notes[NrNotes].NoteID=Note_Ais;
	ptr++;
      }
      break;
    case 'h':
    case 'H':
      /* FIXME: What is this? Non-conforming RTTTL? Is B really H? */
    case 'b':
    case 'B':
      Notes[NrNotes].NoteID=Note_H;
      break;
    default:

      Notes[NrNotes].NoteID=Note_Pause;
    }
    ptr++;

    /* [<scale>] */

    Notes[NrNotes].Scale=GetScale(atoi(ptr));
    if (Notes[NrNotes].Scale==0)
      Notes[NrNotes].Scale=DefNoteScale;

    /* Skip the number in scale specification. */
    if(isdigit(*ptr))
      ptr++;

    /* [<special-duration>] */

    if (*ptr=='.')
      Notes[NrNotes].DurationSpecifier=DottedNote;
    else Notes[NrNotes].DurationSpecifier=NoSpecialDuration;

    NrNotes++;
    ptr=strtok(NULL, ", ");
  }

  size=FB61_PackRingtone(req, Name, DefBeats, NrNotes, Notes);
  return size;
}
