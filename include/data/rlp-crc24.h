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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.

  Header file for CRC24 (aka FCS) implementation in RLP.

*/

#ifndef _gnokii_data_rlp_crc24_h
#define _gnokii_data_rlp_crc24_h

#include "misc.h"

/* Prototypes for functions */
void rlp_crc24checksum_calculate(u8 *data, int length, u8 *crc);
bool rlp_crc24fcs_check(u8 *data, int length);

#endif
