/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

  $Log$
  Revision 1.1  2001-11-19 13:03:18  pkot
  nk3110.c cleanup


*/

typedef GSM_Error (*GSM_RecvFunctionType)(int type, unsigned char *buffer, int length, GSM_Data *data);
typedef GSM_Error (*AT_SendFunctionType)(GSM_Data *data, GSM_Statemachine *s);

typedef struct {
        char *line1;
        char *line2;
        char *line3;
        int length;
} AT_LineBuffer;

GSM_RecvFunctionType AT_InsertRecvFunction(int type, GSM_RecvFunctionType func);
AT_SendFunctionType AT_InsertSendFunction(int type, AT_SendFunctionType func);

GSM_Error AT_SetMemoryType(GSM_MemoryType mt, GSM_Statemachine *state);

void splitlines(AT_LineBuffer *buf);

char *skipcrlf(char *str);
char *findcrlf(char *str, int test, int maxlength);

