/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Functions to read and write common file types.
 
  Last modified: Wed 6 Dec 2000 by Chris Kemp

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "gsm-common.h"
#include "gsm-filetypes.h"
#include "gsm-bitmaps.h"
#include "gsm-ringtones.h"

#ifdef XPM
  #include <X11/xpm.h>
#endif



/* Ringtone File Functions */
/* ####################### */


/* Function to convert scale field in to correct number. */

int GetDuration (char *num)
{

  int duration=0;
  
  switch (atoi(num)) {
    
  case 1:
    duration=128; break;
  case 2:
    duration=64; break;
  case 4:
    duration=32; break;
  case 8:
    duration=16; break;
  case 16:
    duration=8; break;
  case 32:
    duration=4; break;
  }
  
  return (duration);
  
}


int GetScale (char *num)
{

  /* This may well need improving. */

  int scale=0;

  if ((atoi(num))<4) scale=(atoi(num));
  if ((atoi(num))>4) scale=(atoi(num))-4;

  return (scale);
}


/* Currently only reads rttl and ott files - can be later extended to midi etc. */

GSM_Error GSM_ReadRingtoneFile(char *FileName, GSM_Ringtone *ringtone)
{
  FILE *file;
  GSM_Error error;
  GSM_Filetypes filetype;
  
  file = fopen(FileName, "rb");

  if (!file)
    return(GE_CANTOPENFILE);

  /* FIXME: for now identify the filetype based on the extension */
  /* I don't like this but I haven't got any .ott files to work out a better way */

  filetype=RTTL;
  if (strstr(FileName,".ott")) filetype=OTT; /* OTT files saved by NCDS3 */
  
  error=GE_NONE;

  rewind(file);  /* Not necessary for now but safer */

  switch (filetype) {
  case RTTL:
    error=loadrttl(file,ringtone);
    fclose(file);
    break;
  case OTT:
    error=loadott(file,ringtone);
    fclose(file);
    break;
  default:
    error=GE_INVALIDFILEFORMAT;
    break;
  }

  return(error);
}


GSM_Error loadott(FILE *file, GSM_Ringtone *ringtone)
{
  char Buffer[2000];
  int i;
  
  i=fread(Buffer, 1, 2000, file);

  if (!feof(file)) return GE_FILETOOLONG;

  return GSM_UnPackRingtone(ringtone, Buffer, i);
}


GSM_Error loadrttl(FILE *file, GSM_Ringtone *ringtone)
{
  int NrNote=0;

  int DefNoteScale=2;
  int DefNoteDuration=4;
  unsigned char buffer[2000];
  unsigned char *def, *notes, *ptr;

  fread(buffer, 2000, 1, file);


  /* This is for buggy RTTTL ringtones without name. */
  if (buffer[0] != RTTTL_SEP[0]) {
    strtok(buffer, RTTTL_SEP);
    sprintf(ringtone->name, "%s", buffer);
    def=strtok(NULL, RTTTL_SEP);
    notes=strtok(NULL, RTTTL_SEP);
  }
  else {
    sprintf(ringtone->name, "GNOKII");
    def=strtok(buffer, RTTTL_SEP);
    notes=strtok(NULL, RTTTL_SEP);
  }

  ptr=strtok(def, ", ");
  /* Parsing the <defaults> section. */
  ringtone->tempo=63;

  while (ptr) {

    switch(*ptr) {
    case 'd':
    case 'D':
      DefNoteDuration=GetDuration(ptr+2);
      break;
    case 'o':
    case 'O':
      DefNoteScale=GetScale(ptr+2);
      break;
    case 'b':
    case 'B':
      ringtone->tempo=atoi(ptr+2);
      break;
    }

    ptr=strtok(NULL,", ");
  }

#ifdef DEBUG
  printf("DefNoteDuration=%d\n", DefNoteDuration);
  printf("DefNoteScale=%d\n", DefNoteScale);
#endif

  /* Parsing the <note-command>+ section. */
  ptr=strtok(notes, ", ");
  while (ptr && NrNote<MAX_RINGTONE_NOTES) {

    /* [<duration>] */
    ringtone->notes[NrNote].duration=GetDuration(ptr);
    if (ringtone->notes[NrNote].duration==0)
      ringtone->notes[NrNote].duration=DefNoteDuration;

    /* Skip all numbers in duration specification. */
    while(isdigit(*ptr))
      ptr++;

    /* <note> */

    if ((*ptr>='a') && (*ptr<='g')) ringtone->notes[NrNote].note=((*ptr-'a')*2)+10;
    else if ((*ptr>='A') && (*ptr<='G')) ringtone->notes[NrNote].note=((*ptr-'A')*2)+10;
    else if ((*ptr=='H') || (*ptr=='h')) ringtone->notes[NrNote].note=12;
    else ringtone->notes[NrNote].note=255;

    if ((ringtone->notes[NrNote].note>13)&&(ringtone->notes[NrNote].note!=255))
	ringtone->notes[NrNote].note-=14;


    ptr++;
    
    if ((*ptr)=='#') {
      ringtone->notes[NrNote].note++;
      if ((ringtone->notes[NrNote].note==5) || (ringtone->notes[NrNote].note==13))
	ringtone->notes[NrNote].note++;
      ptr++;
    }

    /* Check for dodgy rttl */
    /* [<special-duration>] */
    if (*ptr=='.') {
      ringtone->notes[NrNote].duration*=1.5;
      ptr++;
    }


    /* [<scale>] */

    if (ringtone->notes[NrNote].note!=255) {
      if (isdigit(*ptr)) {
	ringtone->notes[NrNote].note+=GetScale(ptr)*14;
	ptr++;
      }
      else ringtone->notes[NrNote].note+=DefNoteScale*14;
    }

    /* [<special-duration>] */
    if (*ptr=='.') {
      ringtone->notes[NrNote].duration*=1.5;
      ptr++;
    }

    NrNote++;
    ptr=strtok(NULL, ", ");
  }

  ringtone->NrNotes=NrNote;

  return GE_NONE;

}


