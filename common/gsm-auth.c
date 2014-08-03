/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2011 Paweł Kot

  Functions for initialization authentication.

*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "gnokii-internal.h"
#include "gnokii.h"

static int get_password(const char *prompt, char *pass, int length)
{
	int fd = fileno(stdin);
	int count;

	if (isatty(fd)) {
#ifdef HAVE_GETPASS
		const char *s;

		/* FIXME: man getpass says it was removed in POSIX.1-2001 */
		s = getpass(prompt);
		if (s) {
			strncpy(pass, s, length - 1);
			pass[length - 1] = '\0';

			return strlen(pass);
		}
		return -1;
#else
		fprintf(stdout, "%s", prompt);
#endif
	}
	if (fgets(pass, length, stdin)) {
		/* Strip trailing newline like getpass() would do */
		for (count = 0; pass[count] && pass[count] != '\n'; count++)
			;
		pass[count] = '\0';

		return count;
	}
	return -1;
}

/* This function is called only by auth_pin(). No need for any sanity checks here */
static gn_error auth_pin_interactive(gn_data *data, struct gn_statemachine *state)
{
	gn_security_code *security_code = data->security_code;
	char prompt[64];

	memset(&security_code->code, 0, sizeof(security_code->code));
	switch (security_code->type) {
	case GN_SCT_Pin:
	case GN_SCT_Puk:
	case GN_SCT_Pin2:
	case GN_SCT_Puk2:
	case GN_SCT_SecurityCode:
		/*
		 * Translators: %s is replaced by the translation of "Security code",
		 * "PIN", "PIN2", "PUK", "PUK2". The first "Enter" is a verb, the
		 * second "Enter" is the noun of a key on a PC keyboard.
		 */
		snprintf(prompt, sizeof(prompt), _("Enter your %s (press Enter to cancel): "), gn_security_code_type2str(security_code->type));
		if (get_password(prompt, security_code->code, sizeof(security_code->code)) < 1)
			return GN_ERR_USERCANCELED;
		break;
	default:
		return GN_ERR_NOTSUPPORTED;
	}
	return GN_ERR_NONE;
}

/*
 * Returns 1 on success and 0 on failure.
 */
static int read_security_code_from_file(const char *path, gn_security_code *sc)
{
	struct stat buf;
	FILE *f;
	char line[32];
	int cnt = 0;

	/* file handling */
	if (stat(path, &buf) != 0) {
		dprintf("File with the security code not found.\n");
		return 0;
	}
	if ((buf.st_mode & S_IRWXG) != 0 || (buf.st_mode & S_IRWXO) != 0) {
		dprintf("File with the security code cannot be world or group readable.\n");
		return 0;
	}

	f = fopen(path, "r");
	while (fgets(line, sizeof(line), f) != NULL) {
		switch (sc->type) {
		case GN_SCT_SecurityCode:
			cnt = sscanf(line, "SEC:%s", sc->code);
			break;
		case GN_SCT_Pin:
			cnt = sscanf(line, "PIN:%s", sc->code);
			break;
		case GN_SCT_Pin2:
			cnt = sscanf(line, "PIN2:%s", sc->code);
			break;
		case GN_SCT_Puk:
			cnt = sscanf(line, "PUK:%s", sc->code);
			break;
		case GN_SCT_Puk2:
			cnt = sscanf(line, "PUK2:%s", sc->code);
			break;
		default:
			break;
		}
		if (cnt == 1)
			break;
	}
	fclose(f);

	if (cnt == 0)
		dprintf("Could not find %s in file %s.\n", gn_security_code_type2str(sc->type), path);

	return cnt;
}

static gn_error auth_pin(gn_auth_type auth_type, gn_data *data, struct gn_statemachine *state)
{
	gn_error err = GN_ERR_NONE;
	const char *path = state->config.auth_file;

	if (!read_security_code_from_file(path, data->security_code)) {
		if (auth_type != GN_AUTH_TYPE_NONINTERACTIVE) {
			dprintf("Falling back to interactive mode.\n");
			err = auth_pin_interactive(data, state);
		} else {
			err = GN_ERR_NOTAVAILABLE;
		}
	}
	return err;
}

gn_error do_auth(gn_auth_type auth_type, struct gn_statemachine *state)
{
	gn_error err;
	gn_data *data;
	gn_security_code sc;

	data = calloc(1, sizeof(gn_data));
	data->security_code = &sc;

	err = gn_sm_functions(GN_OP_GetSecurityCodeStatus, data, state);
	/* GN_ERR_SIMPROBLEM can be returned in the following cases:
	 *  - CME ERROR: 10 - SIM not inserted
	 *  - CME ERROR: 13 - SIM failure
	 *  - CME ERROR: 15 - SIM wrong
	 * We should ignore these situations. If there is an real error the
	 * next command will detect it anyway.  But if it is just SIM not
	 * inserted (we cannot distinguish here between these three
	 * situations), gnokii is still usable.
	 */
	if (err != GN_ERR_NONE) {
		if (err == GN_ERR_SIMPROBLEM)
			err = GN_ERR_NONE;
		goto out;
	}

	switch (sc.type) {
	case GN_SCT_SecurityCode:
	case GN_SCT_Pin:
	case GN_SCT_Pin2:
	case GN_SCT_Puk:
	case GN_SCT_Puk2:
		break;
	case GN_SCT_None:
		/* err is GN_ERR_NONE but this is to make it explicit */
		err = GN_ERR_NONE;
		goto out;
	default:
		err = GN_ERR_NOTSUPPORTED;
		goto out;
	}

	switch (auth_type) {
	case GN_AUTH_TYPE_TEXT:
		err = auth_pin(auth_type, data, state);
		break;
	case GN_AUTH_TYPE_INTERACTIVE:
	case GN_AUTH_TYPE_NONINTERACTIVE:
		if (!state->callbacks.auth_interactive)
			err = auth_pin(auth_type, data, state);
		else
			err = state->callbacks.auth_interactive(data, state);
		break;
	case GN_AUTH_TYPE_NONE:
	case GN_AUTH_TYPE_BINARY:
		err = GN_ERR_NONE;
		break;
	default:
		err = GN_ERR_NOTSUPPORTED;
		break;
	}
	if (err == GN_ERR_NONE)
		err = gn_sm_functions(GN_OP_EnterSecurityCode, data, state);
out:
	free(data);
	return err;
}

GNOKII_API void gn_auth_interactive_register(gn_auth_interactive_func_t auth_func, struct gn_statemachine *state)
{
	state->callbacks.auth_interactive = auth_func;
}
