/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/

/* ****************************************************************************
*
* Some notes on HTTPS
*
* Lots of info found in http://www.gnu.org/software/libmicrohttpd/tutorial.html
*
* When https is used, the broker must be started with options '-key' and '-cert'.
* Both these two options have a file path associated to it:
*   -key:  path to a file containing the private key for the server
*   -cert: path to a file containing a certificate describing the server in human readable tokens
*
* These files are generated before starting the broker:
*
* o private key:
*     % openssl genrsa -out server.key 1024
*
* o certificate:
*     % openssl req -days 365 -out server.pem -new -x509 -key server.key
*
* After creating these two files, the context broker can be started like this:
*   % contextBroker -fg -https -key server.key -cert server.pem
*
* The clients need to use the 'server.pem' file in the request:
* curl --cacert server.pem
*
*
* To override the security added with the certificate, curl can always be called using the
* CLI option '--insecure'.
*/
#include <stdio.h>
#include <unistd.h>                             // getppid, for, setuid, etc.
#include <string.h>
#include <fcntl.h>                              // open
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <string>
#include <vector>
#include <limits.h>

#include "mongoBackend/MongoGlobal.h"
#include "cache/subCache.h"

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "parseArgs/paBuiltin.h"
#include "parseArgs/paIsSet.h"
#include "parseArgs/paUsage.h"
#include "parseArgs/paIterate.h"
#include "parseArgs/paPrivate.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "jsonParse/jsonRequest.h"
#include "rest/ConnectionInfo.h"
#include "rest/RestService.h"
#include "rest/restReply.h"
#include "rest/rest.h"
#include "rest/httpRequestSend.h"

#include "common/sem.h"
#include "common/globals.h"
#include "common/Timer.h"
#include "common/compileInfo.h"
#include "common/SyncQOverflow.h"

#include "orionTypes/EntityTypeVectorResponse.h"
#include "ngsi/ParseData.h"
#include "ngsiNotify/QueueNotifier.h"
#include "ngsiNotify/QueueWorkers.h"
#include "ngsiNotify/senderThread.h"

#include "contextBroker/version.h"
#include "common/string.h"
#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "logSummary/logSummary.h"

#include "contextBroker/orionRestServices.h"

using namespace orion;



/* ****************************************************************************
*
* DB_NAME_MAX_LEN - max length of database name
*/
#define DB_NAME_MAX_LEN  10



/* ****************************************************************************
*
* Global vars
*/
static bool isFatherProcess = false;



/* ****************************************************************************
*
* Option variables
*/
bool            fg;
char            bindAddress[MAX_LEN_IP];
int             port;
char            dbHost[256];
char            rplSet[64];
char            dbName[64];
char            user[64];
char            pwd[64];
char            authMech[64];
char            authDb[64];
bool            dbSSL;
char            pidPath[256];
bool            harakiri;
bool            useOnlyIPv4;
bool            useOnlyIPv6;
char            httpsKeyFile[1024];
char            httpsCertFile[1024];
bool            https;
bool            mtenant;
char            allowedOrigin[64];
int             maxAge;
long            dbTimeout;
long            httpTimeout;
int             dbPoolSize;
char            reqMutexPolicy[16];
int             writeConcern;
unsigned int    cprForwardLimit;
int             subCacheInterval;
char            notificationMode[64];
char            notifFlowControl[64];
int             notificationQueueSize;
int             notificationThreadNum;
bool            noCache;
unsigned int    connectionMemory;
unsigned int    maxConnections;
unsigned int    reqPoolSize;
bool            simulatedNotification;
bool            statCounters;
bool            statSemWait;
bool            statTiming;
bool            statNotifQueue;
int             lsPeriod;
bool            relogAlarms;
bool            strictIdv1;
bool            disableCusNotif;
bool            logForHumans;
unsigned long   logLineMaxSize;
unsigned long   logInfoPayloadMaxSize;
bool            disableMetrics;
bool            disableFileLog;
int             reqTimeout;
bool            insecureNotif;
bool            ngsiv1Autocast;

bool            fcEnabled;
double          fcGauge;
unsigned long   fcStepDelay;
unsigned long   fcMaxInterval;



/* ****************************************************************************
*
* Definitions to make paArgs lines shorter ...
*/
#define PIDPATH                _i "/tmp/contextBroker.pid"
#define IP_ALL                 _i "0.0.0.0"
#define LOCALHOST              _i "localhost"
#define ONE_MONTH_PERIOD       (3600 * 24 * 31)

