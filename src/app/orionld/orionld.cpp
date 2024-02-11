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
#include <unistd.h>                                         // getppid, fork, setuid, sleep, gethostname, etc.
#include <string.h>
#include <fcntl.h>                                          // open
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <string>
#include <vector>
#include <limits.h>
#include <sys/mman.h>                                       // mlockall
#include <mongo/version.h>                                  // MONGOCLIENT_VERSION

#include "mongoBackend/MongoGlobal.h"
#include "cache/subCache.h"

extern "C"
{
#include "kbase/kInit.h"                                    // kInit
#include "kalloc/kaInit.h"                                  // kaInit
#include "kalloc/kaBufferInit.h"                            // kaBufferInit
#include "kalloc/kaBufferReset.h"                           // kaBufferReset
#include "kjson/kjBufferCreate.h"                           // kjBufferCreate
#include "kjson/kjFree.h"                                   // kjFree
#include "kjson/kjLookup.h"                                 // kjLookup
}

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "parseArgs/paBuiltin.h"
#include "parseArgs/paIsSet.h"
#include "parseArgs/paUsage.h"
#include "logMsg/logMsg.h"

#include "rest/httpRequestSend.h"
#include "common/compileInfo.h"
#include "ngsiNotify/QueueNotifier.h"
#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "logSummary/logSummary.h"

#include "orionld/common/orionldTenantInit.h"                 // orionldTenantInit
#include "orionld/common/orionldState.h"                      // orionldStateRelease, kalloc, ...
#include "orionld/common/tenantList.h"                        // tenantList, tenant0
#include "orionld/common/branchName.h"                        // ORIONLD_BRANCH
#include "orionld/prometheus/promInit.h"                      // promInit
#include "orionld/mongoc/mongocInit.h"                        // mongocInit
#include "orionld/mongoc/mongocServerVersionGet.h"            // mongocServerVersionGet
#include "orionld/context/orionldCoreContext.h"               // ORIONLD_CORE_CONTEXT_URL_*
#include "orionld/context/orionldContextFromUrl.h"            // contextDownloadListInit, contextDownloadListRelease
#include "orionld/contextCache/orionldContextCacheRelease.h"  // orionldContextCacheRelease
#include "orionld/service/orionldServiceInit.h"               // orionldServiceInit
#include "orionld/entityMaps/entityMapsRelease.h"             // entityMapsRelease
#include "orionld/db/dbInit.h"                                // dbInit
#include "orionld/mqtt/mqttRelease.h"                         // mqttRelease
#include "orionld/regCache/regCacheInit.h"                    // regCacheInit
#include "orionld/regCache/regCacheCreate.h"                  // regCacheCreate
#include "orionld/regCache/regCacheRelease.h"                 // regCacheRelease
#include "orionld/pernot/pernotSubCacheInit.h"                // pernotSubCacheInit
#include "orionld/pernot/pernotLoop.h"                        // pernotLoopStart
#include "orionld/pernot/pernotRelease.h"                     // pernotRelease

#include "orionld/version.h"
#include "orionld/orionRestServices.h"
#include "orionld/orionldRestServices.h"

#include "orionld/mongoc/mongocServerVersionGet.h"            // mongocServerVersionGet
#include "orionld/context/orionldContextFromUrl.h"            // contextDownloadListInit, contextDownloadListRelease
#include "orionld/socketService/socketServiceInit.h"          // socketServiceInit
#include "orionld/socketService/socketServiceRun.h"           // socketServiceRun

#include "orionld/troe/troeInit.h"                            // troeInit
#include "orionld/troe/pgVersionGet.h"                        // pgVersionGet
#include "orionld/troe/pgConnectionPoolsFree.h"               // pgConnectionPoolsFree
#include "orionld/troe/pgConnectionPoolsPresent.h"            // pgConnectionPoolsPresent
#include "orionld/distOp/distOpInit.h"                        // distOpInit

#include "orionld/version.h"
#include "orionld/orionRestServices.h"
#include "orionld/orionldRestServices.h"

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
static bool      isFatherProcess = false;
char             localIpAndPort[135];



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
char            dbPwd[512];
char            dbAuthDb[64];
char            dbAuthMechanism[64];
bool            dbSSL;
char            dbCertFile[256];
char            dbURI[1024];
char            pidPath[256];
bool            harakiri;
bool            useOnlyIPv4;
bool            useOnlyIPv6;
char            httpsKeyFile[1024];
char            httpsCertFile[1024];
bool            https;
bool            multitenancy;
char            allowedOrigin[64];
int             maxAge;
long            dbTimeout;
long            httpTimeout;
int             dbPoolSize;
char            reqMutexPolicy[16];
int             writeConcern;
unsigned int    cprForwardLimit;
int             subCacheInterval;
int             subCacheFlushInterval;
char            notificationMode[64];
int             notificationQueueSize;
int             notificationThreadNum;
bool            noCache;
unsigned int    connectionMemory;
unsigned int    maxConnections;
unsigned int    reqPoolSize;