/* Save the ringtone file - this will overwrite the file */
/* Confirming must be done before this is called */

GSM_Error GSM_SaveRingtoneFile(char *FileName, GSM_Ringtone *ringtone)
{
  FILE *file; 
  GSM_Error error;

  file = fopen(FileName, "wb");
  
  if (!file)
    return(GE_CANTOPENFILE);
  
  /* FIXME... */ 
  /* We need a way of passing these functions a filetype rather than rely on the extension */

  if (strstr(FileName,".ott")) {
     error=saveott(file, ringtone);
   }
   else error=saverttl(file, ringtone);

   fclose(file);
   
   return error;
}


GSM_Error saveott(FILE *file, GSM_Ringtone *ringtone)
{
  char Buffer[2000];
  int i=2000;
  
  /* PackRingtone writes up to i chars and returns in i the number written */
  GSM_PackRingtone(ringtone, Buffer, &i);

  if (i<2000) {
    fwrite(Buffer, 1, i, file);
    return GE_NONE;
  } 
  else return GE_FILETOOLONG;

}

GSM_Error saverttl(FILE *file, GSM_Ringtone *ringtone)
{
  int DefDuration, DefScale=2, CurrentNote;
  int buffer[6];
  int i,j,k=0;
  
  /* Saves ringtone name */
  fprintf(file,_("%s:"),ringtone->name);

  /* Find the most frequently used duration and use this for the default */
 
  for (i=0;i<6;i++) buffer[i]=0;
  for (i=0;i<ringtone->NrNotes;i++) {
    switch (ringtone->notes[i].duration) {
      case 192:
	buffer[0]++; break;
      case 128: 
	buffer[0]++; break;
      case 96:	
	buffer[1]++; break;
      case 64: 
	buffer[1]++; break;
      case 48:
	buffer[2]++; break;
      case 32: 
	buffer[2]++; break;
      case 24:
	buffer[3]++; break;
      case 16: 
	buffer[3]++; break;
      case 12:
	buffer[4]++; break;
      case 8: 
	buffer[4]++; break;
      case 6: 
	buffer[5]++; break;
      case 4:
	buffer[5]++; break;
    }
  }

  /* Now find the most frequently used */
  j=0;
  for (i=0;i<6;i++) {
    if (buffer[i]>j) {
      k=i; 
      j=buffer[i];
    }
  }

  /* Finally convert and save the default duration */

  switch (k) {
      case 0:
	DefDuration=128;
	fprintf(file,_("d=1,"));
	break;	
      case 1:
	DefDuration=64;
	fprintf(file,_("d=2,"));
	break;	
      case 2:
	DefDuration=32;
	fprintf(file,_("d=4,"));
	break;	
      case 3:
	DefDuration=16;
	fprintf(file,_("d=8,"));
	break;	
      case 4:
	DefDuration=8;
	fprintf(file,_("d=16,"));
	break;	
      case 5:
	DefDuration=4;
	fprintf(file,_("d=32,"));
	break;	
     default:
	DefDuration=16;
	fprintf(file,_("d=8,"));
	break;	
  }  


  /* Find the most frequently used scale and use this for the default */

  for (i=0;i<6;i++) buffer[i]=0;
  for (i=0;i<ringtone->NrNotes;i++) {
    if (ringtone->notes[i].note!=255) {
      buffer[ringtone->notes[i].note/14]++;
    }
  }
  j=0;
  for (i=0;i<6;i++) {
    if (buffer[i]>j) {
      DefScale=i;
      j=buffer[i];
    }
  }

  /* Save the default scale and tempo */

  fprintf(file,_("o=%i,"),DefScale+4);
  fprintf(file,_("b=%i:"),ringtone->tempo);

#ifdef DEBUG
  printf("DefNoteDuration=%d\n", DefDuration);
  printf("DefNoteScale=%d\n", DefScale);
  printf("Number of notes=%d\n",ringtone->NrNotes);
#endif
  

  /* Now loop round for each note */

  for (i=0;i<ringtone->NrNotes;i++) {
    CurrentNote=ringtone->notes[i].note;

    /* This note has a duration different than the default. We must save it */
    if (ringtone->notes[i].duration!=DefDuration) {
      switch (ringtone->notes[i].duration) {
        case 192:                      //192=128*1.5
	  fprintf(file,_("1")); break;
        case 128:
	  fprintf(file,_("1")); break;
        case 96:                       //96=64*1.5
	  fprintf(file,_("2")); break;
        case 64:
	  fprintf(file,_("2")); break;
        case 48:                       //48=32*1.5
	  fprintf(file,_("4")); break;
        case 32:
	  fprintf(file,_("4")); break;
        case 24:                       //24=16*1.5;
	  fprintf(file,_("8")); break;
        case 16:
	  fprintf(file,_("8")); break;
        case 12:                       //12=8*1.5
	  fprintf(file,_("16")); break;
        case 8:
	  fprintf(file,_("16")); break;
        case 6:                        //6=4*1.5
	  fprintf(file,_("32")); break;
        case 4:
	  fprintf(file,_("32")); break;
        default: 
	  break;
      }
    }
    
    /* Now save the actual note */
    switch (GSM_GetNote(CurrentNote)) {
      case Note_C  :fprintf(file,_("c"));break;
      case Note_Cis:fprintf(file,_("c#"));break;
      case Note_D  :fprintf(file,_("d"));break;
      case Note_Dis:fprintf(file,_("d#"));break;
      case Note_E  :fprintf(file,_("e"));break;
      case Note_F  :fprintf(file,_("f"));break;
      case Note_Fis:fprintf(file,_("f#"));break;
      case Note_G  :fprintf(file,_("g"));break;
      case Note_Gis:fprintf(file,_("g#"));break;
      case Note_A  :fprintf(file,_("a"));break;
      case Note_Ais:fprintf(file,_("a#"));break;
      case Note_H  :fprintf(file,_("h"));break;
      default      :fprintf(file,_("p"));break; //Pause ?
    }

    /* Saving info about special duration */
    if (ringtone->notes[i].duration==128*1.5 ||
        ringtone->notes[i].duration==64*1.5 ||
        ringtone->notes[i].duration==32*1.5 ||
	ringtone->notes[i].duration==16*1.5 ||
        ringtone->notes[i].duration==8*1.5 ||
        ringtone->notes[i].duration==4*1.5)
      fprintf(file,_("."));
    
    /* This note has a scale different than the default, so save it */
    if ( (CurrentNote!=255) && (CurrentNote/14!=DefScale))
        fprintf(file,_("%i"),(CurrentNote/14)+4);
    
    /* And a separator before next note */
    if (i!=ringtone->NrNotes-1)
      fprintf(file,_(","));

  }

  return GE_NONE;
}


