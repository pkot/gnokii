/*

  S M S D

  A Linux/Unix GUI for Nokia mobile phones.
  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & Ján Derfiòák <ja@mail.upjs.sk>.

  Released under the terms of the GNU GPL, see file COPYING for more details.

  $Id$
*/

#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include <dlfcn.h>

#ifndef WIN32
# include <unistd.h>  /* for usleep */
# include <signal.h>
#else
# include <windows.h>
# include "../win32/winserial.h"
# define WRITEPHONE(a, b, c) WriteCommBlock(b, c)
# undef IN
# undef OUT
# define sleep(x) Sleep((x) * 1000)
# define usleep(x) Sleep(((x) < 1000) ? 1 : ((x) / 1000))
#endif

#include <glib.h>

#include "misc.h"

#include "gsm-common.h"
#include "gsm-api.h"
#include "phones/nk7110.h"
#include "phones/nk6510.h"
#include "phones/nk6100.h"
#include "phones/nk3110.h"
#include "phones/nk2110.h"
#include "cfgreader.h"
#include "smsd.h"
#include "lowlevel.h"
#include "db.h"


/* Hold main configuration data for smsd */
SmsdConfig smsdConfig;

/* Global variables */
bool TerminateThread;

/* Local variables */
static DBConfig connect;
void (*DB_Bye) (void) = NULL;;
gint (*DB_ConnectInbox) (const DBConfig) = NULL;
gint (*DB_ConnectOutbox) (const DBConfig) = NULL;
gint (*DB_InsertSMS) (const GSM_SMSMessage * const) = NULL;
void (*DB_Look) (void) = NULL;

static pthread_t db_monitor_th;
pthread_mutex_t db_monitorMutex;
static volatile bool db_monitor;

/* Escapes ' and \ with \. */
/* Returned value needs to be free with g_free(). */
gchar *strEscape (const gchar *const s)
{
  GString *str = g_string_new (s);
  register gint i = 0;
  gchar *ret;
  
  while (str->str[i] != '\0')
  {
    if (str->str[i] == '\\' || str->str[i] == '\'')
      g_string_insert_c (str, i++, '\\');
    i++;
  }
  
  ret = str->str;
  g_string_free (str, FALSE);
  
  return (ret);
}


gint LoadDB (void)
{
  void *handle;
  GString *buf;
  gchar *error;
  
  buf = g_string_sized_new (64);
  
  g_string_sprintf (buf, "%s/lib%s.so", smsdConfig.libDir, smsdConfig.dbMod);
  
  handle = dlopen (buf->str, RTLD_LAZY);
  if (!handle)
    return (1);
    
  DB_Bye = dlsym(handle, "DB_Bye");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }

  DB_ConnectInbox = dlsym(handle, "DB_ConnectInbox");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }

  DB_ConnectOutbox = dlsym(handle, "DB_ConnectOutbox");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }
  
  DB_InsertSMS = dlsym(handle, "DB_InsertSMS");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }
  
  DB_Look = dlsym(handle, "DB_Look");
  if ((error = dlerror ()) != NULL)
  {
    g_print (error);
    return (2);
  }
  
  return (0);
}


static void Usage (gchar *p)
{
  g_print (_("\nUsage:  %s [options]\n"
             "            -u, --user db_username\n" 
             "            -p, --password db_password\n" 
             "            -d, --db db_name\n" 
             "            -c, --host db_hostname\n" 
             "            -r, --reports\n" 
             "            -m, --module db_module (pq, mysql)\n" 
             "            -l, --libdir path_to_db_module\n" 
             "            -h, --help\n"), p);
}


