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
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <uuid/uuid.h>

#include <string>
#include <map>

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet, kTimeDiff
#include "kalloc/kaBufferReset.h"                                // kaBufferReset
#include "kjson/kjFree.h"                                        // kjFree
}

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/limits.h"
#include "common/string.h"
#include "common/globals.h"
#include "common/errorMessages.h"
#include "common/defaultValues.h"
#include "common/clockFunctions.h"
#include "common/statistics.h"
#include "common/tag.h"
#include "common/limits.h"                                       // SERVICE_NAME_MAX_LEN

#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "parse/forbiddenChars.h"
#include "serviceRoutinesV2/getEntityAttributeValue.h"           // getEntityAttributeValue

#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/types/OrionldMimeType.h"                       // mimeTypeFromString
#include "orionld/types/ApiVersion.h"                            // ApiVersion
#include "orionld/common/orionldState.h"                         // orionldState, multitenancy, ...
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/orionldTenantGet.h"                     // orionldTenantGet
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/common/stringStrip.h"                          // stringStrip
#include "orionld/mongoc/mongocConnectionRelease.h"              // Own interface
#include "orionld/notifications/orionldAlterationsTreat.h"       // orionldAlterationsTreat
#include "orionld/mhd/mhdConnectionInit.h"                       // mhdConnectionInit
#include "orionld/mhd/mhdConnectionPayloadRead.h"                // mhdConnectionPayloadRead
#include "orionld/mhd/mhdConnectionTreat.h"                      // mhdConnectionTreat
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease

#include "rest/HttpHeaders.h"                                    // HTTP_* defines
#include "rest/Verb.h"
#include "rest/RestService.h"
#include "rest/restReply.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "rest/restServiceLookup.h"
#include "rest/rest.h"



// -----------------------------------------------------------------------------
//
// TIME_REPORT -
//
#define TIME_REPORT(start, end, text)                    \
{                                                        \
  struct timespec diff;                                  \
  float           diffF;                                 \
                                                         \
  kTimeDiff(&start, &end, &diff, &diffF);                \
  LM_T(LmtPerformance, ("TPUT: %s %f", text, diffF));    \
}



/* ****************************************************************************
*
* Globals
*/
static unsigned short            port                  = 0;
static char                      bindIp[MAX_LEN_IP]    = "0.0.0.0";
static char                      bindIPv6[MAX_LEN_IP]  = "::";
IpVersion                        ipVersionUsed         = IPDUAL;
bool                             multitenant           = false;
bool                             corsEnabled           = false;
char                             corsOrigin[64];
int                              corsMaxAge;
static MHD_Daemon*               mhdDaemon             = NULL;
static MHD_Daemon*               mhdDaemon_v6          = NULL;
static struct sockaddr_in        sad;
static struct sockaddr_in6       sad_v6;
static unsigned int              connMemory;
static unsigned int              maxConns;
static unsigned int              threadPoolSize;
static unsigned int              mhdConnectionTimeout  = 0;



/* ****************************************************************************
*
* restPortGet -
*/
unsigned short restPortGet(void)
{
  return port;
}



/* ****************************************************************************
*
* correlatorGenerate -
*/
static void correlatorGenerate(char* buffer)
{
  uuid_t uuid;

  uuid_generate_time_safe(uuid);
  uuid_unparse_lower(uuid, buffer);
}



/* ****************************************************************************
*
* uriArgumentGet -
*/
MHD_Result uriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* val)
{
  ConnectionInfo*  ciP   = (ConnectionInfo*) cbDataP;

  if ((val == NULL) || (*val == 0))
  {
    std::string  errorString = std::string("Empty right-hand-side for URI param /") + ckey + "/";

    if (orionldState.apiVersion == API_VERSION_NGSI_V2)
    {
      OrionError error(SccBadRequest, errorString);
      orionldState.httpStatusCode = error.code;
      ciP->answer                 = error.smartRender(orionldState.apiVersion);
    }
    else if (orionldState.apiVersion == API_VERSION_ADMIN)
    {
      orionldState.httpStatusCode = SccBadRequest;
      ciP->answer                 = "{" + JSON_STR("error") + ":" + JSON_STR(errorString) + "}";
    }

    return MHD_YES;
  }

  std::string      key   = ckey;
  std::string      value = (val == NULL)? "" : val;

  if (key == URI_PARAM_TYPE)
  {
    if (strstr(val, ","))  // More than ONE type?
    {
      uriParamTypesParse(ciP, val);
    }
    else
    {
      ciP->uriParamTypes.push_back(val);
    }
  }
  else if (key == URI_PARAM_PAGINATION_OFFSET)
  {
    char* cP = (char*) val;

    while (*cP != 0)
    {
      if ((*cP < '0') || (*cP > '9'))
      {
        OrionError error(SccBadRequest, std::string("Bad pagination offset: /") + value + "/ [must be a decimal number]");
        orionldState.httpStatusCode = error.code;
        ciP->answer                 = error.smartRender(orionldState.apiVersion);
        return MHD_YES;
      }

      ++cP;
    }
    orionldState.uriParams.offset = atoi(val);
  }
  else if (key == URI_PARAM_PAGINATION_LIMIT)
  {
    char* cP = (char*) val;

    while (*cP != 0)
    {
      if ((*cP < '0') || (*cP > '9'))
      {
        OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [must be a decimal number]");
        orionldState.httpStatusCode = error.code;
        ciP->answer                 = error.smartRender(orionldState.apiVersion);

        LM_E(("Invalid value for URI parameter 'limit': '%s'", val));
        return MHD_YES;
      }

      ++cP;
    }

    int limit = atoi(val);
    if (limit > atoi(MAX_PAGINATION_LIMIT))
    {
      OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [max: " + MAX_PAGINATION_LIMIT + "]");
      orionldState.httpStatusCode = error.code;
      ciP->answer                 = error.smartRender(orionldState.apiVersion);

      LM_E(("Invalid value for URI parameter 'limit': '%s'", val));
      return MHD_YES;
    }
    else if (limit == 0)
    {
      if (orionldState.apiVersion != API_VERSION_NGSILD_V1)
      {
        OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [a value of ZERO is unacceptable]");
        orionldState.httpStatusCode = error.code;
        ciP->answer                 = error.smartRender(orionldState.apiVersion);
      }
      return MHD_YES;
    }

    orionldState.uriParams.limit = limit;
  }
  else if (key == URI_PARAM_PAGINATION_DETAILS)
  {
    if ((strcasecmp(value.c_str(), "on") != 0) && (strcasecmp(value.c_str(), "off") != 0))
    {
      OrionError error(SccBadRequest, std::string("Bad value for /details/: /") + value + "/ [accepted: /on/, /ON/, /off/, /OFF/. Default is /off/]");
      orionldState.httpStatusCode = error.code;
      ciP->answer                 = error.smartRender(orionldState.apiVersion);
      return MHD_YES;
    }
  }

  //
  // Now check the URI param has no invalid characters
  // Except for the URI params 'q' and 'idPattern' that are not to be checked for invalid characters
  //
  // Another exception: 'geometry' and 'coords' has a relaxed check for forbidden characters that
  // doesn't check for '=' and ';' as those characters are part of the syntaxis for the parameters.
  //
  bool containsForbiddenChars = false;

  if ((key == "geometry") || (key == "georel"))
  {
    containsForbiddenChars = forbiddenChars(val, "=;");
  }
  else if (key == "coords")
  {
    containsForbiddenChars = forbiddenChars(val, ";");
  }
  else if ((key != URI_PARAM_Q) && (key != URI_PARAM_MQ) && (key != "idPattern") && (key != "typePattern"))
  {
    containsForbiddenChars = forbiddenChars(ckey) || forbiddenChars(val);
  }

  if (containsForbiddenChars == true)
  {
    std::string details = std::string("found a forbidden character in URI param '") + key + "'";
    OrionError error(SccBadRequest, "invalid character in URI parameter");

    alarmMgr.badInput(orionldState.clientIp, details);
    orionldState.httpStatusCode = error.code;
    ciP->answer                 = error.smartRender(orionldState.apiVersion);
    LM_W(("Bad Input (forbidden character in URI parameter value: %s=%s)", key.c_str(), val));
  }

  return MHD_YES;
}