/* Bitmap file functions */
/* ##################### */


GSM_Error GSM_ReadBitmapFile(char *FileName, GSM_Bitmap *bitmap)
{

  FILE *file;
  unsigned char buffer[300];
  int error;
  GSM_Filetypes filetype=None;

  file = fopen(FileName, "rb");

  if (!file)
    return(GE_CANTOPENFILE);

  fread(buffer, 1, 9, file); /* Read the header of the file. */

  /* Attempt to identify filetype */

  if (memcmp(buffer, "NOL",3)==0) {  /* NOL files have 'NOL' at the start */
    filetype=NOL;
  } else if (memcmp(buffer, "NGG",3)==0) {  /* NGG files have 'NGG' at the start */
    filetype=NGG;
  } else if (memcmp(buffer, "FORM",4)==0) {  /* NSL files have 'FORM' at the start */
    filetype=NSL;
  } else if (memcmp(buffer, "NLM",3)==0) {  /* NLM files have 'NLM' at the start */
    filetype=NLM;
  } else if (memcmp(buffer, "BM",2)==0) {  /* BMP, I61 and GGP files have 'BM' at the start */
    filetype=BMP;
  } else if (memcmp(buffer, "/* XPM */",9)==0) {  /* XPM files have 'XPM' at the start */  
    filetype=XPMF;
  } else filetype=None;

  if (strstr(FileName,".otb")) filetype=OTA; /* OTA files saved by NCDS3 */
  
  rewind(file);

  switch (filetype) {

  case NOL:
    error=loadnol(file,bitmap);
    fclose(file);
    break;
  case NGG:
    error=loadngg(file,bitmap);
    fclose(file);
    break;
  case NSL:
    error=loadnsl(file,bitmap);
    fclose(file);
    break;
  case NLM:
    error=loadnlm(file,bitmap);
    fclose(file);
    break;
  case OTA:
    error=loadota(file,bitmap);
    fclose(file);
    break;
  case BMP:
    error=loadbmp(file,bitmap);
    fclose(file);
    break;
#ifdef XPM
  case XPMF:
    fclose(file);
    error=loadxpm(FileName,bitmap);
    break;
#endif
  default:
    error=GE_INVALIDFILEFORMAT;
  }

  return(error);
}


#ifdef XPM

