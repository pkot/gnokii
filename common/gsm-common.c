/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2001-2004 Pawel Kot
  Copyright (C) 2001      Chris Kemp, Pavel Machek
  Copyright (C) 2002-2004 BORBELY Zoltan

*/

#include "config.h"

#include <string.h>
#include "compat.h"
#include "gnokii.h"

gn_error unimplemented(void)
{
	return GN_ERR_NOTIMPLEMENTED;
}

GNOKII_API gn_memory_type gn_str2memory_type(const char *s)
{
#define X(a) if (!strcmp(s, #a)) return GN_MT_##a;
	X(ME);
	X(SM);
	X(FD);
	X(ON);
	X(EN);
	X(DC);
	X(RC);
	X(MC);
	X(LD);
	X(BD);
	X(SD);
	X(MT);
	X(TA);
	X(CB);
	X(IN);
	X(OU);
	X(AR);
	X(TE);
	X(SR);
	X(DR);
	X(OUS);
	X(F1);
	X(F2);
	X(F3);
	X(F4);
	X(F5);
	X(F6);
	X(F7);
	X(F8);
	X(F9);
	X(F10);
	X(F11);
	X(F12);
	X(F13);
	X(F14);
	X(F15);
	X(F16);
	X(F17);
	X(F18);
	X(F19);
	X(F20);
	X(BM);
	return GN_MT_XX;
#undef X
}

/* Set of handy functions to print out localized name of the enum value */

GNOKII_API const char *gn_memory_type2str(gn_memory_type mt)
{
#define X(a) case GN_MT_##a: return #a;
	switch (mt) {
	X(ME);
	X(SM);
	X(FD);
	X(ON);
	X(EN);
	X(DC);
	X(RC);
	X(MC);
	X(LD);
	X(BD);
	X(SD);
	X(MT);
	X(TA);
	X(CB);
	X(IN);
	X(OU);
	X(AR);
	X(TE);
	X(SR);
	X(DR);
	X(OUS);
	X(F1);
	X(F2);
	X(F3);
	X(F4);
	X(F5);
	X(F6);
	X(F7);
	X(F8);
	X(F9);
	X(F10);
	X(F11);
	X(F12);
	X(F13);
	X(F14);
	X(F15);
	X(F16);
	X(F17);
	X(F18);
	X(F19);
	X(F20);
	X(BM);
	default: return NULL;
	}
#undef X
}

GNOKII_API const char *gn_memory_type_print(gn_memory_type mt)
{
	switch (mt) {
	case GN_MT_ME: return _("Internal memory");
	case GN_MT_SM: return _("SIM card");
	case GN_MT_FD: return _("Fixed dial numbers");
	case GN_MT_ON: return _("Own numbers");
	case GN_MT_EN: return _("Emergency numbers");
	case GN_MT_DC: return _("Dialed numbers");
	case GN_MT_RC: return _("Received numbers");
	case GN_MT_MC: return _("Missed numbers");
	case GN_MT_LD: return _("Last dialed");
	case GN_MT_BD: return _("Barred dialing numbers");
	case GN_MT_SD: return _("Service dialing numbers");
	case GN_MT_MT: return _("Combined ME and SIM phonebook");
	case GN_MT_TA: return _("Computer memory");
	case GN_MT_CB: return _("Currently selected memory");
	case GN_MT_IN: return _("SMS Inbox");
	case GN_MT_OU: return _("SMS Outbox, sent items");
	case GN_MT_AR: return _("SMS Archive");
	case GN_MT_TE: return _("SMS Templates");
	case GN_MT_SR: return _("SMS Status Reports");
	case GN_MT_DR: return _("SMS Drafts");
	case GN_MT_OUS: return _("SMS Outbox, items to be sent");
	case GN_MT_F1: return _("SMS Folder 1");
	case GN_MT_F2: return _("SMS Folder 2");
	case GN_MT_F3: return _("SMS Folder 3");
	case GN_MT_F4: return _("SMS Folder 4");
	case GN_MT_F5: return _("SMS Folder 5");
	case GN_MT_F6: return _("SMS Folder 6");
	case GN_MT_F7: return _("SMS Folder 7");
	case GN_MT_F8: return _("SMS Folder 8");
	case GN_MT_F9: return _("SMS Folder 9");
	case GN_MT_F10: return _("SMS Folder 10");
	case GN_MT_F11: return _("SMS Folder 11");
	case GN_MT_F12: return _("SMS Folder 12");
	case GN_MT_F13: return _("SMS Folder 13");
	case GN_MT_F14: return _("SMS Folder 14");
	case GN_MT_F15: return _("SMS Folder 15");
	case GN_MT_F16: return _("SMS Folder 16");
	case GN_MT_F17: return _("SMS Folder 17");
	case GN_MT_F18: return _("SMS Folder 18");
	case GN_MT_F19: return _("SMS Folder 19");
	case GN_MT_F20: return _("SMS Folder 20");
	case GN_MT_BM: return _("Cell Broadcast");
	default: return _("Unknown");
	}
}

