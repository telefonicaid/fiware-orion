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
#include <string>
#include <vector>

#include "mongoBackend/MongoGlobal.h"

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "parseArgs/paBuiltin.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "xmlParse/xmlRequest.h"
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

#include "orionTypes/EntityTypesResponse.h"

#include "serviceRoutines/logTraceTreat.h"

#include "ngsi/ParseData.h"
#include "ngsiNotify/onTimeIntervalThread.h"

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
#include "serviceRoutines/postIndividualContextEntity.h"
#include "serviceRoutines/deleteIndividualContextEntity.h"
#include "serviceRoutines/badVerbAllFour.h"
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
#include "serviceRoutines/badNgsi9Request.h"
#include "serviceRoutines/badNgsi10Request.h"
#include "serviceRoutines/badRequest.h"

#include "serviceRoutinesV2/getEntities.h"

#include "contextBroker/version.h"

#include "common/string.h"



/* ****************************************************************************
*
* DB_NAME_MAX_LEN - max length of database name
*/
#define DB_NAME_MAX_LEN  10



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
char            fwdHost[64];
int             fwdPort;
bool            ngsi9Only;
bool            harakiri;
bool            useOnlyIPv4;
bool            useOnlyIPv6;
char            httpsKeyFile[1024];
char            httpsCertFile[1024];
bool            https;
bool            mtenant;
char            rush[256];
char            allowedOrigin[64];
long            dbTimeout;
long            httpTimeout;
int             dbPoolSize;
char            reqMutexPolicy[16];
bool            mutexTimeStat;
int             writeConcern;



/* ****************************************************************************
*
* Definitions to make paArgs lines shorter ...
*/
#define PIDPATH             _i "/tmp/contextBroker.pid"
#define IP_ALL              _i "0.0.0.0"
#define LOCALHOST           _i "localhost"

#define FG_DESC             "don't start as daemon"
#define LOCALIP_DESC        "IP to receive new connections"
#define PORT_DESC           "port to receive new connections"
#define PIDPATH_DESC        "pid file path"
#define DBHOST_DESC         "database host"
#define RPLSET_DESC         "replica set"
#define DBUSER_DESC         "database user"
#define DBPASSWORD_DESC     "database password"
#define DB_DESC             "database name"
#define DB_TMO_DESC         "timeout in milliseconds for connections to the replica set (ignored in the case of not using replica set)"
#define FWDHOST_DESC        "host for forwarding NGSI9 regs"
#define FWDPORT_DESC        "port for forwarding NGSI9 regs"
#define NGSI9_DESC          "run as Configuration Manager"
#define USEIPV4_DESC        "use ip v4 only"
#define USEIPV6_DESC        "use ip v6 only"
#define HARAKIRI_DESC       "commits harakiri on request"
#define HTTPS_DESC          "use the https 'protocol'"
#define HTTPSKEYFILE_DESC   "private server key file (for https)"
#define HTTPSCERTFILE_DESC  "certificate key file (for https)"
#define RUSH_DESC           "rush host (IP:port)"
#define MULTISERVICE_DESC   "service multi tenancy mode"
#define ALLOWED_ORIGIN_DESC "CORS allowed origin. use '__ALL' for any"
#define HTTP_TMO_DESC       "timeout in milliseconds for forwards and notifications"
#define DBPS_DESC           "database connection pool size"
#define MAX_L               900000
#define MUTEX_POLICY_DESC   "mutex policy (none/read/write/all)"
#define MUTEX_TIMESTAT_DESC "measure total semaphore waiting time"
#define WRITE_CONCERN_DESC  "db write concern (0:unacknowledged, 1:acknowledged)"


