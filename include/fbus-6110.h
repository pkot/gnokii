/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See fbus-6110.c for more details.

  Last modification: Mon Mar 20 21:51:59 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef __fbus_6110_h
#define __fbus_6110_h

#ifndef __gsm_common_h
#include "gsm-common.h"
#endif

#define FB61_MAX_TRANSMIT_LENGTH (256)
#define FB61_MAX_RECEIVE_LENGTH  (512)
#define FB61_MAX_CONTENT_LENGTH  (120)

/* Nokia 6110 supports phonebook entries of max. 16 characters and numbers of
   max. 30 digits */

#define FB61_MAX_PHONEBOOK_NAME_LENGTH   (16)
#define FB61_MAX_PHONEBOOK_NUMBER_LENGTH (30)

/* Alternate defines for memory types.  Two letter codes follow GSM 07.07
   release 6.2.0, AT+CPBS "Select Phonebook Memory Storage" */

#define FB61_MEMORY_MT 0x01 /* combined ME and SIM phonebook */
#define FB61_MEMORY_ME 0x02 /* ME (Mobile Equipment) phonebook */
#define FB61_MEMORY_SM 0x03 /* SIM phonebook */
#define FB61_MEMORY_FD 0x04 /* SIM fixdialling-phonebook */
#define FB61_MEMORY_ON 0x05 /* SIM (or ME) own numbers list */
#define FB61_MEMORY_EN 0x06 /* SIM (or ME) emergency number */
#define FB61_MEMORY_DC 0x07 /* ME dialled calls list */
#define FB61_MEMORY_RC 0x08 /* ME received calls list */
#define FB61_MEMORY_MC 0x09 /* ME missed (unanswered received) calls list */

/* This is still unknown, it can also be unimplemented in CDS. */

// #define FB61_MEMORY_LD 0x?? /* SIM last-dialling-phonebook */

/* TA is not applicable (TA == computer) but it's here for 'completeness' */

// #define FB61_MEMORY_TA 0x?? /* TA (Terminal Adaptor) phonebook */

/* This is used when the memory type is unknown. */

#define FB61_MEMORY_XX 0xff

/* Limits for IMEI, Revision and Model string storage. */

#define FB61_MAX_IMEI_LENGTH     (20)
#define FB61_MAX_REVISION_LENGTH (20)
#define FB61_MAX_MODEL_LENGTH    (8)

/* This byte is used to synchronize FBUS channel over cable or IR. */

#define FB61_SYNC_BYTE 0x55

/* This byte is at the beginning of all GSM Frames sent over FBUS to Nokia
   6110 phones. */

#define FB61_FRAME_ID 0x1e

/* Every (well, almost every) frame from the computer starts with this
   sequence. */

#define FB61_FRAME_HEADER 0x00, 0x01, 0x00

/* The FBUS interface is 115200 */

#define FB61_BAUDRATE (B115200)

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

/* Macros for infrared communication */

/* This byte is at the beginning of all GSM Frames sent over IR to Nokia 6110
   phones. */

#define FB61_IR_FRAME_ID 0x1c

/* Infrared communication starts on 9600 bps. */

#define FB61_IR_INIT_SPEED (B9600)

/* This byte is send after all FB61_SYNC_BYTE bytes. */

#define FB61_IR_END_INIT_BYTE  0xc1

/* Global variables */

extern bool            FB61_LinkOK;
extern GSM_Functions   FB61_Functions;
extern GSM_Information FB61_Information;


/* Prototypes for the functions designed to be used externally. */

GSM_Error FB61_Initialise(char *port_device, char *initlength,
                          GSM_ConnectionType connection,
                          void (*rlp_callback)(RLP_F96Frame *frame));

bool      FB61_OpenSerial(void);
bool      FB61_Open_Ir_Serial(void);
void      FB61_Terminate(void);
void      FB61_ThreadLoop(void);
void      FB61_SigHandler(int status);
void      FB61_RX_StateMachine(char rx_byte);
enum FB61_RX_States FB61_RX_DispatchMessage(void);
enum FB61_RX_States FB61_RX_HandleRLPMessage(void);
void      FB61_RX_DisplayMessage(void);

