/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Tue Jul  4 23:26:47 CEST 2000
  Modified by Pawe³ Kot <pkot@linuxnews.pl>

*/

#include <string.h>
#include "misc.h"

#ifndef HAVE_TIMEOPS

/* FIXME: I have timersub defined in sys/time.h :-( PJ
   FIXME: Jano wants this function too... PJ

int timersub(struct timeval *a, struct timeval *b, struct timeval *result) {
  do {
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;
    if ((result)->tv_usec < 0) {
      --(result)->tv_sec;
      (result)->tv_usec += 1000000;
    }
  } while (0);
}
*/

#endif

int GetLine(FILE *File, char *Line, int count) {

  char *ptr;

  if (fgets(Line, count, File)) {
    ptr=Line+strlen(Line)-1;

    while ( (*ptr == '\n' || *ptr == '\r') && ptr>=Line)
      *ptr--='\0';

      return strlen(Line);
  }
  else
    return 0;
}

/* jano: Functions for detecting phone capabiltes. For explanation how  */
/* to use these functions look into xgnokii_lowlevel.c                  */

static PhoneModel models[] = {
  {"1611",  "NHE-5"},
  {"2110i", "NHE-4"},
  {"2148i", "NHK-4"},
  {"8810",  "NSE-6"},
  {"8110i", "NHE-6"},
  {"3110",  "0310" /*"NHE-8"*/ },
  {"3210",  "NSE-8"},
  {"3810",  "NHE-9"},
  {"5110",  "NSE-1"},
  {"5130",  "NSK-1"},
  {"5160",  "NSW-1"},
  {"5190",  "NSB-1"},
  {"6110",  "NSE-3"},
  {"6120",  "NSC-3"},
  {"6130",  "NSK-3"},
  {"6150",  "NSM-1"},
  {"616x",  "NSW-3"},
  {"6185",  "NSD-3"},
  {"6190",  "NSB-3"},
  {"7110",  "NSE-5"},
  {"9000i", "RAE-4"},
  {"9110",  "RAE-2"},
  {"550",   "THF-10"},
  {"540",   "THF-11"},
  {"650",   "THF-12"},
  {"640",   "THF-13"},
  {NULL,    NULL}
};

char *GetModel (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
      return (models[i].model);
    i++;
  }

  return (NULL);
}


int CallerGroupSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 11 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int CalendarSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 11 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int NetmonitorSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 7 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int KeyboardSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 7 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int SMSSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 2 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int DTMFSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 2 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int DataSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if ((i > 2 && i < 6) || (i > 6 && i < 20))
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}


int SpeedDialSupported (const char *num)
{
  register int i = 0;

  while (models[i].number != 0)
  {
    if (strcmp (num, models[i].number) == 0)
    {
      if (i > 7 && i < 19)
        return (1);
      else
        return (0);
    }
    i++;
  }

  return (0);
}
