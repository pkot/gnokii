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


void RLP_DisplayF96Frame(RLP_F96Frame *frame)
{

  int           count;
  RLP_F96Header header;

  /* Check FCS. */

  if (RLP_CheckCRC24FCS((u8 *)frame, 30) == true) {

    /* Here we have correct RLP frame so we can parse the field of the header
       to out structure. */

    RLP_DecodeF96Header(frame, &header);

    switch (header.Type) {

    case RLPFT_U: /* Unnumbered frames. */

      fprintf(stdout, _("Unnumbered Frame M=%02x "), header.M);

      switch (header.M) {

      case RLPU_SABM :

	fprintf(stdout, _("Set Asynchronous Balanced Mode (SABM) "));
	break;

      case RLPU_UA:

	fprintf(stdout, _("Unnumbered Acknowledge (UA) "));
	break;

      case RLPU_DISC:

	fprintf(stdout, _("Disconnect (DISC) "));
	break;

      case RLPU_DM:

	fprintf(stdout, _("Disconnected Mode (DM) "));
	break;

      case RLPU_UI:

	fprintf(stdout, _("Unnumbered Information (UI) "));
	break;

      case RLPU_XID:

	fprintf(stdout, _("Exchange Information (XID) \n"));
	RLP_DisplayXID(frame->Data);
	break;

      case RLPU_TEST:

	fprintf(stdout, _("Test (TEST) "));
	break;

      case RLPU_NULL:

	fprintf(stdout, _("Null information (NULL) "));
	break;

      case RLPU_REMAP:

	fprintf(stdout, _("Remap (REMAP) "));
	break;
                    
      default :

	fprintf(stdout, _("Unknown!!! "));
	break;

      }
      break;
            
    case RLPFT_S: /* Supervisory frames. */

      fprintf(stdout, _("Supervisory Frame S=%x N(R)=%02x"), header.S, header.Nr);
      break;
            
    default:

      fprintf(stdout, _("Info+Supervisory Frame S=%x N(S)=%02x N(R)=%02x"), header.S, header.Ns, header.Nr);
      break;
    }   

    /* Command/Response and Poll/Final bits. */

    fprintf(stdout, _(" C/R=%d P/F=%d \n"), header.CR, header.PF);

    /* Information. */
    
    for (count = 0; count < 25; count ++) {

      if (isprint(frame->Data[count]))
	fprintf(stdout, "[%02x%c]", frame->Data[count], frame->Data[count]);
      else
	fprintf(stdout, "[%02x ]", frame->Data[count]);

      if (count == 15)
	fprintf(stdout, "\n");
    }

    /* FCS. */
    
    fprintf (stdout, _(" FCS: %02x %02x %02x"), frame->FCS[0], frame->FCS[1], frame->FCS[2]);

  fprintf(stdout, "\n\n");
  fflush(stdout);

  }
  else {

    /* RLP Checksum failed - don't we need some statistics about these
       failures? Nothing is printed, because in the first stage of connection
       there are too many bad RLP frames... */

  }

  return;
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
    
  fprintf(stdout, _("XID: "));

  while ((*frame !=0) && (count >= 0)) {

    type = *frame >> 4;
    length = *frame & 0x0f;

    switch (type) {

    case 0x01: /* RLP Version Number, probably 1 for Nokia. */

      frame += length;
      fprintf(stdout, _("Ver %d "), *frame);
      break;
                
    case 0x02: /* IWF to MS window size */

      frame += length;
      fprintf(stdout, _("IWF-MS %d "), *frame);
      break;
                
    case 0x03: /* MS to IWF window size. */

      frame += length;
      fprintf(stdout, _("MS-IWF %d "), *frame);
      break;

    case 0x04: /* Acknowledgement Timer (T1). */

      frame += length;
      fprintf(stdout, _("T1 %dms "), *frame * 10);
      break;

    case 0x05: /* Retransmission attempts (N2). */

      frame += length;
      fprintf(stdout, _("N2 %d "), *frame);
      break;

    case 0x06: /* Reply delay (T2). */

      frame += length;
      fprintf(stdout, _("T2 %dms "), *frame * 10);
      break;

    case 0x07: /* Compression. */

      frame ++;
      fprintf(stdout, _("Comp [Pt=%d "), (*frame >> 4) );
      fprintf(stdout, _("P0=%d "), (*frame & 0x03) );

      frame ++;
      fprintf(stdout, _("P1l=%d "), *frame);
      frame ++;
      fprintf(stdout, _("P1h=%d "), *frame);

      frame ++;
      fprintf(stdout, _("P2=%d] "), *frame);
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
