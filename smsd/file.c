/*

  $Id$

  S M S D

  A Linux/Unix tool for the mobile phones.

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

  Copyright (C) 2002-2005 Jan Derfinak
  
  This file is a module to smsd for file access.

*/

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <glib.h>
#include "smsd.h"
#include "gnokii.h"
#include "compat.h"
#include "utils.h"

static gchar *action;
static gchar *spool;

GNOKII_API void DB_Bye (void)
{
  return;
}


GNOKII_API gint DB_ConnectInbox (DBConfig connect)
{
  struct stat status;
  
  if (connect.user[0] != '\0')
  {
    if ((stat (connect.user, &status)) != 0)
    {
      g_print (_("Cannot stat file %s!\n"), connect.user);
      return (1);
    }
    
    if (!((S_IFREG & status.st_mode) && 
        (((status.st_uid == geteuid ()) && (S_IXUSR & status.st_mode)) ||
         ((status.st_gid == getegid ()) && (S_IXGRP & status.st_mode)) ||
         (S_IXGRP & status.st_mode))))
    {
      g_print (_("File %s is not regular file or\nyou have not executable permission to this file!\n"),
               connect.user);
      return (2);
    }
  }
  
  action = connect.user;
  
  return (0);
}


GNOKII_API gint DB_ConnectOutbox (DBConfig connect)
{
  struct stat status;
  
  if (connect.host[0] == '\0')
    g_print (_("You have not set spool directory, sms sending is disabled!\n"));
  else
  {
    if ((stat (connect.host, &status)) != 0)
    {
      g_print (_("Cannot stat file %s!\n"), connect.host);
      return (1);
    }
    
    if (!((S_IFDIR & status.st_mode) && 
        (((status.st_uid == geteuid ()) && (S_IRUSR & status.st_mode) && (S_IWUSR & status.st_mode)) ||
         ((status.st_gid == getegid ()) && (S_IRGRP & status.st_mode) && (S_IWGRP & status.st_mode)) ||
         ((S_IROTH & status.st_mode) && (S_IWOTH & status.st_mode)))))
    {
      g_print (_("File %s is not directory or\nyou have not read and write permissions to this directory,\nsms sending is disabled!\n!"),
               connect.host);
      return (2);
    }
  }

  spool = connect.host;

  return (0);
}


GNOKII_API gint DB_InsertSMS (const gn_sms * const data, const gchar * const phone)
{
  FILE *p;
  GString *buf;
  gchar *text;
    
  text = strEscape ((gchar *) data->user_data[0].u.text);
  
  if (action[0] == '\0')
    g_print ("Number: %s, Date: %02d-%02d-%02d %02d:%02d:%02d\nText:\n%s\n", \
             data->remote.number, data->smsc_time.year, data->smsc_time.month, \
             data->smsc_time.day, data->smsc_time.hour, data->smsc_time.minute, \
             data->smsc_time.second, text);
  else
  {
    buf = g_string_sized_new (256);
    g_string_printf (buf, "%s %s \"%02d-%02d-%02d %02d:%02d:%02d\"", \
                     action, data->remote.number, data->smsc_time.year, \
                     data->smsc_time.month, data->smsc_time.day, \
                     data->smsc_time.hour, data->smsc_time.minute, \
                     data->smsc_time.second);
    if ((p = popen (buf->str, "w")) == NULL)
    {
      g_free (text);
      g_string_free (buf, TRUE);
      return (1);
    }
    
    g_string_free (buf, TRUE);
    
    fprintf (p, "%s", text);
    pclose (p);
  }
  
  g_free (text);

  return (0);
}


GNOKII_API void DB_Look (const gchar * const phone)
{
  DIR *dir;
  struct dirent *dirent;
  FILE *smsFile;
  GString *buf;
  gint numError, error;


  if (spool[0] == '\0')  // if user don't set spool dir, sending is disabled
    return;
    
  if ((dir = opendir (spool)) == NULL)
  {
    g_print (_("Cannot open directory %s\n"), spool);
    return;
  }

  buf = g_string_sized_new (64);
  
  while ((dirent = readdir (dir)))
  {
    gn_sms sms;
    gint slen = 0;
    
    if (strcmp (dirent->d_name, ".") == 0 || strcmp (dirent->d_name, "..") == 0 ||
        strncmp (dirent->d_name, "ERR.", 4) == 0)
      continue;
    
    g_string_printf (buf, "%s/%s", spool, dirent->d_name);
    
    if ((smsFile = fopen (buf->str, "r")) == NULL)
    {
      g_print (_("Can't open file %s for reading!\n"), buf->str);
      continue;
    }
    
    gn_sms_default_submit (&sms);
    memset (&sms.remote.number, 0, sizeof (sms.remote.number));

    if (fgets (sms.remote.number, sizeof (sms.remote.number), smsFile))
      slen = strlen (sms.remote.number);
    if (slen < 1)
    {
      error = -1;
      fclose (smsFile);
      g_print (_("Remote number is empty in %s!\n"), buf->str);
      goto handle_file;
    }
    
    if (sms.remote.number[slen - 1] == '\n')
      sms.remote.number[slen - 1] = '\0';
    
    /* Initialize SMS text */
    memset (&sms.user_data[0].u.text, 0, sizeof (sms.user_data[0].u.text));
    
    slen = fread ((gchar *) sms.user_data[0].u.text, 1, GN_SMS_MAX_LENGTH, smsFile);
    if (slen > 0 && sms.user_data[0].u.text[slen - 1] == '\n')
      sms.user_data[0].u.text[slen - 1] = '\0';
     
    fclose (smsFile);
    
//    sms.delivery_report = (smsdConfig.smsSets & SMSD_READ_REPORTS);

    if (sms.remote.number[0] == '+')
      sms.remote.type = GN_GSM_NUMBER_International;
    else
      sms.remote.type = GN_GSM_NUMBER_Unknown;
    
    sms.user_data[0].length = strlen ((gchar *) sms.user_data[0].u.text);
    sms.user_data[0].type = GN_SMS_DATA_Text;
    sms.user_data[1].type = GN_SMS_DATA_None;
    if (!gn_char_def_alphabet (sms.user_data[0].u.text))
       sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;


    gn_log_xdebug ("Sending SMS: %s, %s\n", sms.remote.number, sms.user_data[0].u.text);
    
    numError = 0;
    do
    {
      error = WriteSMS (&sms);
      sleep (1);
    }
    while ((error == GN_ERR_TIMEOUT || error == GN_ERR_FAILED) && numError++ < 3);

 handle_file:
    if (error == GN_ERR_NONE)
    {
      if (unlink (buf->str))
        g_print (_("Cannot unlink %s."), buf->str);
    }
    else
    {
      GString *buf2;
      
      buf2 = g_string_sized_new (64);
      g_string_printf (buf2, "%s/ERR.%s", spool, dirent->d_name);
      
      g_print (_("Cannot send sms from file %s\n"), buf->str);
      if (rename (buf->str, buf2->str))
      {
        g_print (_("Cannot rename file %s to %s. Trying to unlink it.\n"),
                 buf->str, buf2->str);
        if (unlink (buf->str))
          g_print (_("Cannot unlink %s."), buf->str);
      }
      g_string_free (buf2, TRUE);
    }
  }
  
  g_string_free (buf, TRUE);
  closedir (dir);
}