extern MimeType acceptHeaderParse(char* accept, bool textOk);
/* ****************************************************************************
*
* httpHeaderGet -
*/
static MHD_Result httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  if      (strcasecmp(key, HTTP_CONTENT_LENGTH)     == 0) orionldState.in.contentLength    = atoi(value);
  else if (strcasecmp(key, HTTP_FIWARE_SERVICEPATH) == 0) orionldState.in.servicePath      = (char*) value;
  else if (strcasecmp(key, "X-Auth-Token")          == 0) orionldState.in.xAuthToken       = (char*) value;
  else if (strcasecmp(key, "Authorization")         == 0) orionldState.in.authorization    = (char*) value;
  else if (strcasecmp(key, HTTP_ORIGIN)             == 0) orionldState.in.origin           = (char*) value;
  else if (strcasecmp(key, HTTP_X_REAL_IP)          == 0) orionldState.in.xRealIp          = (char*) value;
  else if (strcasecmp(key, HTTP_HOST)               == 0) orionldState.in.host             = (char*) value;
  else if (strcasecmp(key, HTTP_FIWARE_CORRELATOR)  == 0) orionldState.correlator          = (char*) value;
  else if (strcasecmp(key, HTTP_CONNECTION)         == 0) orionldState.in.connection       = (char*) value;
  else if (strcasecmp(key, HTTP_NGSIV2_ATTRSFORMAT) == 0) orionldState.attrsFormat         = (char*) value;
  else if (strcasecmp(key, HTTP_X_FORWARDED_FOR)    == 0) orionldState.in.xForwardedFor    = (char*) value;
  else if (strcasecmp(key, HTTP_USER_AGENT)         == 0) {}
  else if (strcasecmp(key, HTTP_EXPECT)             == 0) {}
  else if (strcasecmp(key, HTTP_ACCEPT) == 0)
  {
    orionldState.out.contentType = acceptHeaderParse((char*) value, true);
    if (orionldState.out.contentType == MT_NONE)
    {
      orionldState.httpStatusCode = 406;
      orionldState.pd.detail      = (char*) "no acceptable mime-type in accept header";
    }
  }
  else if (strcasecmp(key, HTTP_CONTENT_TYPE) == 0)
  {
    orionldState.in.contentType       = mimeTypeFromString(value, NULL, false, true, &orionldState.acceptMask);
    orionldState.in.contentTypeString = (char*) value;
  }
  else if ((strcasecmp(key, HTTP_FIWARE_SERVICE) == 0) || (strcasecmp(key, "NGSILD-Tenant") == 0))
  {
    if (multitenancy == true)  // Has the broker been started with multi-tenancy enabled (it's disabled by default)
    {
      toLowercase((char*) value);
      orionldState.tenantName = (char*) value;
    }
    else
    {
      // Tenant used when tenant is not supported by the broker - silently ignored for NGSIv2/v2, error for NGSI-LD
      if (orionldState.apiVersion == API_VERSION_NGSILD_V1)
        orionldError(OrionldBadRequestData, "Tenants not supported", "tenant in use but tenant support is not enabled for the broker", 400);
    }
  }
  else
  {
    LM_W(("'unsupported' HTTP header: '%s', value '%s'", key, value));
  }

  return MHD_YES;
}



/* ****************************************************************************
*
* requestCompleted -
*/
static void requestCompleted
(
  void*                       cls,
  MHD_Connection*             connection,
  void**                      con_cls,
  MHD_RequestTerminationCode  toe
)
{
  PERFORMANCE(requestCompletedStart);

  ConnectionInfo*  ciP      = (ConnectionInfo*) *con_cls;
  const char*      spath    = ((orionldState.apiVersion != API_VERSION_NGSILD_V1) && (ciP->servicePathV.size() > 0))? ciP->servicePathV[0].c_str() : "";
  struct timespec  reqEndTime;

  //
  // Notifications
  //
  if (orionldState.alterations != NULL)
  {
    PERFORMANCE(notifStart);
    orionldAlterationsTreat(orionldState.alterations);
    PERFORMANCE(notifEnd);
  }

  if ((orionldState.in.payload != NULL) && (orionldState.in.payload != orionldState.preallocReqBuf))
  {
    free(orionldState.in.payload);
    orionldState.in.payload = NULL;
  }


  //
  // Release the connection to mongo - after all notifications have been sent (orionldAlterationsTreat takes care of that)
  //
  if (orionldState.mongoc.contextsP)       mongoc_collection_destroy(orionldState.mongoc.contextsP);
  if (orionldState.mongoc.entitiesP)       mongoc_collection_destroy(orionldState.mongoc.entitiesP);
  if (orionldState.mongoc.subscriptionsP)  mongoc_collection_destroy(orionldState.mongoc.subscriptionsP);
  if (orionldState.mongoc.registrationsP)  mongoc_collection_destroy(orionldState.mongoc.registrationsP);

  mongocConnectionRelease();

  //
  // CURL Handles And Headers
  //
  if (orionldState.multiP != NULL)
  {
    for (int ix = 0; ix < orionldState.easyIx; ix++)
    {
      curl_multi_remove_handle(orionldState.multiP, orionldState.easyV[ix]);
      curl_easy_cleanup(orionldState.easyV[ix]);
    }

    curl_multi_cleanup(orionldState.multiP);
  }

  if (orionldState.easyV != NULL)
    free(orionldState.easyV);

  if (orionldState.curlHeadersV != NULL)
  {
    for (int ix = 0; ix < orionldState.curlHeadersIx; ix++)
    {
      curl_slist_free_all(orionldState.curlHeadersV[ix]);
    }

    free(orionldState.curlHeadersV);
  }

  if (orionldState.curlDoMultiP != NULL)
  {
    curl_multi_cleanup(orionldState.curlDoMultiP);
    orionldState.curlDoMultiP = NULL;
  }

  if (orionldState.distOpList != NULL)
    distOpListRelease(orionldState.distOpList);

  lmTransactionEnd();  // Incoming REST request ends

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &reqEndTime);
    clock_difftime(&reqEndTime, &orionldState.timestamp, &threadLastTimeStat.reqTime);
  }


  //
  // Statistics
  //
  // Flush this requests timing measures onto a global var to be read by "GET /statistics".
  // Also, increment the accumulated measures.
  //
  if (timingStatistics)
  {
    timeStatSemTake(__FUNCTION__, "updating statistics");

    memcpy(&lastTimeStat, &threadLastTimeStat, sizeof(lastTimeStat));

    //
    // "Fix" mongoBackendTime
    //   Substract times waiting at mongo driver operation (in mongo[Read|Write|Command]WaitTime counters) so mongoBackendTime
    //   contains at the end the time passed in our logic, i.e. a kind of "self-time" for mongoBackend
    //
    clock_subtime(&threadLastTimeStat.mongoBackendTime, &threadLastTimeStat.mongoReadWaitTime);
    clock_subtime(&threadLastTimeStat.mongoBackendTime, &threadLastTimeStat.mongoWriteWaitTime);
    clock_subtime(&threadLastTimeStat.mongoBackendTime, &threadLastTimeStat.mongoCommandWaitTime);

    clock_addtime(&accTimeStat.jsonV1ParseTime,       &threadLastTimeStat.jsonV1ParseTime);
    clock_addtime(&accTimeStat.jsonV2ParseTime,       &threadLastTimeStat.jsonV2ParseTime);
    clock_addtime(&accTimeStat.mongoBackendTime,      &threadLastTimeStat.mongoBackendTime);
    clock_addtime(&accTimeStat.mongoWriteWaitTime,    &threadLastTimeStat.mongoWriteWaitTime);
    clock_addtime(&accTimeStat.mongoReadWaitTime,     &threadLastTimeStat.mongoReadWaitTime);
    clock_addtime(&accTimeStat.mongoCommandWaitTime,  &threadLastTimeStat.mongoCommandWaitTime);
    clock_addtime(&accTimeStat.renderTime,            &threadLastTimeStat.renderTime);
    clock_addtime(&accTimeStat.reqTime,               &threadLastTimeStat.reqTime);

    timeStatSemGive(__FUNCTION__, "updating statistics");
  }

  //
  // Metrics
  //
  if ((orionldState.apiVersion != API_VERSION_NGSILD_V1) && (metricsMgr.isOn()))
  {
    metricsMgr.add(orionldState.tenantP->tenant, spath, METRIC_TRANS_IN, 1);

    if (orionldState.httpStatusCode >= 400)
      metricsMgr.add(orionldState.tenantP->tenant, spath, METRIC_TRANS_IN_ERRORS, 1);

    if (orionldState.transactionStart.tv_sec != 0)
    {
      struct timeval  end;

      if (gettimeofday(&end, NULL) == 0)
      {
        unsigned long long elapsed =
          (end.tv_sec  - orionldState.transactionStart.tv_sec) * 1000000 +
          (end.tv_usec - orionldState.transactionStart.tv_usec);

        metricsMgr.add(orionldState.tenantP->tenant, spath, _METRIC_TOTAL_SERVICE_TIME, elapsed);
      }
    }
  }

  //
  // delayed release of ContextElementResponseVector must be effectuated now.
  // See github issue #2994
  //
  extern void delayedReleaseExecute(void);
  delayedReleaseExecute();

  if (orionldState.apiVersion != API_VERSION_NGSILD_V1)
    delete(ciP);

  kaBufferReset(&orionldState.kalloc, false);  // 'false': it's reused, but in a different thread ...

  if ((orionldState.responseTree != NULL) && (orionldState.kjsonP == NULL))
    kjFree(orionldState.responseTree);

  *con_cls = NULL;

