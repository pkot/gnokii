	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.

	   Copyright (C) 1999 Pavel Janík ml. & Hugh Blemings. 
	   Released under the terms of the GNU GPL, see file COPYING
	   for more details.
	
	   This file:  fbus-6110.h   Version 0.2.4

	   Header file for the various functions, definitions etc. used
	   to implement the handset interface.  See fbus-6110.c for more details. */

#ifndef		__fbus_6110_h
#define		__fbus_6110_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

#define		FB61_MAX_TRANSMIT_LENGTH			(256)
#define		FB61_MAX_RECEIVE_LENGTH 			(512)

/* Nokia 6110 supports phonebook entries of max. 16 characters and numbers of
   max. 30 digits */

#define		FB61_MAX_PHONEBOOK_NAME_LENGTH			(16)
#define		FB61_MAX_PHONEBOOK_NUMBER_LENGTH		(30)

/* Nokia 6110 has different numbers for Phone and SIM memory then GMT_??? */

#define FB61_MEMORY_PHONE 0x02
#define FB61_MEMORY_SIM 0x03

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

	/* Global variables */
extern bool					FB61_LinkOK;
extern GSM_Functions		FB61_Functions;
extern GSM_Information		FB61_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error	FB61_Initialise(char *port_device, bool enable_monitoring);
bool		FB61_OpenSerial(void);
void		FB61_Terminate(void);
int		FB61_TX_SendMessage(u8 message_length, u8 message_type, u8 *buffer);
int		FB61_TX_SendAck(u8 message_type, u8 message_seq);

GSM_Error	FB61_GetPhonebookLocation(GSM_MemoryType memory_type, int location,
				GSM_PhonebookEntry *entry);

GSM_Error	FB61_WritePhonebookLocation(GSM_MemoryType memory_type, 
				int location, GSM_PhonebookEntry *entry);

GSM_Error	FB61_GetSMSMessage(GSM_MemoryType memory_type, int location,
				 GSM_SMSMessage *message);

GSM_Error	FB61_DeleteSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message);

GSM_Error	FB61_SendSMSMessage(char *message_centre, char *destination,
				 char *text);

GSM_Error	FB61_GetRFLevel(float *level);
GSM_Error	FB61_GetBatteryLevel(float *level);
GSM_Error	FB61_EnterPin(char *pin);
GSM_Error	FB61_GetSMSCenter(u8 priority);
GSM_Error	FB61_GetIMEIAndCode(char *imei, char *code);
GSM_Error	FB61_GetDateTime(GSM_DateTime *date_time);
GSM_Error	FB61_SetDateTime(GSM_DateTime *date_time);
GSM_Error	FB61_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	FB61_SetAlarm(int alarm_number, GSM_DateTime *date_time);

	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __fbus_6110_c isn't 
	   defined. */
#ifdef	__fbus_6110_c

	/* States for receive code. */
enum	FB61_RX_States {FB61_RX_Sync,
			FB61_RX_GetDestination,
			FB61_RX_GetSource,
			FB61_RX_GetType,
			FB61_RX_GetUnknown,
			FB61_RX_GetLength,
			FB61_RX_GetMessage};

#define		FB61_BAUDRATE		(B115200)

void		FB61_Terminate(void);
void		FB61_ThreadLoop(void);
bool		FB61_OpenSerial(void);
void		FB61_SigHandler(int status);
void		FB61_RX_StateMachine(char rx_byte);
void		FB61_RX_DisplayMessage(void);
GSM_Error	FB61_GetSMSStatus() ;

	/* Insert code here as required ? */


#endif	/* __fbus_6110_c */

#endif	/* __fbus_6110_h */
