/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "misc.h"


API void (*GSM_ELogHandler)(const char *fmt, va_list ap) = NULL;

API int GetLine(FILE *File, char *Line, int count)
{
	char *ptr;

	if (fgets(Line, count, File)) {
		ptr = Line + strlen(Line) - 1;

		while ((*ptr == '\n' || *ptr == '\r') && ptr >= Line)
			*ptr-- = '\0';

		return strlen(Line);
	} else {
		return 0;
	}
}

static PhoneModel models[] = {
	{NULL,    "", 0 },
	{"2711",  "?????", PM_SMS },		/* Dancall */
	{"2731",  "?????", PM_SMS },
	{"1611",  "NHE-5", 0 },
	{"2110i", "NHE-4", PM_SMS },
	{"2148i", "NHK-4", 0 },
	{"3110",  "0310" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-8 */
	{"3210",  "NSE-8", PM_SMS | PM_DTMF },
	{"3210",  "NSE-9", PM_SMS | PM_DTMF },
	{"3310",  "NHM-5", PM_SMS | PM_DTMF },
	{"3330",  "NHM-6", PM_SMS | PM_DTMF },
	{"3360",  "NPW-6", PM_SMS | PM_DTMF },
	{"3810",  "0305" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-9 */
	{"5110",  "NSE-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5120",  "NSC-1", PM_KEYBOARD  },
	{"5130",  "NSK-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5160",  "NSW-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"5190",  "NSB-1", PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6110",  "NSE-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6120",  "NSC-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6130",  "NSK-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6150",  "NSM-1", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"616x",  "NSW-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6185",  "NSD-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6190",  "NSB-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"6210",  "NPE-3", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6250",  "NHM-3", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6310",  "NPE-4", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6510",  "NPM-9", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6310i", "NPL-1", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"6310",  "NRE-4", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7110",  "NSE-5", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"7650",  "NHL-2NA", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"8210",  "NSM-3", PM_CALLERGROUP | PM_CALENDAR | PM_NETMONITOR | PM_KEYBOARD | PM_SMS | PM_DTMF | PM_DATA | PM_SPEEDDIAL | PM_AUTHENTICATION },
	{"8310",  "NHM-7", PM_CALLERGROUP | PM_CALENDAR | PM_SPEEDDIAL | PM_NETMONITOR | PM_EXTPBK | PM_SMS | PM_FOLDERS },
	{"8810",  "NSE-6", PM_SMS | PM_DTMF | PM_DATA },
	{"8110i", "0423",  PM_SMS | PM_DTMF | PM_DATA }, /* Guess for NHE-6 */
	{"8110",  "0423" , PM_SMS | PM_DTMF | PM_DATA }, /* NHE-6BX */
	{"9000i", "RAE-4", 0 },
	{"9110",  "RAE-2", 0 },
	{"9210",  "RAE-2", 0 },
	{"550",	  "THF-10", 0 },
	{"540",   "THF-11", 0 },
	{"650",   "THF-12", 0 },
	{"640",   "THF-13", 0 },
	{"RPM-1", "Nokia Card Phone RPM-1 GSM900/1800", PM_SMS | PM_DTMF | PM_DATA | PM_AUTHENTICATION },
	{NULL,    NULL, 0 }
};

PhoneModel *GetPhoneModel (const char *num)
{
	register int i = 0;

	while (models[i].number != NULL) {
		if (strcmp(num, models[i].number) == 0) {
			dprintf("Found model \"%s\"\n", num);
			return (&models[i]);
		} else {
			dprintf("comparing \"%s\" and \"%s\"\n", num, models[i].number);
		}
		i++;
	}

	return (&models[0]);
}

char *GetModel (const char *num)
{
	return (GetPhoneModel(num)->model);
}

void GSM_WriteErrorLog(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (GSM_ELogHandler) {
		GSM_ELogHandler(fmt, ap);
	} else {
#ifndef	DEBUG
		vfprintf(stderr, fmt, ap);
		fflush(stderr);
#endif
	}

	va_end(ap);
}

#define max_buf_len 128
#define lock_path "/var/lock/LCK.."

/* Lock the device. Return allocated string with a lock name */
char *lock_device(const char* port)
{
#ifndef WIN32
	char *lock_file = NULL;
	char buffer[max_buf_len];
	const char *aux = strrchr(port, '/');
	int fd, len = strlen(aux) + strlen(lock_path);

	/* Remove leading '/' */
	if (aux) aux++;
	else aux = port;

	memset(buffer, 0, sizeof(buffer));
	lock_file = calloc(len + 1, 1);
	if (!lock_file) {
		fprintf(stderr, _("Out of memory error while locking device\n"));
		return NULL;
	}
	/* I think we don't need to use strncpy, as we should have enough
	 * buffer due to strlen results
	 */
	strcpy(lock_file, lock_path);
	strcat(lock_file, aux);

	/* Check for the stale lockfile.
	 * The code taken from minicom by Miquel van Smoorenburg */
	if ((fd = open(lock_file, O_RDONLY)) >= 0) {
		char buf[max_buf_len];
		int pid, n = 0;

		n = read(fd, buf, sizeof(buf) - 1);
		close(fd);
		if (n > 0) {
			pid = -1;
			if (n == 4)
				/* Kermit-style lockfile. */
				pid = *(int *)buf;
			else {
				/* Ascii lockfile. */
				buf[n] = 0;
				sscanf(buf, "%d", &pid);
			}
			if (pid > 0 && kill((pid_t)pid, 0) < 0 && errno == ESRCH) {
				fprintf(stderr, _("Lockfile %s is stale. Overriding it..\n"), lock_file);
				sleep(1);
				if (unlink(lock_file) == -1) {
					fprintf(stderr, _("Overriding failed, please check the permissions\n"));
					fprintf(stderr, _("Cannot lock device\n"));
					goto failed;
				}
			} else {
				fprintf(stderr, _("Device already locked.\n"));
				goto failed;
			}
		}
		/* this must not happen. because we could open the file   */
		/* no wrong permissions are set. only reason could be     */
		/* flock/lockf or a empty lockfile due to a broken binary */
		/* which is more likely (like gnokii 0.4.0pre11 ;-)       */
		if (n == 0) {
			fprintf(stderr, _("Unable to read lockfile %s.\n"), lock_file);
			fprintf(stderr, _("Please check for reason and remove the lockfile by hand.\n"));
			fprintf(stderr, _("Cannot lock device\n"));
			goto failed;
		}
	}

	/* Try to create a new file, with 0644 mode */
	fd = open(lock_file, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (fd == -1) {
		if (errno == EEXIST)
			fprintf(stderr, _("Device seems to be locked by unknown process\n"));
		else if (errno == EACCES)
			fprintf(stderr, _("Please check permission on lock directory\n"));
		else if (errno == ENOENT)
			fprintf(stderr, _("Cannot create lockfile %s. Please check for existence of path"), lock_file);
		goto failed;
	}
	sprintf(buffer, "%10ld gnokii\n", (long)getpid());
	write(fd, buffer, strlen(buffer));
	close(fd);
	return lock_file;
failed:
	free(lock_file);
	return NULL;
#else
	return NULL;
#endif /* WIN32 */
}

/* Removes lock and frees memory */
bool unlock_device(char *lock_file)
{
#ifndef WIN32
	int err;

	if (!lock_file) {
		fprintf(stderr, _("Cannot unlock device\n"));
		return false;
	}
	err = unlink(lock_file);
	free(lock_file);
	return (err + 1);
#else
	return true;
#endif /* WIN32 */
}
