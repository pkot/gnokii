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
   or sent via sms to another phone.
   Function returns number of packed notes and changes maxlength to
   number of used chars in "package" */

u8 FB61_PackRingtone(GSM_Ringtone *ringtone, char *package, int *maxlength)
{
  int StartBit=0;
  int i;
  unsigned char CommandLength = 0x02;
  unsigned char spec;
  int oldscale=10, newscale;
  int HowMany=0, HowLong=0, StartNote=0, EndNote=0;

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

  /* Info about song pattern */
  StartBit=FB61_BitPackByte(package, StartBit, 0x01, 8); /* One song pattern */
  StartBit=FB61_BitPackByte(package, StartBit, PatternHeaderId, 3);
  StartBit=FB61_BitPackByte(package, StartBit, A_part, 2);
  StartBit=FB61_BitPackByte(package, StartBit, 0, 4); /* No loop value */

  /* Info, how long is contents for SMS */
  HowLong=30+8*strlen(ringtone->name)+17+8+8+13;
  
  /* Calculate the number of instructions in the tune.
     Each Note contains Note and (sometimes) Scale.
     Default Tempo and Style are instructions too. */
  HowMany=2; /* Default Tempo and Style */

  for(i=0; i<ringtone->NrNotes; i++) {

    /* PC Composer 2.0.010 doesn't like, when we start ringtone from pause:
       it displays that the format is invalid and
       hangs, when you move mouse over place, where pause is */       
    if (FB61_GetNote(ringtone->notes[i].note)==Note_Pause && oldscale==10) {
      StartNote++;
    } else {
      
      /* we don't write Scale info before "Pause" note - it saves space */
      if (FB61_GetNote(ringtone->notes[i].note)!=Note_Pause &&
          oldscale!=(newscale=FB61_GetScale(ringtone->notes[i].note))) {

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
      if ((EndNote-StartNote)==FB61_MAX_RINGTONE_NOTES-1) break;
    }
  }

  StartBit=FB61_BitPackByte(package, StartBit, HowMany, 8);

  /* Style */
  StartBit=FB61_BitPackByte(package, StartBit, StyleInstructionId, 3);
  StartBit=FB61_BitPackByte(package, StartBit, ContinuousStyle, 2);

  /* Beats per minute/tempo of the tune */
  StartBit=FB61_BitPackByte(package, StartBit, TempoInstructionId, 3);
  StartBit=FB61_BitPackByte(package, StartBit, FB61_GetTempo(ringtone->tempo), 5);

  /* Default scale */
  oldscale=10;

  /* Notes packing */
  for(i=StartNote; i<(EndNote+StartNote); i++) {
    
    /* we don't write Scale info before "Pause" note - it saves place */
    if (FB61_GetNote(ringtone->notes[i].note)!=Note_Pause &&
        oldscale!=(newscale=FB61_GetScale(ringtone->notes[i].note))) {
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
  
#ifdef DEBUG
  if (StartBit!=FB61_OctetAlignNumber(HowLong))
    fprintf(stdout,_("Error in PackRingtone - StartBit different to HowLong %d - %d)\n"),StartBit,FB61_OctetAlignNumber(HowLong));
#endif

  *maxlength=StartBit/8;  

  return(EndNote+StartNote);
}


int FB61_BitUnPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
  int i;

  for (i=0; i<Bits; i++)
    if (GetBit(Dest, CurrentBit+i)) {
      SetBit(Source, i);
    } else {
      ClearBit(Source, i);
    }

  return CurrentBit+Bits;
}

int FB61_BitUnPackInt(unsigned char *Src, int CurrentBit, int *integer, int Bits) {

  int l=0,z=128,i;

  for (i=0; i<Bits; i++) {
    if (GetBit(Src, CurrentBit+i)) l=l+z;
    z=z/2;
  }

  *integer=l;
  
  return CurrentBit+i;
}

int FB61_OctetUnAlign(int CurrentBit)
{
  int i=0;

  while((CurrentBit+i)%8) i++;

  return CurrentBit+i;
}


/* TODO: better checking, if contents of ringtone is OK */

GSM_Error FB61_UnPackRingtone(GSM_Ringtone *ringtone, char *package, int maxlength)
{
  int StartBit=0;
  int spec,duration,scale;
  int HowMany;
  int l,q,i;

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,8);
#ifdef DEBUG
  if (l!=0x02)
    fprintf(stdout,_("Not header\n"));  
#endif
  if (l!=0x02) return GE_SUBFORMATNOTSUPPORTED;

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,7);    
#ifdef DEBUG
  if (l!=RingingToneProgramming)
    fprintf(stdout,_("Not RingingToneProgramming\n"));  
#endif
  if (l!=RingingToneProgramming) return GE_SUBFORMATNOTSUPPORTED;
    
  /* The page 3-23 of the specs says that <command-part> is always
     octet-aligned. */
  StartBit=FB61_OctetUnAlign(StartBit);

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,7);    
#ifdef DEBUG
  if (l!=Sound)
    fprintf(stdout,_("Not Sound\n"));  
