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
#include "serviceRoutines/logTraceTreat.h"
#include "serviceRoutines/getEntityTypes.h"
#include "serviceRoutines/getAttributesForEntityType.h"
#include "serviceRoutines/getAllContextEntities.h"
#include "serviceRoutines/versionTreat.h"
#include "serviceRoutines/statisticsTreat.h"
#include "serviceRoutines/exitTreat.h"
#include "serviceRoutines/leakTreat.h"

#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/postSubscribeContext.h"
#include "serviceRoutines/postSubscribeContextAvailability.h"
#include "serviceRoutines/postUnsubscribeContextAvailability.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/postUpdateContextAvailabilitySubscription.h"
#include "serviceRoutines/postUpdateContextSubscription.h"
#include "serviceRoutines/postUnsubscribeContext.h"
#include "serviceRoutines/postNotifyContext.h"
#include "serviceRoutines/postNotifyContextAvailability.h"
#include "serviceRoutines/postSubscribeContextConvOp.h"
#include "serviceRoutines/postSubscribeContextAvailabilityConvOp.h"
#include "serviceRoutines/getContextEntitiesByEntityId.h"
#include "serviceRoutines/postContextEntitiesByEntityId.h"
#include "serviceRoutines/getContextEntityAttributes.h"
#include "serviceRoutines/postContextEntityAttributes.h"
#include "serviceRoutines/getEntityByIdAttributeByName.h"
#include "serviceRoutines/postEntityByIdAttributeByName.h"
#include "serviceRoutines/getContextEntityTypes.h"
#include "serviceRoutines/postContextEntityTypes.h"
#include "serviceRoutines/getContextEntityTypeAttribute.h"
#include "serviceRoutines/postContextEntityTypeAttribute.h"
#include "serviceRoutines/putAvailabilitySubscriptionConvOp.h"
#include "serviceRoutines/deleteAvailabilitySubscriptionConvOp.h"
#include "serviceRoutines/getIndividualContextEntity.h"
#include "serviceRoutines/putIndividualContextEntity.h"
#include "serviceRoutines/badVerbPostOnly.h"
#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostOnly.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutines/postIndividualContextEntity.h"
#include "serviceRoutines/deleteIndividualContextEntity.h"
#include "serviceRoutines/badVerbAllFour.h"
#include "serviceRoutines/badVerbAllFive.h"
#include "serviceRoutines/badVerbPutOnly.h"
#include "serviceRoutines/putIndividualContextEntityAttribute.h"
#include "serviceRoutines/getIndividualContextEntityAttribute.h"
#include "serviceRoutines/getNgsi10ContextEntityTypes.h"
#include "serviceRoutines/getNgsi10ContextEntityTypesAttribute.h"
#include "serviceRoutines/postIndividualContextEntityAttribute.h"
#include "serviceRoutines/deleteIndividualContextEntityAttribute.h"
#include "serviceRoutines/putSubscriptionConvOp.h"
#include "serviceRoutines/deleteSubscriptionConvOp.h"
#include "serviceRoutines/getAttributeValueInstance.h"
#include "serviceRoutines/putAttributeValueInstance.h"
#include "serviceRoutines/deleteAttributeValueInstance.h"
#include "serviceRoutines/getAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/postAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/putAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/deleteAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/getIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/postIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/putIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/deleteIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/getAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/deleteAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/postAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/putAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/getContextEntitiesByEntityIdAndType.h"
#include "serviceRoutines/postContextEntitiesByEntityIdAndType.h"
#include "serviceRoutines/getEntityByIdAttributeByNameWithTypeAndId.h"
#include "serviceRoutines/postEntityByIdAttributeByNameWithTypeAndId.h"
#include "serviceRoutines/badVerbGetPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostDeleteOnly.h"
#include "serviceRoutines/badVerbGetOnly.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutinesV2/badVerbGetPutOnly.h"
#include "serviceRoutinesV2/badVerbGetDeletePatchOnly.h"
#include "serviceRoutines/badNgsi9Request.h"
#include "serviceRoutines/badNgsi10Request.h"
#include "serviceRoutines/badRequest.h"
#include "serviceRoutinesV2/badVerbAllNotDelete.h"

#include "serviceRoutinesV2/getEntities.h"
#include "serviceRoutinesV2/entryPointsTreat.h"
#include "serviceRoutinesV2/getEntity.h"
#include "serviceRoutinesV2/getEntityAttribute.h"
#include "serviceRoutinesV2/putEntityAttribute.h"
#include "serviceRoutinesV2/getEntityAttributeValue.h"
#include "serviceRoutinesV2/putEntityAttributeValue.h"
#include "serviceRoutinesV2/postEntities.h"
#include "serviceRoutinesV2/putEntity.h"
#include "serviceRoutinesV2/postEntity.h"
#include "serviceRoutinesV2/deleteEntity.h"
#include "serviceRoutinesV2/getEntityType.h"
#include "serviceRoutinesV2/getEntityAllTypes.h"
#include "serviceRoutinesV2/patchEntity.h"
#include "serviceRoutinesV2/getAllSubscriptions.h"
#include "serviceRoutinesV2/getSubscription.h"
#include "serviceRoutinesV2/postSubscriptions.h"
#include "serviceRoutinesV2/deleteSubscription.h"
#include "serviceRoutinesV2/patchSubscription.h"
#include "serviceRoutinesV2/postBatchQuery.h"
#include "serviceRoutinesV2/postBatchUpdate.h"
#include "serviceRoutinesV2/logLevelTreat.h"
#include "serviceRoutinesV2/semStateTreat.h"
#include "serviceRoutinesV2/getMetrics.h"
#include "serviceRoutinesV2/deleteMetrics.h"
#include "serviceRoutinesV2/getRegistration.h"
#include "serviceRoutinesV2/getRegistrations.h"
#include "serviceRoutinesV2/postRegistration.h"
#include "serviceRoutinesV2/optionsGetOnly.h"
#include "serviceRoutinesV2/optionsGetPostOnly.h"
#include "serviceRoutinesV2/optionsGetDeleteOnly.h"
#include "serviceRoutinesV2/optionsAllNotDelete.h"
#include "serviceRoutinesV2/optionsGetPutOnly.h"
#include "serviceRoutinesV2/optionsGetPutDeleteOnly.h"
#include "serviceRoutinesV2/optionsGetDeletePatchOnly.h"
#include "serviceRoutinesV2/optionsPostOnly.h"

#include "contextBroker/version.h"
#include "common/string.h"
#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "logSummary/logSummary.h"

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
char            dbHost[64];
char            rplSet[64];
char            dbName[64];
char            user[64];
char            pwd[64];
char            pidPath[256];
bool            harakiri;
bool            useOnlyIPv4;
bool            useOnlyIPv6;
char            httpsKeyFile[1024];
char            httpsCertFile[1024];
bool            https;
bool            mtenant;
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



