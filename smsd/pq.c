/*

  S M S D

  A Linux/Unix tool for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2011 Jan Derfinak
  
  This file is a module to smsd for PostgreSQL db server.

*/

#include "compat.h"
#include <glib.h>
#include <libpq-fe.h>
#include "smsd.h"
#include "gnokii.h"

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


static gint Connect (const DBConfig connect, PGconn **conn)
{
  *conn = PQsetdbLogin (connect.host[0] != '\0' ? connect.host : NULL,
                         NULL,
                         NULL,
                         NULL,
                         connect.db,
                         connect.user[0] != '\0' ? connect.user : NULL,
                         connect.password[0] != '\0' ? connect.password : NULL);
  
  if (PQstatus (*conn) == CONNECTION_BAD)
  {
    g_print (_("Connection to database '%s' on host '%s' failed.\n"),
             connect.db, connect.host);
    g_print (_("Error: %s\n"), PQerrorMessage (*conn));
    return (SMSD_NOK);
  }

  if (connect.clientEncoding[0] != '\0')
    if (PQsetClientEncoding (*conn, connect.clientEncoding))
    {
      g_print (_("Setting client charset '%s' for database '%s' on host '%s' failed.\n"),
               connect.clientEncoding, connect.db, connect.host);
      g_print (_("Error: %s\n"), PQerrorMessage (*conn));
    }
    
  if (schema == NULL)
    schema = g_strdup (connect.schema);

  return (SMSD_OK);
}


GNOKII_API gint DB_ConnectInbox (const DBConfig connect)
{
  return (Connect (connect, &connIn));
}


GNOKII_API gint DB_ConnectOutbox (const DBConfig connect)
{
  return (Connect (connect, &connOut));
}


