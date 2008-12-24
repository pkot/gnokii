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

  Copyright (C) 2004 BORBELY Zoltan, Pawel Kot

  This file contains the encoding/decoding functions for the basic types.

*/

#include "config.h"
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gnokii.h"
#include "gnokii-internal.h"
#include "pkt.h"


void pkt_buffer_set(pkt_buffer *buf, void *addr, int32_t len)
{
	buf->addr = addr;
	buf->size = len;
	buf->offs = 0;
}

static uint8_t *buffer_expand(pkt_buffer *buf, int32_t len)
{
	uint8_t *a;

	assert(buf->size >= buf->offs + len);

	a = buf->addr + buf->offs;
	buf->offs += len;

	return a;
}

void pkt_put_int8(pkt_buffer *buf, int8_t x)
{
	uint8_t *b = buffer_expand(buf, 1);

	b[0] = (uint8_t)x;
}

void pkt_put_int16(pkt_buffer *buf, int16_t x)
{
	uint8_t *b = buffer_expand(buf, 2);

	b[0] = (uint8_t)((x >> 8) & 0xff);
	b[1] = (uint8_t)(x & 0xff);
}

void pkt_put_int32(pkt_buffer *buf, int32_t x)
{
	uint8_t *b = buffer_expand(buf, 4);

	b[0] = (uint8_t)((x >> 24) & 0xff);
	b[1] = (uint8_t)((x >> 16) & 0xff);
	b[2] = (uint8_t)((x >> 8) & 0xff);
	b[3] = (uint8_t)(x & 0xff);
}

void pkt_put_uint8(pkt_buffer *buf, uint8_t x)
{
	uint8_t *b = buffer_expand(buf, 1);

	b[0] = x;
}

void pkt_put_uint16(pkt_buffer *buf, uint16_t x)
{
	uint8_t *b = buffer_expand(buf, 2);

	b[0] = ((x >> 8) & 0xff);
	b[1] = (x & 0xff);
}

void pkt_put_uint32(pkt_buffer *buf, uint32_t x)
{
	uint8_t *b = buffer_expand(buf, 4);

	b[0] = ((x >> 24) & 0xff);
	b[1] = ((x >> 16) & 0xff);
	b[2] = ((x >> 8) & 0xff);
	b[3] = (x & 0xff);
}

void pkt_put_string(pkt_buffer *buf, const char *x)
{
	uint16_t n = strlen(x);
	uint8_t *b;
	unsigned int len;
	char *aux = malloc(n);

	if (!aux)
		return;

	len = char_unicode_encode(aux, x, n);

	/*
	 * gnapplet expects number of chars to be read.
	 * char_unicode_encode() encodes every char on two bytes.
	 * strlen() doesn't return correct values for utf8 strings.
	 */
	pkt_put_uint16(buf, len / 2);

	b = buffer_expand(buf, len);
	memcpy(b, aux, len);
	free(aux);
}

void pkt_put_timestamp(pkt_buffer *buf, const gn_timestamp *x)
{
	pkt_put_uint16(buf, x->year);
	pkt_put_uint8(buf, x->month);
	pkt_put_uint8(buf, x->day);
	pkt_put_uint8(buf, x->hour);
	pkt_put_uint8(buf, x->minute);
	pkt_put_uint8(buf, x->second);
}

void pkt_put_bool(pkt_buffer *buf, int x)
{
	pkt_put_uint8(buf, x ? 1 : 0);
}

void pkt_put_bytes(pkt_buffer *buf, const uint8_t *x, uint16_t n)
{
	uint8_t *b;

	pkt_put_uint16(buf, n);

	b = buffer_expand(buf, n);
	memcpy(b, x, n);
}

int8_t pkt_get_int8(pkt_buffer *buf)
{
	uint8_t *b = buffer_expand(buf, 1);

	return (int8_t)b[0];
}

int16_t pkt_get_int16(pkt_buffer *buf)
{
	uint8_t *b = buffer_expand(buf, 2);

	return (int16_t)((b[0] << 8) + b[1]);
}

int32_t pkt_get_int32(pkt_buffer *buf)
{
	uint8_t *b = buffer_expand(buf, 4);

	return (int32_t)((b[0] << 24) + (b[1] << 16) + (b[2] << 8) + b[3]);
}

uint8_t pkt_get_uint8(pkt_buffer *buf)
{
	uint8_t *b = buffer_expand(buf, 1);

	return b[0];
}

uint16_t pkt_get_uint16(pkt_buffer *buf)
{
	uint8_t *b = buffer_expand(buf, 2);

	return (b[0] << 8) + b[1];
}

uint32_t pkt_get_uint32(pkt_buffer *buf)
{
	uint8_t *b = buffer_expand(buf, 4);

	return (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + b[3];
}

char *pkt_get_string(char *s, int slen, pkt_buffer *buf)
{
	uint16_t l;
	uint8_t *b;

	l = pkt_get_uint16(buf);
	b = buffer_expand(buf, 2 * l);

	char_unicode_decode(s, b, l < slen - 1 ? 2 * l : 2 * slen - 2);

	return s;
}

gn_timestamp *pkt_get_timestamp(gn_timestamp *t, pkt_buffer *buf)
{
	t->year = pkt_get_uint16(buf);
	t->month = pkt_get_uint8(buf);
	t->day = pkt_get_uint8(buf);
	t->hour = pkt_get_uint8(buf);
	t->minute = pkt_get_uint8(buf);
	t->second = pkt_get_uint8(buf);
	t->timezone = 0;

	return t;
}

int pkt_get_bool(pkt_buffer *buf)
{
	return pkt_get_uint8(buf) ? (1 == 1) : 0;
}

uint16_t pkt_get_bytes(uint8_t *s, int len, pkt_buffer *buf)
{
	uint16_t l;
	uint8_t *b;

	l = pkt_get_uint16(buf);
	assert(l <= len);
	b = buffer_expand(buf, l);
	memcpy(s, b, l);

	return l;
}