/* ****************************************************************************
*
* Definitions to make paArgs lines shorter ...
*/
#define PIDPATH             _i "/tmp/contextBroker.pid"
#define IP_ALL              _i "0.0.0.0"
#define LOCALHOST           _i "localhost"
#define ONE_MONTH_PERIOD    (3600 * 24 * 31)

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
  { "-fg",            &fg,           "FOREGROUND",     PaBool,   PaOpt, false,      false,  true,  FG_DESC            },
  { "-localIp",       bindAddress,   "LOCALIP",        PaString, PaOpt, IP_ALL,     PaNL,   PaNL,  LOCALIP_DESC       },
  { "-port",          &port,         "PORT",           PaInt,    PaOpt, 1026,       PaNL,   PaNL,  PORT_DESC          },
  { "-pidpath",       pidPath,       "PID_PATH",       PaString, PaOpt, PIDPATH,    PaNL,   PaNL,  PIDPATH_DESC       },

  { "-dbhost",        dbHost,        "DB_HOST",        PaString, PaOpt, LOCALHOST,  PaNL,   PaNL,  DBHOST_DESC        },
  { "-rplSet",        rplSet,        "RPL_SET",        PaString, PaOpt, _i "",      PaNL,   PaNL,  RPLSET_DESC        },
  { "-dbuser",        user,          "DB_USER",        PaString, PaOpt, _i "",      PaNL,   PaNL,  DBUSER_DESC        },
  { "-dbpwd",         pwd,           "DB_PASSWORD",    PaString, PaOpt, _i "",      PaNL,   PaNL,  DBPASSWORD_DESC    },
  { "-db",            dbName,        "DB",             PaString, PaOpt, _i "orion", PaNL,   PaNL,  DB_DESC            },
  { "-dbTimeout",     &dbTimeout,    "DB_TIMEOUT",     PaDouble, PaOpt, 10000,      PaNL,   PaNL,  DB_TMO_DESC        },
  { "-dbPoolSize",    &dbPoolSize,   "DB_POOL_SIZE",   PaInt,    PaOpt, 10,         1,      10000, DBPS_DESC          },

  { "-ipv4",          &useOnlyIPv4,  "USEIPV4",        PaBool,   PaOpt, false,      false,  true,  USEIPV4_DESC       },
  { "-ipv6",          &useOnlyIPv6,  "USEIPV6",        PaBool,   PaOpt, false,      false,  true,  USEIPV6_DESC       },
  { "-harakiri",      &harakiri,     "HARAKIRI",       PaBool,   PaHid, false,      false,  true,  HARAKIRI_DESC      },

  { "-https",         &https,        "HTTPS",          PaBool,   PaOpt, false,      false,  true,  HTTPS_DESC         },
  { "-key",           httpsKeyFile,  "HTTPS_KEYFILE",  PaString, PaOpt, _i "",      PaNL,   PaNL,  HTTPSKEYFILE_DESC  },
  { "-cert",          httpsCertFile, "HTTPS_CERTFILE", PaString, PaOpt, _i "",      PaNL,   PaNL,  HTTPSCERTFILE_DESC },

  { "-rush",          rush,          "RUSH",           PaString, PaOpt, _i "",      PaNL,   PaNL,  RUSH_DESC          },
  { "-multiservice",  &mtenant,      "MULTI_SERVICE",  PaBool,   PaOpt, false,      false,  true,  MULTISERVICE_DESC  },

  { "-httpTimeout",   &httpTimeout,  "HTTP_TIMEOUT",   PaLong,   PaOpt, -1,         -1,     MAX_L, HTTP_TMO_DESC      },
  { "-reqTimeout",    &reqTimeout,   "REQ_TIMEOUT",    PaLong,   PaOpt,  0,          0,     PaNL,  REQ_TMO_DESC       },
  { "-reqMutexPolicy",reqMutexPolicy,"MUTEX_POLICY",   PaString, PaOpt, _i "all",   PaNL,   PaNL,  MUTEX_POLICY_DESC  },
  { "-writeConcern",  &writeConcern, "WRITE_CONCERN",  PaInt,    PaOpt, 1,          0,      1,     WRITE_CONCERN_DESC },

  { "-corsOrigin",       allowedOrigin,     "ALLOWED_ORIGIN",    PaString, PaOpt, _i "",          PaNL,  PaNL,     ALLOWED_ORIGIN_DESC    },
  { "-corsMaxAge",       &maxAge,           "CORS_MAX_AGE",      PaInt,    PaOpt, 86400,          -1,    86400,    CORS_MAX_AGE_DESC      },
  { "-cprForwardLimit",  &cprForwardLimit,  "CPR_FORWARD_LIMIT", PaUInt,   PaOpt, 1000,           0,     UINT_MAX, CPR_FORWARD_LIMIT_DESC },
  { "-subCacheIval",     &subCacheInterval, "SUBCACHE_IVAL",     PaInt,    PaOpt, 60,             0,     3600,     SUB_CACHE_IVAL_DESC    },
  { "-noCache",          &noCache,          "NOCACHE",           PaBool,   PaOpt, false,          false, true,     NO_CACHE               },
  { "-connectionMemory", &connectionMemory, "CONN_MEMORY",       PaUInt,   PaOpt, 64,             0,     1024,     CONN_MEMORY_DESC       },
  { "-maxConnections",   &maxConnections,   "MAX_CONN",          PaUInt,   PaOpt, 1020,           1,     PaNL,     MAX_CONN_DESC          },
  { "-reqPoolSize",      &reqPoolSize,      "TRQ_POOL_SIZE",     PaUInt,   PaOpt, 0,              0,     1024,     REQ_POOL_SIZE          },

  { "-notificationMode",      &notificationMode,      "NOTIF_MODE", PaString, PaOpt, _i "transient", PaNL,  PaNL, NOTIFICATION_MODE_DESC },
  { "-simulatedNotification", &simulatedNotification, "DROP_NOTIF", PaBool,   PaOpt, false,          false, true, SIMULATED_NOTIF_DESC   },

  { "-statCounters",   &statCounters,   "STAT_COUNTERS",    PaBool, PaOpt, false, false, true, STAT_COUNTERS     },
  { "-statSemWait",    &statSemWait,    "STAT_SEM_WAIT",    PaBool, PaOpt, false, false, true, STAT_SEM_WAIT     },
  { "-statTiming",     &statTiming,     "STAT_TIMING",      PaBool, PaOpt, false, false, true, STAT_TIMING       },
  { "-statNotifQueue", &statNotifQueue, "STAT_NOTIF_QUEUE", PaBool, PaOpt, false, false, true, STAT_NOTIF_QUEUE  },

  { "-logSummary",     &lsPeriod,       "LOG_SUMMARY_PERIOD", PaInt,  PaOpt, 0,     0,     ONE_MONTH_PERIOD, LOG_SUMMARY_DESC },
  { "-relogAlarms",    &relogAlarms,    "RELOG_ALARMS",       PaBool, PaOpt, false, false, true,             RELOGALARMS_DESC },

  { "-strictNgsiv1Ids",             &strictIdv1,      "CHECK_ID_V1",           PaBool, PaOpt, false, false, true, CHECK_v1_ID_DESC      },
  { "-disableCustomNotifications",  &disableCusNotif, "DISABLE_CUSTOM_NOTIF",  PaBool, PaOpt, false, false, true, DISABLE_CUSTOM_NOTIF  },

  { "-logForHumans",   &logForHumans,    "LOG_FOR_HUMANS",     PaBool, PaOpt, false, false, true,             LOG_FOR_HUMANS_DESC },
  { "-disableMetrics", &disableMetrics,  "DISABLE_METRICS",    PaBool, PaOpt, false, false, true,             METRICS_DESC        },

  { "-insecureNotif", &insecureNotif, "INSECURE_NOTIF", PaBool, PaOpt, false, false, true, INSECURE_NOTIF },

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
*   std::string   verb        - GET/POST/PUT/DELETE
*   RequestType   request     - The type of the request
*   int           components  - Number of components in the following URL component vector
*   std::string   compV       - Component vector of the URL
*   std::string   payloadWord - first word in the payload for the request (to verify that the payload matches the URL). If empty, no check is performed)
*   RestTreat     treat       - Function pointer to the function to treat the incoming REST request
*
*/



//
// /v2 API
//
#define EPS                     EntryPointsRequest
#define EPS_COMPS_V2            1, { "v2"             }

#define ENT                     EntitiesRequest
#define ENT_COMPS_V2            2, { "v2", "entities" }
#define ENT_COMPS_WORD          ""

#define IENT                    EntityRequest
#define IENT_COMPS_V2           3, { "v2", "entities", "*" }
#define IENT_COMPS_WORD         ""

#define IENTOA                  EntityRequest
#define IENTOA_COMPS_V2         4, { "v2", "entities", "*", "attrs" }
#define IENTOA_COMPS_WORD       ""

#define IENTATTR                EntityAttributeRequest
#define IENTATTR_COMPS_V2       5, { "v2", "entities", "*", "attrs", "*" }
#define IENTATTR_COMPS_WORD     ""

#define ENTT                    EntityTypeRequest
#define ENTT_COMPS_V2           3, { "v2", "types", "*" }
#define ENTT_COMPS_WORD         ""

#define IENTATTRVAL             EntityAttributeValueRequest
#define IENTATTRVAL_COMPS_V2    6, { "v2", "entities", "*", "attrs", "*", "value" }
#define IENTATTRVAL_COMPS_WORD  ""

#define ETT                     EntityAllTypesRequest
#define ETT_COMPS_V2            2, { "v2", "types" }
#define ETT_COMPS_WORD          ""

#define SSR                     SubscriptionsRequest
#define SSR_COMPS_V2            2, { "v2", "subscriptions" }
#define SSR_COMPS_WORD          ""

#define ISR                     IndividualSubscriptionRequest
#define ISR_COMPS_V2            3, { "v2", "subscriptions", "*" }
#define ISR_COMPS_WORD          ""

#define BQR                     BatchQueryRequest
#define BQR_COMPS_V2            3, { "v2", "op", "query" }
#define BQR_COMPS_WORD          ""

#define BUR                     BatchUpdateRequest
#define BUR_COMPS_V2            3, { "v2", "op", "update" }
#define BUR_COMPS_WORD          ""

//
// /v2 registration API
//
#define REG                     RegistrationRequest
#define REG_COMPS_V2            3, { "v2", "registrations", "*" }
#define REG_COMPS_WORD          ""

#define REGS                     RegistrationsRequest
#define REGS_COMPS_V2            2, { "v2", "registrations" }
#define REGS_COMPS_WORD          ""



//
// NGSI9
//
#define RCR                RegisterContext
#define DCAR               DiscoverContextAvailability
#define SCAR               SubscribeContextAvailability
#define UCAR               UnsubscribeContextAvailability
#define UCAS               UpdateContextAvailabilitySubscription
#define NCAR               NotifyContextAvailability

#define RCR_COMPS_V0       2, { "ngsi9",          "registerContext" }
#define RCR_COMPS_V1       3, { "v1", "registry", "registerContext" }
#define RCR_POST_WORD      "registerContextRequest"