GNOKII_API const char *gn_connection_type2str(gn_connection_type t)
{
	switch (t) {
	case GN_CT_NONE:	return _("None");
	case GN_CT_Serial:	return _("Serial");
	case GN_CT_DAU9P:	return _("Serial DAU9P");
	case GN_CT_DLR3P:	return _("Serial DLR3P");
	case GN_CT_Infrared:	return _("Infrared (old Nokias)");
	case GN_CT_Irda:	return _("IrDA");
	case GN_CT_Bluetooth:	return _("Bluetooth");
	case GN_CT_Tekram:	return _("Tekram Ir-Dongle");
	case GN_CT_TCP:		return _("TCP");
	case GN_CT_M2BUS:	return _("M2BUS");
	case GN_CT_DKU2:	return _("DKU2 (kernel support)");
	case GN_CT_DKU2LIBUSB:	return _("DKU2 (libusb support)");
	case GN_CT_PCSC:	return _("Smart Card reader (libpcsc support)");
	case GN_CT_SOCKETPHONET:	return _("Linux Phonet (kernel support)");
	default:		return _("Unknown");
	}
}

GNOKII_API const char *gn_power_source2str(gn_power_source s)
{
	switch (s) {
	case GN_PS_ACDC:	return _("AC-DC");
	case GN_PS_BATTERY:	return _("Battery");
	case GN_PS_NOBATTERY:	return _("No battery");
	case GN_PS_FAULT:	return _("Power fault");
	default:		return _("Unknown");
	}
}

GNOKII_API const char *gn_security_code_type2str(gn_security_code_type t)
{
	switch (t) {
	case GN_SCT_SecurityCode:	return _("Security code");
	case GN_SCT_Pin:		return _("PIN");
	case GN_SCT_Pin2:		return _("PIN2");
	case GN_SCT_Puk:		return _("PUK");
	case GN_SCT_Puk2:		return _("PUK2");
	case GN_SCT_None:		return _("None");
	default:			return _("Unknown");
	}
}

/* Phonebook */

GNOKII_API const char *gn_phonebook_number_type2str(gn_phonebook_number_type t)
{
	switch (t) {
	case GN_PHONEBOOK_NUMBER_None:		return _("General number");
	case GN_PHONEBOOK_NUMBER_Common:	return _("General number");
	case GN_PHONEBOOK_NUMBER_Home:		return _("Home number");
	case GN_PHONEBOOK_NUMBER_Mobile:	return _("Mobile number");
	case GN_PHONEBOOK_NUMBER_Fax:		return _("Fax number");
	case GN_PHONEBOOK_NUMBER_Work:		return _("Work number");
	case GN_PHONEBOOK_NUMBER_General:	return _("General number");
	default:				return _("Unknown number");
	}
}

