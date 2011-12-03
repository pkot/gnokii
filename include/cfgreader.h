/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

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