GSM_Error loadxpm(char *filename, GSM_Bitmap *bitmap)
{
  int error,x,y;
  XpmImage image;
  XpmInfo info;

  error=XpmReadFileToXpmImage(filename,&image,&info);

  switch (error) {
    case XpmColorError:  return GE_WRONGCOLORS;break;
    case XpmColorFailed: return GE_WRONGCOLORS;break;
    case XpmOpenFailed:  return GE_CANTOPENFILE;break;
    case XpmFileInvalid: return GE_INVALIDFILEFORMAT;break;
    case XpmSuccess: break;
    default: break;
  }

  if (image.ncolors!=2) return GE_WRONGNUMBEROFCOLORS; 

  if ((image.height==48) && (image.width==84)) {
    bitmap->type=GSM_StartupLogo;
  }
  else if ((image.height==14) && (image.width==72)) {
    bitmap->type=GSM_CallerLogo;
  }
  else {
#ifdef DEBUG 
    fprintf(stderr,"Invalid Image Size (%dx%d).\n",image.height,image.width);
#endif
    return GE_INVALIDIMAGESIZE;
  }

  bitmap->height=image.height;
  bitmap->width=image.width;
  bitmap->size=bitmap->height*bitmap->width/8;

  GSM_ClearBitmap(bitmap);
  
  for(y=0;y<image.height;y++) {
    for(x=0;x<image.width;x++) {
      if (image.data[y*image.width+x]==0) GSM_SetPointBitmap(bitmap,x,y);        
    }
  }

  return GE_NONE;
}

#endif


/* Based on the article from the Polish Magazine "Bajtek" 11/92 */
                                     /* Marcin-Wiacek@Topnet.PL */

/* This loads the image as a startup logo - but is resized as necessary later */

GSM_Error loadbmp(FILE *file, GSM_Bitmap *bitmap)
{
  unsigned char buffer[34];
  bool first_white;
  int w,h,pos,y,x,i,sizeimage;

  bitmap->type=GSM_StartupLogo;
  bitmap->width=84;
  bitmap->height=48;
  bitmap->size=bitmap->width*bitmap->height/8;

  GSM_ClearBitmap(bitmap);
  
  fread(buffer, 1, 34, file); //required part of header

  h=buffer[22]+256*buffer[21]; //height of image in the file
  w=buffer[18]+256*buffer[17]; //width of image in the file
#ifdef DEBUG
  printf("Image Size in BMP file: %dx%d\n",w,h);
#endif

#ifdef DEBUG
  printf("Number of colors in BMP file: ");
  switch (buffer[28]) {
    case 1:printf("2 (supported by gnokii)\n");break;
    case 4:printf("16 (not supported by gnokii)\n");break;
    case 8:printf("256 (not supported by gnokii)\n");break;
    case 24:printf("True Color (not supported by gnokii)\n");break;
    default:printf("unknown\n");break;
  }
#endif
  if (buffer[28]!=1) {
    printf("Wrong number of colors\n"); //we support only 2 colors images !
    return GE_WRONGNUMBEROFCOLORS;
  }

#ifdef DEBUG
  printf("Compression in BMP file: ");
  switch (buffer[30]) {
    case 0:printf("no compression (supported by gnokii)\n");break;
    case 1:printf("RLE8 (not supported by gnokii)\n");break;
    case 2:printf("RLE4 (not supported by gnokii)\n");break;
    default:printf("unknown\n");break;
  }
#endif  
  if (buffer[30]!=0) {
#ifdef DEBUG
    printf("Subformat not supported\n"); //we don't support RLE compression
#endif
    return GE_SUBFORMATNOTSUPPORTED;
  }  
  
  pos=buffer[10]-34;
  fread(buffer, 1, pos, file); //rest of header (if exists) and color palette
  
#ifdef DEBUG
  printf("First color in BMP file: %i %i %i ",buffer[pos-8], buffer[pos-7], buffer[pos-6]);
  if (buffer[pos-8]==0 && buffer[pos-7]==0 && buffer[pos-6]==0) printf("(white)");
  if (buffer[pos-8]==0xFF && buffer[pos-7]==0xFF && buffer[pos-6]==0xFF) printf("(black)");
  if (buffer[pos-8]==102 && buffer[pos-7]==204 && buffer[pos-6]==102) printf("(green)");
  printf("\n");

  printf("Second color in BMP file: %i %i %i ",buffer[pos-4], buffer[pos-3], buffer[pos-2]);
  if (buffer[pos-4]==0 && buffer[pos-3]==0 && buffer[pos-2]==0) printf("(white)");
  if (buffer[pos-4]==0xFF && buffer[pos-3]==0xFF && buffer[pos-2]==0xFF) printf("(black)");
  printf("\n");  
#endif

  first_white=true;
  if (buffer[pos-8]!=0 || buffer[pos-7]!=0 || buffer[pos-6]!=0) first_white=false;
 
  sizeimage=0;
  pos=7;
  for (y=h-1;y>=0;y--) { //lines are written from the last to the first
    i=1;
    for (x=0;x<w;x++) {
      if (pos==7) { //new byte !
        fread(buffer, 1, 1, file);
	sizeimage++;
	i++;
	if(i==5) i=1; //each line is written in multiply of 4 bytes
      }
      if (x<=bitmap->width && y<=bitmap->height) { //we have top left corner !
        if (!first_white) {
          if ((buffer[0]&(1<<pos))<=0) GSM_SetPointBitmap(bitmap,x,y);
	} else {
	  if ((buffer[0]&(1<<pos))>0) GSM_SetPointBitmap(bitmap,x,y);
	}
      }
      pos--;
      if (pos<0) pos=7; //going to new byte
    }
    pos=7; //going to new byte
    while (i!=5) //each line is written in multiples of 4 bytes
    {
      fread(buffer, 1, 1, file);
      sizeimage++;
      i++;
    }
  }

#ifdef DEBUG
  printf("Data size in BMP file: %i\n",sizeimage);
#endif
    
  return(GE_NONE);
}

