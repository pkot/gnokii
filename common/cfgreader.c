/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Config file (/etc/gnokiirc and ~/.gnokiirc) reader.

  Modified from code by Tim Potter.

*/

#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "cfgreader.h"

struct CFG_Header *CFG_Info;

/* Read configuration information from a ".INI" style file */
struct CFG_Header *CFG_ReadFile(const char *filename)
{
	FILE *handle;
	char *line;
	char *buf;
	struct CFG_Header *cfg_info = NULL, *cfg_head = NULL;

	/* Error check */
	if (filename == NULL) {
		return NULL;
	}

	/* Initialisation */
	if ((buf = (char *)malloc(255)) == NULL) {
		return NULL;
	}

	/* Open file */
	if ((handle = fopen(filename, "r")) == NULL) {
		dprintf("CFG_ReadFile - open %s: %s\n", filename, strerror(errno));
		return NULL;
	}
	else
		dprintf("Opened configuration file %s\n", filename);

	/* Iterate over lines in the file */
	while (fgets(buf, 255, handle) != NULL) {

		line = buf;

		/* Strip leading, trailing whitespace */
		while(isspace((int) *line))
			line++;

		while((strlen(line) > 0) && isspace((int) line[strlen(line) - 1]))
			line[strlen(line) - 1] = '\0';

		/* Ignore blank lines and comments */
		if ((*line == '\n') || (*line == '\0') || (*line == '#'))
			continue;

		/* Look for "headings" enclosed in square brackets */
		if ((line[0] == '[') && (line[strlen(line) - 1] == ']')) {
			struct CFG_Header *heading;

			/* Allocate new heading entry */
			if ((heading = (struct CFG_Header *)malloc(sizeof(*heading))) == NULL) {
				return NULL;
			}

			/* Fill in fields */
			memset(heading, '\0', sizeof(*heading));

			line++;
			line[strlen(line) - 1] = '\0';

			/* FIXME: strdup is not ANSI C compliant. */
			heading->section = strdup(line);

			/* Add to tail of list  */
			heading->prev = cfg_info;

			if (cfg_info != NULL) {
				cfg_info->next = heading;
			} else {
				/* Store copy of head of list for return value */
				cfg_head = heading;
			}

			cfg_info = heading;

			dprintf("Added new section %s\n", heading->section);

			/* Go on to next line */
			continue;
		}

		/* Process key/value line */

		if ((strchr(line, '=') != NULL) && cfg_info != NULL) {
			struct CFG_Entry *entry;
			char *value;

			/* Allocate new entry */
			if ((entry = (struct CFG_Entry *)malloc(sizeof(*entry))) == NULL) {
				return NULL;
			}

			/* Fill in fields */
			memset(entry, '\0', sizeof(*entry));

			value = strchr(line, '=');
			*value = '\0';		/* Split string */
			value++;

			while(isspace((int) *value)) {      /* Remove leading white */
				value++;
			}

			entry->value = strdup(value);

			while((strlen(line) > 0) && isspace((int) line[strlen(line) - 1])) {
				line[strlen(line) - 1] = '\0';  /* Remove trailing white */
			}

			/* FIXME: strdup is not ANSI C compliant. */
			entry->key = strdup(line);

			/* Add to head of list */

			entry->next = cfg_info->entries;

			if (cfg_info->entries != NULL) {
				cfg_info->entries->prev = entry;
			}

			cfg_info->entries = entry;

			dprintf("Adding key/value %s/%s\n", entry->key, entry->value);

			/* Go on to next line */
			continue;
		}

			/* Line not part of any heading */
		fprintf(stderr, "Orphaned line: %s\n", line);
	}

	/* Return pointer to configuration information */
	return cfg_head;
}

/*  Write configuration information to a config file */

int CFG_WriteFile(struct CFG_Header *cfg, const char *filename)
{
	/* Not implemented - tricky to do and preserve comments */
	return 0;
}

/*
 * Find the value of a key in a config file.  Return value associated
 * with key or NULL if no such key exists.
 */

