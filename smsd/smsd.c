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

  Copyright (C) 1999 Pavel Janik ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak

*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>

#ifndef WIN32
# include <unistd.h>  /* for usleep */
# include <signal.h>
#else
# include <windows.h>
# undef IN
# undef OUT
#endif

#include <glib.h>
#include <gmodule.h>

#include "misc.h"

#include "gnokii.h"
#include "smsd.h"
#include "lowlevel.h"
#include "db.h"

#ifdef ENABLE_NLS
#  include <locale.h>
#endif


/* Hold main configuration data for smsd */
SmsdConfig smsdConfig;

/* Global variables */
bool GTerminateThread;

/* Local variables */
static DBConfig connection;
void (*DB_Bye) (void) = NULL;
gint (*DB_ConnectInbox) (const DBConfig) = NULL;
gint (*DB_ConnectOutbox) (const DBConfig) = NULL;
gint (*DB_InsertSMS) (const gn_sms * const, const gchar * const) = NULL;
void (*DB_Look) (const gchar * const) = NULL;

static pthread_t db_monitor_th;
pthread_mutex_t db_monitorMutex;
static volatile bool db_monitor;


gint LoadDB (void)
{
  GModule *handle;
  gchar *buf;
  gchar *full_name;

  full_name = g_strdup_printf ("smsd_%s", smsdConfig.dbMod);
  buf = g_module_build_path (smsdConfig.libDir, full_name);
  g_free (full_name);
  
  gn_log_xdebug ("Trying to load module %s\n", buf);
    
  handle = g_module_open (buf, G_MODULE_BIND_LAZY);
  g_free (buf);
  if (!handle)
  {
    g_print ("g_module_open error: %s!\n", g_module_error ());
    return (1);
  }
    
  if (g_module_symbol (handle, "DB_Bye", (gpointer *)&DB_Bye) == FALSE)
  {
    g_print ("Error getting symbol 'DB_Bye': %s\n", g_module_error ());
    return (2);
  }

  if (g_module_symbol (handle, "DB_ConnectInbox", (gpointer *)&DB_ConnectInbox) == FALSE)
  {
    g_print ("Error getting symbol 'DB_ConnectInbox':  %s\n", g_module_error ());
    return (2);
  }

  if (g_module_symbol (handle, "DB_ConnectOutbox", (gpointer *)&DB_ConnectOutbox) == FALSE)
  {
    g_print ("Error getting symbol 'DB_ConnectOutbox': %s\n", g_module_error ());
    return (2);
  }

  if (g_module_symbol (handle, "DB_InsertSMS", (gpointer *)&DB_InsertSMS) == FALSE)
  {
    g_print ("Error getting symbol 'DB_InsertSMS': %s\n", g_module_error ());
    return (2);
  }

  if (g_module_symbol (handle, "DB_Look", (gpointer *)&DB_Look) == FALSE)
  {
    g_print ("Error getting symbol 'DB_Look': %s\n", g_module_error ());
    return (2);
  }

  return (0);
}


static void Short_Version (void)
{
  g_print (_("smsd - version %s from gnokii %s\n"), SVERSION, VERSION);
}

static void Version (void)
{
  g_print (_("smsd - version %s from gnokii %s\nCopyright  Jan Derfinak  <ja@mail.upjs.sk>\n"), SVERSION, VERSION);
}


static void Usage (gchar *p)
{
  Version ();
  g_print (_("\nUsage:  %s [options]\n"
             "            -u, --user db_username OR action if -m file\n"
             "            -p, --password db_password\n"
             "            -d, --db db_name\n"
             "            -c, --host db_hostname OR spool directory if -m file\n"
             "            -s, --schema db_schema\n"
             "            -m, --module db_module (pq, mysql, file)\n"
             "            -l, --libdir path_to_db_module\n"
             "            -f, --logfile file\n"
             "            -t, --phone phone_number\n"
             "            -i, --interval polling_interval_for_incoming_sms's_in_seconds\n"
             "            -S, --maxsms number_of_sms's (only in dumb mode)\n"
             "            -b, --inbox memoryType\n"
             "            -0, --firstpos0\n"
             "            -v, --version\n"
             "            -h, --help\n"), p);
}


