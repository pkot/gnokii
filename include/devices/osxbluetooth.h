/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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
