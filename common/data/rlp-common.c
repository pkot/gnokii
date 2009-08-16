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
  Copyright (C) 2001       Chris Kemp
  Copyright (C) 2002-2004  BORBELY Zoltan

  The development of RLP protocol is sponsored by SuSE CR, s.r.o. (Pavel uses
  the SIM card from SuSE for testing purposes).

  Actual implementation of RLP protocol. Based on GSM 04.22 version 7.1.0,
  downloadable from www.etsi.org (if you register with them)

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "compat.h"
#include "gnokii.h"
#include "data/rlp-crc24.h"
#include "misc.h" /* For u8, u32 etc. */

#ifdef WIN32
#  define INLINE __inline
#else
#  define INLINE inline
#endif

#ifndef RLP_DEBUG
#  define rlpprintf(a...) do { } while (0)
#else
#  define rlpprintf(a...) do { gn_log_rlpdebug(a); } while (0)
#endif

/* Our state machine which handles all of nine possible states of RLP
   machine. */
void MAIN_STATE_MACHINE(gn_rlp_f96_frame *frame, rlp_f96_header *header);

/* This is the type we are just handling. */
rlp_frame_types CurrentFrameType;

/* Current state of RLP state machine. */
rlp_state      CurrentState=RLP_S0; /* We start at ADM and Detached */

/* Next state of RLP state machine. */
rlp_state      NextState;

/* Pointer to Send function that sends frame to phone. */
int      (*RLP_SendFunction)(gn_rlp_f96_frame *frame, int out_dtx);

/* Pointer to Passup function which returns data/inds */
int      (*RLP_Passup)(rlp_user_inds ind, unsigned char *buffer, int length);


/* State variables - see GSM 04.22, Annex A, section A.1.2 */

rlp_state_variable UA_State;
rlp_state_variable UI_State;
rlp_state_variable Ackn_State;
rlp_state_variable Poll_State;
rlp_state_variable Poll_xchg;
rlp_state_variable SABM_State;
rlp_state_variable DISC_State;
rlp_state_variable DM_State;  /* FIXME - not handled */
rlp_state_variable XI_R_State;
rlp_state_variable XID_C_State;
rlp_state_variable XID_R_State;
rlp_state_variable TEST_R_State;

u8 VR=0;
u8 VA=0;
u8 VS=0;
u8 VD=0;
u8 DISC_Count;

u8 DTX_VR;
rlp_frame_types DTX_SF;

#define RLP_M 62

rlp_data R[RLP_M];
rlp_data S[RLP_M];

rlp_state_variable SABM_State;
int SABM_Count;

rlp_user_request_store UserRequests;

u8 Poll_Count = 0;

/* For now timing is done based on a frame reception rate of 20ms */
/* Serge has measured it as 18.4ms */
#define RLP_T_Scaling 2

/* Timers - a value of -1 means not set */
/* To set, timer is loaded with RLP_Timeout1_Limit/RLP_T_Scaling. */
/* Each received frame (including NULLS / errors) any >0 timer is decrease */

int T;
int T_RCVS[RLP_M];

bool UA_FBit = true;
bool Ackn_FBit = false;
bool DM_FBit = false;  /* FIXME - not handled */
bool RRReady = false;
bool LRReady = true;   /* FIXME - not handled (as if we couldn't keep up with 9600bps :-) */
bool DISC_PBit = false;

u8 LastStatus = 0xff;   /* Last Status byte */


/* RLP Parameters. FIXME: Reset these - e.g. when entering state 0 */

u8 RLP_SEND_WS = RLP_M-1;
u8 RLP_RCV_WS = RLP_M-1;
u8 RLP_Timeout1_Limit = 55;
u8 RLP_N2 = 15; /* Maximum number of retransmisions. GSM spec says 6 here, but
		   Nokia will XID this. */
u8 RLP_T2 = 0;
u8 RLP_VersionNumber = 0;


/****** Externally called functions ********/
/*******************************************/


/* Function to initialise RLP code.  Main purpose for now is
   to set the address of the RLP send function in the API code. */

void rlp_initialise(int (*rlp_send_function)(gn_rlp_f96_frame *frame, int out_dtx), int (*rlp_passup)(rlp_user_inds ind, unsigned char *buffer, int length))
{
	int i;

	RLP_SendFunction = rlp_send_function;
	RLP_Passup = rlp_passup;
	UserRequests.Conn_Req = false;
	UserRequests.Attach_Req = false;
	UserRequests.Conn_Req_Neg = false;
	UserRequests.Reset_Resp = false;
	UserRequests.Disc_Req = false;
	CurrentState = RLP_S0;
	T = -1;
	for (i = 0; i < RLP_M; i++) T_RCVS[i] = -1;

	UA_FBit = true;
	Ackn_FBit = false;
	DISC_PBit = false;
	LastStatus = 0xff;
	Poll_Count = 0;
	VR = 0;
	VA = 0;
	VS = 0;
	VD = 0;

	RLP_SEND_WS = RLP_M-1;
	RLP_RCV_WS = RLP_M-1;
	RLP_Timeout1_Limit = 55;
	RLP_N2 = 15;
	RLP_T2 = 0;
	RLP_VersionNumber = 0;
}

/* Set a user event */
/* Called by user program for now */

void rlp_user_request_set(rlp_user_requests type, int value)
{
	switch (type) {
	case Conn_Req:
		UserRequests.Conn_Req = value;
		break;
	case Attach_Req:
		UserRequests.Attach_Req = value;
		break;
	case Conn_Req_Neg:
		UserRequests.Conn_Req_Neg = value;
		break;
	case Reset_Resp:
		UserRequests.Reset_Resp = value;
		break;
	case Disc_Req:
		UserRequests.Disc_Req = value;
		break;
	default:
		break;
	}
}


/***** Internal functions **********/
/***********************************/


/* Check whether a user event is set */

int rlp_user_request_get(rlp_user_requests type)
{
	int result = false, *x;

	switch (type) {
	case Conn_Req:
		x = &UserRequests.Conn_Req;
		break;
	case Attach_Req:
		x = &UserRequests.Attach_Req;
		break;
	case Conn_Req_Neg:
		x = &UserRequests.Conn_Req_Neg;
		break;
	case Reset_Resp:
		x = &UserRequests.Reset_Resp;
		break;
	case Disc_Req:
		x = &UserRequests.Disc_Req;
		break;
	default:
		x = &result;
		break;
	}

	result = *x;

	return result;
}

void RLP_SetTimer(int *timer)
{
	*timer = (int)(RLP_Timeout1_Limit/RLP_T_Scaling);
}