GSM_Error loadnol(FILE *file, GSM_Bitmap *bitmap)
{

  unsigned char buffer[2000];
  int i,j;

  bitmap->type=GSM_OperatorLogo;

  fread(buffer, 1, 6, file);
  fread(buffer, 1, 4, file);
  sprintf(bitmap->netcode, "%d %02d", buffer[0]+256*buffer[1], buffer[2]);

  fread(buffer, 1, 4, file); /* Width and height of the icon. */
  bitmap->width=buffer[0];
  bitmap->height=buffer[2];
  bitmap->size=bitmap->height*bitmap->width/8;

  if ((bitmap->height!=14) || (bitmap->width!=72)) {
#ifdef DEBUG
    printf("Invalid Image Size (%dx%d).\n",bitmap->width,bitmap->height);
#endif
    return GE_INVALIDIMAGESIZE;
  }

  fread(buffer, 1, 6, file); /* Unknown bytes. */

  for (i=0; i<bitmap->size; i++) {
    if (fread(buffer, 1, 8, file)==8) {
      bitmap->bitmap[i]=0;
      for (j=7; j>=0;j--)
        if (buffer[7-j] == '1')
	  bitmap->bitmap[i]|=(1<<j);
    }
    else
      return(GE_FILETOOSHORT);
  }

#ifdef DEBUG
  /* Some programs writes here fileinfo */
  if (fread(buffer, 1, 1, file)==1) {
    fprintf(stdout, _("Fileinfo: %c"),buffer[0]);
    while (fread(buffer, 1, 1, file)==1) {
      if (buffer[0]!=0x0A) fprintf(stdout,_("%c"),buffer[0]);
    }  
    fprintf(stdout, _("\n"));
  }
#endif

  return(GE_NONE);
}


GSM_Error loadngg(FILE *file, GSM_Bitmap *bitmap)
{

  unsigned char buffer[2000];
  int i,j;

  bitmap->type=GSM_CallerLogo;

  fread(buffer, 1, 6, file);
  fread(buffer, 1, 4, file); /* Width and height of the icon. */
  bitmap->width=buffer[0];
  bitmap->height=buffer[2];
  bitmap->size=bitmap->height*bitmap->width/8;

  if ((bitmap->height!=14) || (bitmap->width!=72)) {
#ifdef DEBUG
    printf("Invalid Image Size (%dx%d).\n",bitmap->width,bitmap->height);
#endif
    return GE_INVALIDIMAGESIZE;
  }

  fread(buffer, 1, 6, file); /* Unknown bytes. */

  for (i=0; i<bitmap->size; i++) {
    if (fread(buffer, 1, 8, file)==8){
      bitmap->bitmap[i]=0;
      for (j=7; j>=0;j--)
	if (buffer[7-j] == '1')
	  bitmap->bitmap[i]|=(1<<j);
    }
    else
      return(GE_FILETOOSHORT);
  }

#ifdef DEBUG
  /* Some programs writes here fileinfo */
  if (fread(buffer, 1, 1, file)==1) {
    fprintf(stdout, _("Fileinfo: %c"),buffer[0]);
    while (fread(buffer, 1, 1, file)==1) {
      if (buffer[0]!=0x0A) fprintf(stdout,_("%c"),buffer[0]);
    }  
    fprintf(stdout, _("\n"));
  }
#endif
  
  return(GE_NONE);
}



GSM_Error loadnsl(FILE *file, GSM_Bitmap *bitmap)
{

  unsigned char block[6],buffer[505];
  int block_size;

  bitmap->size=0;
  
  while (fread(block,1,6,file)==6) {

    block_size=block[4]*256+block[5];

#ifdef DEBUG
    fprintf(stdout,_("Block %c%c%c%c, size %i\n"),block[0],block[1],block[2],block[3],block_size);
#endif

    if (!strncmp(block, "FORM", 4)) {
#ifdef DEBUG
      fprintf(stdout,_("  File ID\n"));
#endif
    } else
    {
      if (block_size>504) return(GE_INVALIDFILEFORMAT);

      if (block_size!=0) {

        fread(buffer,1,block_size,file);
        buffer[block_size]=0; //if it's string, we end it with \0

#ifdef DEBUG
        if (!strncmp(block, "VERS", 4)) fprintf(stdout,_("  File saved by: %s\n"),buffer);
        if (!strncmp(block, "MODL", 4)) fprintf(stdout,_("  Logo saved from: %s\n"),buffer);
        if (!strncmp(block, "COMM", 4)) fprintf(stdout,_("  Phone was connected to COM port: %s\n"),buffer);
#endif
	
        if (!strncmp(block, "NSLD", 4)) {          
          bitmap->type=GSM_StartupLogo;
          bitmap->height=48;
          bitmap->width=84;
          bitmap->size=(bitmap->height*bitmap->width)/8;

          memcpy(bitmap->bitmap,buffer,bitmap->size);

#ifdef DEBUG
          fprintf(stdout,_("  Startup logo (size %i)\n"),block_size);
#endif
        }
      }
    }
  }
  
  if (bitmap->size==0) return(GE_FILETOOSHORT);

  return(GE_NONE);
}


