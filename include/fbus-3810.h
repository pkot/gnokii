/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for 3810 code.
	
  Last modification: Fri May 19 15:31:26 EST 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#ifndef     __fbus_3810_h
#define     __fbus_3810_h

#ifndef     __gsm_common_h
#include    "gsm-common.h"  /* Needed for GSM_Error etc. */
#endif

    /* Global variables */
extern bool                 FB38_LinkOK;
extern GSM_Functions        FB38_Functions;
extern GSM_Information      FB38_Information;
extern void                 (*FB38_RLP_RXCallback)(RLP_F96Frame *frame);

    /* Prototypes for the functions designed to be used externally. */
GSM_Error   FB38_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame));

bool        FB38_OpenSerial(void);
void        FB38_Terminate(void);

int         FB38_GetMemoryType(GSM_MemoryType memory_type);

GSM_Error   FB38_GetMemoryLocation(GSM_PhonebookEntry *entry);

GSM_Error   FB38_WritePhonebookLocation(GSM_PhonebookEntry *entry);

GSM_Error   FB38_GetSpeedDial(GSM_SpeedDial *entry);

GSM_Error   FB38_SetSpeedDial(GSM_SpeedDial *entry);

GSM_Error   FB38_GetMemoryStatus(GSM_MemoryStatus *Status);

GSM_Error   FB38_GetSMSStatus(GSM_SMSStatus *Status);
GSM_Error   FB38_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error   FB38_GetSMSMessage(GSM_SMSMessage *message);

GSM_Error   FB38_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error   FB38_SetSMSCenter(GSM_MessageCenter *MessageCenter);

GSM_Error   FB38_DeleteSMSMessage(GSM_SMSMessage *message);

GSM_Error	FB38_CancelCall(void);

GSM_Error   FB38_SendSMSMessage(GSM_SMSMessage *SMS, int size);

GSM_Error   FB38_GetRFLevel(GSM_RFUnits *units, float *level);

GSM_Error   FB38_GetBatteryLevel(GSM_BatteryUnits *units, float *level);

bool		FB38_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);

    /* These aren't presently implemented. */
GSM_Error   FB38_GetPowerSource(GSM_PowerSource *source);
GSM_Error   FB38_GetDisplayStatus(int *Status);
GSM_Error   FB38_EnterSecurityCode(GSM_SecurityCode SecurityCode);
GSM_Error   FB38_GetSecurityCodeStatus(int *Status);
GSM_Error   FB38_GetIMEI(char *imei);
GSM_Error   FB38_GetRevision(char *revision);
GSM_Error   FB38_GetModel(char *model);
GSM_Error   FB38_GetDateTime(GSM_DateTime *date_time);
GSM_Error   FB38_SetDateTime(GSM_DateTime *date_time);
GSM_Error   FB38_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error   FB38_SetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error   FB38_DialVoice(char *Number);
GSM_Error   FB38_DialData(char *Number, char type,void (* callpassup)(char c));
GSM_Error   FB38_GetIncomingCallNr(char *Number);
GSM_Error   FB38_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo);
GSM_Error   FB38_GetCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error   FB38_WriteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error   FB38_DeleteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error   FB38_Netmonitor(unsigned char mode, char *Screen);
GSM_Error   FB38_SendDTMF(char *String);
GSM_Error   FB38_GetBitmap(GSM_Bitmap *Bitmap);
GSM_Error   FB38_SetBitmap(GSM_Bitmap *Bitmap);
GSM_Error   FB38_Reset(unsigned char type);
GSM_Error   FB38_GetProfile(GSM_Profile *Profile);
GSM_Error   FB38_SetProfile(GSM_Profile *Profile);

    /* All defines and prototypes from here down are specific to 
       this model and so are #ifdef out if __fbus_3810_c isn't 
       defined. */
#ifdef  __fbus_3810_c

    /* These set length limits for the handset.  This is SIM dependant
       and ultimately will be set automatically once we know how
       to get that information from phone.  INT values are
       selected when the memory type specified is '1'
       this corresponds to internal memory on an 8110 */
#define     FB38_DEFAULT_SIM_PHONEBOOK_NAME_LENGTH      (10)
#define     FB38_DEFAULT_SIM_PHONEBOOK_NUMBER_LENGTH    (30)

