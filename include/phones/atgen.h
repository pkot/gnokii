/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for mobile phones.

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

  Copyright 2001 Manfred Jonsson <manfred.jonsson@gmx.de>

  This file provides functions specific to generic at command compatible
  phones. See README for more details on supported mobile phones.

*/

#ifndef __atgen_h_
#define __atgen_h_

typedef enum {
	GOPAT_GetCharset = GOP_Max,
	GOPAT_SetPDUMode,
	GOPAT_Prompt,
	GOPAT_Max	/* don't append anything after this entry */
} GSMAT_Operation;

typedef enum {
	CHARNONE,
	CHARUNKNOWN,
	CHARGSM,
	CHARCP437,
	CHARHEXGSM,
	CHARHEX437,
	CHARUCS2
} GSMAT_Charset;

typedef gn_error (*GSM_RecvFunctionType)(int type, unsigned char *buffer, int length, GSM_Data *data, GSM_Statemachine *state);
typedef gn_error (*AT_SendFunctionType)(GSM_Data *data, GSM_Statemachine *s);

typedef struct {
	char *line1;
	char *line2;
	char *line3;
	char *line4; /* When reading SMS there are 4 ouput lines. Maybe create a table here? */
	int length;
} AT_LineBuffer;

GSM_RecvFunctionType AT_InsertRecvFunction(int type, GSM_RecvFunctionType func);
AT_SendFunctionType AT_InsertSendFunction(int type, AT_SendFunctionType func);

gn_error AT_SetMemoryType(GSM_MemoryType mt, GSM_Statemachine *state);

void splitlines(AT_LineBuffer *buf);

char *skipcrlf(unsigned char *str);
char *findcrlf(unsigned char *str, int test, int maxlength);

#endif
