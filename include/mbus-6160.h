/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for 6160 code.	

  Last modification: Mon Mar 20 21:40:04 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef		__mbus_6160_h
#define		__mbus_6160_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

	/* Global variables */
extern bool					MB61_LinkOK;
extern GSM_Functions		MB61_Functions;
extern GSM_Information		MB61_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error   MB61_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame));

bool		MB61_OpenSerial(void);
void		MB61_Terminate(void);

int		MB61_GetMemoryType(GSM_MemoryType memory_type);

GSM_Error	MB61_GetMemoryLocation(GSM_PhonebookEntry *entry);

GSM_Error	MB61_WritePhonebookLocation(GSM_PhonebookEntry *entry);

GSM_Error	MB61_GetSpeedDial(GSM_SpeedDial *entry);

GSM_Error	MB61_SetSpeedDial(GSM_SpeedDial *entry);

GSM_Error	MB61_GetMemoryStatus(GSM_MemoryStatus *Status);

GSM_Error	MB61_GetSMSStatus(GSM_SMSStatus *Status);
GSM_Error       MB61_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error	MB61_GetSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB61_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error	MB61_SetSMSCenter(GSM_MessageCenter *MessageCenter);

GSM_Error	MB61_DeleteSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB61_SendSMSMessage(GSM_SMSMessage *SMS, int size);

GSM_Error	MB61_GetRFLevel(GSM_RFUnits *units, float *level);

GSM_Error	MB61_GetBatteryLevel(GSM_BatteryUnits *units, float *level);

	/* These aren't presently implemented. */
GSM_Error	MB61_GetPowerSource(GSM_PowerSource *source);
GSM_Error	MB61_GetDisplayStatus(int *Status);

GSM_Error	MB61_EnterSecurityCode(GSM_SecurityCode SecurityCode);
GSM_Error	MB61_GetSecurityCodeStatus(int *Status);

GSM_Error	MB61_GetIMEI(char *imei);
GSM_Error	MB61_GetRevision(char *revision);
GSM_Error	MB61_GetModel(char *model);
GSM_Error	MB61_GetDateTime(GSM_DateTime *date_time);
GSM_Error	MB61_SetDateTime(GSM_DateTime *date_time);
GSM_Error	MB61_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB61_SetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB61_DialVoice(char *Number);
GSM_Error	MB61_DialData(char *Number, char type);
GSM_Error	MB61_GetIncomingCallNr(char *Number);
GSM_Error	MB61_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo);
GSM_Error	MB61_GetCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB61_WriteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB61_DeleteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB61_Netmonitor(unsigned char mode, char *Screen);
GSM_Error	MB61_SendDTMF(char *String);
GSM_Error	MB61_GetBitmap(GSM_Bitmap *Bitmap);
GSM_Error	MB61_SetBitmap(GSM_Bitmap *Bitmap);
GSM_Error	MB61_Reset(unsigned char type);
GSM_Error	MB61_GetProfile(GSM_Profile *Profile);
GSM_Error	MB61_SetProfile(GSM_Profile *Profile);
bool  		MB61_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);

	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __mbus_6160_c isn't 
	   defined. */
#ifdef	__mbus_6160_c


	/* Prototypes for internal functions. */
void	MB61_ThreadLoop(void);
void    MB61_SigHandler(int status);
void    MB61_ThreadLoop(void);
void	MB61_UpdateSequenceNumber(void);

int     MB61_TX_SendMessage(u8 destination, u8 source, u8 command, u8 sequence_byte, int message_length, u8 *buffer);
void	MB61_TX_SendPhoneIDRequest(void);


#endif	/* __mbus_6160_c */

#endif	/* __mbus_6160_h */