/* Previous sequence number. */
static INLINE u8 Decr(u8 x)
{
	if (x == 0)
		return (RLP_M-1);
	else
		return (x-1);
}

/* Next sequence number. */
static INLINE u8 Incr(u8 x)
{
	if (x == RLP_M-1)
		return 0;
	else
		return (x+1);
}

/* Difference between sequence numbers. */

/* FIXME: Not used now, so I have commented it out. PJ
 * static INLINE u8 Diff(u8 x, u8 y)
 * {
 *   int result = x-y;
 *   return (result >= 0) ? result : result + RLP_M;
 * }
*/

/* Check value is within range */
static bool InWindow(u8 val, u8 lower, u8 upper)
{
	/* allow for one level of wrapping round */
	if (lower >= RLP_M) lower -= RLP_M;
	if (upper >= RLP_M) upper -= RLP_M;
	if (val >= RLP_M)   val -= RLP_M;

	/*   .......L*****U....... */
	if (lower <= upper)
		return (val >= lower) && (val <= upper);

	/*  ******U.........L***** */
	return (val <= upper) || (val >= lower);
}

void RLP_Init_link_vars(void)
{
	int i;

	Ackn_State = _idle;
	Poll_State = _idle;
	Poll_Count = 0;
	Poll_xchg = _idle;
	SABM_State = _idle;
	DISC_State = _idle;
	RRReady = true;  /* This seems a bit strange but it's what the spec says... */
	VA = 0;
	VR = 0;
	VS = 0;
	VD = 0;
	LastStatus = 0xff;

	for (i = 0; i < RLP_M; i++) {
		R[i].State = _idle;
		S[i].State = _idle;
	}
}


void RLP_AddRingBufferDataToSlots(void)
{
	u8 buffer[24];
	int size;

	while ((S[VD].State == _idle)
	       && ((size = RLP_Passup(GetData,buffer,24)) != 0)) {
		memset(S[VD].Data, 0xff, 25);    /* FIXME - this isn't necessary - but makes debugging easier! */
		if (size > 23) {
			S[VD].Data[0] = 0x1e;
			size = 24;
		} else S[VD].Data[0] = size;

		memcpy(&S[VD].Data[1], buffer, size);

		if (size != 24) S[VD].Data[size+1] = 0x1f;

		S[VD].State = _send;
		VD = Incr(VD);
	}
}


static void  RLP_DumpF96Frame(gn_rlp_f96_frame *frame)
{
	rlp_f96_header header;

	rlp_f96_header_decode(frame, &header);

	switch (header.Type) {

	case RLP_FT_U: /* Unnumbered frames. */
		rlpprintf("Unnumbered Frame [$%02x%02x] M=%02x ", frame->Header[0],
			  frame->Header[1],
			  header.M);

		switch (header.M) {
		case RLP_U_SABM :
			rlpprintf("Set Asynchronous Balanced Mode (SABM) ");
			break;

		case RLP_U_UA:
			rlpprintf("Unnumbered Acknowledge (UA) ");
			break;

		case RLP_U_DISC:
			rlpprintf("Disconnect (DISC) ");
			break;

		case RLP_U_DM:
			rlpprintf("Disconnected Mode (DM) ");
			break;

		case RLP_U_UI:
			rlpprintf("Unnumbered Information (UI) ");
			break;

		case RLP_U_XID:
			rlpprintf("Exchange Information (XID) \n");
			rlp_xid_display(frame->Data);
			break;

		case RLP_U_TEST:
			rlpprintf("Test (TEST) ");
			break;

		case RLP_U_NULL:
			rlpprintf("Null information (NULL) ");
			break;

		case RLP_U_REMAP:
			rlpprintf("Remap (REMAP) ");
			break;

		default:
			rlpprintf("Unknown!!! ");
			break;
		}
		break;

	case RLP_FT_S: /* Supervisory frames. */
		rlpprintf("Supervisory Frame [$%02x%02x] S=0x%x N(R)=%d ",
			  frame->Header[0],
			  frame->Header[1],
			  header.S,
			  header.Nr);

		switch (header.S) {
		case RLP_S_RR:
			rlpprintf("RR");
			break;

		case RLP_S_REJ:
			rlpprintf("REJ");
			break;

		case RLP_S_RNR:
			rlpprintf("RNR");
			break;

		case RLP_S_SREJ:
			rlpprintf("SREJ");
			break;

		default:
			rlpprintf("BAD");
			break;
		}
		break;

	default:
		rlpprintf("Info+Supervisory Frame [$%02x%02x] S=0x%x N(S)=%d N(R)=%d ",
			  frame->Header[0],
			  frame->Header[1],
			  header.S,
			  header.Ns,
			  header.Nr);

		switch (header.S) {
		case RLP_S_RR:
			rlpprintf("RR");
			break;

		case RLP_S_REJ:
			rlpprintf("REJ");
			break;

		case RLP_S_RNR:
			rlpprintf("RNR");
			break;

		case RLP_S_SREJ:
			rlpprintf("SREJ");
			break;

		default:
			rlpprintf("BAD");
			break;
		}

		break;
	}

	/* Command/Response and Poll/Final bits. */
	rlpprintf(" C/R=%d P/F=%d", header.CR, header.PF);

	/* Information. */
	/*
	if (CurrentFrameType != RLP_FT_U_NULL) {

		dprintf("\n");

		for (count = 0; count < 25; count ++) {

			if (isprint(frame->Data[count]))
				dprintf("[%02x%c]", frame->Data[count], frame->Data[count]);
			else
				dprintf("[%02x ]", frame->Data[count]);

			if (count == 15)
				dprintf("\n");
		}
	}
	*/

	/* FCS. */
	rlpprintf(" FCS: %02x %02x %02x\n\n", frame->FCS[0],
		  frame->FCS[1],
		  frame->FCS[2]);
}

/* FIXME: Remove this after finishing. */
void X(gn_rlp_f96_frame *frame)
{
	int i;

	for (i = 0; i < 30; i++)
		dprintf("byte[%2d]: %02x\n", i, *( (u8 *)frame + i));
}


void ResetAllT_RCVS(void)
{
	int i;
	for (i = 0; i < RLP_M; i++) T_RCVS[i] = -1;
}