#define DCAR_COMPS_V0      2, { "ngsi9",          "discoverContextAvailability" }
#define DCAR_COMPS_V1      3, { "v1", "registry", "discoverContextAvailability" }
#define DCAR_POST_WORD     "discoverContextAvailabilityRequest"

#define SCAR_COMPS_V0      2, { "ngsi9",          "subscribeContextAvailability" }
#define SCAR_COMPS_V1      3, { "v1", "registry", "subscribeContextAvailability" }
#define SCAR_POST_WORD     "subscribeContextAvailabilityRequest"

#define UCAR_COMPS_V0      2, { "ngsi9",          "unsubscribeContextAvailability" }
#define UCAR_COMPS_V1      3, { "v1", "registry", "unsubscribeContextAvailability" }
#define UCAR_POST_WORD     "unsubscribeContextAvailabilityRequest"

#define UCAS_COMPS_V0      2, { "ngsi9",          "updateContextAvailabilitySubscription" }
#define UCAS_COMPS_V1      3, { "v1", "registry", "updateContextAvailabilitySubscription" }
#define UCAS_POST_WORD     "updateContextAvailabilitySubscriptionRequest"

#define NCAR_COMPS_V0      2, { "ngsi9",          "notifyContextAvailability" }
#define NCAR_COMPS_V1      3, { "v1", "registry", "notifyContextAvailability" }
#define NCAR_POST_WORD     "notifyContextAvailabilityRequest"



//
// NGSI10
//
#define UPCR          UpdateContext
#define QCR           QueryContext
#define SCR           SubscribeContext
#define UCSR          UpdateContextSubscription
#define UNCR          UnsubscribeContext
#define NCR           NotifyContext

#define UPCR_COMPS_V0       2, { "ngsi10",  "updateContext" }
#define UPCR_COMPS_V1       2, { "v1",      "updateContext" }
#define UPCR_POST_WORD     "updateContextRequest"

#define QCR_COMPS_V0        2, { "ngsi10",  "queryContext" }
#define QCR_COMPS_V1        2, { "v1",      "queryContext" }
#define QCR_POST_WORD      "queryContextRequest"

#define SCR_COMPS_V0        2, { "ngsi10",  "subscribeContext" }
#define SCR_COMPS_V1        2, { "v1",      "subscribeContext" }
#define SCR_POST_WORD      "subscribeContextRequest"

#define UCSR_COMPS_V0       2, { "ngsi10",  "updateContextSubscription" }
#define UCSR_COMPS_V1       2, { "v1",      "updateContextSubscription" }
#define UCSR_POST_WORD     "updateContextSubscriptionRequest"

#define UNCR_COMPS_V0       2, { "ngsi10",  "unsubscribeContext" }
#define UNCR_COMPS_V1       2, { "v1",      "unsubscribeContext" }
#define UNCR_POST_WORD     "unsubscribeContextRequest"

#define NCR_COMPS_V0        2, { "ngsi10",  "notifyContext" }
#define NCR_COMPS_V1        2, { "v1",      "notifyContext" }
#define NCR_POST_WORD      "notifyContextRequest"


//
// NGSI9 Convenience Operations
//
#define CE                 ContextEntitiesByEntityId
#define CE_COMPS_V0        3, { "ngsi9",          "contextEntities", "*" }
#define CE_COMPS_V1        4, { "v1", "registry", "contextEntities", "*" }
#define CE_POST_WORD       "registerProviderRequest"

#define CEA                ContextEntityAttributes
#define CEA_COMPS_V0       4, { "ngsi9",          "contextEntities", "*", "attributes" }
#define CEA_COMPS_V1       5, { "v1", "registry", "contextEntities", "*", "attributes" }
#define CEA_POST_WORD      "registerProviderRequest"

#define CEAA               EntityByIdAttributeByName
#define CEAA_COMPS_V0      5, { "ngsi9",          "contextEntities", "*", "attributes", "*" }
#define CEAA_COMPS_V1      6, { "v1", "registry", "contextEntities", "*", "attributes", "*" }
#define CEAA_POST_WORD     "registerProviderRequest"

#define CT                 ContextEntityTypes
#define CT_COMPS_V0        3, { "ngsi9",          "contextEntityTypes", "*" }
#define CT_COMPS_V1        4, { "v1", "registry", "contextEntityTypes", "*" }
#define CT_POST_WORD       "registerProviderRequest"

#define CTA                ContextEntityTypeAttributeContainer
#define CTA_COMPS_V0       4, { "ngsi9",          "contextEntityTypes", "*", "attributes" }
#define CTA_COMPS_V1       5, { "v1", "registry", "contextEntityTypes", "*", "attributes" }
#define CTA_POST_WORD      "registerProviderRequest"

#define CTAA               ContextEntityTypeAttribute
#define CTAA_COMPS_V0      5, { "ngsi9",          "contextEntityTypes", "*", "attributes", "*" }
#define CTAA_COMPS_V1      6, { "v1", "registry", "contextEntityTypes", "*", "attributes", "*" }
#define CTAA_POST_WORD     "registerProviderRequest"

#define SCA                SubscribeContextAvailability
#define SCA_COMPS_V0       2, { "ngsi9",          "contextAvailabilitySubscriptions" }
#define SCA_COMPS_V1       3, { "v1", "registry", "contextAvailabilitySubscriptions" }
#define SCA_POST_WORD      "subscribeContextAvailabilityRequest"

#define SCAS               Ngsi9SubscriptionsConvOp
#define SCAS_COMPS_V0      3, { "ngsi9",          "contextAvailabilitySubscriptions", "*" }
#define SCAS_COMPS_V1      4, { "v1", "registry", "contextAvailabilitySubscriptions", "*" }
#define SCAS_PUT_WORD      "updateContextAvailabilitySubscriptionRequest"



//
// NGSI10 Convenience Operations
//
#define ICE                IndividualContextEntity
#define ICE_COMPS_V0       3, { "ngsi10",  "contextEntities", "*" }
#define ICE_COMPS_V1       3, { "v1",      "contextEntities", "*" }
#define ICE_POST_WORD      "appendContextElementRequest"
#define ICE_PUT_WORD       "updateContextElementRequest"

#define ICEA               IndividualContextEntityAttributes
#define ICEA_COMPS_V0      4, { "ngsi10",  "contextEntities", "*", "attributes" }
#define ICEA_COMPS_V1      4, { "v1",      "contextEntities", "*", "attributes" }
#define ICEA_POST_WORD     "appendContextElementRequest"
#define ICEA_PUT_WORD      "updateContextElementRequest"

#define ICEAA              IndividualContextEntityAttribute
#define ICEAA_COMPS_V0     5, { "ngsi10",  "contextEntities", "*", "attributes", "*" }
#define ICEAA_COMPS_V1     5, { "v1",      "contextEntities", "*", "attributes", "*" }
// FIXME P10: funny having updateContextAttributeRequest for both ... Error in NEC-SPEC?
#define ICEAA_POST_WORD    "updateContextAttributeRequest"
#define ICEAA_PUT_WORD     "updateContextAttributeRequest"

#define AVI                AttributeValueInstance
#define AVI_COMPS_V0       6, { "ngsi10",  "contextEntities", "*", "attributes", "*", "*" }
#define AVI_COMPS_V1       6, { "v1",      "contextEntities", "*", "attributes", "*", "*" }
#define AVI_PUT_WORD       "updateContextAttributeRequest"

#define CET                Ngsi10ContextEntityTypes
#define CET_COMPS_V0       3, { "ngsi10",  "contextEntityTypes", "*" }
#define CET_COMPS_V1       3, { "v1",      "contextEntityTypes", "*" }

#define CETA               Ngsi10ContextEntityTypesAttributeContainer
#define CETA_COMPS_V0      4, { "ngsi10",  "contextEntityTypes", "*", "attributes" }
#define CETA_COMPS_V1      4, { "v1",      "contextEntityTypes", "*", "attributes" }

#define CETAA              Ngsi10ContextEntityTypesAttribute
#define CETAA_COMPS_V0     5, { "ngsi10",  "contextEntityTypes", "*", "attributes", "*" }
#define CETAA_COMPS_V1     5, { "v1",      "contextEntityTypes", "*", "attributes", "*" }

#define SC                 SubscribeContext
#define SC_COMPS_V0        2, { "ngsi10",  "contextSubscriptions" }
#define SC_COMPS_V1        2, { "v1",      "contextSubscriptions" }
#define SC_POST_WORD       "subscribeContextRequest"

#define SCS                Ngsi10SubscriptionsConvOp
#define SCS_COMPS_V0       3, { "ngsi10",  "contextSubscriptions", "*" }
#define SCS_COMPS_V1       3, { "v1",      "contextSubscriptions", "*" }
#define SCS_PUT_WORD       "updateContextSubscriptionRequest"



//
// TID Convenience Operations
//
#define ET                 EntityTypes
#define ET_COMPS_V1        2, { "v1", "contextTypes" }