#define FG_DESC                "don't start as daemon"
#define LOCALIP_DESC           "IP to receive new connections"
#define PORT_DESC              "port to receive new connections"
#define PIDPATH_DESC           "pid file path"
#define DBHOST_DESC            "database host"
#define RPLSET_DESC            "replica set"
#define DBUSER_DESC            "database user"
#define DBPASSWORD_DESC        "database password"
#define DBAUTHMECH_DESC        "database authentication mechanism (either SCRAM-SHA-1 or MONGODB-CR)"
#define DBAUTHDB_DESC          "database used for authentication"
#define DBSSL_DESC             "enable SSL connection to DB"
#define DB_DESC                "database name"
#define DB_TMO_DESC            "timeout in milliseconds for connections to the replica set (ignored in the case of not using replica set)"
#define USEIPV4_DESC           "use ip v4 only"
#define USEIPV6_DESC           "use ip v6 only"
#define HARAKIRI_DESC          "commits harakiri on request"
#define HTTPS_DESC             "use the https 'protocol'"
#define HTTPSKEYFILE_DESC      "private server key file (for https)"
#define HTTPSCERTFILE_DESC     "certificate key file (for https)"
#define MULTISERVICE_DESC      "service multi tenancy mode"
#define ALLOWED_ORIGIN_DESC    "enable Cross-Origin Resource Sharing with allowed origin. Use '__ALL' for any"
#define CORS_MAX_AGE_DESC      "maximum time in seconds preflight requests are allowed to be cached. Default: 86400"
#define HTTP_TMO_DESC          "timeout in milliseconds for forwards and notifications"
#define DBPS_DESC              "database connection pool size"
#define MAX_L                  900000
#define MUTEX_POLICY_DESC      "mutex policy (none/read/write/all)"
#define WRITE_CONCERN_DESC     "db write concern (0:unacknowledged, 1:acknowledged)"
#define CPR_FORWARD_LIMIT_DESC "maximum number of forwarded requests to Context Providers for a single client request"
#define SUB_CACHE_IVAL_DESC    "interval in seconds between calls to Subscription Cache refresh (0: no refresh)"
#define NOTIFICATION_MODE_DESC "notification mode (persistent|transient|threadpool:q:n)"
#define FLOW_CONTROL_DESC      "notification flow control parameters (gauge:stepDelay:maxInterval)"
#define NO_CACHE               "disable subscription cache for lookups"
#define CONN_MEMORY_DESC       "maximum memory size per connection (in kilobytes)"
#define MAX_CONN_DESC          "maximum number of simultaneous connections"
#define REQ_POOL_SIZE          "size of thread pool for incoming connections"
#define IN_REQ_PAYLOAD_MAX_SIZE_DESC   "maximum size (in bytes) of the payload of incoming requests"
#define OUT_REQ_MSG_MAX_SIZE_DESC      "maximum size (in bytes) of outgoing forward and notification request messages"
#define SIMULATED_NOTIF_DESC   "simulate notifications instead of actual sending them (only for testing)"
#define STAT_COUNTERS          "enable request/notification counters statistics"
#define STAT_SEM_WAIT          "enable semaphore waiting time statistics"
#define STAT_TIMING            "enable request-time-measuring statistics"
#define STAT_NOTIF_QUEUE       "enable thread pool notifications queue statistics"
#define LOG_SUMMARY_DESC       "log summary period in seconds (defaults to 0, meaning 'off')"
#define RELOGALARMS_DESC       "log messages for existing alarms beyond the raising alarm log message itself"
#define CHECK_v1_ID_DESC       "additional checks for id fields in the NGSIv1 API"
#define DISABLE_CUSTOM_NOTIF   "disable NGSIv2 custom notifications"
#define DISABLE_FILE_LOG       "disable logging into files"
#define LOG_FOR_HUMANS_DESC    "human readible log to screen"
#define LOG_LINE_MAX_SIZE_DESC "log line maximum size (in bytes)"
#define LOG_INFO_PAYLOAD_MAX_SIZE_DESC  "maximum length for request or response payload in INFO log level (in bytes)"
#define METRICS_DESC           "turn off the 'metrics' feature"
#define REQ_TMO_DESC           "connection timeout for REST requests (in seconds)"
#define INSECURE_NOTIF         "allow HTTPS notifications to peers which certificate cannot be authenticated with known CA certificates"
#define NGSIV1_AUTOCAST        "automatic cast for number, booleans and dates in NGSIv1 update/create attribute operations"



