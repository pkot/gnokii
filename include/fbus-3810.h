	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  fbus-3810.h   Version 0.2.4

	   Header file for the various functions, definitions etc. used
	   to implement the handset interface.  See fbus-3810.c for more details. */

#ifndef		__fbus_3810_h
#define		__fbus_3810_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

	/* Global variables */
extern bool					FB38_LinkOK;
extern GSM_Functions		FB38_Functions;
extern GSM_Information		FB38_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error	FB38_Initialise(char *port_device, bool enable_monitoring);
bool		FB38_OpenSerial(void);
void		FB38_Terminate(void);

GSM_Error	FB38_GetPhonebookLocation(GSM_MemoryType memory_type, int location,
				GSM_PhonebookEntry *entry);

GSM_Error	FB38_WritePhonebookLocation(GSM_MemoryType memory_type, 
				int location, GSM_PhonebookEntry *entry);

GSM_Error	FB38_GetSMSMessage(GSM_MemoryType memory_type, int location,
				 GSM_SMSMessage *message);

GSM_Error	FB38_DeleteSMSMessage(GSM_MemoryType memory_type, int location, GSM_SMSMessage *message);

GSM_Error	FB38_SendSMSMessage(char *message_centre, char *destination,
				 char *text, u8 *return_code1, u8 *return_code2);

	/* These aren't presently implemented. */
GSM_Error	FB38_GetRFLevel(float *level);
GSM_Error	FB38_GetBatteryLevel(float *level);
GSM_Error	FB38_EnterPin(char *pin);
GSM_Error	FB38_GetIMEIAndCode(char *imei, char *code);
GSM_Error	FB38_GetDateTime(GSM_DateTime *date_time);
GSM_Error	FB38_SetDateTime(GSM_DateTime *date_time);
GSM_Error	FB38_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	FB38_SetAlarm(int alarm_number, GSM_DateTime *date_time);


	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __fbus_3810_c isn't 
	   defined. */
#ifdef	__fbus_3810_c

	/* These set length limits for the handset.  This is SIM dependant
	   and ultimately will be set automatically once we know how
	   to get that information from phone.  INT values are
	   selected when the memory type specified is '1'
	   this corresponds to internal memory on an 8110 */
#define		FB38_DEFAULT_SIM_PHONEBOOK_NAME_LENGTH		(10)
#define		FB38_DEFAULT_SIM_PHONEBOOK_NUMBER_LENGTH	(30)

#define		FB38_DEFAULT_INT_PHONEBOOK_NAME_LENGTH		(20)
#define		FB38_DEFAULT_INT_PHONEBOOK_NUMBER_LENGTH	(30)

	/* Miscellaneous values. */
#define		FB38_MAX_RECEIVE_LENGTH 			(512)
#define		FB38_MAX_TRANSMIT_LENGTH			(256)
#define 	FB38_BAUDRATE 						(B115200)

	/* Limits for sizing of array in FB38_PhonebookEntry */
#define		FB38_MAX_PHONEBOOK_NAME_LENGTH				(30)
#define		FB38_MAX_PHONEBOOK_NUMBER_LENGTH			(30)

	/* Limits to do with SMS messages. */
#define		FB38_MAX_MSG_CENTRE_LENGTH					(30)
#define		FB38_MAX_SENDER_LENGTH						(30)
#define		FB38_MAX_SMS_LENGTH							(160)

	/* States for receive code. */
enum	FB38_RX_States {FB38_RX_Sync,
						FB38_RX_GetLength,
						FB38_RX_GetMessage,
						FB38_RX_Off};

	/* Prototypes for internal functions. */
void	FB38_SigHandler(int status);
void	FB38_ThreadLoop(void);

void	FB38_RX_StateMachine(char rx_byte);
enum FB38_RX_States	FB38_RX_DispatchMessage(void);
void	FB38_RX_DisplayMessage(void);

void    FB38_RX_Handle0x0b_IncomingCall(void);
void    FB38_RX_Handle0x4b_Status(void);
void    FB38_RX_Handle0x10_EndOfOutgoingCall(void);
void    FB38_RX_Handle0x11_EndOfIncomingCall(void);
void    FB38_RX_Handle0x12_EndOfOutgoingCall(void);
void	FB38_RX_Handle0x27_SMSMessageText(void);
void	FB38_RX_Handle0x30_IncomingSMSNotification(void);
void    FB38_RX_Handle0x0d_IncomingCallAnswered(void);
void    FB38_RX_Handle0x0e_OutgoingCallAnswered(void);
void	FB38_RX_Handle0x2c_SMSHeader(void);
void	FB38_RX_Handle0x41_SMSMessageCentreData(void);
void    FB38_RX_Handle0x46_MemoryLocationData(void);

void    FB38_TX_UpdateSequenceNumber(void);
int     FB38_TX_SendStandardAcknowledge(u8 message_type);

int		FB38_TX_SendMessage(u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer);
void 	FB38_TX_Send0x25_RequestSMSMemoryLocation(u8 memory_type, u8 location); 
void 	FB38_TX_Send0x26_DeleteSMSMemoryLocation(u8 memory_type, u8 location);

void	FB38_TX_Send0x23_SendSMSHeader(char *message_centre, char *destination, u8 total_length);
void	FB38_TX_Send0x27_SendSMSMessageText(u8 block_number, u8 block_length, char *text);
void 	FB38_TX_Send0x3fMessage(void); 
void 	FB38_TX_Send0x4aMessage(void);
int		FB38_TX_Send0x42_WriteMemoryLocation(u8 memory_area, u8 location, char *label, char *number);
void    FB38_TX_Send0x43_RequestMemoryLocation(u8 memory_area, u8 location);
void    FB38_TX_Send0x15Message(u8 sequence_number);


#endif	/* __fbus_3810_c */

#endif	/* __fbus_3810_h */
