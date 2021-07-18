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
#include "common/wsStrip.h"
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

#include "orionld/common/orionldState.h"                         // orionldState, multitenancy, ...
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldTenantGet.h"                     // orionldTenantGet
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/rest/orionldMhdConnectionInit.h"               // orionldMhdConnectionInit
#include "orionld/rest/orionldMhdConnectionPayloadRead.h"        // orionldMhdConnectionPayloadRead
#include "orionld/rest/orionldMhdConnectionTreat.h"              // orionldMhdConnectionTreat
#include "orionld/serviceRoutines/orionldNotify.h"               // orionldNotify

#include "rest/Verb.h"
#include "rest/HttpHeaders.h"
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
  LM_TMP(("TPUT: %s %f", text, diffF));                  \
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
__thread char                    static_buffer[STATIC_BUFFER_SIZE + 1];
__thread char                    clientIp[IP_LENGTH_MAX + 1];
static unsigned int              connMemory;
static unsigned int              maxConns;
static unsigned int              threadPoolSize;
static unsigned int              mhdConnectionTimeout  = 0;
static int                       reqNo                 = 1;



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

    if (ciP->apiVersion == V2)
    {
      OrionError error(SccBadRequest, errorString);
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);

#ifdef ORIONLD
      orionldErrorResponseCreate(OrionldBadRequestData, "Empty right-hand-side for URI param", ckey);
#endif
    }
    else if (ciP->apiVersion == ADMIN_API)
    {
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = "{" + JSON_STR("error") + ":" + JSON_STR(errorString) + "}";

#ifdef ORIONLD
      orionldErrorResponseCreate(OrionldBadRequestData, "Error in URI param", errorString.c_str());
#endif
    }

    return MHD_YES;
  }

  std::string      key   = ckey;
  std::string      value = (val == NULL)? "" : val;

  if (key == URI_PARAM_PAGINATION_OFFSET)
  {
    char* cP = (char*) val;

    while (*cP != 0)
    {
      if ((*cP < '0') || (*cP > '9'))
      {
        OrionError error(SccBadRequest, std::string("Bad pagination offset: /") + value + "/ [must be a decimal number]");
        ciP->httpStatusCode = error.code;
        ciP->answer         = error.smartRender(ciP->apiVersion);

#ifdef ORIONLD
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /offset/", "must be an integer value >= 0");
#endif
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
        ciP->httpStatusCode = error.code;
        ciP->answer         = error.smartRender(ciP->apiVersion);

#ifdef ORIONLD
        LM_E(("Invalid value for URI parameter 'limit': '%s'", val));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value >= 1");
        orionldState.httpStatusCode = SccBadRequest;
#endif
        return MHD_YES;
      }

      ++cP;
    }

    int limit = atoi(val);
    if (limit > atoi(MAX_PAGINATION_LIMIT))
    {
      OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [max: " + MAX_PAGINATION_LIMIT + "]");
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);

#ifdef ORIONLD
        LM_E(("Invalid value for URI parameter 'limit': '%s'", val));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value <= 1000");
        orionldState.httpStatusCode = SccBadRequest;
#endif
      return MHD_YES;
    }
    else if (limit == 0)
    {
      if (orionldState.apiVersion != NGSI_LD_V1)
      {
        OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [a value of ZERO is unacceptable]");
        ciP->httpStatusCode = error.code;
        ciP->answer         = error.smartRender(ciP->apiVersion);
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
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);

#ifdef ORIONLD
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad value for /details/ - accepted: /on/, /ON/, /off/, /OFF/. Default is /off/", val);
#endif
      return MHD_YES;
    }
  }
  else if (key == URI_PARAM_ATTRIBUTES_FORMAT)
  {
    // If URI_PARAM_ATTRIBUTES_FORMAT used, set URI_PARAM_ATTRIBUTE_FORMAT as well
    ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] = value;
  }
  else if (key == URI_PARAM_ATTRIBUTE_FORMAT)
  {
    // If URI_PARAM_ATTRIBUTE_FORMAT used, set URI_PARAM_ATTRIBUTES_FORMAT as well
    ciP->uriParam[URI_PARAM_ATTRIBUTES_FORMAT] = value;
  }
  else if (key == URI_PARAM_OPTIONS)
  {
    ciP->uriParam[URI_PARAM_OPTIONS] = value;

    if (uriParamOptionsParse(ciP, val) != 0)
    {
      OrionError error(SccBadRequest, "Invalid value for URI param /options/");

      LM_W(("Bad Input (Invalid value for URI param /options/)"));
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);

#ifdef ORIONLD
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /options/", val);
#endif
    }
  }
  else if (key == URI_PARAM_TYPE)
  {
    ciP->uriParam[URI_PARAM_TYPE] = value;

    if (strstr(val, ","))  // More than ONE type?
    {
      uriParamTypesParse(ciP, val);
    }
    else
    {
      ciP->uriParamTypes.push_back(val);
    }
  }
#ifdef ORIONLD
  else if (key == URI_PARAM_PRETTY_PRINT)
  {
    if (strcmp(val, "yes") == 0)
    {
      orionldState.uriParams.prettyPrint = true;
    }
  }
  else if (key == URI_PARAM_SPACES)
  {
    orionldState.uriParams.spaces = atoi(val);
  }
