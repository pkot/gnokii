/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

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

API struct CFG_Header *CFG_Info;

/* Function prototypes */

struct CFG_Header *CFG_ReadFile(const char *filename);
API char *CFG_Get(struct CFG_Header *cfg, const char *section, const char *key);
typedef void (*CFG_GetForeach_func)(const char *section, const char *key, const char *value);
void CFG_GetForeach(struct CFG_Header *cfg, const char *section, CFG_GetForeach_func func);
char *CFG_Set(struct CFG_Header *cfg, const char *section, const char *key, const char *value);
int CFG_WriteFile(struct CFG_Header *cfg, const char *filename);
API int readconfig(char **model, char **port, char **initlength, char **connection, char **bindir);

#endif /* _CFGREADER_H */