#endif
  if (l!=Sound) return GE_SUBFORMATNOTSUPPORTED;

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,3);    
#ifdef DEBUG
  if (l!=BasicSongType)
    fprintf(stdout,_("Not BasicSongType\n"));  
#endif
  if (l!=BasicSongType) return GE_SUBFORMATNOTSUPPORTED;

  /* Getting length of the tune name */
  StartBit=FB61_BitUnPackInt(package,StartBit,&l,4);
  l=l>>4;

  /* Unpacking the name of the tune. */
  StartBit=FB61_BitUnPack(package, StartBit, ringtone->name, 8*l);
  ringtone->name[l]=0;

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,8);    
  if (l!=1) return GE_SUBFORMATNOTSUPPORTED; //we support only one song pattern

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,3);          
#ifdef DEBUG
  if (l!=PatternHeaderId)
    fprintf(stdout,_("Not PatternHeaderId\n"));
#endif
  if (l!=PatternHeaderId) return GE_SUBFORMATNOTSUPPORTED;

  StartBit+=2; //Pattern ID - we ignore it

  StartBit=FB61_BitUnPackInt(package,StartBit,&l,4);          
    
  HowMany=0;
  StartBit=FB61_BitUnPackInt(package, StartBit, &HowMany, 8);

  scale=0;
  ringtone->NrNotes=0;
    
  for (i=0;i<HowMany;i++) {

    StartBit=FB61_BitUnPackInt(package,StartBit,&q,3);              
    switch (q) {
      case VolumeInstructionId:
        StartBit+=4;
        break;
      case StyleInstructionId:
        StartBit=FB61_BitUnPackInt(package,StartBit,&l,2);              
        l=l>>3;
	break;
      case TempoInstructionId:
        StartBit=FB61_BitUnPackInt(package,StartBit,&l,5);              	        l=l>>3;
        ringtone->tempo=BeatsPerMinute[l];
        break;
      case ScaleInstructionId:
        StartBit=FB61_BitUnPackInt(package,StartBit,&scale,2);
	scale=scale>>6;
	break;
      case NoteInstructionId:
        StartBit=FB61_BitUnPackInt(package,StartBit,&l,4);    

        switch (l) {
          case Note_C  :ringtone->notes[ringtone->NrNotes].note=0;break;
          case Note_Cis:ringtone->notes[ringtone->NrNotes].note=1;break;
          case Note_D  :ringtone->notes[ringtone->NrNotes].note=2;break;
          case Note_Dis:ringtone->notes[ringtone->NrNotes].note=3;break;
          case Note_E  :ringtone->notes[ringtone->NrNotes].note=4;break;
          case Note_F  :ringtone->notes[ringtone->NrNotes].note=6;break;
          case Note_Fis:ringtone->notes[ringtone->NrNotes].note=7;break;
          case Note_G  :ringtone->notes[ringtone->NrNotes].note=8;break;
          case Note_Gis:ringtone->notes[ringtone->NrNotes].note=9;break;
          case Note_A  :ringtone->notes[ringtone->NrNotes].note=10;break;
          case Note_Ais:ringtone->notes[ringtone->NrNotes].note=11;break;
          case Note_H  :ringtone->notes[ringtone->NrNotes].note=12;break;
          default      :ringtone->notes[ringtone->NrNotes].note=255;break; //Pause ?
        }
      
        if (ringtone->notes[ringtone->NrNotes].note!=255)
          ringtone->notes[ringtone->NrNotes].note=ringtone->notes[ringtone->NrNotes].note+scale*14;

        StartBit=FB61_BitUnPackInt(package,StartBit,&duration,3);    

        StartBit=FB61_BitUnPackInt(package,StartBit,&spec,2);    

        if (duration==Duration_Full && spec==DottedNote)
            ringtone->notes[ringtone->NrNotes].duration=128*3/2;
        if (duration==Duration_Full && spec==Length_2_3)
            ringtone->notes[ringtone->NrNotes].duration=128*2/3;
        if (duration==Duration_Full && spec==NoSpecialDuration)
            ringtone->notes[ringtone->NrNotes].duration=128;
        if (duration==Duration_1_2 && spec==DottedNote)
            ringtone->notes[ringtone->NrNotes].duration=64*3/2;
        if (duration==Duration_1_2 && spec==Length_2_3)
            ringtone->notes[ringtone->NrNotes].duration=64*2/3;
        if (duration==Duration_1_2 && spec==NoSpecialDuration)
            ringtone->notes[ringtone->NrNotes].duration=64;
        if (duration==Duration_1_4 && spec==DottedNote)
            ringtone->notes[ringtone->NrNotes].duration=32*3/2;
        if (duration==Duration_1_4 && spec==Length_2_3)
            ringtone->notes[ringtone->NrNotes].duration=32*2/3;
        if (duration==Duration_1_4 && spec==NoSpecialDuration)
            ringtone->notes[ringtone->NrNotes].duration=32;
        if (duration==Duration_1_8 && spec==DottedNote)
            ringtone->notes[ringtone->NrNotes].duration=16*3/2;
        if (duration==Duration_1_8 && spec==Length_2_3)
            ringtone->notes[ringtone->NrNotes].duration=16*2/3;
        if (duration==Duration_1_8 && spec==NoSpecialDuration)
            ringtone->notes[ringtone->NrNotes].duration=16;
        if (duration==Duration_1_16 && spec==DottedNote)
            ringtone->notes[ringtone->NrNotes].duration=8*3/2;
        if (duration==Duration_1_16 && spec==Length_2_3)
            ringtone->notes[ringtone->NrNotes].duration=8*2/3;
        if (duration==Duration_1_16 && spec==NoSpecialDuration)
            ringtone->notes[ringtone->NrNotes].duration=8;
        if (duration==Duration_1_32 && spec==DottedNote)
            ringtone->notes[ringtone->NrNotes].duration=4*3/2;
        if (duration==Duration_1_32 && spec==Length_2_3)
            ringtone->notes[ringtone->NrNotes].duration=4*2/3;
        if (duration==Duration_1_32 && spec==NoSpecialDuration)
            ringtone->notes[ringtone->NrNotes].duration=4;

        if (ringtone->NrNotes==MAX_RINGTONE_NOTES) break;
	
        ringtone->NrNotes++;
        break;
      default:
#ifdef DEBUG
    fprintf(stdout,_("Unsupported block\n"));  
#endif
        return GE_SUBFORMATNOTSUPPORTED;
    } 
  }

  return GE_NONE;
}

