/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides Nokia authentication protocol.

  This code is written specially for gnokii project by Odinokov Serge.
  If you have some special requests for Serge just write him to
  apskaita@post.omnitel.net or serge@takas.lt

  Reimplemented in C by Pavel Janík ml.

  Last modification: Mon Mar 20 21:51:59 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include "fbus-6110-auth.h"

/* Nokia authentication protocol is used in the communication between Nokia
   mobile phones (e.g. Nokia 6110) and Nokia Cellular Data Suite software,
   commercially sold by Nokia Corp.

   The authentication scheme is based on the token send by the phone to the
   software. The software does it's magic (see the function
   FB61_GetNokiaAuth()) and returns the result back to the phone. If the
   result is correct the phone responds with the message "Accessory
   connected!" displayed on the LCD. Otherwise it will display "Accessory not
   supported" and some functions will not be available for use.

   The specification of the protocol is not publicly available, no comment. */

void FB61_GetNokiaAuth(unsigned char *Imei, unsigned char *MagicBytes, unsigned char *MagicResponse)
{

  int i, j, CRC=0;

  /* This is our temporary working area. */

  unsigned char Temp[16];

  /* Here we put FAC (Final Assembly Code) and serial number into our area. */

  Temp[0]  = Imei[6];
  Temp[1]  = Imei[7];
  Temp[2]  = Imei[8];
  Temp[3]  = Imei[9];
  Temp[4]  = Imei[10];
  Temp[5]  = Imei[11];
  Temp[6]  = Imei[12];
  Temp[7]  = Imei[13];

  /* And now the TAC (Type Approval Code). */

  Temp[8]  = Imei[2];
  Temp[9]  = Imei[3];
  Temp[10] = Imei[4];
  Temp[11] = Imei[5];

  /* And now we pack magic bytes from the phone. */

  Temp[12] = MagicBytes[0];
  Temp[13] = MagicBytes[1];
  Temp[14] = MagicBytes[2];
  Temp[15] = MagicBytes[3];

  for (i=0; i<=11; i++)
    if (Temp[i + 1]& 1)
      Temp[i]<<=1;

  switch (Temp[15] & 0x03) {

  case 1:
  case 2:
    j = Temp[13] & 0x07;

    for (i=0; i<=3; i++)
      Temp[i+j] ^= Temp[i+12];

    break;

  default:
    j = Temp[14] & 0x07;

    for (i=0; i<=3; i++)
      Temp[i + j] |= Temp[i + 12];
  }

  for (i=0; i<=15; i++)
    CRC ^= Temp[i];

  for (i=0; i<=15; i++) {

    switch (Temp[15 - i] & 0x06) {

    case 0:
      j = Temp[i] | CRC;
      break;

    case 2:
    case 4:
      j = Temp[i] ^ CRC;
      break;

    case 6:
      j = Temp[i] & CRC;
      break;
    }
  
    if (j == CRC)
      j = 0x2c;

    if (Temp[i] == 0)
      j = 0;

    MagicResponse[i] = j;

  }
}
