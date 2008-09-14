/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

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

  Copyright (C) 2008 Pawel Kot

  Header file for operations on map data structure (key, value pairs).

*/

#ifndef __map_h_
#define __map_h_

#include <time.h>

struct map {
	char *key;
	void *data;
	time_t timestamp;

	struct map *next;
	struct map *prev;
};

void map_free(struct map **map);
int map_add(struct map **map, char *key, void *data);
void *map_get(struct map **map, char *key, time_t timeout);
int map_del(struct map **map, char *key);

#endif /* __map_h_ */