static void LogFile (gchar *str, ...)
{
  FILE *f;
  va_list ap;
  time_t cas;
  gchar buf[50];
  
  if (smsdConfig.logFile == NULL)
    return;

  if (strcmp (smsdConfig.logFile, "-") == 0)
    f = stdout;
  else if ((f = fopen (smsdConfig.logFile, "a")) == NULL)
  {
    g_print (_("WARNING: Cannot open file %s for appending.\n"), smsdConfig.logFile);
    return;
  }

  cas = time (NULL);
  strftime (buf, 50, "%e %b %Y %T", localtime (&cas));
  fprintf (f, "%s: ", buf);

  va_start (ap, str);
  vfprintf (f, str, ap);
  va_end (ap);
  if (f == stdout)
    fflush (f);
  else
    fclose (f);
  return;
}


static void ReadConfig (gint argc, gchar *argv[])
{
  connection.user = g_strdup ("");
  connection.password = g_strdup ("");
  connection.db = g_strdup ("sms");
  connection.host = g_strdup ("");
  connection.schema = g_strdup ("public");
  smsdConfig.dbMod = g_strdup ("file");
  smsdConfig.libDir = g_strdup (MODULES_DIR);
  smsdConfig.logFile = NULL;
  smsdConfig.phone = g_strdup ("");
  smsdConfig.refreshInt = 1;     // Phone querying interval in seconds
  smsdConfig.maxSMS = 10;        // Smsd uses it if GetSMSStatus isn't implemented
  smsdConfig.firstSMS = 1;
//  smsdConfig.smsSets = 0;
  smsdConfig.memoryType = GN_MT_XX;

  while (1)
  {
    gint optionIndex = 0;
    gint c;
    static struct option longOptions[] =
    {
      {"user", 1, 0, 'u'},
      {"password", 1, 0, 'p'},
      {"db", 1, 0, 'd'},
      {"host", 1, 0, 'c'},
      {"schema", 1, 0, 's'},
      {"module", 1, 0, 'm'},
      {"libdir", 1, 0, 'l'},
      {"logfile", 1, 0, 'f'},
      {"phone", 1, 0, 't'},
      {"version", 0, 0, 'v'},
      {"interval", 1, 0, 'i'},
      {"maxsms", 1, 0, 'S'},
      {"inbox", 1, 0, 'b'},
      {"firstpos0", 0, 0, '0'},
      {"help", 0, 0, 'h'},
      {0, 0, 0, 0}
    };
    
    c = getopt_long (argc, argv, "u:p:d:c:s:m:l:f:t:vi:S:b:0h", longOptions, &optionIndex);
    if (c == EOF)
      break;
    switch (c)
    {
      case 'u':
        g_free (connection.user);
        connection.user = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
      
      case 'p':
        g_free (connection.password);
        connection.password = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'd':
        g_free (connection.db);
        connection.db = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'c':
        g_free (connection.host);
        connection.host = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
      
      case 's':
        g_free (connection.schema);
        connection.schema = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'm':
        g_free (smsdConfig.dbMod);
        smsdConfig.dbMod = g_strdup (optarg);
        break;
        
      case 'l':
        g_free (smsdConfig.libDir);
        smsdConfig.libDir = g_strdup (optarg);
        break;

      case 'f':
        if (smsdConfig.logFile)
          g_free (smsdConfig.logFile);
        smsdConfig.logFile = g_strdup (optarg);
        break;

      case 't':
        if (smsdConfig.phone)
          g_free (smsdConfig.phone);
        smsdConfig.phone = g_strdup (optarg);
        break;

      case 'i':
        smsdConfig.refreshInt = atoi (optarg);
        break;

      case 'S':
        smsdConfig.maxSMS = atoi (optarg);
        break;

      case 'b':
        smsdConfig.memoryType = gn_str2memory_type (optarg);
        break;

      case '0':
        smsdConfig.firstSMS = 0;
        break;
        
      case 'v':
        Version ();
        exit (0);
        break;

      case 'h':
      case '?':
        Usage (argv[0]);
        exit (1);
        break;

      default:
        g_print ("getopt returned 0%o\n", c);
    }
  }
  
  if ((argc - optind) != 0)
  {
    g_print (_("Wrong number of arguments\n"));
    Usage (argv[0]);
    exit (1);
  }

  if (LoadDB ())
  {
    g_print (_("Cannot load database module %s in directory %s!\n"),
              smsdConfig.dbMod, smsdConfig.libDir);
    exit (-2);
  }
}


