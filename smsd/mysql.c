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
#include <mysql.h>
#include "smsd.h"
#include "gsm-sms.h"

static MYSQL *mysqlIn, *sockIn = NULL;
static MYSQL *mysqlOut, *sockOut = NULL;

void DB_Bye (void)
{
  if (sockIn)
    mysql_close (sockIn);

  if (sockOut)
    mysql_close (sockOut);
}


gint DB_ConnectInbox (DBConfig connect)
{
  mysql_init (mysqlIn);
  sockIn = mysql_connect (mysqlIn, connect.host, connect.user, connect.password);

  if (!sockIn)
  {
     g_print (_("Connection to host '%s' failed.\n"), connect.host);
     return (1);
  }

  if (mysql_select_db (sockIn, connect.db))
  {
    g_print (_("Couldn't select database %s!\n"), connect.db);
    g_print (_("Error: %s\n"), mysql_error (sockIn));
    mysql_close (sockIn);
    return (1);
  }

  return (0);
}


gint DB_ConnectOutbox (DBConfig connect)
{
  mysql_init (mysqlOut);
  sockOut = mysql_connect (mysqlOut, connect.host, connect.user, connect.password);

  if (!sockOut)
  {
     g_print (_("Connection to host '%s' failed.\n"), connect.host);
     return (1);
  }

  if (mysql_select_db (sockOut, connect.db))
  {
    g_print (_("Couldn't select database %s!\n"), connect.db);
    g_print (_("Error: %s\n"), mysql_error (sockOut));
    mysql_close (sockOut);
    return (1);
  }

  return (0);
}


gint DB_InsertSMS (const GSM_SMSMessage * const data)
{
  GString *buf;
  gchar *text;
    
  text = strEscape (data->UserData[0].u.Text);
  
  buf = g_string_sized_new (256);
  g_string_sprintf (buf, "INSERT INTO inbox (number, smsdate, insertdate, \
                    text, processed) VALUES ('%s', \
                    '%02d-%02d-%02d %02d:%02d:%02d+01', 'now', '%s', 'f')",
                    data->RemoteNumber.number, data->Time.Year, data->Time.Month,
                    data->Time.Day, data->Time.Hour, data->Time.Minute,
                    data->Time.Second, text);
  g_free (text);
  
  if (mysql_query (sockIn, buf->str))
  {
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
  gint numError,error;
  gchar status;

  buf = g_string_sized_new (128);

  g_string_sprintf (buf, "SELECT id, number, text, error FROM outbox \
                          WHERE processed='f' AND error < 5");

  if (mysql_query (sockOut, buf->str))
  {
    g_print ("%s\n", mysql_error (sockOut));
    g_print ("%d: SELECT FROM command failed\n", __LINE__);
    g_string_free (buf, TRUE);
    return;
  }

  if (!(res1 = mysql_store_result (sockOut)))
  {
    g_print ("%s\n", mysql_error(sockOut));
    g_print ("%d: Store Mysql Result Failed\n", __LINE__);
    g_string_free (buf, TRUE);
    return;
  }
  
  while ((row = mysql_fetch_row (res1)))
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
    sms.Report = (smsdConfig.smsSets = SMSD_READ_REPORTS);
    numError = atoi (row[3]);

    strncpy (sms.RemoteNumber.number, row[1], GSM_MAX_DESTINATION_LENGTH + 1);
    sms.RemoteNumber.number[GSM_MAX_DESTINATION_LENGTH] = '\0';
    if (sms.RemoteNumber.number[0] == '+')
      sms.RemoteNumber.type = SMS_International;
    else
      sms.RemoteNumber.type = SMS_Unknown;
    
    strncpy (sms.UserData[0].u.Text, row[2], GSM_MAX_SMS_LENGTH + 1);
    sms.UserData[0].u.Text[GSM_MAX_SMS_LENGTH] = '\0';

#ifdef XDEBUG
    g_print ("%s, %s\n", sms.RemoteNumber.number, sms.UserData[0].u.Text);
#endif
    
    status = 't';
    error = WriteSMS (&sms);
    if (error == 4 || error == 19)
    {
      if (numError > 0)
        numError = error;
      else
      {
        numError++;
        status = 'f';
      }
    }
    else
      numError = 0;
      
    if (error != 0)
    {
      g_string_sprintf (buf, "UPDATE outbox SET processed='%c', error='%i' WHERE id='%s'",
                        status, error, row[0]);
                        
      if (mysql_query (sockOut, buf->str))
      {
        g_print ("%s\n", mysql_error (sockOut));
        g_print ("%d: UPDATE command failed\n", __LINE__);
      }
    }
  }

  mysql_free_result (res1);

  g_string_free(buf, TRUE);
}