#define AFET               AttributesForEntityType
#define AFET_COMPS_V1      3, { "v1", "contextTypes", "*" }

#define ACE                AllContextEntities
#define ACE_COMPS_V1       2, { "v1", "contextEntities" }
#define ACE_POST_WORD     "appendContextElementRequest"

#define ACET               AllEntitiesWithTypeAndId
#define ACET_COMPS_V1      6, { "v1", "contextEntities", "type", "*", "id", "*" }
#define ACET_POST_WORD     "appendContextElementRequest"
#define ACET_PUT_WORD      "updateContextElementRequest"

#define ICEAAT              IndividualContextEntityAttributeWithTypeAndId
#define ICEAAT_COMPS_V1     8, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*" }
#define ICEAAT_POST_WORD    "updateContextAttributeRequest"
#define ICEAAT_PUT_WORD     "updateContextAttributeRequest"

#define AVIT                AttributeValueInstanceWithTypeAndId
#define AVIT_COMPS_V1       9, { "v1",      "contextEntities", "type", "*", "id", "*", "attributes", "*", "*" }
#define AVIT_PUT_WORD       "updateContextAttributeRequest"
#define AVIT_POST_WORD      "updateContextAttributeRequest"

#define CEET                ContextEntitiesByEntityIdAndType
#define CEET_COMPS_V1       7, { "v1", "registry", "contextEntities", "type", "*", "id", "*" }
#define CEET_POST_WORD      "registerProviderRequest"

#define CEAAT               EntityByIdAttributeByNameIdAndType
#define CEAAT_COMPS_V1      9, { "v1", "registry", "contextEntities", "type", "*", "id", "*", "attributes", "*" }
#define CEAAT_POST_WORD     "registerProviderRequest"



//
// Log, version, statistics ...
//
#define LOG                LogTraceRequest
#define LOGT_COMPS_V0      2, { "log", "trace"                           }
#define LOGTL_COMPS_V0     3, { "log", "trace",      "*"                 }
#define LOG2T_COMPS_V0     2, { "log", "traceLevel"                      }
#define LOG2TL_COMPS_V0    3, { "log", "traceLevel", "*"                 }
#define LOGT_COMPS_V1      4, { "v1", "admin", "log", "trace"            }
#define LOGTL_COMPS_V1     5, { "v1", "admin", "log", "trace",      "*"  }
#define LOG2T_COMPS_V1     4, { "v1", "admin", "log", "traceLevel"       }
#define LOG2TL_COMPS_V1    5, { "v1", "admin", "log", "traceLevel", "*"  }

#define STAT                 StatisticsRequest
#define STAT_COMPS_V0        1, { "statistics"                             }
#define STAT_COMPS_V1        3, { "v1", "admin", "statistics"              }
#define STAT_CACHE_COMPS_V0  2, { "cache", "statistics"                    }
#define STAT_CACHE_COMPS_V1  4, { "v1", "admin", "cache", "statistics"     }



//
// LogLevel
//
#define LOGLEVEL           LogLevelRequest
#define LOGLEVEL_COMPS_V2  2, { "admin", "log"                           }



//
// Semaphore state
//
#define SEM_STATE          SemStateRequest
#define SEM_STATE_COMPS    2, { "admin", "sem"                         }



//
// Metrics
//
#define METRICS            MetricsRequest
#define METRICS_COMPS      2, { "admin", "metrics"                       }


//
// Unversioned requests
//
#define VERS               VersionRequest
#define VERS_COMPS         1, { "version"                                }

#define EXIT               ExitRequest
#define EXIT1_COMPS        1, { "exit"                                   }
#define EXIT2_COMPS        2, { "exit", "*"                              }

#define LEAK               LeakRequest
#define LEAK1_COMPS        1, { "leak"                                   }
#define LEAK2_COMPS        2, { "leak", "*"                              }

#define INV                InvalidRequest
#define INV9_COMPS         2, { "ngsi9",   "*"                           }
#define INV10_COMPS        2, { "ngsi10",  "*"                           }
#define INV_ALL_COMPS      0, { "*", "*", "*", "*", "*", "*"             }



#define API_V2                                                                                         \
  { "GET",    EPS,          EPS_COMPS_V2,         ENT_COMPS_WORD,          entryPointsTreat         }, \
  { "*",      EPS,          EPS_COMPS_V2,         ENT_COMPS_WORD,          badVerbGetOnly           }, \
                                                                                                       \
  { "GET",    ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          getEntities              }, \
  { "POST",   ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          postEntities             }, \
  { "*",      ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          badVerbGetPostOnly       }, \
                                                                                                       \
  { "GET",    IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         getEntity                }, \
  { "DELETE", IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         deleteEntity             }, \
  { "*",      IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         badVerbGetDeleteOnly     }, \
                                                                                                       \
  { "GET",    IENTOA,       IENTOA_COMPS_V2,      IENTOA_COMPS_WORD,       getEntity                }, \
  { "POST",   IENTOA,       IENTOA_COMPS_V2,      IENTOA_COMPS_WORD,       postEntity               }, \
  { "PUT",    IENTOA,       IENTOA_COMPS_V2,      IENTOA_COMPS_WORD,       putEntity                }, \
  { "PATCH",  IENTOA,       IENTOA_COMPS_V2,      IENTOA_COMPS_WORD,       patchEntity              }, \
  { "*",      IENTOA,       IENTOA_COMPS_V2,      IENTOA_COMPS_WORD,       badVerbAllNotDelete      }, \
                                                                                                       \
  { "GET",    IENTATTRVAL,  IENTATTRVAL_COMPS_V2, IENTATTRVAL_COMPS_WORD,  getEntityAttributeValue  }, \
  { "PUT",    IENTATTRVAL,  IENTATTRVAL_COMPS_V2, IENTATTRVAL_COMPS_WORD,  putEntityAttributeValue  }, \
  { "*",      IENTATTRVAL,  IENTATTRVAL_COMPS_V2, IENTATTRVAL_COMPS_WORD,  badVerbGetPutOnly        }, \
                                                                                                       \
  { "GET",    IENTATTR,     IENTATTR_COMPS_V2,    IENTATTR_COMPS_WORD,     getEntityAttribute       }, \
  { "PUT",    IENTATTR,     IENTATTR_COMPS_V2,    IENTATTR_COMPS_WORD,     putEntityAttribute       }, \
  { "DELETE", IENTATTR,     IENTATTR_COMPS_V2,    IENTATTR_COMPS_WORD,     deleteEntity             }, \
  { "*",      IENTATTR,     IENTATTR_COMPS_V2,    IENTATTR_COMPS_WORD,     badVerbGetPutDeleteOnly  }, \
                                                                                                       \
  { "GET",    ENTT,         ENTT_COMPS_V2,        ENTT_COMPS_WORD,         getEntityType            }, \
  { "*",      ENTT,         ENTT_COMPS_V2,        ENTT_COMPS_WORD,         badVerbGetOnly           }, \
                                                                                                       \
  { "GET",    ETT,          ETT_COMPS_V2,         ETT_COMPS_WORD,          getEntityAllTypes        }, \
  { "*",      ETT,          ETT_COMPS_V2,         ETT_COMPS_WORD,          badVerbGetOnly           }, \
                                                                                                       \
  { "GET",    SSR,          SSR_COMPS_V2,         SSR_COMPS_WORD,          getAllSubscriptions      }, \
  { "POST",   SSR,          SSR_COMPS_V2,         SSR_COMPS_WORD,          postSubscriptions        }, \
  { "*",      SSR,          SSR_COMPS_V2,         SSR_COMPS_WORD,          badVerbGetPostOnly       }, \
                                                                                                       \
  { "GET",    ISR,          ISR_COMPS_V2,         ISR_COMPS_WORD,          getSubscription          }, \
  { "DELETE", ISR,          ISR_COMPS_V2,         ISR_COMPS_WORD,          deleteSubscription       }, \
  { "PATCH",  ISR,          ISR_COMPS_V2,         ISR_COMPS_WORD,          patchSubscription        }, \
  { "*",      ISR,          ISR_COMPS_V2,         ISR_COMPS_WORD,          badVerbGetDeletePatchOnly}, \
                                                                                                       \
  { "POST",   BQR,          BQR_COMPS_V2,         BQR_COMPS_WORD,          postBatchQuery           }, \
  { "*",      BQR,          BQR_COMPS_V2,         BQR_COMPS_WORD,          badVerbPostOnly          }, \
                                                                                                       \
  { "POST",   BUR,          BUR_COMPS_V2,         BUR_COMPS_WORD,          postBatchUpdate          }, \
  { "*",      BUR,          BUR_COMPS_V2,         BUR_COMPS_WORD,          badVerbPostOnly          }


