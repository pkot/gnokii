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

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002       Ladis Michl
  Copyright (C) 2002-2004  BORBELY Zoltan, Pawel Kot

*/

#include "config.h"

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <memory.h>

#include "misc.h"
#include "gnokii.h"
#include "devices/serial.h"

#define USECOMM      /* yes, we need the COMM API */

HANDLE hPhone;
OVERLAPPED osWrite, osRead;

/* Open the serial port and store the settings. */
int serial_open(const char *file, int oflags)
{
	COMMTIMEOUTS  CommTimeOuts;

	if ((hPhone =
	    CreateFile(file, GENERIC_READ | GENERIC_WRITE,
	    0,                    /* exclusive access */
	    NULL,                 /* no security attrs */
	    OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL |
	    FILE_FLAG_OVERLAPPED, /* overlapped I/O */
	    NULL)) == (HANDLE) -1)
		return FALSE;
	else {
		/* get any early notifications */
		SetCommMask(hPhone, EV_RXCHAR);

		/* setup device buffers */
		SetupComm(hPhone, 4096, 4096);

		/* purge any information in the buffer */
		PurgeComm(hPhone, PURGE_TXABORT | PURGE_RXABORT |
			  PURGE_TXCLEAR | PURGE_RXCLEAR);

		/* set up for overlapped I/O */
		CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
		CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
		CommTimeOuts.ReadTotalTimeoutConstant = 1000;
#if 0
		/* CBR_9600 is approximately 1byte/ms. For our purposes, allow
		 * double the expected time per character for a fudge factor.
		 */
		CommTimeOuts.WriteTotalTimeoutMultiplier = 2 * CBR_9600 / CBR_115200;
#else
		CommTimeOuts.WriteTotalTimeoutMultiplier = 10;
#endif
		CommTimeOuts.WriteTotalTimeoutConstant = 0;
		SetCommTimeouts(hPhone, &CommTimeOuts);
	}

	return true;
}

/* Close the serial port and restore old settings. */
int serial_close(int fd, struct gn_statemachine *state)
{
	/* disable event notification and wait for thread
	 * to halt
	 */
	SetCommMask(hPhone, 0);

	/* drop DTR */
	EscapeCommFunction(hPhone, CLRDTR);

	/* purge any outstanding reads/writes and close device handle */
	PurgeComm(hPhone, PURGE_TXABORT | PURGE_RXABORT |
		  PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle(hPhone);

	return true;
}

/* Open a device with standard options.
 * Use value (-1) for "with_hw_handshake" if its specification is required from the user.
 */
int serial_opendevice(const char *file, int with_odd_parity,
		      int with_async, int with_hw_handshake,
		      struct gn_statemachine *state)
{
	DCB        dcb;

	if (!serial_open(file, 0)) return -1;

	/* set handshake */

	dcb.DCBlength = sizeof(DCB);
	GetCommState(hPhone, &dcb);
	dcb.fOutxDsrFlow = 0;
	if (state->config.hardware_handshake) {
		dcb.fOutxCtsFlow = TRUE;
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	} else {
		dcb.fOutxCtsFlow = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	}
	if (!SetCommState(hPhone, &dcb)) {
		fprintf(stderr, _("Gnokii serial_opendevice: cannot set handshake\n"));
		serial_close(0, state);
		return -1;
	}

	if (serial_changespeed(0, state->config.serial_baudrate, state) != GN_ERR_NONE)
		serial_changespeed(0, 19200 /* default value */, state);

	return 0;
}

/* Set the DTR and RTS bit of the serial device. */
void serial_setdtrrts(int fd, int dtr, int rts, struct gn_statemachine *state)
{
	BOOL       fRetVal;
	DCB        dcb;

	dcb.DCBlength = sizeof(DCB);

	GetCommState(hPhone, &dcb);

	dcb.fOutxDsrFlow = 0;
	if (dtr)
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
	else
		dcb.fDtrControl = DTR_CONTROL_DISABLE;

	dcb.fOutxCtsFlow = 0;
	if (rts)
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	else
		dcb.fRtsControl = RTS_CONTROL_DISABLE;

	/* no software flow control */

	dcb.fInX = dcb.fOutX = 0;

	fRetVal = SetCommState(hPhone, &dcb);
}


int serial_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
	usleep(timeout->tv_sec * 60 + timeout->tv_usec);
	return 1;
}


/* Change the speed of the serial device.
 * RETURNS: Success
 */
