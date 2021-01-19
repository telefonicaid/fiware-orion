/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
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
* After creating these two files, the broker can be started like this:
*   % orionld -fg -https -key server.key -cert server.pem
*
* The clients need to use the 'server.pem' file in the request:
* curl --cacert server.pem
*
*
* To override the security added with the certificate, curl can always be called using the
* CLI option '--insecure'.
*/
#include <stdio.h>
#include <unistd.h>                             // getppid, fork, setuid, sleep, etc.
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

extern "C"
{
#include "kalloc/kaBufferReset.h"                           // kaBufferReset
#include "kjson/kjFree.h"                                   // kjFree
}

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "parseArgs/paBuiltin.h"
#include "parseArgs/paIsSet.h"
#include "parseArgs/paUsage.h"
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

#include "common/string.h"
#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "logSummary/logSummary.h"

#include "orionld/common/orionldState.h"                    // orionldStateRelease, kalloc, ...
#include "orionld/common/branchName.h"                      // ORIONLD_BRANCH
#include "orionld/context/orionldContextCacheRelease.h"     // orionldContextCacheRelease
#include "orionld/rest/orionldServiceInit.h"                // orionldServiceInit
#include "orionld/db/dbInit.h"                              // dbInit
#include "orionld/mqtt/mqttRelease.h"                       // mqttRelease
#include "orionld/troe/troeInit.h"                          // troeInit

#include "orionld/version.h"
#include "orionld/orionRestServices.h"
#include "orionld/orionldRestServices.h"

#include "orionld/socketService/socketServiceInit.h"        // socketServiceInit
#include "orionld/socketService/socketServiceRun.h"         // socketServiceRun

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
char            dbHost[1024];
char            rplSet[64];
char            dbName[64];
char            dbUser[64];
char            dbPwd[64];
char            pidPath[256];
bool            harakiri;
bool            useOnlyIPv4;
bool            useOnlyIPv6;
char            httpsKeyFile[1024];
char            httpsCertFile[1024];
bool            https;
bool            multitenancy;
char            rush[256];
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
bool            disableMetrics;
int             reqTimeout;
bool            insecureNotif;
bool            ngsiv1Autocast;
int             contextDownloadAttempts;
int             contextDownloadTimeout;
bool            troe;
bool            disableFileLog;
bool            lmtmp;
char            troeHost[64];
unsigned short  troePort;
char            troeUser[64];
char            troePwd[64];
int             troePoolSize;
bool            socketService;
unsigned short  socketServicePort;
bool            mhdTurbo;



/* ****************************************************************************
*
* Definitions to make paArgs lines shorter ...
*/
#define PIDPATH                _i "/tmp/contextBroker.pid"
#define IP_ALL                 _i "0.0.0.0"
#define LOCALHOST              _i "localhost"
#define ONE_MONTH_PERIOD       (3600 * 24 * 31)

