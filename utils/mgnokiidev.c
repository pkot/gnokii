/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Mgnokiidev gets passed a slave pty number by gnokiid and uses this
  information to create a symlink from the pty to /dev/gnokii.

  Using a number rather than a partial device name is more secure but may be
  limiting on some systems. If it is on yours, please tell me and/or submit
  a patch...
	
  Last modification: Mon Mar 20 21:40:04 CET 2000
  Modified by Pavel Janík ml. <Pavel.Janik@linux.cz>

*/

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
  int count, err;
  int dev_number;
  char dev_name[30];

  /* Check we have one and only one command line argument. */
  if (argc != 2) {
    fprintf(stderr, "mgnokiidev takes one and only one argument!\n");
    exit(-2);
  }

  /* Check if argument has a reasonable length (less than 8 characters) */
  if (strlen(argv[1]) >= 8) {
    fprintf(stderr, "Argument must be less than 8 characters.\n");
    exit (-2);
  }

  /* Check for suspicious characters. */
  for (count = 0; count < strlen(argv[1]); count ++)
    if (!isalnum(argv[1][count])) {
      fprintf(stderr, "Suspicious character at index %d in argument.\n", count);
      exit (-2);
    }

  /* Get device number */	
  dev_number = atoi(argv[1]);

  /* Sanity check. */
  if (dev_number < 0 || dev_number > 100) {
    fprintf(stderr, "tty number was bizzare!.\n");
    exit (-2);
  }

  /* Turn device number into full path */
  sprintf(dev_name, "/dev/ttyp%d", dev_number);

  /* Now become root */
  setuid(0);

  /* Change group of slave pty to group of mgnokiidev */
  err = chown(dev_name, -1, getgid());

  if (err < 0) {
    perror("mgnokiidev - chown: ");
    exit (-2);
  }

  /* Change permissions to rw by group */
  err = chmod(dev_name, S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR);
	
  if (err < 0) {
    perror("mgnokiidev - chmod: ");
    exit (-2);
  }

  /* FIXME: Possible bug - should check that /dev/gnokii doesn't already exist
     in case multiple users are trying to run gnokii. Well, but will be
     mgnokiidev called then? I do not think so - you will probably got the
     message serialport in use or similar. Don't you. I haven't tested it
     though. */

  /* Remove symlink in case it already exists. Don't care if it fails.  */
  unlink ("/dev/gnokii");

  /* Create symlink */
  err = symlink(dev_name, "/dev/gnokii");

  if (err < 0) {
    perror("mgnokiidev - symlink: ");
    exit (-2);
  }

  /* Done */
  exit (0);
}
