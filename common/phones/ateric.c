/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to at commands on ericsson
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.1  2001-11-19 13:03:18  pkot
  nk3110.c cleanup

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "misc.h"
#include "gsm-common.h"
#include "gsm-statemachine.h"
#include "phones/generic.h"
#include "phones/atgen.h"
#include "phones/ateric.h"
#include "links/atbus.h"
#include "links/cbus.h"


static GSM_Error GetMemoryStatus(GSM_Data *data,  GSM_Statemachine *state)
{
        char req[128];
        GSM_Error ret;
 
        ret = AT_SetMemoryType(data->MemoryStatus->MemoryType,  state);
        if (ret != GE_NONE)
                return ret;
        sprintf(req, "AT+CPBR=?\r\n");
        if (SM_SendMessage(state, 11, GOP_GetMemoryStatus, req) != GE_NONE)
                return GE_NOTREADY;
        ret = SM_Block(state, data, GOP_GetMemoryStatus);
        return ret;
}


static GSM_Error ReplyMemoryStatus(int messagetype, unsigned char *buffer, int length, GSM_Data *data)
{
	AT_LineBuffer buf;
        char *pos;
 
	buf.line1 = buffer;
	buf.length= length;
	splitlines(&buf);
	if (buf.line1 == NULL)
                return GE_INVALIDMEMORYTYPE;
        if (data->MemoryStatus) {
                if (strstr(buf.line2,"+CPBR")) {
                        pos = strchr(buf.line2, '-');
                        if (pos) {
                                data->MemoryStatus->Used = atoi(++pos);
                                data->MemoryStatus->Free = 0;
                        } else {
                                return GE_NOTSUPPORTED;
                        }
                }
        }
        return GE_NONE;
} 


void AT_InitEricsson(GSM_Statemachine *state, char *foundmodel, char *setupmodel) {
	AT_InsertRecvFunction(GOP_GetMemoryStatus, ReplyMemoryStatus);
	AT_InsertSendFunction(GOP_GetMemoryStatus, GetMemoryStatus);
}
