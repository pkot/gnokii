/*

  S M S D

  A Linux/Unix tool for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  Copyright (C) 1999-2011 Jan Derfinak

  This file is a module to smsd for MySQL db server.
  
*/

#include "compat.h"
#include <glib.h>
#include <mysql.h>
#include "smsd.h"
#include "gnokii.h"

static MYSQL mysqlIn;
static MYSQL mysqlOut;

GNOKII_API void DB_Bye (void)
{
  mysql_close (&mysqlIn);
  mysql_close (&mysqlOut);
}


static gint Connect (const DBConfig connect, MYSQL *mysql)
{
#if MYSQL_VERSION_ID >= 50013
  my_bool reconnect = 1;
#endif

  mysql_init (mysql);
  
  if (connect.clientEncoding[0] != '\0')
    mysql_options (mysql, MYSQL_SET_CHARSET_NAME, connect.clientEncoding);
#if MYSQL_VERSION_ID >= 50500
  else
    mysql_options (mysql, MYSQL_SET_CHARSET_NAME, MYSQL_AUTODETECT_CHARSET_NAME);
#endif

#if MYSQL_VERSION_ID >= 50013
  mysql_options (mysql, MYSQL_OPT_RECONNECT, &reconnect);
#endif

  if (!mysql_real_connect (mysql,
                           connect.host[0] != '\0' ? connect.host : NULL,
                           connect.user[0] != '\0' ? connect.user : NULL,
                           connect.password[0] != '\0' ? connect.password : NULL,
                           connect.db, 0, NULL, 0))
  {
     g_print (_("Connection to database '%s' on host '%s' failed.\n"),
              connect.db, connect.host);
     g_print (_("Error: %s\n"), mysql_error (mysql));
     return (SMSD_NOK);
  }

  return (SMSD_OK);
}


GNOKII_API gint DB_ConnectInbox (const DBConfig connect)
{
  return (Connect (connect, &mysqlIn));
}


GNOKII_API gint DB_ConnectOutbox (const DBConfig connect)
{
  return (Connect (connect, &mysqlOut));
}


