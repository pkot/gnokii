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
#include <pthread.h>
#include <getopt.h>

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

#include "gsm-common.h"
#include "gsm-api.h"
#include "fbus-6110.h"
#include "fbus-3810.h"
#include "cfgreader.h"
#include "smsd.h"
#include "lowlevel.h"
#include "db.h"

#define DB_CONNECT	"dbname=sms"

 
/* Hold main configuration data for smsd */
SmsdConfig smsdConfig;

/* Local variables */
static gchar *DefaultModel = MODEL; 
static gchar *DefaultPort = PORT;
static gchar *DefaultBindir = "/usr/sbin/";
static gchar *DefaultConnection = "serial";

static gchar *connect;

static pthread_t db_monitor_th;
pthread_mutex_t db_monitorMutex;
static volatile bool db_monitor;

static void Usage (gchar *p)
{
  g_print ("\nUsage:  %s [options]\n"
           "            -d, --db     DBconnectInfo\n"
           "            -h, --help\n", p);
}


static void ReadConfig (gint argc, gchar *argv[])
{
  struct CFG_Header *cfg_info;
  gchar *homedir;
  gchar *rcfile;

  connect = g_strdup (DB_CONNECT);
  while (1)
  {
    gint optionIndex = 0;
    gchar c;
    static struct option longOptions[] =
    {
      {"db", 1, 0, 'd'}
    };
    
    c = getopt_long (argc, argv, "d:h", longOptions, &optionIndex);
    if (c == EOF)
      break;
    switch (c)
    {
      case 'd':
        g_free (connect);
        connect = g_strdup (optarg);
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
   
#ifdef WIN32
  homedir = g_get_home_dir ();
  rcfile=g_strconcat(homedir, "\\_gnokiirc", NULL);
#else
  if ((homedir = g_get_home_dir ()) == NULL)
  {
    g_print (_("WARNING: Can't find HOME enviroment variable!\n"));
    exit (-1);
  }
  else if ((rcfile = g_strconcat (homedir, "/.gnokiirc", NULL)) == NULL)
  {
    g_print (_("WARNING: Can't allocate memory for config reading!\n"));
    exit (-1);
  }
#endif

  /* Try opening .gnokirc from users home directory first */
  if ((cfg_info = CFG_ReadFile (rcfile)) == NULL)
    /* It failed so try for /etc/gnokiirc */
    if ( (cfg_info = CFG_ReadFile ("/etc/gnokiirc")) == NULL )
      /* That failed too so go with defaults... */
      g_print (_("Couldn't open %s or /etc/gnokiirc, using default config!\n"), rcfile);

  g_free (rcfile);

  smsdConfig.model = CFG_Get(cfg_info, "global", "model");
  if (smsdConfig.model == NULL)
    smsdConfig.model = DefaultModel;

  smsdConfig.port = CFG_Get(cfg_info, "global", "port");
  if (smsdConfig.port == NULL)
    smsdConfig.port = DefaultPort;

  smsdConfig.initlength = CFG_Get(cfg_info, "global", "initlength");
  if (smsdConfig.initlength == NULL) {
       smsdConfig.initlength = "default";
  }

  smsdConfig.connection = CFG_Get(cfg_info, "global", "connection");
  if (smsdConfig.connection == NULL)
    smsdConfig.connection = DefaultConnection;

  smsdConfig.bindir = CFG_Get(cfg_info, "global", "bindir");
    if (smsdConfig.bindir == NULL)
        smsdConfig.bindir = DefaultBindir;

  smsdConfig.smsSets = 0;
}



static void *SendSMS (void *a)
{
  if (DB_ConnectOutbox (connect))
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

    DB_Look ();

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
  sms->Destination,
  sms->MessageText);
#endif

  error = m->status;
  g_free (m);

  return (error);
}


static void ReadSMS (gpointer d, gpointer userData)
{
  GSM_SMSMessage *data = (GSM_SMSMessage *) d;
  PhoneEvent *e;
  
  if (data->Type == GST_MT || data->Type == GST_DR)
  {
    if (data->Type == GST_DR)
    {
#ifdef XDEBUG
      g_print ("Report\n");
#endif
      e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
      e->event = Event_DeleteSMSMessage;
      e->data = data;
      InsertEvent (e);
    }
    else
    { 
#ifdef XDEBUG 
      g_print ("%d. %s   ", data->Location, data->Sender);
      g_print ("%02d-%02d-%02d %02d:%02d:%02d+%02d %s\n", data->Time.Year + 2000,
               data->Time.Month, data->Time.Day, data->Time.Hour,
               data->Time.Minute, data->Time.Second, data->Time.Timezone,
               data->MessageText);
#endif
      DB_InsertSMS (data);
      
      e = (PhoneEvent *) g_malloc (sizeof (PhoneEvent));
      e->event = Event_DeleteSMSMessage;
      e->data = data;
      InsertEvent (e);
    }
  }
}


static void GetSMS (void)
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
  DB_Bye ();
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

#if defined(__svr4__) || defined(__FreeBSD__)
  act.sa_handler = SIG_IGN;
  sigemptyset (&(act.sa_mask));
  sigaction (SIGALRM, &act, NULL);
#endif

  InitPhoneMonitor ();
  if (DB_ConnectInbox (connect))
    exit (2);
  pthread_create (&monitor_th, NULL, Connect, NULL);
  db_monitor = TRUE;
  pthread_mutex_init (&db_monitorMutex, NULL);
  pthread_create (&db_monitor_th, NULL, SendSMS, NULL);
  GetSMS ();
}


int main (int argc, char *argv[])
{
#ifdef USE_NLS
  textdomain("gnokii");
#endif

  ReadConfig (argc, argv);
  Run ();

  return(0);
}