/* This function is used for sending RLP frames to the phone. */
void RLP_SendF96Frame(rlp_frame_types FrameType,
		      bool OutCR, bool OutPF,
		      u8 OutNR, u8 OutNS,
		      u8 *OutData, u8 OutDTX)
{

	gn_rlp_f96_frame frame;
	int i;

	frame.Header[0] = 0;
	frame.Header[1] = 0;

#define SetCRBit frame.Header[0] |= 0x01;
#define SetPFBit frame.Header[1] |= 0x02;

#define ClearCRBit frame.Header[0] &= (~0x01);
#define ClearPFBit frame.Header[1] &= (~0x02);

	/* If Command/Response bit is set, set it in the header. */
	if (OutCR) SetCRBit;

	/* If Poll/Final bit is set, set it in the header. */
	if (OutPF) SetPFBit;

	/* If OutData is not specified (ie. is NULL) we want to clear frame.Data
	   array for the user. */
	if (!OutData) {
		frame.Data[0] = 0x00; /* 0x1f */
		for (i = 1; i < 25; i++)
			frame.Data[i] = 0;
	} else {
		for (i = 0; i < 25; i++)
			frame.Data[i] = OutData[i];
	}

#define PackM(x)  frame.Header[1] |= ((x) << 2);
#define PackS(x)  frame.Header[0]|=((x) << 1);
#define PackNR frame.Header[1] |= (OutNR << 2);
#define PackNS frame.Header[0] |= (OutNS << 3); frame.Header[1] |= (OutNS >> 5);

	switch (FrameType) {

	/* Unnumbered frames. Be careful - some commands are used as commands
	   only, so we have to set C/R bit later. We should not allow user for
	   example to send SABM as response because in the spec is: The SABM
	   encoding is used as command only. */
	case RLP_FT_U_SABM:
		frame.Header[0] |= 0xf8; /* See page 11 of the GSM 04.22 spec - 0 X X 1 1 1 1 1 */
		frame.Header[1] |= 0x01; /* 1 P/F M1 M2 M3 M4 M5 X */
		SetCRBit; /* The SABM encoding is used as a command only. */
		SetPFBit; /* It is always used with the P-bit set to "1". */
		PackM(RLP_U_SABM);
		break;

	case RLP_FT_U_UA:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		ClearCRBit; /* The UA encoding is used as a response only. */
		PackM(RLP_U_UA);
		break;

	case RLP_FT_U_DISC:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		SetCRBit; /* The DISC encoding is used as a command only. */
		PackM(RLP_U_DISC);
		break;

	case RLP_FT_U_DM:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		ClearCRBit; /* The DM encoding is used as a response only. */
		PackM(RLP_U_DM);
		break;

	case RLP_FT_U_NULL:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		PackM(RLP_U_NULL);
		break;

	case RLP_FT_U_UI:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		PackM(RLP_U_UI);
		break;

	case RLP_FT_U_XID:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		SetPFBit; /* XID frames are always used with the P/F-bit set to "1". */
		PackM(RLP_U_XID);
		break;

	case RLP_FT_U_TEST:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		PackM(RLP_U_TEST);
		break;

	case RLP_FT_U_REMAP:
		frame.Header[0] |= 0xf8;
		frame.Header[1] |= 0x01;
		ClearPFBit; /* REMAP frames are always used with P/F-bit set to "0". */
		PackM(RLP_U_REMAP);
		break;

	case RLP_FT_S_RR:
		frame.Header[0] |= 0xf0;  /* See page 11 of the GSM 04.22 spec - 0 X X 1 1 1 1 1 */
		frame.Header[1] |= 0x01; /* 1 P/F ...N(R)... */
		PackNR;
		PackS(RLP_S_RR);
		break;

	case RLP_FT_S_REJ:
		frame.Header[0] |= 0xf0;
		frame.Header[1] |= 0x01;
		PackNR;
		PackS(RLP_S_REJ);
		break;

	case RLP_FT_S_RNR:
		frame.Header[0] |= 0xf0;
		frame.Header[1] |= 0x01;
		PackNR;
		PackS(RLP_S_RNR);
		break;

	case RLP_FT_S_SREJ:
		frame.Header[0] |= 0xf0;
		frame.Header[1] |= 0x01;
		PackNR;
		PackS(RLP_S_SREJ);
		break;

	case RLP_FT_SI_RR:
		PackNR;
		PackNS;
		PackS(RLP_S_RR);
		break;

	case RLP_FT_SI_REJ:
		PackNR;
		PackNS;
		PackS(RLP_S_REJ);
		break;

	case RLP_FT_SI_RNR:
		PackNR;
		PackNS;
		PackS(RLP_S_RNR);
		break;

	case RLP_FT_SI_SREJ:
		PackNR;
		PackNS;
		PackS(RLP_S_SREJ);
		break;

	default:
		break;
	}

	/* Store FCS in the frame. */
	rlp_crc24checksum_calculate((u8 *)&frame, 27, frame.FCS);

	/* X(&frame); */

	rlpprintf("S ");
	RLP_DumpF96Frame(&frame);

	if (RLP_SendFunction)
		RLP_SendFunction(&frame, OutDTX);
}

