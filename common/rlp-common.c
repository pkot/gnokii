/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.
  Copyright (C) Hugh Blemings & Pavel Janík ml., 1999.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  The development of RLP protocol is sponsored by SuSE CR, s.r.o. (Pavel use
  the SIM card from SuSE for testing purposes).

  This file:  rlp-common.c   Version 0.3.1

  Actual implementation of RLP protocol.

  Last modification: Fri Dec  3 00:15:55 CET 1999
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include <stdio.h>
#include <ctype.h>

#include "rlp-common.h"
#include "rlp-crc24.h"
#include "misc.h" /* For u8, u32 etc. */

/* Our state machine which handles all of nine possible states of RLP
   machine. */
void MAIN_STATE_MACHINE(RLP_F96Frame *frame, RLP_F96Header *header);

/* This is the type we are just handling. */
RLP_FrameTypes CurrentFrameType;

/* Current state of RLP state machine. */
RLP_State      CurrentState=RLP_S0; /* We start at ADM and Detached */

/* Next state of RLP state machine. */
RLP_State      NextState;


/* State variables - see GSM 04.22, Annex A, section A.1.2 */

RLP_StateVariable UA_State;

/* RLP Constants. */

u8 RLP_M = 62; /* Number of different sequence numbers (modulus) */

u8 RLP_N2 = 15; /* Maximum number of retransmisions. GSM spec says 6 here, but
                   Nokia will XID this. */



/* Previous sequence number. */
u8 Decr(x) {

  if (x==0)
    return (RLP_M-1);
  else
    return (x-1);
}

/* Next sequence number. */
u8 Incr(x) {

    return (x+1) % RLP_M;
}

/* FIXME: Remove this after finishing. */

void X(RLP_F96Frame *frame) {

  int i;

  for (i=0; i<30; i++)
    printf("byte[%2d]: %02x\n", i, *( (u8 *)frame+i));
   
}

/* This function is used for sending RLP frames to the phone. */

