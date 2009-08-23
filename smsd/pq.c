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
  
  This file is a module to smsd for PostgreSQL db server.

*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <libpq-fe.h>
#include "smsd.h"
#include "gnokii.h"
#include "compat.h"
#include "utils.h"

static PGconn *connIn = NULL;
static PGconn *connOut = NULL;
static gchar *schema = NULL;		/* database schema */

GNOKII_API void DB_Bye (void)
{
  if (connIn)
    PQfinish (connIn);

  if (connOut)
    PQfinish (connOut);
}


GNOKII_API gint DB_ConnectInbox (DBConfig connect)
{
  connIn = PQsetdbLogin (connect.host[0] != '\0' ? connect.host : NULL,
                         NULL,
                         NULL,
                         NULL,
                         connect.db,
                         connect.user[0] != '\0' ? connect.user : NULL,
                         connect.password[0] != '\0' ? connect.password : NULL);
  
  if (PQstatus (connIn) == CONNECTION_BAD)
  {
    g_print (_("Connection to database '%s' on host '%s' failed.\n"),
             connect.db, connect.host);
    g_print (_("Error: %s\n"), PQerrorMessage (connIn));
    return (1);
  }

  if (schema == NULL)
    schema = g_strdup (connect.schema);

  return (0);
}


GNOKII_API gint DB_ConnectOutbox (DBConfig connect)
{
  connOut = PQsetdbLogin (connect.host[0] != '\0' ? connect.host : NULL,
                          NULL,
                          NULL,
                          NULL,
                          connect.db,
                          connect.user[0] != '\0' ? connect.user : NULL,
                          connect.password[0] != '\0' ? connect.password : NULL);

  if (PQstatus (connOut) == CONNECTION_BAD)
  {
    g_print (_("Connection to database '%s' on host '%s' failed.\n"),
             connect.db, connect.host);
    g_print (_("Error: %s\n"), PQerrorMessage (connOut));
    return (1);
  }

  if (schema == NULL)
    schema = g_strdup (connect.schema);

  return (0);
}


GNOKII_API gint DB_InsertSMS (const gn_sms * const data, const gchar * const phone)
{
  GString *buf, *phnStr;
  gchar *text;
  PGresult *res;

  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_printf (phnStr, "'%s',", phone);
  }

  text = strEscape ((gchar *) data->user_data[0].u.text);
  
  buf = g_string_sized_new (256);
  g_string_printf (buf, "INSERT INTO %s.inbox (\"number\", \"smsdate\", \"insertdate\",\
                         \"text\", %s \"processed\") VALUES ('%s', \
                         '%02d-%02d-%02d %02d:%02d:%02d+01', 'now', '%s', %s 'f')",
                   schema,
                   phone[0] != '\0' ? "\"phone\"," : "", data->remote.number,
                   data->smsc_time.year, data->smsc_time.month,
                   data->smsc_time.day, data->smsc_time.hour,
                   data->smsc_time.minute, data->smsc_time.second, text, phnStr->str);
  g_free (text);
  g_string_free (phnStr, TRUE);
  
  res = PQexec (connIn, buf->str);
  g_string_free (buf, TRUE);
  if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    g_print (_("%d: INSERT INTO %s.inbox failed.\n"), __LINE__, schema);
    g_print (_("Error: %s\n"), PQerrorMessage (connIn));
    PQclear (res);
    return (1);
  }
  
  PQclear (res);
    
  return (0);
}


GNOKII_API void DB_Look (const gchar * const phone)
{
  GString *buf, *phnStr;
  PGresult *res1, *res2;
  register int i;
  gint numError, error;

  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_printf (phnStr, "AND phone = '%s'", phone);
  }

  buf = g_string_sized_new (128);

  res1 = PQexec (connOut, "BEGIN");
  PQclear (res1);

  g_string_printf (buf, "SELECT id, number, text, dreport FROM %s.outbox \
                         WHERE processed='f' AND localtime(0) >= not_before \
                         AND localtime(0) <= not_after %s FOR UPDATE",
                   schema, phnStr->str);
  g_string_free (phnStr, TRUE);

  res1 = PQexec (connOut, buf->str);
  if (!res1 || PQresultStatus (res1) != PGRES_TUPLES_OK)
  {
    g_print (_("%d: SELECT FROM %s.outbox command failed.\n"), __LINE__, schema);
    g_print (_("Error: %s\n"), PQerrorMessage (connOut));
    PQclear (res1);
    res1 = PQexec (connOut, "ROLLBACK TRANSACTION");
    PQclear (res1);
    g_string_free (buf, TRUE);
    return;
  }

  for (i = 0; i < PQntuples (res1); i++)
  {
    gn_sms sms;
    
    gn_sms_default_submit (&sms);
    memset (&sms.remote.number, 0, sizeof (sms.remote.number));
    sms.delivery_report = atoi (PQgetvalue (res1, i, 3));

    strncpy (sms.remote.number, PQgetvalue (res1, i, 1), sizeof (sms.remote.number) - 1);
    sms.remote.number[sizeof(sms.remote.number) - 1] = '\0';
    if (sms.remote.number[0] == '+')
      sms.remote.type = GN_GSM_NUMBER_International;
    else
      sms.remote.type = GN_GSM_NUMBER_Unknown;
    
    strncpy ((gchar *) sms.user_data[0].u.text, PQgetvalue (res1, i, 2), 10 * GN_SMS_MAX_LENGTH + 1);
    sms.user_data[0].u.text[10 * GN_SMS_MAX_LENGTH] = '\0';
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

    g_string_printf (buf, "UPDATE %s.outbox SET processed='t', error='%d', \
                           processed_date='now' WHERE id='%s'",
                     schema, error, PQgetvalue (res1, i, 0));

    res2 = PQexec (connOut, buf->str);
    if (!res2 || PQresultStatus (res2) != PGRES_COMMAND_OK)
    {
      g_print (_("%d: UPDATE command failed.\n"), __LINE__);   
      g_print (_("Error: %s\n"), PQerrorMessage (connOut));
    }

    PQclear (res2);
  }

  PQclear (res1);

  res1 = PQexec(connOut, "COMMIT");

  g_string_free(buf, TRUE);
  PQclear (res1);
}