/* Check_input_PDU in Serge's code. */
void rlp_f96_frame_display(gn_rlp_f96_frame *frame)
{
	int count;
	rlp_f96_header header;

	if (T >= 0) T--;
	for (count = 0; count < RLP_M; count++)
		if (T_RCVS[count] >= 0) T_RCVS[count]--;

	CurrentFrameType = RLP_FT_BAD;

	if (!frame) {
		/* no frame provided, drop through to state machine anyway */
	} else if (rlp_crc24fcs_check((u8 *)frame, 30) == true) {

		/* Here we have correct RLP frame so we can parse the field of the header
		   to out structure. */

		rlpprintf("R ");
		RLP_DumpF96Frame(frame);

		rlp_f96_header_decode(frame, &header);

		switch (header.Type) {

		case RLP_FT_U: /* Unnumbered frames. */

			switch (header.M) {
			case RLP_U_SABM :
				if (header.CR == 0 || header.PF == 0) break;
				CurrentFrameType = RLP_FT_U_SABM;
				break;

			case RLP_U_UA:
				if (header.CR == 1) break;
				CurrentFrameType = RLP_FT_U_UA;
				break;

			case RLP_U_DISC:
				if (header.CR == 0) break;
				CurrentFrameType = RLP_FT_U_DISC;
				break;

			case RLP_U_DM:
				if (header.CR == 1) break;
				CurrentFrameType = RLP_FT_U_DM;
				break;

			case RLP_U_UI:
				CurrentFrameType = RLP_FT_U_UI;
				break;

			case RLP_U_XID:
				CurrentFrameType = RLP_FT_U_XID;
				break;

			case RLP_U_TEST:
				CurrentFrameType = RLP_FT_U_TEST;
				break;

			case RLP_U_NULL:
				CurrentFrameType = RLP_FT_U_NULL;
				break;

			case RLP_U_REMAP:
				CurrentFrameType = RLP_FT_U_REMAP;
				break;

			default:
				CurrentFrameType = RLP_FT_BAD;
				break;
			}
			break;

		case RLP_FT_S: /* Supervisory frames. */

			switch (header.S) {
			case RLP_S_RR:
				CurrentFrameType = RLP_FT_S_RR;
				break;

			case RLP_S_REJ:
				CurrentFrameType = RLP_FT_S_REJ;
				break;

			case RLP_S_RNR:
				CurrentFrameType = RLP_FT_S_RNR;
				break;

			case RLP_S_SREJ:
				CurrentFrameType = RLP_FT_S_SREJ;
				break;

			default:
				CurrentFrameType = RLP_FT_BAD;
				break;
			}
			break;

		default:

			switch (header.S) {
			case RLP_S_RR:
				CurrentFrameType = RLP_FT_SI_RR;
				break;

			case RLP_S_REJ:
				CurrentFrameType = RLP_FT_SI_REJ;
				break;

			case RLP_S_RNR:
				CurrentFrameType = RLP_FT_SI_RNR;
				break;

			case RLP_S_SREJ:
				CurrentFrameType = RLP_FT_SI_SREJ;
				break;

			default:
				CurrentFrameType = RLP_FT_BAD;
				break;
			}

			break;
		}

	} else {
		/* RLP Checksum failed - don't we need some statistics about these
		   failures? Nothing is printed, because in the first stage of connection
		   there are too many bad RLP frames... */
		dprintf("Frame FCS is bad. Ignoring...\n");
	}

	MAIN_STATE_MACHINE(frame, &header);
	/*
	  Y:= outblock();
	*/
	return;
}

/* FIXME: real TEST_Handling - we do not handle TEST yet. */
void TEST_Handling()
{
	return;
}

/* FIXME: better XID_handling - but this will answer a XID command. */
bool XID_Handling (gn_rlp_f96_frame *frame, rlp_f96_header *header)
{
	u8 count;
	u8 type;
	u8 length;

	if (CurrentFrameType == RLP_FT_U_XID) {
		count = 0;
		while (frame->Data[count] != 0) {

			type = frame->Data[count] >> 4;
			length = frame->Data[count] & 0x0f;
			count++;

			switch (type) {

			case 0x01: /* RLP Version Number */
				RLP_VersionNumber = frame->Data[count];
				count += length;
				break;
			case 0x02: /* Interworking Function (IWF) to Mobile Station (MS) window size */
				if (frame->Data[count] >= 1 && frame->Data[count] < RLP_M)
					RLP_RCV_WS = frame->Data[count];
				count += length;
				break;
			case 0x03: /* MS to IWF window size */
				if (frame->Data[count] >= 1 && frame->Data[count] < RLP_M)
					RLP_SEND_WS = frame->Data[count];
				count += length;
				break;
			case 0x04: /* Acknowledgement Timer (T1). */
				RLP_Timeout1_Limit = frame->Data[count];
				count += length;
				break;
			case 0x05: /* Retransmission attempts (N2). */
				RLP_N2 = frame->Data[count];
				count += length;
				break;
			case 0x06: /* Reply delay (T2). */
				RLP_T2 = frame->Data[count];
				count += length;
				break;
			case 0x07: /* Compression - not yet! */
				break;
			default:
				count += length;
				break;
			}
		}

		/* Now reassemble a reply */
		count = 0;
		memset(frame->Data, 0x00, 25);  /* Makes debugging easier */

		/* Version Number - force to 0 for now */
		RLP_VersionNumber = 0;
		frame->Data[count++] = 0x11;
		frame->Data[count++] = RLP_VersionNumber;

		/* Window sizes */
		frame->Data[count++] = 0x21;
		frame->Data[count++] = RLP_RCV_WS;
		frame->Data[count++] = 0x31;
		frame->Data[count++] = RLP_SEND_WS;

		/* Acknowledgement Timer (T1). */
		frame->Data[count++] = 0x41;
		frame->Data[count++] = RLP_Timeout1_Limit;

		/* Retransmission attempts (N2). */
		frame->Data[count++] = 0x51;
		frame->Data[count++] = RLP_N2;

		/* Reply delay (T2). */
		frame->Data[count++] = 0x61;
		frame->Data[count++] = RLP_T2;

		XID_R_State = _send;

		return true;
	}

	return false;
}


bool Send_TXU(gn_rlp_f96_frame *frame, rlp_f96_header *header)
{
	dprintf("Send_TXU()\n");
	dprintf("XID_R_State=%d\n", XID_R_State);

	/*
	if (RLP_UserEvent(TEST_R_State)) {
		RLP_SendF96Frame(RLP_FT_U_TEST, false, TEST_R_FBit, 0, 0, TEST_R_Data, false);
		return true;
	}  else
	*/

	if (XID_R_State == _send && frame) {
		RLP_SendF96Frame(RLP_FT_U_XID, false, true, 0, 0, frame->Data, false);
		XID_R_State = _idle;
		return true;
	}

	/*
	else if ((XID_C_State == _send ) && (Poll_xchg == _idle)) {
		RLP_SendF96Frame(RLP_FT_U_XID, true, true, 0, 0, XID_C_Data, false);
		XID_C_State = _wait;
		T_XID = 1;
		Poll_xchg = _wait;
		return true;
	} else if (RLP_UserEvent(UI_State)) {
		RLP_SendF96Frame(RLP_FT_U_UI, true, false, 0, 0, NULL, false);
		return true;
	}
	*/

	return false;
}


/* Deliver data */

void RLP_DeliverAllInSeqIF()
{
	int i,j;

	do {
		if ((R[VR].Data[0] & 0xE0) != LastStatus) {
			LastStatus = (R[VR].Data[0] & 0xE0);
			RLP_Passup(StatusChange, &LastStatus, 0);
		}

		j = 0;
		i = R[VR].Data[0] & 0x1f;
		if (i == 0x1e) j = 24;
		if (i < 0x18) j = i;

		/* FIXME - should check for more data in the frame */

		RLP_Passup(Data, R[VR].Data+1, j);

		R[VR].State = _idle;
		VR = Incr(VR);

	} while (R[VR].State == _rcvd);
}