#endif
  else if ((key != URI_PARAM_Q)       &&
           (key != URI_PARAM_MQ)      &&
           (key != URI_PARAM_LEVEL))  // FIXME P1: possibly more known options here ...
  {
    LM_T(LmtUriParams, ("Received unrecognized URI parameter: '%s'", key.c_str()));
  }

  if (val != NULL)
  {
    ciP->uriParam[key] = value;
  }
  else
  {
    ciP->uriParam[key] = "SET";
  }

  LM_T(LmtUriParams, ("URI parameter:   %s: %s", key.c_str(), ciP->uriParam[key].c_str()));

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

#ifdef ORIONLD
    orionldErrorResponseCreate(OrionldBadRequestData, "found a forbidden character in URI param", key.c_str());
#endif
    alarmMgr.badInput(clientIp, details);
    ciP->httpStatusCode = error.code;
    ciP->answer         = error.smartRender(ciP->apiVersion);
  }

  return MHD_YES;
}



/* ****************************************************************************
*
* mimeTypeSelect -
*/
static MimeType mimeTypeSelect(ConnectionInfo* ciP)
{
  if (ciP->httpHeaders.accepted("application/json"))
  {
    return JSON;
  }

  if (ciP->httpHeaders.accepted("text/plain"))
  {
    return TEXT;
  }

  return JSON;
}



/* ****************************************************************************
*
* acceptItemParse -
*/
static bool acceptItemParse(ConnectionInfo* ciP, char* value)
{
  HttpHeaders*       headerP        = &ciP->httpHeaders;
  char*              rest           = NULL;
  char*              cP             = (char*) value;
  HttpAcceptHeader*  acceptHeaderP;
  char*              delimiter;

  LM_T(LmtHttpHeaders, ("Initial value of Accept header: %s", value));

  if (value[0] == 0)
  {
    // NOTE
    //   According to https://www.w3.org/Protocols/rfc2616/rfc2616-sec2.html#sec2, empty
    //   items in the comma list of Accepot are allowed, so we simply return OK (true) here
    //   and skip to the next item.
    //
    return true;
  }

  if ((delimiter = strchr(cP, ';')) != NULL)
  {
    *delimiter = 0;
    rest = &delimiter[1];
  }

  //
  // Now we have the 'media-range'.
  // The broker accepts only the following two media types:
  //   - application/json
  //   - text/plain
  //   - application/ld+json (if compiled for orionld)
  //
  // So, if the media-range is anything else, it is rejected immediately and not put in the list
  //
  if ((strcmp(cP, "*/*")                  != 0) &&
      (strcmp(cP, "application/*")        != 0) &&
#ifdef ORIONLD
      (strcmp(cP, "application/ld+json")  != 0) &&
      (strcmp(cP, "application/geo+json") != 0) &&
#endif
      (strcmp(cP, "application/json")     != 0) &&
      (strcmp(cP, "text/*")               != 0) &&
      (strcmp(cP, "text/plain")           != 0))
  {
    return true;  // No error, just a media type that the broker doesn't recognize
  }


  acceptHeaderP = new HttpAcceptHeader();
  acceptHeaderP->mediaRange = cP;
  acceptHeaderP->qvalue     = 1;  // default value of 'q' - may be modified later

  // If nothing after the media-range, we are done
  if (rest == NULL)
  {
    headerP->acceptHeaderV.push_back(acceptHeaderP);
    return true;
  }

  // If we get here, next in line must be a 'q', perhaps preceded by whitespace
  while ((*rest == ' ') || (*rest == '\t'))
  {
    ++rest;
  }

  if (*rest == 0)
  {
    headerP->acceptHeaderV.push_back(acceptHeaderP);
    return true;
  }


  //
  // Next item is q=qvalue
  //

  if (*rest != 'q')
  {
    ciP->acceptHeaderError = "q missing in accept header";
    ciP->httpStatusCode    = SccBadRequest;
    delete acceptHeaderP;
    return false;
  }

  // Pass 'q' and check for '='
  ++rest;
  if (*rest != '=')
  {
    ciP->acceptHeaderError = "missing equal-sign after q in accept header";
    ciP->httpStatusCode    = SccBadRequest;
    delete acceptHeaderP;
    return false;
  }

  // Pass '=' and check for Number
  ++rest;
  // Zero-out ';' if present
  if ((cP = strchr(rest, ';')) != NULL)
  {
    *cP = 0;
  }

  // qvalue there?
  if (*rest == 0)
  {
    ciP->acceptHeaderError = "qvalue in accept header is missing";
    ciP->httpStatusCode    = SccBadRequest;
    delete acceptHeaderP;
    return false;
  }

  // qvalue a number?
  if (str2double(rest, &acceptHeaderP->qvalue) == false)
  {
    ciP->acceptHeaderError = "qvalue in accept header is not a number";
    ciP->httpStatusCode    = SccBadRequest;
    ciP->outMimeType       = mimeTypeSelect(ciP);
    delete acceptHeaderP;
    return false;
  }


  //
  // And now the accept-extensions  (which is ignored for now)
  // FIXME P4: Implement treatment of accept-extensions
  //

  // push the accept header and return true
  headerP->acceptHeaderV.push_back(acceptHeaderP);

  return true;
}



