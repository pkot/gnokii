/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia mobile phones.
	   Copyright (C) Hugh Blemings, Pavel Janík ml and others 1999
	   Released under the terms of the GNU GPL, see file COPYING
	   for more details.
	
	   This file:  mbus-2110.c  Version 0.3.?
	   
	   ... a starting point for whoever wants to take a crack
	   at 2110 support!  Please follow the conventions used
	   in fbus-3810.[ch] and fbus-6110.[ch] wherever possible.
	   Thy tabs &| indents shall be 4 characters wide.
	
	   These functions are only ever called through the GSM_Functions
	   structure defined in gsm-common.h and set up in gsm-api.c */

#define		__mbus_2110_c	/* "Turn on" prototypes in mbus-2110.h */

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
#include	"mbus-2110.h"

	/* Global variables used by code in gsm-api.c to expose the
	   functions supported by this model of phone.  */
bool					MB21_LinkOK;

GSM_Functions			MB21_Functions = {
		MB21_Initialise,
		MB21_Terminate,
		MB21_GetMemoryLocation,
		MB21_WritePhonebookLocation,
		MB21_GetSpeedDial,
		MB21_SetSpeedDial,
		MB21_GetMemoryStatus,
		MB21_GetSMSStatus,
		MB21_GetSMSCenter,
  		MB21_GetSMSMessage,
		MB21_DeleteSMSMessage,
		MB21_SendSMSMessage,
		MB21_GetRFLevel,
		MB21_GetBatteryLevel,
		MB21_GetPowerSource,
		MB21_EnterPin,
		MB21_GetIMEI,
		MB21_GetRevision,
		MB21_GetModel,
		MB21_GetDateTime,
		MB21_SetDateTime,
		MB21_GetAlarm,
		MB21_SetAlarm,
		MB21_DialVoice,
		MB21_DialData,
		MB21_GetIncomingCallNr,
		MB21_SendBitmap,
		MB21_GetNetworkInfo,
		MB21_GetCalendarNote,
		MB21_WriteCalendarNote,
		MB21_DeleteCalendarNote
};

	/* FIXME - these are guesses only... */
GSM_Information			MB21_Information = {
		"2110",					/* Models */
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
bool					MB21_LinkOK;


	/* The following functions are made visible to gsm-api.c and friends. */

	/* Initialise variables and state machine. */
GSM_Error	MB21_Initialise(char *port_device, char *initlength, GSM_ConnectionType connection, bool enable_monitoring)
{

	/* ConnectionType is ignored in 3810 code. */

	int		rtn;

	RequestTerminate = false;
	MB21_LinkOK = false;

		/* Create and start thread, */
	rtn = pthread_create(&Thread, NULL, (void *) MB21_ThreadLoop, (void *)NULL);

    if (rtn == EAGAIN || rtn == EINVAL) {
        return (GE_INTERNALERROR);
    }

	return (GE_NONE);

}

	/* Applications should call MB21_Terminate to shut down
	   the MB21 thread and close the serial port. */
void		MB21_Terminate(void)
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
GSM_Error	MB21_GetMemoryLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Routine to write phonebook location in phone. Designed to 
	   be called by application code.  Will block until location
	   is written or timeout occurs.  */
GSM_Error	MB21_WritePhonebookLocation(GSM_PhonebookEntry *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_SetSpeedDial(GSM_SpeedDial *entry)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_DeleteSMSMessage(GSM_SMSMessage *message)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_SendSMSMessage(GSM_SMSMessage *SMS)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB21_GetRFLevel
	   FIXME (sort of...)
	   For now, GetRFLevel and GetBatteryLevel both rely
	   on data returned by the "keepalive" packets.  I suspect
	   that we don't actually need the keepalive at all but
	   will await the official doco before taking it out.  HAB19990511 */
GSM_Error	MB21_GetRFLevel(GSM_RFUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

	/* MB21_GetBatteryLevel
	   FIXME (see above...) */
GSM_Error	MB21_GetBatteryLevel(GSM_BatteryUnits *units, float *level)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetIMEI(char *imei)
{
	return (GE_NOTIMPLEMENTED);

}

GSM_Error	MB21_GetRevision(char *revision)
{
	return (GE_NOTIMPLEMENTED);

}

GSM_Error	MB21_GetModel(char *model)
{
	return (GE_NOTIMPLEMENTED);

}

/* This function sends to the mobile phone a request for the SMS Center */

GSM_Error	MB21_GetSMSCenter(GSM_MessageCenter *MessageCenter)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Our "Not implemented" functions */
GSM_Error	MB21_GetMemoryStatus(GSM_MemoryStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetSMSStatus(GSM_SMSStatus *Status)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetPowerSource(GSM_PowerSource *source)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_EnterPin(char *pin)
{
	return (GE_NOTIMPLEMENTED);
}


GSM_Error	MB21_GetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_SetDateTime(GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_SetAlarm(int alarm_number, GSM_DateTime *date_time)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_DialVoice(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_DialData(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetIncomingCallNr(char *Number)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_SendBitmap (char *NetworkCode, int width, int height, unsigned char *bitmap)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetNetworkInfo (GSM_NetworkInfo *NetworkInfo)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_GetCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_WriteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

GSM_Error	MB21_DeleteCalendarNote (GSM_CalendarNote *CalendarNote)
{
	return (GE_NOTIMPLEMENTED);
}

	/* Everything from here down is internal to 2110 code. */


	/* This is the main loop for the MB21 functions.  When MB21_Initialise
	   is called a thread is created to run this loop.  This loop is
	   exited when the application calls the MB21_Terminate function. */
void	MB21_ThreadLoop(void)
{

		/* Do initialisation stuff */

	while (!RequestTerminate) {

		usleep(100000);		/* Avoid becoming a "busy" loop. */
	}

}

