/*

  PQ.C

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.
  
  This file is a module to smsd for PostgreSQL db server.

  $Id$
  
*/

#include <string.h>
#include <glib.h>
#include <libpq-fe.h>
#include "smsd.h"
#include "gsm-sms.h"

static PGconn *connIn = NULL;
static PGconn *connOut = NULL;

void DB_Bye (void)
{
  if (connIn)
    PQfinish (connIn);

  if (connOut)
    PQfinish (connOut);
}


gint DB_ConnectInbox (DBConfig connect)
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

  return (0);
}


gint DB_ConnectOutbox (DBConfig connect)
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
  
  res = PQexec(connIn, buf->str);
  g_string_free(buf, TRUE);
  if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    g_print (_("%d: INSERT INTO inbox failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), PQerrorMessage (connIn));
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
  gint numError, error;

  buf = g_string_sized_new (128);

  res1 = PQexec (connOut, "BEGIN");
  PQclear (res1);

  g_string_sprintf (buf, "SELECT id, number, text FROM outbox \
                          WHERE processed='f' FOR UPDATE");

  res1 = PQexec (connOut, buf->str);
  if (!res1 || PQresultStatus (res1) != PGRES_TUPLES_OK)
  {
    g_print (_("%d: SELECT FROM outbox command failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), PQerrorMessage (connOut));
    PQclear (res1);
    res1 = PQexec (connOut, "ROLLBACK TRANSACTION");
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
    sms.Report = (smsdConfig.smsSets & SMSD_READ_REPORTS);

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
    
    numError = 0;
    do
    {
      error = WriteSMS (&sms);
      sleep (2);
    }
    while ((error == GE_TIMEOUT || error == GE_SMSSENDFAILED) && numError++ < 3);

    g_string_sprintf (buf, "UPDATE outbox SET processed='t', error='%d', \
                            processed_date='now' WHERE id='%s'",
                      error, PQgetvalue (res1, i, 0));

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
