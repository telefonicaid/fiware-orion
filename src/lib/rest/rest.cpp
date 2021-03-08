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
#include "common/limits.h"                // SERVICE_NAME_MAX_LEN
#include "common/logTracing.h"

#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "parse/forbiddenChars.h"

#include "rest/Verb.h"
#include "rest/HttpHeaders.h"
#include "rest/RestService.h"
#include "rest/restReply.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "rest/restServiceLookup.h"
#include "rest/rest.h"



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

// FIXME P5: replace 1024 with a proper value, based on literature for URL max length
__thread char                    uriForLogs[1024];



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
static int uriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* val)
{
  ConnectionInfo*  ciP   = (ConnectionInfo*) cbDataP;
  std::string      key   = ckey;
  std::string      value = (val == NULL)? "" : val;

  if (val == NULL || *val == 0)
  {
    std::string  errorString = std::string("Empty right-hand-side for URI param /") + ckey + "/";

    if (ciP->apiVersion == V2)
    {
      OrionError error(SccBadRequest, errorString);
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);
    }
    else if (ciP->apiVersion == ADMIN_API)
    {
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = "{" + JSON_STR("error") + ":" + JSON_STR(errorString) + "}";
    }

    return MHD_YES;
  }

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
        return MHD_YES;
      }

      ++cP;
    }
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
      return MHD_YES;
    }
    else if (limit == 0)
    {
      OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [a value of ZERO is unacceptable]");
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);
      return MHD_YES;
    }
  }
  else if (key == URI_PARAM_PAGINATION_DETAILS)
  {
    if ((strcasecmp(value.c_str(), "on") != 0) && (strcasecmp(value.c_str(), "off") != 0))
    {
      OrionError error(SccBadRequest, std::string("Bad value for /details/: /") + value + "/ [accepted: /on/, /ON/, /off/, /OFF/. Default is /off/]");
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);
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
      ciP->httpStatusCode = error.code;
      ciP->answer         = error.smartRender(ciP->apiVersion);
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
  else if ((key != URI_PARAM_Q)       &&
           (key != URI_PARAM_MQ)      &&
           (key != URI_PARAM_LEVEL))  // FIXME P1: possible more known options here ...
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
  // So, if the media-range is anything else, it is rejected immediately and not put in the list
  //
  if ((strcmp(cP, "*/*")              != 0) &&
      (strcmp(cP, "application/*")    != 0) &&
      (strcmp(cP, "application/json") != 0) &&
      (strcmp(cP, "text/*")           != 0) &&
      (strcmp(cP, "text/plain")       != 0))
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

  if ((ciP->httpHeaders.acceptHeaderV.size() == 0) && (ciP->acceptHeaderError.empty()))
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
static int httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* value)
{
  ConnectionInfo*  ciP     = (ConnectionInfo*) cbDataP;
  HttpHeaders*     headerP = &ciP->httpHeaders;
  std::string      key     = ckey;

  LM_T(LmtHttpHeaders, ("HTTP Header:   %s: %s", key.c_str(), value));

  if (strcasecmp(key.c_str(), HTTP_ACCEPT) == 0)
  {
    headerP->accept = value;
    acceptParse(ciP, value);  // Any errors are flagged in ciP->acceptHeaderError and taken care of later
  }
  else if (strcasecmp(key.c_str(), HTTP_CONTENT_TYPE) == 0)      headerP->contentType    = value;
  else if (strcasecmp(key.c_str(), HTTP_CONTENT_LENGTH) == 0)    headerP->contentLength  = atoi(value);
  else if (strcasecmp(key.c_str(), HTTP_ORIGIN) == 0)            headerP->origin         = value;
  else if (strcasecmp(key.c_str(), HTTP_FIWARE_SERVICE) == 0)
  {
    headerP->tenant = value;
    toLowercase((char*) headerP->tenant.c_str());
  }
  else if (strcasecmp(key.c_str(), HTTP_X_AUTH_TOKEN) == 0)        headerP->xauthToken         = value;
  else if (strcasecmp(key.c_str(), HTTP_X_REAL_IP) == 0)           headerP->xrealIp            = value;
  else if (strcasecmp(key.c_str(), HTTP_X_FORWARDED_FOR) == 0)     headerP->xforwardedFor      = value;
  else if (strcasecmp(key.c_str(), HTTP_FIWARE_CORRELATOR) == 0)   headerP->correlator         = value;
  else if (strcasecmp(key.c_str(), HTTP_NGSIV2_ATTRSFORMAT) == 0)  headerP->ngsiv2AttrsFormat  = value;
  else if (strcasecmp(key.c_str(), HTTP_FIWARE_SERVICEPATH) == 0)
  {
    headerP->servicePath         = value;
    headerP->servicePathReceived = true;
  }
  else
  {
    LM_T(LmtHttpUnsupportedHeader, ("'unsupported' HTTP header: '%s', value '%s'", ckey, value));
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
  //
  // delayed release of ContextElementResponseVector must be effectuated now.
  // See github issue #2994
  //
  extern void delayedReleaseExecute(void);
  delayedReleaseExecute();

  // It's unsual, but *con_cls can be NULL under some circustances, e.g. toe=MHD_REQUEST_TERMINATED_CLIENT_ABORT
  // In addition, we add a similar check for con_cls (we haven't found any case in which this happens, but
  // let's be conservative... otherwise CB will crash)
  if ((con_cls == NULL) || (*con_cls == NULL))
  {
    lmTransactionEnd();  // Incoming REST request ends (maybe unneeded as the transaction hasn't start, but doesn't hurt)
    return;
  }

  ConnectionInfo*  ciP      = (ConnectionInfo*) *con_cls;

  *con_cls = NULL;

  std::string      spath    = (ciP->servicePathV.size() > 0)? ciP->servicePathV[0] : "";
  struct timespec  reqEndTime;

  if ((ciP->verb != GET) && (ciP->verb != DELETE) && (ciP->payload != NULL) && (strlen(ciP->payload) > 0))
  {
    // Variant with payload
    logInfoRequestWithPayload(ciP->method.c_str(), ciP->uriForLogs.c_str(), ciP->payload, ciP->httpStatusCode);
  }
  else
  {
    // Variant without payload
    logInfoRequestWithoutPayload(ciP->method.c_str(), ciP->uriForLogs.c_str(), ciP->httpStatusCode);
  }

  if ((ciP->payload != NULL) && (ciP->payload != static_buffer))
  {
    free(ciP->payload);
  }

  lmTransactionEnd();  // Incoming REST request ends

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &reqEndTime);
    clock_difftime(&reqEndTime, &ciP->reqStartTime, &threadLastTimeStat.reqTime);

    //
    // Statistics
    //
    // Flush this requests timing measures onto a global var to be read by "GET /statistics".
    // Also, increment the accumulated measures.
    //
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
  metricsMgr.add(ciP->httpHeaders.tenant, spath, METRIC_TRANS_IN, 1);

  //
  // If the httpStatusCode is above the set of 200s, an error has occurred
  //
  if (ciP->httpStatusCode >= SccBadRequest)
  {
    metricsMgr.add(ciP->httpHeaders.tenant, spath, METRIC_TRANS_IN_ERRORS, 1);
  }

  if (metricsMgr.isOn() && (ciP->transactionStart.tv_sec != 0))
  {
    struct timeval  end;

    if (gettimeofday(&end, NULL) == 0)
    {
      unsigned long long elapsed =
          (end.tv_sec  - ciP->transactionStart.tv_sec) * 1000000 +
          (end.tv_usec - ciP->transactionStart.tv_usec);

      metricsMgr.add(ciP->httpHeaders.tenant, spath, _METRIC_TOTAL_SERVICE_TIME, elapsed);
    }
  }

  delete(ciP);
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
  return ((!requestOrigin.empty()) && ((strcmp(corsOrigin, "__ALL") == 0) || (strcmp(requestOrigin.c_str(), corsOrigin) == 0)));
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
  if ((ciP->httpHeaders.servicePathReceived == true) && (ciP->httpHeaders.servicePath.empty()))
  {
    OrionError e(SccBadRequest, "empty service path");
    ciP->answer = e.render();
    alarmMgr.badInput(clientIp, "empty service path");
    return -1;
  }
#endif
  char* servicePathCopy = NULL;
  int   servicePaths    = 0;

  if (!ciP->httpHeaders.servicePath.empty())
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
    ciP->answer = e.toJsonV1();

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
    LM_T(LmtOldInfo, ("Service Path %d: '%s'", ix, ciP->servicePathV[ix].c_str()));
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
  if (ciP->httpHeaders.contentType.empty())
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



RestService restServiceForBadVerb;



/* ****************************************************************************
*
* getUriForLog -
*
* This handle is described at: https://www.gnu.org/software/libmicrohttpd/manual/html_node/microhttpd_002dconst.html
*
*/
static void* getUriForLog(void* cls, const char* uri, struct MHD_Connection *con)
{
  // We need this for getting raw URL (path + query params) for logs. Note we
  // cannot use ciP->url, as it only has the path part

  // Note that the uriForLogs variable set by this handler is later used to
  // set ciP->uriForLogs in connectionTreat(). We cannot do this assignment
  // direclty, as at the moment this getUriForLog callback is called the ciP object
  // doesn't exist so we use a thread variable

  strncpy(uriForLogs, uri, sizeof(uriForLogs));

  return NULL;
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
static int reqNo       = 1;
static int connectionTreat
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
  ConnectionInfo*        ciP         = (ConnectionInfo*) *con_cls;
  size_t                 dataLen     = *upload_data_size;

  // 1. First call - setup ConnectionInfo and get/check HTTP headers
  if (ciP == NULL)
  {
    struct timeval transactionStart;

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
    char            ip[32];
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
      return MHD_NO;
    }


    // Get API version
    // FIXME #3109-PR: this assignment will be removed in a subsequent PR, where the function apiVersionGet() is used instead
    //
    ciP->apiVersion = (url[2] == '2')? V2 : V1;  // If an APIv2 request, the URL starts with "/v2/". Only V2 requests.

    // LM_TMP(("--------------------- Serving APIv%d request %s %s -----------------", ciP->apiVersion, method, url));

    // Lookup Rest Service
    bool badVerb = false;
    ciP->restServiceP = restServiceLookup(ciP, &badVerb);

    if (badVerb)
    {
      // Bad Verb is taken care of later
      ciP->httpStatusCode = SccBadVerb;
      ciP->restServiceP   = &restServiceForBadVerb;  // FIXME #3109-PR: Try to remove this, or make restServiceLookup return a dummy
    }

    ciP->transactionStart.tv_sec  = transactionStart.tv_sec;
    ciP->transactionStart.tv_usec = transactionStart.tv_usec;

    if (timingStatistics)
    {
      clock_gettime(CLOCK_REALTIME, &ciP->reqStartTime);
    }

    // WARNING: This log message below is crucial for the correct function of the Behave tests - CANNOT BE REMOVED
    LM_T(LmtRequest, ("--------------------- Serving request %s %s -----------------", method, url));
    *con_cls     = (void*) ciP; // Pointer to ConnectionInfo for subsequent calls
    ciP->port    = port;
    ciP->ip      = ip;
    ciP->callNo  = reqNo;

    ++reqNo;

    // To be used by logs during the processing of the request
    // It is assumed that as this point of code the getUriForLog handler has been
    // previously called, so uriForLogs is not null
    ciP->uriForLogs = std::string(uriForLogs);

    //
    // URI parameters
    //
    // FIXME P1: We might not want to do all these assignments, they are not used in all requests ...
    //           Once we *really* look to scratch some efficiency, this change should be made.
    //
    ciP->uriParam[URI_PARAM_PAGINATION_OFFSET]  = DEFAULT_PAGINATION_OFFSET;
    ciP->uriParam[URI_PARAM_PAGINATION_LIMIT]   = DEFAULT_PAGINATION_LIMIT;
    ciP->uriParam[URI_PARAM_PAGINATION_DETAILS] = DEFAULT_PAGINATION_DETAILS;

    // Note that we need to get API version before MHD_get_connection_values() as the later
    // function may result in an error after processing Accept headers (and the
    // render for the error depends on API version)
    ciP->apiVersion = apiVersionGet(ciP->url.c_str());

    MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);

    if (ciP->httpHeaders.accept.empty())  // No Accept: given, treated as */*
    {
      ciP->httpHeaders.accept = "*/*";
      acceptParse(ciP, "*/*");
    }

    char correlator[CORRELATOR_ID_SIZE + 1];
    if (ciP->httpHeaders.correlator.empty())
    {
      correlatorGenerate(correlator);
      ciP->httpHeaders.correlator = correlator;
    }

    correlatorIdSet(ciP->httpHeaders.correlator.c_str());

    ciP->httpHeader.push_back(HTTP_FIWARE_CORRELATOR);
    ciP->httpHeaderValue.push_back(ciP->httpHeaders.correlator);

    if ((ciP->httpHeaders.contentLength > inReqPayloadMaxSize) && (ciP->apiVersion == V2))
    {
      char details[256];
      snprintf(details, sizeof(details), "payload size: %d, max size supported: %llu", ciP->httpHeaders.contentLength, inReqPayloadMaxSize);

      alarmMgr.badInput(clientIp, details);
      OrionError oe(SccRequestEntityTooLarge, details);

      ciP->httpStatusCode = oe.code;
      restReply(ciP, oe.smartRender(ciP->apiVersion));
      return MHD_YES;
    }

    /* X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides ip.
       X-Real-IP takes preference over X-Forwarded-For, if both appear */
    std::string from;
    if (!ciP->httpHeaders.xrealIp.empty())
    {
      from = ciP->httpHeaders.xrealIp;

    }
    else if (!ciP->httpHeaders.xforwardedFor.empty())
    {
      from = ciP->httpHeaders.xforwardedFor;
    }
    else
    {
      from = ip;
    }

    char tenant[DB_AND_SERVICE_NAME_MAX_LEN];

    ciP->tenantFromHttpHeader = strToLower(tenant, ciP->httpHeaders.tenant.c_str(), sizeof(tenant));
    ciP->outMimeType          = mimeTypeSelect(ciP);

    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

    // Mark the init of the transaction - Incoming REST request starts
    lmTransactionStart("from", "", ip, port, url, ciP->tenantFromHttpHeader.c_str(), ciP->httpHeaders.servicePath.c_str(), from.c_str());

    return MHD_YES;
  }


  //
  // 2. Data gathering calls
  //
  // 2-1. Data gathering calls, just wait
  // 2-2. Last data gathering call, acknowledge the receipt of data
  //
  if (dataLen != 0)
  {
    //
    // If the HTTP header says the request is bigger than our inReqPayloadMaxSize,
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
    if (ciP->httpHeaders.contentLength > inReqPayloadMaxSize)
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

  //
  // 3. Finally, serve the request (unless an error has occurred)
  //
  // URL and headers checks are delayed to the "third" MHD call, as no
  // errors can be sent before all the request has been read
  //
  if (urlCheck(ciP, ciP->url) == false)
  {
    alarmMgr.badInput(clientIp, "error in URI path");
    restReply(ciP, ciP->answer);
    return MHD_YES;
  }
  else if (servicePathSplit(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "error in ServicePath http-header");
    restReply(ciP, ciP->answer);
    return MHD_YES;
  }
  else if (contentTypeCheck(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "invalid mime-type in Content-Type http-header");
    restReply(ciP, ciP->answer);
    return MHD_YES;
  }
  else
  {
    ciP->inMimeType = mimeTypeParse(ciP->httpHeaders.contentType, NULL);
  }

  if (ciP->httpStatusCode != SccOk)
  {
    alarmMgr.badInput(clientIp, "error in URI parameters");
    restReply(ciP, ciP->answer);
    return MHD_YES;
  }

  //
  // Here, if the incoming request was too big, return error about it
  //
  if (ciP->httpHeaders.contentLength > inReqPayloadMaxSize)
  {
    char details[256];

    snprintf(details, sizeof(details), "payload size: %d, max size supported: %llu", ciP->httpHeaders.contentLength, inReqPayloadMaxSize);
    alarmMgr.badInput(clientIp, details);
    restErrorReplyGet(ciP, SccRequestEntityTooLarge, details, &ciP->answer);

    ciP->httpStatusCode = SccRequestEntityTooLarge;
  }


  //
  // Check for error during Accept Header parsing
  //
  if (!ciP->acceptHeaderError.empty())
  {
    OrionError   oe(SccBadRequest, ciP->acceptHeaderError);

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, ciP->acceptHeaderError);
    restReply(ciP, oe.smartRender(ciP->apiVersion));
    return MHD_YES;
  }


  //
  // Check that Accept Header values are acceptable
  //
  bool textAccepted = false;
  if (!acceptHeadersAcceptable(ciP, &textAccepted))
  {
    OrionError oe(SccNotAcceptable, "");

    if (textAccepted)
    {
      oe.details = "acceptable MIME types: application/json, text/plain";
    }
    else
    {
      oe.details = "acceptable MIME types: application/json";
    }

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, oe.details);
    restReply(ciP, oe.smartRender(ciP->apiVersion));
    return MHD_YES;
  }


  // Note that ciP->outMimeType is not set here.
  // Why?
  // If text/plain is asked for and accepted ('*/value' operations) but something goes wrong,
  // then application/json is used for the error
  // If all goes well, the proper service routine will set ciP->outMimeType to text/plain
  //
  if (ciP->httpHeaders.outformatSelect() == NOMIMETYPE)
  {
    OrionError oe(SccNotAcceptable, "");

    if (textAccepted)
    {
      oe.details = "acceptable MIME types: application/json, text/plain";
    }
    else
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
  if ((!ciP->httpHeaders.contentType.empty()) && (ciP->httpHeaders.contentLength == 0) && ((ciP->verb == GET) || (ciP->verb == DELETE)))
  {
    const char*  details = "Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden";
    OrionError   oe(SccBadRequest, details);

    ciP->httpStatusCode = oe.code;
    alarmMgr.badInput(clientIp, details);
    restReply(ciP, oe.smartRender(ciP->apiVersion));
    return MHD_YES;
  }


  //
  // Requests of verb POST, PUT or PATCH are considered erroneous if no payload is present - with two exceptions.
  //
  // - Old log requests  (URL contains '/log/')
  // - New log requests  (URL is exactly '/admin/log')
  //
  if (((ciP->verb == POST) || (ciP->verb == PUT) || (ciP->verb == PATCH )) &&
      (ciP->httpHeaders.contentLength == 0) &&
      ((strncasecmp(ciP->url.c_str(), "/log/", 5) != 0) && (strncasecmp(ciP->url.c_str(), "/admin/log", 10) != 0)))
  {
    std::string errorMsg;

    restErrorReplyGet(ciP, SccContentLengthRequired, "Zero/No Content-Length in PUT/POST/PATCH request", &errorMsg);
    ciP->httpStatusCode  = SccContentLengthRequired;
    restReply(ciP, errorMsg);
    alarmMgr.badInput(clientIp, errorMsg);
  }
  else if (!ciP->answer.empty())
  {
    alarmMgr.badInput(clientIp, ciP->answer);
    restReply(ciP, ciP->answer);
  }
  else
  {
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
  int       serverMode     = MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL;

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
      LM_T(LmtMhd, ("Starting HTTPS daemon on IPv4 %s port %d, serverMode: 0x%x", bindIp, port, serverMode));
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
                                   MHD_OPTION_URI_LOG_CALLBACK,         getUriForLog, NULL,
                                   MHD_OPTION_END);

    }
    else
    {
      LM_T(LmtMhd, ("Starting HTTP daemon on IPv4 %s port %d, serverMode: 0x%x", bindIp, port, serverMode));
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
                                   MHD_OPTION_URI_LOG_CALLBACK,         getUriForLog, NULL,
                                   MHD_OPTION_END);

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
      LM_T(LmtMhd, ("Starting HTTPS daemon on IPv6 %s port %d, serverMode: 0x%x", bindIPv6, port, serverMode));
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
                                      MHD_OPTION_URI_LOG_CALLBACK,         getUriForLog, NULL,
                                      MHD_OPTION_END);
    }
    else
    {
      LM_T(LmtMhd, ("Starting HTTP daemon on IPv6 %s port %d, serverMode: 0x%x", bindIPv6, port, serverMode));
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
                                      MHD_OPTION_URI_LOG_CALLBACK,         getUriForLog, NULL,
                                      MHD_OPTION_END);
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

  strncpy(corsOrigin, _corsOrigin, sizeof(corsOrigin));
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