GSM_Error loadnlm (FILE *file, GSM_Bitmap *bitmap)
{
  unsigned char buffer[84*48];
  int pos,pos2,x,y;
  div_t division;

  fread(buffer,1,5,file);
  fread(buffer,1,1,file);

  switch (buffer[0]) {
  case 0x00:
    bitmap->type=GSM_OperatorLogo;
    break;
  case 0x01:
    bitmap->type=GSM_CallerLogo;
    break;
  case 0x02:
    bitmap->type=GSM_StartupLogo;
    break;
  case 0x03:
    bitmap->type=GSM_PictureImage;
    break;
  default:
    return(GE_SUBFORMATNOTSUPPORTED);
  }
  
  fread(buffer,1,4,file);
  bitmap->width=buffer[1];
  bitmap->height=buffer[2];
  bitmap->size=bitmap->width*bitmap->height/8;

  division=div(bitmap->width,8);
  if (division.rem!=0) division.quot++; /* For startup logos */
  
  if (fread(buffer,1,(division.quot*bitmap->height),file)!=(division.quot*bitmap->height))
    return(GE_FILETOOSHORT);
    
  GSM_ClearBitmap(bitmap);
  
  pos=0;pos2=7;
  for (y=0;y<bitmap->height;y++) {
    for (x=0;x<bitmap->width;x++) {
      if ((buffer[pos]&(1<<pos2))>0) GSM_SetPointBitmap(bitmap,x,y);
      pos2--;
      if (pos2<0) {pos2=7;pos++;} //going to new byte
    }
    if (pos2!=7) {pos2=7;pos++;} //for startup logos-new line means new byte
  }

  return (GE_NONE);
}


GSM_Error loadota(FILE *file, GSM_Bitmap *bitmap)
{

  char buffer[4];

  /* We could check for extended info here - indicated by the 7th bit being set in the first byte */
	
  fread(buffer,1,4,file);

  bitmap->width=buffer[1];
  bitmap->height=buffer[2];
  bitmap->size=bitmap->width*bitmap->height/8;

  if ((bitmap->height==48) && (bitmap->width==84)) {
    bitmap->type=GSM_StartupLogo;
  }
  else if ((bitmap->height==14) && (bitmap->width==72)) {
    bitmap->type=GSM_CallerLogo;
  }
  else {
#ifdef DEBUG
    printf("Invalid Image Size (%dx%d).\n",bitmap->width,bitmap->height);
#endif
    return GE_INVALIDIMAGESIZE;
  }
  
  if (fread(bitmap->bitmap,1,bitmap->size,file)!=bitmap->size)
    return(GE_FILETOOSHORT);

  return(GE_NONE);
}


/* This overwrites an existing file - so this must be checked before calling */

GSM_Error GSM_SaveBitmapFile(char *FileName, GSM_Bitmap *bitmap)
{

  FILE *file;
  bool done=false;


  /* XPMs are a bit messy because we have to pass it the filename */
  
#ifdef XPM
   if (strstr(FileName,".xpm"))
   {
     savexpm(FileName, bitmap);
   } else
   {
#endif    
   
   file = fopen(FileName, "wb");
      
   if (!file)
     return(GE_CANTOPENFILE);
  	
   if (strstr(FileName,".nlm"))
   {
     savenlm(file, bitmap);
     done=true;
   }
   if (strstr(FileName,".ngg"))
   {
     savengg(file, bitmap);
     done=true;    
   }
   if (strstr(FileName,".nsl"))
   {
     savensl(file, bitmap);
     done=true;
   }
   if (strstr(FileName,".otb"))
   {
     saveota(file, bitmap);
     done=true;
   }
   if (strstr(FileName,".nol"))
   {
     savenol(file, bitmap);
     done=true;
   }
   if (strstr(FileName,".bmp") ||
       strstr(FileName,".ggp") ||
       strstr(FileName,".i61"))
   {
     savebmp(file, bitmap);
     done=true;
   }
   
   if (!done)
   {
     switch (bitmap->type) {
       case GSM_CallerLogo:
         savengg(file, bitmap);
         break;
       case GSM_OperatorLogo:
         savenol(file, bitmap);
         break;
       case GSM_StartupLogo:
         savensl(file, bitmap);
         break;
       case GSM_PictureImage:
         savenlm(file, bitmap);
         break;
       case GSM_WelcomeNoteText:
         break;
       case GSM_DealerNoteText:
         break;
       case GSM_None:
         break;
     }      
   }
  
   fclose(file);
   
#ifdef XPM
   }
#endif    
     
   return GE_NONE;
}


/* FIXME - this should not ask for confirmation here - I'm not sure what calls it though */

/* mode == 0 -> overwrite
 * mode == 1 -> ask
 * mode == 2 -> append
 */
