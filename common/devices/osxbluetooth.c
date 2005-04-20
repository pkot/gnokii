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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2003       Siegfried Schloissnig

*/

#include "config.h"

#ifdef HAVE_BLUETOOTH_MACOSX

#include "compat.h"
#include "devices/osxbluetooth.h"
#include "misc.h"
#include "gnokii.h"
#include "devices/unixbluetooth.h"

/* ----- bluetooth io thread ----- */

static void thread_rfcommDataListener(IOBluetoothRFCOMMChannelRef rfcommChannel,
                               void* data, UInt16 length, void* refCon)
{
	threadContext *pContext = (threadContext *)refCon;
	void *pBuffer = malloc(length);
	dataBlock *pDataBlock = (dataBlock *)malloc(sizeof(dataBlock));

	memcpy(pBuffer, data, length);

	pDataBlock->pData = pBuffer;
	pDataBlock->nSize = length;

	pthread_mutex_lock(&(pContext->mutexWait));
	CFArrayAppendValue(pContext->arrDataReceived, pDataBlock);
	pthread_mutex_unlock(&(pContext->mutexWait));
}

static void *thread_main(void *pArg)
{
	threadContext* pContext = (threadContext *)pArg;
	IOBluetoothDeviceRef device = IOBluetoothDeviceCreateWithAddress(&(pContext->deviceAddress));
	IOBluetoothRFCOMMChannelRef rfcommChannel;

	if (IOBluetoothDeviceOpenRFCOMMChannel(device, pContext->nChannel,
				      &rfcommChannel) != kIOReturnSuccess) {
		rfcommChannel = 0;
	} else {
		/* register an incoming data listener */
		if (IOBluetoothRFCOMMChannelRegisterIncomingDataListener(rfcommChannel,
			 thread_rfcommDataListener, pArg) != kIOReturnSuccess) {
		    rfcommChannel = 0;
		}
	}

	pContext->rfcommChannel = rfcommChannel;

	pthread_mutex_unlock(&(pContext->mutexWait));

	/* start the runloop */
	CFRunLoopRun();
}

/* ---- bluetooth io thread ---- */

int bluetooth_open(const char* addr, uint8_t channel, struct gn_statemachine* state)
{
	/* create the thread context and start the thread */
	CFStringRef strDevice;
	pthread_t threadID;
	threadContext *pContext = (threadContext *)malloc(sizeof(threadContext));

	strDevice = CFStringCreateWithCString(kCFAllocatorDefault, addr, kCFStringEncodingMacRomanLatin1);
	IOBluetoothNSStringToDeviceAddress(strDevice, &pContext->deviceAddress);
	CFRelease(strDevice);

	pContext->arrDataReceived = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
	pContext->rfcommChannel = 0;
	pContext->nChannel = channel;

	pthread_mutex_init(&(pContext->mutexWait), NULL);
	pthread_mutex_lock(&(pContext->mutexWait));	/* lock */

	pthread_create(&(pContext->threadID), NULL, thread_main, pContext);

	/* wait until main finishes its initialization */
	pthread_mutex_lock(&(pContext->mutexWait));
	/* unblock the mutex */
	pthread_mutex_unlock(&(pContext->mutexWait));

	if (pContext->rfcommChannel == 0)
		return -1;
	else
		/* return the thread context as the file descriptor */
		return (int)pContext;
}

int bluetooth_close(int fd, struct gn_statemachine *state)
{
	threadContext *pContext = (threadContext *)fd;
        IOBluetoothDeviceRef device;

	sleep(2);

	if (fd != -1 && pContext->rfcommChannel > 0) {
		/* de-register the callback */
		IOBluetoothRFCOMMChannelRegisterIncomingDataListener(pContext->rfcommChannel, NULL, NULL);

		/* close channel and device connection */
		IOBluetoothRFCOMMChannelCloseChannel(pContext->rfcommChannel);
		device = IOBluetoothRFCOMMChannelGetDevice(pContext->rfcommChannel);
		IOBluetoothDeviceCloseConnection(device);
		/* IOBluetoothObjectRelease(pContext->rfcommChannel); */
		IOBluetoothObjectRelease(device);
	}

	return 1;
}

int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
	threadContext *pContext = (threadContext *)fd;

	if (IOBluetoothRFCOMMChannelWrite(pContext->rfcommChannel, bytes, size, TRUE) != kIOReturnSuccess)
		return -1;

	return size;
}

int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
	threadContext *pContext = (threadContext *)fd;
	int nOffset = 0;
	int nBytes = 0;
        dataBlock* pDataBlock;

	/* no data received so far */
	if (CFArrayGetCount(pContext->arrDataReceived) == 0)
		return 0;

	while (CFArrayGetCount(pContext->arrDataReceived) != 0) {
		pthread_mutex_lock(&(pContext->mutexWait));
		pDataBlock = (dataBlock*)CFArrayGetValueAtIndex(pContext->arrDataReceived, 0);
		pthread_mutex_unlock(&(pContext->mutexWait));

		if (pDataBlock->nSize == size) {
			/* copy data and remove block */
			memcpy(bytes + nOffset, pDataBlock->pData, size);

			pthread_mutex_lock(&(pContext->mutexWait));
			CFArrayRemoveValueAtIndex(pContext->arrDataReceived, 0);
			pthread_mutex_unlock(&(pContext->mutexWait));

			free(pDataBlock->pData);
			free(pDataBlock);

			return nBytes + size;
		} else if (pDataBlock->nSize > size) {
			/* copy data and update block contents */
			memcpy(bytes + nOffset, pDataBlock->pData, size);
			memmove(pDataBlock->pData, pDataBlock->pData + size, pDataBlock->nSize - size);
			pDataBlock->nSize -= size;
			return nBytes + size;
		} else { /* pDataBlock->nSize < size */
			/* copy data and remove block */
			memcpy(bytes + nOffset, pDataBlock->pData, pDataBlock->nSize);

			size -= pDataBlock->nSize;
			nOffset += pDataBlock->nSize;
			nBytes += pDataBlock->nSize;

			pthread_mutex_lock(&(pContext->mutexWait));
			CFArrayRemoveValueAtIndex(pContext->arrDataReceived, 0);
			pthread_mutex_unlock(&(pContext->mutexWait));

			free(pDataBlock->pData);
			free(pDataBlock);
		}
	}

	return nBytes;
}

int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	threadContext *pContext = (threadContext *)fd;
	int nRetVal = 0;

	usleep(timeout->tv_usec);

	pthread_mutex_lock(&(pContext->mutexWait));

	if (CFArrayGetCount(pContext->arrDataReceived) > 0)
		nRetVal = 1;

	pthread_mutex_unlock(&(pContext->mutexWait));

	return nRetVal;
}

#endif	/* HAVE_BLUETOOTH_MACOSX */
