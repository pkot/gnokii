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

/* Structure definitions */

/* A linked list of key/value pairs of the configuration file */
struct gn_cfg_entry {
	struct gn_cfg_entry *next, *prev;
	char *key;
	char *value;
};

/* A linked list of the config file sections together with key included */
struct gn_cfg_header {
	struct gn_cfg_header *next, *prev;
	struct gn_cfg_entry *entries;
	char *section;
};

#endif /* _gnokii_cfgreader_h */