/* Mark any missing information frames between VR and Ns */
void RLP_MarkMissingIF(u8 Ns)
{
	u8 i;
	for (i = VR; i != Ns; i = Incr(i)) {
		if (R[i].State == _idle) R[i].State = _srej;  /* bug in spec, fig A.23 */
	}
}


/* Information frame handler */

bool RLP_I_Handler(gn_rlp_f96_frame *frame, rlp_f96_header *header)
{
	if ((header->CR) && (header->PF))
		return true;

	/* If the window size is 61, a received frame must have a sequence
	   number between VR and VR+60 */
	if (!InWindow(header->Ns, VR, VR + RLP_RCV_WS - 1))
		return true;

	if (header->Ns == VR) {
		/* This is not in the spec but I think it is necessary */
		if (R[header->Ns].State == _wait) T_RCVS[header->Ns] = -1;
		R[VR].State = _rcvd;
		memcpy(R[VR].Data, frame->Data, 25);
		RLP_DeliverAllInSeqIF();
		Ackn_State = _send;
	} else {  /* Out of sequence, cause a SREJ */
		if (R[header->Ns].State == _wait) T_RCVS[header->Ns] = -1;
		R[header->Ns].State = _rcvd;
		memcpy(R[header->Ns].Data, frame->Data, 25);
		RLP_MarkMissingIF(header->Ns);
	}
	return false;
}


/* Mark acknowledged send frames */
void RLP_AdvanceVA(u8 Nr)
{
	while (VA != Nr) {
		S[VA].State = _idle;
		VA = Incr(VA);
	}
}


/* Decrease VS back down to Nr since these have not been acknowledged */
void RLP_DecreaseVS(u8 Nr)
{
	while (VS != Nr) {
		VS = Decr(VS);
		S[VS].State = _send;
	}
}

/* Supervisory frame handling */
void RLP_S_Handler(gn_rlp_f96_frame *frame, rlp_f96_header *header)
{
	u8 i;

	if ((header->CR) && (header->PF)) {
		/* Special exchange (ie. error) - counter? */
		rlpprintf("Got Poll command\n");
		Ackn_State = _send;
		Ackn_FBit = true;
		for (i = 0; i < RLP_M; i++) R[i].State = _idle;
		ResetAllT_RCVS();
	}
	if (Poll_State != _idle) {
		if (header->PF == 0) return;
		if ((CurrentFrameType == RLP_FT_S_SREJ) || (CurrentFrameType == RLP_FT_S_REJ) ||
		    (CurrentFrameType == RLP_FT_SI_SREJ) || (CurrentFrameType == RLP_FT_SI_REJ)) return;
		RLP_DecreaseVS(header->Nr);
		Poll_State = _idle;
		Poll_xchg = _idle;
	}

	switch (CurrentFrameType) {
	case RLP_FT_S_RR:
	case RLP_FT_SI_RR:
		RLP_AdvanceVA(header->Nr);
		RRReady = true;
		break;
	case RLP_FT_S_RNR:
	case RLP_FT_SI_RNR:
		RLP_AdvanceVA(header->Nr);
		RRReady = false;
		break;
	case RLP_FT_S_REJ:
	case RLP_FT_SI_REJ:
		RLP_AdvanceVA(header->Nr);
		RRReady = true;
		RLP_DecreaseVS(header->Nr);
		break;
	case RLP_FT_S_SREJ:
	case RLP_FT_SI_SREJ:
		S[header->Nr].State = _send;
		T = -1;
		return;
	default:
		break;
	}

	if (VA == VS) T = -1;
}


/* Find the first SREJ frame */
bool RLP_SREJSlot(u8 *x)
{
	u8 i;

	for (i = Incr(VR); i != VR; i = Incr(i))
		if (R[i].State == _srej) {
			*x = i;
			return true;
		}

	return false;
}


/* Check if any SREJ frames need sending, if not send the next in line */
bool RLP_PrepareDataToTransmit(u8 *p)
{
	u8 i;

	for (i = VA; i != VS; i = Incr(i))
		if (S[i].State == _send) {
			*p = i;
			S[i].State = _wait;
			return true;
		}
	if (S[VS].State != _send) return false;
	if (!InWindow(VS, VA, VA + RLP_SEND_WS - 1))
		return false;
	*p = VS;
	S[VS].State = _wait;
	VS = Incr(VS);
	return true;
}


/* Send a SREJ command */
void RLP_SendSREJ(u8 x)
{
	u8 k;

	if ((Poll_xchg == _idle) && (Poll_State == _send)) {
		rlpprintf("Sending SREJ with poll\n");
		RLP_SendF96Frame(RLP_FT_S_SREJ, true, true, x, 0, NULL, false);
		R[x].State = _wait;
		RLP_SetTimer(&T_RCVS[x]);
		Poll_Count++;
		Poll_State = _wait;
		Poll_xchg = _wait;
		RLP_SetTimer(&T);
	} else if (RRReady && RLP_PrepareDataToTransmit(&k)) {
		rlpprintf("Sending SREJ for %d along with frame %d\n", x, k);
		RLP_SendF96Frame(RLP_FT_SI_SREJ, true, false, x , k , S[k].Data, false);
		R[x].State = _wait;
		RLP_SetTimer(&T_RCVS[x]);
		RLP_SetTimer(&T);
	} else {
		rlpprintf("Sending SREJ for %d\n",x);
		RLP_SendF96Frame(RLP_FT_S_SREJ, true, false, x, 0, NULL, false);
		R[x].State = _wait;
		RLP_SetTimer(&T_RCVS[x]);
	}
}


/* Send a command */
void RLP_Send_XX_Cmd(rlp_frame_types type)
{
	u8 k;

	if ((Poll_xchg != _wait) && (Poll_State == _send)) {
		RLP_SendF96Frame(type, true, true, VR, 0, NULL, false);
		rlpprintf("Sending Comd %x with Poll\n",type);
		Ackn_State = _idle;
		Poll_Count++;
		Poll_State = _wait;
		Poll_xchg = _wait;
		RLP_SetTimer(&T);
	} else if (RRReady && RLP_PrepareDataToTransmit(&k)) {
		rlpprintf("Sending Comd %x with frame %d\n", type, k);
		RLP_SendF96Frame(type + 4, true, false, VR, k, S[k].Data, false);
		Ackn_State = _idle;
		RLP_SetTimer(&T);
	} else {
		if (type != 9)
			rlpprintf("Sending Comd %x\n",type);
		RLP_SendF96Frame(type, true, false, VR, 0, NULL, false);
		Ackn_State = _idle;
		DTX_SF = type;
		DTX_VR = VR;   /* As v7.1.0 spec */
	}
}


