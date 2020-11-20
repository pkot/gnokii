/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janik ml.

  Header file for CRC24 (aka FCS) implementation in RLP.

*/

#ifndef _gnokii_data_rlp_crc24_h
#define _gnokii_data_rlp_crc24_h

#include "misc.h"

/* Prototypes for functions */
void rlp_crc24checksum_calculate(uint8_t *data, int length, uint8_t *crc);
bool rlp_crc24fcs_check(uint8_t *data, int length);

#endif
