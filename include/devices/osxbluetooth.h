/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2003       Siegfried Schloissnig

*/

#ifndef _gnokii_osx_bluetooth_h
#define _gnokii_osx_bluetooth_h

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOBluetooth/Bluetooth.h>
#include <IOBluetooth/IOBluetoothUserLib.h>
#include <pthread.h>

typedef struct {
	IOBluetoothRFCOMMChannelRef rfcommChannel;
	IOReturn ioReturnValue;
	pthread_t threadID;

	BluetoothDeviceAddress deviceAddress;
	uint8_t nChannel;

	pthread_mutex_t mutexWait;

	CFMutableArrayRef arrDataReceived;
} threadContext;

typedef struct {
	void *pData;
	unsigned int nSize;
} dataBlock;

#endif /* _gnokii_osx_bluetooth_h */
