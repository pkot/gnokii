    /* G N O K I I
       A Linux/Unix toolset and driver for Nokia mobile phones.
       Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
    
       This file:  rlp-common.c   Version 0.3.1

       Common/generic RLP functions. */

#include    <stdio.h>
#include    <ctype.h>


#include    "rlp-common.h"
#include    "rlp-crc24.h"
#include    "misc.h"        /* For u8, u32 etc. */

void MAIN_STATE_MACHINE(RLP_F96Frame *frame);

/* This is the type we are just handling. */
RLP_FrameTypes CurrentFrameType;

/* Current state of RLP state machine. */
RLP_State      CurrentState=RLP_S0; /* We start at ADM and Detached */

/* Next state of RLP state machine. */
RLP_State      NextState;


/* State variables - see GSM 04.22, Annex A, section A.1.2 */

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

/* This function is used for sending RLP frames to the phone. */

void RLP_SendF96Frame(RLP_FrameTypes FrameType, u8 OuTCR, u8 OutPF,
                      u8 OutNR, u8 OutNS,
                      u8 *OutData)

{

  /* RLP_F96Frame frame; */

  /* u8 req[64]; */

  /* Store FCS in the frame. */

  // RLP_CheckCRC24FCS((u8 *)frame, 30);

}

/* Check_input_PDU in Serge's code. */

void RLP_DisplayF96Frame(RLP_F96Frame *frame)
{

  int           count;
  RLP_F96Header header;

  /*
    IncrementTimes();
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
      fprintf(stdout, "Unnumbered Frame M=%02x ", header.M);
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
      fprintf(stdout, "Supervisory Frame S=%x N(R)=%02x", header.S, header.Nr);
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
      fprintf(stdout, "Info+Supervisory Frame S=%x N(S)=%02x N(R)=%02x ", header.S, header.Ns, header.Nr);
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
      fprintf(stdout, "RRN");
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

    fprintf(stdout, " C/R=%d P/F=%d ", header.CR, header.PF);

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

  }

  MAIN_STATE_MACHINE(frame);

  /*
    Y:= outblock();
  */

  return;
}

void MAIN_STATE_MACHINE(RLP_F96Frame *frame) {

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