GNOKII_API gint DB_InsertSMS (const gn_sms * const data, const gchar * const phone)
{
  GString *buf, *phnStr;
  gchar *text;
  MYSQL_RES *res;
  MYSQL_ROW row;

  if (phone[0] == '\0')
    phnStr = g_string_new ("");
  else
  {
    phnStr = g_string_sized_new (32);
    g_string_printf (phnStr, "'%s',", phone);
  }

  text = g_malloc (strlen ((gchar *)data->user_data[0].u.text) * 2 + 1);
  mysql_real_escape_string (&mysqlIn, text, data->user_data[0].u.text, strlen ((gchar *)data->user_data[0].u.text));
  buf = g_string_sized_new (256);
  
  if (data->udh.udh[0].type == GN_SMS_UDH_ConcatenatedMessages)
  { // Multipart Message !
    gn_log_xdebug ("Multipart message\n");
    /* Check for duplicates */
    g_string_printf (buf, "SELECT count(id) FROM multipartinbox \
                           WHERE number = '%s' AND text = '%s' AND \
                           refnum = %d AND maxnum = %d AND curnum = %d AND \
                           smsdate = '%04d-%02d-%02d %02d:%02d:%02d'",
                     data->remote.number, text, data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number,
                     data->udh.udh[0].u.concatenated_short_message.current_number,
                     data->smsc_time.year, data->smsc_time.month,
                     data->smsc_time.day, data->smsc_time.hour,
                     data->smsc_time.minute, data->smsc_time.second);
    if (mysql_real_query (&mysqlIn, buf->str, buf->len))
    {
      g_print (_("%d: SELECT FROM multipart command failed.\n"), __LINE__);
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);
    }
    
    res = mysql_store_result (&mysqlIn);
    row = mysql_fetch_row (res);
    mysql_free_result (res);
    if (atoi (row[0]) > 0)
    {
      gn_log_xdebug ("%d: SMS already stored in the database (refnum=%i, maxnum=%i, curnum=%i).\n", __LINE__,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number,
                     data->udh.udh[0].u.concatenated_short_message.current_number);
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_DUPLICATE);
    }

    /* insert into multipart */
    g_string_printf (buf, "INSERT INTO multipartinbox (number, smsdate, \
                           text, refnum , maxnum , curnum, %s processed) VALUES ('%s', \
                           '%04d-%02d-%02d %02d:%02d:%02d', '%s', %d, %d, %d, %s '0')",
                     phone[0] != '\0' ? "phone," : "", data->remote.number,
                     data->smsc_time.year, data->smsc_time.month,
                     data->smsc_time.day, data->smsc_time.hour,
                     data->smsc_time.minute, data->smsc_time.second, text,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number,
                     data->udh.udh[0].u.concatenated_short_message.current_number, phnStr->str);
    if (mysql_real_query (&mysqlIn, buf->str, buf->len))
    {
      g_print (_("%d: INSERT INTO multipartinbox command failed.\n"), __LINE__);
      gn_log_xdebug ("%s\n", buf->str);      
      g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);
    }

    /* If all parts are already in multipart inbox, move it into inbox */
    g_string_printf (buf, "SELECT text FROM multipartinbox \
                           WHERE number='%s' AND refnum=%d AND maxnum=%d \
                           ORDER BY curnum",
                     data->remote.number,
                     data->udh.udh[0].u.concatenated_short_message.reference_number,
                     data->udh.udh[0].u.concatenated_short_message.maximum_number);
    if (mysql_real_query (&mysqlOut, buf->str, buf->len))
    {
      g_print (_("%d: SELECT FROM multipartinbox command failed.\n"), __LINE__);
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);
    }

    if (!(res = mysql_store_result (&mysqlOut)))
    {
      gn_log_xdebug ("%d: Store Mysql Result Failed.\n", __LINE__);
      gn_log_xdebug (_("Error: %s\n"), mysql_error (&mysqlOut));
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_NOK);
    }

    gn_log_xdebug ("maxnumber: %d - count: %d\n", data->udh.udh[0].u.concatenated_short_message.maximum_number, mysql_num_rows (res));
    if (mysql_num_rows (res) == data->udh.udh[0].u.concatenated_short_message.maximum_number ) /* all parts collected */
    {
      GString *mbuf = g_string_sized_new (256);
      
      while ((row = mysql_fetch_row (res)))
        g_string_append (mbuf, row[0]);

      mysql_free_result (res);
      
      g_string_printf(buf, "DELETE from multipartinbox \
                            WHERE number='%s' AND refnum=%d AND maxnum=%d",
                      data->remote.number,
                      data->udh.udh[0].u.concatenated_short_message.reference_number,
                      data->udh.udh[0].u.concatenated_short_message.maximum_number);
      if (mysql_real_query (&mysqlIn, buf->str, buf->len))
      {
        g_print (_("%d: DELETE FROM multipartinbox command failed.\n"), __LINE__);
        gn_log_xdebug ("%s\n", buf->str);
        g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
        g_string_free (buf, TRUE);
        g_string_free (mbuf, TRUE);
        g_string_free (phnStr, TRUE);
        g_free (text);
        return (SMSD_NOK);
      }
      
      g_free (text);
      text = g_malloc (mbuf->len * 2 + 1);
      mysql_real_escape_string (&mysqlIn, text, mbuf->str, mbuf->len);
      g_string_free (mbuf, TRUE);
    } 
    else
    {
      mysql_free_result (res);
      gn_log_xdebug ("Not whole message collected.\n");
      g_string_free (buf, TRUE);
      g_string_free (phnStr, TRUE);
      g_free (text);
      return (SMSD_WAITING);
    }
  }

  gn_log_xdebug ("Message: %s\n", text);
  
  /* Detect duplicates */
  g_string_printf (buf, "SELECT count(id) FROM inbox \
                         WHERE number = '%s' AND text = '%s' AND \
                         smsdate = '%04d-%02d-%02d %02d:%02d:%02d'",
		   data->remote.number, text, data->smsc_time.year,
                   data->smsc_time.month, data->smsc_time.day,
                   data->smsc_time.hour, data->smsc_time.minute,
                   data->smsc_time.second);
  if (mysql_real_query (&mysqlIn, buf->str, buf->len))
  {
    g_print (_("%d: SELECT FROM inbox command failed.\n"), __LINE__);
    gn_log_xdebug ("%s\n", buf->str);
    g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
    g_string_free (buf, TRUE);
    g_string_free (phnStr, TRUE);
    g_free (text);
    return (SMSD_NOK);
  }
  
  res = mysql_store_result (&mysqlIn);
  row = mysql_fetch_row (res);
  mysql_free_result (res);
  if (atoi (row[0]) > 0)
  {
    gn_log_xdebug ("%d: MSG already stored in database.\n", __LINE__);
    g_string_free (buf, TRUE);
    g_string_free (phnStr, TRUE);
    g_free (text);
    return (SMSD_DUPLICATE);
  }

  g_string_printf (buf, "INSERT INTO inbox (number, smsdate, \
                          text, %s processed) VALUES ('%s', \
                          '%04d-%02d-%02d %02d:%02d:%02d', '%s', %s '0')",
                   phone[0] != '\0' ? "phone," : "", data->remote.number,
                   data->smsc_time.year, data->smsc_time.month,
                   data->smsc_time.day, data->smsc_time.hour,
                   data->smsc_time.minute, data->smsc_time.second, text, phnStr->str);
  g_free (text);
  g_string_free (phnStr, TRUE);

  if (mysql_real_query (&mysqlIn, buf->str, buf->len))
  {
    g_print (_("%d: INSERT INTO inbox command failed.\n"), __LINE__);
    gn_log_xdebug ("%s\n", buf->str);
    g_print (_("Error: %s\n"), mysql_error (&mysqlIn));
    g_string_free (buf, TRUE);
    return (SMSD_NOK);
  }

  g_string_free (buf, TRUE);
  gn_log_xdebug ("Everything OK\n");
  return SMSD_OK;
}


