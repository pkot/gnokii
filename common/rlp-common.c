    /* G N O K I I
       A Linux/Unix toolset and driver for Nokia mobile phones.
       Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
    
       This file:  rlp-common.c   Version 0.3.1

       Common/generic RLP functions. */

#include    <stdio.h>
#include    <ctype.h>


#include    "rlp-common.h"
#include    "misc.h"        /* For u8, u32 etc. */
#include    "crc24table.h"


void    RLP_DisplayF96Frame(RLP_F96Frame *frame)
{
    int             count;
    RLP_F96Header   header;

    RLP_DecodeF96Header(frame, &header);

    switch (header.Type) {
        case RLPFT_U:
            fprintf(stdout, _("Unnumbered Frame M=%02x "), header.M);
            switch (header.M) {
                case RLPU_SABM :
                    fprintf(stdout, _("Set Asynchronous Balanced Mode "));
                    break;

                case RLPU_UA:
                    fprintf(stdout, _("Unnumbered Acknowledge "));
                    break;

                case RLPU_DISC:
                    fprintf(stdout, _("Disconnect "));
                    break;

                case RLPU_DM:
                    fprintf(stdout, _("Disconnected Mode "));
                    break;

                case RLPU_NULL:
                    fprintf(stdout, _("Null information "));
                    break;

                case RLPU_UI:
                    fprintf(stdout, _("Unnumberd Information "));
                    break;

                case RLPU_XID:
                    fprintf(stdout, _("Exchange Information "));
                    break;

                case RLPU_TEST:
                    fprintf(stdout, _("Test "));
                    break;

                case RLPU_REMAP:
                    fprintf(stdout, _("Remap "));
                    break;
                    
                default :
                    fprintf(stdout, _("Unknown!!! "));
                    break;
            }
            break;
            
        case RLPFT_S: 
            fprintf(stdout, _("Supervisory Frame S=%x N(R)=%02x"), header.S, header.Nr);
            break;
            
        default:
            fprintf(stdout, _("Info+Supervisory Frame S=%x N(S)=%02x N(R)=%02x"), header.S, header.Ns, header.Nr);
            break;
    }   

    fprintf(stdout, _(" P/F=%d C/R=%d \n"), header.PF, header.CR);

    for (count = 0; count < 25; count ++) {
        if (isprint(frame->Data[count])) {
            fprintf(stdout, "[%02x%c]", frame->Data[count], frame->Data[count]);
        }
        else {
            fprintf(stdout, "[%02x ]", frame->Data[count]);
        }
        if (count == 15) {
            fprintf(stdout, "\n");
        }
    }
    
    fprintf (stdout, _(" FCS: %02x %02x %02x"), frame->FCS[0], frame->FCS[1], frame->FCS[2]);

    fprintf(stdout, "\n\n");
    fflush(stdout);
}


    /* Given a pointer to an F9.6 Frame, split data out into
       component parts of header and determine frame type. */
void    RLP_DecodeF96Header(RLP_F96Frame *frame, RLP_F96Header *header)
{
        /* Poll/Final bit */
    if ((frame->Header[1] & 0x02) == 0x02) {
        header->PF = true;
    }
    else {
        header->PF = false;
    }
    
        /* Command/Response bit */
    if ((frame->Header[0] & 0x01) == 0x01) {
        header->CR = true;
    }
    else {
        header->CR = false;
    }

        /* Send Sequence Number */  
    header->Ns = frame->Header[0] >> 3;
    if ((frame->Header[1] & 0x01) == 0x01) {
        header->Ns |= 0x20;
    }

        /* Determine frame type */
    switch (header->Ns) {
            /* Unnumbered frames have an M/frame type field instead of Nr */
        case 0x3f:  header->Type = RLPFT_U;
                    header->M = (frame->Header[1] >> 2) & 0x1f;
                    return;
                    
        case 0x3e:  header->Type = RLPFT_S;
                    break;
                    
        default:    header->Type = RLPFT_IS;
                    break;
    }

        /* Receive Sequence Number */
    header->Nr = frame->Header[1] >> 2;

        /* Status bits */
    header->S = (frame->Header[0] >> 1) & 0x03;

}

void    RLP_CalculateCRC24Polinomial(u8 *data, int length, u32 *polinomial)
{

    int     i;
    u8      cur;

    *polinomial = 0x00ffffff;

    for (i = 0; i < length; i++) {
        cur = (*polinomial & 0x0000ffff) ^ data[i];
        *polinomial = (*polinomial >> 8) ^ CRC24_Table[cur];
    }

    *polinomial = ((~*polinomial) & 0x00ffffff);
}


void    RLP_CalculateCRC24Checksum(u8 *data, int length, u8 *crc)
{
    u32     polinomial;

    RLP_CalculateCRC24Polinomial(data, length, &polinomial);
    crc[0] = polinomial & 0x0000ffff;
    crc[1] = (polinomial >> 8) & 0x0000ffff;
    crc[2] = (polinomial >> 16) & 0x0000ffff;

}

bool    RLP_CheckCRC24FCS(u8 *data, int length)
{

    u8     crc[] = { 0x00, 0x00, 0x00 };

    RLP_CalculateCRC24Checksum(data, length - 3, crc);

    if (((data[length - 3] == crc[0]) &&
        (data[length - 2] == crc[1]) &&
        (data[length - 1] == crc[2]))) {
        return (true);
    }
    return (false);
}