/* Send a Response */
void RLP_Send_XX_Resp(rlp_frame_types type)
{
	u8 k;

	if (RRReady && RLP_PrepareDataToTransmit(&k)) {
		rlpprintf("Sending Resp %x with frame %d\n",type+4,k);
		RLP_SendF96Frame(type + 4, false, true, VR, k, S[k].Data, false);
		Ackn_State = _idle;
		Ackn_FBit = false;
		RLP_SetTimer(&T);
	} else {
		rlpprintf("Sending Resp %x\n",type);
		RLP_SendF96Frame(type, false, true, VR, 0, NULL, false);
		Ackn_State = _idle;
		Ackn_FBit = false;
	}
}


/* Decide which frame to use and send it - currently only used in state 4 */
void RLP_SendData()
{
	u8 x;

	if (UA_State == _send) {
		RLP_SendF96Frame(RLP_FT_U_UA, false, UA_FBit, 0, 0, NULL, false);
		UA_State = _idle;
	} else if (Ackn_FBit == true) {
		rlpprintf("About to send Poll resp\n");
		if (LRReady) RLP_Send_XX_Resp(RLP_FT_S_RR);
		else RLP_Send_XX_Resp(RLP_FT_S_RNR);
	} else if (RLP_SREJSlot(&x)) RLP_SendSREJ(x);
	else if (LRReady) RLP_Send_XX_Cmd(RLP_FT_S_RR);
	else RLP_Send_XX_Cmd(RLP_FT_S_RNR);
}