#ifdef REQUEST_PERFORMANCE
  // if (requestNo % 100 == 0)
  {
    PERFORMANCE(reqEnd);

    TIME_REPORT(performanceTimestamps.reqStart,            performanceTimestamps.serviceRoutineStart,    "Before Service Routine:    ");
    TIME_REPORT(performanceTimestamps.serviceRoutineStart, performanceTimestamps.serviceRoutineEnd,      "During Service Routine:    ");
    TIME_REPORT(performanceTimestamps.serviceRoutineEnd,   performanceTimestamps.reqEnd,                 "After Service Routine:     ");
    TIME_REPORT(performanceTimestamps.parseStart,          performanceTimestamps.parseEnd,               "Payload Parse:             ");
    TIME_REPORT(performanceTimestamps.renderStart,         performanceTimestamps.renderEnd,              "Rendering Response:        ");
    TIME_REPORT(performanceTimestamps.restReplyStart,      performanceTimestamps.restReplyEnd,           "Sending Response:          ");
    TIME_REPORT(performanceTimestamps.forwardStart,        performanceTimestamps.forwardEnd,             "Forwarding:                ");
    TIME_REPORT(performanceTimestamps.forwardDbStart,      performanceTimestamps.forwardDbEnd,           "DB Query for Forwarding:   ");
    TIME_REPORT(performanceTimestamps.reqStart,            performanceTimestamps.reqEnd,                 "Entire request:            ");
    TIME_REPORT(performanceTimestamps.troeStart,           performanceTimestamps.troeEnd,                "TRoE Processing:           ");
    TIME_REPORT(performanceTimestamps.requestPartEnd,      performanceTimestamps.requestCompletedStart,  "MHD Delay (send response): ");

    TIME_REPORT(performanceTimestamps.dbStart,             performanceTimestamps.dbEnd,                  "Awaiting DB:               ");
    TIME_REPORT(performanceTimestamps.mongoBackendStart,   performanceTimestamps.mongoBackendEnd,        "Awaiting MongoBackend:     ");

    for (int ix = 0; ix < 50; ix++)
    {
      if (performanceTimestamps.srDesc[ix] != NULL)
        TIME_REPORT(performanceTimestamps.srStart[ix], performanceTimestamps.srEnd[ix], performanceTimestamps.srDesc[ix]);
    }

    struct timespec  all;
    struct timespec  mongo;
    float            mongoF;
    float            allF;

    kTimeDiff(&performanceTimestamps.reqStart, &performanceTimestamps.reqEnd, &all,   &allF);

    if (performanceTimestamps.dbStart.tv_sec != 0)
      kTimeDiff(&performanceTimestamps.dbStart,           &performanceTimestamps.dbEnd,           &mongo, &mongoF);
    else
      kTimeDiff(&performanceTimestamps.mongoBackendStart, &performanceTimestamps.mongoBackendEnd, &mongo, &mongoF);

    LM_T(LmtPerformance, ("TPUT: Entire request - DB:        %f", allF - mongoF));  // Only for REQUEST_PERFORMANCE
    LM_T(LmtPerformance, ("TPUT: mongoConnect Accumulated:   %f (%d calls)", performanceTimestamps.mongoConnectAccumulated, performanceTimestamps.getMongoConnectionCalls));
  }
#endif
}



/* ****************************************************************************
*
* servicePathCheck - check vector of service paths
*
* This function is called for ALL requests, when a service-path URI-parameter is found.
* So, '#' is considered a valid character as it is valid for discoveries and queries.
* Later on, if the request is a registration or notification, another function is called
* to make sure there is only ONE service path and that there is no '#' present.
*
* FIXME P5: updates should also call the other servicePathCheck (in common lib)
*/
int servicePathCheck(ConnectionInfo* ciP, const char* servicePath)
{
  //
  // 1. Max 10 paths  - ONLY ONE path allowed at this moment
  // 2. Max 10 levels in each path
  // 3. Min 1, Max 50 characters in each path component
  // 4. Only alphanum and underscore allowed (just like in tenants)
  //    OR: Last component is EXACTLY '#'
  //
  // About the constants 10 and 50, see common/limits.h:
  //   - SERVICE_PATH_MAX_COMPONENTS
  //   - SERVICE_PATH_MAX_LEVELS
  //   - SERVICE_PATH_MAX_COMPONENT_LEN
  //
  std::vector<std::string> compV;
  int                      components;


  if (servicePath == NULL)
    return 0;

  if (servicePath[0] == 0)  // Special case, corresponding to default service path
    return 0;


  if (servicePath[0] != '/')
  {
    OrionError oe(SccBadRequest, "Only /absolute/ Service Paths allowed [a service path must begin with /]");
    ciP->answer = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
    return 1;
  }

  components = stringSplit(servicePath, '/', compV);

  if (components > SERVICE_PATH_MAX_LEVELS)
  {
    OrionError oe(SccBadRequest, "too many components in ServicePath");
    ciP->answer = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
    return 2;
  }

  for (int ix = 0; ix < components; ++ix)
  {
    if (strlen(compV[ix].c_str()) > SERVICE_PATH_MAX_COMPONENT_LEN)
    {
      OrionError oe(SccBadRequest, "component-name too long in ServicePath");
      ciP->answer = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
      return 3;
    }

    if (compV[ix].c_str()[0] == 0)
    {
      OrionError oe(SccBadRequest, "empty component in ServicePath");
      ciP->answer = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
      return 3;
    }


    // Last token in the path is allowed to be *exactly* "#", as in /Madrid/Gardens/#. Note that
    // /Madrid/Gardens/North# is not allowed
    if ((ix == components - 1) && (compV[ix] == "#"))
    {
      continue;
    }

    const char* comp = compV[ix].c_str();

    for (unsigned int cIx = 0; cIx < strlen(comp); ++cIx)
    {
      if (!isalnum(comp[cIx]) && (comp[cIx] != '_'))
      {
        OrionError oe(SccBadRequest, "a component of ServicePath contains an illegal character");
        ciP->answer = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);
        return 4;
      }
    }
  }

  return 0;
}