/* ****************************************************************************
*
* acceptParse -
*/
static void acceptParse(ConnectionInfo* ciP, const char* value)
{
  char*         itemStart  = (char*) value;
  char*         cP         = (char*) value;

  if (value[0] == 0)
  {
    ciP->acceptHeaderError = "empty accept header";
    ciP->httpStatusCode    = SccBadRequest;
    return;
  }

  while (*cP != 0)
  {
    if (*cP != ',')
    {
      ++cP;
    }
    else
    {
      *cP = 0;

      // step over the comma
      ++cP;

      // step over initial whitespace
      while ((*cP == ' ') || (*cP == '\t'))
      {
        ++cP;
      }

      acceptItemParse(ciP, itemStart);
      itemStart = cP;
    }
  }

  acceptItemParse(ciP, itemStart);

  if ((ciP->httpHeaders.acceptHeaderV.size() == 0) && (ciP->acceptHeaderError == ""))
  {
    ciP->httpStatusCode    = SccNotAcceptable;
    ciP->acceptHeaderError = "no acceptable mime-type in accept header";
  }

  if ((ciP->httpStatusCode == SccNotAcceptable) || (ciP->httpStatusCode == SccBadRequest))
  {
    OrionError oe(ciP->httpStatusCode, ciP->acceptHeaderError);

    ciP->answer = oe.smartRender(ciP->apiVersion);
  }
}



/* ****************************************************************************
*
* httpHeaderGet -
*/
MHD_Result httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  ConnectionInfo*  ciP     = (ConnectionInfo*) cbDataP;
  HttpHeaders*     headerP = &ciP->httpHeaders;

  if      (strcasecmp(key, HTTP_USER_AGENT) == 0)        headerP->userAgent      = value;
  else if (strcasecmp(key, HTTP_HOST) == 0)              headerP->host           = value;
  else if (strcasecmp(key, HTTP_ACCEPT) == 0)
  {
    headerP->accept = value;
    acceptParse(ciP, value);  // Any errors are flagged in ciP->acceptHeaderError and taken care of later
  }
  else if (strcasecmp(key, HTTP_EXPECT) == 0)            headerP->expect         = value;
  else if (strcasecmp(key, HTTP_CONNECTION) == 0)        headerP->connection     = value;
  else if (strcasecmp(key, HTTP_CONTENT_TYPE) == 0)
  {
    headerP->contentType = value;

    if (strcmp(value, "application/ld+json") == 0)
      orionldState.ngsildContent = true;
  }
  else if (strcasecmp(key, HTTP_CONTENT_LENGTH) == 0)    headerP->contentLength  = atoi(value);
  else if (strcasecmp(key, HTTP_ORIGIN) == 0)            headerP->origin         = value;
  else if ((strcasecmp(key, HTTP_FIWARE_SERVICE) == 0) || (strcasecmp(key, "NGSILD-Tenant") == 0))
  {
    if (multitenancy == true)  // Has the broker been started with multi-tenancy enabled (it's disabled by default)
    {
      toLowercase((char*) value);
      orionldState.tenantName = (char*) value;
    }
    else
    {
      // Tenant used when tenant is not supported by the broker
      LM_E(("tenant in use but tenant support is not enable for the broker"));
      orionldState.httpStatusCode = 400;
      orionldErrorResponseCreate(OrionldBadRequestData, "Tenants not supported", "tenant in use but tenant support is not enable for the broker");
    }
    LM_TMP(("TENANT: orionldState.tenantName == %s", orionldState.tenantName));
  }
  else if (strcasecmp(key, "NGSILD-Path") == 0)
    orionldState.servicePath = (char*) value;
  else if (strcasecmp(key, HTTP_X_AUTH_TOKEN) == 0)
  {
    orionldState.xauthHeader    = (char*) value;
    headerP->xauthToken         = value;
  }
  else if (strcasecmp(key, HTTP_X_REAL_IP) == 0)           headerP->xrealIp            = value;
  else if (strcasecmp(key, HTTP_X_FORWARDED_FOR) == 0)     headerP->xforwardedFor      = value;
  else if (strcasecmp(key, HTTP_FIWARE_CORRELATOR) == 0)   headerP->correlator         = value;
  else if (strcasecmp(key, HTTP_NGSIV2_ATTRSFORMAT) == 0)  headerP->ngsiv2AttrsFormat  = value;
  else if (strcasecmp(key, HTTP_FIWARE_SERVICEPATH) == 0)
  {
    headerP->servicePath         = value;
    headerP->servicePathReceived = true;
#ifdef ORIONLD
    orionldState.servicePath = (char*) value;
#endif
  }
#ifdef ORIONLD
  else if (strcasecmp(key, HTTP_LINK) == 0)
  {
    orionldState.link                  = (char*) value;
    orionldState.linkHttpHeaderPresent = true;
  }
  else if (strcasecmp(key, "Prefer") == 0)
  {
    orionldState.preferHeader = (char*) value;
  }