GNOKII_API gint DB_Look (const gchar * const phone)
{
  GString *buf, *phnStr;
  MYSQL_RES *res1;
  MYSQL_ROW row;
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

  mysql_real_query (&mysqlOut, "BEGIN", strlen ("BEGIN"));
  
  g_string_printf (buf, "SELECT id, number, text, dreport FROM outbox \
                         WHERE processed='0' AND CURTIME() >= not_before \
                         AND CURTIME() <= not_after %s LIMIT 1 FOR UPDATE", phnStr->str);
  g_string_free (phnStr, TRUE);
  
  if (mysql_real_query (&mysqlOut, buf->str, buf->len))
  {
    g_print (_("%d: SELECT FROM outbox command failed.\n"), __LINE__);
    gn_log_xdebug ("%s\n", buf->str);
    g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
    mysql_real_query (&mysqlOut, "ROLLBACK TRANSACTION", strlen ("ROLLBACK TRANSACTION"));
    g_string_free (buf, TRUE);
    return (SMSD_NOK);
  }

  if (!(res1 = mysql_store_result (&mysqlOut)))
  {
    g_print (_("%d: Store Mysql Result Failed.\n"), __LINE__);
    g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
    mysql_real_query (&mysqlOut, "ROLLBACK TRANSACTION", strlen ("ROLLBACK TRANSACTION"));
    g_string_free (buf, TRUE);
    return (SMSD_NOK);
  }
  
  while ((row = mysql_fetch_row (res1)))
  {
    gn_sms sms;

    empty = 0;
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
      strncpy((gchar *)sms.user_data[0].u.text, row[2], 10 * GN_SMS_MAX_LENGTH + 1);
    else
      *sms.user_data[0].u.text = '\0';
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

    g_string_printf (buf, "UPDATE outbox SET processed='1', error='%d', \
                           processed_date=NULL WHERE id='%s'",
                     error, row[0]);
                        
    if (mysql_real_query (&mysqlOut, buf->str, buf->len))
    {
      g_print (_("%d: UPDATE command failed.\n"), __LINE__);
      gn_log_xdebug ("%s\n", buf->str);
      g_print (_("Error: %s\n"), mysql_error (&mysqlOut));
    }
  }

  mysql_free_result (res1);

  mysql_real_query (&mysqlOut, "COMMIT", strlen ("COMMIT"));
  
  g_string_free (buf, TRUE);
  
  if (empty)
    return (SMSD_OUTBOXEMPTY);
  else
    return (SMSD_OK);
}