GNOKII_API const char *gn_phonebook_entry_type2str(gn_phonebook_entry_type t)
{
	switch (t) {
	case GN_PHONEBOOK_ENTRY_Name:			return _("Name");
	case GN_PHONEBOOK_ENTRY_Email:			return _("Email");
	case GN_PHONEBOOK_ENTRY_Postal:			return _("Postal address");
	case GN_PHONEBOOK_ENTRY_Note:			return _("Note");
	case GN_PHONEBOOK_ENTRY_Number:			return _("Number");
	case GN_PHONEBOOK_ENTRY_Ringtone:		return _("Ringtone");
	case GN_PHONEBOOK_ENTRY_Date:			return _("Date");
	case GN_PHONEBOOK_ENTRY_Pointer:		return _("Pointer");
	case GN_PHONEBOOK_ENTRY_Logo:			return _("Logo");
	case GN_PHONEBOOK_ENTRY_LogoSwitch:		return _("Logo switch");
	case GN_PHONEBOOK_ENTRY_Group:			return _("Group");
	case GN_PHONEBOOK_ENTRY_URL:			return _("URL");
	case GN_PHONEBOOK_ENTRY_Location:		return _("Location");
	case GN_PHONEBOOK_ENTRY_Image:			return _("Image");
	case GN_PHONEBOOK_ENTRY_RingtoneAdv:		return _("Ringtone");
	case GN_PHONEBOOK_ENTRY_UserID:			return _("User ID");
	case GN_PHONEBOOK_ENTRY_PTTAddress:		return _("Push-To-Talk address");
	case GN_PHONEBOOK_ENTRY_FirstName:		return _("First name");
	case GN_PHONEBOOK_ENTRY_LastName:		return _("Last name");
	case GN_PHONEBOOK_ENTRY_PostalAddress:		return _("Postal address");
	case GN_PHONEBOOK_ENTRY_ExtendedAddress:	return _("Extended address");
	case GN_PHONEBOOK_ENTRY_Street:			return _("Street");
	case GN_PHONEBOOK_ENTRY_City:			return _("City");
	case GN_PHONEBOOK_ENTRY_StateProvince:		return _("State or province");
	case GN_PHONEBOOK_ENTRY_ZipCode:		return _("Zip code");
	case GN_PHONEBOOK_ENTRY_Country:		return _("Country");
	case GN_PHONEBOOK_ENTRY_FormalName:		return _("Formal name");
	case GN_PHONEBOOK_ENTRY_JobTitle:		return _("Job title");
	case GN_PHONEBOOK_ENTRY_Company:		return _("Company");
	case GN_PHONEBOOK_ENTRY_Nickname:		return _("Nickname");
	case GN_PHONEBOOK_ENTRY_Birthday:		return _("Birthday");
	case GN_PHONEBOOK_ENTRY_ExtGroup:		return _("Caller group id");
	default:					return _("Unknown");
	}
}

GNOKII_API const char *gn_subentrytype2string(gn_phonebook_entry_type entry_type, gn_phonebook_number_type number_type)
{
	switch (entry_type) {
	case GN_PHONEBOOK_ENTRY_Name:
	case GN_PHONEBOOK_ENTRY_Email:
	case GN_PHONEBOOK_ENTRY_Postal:
	case GN_PHONEBOOK_ENTRY_Note:
	case GN_PHONEBOOK_ENTRY_Ringtone:
	case GN_PHONEBOOK_ENTRY_Date:
	case GN_PHONEBOOK_ENTRY_Pointer:
	case GN_PHONEBOOK_ENTRY_Logo:
	case GN_PHONEBOOK_ENTRY_LogoSwitch:
	case GN_PHONEBOOK_ENTRY_Group:
	case GN_PHONEBOOK_ENTRY_URL:
	case GN_PHONEBOOK_ENTRY_Location:
	case GN_PHONEBOOK_ENTRY_Image:
	case GN_PHONEBOOK_ENTRY_RingtoneAdv:
	case GN_PHONEBOOK_ENTRY_UserID:
	case GN_PHONEBOOK_ENTRY_PTTAddress:
	case GN_PHONEBOOK_ENTRY_FirstName:
	case GN_PHONEBOOK_ENTRY_LastName:
	case GN_PHONEBOOK_ENTRY_PostalAddress:
	case GN_PHONEBOOK_ENTRY_ExtendedAddress:
	case GN_PHONEBOOK_ENTRY_Street:
	case GN_PHONEBOOK_ENTRY_City:
	case GN_PHONEBOOK_ENTRY_StateProvince:
	case GN_PHONEBOOK_ENTRY_ZipCode:
	case GN_PHONEBOOK_ENTRY_Country:
	case GN_PHONEBOOK_ENTRY_FormalName:
	case GN_PHONEBOOK_ENTRY_JobTitle:
	case GN_PHONEBOOK_ENTRY_Company:
	case GN_PHONEBOOK_ENTRY_Nickname:
	case GN_PHONEBOOK_ENTRY_Birthday:
	case GN_PHONEBOOK_ENTRY_ExtGroup:
		return gn_phonebook_entry_type2str(entry_type);
	case GN_PHONEBOOK_ENTRY_Number:
		switch (number_type) {
		case GN_PHONEBOOK_NUMBER_Home:
		case GN_PHONEBOOK_NUMBER_Mobile:
		case GN_PHONEBOOK_NUMBER_Fax:
		case GN_PHONEBOOK_NUMBER_Work:
		case GN_PHONEBOOK_NUMBER_None:
		case GN_PHONEBOOK_NUMBER_Common:
		case GN_PHONEBOOK_NUMBER_General:
			return gn_phonebook_number_type2str(number_type);
		default:
			return _("Unknown number");
		}
	default:
		return _("Unknown");
	}
}

