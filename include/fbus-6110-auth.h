/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for the Nokia authentication protocol.

  This code is written specially for gnokii project by Odinokov Serge.
  If you have some special requests for Serge just write him to
  apskaita@post.omnitel.net or serge@takas.lt
      
  Reimplemented in C by Pavel Janík ml.
        
  Last modification: Mon Mar 20 21:51:59 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __fbus_6110_auth_h
#define __fbus_6110_auth_h

void FB61_GetNokiaAuth( unsigned char *Imei,
                        unsigned char *MagicBytes,
                        unsigned char *MagicResponse );

#endif /* __fbus_6110_auth_h */