int       FB61_TX_SendMessage(u16 message_length, u8 message_type, u8 *buffer);
int       FB61_TX_SendAck(u8 message_type, u8 message_seq);
int       FB61_TX_SendFrame(u8 message_length, u8 message_type, u8 *buffer); 

int       FB61_GetMemoryType(GSM_MemoryType memory_type);
int       SemiOctetPack(char *Number, unsigned char *Output);

GSM_Error FB61_GetMemoryLocation(GSM_PhonebookEntry *entry);
GSM_Error FB61_WritePhonebookLocation(GSM_PhonebookEntry *entry);

GSM_Error FB61_GetSpeedDial(GSM_SpeedDial *entry);
GSM_Error FB61_SetSpeedDial(GSM_SpeedDial *entry);

GSM_Error FB61_GetMemoryStatus(GSM_MemoryStatus *Status);
GSM_Error FB61_GetSMSStatus(GSM_SMSStatus *Status);
GSM_Error FB61_GetSMSCenter(GSM_MessageCenter *MessageCenter);
  
GSM_Error FB61_GetSMSMessage(GSM_SMSMessage *Message);
GSM_Error FB61_DeleteSMSMessage(GSM_SMSMessage *Message);
GSM_Error FB61_SendSMSMessage(GSM_SMSMessage *Message, int size);

GSM_Error FB61_GetRFLevel(GSM_RFUnits *units, float *level);
GSM_Error FB61_GetBatteryLevel(GSM_BatteryUnits *units, float *level);
GSM_Error FB61_GetPowerSource(GSM_PowerSource *source);
GSM_Error FB61_GetDisplayStatus(int *Status);

GSM_Error FB61_EnterSecurityCode(GSM_SecurityCode SecurityCode);
GSM_Error FB61_GetSecurityCodeStatus(int *Status);

GSM_Error FB61_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error FB61_SetSMSCenter(GSM_MessageCenter *MessageCenter);

GSM_Error FB61_GetIMEI(char *imei);
GSM_Error FB61_GetRevision(char *revision);
GSM_Error FB61_GetModel(char *model);

GSM_Error FB61_GetDateTime(GSM_DateTime *date_time);
GSM_Error FB61_SetDateTime(GSM_DateTime *date_time);

GSM_Error FB61_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error FB61_SetAlarm(int alarm_number, GSM_DateTime *date_time);

GSM_Error FB61_DialVoice(char *Number);
GSM_Error FB61_DialData(char *Number, char type,  void (* callpassup)(char c));

GSM_Error FB61_GetIncomingCallNr(char *Number);

GSM_Error FB61_SendBitmap(char *NetworkCode, int width, int height,
                          unsigned char *bitmap);

GSM_Error FB61_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo);

GSM_Error FB61_GetCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error FB61_WriteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error FB61_DeleteCalendarNote(GSM_CalendarNote *CalendarNote);

GSM_Error FB61_NetMonitor(unsigned char mode, char *Screen);

GSM_Error FB61_SetBitmap( GSM_Bitmap *Bitmap );
GSM_Error FB61_GetBitmap( GSM_Bitmap *Bitmap );

GSM_Error FB61_SendDTMF(char *String);

GSM_Error FB61_Reset(unsigned char type);

GSM_Error FB61_GetProfile(GSM_Profile *Profile);
GSM_Error FB61_SetProfile(GSM_Profile *Profile);
bool      FB61_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);

GSM_Error FB61_CancelCall(void);

/* States for receive code. */

enum FB61_RX_States {
  FB61_RX_Sync,
  FB61_RX_GetDestination,
  FB61_RX_GetSource,
  FB61_RX_GetType,
  FB61_RX_GetUnknown,
  FB61_RX_GetLength,
  FB61_RX_GetMessage
};

GSM_Error FB61_TX_SendStatusRequest(void);

#endif /* __fbus_6110_h */