#endif
  else
  {
    LM_T(LmtHttpUnsupportedHeader, ("'unsupported' HTTP header: '%s', value '%s'", key, value));
  }

  if ((strcasecmp(key, "connection") == 0) && (headerP->connection != "") && (headerP->connection != "close"))
  {
    LM_T(LmtRest, ("connection '%s' - currently not supported, sorry ...", headerP->connection.c_str()));
  }

  /* Note that the strategy to "fix" the Content-Type is to replace the ";" with 0
   * to "deactivate" this part of the string in the checking done at connectionTreat() */
  char* cP = (char*) headerP->contentType.c_str();
  char* match;
  if ((match = strstr(cP, ";")) != NULL)
  {
     *match = 0;
     headerP->contentType = cP;
  }

  headerP->gotHeaders = true;

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
  const char*      spath    = (ciP->servicePathV.size() > 0)? ciP->servicePathV[0].c_str() : "";
  struct timespec  reqEndTime;

  if (orionldState.notify == true)
  {
    PERFORMANCE(notifStart);
    orionldNotify();
    PERFORMANCE(notifEnd);
  }

  if ((ciP->payload != NULL) && (ciP->payload != static_buffer))
  {
    free(ciP->payload);
    ciP->payload = NULL;
  }


  lmTransactionEnd();  // Incoming REST request ends

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &reqEndTime);
    clock_difftime(&reqEndTime, &ciP->reqStartTime, &threadLastTimeStat.reqTime);
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
  if (metricsMgr.isOn())
    metricsMgr.add(orionldState.tenantP->tenant, spath, METRIC_TRANS_IN, 1);

  //
  // If the httpStatusCode is above the set of 200s, an error has occurred
  //
  if (ciP->httpStatusCode >= SccBadRequest)
  {
    if (metricsMgr.isOn())
      metricsMgr.add(orionldState.tenantP->tenant, spath, METRIC_TRANS_IN_ERRORS, 1);
  }

  if (metricsMgr.isOn() && (ciP->transactionStart.tv_sec != 0))
  {
    struct timeval  end;

    if (gettimeofday(&end, NULL) == 0)
    {
      unsigned long long elapsed =
        (end.tv_sec  - ciP->transactionStart.tv_sec) * 1000000 +
        (end.tv_usec - ciP->transactionStart.tv_usec);

      metricsMgr.add(orionldState.tenantP->tenant, spath, _METRIC_TOTAL_SERVICE_TIME, elapsed);
    }
  }


  //
  // delayed release of ContextElementResponseVector must be effectuated now.
  // See github issue #2994
  //
  extern void delayedReleaseExecute(void);
  delayedReleaseExecute();

  delete(ciP);

#ifdef ORIONLD
  kaBufferReset(&orionldState.kalloc, false);  // 'false': it's reused, but in a different thread ...

  if ((orionldState.responseTree != NULL) && (orionldState.kjsonP == NULL))
    kjFree(orionldState.responseTree);
#endif

  *con_cls = NULL;