void RLP_SendF96Frame(RLP_FrameTypes FrameType,
                      bool OutCR, bool OutPF,
                      u8 OutNR, u8 OutNS,
                      u8 *OutData,
		      u8 OutDTX)
{

  RLP_F96Frame frame;
  u8 req[60] = { 0x00, 0xd9 };
  int i;

  /* Discontinuos transmission (DTX).  See section 5.6 of GSM 04.22 version
     7.0.1. */

  if (OutDTX)
    req[1]=0x01;

  frame.Header[0]=0;
  frame.Header[1]=0;

#define SetCRBit frame.Header[0]|=0x01;
#define SetPFBit frame.Header[1]|=0x02;

#define ClearCRBit frame.Header[0]&=(~0x01);
#define ClearPFBit frame.Header[1]&=(~0x02);

  /* If Command/Response bit is set, set it in the header. */
  if (OutCR)
    SetCRBit;


  /* If Poll/Final bit is set, set it in the header. */
  if (OutPF)
    SetPFBit;


  /* If OutData is not specified (ie. is NULL) we want to clear frame.Data
     array for the user. */
  if (!OutData) {

    frame.Data[0]=0x1f;

    for (i=1; i<25; i++)
      frame.Data[i]=0;
  }
  else {
    for (i=0; i<25; i++)
      frame.Data[i]=OutData[i];
  }

#define PackM(x)  frame.Header[1]|=((x)<<2);
#define PackS(x)  frame.Header[0]|=((x)<<1);
#define PackNR frame.Header[1]|=(OutNR<<2);
#define PackNS frame.Header[0]|=(OutNS<<3);frame.Header[1]|=(OutNS>>5);

  switch (FrameType) {

    /* Unnumbered frames. Be careful - some commands are used as commands
       only, so we have to set C/R bit later. We should not allow user for
       example to send SABM as response because in the spec is: The SABM
       encoding is used as command only. */

  case RLPFT_U_SABM:

    frame.Header[0]|=0xf8; /* See page 11 of the GSM 04.22 spec - 0 X X 1 1 1 1 1 */
    frame.Header[1]|=0x01; /* 1 P/F M1 M2 M3 M4 M5 X */

    SetCRBit; /* The SABM encoding is used as a command only. */
    SetPFBit; /* It is always used with the P-bit set to "1". */

    PackM(RLPU_SABM);

    break;

  case RLPFT_U_UA:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    ClearCRBit; /* The UA encoding is used as a response only. */

    PackM(RLPU_UA);

    break;

  case RLPFT_U_DISC:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    SetCRBit; /* The DISC encoding is used as a command only. */

    PackM(RLPU_DISC);

    break;

  case RLPFT_U_DM:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    ClearCRBit; /* The DM encoding is used as a response only. */

    PackM(RLPU_DM);

    break;

  case RLPFT_U_NULL:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    PackM(RLPU_NULL);

    break;

  case RLPFT_U_UI:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    PackM(RLPU_UI);

    break;

  case RLPFT_U_XID:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    SetPFBit; /* XID frames are always used with the P/F-bit set to "1". */

    PackM(RLPU_XID);

    break;

  case RLPFT_U_TEST:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    PackM(RLPU_TEST);

    break;

  case RLPFT_U_REMAP:

    frame.Header[0]|=0xf8;
    frame.Header[1]|=0x01;

    ClearPFBit; /* REMAP frames are always used with P/F-bit set to "0". */

    PackM(RLPU_REMAP);

    break;

  case RLPFT_S_RR:

    frame.Header[0]|=0xf0;  /* See page 11 of the GSM 04.22 spec - 0 X X 1 1 1 1 1 */
    frame.Header[1]|=0x01; /* 1 P/F ...N(R)... */

    PackNR;

    PackS(RLPS_RR);

    break;

  case RLPFT_S_REJ:

    frame.Header[0]|=0xf0;
    frame.Header[1]|=0x01;

    PackNR;

    PackS(RLPS_REJ);

    break;

  case RLPFT_S_RNR:

    frame.Header[0]|=0xf0;
    frame.Header[1]|=0x01;

    PackNR;

    PackS(RLPS_RNR);

    break;

  case RLPFT_S_SREJ:

    frame.Header[0]|=0xf0;
    frame.Header[1]|=0x01;

    PackNR;

    PackS(RLPS_SREJ);

    break;

  case RLPFT_SI_RR:

    PackNR;
    PackNS;

    PackS(RLPS_RR);

    break;

  case RLPFT_SI_REJ:
    PackNR;
    PackNS;

    PackS(RLPS_REJ);

    break;

  case RLPFT_SI_RNR:

    PackNR;
    PackNS;

    PackS(RLPS_RNR);

    break;

  case RLPFT_SI_SREJ:
    PackNR;
    PackNS;

    PackS(RLPS_SREJ);

    break;

  default:
    break;
  }


  /* u8 req[64]; */

  /* Store FCS in the frame. */
  RLP_CalculateCRC24Checksum((u8 *)&frame, 27, frame.FCS);

  // X(&frame);

  //  RLP_DisplayF96Frame(&frame);

  memcpy(req+2, (u8 *) &frame, 32);

  /* FIXME: how should we send RLP frames? Some pointer to the actual
     function? */

  FB61_TX_SendFrame(32, 0xf0, req);
}

/* Check_input_PDU in Serge's code. */