void MAIN_STATE_MACHINE(gn_rlp_f96_frame *frame, rlp_f96_header *header)
{
	int i;

	switch (CurrentState) {
	/***** RLP State 0. *****/

	/* ADM and Detached.

	   This is the initial state after power on.

	   As long as the RLP entity is "Detached", DISC(P) and/or SABM at the
	   lower interface is acted upon by sending DM(P) or DM(1). Any other
	   stimulus at the lower interface is ignored.

	   This state can be exited only with Attach_Req. */

	case RLP_S0:
		dprintf("RLP state 0.\n");

		switch (CurrentFrameType) {

		case RLP_FT_U_DISC:
			RLP_SendF96Frame(RLP_FT_U_DM, false, header->PF, 0, 0, NULL, false);
			break;

		case RLP_FT_U_SABM:
			RLP_SendF96Frame(RLP_FT_U_DM, false, true, 0, 0, NULL, false);
			break;

		default:
			RLP_SendF96Frame(RLP_FT_U_NULL, false, false, 0, 0, NULL, false);
			if (rlp_user_request_get(Attach_Req)) {
				NextState = RLP_S1;
				UA_State = _idle;
			}
			break;
		}

		break;

	/***** RLP State 1. *****/

	/* ADM and Attached.

	   The RLP entity is ready to established a connection, either by
	   initiating the connection itself (Conn_Req) or by responding to an
	   incoming connection request (SABM).

	   Upon receiving a DISC PDU, the handling of the UA response is
	   initiated. */

	case RLP_S1:
		dprintf("RLP state 1.\n");

		if (!XID_Handling(frame, header)) {

			switch(CurrentFrameType) {

			case RLP_FT_U_TEST:
				TEST_Handling();
				break;

			case RLP_FT_U_SABM:
				RLP_Passup(Conn_Ind, NULL, 0);
				NextState = RLP_S3;
				break;

			case RLP_FT_U_DISC:
				UA_State = _send;
				UA_FBit = header->PF;
				break;

			case RLP_FT_BAD:  /* If we get a bad frame we can still respond with SABM */
			default:
				if (rlp_user_request_get(Conn_Req)) {
					SABM_State = _send;
					SABM_Count = 0;
					NextState = RLP_S2;
				}
				break;
			}

		}
		if (!Send_TXU(frame, header)) {

			if (UA_State == _send) {
				RLP_SendF96Frame(RLP_FT_U_UA, false, UA_FBit, 0, 0, NULL, false);
				UA_State = _idle;
			} else
				RLP_SendF96Frame(RLP_FT_U_NULL, false, false, 0, 0, NULL, false);
		}
		break;

	/***** RLP State 2. *****/

	case RLP_S2:
		dprintf("RLP state 2.\n");

		if (!XID_Handling(frame, header)) {

			switch(CurrentFrameType) {

			case RLP_FT_U_TEST:
				TEST_Handling();
				break;

			case RLP_FT_U_SABM:
				/*
				  Conn_Conf = true;
				*/
				T = -1;
				UA_State = _send;
				UA_FBit = true;
				RLP_Init_link_vars();
				NextState = RLP_S4;
				break;

			case RLP_FT_U_DISC:
				T = -1;
				RLP_Passup(Disc_Ind, NULL, 0);
				UA_State = _send;
				UA_FBit = header->PF;
				NextState = RLP_S1;
				break;

			case RLP_FT_U_UA:
				dprintf("UA received in RLP state 2.\n");

				if (SABM_State == _wait && header->PF) {
					T = -1;
					/* Conn_Conf = true; */
					RLP_Init_link_vars();
					NextState = RLP_S4;
				}
				break;

			case RLP_FT_U_DM:
				if (SABM_State == _wait && header->PF) {
					Poll_xchg = _idle;
					/* Conn_Conf_Neg = true; */
					NextState = RLP_S1;
				}
				break;

			default:
				if (T == RLP_Timeout1_Limit || T == 0) {
					Poll_xchg = _idle;
					if (SABM_Count > RLP_N2)
						NextState = RLP_S8;
					SABM_State = _send;
				}
				break;
			}
		}

		if (!Send_TXU(frame, header)) {

			if (SABM_State == _send && Poll_xchg == _idle) {
				RLP_SendF96Frame(RLP_FT_U_SABM, true, true, 0, 0, NULL, false);
				SABM_State = _wait;
				SABM_Count++;
				Poll_xchg = _wait;
				RLP_SetTimer(&T);
			} else
				RLP_SendF96Frame(RLP_FT_U_NULL, false, false, 0, 0, NULL, false);
		}

		if (rlp_user_request_get(Disc_Req)) {
			T = -1;
			DISC_State = _send;
			DISC_Count = 0;
			DISC_PBit = (Poll_xchg == _idle);
			NextState = 5;
		}

		break;

	/***** RLP State 3. *****/

	case RLP_S3:
		dprintf("RLP state 3.\n");

		if (!XID_Handling(frame, header)) {

			switch(CurrentFrameType) {

			case RLP_FT_U_TEST:
				TEST_Handling();
				break;

			case RLP_FT_U_DISC:
				RLP_Passup(Disc_Ind, NULL, 0);
				UA_State = _send;
				UA_FBit = header->PF;
				NextState = RLP_S1;
				break;

			default:
				if (rlp_user_request_get(Conn_Req)) {
					UA_State = _send;
					UA_FBit = true;
					NextState = RLP_S4;
					RLP_Init_link_vars();
				} else if (rlp_user_request_get(Conn_Req_Neg)) {
					DM_State = _send;  /* FIXME - code to handle DM_State - missing from spec? */
					DM_FBit = true;
					NextState = RLP_S1;
				}
				break;
			}
		}

		if (!Send_TXU(frame, header)) {
			if (UA_State == _send) {
				RLP_SendF96Frame(RLP_FT_U_UA, false, UA_FBit, 0, 0, NULL, false);
				UA_State = _idle;
			} else
				RLP_SendF96Frame(RLP_FT_U_NULL, false, false, 0, 0, NULL, false);
		}

		if (rlp_user_request_get(Disc_Req)) {
			T = -1;
			DISC_State = _send;
			DISC_Count = 0;
			DISC_PBit = (Poll_xchg == _idle);
			NextState = 5;
		}
		break;

	/***** RLP State 4. *****/

	case RLP_S4:
		dprintf("RLP state 4.\n");

		if (!XID_Handling(frame, header)) {

			switch (CurrentFrameType) {

			case RLP_FT_U_TEST:
				TEST_Handling();
				break;
			case RLP_FT_U_DISC:
				T = -1;
				ResetAllT_RCVS();
				RLP_Passup(Disc_Ind, NULL, 0);
				UA_State = _send;
				UA_FBit = header->PF;
				NextState = RLP_S1;
				break;
			case RLP_FT_U_SABM:
				T = -1;
				ResetAllT_RCVS();
				RLP_Passup(Reset_Ind, NULL, 0);
				NextState = RLP_S7;
				break;
			case RLP_FT_S_RR:
			case RLP_FT_S_RNR:
			case RLP_FT_S_REJ:
			case RLP_FT_S_SREJ:
				/* Should check here for unsolicited Fbit */
				/* Spec says: "Nr must be within the set of not yet
				   acknowledged I-frames or it must be the next possible
				   frame number." That's VA..VS-1 or VS, i.e. VA..VS */
				if (!InWindow(header->Nr, VA, VS))
					break;
				RLP_S_Handler(frame, header);
				break;
			case RLP_FT_SI_RR:
			case RLP_FT_SI_RNR:
			case RLP_FT_SI_REJ:
			case RLP_FT_SI_SREJ:
				/* Should check here for unsolicited Fbit */
				if (!InWindow(header->Nr, VA, VS))
					break;
				if (!RLP_I_Handler(frame, header)) RLP_S_Handler(frame, header);
				break;
			default:
				break;
			}
		}
		for (i = 0; i < RLP_M; i++)
			if (T_RCVS[i] == 0) {
				rlpprintf("T_RCVS[%d] Timeout in State 4\n", i);
				R[i].State = _srej;
			}
		if (T == 0) {
			T = -1;
			rlpprintf("T Timeout in State 4\n");

			Poll_xchg = _idle;
			if (Poll_State == _idle) {
				Poll_State = _send;
				Poll_Count = 0;
			} else {
				if (Poll_Count > RLP_N2)
					rlpprintf("N2 Errors in State 4\n");
				Poll_State = _send;
				Poll_Count++;
			}
		}

		if (!Send_TXU(frame, header)) {
			if (UA_State == _send) {
				RLP_SendF96Frame(RLP_FT_U_UA, false, UA_FBit, 0, 0, NULL, false);
				UA_State = _idle;
			} else RLP_SendData();
		}

		/* Load any data from the Send ringbuffer into the send slots */
		RLP_AddRingBufferDataToSlots();

		rlpprintf("VD=%d, VA=%d, VS=%d, VR=%d\n", VD, VA, VS, VR);
#ifdef RLP_DEBUG_STATE
		{
			int zzz;

			if (UA_State != _idle) rlpprintf("[UA_State %d]", UA_State);
			if (UI_State != _idle) rlpprintf("[UI_State %d]", UI_State);
			if (Ackn_State != _idle) rlpprintf("[Ackn_State %d]", Ackn_State);
			if (Poll_State != _idle) rlpprintf("[Poll_State %d]", Poll_State);
			if (Poll_xchg != _idle) rlpprintf("[Poll_xchg %d]", Poll_xchg);
			if (SABM_State != _idle) rlpprintf("[SABM_State %d]", SABM_State);
			if (DISC_State != _idle) rlpprintf("[DISC_State %d]", DISC_State);
			if (DM_State != _idle) rlpprintf("[DM_State %d]", DM_State);
			if (XI_R_State != _idle) rlpprintf("[XI_R_State %d]", XI_R_State);
			if (XID_C_State != _idle) rlpprintf("[XID_C_State %d]", XID_C_State);
			if (XID_R_State != _idle) rlpprintf("[XID_R_State %d]", XID_R_State);
			if (TEST_R_State != _idle) rlpprintf("[TEST_R_State %d]", TEST_R_State);

			rlpprintf("S: ");
			for (zzz = 0; zzz < RLP_M; zzz++) rlpprintf("%d ", S[zzz].State);
			rlpprintf("\nR: ");
			for (zzz = 0; zzz < RLP_M; zzz++) rlpprintf("%d ", R[zzz].State);
			rlpprintf("\nT: %d, T_RCVS: ", T);
			for (zzz = 0; zzz < RLP_M; zzz++) rlpprintf("%d ", T_RCVS[zzz]);
			rlpprintf("\n");
		}
#endif

		if (rlp_user_request_get(Disc_Req)) {
			T = -1;
			ResetAllT_RCVS();
			DISC_State = _send;
			DISC_Count = 0;
			DISC_PBit = (Poll_xchg == _idle);
			NextState = 5;
		}

		break;

	/***** RLP State 5. *****/

	case RLP_S5:
		dprintf("RLP state 5.\n");

		if (!XID_Handling(frame, header)) {

			switch (CurrentFrameType) {

			case RLP_FT_U_UA:
			case RLP_FT_U_DM:
				if ((DISC_State == _wait) && (DISC_PBit == header->PF)) {
					if (DISC_PBit == true) Poll_xchg = _idle;
					T = -1;
					NextState = 1;
				}
				break;
			case RLP_FT_U_DISC:
				T = -1;
				UA_State = _send;
				UA_FBit = header->PF;
				NextState = 1;
				break;
			default:
				break;
			}
		}

		if (!Send_TXU(frame, header)) {
			if ((DISC_State != _wait) && !((DISC_PBit == true) && (Poll_xchg == _wait))) {
				RLP_SendF96Frame(RLP_FT_U_DISC, true, DISC_PBit, 0, 0, NULL, false);
				if (DISC_PBit == true) Poll_xchg = _wait;
				DISC_State = _wait;
				DISC_Count++;
				RLP_SetTimer(&T);
			} else
				RLP_SendF96Frame(RLP_FT_U_NULL, false, false, 0, 0, NULL, false);
		}

		if (T == 0) {
			if (DISC_PBit == 1) Poll_xchg = _idle;
			DISC_Count++;
			if (DISC_Count > RLP_N2)
				rlpprintf("N2 error in State 5!\n");
			DISC_State = _send;
		}

		break;

	/***** RLP State 6. *****/
	/* We should only get here after a Reset_Req which is not yet supported */

	case RLP_S6:
		dprintf("RLP state 6 - not yet implemented!\n");

		if (!XID_Handling(frame, header)) {

			switch (CurrentFrameType) {
			default:
				break;
			}

		}

		if (!Send_TXU(frame,header)) {
		}

		if (rlp_user_request_get(Disc_Req)) {
			T = -1;
			DISC_State = _send;
			DISC_Count = 0;
			DISC_PBit = (Poll_xchg == _idle);
			NextState = 5;
		}

		break;

	/***** RLP State 7. *****/

	case RLP_S7:
		dprintf("RLP state 7.\n");

		if (!XID_Handling(frame, header)) {

			switch (CurrentFrameType) {
			case RLP_FT_U_DISC:
				RLP_Passup(Disc_Ind, NULL, 0);
				UA_State = _send;
				UA_FBit = header->PF;
				NextState = RLP_S1;
				break;
			default:
				break;
			}
		}

		if (rlp_user_request_get(Reset_Resp)) {
			UA_State = _send;
			UA_FBit = 1;
			RLP_Init_link_vars();
			NextState = RLP_S4;
		}

		if (!Send_TXU(frame, header)) {
			RLP_SendF96Frame(RLP_FT_U_NULL, false, false, 0, 0, NULL, false);
		}

		if (rlp_user_request_get(Disc_Req)) {
			T = -1;
			DISC_State = _send;
			DISC_Count = 0;
			DISC_PBit = (Poll_xchg == _idle);
			NextState = 5;
		}
		break;

	default:
		dprintf("DEBUG: Unknown RLP state!\n");
		break;
	}

	CurrentState = NextState;
}

