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
* fermin at tid dot es
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

#include "common/sem.h"
#include "common/globals.h"
#include "common/Timer.h"
#include "common/compileInfo.h"

#include "serviceRoutines/logTraceTreat.h"

#include "ngsi/ParseData.h"
#include "ngsiNotify/onTimeIntervalThread.h"

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
#include "serviceRoutines/getIndividualContextEntityAttributes.h"
#include "serviceRoutines/putIndividualContextEntityAttributes.h"
#include "serviceRoutines/putIndividualContextEntityAttribute.h"
#include "serviceRoutines/postIndividualContextEntityAttributes.h"
#include "serviceRoutines/deleteIndividualContextEntityAttributes.h"
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

#include "serviceRoutines/badVerbGetPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostDeleteOnly.h"
#include "serviceRoutines/badVerbGetOnly.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutines/badNgsi9Request.h"
#include "serviceRoutines/badNgsi10Request.h"
#include "serviceRoutines/badRequest.h"
#include "contextBroker/version.h"

#include "common/string.h"



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
char            mtenant[32];
char            rush[256];



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
#define MULTISERVICE_DESC   "service multi tenancy mode (off|url|header)"



/* ****************************************************************************
*
* parse arguments
*/
PaArgument paArgs[] =
{
  { "-fg",           &fg,           "FOREGROUND",     PaBool,   PaOpt, false,      false,  true,  FG_DESC            },
  { "-localIp",      bindAddress,   "LOCALIP",        PaString, PaOpt, IP_ALL,     PaNL,   PaNL,  LOCALIP_DESC       },
  { "-port",         &port,         "PORT",           PaInt,    PaOpt, 1026,       PaNL,   PaNL,  PORT_DESC          },
  { "-pidpath",      pidPath,       "PID_PATH",       PaString, PaOpt, PIDPATH,    PaNL,   PaNL,  PIDPATH_DESC       },

  { "-dbhost",       dbHost,        "DB_HOST",        PaString, PaOpt, LOCALHOST,  PaNL,   PaNL,  DBHOST_DESC        },
  { "-rplSet",       rplSet,        "RPL_SET",        PaString, PaOpt, _i "",      PaNL,   PaNL,  RPLSET_DESC        },
  { "-dbuser",       user,          "DB_USER",        PaString, PaOpt, _i "",      PaNL,   PaNL,  DBUSER_DESC        },
  { "-dbpwd",        pwd,           "DB_PASSWORD",    PaString, PaOpt, _i "",      PaNL,   PaNL,  DBPASSWORD_DESC    },
  { "-db",           dbName,        "DB",             PaString, PaOpt, _i "orion", PaNL,   PaNL,  DB_DESC            },

  { "-fwdHost",      fwdHost,       "FWD_HOST",       PaString, PaOpt, LOCALHOST,  PaNL,   PaNL,  FWDHOST_DESC       },
  { "-fwdPort",      &fwdPort,      "FWD_PORT",       PaInt,    PaOpt, 0,          0,      65000, FWDPORT_DESC       },
  { "-ngsi9",        &ngsi9Only,    "CONFMAN",        PaBool,   PaOpt, false,      false,  true,  NGSI9_DESC         },
  { "-ipv4",         &useOnlyIPv4,  "USEIPV4",        PaBool,   PaOpt, false,      false,  true,  USEIPV4_DESC       },
  { "-ipv6",         &useOnlyIPv6,  "USEIPV6",        PaBool,   PaOpt, false,      false,  true,  USEIPV6_DESC       },
  { "-harakiri",     &harakiri,     "HARAKIRI",       PaBool,   PaHid, false,      false,  true,  HARAKIRI_DESC      },

  { "-https",        &https,        "HTTPS",          PaBool,   PaOpt, false,      false,  true,  HTTPS_DESC         },
  { "-key",          httpsKeyFile,  "HTTPS_KEYFILE",  PaString, PaOpt, _i "",      PaNL,   PaNL,  HTTPSKEYFILE_DESC  },
  { "-cert",         httpsCertFile, "HTTPS_CERTFILE", PaString, PaOpt, _i "",      PaNL,   PaNL,  HTTPSCERTFILE_DESC },

  { "-rush",         rush,          "RUSH",           PaString, PaOpt, _i "",      PaNL,   PaNL,  RUSH_DESC          },
  { "-multiservice", mtenant,       "MULTI_SERVICE",  PaString, PaOpt, _i "off",   PaNL,   PaNL,  MULTISERVICE_DESC  },


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
// NGSI9
//
#define RCR                RegisterContext
#define DCAR               DiscoverContextAvailability
#define SCAR               SubscribeContextAvailability
#define UCAR               UnsubscribeContextAvailability
#define UCAS               UpdateContextAvailabilitySubscription
#define NCAR               NotifyContextAvailability

#define RCR_COMPS          2, {      "ngsi9",  "registerContext" }
#define RCR_COMPS_MULTI    3, { "*", "ngsi9",  "registerContext" }
#define RCR_POST_WORD      "registerContextRequest"

#define DCAR_COMPS         2, {      "ngsi9",  "discoverContextAvailability" }
#define DCAR_COMPS_MULTI   3, { "*", "ngsi9",  "discoverContextAvailability" }
#define DCAR_POST_WORD     "discoverContextAvailabilityRequest"

#define SCAR_COMPS         2, {      "ngsi9",  "subscribeContextAvailability" }
#define SCAR_COMPS_MULTI   3, { "*", "ngsi9",  "subscribeContextAvailability" }
#define SCAR_POST_WORD     "subscribeContextAvailabilityRequest"

#define UCAR_COMPS         2, {      "ngsi9",  "unsubscribeContextAvailability" }
#define UCAR_COMPS_MULTI   3, { "*", "ngsi9",  "unsubscribeContextAvailability" }
#define UCAR_POST_WORD     "unsubscribeContextAvailabilityRequest"

#define UCAS_COMPS         2, {      "ngsi9",  "updateContextAvailabilitySubscription" }
#define UCAS_COMPS_MULTI   3, { "*", "ngsi9",  "updateContextAvailabilitySubscription" }
#define UCAS_POST_WORD     "updateContextAvailabilitySubscriptionRequest"

#define NCAR_COMPS         2, {      "ngsi9",  "notifyContextAvailability" }
#define NCAR_COMPS_MULTI   3, { "*", "ngsi9",  "notifyContextAvailability" }
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

#define UPCR_COMPS         2, {      "ngsi10",  "updateContext" }
#define UPCR_COMPS_MULTI   3, { "*", "ngsi10",  "updateContext" }
#define UPCR_POST_WORD     "updateContextRequest"

#define QCR_COMPS          2, {      "ngsi10",  "queryContext" }
#define QCR_COMPS_MULTI    3, { "*", "ngsi10",  "queryContext" }
#define QCR_POST_WORD      "queryContextRequest"

#define SCR_COMPS          2, {      "ngsi10",  "subscribeContext" }
#define SCR_COMPS_MULTI    3, { "*", "ngsi10",  "subscribeContext" }
#define SCR_POST_WORD      "subscribeContextRequest"

#define UCSR_COMPS         2, {      "ngsi10",  "updateContextSubscription" }
#define UCSR_COMPS_MULTI   3, { "*", "ngsi10",  "updateContextSubscription" }
#define UCSR_POST_WORD     "updateContextSubscriptionRequest"

#define UNCR_COMPS         2, {      "ngsi10",  "unsubscribeContext" }
#define UNCR_COMPS_MULTI   3, { "*", "ngsi10",  "unsubscribeContext" }
#define UNCR_POST_WORD     "unsubscribeContextRequest"

#define NCR_COMPS          2, {      "ngsi10",  "notifyContext" }
#define NCR_COMPS_MULTI    3, { "*", "ngsi10",  "notifyContext" }
#define NCR_POST_WORD      "notifyContextRequest"


//
// NGSI9 Convenience Operations
//
#define CE                 ContextEntitiesByEntityId
#define CE_COMPS           3, {      "ngsi9",  "contextEntities", "*" }
#define CE_COMPS_MULTI     4, { "*", "ngsi9",  "contextEntities", "*" }
#define CE_POST_WORD       "registerProviderRequest"

#define CEA                ContextEntityAttributes
#define CEA_COMPS          4, {      "ngsi9",  "contextEntities", "*", "attributes" }
#define CEA_COMPS_MULTI    5, { "*", "ngsi9",  "contextEntities", "*", "attributes" }
#define CEA_POST_WORD      "registerProviderRequest"

#define CEAA               EntityByIdAttributeByName
#define CEAA_COMPS         5, {      "ngsi9",  "contextEntities", "*", "attributes", "*" }
#define CEAA_COMPS_MULTI   6, { "*", "ngsi9",  "contextEntities", "*", "attributes", "*" }
#define CEAA_POST_WORD     "registerProviderRequest"

#define CT                 ContextEntityTypes
#define CT_COMPS           3, {      "ngsi9",  "contextEntityTypes", "*" }
#define CT_COMPS_MULTI     4, { "*", "ngsi9",  "contextEntityTypes", "*" }
#define CT_POST_WORD       "registerProviderRequest"

#define CTA                ContextEntityTypeAttributeContainer
#define CTA_COMPS          4, {      "ngsi9",  "contextEntityTypes", "*", "attributes" }
#define CTA_COMPS_MULTI    5, { "*", "ngsi9",  "contextEntityTypes", "*", "attributes" }
#define CTA_POST_WORD      "registerProviderRequest"

#define CTAA               ContextEntityTypeAttribute
#define CTAA_COMPS         5, {      "ngsi9",  "contextEntityTypes", "*", "attributes", "*" }
#define CTAA_COMPS_MULTI   6, { "*", "ngsi9",  "contextEntityTypes", "*", "attributes", "*" }
#define CTAA_POST_WORD     "registerProviderRequest"

#define SCA                SubscribeContextAvailability
#define SCA_COMPS          2, {      "ngsi9",  "contextAvailabilitySubscriptions" }
#define SCA_COMPS_MULTI    3, { "*", "ngsi9",  "contextAvailabilitySubscriptions" }
#define SCA_POST_WORD      "subscribeContextAvailabilityRequest"

#define SCAS               Ngsi9SubscriptionsConvOp
#define SCAS_COMPS         3, {      "ngsi9",  "contextAvailabilitySubscriptions", "*" }
#define SCAS_COMPS_MULTI   4, { "*", "ngsi9",  "contextAvailabilitySubscriptions", "*" }
#define SCAS_PUT_WORD      "updateContextAvailabilitySubscriptionRequest"



//
// NGSI10 Convenience Operations
//
#define ICE                IndividualContextEntity
#define ICE_COMPS           3, {      "ngsi10",  "contextEntities", "*" }
#define ICE_COMPS_MULTI     4, { "*", "ngsi10",  "contextEntities", "*" }
#define ICE_POST_WORD       "appendContextElementRequest"
#define ICE_PUT_WORD        "updateContextElementRequest"

#define ICEA               IndividualContextEntityAttributes
#define ICEA_COMPS         4, {      "ngsi10",  "contextEntities", "*", "attributes" }
#define ICEA_COMPS_MULTI   5, { "*", "ngsi10",  "contextEntities", "*", "attributes" }
#define ICEA_POST_WORD     "appendContextElementRequest"
#define ICEA_PUT_WORD      "updateContextElementRequest"

#define ICEAA              IndividualContextEntityAttribute
#define ICEAA_COMPS        5, {      "ngsi10",  "contextEntities", "*", "attributes", "*" }
#define ICEAA_COMPS_MULTI  6, { "*", "ngsi10",  "contextEntities", "*", "attributes", "*" }
// FIXME P10: funny having updateContextAttributeRequest for both ... Error in NEC-SPEC?
#define ICEAA_POST_WORD    "updateContextAttributeRequest"
#define ICEAA_PUT_WORD     "updateContextAttributeRequest"

#define AVI                AttributeValueInstance
#define AVI_COMPS          6, {      "ngsi10",  "contextEntities", "*", "attributes", "*", "*" }
#define AVI_COMPS_MULTI    7, { "*", "ngsi10",  "contextEntities", "*", "attributes", "*", "*" }
#define AVI_PUT_WORD       "updateContextAttributeRequest"

#define CET                Ngsi10ContextEntityTypes
#define CET_COMPS          3, {      "ngsi10",  "contextEntityTypes", "*" }
#define CET_COMPS_MULTI    4, { "*", "ngsi10",  "contextEntityTypes", "*" }

#define CETA               Ngsi10ContextEntityTypesAttributeContainer
#define CETA_COMPS         4, {      "ngsi10",  "contextEntityTypes", "*", "attributes" }
#define CETA_COMPS_MULTI   5, { "*", "ngsi10",  "contextEntityTypes", "*", "attributes" }

#define CETAA              Ngsi10ContextEntityTypesAttribute
#define CETAA_COMPS        5, {      "ngsi10",  "contextEntityTypes", "*", "attributes", "*" }
#define CETAA_COMPS_MULTI  6, { "*", "ngsi10",  "contextEntityTypes", "*", "attributes", "*" }

#define SC                 SubscribeContext
#define SC_COMPS           2, {      "ngsi10",  "contextSubscriptions" }
#define SC_COMPS_MULTI     3, { "*", "ngsi10",  "contextSubscriptions" }
#define SC_POST_WORD       "subscribeContextRequest"

#define SCS                Ngsi10SubscriptionsConvOp
#define SCS_COMPS          3, {      "ngsi10",  "contextSubscriptions", "*" }
#define SCS_COMPS_MULTI    4, { "*", "ngsi10",  "contextSubscriptions", "*" }
#define SCS_PUT_WORD       "updateContextSubscriptionRequest"

#define LOG                LogRequest
#define LOGT_COMPS         2, { "log", "trace"            }
#define LOGTL_COMPS        3, { "log", "trace",      "*"  }
#define LOG2T_COMPS        2, { "log", "traceLevel"       }
#define LOG2TL_COMPS       3, { "log", "traceLevel", "*"  }

#define VERS               VersionRequest
#define VERS_COMPS         1, { "version"                 }

#define STAT               StatisticsRequest
#define STAT_COMPS         1, { "statistics"              }

#define EXIT               ExitRequest
#define EXIT1_COMPS        1, { "exit"                    }
#define EXIT2_COMPS        2, { "exit", "*"               }

#define LEAK               LeakRequest
#define LEAK1_COMPS        1, { "leak"                    }
#define LEAK2_COMPS        2, { "leak", "*"               }

#define INV                InvalidRequest
#define INV9_COMPS         2, { "ngsi9",   "*"                }
#define INV10_COMPS        2, { "ngsi10",  "*"                }
#define INV_ALL_COMPS      0, { "*", "*", "*", "*", "*", "*"  }


#define NGSI9_STANDARD_REQUESTS                                                                       \
  { "POST",   RCR,   RCR_COMPS,         RCR_POST_WORD,   postRegisterContext                       }, \
  { "*",      RCR,   RCR_COMPS,         RCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   DCAR,  DCAR_COMPS,        DCAR_POST_WORD,  postDiscoverContextAvailability           }, \
  { "*",      DCAR,  DCAR_COMPS,        DCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   SCAR,  SCAR_COMPS,        SCAR_POST_WORD,  postSubscribeContextAvailability          }, \
  { "*",      SCAR,  SCAR_COMPS,        SCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAR,  UCAR_COMPS,        UCAR_POST_WORD,  postUnsubscribeContextAvailability        }, \
  { "*",      UCAR,  UCAR_COMPS,        UCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAS,  UCAS_COMPS,        UCAS_POST_WORD,  postUpdateContextAvailabilitySubscription }, \
  { "*",      UCAS,  UCAS_COMPS,        UCAS_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCAR,  NCAR_COMPS,        NCAR_POST_WORD,  postNotifyContextAvailability             }, \
  { "*",      NCAR,  NCAR_COMPS,        NCAR_POST_WORD,  badVerbPostOnly                           }



#define NGSI9_STANDARD_REQUESTS_WITH_TENANT                                                           \
  { "POST",   RCR,   RCR_COMPS_MULTI,   RCR_POST_WORD,   postRegisterContext                       }, \
  { "*",      RCR,   RCR_COMPS_MULTI,   RCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   DCAR,  DCAR_COMPS_MULTI,  DCAR_POST_WORD,  postDiscoverContextAvailability           }, \
  { "*",      DCAR,  DCAR_COMPS_MULTI,  DCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   SCAR,  SCAR_COMPS_MULTI,  SCAR_POST_WORD,  postSubscribeContextAvailability          }, \
  { "*",      SCAR,  SCAR_COMPS_MULTI,  SCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAR,  UCAR_COMPS_MULTI,  UCAR_POST_WORD,  postUnsubscribeContextAvailability        }, \
  { "*",      UCAR,  UCAR_COMPS_MULTI,  UCAR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UCAS,  UCAS_COMPS_MULTI,  UCAS_POST_WORD,  postUpdateContextAvailabilitySubscription }, \
  { "*",      UCAS,  UCAS_COMPS_MULTI,  UCAS_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCAR,  NCAR_COMPS_MULTI,  NCAR_POST_WORD,  postNotifyContextAvailability             }, \
  { "*",      NCAR,  NCAR_COMPS_MULTI,  NCAR_POST_WORD,  badVerbPostOnly                           }


#define NGSI10_STANDARD_REQUESTS                                                                      \
  { "POST",   UPCR,  UPCR_COMPS,        UPCR_POST_WORD,  postUpdateContext                         }, \
  { "*",      UPCR,  UPCR_COMPS,        UPCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   QCR,   QCR_COMPS,         QCR_POST_WORD,   postQueryContext                          }, \
  { "*",      QCR,   QCR_COMPS,         QCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   SCR,   SCR_COMPS,         SCR_POST_WORD,   postSubscribeContext                      }, \
  { "*",      SCR,   SCR_COMPS,         SCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   UCSR,  UCSR_COMPS,        UCSR_POST_WORD,  postUpdateContextSubscription             }, \
  { "*",      UCSR,  UCSR_COMPS,        UCSR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UNCR,  UNCR_COMPS,        UNCR_POST_WORD,  postUnsubscribeContext                    }, \
  { "*",      UNCR,  UNCR_COMPS,        UNCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCR,   NCR_COMPS,         NCR_POST_WORD,   postNotifyContext                         }, \
  { "*",      NCR,   NCR_COMPS,         NCR_POST_WORD,   badVerbPostOnly                           }


#define NGSI10_STANDARD_REQUESTS_WITH_TENANT                                                          \
  { "POST",   UPCR,  UPCR_COMPS_MULTI,  UPCR_POST_WORD,  postUpdateContext                         }, \
  { "*",      UPCR,  UPCR_COMPS_MULTI,  UPCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   QCR,   QCR_COMPS_MULTI,   QCR_POST_WORD,   postQueryContext                          }, \
  { "*",      QCR,   QCR_COMPS_MULTI,   QCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   SCR,   SCR_COMPS_MULTI,   SCR_POST_WORD,   postSubscribeContext                      }, \
  { "*",      SCR,   SCR_COMPS_MULTI,   SCR_POST_WORD,   badVerbPostOnly                           }, \
  { "POST",   UCSR,  UCSR_COMPS_MULTI,  UCSR_POST_WORD,  postUpdateContextSubscription             }, \
  { "*",      UCSR,  UCSR_COMPS_MULTI,  UCSR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   UNCR,  UNCR_COMPS_MULTI,  UNCR_POST_WORD,  postUnsubscribeContext                    }, \
  { "*",      UNCR,  UNCR_COMPS_MULTI,  UNCR_POST_WORD,  badVerbPostOnly                           }, \
  { "POST",   NCR,   NCR_COMPS_MULTI,   NCR_POST_WORD,   postNotifyContext                         }, \
  { "*",      NCR,   NCR_COMPS_MULTI,   NCR_POST_WORD,   badVerbPostOnly                           }


#define NGSI9_CONVENIENCE_OPERATIONS                                                                  \
  { "GET",    CE,    CE_COMPS,          "",              getContextEntitiesByEntityId              }, \
  { "POST",   CE,    CE_COMPS,          CE_POST_WORD,    postContextEntitiesByEntityId             }, \
  { "*",      CE,    CE_COMPS,          "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CEA,   CEA_COMPS,         "",              getContextEntityAttributes                }, \
  { "POST",   CEA,   CEA_COMPS,         CEA_POST_WORD,   postContextEntityAttributes               }, \
  { "*",      CEA,   CEA_COMPS,         "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CEAA,  CEAA_COMPS,        "",              getEntityByIdAttributeByName              }, \
  { "POST",   CEAA,  CEAA_COMPS,        CEAA_POST_WORD,  postEntityByIdAttributeByName             }, \
  { "*",      CEAA,  CEAA_COMPS,        "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CT,    CT_COMPS,          "",              getContextEntityTypes                     }, \
  { "POST",   CT,    CT_COMPS,          CT_POST_WORD,    postContextEntityTypes                    }, \
  { "*",      CT,    CT_COMPS,          "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CTA,   CTA_COMPS,         "",              getContextEntityTypes                     }, \
  { "POST",   CTA,   CTA_COMPS,         CTA_POST_WORD,   postContextEntityTypes                    }, \
  { "*",      CTA,   CTA_COMPS,         "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CTAA,  CTAA_COMPS,        "",              getContextEntityTypeAttribute             }, \
  { "POST",   CTAA,  CTAA_COMPS,        CTAA_POST_WORD,  postContextEntityTypeAttribute            }, \
  { "*",      CTAA,  CTAA_COMPS,        "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "POST",   SCA,   SCA_COMPS,         SCA_POST_WORD,   postSubscribeContextAvailability          }, \
  { "*",      SCA,   SCA_COMPS,         "",              badVerbPostOnly                           }, \
                                                                                                      \
  { "PUT",    SCAS,  SCAS_COMPS,        SCAS_PUT_WORD,   putAvailabilitySubscriptionConvOp         }, \
  { "DELETE", SCAS,  SCAS_COMPS,        "",              deleteAvailabilitySubscriptionConvOp      }, \
  { "*",      SCAS,  SCAS_COMPS,        "",              badVerbPutDeleteOnly                      }


#define NGSI9_CONVENIENCE_OPERATIONS_WITH_TENANT                                                      \
  { "GET",    CE,    CE_COMPS_MULTI,    "",              getContextEntitiesByEntityId              }, \
  { "POST",   CE,    CE_COMPS_MULTI,    CE_POST_WORD,    postContextEntitiesByEntityId             }, \
  { "*",      CE,    CE_COMPS_MULTI,    "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CEA,   CEA_COMPS_MULTI,   "",              getContextEntityAttributes                }, \
  { "POST",   CEA,   CEA_COMPS_MULTI,   CEA_POST_WORD,   postContextEntityAttributes               }, \
  { "*",      CEA,   CEA_COMPS_MULTI,   "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CEAA,  CEAA_COMPS_MULTI,  "",              getEntityByIdAttributeByName              }, \
  { "POST",   CEAA,  CEAA_COMPS_MULTI,  CEAA_POST_WORD,  postEntityByIdAttributeByName             }, \
  { "*",      CEAA,  CEAA_COMPS_MULTI,  "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CT,    CT_COMPS_MULTI,    "",              getContextEntityTypes                     }, \
  { "POST",   CT,    CT_COMPS_MULTI,    CT_POST_WORD,    postContextEntityTypes                    }, \
  { "*",      CT,    CT_COMPS_MULTI,    "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CTA,   CTA_COMPS_MULTI,   "",              getContextEntityTypes                     }, \
  { "POST",   CTA,   CTA_COMPS_MULTI,   CTA_POST_WORD,   postContextEntityTypes                    }, \
  { "*",      CTA,   CTA_COMPS_MULTI,   "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "GET",    CTAA,  CTAA_COMPS_MULTI,  "",              getContextEntityTypeAttribute             }, \
  { "POST",   CTAA,  CTAA_COMPS_MULTI,  CTAA_POST_WORD,  postContextEntityTypeAttribute            }, \
  { "*",      CTAA,  CTAA_COMPS_MULTI,  "",              badVerbGetPostOnly                        }, \
                                                                                                      \
  { "POST",   SCA,   SCA_COMPS_MULTI,   SCA_POST_WORD,   postSubscribeContextAvailability          }, \
  { "*",      SCA,   SCA_COMPS_MULTI,   "",              badVerbPostOnly                           }, \
                                                                                                      \
  { "PUT",    SCAS,  SCAS_COMPS_MULTI,  SCAS_PUT_WORD,   putAvailabilitySubscriptionConvOp         }, \
  { "DELETE", SCAS,  SCAS_COMPS_MULTI,  "",              deleteAvailabilitySubscriptionConvOp      }, \
  { "*",      SCAS,  SCAS_COMPS_MULTI,  "",              badVerbPutDeleteOnly                      }


#define NGSI10_CONVENIENCE_OPERATIONS                                                                 \
  { "GET",    ICE,   ICE_COMPS,         "",              getIndividualContextEntity                }, \
  { "PUT",    ICE,   ICE_COMPS,         ICE_PUT_WORD,    putIndividualContextEntity                }, \
  { "POST",   ICE,   ICE_COMPS,         ICE_POST_WORD,   postIndividualContextEntity               }, \
  { "DELETE", ICE,   ICE_COMPS,         "",              deleteIndividualContextEntity             }, \
  { "*",      ICE,   ICE_COMPS,         "",              badVerbAllFour                            }, \
                                                                                                      \
  { "GET",    ICEA,  ICEA_COMPS,        "",              getIndividualContextEntityAttributes      }, \
  { "PUT",    ICEA,  ICEA_COMPS,        ICEA_PUT_WORD,   putIndividualContextEntityAttributes      }, \
  { "POST",   ICEA,  ICEA_COMPS,        ICEA_POST_WORD,  postIndividualContextEntityAttributes     }, \
  { "DELETE", ICEA,  ICEA_COMPS,        "",              deleteIndividualContextEntityAttributes   }, \
  { "*",      ICEA,  ICEA_COMPS,        "",              badVerbAllFour                            }, \
                                                                                                      \
  { "GET",    ICEAA, ICEAA_COMPS,       "",              getIndividualContextEntityAttribute       }, \
  { "PUT",    ICEAA, ICEAA_COMPS,       ICEAA_PUT_WORD,  putIndividualContextEntityAttribute       }, \
  { "POST",   ICEAA, ICEAA_COMPS,       ICEAA_POST_WORD, postIndividualContextEntityAttribute      }, \
  { "DELETE", ICEAA, ICEAA_COMPS,       "",              deleteIndividualContextEntityAttribute    }, \
  { "*",      ICEAA, ICEAA_COMPS,       "",              badVerbGetPostDeleteOnly                  }, \
                                                                                                      \
  { "GET",    AVI,   AVI_COMPS,         "",              getAttributeValueInstance                 }, \
  { "PUT",    AVI,   AVI_COMPS,         AVI_PUT_WORD,    putAttributeValueInstance                 }, \
  { "DELETE", AVI,   AVI_COMPS,         "",              deleteAttributeValueInstance              }, \
  { "*",      AVI,   AVI_COMPS,         "",              badVerbGetPutDeleteOnly                   }, \
                                                                                                      \
  { "GET",    CET,   CET_COMPS,         "",              getNgsi10ContextEntityTypes               }, \
  { "*",      CET,   CET_COMPS,         "",              badVerbGetOnly                            }, \
                                                                                                      \
  { "GET",    CETA,  CETA_COMPS,        "",              getNgsi10ContextEntityTypes               }, \
  { "*",      CETA,  CETA_COMPS,        "",              badVerbGetOnly                            }, \
                                                                                                      \
  { "GET",    CETAA, CETAA_COMPS,       "",              getNgsi10ContextEntityTypesAttribute      }, \
  { "*",      CETAA, CETAA_COMPS,       "",              badVerbGetOnly                            }, \
                                                                                                      \
  { "POST",   SC,    SC_COMPS,          SC_POST_WORD,    postSubscribeContext                      }, \
  { "*",      SC,    SC_COMPS,          "",              badVerbPostOnly                           }, \
                                                                                                      \
  { "PUT",    SCS,   SCS_COMPS,         SCS_PUT_WORD,    putSubscriptionConvOp                     }, \
  { "DELETE", SCS,   SCS_COMPS,         "",              deleteSubscriptionConvOp                  }, \
  { "*",      SCS,   SCS_COMPS,         "",              badVerbPutDeleteOnly                      }


#define NGSI10_CONVENIENCE_OPERATIONS_WITH_TENANT                                                     \
  { "GET",    ICE,   ICE_COMPS_MULTI,   "",              getIndividualContextEntity                }, \
  { "PUT",    ICE,   ICE_COMPS_MULTI,   ICE_PUT_WORD,    putIndividualContextEntity                }, \
  { "POST",   ICE,   ICE_COMPS_MULTI,   ICE_POST_WORD,   postIndividualContextEntity               }, \
  { "DELETE", ICE,   ICE_COMPS_MULTI,   "",              deleteIndividualContextEntity             }, \
  { "*",      ICE,   ICE_COMPS_MULTI,   "",              badVerbAllFour                            }, \
                                                                                                      \
  { "GET",    ICEA,  ICEA_COMPS_MULTI,  "",              getIndividualContextEntityAttributes      }, \
  { "PUT",    ICEA,  ICEA_COMPS_MULTI,  ICEA_PUT_WORD,   putIndividualContextEntityAttributes      }, \
  { "POST",   ICEA,  ICEA_COMPS_MULTI,  ICEA_POST_WORD,  postIndividualContextEntityAttributes     }, \
  { "DELETE", ICEA,  ICEA_COMPS_MULTI,  "",              deleteIndividualContextEntityAttributes   }, \
  { "*",      ICEA,  ICEA_COMPS_MULTI,  "",              badVerbAllFour                            }, \
                                                                                                      \
  { "GET",    ICEAA, ICEAA_COMPS_MULTI, "",              getIndividualContextEntityAttribute       }, \
  { "PUT",    ICEAA, ICEAA_COMPS_MULTI, ICEAA_PUT_WORD,  putIndividualContextEntityAttribute       }, \
  { "POST",   ICEAA, ICEAA_COMPS_MULTI, ICEAA_POST_WORD, postIndividualContextEntityAttribute      }, \
  { "DELETE", ICEAA, ICEAA_COMPS_MULTI, "",              deleteIndividualContextEntityAttribute    }, \
  { "*",      ICEAA, ICEAA_COMPS_MULTI, "",              badVerbGetPostDeleteOnly                  }, \
                                                                                                      \
  { "GET",    AVI,   AVI_COMPS_MULTI,   "",               getAttributeValueInstance                }, \
  { "PUT",    AVI,   AVI_COMPS_MULTI,   AVI_PUT_WORD,     putAttributeValueInstance                }, \
  { "DELETE", AVI,   AVI_COMPS_MULTI,   "",               deleteAttributeValueInstance             }, \
  { "*",      AVI,   AVI_COMPS_MULTI,   "",               badVerbGetPutDeleteOnly                  }, \
                                                                                                      \
  { "GET",    CET,   CET_COMPS_MULTI,   "",               getNgsi10ContextEntityTypes              }, \
  { "*",      CET,   CET_COMPS_MULTI,   "",               badVerbGetOnly                           }, \
                                                                                                      \
  { "GET",    CETA,  CETA_COMPS_MULTI,  "",               getNgsi10ContextEntityTypes              }, \
  { "*",      CETA,  CETA_COMPS_MULTI,  "",               badVerbGetOnly                           }, \
                                                                                                      \
  { "GET",    CETAA, CETAA_COMPS_MULTI, "",               getNgsi10ContextEntityTypesAttribute     }, \
  { "*",      CETAA, CETAA_COMPS_MULTI, "",               badVerbGetOnly                           }, \
                                                                                                      \
  { "POST",   SC,    SC_COMPS_MULTI,    SC_POST_WORD,     postSubscribeContext                     }, \
  { "*",      SC,    SC_COMPS_MULTI,    "",               badVerbPostOnly                          }, \
                                                                                                      \
  { "PUT",    SCS,   SCS_COMPS_MULTI,   SCS_PUT_WORD,     putSubscriptionConvOp                    }, \
  { "DELETE", SCS,   SCS_COMPS_MULTI,   "",               deleteSubscriptionConvOp                 }, \
  { "*",      SCS,   SCS_COMPS_MULTI,   "",               badVerbPutDeleteOnly                      }


/* *****************************************************************************
*  
* log requests
* The documentation (Installation and Admin Guide) says /log/trace ...
* ... and to maintain backward compatibility we keep supporting /log/traceLevel too
*/
#define LOG_REQUESTS                                                                 \
  { "GET",    LOG,  LOGT_COMPS,    "",  logTraceTreat                             }, \
  { "DELETE", LOG,  LOGT_COMPS,    "",  logTraceTreat                             }, \
  { "*",      LOG,  LOGT_COMPS,    "",  badVerbAllFour                            }, \
  { "PUT",    LOG,  LOGTL_COMPS,   "",  logTraceTreat                             }, \
  { "DELETE", LOG,  LOGTL_COMPS,   "",  logTraceTreat                             }, \
  { "*",      LOG,  LOGTL_COMPS,   "",  badVerbAllFour                            }, \
  { "GET",    LOG,  LOG2T_COMPS,   "",  logTraceTreat                             }, \
  { "DELETE", LOG,  LOG2T_COMPS,   "",  logTraceTreat                             }, \
  { "*",      LOG,  LOG2T_COMPS,   "",  badVerbAllFour                            }, \
  { "PUT",    LOG,  LOG2TL_COMPS,  "",  logTraceTreat                             }, \
  { "DELETE", LOG,  LOG2TL_COMPS,  "",  logTraceTreat                             }, \
  { "*",      LOG,  LOG2TL_COMPS,  "",  badVerbAllFour                            }

#define VERSION_REQUESTS                                                             \
  { "GET",    VERS, VERS_COMPS,    "",  versionTreat                              }, \
  { "*",      VERS, VERS_COMPS,    "",  badVerbGetOnly                            }

#define STAT_REQUESTS                                                                \
  { "GET",    STAT, STAT_COMPS,    "",  statisticsTreat                           }, \
  { "DELETE", STAT, STAT_COMPS,    "",  statisticsTreat                           }, \
  { "*",      STAT, STAT_COMPS,    "",  badVerbGetDeleteOnly                      }

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
* restServiceMTenant - services for BROKER (ngsi9/10) and tenants 
*
* This service vector (configuration) is used if the broker is started with
* the the -multiservice option (but not the -ngsi9 option)
*/
RestService restServiceMTenant[] =
{
  NGSI9_STANDARD_REQUESTS,
  NGSI9_STANDARD_REQUESTS_WITH_TENANT,
  NGSI9_CONVENIENCE_OPERATIONS,
  NGSI9_CONVENIENCE_OPERATIONS_WITH_TENANT,
  NGSI10_STANDARD_REQUESTS,
  NGSI10_STANDARD_REQUESTS_WITH_TENANT,
  NGSI10_CONVENIENCE_OPERATIONS,
  NGSI10_CONVENIENCE_OPERATIONS_WITH_TENANT,
  LOG_REQUESTS,
  VERSION_REQUESTS,
  STAT_REQUESTS,

#ifdef DEBUG
  EXIT_REQUESTS,
  LEAK_REQUESTS,
#endif

  INVALID_REQUESTS,
  END_REQUEST
};



/* ****************************************************************************
*
* restServiceV - services for BROKER (ngsi9/10) without tenants
*
* This is the default service vector, that is used if the broker is started without
* the -ngsi9 and -multiservice options
*/
RestService restServiceV[] =
{
  NGSI9_STANDARD_REQUESTS,
  NGSI10_STANDARD_REQUESTS,
  NGSI9_CONVENIENCE_OPERATIONS,
  NGSI10_CONVENIENCE_OPERATIONS,
  LOG_REQUESTS,
  VERSION_REQUESTS,
  STAT_REQUESTS,

#ifdef DEBUG
  EXIT_REQUESTS,
  LEAK_REQUESTS,
#endif

  INVALID_REQUESTS,
  END_REQUEST
};



/* ****************************************************************************
*
* restServiceNgsi9 - services for CONF MAN without tenants
*
* This service vector (configuration) is used if the broker is started as
* CONFIGURATION MANAGER (using the -ngsi9 option) and without using the
* -multiservice option.
*/
RestService restServiceNgsi9[] =
{
  NGSI9_STANDARD_REQUESTS,   // FIXME P10:  NCAR is added here, is that OK?
  NGSI9_CONVENIENCE_OPERATIONS,
  LOG_REQUESTS,
  VERSION_REQUESTS,
  STAT_REQUESTS,

#ifdef DEBUG
  EXIT_REQUESTS,
  LEAK_REQUESTS,
#endif

  INVALID_REQUESTS,
  END_REQUEST
};



/* ****************************************************************************
*
* restServiceNgsi9MTenant - services for CONF MAN with tenants
*
* This service vector (configuration) is used if the broker is started as
* CONFIGURATION MANAGER (using the -ngsi9 option) and also with the
* -multiservice option.
*/
RestService restServiceNgsi9MTenant[] =
{
  NGSI9_STANDARD_REQUESTS,  // FIXME P10:  NCAR is added here, is that OK?
  NGSI9_STANDARD_REQUESTS_WITH_TENANT,
  NGSI9_CONVENIENCE_OPERATIONS,
  NGSI9_CONVENIENCE_OPERATIONS_WITH_TENANT,
  LOG_REQUESTS,
  VERSION_REQUESTS,
  STAT_REQUESTS,

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
}



/* ****************************************************************************
*
* mongoInit -
*/
static void mongoInit(const char* dbHost, const char* rplSet, std::string dbName, const char* user, const char* pwd)
{
  std::string multitenant = mtenant;

  if (!mongoConnect(dbHost, dbName.c_str(), rplSet, user, pwd, multitenant != "off"))
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
  setEntitiesCollectionName("entities");
  setRegistrationsCollectionName("registrations");
  setSubscribeContextCollectionName("csubs");
  setSubscribeContextAvailabilityCollectionName("casubs");
  setAssociationsCollectionName("associations");

  //
  // Note that index creation operation is idempotent.
  // From http://docs.mongodb.org/manual/reference/method/db.collection.ensureIndex:
  // "If you call multiple ensureIndex() methods with the same index specification at the same time,
  // only the first operation will succeed, all other operations will have no effect."
  //
  ensureLocationIndex("");
  if (multitenant != "off")
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
  LM_I(("Orion Context Broker is running"));

  std::string multitenant = mtenant;
  if ((multitenant != "off") && (multitenant != "header") && (multitenant != "url"))
  {
    LM_X(1, ("Fatal Error (bad value for -multiservice ['%s']. Allowed values: 'off', 'header' and 'url')", mtenant));
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

  if (fg == false)
  {
    daemonize();
  }

#if 0
  //
  // This 'almost always outdeffed' piece of code is used whenever a change is done to the
  // valgrind test suite, just to make sure that the tool actually detects memory leaks,
  //
  char* x = (char*) malloc(100000);
  snprintf(x, sizeof(x), "A hundred thousand bytes lost here");
  LM_M(("x: '%s'", x));
  x = (char*) "LOST";
  LM_M(("x: '%s'", x));
#endif

  RestService* rsP = restServiceV;
  if      (ngsi9Only  && multitenant == "url") rsP = restServiceNgsi9MTenant;
  else if (!ngsi9Only && multitenant == "url") rsP = restServiceMTenant;
  else if (ngsi9Only)                          rsP = restServiceNgsi9;

  IpVersion    ipVersion = IPDUAL;
  if      (useOnlyIPv4)    ipVersion = IPV4;
  else if (useOnlyIPv6)    ipVersion = IPV6;

  pidFile();
  orionInit(orionExit, ORION_VERSION);
  mongoInit(dbHost, rplSet, dbName, user, pwd);
  contextBrokerInit(ngsi9Only, dbName, multitenant != "off");
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

    restInit(rsP, ipVersion, bindAddress, port, mtenant, rushHost, rushPort, httpsPrivateServerKey, httpsCertificate);

    free(httpsPrivateServerKey);
    free(httpsCertificate);
  }
  else
  {
    restInit(rsP, ipVersion, bindAddress, port, mtenant, rushHost, rushPort);
  }

  LM_I(("Startup completed"));

  while (1)
  {
    sleep(10);
  }
}