void RLP_DisplayF96Frame(RLP_F96Frame *frame)
{

  int           count;
  RLP_F96Header header;

  /*
    FIXME: IncrementTimes();
  */

  CurrentFrameType=RLPFT_BAD;

  /* Check FCS. */

  if (RLP_CheckCRC24FCS((u8 *)frame, 30) == true) {

    /* Here we have correct RLP frame so we can parse the field of the header
       to out structure. */

    RLP_DecodeF96Header(frame, &header);

    switch (header.Type) {

    case RLPFT_U: /* Unnumbered frames. */

#ifdef DEBUG
      fprintf(stdout, "Unnumbered Frame [$%02x%02x] M=%02x ", frame->Header[0],
                                                              frame->Header[1],
                                                              header.M);
#endif

      switch (header.M) {

      case RLPU_SABM :

#ifdef DEBUG
	fprintf(stdout, "Set Asynchronous Balanced Mode (SABM) ");
#endif

	CurrentFrameType=RLPFT_U_SABM;

	break;

      case RLPU_UA:

#ifdef DEBUG
	fprintf(stdout, "Unnumbered Acknowledge (UA) ");
#endif

	CurrentFrameType=RLPFT_U_UA;

	break;

      case RLPU_DISC:

#ifdef DEBUG
	fprintf(stdout, "Disconnect (DISC) ");
#endif

	CurrentFrameType=RLPFT_U_DISC;

	break;

      case RLPU_DM:

#ifdef DEBUG
	fprintf(stdout, "Disconnected Mode (DM) ");
#endif
	CurrentFrameType=RLPFT_U_DM;

	break;

      case RLPU_UI:

#ifdef DEBUG
	fprintf(stdout, "Unnumbered Information (UI) ");
#endif

	CurrentFrameType=RLPFT_U_UI;

	break;

      case RLPU_XID:

#ifdef DEBUG
	fprintf(stdout, "Exchange Information (XID) \n");
	RLP_DisplayXID(frame->Data);
#endif

	CurrentFrameType=RLPFT_U_XID;

	break;

      case RLPU_TEST:

#ifdef DEBUG
	fprintf(stdout, "Test (TEST) ");
#endif

	CurrentFrameType=RLPFT_U_TEST;

	break;

      case RLPU_NULL:

#ifdef DEBUG
	fprintf(stdout, "Null information (NULL) ");
#endif

	CurrentFrameType=RLPFT_U_NULL;

	break;

      case RLPU_REMAP:

#ifdef DEBUG
	fprintf(stdout, "Remap (REMAP) ");
#endif

	CurrentFrameType=RLPFT_U_REMAP;

	break;
                    
      default :

#ifdef DEBUG
	fprintf(stdout, _("Unknown!!! "));
#endif

	CurrentFrameType=RLPFT_BAD;

	break;

      }
      break;
            
    case RLPFT_S: /* Supervisory frames. */

#ifdef DEBUG
      fprintf(stdout, "Supervisory Frame [$%02x%02x] S=%x N(R)=%02x ",
                      frame->Header[0],
                      frame->Header[1],
	              header.S,
                      header.Nr);
#endif

      switch (header.S) {

      case RLPS_RR:

#ifdef DEBUG
      fprintf(stdout, "RR");
#endif

      CurrentFrameType=RLPFT_S_RR;

	break;

      case RLPS_REJ:

#ifdef DEBUG
      fprintf(stdout, "REJ");
#endif

      CurrentFrameType=RLPFT_S_REJ;

	break;

      case RLPS_RNR:

#ifdef DEBUG
      fprintf(stdout, "RNR");
#endif

      CurrentFrameType=RLPFT_S_RNR;

	break;

      case RLPS_SREJ:

#ifdef DEBUG
      fprintf(stdout, "SREJ");
#endif

      CurrentFrameType=RLPFT_S_SREJ;

	break;

      default:

#ifdef DEBUG
      fprintf(stdout, _("BAD"));
#endif

      CurrentFrameType=RLPFT_BAD;

      }

      break;
            
    default:

#ifdef DEBUG
      fprintf(stdout, "Info+Supervisory Frame [$%02x%02x] S=%x N(S)=%02x N(R)=%02x ",
                      frame->Header[0],
                      frame->Header[1],
                      header.S,
                      header.Ns,
                      header.Nr);
#endif

      switch (header.S) {

      case RLPS_RR:

#ifdef DEBUG
      fprintf(stdout, "RR");
#endif

      CurrentFrameType=RLPFT_SI_RR;

	break;

      case RLPS_REJ:

#ifdef DEBUG
      fprintf(stdout, "REJ");
#endif

      CurrentFrameType=RLPFT_SI_REJ;

	break;

      case RLPS_RNR:

#ifdef DEBUG
      fprintf(stdout, "RNR");
#endif

      CurrentFrameType=RLPFT_SI_RNR;

	break;

      case RLPS_SREJ:

#ifdef DEBUG
      fprintf(stdout, "SREJ");
#endif

      CurrentFrameType=RLPFT_SI_SREJ;

	break;

      default:

#ifdef DEBUG
      fprintf(stdout, "BAD");
#endif

      CurrentFrameType=RLPFT_BAD;

      }

      break;
    }   

#ifdef DEBUG

    /* Command/Response and Poll/Final bits. */

    fprintf(stdout, " C/R=%d P/F=%d", header.CR, header.PF);

    /* Information. */
    
    if (CurrentFrameType!=RLPFT_U_NULL) {

      fprintf(stdout, "\n");

      for (count = 0; count < 25; count ++) {

	if (isprint(frame->Data[count]))
	  fprintf(stdout, "[%02x%c]", frame->Data[count], frame->Data[count]);
	else
	  fprintf(stdout, "[%02x ]", frame->Data[count]);

	if (count == 15)
	  fprintf(stdout, "\n");
      }
    }

    /* FCS. */
    
    fprintf (stdout, " FCS: %02x %02x %02x\n\n", frame->FCS[0],
                                                 frame->FCS[1],
                                                 frame->FCS[2]);

    fflush(stdout);

#endif

  }
  else {

    /* RLP Checksum failed - don't we need some statistics about these
       failures? Nothing is printed, because in the first stage of connection
       there are too many bad RLP frames... */

#ifdef DEBUG
    fprintf(stdout, _("Frame FCS is bad. Ignoring...\n"));
#endif DEBUG

  }

  MAIN_STATE_MACHINE(frame, &header);

  /*
    Y:= outblock();
  */

  return;
}