/* ****************************************************************************
*
* removeTrailingSlash -
*/
static char* removeTrailingSlash(char* path)
{
  int len = strlen(path);

  // len > 1 ensures that root service path "/" is not touched
  while ((len > 1) && (path[len - 1] == '/'))
  {
    path[len - 1] = 0;
    --len;
  }

  return path;
}



/* ****************************************************************************
*
* firstServicePath - extract first component of service-path
*/
void firstServicePath(const char* servicePath, char* servicePath0, int servicePath0Len)
{
  char* spEnd;

  memset(servicePath0, 0, servicePath0Len);

  if ((spEnd = strchr((char*) servicePath, ',')) != NULL)
  {
    strncpy(servicePath0, servicePath, spEnd - servicePath);
  }
  else
  {
    strncpy(servicePath0, servicePath, servicePath0Len);
  }
}



/* ****************************************************************************
*
* isOriginAllowedForCORS - checks the Origin header of the request and returns
* true if that Origin is allowed to make a CORS request
*/
bool isOriginAllowedForCORS(const char* requestOrigin)
{
  return ((requestOrigin != NULL) && ((strcmp(corsOrigin, "__ALL") == 0) || (strcmp(requestOrigin, corsOrigin) == 0)));
}



/* ****************************************************************************
*
* servicePathSplit -
*/
int servicePathSplit(ConnectionInfo* ciP)
{
  char* servicePathCopy = NULL;
  int   servicePaths    = 0;

  if (orionldState.in.servicePath != NULL)
  {
    servicePathCopy = strdup(orionldState.in.servicePath);
    servicePaths    = stringSplit(servicePathCopy, ',', ciP->servicePathV);
  }
  else
  {
    // No service path? Add a single item being the empty string
    ciP->servicePathV.push_back("");
  }

  if (servicePaths == 0)
  {
    // In this case the result is a vector with one item that is an empty string
    if (servicePathCopy != NULL)
    {
      free(servicePathCopy);
    }
    return 0;
  }

  if (servicePaths > SERVICE_PATH_MAX_COMPONENTS)
  {
    OrionError e(SccBadRequest, "too many service paths - a maximum of ten service paths is allowed");
    ciP->answer = e.render();

    if (servicePathCopy != NULL)
    {
      free(servicePathCopy);
    }

    return -1;
  }

  for (int ix = 0; ix < servicePaths; ++ix)
  {
    char* stripped = stringStrip((char*) ciP->servicePathV[ix].c_str());

    ciP->servicePathV[ix] = removeTrailingSlash(stripped);

    //
    // This was previously an LM_T trace, but we have "promoted" it to INFO as
    // it is needed to check logs in a .test case (case 0392 service_path_http_header.test)
    //
    LM_K(("Service Path %d: '%s'", ix, ciP->servicePathV[ix].c_str()));  // Sacred - used by functest service_path_http_header.test
  }


  for (int ix = 0; ix < servicePaths; ++ix)
  {
    int s;

    if ((s = servicePathCheck(ciP, ciP->servicePathV[ix].c_str())) != 0)
    {
      if (servicePathCopy != NULL)
      {
        free(servicePathCopy);
      }

      return s;
    }
  }

  if (servicePathCopy != NULL)
  {
    free(servicePathCopy);
  }

  return 0;
}