unsigned long long  inReqPayloadMaxSize;
unsigned long long  outReqMsgMaxSize;

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
bool            pernot;
bool            disableFileLog;
bool            lmtmp;
char            troeHost[256];
unsigned short  troePort;
char            troeUser[256];
char            troePwd[256];
int             troePoolSize;
bool            socketService;
unsigned short  socketServicePort;
bool            distributed;
char            brokerId[136];
char            wip[512];
bool            noNotifyFalseUpdate;
bool            idIndex;
bool            noswap;
bool            experimental = false;
bool            mongocOnly   = false;
bool            debugCurl    = false;
uint32_t        cSubCounters;
char            coreContextVersion[64];
bool            triggerOperation = false;
bool            noprom           = false;



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
#define MULTISERVICE_DESC      "service multi tenancy mode"
#define ALLOWED_ORIGIN_DESC    "enable Cross-Origin Resource Sharing with allowed origin. Use '__ALL' for any"
#define CORS_MAX_AGE_DESC      "maximum time in seconds preflight requests are allowed to be cached. Default: 86400"
#define HTTP_TMO_DESC          "timeout in milliseconds for distributed requests and notifications"
#define DBPS_DESC              "database connection pool size"
#define MAX_L                  900000
#define MUTEX_POLICY_DESC      "mutex policy (none/read/write/all)"
#define WRITE_CONCERN_DESC     "db write concern (0:unacknowledged, 1:acknowledged)"
#define CPR_FORWARD_LIMIT_DESC "maximum number of distributed requests to Context Providers for a single client request"
#define SUB_CACHE_IVAL_DESC    "interval in seconds between calls to Subscription Cache refresh (0: no refresh)"
#define SUB_CACHE_FLUSH_IVAL_DESC    "interval in seconds between calls to Pernot Subscription Cache Flush to DB (0: no flush)"
#define NOTIFICATION_MODE_DESC "notification mode (persistent|transient|threadpool:q:n)"
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
#define LOG_TO_SCREEN_DESC     "log to screen"
#define LOG_FOR_HUMANS_DESC    "human readible log to screen"
#define METRICS_DESC           "turn off the 'metrics' feature"
#define REQ_TMO_DESC           "connection timeout for REST requests (in seconds)"
#define INSECURE_NOTIF         "allow HTTPS notifications to peers which certificate cannot be authenticated with known CA certificates"
#define NGSIV1_AUTOCAST        "automatic cast for number, booleans and dates in NGSIv1 update/create attribute operations"
#define TROE_DESC              "enable TRoE - temporal representation of entities"
#define PERNOT_DESC            "enable Pernot - Periodic Notifications"
#define DISABLE_FILE_LOG       "disable logging into file"
#define TMPTRACES_DESC         "disable traces"
#define TROE_HOST_DESC         "host for troe database db server"
#define TROE_PORT_DESC         "port for troe database db server"
#define TROE_HOST_USER         "username for troe database db server"
#define TROE_HOST_PWD          "password for troe database db server"
#define TROE_POOL_DESC         "size of the connection pool for TRoE Postgres database connections"
#define SOCKET_SERVICE_DESC    "enable the socket service - accept connections via a normal TCP socket"
#define SOCKET_SERVICE_PORT_DESC  "port to receive new socket service connections"
#define DISTRIBUTED_DESC       "turn on distributed operation"
#define BROKER_ID_DESC         "identity of this broker instance for registrations - for the Via header"
#define WIP_DESC               "Enable concepts that are 'Work In Progress' (e.g. -wip entityMaps)"
#define FORWARDING_DESC        "turn on distributed operation (deprecated)"
#define ID_INDEX_DESC          "automatic mongo index on _id.id"
#define NOSWAP_DESC            "no swapping - for testing only!!!"
#define NO_NOTIFY_FALSE_UPDATE_DESC  "turn off notifications on non-updates"
#define TRIGGER_OPERATION_DESC "include the operation that triggered the notification"
#define EXPERIMENTAL_DESC      "enable experimental implementation - use at own risk - see release notes of Orion-LD v1.1.0"
#define MONGOCONLY_DESC        "enable experimental implementation + turn off mongo legacy driver"
#define DBAUTHDB_DESC          "database used for authentication"
#define DBAUTHMECHANISM_DESC   "database authentication mechanism (either SCRAM-SHA-1 or SCRAM-SHA-256)"
#define DBSSL_DESC             "enable SSL connection to DB"
#define DBCERTFILE_DESC        "path to TLS certificate file"
#define DBURI_DESC             "complete URI for database connection"
#define DEBUG_CURL_DESC        "turn on debugging of libcurl - to the broker's logfile"
#define CSUBCOUNTERS_DESC      "number of subscription counter updates before flush from sub-cache to DB (0: never, 1: always)"
#define CORE_CONTEXT_DESC      "core context version (v1.0|v1.3|v1.4|v1.5|v1.6|v1.7) - v1.6 is default"
#define NO_PROM_DESC           "run without Prometheus metrics"



