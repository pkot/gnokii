/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml. 

  Released under the terms of the GNU GPL, see file COPYING for more details.
	
  Functions to read and write common file types.
 
  Last modified: Mon Mar 20 22:02:15 CET 2000
  Modified by Chris Kemp

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gsm-common.h"
#include "gsm-filetypes.h"

#ifdef XPM
  #include <X11/xpm.h>
#endif

int GSM_ReadBitmapFile(char *FileName, GSM_Bitmap *bitmap)
{

  FILE *file;
  unsigned char buffer[300];
  int error;
  GSM_Filetypes filetype=None;

  file = fopen(FileName, "r");

  if (!file)
    return(-1);

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
  } else if (memcmp(buffer, "/* XPM */",9)==0) {  /* XPM files have 'XPM' at the start */  
    filetype=XPMF;
  } else filetype=None;

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
#ifdef XPM
  case XPMF:
    fclose(file);
    error=loadxpm(FileName,bitmap);
    break;
#endif
  default:
    error=-1;
  }

  return(error);
}

#ifdef XPM

int loadxpm(char *filename, GSM_Bitmap *bitmap)
{
  int error,i,j;
  XpmImage image;
  XpmInfo info;

  error=XpmReadFileToXpmImage(filename,&image,&info);

  if (error!=0) return error;

  if (image.ncolors!=2) {
    printf("Wrong number of colors\n");
    return -1;
  }

  if ((image.height==48) && (image.width==84)) {
    bitmap->type=GSM_StartupLogo;
  }
  else if ((image.height==14) && (image.width==72)) {
    bitmap->type=GSM_CallerLogo;
  }
  else {
    printf("Invalid Image Size (%dx%d).\n",image.height,image.width);
    return -1;
  }

  bitmap->height=image.height;
  bitmap->width=image.width;
  bitmap->size=bitmap->height*bitmap->width/8;

  if (bitmap->type==GSM_StartupLogo)
    {
      for(i=0;i<bitmap->height;i++) {
	for(j=0;j<bitmap->width;j++) {
	  if (image.data[i*bitmap->width+j]==1) 
	    bitmap->bitmap[((i/8)*84) + j ]&= 255 - (1<<((i%8)));
	  else bitmap->bitmap[((i/8)*84) + j]|=1<<((i%8));
	}
      }
    }
  else {
    for(i=0;i<bitmap->height;i++) {
	for(j=0;j<bitmap->width;j++) {
	  if (image.data[i*bitmap->width+j]==1) 
	    bitmap->bitmap[9*i + (j/8)]&= 255 - (1<<(7-(j%8)));
	  else bitmap->bitmap[9*i + (j/8)]|=1<<(7-(j%8));
	}
      }
  }
  return error;
}

#endif

int loadnol(FILE *file, GSM_Bitmap *bitmap)
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

  fread(buffer, 1, 6, file); /* Unknown bytes. */

  for (i=0; i<bitmap->size; i++) {
    if (fread(buffer, 1, 8, file)==8) {
      bitmap->bitmap[i]=0;
      for (j=7; j>=0;j--)
        if (buffer[7-j] == '1')
	  bitmap->bitmap[i]|=(1<<j);
    }
    else
      return(-1);
  }

  return(0);
}

int loadngg(FILE *file, GSM_Bitmap *bitmap)
{

  unsigned char buffer[2000];
  int i,j;

  bitmap->type=GSM_CallerLogo;

  fread(buffer, 1, 6, file);
  fread(buffer, 1, 4, file); /* Width and height of the icon. */
  bitmap->width=buffer[0];
  bitmap->height=buffer[2];
  bitmap->size=bitmap->height*bitmap->width/8;

  fread(buffer, 1, 6, file); /* Unknown bytes. */

  for (i=0; i<bitmap->size; i++) {
    if (fread(buffer, 1, 8, file)==8){
      bitmap->bitmap[i]=0;
      for (j=7; j>=0;j--)
	if (buffer[7-j] == '1')
	  bitmap->bitmap[i]|=(1<<j);
    }
    else
      return(-1);
  }

  return(0);
}