GNOKII_API const char *gn_phonebook_group_type2str(gn_phonebook_group_type t)
{
	switch (t) {
	case GN_PHONEBOOK_GROUP_Family:	return _("Family");
	case GN_PHONEBOOK_GROUP_Vips:	return _("VIPs");
	case GN_PHONEBOOK_GROUP_Friends:return _("Friends");
	case GN_PHONEBOOK_GROUP_Work:	return _("Work");
	case GN_PHONEBOOK_GROUP_Others:	return _("Others");
	case GN_PHONEBOOK_GROUP_None:	return _("None");
	default:			return _("Unknown");
	}
}

/* SMS */

GNOKII_API const char *gn_sms_udh_type2str(gn_sms_udh_type t)
{
	switch (t) {
	case GN_SMS_UDH_None:			return _("Text");
	case GN_SMS_UDH_ConcatenatedMessages:	return _("Linked");
	case GN_SMS_UDH_Ringtone:		return _("Ringtone");
	case GN_SMS_UDH_OpLogo:			return _("GSM Operator Logo");
	case GN_SMS_UDH_CallerIDLogo:		return _("Logo");
	case GN_SMS_UDH_MultipartMessage:	return _("Multipart Message");
	case GN_SMS_UDH_WAPvCard:		return _("WAP vCard");
	case GN_SMS_UDH_WAPvCalendar:		return _("WAP vCalendar");
	case GN_SMS_UDH_WAPvCardSecure:		return _("WAP vCardSecure");
	case GN_SMS_UDH_WAPvCalendarSecure:	return _("WAP vCalendarSecure");
	case GN_SMS_UDH_VoiceMessage:		return _("Voice Message");
	case GN_SMS_UDH_FaxMessage:		return _("Fax Message");
	case GN_SMS_UDH_EmailMessage:		return _("Email Message");
	case GN_SMS_UDH_WAPPush:		return _("WAP Push");
	case GN_SMS_UDH_OtherMessage:		return _("Other Message");
	case GN_SMS_UDH_Unknown:		return _("Unknown");
	default:				return _("Unknown");
	}
}

