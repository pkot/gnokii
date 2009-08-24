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

  Copyright (C) 2002 BORBELY Zoltan, Markus Plail
  Copyright (C) 2002-2009 Pawel Kot

  Various compatibility functions.

*/

#include "config.h"
#include "compat.h"

#ifdef WIN32
#  include <windows.h>
#  include <sys/timeb.h>
#  include <time.h>
#  define ftime _ftime
#  define timeb _timeb

int setenv(const char *name, const char *value, int overwrite)
{
	return (int)SetEnvironmentVariable(name, value);
}

int unsetenv(const char *name)
{
	SetEnvironmentVariable(name, NULL);
	return 0;
}

#endif

#ifndef HAVE_SETENV
#  include <stdlib.h>
/* Implemented according to http://www.greenend.org.uk/rjk/2008/putenv.html and Linux manpage */
int setenv(const char *envname, const char *envvalue, int overwrite)
{
	char *str;

	if (!overwrite && getenv(envname))
		return 0;

	str = malloc(strlen(envname) + 1 + strlen(envvalue) + 1);
	if (str == NULL)
		return 1;

	sprintf(str, "%s=%s", envname, envvalue);

	if (putenv(str) != 0) {
		free(str);
		return 1;
	}

	return 0;
}

int unsetenv(const char *name)
{
	return setenv(name, "", 1);
}
#endif
                  
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#ifndef	HAVE_GETTIMEOFDAY

int gettimeofday(struct timeval *tv, void *tz)
{
	struct timeb t;

	ftime(&t);

	if (tv) {
		tv->tv_sec = t.time;
		tv->tv_usec = 1000 * t.millitm;
	}

	return 0;
}

#endif



#ifndef HAVE_STRSEP

/*
 * strsep() implementation for systems that don't have it.
 *
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>

/* 
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strsep.c	8.1 (Berkeley) 6/4/93";
#endif */  /* LIBC_SCCS and not lint */


/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */

char *strsep(char **stringp, const char *delim)
{
	register char *s;
	register const char *spanp;
	register int c, sc;
	char *tok;
	
	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
}

#endif

#ifndef HAVE_TIMEGM

#include <time.h>
#include <stdlib.h>

time_t timegm(struct tm *tm)
{
	time_t ret;
	char *tz;

	tz = getenv("TZ");
	setenv("TZ", "", 1);
	tzset();
	ret = mktime(tm);
	if (tz)
		setenv("TZ", tz, 1);
	else
		unsetenv("TZ");
	tzset();
	return ret;
}

#endif

#ifndef HAVE_STRNDUP
char *strndup(const char *src, size_t n)
{
	char *dst = malloc(n + 1);

	if (!dst)
		return NULL;

	dst[n] = '\0';
	return (char *)memcpy(dst, src, n);
}
#endif