GNOKII_API gint DB_InsertSMS (const gn_sms * const data, const gchar * const phone)
{
  GString *buf, *phnStr;
  gchar *text;
  PGresult *res;
  gint err;


  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_printf (phnStr, "'%s',", phone);
  }

  text = g_malloc (strlen ((gchar *)data->user_data[0].u.text) * 2 + 1);
  PQescapeStringConn (connIn, text, (gchar *) data->user_data[0].u.text,
                      strlen ((gchar *)data->user_data[0].u.text), &err);
  if (err)
  {
    g_print (_("%d: Escaping string failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), PQerrorMessage (connIn));
    g_string_free (phnStr, TRUE);
    g_free (text);
    return (SMSD_NOK);
  }
  
//  text = strEscape ((gchar *) data->user_data[0].u.text);
  buf = g_string_sized_new (256);
  
  if (data->udh.udh[0].type == GN_SMS_UDH_ConcatenatedMessages)
  { // Multipart Message !
    gn_log_xdebug ("Multipart message\n");
    /* Check for duplicates */
    g_string_printf (buf, "SELECT count(id) FROM %s.multipartinbox \
                           WHERE number = '%s' AND text = '%s' AND \
                           refnum = %d AND maxnum = %d AND curnum = %d AND \
                           smsdate = '%04d-%02d-%02d %02d:%02d:%02d'",
                     schema, data->remote.number, text,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number,
                     data->udh.udh[0].u.concatenated_short_message.current_number,
                     data->smsc_time.year, data->smsc_time.month,
                     data->smsc_time.day, data->smsc_time.hour,
                     data->smsc_time.minute, data->smsc_time.second);
    res = PQexec (connIn, buf->str);
    if (!res || PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      g_print (_("%d: SELECT FROM %s.multipart command failed.\n"), __LINE__, schema);
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), PQerrorMessage (connIn));
      PQclear (res);
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);
    }
                                                       
    if (atoi (PQgetvalue (res, 0, 0)) > 0)
    {
      gn_log_xdebug ("%d: SMS already stored in the database (refnum=%d, maxnum=%d, curnum=%d).\n", __LINE__,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number,
                     data->udh.udh[0].u.concatenated_short_message.current_number);
      PQclear (res);
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_DUPLICATE);
    }

    PQclear (res);
    
    /* insert into multipart */
    g_string_printf (buf, "INSERT INTO %s.multipartinbox (number, smsdate, \
                           text, refnum , maxnum , curnum, %s processed) VALUES ('%s', \
                           '%04d-%02d-%02d %02d:%02d:%02d', '%s', %d, %d, %d, %s '0')",
                     schema,
                     phone[0] != '\0' ? "phone," : "", data->remote.number,
                     data->smsc_time.year, data->smsc_time.month,
                     data->smsc_time.day, data->smsc_time.hour,
                     data->smsc_time.minute, data->smsc_time.second, text,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number,
                     data->udh.udh[0].u.concatenated_short_message.current_number, phnStr->str);
    res = PQexec (connIn, buf->str);
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      g_print (_("%d: INSERT INTO %s.multipartinbox failed.\n"), __LINE__, schema);
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), PQerrorMessage (connIn));
      PQclear (res);
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);   
    }

    PQclear (res);
    
    /* If all parts are already in multipart inbox, move it into inbox */
    g_string_printf (buf, "SELECT text FROM %s.multipartinbox \
                           WHERE number='%s' AND refnum=%d AND maxnum=%d \
                           ORDER BY curnum",
                     schema, data->remote.number,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number);
    res = PQexec (connIn, buf->str);
    if (!res || PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      g_print (_("%d: SELECT FROM %s.multipart command failed.\n"), __LINE__, schema);
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), PQerrorMessage (connIn));
      PQclear (res);
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);
    }

    gn_log_xdebug ("maxnumber: %d - count: %d\n", data->udh.udh[0].u.concatenated_short_message.maximum_number,  PQntuples (res));
    if (PQntuples (res) == data->udh.udh[0].u.concatenated_short_message.maximum_number ) /* all parts collected */
    {
      gint i;
      GString *mbuf = g_string_sized_new (256);

      for (i = 0; i < PQntuples (res); i++)
        g_string_append (mbuf, PQgetvalue (res, i, 0));
        
      PQclear (res);
 
      g_string_printf(buf, "DELETE from %s.multipartinbox \
                            WHERE number='%s' AND refnum=%d AND maxnum=%d",
                      schema, data->remote.number,
                      data->udh.udh[0].u.concatenated_short_message.reference_number,
                      data->udh.udh[0].u.concatenated_short_message.maximum_number);
      res = PQexec (connIn, buf->str);
      if (!res || PQresultStatus (res) != PGRES_COMMAND_OK)
      {
        g_print (_("%d: DELETE FROM %s.multipartinbox command failed.\n"), __LINE__, schema);   
        gn_log_xdebug ("%s\n", buf->str);
        g_print (_("Error: %s\n"), PQerrorMessage (connIn));
        PQclear (res);
        g_string_free (buf, TRUE);
        g_string_free (mbuf, TRUE);
        g_string_free (phnStr, TRUE);
        g_free (text);
        return (SMSD_NOK);
      }
    
      PQclear (res);
      g_free (text);

      text = g_malloc (strlen ((gchar *)data->user_data[0].u.text) * 2 + 1);
      PQescapeStringConn (connIn, text, mbuf->str, mbuf->len, &err);
      if (err)
      {
        g_print (_("%d: Escaping string failed.\n"), __LINE__);
        g_print (_("Error: %s\n"), PQerrorMessage (connIn));
        g_string_free (buf, TRUE);
        g_string_free (mbuf, TRUE);
        g_string_free (phnStr, TRUE);
        g_free (text);
        return (SMSD_NOK);
      }

//      text = strEscape (mbuf->str);
      g_string_free (mbuf, TRUE);
    } 
    else
    {
      PQclear (res);
      gn_log_xdebug ("Not whole message collected.\n");
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_WAITING);
    }
  }
  
  gn_log_xdebug ("Message: %s\n", text);
  
  /* Detect duplicates */
  g_string_printf (buf, "SELECT count(id) FROM %s.inbox \
                         WHERE number = '%s' AND text = '%s' AND \
                         smsdate = '%04d-%02d-%02d %02d:%02d:%02d'",
		   schema,
                   data->remote.number, text, data->smsc_time.year,
                   data->smsc_time.month, data->smsc_time.day,
                   data->smsc_time.hour, data->smsc_time.minute,
                   data->smsc_time.second);
  res = PQexec (connIn, buf->str);
  if (!res || PQresultStatus (res) != PGRES_TUPLES_OK)
  {
    g_print (_("%d: SELECT FROM %s.inbox command failed.\n"), __LINE__, schema);
    gn_log_xdebug ("%s\n", buf->str);
    g_print (_("Error: %s\n"), PQerrorMessage (connIn));
    PQclear (res);
    g_string_free (buf, TRUE);
    g_string_free (phnStr, TRUE);
    g_free (text);
    return (SMSD_NOK);
  }
  
  if (atoi (PQgetvalue (res, 0, 0)) > 0)
  {
    gn_log_xdebug ("%d: MSG already stored in database.\n", __LINE__);
    PQclear (res);
    g_string_free (buf, TRUE);
    g_string_free (phnStr, TRUE);
    g_free (text);
    return (SMSD_DUPLICATE);
  }
  
  PQclear (res);

  g_string_printf (buf, "INSERT INTO %s.inbox (\"number\", \"smsdate\", \"insertdate\",\
                         \"text\", %s \"processed\") VALUES ('%s', \
                         '%04d-%02d-%02d %02d:%02d:%02d', 'now', '%s', %s 'f')",
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
    gn_log_xdebug ("%s\n", buf->str);
    g_print (_("Error: %s\n"), PQerrorMessage (connIn));
    PQclear (res);
    return (SMSD_NOK);
  }
  
  PQclear (res);
    
  return (SMSD_OK);
}


