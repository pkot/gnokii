/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 2001 Pawe³ Kot <pkot@linuxnews.pl>

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Library for parsing and creating Short Messages (SMS).

  $Log$
  Revision 1.1  2001-07-09 23:06:26  pkot
  Moved sms.* files from my hard disk to CVS


*/

#include <stdlib.h>

#include "sms.h"

#define NUMBER_OF_7_BIT_ALPHABET_ELEMENTS 128

static GSM_Error error;

static unsigned char GSM_DefaultAlphabet[NUMBER_OF_7_BIT_ALPHABET_ELEMENTS] = {

	/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
	/* Characters in hex position 10, [12 to 1a] and 24 are not present on
	   latin1 charset, so we cannot reproduce on the screen, however they are
	   greek symbol not present even on my Nokia */
	
	'@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec, 
	0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
	'?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
	' ',  '!',  '\"', '#',  0xa4,  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
	0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

static unsigned char EncodeWithDefaultAlphabet(unsigned char value)
{
	unsigned char i;
	
	if (value == '?') return  0x3f;
	
	for (i = 0; i < NUMBER_OF_7_BIT_ALPHABET_ELEMENTS; i++)
		if (GSM_DefaultAlphabet[i] == value)
			return i;
	
	return '?';
}

static wchar_t EncodeWithUnicodeAlphabet(unsigned char value)
{
	wchar_t retval;

	if (mbtowc(&retval, &value, 1) == -1) return '?';
	else return retval;
}

static unsigned char DecodeWithDefaultAlphabet(unsigned char value)
{
	return GSM_DefaultAlphabet[value];
}

static unsigned char DecodeWithUnicodeAlphabet(wchar_t value)
{
	unsigned char retval;

	if (wctomb(&retval, value) == -1) return '?';
	else return retval;
}

#define ByteMask ((1 << Bits) - 1)

static int Unpack7BitCharacters(int offset, int in_length, int out_length,
                           unsigned char *input, unsigned char *output)
{
        unsigned char *OUT = output; /* Current pointer to the output buffer */
        unsigned char *IN  = input;  /* Current pointer to the input buffer */
        unsigned char Rest = 0x00;
        int Bits;

        Bits = offset ? offset : 7;

        while ((IN - input) < in_length) {

                *OUT = ((*IN & ByteMask) << (7 - Bits)) | Rest;
                Rest = *IN >> Bits;

                /* If we don't start from 0th bit, we shouldn't go to the
                   next char. Under *OUT we have now 0 and under Rest -
                   _first_ part of the char. */
                if ((IN != input) || (Bits == 7)) OUT++;
                IN++;

                if ((OUT - output) >= out_length) break;

                /* After reading 7 octets we have read 7 full characters but
                   we have 7 bits as well. This is the next character */
                if (Bits == 1) {
                        *OUT = Rest;
                        OUT++;
                        Bits = 7;
                        Rest = 0x00;
                } else {
                        Bits--;
                }
        }

        return OUT - output;
}

static int Pack7BitCharacters(int offset, unsigned char *input, unsigned char *output)
{

        unsigned char *OUT = output; /* Current pointer to the output buffer */
        unsigned char *IN  = input;  /* Current pointer to the input buffer */
        int Bits;                    /* Number of bits directly copied to
                                        the output buffer */

        Bits = (7 + offset) % 8;

        /* If we don't begin with 0th bit, we will write only a part of the
           first octet */
        if (offset) {
                *OUT = 0x00;
                OUT++;
        }

        while ((IN - input) < strlen(input)) {

                unsigned char Byte = EncodeWithDefaultAlphabet(*IN);

                *OUT = Byte >> (7 - Bits);
                /* If we don't write at 0th bit of the octet, we should write
                   a second part of the previous octet */
                if (Bits != 7)
                        *(OUT-1) |= (Byte & ((1 << (7-Bits)) - 1)) << (Bits+1);

                Bits--;

                if (Bits == -1) Bits = 7;
                else OUT++;

                IN++;
        }

        return (OUT - output);
}

/* Concatenated messages */
#define SMS_UDH_CM_Length 0x05
#define SMS_UDH_CM "\x00\x03\x01\x00\x00"
/* Operator logos */
#define SMS_UDH_OL_Length 0x06
#define SMS_UDH_OL "\x05\x04\x15\x82\x00\x00"
/* Caller logos */
#define SMS_UDH_CL_Length 0x06
#define SMS_UDH_CL "\x05\x04\x15\x82\x00\x00"
/* Ringtones */
#define SMS_UDH_RT_Length 0x06
#define SMS_UDH_RT "\x05\x04\x15\x81\x00\x00"
/* Voice Messages */
#define SMS_UDH_VM_Length 0x04
#define SMS_UDH_VM "\x03\x01\x00\x00"
/* Fax Messages */
#define SMS_UDH_FM_Length 0x04
#define SMS_UDH_FM "\x03\x01\x01\x00"
/* Email Messages */
#define SMS_UDH_EM_Length 0x04
#define SMS_UDH_EM "\x03\x01\x02\x00"

struct {
	{"3810", 0},
	{"6110", 1},
	{"6130", 1},
	{"6150", 1},
	{"6210", 1},
	{"7110", 1}
} PhoneSMSEncoding;

/* This function encodes the UserDataHeader as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
*/
static GSM_Error EncodeUDH(SMS_UDHInfo UDHi, char *UDH)
{
	unsigned char pos;

	pos = UDH[0];
	switch (UDHi.Type) {
	case SMS_NoUDH:
		break;
	case SMS_ConcatenatedMessages:
		UDH[0] += SMS_UDH_CM_Length; // UDH Length
		memcpy(UDH+pos+1, SMS_UDH_CM, SMS_UDH_CM_Length);
		break;
	case SMS_OpLogo:
		UDH[0] += SMS_UDH_OL_Length;
		memcpy(UDH+pos+1, SMS_UDH_OL, SMS_UDH_OL_Length);
		break;
	case SMS_CallerIDLogo:
		UDH[0] += SMS_UDH_CL_Length;
		memcpy(UDH+pos+1, SMS_UDH_OL, SMS_UDH_OL_Length);
		break;
	case SMS_Ringtone:
		UDH[0] += SMS_UDH_RT_Length;
		memcpy(UDH+pos+1, SMS_UDH_RT, SMS_UDH_RT_Length);
		break;
	case SMS_VoiceMessage:
		UDH[0] += SMS_UDH_VM_Length;
		memcpy(UDH+pos+1, SMS_UDH_VM, SMS_UDH_VM_Length);
		UDH[pos+4] = UDHi.u.SpecialSMSMessageIndication.MessageCount;
		if (UDHi.u.SpecialSMSMessageIndication.Store) UDH[pos+3] |= 0x80;
		break;
	case SMS_FaxMessage:
		UDH[0] += SMS_UDH_FM_Length;
		memcpy(UDH+pos+1, SMS_UDH_FM, SMS_UDH_FM_Length);
		UDH[pos+4] = UDHi.u.SpecialSMSMessageIndication.MessageCount;
		if (UDHi.u.SpecialSMSMessageIndication.Store) UDH[pos+3] |= 0x80;
		break;
	case SMS_EmailMessage:
		UDH[0] += SMS_UDH_EM_Length;
		memcpy(UDH+pos+1, SMS_UDH_EM, SMS_UDH_EM_Length);
		UDH[pos+4] = UDHi.u.SpecialSMSMessageIndication.MessageCount;
		if (UDHi.u.SpecialSMSMessageIndication.Store) UDH[pos+3] |= 0x80;
		break;
	default:
		dprintf(_("Not supported User Data Header type\n"));
		break;
	}
	return GE_NONE;
}

static GSM_Error EncodeSMSSubmitHeader(GSM_SMSMessage *SMS, char *frame)
{
	/* SMS Center */

	/* Reply Path */
	if (SMS->ReplyViaSameSMSC) frame[13] |= 0x80;

	/* User Data Header */
	if (SMS->UDH.Type) {
		frame[13] |= 0x40;
		if ((error = EncodeUDH(SMS->UDH, frame)) != GE_NONE) return error;
	}

	/* Status (Delivery) Report Request */
	if (SMS->Report) frame[13] |= 0x20;

	/* Validity Period Format: mask - 0x00, 0x10, 0x08, 0x18 */
	frame[13] |= ((SMS->Validity.VPF & 0x03) << 3);

	/* Reject Duplicates */
	if (SMS->RejectDuplicates) frame[13] |= 0x04;

	/* Message Type is already set */

	/* Message Reference */
	/* Can we set this? */

	/* Protocol Identifier */
	/* FIXME: allow to change this in better way.
	          currently only 0x5f == `Return Call Message' is used */
	frame[16] = SMS->PID;

	/* Data Coding Scheme */
	switch (SMS->DCS.Type) {
	case SMS_GeneralDataCoding:
		if (SMS->DCS.u.General.Compressed) frame[17] |= 0x20;
		if (SMS->DCS.u.General.Class) frame[17] |= (0x10 | (SMS->DCS.u.General.Class - 1));
		frame[17] |= ((SMS->DCS.u.General.Alphabet & 0x03) << 2);
		break;
	case SMS_MessageWaiting:
		if (SMS->DCS.u.MessageWaiting.Discard) frame[17] |= 0xc0;
		else if (SMS->DCS.u.MessageWaiting.Alphabet == SMS_UCS2) frame[17] |= 0xe0;
		else frame[17] |= 0xd0;
		if (SMS->DCS.u.MessageWaiting.Active) frame[17] |= 0x80;
		frame[17] |= (SMS->DCS.u.MessageWaiting.Type & 0x03);
		break;
	default:
		dprintf(_("Wrong Data Coding Scheme (DCS) format\n"));
		return GE_SMSWRONGFORMAT;
	}

	/* User Data Length */
	/* Will be filled later. */

	/* Destination Address */

	/* Validity Period */
	switch (SMS->Validity.VPF) {
	case SMS_EnhancedFormat:
		break;
	case SMS_RelativeFormat:
		break;
	case SMS_AbsoluteFormat:
		break;
	default:
		break;
	}

	return GE_NONE;
}

static GSM_Error EncodeSMSDeliverHeader()
{
	return GE_NONE;
}

static GSM_Error EncodeSMSHeader(GSM_SMSMessage *SMS, char *frame)
/* We can create either SMS DELIVER (for saving in Inbox) or SMS SUBMIT
   (for sending or saving in Outbox) message */
{
	/* Set SMS type */
	frame[12] |= (SMS->Type >> 1);
	switch (SMS->Type) {
	case SMS_Submit: /* we send SMS or save it in Outbox */
		return EncodeSMSSubmitHeader(SMS, frame);
	case SMS_Deliver: /* we save SMS in Inbox */
		return EncodeSMSSubmitHeader(SMS, frame);
	default: /* we don't create other formats of SMS */
		return GE_SMSWRONGFORMAT;
	}
}

/* This function encodes SMS as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9
*/
GSM_Error EncodePDUSMS(GSM_SMSMessage *SMS, char *frame)
{
	/* Short Message Data info element id */

	/* Length of Short Message Data */

	/* Short Message Reference value */
	if (SMS->Location) frame[3] = SMS->Location;

	/* Short Message status */
	if (SMS->Status) frame[1] = SMS->Status;

	/* Message Type */
	frame[4];

	/* SMSC number */
	if (SMS->SMSC.Number) {
		error = GetSMSCenter(&SMS->SMSC);
		if (error != GE_NONE)
			return error;
		SMS->SMSC.Number = 0;
	}
	dprintf(_("Sending SMS to %s via message center %s\n"), SMS->Address, SMS->MessageCenter.Number);

	/* Header coding */
	EncodeHeader(SMS, frame);
	if (error) return error;

	/* User Data Header - if present */
	if (SMS->UDH.Type != SMS_NoUDH) {
		EncodeUDH(&(SMS->UDH), frame);
		if (error) return error;
	}

	/* User Data */
	EncodeData(&(SMS->Data), &(SMS->DCS), frame);

	return error;
}

/* This function does simple SMS encoding - no PDU coding */
GSM_Error EncodeTextSMS()
{
	return GE_NONE;
}

/* This function decodes UDH as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9.2.3.24
   - Smart Messaging Specification, Revision 1.0.0, September 15, 1997
*/
static GSM_Error DecodeUDH(char *SMSMessage, SMS_UDHInfo **UDHi)
{
	unsigned char length, pos, nr;

	length = SMSMessage[0];
	pos = 1;
	nr = 0;
	while (length > 0) {
		unsigned char udh_length;

		udh_length = SMSMessage[pos+1];
		switch (SMSMessage[pos]) {
		case 0x00: // Concatenated short messages
			UDHi[nr]->Type = SMS_ConcatenatedMessages;
			UDHi[nr]->u.ConcatenatedShortMessage.ReferenceNumber = SMSMessage[pos + 3];
			UDHi[nr]->u.ConcatenatedShortMessage.MaximumNumber   = SMSMessage[pos + 4];
			UDHi[nr]->u.ConcatenatedShortMessage.CurrentNumber   = SMSMessage[pos + 5];
			break;
		case 0x01: // Special SMS Message Indication
			switch (SMSMessage[pos + 3] & 0x03) {
			case 0x00:
				UDHi[nr]->Type = SMS_VoiceMessage;
				break;
			case 0x01:
				UDHi[nr]->Type = SMS_FaxMessage;
				break;
			case 0x02:
				UDHi[nr]->Type = SMS_EmailMessage;
				break;
			default:
				UDHi[nr]->Type = SMS_UnknownUDH;
				break;
			}
			UDHi[nr]->u.SpecialSMSMessageIndication.Store = (SMSMessage[pos + 3] & 0x80) >> 7;
			UDHi[nr]->u.SpecialSMSMessageIndication.MessageCount = SMSMessage[pos + 4];
			break;
		case 0x04: // Application port addression scheme, 8 bit address
			break;
		case 0x05: // Application port addression scheme, 16 bit address
			break;
		case 0x06: // SMSC Control Parameters
			break;
		case 0x07: // UDH Source Indicator
			break;
		default:
			break;
		}
		length -= (udh_length + 2);
		pos += (udh_length + 2);
		nr++;
	}
	return GE_NONE;
}

static GSM_Error DecodeSMSSubmit()
{
	return GE_NONE;
}

static GSM_Error DecodeSMSDeliver()
{
	return GE_NONE;
}

static GSM_Error DecodeSMSHeader()
{
	return GE_NONE;
}

/* This function decodes SMS as described in:
   - GSM 03.40 version 6.1.0 Release 1997, section 9
*/
GSM_Error DecodePDUSMS()
{
	return GE_NONE;
}

/* This function does simple SMS decoding - no PDU coding */
GSM_Error DecodeTextSMS()
{
	return GE_NONE;
}
