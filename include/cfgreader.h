/*

  $Id$
  
  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for config file reader.

*/

#ifndef _CFGREADER_H
#define _CFGREADER_H

#include "config.h"

/* Structure definitions */

/* A linked list of key/value pairs */

struct CFG_Entry {
        struct CFG_Entry *next, *prev;
        char *key;
        char *value;
};

struct CFG_Header {
        struct CFG_Header *next, *prev;
        struct CFG_Entry *entries;
        char *section;
};

/* Global variables */

extern struct CFG_Header *CFG_Info;

/* Function prototypes */

struct CFG_Header *CFG_ReadFile(const char *filename);
char              *CFG_Get(struct CFG_Header *cfg, const char *section, const char *key);
typedef void (*CFG_GetForeach_func)(const char *section, const char *key, const char *value);
void               CFG_GetForeach(struct CFG_Header *cfg, const char *section, CFG_GetForeach_func func);
char              *CFG_Set(struct CFG_Header *cfg, const char *section, const char *key, const char *value);
int                CFG_WriteFile(struct CFG_Header *cfg, const char *filename);
int                readconfig(char **model, char **port, char **initlength, char **connection, char **bindir);

#endif /* _CFGREADER_H */