int GSM_SaveTextFile(char *FileName, char *text, int mode)
{

  FILE *file;
  int confirm = -1;
  char ans[5];
  struct stat buf;

  /* Ask before overwriting */
  
  if ((mode == 1) && (stat(FileName, &buf) == 0)) {
    fprintf(stdout, _("File %s exists.\n"), FileName);
    while (confirm < 0) {
      fprintf(stderr, _("Overwrite? (yes/no) "));
      GetLine(stdin, ans, 4);
      if (!strcmp(ans, "yes")) confirm = 1;
      else if (!strcmp(ans, "no")) confirm = 0;
    }  
    if (!confirm) return -1;
  }

  if (mode == 2) file = fopen(FileName, "a");
  else file = fopen(FileName, "w");

  if (!file) {
    fprintf(stderr, _("Failed to write file %s\n"),  FileName);
    return(-1);
  }

  fprintf(file, "%s\n", text);

  fclose(file);

  return 2;
}


#ifdef XPM

void savexpm(char *filename, GSM_Bitmap *bitmap)
{
  XpmColor colors[2]={{".","c","#000000","#000000","#000000","#000000"},
		      {"#","c","#ffffff","#ffffff","#ffffff","#ffffff"}};
  XpmImage image;
  unsigned int data[4032];
  int x,y;

  image.height=bitmap->height;
  image.width=bitmap->width;
  image.cpp=1;
  image.ncolors=2;
  image.colorTable=colors;
  image.data=data;

  for (y=0;y<image.height;y++) {
    for (x=0;x<image.width;x++)
      if (GSM_IsPointBitmap(bitmap,x,y))
        data[y*image.width+x]=0;
      else
        data[y*image.width+x]=1;
  }
  
  XpmWriteFileFromXpmImage(filename,&image,NULL);
}

#endif


/* Based on the article from the Polish Magazine "Bajtek" 11/92 */
                                     /* Marcin-Wiacek@Topnet.PL */
void savebmp(FILE *file, GSM_Bitmap *bitmap)
{
  int x,y,pos,i,sizeimage;
  unsigned char buffer[1];
  div_t division;
  
  char header[]={
/*1'st header*/   'B','M',             /* BMP file ID */
                  0x00,0x00,0x00,0x00, /* Size of file */
		  0x00,0x00,           /* Reserved for future use */
		  0x00,0x00,           /* Reserved for future use */
	            62,0x00,0x00,0x00, /* Offset for image data */
		 
/*2'nd header*/     40,0x00,0x00,0x00, /* Length of this part of header */
		  0x00,0x00,0x00,0x00, /* Width of image */
		  0x00,0x00,0x00,0x00, /* Height of image */		 
		     1,0x00,           /* How many planes in target device */
		     1,0x00,           /* How many colors in image. 1 means 2^1=2 colors */
		  0x00,0x00,0x00,0x00, /* Type of compression. 0 means no compression */
/*Sometimes */    0x00,0x00,0x00,0x00, /* Size of part with image data */
/*ttttttt...*/    0xE8,0x03,0x00,0x00, /* XPelsPerMeter */
/*hhiiiiissss*/   0xE8,0x03,0x00,0x00, /* YPelsPerMeter */		  
/*part of header*/2,0x00,0x00,0x00, /* How many colors from palette is used */
/*doesn't exist*/ 0x00,0x00,0x00,0x00, /* How many colors from palette is required to display image. 0 means all */
		 
/*Color palette*/ 0x00,0x00,0x00,      /* First color in palette in Blue, Green, Red. Here white */
		  0x00,                /* Each color in palette is end by 4'th byte */
                  0xFF,0xFF,0xFF,      /* Second color in palette in Blue, Green, Red. Here black */
		  0x00};               /* Each color in palette is end by 4'th byte */

  header[22]=bitmap->height;
  header[18]=bitmap->width;
  
  pos=7;
  sizeimage=0;
  for (y=bitmap->height-1;y>=0;y--) { //lines are written from the last to the first
    i=1;
    for (x=0;x<bitmap->width;x++) {
      if (pos==7) { //new byte !
        sizeimage++;
	i++;
	if(i==5) i=1; //each line is written in multiply of 4 bytes
      }
      pos--;
      if (pos<0) pos=7; //going to new byte
    }
    pos=7; //going to new byte
    while (i!=5) //each line is written in multiply of 4 bytes
    {
      sizeimage++;
      i++;
    }
  }
#ifdef DEBUG
  printf("Data size in BMP file: %i\n",sizeimage);
#endif
  division=div(sizeimage,256);
  header[35]=division.quot;
  header[34]=sizeimage-(division.quot*256);
  
  sizeimage=sizeimage+sizeof(header);
#ifdef DEBUG
  printf("Size of BMP file: %i\n",sizeimage);
#endif
  division=div(sizeimage,256);
  header[3]=division.quot;
  header[2]=sizeimage-(division.quot*256);
       
  fwrite(header,1,sizeof(header),file);

  pos=7;
  for (y=bitmap->height-1;y>=0;y--) { //lines are written from the last to the first
    i=1;
    for (x=0;x<bitmap->width;x++) {
      if (pos==7) { //new byte !
        if (x!=0) fwrite(buffer, 1, sizeof(buffer), file);
	i++;
	if(i==5) i=1; //each line is written in multiply of 4 bytes
	buffer[0]=0;
      }
      if (GSM_IsPointBitmap(bitmap,x,y)) buffer[0]|=(1<<pos);
      pos--;
      if (pos<0) pos=7; //going to new byte
    }
    pos=7; //going to new byte
    fwrite(buffer, 1, sizeof(buffer), file);
    while (i!=5) //each line is written in multiply of 4 bytes
    {
      buffer[0]=0;
      fwrite(buffer, 1, sizeof(buffer), file);
      i++;
    }
  }
}

