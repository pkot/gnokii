/* This is bus for dancall phones.
   It is currently not used by anything, and probably was never tested.
*/

/* System header files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Various header file */

#define DEBUG
#include "config.h"
#include "misc.h"
#include "gsm-common.h"
#include "gsm-ringtones.h"
#include "gsm-networks.h"
#include "device.h"
#include "links/utils.h"

#define __links_cbus_
#include "cbus.h"

/* FIXME - pass device_* the link stuff?? */
/* FIXME - win32 stuff! */


/* Some globals */

GSM_Link *glink;
GSM_Phone *gphone;
CBUS_Link clink;	/* CBUS specific stuff, internal to this file */

int init_okay = 0;


/*--------------------------------------------------------------------------*/

bool CBUS_OpenSerial()
{
	int result;
	result = device_open(glink->PortDevice, false, false, GCT_Serial);
	if (!result) {
		perror(_("Couldn't open CBUS device"));
		return (false);
	}
	device_changespeed(9600);
	device_setdtrrts(1, 0);
	return (true);
}

/* -------------------------------------------------------------------- */

int xread(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_read(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				printf("I/O error : %m?!\n");
				return -1;
			} else device_select(NULL);
		} else {
			d += res;
			len -= res;
			printf("(%d)", len);
		}
	}
	return 0;
}

int xwrite(unsigned char *d, int len)
{
	int res;
	while (len) {
		res = device_write(d, len);
		if (res == -1) {
			if (errno != EAGAIN) {
				printf("I/O error : %m?!\n");
				return -1;
			}
		} else {
			d += res;
			len -= res;
			printf("<%d>", len);
		}
	}
	return 0;
}

void
say(unsigned char *c, int len)
{
	unsigned char d[10240];

	xwrite(c, len);
	xread(d, len);
	if (memcmp(c, d, len)) {
		int i;
		printf("Did not get my own echo?: ");
		for (i=0; i<len; i++)
			printf("%x ", d[i]);
		printf("\n");
	}			
}

int
waitack(void)
{
	unsigned char c;
	printf("Waiting ack\n");
	if (xread(&c, 1) == 0) {
		printf("Got %x\n", c);
		if (c != 0xa5)
			printf("???\n");
		else return 1;
	} else printf("Timeout?\n");
	return 0;
}

void
sendpacket(unsigned char *msg, int len, unsigned short cmd)
{
	unsigned char p[10240], csum = 0;
	int pos;

	p[0] = 0x34;
	p[1] = 0x19;
	p[2] = cmd & 0xff;
	p[3] = 0x68;
	p[4] = len & 0xff;
	p[5] = len >> 8;
	memcpy(p+6, msg, len);
	
	pos = 6+len;
	{
		int i;
		for (i=0; i<pos; i++) {
			csum = csum ^ p[i];
		}
	}
	p[pos] = csum;
	{
		int i;
		printf("Sending: ");
		for (i=0; i<=pos; i++) {
			printf("%x ", p[i]);
		}
		printf("\n");
	}
	say(p, pos+1);
	waitack();
}

/* -------------------------------------------------------------------- */

/* RX_State machine for receive handling.  Called once for each character
   received from the phone. */