// -----------------------------------------------------------------------------
//
// MB - megabytes
//
#define MB(mbs) (1024 * 1024 * mbs)



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
  { "-coreContext",           coreContextVersion,       "CORE_CONTEXT",              PaString,  PaOpt,  _i ORIONLD_CORE_CONTEXT_URL_DEFAULT, PaNL, PaNL, CORE_CONTEXT_DESC },

  { "-fg",                    &fg,                      "FOREGROUND",                PaBool,    PaOpt,  false,           false,  true,             FG_DESC                  },
  { "-localIp",               bindAddress,              "LOCALIP",                   PaString,  PaOpt,  IP_ALL,          PaNL,   PaNL,             LOCALIP_DESC             },
  { "-port",                  &port,                    "PORT",                      PaInt,     PaOpt,  1026,            1024,   65535,            PORT_DESC                },
  { "-pidpath",               pidPath,                  "PID_PATH",                  PaString,  PaOpt,  PIDPATH,         PaNL,   PaNL,             PIDPATH_DESC             },
  { "-dbhost",                dbHost,                   "MONGO_HOST",                PaString,  PaOpt,  LOCALHOST,       PaNL,   PaNL,             DBHOST_DESC              },
  { "-rplSet",                rplSet,                   "MONGO_REPLICA_SET",         PaString,  PaOpt,  _i "",           PaNL,   PaNL,             RPLSET_DESC              },
  { "-dbuser",                dbUser,                   "MONGO_USER",                PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBUSER_DESC              },
  { "-dbpwd",                 dbPwd,                    "MONGO_PASSWORD",            PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBPASSWORD_DESC          },
  { "-dbAuthMech",            dbAuthMechanism,          "MONGO_AUTH_MECH",           PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBAUTHMECHANISM_DESC     },
  { "-dbAuthDb",              dbAuthDb,                 "MONGO_AUTH_SOURCE",         PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBAUTHDB_DESC            },
  { "-dbSSL",                 &dbSSL,                   "MONGO_SSL",                 PaBool,    PaOpt,  false,           false,  true,             DBSSL_DESC               },
  { "-dbCertFile",            &dbCertFile,              "MONGO_CERT_FILE",           PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBCERTFILE_DESC          },
  { "-dbURI",                 &dbURI,                   "MONGO_URI",                 PaString,  PaOpt,  _i "",           PaNL,   PaNL,             DBURI_DESC               },
  { "-db",                    dbName,                   "MONGO_DB",                  PaString,  PaOpt,  _i "orion",      PaNL,   PaNL,             DB_DESC                  },
  { "-dbTimeout",             &dbTimeout,               "MONGO_TIMEOUT",             PaDouble,  PaOpt,  10000,           PaNL,   PaNL,             DB_TMO_DESC              },
  { "-dbPoolSize",            &dbPoolSize,              "MONGO_POOL_SIZE",           PaInt,     PaOpt,  10,              1,      10000,            DBPS_DESC                },
  { "-writeConcern",          &writeConcern,            "MONGO_WRITE_CONCERN",       PaInt,     PaOpt,  1,               0,      1,                WRITE_CONCERN_DESC       },
  { "-ipv4",                  &useOnlyIPv4,             "USEIPV4",                   PaBool,    PaOpt,  false,           false,  true,             USEIPV4_DESC             },
  { "-ipv6",                  &useOnlyIPv6,             "USEIPV6",                   PaBool,    PaOpt,  false,           false,  true,             USEIPV6_DESC             },
  { "-https",                 &https,                   "HTTPS",                     PaBool,    PaOpt,  false,           false,  true,             HTTPS_DESC               },
  { "-key",                   httpsKeyFile,             "HTTPS_KEYFILE",             PaString,  PaOpt,  _i "",           PaNL,   PaNL,             HTTPSKEYFILE_DESC        },
  { "-cert",                  httpsCertFile,            "HTTPS_CERTFILE",            PaString,  PaOpt,  _i "",           PaNL,   PaNL,             HTTPSCERTFILE_DESC       },
  { "-multiservice",          &multitenancy,            "MULTI_SERVICE",             PaBool,    PaOpt,  false,           false,  true,             MULTISERVICE_DESC        },
  { "-httpTimeout",           &httpTimeout,             "HTTP_TIMEOUT",              PaLong,    PaOpt,  -1,              -1,     MAX_L,            HTTP_TMO_DESC            },
  { "-reqTimeout",            &reqTimeout,              "REQ_TIMEOUT",               PaLong,    PaOpt,   0,              0,      PaNL,             REQ_TMO_DESC             },
  { "-reqMutexPolicy",        reqMutexPolicy,           "MUTEX_POLICY",              PaString,  PaOpt,  _i "none",       PaNL,   PaNL,             MUTEX_POLICY_DESC        },
  { "-corsOrigin",            allowedOrigin,            "CORS_ALLOWED_ORIGIN",       PaString,  PaOpt,  _i "",           PaNL,   PaNL,             ALLOWED_ORIGIN_DESC      },
  { "-corsMaxAge",            &maxAge,                  "CORS_MAX_AGE",              PaInt,     PaOpt,  86400,           -1,     86400,            CORS_MAX_AGE_DESC        },
  { "-cprForwardLimit",       &cprForwardLimit,         "CPR_FORWARD_LIMIT",         PaUInt,    PaOpt,  1000,            0,      UINT_MAX,         CPR_FORWARD_LIMIT_DESC   },
  { "-subCacheIval",          &subCacheInterval,        "SUBCACHE_IVAL",             PaInt,     PaOpt,  0,               0,      3600,             SUB_CACHE_IVAL_DESC      },
  { "-subCacheFlushIval",     &subCacheFlushInterval,   "SUBCACHE_FLUSH_IVAL",       PaInt,     PaOpt,  10,              0,      3600,             SUB_CACHE_FLUSH_IVAL_DESC },
  { "-noCache",               &noCache,                 "NOCACHE",                   PaBool,    PaOpt,  false,           false,  true,             NO_CACHE                 },
  { "-connectionMemory",      &connectionMemory,        "CONN_MEMORY",               PaUInt,    PaOpt,  64,              0,      1024,             CONN_MEMORY_DESC         },
  { "-maxConnections",        &maxConnections,          "MAX_CONN",                  PaUInt,    PaOpt,  1020,            1,      PaNL,             MAX_CONN_DESC            },
  { "-reqPoolSize",           &reqPoolSize,             "TRQ_POOL_SIZE",             PaUInt,    PaOpt,  0,               0,      1024,             REQ_POOL_SIZE            },

  { "-inReqPayloadMaxSize",   &inReqPayloadMaxSize,     "IN_REQ_PAYLOAD_MAX_SIZE",   PaULong,   PaOpt,  MB(1),           0,      PaNL,             IN_REQ_PAYLOAD_MAX_SIZE_DESC },
  { "-outReqMsgMaxSize",      &outReqMsgMaxSize,        "OUT_REQ_MSG_MAX_SIZE",      PaULong,   PaOpt,  MB(8),           0,      PaNL,             OUT_REQ_MSG_MAX_SIZE_DESC    },
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
  { "-pernot",                &pernot,                  "PERNOT",                    PaBool,    PaOpt,  false,           false,  true,             PERNOT_DESC              },
  { "-troe",                  &troe,                    "TROE",                      PaBool,    PaOpt,  false,           false,  true,             TROE_DESC                },
  { "-troeHost",              troeHost,                 "TROE_HOST",                 PaString,  PaOpt,  _i "localhost",  PaNL,   PaNL,             TROE_HOST_DESC           },
  { "-troePort",              &troePort,                "TROE_PORT",                 PaInt,     PaOpt,  5432,            PaNL,   PaNL,             TROE_PORT_DESC           },
  { "-troeUser",              troeUser,                 "TROE_USER",                 PaString,  PaOpt,  _i "postgres",   PaNL,   PaNL,             TROE_HOST_USER           },
  { "-troePwd",               troePwd,                  "TROE_PWD",                  PaString,  PaOpt,  _i "password",   PaNL,   PaNL,             TROE_HOST_PWD            },
  { "-troePoolSize",          &troePoolSize,            "TROE_POOL_SIZE",            PaInt,     PaOpt,  10,              0,      1000,             TROE_POOL_DESC           },
  { "-noNotifyFalseUpdate",   &noNotifyFalseUpdate,     "NO_NOTIFY_FALSE_UPDATE",    PaBool,    PaOpt,  false,           false,  true,             NO_NOTIFY_FALSE_UPDATE_DESC  },
  { "-experimental",          &experimental,            "EXPERIMENTAL",              PaBool,    PaOpt,  false,           false,  true,             EXPERIMENTAL_DESC        },
  { "-mongocOnly",            &mongocOnly,              "MONGOCONLY",                PaBool,    PaOpt,  false,           false,  true,             MONGOCONLY_DESC          },
  { "-cSubCounters",          &cSubCounters,            "CSUB_COUNTERS",             PaInt,     PaOpt,  20,              0,      PaNL,             CSUBCOUNTERS_DESC        },
  { "-distributed",           &distributed,             "DISTRIBUTED",               PaBool,    PaOpt,  false,           false,  true,             DISTRIBUTED_DESC         },
  { "-brokerId",              &brokerId,                "BROKER_ID",                 PaStr,     PaOpt,  _i "",           PaNL,   PaNL,             BROKER_ID_DESC           },
  { "-wip",                   wip,                      "WIP",                       PaStr,     PaHid,  _i "",           PaNL,   PaNL,             WIP_DESC                 },
  { "-triggerOperation",      &triggerOperation,        "TRIGGER_OPERATION",         PaBool,    PaHid,  false,           false,  true,             TRIGGER_OPERATION_DESC   },
  { "-forwarding",            &distributed,             "FORWARDING",                PaBool,    PaHid,  false,           false,  true,             FORWARDING_DESC          },
  { "-socketService",         &socketService,           "SOCKET_SERVICE",            PaBool,    PaHid,  false,           false,  true,             SOCKET_SERVICE_DESC      },
  { "-ssPort",                &socketServicePort,       "SOCKET_SERVICE_PORT",       PaUShort,  PaHid,  1027,            PaNL,   PaNL,             SOCKET_SERVICE_PORT_DESC },
  { "-harakiri",              &harakiri,                "HARAKIRI",                  PaBool,    PaHid,  false,           false,  true,             HARAKIRI_DESC            },
  { "-idIndex",               &idIndex,                 "MONGO_ID_INDEX",            PaBool,    PaHid,  false,           false,  true,             ID_INDEX_DESC            },
  { "-noswap",                &noswap,                  "NOSWAP",                    PaBool,    PaHid,  false,           false,  true,             NOSWAP_DESC              },
  { "-lmtmp",                 &lmtmp,                   "TMP_TRACES",                PaBool,    PaHid,  true,            false,  true,             TMPTRACES_DESC           },
  { "-debugCurl",             &debugCurl,               "DEBUG_CURL",                PaBool,    PaHid,  false,           false,  true,             DEBUG_CURL_DESC          },
  { "-lmtmp",                 &lmtmp,                   "TMP_TRACES",                PaBool,    PaHid,  true,            false,  true,             TMPTRACES_DESC           },
  { "-noprom",                &noprom,                  "NO_PROM",                   PaBool,    PaHid,  false,           false,  true,             NO_PROM_DESC             },

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