#define API_V2_CORS                                                                                    \
  { "OPTIONS", EPS,         EPS_COMPS_V2,         ENT_COMPS_WORD,          optionsGetOnly           }, \
  { "OPTIONS", ENT,         ENT_COMPS_V2,         ENT_COMPS_WORD,          optionsGetPostOnly       }, \
  { "OPTIONS", IENT,        IENT_COMPS_V2,        IENT_COMPS_WORD,         optionsGetDeleteOnly     }, \
  { "OPTIONS", IENTOA,      IENTOA_COMPS_V2,      IENTOA_COMPS_WORD,       optionsAllNotDelete      }, \
  { "OPTIONS", IENTATTRVAL, IENTATTRVAL_COMPS_V2, IENTATTRVAL_COMPS_WORD,  optionsGetPutOnly        }, \
  { "OPTIONS", IENTATTR,    IENTATTR_COMPS_V2,    IENTATTR_COMPS_WORD,     optionsGetPutDeleteOnly  }, \
  { "OPTIONS", ENTT,        ENTT_COMPS_V2,        ENTT_COMPS_WORD,         optionsGetOnly           }, \
  { "OPTIONS", ETT,         ETT_COMPS_V2,         ETT_COMPS_WORD,          optionsGetOnly           }, \
  { "OPTIONS", SSR,         SSR_COMPS_V2,         SSR_COMPS_WORD,          optionsGetPostOnly       }, \
  { "OPTIONS", ISR,         ISR_COMPS_V2,         ISR_COMPS_WORD,          optionsGetDeletePatchOnly}, \
  { "OPTIONS", BQR,         BQR_COMPS_V2,         BQR_COMPS_WORD,          optionsPostOnly          }, \
  { "OPTIONS", BUR,         BUR_COMPS_V2,         BUR_COMPS_WORD,          optionsPostOnly          }


#define API_V2_REGISTRY                                                                                \
  { "GET",    REG,          REG_COMPS_V2,         REG_COMPS_WORD,          getRegistration          }, \
  { "*",      REG,          REG_COMPS_V2,         REG_COMPS_WORD,          badVerbGetOnly           }, \
                                                                                                       \
  { "POST",   REGS,         REGS_COMPS_V2,        REGS_COMPS_WORD,         postRegistration         }, \
  { "GET",    REGS,         REGS_COMPS_V2,        REGS_COMPS_WORD,         getRegistrations         }, \
  { "*",      REGS,         REGS_COMPS_V2,        REGS_COMPS_WORD,         badVerbPostOnly          }
  

#define REGISTRY_STANDARD_REQUESTS_V0                                   \
  { "POST",   RCR,   RCR_COMPS_V0,         RCR_POST_WORD,   postRegisterContext                       }, \
  { "*",      RCR,   RCR_COMPS_V0,         RCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   DCAR,  DCAR_COMPS_V0,        DCAR_POST_WORD,  postDiscoverContextAvailability           }, \
  { "*",      DCAR,  DCAR_COMPS_V0,        DCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   SCAR,  SCAR_COMPS_V0,        SCAR_POST_WORD,  postSubscribeContextAvailability          }, \
  { "*",      SCAR,  SCAR_COMPS_V0,        SCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAR,  UCAR_COMPS_V0,        UCAR_POST_WORD,  postUnsubscribeContextAvailability        }, \
  { "*",      UCAR,  UCAR_COMPS_V0,        UCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAS,  UCAS_COMPS_V0,        UCAS_POST_WORD,  postUpdateContextAvailabilitySubscription }, \
  { "*",      UCAS,  UCAS_COMPS_V0,        UCAS_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCAR,  NCAR_COMPS_V0,        NCAR_POST_WORD,  postNotifyContextAvailability             }, \
  { "*",      NCAR,  NCAR_COMPS_V0,        NCAR_POST_WORD,  badVerbPostOnly                           }



#define REGISTRY_STANDARD_REQUESTS_V1                                                                      \
  { "POST",   RCR,   RCR_COMPS_V1,           RCR_POST_WORD,   postRegisterContext                       }, \
  { "*",      RCR,   RCR_COMPS_V1,           RCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   DCAR,  DCAR_COMPS_V1,          DCAR_POST_WORD,  postDiscoverContextAvailability           }, \
  { "*",      DCAR,  DCAR_COMPS_V1,          DCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   SCAR,  SCAR_COMPS_V1,          SCAR_POST_WORD,  postSubscribeContextAvailability          }, \
  { "*",      SCAR,  SCAR_COMPS_V1,          SCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAR,  UCAR_COMPS_V1,          UCAR_POST_WORD,  postUnsubscribeContextAvailability        }, \
  { "*",      UCAR,  UCAR_COMPS_V1,          UCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAS,  UCAS_COMPS_V1,          UCAS_POST_WORD,  postUpdateContextAvailabilitySubscription }, \
  { "*",      UCAS,  UCAS_COMPS_V1,          UCAS_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCAR,  NCAR_COMPS_V1,          NCAR_POST_WORD,  postNotifyContextAvailability             }, \
  { "*",      NCAR,  NCAR_COMPS_V1,          NCAR_POST_WORD,  badVerbPostOnly                           }



#define STANDARD_REQUESTS_V0                                                                             \
  { "POST",   UPCR,  UPCR_COMPS_V0,        UPCR_POST_WORD,  (RestTreat) postUpdateContext             }, \
  { "*",      UPCR,  UPCR_COMPS_V0,        UPCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   QCR,   QCR_COMPS_V0,         QCR_POST_WORD,   postQueryContext                          }, \
  { "*",      QCR,   QCR_COMPS_V0,         QCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   SCR,   SCR_COMPS_V0,         SCR_POST_WORD,   postSubscribeContext                      }, \
  { "*",      SCR,   SCR_COMPS_V0,         SCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   UCSR,  UCSR_COMPS_V0,        UCSR_POST_WORD,  postUpdateContextSubscription             }, \
  { "*",      UCSR,  UCSR_COMPS_V0,        UCSR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UNCR,  UNCR_COMPS_V0,        UNCR_POST_WORD,  postUnsubscribeContext                    }, \
  { "*",      UNCR,  UNCR_COMPS_V0,        UNCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCR,   NCR_COMPS_V0,         NCR_POST_WORD,   postNotifyContext                         }, \
  { "*",      NCR,   NCR_COMPS_V0,         NCR_POST_WORD,   badVerbPostOnly                           }



#define STANDARD_REQUESTS_V1                                                                               \
  { "POST",   UPCR,  UPCR_COMPS_V1,          UPCR_POST_WORD,  (RestTreat) postUpdateContext             }, \
  { "*",      UPCR,  UPCR_COMPS_V1,          UPCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   QCR,   QCR_COMPS_V1,           QCR_POST_WORD,   postQueryContext                          }, \
  { "*",      QCR,   QCR_COMPS_V1,           QCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   SCR,   SCR_COMPS_V1,           SCR_POST_WORD,   postSubscribeContext                      }, \
  { "*",      SCR,   SCR_COMPS_V1,           SCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   UCSR,  UCSR_COMPS_V1,          UCSR_POST_WORD,  postUpdateContextSubscription             }, \
  { "*",      UCSR,  UCSR_COMPS_V1,          UCSR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UNCR,  UNCR_COMPS_V1,          UNCR_POST_WORD,  postUnsubscribeContext                    }, \
  { "*",      UNCR,  UNCR_COMPS_V1,          UNCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCR,   NCR_COMPS_V1,           NCR_POST_WORD,   postNotifyContext                         }, \
  { "*",      NCR,   NCR_COMPS_V1,           NCR_POST_WORD,   badVerbPostOnly                           }



#define REGISTRY_CONVENIENCE_OPERATIONS_V0                                                               \
  { "GET",    CE,    CE_COMPS_V0,          "",              getContextEntitiesByEntityId              }, \
  { "POST",   CE,    CE_COMPS_V0,          CE_POST_WORD,    postContextEntitiesByEntityId             }, \
  { "*",      CE,    CE_COMPS_V0,          "",              badVerbGetPostOnly                        }, \
                                                                                                         \
  { "GET",    CEA,   CEA_COMPS_V0,         "",              getContextEntityAttributes                }, \
  { "POST",   CEA,   CEA_COMPS_V0,         CEA_POST_WORD,   postContextEntityAttributes               }, \
  { "*",      CEA,   CEA_COMPS_V0,         "",              badVerbGetPostOnly                        }, \
                                                                                                         \
  { "GET",    CEAA,  CEAA_COMPS_V0,        "",              getEntityByIdAttributeByName              }, \
  { "POST",   CEAA,  CEAA_COMPS_V0,        CEAA_POST_WORD,  postEntityByIdAttributeByName             }, \
  { "*",      CEAA,  CEAA_COMPS_V0,        "",              badVerbGetPostOnly                        }, \
                                                                                                         \
  { "GET",    CT,    CT_COMPS_V0,          "",              getContextEntityTypes                     }, \
  { "POST",   CT,    CT_COMPS_V0,          CT_POST_WORD,    postContextEntityTypes                    }, \
  { "*",      CT,    CT_COMPS_V0,          "",              badVerbGetPostOnly                        }, \
                                                                                                         \
  { "GET",    CTA,   CTA_COMPS_V0,         "",              getContextEntityTypes                     }, \
  { "POST",   CTA,   CTA_COMPS_V0,         CTA_POST_WORD,   postContextEntityTypes                    }, \
  { "*",      CTA,   CTA_COMPS_V0,         "",              badVerbGetPostOnly                        }, \
                                                                                                         \
  { "GET",    CTAA,  CTAA_COMPS_V0,        "",              getContextEntityTypeAttribute             }, \
  { "POST",   CTAA,  CTAA_COMPS_V0,        CTAA_POST_WORD,  postContextEntityTypeAttribute            }, \
  { "*",      CTAA,  CTAA_COMPS_V0,        "",              badVerbGetPostOnly                        }, \
                                                                                                         \
  { "POST",   SCA,   SCA_COMPS_V0,         SCA_POST_WORD,   postSubscribeContextAvailabilityConvOp    }, \
  { "*",      SCA,   SCA_COMPS_V0,         "",              badVerbPostOnly                           }, \
                                                                                                         \
  { "PUT",    SCAS,  SCAS_COMPS_V0,        SCAS_PUT_WORD,   putAvailabilitySubscriptionConvOp         }, \
  { "DELETE", SCAS,  SCAS_COMPS_V0,        "",              deleteAvailabilitySubscriptionConvOp      }, \
  { "*",      SCAS,  SCAS_COMPS_V0,        "",              badVerbPutDeleteOnly                      }



