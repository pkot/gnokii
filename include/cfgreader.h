/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	cfgreader.h - header file for code in cfgreader.c

*/

#ifndef _CFGREADER_H
#define _CFGREADER_H

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

	/* Function prototypes */

struct CFG_Header	*CFG_ReadFile(char *filename);
char 				*CFG_Get(struct CFG_Header *cfg, char *section, char *key);
char				*CFG_Set(struct CFG_Header *cfg, char *section, char *key, 
		    				char *value);
int 				CFG_WriteFile(struct CFG_Header *cfg, char *filename);

#endif /* _CFGREADER_H */