// -----------------------------------------------------------------------------
//
// daemonize -
//
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
  reqSemTryToTake();
  subCacheDestroy();
#endif

  metricsMgr.release();

  curl_context_cleanup();
  curl_global_cleanup();

  //
  // Free the context cache ?
  // Or, is freeing up the global KAlloc instance sufficient ... ?
  //

  // Free up the context download list, if needed
  contextDownloadListRelease();

  //
  // Contexts that have been cloned must be freed
  //
  orionldContextCacheRelease();

  // Free the tenant list
  OrionldTenant* tenantP = tenantList;
  while (tenantP != NULL)
  {
    OrionldTenant* next = tenantP->next;

    if (tenantP->regCache != NULL)
      regCacheRelease(tenantP->regCache);

    free(tenantP);
    tenantP = next;
  }

  if (tenant0.regCache != NULL)
      regCacheRelease(tenant0.regCache);

  // Disconnect from all MQTT brokers and free the connections
  mqttRelease();

  //
  // Freeing the postgres connection pools
  //
  if (troe)
  {
    pgConnectionPoolsPresent();
    pgConnectionPoolsFree();
  }

  // Cleanup entity maps
  if (entityMaps != NULL)
    entityMapsRelease();

  // Cleanup periodic notifications
  if (pernot == true)
    pernotRelease();

  kaBufferReset(&kalloc, KFALSE);
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
      LM_X(1, ("Runtime Error starting notification queue workers (%d)", rc));

    pNotifier = pQNotifier;
  }
  else
    pNotifier = new Notifier();

  /* Set notifier object (singleton) */
  setNotifier(pNotifier);

  /* Set HTTP timeout */
  httpRequestInit(httpTimeout);
}



