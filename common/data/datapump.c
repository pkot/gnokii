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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  This file provides routines to handle processing of data when connected in
  fax or data mode. Converts data from/to GSM phone to virtual modem
  interface.

*/

#define		__data_datapump_c


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
#include <unistd.h>
#include <pthread.h>


#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "device.h"
#include "data/at-emulator.h"
#include "data/virtmodem.h"
#include "data/datapump.h"
#include "data/rlp-common.h"

/* Prototypes */
static int	DP_CallBack(RLP_UserInds ind, u8 *buffer, int length);
static int	DP_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);
static void	*DP_ThreadLoop(void *v);

/* Global variables */
extern bool CommandMode;

/* Local variables */
static int	PtyRDFD;	/* File descriptor for reading and writing to/from */
static int	PtyWRFD;	/* pty interface - only different in debug mode. */
static int	rfds_n;
static fd_set	rfds;
static pthread_t dp_thread = 0;
u8 pluscount;
bool connected;

bool DP_Initialise(int read_fd, int write_fd)
{
	PtyRDFD = read_fd;
	PtyWRFD = write_fd;
	FD_ZERO(&rfds);
	FD_SET(PtyRDFD, &rfds);
	rfds_n = PtyRDFD + 1;
	RLP_Initialise(DP_SendRLPFrame, DP_CallBack);
	RLP_SetUserRequest(Attach_Req, true);
	pluscount = 0;
	connected = false;
	data.RLP_RX_Callback = RLP_DisplayF96Frame;
	SM_Functions(GOP_SetRLPRXCallback, &data, sm);

	if (dp_thread == 0)
		if (pthread_create(&dp_thread, NULL, DP_ThreadLoop, NULL) != 0) {
			dprintf("Cannot create DP thread\n");
			return false;
		}

	return true;
}


static int DP_CallBack(RLP_UserInds ind, u8 *buffer, int length)
{
	int temp;
	struct timeval tv;
	fd_set t_rfds;

	switch(ind) {
	case Data:
		if (CommandMode == false) write(PtyWRFD, buffer, length);
		break;
	case Conn_Ind:
		if (CommandMode == false) ATEM_ModemResult(MR_CARRIER);
		RLP_SetUserRequest(Conn_Req, true);
		break;
	case StatusChange:
		if (buffer[0] == 0) {
			connected = true;
			if (CommandMode == false) ATEM_ModemResult(MR_CONNECT);
		}
		break;
	case Disc_Ind:
		if (CommandMode == false) ATEM_ModemResult(MR_NOCARRIER);
		connected = false;
		/* Set the call passup back to the at emulator */
		data.CallNotification = ATEM_CallPassup;
		SM_Functions(GOP_SetCallNotification, &data, sm);
		CommandMode = true;
		break;
	case Reset_Ind:
		RLP_SetUserRequest(Reset_Resp, true);
		break;
	case GetData:
		memset(&tv, 0, sizeof(tv));
		memcpy(&t_rfds, &rfds, sizeof(t_rfds));
		if (select(rfds_n, &t_rfds, NULL, NULL, &tv) != 0) {

			/* Check if the program has closed */
			/* Return to command mode */
			/* Note that the call will still be in progress, */
			/* as with a normal modem (I think) */

			if (FD_ISSET(PtyRDFD, &t_rfds))
				temp = read(PtyRDFD, buffer, length);
			else
				temp = 0;

			if (temp == -1 && errno == EINTR) return 0;

			if (temp <= 0) {
				CommandMode = true;
				/* Set the call passup back to the at emulator */
				data.CallNotification = ATEM_CallPassup;
				SM_Functions(GOP_SetCallNotification, &data, sm);
				return 0;
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
				data.CallNotification = ATEM_CallPassup;
				SM_Functions(GOP_SetCallNotification, &data, sm);
				ATEM_ModemResult(MR_OK);
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

void DP_CallPassup(GSM_CallStatus CallStatus, GSM_CallInfo *CallInfo)
{
	dprintf("DP_CallPassup called with %d\n", CallStatus);

	switch (CallStatus) {
	case GSM_CS_Established:
		if (CommandMode == false) ATEM_ModemResult(MR_CARRIER);
		connected = true;
		break;
	case GSM_CS_LocalHangup:
	case GSM_CS_RemoteHangup:
		CommandMode = true;
		/* Set the call passup back to the at emulator */
		data.CallNotification = ATEM_CallPassup;
		SM_Functions(GOP_SetCallNotification, &data, sm);
		ATEM_ModemResult(MR_NOCARRIER);
		RLP_SetUserRequest(Disc_Req, true);
		connected = false;
		break;
	default:
		break;
	}
}

static int DP_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
	data.RLP_Frame = frame;
	data.RLP_OutDTX = out_dtx;

	return SM_Functions(GOP_SendRLPFrame, &data, sm);
}

static void *DP_ThreadLoop(void *v)
{
	for (;;)
	{
		if (!CommandMode) SM_Loop(sm, 1); else usleep(100000);
	}
}
