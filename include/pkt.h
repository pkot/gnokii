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

  Copyright (C) 2004 BORBELY Zoltan

  This file contains the encoding/decoding functions for the basic types.

*/

#ifndef _gnokii_pkt_h
#define _gnokii_pkt_h

#include "compat.h"

typedef struct {
	uint8_t *addr;
	int32_t size;
	int32_t offs;
} pkt_buffer;

void pkt_buffer_set(pkt_buffer *buf, void *addr, int32_t len);

void pkt_put_int8(pkt_buffer *buf, int8_t x);
void pkt_put_int16(pkt_buffer *buf, int16_t x);
void pkt_put_int32(pkt_buffer *buf, int32_t x);
void pkt_put_uint8(pkt_buffer *buf, uint8_t x);
void pkt_put_uint16(pkt_buffer *buf, uint16_t x);
void pkt_put_uint32(pkt_buffer *buf, uint32_t x);
void pkt_put_string(pkt_buffer *buf, const char *x);
void pkt_put_timestamp(pkt_buffer *buf, const gn_timestamp *x);
void pkt_put_bool(pkt_buffer *buf, int x);
void pkt_put_bytes(pkt_buffer *buf, const uint8_t *x, uint16_t n);

int8_t pkt_get_int8(pkt_buffer *buf);
int16_t pkt_get_int16(pkt_buffer *buf);
int32_t pkt_get_int32(pkt_buffer *buf);
uint8_t pkt_get_uint8(pkt_buffer *buf);
uint16_t pkt_get_uint16(pkt_buffer *buf);
uint32_t pkt_get_uint32(pkt_buffer *buf);
char *pkt_get_string(char *s, int slen, pkt_buffer *buf);
gn_timestamp *pkt_get_timestamp(gn_timestamp *t, pkt_buffer *buf);
int pkt_get_bool(pkt_buffer *buf);
uint16_t pkt_get_bytes(uint8_t *s, int len, pkt_buffer *buf);

#endif
