/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file:  rlp-crc24.h   Version 0.3.1

  Header file for CRC24 (aka FCS) implementation in RLP.

  $Log$
  Revision 1.2  2001-11-27 12:19:01  pkot
  Cleanup, indentation, ANSI complaint preprocesor symbols (Jan Kratochvil, me)

  Revision 1.1  2001/02/21 19:57:11  chris
  More fiddling with the directory layout


*/

#ifndef __data_rlp_crc24_h
#define __data_rlp_crc24_h

#ifndef __misc_h
#  include    "misc.h"
#endif

/* Prototypes for functions */

void RLP_CalculateCRC24Checksum(u8 *data, int length, u8 *crc);
bool RLP_CheckCRC24FCS(u8 *data, int length);

#endif
