/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id$
  
*/

#include <string.h>
#include <glib.h>
#include <libpq-fe.h>
#include "smsd.h"
#include "gsm-common.h"

static PGconn *conn = NULL;

void DB_Bye (void)
{
  if (conn)
    PQfinish (conn);
}


gint DB_Connect (DBConfig connect)
{
  GString *buf = g_string_sized_new (64);

  g_string_sprintf (buf, "dbname=%s", connect.db);

  if (connect.user[0] != '\0')
    g_string_sprintfa (buf, " user=%s", connect.user);

  if (connect.password[0] != '\0')
    g_string_sprintfa (buf, " password=%s", connect.password);

  if (connect.host[0] != '\0')
    g_string_sprintfa (buf, " host=%s", connect.host);
 
  conn = PQconnectdb (buf->str);
  
  if (PQstatus (conn) == CONNECTION_BAD)
  {
     g_print ("Connection to database '%s' failed.\n", buf->str);
     g_print ("%s", PQerrorMessage(conn));
     g_string_free (buf, true);
     return (1);
  }

  g_string_free (buf, true);
  return (0);
}


gint DB_InsertSMS (const GSM_SMSMessage * const data)
{
  GString *buf;
  gchar *text;
  PGresult *res;
    
  text = strEscape (data->UserData[0].u.Text);
  
  buf = g_string_sized_new (256);
  g_string_sprintf (buf, "INSERT INTO inbox (\"number\", \"smsdate\", \"insertdate\",\
                    \"text\", \"processed\") VALUES ('%s',
                    '%02d-%02d-%02d %02d:%02d:%02d+01', 'now', '%s', 'f')",
                    data->RemoteNumber.number, data->Time.Year, data->Time.Month,
                    data->Time.Day, data->Time.Hour, data->Time.Minute,
                    data->Time.Second, text);
  g_free (text);
  
  res = PQexec(conn, buf->str);
  g_string_free(buf, TRUE);
  if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    PQclear (res);
    return (1);
  }
  
  PQclear (res);
    
  return (0);
}


void DB_Look (void)
{
  GString *buf;
  PGresult *res1, *res2;
  register int i;

  buf = g_string_sized_new (128);

  g_string_sprintf (buf, "BEGIN");

  res1 = PQexec (conn, buf->str);
  PQclear (res1);

  g_string_sprintf (buf, "SELECT id, number, text FROM outbox \
                          WHERE processed='f' FOR UPDATE");

  res1 = PQexec (conn, buf->str);
  if (!res1 || PQresultStatus (res1) != PGRES_TUPLES_OK)
  {
    g_print ("%s\n", PQcmdStatus (res1));
    PQclear (res1);
    g_print ("%d: SELECT FROM command failed\n", __LINE__);
    res1 = PQexec (conn, "ROLLBACK TRANSACTION");
    PQclear (res1);
    g_string_free (buf, TRUE);
    return;
  }

  for (i = 0; i < PQntuples (res1); i++)
  {
    GSM_SMSMessage sms;
    
    sms.MessageCenter.No = 1;
    sms.Type = SMS_Submit;
    sms.DCS.Type = SMS_GeneralDataCoding;
    sms.DCS.u.General.Compressed = false;
    sms.DCS.u.General.Alphabet = SMS_DefaultAlphabet;
    sms.DCS.u.General.Class = 0;
    sms.Validity.VPF = SMS_RelativeFormat;
    sms.Validity.u.Relative = 4320; /* 4320 minutes == 72 hours */
    sms.UDH_No = 0;
    sms.Report = false;

    strncpy (sms.RemoteNumber.number, PQgetvalue (res1, i, 1), GSM_MAX_DESTINATION_LENGTH + 1);
    sms.RemoteNumber.number[GSM_MAX_DESTINATION_LENGTH] = '\0';
    if (sms.RemoteNumber.number[0] == '+')
      sms.RemoteNumber.type = SMS_International;
    else
      sms.RemoteNumber.type = SMS_Unknown;
    
    strncpy (sms.UserData[0].u.Text, PQgetvalue (res1, i, 2), GSM_MAX_SMS_LENGTH + 1);
    sms.UserData[0].u.Text[GSM_MAX_SMS_LENGTH] = '\0';

#ifdef XDEBUG
    g_print ("%s, %s\n", sms.RemoteNumber.number, sms.UserData[0].u.Text);
#endif
    
    if (WriteSMS (&sms) != 0)
    {
      g_string_sprintf (buf, "UPDATE outbox SET processed='t' WHERE id='%s'",
                        PQgetvalue (res1, i, 0));
      res2 = PQexec(conn, buf->str);
      if (!res2 || PQresultStatus (res2) != PGRES_COMMAND_OK)
      {
        g_print ("%s\n", PQcmdStatus (res2));
        g_print ("%d: UPDATE command failed\n", __LINE__);
      }
      PQclear (res2);
    }
  }

  PQclear (res1);

  g_string_sprintf (buf, "COMMIT");
  res1 = PQexec(conn, buf->str);

  g_string_free(buf, TRUE);
  PQclear (res1);
}
