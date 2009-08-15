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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001-2004  Pawel Kot
  Copyright (C) 2002-2004  BORBELY Zoltan

  This file provides routines to handle processing of data when connected in
  fax or data mode. Converts data from/to GSM phone to virtual modem
  interface.

*/

#define		__data_datapump_c

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "misc.h"
#include "gnokii.h"
#include "compat.h"
#include "device.h"
#include "data/at-emulator.h"
#include "data/datapump.h"

/* Prototypes */
static int	DP_CallBack(rlp_user_inds ind, unsigned char *buffer, int length);
static int	DP_SendRLPFrame(gn_rlp_f96_frame *frame, int out_dtx);

/* Global variables */
extern bool CommandMode;

/* Local variables */
static int	PtyRDFD;	/* File descriptor for reading and writing to/from */
static int	PtyWRFD;	/* pty interface - only different in debug mode. */
u8 pluscount;
bool connected;

bool dp_Initialise(int read_fd, int write_fd)
{
	PtyRDFD = read_fd;
	PtyWRFD = write_fd;
	rlp_initialise(DP_SendRLPFrame, DP_CallBack);
	rlp_user_request_set(Attach_Req, true);
	pluscount = 0;
	connected = false;
	data.rlp_rx_callback = rlp_f96_frame_display;
	gn_sm_functions(GN_OP_SetRLPRXCallback, &data, sm);

	return true;
}


static int DP_CallBack(rlp_user_inds ind, unsigned char *buffer, int length)
{
	int i, temp;

	switch(ind) {
	case Data:
		if (CommandMode == false) write(PtyWRFD, buffer, length);
		break;
	case Conn_Ind:
		if (CommandMode == false) gn_atem_modem_result(MR_CARRIER);
		rlp_user_request_set(Conn_Req, true);
		break;
	case StatusChange:
		if (buffer[0] == 0) {
			connected = true;
			if (CommandMode == false) gn_atem_modem_result(MR_CONNECT);
		}
		break;
	case Disc_Ind:
		if (CommandMode == false) gn_atem_modem_result(MR_NOCARRIER);
		connected = false;
		/* Set the call passup back to the at emulator */
		data.call_notification = gn_atem_call_passup;
		gn_sm_functions(GN_OP_SetCallNotification, &data, sm);
		CommandMode = true;
		break;
	case Reset_Ind:
		rlp_user_request_set(Reset_Resp, true);
		break;
	case GetData:
		if (queue.n > 0) {
			temp = queue.n < sizeof(buffer) ? queue.n : sizeof(buffer);
			for (i = 0; i < temp; i++) {
				buffer[i] = queue.buf[queue.head++];
				queue.head %= sizeof(queue.buf);
				queue.n--;
			}

			/* This will only check +++ and the beginning of a read */
			/* But there should be a pause before it anyway */

			if (buffer[0] == '+') {
				pluscount++;
				if (temp > 1) {
					if (buffer[1] == '+') pluscount++;
					else pluscount = 0;
					if (temp > 2) {
						if (buffer[2] == '+') pluscount++;
						else pluscount = 0;
						if (temp > 3) pluscount = 0;
					}
				}
			} else pluscount = 0;

			if (pluscount == 3) {
				CommandMode = true;
				/* Set the call passup back to the at emulator */
				data.call_notification = gn_atem_call_passup;
				gn_sm_functions(GN_OP_SetCallNotification, &data, sm);
				gn_atem_string_out("\r\n");
				gn_atem_modem_result(MR_OK);
				break;
			}

			return temp;
		}
		break;
	default:
		break;
	}
	return 0;
}

void dp_CallPassup(gn_call_status CallStatus, gn_call_info *CallInfo, struct gn_statemachine *state, void *callback_data)
{
	dprintf("dp_CallPassup called with %d\n", CallStatus);

	switch (CallStatus) {
	case GN_CALL_Established:
		if (CommandMode == false) gn_atem_modem_result(MR_CARRIER);
		rlp_user_request_set(Conn_Req, true);
		connected = true;
		break;
	case GN_CALL_LocalHangup:
	case GN_CALL_RemoteHangup:
		CommandMode = true;
		/* Set the call passup back to the at emulator */
		data.call_notification = gn_atem_call_passup;
		gn_sm_functions(GN_OP_SetCallNotification, &data, sm);
		gn_atem_modem_result(MR_NOCARRIER);
		rlp_user_request_set(Disc_Req, true);
		connected = false;
		/* send the hangup event to the at emulator */
		gn_atem_call_passup(CallStatus, CallInfo, state, NULL);
		break;
	default:
		break;
	}
}

static int DP_SendRLPFrame(gn_rlp_f96_frame *frame, int out_dtx)
{
	data.rlp_frame = frame;
	data.rlp_out_dtx = out_dtx;

	return gn_sm_functions(GN_OP_SendRLPFrame, &data, sm);
}
