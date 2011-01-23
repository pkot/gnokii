/*

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

  Copyright (C) 1999      Tim Potter
  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2000      Jan Derfinak
  Copyright (C) 2001      Jan Kratochvil
  Copyright (C) 2001-2011 Pawel Kot
  Copyright (C) 2002-2004 BORBELY Zoltan
  Copyright (C) 2005      Bastien Nocera

  Config reader.

  Modified from code by Tim Potter.

*/

#include "config.h"
#include "compat.h"
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "cfgreader.h"
#include "gnokii-internal.h"

GNOKII_API struct gn_cfg_header *gn_cfg_info;
static gn_config gn_config_default, gn_config_global;

/* Handy macros */
#define FREE(x)	do { if (x) free(x); (x) = NULL; } while (0)

/*
 * Function to dump configuration pointed by @config
 * @config:	configuration to be dumped
 */
static void cfg_dump(struct gn_cfg_header *config)
{
	struct gn_cfg_header *hdr = config;

	dprintf("Dumping configuration.\n");
	/* Loop over sections */
	while (hdr) {
		struct gn_cfg_entry *entry = hdr->entries;

		dprintf("[%s]\n", hdr->section);
		/* Loop over entries */
		while (entry) {
			dprintf("%s = %s\n", entry->key, entry->value);
			entry = entry->next;
		}
		hdr = hdr->next;
	}
}

/*
 * Function to allocate configuration header structure.
 * Structure is appended to @config.
 * @config:	existing config structure
 * @name:	section name; default (if name==NULL) is "global"
 */
static struct gn_cfg_header *cfg_header_allocate(struct gn_cfg_header *config, char *name)
{
	struct gn_cfg_header *hdr;

	/*
	 * Allocate memory for the header structure and assign name.
	 * Default name is "global".
	 */
	hdr = calloc(sizeof(struct gn_cfg_header), 1);
	if (!hdr) {
		dprintf("Failed to allocate gn_cfg_header.\n");
		return NULL;
	}
	if (name)
		hdr->section = strdup(name);
	else
		hdr->section = strdup("global");
	if (!hdr->section) {
		dprintf("Failed to assign a name to gn_cfg_header.\n");
		free(hdr);
		return NULL;
	}

	/* Add to cfg_info list */
	hdr->prev = config;
	if (config)
		config->next = hdr;

	dprintf("Adding new section %s\n", hdr->section);

	return hdr;
}

/*
 * Function to find config section identified by @name. If name is not given
 * "global" section is returned.
 * @config:	pointer to the configuration structure
 * @name:	section name
 */
static struct gn_cfg_header *cfg_header_get(struct gn_cfg_header *config, char *name)
{
	struct gn_cfg_header *hdr = config;

	if (!config)
		return NULL;

	if (!name)
		name = "global";

	while (hdr) {
		if (!strcmp(name, hdr->section))
			return hdr;
		hdr = hdr->next;
	}
	return NULL;
}

/*
 * Function to assign a variable in the given config section.
 * @config:	given configuration section
 * @name:	variable (key) name
 * @value:	variable (key) value
 * @overwrite:	overwrite existing keys?
 */
static struct gn_cfg_header *cfg_variable_set(struct gn_cfg_header *config, char *name, char *value, int overwrite)
{
	struct gn_cfg_entry *oldentry, *entry;

	if (!name || !value) {
		dprintf("Neither name nor value can be NULL.\n");
		return NULL;
	}

	/* Find a place in the section, where to add the new entry. */
	oldentry = config->entries;
	while (oldentry) {
		if (!strcmp(name, oldentry->key))
			break;
		oldentry = oldentry->next;
	}

	/* If found and instructed not to overwrite, return */
	if (oldentry && !overwrite) {
		dprintf("Key %s already exists in section %s\n", name, config->section);
		return NULL;
	}

	/* Allocate new entry */
	entry = calloc(sizeof(struct gn_cfg_entry), 1);
	if (!entry) {
		dprintf("Failed to allocate gn_cfg_entry.\n");
		return NULL;
	}
	entry->key = strdup(name);
	entry->value = strdup(value);
	if (!entry->key || !entry->value) {
		dprintf("Failed to allocate key/value for the entry.\n");
		FREE(entry->key);
		FREE(entry->value);
		free(entry);
		return NULL;
	}

	/* Add to existing variables in section. */
	entry->next = config->entries;
	if (config->entries)
		config->entries->prev = entry;
	config->entries = entry;