/* ****************************************************************************
*
* parse arguments
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

  { "-fwdHost",       fwdHost,       "FWD_HOST",       PaString, PaOpt, LOCALHOST,  PaNL,   PaNL,  FWDHOST_DESC       },
  { "-fwdPort",       &fwdPort,      "FWD_PORT",       PaInt,    PaOpt, 0,          0,      65000, FWDPORT_DESC       },
  { "-ngsi9",         &ngsi9Only,    "CONFMAN",        PaBool,   PaOpt, false,      false,  true,  NGSI9_DESC         },
  { "-ipv4",          &useOnlyIPv4,  "USEIPV4",        PaBool,   PaOpt, false,      false,  true,  USEIPV4_DESC       },
  { "-ipv6",          &useOnlyIPv6,  "USEIPV6",        PaBool,   PaOpt, false,      false,  true,  USEIPV6_DESC       },
  { "-harakiri",      &harakiri,     "HARAKIRI",       PaBool,   PaHid, false,      false,  true,  HARAKIRI_DESC      },

  { "-https",         &https,        "HTTPS",          PaBool,   PaOpt, false,      false,  true,  HTTPS_DESC         },
  { "-key",           httpsKeyFile,  "HTTPS_KEYFILE",  PaString, PaOpt, _i "",      PaNL,   PaNL,  HTTPSKEYFILE_DESC  },
  { "-cert",          httpsCertFile, "HTTPS_CERTFILE", PaString, PaOpt, _i "",      PaNL,   PaNL,  HTTPSCERTFILE_DESC },

  { "-rush",          rush,          "RUSH",           PaString, PaOpt, _i "",      PaNL,   PaNL,  RUSH_DESC          },
  { "-multiservice",  &mtenant,      "MULTI_SERVICE",  PaBool,   PaOpt, false,      false,  true,  MULTISERVICE_DESC  },

  { "-httpTimeout",   &httpTimeout,  "HTTP_TIMEOUT",   PaLong,   PaOpt, -1,         -1,     MAX_L, HTTP_TMO_DESC      },
  { "-reqMutexPolicy",reqMutexPolicy,"MUTEX_POLICY",   PaString, PaOpt, _i "all",   PaNL,   PaNL,  MUTEX_POLICY_DESC  },
  { "-mutexTimeStat", &mutexTimeStat,"MUTEX_TIME_STAT",PaBool,   PaOpt, false,      false,  true,  MUTEX_TIMESTAT_DESC},
  { "-writeConcern",  &writeConcern, "WRITE_CONCERN",  PaInt,    PaOpt, 1,          0,      1,     WRITE_CONCERN_DESC },

  { "-corsOrigin",    allowedOrigin, "ALLOWED_ORIGIN", PaString, PaOpt, _i "",      PaNL,   PaNL,  ALLOWED_ORIGIN_DESC},

  PA_END_OF_ARGS
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
#define ENT                EntitiesRequest
#define ENT_COMPS_V2       2, { "v2", "entities" }
#define ENT_COMPS_WORD     ""

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
#define LOG                LogRequest
#define LOGT_COMPS_V0      2, { "log", "trace"                           }
#define LOGTL_COMPS_V0     3, { "log", "trace",      "*"                 }
#define LOG2T_COMPS_V0     2, { "log", "traceLevel"                      }
#define LOG2TL_COMPS_V0    3, { "log", "traceLevel", "*"                 }
#define LOGT_COMPS_V1      4, { "v1", "admin", "log", "trace"            }
#define LOGTL_COMPS_V1     5, { "v1", "admin", "log", "trace",      "*"  }
#define LOG2T_COMPS_V1     4, { "v1", "admin", "log", "traceLevel"       }
#define LOG2TL_COMPS_V1    5, { "v1", "admin", "log", "traceLevel", "*"  }

#define STAT               StatisticsRequest
#define STAT_COMPS_V0      1, { "statistics"                             }
#define STAT_COMPS_V1      3, { "v1", "admin", "statistics"              }



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



#define API_V2                                                                                           \
  { "GET",    ENT,   ENT_COMPS_V2,         ENT_COMPS_WORD,  getEntities                               }, \
  { "*",      ENT,   ENT_COMPS_V2,         ENT_COMPS_WORD,  badVerbGetOnly                            }



#define REGISTRY_STANDARD_REQUESTS_V0                                                                    \
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
  { "POST",   UPCR,  UPCR_COMPS_V0,        UPCR_POST_WORD,  postUpdateContext                         }, \
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
  { "POST",   UPCR,  UPCR_COMPS_V1,          UPCR_POST_WORD,  postUpdateContext                         }, \
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
  { "*",      ICEAA, ICEAA_COMPS_V0,       "",              badVerbGetPostDeleteOnly                  }, \
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
  { "*",      ICEAA, ICEAA_COMPS_V1,         "",              badVerbGetPostDeleteOnly                  }, \
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
  { "GET",    AFET,  AFET_COMPS_V1,          "",              getAttributesForEntityType                }, \
  { "*",      AFET,  AFET_COMPS_V1,          "",              badVerbGetOnly                            }, \
                                                                                                           \
  { "GET",    ACE,   ACE_COMPS_V1,           "",              getAllContextEntities                     }, \
  { "POST",   ACE,   ACE_COMPS_V1,           ACE_POST_WORD,   postIndividualContextEntity               }, \
  { "*",      ACE,   ACE_COMPS_V1,           "",              badVerbGetOnly                            }, \
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
  { "*",      LOG,  LOGT_COMPS_V0,    "",  badVerbAllFour                         }, \
  { "PUT",    LOG,  LOGTL_COMPS_V0,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGTL_COMPS_V0,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGTL_COMPS_V0,   "",  badVerbAllFour                         }, \
  { "GET",    LOG,  LOG2T_COMPS_V0,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2T_COMPS_V0,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2T_COMPS_V0,   "",  badVerbAllFour                         }, \
  { "PUT",    LOG,  LOG2TL_COMPS_V0,  "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2TL_COMPS_V0,  "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2TL_COMPS_V0,  "",  badVerbAllFour                         }

#define LOG_REQUESTS_V1                                                              \
  { "GET",    LOG,  LOGT_COMPS_V1,    "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGT_COMPS_V1,    "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGT_COMPS_V1,    "",  badVerbAllFour                         }, \
  { "PUT",    LOG,  LOGTL_COMPS_V1,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOGTL_COMPS_V1,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOGTL_COMPS_V1,   "",  badVerbAllFour                         }, \
  { "GET",    LOG,  LOG2T_COMPS_V1,   "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2T_COMPS_V1,   "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2T_COMPS_V1,   "",  badVerbAllFour                         }, \
  { "PUT",    LOG,  LOG2TL_COMPS_V1,  "",  logTraceTreat                          }, \
  { "DELETE", LOG,  LOG2TL_COMPS_V1,  "",  logTraceTreat                          }, \
  { "*",      LOG,  LOG2TL_COMPS_V1,  "",  badVerbAllFour                         }

#define STAT_REQUESTS_V0                                                             \
  { "GET",    STAT, STAT_COMPS_V0,    "",  statisticsTreat                        }, \
  { "DELETE", STAT, STAT_COMPS_V0,    "",  statisticsTreat                        }, \
  { "*",      STAT, STAT_COMPS_V0,    "",  badVerbGetDeleteOnly                   }

#define STAT_REQUESTS_V1                                                             \
  { "GET",    STAT, STAT_COMPS_V1,    "",  statisticsTreat                        }, \
  { "DELETE", STAT, STAT_COMPS_V1,    "",  statisticsTreat                        }, \
  { "*",      STAT, STAT_COMPS_V1,    "",  badVerbGetDeleteOnly                   }

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



/* ****************************************************************************
*
* END_REQUEST - End marker for the array
*/
#define END_REQUEST  { "", INV,  0, {}, "", NULL }



