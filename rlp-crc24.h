/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings & Pavel Janík ml., 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file:  rlp-crc24.h   Version 0.3.1

  Header file for CRC24 (aka FCS) implementation in RLP.

  Last modification: Sun Oct 24 17:52:32 CEST 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __rlp_crc24_h
#define __rlp_crc24_h

#ifndef __misc_h
  #include    "misc.h"
#endif

/* Prototypes for functions */

void RLP_CalculateCRC24Checksum(u8 *data, int length, u8 *crc);
bool RLP_CheckCRC24FCS(u8 *data, int length);

#endif
