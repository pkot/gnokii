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

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak

  This file is a module to smsd for MySQL db server.
  
*/

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <mysql.h>
#include "smsd.h"
#include "gnokii.h"
#include "compat.h"

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
     g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
     return (1);
  }

  return (0);
}


gint DB_InsertSMS (const gn_sms * const data, const gchar * const phone)
{
  GString *buf, *phnStr;
  gchar *text;
  gint l;


  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_sprintf (phnStr, "'%s',", phone);
  }

/* MySQL has own escape function.    
  text = strEscape (data->UserData[0].u.Text);
*/
  text = g_malloc (strlen (data->user_data[0].u.text) * 2 + 1);
  mysql_real_escape_string (&mysqlIn, text, data->user_data[0].u.text, strlen (data->user_data[0].u.text));
  
  buf = g_string_sized_new (256);
  g_string_sprintf (buf, "INSERT INTO inbox (number, smsdate, \
                    text, %s processed) VALUES ('%s', \
                    '%04d-%02d-%02d %02d:%02d:%02d', '%s', %s '0')",
                    phone[0] != '\0' ? "phone," : "", data->remote.number,
                    data->smsc_time.year, data->smsc_time.month,
                    data->smsc_time.day, data->smsc_time.hour,
                    data->smsc_time.minute, data->smsc_time.second, text, phnStr->str);
  g_free (text);
  g_string_free(phnStr, TRUE);

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


void DB_Look (const gchar * const phone)
{
  GString *buf, *phnStr;
  MYSQL_RES *res1;
  MYSQL_ROW row;
  gint numError, error;

  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_sprintf (phnStr, "AND phone = '%s'", phone);
  }

  buf = g_string_sized_new (128);

  g_string_sprintf (buf, "SELECT id, number, text, dreport FROM outbox \
                          WHERE processed='0' AND CURTIME() >= not_before \
                          AND CURTIME() <= not_after %s", phnStr->str);
  g_string_free (phnStr, TRUE);
  
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
    gn_sms sms;

    gn_sms_default_submit (&sms);    
    memset (&sms.remote.number, 0, sizeof (sms.remote.number));
    sms.delivery_report = atoi (row[3]);

    if (row[1] != NULL)
      strncpy (sms.remote.number, row[1], sizeof (sms.remote.number) - 1);
    else
      *sms.remote.number = '\0';
    sms.remote.number[sizeof (sms.remote.number) - 1] = '\0';
    if (sms.remote.number[0] == '+')
      sms.remote.type = GN_GSM_NUMBER_International;
    else
      sms.remote.type = GN_GSM_NUMBER_Unknown;
    
    if (row[2] != NULL)
      strncpy (sms.user_data[0].u.text, row[2], GN_SMS_MAX_LENGTH + 1);
    else
      *sms.user_data[0].u.text = '\0';
    sms.user_data[0].u.text[GN_SMS_MAX_LENGTH] = '\0';
    sms.user_data[0].length = strlen (sms.user_data[0].u.text);
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