/* ****************************************************************************
*
* loadFile -
*/
static char* loadFile(char* path)
{
  struct stat  statBuf;
  int          nb;
  int          fd = open(path, O_RDONLY);
  char*        buf;

  if (fd == -1)
    LM_RE(NULL, ("HTTPS Error (error opening '%s': %s)", path, strerror(errno)));

  if (stat(path, &statBuf) != 0)
  {
    close(fd);
    LM_RE(NULL, ("HTTPS Error (stat '%s': %s)", path, strerror(errno)));
  }

  buf = (char*) malloc(statBuf.st_size + 1);

  if (buf == NULL)
  {
    close(fd);
    LM_RE(NULL, ("HTTPS Error (out of memory allocating room for https key/cert file of %d bytes)", statBuf.st_size + 1));
  }

  nb = read(fd, buf, statBuf.st_size);
  close(fd);

  if (nb == -1)
    LM_RE(NULL, ("HTTPS Error (reading from '%s': %s)", path, strerror(errno)));

  if (nb != statBuf.st_size)
    LM_RE(NULL, ("HTTPS Error (invalid size read from '%s': %d, wanted %d)", path, nb, statBuf.st_size));

  buf[statBuf.st_size] = 0;  // Zero-terminate the buffer

  return buf;
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
static void notificationModeParse(char* notificationMode, int* pQueueSize, int* pNumThreads)
{
  if (strncmp(notificationMode, "threadpool", 10) == 0)
  {
    if (notificationMode[10] == 0)
    {
      *pQueueSize  = DEFAULT_NOTIF_QS;
      *pNumThreads = DEFAULT_NOTIF_TN;
      return;
    }
    else if (notificationMode[10] == ':')
    {
      notificationMode[10] = 0;
      char* colon2 = strchr(&notificationMode[11], ':');

      if (colon2 == NULL)
        LM_X(1, ("Invalid notificationMode (first colon found, second is missing)"));
      *pQueueSize  = atoi(&notificationMode[11]);
      *pNumThreads = atoi(&colon2[1]);
    }
    else
      LM_X(1, ("Invalid notificationMode '%s'", notificationMode));
  }
  else if ((strcmp(notificationMode, "transient") != 0) && (strcmp(notificationMode, "persistent") != 0))
    LM_X(1, ("Invalid notificationMode '%s'", notificationMode));
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
  LM_K(("core @context:      %s", coreContextUrl));
  LM_K(("git hash:           %s", GIT_HASH));
  LM_K(("build branch:       %s", ORIONLD_BRANCH));
  LM_K(("compiled by:        %s", COMPILED_BY));
  LM_K(("compiled in:        %s", COMPILED_IN));
  LM_K(("-----------------------------------------"));
}



// -----------------------------------------------------------------------------
//
// libLogBuffer -
//
thread_local char libLogBuffer[1024 * 32];



// -----------------------------------------------------------------------------
//
// libLogFunction -
//
static void libLogFunction
(
  int          severity,              // 1: Error, 2: Warning, 3: Info, 4: Verbose, 5: Trace
  int          level,                 // Trace level || Error code || Info Code
  const char*  fileName,
  int          lineNo,
  const char*  functionName,
  const char*  format,
  ...
)
{
  va_list  args;

  /* "Parse" the variable arguments */
  va_start(args, format);

  /* Print message to variable */
  vsnprintf(libLogBuffer, sizeof(libLogBuffer), format, args);
  va_end(args);

  // LM_K(("Got a lib log message, severity: %d: %s", severity, libLogBuffer));

  if (severity == 1)
    lmOut(libLogBuffer, 'E', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 2)
    lmOut(libLogBuffer, 'W', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 3)
    lmOut(libLogBuffer, 'I', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 4)
    lmOut(libLogBuffer, 'V', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 5)
    lmOut(libLogBuffer, 'T', fileName, lineNo, functionName, level + LmtKjlParse, NULL);
}


#ifdef DEBUG
// -----------------------------------------------------------------------------
//
// regCachePresent -
//
void regCachePresent(void)
{
  for (OrionldTenant* tenantP = &tenant0; tenantP != NULL; tenantP = tenantP->next)
  {
    if (tenantP->regCache == NULL)
      LM_T(LmtRegCache, ("Tenant '%s': No regCache", tenantP->mongoDbName));
    else
    {
      LM_T(LmtRegCache, ("Tenant '%s':", tenantP->mongoDbName));
      RegCacheItem* rciP = tenantP->regCache->regList;

      while (rciP != NULL)
      {
        KjNode* regIdP = kjLookup(rciP->regTree, "id");

        LM_T(LmtRegCache, ("  o Registration %s:", (regIdP != NULL)? regIdP->value.s : "unknown"));
        LM_T(LmtRegCache, ("    o mode:  %s", registrationModeToString(rciP->mode)));
        LM_T(LmtRegCache, ("    o ops:   0x%x", rciP->opMask));

        if (rciP->idPatternRegexList != NULL)
        {
          LM_T(LmtRegCache, ("    o patterns:"));
          for (RegIdPattern* ripP = rciP->idPatternRegexList; ripP != NULL; ripP = ripP->next)
          {
            LM_T(LmtRegCache, ("      o %s (idPattern at %p)", ripP->owner->value.s, ripP->owner));
          }
        }
        else
          LM_T(LmtRegCache, ("    o patterns: NONE"));
        LM_T(LmtRegCache, ("  -----------------------------------"));
        rciP = rciP->next;
      }
    }
  }
}
#endif



// -----------------------------------------------------------------------------
//
// coreContextUrlSetup -
//
static char* coreContextUrlSetup(const char* version)
{
  if      (strcmp(version, "v1.0") == 0)    return ORIONLD_CORE_CONTEXT_URL_V1_0;
  else if (strcmp(version, "v1.3") == 0)    return ORIONLD_CORE_CONTEXT_URL_V1_3;
  else if (strcmp(version, "v1.4") == 0)    return ORIONLD_CORE_CONTEXT_URL_V1_4;
  else if (strcmp(version, "v1.5") == 0)    return ORIONLD_CORE_CONTEXT_URL_V1_5;
  else if (strcmp(version, "v1.6") == 0)    return ORIONLD_CORE_CONTEXT_URL_V1_6;
  else if (strcmp(version, "v1.7") == 0)    return ORIONLD_CORE_CONTEXT_URL_V1_7;

  return NULL;
}



#define LOG_FILE_LINE_FORMAT "time=DATE | lvl=TYPE | corr=CORR_ID | trans=TRANS_ID | from=FROM_IP | srv=SERVICE | subsrv=SUB_SERVICE | comp=Orion | op=FILE[LINE]:FUNC | msg=TEXT"
/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{
#if 0
  //
  // Just an experiment.
  // It's an interesting way of "comparing strings"
  // The problem is "const char*" vs "char*" - stupid C++ and its type checking!!!
  //
  // char* SUB_CACHE_DISABLED = NULL;

  char* sc = SUB_CACHE_DISABLED;
  if (sc == SUB_CACHE_DISABLED)
  {
    printf("the sub cache is disabled\n");
    exit(1);
  }
#endif

#if LEAK_TEST
  char* allocated = strdup("123");
  if (allocated == NULL)
    exit(7);
  else
    allocated = NULL;
#endif

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
  // If trace levels are set, set logLevel to DEBUG, so that the trace messages will actually pass through
  //
  if (paIsSet(argC, argV, paArgs, "-t"))
    strncpy(paLogLevel, "DEBUG", sizeof(paLogLevel) - 1);

  paParse(paArgs, argC, (char**) argV, 1, false);

  coreContextUrl = coreContextUrlSetup(coreContextVersion);
  if (coreContextUrl == NULL)
    LM_X(1, ("Invalid version for the Core Context: %s (valid: v1.0|v1.3|v1.4|v1.5|v1.6|v1.7)", coreContextVersion));

  lmTimeFormat(0, (char*) "%Y-%m-%dT%H:%M:%S");

  if ((debugCurl == true) && ((lmTraceIsSet(LmtCurl) == false) || (strcmp(paLogLevel, "DEBUG") != 0)))
  {
    strncpy(paLogLevel, "DEBUG", sizeof(paLogLevel) - 1);
    lmTraceLevelSet(LmtCurl, true);
  }

  if (wip[0] != 0)
  {
    if (strcmp(wip, "entityMaps") == 0)
      entityMapsEnabled = true;
  }

#if 0
  //
  // Uncomment this piece of code and run the functests (-ld) to make sure everything works "more or less" with the Legacy Driver disabled.
  //
  // The following tests fail (for obvious reasons):
  //   o ngsild_langprop-and-notifications.test                                  (LD notif instead of v2)
  //   o ngsild_new-false-error-for-optional-field-not-present.test              (Registrations not supported with mongoc)
  //   o ngsild_new_atid_attype_alias_validation.test                            (Registrations not supported with mongoc)
  //   o ngsild_new_atid_attype_alias_validation_errors.test                     (Registrations not supported with mongoc)
  //   o ngsild_new_entity_attributes.test                                       (Registrations not supported with mongoc)
  //   o ngsild_new_entity_types-merge-of-entities-and-registrations.test        (Registrations not supported with mongoc)
  //   o ngsild_new_entity_types-merge-of-registrations.test                     (Registrations not supported with mongoc)
  //   o ngsild_new_entity_types.test                                            (Registrations not supported with mongoc)
  //   o ngsild_new_notification_with-q-or.test                                  (NGSIv2 request)
  //   o ngsild_new_url_parse.test                                               (Registrations not supported with mongoc)
  //   o ngsild_subCache-counters-with-ngsiv2-subscriptions.test                 (Registrations not supported with mongoc)
  //   o ngsild_subCache-with-mongoc-on-startup-with-ngsiv2-subscriptions.test   (Registrations not supported with mongoc)
  //
  if (experimental == true)
    mongocOnly = true;
#endif

  if (mongocOnly == true)
    experimental = true;

  if (noswap == true)
  {
    LM_W(("All the broker's memory is locked in RAM - swapping disabled!!!"));
    mlockall(MCL_CURRENT | MCL_FUTURE);
  }

  //
  // Set portNo
  //
  portNo = port;


  //
  // Set global variables from the arguments
  //
  dbNameLen = strlen(dbName);

  paCleanup();

  if (strlen(dbName) > DB_NAME_MAX_LEN)
    LM_X(1, ("dbName too long (max %d characters)", DB_NAME_MAX_LEN));

  if (useOnlyIPv6 && useOnlyIPv4)
    LM_X(1, ("Fatal Error (-ipv4 and -ipv6 can not be activated at the same time. They are incompatible)"));

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

  notificationModeParse(notificationMode, &notificationQueueSize, &notificationThreadNum);
  LM_I(("Orion Context Broker is running"));

  versionInfo();

  if (fg == false)
    daemonize();

  if (noprom == true)
    LM_W(("Running without Prometheus metrics"));
  else if (promInit(8000) != 0)
    LM_W(("Error initializing Prometheus Metrics library"));

  IpVersion ipVersion = IPDUAL;

  if (useOnlyIPv4)
    ipVersion = IPV4;
  else if (useOnlyIPv6)
    ipVersion = IPV6;

  SemOpType policy = policyGet(reqMutexPolicy);
  orionInit(orionExit, ORION_VERSION, policy, statCounters, statSemWait, statTiming, statNotifQueue, strictIdv1);

  //
  // The database for Temporal Representation of Entities must be initialized before mongodb
  // as callbacks to create tenants (== postgres databases) and their tables are called from the
  // initialization routines of mongodb - if postgres is not initialized, this will fail.
  //
  if (troe)
  {
    // Close stderr, as postgres driver prints garbage to it!
    // close(2);

    if (troeInit() == false)
      LM_X(1, ("Database Error (unable to initialize the layer for Temporal Representation of Entities)"));
  }


  //
  // Initialize the 'context download list' - to avoid multiple downloads of the same contexts
  //
  contextDownloadListInit();


  //
  // Initialize the KBASE library
  // This call redirects all log messages from the K-libs to the brokers log file.
  //
  kInit(libLogFunction);


  //
  // Initialize the KALLOC library
  //
  kaInit(libLogFunction);
  kaBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 32 * 1024, NULL, "Global KAlloc buffer");


  //
  // Initialize the KJSON library
  // This sets up the global kjson instance with preallocated kalloc buffer
  //
  kjsonP = kjBufferCreate(&kjson, &kalloc);


  //
  // Get the hostname - needed for contexts created by the broker + notifications
  //
  gethostname(orionldHostName, sizeof(orionldHostName));
  orionldHostNameLen = strlen(orionldHostName);

  // localIpAndPort - IP:port for X-Forwarded-For
  snprintf(localIpAndPort, sizeof(localIpAndPort), "%s:%d", orionldHostName, port);

  // brokerId - for the Via header
  if (brokerId[0] == 0)
    strncpy(brokerId, localIpAndPort, sizeof(brokerId) - 1);
  else
    distributed = true;  // Turn on forwarding if the brokerId CLI is used

  orionldStateInit(NULL);

  // mongocInit calls mongocGeoIndexInit - tenant0 must be ready for that
  orionldTenantInit();
  orionldState.tenantP = &tenant0;

  mongocInit(dbURI, dbHost, dbUser, dbPwd, dbAuthDb, rplSet, dbAuthMechanism, dbSSL, dbCertFile);

  //
  // Now that the DB is ready to be used, we can populate the regCache for the different tenants
  // Note that regCacheInit uses the tenantList, so orionldTenantInit must be called before regCacheInit
  //
  regCacheInit();

  if (pernot == true)
    pernotSubCacheInit();

  orionldServiceInit(restServiceVV, 9);

  if (mongocOnly == false)
  {
    // Initialize Mongo Legacy C++ driver
    mongoInit(dbHost, rplSet, dbName, dbUser, dbPwd, multitenancy, dbTimeout, writeConcern, dbPoolSize, statSemWait);
  }

  // Initialize libs
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
    orionldStartup = true;
    subCacheInit(multitenancy);

    if (subCacheInterval == 0)
    {
      // Populate subscription cache from database
      subCacheRefresh(false);
    }
    else
    {
      // Populate subscription cache AND start sub-cache-refresh-thread
      subCacheStart();
    }
    orionldStartup = false;
  }

  dbInit(dbHost, dbName);  // Move to be next to mongocInit ?

  //
  // Given that contextBrokerInit() may create thread (in the threadpool notification mode,
  // it has to be done before curl_global_init(), see https://curl.haxx.se/libcurl/c/threaded-ssl.html
  // Otherwise, we have empirically checked that CB may randomly crash
  //
  contextBrokerInit(dbName, multitenancy);

  if (distributed)
    distOpInit();

  if (https)
  {
    char* httpsPrivateServerKey = loadFile(httpsKeyFile);
    char* httpsCertificate      = loadFile(httpsCertFile);

    if (httpsPrivateServerKey == NULL)
    {
      if (httpsCertificate != NULL)
        free(httpsCertificate);
      LM_E(("Fatal Error (loading private server key from '%s')", httpsKeyFile));
      exit(1);
    }

    if (httpsCertificate == NULL)
    {
      if (httpsPrivateServerKey != NULL)
        free(httpsPrivateServerKey);
      LM_E(("Fatal Error (loading certificate from '%s')", httpsCertFile));
      exit(1);
    }

    orionRestServicesInit(ipVersion,
                          bindAddress,
                          port,
                          multitenancy,
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
                          multitenancy,
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
  orionldPhase = OrionldPhaseServing;

  //
  // Get the version of the mongo server (could be in mongocInit instead)
  //
  if (mongocServerVersionGet(mongocServerVersion) == false)
    LM_X(1, ("Unable to contact the MongoDB Server"));

  if (troe)
  {
    if (pgVersionGet(postgresServerVersion, sizeof(postgresServerVersion)) == false)
      LM_X(1, ("Unable to contact the Postgres Server"));
  }

  LM_K(("Initialization is Done"));
  LM_K(("  Accepting REST requests on port %d (experimental API endpoints are %sabled)", port, (experimental == true)? "en" : "dis"));
  LM_K(("  TRoE:                    %s", (troe          == true)? "Enabled" : "Disabled"));
  LM_K(("  Distributed Operation:   %s", (distributed   == true)? "Enabled" : "Disabled"));
  LM_K(("  Health Check:            %s", (socketService == true)? "Enabled" : "Disabled"));

  if (troe)
    LM_K(("  Postgres Server Version: %s", postgresServerVersion));

  LM_K(("  Mongo Server Version:    %s", mongocServerVersion));

  if (mongocOnly == true)
  {
    LM_K(("  Mongo Driver:            mongoc driver- ONLY (MongoDB C++ Legacy Driver is DISABLED)"));
    LM_K(("  MongoC Driver Version:   %s", MONGOC_VERSION_S));
  }
  else if (experimental  == true)
  {
    LM_K(("  Mongo Driver:            mongoc driver for NGSI-LD requests, Legacy Mongo C++ Driver for NGSIv1&2"));
    LM_K(("  MongoC Driver Version:   %s", MONGOC_VERSION_S));
  }
  else
    LM_K(("  Mongo Driver:            Legacy C++ Driver (deprecated by mongodb)"));

  // Startup is done - we can free up the allocated kalloc buffers - assuming socketService doesn't use kalloc ...
  kaBufferReset(&orionldState.kalloc, KFALSE);


  // Start the thread for periodic notifications
  if (pernot == true)
    pernotLoopStart();

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