gn_error serial_changespeed(int fd, int speed, struct gn_statemachine *state)
{
	BOOL  fRetVal;
	DCB   dcb;

	dcb.DCBlength = sizeof(DCB);

	GetCommState(hPhone, &dcb);

	dcb.BaudRate = speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	fRetVal = SetCommState(hPhone, &dcb);

	if (fRetVal == 0) fRetVal = GetLastError();

	return GN_ERR_NONE;
}

/* Read from serial device. */
size_t serial_read(int fd, __ptr_t buf, size_t nbytes, struct gn_statemachine *state)
{
	BOOL    fReadStat;
	COMSTAT ComStat;
	DWORD   dwErrorFlags;
	DWORD   dwLength=0;
	DWORD   dwError;

	/* only try to read number of bytes in queue */
	ClearCommError(hPhone, &dwErrorFlags, &ComStat);
	dwLength = min((DWORD) nbytes, ComStat.cbInQue);

	if (dwLength > 0) {
		fReadStat = ReadFile(hPhone, buf,
				     dwLength, &dwLength, &osRead);
		if (!fReadStat) {
			if (GetLastError() == ERROR_IO_PENDING) {
				fprintf(stderr, _("\n\rIO Pending"));
				/* We have to wait for read to complete.
				 * This function will timeout according to the
				 * CommTimeOuts.ReadTotalTimeoutConstant variable
				 * Every time it times out, check for port errors
				 */
				while (!GetOverlappedResult(hPhone, &osRead, &dwLength, TRUE)) {
					dwError = GetLastError();
					if (dwError == ERROR_IO_INCOMPLETE) {
						/* normal result if not finished */
						continue;
					} else {
						/* an error occurred, try to recover */
						ClearCommError(hPhone, &dwErrorFlags, &ComStat);
						break;
					}
				}
			} else {
				/* some other error occurred */
				dwLength = 0;
				ClearCommError(hPhone, &dwErrorFlags, &ComStat);
			}
		}
	}

	return dwLength;
}

/* Write to serial device. */
size_t serial_write(int fd, __ptr_t buf, size_t n, struct gn_statemachine *state)
{
	BOOL    fWriteStat;
	DWORD   dwBytesWritten;
	DWORD   dwErrorFlags;
	DWORD   dwError;
	DWORD   dwBytesSent = 0;
	COMSTAT ComStat;

	fWriteStat = WriteFile(hPhone, buf, n, &dwBytesWritten, &osWrite);

	/* Note that normally the code will not execute the following
	 * because the driver caches write operations. Small I/O requests
	 * (up to several thousand bytes) will normally be accepted
	 * immediately and WriteFile will return true even though an
	 * overlapped operation was specified
	 */

	if (!fWriteStat) {
		if (GetLastError() == ERROR_IO_PENDING) {
			/* We should wait for the completion of the write operation
			 * so we know if it worked or not
			 *
			 * This is only one way to do this. It might be beneficial to
			 * place the write operation in a separate thread
			 * so that blocking on completion will not negatively
			 * affect the responsiveness of the UI
			 *
			 * If the write takes too long to complete, this
			 * function will timeout according to the
			 * CommTimeOuts.WriteTotalTimeoutMultiplier variable.
			 * This code logs the timeout but does not retry
			 * the write.
			 */
			while (!GetOverlappedResult(hPhone, &osWrite, &dwBytesWritten, TRUE)) {
				dwError = GetLastError();
				if (dwError == ERROR_IO_INCOMPLETE) {
					/* normal result if not finished */
					dwBytesSent += dwBytesWritten;
					continue;
				} else {
					/* an error occurred, try to recover */
					ClearCommError(hPhone, &dwErrorFlags, &ComStat);
					break;
				}
			}
			dwBytesSent += dwBytesWritten;
#if 0
			if (dwBytesSent != dwBytesToWrite)
				fprintf(stderr, _("\nProbable Write Timeout: Total of %ld bytes sent (%ld)"), dwBytesSent, dwBytesToWrite);
			else
				fprintf(stderr, _("\n%ld bytes written"), dwBytesSent);
#endif
		} else {
			/* some other error occurred */
			ClearCommError(hPhone, &dwErrorFlags, &ComStat);
			return 0;
		}
	}
	return n;
}

gn_error serial_nreceived(int fd, int *n, struct gn_statemachine *state)
{
	return GN_ERR_NONE;
}

gn_error serial_flush(int fd, struct gn_statemachine *state)
{
	return GN_ERR_NONE;
}
