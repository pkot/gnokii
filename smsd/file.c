/*

  $Id$

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.

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

  Copyright (C) 2002 Ján Derfiòák <ja@mail.upjs.sk>.
  
  This file is a module to smsd for file access.

*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <glib.h>
#include "smsd.h"
#include "gsm-sms.h"
#include "gsm-encoding.h"

static gchar *action;
static gchar *spool;

inline void DB_Bye (void)
{
  return;
}


gint DB_ConnectInbox (DBConfig connect)
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


gint DB_ConnectOutbox (DBConfig connect)
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


gint DB_InsertSMS (const GSM_API_SMS * const data)
{
  FILE *p;
  GString *buf;
  gchar *text;
    
  text = strEscape (data->UserData[0].u.Text);
  
  if (action[0] == '\0')
    g_print ("Number: %s, Date: %02d-%02d-%02d %02d:%02d:%02d\nText:\n%s\n", \
             data->Remote.Number, data->SMSCTime.Year, data->SMSCTime.Month, \
             data->SMSCTime.Day, data->SMSCTime.Hour, data->SMSCTime.Minute, \
             data->SMSCTime.Second, text);
  else
  {
    buf = g_string_sized_new (256);
    g_string_sprintf (buf, "%s %s \"%02d-%02d-%02d %02d:%02d:%02d\"", \
                      action, data->Remote.Number, data->SMSCTime.Year, \
                      data->SMSCTime.Month, data->SMSCTime.Day, \
                      data->SMSCTime.Hour, data->SMSCTime.Minute, \
                      data->SMSCTime.Second);
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


void DB_Look (void)
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
    GSM_API_SMS sms;
    
    if (strcmp (dirent->d_name, ".") == 0 || strcmp (dirent->d_name, "..") == 0 ||
        strncmp (dirent->d_name, "ERR.", 4) == 0)
      continue;
    
    g_string_sprintf (buf, "%s/%s", spool, dirent->d_name);
    
    if ((smsFile = fopen (buf->str, "r")) == NULL)
    {
      g_print (_("Cannot open %s.\n"), buf->str);
      continue;
    }
    
    gn_sms_default_submit (&sms);
    memset (&sms.Remote.Number, 0, sizeof (sms.Remote.Number));

    fgets (sms.Remote.Number, sizeof (sms.Remote.Number), smsFile);
    if (sms.Remote.Number[strlen (sms.Remote.Number) - 1] == '\n')
      sms.Remote.Number[strlen (sms.Remote.Number) - 1] = '\0';
    
    fgets (sms.UserData[0].u.Text, GSM_MAX_SMS_LENGTH + 1, smsFile);
    if (sms.UserData[0].u.Text[strlen (sms.UserData[0].u.Text) - 1] == '\n')
      sms.UserData[0].u.Text[strlen (sms.UserData[0].u.Text) - 1] = '\0';
     
    fclose (smsFile);
    
    sms.DeliveryReport = (smsdConfig.smsSets & SMSD_READ_REPORTS);

    if (sms.Remote.Number[0] == '+')
      sms.Remote.Type = SMS_International;
    else
      sms.Remote.Type = SMS_Unknown;
    
    sms.UserData[0].Length = strlen (sms.UserData[0].u.Text);
    sms.UserData[0].Type = SMS_PlainText;
    sms.UserData[1].Type = SMS_NoData;
    if (!gn_char_def_alphabet (sms.UserData[0].u.Text))
       sms.DCS.u.General.Alphabet = SMS_UCS2;


#ifdef XDEBUG
    g_print ("Sending SMS: %s, %s\n", sms.Remote.Number, sms.UserData[0].u.Text);
#endif
    
    numError = 0;
    do
    {
      error = WriteSMS (&sms);
      sleep (1);
    }
    while ((error == GN_ERR_TIMEOUT || error == GN_ERR_FAILED) && numError++ < 3);

    if (error == GN_ERR_NONE)
    {
      if (unlink (buf->str))
        g_print (_("Cannot unlink %s."), buf->str);
    }
    else
    {
      GString *buf2;
      
      buf2 = g_string_sized_new (64);
      g_string_sprintf (buf2, "%s/ERR.%s", spool, dirent->d_name);
      
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
