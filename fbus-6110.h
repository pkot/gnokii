	/* G N O K I I
	   A Linux/Unix toolset and driver for the Nokia 3110/6110/8110 mobiles.
	   Copyright (C) Hugh Blemings ?????, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  fbus-6110.h   Version 0.2.3

	   Header file for the various functions, definitions etc. used
	   to implement the handset interface.  See fbus-6110.c for more details. */

#ifndef		__fbus_6110_h
#define		__fbus_6110_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

#define         FB61_MAX_TRANSMIT_LENGTH                        (256)


/* This byte is at the beginning of all GSM Frames sent over FBUS to Nokia
   6110 phones */

#define FB61_FRAME_ID 0x1e

/* The next two macros define the source and destination addresses used for
   specifying the device connected on the cable. */

/* Nokia mobile phone. */
#define FB61_DEVICE_PHONE 0x00

/* Our PC. */
#define FB61_DEVICE_PC 0x0c

/* These macros are used to specify the type of the frame. Only few types are
   known yet - if you can cast some light on these, please let us known. */

/* Acknowledge of the received frame. */
#define FB61_FRTYPE_ACK 0x7f

/* This structure will represent each frame on the cable. */

typedef struct {

  u8 ID; /* Frame identification - should always be FB61_FRAME_ID (0x1e) */

  u8 Destination; /* The frame destination */
  u8 Source; /* The frame source - this should be FB61_DEVICE_PC when sending */

  u8 Type; /* The frame type */

  u8 Unknown1; /* Unknown - this should be MSB of the frame length */
  u8 Length; /* Length of the frame */

  u8 *Message; /* Message */

  /* The frame checksum */

  u8 CheckSumOdd; /* Checksum of odd bytes */
  u8 CheckSumEven; /* Checksum of even bytes */

} FB61_Frame;


	/* Global variables */
extern bool					FB61_LinkOK;
extern GSM_Functions		FB61_Functions;
extern GSM_Information		FB61_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error	FB61_Initialise(char *port_device, bool enable_monitoring);
bool		FB61_OpenSerial(void);
void		FB61_Terminate(void);
int		FB61_TX_SendMessage(u8 message_length, u8 message_type, u8 *buffer);
int		FB61_TX_SendAck(u8 message_type);

GSM_Error	FB61_GetPhonebookLocation(GSM_MemoryType memory_type, int location,
				GSM_PhonebookEntry *entry);

GSM_Error	FB61_WritePhonebookLocation(GSM_MemoryType memory_type, 
				int location, GSM_PhonebookEntry *entry);

GSM_Error	FB61_GetSMSMessage(GSM_MemoryType memory_type, int location,
				 GSM_SMSMessage *message);

GSM_Error	FB61_SendSMSMessage(char *message_centre, char *destination,
				 char *text, u8 *return_code1, u8 *return_code2);

GSM_Error	FB61_GetRFLevel(float *level);
GSM_Error	FB61_GetBatteryLevel(float *level);
GSM_Error	FB61_EnterPin(char *pin);
GSM_Error	FB61_GetIMEIAndCode(char *imei, char *code);

	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __fbus_6110_c isn't 
	   defined. */
#ifdef	__fbus_6110_c

#define		FB61_BAUDRATE		(B115200)

void		FB61_Terminate(void);
void		FB61_ThreadLoop(void);
bool		FB61_OpenSerial(void);
void		FB61_SigHandler(int status);

	/* Insert code here as required ? */


#endif	/* __fbus_6110_c */

#endif	/* __fbus_6110_h */