bool XID_Handling (RLP_F96Frame *frame, RLP_F96Header *header) {

  if (CurrentFrameType == RLPFT_U_XID) {

    u8 Data[25];
    int i;

    fprintf(stdout, "DEBUG: XID received in state 1.\n");

    for (i=0; i<25; i++) {
      printf("XID data[%02d]: %02x\n", i, frame->Data[i]);
      Data[i]=frame->Data[i];
    }
    
    RLP_SendF96Frame(RLPFT_U_XID, false, false, 0, 0, Data, false);

    return true;
  }

  return false;
}

/* FIXME: temporary.  */

#define RLP_UserEvent(x) true

void MAIN_STATE_MACHINE(RLP_F96Frame *frame, RLP_F96Header *header) {

  switch (CurrentState) {


    /***** RLP State 0. *****/


    /* ADM and Detached.

       This is the initial state after power on.

       As long as the RLP entity is "Detached", DISC(P) and/or SABM at the
       lower interface is acted upon by sending DM(P) or DM(1). Any other
       stimulus at the lower interface is ignored.

       This state can be exited only with Attach_Req. */

  case RLP_S0:

#ifdef DEBUG
    fprintf(stdout, _("RLP state 0.\n"));
#endif

    switch (CurrentFrameType) {

      /* FIXME: Do we really need DISC and SABM handling? */

    case RLPFT_U_DISC:

      RLP_SendF96Frame(RLPFT_U_DM, false, header->PF, 0, 0, NULL, false);

      break;

    case RLPFT_U_SABM:

      RLP_SendF96Frame(RLPFT_U_DM, false, true, 0, 0, NULL, false);

      break;

    default:

      RLP_SendF96Frame(RLPFT_U_NULL, false, false, 0, 0, NULL, false);

      if (RLP_UserEvent(Attach_Req)) {
	NextState=RLP_S1;
	UA_State=_idle;
      }
    }

    break;


    /***** RLP State 1. *****/


  case RLP_S1:

#ifdef DEBUG
    fprintf(stdout, _("RLP state 1.\n"));
#endif

    // FIXME: continue here...

    if (!XID_Handling(frame, header)) {

      static FIXMEcounter=0;

      FIXMEcounter++;
      printf("FIXMEcounter: %d\n", FIXMEcounter);
      if (FIXMEcounter==700)
	RLP_SendF96Frame(RLPFT_U_SABM, true, true, 0, 0, NULL, false);
      else
	RLP_SendF96Frame(RLPFT_U_NULL, false, false, 0, 0, NULL, false);

    }


    break;

  default:

  }

  CurrentState=NextState;

}

/* Given a pointer to an RLP XID frame, display contents in human readable
   form.  Note for now only Version 0 and 1 are supported.  Fields can appear
   in any order and are delimited by a zero type field. This function is the
   exact implementation of section 5.2.2.6, Exchange Identification, XID of
   the GSM specification 04.22. */

void RLP_DisplayXID(u8 *frame) 
{

  int count = 25;  /* Sanity check */
  u8  type, length;
    
  fprintf(stdout, "XID: ");

  while ((*frame !=0) && (count >= 0)) {

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
    count --;
    frame ++;
  } 

  return;
}

/* Given a pointer to an F9.6 Frame, split data out into component parts of
   header and determine frame type. */

void RLP_DecodeF96Header(RLP_F96Frame *frame, RLP_F96Header *header)
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

    header->Type = RLPFT_U;
    header->M = (frame->Header[1] >> 2) & 0x1f;
    return; /* For U frames, we do not need N(R) and bits S1 and S2. */
                    
  case 0x3e: /* Frames of type S, supervisory frames. */

    header->Type = RLPFT_S;
    break;
                    
  default: /* Frames of type I+S, numbered information transfer ans
              supervisory frames combined. */

    header->Type = RLPFT_IS;
    break;
  }

  /* Receive Sequence Number N(R). */
  header->Nr = frame->Header[1] >> 2;

  /* Status bits (S1 and S2). */
  header->S = (frame->Header[0] >> 1) & 0x03;

  return;
}