	/* Remove the old entry */
	if (oldentry) {
		if (oldentry->next)
			oldentry->next->prev = oldentry->prev;
		if (oldentry->prev)
			oldentry->prev->next = oldentry->next;
		free(oldentry->key);
		free(oldentry->value);
		free(oldentry);
	}

	dprintf("Added %s/%s to section %s.\n", entry->key, entry->value, config->section);

	return config;
}

/*
 * Function to assign a variable in the given section. If the section is not
 * found in the config structure, new section is created.
 * @config:	configuration
 * @section:	section name
 * @name:	variable (key) name
 * @value:	variable (key) value
 * @overwrite:	overwrite existing keys?
 */
GNOKII_API struct gn_cfg_header *gn_cfg_variable_set(struct gn_cfg_header *config, char *section, char *name, char *value, int overwrite)
{
	struct gn_cfg_header *hdr;

	/* Find the appropriate section */
	hdr = cfg_header_get(config, section);
	if (!hdr)
		hdr = cfg_header_allocate(config, section);
	if (!hdr) {
		dprintf("Failed to set variable (%s %s %s).\n", section, name, value);
		return NULL;
	}

	hdr = cfg_variable_set(hdr, name, value, overwrite);

	return hdr;
}

/*
 * Function to create generic configuration section. Basic config consists of model,
 * connection and port variables. If no section name is given, global one is used.
 * Please note that valid section names for phone config are: global and phone_*.
 * @section:	optional section name
 * @model:	model to be set
 * @connection:	connection to be set
 * @port:	port to be set
 */
GNOKII_API struct gn_cfg_header *gn_cfg_section_create(char *section, char *model, char *connection, char *port)
{
	char *sname;
	struct gn_cfg_header *hdr = NULL, *config = NULL;

	if (!model | !connection | !port) {
		dprintf("Neither model nor connection nor port can be NULL.\n");
		return NULL;
	}

	sname = (section ? section : "global");

	config = cfg_header_allocate(NULL, sname);
	if (!config) {
		dprintf("Failed to create config.\n");
		return NULL;
	}
	hdr = gn_cfg_variable_set(config, sname, "model", model, 1);
	if (!hdr) {
		dprintf("Failed to create config.\n");
		free(config);
		return NULL;
	}
	hdr = gn_cfg_variable_set(config, sname, "connection", connection, 1);
	if (!hdr) {
		dprintf("Failed to create config.\n");
		free(config);
		return NULL;
	}
	hdr = gn_cfg_variable_set(config, sname, "port", port, 1);
	if (!hdr) {
		dprintf("Failed to create config.\n");
		free(config);
		return NULL;
	}

	return config;
}

/*
 * Function to create generic config. Config is assigned to gn_cfg_info structure.
 * @model:	model to be set
 * @connection:	connection to be set
 * @port:	port to be set
 */
GNOKII_API struct gn_cfg_header *gn_cfg_generic_create(char *model, char *connection, char *port)
{
	struct gn_cfg_header *config;

	config = gn_cfg_section_create(NULL, model, connection, port);
	if (!config)
		return NULL;

	/* Dump config */
	cfg_dump(config);

	/* Assign to gn_cfg_info. libgnokii needs it so far. */
	gn_cfg_info = config;
	return config;
}

/*
 * Function to create basic config for bluetooth setup. Config is assigned to gn_cfg_info structure.
 * @model:	model to be set
 * @connection:	connection to be set
 * @port:	port to be set
 */
GNOKII_API struct gn_cfg_header *gn_cfg_bluetooth_create(char *model, char *btmac, char *rfchannel)
{
	struct gn_cfg_header *hdr, *config;

	if (!model | !btmac | !rfchannel) {
		dprintf("Neither model nor Bluetooth mac address nor rfcomm channel can be NULL.\n");
		return NULL;
	}

	config = gn_cfg_section_create(NULL, model, "bluetooth", btmac);
	if (!config)
		return NULL;
	hdr = gn_cfg_variable_set(config, "global", "rfcomm_channel", rfchannel, 1);
	if (!hdr) {
		dprintf("Failed to create config.\n");
		free(config);
		return NULL;
	}

	/* Dump config */
	cfg_dump(config);

	/* Assign to gn_cfg_info. libgnokii needs it so far. */
	gn_cfg_info = config;
	return config;
}

struct gn_cfg_header *cfg_memory_read(const char **lines)
{
	char *line, *buf;
	struct gn_cfg_header *cfg_info = NULL, *cfg_head = NULL;
	int i;

	/* Check whether the given memory location is not null */
	if (lines == NULL) {
		dprintf("cfg_memory_read - passed nil data\n");
		return NULL;
	} else {
		dprintf("Opened configuration file from memory\n");
	}