int loadnsl(FILE *file, GSM_Bitmap *bitmap)
{

  unsigned char buffer[504];
  int i;
  
  do {    /* Search for 'NSL' */
    buffer[2]=buffer[1];
    buffer[1]=buffer[0];
    i=fread(buffer, 1, 1, file); 
  } while ((memcmp(buffer, "LSN", 3)!=0)&&(i==1));

  if (i!=1) {       /* Obviously not!! */
    fclose(file);
    return(-1);
  }

  fread(buffer, 1, 3, file);

  if (fread(buffer,1,504,file)==504) {

    /* No obvious height / width information :-( */

    bitmap->type=GSM_StartupLogo;
    bitmap->height=48;
    bitmap->width=84;
    bitmap->size=(bitmap->height*bitmap->width)/8;

    memcpy(bitmap->bitmap,buffer,bitmap->size); 
  }
  else
    return(-1);

  return(0);
}

int loadnlm (FILE *file, GSM_Bitmap *bitmap)
{

  unsigned char buffer[4];

  fread(buffer,1,4,file);
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
  default:
    return(-1);
  }

  return (loadota(file,bitmap));
}

int loadota(FILE *file, GSM_Bitmap *bitmap)
{

  char buffer[4];
  do {
    fread(buffer,1,1,file);
  } while ((buffer[0]&0x40)==0x40);  /* 7th bit indicates extd. info */
  fread(buffer,1,1,file); /* It seems to need this */

  fread(buffer,1,3,file);
  bitmap->width=buffer[0];
  bitmap->height=buffer[1];
  bitmap->size=bitmap->width*bitmap->height/8;
  if (fread(bitmap->bitmap,1,bitmap->size,file)!=bitmap->size)
    return(-1);

  return(0);
}

int GSM_SaveBitmapFile(char *FileName, GSM_Bitmap *bitmap)
{

  FILE *file;

#ifdef XPM
  /* Does the filename contain  .xpm ? */

  if (strstr(FileName,".xpm")) savexpm(FileName, bitmap);
  else {
#endif    

    file = fopen(FileName, "w");
    
    if (!file)
      return(-1);
    
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
    case GSM_None:
      break;
    }
    
    fclose(file);
    
#ifdef XPM
  }
#endif

  return 0;
}


#ifdef XPM

void savexpm(char *filename, GSM_Bitmap *bitmap)
{
  XpmColor colors[2]={{".","c","#000000","#000000","#000000","#000000"},
		      {"#","c","#ffffff","#ffffff","#ffffff","#ffffff"}};
  XpmImage image;
  unsigned int data[4032];
  int i,j;

  image.height=bitmap->height;
  image.width=bitmap->width;
  image.cpp=1;
  image.ncolors=2;
  image.colorTable=colors;
  image.data=data;

  if (bitmap->type==GSM_StartupLogo)
    {
      for(i=0;i<bitmap->height;i++) {
	for(j=0;j<bitmap->width;j++) {
	  if (bitmap->bitmap[((i/8)*84) + j] & 1<<((i%8))) data[i*bitmap->width+j]=0;
	  else data[i*bitmap->width+j]=1;
	}
      }
    }
  else {
    for(i=0;i<bitmap->height;i++) {
	for(j=0;j<bitmap->width;j++) {
	  if (bitmap->bitmap[9*i + (j/8)] & 1<<(7-(j%8))) data[i*bitmap->width+j]=0;
	  else data[i*bitmap->width+j]=1;
	}
      }
  } 
 
  XpmWriteFileFromXpmImage(filename,&image,NULL);

}


#endif

void savengg(FILE *file, GSM_Bitmap *bitmap)
{

  char header[]={'N','G','G',0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x4A,0x00};  /* 4A may be a checksum .... */
  char buffer[8];
  int i,j;

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

  char header[]={'N','O','L',0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x53,0x00};
  char buffer[8];
  int i,j,country,net;

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

  u8 header[]={'F','O','R','M',0x02,0x3E,'N','S','L',0x44,0x01,0xF8};

  /* This filetype is not self evident and these files are unlikely to work in
     another app. */

  fwrite(header,1,sizeof(header),file);

  fwrite(bitmap->bitmap,1,bitmap->size,file);
}

void savenlm(FILE *file, GSM_Bitmap *bitmap)
{

  char header[]={'N','L','M', /* Nokia Logo Manager file ID. */
                 0x20,
                 0x01,
                 0x00,        /* 0x00 (OP), 0x01 (CLI), 0x02 (Startup)*/
                 0x00,
                 0x00,        /* Width. */
                 0x00,        /* Height. */
                 0x01};

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
  case GSM_None:
    break;
  }
  
  header[7]=bitmap->width;
  header[8]=bitmap->height;
  
  fwrite(header,1,sizeof(header),file);

  fwrite(bitmap->bitmap,1,bitmap->size,file);
}