static void ReadConfig (gint argc, gchar *argv[])
{
  connect.user = g_strdup ("");
  connect.password = g_strdup ("");
  connect.db = g_strdup ("sms");
  connect.host = g_strdup ("");
  smsdConfig.dbMod = g_strdup ("pq");
  smsdConfig.libDir = g_strdup (MODULES_DIR);
  smsdConfig.smsSets = 0;

  while (1)
  {
    gint optionIndex = 0;
    gchar c;
    static struct option longOptions[] =
    {
      {"user", 1, 0, 'u'},
      {"password", 1, 0, 'p'},
      {"db", 1, 0, 'd'},
      {"host", 1, 0, 'c'},
      {"reports", 0, 0, 'r'},
      {"module", 1, 0, 'm'},
      {"libdir", 1, 0, 'l'}
    };
    
    c = getopt_long (argc, argv, "u:p:d:c:rm:l:h", longOptions, &optionIndex);
    if (c == EOF)
      break;
    switch (c)
    {
      case 'u':
        g_free (connect.user);
        connect.user = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
      
      case 'p':
        g_free (connect.password);
        connect.password = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'd':
        g_free (connect.db);
        connect.db = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'c':
        g_free (connect.host);
        connect.host = g_strdup (optarg);
        memset (optarg, 'x', strlen (optarg));
        break;
        
      case 'r':
        smsdConfig.smsSets = SMSD_READ_REPORTS;
        break;
      
      case 'm':
        g_free (smsdConfig.dbMod);
        smsdConfig.dbMod = g_strdup (optarg);
        break;
        
      case 'l':
        g_free (smsdConfig.libDir);
        smsdConfig.libDir = g_strdup (optarg);
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
    g_print ("Wrong argument number!\n");
    Usage (argv[0]);
    exit (1);
  }

  if (LoadDB ())
  {
    g_print (_("Cannot load database module %s in directory %s!\n"),
              smsdConfig.dbMod, smsdConfig.libDir);
    exit (-2);
  }
  
  if (readconfig (&smsdConfig.model, &smsdConfig.port, &smsdConfig.initlength,
      &smsdConfig.connection, &smsdConfig.bindir) < 0)
    exit (-1);
}


static void *SendSMS2 (void *a)
{
  if ((*DB_ConnectOutbox) (connect))
  {
    pthread_exit (0);
    return (0);
  }

  while (1)
  {
    pthread_mutex_lock (&db_monitorMutex);
    if (!db_monitor)
    {
      pthread_mutex_unlock (&db_monitorMutex);
      pthread_exit (0);
      return (0);
    }
    pthread_mutex_unlock (&db_monitorMutex);

    (*DB_Look) ();

    sleep (3);
  }
}


gint WriteSMS (GSM_SMSMessage *sms)
{
  GSM_Error error;
  PhoneEvent *e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
  D_SMSMessage *m = (D_SMSMessage *) g_malloc (sizeof (D_SMSMessage));

  m->sms = sms;
  e->event = Event_SendSMSMessage;
  e->data = m;
  InsertEvent (e);
  pthread_mutex_lock (&sendSMSMutex);
  pthread_cond_wait (&sendSMSCond, &sendSMSMutex);
  pthread_mutex_unlock (&sendSMSMutex);

#ifdef XDEBUG
  g_print ("Address: %s\nText: %s\n",
  sms->RemoteNumber.number,
  sms->UserData[0].u.Text);
#endif

  error = m->status;
  g_free (m);

  return (error);
}


static void ReadSMS (gpointer d, gpointer userData)
{
  GSM_SMSMessage *data = (GSM_SMSMessage *) d;
  PhoneEvent *e;
  
  if (data->Type == SMS_Deliver || data->Type == SMS_Delivery_Report)
  {
    if (data->Type == SMS_Delivery_Report)
    {
      if (smsdConfig.smsSets & SMSD_READ_REPORTS)
        (*DB_InsertSMS) (data);
    }
    else
    { 
#ifdef XDEBUG 
      g_print ("%d. %s   ", data->Number, data->RemoteNumber.number);
      g_print ("%02d-%02d-%02d %02d:%02d:%02d+%02d %s\n", data->Time.Year,
               data->Time.Month, data->Time.Day, data->Time.Hour,
               data->Time.Minute, data->Time.Second, data->Time.Timezone,
               data->UserData[0].u.Text);
#endif
      (*DB_InsertSMS) (data);
    }
    
    e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
    e->event = Event_DeleteSMSMessage;
    e->data = data;
    InsertEvent (e);
  }
}


static void GetSMS2 (void)
{
  while (1)
  {
    pthread_mutex_lock (&smsMutex);
    pthread_cond_wait (&smsCond, &smsMutex);

    // Waiting for signal
    pthread_mutex_unlock (&smsMutex);
    // Signal arrived.
    
    pthread_mutex_lock (&smsMutex);
    g_slist_foreach (phoneMonitor.sms.messages, ReadSMS, (gpointer) NULL);
    pthread_mutex_unlock (&smsMutex);
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
  exit (0);
}


static void Run (void)
{
  struct sigaction act;
  
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

  InitPhoneMonitor ();
  if ((*DB_ConnectInbox) (connect))
    exit (2);
  pthread_create (&monitor_th, NULL, Connect, NULL);
  db_monitor = TRUE;
  pthread_mutex_init (&db_monitorMutex, NULL);
  pthread_create (&db_monitor_th, NULL, SendSMS2, NULL);
  GetSMS2 ();
}


int main (int argc, char *argv[])
{
#ifdef USE_NLS
  textdomain("gnokii");
#endif

  ReadConfig (argc, argv);
  TerminateThread = false;
  Run ();

  return(0);
}
