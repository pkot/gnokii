#ifndef __cbus_h
#define __cbus_h

#define CBUS_MAX_FRAME_LENGTH 256
#define CBUS_MAX_TRANSMIT_LENGTH 256
#define CBUS_MAX_MSG_LENGTH 256

enum CBUS_RX_States {
  CBUS_RX_Header,
  CBUS_RX_FrameType1,
  CBUS_RX_FrameType2,
  CBUS_RX_GetLengthLB,
  CBUS_RX_GetLengthHB,
  CBUS_RX_GetMessage,
  CBUS_RX_GetCSum
};


typedef struct{
  int checksum;
  int BufferCount;
  enum CBUS_RX_States state;
  int FrameHeader1;
  int FrameHeader2;
  int FrameType1;
  int FrameType2;
  int MessageLength;
  unsigned char buffer[CBUS_MAX_FRAME_LENGTH];
  u8 prev_rx_byte;
} CBUS_IncomingFrame;

typedef struct {
  int message_length;
  unsigned char buffer[CBUS_MAX_MSG_LENGTH];
} CBUS_OutgoingMessage;

typedef struct{
  CBUS_IncomingFrame i;
} CBUS_Link;

GSM_Error CBUS_Initialise(GSM_Link *newlink, GSM_Phone *newphone);

void sendat(char *msg);

#ifdef __cbus_c  /* Prototype functions for cbus.c only */

GSM_Error CBUS_Loop(struct timeval *timeout);
bool CBUS_OpenSerial();
void CBUS_RX_StateMachine(unsigned char rx_byte);
int CBUS_TX_SendFrame(u8 message_length, u8 message_type, u8 *buffer);
int CBUS_SendMessage(u16 messagesize, u8 messagetype, void *message);
int CBUS_TX_SendAck(u8 message_type, u8 message_seq);

#endif   /* #ifdef __cbus_c */

#endif   /* #ifndef __cbus_h */