void CBUS_RX_StateMachine(unsigned char rx_byte)
{
	CBUS_IncomingFrame *i = &clink.i;

	/* checksum is XOR of all bytes in the frame */
	if (i->state != CBUS_RX_GetCSum) i->checksum ^= rx_byte;

	switch (i->state) {

	case CBUS_RX_Header:
		switch (rx_byte) {
			case 0x38:
			case 0x34:
				if (i->prev_rx_byte == 0x19) {
					i->state = CBUS_RX_FrameType1;
				}
				break;
			case 0x19:
				if ((i->prev_rx_byte == 0x38) || (i->prev_rx_byte == 0x34)) {
					i->state = CBUS_RX_FrameType1;
				}
				break;
			default:
		}
		if (i->state != CBUS_RX_Header) {
			i->FrameHeader1 = i->prev_rx_byte;
			i->FrameHeader2 = rx_byte;
			i->BufferCount = 0;
			i->checksum = i->prev_rx_byte ^ rx_byte;
		}
		break;
	
	/* FIXME: Do you know exact meaning? just mail me. ladis. */ 
	case CBUS_RX_FrameType1:
		i->FrameType1 = rx_byte;
		i->state = CBUS_RX_FrameType2;
		break;

	/* FIXME: Do you know exact meaning? just mail me. ladis. */
	case CBUS_RX_FrameType2:
		i->FrameType2 = rx_byte; 
		i->state = CBUS_RX_GetLengthLB;
		break;

	/* message length - low byte */
	case CBUS_RX_GetLengthLB:
		i->MessageLength = rx_byte;
		i->state = CBUS_RX_GetLengthHB;
		break;
	
	/* message length - high byte */
	case CBUS_RX_GetLengthHB:
		i->MessageLength = i->MessageLength | rx_byte << 8;
		/* there are also empty commands */
		if (i->MessageLength == 0) 
			i->state = CBUS_RX_GetCSum;
		else 
			i->state = CBUS_RX_GetMessage;
		break;

	/* get each byte of the message body */
	case CBUS_RX_GetMessage:
		i->buffer[i->BufferCount++] = rx_byte;
		/* avoid buffer overflow */
		if (i->BufferCount > CBUS_MAX_MSG_LENGTH) {
			dprintf("CBUS: Message buffer overun - resetting\n");
			i->state = CBUS_RX_Header;
			break;
		}
		
		if (i->BufferCount == i->MessageLength) 
			i->state = CBUS_RX_GetCSum;
		break;

	/* get checksum */
	case CBUS_RX_GetCSum:
		/* compare against calculated checksum. */
		if (i->checksum == rx_byte) {
			u8 ack = 0xa5;

			xwrite(&ack, 1);
			xread(&ack, 1);
			if (ack != 0xa5)
				printf("ack lost, expect armagedon!\n");

			/* Got checksum, matches calculated one, so  
			 * now pass to appropriate dispatch handler. */
			i->buffer[i->MessageLength + 1] = 0;
			/* FIXME: really call it :-) */

			switch(i->FrameType2) {
			case 0x68:
				link_dispatch(glink, gphone, i->FrameType1, i->buffer, i->MessageLength);
				break;
			case 0x70:
				if (i->FrameType1 == 0x91) {
					init_okay = 1;
					printf("Registration acknowledged\n");
				} else
					printf("Unknown message\n");
			}
		} else { 
			/* checksum doesn't match so ignore. */
			dprintf("CBUS: Checksum error; expected: %02x, got: %02x", i->checksum, rx_byte);
		}
		
		i->state = CBUS_RX_Header;
		break;
		
	default:
	}			
	i->prev_rx_byte = rx_byte;
}

int CBUS_SendMessage(u16 message_length, u8 message_type, void * buffer)
{
	sendpacket(buffer, message_length, message_type);
	return true;
}

/* This is the main loop function which must be called regularly */
/* timeout can be used to make it 'busy' or not */

/* ladis: this function ought to be the same for all phones... */

GSM_Error CBUS_Loop(struct timeval *timeout)
{
	unsigned char buffer[255];
	int count, res;

	res = device_select(timeout);
	if (res > 0) {
		res = device_read(buffer, 255);
		for (count = 0; count < res; count++)
			CBUS_RX_StateMachine(buffer[count]);
	} else
		return GE_TIMEOUT;

	/* This traps errors from device_read */
	if (res > 0)
		return GE_NONE;
	else
		return GE_INTERNALERROR;
}


int CBUS_TX_SendAck(u8 message_type, u8 message_seq)
{
	dprintf("[Sending Ack]\n");
	return (0);
}


/* Initialise variables and start the link */

GSM_Error CBUS_Initialise(GSM_Link * newlink, GSM_Phone * newphone)
{
        setvbuf(stdout, _IONBF, NULL, 0);
        setvbuf(stderr, _IONBF, NULL, 0);

	/* 'Copy in' the global structures */
	glink = newlink;
	gphone = newphone;

	/* Fill in the link functions */
	glink->Loop = &CBUS_Loop;
	glink->SendMessage = &CBUS_SendMessage;

	if (glink->ConnectionType == GCT_Serial) {
		if (!CBUS_OpenSerial())
			return GE_DEVICEOPENFAILED;
	} else {
		fprintf(stderr, "Device not supported by CBUS");
		return GE_DEVICEOPENFAILED;
	}

	dprintf("Saying hello...");
	{
		char init1[] = { 0, 0x38, 0x19, 0xa6, 0x70, 0x00, 0x00, 0xf7, 0xa5 };
		say( init1, 8 );
		say( init1, 8 );
	}
	usleep(10000);
	dprintf("second hello...");
    	{
		char init1[] = { 0x38, 0x19, 0x90, 0x70, 0x01, 0x00, 0x1f, 0xdf };
		say( init1, 8 );
		if (!waitack())
			return GE_BUSY;
	}
	while(!init_okay)
		CBUS_Loop(NULL);
	return GE_NONE;
}