void savengg(FILE *file, GSM_Bitmap *bitmap)
{

  char header[]={'N','G','G',0x00,0x01,0x00,
                 0x00,0x00,           /* Width */
		 0x00,0x00,           /* Height */
		 0x01,0x00,0x01,0x00,
		 0x00,                /* Unknown.Can't be checksum - for */
		                      /* the same logo files can be different */
		 0x00}; 

  char buffer[8];
  int i,j;

  GSM_ResizeBitmap(bitmap,GSM_CallerLogo);

  header[6]=bitmap->width;
  header[8]=bitmap->height;

  fwrite(header,1,sizeof(header),file);

  for (i=0; i<bitmap->size; i++) {
    for (j=7; j>=0;j--)
      if ((bitmap->bitmap[i]&(1<<j))>0) {
	buffer[7-j] = '1';
      } else {
	buffer[7-j] = '0';
      }
    fwrite(buffer,1,8,file);
  }
}
  
void savenol(FILE *file, GSM_Bitmap *bitmap)
{

  char header[]={'N','O','L',0x00,0x01,0x00,
                 0x00,0x00,           /* MCC */
		 0x00,0x00,           /* MNC */
		 0x00,0x00,           /* Width */
		 0x00,0x00,           /* Height */
		 0x01,0x00,0x01,0x00,
		 0x00,                /* Unknown.Can't be checksum - for */
		                      /* the same logo files can be different */
		 0x00};
  char buffer[8];
  int i,j,country,net;

  GSM_ResizeBitmap(bitmap,GSM_OperatorLogo);

  sscanf(bitmap->netcode, "%d %d", &country, &net);

  header[6]=country%256;
  header[7]=country/256;
  header[8]=net%256;
  header[9]=net/256;
  header[10]=bitmap->width;
  header[12]=bitmap->height;

  fwrite(header,1,sizeof(header),file);
  
  for (i=0; i<bitmap->size; i++) {
    for (j=7; j>=0;j--)
      if ((bitmap->bitmap[i]&(1<<j))>0) {
	buffer[7-j] = '1';
      } else {
	buffer[7-j] = '0';
      }
    fwrite(buffer,1,8,file);
  }
}


void savensl(FILE *file, GSM_Bitmap *bitmap)
{

  u8 header[]={'F','O','R','M', 0x01,0xFE,  /* File ID block,      size 1*256+0xFE=510*/
	       'N','S','L','D', 0x01,0xF8}; /* Startup Logo block, size 1*256+0xF8=504*/

  GSM_ResizeBitmap(bitmap,GSM_StartupLogo);

  fwrite(header,1,sizeof(header),file);

  fwrite(bitmap->bitmap,1,bitmap->size,file);
}


void saveota(FILE *file, GSM_Bitmap *bitmap)
{

  char header[]={0x01,
                 0x00, /* Width */
		 0x00, /* Height */
		 0x01};

  header[1]=bitmap->width;
  header[2]=bitmap->height;
    
  fwrite(header,1,sizeof(header),file);

  fwrite(bitmap->bitmap,1,bitmap->size,file);
}

void savenlm(FILE *file, GSM_Bitmap *bitmap)
{

  char header[]={'N','L','M', /* Nokia Logo Manager file ID. */
                 0x20,
                 0x01,
                 0x00,        /* 0x00 (OP), 0x01 (CLI), 0x02 (Startup), 0x03 (Picture)*/
                 0x00,
                 0x00,        /* Width. */
                 0x00,        /* Height. */
                 0x01};
		 
  unsigned char buffer[11*48];
  int x,y,pos,pos2;
  div_t division;

  switch (bitmap->type) {
  case GSM_OperatorLogo:
    header[5]=0x00;
    break;
  case GSM_CallerLogo:
    header[5]=0x01;
    break;
  case GSM_StartupLogo:
    header[5]=0x02;
    break;
  case GSM_PictureImage:
    header[5]=0x03;
    break;
  case GSM_WelcomeNoteText:
    break;
  case GSM_DealerNoteText:
    break;
  case GSM_None:
    break;
  }
  
  header[7]=bitmap->width;
  header[8]=bitmap->height;
  
  pos=0;pos2=7;
  for (y=0;y<bitmap->height;y++) {
    for (x=0;x<bitmap->width;x++) {
      if (pos2==7) buffer[pos]=0;
      
      if (GSM_IsPointBitmap(bitmap,x,y)) buffer[pos]|=(1<<pos2);
      
      pos2--;
      if (pos2<0) {pos2=7;pos++;} //going to new line
    }
    if (pos2!=7) {pos2=7;pos++;} //for startup logos - new line with new byte
  }
  
  division=div(bitmap->width,8);
  if (division.rem!=0) division.quot++; /* For startup logos */
  
  fwrite(header,1,sizeof(header),file);

  fwrite(buffer,1,(division.quot*bitmap->height),file);
}


