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

#ifndef _gnokii_cfgreader_h
#define _gnokii_cfgreader_h

#include "config.h"

/* Structure definitions */

/*
 * Exported things
 */

/* A linked list of key/value pairs */
struct gn_cfg_entry {
	struct gn_cfg_entry *next, *prev;
	char *key;
char *value;
};

struct gn_cfg_header {
	struct gn_cfg_header *next, *prev;
	struct gn_cfg_entry *entries;
	char *section;
};

/* Global variables */
extern API struct gn_cfg_header *gn_cfg_info;

/* Functions */
API char *gn_cfg_get(struct gn_cfg_header *cfg, const char *section, const char *key);
API int gn_cfg_readconfig(char **model, char **port, char **initlength, char **connection, char **bindir);

/*
 * Exported things
 */

struct gn_cfg_header *cfg_read_file(const char *filename);
typedef void (*cfg_get_foreach_func)(const char *section, const char *key, const char *value);
void cfg_get_foreach(struct gn_cfg_header *cfg, const char *section, cfg_get_foreach_func func);
char *cfg_set(struct gn_cfg_header *cfg, const char *section, const char *key, const char *value);
int cfg_write_file(struct gn_cfg_header *cfg, const char *filename);

#endif /* _gnokii_cfgreader_h */
