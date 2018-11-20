/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  Copyright (C) 2001       Jan Kratochvil
  Copyright (C) 2001-2011  Pawel Kot

  This file is part of gnokii.

*/

#include "compat.h"
#include "misc.h"
#include "gnokii.h"
#include "gnokii-internal.h"

#include <errno.h>
#include <sys/wait.h>

static void device_script_cfgfunc(const char *section, const char *key, const char *value)
{
	setenv(key, value, 1); /* errors ignored */
}

int device_script(int fd, int connect, struct gn_statemachine *state)
{
	const char *scriptname, *section;
	int status;
	pid_t pid;

	if (connect) {
		scriptname = state->config.connect_script;
		section = "connect_script";
	} else {
		scriptname = state->config.disconnect_script;
		section = "disconnect_script";
	}
	if (scriptname[0] == '\0')
		return 0;

	errno = 0;
	switch ((pid = fork())) {
	case -1:
		fprintf(stderr, _("device_script(\"%s\"): fork() failure: %s!\n"), scriptname, strerror(errno));
		return -1;

	case 0: /* child */
		cfg_foreach(section, device_script_cfgfunc);
		errno = 0;
		if (dup2(fd, 0) != 0 || dup2(fd, 1) != 1 || close(fd)) {
			fprintf(stderr, _("device_script(\"%s\"): file descriptor preparation failure: %s\n"), scriptname, strerror(errno));
			_exit(-1);
		}
		/* FIXME: close all open descriptors - how to track them?
		 */
		execl("/bin/sh", "sh", "-c", scriptname, NULL);
		fprintf(stderr, _("device_script(\"%s\"): script execution failure: %s\n"), scriptname, strerror(errno));
		_exit(-1);
		/* NOTREACHED */

	default:
		if (pid == waitpid(pid, &status, 0 /* options */) && WIFEXITED(status) && !WEXITSTATUS(status))
			return 0;
		fprintf(stderr, _("device_script(\"%s\"): child script execution failure: %s, exit code=%d\n"), scriptname,
			(WIFEXITED(status) ? _("normal exit") : _("abnormal exit")),
			(WIFEXITED(status) ? WEXITSTATUS(status) : -1));
		errno = EIO;
		return -1;

	}
	/* NOTREACHED */
}