/* ****************************************************************************
*
* contentTypeCheck -
*
* NOTE
*   Any failure about Content-Type is an error in the HTTP layer (not exclusively NGSI)
*   so we don't want to use the default 200
*
* NOTE
*   In version 1 of the protocol, we admit ONLY application/json
*   In version 2 of the protocol, we admit ONLY application/json and text/plain
*/
static int contentTypeCheck(ConnectionInfo* ciP)
{
  //
  // Five cases:
  //   1. If there is no payload, the Content-Type is not interesting
  //   2. Payload present but no Content-Type
  //   3. Content-Type present but not supported
  //   4. API version 2 and not 'application/json' || text/plain
  //


  // Case 1
  if (orionldState.in.contentLength == 0)
  {
    return 0;
  }


  // Case 2
  if (orionldState.in.contentType == MT_NOTGIVEN)
  {
    std::string details = "Content-Type header not used, default application/octet-stream is not supported";
    orionldState.httpStatusCode = SccUnsupportedMediaType;
    restErrorReplyGet(ciP, SccUnsupportedMediaType, details, &ciP->answer);
    orionldState.httpStatusCode = SccUnsupportedMediaType;

    return 1;
  }

  // Case 3
  if ((orionldState.apiVersion == API_VERSION_NGSI_V1) && (orionldState.in.contentType != MT_JSON))
  {
    std::string details = std::string("not supported content type: ");

    if (orionldState.in.contentTypeString == NULL)
      details = "content type missing";
    else
      details += orionldState.in.contentTypeString;

    orionldState.httpStatusCode = SccUnsupportedMediaType;
    restErrorReplyGet(ciP, SccUnsupportedMediaType, details, &ciP->answer);
    orionldState.httpStatusCode = SccUnsupportedMediaType;
    return 1;
  }


  // Case 4
  if ((orionldState.apiVersion == API_VERSION_NGSI_V2) && (orionldState.in.contentType != MT_JSON) && (orionldState.in.contentType != MT_TEXT))
  {
    std::string details = std::string("not supported content type: ") + orionldState.in.contentTypeString;
    orionldState.httpStatusCode = SccUnsupportedMediaType;
    restErrorReplyGet(ciP, SccUnsupportedMediaType, details, &ciP->answer);
    orionldState.httpStatusCode = SccUnsupportedMediaType;
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* urlCheck - check for forbidden characters and remove trailing slashes
*
* Returns 'true' if the URL is OK, 'false' otherwise.
* ciP->answer and orionldState.httpStatusCode are set if an error is encountered.
*
*/
bool urlCheck(ConnectionInfo* ciP, const std::string& url)
{
  if (forbiddenChars(url.c_str()) == true)
  {
    OrionError error(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI);
    orionldState.httpStatusCode = error.code;
    ciP->answer                 = error.smartRender(orionldState.apiVersion);
    return false;
  }

  //
  // Remove ONE '/' at end of URL path
  //
  char* s = (char*) url.c_str();
  if (s[strlen(s) - 1] == '/')
  {
    s[strlen(s) - 1] = 0;
  }

  return true;
}



/* ****************************************************************************
*
* apiVersionGet -
*
* This function returns the version of the API for the incoming message,
* based on the URL according to:
*
*  API_VERSION_NGSI_V2:  for URLs in the /v2 path
*  API_VERSION_NGSI_V1:  for URLs in the /v1 or with an equivalence (e.g. /ngi10, /log, etc.)
*  API_VERSION_ADMIN:    admin operations without /v1 alias
*  API_VERSION_NONE:     others (invalid paths)
*
*/
static ApiVersion apiVersionGet(const char* path)
{
  if ((path[1] == 'v') && (path[2] == '2'))
  {
    return API_VERSION_NGSI_V2;
  }

  // Unlike v2, v1 is case-insensitive (see case/2057 test)
  if (((path[1] == 'v') || (path[1] == 'V')) && (path[2] == '1'))
  {
    return API_VERSION_NGSI_V1;
  }

  if ((strncasecmp("/ngsi9",      path, strlen("/ngsi9"))      == 0)  ||
      (strncasecmp("/ngsi10",     path, strlen("/ngsi10"))     == 0))
  {
    return API_VERSION_NGSI_V1;
  }

  if ((strncasecmp("/log",        path, strlen("/log"))        == 0)  ||
      (strncasecmp("/cache",      path, strlen("/cache"))      == 0)  ||
      (strncasecmp("/statistics", path, strlen("/statistics")) == 0))
  {
    return API_VERSION_NGSI_V1;
  }

  if ((strncmp("/admin",   path, strlen("/admin"))   == 0) ||
      (strncmp("/version", path, strlen("/version")) == 0))
  {
    return API_VERSION_ADMIN;
  }

  return API_VERSION_NONE;
}



/* ****************************************************************************
*
* restServiceForBadVerb - dummy instance
*/
RestService restServiceForBadVerb;



extern Verb verbGet(const char* method);
extern MHD_Result orionldUriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value);
/* ****************************************************************************
*
* connectionTreatInit -
*/
ConnectionInfo* connectionTreatInit
(
  MHD_Connection*  connection,
  const char*      url,
  const char*      method,
  const char*      version,
  MHD_Result*      retValP
)
{
  struct timeval   transactionStart;
  ConnectionInfo*  ciP;

  *retValP = MHD_YES;  // MHD_NO only if allocation of ConnectionInfo fails

  // Create point in time for transaction metrics
  if (metricsMgr.isOn())
  {
    if (gettimeofday(&transactionStart, NULL) == -1)
    {
      transactionStart.tv_sec  = 0;
      transactionStart.tv_usec = 0;
    }
  }

  //
  // First thing to do on a new connection, set correlator to N/A.
  // After reading HTTP headers, the correlator id either changes due to encountering a
  // Fiware-Correlator HTTP Header, or, if no HTTP header with Fiware-Correlator is found,
  // a new correlator is generated.
  //
  correlatorIdSet("N/A");

  //
  // IP Address and port of caller
  //
  char            ip[IP_LENGTH_MAX];
  unsigned short  port = 0;

  const union MHD_ConnectionInfo* mciP = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

  if (mciP != NULL)
  {
    struct sockaddr* addr = (struct sockaddr*) mciP->client_addr;

    port = (addr->sa_data[0] << 8) + addr->sa_data[1];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
             addr->sa_data[2] & 0xFF,
             addr->sa_data[3] & 0xFF,
             addr->sa_data[4] & 0xFF,
             addr->sa_data[5] & 0xFF);
    snprintf(orionldState.clientIp, sizeof(orionldState.clientIp), "%s", ip);
  }
  else
  {
    port = 0;
    snprintf(ip, sizeof(ip), "IP unknown");
  }


  //
  // Reset time measuring?
  //
  if (timingStatistics)
  {
    memset(&threadLastTimeStat, 0, sizeof(threadLastTimeStat));
  }


  orionldState.transactionStart.tv_sec  = transactionStart.tv_sec;
  orionldState.transactionStart.tv_usec = transactionStart.tv_usec;

  //
  // ConnectionInfo
  //
  if ((ciP = new ConnectionInfo(connection)) == NULL)
  {
    LM_E(("Runtime Error (error allocating ConnectionInfo)"));
    // No METRICS here ... Without ConnectionInfo we have no service/subService ...
    *retValP = MHD_NO;
    return NULL;
  }

  //
  // HTTP Headers
  //
  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);

  if ((orionldState.correlator == NULL) || (orionldState.correlator[0] == 0))
  {
    orionldState.correlator = kaAlloc(&orionldState.kalloc, CORRELATOR_ID_SIZE + 1);
    correlatorGenerate(orionldState.correlator);
  }

  correlatorIdSet(orionldState.correlator);

  orionldHeaderAdd(&orionldState.out.headers, HttpCorrelator, orionldState.correlator, 0);

  if (((unsigned long long) orionldState.in.contentLength > inReqPayloadMaxSize) && (orionldState.apiVersion == API_VERSION_NGSI_V2))
  {
    char details[256];
    snprintf(details, sizeof(details), "payload size: %d, max size supported: %llu", orionldState.in.contentLength, inReqPayloadMaxSize);

    alarmMgr.badInput(orionldState.clientIp, details);
    OrionError oe(SccRequestEntityTooLarge, details);

    orionldState.httpStatusCode = oe.code;
    ciP->answer                 = oe.smartRender(orionldState.apiVersion);

    //
    // FIXME P4:
    // Supposedly, we aren't ready to respond to the HTTP request at this early stage, before reading the content.
    // However, tests have shown that the broker hangs if the response is delayed until later calls ...
    //
    //
    restReply(ciP, ciP->answer.c_str());  // to not hang on too big payloads
    return ciP;
  }


  // Error detected in HTTP headers?
  if (orionldState.httpStatusCode != 200)
  {
    LM_E(("ERROR in HTTP Headers (%s: %s)", orionldState.pd.title, orionldState.pd.detail));
    OrionError oe((HttpStatusCode) orionldState.httpStatusCode, orionldState.pd.detail);
    ciP->answer = oe.smartRender(orionldState.apiVersion);

    return ciP;
  }

  //
  // URI parameters
  //
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, orionldUriArgumentGet, ciP);
  if (orionldState.httpStatusCode >= 400)
  {
    LM_W(("Bad Request (error in URI parameters - %s: %s)", orionldState.pd.title, orionldState.pd.detail));
    if (orionldState.httpStatusCode != 406)
    {
      OrionError oe(SccBadRequest, orionldState.pd.title? orionldState.pd.title : "TITLE");
      ciP->answer = oe.smartRender(orionldState.apiVersion);
    }

    return ciP;
  }


  //
  // Transaction starts here
  //
  lmTransactionStart("from", "", ip, port, url);  // Incoming REST request starts

  //
  // X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides ip.
  // X-Real-IP takes preference over X-Forwarded-For, if both appear */
  //
  char* transactionIp = ip;

  if (orionldState.in.xRealIp != NULL)
    transactionIp = orionldState.in.xRealIp;
  else if (orionldState.in.xForwardedFor != NULL)
    transactionIp = orionldState.in.xForwardedFor;

  lmTransactionSetFrom(transactionIp);

  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

  // Lookup Rest Service
  bool badVerb = false;

  ciP->restServiceP = restServiceLookup(ciP, &badVerb);

  if (urlCheck(ciP, orionldState.urlPath) == false)
  {
    alarmMgr.badInput(orionldState.clientIp, "error in URI path");
  }
  else if (servicePathSplit(ciP) != 0)
  {
    alarmMgr.badInput(orionldState.clientIp, "error in ServicePath http-header");
  }
  else if (contentTypeCheck(ciP) != 0)
  {
    alarmMgr.badInput(orionldState.clientIp, "invalid mime-type in Content-Type http-header");
  }
  //
  // Requests of verb POST, PUT or PATCH are considered erroneous if no payload is present - with the exception of log requests.
  //
  else if ((orionldState.in.contentLength == 0) &&
           ((orionldState.verb == HTTP_POST) || (orionldState.verb == HTTP_PUT) || (orionldState.verb == HTTP_PATCH )) &&
           (strncasecmp(orionldState.urlPath, "/log/", 5) != 0) &&
           (strncasecmp(orionldState.urlPath, "/admin/log", 10) != 0))
  {
    std::string errorMsg;

    restErrorReplyGet(ciP, SccContentLengthRequired, "Zero/No Content-Length in PUT/POST/PATCH request", &errorMsg);
    orionldState.httpStatusCode  = SccContentLengthRequired;
    restReply(ciP, errorMsg.c_str());  // OK to respond as no payload
    alarmMgr.badInput(orionldState.clientIp, errorMsg);
  }
  else if (orionldState.badVerb == true)
  {
    // Not ready to answer here - must wait until all the payload has been read
    orionldState.httpStatusCode = SccBadVerb;
  }

  //
  // If Accept: TEXT was chosen, only a few service routines allow this.
  // If also JSON is accepted, then all is OK - just change the chosen format for JSON
  // If JSON is NOT accepted, then we have an error
  //
  // This algorithm would be better if we knew this beforehand - before calling acceptHeaderParse.
  // So, could be better ...
  //
  if (orionldState.out.contentType == MT_TEXT)
  {
    if ((orionldState.acceptMask & (1 << MT_JSON)) == 0)
    {
      if (ciP->restServiceP->treat != getEntityAttributeValue)
      {
        OrionError oe(SccNotAcceptable, "Invalid Mime Type");
        ciP->answer = oe.smartRender(orionldState.apiVersion);
        orionldState.httpStatusCode  = 406;
        orionldState.out.contentType = MT_JSON;  // JSON output for the error?
      }
    }
  }

  return ciP;
}