static void *SendSMS (void *a)
{
  if ((*DB_ConnectOutbox) (connection))
  {
    pthread_exit (0);
    return (NULL);
  }

  while (1)
  {
    pthread_mutex_lock (&db_monitorMutex);
    if (!db_monitor)
    {
      pthread_mutex_unlock (&db_monitorMutex);
      pthread_exit (0);
      return (NULL);
    }
    pthread_mutex_unlock (&db_monitorMutex);

    (*DB_Look) (smsdConfig.phone);

    sleep (3);
  }
}


static void MainExit (gint sig)
{
  PhoneEvent *e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));

  e->event = Event_Exit;
  e->data = NULL;
  InsertEvent (e);
  
  pthread_mutex_lock (&db_monitorMutex);
  db_monitor = FALSE;
  pthread_mutex_unlock (&db_monitorMutex);
  
  pthread_join (monitor_th, NULL);
  pthread_join (db_monitor_th, NULL);
  (*DB_Bye) ();
  exit (sig);
}


GNOKII_API gint WriteSMS (gn_sms *sms)
{
  gn_error error;
  PhoneEvent *e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
  D_SMSMessage *m = (D_SMSMessage *) g_malloc (sizeof (D_SMSMessage));

  m->sms = sms;
  e->event = Event_SendSMSMessage;
  e->data = m;
  InsertEvent (e);
  pthread_mutex_lock (&sendSMSMutex);
  pthread_cond_wait (&sendSMSCond, &sendSMSMutex);
  pthread_mutex_unlock (&sendSMSMutex);

  gn_log_xdebug ("Address: %s\nText: %s\n", sms->remote.number, sms->user_data[0].u.text);

  error = m->status;
  g_free (m);

  if (error)
  {
    gn_log_xdebug ("Sending to %s unsuccessful. Error: %s\n", sms->remote.number, gn_error_print(error));
    if (smsdConfig.logFile)
      LogFile (_("Sending to %s unsuccessful. Error: %s\n"), sms->remote.number, gn_error_print(error));
  }
   else
  {
    gn_log_xdebug ("Sending to %s successful.\n", sms->remote.number);
    if (smsdConfig.logFile)
      LogFile (_("Sending to %s successful.\n"), sms->remote.number);
  }

  return (error);
}


