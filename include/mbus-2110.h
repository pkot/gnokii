/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for 2110 code.	

  Last modification: Fri May 19 15:31:26 EST 2000
  Modified by Hugh Blemings <hugh@linuxcare.com>

*/

#ifndef		__mbus_2110_h
#define		__mbus_2110_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

	/* Global variables */
extern bool					MB21_LinkOK;
extern GSM_Functions		MB21_Functions;
extern GSM_Information		MB21_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error   MB21_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame));

bool		MB21_OpenSerial(void);
void		MB21_Terminate(void);

int		MB21_GetMemoryType(GSM_MemoryType memory_type);

GSM_Error	MB21_GetMemoryLocation(GSM_PhonebookEntry *entry);

GSM_Error	MB21_WritePhonebookLocation(GSM_PhonebookEntry *entry);

GSM_Error	MB21_GetSpeedDial(GSM_SpeedDial *entry);

GSM_Error	MB21_SetSpeedDial(GSM_SpeedDial *entry);

GSM_Error	MB21_GetMemoryStatus(GSM_MemoryStatus *Status);

GSM_Error	MB21_GetSMSStatus(GSM_SMSStatus *Status);
GSM_Error       MB21_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error	MB21_GetSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB21_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error	MB21_SetSMSCenter(GSM_MessageCenter *MessageCenter);

GSM_Error	MB21_DeleteSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB21_SendSMSMessage(GSM_SMSMessage *SMS, int size);
GSM_Error	MB21_SaveSMSMessage(GSM_SMSMessage *SMS);

GSM_Error	MB21_GetRFLevel(GSM_RFUnits *units, float *level);

GSM_Error	MB21_GetBatteryLevel(GSM_BatteryUnits *units, float *level);

	/* These aren't presently implemented. */
GSM_Error	MB21_GetPowerSource(GSM_PowerSource *source);
GSM_Error	MB21_GetDisplayStatus(int *Status);

GSM_Error	MB21_EnterSecurityCode(GSM_SecurityCode SecurityCode);
GSM_Error	MB21_GetSecurityCodeStatus(int *Status);

GSM_Error	MB21_GetIMEI(char *imei);
GSM_Error	MB21_GetRevision(char *revision);
GSM_Error	MB21_GetModel(char *model);
GSM_Error	MB21_GetDateTime(GSM_DateTime *date_time);
GSM_Error	MB21_SetDateTime(GSM_DateTime *date_time);
GSM_Error	MB21_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB21_SetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB21_DialVoice(char *Number);
GSM_Error	MB21_DialData(char *Number, char type,void (* callpassup)(char c));
GSM_Error	MB21_GetIncomingCallNr(char *Number);
GSM_Error	MB21_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo);
GSM_Error	MB21_GetCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB21_WriteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB21_DeleteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB21_Netmonitor(unsigned char mode, char *Screen);
GSM_Error	MB21_SendDTMF(char *String);
GSM_Error	MB21_GetBitmap(GSM_Bitmap *Bitmap);
GSM_Error	MB21_SetBitmap(GSM_Bitmap *Bitmap);
GSM_Error       MB21_SetRingTone(GSM_Ringtone *ringtone);
GSM_Error       MB21_SendRingTone(GSM_Ringtone *ringtone, char *dest);
GSM_Error	MB21_Reset(unsigned char type);
GSM_Error	MB21_GetProfile(GSM_Profile *Profile);
GSM_Error	MB21_SetProfile(GSM_Profile *Profile);
bool  		MB21_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);
GSM_Error	MB21_CancelCall(void);

GSM_Error 	MB21_EnableDisplayOutput();
GSM_Error 	MB21_DisableDisplayOutput();

GSM_Error 	MB21_EnableCellBroadcast ();
GSM_Error 	MB21_DisableCellBroadcast(void);
GSM_Error 	MB21_ReadCellBroadcast (GSM_CBMessage *Message);
	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __mbus_2110_c isn't 
	   defined. */
#ifdef	__mbus_2110_c


	/* Prototypes for internal functions. */
void	MB21_ThreadLoop(void);


#endif	/* __mbus_2110_c */

#endif	/* __mbus_2110_h */