/* ****************************************************************************
*
* connectionTreatDataReceive -
*/
static MHD_Result connectionTreatDataReceive(ConnectionInfo* ciP, size_t* upload_data_size, const char* upload_data)
{
  size_t  dataLen = *upload_data_size;

  //
  // If the HTTP header says the request is bigger than inReqPayloadMaxSize,
  // just silently "eat" the entire message.
  //
  // The problem occurs when the broker is lied to and there aren't orionldState.in.contentLength
  // bytes to read.
  // When this happens, MHD blocks until it times out (MHD_OPTION_CONNECTION_TIMEOUT defaults to 5 seconds),
  // and the broker isn't able to respond. MHD just closes the connection.
  // Question asked in mhd mailing list.
  //
  // See github issue:
  //   https://github.com/telefonicaid/fiware-orion/issues/2761
  //
  if ((unsigned long long) orionldState.in.contentLength > inReqPayloadMaxSize)
  {
    //
    // Errors can't be returned yet, postpone ...
    //
    *upload_data_size = 0;
    return MHD_YES;
  }

  //
  // First call with payload - use "orionldState.preallocReqBuf" if possible,
  // otherwise allocate a bigger buffer
  //
  // FIXME P1: This could be done in "Part I" instead, saving an "if" for each "Part II" call
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  if (orionldState.in.payloadSize == 0)  // First call with payload
  {
    if (orionldState.in.contentLength >= (int) sizeof(orionldState.preallocReqBuf))
    {
      orionldState.in.payload = (char*) malloc(orionldState.in.contentLength + 1);
    }
    else
    {
      orionldState.in.payload = orionldState.preallocReqBuf;
    }
  }

  // Copy the chunk
  memcpy(&orionldState.in.payload[orionldState.in.payloadSize], upload_data, dataLen);

  // Add to the size of the accumulated read buffer
  orionldState.in.payloadSize += *upload_data_size;

  // Zero-terminate the payload
  orionldState.in.payload[orionldState.in.payloadSize] = 0;

  // Acknowledge the data and return
  *upload_data_size = 0;
  return MHD_YES;
}