static void ReadSMS (gpointer d, gpointer userData)
{
  gn_sms *data = (gn_sms *)d;
  PhoneEvent *e;
  gint error;

  if (data->type == GN_SMS_MT_Deliver || data->type == GN_SMS_MT_StatusReport)
  {
    gn_log_xdebug ("%d. %s   ", data->number, data->remote.number);
    gn_log_xdebug ("%02d-%02d-%02d %02d:%02d:%02d+%02d %s\n", data->smsc_time.year,
                    data->smsc_time.month, data->smsc_time.day, data->smsc_time.hour,
                    data->smsc_time.minute, data->smsc_time.second, data->smsc_time.timezone,
                    data->user_data[0].u.text);
    error = (*DB_InsertSMS) (data, smsdConfig.phone);

    switch (error) 
    {
      case SMSD_OK:
        if (smsdConfig.logFile)
          LogFile (_("Inserting sms from %s successful.\n"), data->remote.number);
        gn_log_xdebug ("Inserting sms from %s successful.\n", data->remote.number);
        e = (PhoneEvent *) g_malloc (sizeof(PhoneEvent));
        e->event = Event_DeleteSMSMessage;
        e->data = data;
        InsertEvent (e);
        break;
        
      case SMSD_DUPLICATE:
        if (smsdConfig.logFile)
          LogFile (_("Duplicated sms from %s.\n"), data->remote.number);
        gn_log_xdebug ("Duplicated sms from %s.\n", data->remote.number);
        e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
        e->event = Event_DeleteSMSMessage;
        e->data = data;
        InsertEvent (e);
        break;

      case SMSD_WAITING:
        if (smsdConfig.logFile)
          LogFile (_("Multipart sms from %s. Waiting for the next parts.\n"), data->remote.number);
        gn_log_xdebug ("Multipart sms from %s. Waiting for the next parts.\n", data->remote.number);
        e = (PhoneEvent *) g_malloc (sizeof(PhoneEvent));
        e->event = Event_DeleteSMSMessage;
        e->data = data;
        InsertEvent (e);
        break;
        
      default:
        if (smsdConfig.logFile)
          LogFile (_("Inserting sms from %s unsuccessful.\nDate: %02d-%02d-%02d %02d:%02d:%02d+%02d\nText: %s\n\nExiting.\n"),
                   data->remote.number, data->smsc_time.year,
                   data->smsc_time.month, data->smsc_time.day,
                   data->smsc_time.hour, data->smsc_time.minute,
                   data->smsc_time.second, data->smsc_time.timezone,
                   data->user_data[0].u.text);
        gn_log_xdebug ("Inserting sms from %s unsuccessful.\nDate: %02d-%02d-%02d %02d:%02d:%02d+%02d\nText: %s\n\nExiting.\n",
                        data->remote.number, data->smsc_time.year,
                        data->smsc_time.month, data->smsc_time.day,
                        data->smsc_time.hour, data->smsc_time.minute,
                        data->smsc_time.second, data->smsc_time.timezone,
                        data->user_data[0].u.text);
        break;
    }
  }
}


static void GetSMS (void)
{
  while (1)
  {
    pthread_mutex_lock (&smsMutex);
    // Waiting for signal
    pthread_cond_wait (&smsCond, &smsMutex);

    // Signal arrived.
    pthread_mutex_unlock (&smsMutex);

    // Check if Mutex is free
    pthread_mutex_lock (&smsMutex);
    phoneMonitor.sms.messages = g_slist_reverse(phoneMonitor.sms.messages);
    g_slist_foreach (phoneMonitor.sms.messages, ReadSMS, (gpointer) NULL);
    pthread_mutex_unlock (&smsMutex);
  }
}


static void Run (void)
{
    /* Windows doesn't support signaling */
#ifndef WIN32
  struct sigaction act;
  
  act.sa_flags = 0;
  act.sa_handler = MainExit;
  sigemptyset (&(act.sa_mask));
  
  sigaction (SIGQUIT, &act, NULL);
  sigaction (SIGTERM, &act, NULL);
  sigaction (SIGINT, &act, NULL);

#if __unices__
  act.sa_handler = SIG_IGN;
  sigemptyset (&(act.sa_mask));
  sigaction (SIGALRM, &act, NULL);
#endif
#endif

  InitPhoneMonitor ();
  if ((*DB_ConnectInbox) (connection))
    exit (2);
  pthread_create (&monitor_th, NULL, Connect, smsdConfig.phone);
  db_monitor = TRUE;
  pthread_mutex_init (&db_monitorMutex, NULL);
  pthread_create (&db_monitor_th, NULL, SendSMS, NULL);
  GetSMS ();
}


int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  textdomain(GETTEXT_PACKAGE);
#endif

  gn_elog_handler = NULL;
  ReadConfig (argc, argv);
  /* Introduce yourself */
  Short_Version ();
  GTerminateThread = false;
  Run ();

  return(0);
}