/* ****************************************************************************
*
* restServiceV - services for BROKER (ngsi9/10)
*
* This is the default service vector, that is used if the broker is started without the -ngsi9 option
*/
RestService restServiceV[] =
{
  API_V2,

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
  VERSION_REQUESTS,

#ifdef DEBUG
  EXIT_REQUESTS,
  LEAK_REQUESTS,
#endif

  INVALID_REQUESTS,
  END_REQUEST
};



/* ****************************************************************************
*
* restServiceNgsi9 - services for CONF MAN
*
* This service vector (configuration) is used if the broker is started as
* CONFIGURATION MANAGER (using the -ngsi9 option) and without using the
* -multiservice option.
*/
RestService restServiceNgsi9[] =
{
  REGISTRY_STANDARD_REQUESTS_V0,   // FIXME P10:  NCAR is added here, is that OK?
  REGISTRY_STANDARD_REQUESTS_V1,
  REGISTRY_CONVENIENCE_OPERATIONS_V0,
  REGISTRY_CONVENIENCE_OPERATIONS_V1,
  LOG_REQUESTS_V0,
  LOG_REQUESTS_V1,
  STAT_REQUESTS_V0,
  STAT_REQUESTS_V1,
  VERSION_REQUESTS,

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
  OID::justForked();
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

  //
  // Cancel all threads to avoid false leaks in valgrind
  //
  std::vector<std::string> dbs;
  getOrionDatabases(dbs);
  for (unsigned int ix = 0; ix < dbs.size(); ++ix)
  {
    destroyAllOntimeIntervalThreads(dbs[ix]);
  }

  mongoDisconnect();
  exit(code);
}