/* ****************************************************************************
*
* connectionTreat -
*
* This is the MHD_AccessHandlerCallback function for MHD_start_daemon
* This function returns:
* o MHD_YES  if the connection was handled successfully
* o MHD_NO   if the socket must be closed due to a serious error
*
* - This function is called once when the headers are read and the ciP is created.
* - Then it is called for data payload and once all the payload an acknowledgement
*   must be done, setting *upload_data_size to ZERO.
* - The last call is made with *upload_data_size == 0 and now is when the connection
*   is open to send responses.
*
* Call 1: *con_cls == NULL
* Call 2: *con_cls != NULL  AND  *upload_data_size != 0
* Call 3: *con_cls != NULL  AND  *upload_data_size == 0
*/
static MHD_Result connectionTreat
(
   void*            cls,
   MHD_Connection*  connection,
   const char*      url,
   const char*      method,
   const char*      version,
   const char*      upload_data,
   size_t*          upload_data_size,
   void**           con_cls
)
{
  //
  // NGSI-LD requests implement a different URL parsing algorithm, a different payload parse algorithm, etc.
  // A complete new set of functions are used for NGSI-LD, so ...
  //
  if (strncmp(url, "/ngsi-ld/", 9) == 0)
  {
    if (*con_cls == NULL)
    {
      *con_cls = &cls;  // to "acknowledge" the first call

#ifdef REQUEST_PERFORMANCE
      bzero(&performanceTimestamps, sizeof(performanceTimestamps));
      kTimeGet(&performanceTimestamps.reqStart);
#endif
      // Reset the Compound stuff
      compoundInfo.compoundValueRoot = NULL;
      compoundInfo.compoundValueP    = NULL;
      compoundInfo.inCompoundValue   = false;

      return mhdConnectionInit(connection, url, method, version, con_cls);
    }
    else if (*upload_data_size != 0)
      return mhdConnectionPayloadRead(upload_data_size, upload_data);

    //
    // The entire message has been read, we're allowed to respond.
    //

    // Mark the request as "finished", by setting upload_data_size to 0
    *upload_data_size = 0;

    // Then treat the request
    return mhdConnectionTreat();
  }

  //
  //  NOT NGSI-LD
  //

  // 1. First call - setup ConnectionInfo and get/check HTTP headers
  if (*con_cls == NULL)
  {
    MHD_Result retVal;

    ++requestNo;
    LM_K(("------------------------- Servicing NGSIv2 request %03d: %s %s --------------------------", requestNo, method, url));

    // Reset the Compound stuff
    compoundInfo.compoundValueRoot = NULL;
    compoundInfo.compoundValueP    = NULL;
    compoundInfo.inCompoundValue   = false;

    //
    // Setting crucial fields of orionldState - those that are used for non-ngsi-ld requests
    //
    kTimeGet(&orionldState.timestamp);
    orionldStateInit(connection);
    orionldState.httpVersion    = (char*) version;
    orionldState.apiVersion     = apiVersionGet(url);
    orionldState.verbString     = (char*) method;
    orionldState.verb           = verbGet(method);
    orionldState.requestTime    = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;
    orionldState.responseTree   = NULL;
    orionldState.urlPath        = (char*) url;
    orionldState.attrsFormat    = (char*) "normalized";
    orionldState.correlator     = (char*) "";
    orionldState.httpStatusCode = 200;

    *con_cls = connectionTreatInit(connection, url, method, version, &retVal);

    ConnectionInfo* ciP = (ConnectionInfo*) *con_cls;

    if ((mongocOnly == true) && (strcmp("/exit/harakiri", url) != 0) && (strcmp("/version", url) != 0))
    {
      OrionError error(SccNotImplemented, "Non NGSI-LD requests are not supported with -mongocOnly is set");
      LM_E(("Non NGSI-LD requests are not supported with -mongocOnly is set"));

      orionldState.httpStatusCode = 501;
      ciP->answer                 = error.smartRender(orionldState.apiVersion);

      return MHD_YES;
    }

    //
    // Check validity of URI parameters
    //
    bool keyValuesEtAl = true;

    if      ((orionldState.uriParamOptions.keyValues) && (orionldState.uriParamOptions.values))        keyValuesEtAl = false;
    else if ((orionldState.uriParamOptions.keyValues) && (orionldState.uriParamOptions.uniqueValues))  keyValuesEtAl = false;
    else if ((orionldState.uriParamOptions.values)    && (orionldState.uriParamOptions.uniqueValues))  keyValuesEtAl = false;
    if (keyValuesEtAl == false)
    {
      OrionError error(SccBadRequest, "Invalid value for URI param /options/");
      LM_W(("Bad Input (Invalid value for URI param /options/)"));

      orionldState.httpStatusCode = 400;
      ciP->answer                 = error.smartRender(orionldState.apiVersion);

      return MHD_YES;
    }

    if ((orionldState.httpStatusCode >= 400) && (orionldState.httpStatusCode != 405) && (ciP->answer == ""))
    {
      const char* title  = (orionldState.pd.title  != NULL)? orionldState.pd.title  : "no title";
      const char* detail = (orionldState.pd.detail != NULL)? orionldState.pd.detail : "no detail";
      OrionError  error((HttpStatusCode) orionldState.httpStatusCode, title, detail);

      ciP->answer = error.smartRender(orionldState.apiVersion);
      return MHD_YES;
    }

    return retVal;
  }



  // 2. Data gathering calls
  ConnectionInfo* ciP = (ConnectionInfo*) *con_cls;

  if (*upload_data_size != 0)
  {
    return connectionTreatDataReceive(ciP, upload_data_size, upload_data);
  }


  //
  // The entire payload has been read and we are ready to serve the request.
  //
  // Before this point, MHD is not ready to respond tp the caller.
  // Plenty of validity checks of the request are performed here, and error responses ar sent if errors found
  // Finally, if all is OK, the request is served by calling the function orion::requestServe
  //

  //
  // As older (non NGSI-LD) requests also need orionldState.tenantP, this piece of code has been copied
  // from mhdConnectionTreat(). Had to be a little simplified though ...
  //
  if (orionldState.tenantName != NULL)
    orionldState.tenantP = orionldTenantGet(orionldState.tenantName);
  else
    orionldState.tenantP = &tenant0;

  lmTransactionSetSubservice(orionldState.in.servicePath);

  if ((orionldState.httpStatusCode != SccOk) && (orionldState.httpStatusCode != SccBadVerb))
  {
    // An error has occurred. Here we are ready to respond, as all data has been read
    // However, if badVerb, then the service routine needs to execute to add the "Allow" HTTP header
    restReply(ciP, ciP->answer.c_str());
    return MHD_YES;
  }

  //
  // If the incoming request was too big, return error about it
  //
  if ((unsigned long long) orionldState.in.contentLength > inReqPayloadMaxSize)
  {
    char details[256];

    snprintf(details, sizeof(details), "payload size: %d, max size supported: %llu", orionldState.in.contentLength, inReqPayloadMaxSize);
    alarmMgr.badInput(orionldState.clientIp, details);
    restErrorReplyGet(ciP, SccRequestEntityTooLarge, details, &ciP->answer);

    orionldState.httpStatusCode = SccRequestEntityTooLarge;
  }


  //
  // Check for error during Accept Header parsing
  //
  if (orionldState.out.acceptErrorDetail != NULL)
  {
    OrionError oe(SccBadRequest, (orionldState.out.acceptErrorDetail == NULL)? "no detail" : orionldState.out.acceptErrorDetail);

    orionldState.httpStatusCode = oe.code;
    alarmMgr.badInput(orionldState.clientIp, orionldState.out.acceptErrorDetail);
    restReply(ciP, oe.smartRender(orionldState.apiVersion).c_str());
    return MHD_YES;
  }


  //
  // Check that Accept Header values are valid
  //
  if (orionldState.out.contentType == MT_NONE)
  {
    OrionError oe(SccNotAcceptable, "acceptable MIME types: application/json, text/plain");

    if ((orionldState.acceptMask & (1 << MT_TEXT)) == 0)
    {
      oe.details = "acceptable MIME types: application/json";
    }

    orionldState.httpStatusCode = oe.code;
    alarmMgr.badInput(orionldState.clientIp, oe.details);
    restReply(ciP, oe.smartRender(orionldState.apiVersion).c_str());
    return MHD_YES;
  }

  if (orionldState.out.contentType == MT_NONE)
  {
    OrionError oe(SccNotAcceptable, "acceptable MIME types: application/json, text/plain");

    if ((orionldState.acceptMask & (1 << MT_TEXT)) == 0)
    {
      oe.details = "acceptable MIME types: application/json";
    }

    orionldState.httpStatusCode = oe.code;
    alarmMgr.badInput(orionldState.clientIp, oe.details);
    restReply(ciP, oe.smartRender(orionldState.apiVersion).c_str());
    return MHD_YES;
  }


  //
  // Check Content-Type and Content-Length for GET/DELETE requests
  //
  LM_W(("orionldState.in.contentType: %s", mimeType(orionldState.in.contentType)));
  if ((orionldState.in.contentType != MT_NOTGIVEN) && (orionldState.in.contentType != MT_NONE) && (orionldState.in.contentLength == 0) && ((orionldState.verb == HTTP_GET) || (orionldState.verb == HTTP_DELETE)))
  {
    const char*  details = "Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden";
    OrionError   oe(SccBadRequest, details);

    orionldState.httpStatusCode = oe.code;
    alarmMgr.badInput(orionldState.clientIp, details);
    restReply(ciP, oe.smartRender(orionldState.apiVersion).c_str());

    return MHD_YES;
  }


  //
  // If ciP->answer is non-empty, then an error has been detected
  //
  if (ciP->answer != "")
  {
    alarmMgr.badInput(orionldState.clientIp, ciP->answer);
    restReply(ciP, ciP->answer.c_str());

    return MHD_YES;
  }


  //
  // If error detected, just call treat function and respond to caller
  //
  if (orionldState.httpStatusCode != SccOk)
  {
    ciP->answer = ciP->restServiceP->treat(ciP, ciP->urlComponents, ciP->urlCompV, NULL);

    // Bad Verb in API v1 should have empty payload
    if ((orionldState.apiVersion == API_VERSION_NGSI_V1) && (orionldState.httpStatusCode == SccBadVerb))
    {
      ciP->answer = "";
    }

    restReply(ciP, ciP->answer.c_str());
  }
  else
  {
    // All is good. The request can be served.
    orion::requestServe(ciP);
  }

  return MHD_YES;
}