	/* Iterate over lines in the file */
	for (i = 0; lines[i] != NULL; i++) {

		line = strdup (lines[i]);
		buf = line;

		/* Strip leading, trailing whitespace */
		while (isspace((int) *line))
			line++;

		while ((strlen(line) > 0) && isspace((int) line[strlen(line) - 1]))
			line[strlen(line) - 1] = '\0';

		/* Ignore blank lines and comments */
		if ((*line == '\n') || (*line == '\0') || (*line == '#')) {
			free(buf);
			continue;
		}

		/* Look for "headings" enclosed in square brackets */
		if ((line[0] == '[') && (line[strlen(line) - 1] == ']')) {
			struct gn_cfg_header *heading;

			/* Allocate new heading entry */
			if ((heading = (struct gn_cfg_header *)malloc(sizeof(*heading))) == NULL) {
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

			free(buf);

			/* Go on to next line */
			continue;
		}

		/* Process key/value line */
		if ((strchr(line, '=') != NULL) && cfg_info != NULL) {
			struct gn_cfg_entry *entry;
			char *value;

			/* Allocate new entry */
			if ((entry = (struct gn_cfg_entry *)malloc(sizeof(*entry))) == NULL) {
				return NULL;
			}

			/* Fill in fields */
			memset(entry, '\0', sizeof(*entry));

			value = strchr(line, '=');
			*value = '\0';	/* Split string */
			value++;

			while (isspace((int) *value)) { /* Remove leading white */
				value++;
			}

			entry->value = strdup(value);

			while ((strlen(line) > 0) && isspace((int) line[strlen(line) - 1])) {
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

			free(buf);

			/* Go on to next line */
			continue;
		}

		/* Line not part of any heading */
		fprintf(stderr, _("Orphaned line: %s\nIf in doubt place this line into [global] section.\n"), line);

		free(buf);
	}

	/* Return pointer to configuration information */
	return cfg_head;
}

GNOKII_API void gn_cfg_free_default()
{
	while (gn_cfg_info) {
		struct gn_cfg_header *next;
		struct gn_cfg_entry *entry;

		/* free section name */
		free(gn_cfg_info->section);

		/* delete all entries */
		entry = gn_cfg_info->entries;
		while (entry) {
			struct gn_cfg_entry *entry_next;
			free(entry->key);
			free(entry->value);
			entry_next = entry->next;
			free(entry);
			entry = entry_next;
		}

		/* proceed with next section */
		next = gn_cfg_info->next;
		free(gn_cfg_info);
		gn_cfg_info = next;
	}
}

#define READ_CHUNK_SIZE 64

/* Read configuration information from a ".INI" style file */
struct gn_cfg_header *cfg_file_read(const char *filename)
{
	FILE *handle;
	char *lines, *line_begin, *line_end, *pos;
	char **split_lines;
	int read, ret, num_lines, i, copied;
	struct gn_cfg_header *header = NULL;

	/* Open file */
	if ((handle = fopen(filename, "r")) == NULL) {
		dprintf("cfg_file_read - open %s: %s\n", filename, strerror(errno));
		goto out;
	} else {
		dprintf("Opened configuration file %s\n", filename);
	}

	/* Read the lines */
	lines = NULL;
	read = 0;
	do {
		lines = realloc(lines, read + READ_CHUNK_SIZE);
		if (!lines)
			goto err_read;

		ret = fread(lines + read, 1, READ_CHUNK_SIZE, handle);
		/* Read error */
		if (ret < 0 && feof(handle) == 0)
			goto err_read;

		/* Overflow */
		if (read + ret < read)
			goto err_read;

		read += ret;
	} while (ret > 0);

	fclose(handle);
	/* Make sure there's '\n' after the last line and NULL-terminate. */
	lines = realloc(lines, read + 2);
	lines[read] = '\n';
	lines[read+1] = '\0';

	/* Now split the lines */
	split_lines = NULL;
	line_begin = lines;
	num_lines = 0;
	copied = 0;

	while ((pos = strchr(line_begin, '\n')) && copied < read) {
		char *buf, *hash, *tmp;
		int nonempty = 0;

		/* Strip comments from the current line */
		hash = strchr(line_begin, '#');
		if (hash && (hash < pos || !pos))
			line_begin[hash-line_begin] = '\0';
		else
			hash = NULL;

		if (!pos) {
			line_end = lines + read;
		} else {
			line_end = pos;
		}

		/* skip empty lines */
		tmp = line_begin;
		nonempty = 0;
		while (*tmp && tmp < line_end && !nonempty) {
			if (!isspace(*tmp))
				nonempty = 1;
			tmp++;
		}

		if (nonempty) {
			num_lines++;
			buf = malloc((hash ? hash : line_end) - line_begin + 1);
			snprintf(buf, (hash ? hash : line_end) - line_begin + 1, "%s", line_begin);
			split_lines = realloc(split_lines,
					(num_lines + 1) * sizeof(char*));
			split_lines[num_lines - 1] = buf;
		}

		if (pos) {
			copied += (line_end + 1 - line_begin);
			line_begin = line_end + 1;
		}
	}


	free(lines);
	if (split_lines == NULL)
		goto out;
	split_lines[num_lines] = NULL;

	/* Finally, load the configuration from the split lines */
	header = cfg_memory_read((const char **)split_lines);

	/* Free the split_lines */
	for (i = 0; split_lines[i] != NULL; i++) {
		free(split_lines[i]);
	}
	free(split_lines);

	goto out;

err_read:
	fclose(handle);
	if (lines)
		free(lines);

out:
	return header;
}

/*  Write configuration information to a config file */
int cfg_file_write(struct gn_cfg_header *cfg, const char *filename)
{
	/* Not implemented - tricky to do and preserve comments */
	return 0;
}

/*
 * Find the value of a key in a config file.  Return value associated
 * with key or NULL if no such key exists.
 */
GNOKII_API char *gn_cfg_get(struct gn_cfg_header *cfg, const char *section, const char *key)
{
	struct gn_cfg_header *h;
	struct gn_cfg_entry *e;

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

int cfg_section_exists(struct gn_cfg_header *cfg, const char *section)
{
	struct gn_cfg_header *h;

	if (!cfg || !section)
		return false;
	/* Search for section name */
	for (h = cfg; h != NULL; h = h->next)
		if (!strcmp(section, h->section))
			return true;
	return false;
}

/*
 * Return all the entries of the given section.
 */
void cfg_foreach(const char *section, cfg_foreach_func func)
{
	struct gn_cfg_header *h;
	struct gn_cfg_entry *e;
	struct gn_cfg_header *cfg = gn_cfg_info;

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
char *cfg_set(struct gn_cfg_header *cfg, const char *section, const char *key,
	      const char *value)
{
	struct gn_cfg_header *h;
	struct gn_cfg_entry *e;

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

static gn_error cfg_psection_load(gn_config *cfg, const char *section, const gn_config *def)
{
	const char *val;
	char ch;

	memset(cfg, '\0', sizeof(gn_config));

	if (!cfg_section_exists(gn_cfg_info, section)) {
		fprintf(stderr, _("No %s section in the config file.\n"), section);
		return GN_ERR_NOPHONE;
	}

	/* You need to specify at least model, port and connection in the phone section */
	if (!(val = gn_cfg_get(gn_cfg_info, section, "model"))) {
		fprintf(stderr, _("You need to define '%s' in the config file.\n"), "model");
		return GN_ERR_NOMODEL;
	} else
		snprintf(cfg->model, sizeof(cfg->model), "%s", val);

	if (!(val = gn_cfg_get(gn_cfg_info, section, "port"))) {
		fprintf(stderr, _("You need to define '%s' in the config file.\n"), "port");
		return GN_ERR_NOPORT;
	} else
		snprintf(cfg->port_device, sizeof(cfg->port_device), "%s", val);

	if (!(val = gn_cfg_get(gn_cfg_info, section, "connection"))) {
		fprintf(stderr, _("You need to define '%s' in the config file.\n"), "connection");
		return GN_ERR_NOCONNECTION;
	} else {
		cfg->connection_type = gn_get_connectiontype(val);
		if (cfg->connection_type == GN_CT_NONE) {
			fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "connection", val);
			return GN_ERR_NOCONNECTION;
		}
		if (!strcmp("pcsc", val))
			fprintf(stderr, "WARNING: %s=%s is deprecated and will soon be removed. Use %s=%s instead.\n", "connection", val, "connection", "libpcsclite");
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "initlength")))
		cfg->init_length = def->init_length;
	else {
		if (!strcasecmp(val, "default"))
			cfg->init_length = 0;
		else if (sscanf(val, " %d %c", &cfg->init_length, &ch) != 1) {
			fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "initlength", val);
			fprintf(stderr, _("Assuming: %d\n"), def->init_length);
			cfg->init_length = def->init_length;
		}
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "serial_baudrate")))
		cfg->serial_baudrate = def->serial_baudrate;
	else if (sscanf(val, " %d %c", &cfg->serial_baudrate, &ch) != 1) {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "serial_baudrate", val);
		fprintf(stderr, _("Assuming: %d\n"), def->serial_baudrate);
		cfg->serial_baudrate = def->serial_baudrate;
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "serial_write_usleep")))
		cfg->serial_write_usleep = def->serial_write_usleep;
	else if (sscanf(val, " %d %c", &cfg->serial_write_usleep, &ch) != 1) {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "serial_write_usleep", val);
		fprintf(stderr, _("Assuming: %d\n"), def->serial_write_usleep);
		cfg->serial_write_usleep = def->serial_write_usleep;
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "handshake")))
		cfg->hardware_handshake = def->hardware_handshake;
	else if (!strcasecmp(val, "software") || !strcasecmp(val, "rtscts"))
		cfg->hardware_handshake = false;
	else if (!strcasecmp(val, "hardware") || !strcasecmp(val, "xonxoff"))
		cfg->hardware_handshake = true;
	else {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "handshake", val);
		fprintf(stderr, _("Use either \"%s\" or \"%s\".\n"), "software", "hardware");
		fprintf(stderr, _("Assuming: %s\n"), def->hardware_handshake ? "software" : "hardware");
		cfg->hardware_handshake = def->hardware_handshake;
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "require_dcd")))
		cfg->require_dcd = def->require_dcd;
	else if (sscanf(val, " %d %c", &cfg->require_dcd, &ch) != 1) {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "require_dcd", val);
		fprintf(stderr, _("Assuming: %d\n"), def->require_dcd);
		cfg->require_dcd = def->require_dcd;
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "smsc_timeout")))
		cfg->smsc_timeout = def->smsc_timeout;
	else if (sscanf(val, " %d %c", &cfg->smsc_timeout, &ch) == 1)
		cfg->smsc_timeout *= 10;
	else {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "smsc_timeout", val);
		fprintf(stderr, _("Assuming: %d\n"), def->smsc_timeout);
		cfg->smsc_timeout = def->smsc_timeout;
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "connect_script")))
		snprintf(cfg->connect_script, sizeof(cfg->connect_script), "%s", def->connect_script);
	else
		snprintf(cfg->connect_script, sizeof(cfg->connect_script), "%s", val);

	if (!(val = gn_cfg_get(gn_cfg_info, section, "disconnect_script")))
		snprintf(cfg->disconnect_script, sizeof(cfg->disconnect_script), "%s", def->disconnect_script);
	else
		snprintf(cfg->disconnect_script, sizeof(cfg->disconnect_script), "%s", val);

	if (!(val = gn_cfg_get(gn_cfg_info, section, "rfcomm_channel")))
		cfg->rfcomm_cn = def->rfcomm_cn;
	else if (sscanf(val, " %hhu %c", &cfg->rfcomm_cn, &ch) != 1) {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "rfcomm_channel", val);
		fprintf(stderr, _("Assuming: %d\n"), def->rfcomm_cn);
		cfg->rfcomm_cn = def->rfcomm_cn;
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "sm_retry")))
		cfg->sm_retry = def->sm_retry;
	else if (sscanf(val, " %d %c", &cfg->sm_retry, &ch) != 1) {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "sm_retry", val);
		fprintf(stderr, _("Assuming: %d\n"), def->sm_retry);
		cfg->sm_retry = def->sm_retry;
	}

	/* There is no global setting. You need to set irda_string
	 * in each section if you want it working */
	if (!(val = gn_cfg_get(gn_cfg_info, section, "irda_string")))
		cfg->irda_string[0] = 0;
	else {
		snprintf(cfg->irda_string, sizeof(cfg->irda_string), "%s", val);
		dprintf("Setting irda_string in section %s to %s\n", section, cfg->irda_string);
	}

	if (!(val = gn_cfg_get(gn_cfg_info, section, "use_locking")))
		cfg->use_locking = def->use_locking;
	else if (!strcasecmp(val, "no"))
		cfg->use_locking = 0;
	else if (!strcasecmp(val, "yes"))
		cfg->use_locking = 1;
	else {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), section, "use_locking", val);
		fprintf(stderr, _("Use either \"%s\" or \"%s\".\n"), "yes", "no");
		fprintf(stderr, _("Assuming: %s\n"), def->use_locking ? "yes" : "no");
		cfg->use_locking = def->use_locking;
	}

	return GN_ERR_NONE;
}