#define CTX_TMO_DESC           "Timeout in milliseconds for downloading of contexts"
#define CTX_ATT_DESC           "Number of attempts for downloading of contexts"
#define FG_DESC                "don't start as daemon"
#define LOCALIP_DESC           "IP to receive new connections"
#define PORT_DESC              "port to receive new connections"
#define PIDPATH_DESC           "pid file path"
#define DBHOST_DESC            "database host"
#define RPLSET_DESC            "replica set"
#define DBUSER_DESC            "database user"
#define DBPASSWORD_DESC        "database password"
#define DB_DESC                "database name"
#define DB_TMO_DESC            "timeout in milliseconds for connections to the replica set (ignored in the case of not using replica set)"
#define USEIPV4_DESC           "use ip v4 only"
#define USEIPV6_DESC           "use ip v6 only"
#define HARAKIRI_DESC          "commits harakiri on request"
#define HTTPS_DESC             "use the https 'protocol'"
#define HTTPSKEYFILE_DESC      "private server key file (for https)"
#define HTTPSCERTFILE_DESC     "certificate key file (for https)"
#define RUSH_DESC              "rush host (IP:port)"
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
#define NO_CACHE               "disable subscription cache for lookups"
#define CONN_MEMORY_DESC       "maximum memory size per connection (in kilobytes)"
#define MAX_CONN_DESC          "maximum number of simultaneous connections"
#define REQ_POOL_SIZE          "size of thread pool for incoming connections"
#define SIMULATED_NOTIF_DESC   "simulate notifications instead of actual sending them (only for testing)"
#define STAT_COUNTERS          "enable request/notification counters statistics"
#define STAT_SEM_WAIT          "enable semaphore waiting time statistics"
#define STAT_TIMING            "enable request-time-measuring statistics"
#define STAT_NOTIF_QUEUE       "enable thread pool notifications queue statistics"
#define LOG_SUMMARY_DESC       "log summary period in seconds (defaults to 0, meaning 'off')"
#define RELOGALARMS_DESC       "log messages for existing alarms beyond the raising alarm log message itself"
#define CHECK_v1_ID_DESC       "additional checks for id fields in the NGSIv1 API"
#define DISABLE_CUSTOM_NOTIF   "disable NGSIv2 custom notifications"
#define LOG_TO_SCREEN_DESC     "log to screen"
#define LOG_FOR_HUMANS_DESC    "human readible log to screen"
#define METRICS_DESC           "turn off the 'metrics' feature"
#define REQ_TMO_DESC           "connection timeout for REST requests (in seconds)"
#define INSECURE_NOTIF         "allow HTTPS notifications to peers which certificate cannot be authenticated with known CA certificates"
#define NGSIV1_AUTOCAST        "automatic cast for number, booleans and dates in NGSIv1 update/create attribute operations"
#define TROE_DESC              "enable TRoE - temporal representation of entities"
#define DISABLE_FILE_LOG       "disable logging into file"
#define TMPTRACES_DESC         "disable LM_TMP traces"
#define TROE_HOST_DESC         "host for troe database db server"
#define TROE_PORT_DESC         "port for troe database db server"
#define TROE_HOST_USER         "username for troe database db server"
#define TROE_HOST_PWD          "password for troe database db server"
#define TROE_POOL_DESC         "size of the connection pool for TRoE Postgres database connections"
#define SOCKET_SERVICE_DESC    "enable the socket service - accept connections via a normal TCP socket"
#define SOCKET_SERVICE_PORT_DESC  "port to receive new socket service connections"
#define MHDTURBO_DESC          "turn on MHD TURBO"



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
  { "-fg",                    &fg,                      "FOREGROUND",                PaBool,    PaOpt,  false,           false,  true,             FG_DESC                  },
  { "-localIp",               bindAddress,              "LOCALIP",                   PaString,  PaOpt,  IP_ALL,          PaNL,   PaNL,             LOCALIP_DESC             },
  { "-port",                  &port,                    "PORT",                      PaInt,     PaOpt,  1026,            PaNL,   PaNL,             PORT_DESC                },
  { "-pidpath",               pidPath,                  "PID_PATH",                  PaString,  PaOpt,  PIDPATH,         PaNL,   PaNL,             PIDPATH_DESC             },
  { "-dbhost",                dbHost,                   "MONGO_HOST",                PaString,  PaOpt,  LOCALHOST,       PaNL,   PaNL,             DBHOST_DESC              },
  { "-rplSet",                rplSet,                   "MONGO_REPLICA_SET",         PaString,  PaOpt,  _i "",           PaNL,   PaNL,             RPLSET_DESC              },
  { "-dbuser",                dbUser,                   "MONGO_USER",                PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBUSER_DESC              },
  { "-dbpwd",                 dbPwd,                    "MONGO_PASSWORD",            PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBPASSWORD_DESC          },
  { "-db",                    dbName,                   "MONGO_DB",                  PaString,  PaOpt,  _i "orion",      PaNL,   PaNL,             DB_DESC                  },
  { "-dbTimeout",             &dbTimeout,               "MONGO_TIMEOUT",             PaDouble,  PaOpt,  10000,           PaNL,   PaNL,             DB_TMO_DESC              },
  { "-dbPoolSize",            &dbPoolSize,              "MONGO_POOL_SIZE",           PaInt,     PaOpt,  10,              1,      10000,            DBPS_DESC                },
  { "-writeConcern",          &writeConcern,            "MONGO_WRITE_CONCERN",       PaInt,     PaOpt,  1,               0,      1,                WRITE_CONCERN_DESC       },
  { "-ipv4",                  &useOnlyIPv4,             "USEIPV4",                   PaBool,    PaOpt,  false,           false,  true,             USEIPV4_DESC             },
  { "-ipv6",                  &useOnlyIPv6,             "USEIPV6",                   PaBool,    PaOpt,  false,           false,  true,             USEIPV6_DESC             },
  { "-harakiri",              &harakiri,                "HARAKIRI",                  PaBool,    PaHid,  false,           false,  true,             HARAKIRI_DESC            },
  { "-https",                 &https,                   "HTTPS",                     PaBool,    PaOpt,  false,           false,  true,             HTTPS_DESC               },
  { "-key",                   httpsKeyFile,             "HTTPS_KEYFILE",             PaString,  PaOpt,  _i "",           PaNL,   PaNL,             HTTPSKEYFILE_DESC        },
  { "-cert",                  httpsCertFile,            "HTTPS_CERTFILE",            PaString,  PaOpt,  _i "",           PaNL,   PaNL,             HTTPSCERTFILE_DESC       },
  { "-rush",                  rush,                     "RUSH",                      PaString,  PaOpt,  _i "",           PaNL,   PaNL,             RUSH_DESC                },
  { "-multiservice",          &multitenancy,            "MULTI_SERVICE",             PaBool,    PaOpt,  false,           false,  true,             MULTISERVICE_DESC        },
  { "-httpTimeout",           &httpTimeout,             "HTTP_TIMEOUT",              PaLong,    PaOpt,  -1,              -1,     MAX_L,            HTTP_TMO_DESC            },
  { "-reqTimeout",            &reqTimeout,              "REQ_TIMEOUT",               PaLong,    PaOpt,   0,              0,      PaNL,             REQ_TMO_DESC             },
  { "-reqMutexPolicy",        reqMutexPolicy,           "MUTEX_POLICY",              PaString,  PaOpt,  _i "none",       PaNL,   PaNL,             MUTEX_POLICY_DESC        },
  { "-corsOrigin",            allowedOrigin,            "CORS_ALLOWED_ORIGIN",       PaString,  PaOpt,  _i "",           PaNL,   PaNL,             ALLOWED_ORIGIN_DESC      },
  { "-corsMaxAge",            &maxAge,                  "CORS_MAX_AGE",              PaInt,     PaOpt,  86400,           -1,     86400,            CORS_MAX_AGE_DESC        },
  { "-cprForwardLimit",       &cprForwardLimit,         "CPR_FORWARD_LIMIT",         PaUInt,    PaOpt,  1000,            0,      UINT_MAX,         CPR_FORWARD_LIMIT_DESC   },
  { "-subCacheIval",          &subCacheInterval,        "SUBCACHE_IVAL",             PaInt,     PaOpt,  0,               0,      3600,             SUB_CACHE_IVAL_DESC      },
  { "-noCache",               &noCache,                 "NOCACHE",                   PaBool,    PaOpt,  false,           false,  true,             NO_CACHE                 },
  { "-connectionMemory",      &connectionMemory,        "CONN_MEMORY",               PaUInt,    PaOpt,  64,              0,      1024,             CONN_MEMORY_DESC         },
  { "-maxConnections",        &maxConnections,          "MAX_CONN",                  PaUInt,    PaOpt,  1020,            1,      PaNL,             MAX_CONN_DESC            },
  { "-reqPoolSize",           &reqPoolSize,             "TRQ_POOL_SIZE",             PaUInt,    PaOpt,  0,               0,      1024,             REQ_POOL_SIZE            },
  { "-notificationMode",      &notificationMode,        "NOTIF_MODE",                PaString,  PaOpt,  _i "transient",  PaNL,   PaNL,             NOTIFICATION_MODE_DESC   },
  { "-simulatedNotification", &simulatedNotification,   "DROP_NOTIF",                PaBool,    PaOpt,  false,           false,  true,             SIMULATED_NOTIF_DESC     },
  { "-statCounters",          &statCounters,            "STAT_COUNTERS",             PaBool,    PaOpt,  false,           false,  true,             STAT_COUNTERS            },
  { "-statSemWait",           &statSemWait,             "STAT_SEM_WAIT",             PaBool,    PaOpt,  false,           false,  true,             STAT_SEM_WAIT            },
  { "-statTiming",            &statTiming,              "STAT_TIMING",               PaBool,    PaOpt,  false,           false,  true,             STAT_TIMING              },
  { "-statNotifQueue",        &statNotifQueue,          "STAT_NOTIF_QUEUE",          PaBool,    PaOpt,  false,           false,  true,             STAT_NOTIF_QUEUE         },
  { "-logSummary",            &lsPeriod,                "LOG_SUMMARY_PERIOD",        PaInt,     PaOpt,  0,               0,      ONE_MONTH_PERIOD, LOG_SUMMARY_DESC         },
  { "-relogAlarms",           &relogAlarms,             "RELOG_ALARMS",              PaBool,    PaOpt,  false,           false,  true,             RELOGALARMS_DESC         },
  { "-strictNgsiv1Ids",       &strictIdv1,              "CHECK_ID_V1",               PaBool,    PaOpt,  false,           false,  true,             CHECK_v1_ID_DESC         },
  { "-disableCustomNotifications",  &disableCusNotif,   "DISABLE_CUSTOM_NOTIF",      PaBool,    PaOpt,  false,           false,  true,             DISABLE_CUSTOM_NOTIF     },
  { "-logForHumans",          &logForHumans,            "LOG_FOR_HUMANS",            PaBool,    PaOpt,  false,           false,  true,             LOG_FOR_HUMANS_DESC      },
  { "-disableFileLog",        &disableFileLog,          "DISABLE_FILE_LOG",          PaBool,    PaOpt,  false,           false,  true,             DISABLE_FILE_LOG         },
  { "-disableMetrics",        &disableMetrics,          "DISABLE_METRICS",           PaBool,    PaOpt,  false,           false,  true,             METRICS_DESC             },
  { "-insecureNotif",         &insecureNotif,           "INSECURE_NOTIF",            PaBool,    PaOpt,  false,           false,  true,             INSECURE_NOTIF           },
  { "-ngsiv1Autocast",        &ngsiv1Autocast,          "NGSIV1_AUTOCAST",           PaBool,    PaOpt,  false,           false,  true,             NGSIV1_AUTOCAST          },
  { "-ctxTimeout",            &contextDownloadTimeout,  "CONTEXT_DOWNLOAD_TIMEOUT",  PaInt,     PaOpt,  5000,            0,      20000,            CTX_TMO_DESC             },
  { "-ctxAttempts",           &contextDownloadAttempts, "CONTEXT_DOWNLOAD_ATTEMPTS", PaInt,     PaOpt,  3,               0,      100,              CTX_ATT_DESC             },
  { "-troe",                  &troe,                    "TROE",                      PaBool,    PaOpt,  false,           false,  true,             TROE_DESC                },
  { "-lmtmp",                 &lmtmp,                   "TMP_TRACES",                PaBool,    PaHid,  true,            false,  true,             TMPTRACES_DESC           },
  { "-socketService",         &socketService,           "SOCKET_SERVICE",            PaBool,    PaHid,  false,           false,  true,             SOCKET_SERVICE_DESC      },
  { "-troeHost",              troeHost,                 "TROE_HOST",                 PaString,  PaOpt,  _i "localhost",  PaNL,   PaNL,             TROE_HOST_DESC           },
  { "-troePort",              &troePort,                "TROE_PORT",                 PaInt,     PaOpt,  5432,            PaNL,   PaNL,             TROE_PORT_DESC           },
  { "-troeUser",              troeUser,                 "TROE_USER",                 PaString,  PaOpt,  _i "postgres",   PaNL,   PaNL,             TROE_HOST_USER           },
  { "-troePwd",               troePwd,                  "TROE_PWD",                  PaString,  PaOpt,  _i "password",   PaNL,   PaNL,             TROE_HOST_PWD            },
  { "-troePoolSize",          &troePoolSize,            "TROE_POOL_SIZE",            PaInt,     PaOpt,  10,              0,      1000,             TROE_POOL_DESC           },
  { "-ssPort",                &socketServicePort,       "SOCKET_SERVICE_PORT",       PaUShort,  PaHid,  1027,            PaNL,   PaNL,             SOCKET_SERVICE_PORT_DESC },
  { "-mhdTurbo",              &mhdTurbo,                "MHD_TURBO",                 PaBool,    PaHid,  false,           false,  true,             MHDTURBO_DESC            },

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
  LM_I(("Signal Handler (caught signal %d)", sigNo));

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

  orionldStateRelease();

  //
  // Contexts that have been cloned must be freed
  //
  orionldContextCacheRelease();

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

  //
  // Free the context cache ?
  // Or, is freeing up the global KAlloc instance sufficient ... ?
  //

  //
  // Free the kalloc buffer
  //
  kaBufferReset(&kalloc, false);

  if (unlink(pidPath) != 0)
  {
    LM_T(LmtSoftError, ("error removing PID file '%s': %s", pidPath, strerror(errno)));
  }

  // Free the tenant list
  for (unsigned int ix = 0; ix < tenants; ix++)
    free(tenantV[ix]);

  // Disconnect from all MQTT btokers and free the connections
  mqttRelease();
}



