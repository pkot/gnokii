

/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, Pavel Janík ml and others 1999
	   Released under the terms of the GNU GPL, see file COPYING
	   for more details.
	
	   This file:  mbus-2110.c  Version 0.3.?
	   
	   ... a starting point for whoever wants to take a crack
	   at 2110 support!
	
	   Header file for the various functions, definitions etc. used
	   to implement the handset interface.  See mbus-2110.c for more details. */

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
                            bool enable_monitoring,
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

GSM_Error	MB21_DeleteSMSMessage(GSM_SMSMessage *message);

GSM_Error	MB21_SendSMSMessage(GSM_SMSMessage *SMS);

GSM_Error	MB21_GetRFLevel(GSM_RFUnits *units, float *level);

GSM_Error	MB21_GetBatteryLevel(GSM_BatteryUnits *units, float *level);

	/* These aren't presently implemented. */
GSM_Error	MB21_GetPowerSource(GSM_PowerSource *source);
GSM_Error	MB21_GetDisplayStatus(int *Status);
GSM_Error	MB21_EnterPin(char *pin);
GSM_Error	MB21_GetIMEI(char *imei);
GSM_Error	MB21_GetRevision(char *revision);
GSM_Error	MB21_GetModel(char *model);
GSM_Error	MB21_GetDateTime(GSM_DateTime *date_time);
GSM_Error	MB21_SetDateTime(GSM_DateTime *date_time);
GSM_Error	MB21_GetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB21_SetAlarm(int alarm_number, GSM_DateTime *date_time);
GSM_Error	MB21_DialVoice(char *Number);
GSM_Error	MB21_DialData(char *Number);
GSM_Error	MB21_GetIncomingCallNr(char *Number);
GSM_Error	MB21_SendBitmap(char *NetworkCode, int width, int height, unsigned char *bitmap);
GSM_Error	MB21_GetNetworkInfo(GSM_NetworkInfo *NetworkInfo);
GSM_Error	MB21_GetCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB21_WriteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB21_DeleteCalendarNote(GSM_CalendarNote *CalendarNote);
GSM_Error	MB21_Netmonitor(unsigned char mode, char *Screen);

	/* All defines and prototypes from here down are specific to 
	   this model and so are #ifdef out if __mbus_2110_c isn't 
	   defined. */
#ifdef	__mbus_2110_c


	/* Prototypes for internal functions. */
void	MB21_ThreadLoop(void);


#endif	/* __mbus_2110_c */

#endif	/* __mbus_2110_h */
