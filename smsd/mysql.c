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

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  This file is a module to smsd for MySQL db server.
  
*/

#include <string.h>
#include <glib.h>
#include <mysql.h>
#include "smsd.h"
#include "gsm-sms.h"

static MYSQL mysqlIn;
static MYSQL mysqlOut;

void DB_Bye (void)
{
  mysql_close (&mysqlIn);
  mysql_close (&mysqlOut);
}


gint DB_ConnectInbox (DBConfig connect)
{
  mysql_init (&mysqlIn);
  if (!mysql_real_connect (&mysqlIn,
                           connect.host[0] != '\0' ? connect.host : NULL,
                           connect.user[0] != '\0' ? connect.user : NULL,
                           connect.password[0] != '\0' ? connect.password : NULL,
                           connect.db, 0, NULL, 0))
  {
     g_print (_("Connection to database '%s' on host '%s' failed.\n"),
              connect.db, connect.host);
     g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
     return (1);
  }

  return (0);
}


gint DB_ConnectOutbox (DBConfig connect)
{
  mysql_init (&mysqlOut);
  if (!mysql_real_connect (&mysqlOut,
                           connect.host[0] != '\0' ? connect.host : NULL,
                           connect.user[0] != '\0' ? connect.user : NULL,
                           connect.password[0] != '\0' ? connect.password : NULL,
                           connect.db, 0, NULL, 0))
  {
     g_print (_("Connection to database '%s' on host '%s' failed.\n"),
              connect.db, connect.host);
     g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
     return (1);
  }

  return (0);
}


gint DB_InsertSMS (const GSM_API_SMS * const data)
{
  GString *buf;
  gchar *text;
  gint l;

/* MySQL has own escape function.    
  text = strEscape (data->UserData[0].u.Text);
*/
  
  text = g_malloc (strlen (data->UserData[0].u.Text) * 2 + 1);
  mysql_real_escape_string (&mysqlIn, text, data->UserData[0].u.Text, strlen (data->UserData[0].u.Text));
  
  buf = g_string_sized_new (256);
  g_string_sprintf (buf, "INSERT INTO inbox (number, smsdate, \
                    text, processed) VALUES ('%s', \
                    '%04d-%02d-%02d %02d:%02d:%02d', '%s', '0')",
                    data->Remote.Number, data->SMSCTime.Year, data->SMSCTime.Month,
                    data->SMSCTime.Day, data->SMSCTime.Hour, data->SMSCTime.Minute,
                    data->SMSCTime.Second, text);
  g_free (text);

  if (mysql_real_query (&mysqlIn, buf->str, buf->len))
  {
    g_print (_("%d: INSERT INTO inbox failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
    g_string_free(buf, TRUE);
    return (1);
  }
  
  g_string_free(buf, TRUE);
  
  return (0);
}


void DB_Look (void)
{
  GString *buf;
  MYSQL_RES *res1;
  MYSQL_ROW row;
  gint numError, error;

  buf = g_string_sized_new (128);

  g_string_sprintf (buf, "SELECT id, number, text FROM outbox \
                          WHERE processed='0'");

  if (mysql_real_query (&mysqlOut, buf->str, buf->len))
  {
    g_print (_("%d: SELECT FROM outbox command failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
    g_string_free (buf, TRUE);
    return;
  }

  if (!(res1 = mysql_store_result (&mysqlOut)))
  {
    g_print (_("%d: Store Mysql Result Failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
    g_string_free (buf, TRUE);
    return;
  }
  
  while ((row = mysql_fetch_row (res1)))
  {
    GSM_API_SMS sms;

    DefaultSubmitSMS (&sms);    
    memset (&sms.Remote.Number, 0, sizeof (sms.Remote.Number));
    sms.DeliveryReport = (smsdConfig.smsSets & SMSD_READ_REPORTS);

    strncpy (sms.Remote.Number, row[1], sizeof (sms.Remote.Number) - 1);
    sms.Remote.Number[sizeof(sms.Remote.Number) - 1] = '\0';
    if (sms.Remote.Number[0] == '+')
      sms.Remote.Type = SMS_International;
    else
      sms.Remote.Type = SMS_Unknown;
    
    strncpy (sms.UserData[0].u.Text, row[2], GSM_MAX_SMS_LENGTH + 1);
    sms.UserData[0].u.Text[GSM_MAX_SMS_LENGTH] = '\0';
    sms.UserData[0].Length = strlen (sms.UserData[0].u.Text);
    sms.UserData[0].Type = SMS_PlainText;
    sms.UserData[1].Type = SMS_NoData;

#ifdef XDEBUG
    g_print ("%s, %s\n", sms.Remote.Number, sms.UserData[0].u.Text);
#endif
    
    numError = 0;
    do
    {
      error = WriteSMS (&sms);
      sleep (1);
    }
    while ((error == GE_TIMEOUT || error == GE_SMSSENDFAILED) && numError++ < 3);

    g_string_sprintf (buf, "UPDATE outbox SET processed='1', error='%d', \
                            processed_date=NULL WHERE id='%s'",
                      error, row[0]);
                        
    if (mysql_real_query (&mysqlOut, buf->str, buf->len))
    {
      g_print (_("%d: UPDATE command failed.\n"), __LINE__);
      g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
    }
  }

  mysql_free_result (res1);

  g_string_free (buf, TRUE);
}