GNOKII_API gint DB_Look (const gchar * const phone)
{
  GString *buf, *phnStr;
  PGresult *res1, *res2;
  register int i;
  gint numError, error;
  gint empty = 1;

  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_printf (phnStr, "AND phone = '%s'", phone);
  }

  buf = g_string_sized_new (256);

  res1 = PQexec (connOut, "BEGIN");
  PQclear (res1);

  g_string_printf (buf, "SELECT id, number, text, dreport FROM %s.outbox \
                         WHERE processed='f' AND localtime(0) >= not_before \
                         AND localtime(0) <= not_after %s LIMIT 1 FOR UPDATE",
                   schema, phnStr->str);
  g_string_free (phnStr, TRUE);

  res1 = PQexec (connOut, buf->str);
  if (!res1 || PQresultStatus (res1) != PGRES_TUPLES_OK)
  {
    g_print (_("%d: SELECT FROM %s.outbox command failed.\n"), __LINE__, schema);
    gn_log_xdebug ("%s\n", buf->str);
    g_print (_("Error: %s\n"), PQerrorMessage (connOut));
    PQclear (res1);
    res1 = PQexec (connOut, "ROLLBACK TRANSACTION");
    PQclear (res1);
    g_string_free (buf, TRUE);
    return (SMSD_NOK);
  }

  for (i = 0; i < PQntuples (res1); i++)
  {
    gn_sms sms;
    
    empty = 0;
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
      if ((error = WriteSMS (&sms)) == GN_ERR_NONE)
        break;
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
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), PQerrorMessage (connOut));
    }

    PQclear (res2);
  }

  PQclear (res1);
  res1 = PQexec (connOut, "COMMIT");
  PQclear (res1);

  g_string_free (buf, TRUE);
  
  if (empty)
    return (SMSD_OUTBOXEMPTY);
  else
    return (SMSD_OK);
}