/* ****************************************************************************
*
* paArgs - option vector for the Parse CLI arguments library
*
* A note about the default value of -maxConnections.
* In older implementations of the broker, select was used in MHD and not poll/epoll.
* The old default value (1024 - 4), that was a recommendation by MHD, has been kept.
* More info about this can be found in the documentation of MHD.
*/
PaArgument paArgs[] =
{
  { "-fg",                          &fg,                    "FOREGROUND",               PaBool,   PaOpt, false,                           false, true,             FG_DESC                      },
  { "-localIp",                     bindAddress,            "LOCALIP",                  PaString, PaOpt, IP_ALL,                          PaNL,  PaNL,             LOCALIP_DESC                 },
  { "-port",                        &port,                  "PORT",                     PaInt,    PaOpt, 1026,                            PaNL,  PaNL,             PORT_DESC                    },
  { "-pidpath",                     pidPath,                "PID_PATH",                 PaString, PaOpt, PIDPATH,                         PaNL,  PaNL,             PIDPATH_DESC                 },

  { "-dbhost",                      dbHost,                 "MONGO_HOST",               PaString, PaOpt, LOCALHOST,                       PaNL,  PaNL,             DBHOST_DESC                  },
  { "-rplSet",                      rplSet,                 "MONGO_REPLICA_SET",        PaString, PaOpt, _i "",                           PaNL,  PaNL,             RPLSET_DESC                  },
  { "-dbuser",                      user,                   "MONGO_USER",               PaString, PaOpt, _i "",                           PaNL,  PaNL,             DBUSER_DESC                  },
  { "-dbpwd",                       pwd,                    "MONGO_PASSWORD",           PaString, PaOpt, _i "",                           PaNL,  PaNL,             DBPASSWORD_DESC              },

  { "-dbAuthMech",                  authMech,               "MONGO_AUTH_MECH",          PaString, PaOpt, _i "SCRAM-SHA-1",                PaNL,  PaNL,             DBAUTHMECH_DESC              },
  { "-dbAuthDb",                    authDb,                 "MONGO_AUTH_SOURCE",        PaString, PaOpt, _i "",                           PaNL,  PaNL,             DBAUTHDB_DESC                },
  { "-dbSSL",                       &dbSSL,                 "MONGO_SSL",                PaBool,   PaOpt, false,                           false, true,             DBSSL_DESC                   },

  { "-db",                          dbName,                 "MONGO_DB",                 PaString, PaOpt, _i "orion",                      PaNL,  PaNL,             DB_DESC                      },
  { "-dbTimeout",                   &dbTimeout,             "MONGO_TIMEOUT",            PaDouble, PaOpt, 10000,                           PaNL,  PaNL,             DB_TMO_DESC                  },
  { "-dbPoolSize",                  &dbPoolSize,            "MONGO_POOL_SIZE",          PaInt,    PaOpt, 10,                              1,     10000,            DBPS_DESC                    },

  { "-ipv4",                        &useOnlyIPv4,           "USEIPV4",                  PaBool,   PaOpt, false,                           false, true,             USEIPV4_DESC                 },
  { "-ipv6",                        &useOnlyIPv6,           "USEIPV6",                  PaBool,   PaOpt, false,                           false, true,             USEIPV6_DESC                 },
  { "-harakiri",                    &harakiri,              "HARAKIRI",                 PaBool,   PaHid, false,                           false, true,             HARAKIRI_DESC                },

  { "-https",                       &https,                 "HTTPS",                    PaBool,   PaOpt, false,                           false, true,             HTTPS_DESC                   },
  { "-key",                         httpsKeyFile,           "HTTPS_KEYFILE",            PaString, PaOpt, _i "",                           PaNL,  PaNL,             HTTPSKEYFILE_DESC            },
  { "-cert",                        httpsCertFile,          "HTTPS_CERTFILE",           PaString, PaOpt, _i "",                           PaNL,  PaNL,             HTTPSCERTFILE_DESC           },

  { "-multiservice",                &mtenant,               "MULTI_SERVICE",            PaBool,   PaOpt, false,                           false, true,             MULTISERVICE_DESC            },

  { "-httpTimeout",                 &httpTimeout,           "HTTP_TIMEOUT",             PaLong,   PaOpt, -1,                              -1,    MAX_L,            HTTP_TMO_DESC                },
  { "-reqTimeout",                  &reqTimeout,            "REQ_TIMEOUT",              PaLong,   PaOpt,  0,                               0,    PaNL,             REQ_TMO_DESC                 },
  { "-reqMutexPolicy",              reqMutexPolicy,         "MUTEX_POLICY",             PaString, PaOpt, _i "all",                        PaNL,  PaNL,             MUTEX_POLICY_DESC            },
  { "-writeConcern",                &writeConcern,          "MONGO_WRITE_CONCERN",      PaInt,    PaOpt, 1,                               0,     1,                WRITE_CONCERN_DESC           },

  { "-corsOrigin",                  allowedOrigin,          "CORS_ALLOWED_ORIGIN",      PaString, PaOpt, _i "",                           PaNL,  PaNL,             ALLOWED_ORIGIN_DESC          },
  { "-corsMaxAge",                  &maxAge,                "CORS_MAX_AGE",             PaInt,    PaOpt, 86400,                           -1,    86400,            CORS_MAX_AGE_DESC            },
  { "-cprForwardLimit",             &cprForwardLimit,       "CPR_FORWARD_LIMIT",        PaUInt,   PaOpt, 1000,                            0,     UINT_MAX,         CPR_FORWARD_LIMIT_DESC       },
  { "-subCacheIval",                &subCacheInterval,      "SUBCACHE_IVAL",            PaInt,    PaOpt, 60,                              0,     3600,             SUB_CACHE_IVAL_DESC          },
  { "-noCache",                     &noCache,               "NOCACHE",                  PaBool,   PaOpt, false,                           false, true,             NO_CACHE                     },
  { "-connectionMemory",            &connectionMemory,      "CONN_MEMORY",              PaUInt,   PaOpt, 64,                              0,     1024,             CONN_MEMORY_DESC             },
  { "-maxConnections",              &maxConnections,        "MAX_CONN",                 PaUInt,   PaOpt, 1020,                            1,     PaNL,             MAX_CONN_DESC                },
  { "-reqPoolSize",                 &reqPoolSize,           "TRQ_POOL_SIZE",            PaUInt,   PaOpt, 0,                               0,     1024,             REQ_POOL_SIZE                },

  { "-inReqPayloadMaxSize",         &inReqPayloadMaxSize,   "IN_REQ_PAYLOAD_MAX_SIZE",  PaULong,  PaOpt, DEFAULT_IN_REQ_PAYLOAD_MAX_SIZE, 0,     PaNL,             IN_REQ_PAYLOAD_MAX_SIZE_DESC },
  { "-outReqMsgMaxSize",            &outReqMsgMaxSize,      "OUT_REQ_MSG_MAX_SIZE",     PaULong,  PaOpt, DEFAULT_OUT_REQ_MSG_MAX_SIZE,    0,     PaNL,             OUT_REQ_MSG_MAX_SIZE_DESC    },

  { "-notificationMode",            &notificationMode,      "NOTIF_MODE",               PaString, PaOpt, _i "transient",                  PaNL,  PaNL,             NOTIFICATION_MODE_DESC       },
  { "-notifFlowControl",            &notifFlowControl,      "NOTIF_FLOW_CONTROL",       PaString, PaOpt, _i "",                           PaNL,  PaNL,             FLOW_CONTROL_DESC            },
  { "-simulatedNotification",       &simulatedNotification, "DROP_NOTIF",               PaBool,   PaOpt, false,                           false, true,             SIMULATED_NOTIF_DESC         },

  { "-statCounters",                &statCounters,          "STAT_COUNTERS",            PaBool,   PaOpt, false,                           false, true,             STAT_COUNTERS                },
  { "-statSemWait",                 &statSemWait,           "STAT_SEM_WAIT",            PaBool,   PaOpt, false,                           false, true,             STAT_SEM_WAIT                },
  { "-statTiming",                  &statTiming,            "STAT_TIMING",              PaBool,   PaOpt, false,                           false, true,             STAT_TIMING                  },
  { "-statNotifQueue",              &statNotifQueue,        "STAT_NOTIF_QUEUE",         PaBool,   PaOpt, false,                           false, true,             STAT_NOTIF_QUEUE             },

  { "-logSummary",                  &lsPeriod,              "LOG_SUMMARY_PERIOD",       PaInt,    PaOpt, 0,                               0,     ONE_MONTH_PERIOD, LOG_SUMMARY_DESC             },
  { "-relogAlarms",                 &relogAlarms,           "RELOG_ALARMS",             PaBool,   PaOpt, false,                           false, true,             RELOGALARMS_DESC             },

  { "-strictNgsiv1Ids",             &strictIdv1,            "CHECK_ID_V1",              PaBool,   PaOpt, false,                           false, true,             CHECK_v1_ID_DESC             },
  { "-disableCustomNotifications",  &disableCusNotif,       "DISABLE_CUSTOM_NOTIF",     PaBool,   PaOpt, false,                           false, true,             DISABLE_CUSTOM_NOTIF         },

  { "-disableFileLog",              &disableFileLog,        "DISABLE_FILE_LOG",         PaBool,   PaOpt, false,                           false, true,             DISABLE_FILE_LOG             },
  { "-logForHumans",                &logForHumans,          "LOG_FOR_HUMANS",           PaBool,   PaOpt, false,                           false, true,             LOG_FOR_HUMANS_DESC          },
  { "-logLineMaxSize",              &logLineMaxSize,        "LOG_LINE_MAX_SIZE",        PaLong,   PaOpt, (32 * 1024),                     100,   PaNL,             LOG_LINE_MAX_SIZE_DESC       },
  { "-logInfoPayloadMaxSize",       &logInfoPayloadMaxSize, "LOG_INFO_PAYLOAD_MAX_SIZE",PaLong,   PaOpt, (5 * 1024),                      0,     PaNL,             LOG_INFO_PAYLOAD_MAX_SIZE_DESC  },

  { "-disableMetrics",              &disableMetrics,        "DISABLE_METRICS",          PaBool,   PaOpt, false,                           false, true,             METRICS_DESC                 },

  { "-insecureNotif",               &insecureNotif,         "INSECURE_NOTIF",           PaBool,   PaOpt, false,                           false, true,             INSECURE_NOTIF               },

  { "-ngsiv1Autocast",              &ngsiv1Autocast,        "NGSIV1_AUTOCAST",          PaBool,   PaOpt, false,                           false, true,             NGSIV1_AUTOCAST              },

  PA_END_OF_ARGS
};



