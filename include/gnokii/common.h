	/* G N O K I I
	   A Linux/Unix toolset and driver for Nokia GSM  mobiles.
	   Copyright (C) Hugh Blemings, 1999  Released under the terms of 
       the GNU GPL, see file COPYING for more details.
	
	   This file:  gsm-common.h   Version 0.2.3

	   Header file for the definitions, enums etc. that are used by
	   all models of handset. */

#ifndef	__gsm_common_h
#define	__gsm_common_h


	/* Maximum length of device name for serial port */
#define		GSM_MAX_DEVICE_NAME_LENGTH					(100)

	/* Define an enum for specifying memory types for retrieving 
	   phonebook entries, SMS messages etc. */
typedef enum 	{GMT_INTERNAL, GMT_SIM } GSM_MemoryType;

	/* Limits to do with SMS messages. */
#define		GSM_MAX_MSG_CENTRE_LENGTH					(40)
#define		GSM_MAX_SENDER_LENGTH						(40)
#define		GSM_MAX_SMS_LENGTH							(160)

	/* Define datatype for SMS messages, used for getting SMS
	   messages from the phones memory. */
typedef struct {
	int					Year, Month, Day;	/* Time/date of reception of messages. */
	int					Hour, Minute, Second;
	int					Length;
	char				MessageText[GSM_MAX_SMS_LENGTH + 1]; 	/* Room for null term. */
	char				MessageCentre[GSM_MAX_MSG_CENTRE_LENGTH + 1];
	char				Sender[GSM_MAX_SENDER_LENGTH + 1];
	int					MessageNumber;	/* Location in phones memory. */
	GSM_MemoryType		MemoryType;		/* Which memory type message is stored in. */

	u8					Unk1, Unk2, Unk3, Unk4, Unk5,
						Unk6, Unk7, Unk8, Unk9, UnkEnd;	/* Unknown bytes. */
} GSM_SMSMessage;


	/* Limits for sizing of array in GSM_PhonebookEntry.  Individual
	   handsets may not support these lengths so they have their
 	   own limits set. */
#define		GSM_MAX_PHONEBOOK_NAME_LENGTH				(40)
#define		GSM_MAX_PHONEBOOK_NUMBER_LENGTH				(40)

	/* Define datatype for phonebook entry, used for getting/writing
	   phonebook entries. */
typedef struct {
	bool	Empty;
	char	Name[GSM_MAX_PHONEBOOK_NAME_LENGTH + 1]; /* Plus 1 for null terminator. */
	char	Number[GSM_MAX_PHONEBOOK_NUMBER_LENGTH + 1];
} GSM_PhonebookEntry;

	/* Define enums for RF units and Battery units */
typedef enum	{GRF_Arbitrary, GRF_dBm, GRF_mV, GRF_uV} GSM_RFUnits;
typedef enum	{GBU_Arbitrary, GBU_Volts, GBU_Minutes} GSM_BatteryUnits;

	/* This structure is provided to allow common information about
	   the particular model to be looked up in a model independant
	   way.  Some of the values here define minimum and maximum
	   levels for values retrieved by the various Get functions 
	   for example battery level.  They are not defined as consts
	   to allow model specific code to set them during initialisation */

typedef struct {
	 	/* Models covered by this type, pipe '|' delimited */
	char			*Models;

		/* Minimum and maximum levels for RF signal strength.  Units
		   are as per the setting of RFLevelUnits. */
	float				MaxRFLevel;
	float				MinRFLevel;
	GSM_RFUnits			RFLevelUnits;

		/* Minimum anx maximum levels for battery level.  Again, units
		   are as per GSM_BatteryLevelUnits */
	float				MaxBatteryLevel;
	float				MinBatteryLevel;
	GSM_BatteryUnits	BatteryLevelUnits;
} GSM_Information;


	/* Define standard GSM error/return code values.  These codes
	   are also used for some internal functions such as SIM read/write
	   in the model specific code.  */
typedef enum {
	GE_NONE = 0,			/* No error */
	GE_DEVICEOPENFAILED,	/* Couldn't open specified /dev */
	GE_UNKNOWNMODEL,		/* Model specified isn't known/supported. */
  	GE_NOLINK,				/* Couldn't establish link with phone */
  	GE_TIMEOUT,				/* Command timed out. */
	GE_NOTIMPLEMENTED,		/* Command called isn't implemented in model. */
  	GE_INVALIDSMSLOCATION,
  	GE_INVALIDPHBOOKLOCATION,
	GE_INVALIDMEMORYTYPE,
  	GE_EMPTYSMSLOCATION,
  	GE_PHBOOKNAMETOOLONG,
  	GE_PHBOOKNUMBERTOOLONG,
  	GE_PHBOOKWRITEFAILED,
  	GE_SMSSENDOK,
  	GE_SMSSENDFAILED,
  	GE_SMSTOOLONG,
  	GE_INTERNALERROR,		/* Problem occured internal to model specific code. */
  	GE_BUSY,				/* Command is still being executed. */
		/* The following are here in anticpation of data call requirements. */
	GE_LINEBUSY,			/* Outgoing call requested reported line busy */
	GE_NOCARRIER,			/* No Carrier error during data call setup ? */

} GSM_Error;


	/* Define the structure used to hold pointers to the various 
	   API functions.  This is in effect the master list of functions
	   provided by the gnokii API.  Modules containing the model specific
	   code each contain one of these structures which is "filled in"
	   with the corresponding function within the model specific
	   code.  If a function is not supported or not implemented , a
 	   generic not implemented function is used to return a
	   GE_NOTIMPLEMENTED error code. */
typedef struct {
		GSM_Error	(*Initialise)(char *port_device, bool enable_monitoring);

		void		(*Terminate)(void);	

		GSM_Error	(*GetPhonebookLocation)(GSM_MemoryType memory_type,
						 int location, GSM_PhonebookEntry *entry);

		GSM_Error	(*WritePhonebookLocation)(GSM_MemoryType memory_type,
						 int location, GSM_PhonebookEntry *entry);

		GSM_Error	(*GetSMSMessage)(GSM_MemoryType memory_type, int location,
						 GSM_SMSMessage *message);

		GSM_Error	(*SendSMSMessage)(char *message_centre, char *destination,
						 char *text, u8 *return_code1, u8 *return_code2);

		GSM_Error	(*GetRFLevel)(float *level);

		GSM_Error	(*GetBatteryLevel)(float *level);

		GSM_Error	(*EnterPin)(char *pin);

		GSM_Error	(*GetIMEIAndCode)(char *imei, char *code);
} GSM_Functions;


#endif	/* __gsm_common_h */
