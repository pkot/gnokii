/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

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