/* ****************************************************************************
*
* validLogLevels - to pass to parseArgs library for validation of --logLevel
*/
static const char* validLogLevels[] =
{
  "NONE",
  "FATAL",
  "ERROR",
  "WARN",
  "INFO",
  "DEBUG",
  NULL
};



/* ****************************************************************************
*
* restService* - vectors of REST services for the context broker
*
* This vector matches an incoming REST service, using the path of the URL, to a function
* to treat the incoming request.
*
* The URL path is divided into components (Using '/' as field separator) so that the URL
* "/ngsi9/registerContext" becomes a component vector of the two components
* "ngsi9" and "registerContext".
*
* Each line contains the necessary information for ONE service:
*   RequestType   request     - The type of the request
*   int           components  - Number of components in the following URL component vector
*   std::string   compV       - Component vector of the URL
*   RestTreat     treat       - Function pointer to the function to treat the incoming REST request
*
*/



/* ****************************************************************************
*
* fileExists -
*/
static bool fileExists(char* path)
{
  if (access(path, F_OK) == 0)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* pidFile -
*
* When run "interactively" (with the CLI option '-fg' set), the error messages get really ugly.
* However, that is a minor bad, compared to what would happen to a 'nice printf message' when started as a service.
* It would be lost. The log file is important and we can't just use 'fprintf(stderr, ...)' ...
*/
int pidFile(bool justCheck)
{
  if (fileExists(pidPath))
  {
    LM_E(("PID-file '%s' found. A broker seems to be running already", pidPath));
    return 1;
  }

  if (justCheck == true)
  {
    return 0;
  }

  int    fd = open(pidPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  pid_t  pid;
  char   buffer[32];
  int    sz;
  int    nb;

  if (fd == -1)
  {
    LM_E(("PID File (open '%s': %s)", pidPath, strerror(errno)));
    return 2;
  }

  pid = getpid();

  snprintf(buffer, sizeof(buffer), "%d", pid);
  sz = strlen(buffer);
  nb = write(fd, buffer, sz);
  if (nb != sz)
  {
    LM_E(("PID File (written %d bytes and not %d to '%s': %s)", nb, sz, pidPath, strerror(errno)));
    return 3;
  }

  return 0;
}



/* ****************************************************************************
*
* daemonize -
*/
void daemonize(void)
{
  pid_t  pid;
  pid_t  sid;

  // already daemon
  if (getppid() == 1)
  {
    return;
  }

  pid = fork();
  if (pid == -1)
  {
    LM_X(1, ("Fatal Error (fork: %s)", strerror(errno)));
  }

  // Exiting father process
  if (pid > 0)
  {
    isFatherProcess = true;
    exit(0);
  }

  // Change the file mode mask */
  umask(0);

  // Removing the controlling terminal
  sid = setsid();
  if (sid == -1)
  {
    LM_X(1, ("Fatal Error (setsid: %s)", strerror(errno)));
  }

  // Change current working directory.
  // This prevents the current directory from being locked; hence not being able to remove it.
  if (chdir("/") == -1)
  {
    LM_X(1, ("Fatal Error (chdir: %s)", strerror(errno)));
  }

  // We have to call this after a fork, see: http://api.mongodb.org/cplusplus/2.2.2/classmongo_1_1_o_i_d.html
  mongo::OID::justForked();
}



/* ****************************************************************************
*
* sigHandler -
*/
void sigHandler(int sigNo)
{
  LM_T(LmtOldInfo, ("Signal Handler (caught signal %d)", sigNo));

  switch (sigNo)
  {
  case SIGINT:
  case SIGTERM:
  case SIGHUP:
    LM_I(("Orion context broker exiting due to receiving a signal"));
    exit(0);
    break;
  }
}



/* ****************************************************************************
*
* orionExit -
*/
void orionExit(int code, const std::string& reason)
{
  if (code == 0)
  {
    LM_I(("Orion context broker exits in an ordered manner (%s)", reason.c_str()));
  }
  else
  {
    LM_E(("Fatal Error (reason: %s)", reason.c_str()));
  }

  exit(code);
}



/* ****************************************************************************
*
* exitFunc -
*/
void exitFunc(void)
{
  if (isFatherProcess)
  {
    isFatherProcess = false;
    return;
  }

#ifdef DEBUG
  // Take mongo req-sem ?
  LM_T(LmtSubCache, ("try-taking req semaphore"));
  reqSemTryToTake();
  LM_T(LmtSubCache, ("calling subCacheDestroy"));
  subCacheDestroy();
#endif

  metricsMgr.release();

  curl_context_cleanup();
  curl_global_cleanup();

  if (unlink(pidPath) != 0)
  {
    LM_T(LmtSoftError, ("error removing PID file '%s': %s", pidPath, strerror(errno)));
  }
}



/* ****************************************************************************
*
* description -
*/
const char* description =
  "\n"
  "Orion context broker version details:\n"
  "  version:            " ORION_VERSION   "\n"
  "  git hash:           " GIT_HASH        "\n"
  "  compile time:       " COMPILE_TIME    "\n"
  "  compiled by:        " COMPILED_BY     "\n"
  "  compiled in:        " COMPILED_IN     "\n";



/* ****************************************************************************
*
* contextBrokerInit -
*/
static void contextBrokerInit(std::string dbPrefix, bool multitenant)
{
  Notifier* pNotifier = NULL;

  /* If we use a queue for notifications, start worker threads */
  if (strcmp(notificationMode, "threadpool") == 0)
  {
    QueueNotifier*  pQNotifier = new QueueNotifier(notificationQueueSize, notificationThreadNum);
    int             rc         = pQNotifier->start();

    if (rc != 0)
    {
      LM_X(1,("Runtime Error starting notification queue workers (%d)", rc));
    }

    pNotifier = pQNotifier;
  }
  else
  {
    pNotifier = new Notifier();
  }

  /* Set notifier object (singleton) */
  setNotifier(pNotifier);

  /* Set HTTP timeout */
  httpRequestInit(httpTimeout);
  
  //WARN about insecureNotifications mode
  if (insecureNotif == true)
   {
      LM_W(("contextBroker started in insecure notifications mode (-insecureNotif)"));
   }
}



/* ****************************************************************************
*
* loadFile -
*/
static int loadFile(char* path, char* out, int outSize)
{
  struct stat  statBuf;
  int          nb;
  int          fd = open(path, O_RDONLY);

  if (fd == -1)
  {
    LM_E(("HTTPS Error (error opening '%s': %s)", path, strerror(errno)));
    return -1;
  }

  if (stat(path, &statBuf) != 0)
  {
    close(fd);
    LM_E(("HTTPS Error (error 'stating' '%s': %s)", path, strerror(errno)));
    return -1;
  }

  if (statBuf.st_size > outSize)
  {
    close(fd);
    LM_E(("HTTPS Error (file '%s' is TOO BIG (%d) - max size is %d bytes)", path, outSize));
    return -1;
  }

  nb = read(fd, out, statBuf.st_size);
  close(fd);

  if (nb == -1)
  {
    LM_E(("HTTPS Error (reading from '%s': %s)", path, strerror(errno)));
    return -1;
  }

  if (nb != statBuf.st_size)
  {
    LM_E(("HTTPS Error (invalid size read from '%s': %d, wanted %d)", path, nb, statBuf.st_size));
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* policyGet -
*/
static SemOpType policyGet(std::string mutexPolicy)
{
  if (mutexPolicy == "read")
  {
    return SemReadOp;
  }
  else if (mutexPolicy == "write")
  {
    return SemWriteOp;
  }
  else if (mutexPolicy == "all")
  {
    return SemReadWriteOp;
  }
  else if (mutexPolicy == "none")
  {
    return SemNoneOp;
  }

  //
  // Default is to protect both reads and writes
  //
  return SemReadWriteOp;
}



/* ****************************************************************************
*
* notificationModeParse -
*/
static void notificationModeParse(char *notifModeArg, int *pQueueSize, int *pNumThreads)
{
  char* mode;
  char* first_colon;
  int   flds_num;

  errno = 0;
  // notifModeArg is a char[64], pretty sure not a huge input to break sscanf
  // cppcheck-suppress invalidscanf
  flds_num = sscanf(notifModeArg, "%m[^:]:%d:%d", &mode, pQueueSize, pNumThreads);
  if (errno != 0)
  {
    LM_X(1, ("Fatal Error parsing notification mode: sscanf (%s)", strerror(errno)));
  }
  if (flds_num == 3 && strcmp(mode, "threadpool") == 0)
  {
    if (*pQueueSize <= 0)
    {
      LM_X(1, ("Fatal Error parsing notification mode: invalid queue size (%d)", *pQueueSize));
    }
    if (*pNumThreads <= 0)
    {
      LM_X(1, ("Fatal Error parsing notification mode: invalid number of threads (%d)",*pNumThreads));
    }
  }
  else if (flds_num == 1 && strcmp(mode, "threadpool") == 0)
  {
    *pQueueSize = DEFAULT_NOTIF_QS;
    *pNumThreads = DEFAULT_NOTIF_TN;
  }
  else if (!(
             flds_num == 1 &&
             (strcmp(mode, "transient") == 0 || strcmp(mode, "persistent") == 0)
             ))
  {
    LM_X(1, ("Fatal Error parsing notification mode: invalid mode (%s)", notifModeArg));
  }

  // get rid of params, if any, in notifModeArg
  first_colon = strchr(notifModeArg, ':');
  if (first_colon != NULL)
  {
    *first_colon = '\0';
  }

  free(mode);
}



/* ****************************************************************************
*
* notifFlowControlParse -
*/
static void notifFlowControlParse
(
  char*           notifFlowControl,
  double*         fcGaugeP,
  unsigned long*  fcStepDelayP,
  unsigned long*  fcMaxIntervalP
)
{
  std::stringstream ss(notifFlowControl);
  std::vector<std::string> tokens;
  while(ss.good())
  {
    std::string substr;
    getline(ss, substr, ':');
    tokens.push_back(substr);
  }

  if (tokens.size() != 3)
  {
    LM_X(1, ("Fatal Error parsing notification flow control: more tokens than expected (%d)", tokens.size()));
  }

  std::string error = "";
  *fcGaugeP = atoF(tokens[0].c_str(), &error);
  if (error != "")
  {
    LM_X(1, ("Fatal Error parsing notification flow control: error parsing gauge '%s': %s",
             tokens[0].c_str(), error.c_str()));
  }
  if (*fcGaugeP > 1 || *fcGaugeP < 0)
  {
    LM_X(1, ("Fatal Error parsing notification flow control: gauge must be between 0 and 1 and is %f", *fcGaugeP));
  }

  *fcStepDelayP = atoUL(tokens[1].c_str(), &error);
  if (error != "")
  {
    LM_X(1, ("Fatal Error parsing notification flow control: error parsing stepDelay '%s': %s",
             tokens[1].c_str(), error.c_str()));
  }
  if (*fcStepDelayP == 0)
  {
    LM_X(1, ("Fatal Error parsing notification flow control: stepDelay must be strictly greater than 0", *fcStepDelayP));
  }

  *fcMaxIntervalP = atoUL(tokens[2].c_str(), &error);
  if (error != "")
  {
    LM_X(1, ("Fatal Error parsing notification flow control: error parsing maxInterval '%s': %s",
             tokens[2].c_str(), error.c_str()));
  }
  if (*fcMaxIntervalP == 0)
  {
    LM_X(1, ("Fatal Error parsing notification flow control: maxInterval must be strictly greater than 0", *fcMaxIntervalP));
  }
}


/* ****************************************************************************
*
* cmdLineString -
*
* Exceptionally, this function return value doesn't follow the coding style rule
* of not returning objects. It is due to simplicity. Note this function is
* used only at startup, so no impact in performance is expected
*/
static std::string cmdLineString(int argC, char* argV[])
{
  std::string s;
  for (int ix =  0; ix < argC; ix++)
  {
    s += std::string(argV[ix]);
    if (ix != argC -1)
    {
      s += " ";
    }
  }
  return s;
}



/* ****************************************************************************
*
* logEnvVars -
*
* Print env var configuration in INFO traces
*/
static void logEnvVars(void)
{
  PaiArgument* aP;
  paIterateInit();
  while ((aP = paIterateNext(paiList)) != NULL)
  {
    if ((aP->from == PafEnvVar) && (aP->isBuiltin == false))
    {
      if (aP->type == PaString)
      {
        LM_I(("env var ORION_%s (%s): %s", aP->envName, aP->option, (char*) aP->varP));
      }
      else if (aP->type == PaBool)
      {
        LM_I(("env var ORION_%s (%s): %d", aP->envName, aP->option, (bool) aP->varP));
      }
      else if (aP->type == PaInt)
      {
        LM_I(("env var ORION_%s (%s): %d", aP->envName, aP->option, *((int*) aP->varP)));
      }
      else if (aP->type == PaDouble)
      {
        LM_I(("env var ORION_%s (%s): %d", aP->envName, aP->option, *((double*) aP->varP)));
      }
      else
      {
        LM_I(("env var ORION_%s (%s): %d", aP->envName, aP->option));
      }
    }
  }
}



#define LOG_FILE_LINE_FORMAT "time=DATE | lvl=TYPE | corr=CORR_ID | trans=TRANS_ID | from=FROM_IP | srv=SERVICE | subsrv=SUB_SERVICE | comp=Orion | op=FILE[LINE]:FUNC | msg=TEXT"
/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{
  int s;

  lmTransactionReset();

  signal(SIGINT,  sigHandler);
  signal(SIGTERM, sigHandler);
  signal(SIGHUP,  sigHandler);

  atexit(exitFunc);

  paConfig("remove builtin", "-d");
  paConfig("remove builtin", "-r");
  paConfig("remove builtin", "-w");
  paConfig("remove builtin", "-F");
  paConfig("remove builtin", "-B");
  paConfig("remove builtin", "-b");
  paConfig("remove builtin", "-?");
  paConfig("remove builtin", "-toDo");
  paConfig("remove builtin", "-lmnc");
  paConfig("remove builtin", "-lmca");
  paConfig("remove builtin", "-lmkl");
  paConfig("remove builtin", "-lmll");
  paConfig("remove builtin", "-assert");
  paConfig("remove builtin", "-version");
  paConfig("remove builtin", "-h");
  paConfig("remove builtin", "-help");
  paConfig("remove builtin", "-v");
  paConfig("remove builtin", "-vv");
  paConfig("remove builtin", "-vvv");
  paConfig("remove builtin", "-vvvv");
  paConfig("remove builtin", "-vvvvv");
  paConfig("remove builtin", "--silent");
  paConfig("bool option with value as non-recognized option", NULL);

  paConfig("man exitstatus", (void*) "The orion broker is a daemon. If it exits, something is wrong ...");

  std::string versionString = std::string(ORION_VERSION) + " (git version: " + GIT_HASH + ")";

  paConfig("man synopsis",                  (void*) "[options]");
  paConfig("man shortdescription",          (void*) "Options:");
  paConfig("man description",               (void*) description);
  paConfig("man author",                    (void*) "Telefonica I+D");
  paConfig("man version",                   (void*) versionString.c_str());
  paConfig("log file line format",          (void*) LOG_FILE_LINE_FORMAT);
  paConfig("log file time format",          (void*) "%Y-%m-%dT%H:%M:%S");
  paConfig("screen time format",            (void*) "%Y-%m-%dT%H:%M:%S");
  paConfig("builtin prefix",                (void*) "ORION_");
  paConfig("prefix",                        (void*) "ORION_");
  paConfig("usage and exit on any warning", (void*) true);
  paConfig("no preamble",                   NULL);
  paConfig("valid log level strings",       validLogLevels);
  paConfig("default value",                 "-logLevel", "WARN");

  //
  // If option '-fg' is set, print traces to stdout as well, otherwise, only to file
  //
  if (paIsSet(argC, argV, "-fg"))
  {
    paConfig("log to screen",                 (void*) true);
  }

  paParse(paArgs, argC, (char**) argV, 1, false);
  lmTimeFormat(0, (char*) "%Y-%m-%dT%H:%M:%S");

  if (logForHumans)
  {
    paConfig("screen line format", (void*) "TYPE@TIME  FILE[LINE]: TEXT");
  }
  else
  {
    paConfig("screen line format", LOG_FILE_LINE_FORMAT);
  }

  //
  // disable file logging if the corresponding option is set. 
  //
  if (disableFileLog)
  {
    paConfig("log to file",                   (void*) false);
  } 
  else
  {
    paConfig("log to file",                   (void*) true);
  }

  //
  // NOTE: Calling '_exit()' and not 'exit()' if 'pidFile()' returns error.
  //       The exit-function removes the PID-file and we don't want that. We want
  //       the PID-file to remain.
  //       Calling '_exit()' instead of 'exit()' makes sure that the exit-function is not called.
  //
  //       This call here is just to check for the existance of the PID-file.
  //       If the file exists, the broker dies here.
  //       The creation of the PID-file must be done AFTER "daemonize()" as here we still don't know the
  //       PID of the broker. The father process dies and the son-process continues, in "daemonize()".
  //
  if ((s = pidFile(true)) != 0)
  {
    _exit(s);
  }

  // print startup info in logs
  LM_I(("start command line <%s>", cmdLineString(argC, argV).c_str()));
  logEnvVars();

  // Argument consistency check (-t AND NOT -logLevel)
  if ((paTraceV[0] != 0) && (strcmp(paLogLevel, "DEBUG") != 0))
  {
    printf("incompatible options: traceLevels cannot be used without setting -logLevel to DEBUG\n");
    paUsage();
    exit(1);
  }

  paCleanup();

#ifdef DEBUG_develenv
  //
  // FIXME P9: Temporary setting trace level 250 in jenkins only, until the ftest-ftest-ftest bug is solved
  //           See issue #652
  //
  lmTraceLevelSet(LmtBug, true);
#endif

  if (strlen(dbName) > DB_NAME_MAX_LEN)
  {
    LM_X(1, ("dbName too long (max %d characters)", DB_NAME_MAX_LEN));
  }

  if ((strncmp(authMech, "SCRAM-SHA-1", strlen("SCRAM-SHA-1")) != 0) && (strncmp(authMech, "MONGODB-CR", strlen("MONGODB-CR")) != 0))
  {
    LM_X(1, ("Fatal Error (-dbAuthMech must be either SCRAM-SHA-1 or MONGODB-CR"));
  }

  if (useOnlyIPv6 && useOnlyIPv4)
  {
    LM_X(1, ("Fatal Error (-ipv4 and -ipv6 can not be activated at the same time. They are incompatible)"));
  }

  if (https)
  {
    if (httpsKeyFile[0] == 0)
    {
      LM_X(1, ("Fatal Error (when option '-https' is used, option '-key' is mandatory)"));
    }
    if (httpsCertFile[0] == 0)
    {
      LM_X(1, ("Fatal Error (when option '-https' is used, option '-cert' is mandatory)"));
    }
  }

  notificationModeParse(notificationMode, &notificationQueueSize, &notificationThreadNum); // This should be called before contextBrokerInit()
  LM_T(LmtNotifier, ("notification mode: '%s', queue size: %d, num threads %d", notificationMode, notificationQueueSize, notificationThreadNum));

  if ((strcmp(notifFlowControl, "") != 0) && (strcmp(notificationMode, "threadpool") != 0))
  {
      LM_X(1, ("Fatal Error ('-notifFlowControl' must be used in combination with threadpool notification mode)"));
  }

  if (strcmp(notifFlowControl, "") != 0)
  {
    fcEnabled = true;
    notifFlowControlParse(notifFlowControl, &fcGauge, &fcStepDelay, &fcMaxInterval);
    LM_T(LmtNotifier, ("notification flow control: enabled - gauge: %f, stepDelay: %d, maxInterval: %d", fcGauge, fcStepDelay, fcMaxInterval));
  }
  else
  {
    fcEnabled = false;
    LM_T(LmtNotifier, ("notification flow control: disabled"));
  }

  LM_I(("Orion Context Broker is running"));

  if (fg == false)
  {
    daemonize();
  }

  if ((s = pidFile(false)) != 0)
  {
    _exit(s);
  }

#if 0
  //
  // This 'almost always outdeffed' piece of code is used whenever a change is done to the
  // valgrind test suite, just to make sure that the tool actually detects memory leaks.
  //
  char* x = (char*) malloc(100000);
  snprintf(x, sizeof(x), "A hundred thousand bytes lost here");
  LM_M(("x: '%s'", x));  // Outdeffed
  x = (char*) "LOST";
  LM_M(("x: '%s'", x));  // Outdeffed
#endif

  IpVersion    ipVersion        = IPDUAL;

  if (useOnlyIPv4)
  {
    ipVersion = IPV4;
  }
  else if (useOnlyIPv6)
  {
    ipVersion = IPV6;
  }

  SemOpType policy = policyGet(reqMutexPolicy);
  orionInit(orionExit, ORION_VERSION, policy, statCounters, statSemWait, statTiming, statNotifQueue, strictIdv1);
  mongoInit(dbHost, rplSet, dbName, user, pwd, authMech, authDb, dbSSL, mtenant, dbTimeout, writeConcern, dbPoolSize, statSemWait);
  alarmMgr.init(relogAlarms);
  metricsMgr.init(!disableMetrics, statSemWait);
  logSummaryInit(&lsPeriod);

  // According to http://stackoverflow.com/questions/28048885/initializing-ssl-and-libcurl-and-getting-out-of-memory/37295100,
  // openSSL library needs to be initialized with SSL_library_init() before any use of it by any other libraries
  SSL_library_init();

  // Startup libcurl
  if (curl_global_init(CURL_GLOBAL_SSL) != 0)
  {
    LM_X(1, ("Fatal Error (could not initialize libcurl)"));
  }

  if (noCache == false)
  {
    subCacheInit(mtenant);

    if (subCacheInterval == 0)
    {
      // Populate subscription cache from database
      subCacheRefresh();
    }
    else
    {
      // Populate subscription cache AND start sub-cache-refresh-thread
      subCacheStart();
    }
  }
  else
  {
    LM_T(LmtSubCache, ("noCache == false"));
  }

  // Given that contextBrokerInit() may create thread (in the threadpool notification mode,
  // it has to be done before curl_global_init(), see https://curl.haxx.se/libcurl/c/threaded-ssl.html
  // Otherwise, we have empirically checked that CB may randomly crash
  contextBrokerInit(dbName, mtenant);

  if (https)
  {
    char* httpsPrivateServerKey = (char*) malloc(2048);
    char* httpsCertificate      = (char*) malloc(2048);

    if (loadFile(httpsKeyFile, httpsPrivateServerKey, 2048) != 0)
    {
      LM_X(1, ("Fatal Error (loading private server key from '%s')", httpsKeyFile));
    }
    if (loadFile(httpsCertFile, httpsCertificate, 2048) != 0)
    {
      LM_X(1, ("Fatal Error (loading certificate from '%s')", httpsCertFile));
    }

    LM_T(LmtHttps, ("httpsKeyFile:  '%s'", httpsKeyFile));
    LM_T(LmtHttps, ("httpsCertFile: '%s'", httpsCertFile));

    orionRestServicesInit(ipVersion,
                          bindAddress,
                          port,
                          mtenant,
                          connectionMemory,
                          maxConnections,
                          reqPoolSize,
                          allowedOrigin,
                          maxAge,
                          reqTimeout,
                          httpsPrivateServerKey,
                          httpsCertificate);
    
    free(httpsPrivateServerKey);
    free(httpsCertificate);
  }
  else
  {
    orionRestServicesInit(ipVersion,
                          bindAddress,
                          port,
                          mtenant,
                          connectionMemory,
                          maxConnections,
                          reqPoolSize,
                          allowedOrigin,
                          maxAge,
                          reqTimeout,
                          NULL,
                          NULL);
  }

  LM_I(("Startup completed"));
  if (simulatedNotification)
  {
    LM_W(("simulatedNotification is 'true', outgoing notifications won't be sent"));
  }

  while (1)
  {
    sleep(60);
  }
}