/* ****************************************************************************
*
* description -
*/
const char* description =
  "\n"
  "Orion-LD context broker version details:\n"
  "  orionld version:    " ORIONLD_VERSION "\n"
  "  orion version:      " ORION_VERSION   "\n"
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
* rushParse - parse rush host and port from CLI argument
*
* The '-rush' CLI argument has the format "host:port" and this function
* splits that argument into rushHost and rushPort.
* If there is a syntax error in the argument, the function exists the program
* with an error message
*/
static void rushParse(char* rush, std::string* rushHostP, uint16_t* rushPortP)
{
  char* colon = strchr(rush, ':');
  char* copy  = strdup(rush);

  if (colon == NULL)
  {
    LM_X(1, ("Fatal Error (Bad syntax of '-rush' value: '%s' - expected syntax: 'host:port')", rush));
  }

  *colon = 0;
  ++colon;

  *rushHostP = rush;
  *rushPortP = atoi(colon);

  if ((*rushHostP == "") || (*rushPortP == 0))
  {
    LM_X(1, ("Fatal Error (bad syntax of '-rush' value: '%s' - expected syntax: 'host:port')", copy));
  }

  free(copy);
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



// -----------------------------------------------------------------------------
//
// versionInfo -
//
static void versionInfo(void)
{
  LM_K(("Version Info:"));
  LM_K(("-----------------------------------------"));
  LM_K(("orionld version:    %s", orionldVersion));
  LM_K(("based on orion:     %s", ORION_VERSION));
  LM_K(("git hash:           %s", GIT_HASH));
  LM_K(("build branch:       %s", ORIONLD_BRANCH));
  LM_K(("compiled by:        %s", COMPILED_BY));
  LM_K(("compiled in:        %s", COMPILED_IN));
  LM_K(("-----------------------------------------"));
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

  uint16_t       rushPort = 0;
  std::string    rushHost = "";

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
  paConfig("man author",                    (void*) "Telefonica I+D and FIWARE Foundation");
  paConfig("man version",                   (void*) versionString.c_str());
  paConfig("log file line format",          (void*) LOG_FILE_LINE_FORMAT);
  paConfig("log file time format",          (void*) "%Y-%m-%dT%H:%M:%S");
  paConfig("builtin prefix",                (void*) "ORIONLD_");
  paConfig("prefix",                        (void*) "ORIONLD_");
  paConfig("usage and exit on any warning", (void*) true);
  paConfig("no preamble",                   NULL);
  paConfig("valid log level strings",       validLogLevels);
  paConfig("default value",                 "-logLevel", "WARN");


  //
  // If option '-disableFileLog' is set, no log to file
  // If option '-fg' is set, print traces to stdout as well, otherwise, only to file (unless -disableFileLog is set)
  // A combination of the two makes the broker run in foreground (not as a daemon) and printing traces to stdout only
  //
  fg             = paIsSet(argC, argV, paArgs, "-fg");
  disableFileLog = paIsSet(argC, argV, paArgs, "-disableFileLog");

  if (disableFileLog && fg)
  {
    paConfig("log to screen", (void*) true);
    paConfig("log to file",   (void*) false);
  }
  else if (fg)
  {
    paConfig("log to screen", (void*) true);
    paConfig("log to file",   (void*) true);
  }
  else if (disableFileLog)
  {
    paConfig("log to screen", (void*) false);
    paConfig("log to file",   (void*) false);
  }
  else
  {
    paConfig("log to file",   (void*) true);
  }

  if (paIsSet(argC, argV, paArgs, "-logForHumans"))
  {
    paConfig("screen line format", (void*) "TYPE@TIME  FILE[LINE]: TEXT");
  }
  else
  {
    paConfig("screen line format", LOG_FILE_LINE_FORMAT);
  }

  //
  // If trace levels are set, turn set logLevel to DEBUG, so that the trace messages will actually pass through
  //
  if (paIsSet(argC, argV, paArgs, "-t"))
    strncpy(paLogLevel, "DEBUG", sizeof(paLogLevel));

  paParse(paArgs, argC, (char**) argV, 1, false);
  lmTimeFormat(0, (char*) "%Y-%m-%dT%H:%M:%S");


  //
  // Set portNo
  //
  portNo = port;


  //
  // Set global variables from the arguments
  //
  dbNameLen = strlen(dbName);


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

  LM_I(("Orion Context Broker is running"));

  versionInfo();

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

  IpVersion ipVersion = IPDUAL;

  if (useOnlyIPv4)
    ipVersion = IPV4;
  else if (useOnlyIPv6)
    ipVersion = IPV6;

  SemOpType policy = policyGet(reqMutexPolicy);
  orionInit(orionExit, ORION_VERSION, policy, statCounters, statSemWait, statTiming, statNotifQueue, strictIdv1);

  mongoInit(dbHost, rplSet, dbName, dbUser, dbPwd, multitenancy, dbTimeout, writeConcern, dbPoolSize, statSemWait);
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

  if (rush[0] != 0)
  {
    rushParse(rush, &rushHost, &rushPort);
    LM_T(LmtRush, ("rush host: '%s', rush port: %d", rushHost.c_str(), rushPort));
  }

  if (noCache == false)
  {
    subCacheInit(multitenancy);

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

  //
  // If the Env Var ORIONLD_CACHED_CONTEXT_DIRECTORY is set, then at startup, the broker will read all context files
  // inside that directory and add them to the linked list of "downloaded" contexts.
  //
  // The main reason behind this "feature" is that functional tests take a lot less time to execute.
  //
  // Another reason may be to "shadow" the important default and core contexts, also for testing
  //
  // Actually, another interesting usage is that a functional test case could create a context before starting the broker and
  // use that conext during the test.
  //

  //
  // Initialize orionld
  //
  orionldServiceInit(restServiceVV, 9, getenv("ORIONLD_CACHED_CONTEXT_DIRECTORY"));
  dbInit(dbHost, dbName);

  //
  // The database for Temporal Representation of Entities must be initialized before mongodb
  // as callbacks to create tenants (== postgres databases) and their tables are called from the
  // initialization routines of mongodb - if postgres is not initialized, this will fail.
  //
  if (troe)
  {
    if (troeInit() == false)
      LM_X(1, ("Database Error (unable to initialize the layer for Temporal Representation of Entities)"));
  }

  //
  // Given that contextBrokerInit() may create thread (in the threadpool notification mode,
  // it has to be done before curl_global_init(), see https://curl.haxx.se/libcurl/c/threaded-ssl.html
  // Otherwise, we have empirically checked that CB may randomly crash
  //
  contextBrokerInit(dbName, multitenancy);

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
                          multitenancy,
                          connectionMemory,
                          maxConnections,
                          reqPoolSize,
                          rushHost,
                          rushPort,
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
                          multitenancy,
                          connectionMemory,
                          maxConnections,
                          reqPoolSize,
                          rushHost,
                          rushPort,
                          allowedOrigin,
                          maxAge,
                          reqTimeout,
                          NULL,
                          NULL);
  }

  LM_I(("Startup completed"));
  orionldPhase = OrionldPhaseServing;

  if (simulatedNotification)
  {
    LM_W(("simulatedNotification is 'true', outgoing notifications won't be sent"));
  }

  LM_K(("Initialization ready - accepting REST requests on port %d", port));

  if (socketService == true)
  {
    int fd;

    if ((fd = socketServiceInit(socketServicePort)) == -1)
      LM_X(1, ("Can't initialize socketService"));
    LM_K(("Initialization ready - accepting SOCKET requests on port %d", socketServicePort));

    socketServiceRun(fd);
    LM_X(1, ("Socket Service terminated"));
  }
  else
  {
    while (1)
    {
      sleep(60);
    }
  }
}