/* ****************************************************************************
*
* restStart -
*
* NOTE, according to MHD documentation, thread pool (MHD_OPTION_THREAD_POOL_SIZE) cannot be used
* is conjunction with MHD_USE_THREAD_PER_CONNECTION.
* However, we have seen that if the thread pool size is 0 (the case of NOT using thread pool), then
* MHD_start_daemon is OK with it.
*
* From MHD documentation:
* MHD_OPTION_THREAD_POOL_SIZE
*   Number (unsigned int) of threads in thread pool. Enable thread pooling by setting this value to to something greater than 1.
*   Currently, thread model must be MHD_USE_SELECT_INTERNALLY if thread pooling is enabled (MHD_start_daemon returns NULL for
*   an unsupported thread model).
*
* MHD_USE_EPOLL:
*   Use `epoll()` instead of `select()` or `poll()` for the event loop.
*   This option is only available on some systems; using the option on
*   systems without epoll will cause #MHD_start_daemon to fail.  Using
*   this option is not supported with #MHD_USE_THREAD_PER_CONNECTION.
*/
static int restStart(IpVersion ipVersion, const char* httpsKey = NULL, const char* httpsCertificate = NULL)
{
  bool      mhdStartError  = true;
  size_t    memoryLimit    = connMemory * 1024; // Connection memory is expressed in kilobytes
  int       serverMode     = MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD;

  if (port == 0)
  {
    LM_X(1, ("Fatal Error (please call restInit before starting the REST service)"));
  }

  if (threadPoolSize != 0)
  {
    //
    // To use poll() instead of select(), MHD 0.9.48 has the define MHD_USE_EPOLL_LINUX_ONLY,
    // while in MHD 0.9.51, the name of the define has changed to MHD_USE_EPOLL.
    // So, to support both names, we need a ifdef/else cpp directive here.
    //
    // And, in some newer versions of microhttpd, MHD_USE_EPOLL is an enum and not a define,
    // so, an additional check in needed
    //
#if defined(MHD_USE_EPOLL) || MHD_VERSION >= 0x00095100
    serverMode = MHD_USE_SELECT_INTERNALLY | MHD_USE_EPOLL;
#else
    serverMode = MHD_USE_SELECT_INTERNALLY | MHD_USE_EPOLL_LINUX_ONLY;
#endif
  }

  //
  // Adding logging for MHD
  //
  serverMode |= MHD_USE_ERROR_LOG | MHD_USE_DEBUG;

  if ((ipVersion == IPV4) || (ipVersion == IPDUAL))
  {
    memset(&sad, 0, sizeof(sad));
    if (inet_pton(AF_INET, bindIp, &(sad.sin_addr.s_addr)) != 1)
    {
      LM_X(2, ("Fatal Error (V4 inet_pton fail for %s)", bindIp));
    }

    sad.sin_family = AF_INET;
    sad.sin_port   = htons(port);

    if ((httpsKey != NULL) && (httpsCertificate != NULL))
    {
      serverMode |= MHD_USE_SSL;
      if (threadPoolSize == 0)
      {
        mhdDaemon = MHD_start_daemon(serverMode,
                                     htons(port),
                                     NULL,
                                     NULL,
                                     connectionTreat,                     NULL,
                                     MHD_OPTION_HTTPS_MEM_KEY,            httpsKey,
                                     MHD_OPTION_HTTPS_MEM_CERT,           httpsCertificate,
                                     MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                     MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                     MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad,
                                     MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                     MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                     MHD_OPTION_END);
      }
      else
      {
        mhdDaemon = MHD_start_daemon(serverMode,
                                     htons(port),
                                     NULL,
                                     NULL,
                                     connectionTreat,                     NULL,
                                     MHD_OPTION_HTTPS_MEM_KEY,            httpsKey,
                                     MHD_OPTION_HTTPS_MEM_CERT,           httpsCertificate,
                                     MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                     MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                     MHD_OPTION_THREAD_POOL_SIZE,         threadPoolSize,
                                     MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad,
                                     MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                     MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                     MHD_OPTION_END);
      }
    }
    else
    {
      if (threadPoolSize == 0)
      {
        mhdDaemon = MHD_start_daemon(serverMode,
                                     htons(port),
                                     NULL,
                                     NULL,
                                     connectionTreat,                     NULL,
                                     MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                     MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                     MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad,
                                     MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                     MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                     MHD_OPTION_END);
      }
      else
      {
        mhdDaemon = MHD_start_daemon(serverMode,
                                     htons(port),
                                     NULL,
                                     NULL,
                                     connectionTreat,                     NULL,
                                     MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                     MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                     MHD_OPTION_THREAD_POOL_SIZE,         threadPoolSize,
                                     MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad,
                                     MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                     MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                     MHD_OPTION_END);
      }
    }

    if (mhdDaemon != NULL)
    {
      mhdStartError = false;
    }
  }

  if ((ipVersion == IPV6) || (ipVersion == IPDUAL))
  {
    memset(&sad_v6, 0, sizeof(sad_v6));
    if (inet_pton(AF_INET6, bindIPv6, &(sad_v6.sin6_addr.s6_addr)) != 1)
    {
      LM_X(4, ("Fatal Error (V6 inet_pton fail for %s)", bindIPv6));
    }

    sad_v6.sin6_family = AF_INET6;
    sad_v6.sin6_port = htons(port);

    serverMode |= MHD_USE_IPv6;

    if ((httpsKey != NULL) && (httpsCertificate != NULL))
    {
      serverMode |= MHD_USE_SSL;
      if (threadPoolSize == 0)
      {
        mhdDaemon_v6 = MHD_start_daemon(serverMode,
                                        htons(port),
                                        NULL,
                                        NULL,
                                        connectionTreat,                     NULL,
                                        MHD_OPTION_HTTPS_MEM_KEY,            httpsKey,
                                        MHD_OPTION_HTTPS_MEM_CERT,           httpsCertificate,
                                        MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                        MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                        MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad_v6,
                                        MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                        MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                        MHD_OPTION_END);
      }
      else
      {
        mhdDaemon_v6 = MHD_start_daemon(serverMode,
                                        htons(port),
                                        NULL,
                                        NULL,
                                        connectionTreat,                     NULL,
                                        MHD_OPTION_HTTPS_MEM_KEY,            httpsKey,
                                        MHD_OPTION_HTTPS_MEM_CERT,           httpsCertificate,
                                        MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                        MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                        MHD_OPTION_THREAD_POOL_SIZE,         threadPoolSize,
                                        MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad_v6,
                                        MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                        MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                        MHD_OPTION_END);
      }
    }
    else
    {
      if (threadPoolSize == 0)
      {
        mhdDaemon_v6 = MHD_start_daemon(serverMode,
                                        htons(port),
                                        NULL,
                                        NULL,
                                        connectionTreat,                     NULL,
                                        MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                        MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                        MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad_v6,
                                        MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                        MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                        MHD_OPTION_END);
      }
      else
      {
        mhdDaemon_v6 = MHD_start_daemon(serverMode,
                                        htons(port),
                                        NULL,
                                        NULL,
                                        connectionTreat,                     NULL,
                                        MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                        MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                        MHD_OPTION_THREAD_POOL_SIZE,         threadPoolSize,
                                        MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad_v6,
                                        MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                        MHD_OPTION_CONNECTION_TIMEOUT,       mhdConnectionTimeout,
                                        MHD_OPTION_END);
      }
    }

    if (mhdDaemon_v6 != NULL)
    {
      mhdStartError = false;
    }
  }


  if (mhdStartError == true)
  {
    LM_X(5, ("Fatal Error (error starting REST interface)"));
  }

  return 0;
}



/* ****************************************************************************
*
* restInit -
*
* FIXME P5: add vector of the accepted content-types, instead of the bool
*           See Issue #256
*/
void restInit
(
  RestService*        _getServiceV,
  RestService*        _putServiceV,
  RestService*        _postServiceV,
  RestService*        _patchServiceV,
  RestService*        _deleteServiceV,
  RestService*        _optionsServiceV,
  RestService*        _restBadVerbV,
  IpVersion           _ipVersion,
  const char*         _bindAddress,
  unsigned short      _port,
  bool                _multitenant,
  unsigned int        _connectionMemory,
  unsigned int        _maxConnections,
  unsigned int        _mhdThreadPoolSize,
  const char*         _corsOrigin,
  int                 _corsMaxAge,
  int                 _mhdTimeoutInSeconds,
  const char*         _httpsKey,
  const char*         _httpsCertificate
)
{
  const char* key  = _httpsKey;
  const char* cert = _httpsCertificate;

  serviceVectorsSet(_getServiceV, _putServiceV, _postServiceV, _patchServiceV, _deleteServiceV, _optionsServiceV, _restBadVerbV);

  port             = _port;
  ipVersionUsed    = _ipVersion;
  multitenant      = _multitenant;
  connMemory       = _connectionMemory;
  maxConns         = _maxConnections;
  threadPoolSize   = _mhdThreadPoolSize;
  corsMaxAge       = _corsMaxAge;

  mhdConnectionTimeout = _mhdTimeoutInSeconds;

  strncpy(corsOrigin, _corsOrigin, sizeof(corsOrigin) - 1);
  corsEnabled = (corsOrigin[0] != 0);

  strncpy(bindIp, LOCAL_IP_V4, MAX_LEN_IP - 1);
  strncpy(bindIPv6, LOCAL_IP_V6, MAX_LEN_IP - 1);

  if (isIPv6(std::string(_bindAddress)))
  {
    strncpy(bindIPv6, _bindAddress, MAX_LEN_IP - 1);
  }
  else
  {
    strncpy(bindIp, _bindAddress, MAX_LEN_IP - 1);
  }

  // Starting REST interface
  int r;
  if ((r = restStart(_ipVersion, key, cert)) != 0)
  {
    fprintf(stderr, "restStart: error %d\n", r);
    orionExitFunction(1, "restStart: error");
  }
}