#define     FB38_DEFAULT_INT_PHONEBOOK_NAME_LENGTH      (20)
#define     FB38_DEFAULT_INT_PHONEBOOK_NUMBER_LENGTH    (30)

    /* Number of times to try resending SMS (empirical) */
#define     FB38_SMS_SEND_RETRY_COUNT                   (4)

    /* Miscellaneous values. */
#define     FB38_MAX_RECEIVE_LENGTH             (512)
#define     FB38_MAX_TRANSMIT_LENGTH            (256)
#define     FB38_BAUDRATE                       (B115200)

    /* Limits for IMEI, Revision and Model string storage. */
#define     FB38_MAX_IMEI_LENGTH            (20)
#define     FB38_MAX_REVISION_LENGTH        (10)
#define     FB38_MAX_MODEL_LENGTH           (8)

    /* Limits for sizing of array in FB38_PhonebookEntry */
#define     FB38_MAX_PHONEBOOK_NAME_LENGTH              (30)
#define     FB38_MAX_PHONEBOOK_NUMBER_LENGTH            (30)

    /* Limits to do with SMS messages. */
#define     FB38_MAX_SMS_CENTER_LENGTH                  (30)
#define     FB38_MAX_SENDER_LENGTH                      (30)
#define     FB38_MAX_SMS_LENGTH                         (160)

    /* States for receive code. */
enum    FB38_RX_States {FB38_RX_Sync,
                        FB38_RX_GetLength,
                        FB38_RX_GetMessage,
                        FB38_RX_Off};

    /* Prototypes for internal functions. */
void    FB38_SigHandler(int status);
void    FB38_ThreadLoop(void);

void    FB38_RX_StateMachine(char rx_byte);
enum    FB38_RX_States FB38_RX_DispatchMessage(void);
enum    FB38_RX_States FB38_RX_HandleRLPMessage(void);
void    FB38_RX_DisplayMessage(void);
void    FB38_RX_Handle0x0b_IncomingCall(void);
void    FB38_RX_Handle0x4b_Status(void);
void    FB38_RX_Handle0x10_EndOfOutgoingCall(void);
void    FB38_RX_Handle0x11_EndOfIncomingCall(void);
void    FB38_RX_Handle0x12_EndOfOutgoingCall(void);
void    FB38_RX_Handle0x27_SMSMessageText(void);
void    FB38_RX_Handle0x30_IncomingSMSNotification(void);
void	FB38_RX_Handle0x32_SMSDelivered(void);
void    FB38_RX_Handle0x0d_IncomingCallAnswered(void);
void    FB38_RX_Handle0x0e_CallEstablished(void);
void    FB38_RX_Handle0x2c_SMSHeader(void);
void    FB38_RX_Handle0x41_SMSMessageCenterData(void);
void    FB38_RX_Handle0x46_MemoryLocationData(void);
void    FB38_RX_Handle0x4d_IMEIRevisionModelData(void);

void    FB38_TX_UpdateSequenceNumber(void);
int     FB38_TX_SendStandardAcknowledge(u8 message_type);
GSM_Error   FB38_TX_SendDialCommand(u8 call_type, char *Number);
bool 	FB38_TX_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);

int     FB38_TX_SendMessage(u8 message_length, u8 message_type, u8 sequence_byte, u8 *buffer);
void	FB38_TX_Send0x0f_HangupMessage(void);
void    FB38_TX_Send0x25_RequestSMSMemoryLocation(u8 memory_type, u8 location); 
void    FB38_TX_Send0x26_DeleteSMSMemoryLocation(u8 memory_type, u8 location);

void    FB38_TX_Send0x23_SendSMSHeader(char *message_center, char *destination, u8 total_length);
void    FB38_TX_Send0x27_SendSMSMessageText(u8 block_number, u8 block_length, char *text);
void    FB38_TX_Send0x3fMessage(void); 
void    FB38_TX_Send0x4aMessage(void);
int     FB38_TX_Send0x42_WriteMemoryLocation(u8 memory_area, u8 location, char *label, char *number);
void    FB38_TX_Send0x43_RequestMemoryLocation(u8 memory_area, u8 location);
void    FB38_TX_Send0x4c_RequestIMEIRevisionModelData(void);
void    FB38_TX_Send0x15Message(u8 sequence_number);
void    FB38_TX_SendExploreMessage(u8 message);

#endif  /* __fbus_3810_c */

#endif  /* __fbus_3810_h */