/* ****************************************************************************
*
* exitFunc -
*/
void exitFunc(void)
{
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
static void contextBrokerInit(bool ngsi9Only, std::string dbPrefix, bool multitenant)
{
  /* Set notifier object (singleton) */
  setNotifier(new Notifier());

  /* Launch threads corresponding to ONTIMEINTERVAL subscriptions in the database (unless ngsi9 only mode) */
  if (!ngsi9Only)
  {
    recoverOntimeIntervalThreads("");

    if (multitenant)
    {
      /* We get tenant database names and recover ontime interval threads on each one */
      std::vector<std::string> orionDbs;
      getOrionDatabases(orionDbs);
      for (unsigned int ix = 0; ix < orionDbs.size(); ++ix)
      {
        std::string orionDb = orionDbs[ix];
        std::string tenant = orionDb.substr(dbPrefix.length() + 1);   // + 1 for the "_" in "orion_tenantA"
        recoverOntimeIntervalThreads(tenant);
      }
    }
  }
  else
  {
    LM_I(("Running in NGSI9 only mode"));
  }

  httpRequestInit(httpTimeout);
}



/* ****************************************************************************
*
* mongoInit -
*/
static void mongoInit
(
  const char*  dbHost,
  const char*  rplSet,
  std::string  dbName,
  const char*  user,
  const char*  pwd,
  long         timeout,
  int          writeConcern,
  int          dbPoolSize,
  bool         mutexTimeStat
)
{
  double tmo = timeout / 1000.0;  // milliseconds to float value in seconds

  if (!mongoStart(dbHost, dbName.c_str(), rplSet, user, pwd, mtenant, tmo, writeConcern, dbPoolSize, mutexTimeStat))
  {
    LM_X(1, ("Fatal Error (MongoDB error)"));
  }

  if (user[0] != 0)
  {
    LM_I(("Connected to mongo at %s:%s as user '%s'", dbHost, dbName.c_str(), user));
  }
  else
  {
    LM_I(("Connected to mongo at %s:%s", dbHost, dbName.c_str()));
  }

  setDbPrefix(dbName);
  setEntitiesCollectionName(COL_ENTITIES);
  setRegistrationsCollectionName(COL_REGISTRATIONS);
  setSubscribeContextCollectionName(COL_CSUBS);
  setSubscribeContextAvailabilityCollectionName(COL_CASUBS);
  setAssociationsCollectionName(COL_ASSOCIATIONS);

  //
  // Note that index creation operation is idempotent.
  // From http://docs.mongodb.org/manual/reference/method/db.collection.ensureIndex:
  // "If you call multiple ensureIndex() methods with the same index specification at the same time,
  // only the first operation will succeed, all other operations will have no effect."
  //
  ensureLocationIndex("");
  if (mtenant)
  {
    /* We get tenant database names and apply ensure the location index in each one */
    std::vector<std::string> orionDbs;
    getOrionDatabases(orionDbs);
    for (unsigned int ix = 0; ix < orionDbs.size(); ++ix)
    {
      std::string orionDb = orionDbs[ix];
      std::string tenant = orionDb.substr(dbName.length() + 1);   // + 1 for the "_" in "orion_tenantA"
      ensureLocationIndex(tenant);
    }
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
static SemRequestType policyGet(std::string mutexPolicy)
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



#define LOG_FILE_LINE_FORMAT "time=DATE | lvl=TYPE | trans=TRANS_ID | function=FUNC | comp=Orion | msg=FILE[LINE]: TEXT"
/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{
  strncpy(transactionId, "N/A", sizeof(transactionId));

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

  paConfig("man exitstatus", (void*) "The orion broker is a daemon. If it exits, something is wrong ...");

  paConfig("man synopsis",                  (void*) "[options]");
  paConfig("man shortdescription",          (void*) "Options:");
  paConfig("man description",               (void*) description);
  paConfig("man author",                    (void*) "Telefonica I+D");
  paConfig("man version",                   (void*) ORION_VERSION);
  paConfig("log to screen",                 (void*) true);
  paConfig("log to file",                   (void*) true);
  paConfig("log file line format",          (void*) LOG_FILE_LINE_FORMAT);
  paConfig("screen line format",            (void*) "TYPE@TIME  FILE[LINE]: TEXT");
  paConfig("builtin prefix",                (void*) "ORION_");
  paConfig("usage and exit on any warning", (void*) true);
  paConfig("no preamble",                   NULL);
  paConfig("log file time format",          (void*) "%Y-%m-%dT%H:%M:%S");

  paParse(paArgs, argC, (char**) argV, 1, false);
  lmTimeFormat(0, (char*) "%Y-%m-%dT%H:%M:%S");

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

  LM_I(("Orion Context Broker is running"));

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

  RestService* rsP       = (ngsi9Only == true)? restServiceNgsi9 : restServiceV;
  IpVersion    ipVersion = IPDUAL;

  if (useOnlyIPv4)
  {
    ipVersion = IPV4;
  }
  else if (useOnlyIPv6)
  {
    ipVersion = IPV6;
  }

  pidFile();
  SemRequestType policy = policyGet(reqMutexPolicy);
  orionInit(orionExit, ORION_VERSION, policy, mutexTimeStat);
  mongoInit(dbHost, rplSet, dbName, user, pwd, dbTimeout, writeConcern, dbPoolSize, mutexTimeStat);
  contextBrokerInit(ngsi9Only, dbName, mtenant);
  curl_global_init(CURL_GLOBAL_NOTHING);

  if (rush[0] != 0)
  {
    rushParse(rush, &rushHost, &rushPort);
    LM_T(LmtRush, ("rush host: '%s', rush port: %d", rushHost.c_str(), rushPort));
  }

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

    restInit(rsP, ipVersion, bindAddress, port, mtenant, rushHost, rushPort, allowedOrigin, httpsPrivateServerKey, httpsCertificate);

    free(httpsPrivateServerKey);
    free(httpsCertificate);
  }
  else
  {
    restInit(rsP, ipVersion, bindAddress, port, mtenant, rushHost, rushPort, allowedOrigin);
  }

  LM_I(("Startup completed"));

  while (1)
  {
    sleep(10);
  }
}