#define SETFLAG(f) \
if (!gnokii_strcmpsep(#f, val, ',')) {\
	phone_model.flags |= PM_##f;\
	dprintf("Flag PM_%s\n", #f);\
	continue;\
}
/* Load phone flags (formerly a static array in misc.c) */
gn_phone_model *gn_cfg_get_phone_model(struct gn_cfg_header *cfg, const char *product_name)
{
	struct gn_cfg_header *hdr;
	char *val, *comma, *end, *section = "flags";
	int count;
	static gn_phone_model phone_model = {NULL, NULL, 0};
	static char model[GN_MODEL_MAX_LENGTH] = "";

	if (phone_model.model)
		return &phone_model;

	val = gn_cfg_get(gn_cfg_info, section, product_name);
	if (val) {
		phone_model.model = model;
		/* Ignore trailing whitespace (leading whitespace has been stripped when loading the config file) */
		comma = val;
		while (*comma && *comma != ',')
			comma++;
		end = comma;
		while (end > val && (isblank(*end) || *end == ','))
			end--;
		count = end - val + 1;
		snprintf(model, GN_MODEL_MAX_LENGTH, "%.*s", count, val);

		/* Check flags */
		val = comma;
		for (;;) {
			while (*val && *val != ',')
				val++;
			/* This skips also empty fields */
			while (*val && (*val == ',' || isblank(*val)))
				val++;
			if (!*val)
				break;
			SETFLAG(OLD_DEFAULT);
			SETFLAG(DEFAULT);
			SETFLAG(DEFAULT_S40_3RD);
			SETFLAG(CALLERGROUP);
			SETFLAG(NETMONITOR);
			SETFLAG(KEYBOARD);
			SETFLAG(SMS);
			SETFLAG(CALENDAR);
			SETFLAG(DTMF);
			SETFLAG(DATA);
			SETFLAG(SPEEDDIAL);
			SETFLAG(EXTPBK);
			SETFLAG(AUTHENTICATION);
			SETFLAG(FOLDERS);
			SETFLAG(FULLPBK);
			SETFLAG(SMSFILE);
			SETFLAG(EXTPBK2);
			SETFLAG(EXTCALENDAR);
			SETFLAG(XGNOKIIBREAKAGE);
			/* FIXME? duplicated code from above */
			comma = val;
			while (*comma && *comma != ',')
				comma++;
			end = comma;
			while (end > val && (isblank(*end) || *end == ','))
				end--;
			count = end - val + 1;
			dprintf("Unknown flag \"%.*s\"\n", count, val);
		}
		return &phone_model;
	}
#undef SETFLAG

	/* Give the user some hint on why the product_name wasn't found */
	hdr = cfg_header_get(cfg, (char *)section);
	if (!hdr) {
		fprintf(stderr, _("No %s section in the config file.\n"), section);
	}

	return &phone_model;
}

static bool cfg_get_log_target(gn_log_target *t, const char *opt)
{
	char *val;

	if (!(val = gn_cfg_get(gn_cfg_info, "logging", opt)))
		val = "off";

	if (!strcasecmp(val, "off"))
		*t = GN_LOG_T_NONE;
	else if (!strcasecmp(val, "on"))
		*t = GN_LOG_T_STDERR;
	else {
		fprintf(stderr, _("Unsupported [%s] %s value \"%s\"\n"), "logging", opt, val);
		fprintf(stderr, _("Use either \"%s\" or \"%s\".\n"), "off", "on");
		fprintf(stderr, _("Assuming: %s\n"), "off");
		*t = GN_LOG_T_NONE;
	}

	return true;
}

#define MAX_PATH_LEN 255

#define CHECK_SIZE()	if (*retval >= size) { \
	size *= 2; \
	config_file_locations = realloc(config_file_locations, size); \
}

/*
 * get_locations() returns the list of possible config file locations that
 * may be used on the given system.  It returns the array of paths to the
 * config files.  @retval parameter denotes how many locations are returned. 
 * The resulting array needs to be freed afterwards.
 */ 
#ifdef WIN32
/* Windows version */
static char **get_locations(int *retval)
{
	char **config_file_locations;
	char *appdata, *homedrive, *homepath, *systemroot; /* env variables */
	int size = 3; /* default size for config_file_locations */
	char path[MAX_PATH_LEN];
	const char *fname = "gnokii.ini";

	*retval = size;

	config_file_locations = calloc(size, sizeof(char *));

	appdata = getenv("APPDATA");
	homedrive = getenv("HOMEDRIVE");
	homepath = getenv("HOMEPATH");
	systemroot = getenv("SYSTEMROOT");
	/* 1. %APPDATA%\gnokii\config */
	snprintf(path, MAX_PATH_LEN, "%s\\gnokii\\%s", appdata, fname);
	config_file_locations[0] = strdup(path);
	/* old gnokii behaviour */
	/* 2. %HOMEDRIVE%\%HOMEPATH%\_gnokiirc */
	snprintf(path, MAX_PATH_LEN, "%s\\%s\\%s", homedrive, homepath, fname);
	config_file_locations[1] = strdup(path);
	/* 3. %SYSTEMROOT%\gnokiirc */
	snprintf(path, MAX_PATH_LEN, "%s\\%s", systemroot, fname);
	config_file_locations[2] = strdup(path);

	return config_file_locations;
}
#else /* WIN32 */
#  ifdef __MACH__
/* Mac OS X version */
static char **get_locations(int *retval)
{
	char **config_file_locations;
	char *home; /* env variables */
	int size = 3; /* default size for config_file_locations */
	char path[MAX_PATH_LEN];

	*retval = size;

	config_file_locations = calloc(size, sizeof(char *));

	home = getenv("HOME");
	/* 1. $HOME/Library/Preferences/gnokii/config */
	snprintf(path, MAX_PATH_LEN, "%s/Library/Preferences/gnokii/config", home);
	config_file_locations[0] = strdup(path);
	/* old gnokii behaviour */
	/* 2. $HOME/.gnokiirc */
	snprintf(path, MAX_PATH_LEN, "%s/.gnokiirc", home);
	config_file_locations[1] = strdup(path);
	/* 3. /etc/gnokiirc */
	snprintf(path, MAX_PATH_LEN, "/etc/gnokiirc");
	config_file_locations[2] = strdup(path);

	return config_file_locations;
}
#  else /* !__MACH__ */
/* freedesktop.org compliancy: http://standards.freedesktop.org/basedir-spec/latest/ar01s03.html */
#define XDG_CONFIG_HOME	"/.config"	/* $HOME/.config */
#define XDG_CONFIG_DIRS "/etc/xdg"
static char **get_locations(int *retval)
{
	char **config_file_locations;
	char *xdg_config_home, *xdg_config_dirs, *home; /* env variables */
	char **xdg_config_dir;
	char *aux, *dirs;
	int j, i = 0;
	char path[MAX_PATH_LEN];
	int size = 8; /* default size for config_file_locations */
	int xcd_size = 4; /* default size for xdg_config_dir - number of elements */
	int free_xdg_config_home = 0;

	*retval = 0;
	config_file_locations = calloc(size, sizeof(char *));

	/* First, let's get all env variables we will need */
	home = getenv("HOME");

	xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (!xdg_config_home) {
		xdg_config_home = calloc(MAX_PATH_LEN, sizeof(char));
		free_xdg_config_home = 1;
		sprintf(xdg_config_home, "%s%s", home, XDG_CONFIG_HOME);
	}

	aux = getenv("XDG_CONFIG_DIRS");
	if (aux)
		xdg_config_dirs = strdup(aux);
	else
		xdg_config_dirs = strdup(XDG_CONFIG_DIRS);

	dirs = xdg_config_dirs;
	/* split out xdg_config_dirs into tokens separated by ':' */
	xdg_config_dir = calloc(xcd_size, sizeof(char *));
	while ((aux = strsep(&xdg_config_dirs, ":")) != NULL) {
		xdg_config_dir[i++] = strdup(aux);
		if (i >= xcd_size) {
			xcd_size *= 2;
			xdg_config_dir = realloc(xdg_config_dir, xcd_size);
		}
	}
	free(dirs);

	/* 1. $XDG_CONFIG_HOME/gnokii/config ($HOME/.config) */
	snprintf(path, MAX_PATH_LEN, "%s/gnokii/config", xdg_config_home);
	config_file_locations[(*retval)++] = strdup(path);

	/* 2. $XDG_CONFIG_DIRS/gnokii/config (/etc/xdg) */
	for (j = 0; j < i; j++) {
		snprintf(path, MAX_PATH_LEN, "%s/gnokii/config", xdg_config_dir[j]);
		config_file_locations[(*retval)++] = strdup(path);
		CHECK_SIZE();
		free(xdg_config_dir[j]);
	}
	free(xdg_config_dir);

	/* old gnokii behaviour */

	/* 3. $HOME/.gnokiirc */
	snprintf(path, MAX_PATH_LEN, "%s/.gnokiirc", home);
	config_file_locations[(*retval)++] = strdup(path);
	CHECK_SIZE();

	/* 4. /etc/gnokiirc */
	snprintf(path, MAX_PATH_LEN, "/etc/gnokiirc");
	config_file_locations[(*retval)++] = strdup(path);

	if (free_xdg_config_home)
		free(xdg_config_home);

	return config_file_locations;
}
#  endif /* !__MACH__ */
#endif /* !WIN32 */

GNOKII_API gn_error gn_cfg_read_default()
{
	gn_error error = GN_ERR_FAILED;
	char **config_file_locations = NULL;
	int num, i;

	config_file_locations = get_locations(&num);

	for (i = 0; i < num; i++) {
		if (error != GN_ERR_NONE)
			error = gn_cfg_file_read(config_file_locations[i]);
		if (error != GN_ERR_NONE)
			fprintf(stderr, _("Couldn't read %s config file.\n"), config_file_locations[i]);
		free(config_file_locations[i]);
	}		
	free(config_file_locations);
	return error;
}

/* DEPRECATED */
static gn_error cfg_file_or_memory_read(const char *file, const char **lines)
{
	char *val;
	gn_error error;

	error = gn_lib_init();
	if (error != GN_ERR_NONE) {
		fprintf(stderr, _("Failed to initialize libgnokii.\n"));
		return error;
	}

	if (file == NULL && lines == NULL) {
		fprintf(stderr, _("Couldn't open a config file or memory.\n"));
		return GN_ERR_NOCONFIG;
	}

	/* I know that it doesn't belong here but currently there is now generic
	 * application init function anywhere.
	 */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/*
	 * Try opening a given config file
	 */
	if (file != NULL)
		gn_cfg_info = cfg_file_read(file);
	else
		gn_cfg_info = cfg_memory_read(lines);

	if (gn_cfg_info == NULL) {
		/* this is bad, but the previous was much worse - bozo */
		return GN_ERR_NOCONFIG;
	}
	gn_config_default.model[0] = 0;
	gn_config_default.port_device[0] = 0;
	gn_config_default.connection_type = GN_CT_Serial;
	gn_config_default.init_length = 0;
	gn_config_default.serial_baudrate = 19200;
	gn_config_default.serial_write_usleep = -1;
	gn_config_default.hardware_handshake = false;
	gn_config_default.require_dcd = false;
	gn_config_default.smsc_timeout = -1;
	gn_config_default.irda_string[0] = 0;
	gn_config_default.connect_script[0] = 0;
	gn_config_default.disconnect_script[0] = 0;
	gn_config_default.rfcomm_cn = 0;
	gn_config_default.sm_retry = 0;
	gn_config_default.use_locking = 0;

	if ((error = cfg_psection_load(&gn_config_global, "global", &gn_config_default)) != GN_ERR_NONE)
		return error;

	/* hack to support [sms] / smsc_timeout parameter */
	if (gn_config_global.smsc_timeout < 0) {
		if (!(val = gn_cfg_get(gn_cfg_info, "sms", "timeout")))
			gn_config_global.smsc_timeout = 100;
		else
			gn_config_global.smsc_timeout = 10 * atoi(val);
	}

	if (!cfg_get_log_target(&gn_log_debug_mask, "debug") ||
	    !cfg_get_log_target(&gn_log_rlpdebug_mask, "rlpdebug") ||
	    !cfg_get_log_target(&gn_log_xdebug_mask, "xdebug"))
		return GN_ERR_NOLOG;

	gn_log_debug("LOG: debug mask is 0x%x\n", gn_log_debug_mask);
	gn_log_rlpdebug("LOG: rlpdebug mask is 0x%x\n", gn_log_rlpdebug_mask);
	gn_log_xdebug("LOG: xdebug mask is 0x%x\n", gn_log_xdebug_mask);
	if (file)
		dprintf("Config read from file %s.\n", file);
	return GN_ERR_NONE;
}

GNOKII_API gn_error gn_cfg_file_read(const char *file)
{
	return cfg_file_or_memory_read(file, NULL);
}

GNOKII_API gn_error gn_cfg_memory_read(const char **lines)
{
	return cfg_file_or_memory_read(NULL, lines);
}

GNOKII_API gn_error gn_cfg_phone_load(const char *iname, struct gn_statemachine *state)
{
	char section[256];
	gn_error error;

	if (!state)
		return GN_ERR_INTERNALERROR;

	if (iname == NULL || *iname == '\0') {
		state->config = gn_config_global;
	} else {
		snprintf(section, sizeof(section), "phone_%s", iname);
		if ((error = cfg_psection_load(&state->config, section, &gn_config_global)) != GN_ERR_NONE)
			return error;
	}

	if (state->config.model[0] == '\0') {
		fprintf(stderr, _("Config error - no model specified.\n"));
		return GN_ERR_NOMODEL;
	}
	if (state->config.port_device[0] == '\0') {
		fprintf(stderr, _("Config error - no port specified.\n"));
		return GN_ERR_NOPORT;
	}

	return GN_ERR_NONE;
}
