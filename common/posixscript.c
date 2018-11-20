/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2018       Ladislav Michl

*/

#include "config.h"
#include "compat.h"
#include "cfgreader.h"
#include "misc.h"
#include "gnokii.h"
#include "gnokii-internal.h"

#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define cfg_foreach_entry(section, header, entry) \
	for (h = gn_cfg_info; header != NULL; header = header->next) \
		if (strcmp(section, h->section) == 0) \
			for (entry = h->entries; entry != NULL; entry = entry->next)

int device_script(int fd, int connect, struct gn_statemachine *state)
{
	const char *scriptname, *section;
	char **envp, *env;
	struct gn_cfg_entry *e;
	struct gn_cfg_header *h;
	posix_spawn_file_actions_t fa;
	pid_t pid;
	int cnt, ret, status;
	size_t len;

	if (connect) {
		scriptname = state->config.connect_script;
		section = "connect_script";
	} else {
		scriptname = state->config.disconnect_script;
		section = "disconnect_script";
	}
	if (scriptname[0] == '\0')
		return 0;

	cnt = 0; len = 0; ret = -1;
	h = gn_cfg_info;
	cfg_foreach_entry(section, h, e) {
		len += strlen(e->key);
		len += strlen(e->value);
		len += 1 + 1 + sizeof(char *);
		cnt++;
	}

	env = malloc(len + sizeof(char *));
	if (!env) {
		fprintf(stderr, _("device_script(\"%s\"): out of memory\n"), scriptname);
		goto out;
	}

	envp = &env;
	env += sizeof(char *) * (cnt + 1);
	h = gn_cfg_info;
	cfg_foreach_entry(section, h, e) {
		*envp++ = env;
		env += sprintf(env, "%s=%s", e->key, e->value);
		env++;
	}
	*envp = NULL;

	if (posix_spawn_file_actions_init(&fa)) {
		fprintf(stderr, _("device_script(\"%s\"): file descriptor preparation failure: %s\n"),
			scriptname, strerror(errno));
		goto out_free;
	}
	if (posix_spawn_file_actions_adddup2(&fa, fd, STDIN_FILENO) ||
	    posix_spawn_file_actions_adddup2(&fa, fd, STDOUT_FILENO) ||
	    posix_spawn_file_actions_addclose(&fa, fd)) {
		fprintf(stderr, _("device_script(\"%s\"): file descriptor preparation failure: %s\n"),
			scriptname, strerror(errno));
		goto out_destroy;
	}
	if (posix_spawn(&pid, scriptname, &fa, NULL, NULL, envp)) {
		fprintf(stderr, _("device_script(\"%s\"): script execution failure: %s\n"),
			scriptname, strerror(errno));
		goto out_destroy;
	}

	if (pid != waitpid(pid, &status, 0) && WIFEXITED(status) && !WEXITSTATUS(status)) {
		fprintf(stderr, _("device_script(\"%s\"): child script execution failure: %s, exit code=%d\n"), scriptname,
			(WIFEXITED(status) ? _("normal exit") : _("abnormal exit")),
			(WIFEXITED(status) ? WEXITSTATUS(status) : -1));
	} else {
		ret = 0;
	}

out_destroy:
	posix_spawn_file_actions_destroy(&fa);
out_free:
	free(env);
out:
	return ret;
}