#define REGISTRY_CONVENIENCE_OPERATIONS_V1                                                                 \
  { "GET",    CE,    CE_COMPS_V1,            "",              getContextEntitiesByEntityId              }, \
  { "POST",   CE,    CE_COMPS_V1,            CE_POST_WORD,    postContextEntitiesByEntityId             }, \
  { "*",      CE,    CE_COMPS_V1,            "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "GET",    CEA,   CEA_COMPS_V1,           "",              getContextEntityAttributes                }, \
  { "POST",   CEA,   CEA_COMPS_V1,           CEA_POST_WORD,   postContextEntityAttributes               }, \
  { "*",      CEA,   CEA_COMPS_V1,           "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "GET",    CEAA,  CEAA_COMPS_V1,          "",              getEntityByIdAttributeByName              }, \
  { "POST",   CEAA,  CEAA_COMPS_V1,          CEAA_POST_WORD,  postEntityByIdAttributeByName             }, \
  { "*",      CEAA,  CEAA_COMPS_V1,          "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "GET",    CT,    CT_COMPS_V1,            "",              getContextEntityTypes                     }, \
  { "POST",   CT,    CT_COMPS_V1,            CT_POST_WORD,    postContextEntityTypes                    }, \
  { "*",      CT,    CT_COMPS_V1,            "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "GET",    CTA,   CTA_COMPS_V1,           "",              getContextEntityTypes                     }, \
  { "POST",   CTA,   CTA_COMPS_V1,           CTA_POST_WORD,   postContextEntityTypes                    }, \
  { "*",      CTA,   CTA_COMPS_V1,           "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "GET",    CTAA,  CTAA_COMPS_V1,          "",              getContextEntityTypeAttribute             }, \
  { "POST",   CTAA,  CTAA_COMPS_V1,          CTAA_POST_WORD,  postContextEntityTypeAttribute            }, \
  { "*",      CTAA,  CTAA_COMPS_V1,          "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "POST",   SCA,   SCA_COMPS_V1,           SCA_POST_WORD,   postSubscribeContextAvailability          }, \
  { "*",      SCA,   SCA_COMPS_V1,           "",              badVerbPostOnly                           }, \
                                                                                                           \
  { "PUT",    SCAS,  SCAS_COMPS_V1,          SCAS_PUT_WORD,   putAvailabilitySubscriptionConvOp         }, \
  { "DELETE", SCAS,  SCAS_COMPS_V1,          "",              deleteAvailabilitySubscriptionConvOp      }, \
  { "*",      SCAS,  SCAS_COMPS_V1,          "",              badVerbPutDeleteOnly                      }



#define CONVENIENCE_OPERATIONS_V0                                                                        \
  { "GET",    ICE,   ICE_COMPS_V0,         "",              getIndividualContextEntity                }, \
  { "PUT",    ICE,   ICE_COMPS_V0,         ICE_PUT_WORD,    putIndividualContextEntity                }, \
  { "POST",   ICE,   ICE_COMPS_V0,         ICE_POST_WORD,   postIndividualContextEntity               }, \
  { "DELETE", ICE,   ICE_COMPS_V0,         "",              deleteIndividualContextEntity             }, \
  { "*",      ICE,   ICE_COMPS_V0,         "",              badVerbAllFour                            }, \
                                                                                                         \
  { "GET",    ICEA,  ICEA_COMPS_V0,        "",              getIndividualContextEntity                }, \
  { "PUT",    ICEA,  ICEA_COMPS_V0,        ICEA_PUT_WORD,   putIndividualContextEntity                }, \
  { "POST",   ICEA,  ICEA_COMPS_V0,        ICEA_POST_WORD,  postIndividualContextEntity               }, \
  { "DELETE", ICEA,  ICEA_COMPS_V0,        "",              deleteIndividualContextEntity             }, \
  { "*",      ICEA,  ICEA_COMPS_V0,        "",              badVerbAllFour                            }, \
                                                                                                         \
  { "GET",    ICEAA, ICEAA_COMPS_V0,       "",              getIndividualContextEntityAttribute       }, \
  { "PUT",    ICEAA, ICEAA_COMPS_V0,       ICEAA_PUT_WORD,  putIndividualContextEntityAttribute       }, \
  { "POST",   ICEAA, ICEAA_COMPS_V0,       ICEAA_POST_WORD, postIndividualContextEntityAttribute      }, \
  { "DELETE", ICEAA, ICEAA_COMPS_V0,       "",              deleteIndividualContextEntityAttribute    }, \
  { "*",      ICEAA, ICEAA_COMPS_V0,       "",              badVerbAllFour                            }, \
                                                                                                         \
  { "GET",    AVI,   AVI_COMPS_V0,         "",              getAttributeValueInstance                 }, \
  { "PUT",    AVI,   AVI_COMPS_V0,         AVI_PUT_WORD,    putAttributeValueInstance                 }, \
  { "DELETE", AVI,   AVI_COMPS_V0,         "",              deleteAttributeValueInstance              }, \
  { "*",      AVI,   AVI_COMPS_V0,         "",              badVerbGetPutDeleteOnly                   }, \
                                                                                                         \
  { "GET",    CET,   CET_COMPS_V0,         "",              getNgsi10ContextEntityTypes               }, \
  { "*",      CET,   CET_COMPS_V0,         "",              badVerbGetOnly                            }, \
                                                                                                         \
  { "GET",    CETA,  CETA_COMPS_V0,        "",              getNgsi10ContextEntityTypes               }, \
  { "*",      CETA,  CETA_COMPS_V0,        "",              badVerbGetOnly                            }, \
                                                                                                         \
  { "GET",    CETAA, CETAA_COMPS_V0,       "",              getNgsi10ContextEntityTypesAttribute      }, \
  { "*",      CETAA, CETAA_COMPS_V0,       "",              badVerbGetOnly                            }, \
                                                                                                         \
  { "POST",   SC,    SC_COMPS_V0,          SC_POST_WORD,    postSubscribeContextConvOp                }, \
  { "*",      SC,    SC_COMPS_V0,          "",              badVerbPostOnly                           }, \
                                                                                                         \
  { "PUT",    SCS,   SCS_COMPS_V0,         SCS_PUT_WORD,    putSubscriptionConvOp                     }, \
  { "DELETE", SCS,   SCS_COMPS_V0,         "",              deleteSubscriptionConvOp                  }, \
  { "*",      SCS,   SCS_COMPS_V0,         "",              badVerbPutDeleteOnly                      }