/* Given a pointer to an RLP XID frame, display contents in human readable
   form.  Note for now only Version 0 and 1 are supported.  Fields can appear
   in any order and are delimited by a zero type field. This function is the
   exact implementation of section 5.2.2.6, Exchange Identification, XID of
   the GSM specification 04.22. */
void rlp_xid_display(u8 *frame)
{
	int count = 25;  /* Sanity check */
	u8  type, length;

	fprintf(stdout, "XID: ");

	while ((*frame != 0) && (count >= 0)) {
		type = *frame >> 4;
		length = *frame & 0x0f;

		switch (type) {

		case 0x01: /* RLP Version Number, probably 1 for Nokia. */
			frame += length;
			fprintf(stdout, "Ver %d ", *frame);
			break;

		case 0x02: /* IWF to MS window size */
			frame += length;
			fprintf(stdout, "IWF-MS %d ", *frame);
			break;

		case 0x03: /* MS to IWF window size. */
			frame += length;
			fprintf(stdout, "MS-IWF %d ", *frame);
			break;

		case 0x04: /* Acknowledgement Timer (T1). */
			frame += length;
			fprintf(stdout, "T1 %dms ", *frame * 10);
			break;

		case 0x05: /* Retransmission attempts (N2). */
			frame += length;
			fprintf(stdout, "N2 %d ", *frame);
			break;

		case 0x06: /* Reply delay (T2). */
			frame += length;
			fprintf(stdout, "T2 %dms ", *frame * 10);
			break;

		case 0x07: /* Compression. */
			frame ++;
			fprintf(stdout, "Comp [Pt=%d ", (*frame >> 4) );
			fprintf(stdout, "P0=%d ", (*frame & 0x03) );
			frame ++;
			fprintf(stdout, "P1l=%d ", *frame);
			frame ++;
			fprintf(stdout, "P1h=%d ", *frame);
			frame ++;
			fprintf(stdout, "P2=%d] ", *frame);
			break;

		default:
			frame += length;
			fprintf(stdout, "Unknown! type=%02x, length=%02x", type, length);
			break;
		}
		count--;
		frame++;
	}

	fprintf(stdout, "\n");

	return;
}

/* Given a pointer to an F9.6 Frame, split data out into component parts of
   header and determine frame type. */
void rlp_f96_header_decode(gn_rlp_f96_frame *frame, rlp_f96_header *header)
{
	/* Poll/Final bit. */
	if ((frame->Header[1] & 0x02))
		header->PF = true;
	else
		header->PF = false;

	/* Command/Response bit. */
	if ((frame->Header[0] & 0x01))
		header->CR = true;
	else
		header->CR = false;

	/* Send Sequence Number. */
	header->Ns = frame->Header[0] >> 3;

	if ((frame->Header[1] & 0x01))
		header->Ns |= 0x20; /* Most significant bit. */

	/* Determine frame type. See the section 5.2.1 in the GSM 04.22
	   specification. */
	switch (header->Ns) {

	case 0x3f: /* Frames of type U, unnumbered frames. */
		/* U frames have M1, ..., M5 stored in the place of N(R). */
		header->Type = RLP_FT_U;
		header->M = (frame->Header[1] >> 2) & 0x1f;
		return; /* For U frames, we do not need N(R) and bits S1 and S2. */

	case 0x3e: /* Frames of type S, supervisory frames. */
		header->Type = RLP_FT_S;
		break;

	default: /* Frames of type I+S, numbered information transfer ans
		    supervisory frames combined. */
		header->Type = RLP_FT_IS;
		break;
	}

	/* Receive Sequence Number N(R). */
	header->Nr = frame->Header[1] >> 2;

	/* Status bits (S1 and S2). */
	header->S = (frame->Header[0] >> 1) & 0x03;

	return;
}
