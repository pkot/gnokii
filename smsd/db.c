/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  Last modification: Sun Dec 17 2000
  Modified by Jan Derfinak

*/

#include <string.h>
#include <glib.h>
#include <libpq-fe.h>
#include "db.h"
#include "smsd.h"
#include "gsm-common.h"

static PGconn *connIn = NULL;
static PGconn *connOut = NULL;

void DB_Bye (void)
{
  if (connIn)
    PQfinish (connIn);
  
  if (connOut)
    PQfinish (connOut);
}


gint DB_ConnectInbox (const gchar * const conninfo)
{
  connIn = PQconnectdb (conninfo);
  
  if (PQstatus (connIn) == CONNECTION_BAD)
  {
     g_print ("Connection to database '%s' failed.\n", conninfo);
     g_print ("%s", PQerrorMessage(connIn));
     return (1);
  }

  return (0);
}


gint DB_ConnectOutbox (const gchar * const conninfo)
{
  connOut = PQconnectdb (conninfo);
  
  if (PQstatus (connOut) == CONNECTION_BAD)
  {
     g_print ("Connection to database '%s' failed.\n", conninfo);
     g_print ("%s", PQerrorMessage(connOut));
     return (1);
  }

  return (0);
}


gint DB_InsertSMS (const GSM_SMSMessage * const data)
{
  GString *buf;
  PGresult *res;
    
  buf = g_string_sized_new (128);
  g_string_sprintf (buf, "INSERT INTO inbox VALUES ('%s',
                    '%02d-%02d-%02d %02d:%02d:%02d+01', 'now', '%s', 'f')",
                    data->RemoteNumber.number, data->Time.Year + 2000, data->Time.Month,
                    data->Time.Day, data->Time.Hour, data->Time.Minute,
                    data->Time.Second, data->UserData[0].u.Text);
  res = PQexec(connIn, buf->str);
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

  res1 = PQexec(connOut, buf->str);
  PQclear (res1);

  g_string_sprintf (buf, "SELECT id, number, text FROM outbox \
                          WHERE processed='f' FOR UPDATE");

  res1 = PQexec(connOut, buf->str);
  if (!res1 || PQresultStatus (res1) != PGRES_TUPLES_OK)
  {
    g_print ("%s\n", PQcmdStatus (res1));
    PQclear (res1);
    g_print ("%d: SELECT FROM command failed\n", __LINE__);
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
    sms.Report = false;

    strncpy (sms.RemoteNumber.number, PQgetvalue (res1, i, 1), GSM_MAX_DESTINATION_LENGTH + 1);
    sms.RemoteNumber.number[GSM_MAX_DESTINATION_LENGTH] = '\0';
    if (sms.RemoteNumber.number[0] == '+') sms.RemoteNumber.type = SMS_International;
    else sms.RemoteNumber.type = SMS_Unknown;
    
    strncpy (sms.UserData[0].u.Text, PQgetvalue (res1, i, 2), GSM_MAX_SMS_LENGTH + 1);
    sms.UserData[0].u.Text[GSM_MAX_SMS_LENGTH] = '\0';

#ifdef XDEBUG
    g_print ("%s, %s\n", sms.RemoteNumber.Number, sms.UserData[0].u.Text);
#endif
    
    if (WriteSMS (&sms) != 0)
    {
      g_string_sprintf (buf, "UPDATE outbox SET processed='t' WHERE id='%s'",
                        PQgetvalue (res1, i, 0));
      res2 = PQexec(connOut, buf->str);
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
  res1 = PQexec(connOut, buf->str);

  g_string_free(buf, TRUE);
  PQclear (res1);
}
