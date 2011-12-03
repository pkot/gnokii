/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001      Chris Kemp
  Copyright (C) 2002-2003 Pawel Kot, BORBELY Zoltan

  Header file for virtmodem code in virtmodem.c

*/

#ifndef _gnokii_virtmodem_h
#define _gnokii_virtmodem_h

struct vm_queue {
	int n;
	int head;
	int tail;
	unsigned char buf[256];
};

extern struct vm_queue queue;

/* Prototypes */
GNOKII_API int gn_vm_initialise(const char *iname,
			 const char *bindir,
			 int debug_mode,
			 int gn_init);
GNOKII_API void gn_vm_loop(void);
GNOKII_API void gn_vm_terminate(void);

#endif	/* _gnokii_virtmodem_h */
