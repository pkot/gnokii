	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  rlp-common.h   Version 0.3.1

	   Header file for various functions, definitions and datatypes
	   etc. used in RLP handling. */

#ifndef		__rlp_common_h
#define		__rlp_common_h

#ifndef     __misc_h
    #include    "misc.h"
#endif

	/* Global variables */

    /* Defines */
  
    /* Data types */

    /* Typedef for frame type - they are the same for RLP version 0,1 and 2  */
typedef enum {
    RLPFT_X,    /* Unknown */
    RLPFT_U,    /* Unnumbered Frame */
    RLPFT_S,    /* Supervisory Frame */
    RLPFT_IS    /* Information plus Supervisory (I+S) frame */
}   RLP_FrameType;

    /* Define the various Unnumbered frame types.  Numbering is
       as we receive it - bit reversed relative to ETSI 04.22 */ 
typedef enum {
    RLPU_SABM   = 0x07,     /* Set Asynchronous Balanced Mode */
    RLPU_UA     = 0x0c,     /* Unnumbered Acknowledge */
    RLPU_DISC   = 0x08,     /* Disconnect */
    RLPU_DM     = 0x03,     /* Disconnected Mode */
    RLPU_NULL   = 0x0f,     /* Null information */
    RLPU_UI     = 0x00,     /* Unnumbered Information */
    RLPU_XID    = 0x17,     /* Exchange Identification */
    RLPU_TEST   = 0x1c,     /* Test */
    RLPU_REMAP  = 0x11      /* Remap */
}   RLP_UFrameType;

    /* Define supervisory frame field */
typedef enum {
    RLPS_RR     = 0x00,     /* Receive Ready */
    RLPS_REJ    = 0x02,     /* Reject */
    RLPS_RNR    = 0x01,     /* Receive Not Ready */
    RLPS_SREJ   = 0x03      /* Selective Reject */
}   RLP_SFrameField;


    /* Frame definition for TCH/F9.6 Frame */
typedef struct {
    u8      Header[2];
    u8      Data[25];    
    u8      FCS[3];
}   RLP_F96Frame;   

    /* Header data "split up" for TCH/F9.6 Frame */
typedef struct {
    u8              Ns;     /* Send sequence number */
    u8              Nr;     /* Receive sequence number */
    u8              M;      /* Unumbered frame type */
    u8              S;      /* Status */
    bool            PF;     /* Poll/Final */
    bool            CR;     /* Command/Response */
    RLP_FrameType   Type;   /* Frame type */

}   RLP_F96Header;


	/* Prototypes for functions */
void    RLP_DisplayF96Frame(RLP_F96Frame *frame);
void    RLP_DecodeF96Header(RLP_F96Frame *frame, RLP_F96Header *header);
void    RLP_CalculateCRC24Checksum(u8 *data, int length, u8 *crc);
bool    RLP_CheckCRC24FCS(u8 *data, int length);
void    RLP_DisplayXID(u8 *frame);



#endif	/* __rlp_common_h */
