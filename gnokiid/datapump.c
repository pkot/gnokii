/*

	G N O K I I

	A Linux/Unix toolset and driver for Nokia mobile phones.
	Copyright (C) Hugh Blemings, 1999.

	Released under the terms of the GNU GPL, see file COPYING for more details.

	datapump.c - provides routines to handle processing of data
	when connected in fax or data mode.  Converts data from/to
	GSM phone to virtual modem interface.

*/

#define		__datapump_c


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


#include "misc.h"
#include "gsm-common.h"
#include "gsm-api.h"
#include "at-emulator.h"
#include "virtmodem.h"
#include "datapump.h"
#include "rlp-common.h"

	/* Global variables */
extern bool CommandMode;


	/* Local variables */
int		PtyRDFD;	/* File descriptor for reading and writing to/from */
int		PtyWRFD;	/* pty interface - only different in debug mode. */ 
u8 pluscount;

bool DP_Initialise(int read_fd, int write_fd)
{
  PtyRDFD = read_fd;
  PtyWRFD = write_fd;
  RLP_Initialise(GSM->SendRLPFrame, DP_CallBack);
  RLP_SetUserRequest(Attach_Req,true);
  pluscount=0;
  return true;
}



void DP_HandleIncomingData(u8 *buffer, int length)
{
  char ok[]="OK\n\r";

  /* FIXME - check +++ more thoroughly*/
  
  if (buffer[0]=='+') pluscount++;
    else pluscount=0;
  if (pluscount==3) {
    CommandMode=true;
    write(PtyWRFD, ok, strlen(ok));
  }
  else RLP_Send(buffer, length);
  
}


void DP_CallBack(RLP_UserInds ind, u8 *buffer, int length)
{
  char carrier[]="CARRIER\n\r";
  char connect[]="CONNECT\n\r";
  char carrierlost[]="CARRIER LOST\n\r";

  switch(ind) {
  case Data:
    if (CommandMode==false) write(PtyWRFD, buffer, length);
    break;
  case Conn_Ind:
    if (CommandMode==false) write(PtyWRFD, carrier, strlen(carrier));
    RLP_SetUserRequest(Conn_Req,true);
    break;
  case StatusChange:
    if (buffer[0]==0) {
      if (CommandMode==false) write(PtyWRFD, connect, strlen(connect));
    }
    break;
  case Disc_Ind:
    if (CommandMode==false) write(PtyWRFD, carrier, strlen(carrierlost));
    CommandMode=true;
    break;
  case Reset_Ind:
    RLP_SetUserRequest(Reset_Resp,true);
    break;
  default:
  }
}