char *CFG_Get(struct CFG_Header *cfg, const char *section, const char *key)
{
	struct CFG_Header *h;
	struct CFG_Entry *e;

	if ((cfg == NULL) || (section == NULL) || (key == NULL)) {
		return NULL;
	}

	/* Search for section name */
	for (h = cfg; h != NULL; h = h->next) {
		if (strcmp(section, h->section) == 0) {
			/* Search for key within section */
			for (e = h->entries; e != NULL; e = e->next) {
				if (strcmp(key, e->key) == 0) {
					/* Found! */
					return e->value;
				}
			}
		}
	}
	/* Key not found in section */
	return NULL;
}

/*
 * Return all the entries of the fiven section.
 */

void CFG_GetForeach(struct CFG_Header *cfg, const char *section, CFG_GetForeach_func func)
{
	struct CFG_Header *h;
	struct CFG_Entry *e;

	if ((cfg == NULL) || (section == NULL) || (func == NULL)) {
		return;
	}

	/* Search for section name */
	for (h = cfg; h != NULL; h = h->next) {
		if (strcmp(section, h->section) == 0) {
			/* Search for key within section */
			for (e = h->entries; e != NULL; e = e->next)
				(*func)(section, e->key, e->value);
		}
	}
}

/*  Set the value of a key in a config file.  Return the new value if
    the section/key can be found, else return NULL.  */

char *CFG_Set(struct CFG_Header *cfg, const char *section, const char *key,
		const char *value)
{
	struct CFG_Header *h;
	struct CFG_Entry *e;

	if ((cfg == NULL) || (section == NULL) || (key == NULL) ||
	    (value == NULL)) {
		return NULL;
	}

	/* Search for section name */
	for (h = cfg; h != NULL; h = h->next) {
		if (strcmp(section, h->section) == 0) {
			/* Search for key within section */
			for (e = h->entries; e != NULL; e = e->next) {
				if ((e->key != NULL) && strcmp(key, e->key) == 0) {
					/* Found - set value */
					free(e->key);
					/* FIXME: strdup is not ANSI C compliant. */
					e->key = strdup(value);
					return e->value;
				}
			}
		}
	}
	/* Key not found in section */
	return NULL;
}

int readconfig(char **model, char **port, char **initlength,
	       char **connection, char **bindir)
{
	char *homedir;
	char rcfile[200];
	char *DefaultConnection = "serial";
	char *DefaultBindir     = "/usr/local/sbin/";

	/* I know that it doesn't belong here but currently there is now generic
	 * application init function anywhere.
	 */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

#ifdef WIN32
	homedir = getenv("HOMEDRIVE");
	strncpy(rcfile, homedir ? homedir : "", 200);
	homedir = getenv("HOMEPATH");
	strncat(rcfile, homedir ? homedir : "", 200);
	strncat(rcfile, "\\_gnokiirc", 200);
#else
	homedir = getenv("HOME");
	if (homedir) strncpy(rcfile, homedir, 200);
	strncat(rcfile, "/.gnokiirc", 200);
#endif

	/* Try opening .gnokirc from users home directory first */
	if ((CFG_Info = CFG_ReadFile(rcfile)) == NULL) {
		/* It failed so try for /etc/gnokiirc */
		if ((CFG_Info = CFG_ReadFile("/etc/gnokiirc")) == NULL) {
			/* That failed too so exit */
			fprintf(stderr, _("Couldn't open %s or /etc/gnokiirc. Exiting now...\n"), rcfile);
			return -1;
		}
	}

	(char *)*model = CFG_Get(CFG_Info, "global", "model");
	if (!*model) {
		fprintf(stderr, _("Config error - no model specified. Exiting now...\n"));
		return -2;
	}

	(char *)*port = CFG_Get(CFG_Info, "global", "port");
	if (!*port) {
		fprintf(stderr, _("Config error - no port specified. Exiting now...\n"));
		return -3;
	}

	(char *)*initlength = CFG_Get(CFG_Info, "global", "initlength");
	if (!*initlength) (char *)*initlength = "default";

	(char *)*connection = CFG_Get(CFG_Info, "global", "connection");
	if (!*connection) (char *)*connection = DefaultConnection;

	(char *)*bindir = CFG_Get(CFG_Info, "global", "bindir");
	if (!*bindir) (char *)*bindir = DefaultBindir;

	return 0;
}