#ifdef REQUEST_PERFORMANCE
  PERFORMANCE(reqEnd);

  TIME_REPORT(timestamps.reqStart,            timestamps.serviceRoutineStart,    "Before Service Routine:    ");
  TIME_REPORT(timestamps.serviceRoutineStart, timestamps.serviceRoutineEnd,      "During Service Routine:    ");
  TIME_REPORT(timestamps.serviceRoutineEnd,   timestamps.reqEnd,                 "After Service Routine:     ");
  TIME_REPORT(timestamps.parseStart,          timestamps.parseEnd,               "Payload Parse:             ");
  TIME_REPORT(timestamps.renderStart,         timestamps.renderEnd,              "Rendering Response:        ");
  TIME_REPORT(timestamps.restReplyStart,      timestamps.restReplyEnd,           "Sending Response:          ");
  TIME_REPORT(timestamps.forwardStart,        timestamps.forwardEnd,             "Forwarding:                ");
  TIME_REPORT(timestamps.forwardDbStart,      timestamps.forwardDbEnd,           "DB Query for Forwarding:   ");
  TIME_REPORT(timestamps.reqStart,            timestamps.reqEnd,                 "Entire request:            ");
  TIME_REPORT(timestamps.troeStart,           timestamps.troeEnd,                "TRoE Processing:           ");
  TIME_REPORT(timestamps.requestPartEnd,      timestamps.requestCompletedStart,  "MHD Delay (send response): ");

  TIME_REPORT(timestamps.dbStart,             timestamps.dbEnd,                  "Awaiting DB:               ");
  TIME_REPORT(timestamps.mongoBackendStart,   timestamps.mongoBackendEnd,        "Awaiting MongoBackend:     ");

  for (int ix = 0; ix < 50; ix++)
  {
    if (timestamps.srDesc[ix] != NULL)
      TIME_REPORT(timestamps.srStart[ix], timestamps.srEnd[ix], timestamps.srDesc[ix]);
  }

  struct timespec  all;
  struct timespec  mongo;
  float            mongoF;
  float            allF;

  kTimeDiff(&timestamps.reqStart, &timestamps.reqEnd, &all,   &allF);
  kTimeDiff(&timestamps.dbStart,  &timestamps.dbEnd,  &mongo, &mongoF);
  LM_TMP(("TPUT: Entire request - DB:        %f", allF - mongoF));  // Only for REQUEST_PERFORMANCE
  LM_TMP(("TPUT: mongoConnect Accumulated:   %f (%d calls)", timestamps.mongoConnectAccumulated, timestamps.getMongoConnectionCalls));
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


  if (ciP->httpHeaders.servicePathReceived == false)
  {
    return 0;
  }

  if (servicePath[0] == 0)
  {
    // Special case, corresponding to default service path
    return 0;
  }


  if (servicePath[0] != '/')
  {
    OrionError oe(SccBadRequest, "Only /absolute/ Service Paths allowed [a service path must begin with /]");
    ciP->answer = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
    return 1;
  }

  components = stringSplit(servicePath, '/', compV);

  if (components > SERVICE_PATH_MAX_LEVELS)
  {
    OrionError oe(SccBadRequest, "too many components in ServicePath");
    ciP->answer = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
    return 2;
  }

  for (int ix = 0; ix < components; ++ix)
  {
    if (strlen(compV[ix].c_str()) > SERVICE_PATH_MAX_COMPONENT_LEN)
    {
      OrionError oe(SccBadRequest, "component-name too long in ServicePath");
      ciP->answer = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
      return 3;
    }

    if (compV[ix].c_str()[0] == 0)
    {
      OrionError oe(SccBadRequest, "empty component in ServicePath");
      ciP->answer = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
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
        ciP->answer = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
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
static char* removeTrailingSlash(std::string path)
{
  char* cpath = (char*) path.c_str();

  /* strlen(cpath) > 1 ensures that root service path "/" is not touched */
  while ((strlen(cpath) > 1) && (cpath[strlen(cpath) - 1] == '/'))
  {
    cpath[strlen(cpath) - 1] = 0;
  }

  return cpath;
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
bool isOriginAllowedForCORS(const std::string& requestOrigin)
{
  return ((requestOrigin != "") && ((strcmp(corsOrigin, "__ALL") == 0) || (strcmp(requestOrigin.c_str(), corsOrigin) == 0)));
}



/* ****************************************************************************
*
* servicePathSplit -
*/
int servicePathSplit(ConnectionInfo* ciP)
{
#if 0
  //
  // Special case: empty service-path
  //
  // FIXME P4: We're not sure what this 'fix' really fixes.
  //           Must implement a functest to reproduce this situation.
  //           And, if that is not possible, just remove the whole thing
  //
  if ((ciP->httpHeaders.servicePathReceived == true) && (ciP->httpHeaders.servicePath == ""))
  {
    OrionError e(SccBadRequest, "empty service path");
    ciP->answer = e.render();
    alarmMgr.badInput(clientIp, "empty service path");
    return -1;
  }
#endif
  char* servicePathCopy = NULL;
  int   servicePaths    = 0;

  if (ciP->httpHeaders.servicePath != "")
  {
    servicePathCopy = strdup(ciP->httpHeaders.servicePath.c_str());
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
    std::string stripped = std::string(wsStrip((char*) ciP->servicePathV[ix].c_str()));

    ciP->servicePathV[ix] = removeTrailingSlash(stripped);

    //
    // This was previously an LM_T trace, but we have "promoted" it to INFO due to
    // it is needed to check logs in a .test case (case 0392 service_path_http_header.test)
    //
    LM_K(("Service Path %d: '%s'", ix, ciP->servicePathV[ix].c_str()));
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
  if (ciP->httpHeaders.contentLength == 0)
  {
    return 0;
  }


  // Case 2
  if (ciP->httpHeaders.contentType == "")
  {
    std::string details = "Content-Type header not used, default application/octet-stream is not supported";
    ciP->httpStatusCode = SccUnsupportedMediaType;
    restErrorReplyGet(ciP, SccUnsupportedMediaType, details, &ciP->answer);
    ciP->httpStatusCode = SccUnsupportedMediaType;

    return 1;
  }

  // Case 3
  if ((ciP->apiVersion == V1) && (ciP->httpHeaders.contentType != "application/json"))
  {
    std::string details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;
    ciP->httpStatusCode = SccUnsupportedMediaType;
    restErrorReplyGet(ciP, SccUnsupportedMediaType, details, &ciP->answer);
    ciP->httpStatusCode = SccUnsupportedMediaType;
    return 1;
  }


  // Case 4
  if ((ciP->apiVersion == V2) && (ciP->httpHeaders.contentType != "application/json") && (ciP->httpHeaders.contentType != "text/plain"))
  {
    std::string details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;
    ciP->httpStatusCode = SccUnsupportedMediaType;
    restErrorReplyGet(ciP, SccUnsupportedMediaType, details, &ciP->answer);
    ciP->httpStatusCode = SccUnsupportedMediaType;
    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* urlCheck - check for forbidden characters and remove trailing slashes
*
* Returns 'true' if the URL is OK, 'false' otherwise.
* ciP->answer and ciP->httpStatusCode are set if an error is encountered.
*
*/
bool urlCheck(ConnectionInfo* ciP, const std::string& url)
{
  if (forbiddenChars(url.c_str()) == true)
  {
    OrionError error(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI);
    ciP->httpStatusCode = error.code;
    ciP->answer         = error.smartRender(ciP->apiVersion);
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
*  V2:         for URLs in the /v2 path
*  V1:         for URLs in the /v1 or with an equivalence (e.g. /ngi10, /log, etc.)
*  ADMIN_API:  admin operations without /v1 alias
*  NO_VERSION: others (invalid paths)
*
*/
static ApiVersion apiVersionGet(const char* path)
{
  if ((path[1] == 'v') && (path[2] == '2'))
  {
    return V2;
  }

  // Unlike v2, v1 is case-insensitive (see case/2057 test)
  if (((path[1] == 'v') || (path[1] == 'V')) && (path[2] == '1'))
  {
    return V1;
  }

  if ((strncasecmp("/ngsi9",      path, strlen("/ngsi9"))      == 0)  ||
      (strncasecmp("/ngsi10",     path, strlen("/ngsi10"))     == 0))
  {
    return V1;
  }

  if ((strncasecmp("/log",        path, strlen("/log"))        == 0)  ||
      (strncasecmp("/cache",      path, strlen("/cache"))      == 0)  ||
      (strncasecmp("/statistics", path, strlen("/statistics")) == 0))
  {
    return V1;
  }

  if ((strncmp("/admin",   path, strlen("/admin"))   == 0) ||
      (strncmp("/version", path, strlen("/version")) == 0))
  {
    return ADMIN_API;
  }

  return NO_VERSION;
}



/* ****************************************************************************
*
* acceptHeadersAcceptable -
*
* URI paths ending with '/value' accept both text/plain and application/json.
* All other requests accept only application/json.
*
* This function just checks that the media types flagged as accepted by the client
* are OK to work with for the broker.
* The media type to be used is selected later, depending on the request.
* Actually, all requests except those ending in '/value' will use application/json.
*
*/
static bool acceptHeadersAcceptable(ConnectionInfo* ciP, bool* textAcceptedP)
{
  char* eopath = (char*) ciP->url.c_str();
  int   urllen = strlen(eopath);

  if (urllen > 6)
  {
    eopath = &eopath[urllen - 6];  // to point to '/value' if present
  }

  if (strcmp(eopath, "/value") == 0)
  {
    *textAcceptedP = true;
  }


  //
  // Go over vector with accepted mime types
  //
  for (unsigned int ix = 0; ix < ciP->httpHeaders.acceptHeaderV.size(); ++ix)
  {
    HttpAcceptHeader* haP = ciP->httpHeaders.acceptHeaderV[ix];

    if (haP->mediaRange == "*/*")
    {
      return true;
    }

    if (haP->mediaRange == "application/*")
    {
      return true;
    }

    if (haP->mediaRange == "application/json")
    {
      return true;
    }

    if (*textAcceptedP == true)
    {
      if (haP->mediaRange == "text/*")
      {
        return true;
      }

      if (haP->mediaRange == "text/plain")
      {
        return true;
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* restServiceForBadVerb - dummy instance
*/
RestService restServiceForBadVerb;



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

  //
  // Setting crucial fields of orionldState - those that are used for non-ngsi-ld requests
  //
  kTimeGet(&orionldState.timestamp);

  orionldState.requestTime  = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;
  orionldState.responseTree = NULL;
  orionldState.notify       = false;

  *retValP = MHD_YES;  // Only MHD_NO if allocation of ConnectionInfo fails

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
    snprintf(clientIp, sizeof(clientIp), "%s", ip);
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


  //
  // ConnectionInfo
  //
  // FIXME P1: ConnectionInfo could be a thread variable (like the static_buffer),
  // as long as it is properly cleaned up between calls.
  // We would save the call to new/free for each and every request.
  // Once we *really* look to scratch some efficiency, this change should be made.
  //
  // Also, is ciP->ip really used?
  //
  if ((ciP = new ConnectionInfo(url, method, version, connection)) == NULL)
  {
    LM_E(("Runtime Error (error allocating ConnectionInfo)"));
    // No METRICS here ... Without ConnectionInfo we have no service/subService ...
    *retValP = MHD_NO;
    return NULL;
  }

  //
  // Get API version
  //   Note that we need to get API version before MHD_get_connection_values() as the later
  //   function may result in an error after processing Accept headers (and the
  //   render for the error depends on API version)
  //
  ciP->apiVersion = apiVersionGet(ciP->url.c_str());


  // LM_K(("--------------------- Serving APIv%d request %s %s -----------------", ciP->apiVersion, method, url));

  ciP->transactionStart.tv_sec  = transactionStart.tv_sec;
  ciP->transactionStart.tv_usec = transactionStart.tv_usec;

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &ciP->reqStartTime);
  }

  // WARNING: This log message below is crucial for the correct function of the Behave tests - CANNOT BE REMOVED
  LM_T(LmtRequest, ("--------------------- Serving request %s %s -----------------", method, url));
  ciP->port    = port;
  ciP->ip      = ip;
  ciP->callNo  = reqNo;

  ++reqNo;


  //
  // URI parameters
  //
  // FIXME P1: We might not want to do all these assignments, they are not used in all requests ...
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  ciP->uriParam[URI_PARAM_PAGINATION_OFFSET]  = DEFAULT_PAGINATION_OFFSET;
  ciP->uriParam[URI_PARAM_PAGINATION_LIMIT]   = DEFAULT_PAGINATION_LIMIT;
  ciP->uriParam[URI_PARAM_PAGINATION_DETAILS] = DEFAULT_PAGINATION_DETAILS;

  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);

  if (ciP->httpHeaders.accept == "")  // No Accept: given, treated as */*
  {
    ciP->httpHeaders.accept = "*/*";
    acceptParse(ciP, "*/*");
  }

  char correlator[CORRELATOR_ID_SIZE + 1];
  if (ciP->httpHeaders.correlator == "")
  {
    correlatorGenerate(correlator);
    ciP->httpHeaders.correlator = correlator;
  }

  correlatorIdSet(ciP->httpHeaders.correlator.c_str());

  ciP->httpHeader.push_back(HTTP_FIWARE_CORRELATOR);
  ciP->httpHeaderValue.push_back(ciP->httpHeaders.correlator);

  if ((ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE) && (ciP->apiVersion == V2))
  {
    char details[256];
    snprintf(details, sizeof(details), "payload size: %d, max size supported: %d", ciP->httpHeaders.contentLength, PAYLOAD_MAX_SIZE);

    alarmMgr.badInput(clientIp, details);
    OrionError oe(SccRequestEntityTooLarge, details);

    ciP->httpStatusCode = oe.code;
    ciP->answer = oe.smartRender(ciP->apiVersion);

    //
    // FIXME P4:
    // Supposedly, we aren't ready to respond to the HTTP request at this early stage, before reading the content.
    // However, tests have shown that the broker hangs if the response is delayed until later calls ...
    //
    //
    restReply(ciP, ciP->answer);  // to not hang on too big payloads
    return ciP;
  }


  //
  // Transaction starts here
  //
  lmTransactionStart("from", "", ip, port, url);  // Incoming REST request starts

  /* X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides ip.
     X-Real-IP takes preference over X-Forwarded-For, if both appear */
  if (ciP->httpHeaders.xrealIp != "")
  {
    lmTransactionSetFrom(ciP->httpHeaders.xrealIp.c_str());
  }
  else if (ciP->httpHeaders.xforwardedFor != "")
  {
    lmTransactionSetFrom(ciP->httpHeaders.xforwardedFor.c_str());
  }
  else
  {
    lmTransactionSetFrom(ip);
  }

  char tenant[DB_AND_SERVICE_NAME_MAX_LEN];

  if (orionldState.tenantP->tenant != NULL)
    ciP->tenantFromHttpHeader = strToLower(tenant, orionldState.tenantP->tenant, sizeof(tenant));

  ciP->outMimeType          = mimeTypeSelect(ciP);

  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

  // Lookup Rest Service
  bool badVerb = false;

  ciP->restServiceP = restServiceLookup(ciP, &badVerb);

  if (urlCheck(ciP, ciP->url) == false)
  {
    alarmMgr.badInput(clientIp, "error in URI path");
  }
  else if (servicePathSplit(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "error in ServicePath http-header");
  }
  else if (contentTypeCheck(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "invalid mime-type in Content-Type http-header");
  }
  //
  // Requests of verb POST, PUT or PATCH are considered erroneous if no payload is present - with the exception of log requests.
  //
  else if ((ciP->httpHeaders.contentLength == 0) &&
      ((ciP->verb == POST) || (ciP->verb == PUT) || (ciP->verb == PATCH )) &&
      (strncasecmp(ciP->url.c_str(), "/log/", 5) != 0) &&
      (strncasecmp(ciP->url.c_str(), "/admin/log", 10) != 0))
  {
    std::string errorMsg;

    restErrorReplyGet(ciP, SccContentLengthRequired, "Zero/No Content-Length in PUT/POST/PATCH request", &errorMsg);
    ciP->httpStatusCode  = SccContentLengthRequired;
    restReply(ciP, errorMsg);  // OK to respond as no payload
    alarmMgr.badInput(clientIp, errorMsg);

    return ciP;
  }
  else if (ciP->badVerb == true)
  {
    std::vector<std::string> compV;

    // Not ready to answer here - must wait until all payload has been read
    ciP->httpStatusCode = SccBadVerb;
  }
  else
  {
    ciP->inMimeType = mimeTypeParse(ciP->httpHeaders.contentType, NULL);

    if (ciP->httpStatusCode != SccOk)
    {
      alarmMgr.badInput(clientIp, "error in URI parameters");
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
  // If the HTTP header says the request is bigger than our PAYLOAD_MAX_SIZE,
  // just silently "eat" the entire message.
  //
  // The problem occurs when the broker is lied to and there aren't ciP->httpHeaders.contentLength
  // bytes to read.
  // When this happens, MHD blocks until it times out (MHD_OPTION_CONNECTION_TIMEOUT defaults to 5 seconds),
  // and the broker isn't able to respond. MHD just closes the connection.
  // Question asked in mhd mailing list.
  //
  // See github issue:
  //   https://github.com/telefonicaid/fiware-orion/issues/2761
  //
  if (ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE)
  {
    //
    // Errors can't be returned yet, postpone ...
    //
    *upload_data_size = 0;
    return MHD_YES;
  }

  //
  // First call with payload - use the thread variable "static_buffer" if possible,
  // otherwise allocate a bigger buffer
  //
  // FIXME P1: This could be done in "Part I" instead, saving an "if" for each "Part II" call
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  if (ciP->payloadSize == 0)  // First call with payload
  {
    if (ciP->httpHeaders.contentLength > STATIC_BUFFER_SIZE)
    {
      ciP->payload = (char*) malloc(ciP->httpHeaders.contentLength + 1);
    }
    else
    {
      ciP->payload = static_buffer;
    }
  }

  // Copy the chunk
  LM_T(LmtPartialPayload, ("Got %d of payload of %d bytes", dataLen, ciP->httpHeaders.contentLength));
  memcpy(&ciP->payload[ciP->payloadSize], upload_data, dataLen);

  // Add to the size of the accumulated read buffer
  ciP->payloadSize += *upload_data_size;

  // Zero-terminate the payload
  ciP->payload[ciP->payloadSize] = 0;

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
#ifdef ORIONLD
  //
  // NGSI-LD requests implement a different URL parsing algorithm, a different payload parse algorithm, etc.
  // A complete new set of functions are used for NGSI-LD, so ...
  //
  if (url[5] == '-')  // URL indicates an /ngsi-ld request
  {
    //
    // Seems like an NGSI-LD request, but, let's make sure
    //
    if ((url[0] == '/') && (url[1] == 'n') && (url[2] == 'g') && (url[3] == 's') && (url[4] == 'i') && (url[5] == '-') && (url[6] == 'l') && (url[7] == 'd') && (url[8] == '/'))
    {
      orionldState.apiVersion = NGSI_LD_V1;

      if (*con_cls == NULL)
      {
#ifdef REQUEST_PERFORMANCE
        bzero(&timestamps, sizeof(timestamps));
        kTimeGet(&timestamps.reqStart);
#endif
        return orionldMhdConnectionInit(connection, url, method, version, con_cls);
      }
      else if (*upload_data_size != 0)
        return orionldMhdConnectionPayloadRead((ConnectionInfo*) *con_cls, upload_data_size, upload_data);

      // else ...
      //
      // The entire message has been read, we're allowed to respond.
      //
      // If any error has been encountered during stage I and II (init + payload-read), then ciP->httpStatusCode has been set to != SccOk (200)
      // and, optionally a payload tree hangs under ciP->response.
      // No need to call the stage III function if this is the case.
      //

      // Mark the request as "finished", by setting upload_data_size to 0
      *upload_data_size = 0;

      // Then treat the request
      return orionldMhdConnectionTreat((ConnectionInfo*) *con_cls);
    }
  }
#endif

  // 1. First call - setup ConnectionInfo and get/check HTTP headers
  if (*con_cls == NULL)
  {
    MHD_Result retVal;
    *con_cls = connectionTreatInit(connection, url, method, version, &retVal);
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
  // from orionldMhdConnectionTreat(). Had to be a little simplified though ...
  //
  LM_TMP(("TENANT: '%s'", orionldState.tenantName));
  if (orionldState.tenantName != NULL)
    orionldState.tenantP = orionldTenantGet(orionldState.tenantName);
  else
    orionldState.tenantP = &tenant0;

  LM_TMP(("TENANT: '%s', at %p", orionldState.tenantP->tenant, orionldState.tenantP));

  lmTransactionSetSubservice(ciP->httpHeaders.servicePath.c_str());

  if ((ciP->httpStatusCode != SccOk) && (ciP->httpStatusCode != SccBadVerb))
  {
    // An error has occurred. Here we are ready to respond, as all data has been read
    // However, if badVerb, then the service routine needs to execute to add the "Allow" HTTP header
    restReply(ciP, ciP->answer);
    return MHD_YES;
  }

  //
  // If the incoming request was too big, return error about it
  //
  if (ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE)
  {
    char details[256];

    snprintf(details, sizeof(details), "payload size: %d, max size supported: %d", ciP->httpHeaders.contentLength, PAYLOAD_MAX_SIZE);
    alarmMgr.badInput(clientIp, details);
    restErrorReplyGet(ciP, SccRequestEntityTooLarge, details, &ciP->answer);

    ciP->httpStatusCode = SccRequestEntityTooLarge;
  }


  //
  // Check for error during Accept Header parsing
  //
  if (ciP->acceptHeaderError != "")
  {
    OrionError   oe(SccBadRequest, ciP->acceptHeaderError);

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, ciP->acceptHeaderError);
    restReply(ciP, oe.smartRender(ciP->apiVersion));
    return MHD_YES;
  }


  //
  // Check that Accept Header values are valid
  //
  bool textAccepted = false;

  if (!acceptHeadersAcceptable(ciP, &textAccepted))
  {
    OrionError oe(SccNotAcceptable, "acceptable MIME types: application/json, text/plain");

    if (!textAccepted)
    {
      oe.details = "acceptable MIME types: application/json";
    }

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, oe.details);
    restReply(ciP, oe.smartRender(ciP->apiVersion));
    return MHD_YES;
  }


  //
  // Note that ciP->outMimeType is not set here.
  // Why?
  // If text/plain is asked for and accepted ('*/value' operations) but something goes wrong,
  // then application/json is used for the error
  // If all goes well, the proper service routine will set ciP->outMimeType to text/plain
  //
  if (ciP->httpHeaders.outformatSelect() == NOMIMETYPE)
  {
    OrionError oe(SccNotAcceptable, "acceptable MIME types: application/json, text/plain");

    if (!textAccepted)
    {
      oe.details = "acceptable MIME types: application/json";
    }

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, oe.details);
    restReply(ciP, oe.smartRender(ciP->apiVersion));
    return MHD_YES;
  }


  //
  // Check Content-Type and Content-Length for GET/DELETE requests
  //
  if ((ciP->httpHeaders.contentType != "") && (ciP->httpHeaders.contentLength == 0) && ((ciP->verb == GET) || (ciP->verb == DELETE)))
  {
    const char*  details = "Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden";
    OrionError   oe(SccBadRequest, details);

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, details);
    restReply(ciP, oe.smartRender(ciP->apiVersion));

    return MHD_YES;
  }


  //
  // If ciP->answer is non-empty, then an error has been detected
  //
  if (ciP->answer != "")
  {
    alarmMgr.badInput(clientIp, ciP->answer);
    restReply(ciP, ciP->answer);

    return MHD_YES;
  }


  //
  // If error detected, just call treat function and respond to caller
  //
  if (ciP->httpStatusCode != SccOk)
  {
    ciP->answer = ciP->restServiceP->treat(ciP, ciP->urlComponents, ciP->urlCompV, NULL);

    // Bad Verb in API v1 should have empty payload
    if ((ciP->apiVersion == V1) && (ciP->httpStatusCode == SccBadVerb))
    {
      ciP->answer = "";
    }

    restReply(ciP, ciP->answer);
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
