/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 2002,2020  Ladislav Michl
  Copyright (C) 2002-2004  BORBELY Zoltan, Pawel Kot

*/

#include "compat.h"

#include <io.h>
#include <memory.h>

#include "misc.h"
#include "gnokii.h"
#include "devices/serial.h"

#define USECOMM      /* yes, we need the COMM API */

struct instance_data {
	HANDLE hPhone;
};

#define SELF(m) (((struct instance_data *)(instance))->m)

/* Open the serial port and store the settings. */
void* serial_init(const char *file, int oflags)
{
	HANDLE hPhone;
	COMMTIMEOUTS CommTimeOuts;
	struct instance_data *data;

	if ((hPhone =
	    CreateFile(file, GENERIC_READ | GENERIC_WRITE,
	    0,                    /* exclusive access */
	    NULL,                 /* no security attrs */
	    OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL |
	    FILE_FLAG_OVERLAPPED, /* overlapped I/O */
	    NULL)) == (HANDLE) -1)
		return NULL;

	data = calloc(sizeof(struct instance_data), 1);
	if (data == NULL) {
		CloseHandle(hPhone);
		return NULL;
	}
	data->hPhone = hPhone;

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

	return data;
}

/* Open a device with standard options.
 */
void* serial_open(gn_config *cfg, int with_odd_parity, int with_async)
{
	DCB        dcb;
	struct instance_data *data;

	data = (struct instance_data *)serial_init(cfg->port_device, 0);
	if (!data)
		return NULL;

	/* set handshake */
	dcb.DCBlength = sizeof(DCB);
	GetCommState(data->hPhone, &dcb);
	dcb.fOutxDsrFlow = 0;
	if (cfg->hardware_handshake) {
		dcb.fOutxCtsFlow = TRUE;
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	} else {
		dcb.fOutxCtsFlow = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	}
	if (!SetCommState(data->hPhone, &dcb)) {
		fprintf(stderr, _("Gnokii serial_opendevice: cannot set handshake\n"));
		serial_close(data);
		free(data);
		return NULL;
	}

	if (serial_changespeed(data, cfg->serial_baudrate) != GN_ERR_NONE)
		serial_changespeed(data, 19200 /* default value */);

	return data;
}

/* Close the serial port and restore old settings. */
void serial_close(void *instance)
{
	/* disable event notification and wait for thread
	 * to halt
	 */
	SetCommMask(SELF(hPhone), 0);

	/* drop DTR */
	EscapeCommFunction(SELF(hPhone), CLRDTR);

	/* purge any outstanding reads/writes and close device handle */
	PurgeComm(SELF(hPhone), PURGE_TXABORT | PURGE_RXABORT |
		  PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle(SELF(hPhone));
}

/* Set the DTR and RTS bit of the serial device. */
void serial_setdtrrts(void *instance, int dtr, int rts)
{
	DCB dcb;

	dcb.DCBlength = sizeof(DCB);

	GetCommState(SELF(hPhone), &dcb);

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

	SetCommState(SELF(hPhone), &dcb);
}


int serial_select(void *instance, struct timeval *timeout)
{
	usleep(timeout->tv_sec * 60 + timeout->tv_usec);
	return 1;
}


/* Change the speed of the serial device.
 * RETURNS: Success
 */
gn_error serial_changespeed(void *instance, int speed)
{
	BOOL  fRetVal;
	DCB   dcb;

	dcb.DCBlength = sizeof(DCB);

	GetCommState(SELF(hPhone), &dcb);

	dcb.BaudRate = speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	fRetVal = SetCommState(SELF(hPhone), &dcb);

	return fRetVal ? GN_ERR_NONE : GN_ERR_FAILED;
}

/* Read from serial device. */
size_t serial_read(void *instance, __ptr_t buf, size_t nbytes)
{
	BOOL    fReadStat;
	COMSTAT ComStat;
	DWORD   dwErrorFlags;
	DWORD   dwLength;
	DWORD   dwError;
	OVERLAPPED osRead;
	HANDLE  hPhone = SELF(hPhone);

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
				ClearCommError(SELF(hPhone), &dwErrorFlags, &ComStat);
			}
		}
	}

	return dwLength;
}

/* Write to serial device. */
size_t serial_write(void *instance, __ptr_t buf, size_t n)
{
	BOOL    fWriteStat;
	DWORD   dwBytesWritten;
	DWORD   dwErrorFlags;
	DWORD   dwError;
	DWORD   dwBytesSent = 0;
	COMSTAT ComStat;
	OVERLAPPED osWrite;
	HANDLE  hPhone = SELF(hPhone);

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

gn_error serial_nreceived(void *instance, int *n)
{
	return GN_ERR_NONE;
}

gn_error serial_flush(void *instance)
{
	return GN_ERR_NONE;
}
