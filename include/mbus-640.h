/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Header file for 640 code.
	
  Last modification: Mon Mar 20 21:40:04 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#ifndef		__mbus_640_h
#define		__mbus_640_h

#ifndef		__gsm_common_h
#include	"gsm-common.h"	/* Needed for GSM_Error etc. */
#endif

	/* Global variables */
extern bool					    MB640_LinkOK;
extern GSM_Functions		MB640_Functions;
extern GSM_Information	MB640_Information;


	/* Prototypes for the functions designed to be used externally. */
GSM_Error   MB640_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame));

void		MB640_Terminate(void);

int		MB640_GetMemoryType(GSM_MemoryType memory_type);

GSM_Error	MB640_GetMemoryLocation(GSM_PhonebookEntry *entry);

GSM_Error	MB640_WritePhonebookLocation(GSM_PhonebookEntry *entry);

GSM_Error	MB640_GetSpeedDial(GSM_SpeedDial *entry);

GSM_Error	MB640_SetSpeedDial(GSM_SpeedDial *entry);

GSM_Error	MB640_GetMemoryStatus(GSM_MemoryStatus *Status);

GSM_Error	MB640_GetSMSStatus(GSM_SMSStatus *Status);
GSM_Error       MB640_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error	MB640_GetSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB640_GetSMSCenter(GSM_MessageCenter *MessageCenter);
GSM_Error	MB640_SetSMSCenter(GSM_MessageCenter *MessageCenter);

GSM_Error	MB640_DeleteSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB640_SendSMSMessage(GSM_SMSMessage *SMS, int data_size);

GSM_Error	MB640_GetRFLevel(GSM_RFUnits *units, float *level);

GSM_Error	MB640_GetBatteryLevel(GSM_BatteryUnits *units, float *level);

	/* These aren't presently implemented. */
GSM_Error	MB640_GetPowerSource(GSM_PowerSource *source);
GSM_Error	MB640_GetDisplayStatus(int *Status);

GSM_Error	MB640_EnterSecurityCode(GSM_SecurityCode SecurityCode);
GSM_Error	MB640_GetSecurityCodeStatus(int *Status);

GSM_Error	MB640_GetIMEI(char *imei);
GSM_Error	MB640_GetRevision(char *revision);
GSM_Error	MB640_GetModel(char *model);
GSM_Error	MB640_GetDateTime(GSM_DateTime *date_time);
GSM_Error	MB640_SetDateTime(GSM_DateTime *date_time);
GSM_Error	MB640_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB640_SetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB640_DialVoice(char *Number);
GSM_Error	MB640_DialData(char *Number, char type, void (* callpassup)(char c));
GSM_Error	MB640_GetIncomingCallNr(char *Number);
GSM_Error	MB640_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo);
GSM_Error	MB640_GetCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB640_WriteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB640_DeleteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB640_Netmonitor(unsigned char mode, char *Screen);
GSM_Error	MB640_SendDTMF(char *String);
GSM_Error	MB640_GetBitmap(GSM_Bitmap *Bitmap);
GSM_Error	MB640_SetBitmap(GSM_Bitmap *Bitmap);
GSM_Error	MB640_Reset(unsigned char type);
GSM_Error	MB640_GetProfile(GSM_Profile *Profile);
GSM_Error	MB640_SetProfile(GSM_Profile *Profile);
bool  		MB640_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx);

	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __mbus_640_c isn't 
	   defined. */
#ifdef	__mbus_640_c

#define     MB640_MAX_MODEL_LENGTH           (8)

	/* Prototypes for internal functions. */
void	    MB640_ThreadLoop(void);
void      MB640_SigHandler(int status);
bool      MB640_OpenSerial(void);
GSM_Error MB640_SendPacket( u8 *buffer, u8 length );


#endif	/* __mbus_640_c */

#endif	/* __mbus_640_h */
