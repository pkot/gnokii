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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001      Chris Kemp, Pavel Machek
  Copyright (C) 2003      Pawel Kot

  The development of RLP protocol was sponsored by SuSE CR, s.r.o. (Pavel use
  the SIM card from SuSE for testing purposes).

  Header file for RLP protocol.

*/

#ifndef _gnokii_rlp_common_h
#define _gnokii_rlp_common_h

/* Typedef for frame type - they are the same for RLP version 0, 1 and 2. */
typedef enum {
	RLP_FT_X, /* Unknown. */
	RLP_FT_U, /* Unnumbered frame. */
	RLP_FT_S, /* Supervisory frame. */
	RLP_FT_IS /* Information plus Supervisory (I+S) frame. */
} rlp_frame_type;

/* Define the various Unnumbered frame types. Numbering is bit reversed
   relative to ETSI GSM 04.22 for easy parsing. */
typedef enum {
	RLP_U_SABM  = 0x07, /* Set Asynchronous Balanced Mode. */
	RLP_U_UA    = 0x0c, /* Unnumbered Acknowledge. */
	RLP_U_DISC  = 0x08, /* Disconnect. */
	RLP_U_DM    = 0x03, /* Disconnected Mode. */
	RLP_U_NULL  = 0x0f, /* Null information. */
	RLP_U_UI    = 0x00, /* Unnumbered Information. */
	RLP_U_XID   = 0x17, /* Exchange Identification. */
	RLP_U_TEST  = 0x1c, /* Test. */
	RLP_U_REMAP = 0x11  /* Remap. */
} rlp_uframe_type;

/* Define supervisory frame field. */
typedef enum {
	RLP_S_RR   = 0x00, /* Receive Ready. */
	RLP_S_REJ  = 0x02, /* Reject. */
	RLP_S_RNR  = 0x01, /* Receive Not Ready. */
	RLP_S_SREJ = 0x03  /* Selective Reject. */
} rlp_sframe_field;

/* Used for CurrentFrameType. */
typedef enum {
	RLP_FT_U_SABM = 0x00,
	RLP_FT_U_UA,
	RLP_FT_U_DISC,
	RLP_FT_U_DM,
	RLP_FT_U_NULL,
	RLP_FT_U_UI,
	RLP_FT_U_XID,
	RLP_FT_U_TEST,
	RLP_FT_U_REMAP,
	RLP_FT_S_RR,
	RLP_FT_S_REJ,
	RLP_FT_S_RNR,
	RLP_FT_S_SREJ,
	RLP_FT_SI_RR,
	RLP_FT_SI_REJ,
	RLP_FT_SI_RNR,
	RLP_FT_SI_SREJ,
	RLP_FT_BAD
} rlp_frame_types;

/* Frame definition for TCH/F9.6 frame. */
typedef struct {
	unsigned char Header[2];
	unsigned char Data[25];
	unsigned char FCS[3];
} gn_rlp_f96_frame;

/* Header data "split up" for TCH/F9.6 frame. */
typedef struct {
	unsigned char Ns;	/* Send sequence number. */
	unsigned char Nr;	/* Receive sequence number. */
	unsigned char M;	/* Unumbered frame type. */
	unsigned char S;	/* Status. */
	int PF;			/* Poll/Final. */
	int CR;			/* Command/Response. */
	rlp_frame_type Type;	/* Frame type. */
} rlp_f96_header;


/* RLP User requests */
typedef struct {
	int Conn_Req;
	int Attach_Req;
	int Conn_Req_Neg;
	int Reset_Resp;
	int Disc_Req;
} rlp_user_request_store;

typedef enum {
	Conn_Req,
	Attach_Req,
	Conn_Req_Neg,
	Reset_Resp,
	Disc_Req
} rlp_user_requests;

typedef enum {
	Conn_Ind,
	Conn_Conf,
	Disc_Ind,
	Reset_Ind,
	Data,		/* FIXME: This should really be called RLP_Data, otherwise it hogs name "Data"! */
	StatusChange,
	GetData
} rlp_user_inds;

/* RLP (main) states. See GSM specification 04.22 Annex A, Section A.1.1. */
typedef enum {
	RLP_S0, /* ADM and Detached */
	RLP_S1, /* ADM and Attached */
	RLP_S2, /* Pending Connect Request */
	RLP_S3, /* Pending Connect Indication */
	RLP_S4, /* ABM and Connection Established */
	RLP_S5, /* Disconnect Initiated */
	RLP_S6, /* Pending Reset Request */
	RLP_S7, /* Pending Reset Indication */
	RLP_S8  /* Error */
} rlp_state;

/* RLP specification defines several states in which variables can be. */
typedef enum {
	_idle=0,
	_send,
	_wait,
	_rcvd,
	_ackn,
	_rej,
	_srej
} rlp_state_variable;

/* RLP Data */
typedef struct {
	unsigned char Data[25];
	rlp_state_variable State;
} rlp_data;

/* Prototypes for functions. */
void rlp_f96_frame_display(gn_rlp_f96_frame *frame);
void rlp_f96_header_decode(gn_rlp_f96_frame *frame, rlp_f96_header *header);
void rlp_xid_display(unsigned char *frame);
void rlp_initialise(int (*rlp_send_function)(gn_rlp_f96_frame *frame, int out_dtx), int (*rlp_passup)(rlp_user_inds ind, unsigned char *buffer, int length));
void rlp_link_vars_init(void);
void rlp_user_request_set(rlp_user_requests type, int value);
void rlp_send(char *buffer, int length);

#endif	/* _gnokii_rlp_common_h */