GNOKII_API const char *gn_sms_message_type2str(gn_sms_message_type t)
{
	switch (t) {
	case GN_SMS_MT_Deliver:		return _("Inbox Message");
	case GN_SMS_MT_DeliveryReport:	return _("Delivery Report");
	case GN_SMS_MT_Submit:		return _("MO Message");
	case GN_SMS_MT_SubmitReport:	return _("Submit Report");
	case GN_SMS_MT_Command:		return _("Command");
	case GN_SMS_MT_StatusReport:	return _("Status Report");
	case GN_SMS_MT_Picture:		return _("Picture Message");
	case GN_SMS_MT_TextTemplate:	return _("Template");
	case GN_SMS_MT_PictureTemplate:	return _("Picture Message Template");
	case GN_SMS_MT_SubmitSent:	return _("MO Message");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_sms_message_status2str(gn_sms_message_status t)
{
	switch (t) {
	case GN_SMS_Read:	return _("Read");
	case GN_SMS_Unread:	return _("Unread");
	case GN_SMS_Sent:	return _("Sent");
	case GN_SMS_Unsent:	return _("Unsent");
	default:		return _("Unknown");
	}
}

GNOKII_API const char *gn_sms_message_format2str(gn_sms_message_format t)
{
	switch (t) {
	case GN_SMS_MF_Text:	return _("Text");
	case GN_SMS_MF_Fax:	return _("Fax");
	case GN_SMS_MF_Voice:	return _("VoiceMail");
	case GN_SMS_MF_ERMES:	return _("ERMES");
	case GN_SMS_MF_Paging:	return _("Paging");
	case GN_SMS_MF_UCI:	return _("Email message in 8110i");
	case GN_SMS_MF_Email:	return _("Email");
	case GN_SMS_MF_X400:	return _("X.400");
	default:		return _("Unknown");
	}
}

GNOKII_API const char *gn_sms_vp_time2str(gn_sms_vp_time t)
{
	switch (t) {
	case GN_SMS_VP_1H:	return _("1 hour");
	case GN_SMS_VP_6H:	return _("6 hours");
	case GN_SMS_VP_24H:	return _("24 hours");
	case GN_SMS_VP_72H:	return _("72 hours");
	case GN_SMS_VP_1W:	return _("1 week");
	case GN_SMS_VP_Max:	return _("Maximum time");
	default:		return _("Unknown");
	}
}

/* Calendar and ToDo */

GNOKII_API const char *gn_calnote_type2str(gn_calnote_type t)
{
	switch (t) {
	case GN_CALNOTE_MEETING:	return _("Meeting");
	case GN_CALNOTE_CALL:		return _("Call");
	case GN_CALNOTE_BIRTHDAY:	return _("Birthday");
	case GN_CALNOTE_REMINDER:	return _("Reminder");
	case GN_CALNOTE_MEMO:		return _("Memo");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_calnote_recurrence2str(gn_calnote_recurrence r)
{
	switch (r) {
	case GN_CALNOTE_NEVER:	return _("Never");
	case GN_CALNOTE_DAILY:	return _("Daily");
	case GN_CALNOTE_WEEKLY:	return _("Weekly");
	case GN_CALNOTE_2WEEKLY:return _("Every 2 weeks");
	case GN_CALNOTE_MONTHLY:return _("Monthly");
	case GN_CALNOTE_YEARLY:	return _("Yearly");
	default:		return _("Unknown");
	}
}

GNOKII_API const char *gn_todo_priority2str(gn_todo_priority p)
{
	switch (p) {
	case GN_TODO_LOW:	return _("Low");
	case GN_TODO_MEDIUM:	return _("Medium");
	case GN_TODO_HIGH:	return _("High");
	default:		return _("Unknown");
	}
}

/* WAP */

GNOKII_API const char *gn_wap_session2str(gn_wap_session p)
{
	switch (p) {
	case GN_WAP_SESSION_TEMPORARY:	return _("Temporary");
	case GN_WAP_SESSION_PERMANENT:	return _("Permanent");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_wap_authentication2str(gn_wap_authentication p)
{
	switch (p) {
	case GN_WAP_AUTH_NORMAL:	return _("Normal");
	case GN_WAP_AUTH_SECURE:	return _("Secure");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_wap_bearer2str(gn_wap_bearer p)
{
	switch (p) {
	case GN_WAP_BEARER_GSMDATA:	return _("GSM data");
	case GN_WAP_BEARER_GPRS:	return _("GPRS");
	case GN_WAP_BEARER_SMS:		return _("SMS");
	case GN_WAP_BEARER_USSD:	return _("USSD");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_wap_call_type2str(gn_wap_call_type p)
{
	switch (p) {
	case GN_WAP_CALL_ANALOGUE:	return _("Analogue");
	case GN_WAP_CALL_ISDN:		return _("ISDN");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_wap_call_speed2str(gn_wap_call_speed p)
{
	switch (p) {
	case GN_WAP_CALL_AUTOMATIC:	return _("Automatic");
	case GN_WAP_CALL_9600:		return _("9600");
	case GN_WAP_CALL_14400:		return _("14400");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_wap_login2str(gn_wap_login p)
{
	switch (p) {
	case GN_WAP_LOGIN_MANUAL:	return _("Manual");
	case GN_WAP_LOGIN_AUTOLOG:	return _("Automatic");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_wap_gprs2str(gn_wap_gprs p)
{
	switch (p) {
	case GN_WAP_GPRS_ALWAYS:	return _("Always");
	case GN_WAP_GPRS_WHENNEEDED:	return _("When needed");
	default:			return _("Unknown");
	}
}

/* Profiles */

GNOKII_API const char *gn_profile_message_type2str(gn_profile_message_type p)
{
	switch (p) {
	case GN_PROFILE_MESSAGE_NoTone:		return _("No tone");
	case GN_PROFILE_MESSAGE_Standard:	return _("Standard");
	case GN_PROFILE_MESSAGE_Special:	return _("Special");
	case GN_PROFILE_MESSAGE_BeepOnce:	return _("Beep once");
	case GN_PROFILE_MESSAGE_Ascending:	return _("Ascending");
	default:				return _("Unknown");
	}
}

GNOKII_API const char *gn_profile_warning_type2str(gn_profile_warning_type p)
{
	switch (p) {
	case GN_PROFILE_WARNING_Off:		return _("Off");
	case GN_PROFILE_WARNING_On:		return _("On");
	default:				return _("Unknown");
	}
}

GNOKII_API const char *gn_profile_vibration_type2str(gn_profile_vibration_type p)
{
	switch (p) {
	case GN_PROFILE_VIBRATION_Off:		return _("Off");
	case GN_PROFILE_VIBRATION_On:		return _("On");
	default:				return _("Unknown");
	}
}

GNOKII_API const char *gn_profile_callalert_type2str(gn_profile_callalert_type p)
{
	switch (p) {
	case GN_PROFILE_CALLALERT_Ringing:	return _("Ringing");
	case GN_PROFILE_CALLALERT_BeepOnce:	return _("Beep once");
	case GN_PROFILE_CALLALERT_Off:		return _("Off");
	case GN_PROFILE_CALLALERT_RingOnce:	return _("Ring once");
	case GN_PROFILE_CALLALERT_Ascending:	return _("Ascending");
	case GN_PROFILE_CALLALERT_CallerGroups:	return _("Caller groups");
	default:				return _("Unknown");
	}
}

GNOKII_API const char *gn_profile_keyvol_type2str(gn_profile_keyvol_type p)
{
	switch (p) {
	case GN_PROFILE_KEYVOL_Off:	return _("Off");
	case GN_PROFILE_KEYVOL_Level1:	return _("Level 1");
	case GN_PROFILE_KEYVOL_Level2:	return _("Level 2");
	case GN_PROFILE_KEYVOL_Level3:	return _("Level 3");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_profile_volume_type2str(gn_profile_volume_type p)
{
	switch (p) {
	case GN_PROFILE_VOLUME_Level1:	return _("Level 1");
	case GN_PROFILE_VOLUME_Level2:	return _("Level 2");
	case GN_PROFILE_VOLUME_Level3:	return _("Level 3");
	case GN_PROFILE_VOLUME_Level4:	return _("Level 4");
	case GN_PROFILE_VOLUME_Level5:	return _("Level 5");
	default:			return _("Unknown");
	}
}

/* Call divert */

GNOKII_API const char *gn_call_divert_type2str(gn_call_divert_type p)
{
	switch (p) {
	case GN_CDV_Busy:		return _("Busy");
	case GN_CDV_NoAnswer:		return _("No answer");
	case GN_CDV_OutOfReach:		return _("Out of reach");
	case GN_CDV_NotAvailable:	return _("Not available");
	case GN_CDV_AllTypes:		return _("All");
	default:			return _("Unknown");
	}
}

GNOKII_API const char *gn_call_divert_call_type2str(gn_call_divert_call_type p)
{
	switch (p) {
	case GN_CDV_VoiceCalls:	return _("Voice");
	case GN_CDV_FaxCalls:	return _("Fax");
	case GN_CDV_DataCalls:	return _("Data");
	case GN_CDV_AllCalls:	return _("All");
	default:		return _("Unknown");
	}
}

GNOKII_API const char *gn_call_divert_operation2str(gn_call_divert_operation p)
{
	switch (p) {
	case GN_CDV_Disable:	return _("Disable");
	case GN_CDV_Enable:	return _("Enable");
	case GN_CDV_Query:	return _("Query");
	case GN_CDV_Register:	return _("Register");
	case GN_CDV_Erasure:	return _("Erasure");
	default:		return _("Unknown");
	}
}


/**
 * gn_number_sanitize - Remove all white charactes from the string
 * @number: input/output number string
 * @maxlen: maximal number length
 *
 * Use this function to sanitize GSM phone number format. It changes
 * number argument.
 */
GNOKII_API void gn_number_sanitize(char *number, int maxlen)
{
	char *iter, *e;

	iter = e = number;
	while (*iter && *e && (e - number < maxlen)) {
		*iter = *e;
		if (isspace(*iter)) {
			while (*e && isspace(*e) && (e - number < maxlen)) {
				e++;
			}
		}
		*iter = *e;
		iter++;
		e++;
	}
	*iter = 0;
}

/**
 * gn_phonebook_entry_sanitize - Remove all white charactes from the string
 * @entry: phonebook entry to sanitize
 *
 * Use this function before any attempt to write an entry to the phone.
 */
GNOKII_API void gn_phonebook_entry_sanitize(gn_phonebook_entry *entry)
{
	int i;

	gn_number_sanitize(entry->number, GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1);
	for (i = 0; i < entry->subentries_count; i++) {
		if (entry->subentries[i].entry_type == GN_PHONEBOOK_ENTRY_Number)
			gn_number_sanitize(entry->subentries[i].data.number, GN_PHONEBOOK_NUMBER_MAX_LENGTH + 1);
	}
}

/* 
 * This very small function is just to make it easier to clear
 * the data struct every time one is created 
 */
GNOKII_API void gn_data_clear(gn_data *data)
{
	memset(data, 0, sizeof(gn_data));
}