#define CONVENIENCE_OPERATIONS_V1                                                                          \
  { "GET",    ICE,   ICE_COMPS_V1,           "",              getIndividualContextEntity                }, \
  { "PUT",    ICE,   ICE_COMPS_V1,           ICE_PUT_WORD,    putIndividualContextEntity                }, \
  { "POST",   ICE,   ICE_COMPS_V1,           ICE_POST_WORD,   postIndividualContextEntity               }, \
  { "DELETE", ICE,   ICE_COMPS_V1,           "",              deleteIndividualContextEntity             }, \
  { "*",      ICE,   ICE_COMPS_V1,           "",              badVerbAllFour                            }, \
                                                                                                           \
  { "GET",    ICEA,  ICEA_COMPS_V1,          "",              getIndividualContextEntity                }, \
  { "PUT",    ICEA,  ICEA_COMPS_V1,          ICEA_PUT_WORD,   putIndividualContextEntity                }, \
  { "POST",   ICEA,  ICEA_COMPS_V1,          ICEA_POST_WORD,  postIndividualContextEntity               }, \
  { "DELETE", ICEA,  ICEA_COMPS_V1,          "",              deleteIndividualContextEntity             }, \
  { "*",      ICEA,  ICEA_COMPS_V1,          "",              badVerbAllFour                            }, \
                                                                                                           \
  { "GET",    ICEAA, ICEAA_COMPS_V1,         "",              getIndividualContextEntityAttribute       }, \
  { "PUT",    ICEAA, ICEAA_COMPS_V1,         ICEAA_PUT_WORD,  putIndividualContextEntityAttribute       }, \
  { "POST",   ICEAA, ICEAA_COMPS_V1,         ICEAA_POST_WORD, postIndividualContextEntityAttribute      }, \
  { "DELETE", ICEAA, ICEAA_COMPS_V1,         "",              deleteIndividualContextEntityAttribute    }, \
  { "*",      ICEAA, ICEAA_COMPS_V1,         "",              badVerbAllFour                            }, \
                                                                                                           \
  { "GET",    AVI,   AVI_COMPS_V1,           "",              getAttributeValueInstance                 }, \
  { "PUT",    AVI,   AVI_COMPS_V1,           AVI_PUT_WORD,    putAttributeValueInstance                 }, \
  { "DELETE", AVI,   AVI_COMPS_V1,           "",              deleteAttributeValueInstance              }, \
  { "*",      AVI,   AVI_COMPS_V1,           "",              badVerbGetPutDeleteOnly                   }, \
                                                                                                           \
  { "GET",    CET,   CET_COMPS_V1,           "",              getNgsi10ContextEntityTypes               }, \
  { "*",      CET,   CET_COMPS_V1,           "",              badVerbGetOnly                            }, \
                                                                                                           \
  { "GET",    CETA,  CETA_COMPS_V1,          "",              getNgsi10ContextEntityTypes               }, \
  { "*",      CETA,  CETA_COMPS_V1,          "",              badVerbGetOnly                            }, \
                                                                                                           \
  { "GET",    CETAA, CETAA_COMPS_V1,         "",              getNgsi10ContextEntityTypesAttribute      }, \
  { "*",      CETAA, CETAA_COMPS_V1,         "",              badVerbGetOnly                            }, \
                                                                                                           \
  { "POST",   SC,    SC_COMPS_V1,            SC_POST_WORD,    postSubscribeContextConvOp                }, \
  { "*",      SC,    SC_COMPS_V1,            "",              badVerbPostOnly                           }, \
                                                                                                           \
  { "PUT",    SCS,   SCS_COMPS_V1,           SCS_PUT_WORD,    putSubscriptionConvOp                     }, \
  { "DELETE", SCS,   SCS_COMPS_V1,           "",              deleteSubscriptionConvOp                  }, \
  { "*",      SCS,   SCS_COMPS_V1,           "",              badVerbPutDeleteOnly                      }, \
                                                                                                           \
  { "GET",    ET,    ET_COMPS_V1,            "",              getEntityTypes                            }, \
  { "*",      ET,    ET_COMPS_V1,            "",              badVerbGetOnly                            }, \
                                                                                                           \
  { "GET",    AFET,  AFET_COMPS_V1,          "",              getAttributesForEntityType                }, \
  { "*",      AFET,  AFET_COMPS_V1,          "",              badVerbGetOnly                            }, \
                                                                                                           \
  { "GET",    ACE,   ACE_COMPS_V1,           "",              getAllContextEntities                     }, \
  { "POST",   ACE,   ACE_COMPS_V1,           ACE_POST_WORD,   postIndividualContextEntity               }, \
  { "*",      ACE,   ACE_COMPS_V1,           "",              badVerbGetPostOnly                        }, \
                                                                                                           \
  { "GET",    ACET,  ACET_COMPS_V1,          "",              getAllEntitiesWithTypeAndId               }, \
  { "POST",   ACET,  ACET_COMPS_V1,          ACET_POST_WORD,  postAllEntitiesWithTypeAndId              }, \
  { "PUT",    ACET,  ACET_COMPS_V1,          ACET_PUT_WORD,   putAllEntitiesWithTypeAndId               }, \
  { "DELETE", ACET,  ACET_COMPS_V1,          "",              deleteAllEntitiesWithTypeAndId            }, \
  { "*",      ACET,  ACET_COMPS_V1,          "",              badVerbAllFour                            }, \
                                                                                                           \
  { "GET",    ICEAAT,  ICEAAT_COMPS_V1,      "",               getIndividualContextEntityAttributeWithTypeAndId    }, \
  { "POST",   ICEAAT,  ICEAAT_COMPS_V1,      ICEAAT_POST_WORD, postIndividualContextEntityAttributeWithTypeAndId   }, \
  { "PUT",    ICEAAT,  ICEAAT_COMPS_V1,      ICEAAT_PUT_WORD,  putIndividualContextEntityAttributeWithTypeAndId    }, \
  { "DELETE", ICEAAT,  ICEAAT_COMPS_V1,      "",               deleteIndividualContextEntityAttributeWithTypeAndId }, \
  { "*",      ICEAAT,  ICEAAT_COMPS_V1,      "",               badVerbAllFour                                      }, \
                                                                                                                      \
  { "GET",    AVIT,    AVIT_COMPS_V1,        "",               getAttributeValueInstanceWithTypeAndId              }, \
  { "POST",   AVIT,    AVIT_COMPS_V1,        AVIT_POST_WORD,   postAttributeValueInstanceWithTypeAndId             }, \
  { "PUT",    AVIT,    AVIT_COMPS_V1,        AVIT_PUT_WORD,    putAttributeValueInstanceWithTypeAndId              }, \
  { "DELETE", AVIT,    AVIT_COMPS_V1,        "",               deleteAttributeValueInstanceWithTypeAndId           }, \
  { "*",      AVIT,    AVIT_COMPS_V1,        "",               badVerbAllFour                                      }, \
                                                                                                                      \
  { "GET",    CEET,    CEET_COMPS_V1,        "",               getContextEntitiesByEntityIdAndType                 }, \
  { "POST",   CEET,    CEET_COMPS_V1,        CEET_POST_WORD,   postContextEntitiesByEntityIdAndType                }, \
  { "*",      CEET,    CEET_COMPS_V1,        "",               badVerbGetPostOnly                                  }, \
                                                                                                                      \
  { "GET",    CEAAT,   CEAAT_COMPS_V1,       "",               getEntityByIdAttributeByNameWithTypeAndId           }, \
  { "POST",   CEAAT,   CEAAT_COMPS_V1,       CEAAT_POST_WORD,  postEntityByIdAttributeByNameWithTypeAndId          }, \
  { "*",      CEAAT,   CEAAT_COMPS_V1,       "",               badVerbGetPostOnly                                  }



/* *****************************************************************************
*
* log requests
* The documentation (Installation and Admin Guide) says /log/trace ...
* ... and to maintain backward compatibility we keep supporting /log/traceLevel too
*/
#define LOG_REQUESTS_V0                                                              \
  { "GET",    LOG,  LOGT_COMPS_V0,    "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGT_COMPS_V0,    "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGT_COMPS_V0,    "",  badVerbGetDeleteOnly                   }, \
  { "PUT",    LOG,  LOGTL_COMPS_V0,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGTL_COMPS_V0,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGTL_COMPS_V0,   "",  badVerbPutDeleteOnly                   }, \
  { "GET",    LOG,  LOG2T_COMPS_V0,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2T_COMPS_V0,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2T_COMPS_V0,   "",  badVerbGetDeleteOnly                   }, \
  { "PUT",    LOG,  LOG2TL_COMPS_V0,  "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2TL_COMPS_V0,  "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2TL_COMPS_V0,  "",  badVerbPutDeleteOnly                   }

#define LOG_REQUESTS_V1                                                              \
  { "GET",    LOG,  LOGT_COMPS_V1,    "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGT_COMPS_V1,    "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGT_COMPS_V1,    "",  badVerbGetDeleteOnly                   }, \
  { "PUT",    LOG,  LOGTL_COMPS_V1,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGTL_COMPS_V1,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGTL_COMPS_V1,   "",  badVerbPutDeleteOnly                   }, \
  { "GET",    LOG,  LOG2T_COMPS_V1,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2T_COMPS_V1,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2T_COMPS_V1,   "",  badVerbGetDeleteOnly                   }, \
  { "PUT",    LOG,  LOG2TL_COMPS_V1,  "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2TL_COMPS_V1,  "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2TL_COMPS_V1,  "",  badVerbPutDeleteOnly                   }

#define STAT_REQUESTS_V0                                                             \
  { "GET",    STAT, STAT_COMPS_V0,    "",  statisticsTreat                        }, \
  { "DELETE", STAT, STAT_COMPS_V0,    "",  statisticsTreat                        }, \
  { "*",      STAT, STAT_COMPS_V0,    "",  badVerbGetDeleteOnly                   }

