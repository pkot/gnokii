/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  This code contains the main part of the 5160/6160 code.
	

*/

#ifndef WIN32

#define		__mbus_6160_c	/* "Turn on" prototypes in mbus-6160.h */

#include	<termios.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/ioctl.h>
#include	<string.h>
#include	<pthread.h>
#include	<errno.h>

#include	"misc.h"
#include	"gsm-common.h"
#include	"mbus-6160.h"

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					MB61_LinkOK;

GSM_Functions			MB61_Functions = {
		MB61_Initialise,
		MB61_Terminate,
		MB61_GetMemoryLocation,
		MB61_WritePhonebookLocation,
		MB61_GetSpeedDial,
		MB61_SetSpeedDial,
		MB61_GetMemoryStatus,
		MB61_GetSMSStatus,
		MB61_GetSMSCenter,
		MB61_SetSMSCenter,
  		MB61_GetSMSMessage,
		MB61_DeleteSMSMessage,
		MB61_SendSMSMessage,
		MB61_GetRFLevel,
		MB61_GetBatteryLevel,
		MB61_GetPowerSource,
		MB61_GetDisplayStatus,
		MB61_EnterSecurityCode,
		MB61_GetSecurityCodeStatus,
		MB61_GetIMEI,
		MB61_GetRevision,
		MB61_GetModel,
		MB61_GetDateTime,
		MB61_SetDateTime,
		MB61_GetAlarm,
		MB61_SetAlarm,
		MB61_DialVoice,
		MB61_DialData,
		MB61_GetIncomingCallNr,
		MB61_GetNetworkInfo,
		MB61_GetCalendarNote,
		MB61_WriteCalendarNote,
		MB61_DeleteCalendarNote,
		MB61_Netmonitor,
		MB61_SendDTMF,
		MB61_GetBitmap,
		MB61_SetBitmap,
		MB61_Reset,
		MB61_GetProfile,
		MB61_SetProfile,
		MB61_SendRLPFrame
};

	/* FIXME - these are guesses only... */
GSM_Information			MB61_Information = {
		"5160|6160",			/* Models */
		4, 						/* Max RF Level */
		0,						/* Min RF Level */
		GRF_Arbitrary,			/* RF level units */
		4, 						/* Max Battery Level */
		0,						/* Min Battery Level */
		GBU_Arbitrary,			/* Battery level units */
		GDT_None,				/* No date/time support */
		GDT_None,				/* No alarm support */
		0						/* Max alarms = 0 */
};

	/* Local variables */
pthread_t				Thread;
bool					RequestTerminate;
bool					MB61_LinkOK;


	/* The following functions are made visible to gsm-api.c and friends. */

	/* Initialise variables and state machine. */
GSM_Error   MB61_Initialise(char *port_device, char *initlength,
                            GSM_ConnectionType connection,
                            void (*rlp_callback)(RLP_F96Frame *frame))
{

	int		rtn;

	RequestTerminate = false;
	MB61_LinkOK = false;

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) MB61_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (GE_INTERNALERROR);
    }

	return (GE_NONE);

}

	/* Applications should call MB61_Terminate to shut down
	   the MB61 thread and close the serial port. */
void		MB61_Terminate(void)
{
		/* Request termination of thread */
	RequestTerminate = true;

		/* Now wait for thread to terminate. */
	pthread_join(Thread, NULL);

		/* Close serial port. */
	
}

	/* Routine to get specifed phone book location.  Designed to 
	   be called by application.  Will block until location is
	   retrieved or a timeout/error occurs. */
GSM_Error	MB61_GetMemoryLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	MB61_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DeleteSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SendSMSMessage(GSM_SMSMessage *SMS, int data_size)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB61_GetRFLevel
	   FIXME (sort of...)
	   For now, GetRFLevel and GetBatteryLevel both rely
	   on data returned by the "keepalive" packets.  I suspect
	   that we don't actually need the keepalive at all but
	   will await the official doco before taking it out.  HAB19990511 */
GSM_Error	MB61_GetRFLevel(GSM_RFUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB61_GetBatteryLevel
	   FIXME (see above...) */
GSM_Error	MB61_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetIMEI(char *imei)
{
	return (GE_NOTIMPLEMENTED);

}

GSM_Error	MB61_GetRevision(char *revision)
{
	return (GE_NOTIMPLEMENTED);

}

GSM_Error	MB61_GetModel(char *model)
{
	return (GE_NOTIMPLEMENTED);

}

/* This function sends to the mobile phone a request for the SMS Center */

GSM_Error	MB61_GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Our "Not implemented" functions */
GSM_Error	MB61_GetMemoryStatus(GSM_MemoryStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSMSStatus(GSM_SMSStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetPowerSource(GSM_PowerSource *source)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetDisplayStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_EnterSecurityCode(GSM_SecurityCode SecurityCode)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetSecurityCodeStatus(int *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DialVoice(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DialData(char *Number, char type)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetIncomingCallNr(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetNetworkInfo (GSM_NetworkInfo *NetworkInfo)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_WriteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_DeleteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_Netmonitor(unsigned char mode, char *Screen)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SendDTMF(char *String)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetBitmap(GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetBitmap(GSM_Bitmap *Bitmap)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_Reset(unsigned char type)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_GetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB61_SetProfile(GSM_Profile *Profile)
{
    return (GE_NOTIMPLEMENTED);
}

bool		MB61_SendRLPFrame(RLP_F96Frame *frame, bool out_dtx)
{
    return (false);
}

	/* Everything from here down is internal to 6160 code. */


	/* This is the main loop for the MB61 functions.  When MB61_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB61_Terminate function. */
void	MB61_ThreadLoop(void)
{

		/* Do initialisation stuff */

	while (!RequestTerminate) {

		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}

}

#endif
