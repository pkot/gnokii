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

  Operations on map data structure (key, value pairs).

*/

#include "config.h"
#include "compat.h"
#include "map.h"
#include "misc.h"
#include "gnokii.h"

#include <string.h>
#include <stdlib.h>

/*
 * @map_free
 * Frees all memory allocated for the map data structure.
 */
void map_free(struct map **map)
{
	struct map *tmp = *map;

	while (tmp) {
		free(tmp->key);
		free(tmp->data);
		tmp = tmp->next;
		free(*map);
		*map = tmp;
	}
}

/*
 * @map_add
 * Adds (key, data) pair to the map data structure.
 * key and data need to be in allocatable memory. On free and del
 * operations, we'll try to free them.
 * map pointer gets modified with this operation.
 * retval:
 * 	0: OK
 *	-1: invalid input data (key or data are NULL)
 *	-2: duplicated key
 *	-3: allocation failed
 */
int map_add(struct map **map, char *key, void *data)
{
	struct map *tmp;
	int retval = 0;

	if (!key || !data) {
		retval = -1;
		goto out;
	}

	dprintf("Adding key %s to map %p.\n", key, *map);

	/* Don't allow duplicated keys */
	tmp = *map;
	while (tmp) {
		if (!strcmp(key, tmp->key)) {
			retval = -2;
			goto out;
		}
		tmp = tmp->next;
	}

	/* Prepare new entry */
	tmp = calloc(1, sizeof(struct map));
	/* Allocation failed */
	if (!tmp) {
		retval = -3;
		goto out;
	}
	tmp->key = key;
	tmp->data = data;
	tmp->timestamp = time(NULL);
	tmp->next = *map;
	tmp->prev = NULL;

	/* Prepend current list with newly allocated entry... */
	if (*map)
		(*map)->prev = tmp;
	else
		dprintf("New map %p.\n", *map);
	/* ... and make tmp first element */
	*map = tmp;
out:
	return retval;
}

/*
 * @map_get
 * Reads value associated with the key in map data structure pointed by map.
 * Retrievs just values not older than given timeout (in seconds). timeout = 0
 * means no timeout.
 * retval: pointer to the data associated with the key
 *	if NULL, no key was found or other error occurred
 *	space for the returned value is freshly allocated
 */
void *map_get(struct map **map, char *key, time_t timeout)
{
	struct map *tmp = *map;
	void *value = NULL;
	time_t now = time(NULL);

	if (!*map || !key)
		goto out;

	dprintf("Getting key %s from map %p.\n", key, *map);

	while (tmp) {
		if (!strcmp(key, tmp->key)) {
			if (timeout > 0 && now - tmp->timestamp > timeout) {
				/*
				 * We invalidate the key, but someone
				 * could try it with different timeout.
				 * We don't bother, he'll wait.
				 */
				dprintf("Cache expired for key %s in map %p.\n", key, *map);
				map_del(map, key);
				goto out;
			} else
				value = tmp->data;
			break;
		}
		tmp = tmp->next;
	}
out:
	return value;
}

/*
 * @map_del
 * Removes the tuple denoted by key from map data structure pointed by map.
 * retval:
 *	0:  OK, removal successful
 *	1:  No tuple with given key found
 *	-1: invalid input data (map or key pointers are invalid)
 */
int map_del(struct map **map, char *key)
{
	struct map *tmp = *map;
	int retval = 1;

	if (!tmp || !key) {
		retval = -1;
		goto out;
	}

	dprintf("Deleting key %s from map %p.\n", key, *map);

	while (tmp) {
		if (!strcmp(key, tmp->key)) {
			/* Free allocated space */
			free(tmp->key);
			free(tmp->data);
			if (tmp->prev)
				tmp->prev->next = tmp->next;
			if (tmp->next)
				tmp->next->prev = tmp->prev;
			/*
			 * If we free the first element on list, we need
			 * to move the head pointer to the next position.
			 */
			if (tmp == *map)
				*map = tmp->next;
			free(tmp);
			retval = 0;
			break;
		}
		tmp = tmp->next;
	}

out:
	return retval;
}