#define STAT_REQUESTS_V1                                                             \
  { "GET",    STAT, STAT_COMPS_V1,    "",  statisticsTreat                        }, \
  { "DELETE", STAT, STAT_COMPS_V1,    "",  statisticsTreat                        }, \
  { "*",      STAT, STAT_COMPS_V1,    "",  badVerbGetDeleteOnly                   }

#define STAT_CACHE_REQUESTS_V0                                                       \
  { "GET",    STAT, STAT_CACHE_COMPS_V0,    "",  statisticsCacheTreat             }, \
  { "DELETE", STAT, STAT_CACHE_COMPS_V0,    "",  statisticsCacheTreat             }, \
  { "*",      STAT, STAT_CACHE_COMPS_V0,    "",  badVerbGetDeleteOnly             }

#define STAT_CACHE_REQUESTS_V1                                                       \
  { "GET",    STAT, STAT_CACHE_COMPS_V1,    "",  statisticsCacheTreat             }, \
  { "DELETE", STAT, STAT_CACHE_COMPS_V1,    "",  statisticsCacheTreat             }, \
  { "*",      STAT, STAT_CACHE_COMPS_V1,    "",  badVerbGetDeleteOnly             }

#define VERSION_REQUESTS                                                             \
  { "GET",    VERS, VERS_COMPS,    "",  versionTreat                              }, \
  { "*",      VERS, VERS_COMPS,    "",  badVerbGetOnly                            }

#define EXIT_REQUESTS                                                                \
  { "GET",    EXIT, EXIT2_COMPS,   "",  exitTreat                                 }, \
  { "GET",    EXIT, EXIT1_COMPS,   "",  exitTreat                                 }

#define LEAK_REQUESTS                                                                \
  { "GET",    LEAK, LEAK2_COMPS,   "",  leakTreat                                 }, \
  { "GET",    LEAK, LEAK1_COMPS,   "",  leakTreat                                 }

#define INVALID_REQUESTS                             \
  { "*", INV, INV9_COMPS,    "", badNgsi9Request  }, \
  { "*", INV, INV10_COMPS,   "", badNgsi10Request }, \
  { "*", INV, INV_ALL_COMPS, "", badRequest       }

#define LOGLEVEL_REQUESTS_V2                                                         \
  { "PUT",   LOGLEVEL,  LOGLEVEL_COMPS_V2, "", changeLogLevel                     }, \
  { "GET",   LOGLEVEL,  LOGLEVEL_COMPS_V2, "", getLogLevel                        }, \
  { "*",     LOGLEVEL,  LOGLEVEL_COMPS_V2, "", badVerbPutOnly                     }

#define SEM_STATE_REQUESTS                                                           \
  { "GET",   SEM_STATE, SEM_STATE_COMPS,   "", semStateTreat                      }, \
  { "*",     SEM_STATE, SEM_STATE_COMPS,   "", badVerbGetOnly                     }

#define METRICS_REQUESTS                                                             \
  { "GET",    METRICS, METRICS_COMPS,   "", getMetrics                            }, \
  { "DELETE", METRICS, METRICS_COMPS,   "", deleteMetrics                         }, \
  { "*",      METRICS, METRICS_COMPS,   "", badVerbGetDeleteOnly                  }



/* ****************************************************************************
*
* END_REQUEST - End marker for the array
*/
#define END_REQUEST  { "", INV,  0, {}, "", NULL }



/* ****************************************************************************
*
* restServiceV - services for BROKER (ngsi9/10)
*
* This is the default service vector, that is used if the broker is started without the -corsOrigin option
*/
RestService restServiceV[] =
{
  API_V2,
  API_V2_REGISTRY,

  REGISTRY_STANDARD_REQUESTS_V0,
  REGISTRY_STANDARD_REQUESTS_V1,
  STANDARD_REQUESTS_V0,
  STANDARD_REQUESTS_V1,

  REGISTRY_CONVENIENCE_OPERATIONS_V0,
  REGISTRY_CONVENIENCE_OPERATIONS_V1,
  CONVENIENCE_OPERATIONS_V0,
  CONVENIENCE_OPERATIONS_V1,
  LOG_REQUESTS_V0,
  LOG_REQUESTS_V1,
  STAT_REQUESTS_V0,
  STAT_REQUESTS_V1,
  STAT_CACHE_REQUESTS_V0,
  STAT_CACHE_REQUESTS_V1,
  VERSION_REQUESTS,
  LOGLEVEL_REQUESTS_V2,
  SEM_STATE_REQUESTS,
  METRICS_REQUESTS,

#ifdef DEBUG
  EXIT_REQUESTS,
  LEAK_REQUESTS,
#endif

  INVALID_REQUESTS,
  END_REQUEST
};



/* ****************************************************************************
*
* restServiceCORS
*
* Adds API_V2_CORS definitions on top of the default service vector (restServiceV)
*/
RestService restServiceCORS[] =
{
  API_V2_CORS,
  API_V2,
  API_V2_REGISTRY,

  REGISTRY_STANDARD_REQUESTS_V0,
  REGISTRY_STANDARD_REQUESTS_V1,
  STANDARD_REQUESTS_V0,
  STANDARD_REQUESTS_V1,

  REGISTRY_CONVENIENCE_OPERATIONS_V0,
  REGISTRY_CONVENIENCE_OPERATIONS_V1,
  CONVENIENCE_OPERATIONS_V0,
  CONVENIENCE_OPERATIONS_V1,
  LOG_REQUESTS_V0,
  LOG_REQUESTS_V1,
  STAT_REQUESTS_V0,
  STAT_REQUESTS_V1,
  STAT_CACHE_REQUESTS_V0,
  STAT_CACHE_REQUESTS_V1,
  VERSION_REQUESTS,
  LOGLEVEL_REQUESTS_V2,
  SEM_STATE_REQUESTS,
  METRICS_REQUESTS,

#ifdef DEBUG
  EXIT_REQUESTS,
  LEAK_REQUESTS,
#endif

  INVALID_REQUESTS,
  END_REQUEST
};



/* ****************************************************************************
*
* pidFile -
*/
int pidFile(void)
{
  int    fd = open(pidPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  pid_t  pid;
  char   buffer[32];
  int    sz;
  int    nb;

  if (fd == -1)
  {
    LM_E(("PID File (open '%s': %s", pidPath, strerror(errno)));
    return -1;
  }

  pid = getpid();

  snprintf(buffer, sizeof(buffer), "%d", pid);
  sz = strlen(buffer);
  nb = write(fd, buffer, sz);
  if (nb != sz)
  {
    LM_E(("PID File (written %d bytes and not %d to '%s': %s)", nb, sz, pidPath, strerror(errno)));
    return -2;
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
    int rc = pQNotifier->start();
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

#define LOG_FILE_LINE_FORMAT "time=DATE | lvl=TYPE | corr=CORR_ID | trans=TRANS_ID | from=FROM_IP | srv=SERVICE | subsrv=SUB_SERVICE | comp=Orion | op=FILE[LINE]:FUNC | msg=TEXT"
/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{
  lmTransactionReset();

  uint16_t       rushPort = 0;
  std::string    rushHost = "";

  signal(SIGINT,  sigHandler);
  signal(SIGTERM, sigHandler);

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
  paConfig("log to file",                   (void*) true);
  paConfig("log file line format",          (void*) LOG_FILE_LINE_FORMAT);
  paConfig("log file time format",          (void*) "%Y-%m-%dT%H:%M:%S");
  paConfig("builtin prefix",                (void*) "ORION_");
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

    if (paIsSet(argC, argV, "-logForHumans"))
    {
      paConfig("screen line format", (void*) "TYPE@TIME  FILE[LINE]: TEXT");
    }
    else
    {
      paConfig("screen line format", LOG_FILE_LINE_FORMAT);
    }
  }

  paParse(paArgs, argC, (char**) argV, 1, false);
  lmTimeFormat(0, (char*) "%Y-%m-%dT%H:%M:%S");

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

  if (fg == false)
  {
    daemonize();
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

  RestService* rsP       = (strlen(allowedOrigin) > 0) ? restServiceCORS : restServiceV;
  IpVersion    ipVersion = IPDUAL;

  if (useOnlyIPv4)
  {
    ipVersion = IPV4;
  }
  else if (useOnlyIPv6)
  {
    ipVersion = IPV6;
  }


  //
  //  Where everything begins
  //

  pidFile();
  SemOpType policy = policyGet(reqMutexPolicy);
  orionInit(orionExit, ORION_VERSION, policy, statCounters, statSemWait, statTiming, statNotifQueue, strictIdv1);
  mongoInit(dbHost, rplSet, dbName, user, pwd, mtenant, dbTimeout, writeConcern, dbPoolSize, statSemWait);
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

    restInit(rsP,
             ipVersion,
             bindAddress,
             port,
             mtenant,
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
    restInit(rsP,
             ipVersion,
             bindAddress,
             port,
             mtenant,
             connectionMemory,
             maxConnections,
             reqPoolSize,
             rushHost,
             rushPort,
             allowedOrigin,
             maxAge,
             reqTimeout);
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
