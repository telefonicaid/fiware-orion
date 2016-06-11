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
#include "common/defaultValues.h"
#include "common/clockFunctions.h"
#include "common/statistics.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"

#include "parse/forbiddenChars.h"
#include "rest/RestService.h"
#include "rest/rest.h"
#include "rest/restReply.h"
#include "rest/OrionError.h"
#include "rest/errorAdaptation.h"
#include "rest/uriParamNames.h"
#include "common/limits.h"  // SERVICE_NAME_MAX_LEN



/* ****************************************************************************
*
* Globals
*/
static RestService*              restServiceV          = NULL;
static unsigned short            port                  = 0;
static RestServeFunction         serveFunction         = NULL;
static char                      bindIp[MAX_LEN_IP]    = "0.0.0.0";
static char                      bindIPv6[MAX_LEN_IP]  = "::";
IpVersion                        ipVersionUsed         = IPDUAL;
bool                             multitenant           = false;
std::string                      rushHost              = "";
unsigned short                   rushPort              = NO_PORT;
char                             restAllowedOrigin[64];
static MHD_Daemon*               mhdDaemon             = NULL;
static MHD_Daemon*               mhdDaemon_v6          = NULL;
static struct sockaddr_in        sad;
static struct sockaddr_in6       sad_v6;
__thread char                    static_buffer[STATIC_BUFFER_SIZE + 1];
__thread char                    clientIp[IP_LENGTH_MAX + 1];
static unsigned int              connMemory;
static unsigned int              maxConns;
static unsigned int              threadPoolSize;



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
* setStatusCodeAndSmartRender -
*
* FIXME PR: same function as in errorAdaptation.h. I need this here by the moment,
* to avoid a linking problem
*/
std::string setStatusCodeAndSmartRender(ConnectionInfo* ciP, OrionError& oe)
{
  if (ciP->apiVersion == "v2")
  {
    ciP->httpStatusCode = oe.code;
  }
  return oe.smartRender(ciP->apiVersion);
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
    OrionError error(SccBadRequest, std::string("Empty right-hand-side for URI param /") + ckey + "/");
    ciP->answer = setStatusCodeAndSmartRender(ciP, error);
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
        ciP->answer = setStatusCodeAndSmartRender(ciP, error);
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
        ciP->httpStatusCode = SccBadRequest;
        ciP->answer         = error.render(ciP, "");
        return MHD_YES;
      }

      ++cP;
    }

    int limit = atoi(val);
    if (limit > atoi(MAX_PAGINATION_LIMIT))
    {
      OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [max: " + MAX_PAGINATION_LIMIT + "]");
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = error.render(ciP, "");
      return MHD_YES;
    }
    else if (limit == 0)
    {
      OrionError error(SccBadRequest, std::string("Bad pagination limit: /") + value + "/ [a value of ZERO is unacceptable]");
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = error.render(ciP, "");
      return MHD_YES;
    }
  }
  else if (key == URI_PARAM_PAGINATION_DETAILS)
  {
    if ((strcasecmp(value.c_str(), "on") != 0) && (strcasecmp(value.c_str(), "off") != 0))
    {
      OrionError error(SccBadRequest, std::string("Bad value for /details/: /") + value + "/ [accepted: /on/, /ON/, /off/, /OFF/. Default is /off/]");
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = error.render(ciP, "");
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
      ciP->answer = setStatusCodeAndSmartRender(ciP, error);
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
  else if (key != URI_PARAM_Q)  // FIXME P1: possible more known options here ...
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
  else if ((key != "q") && (key != "idPattern"))
  {
    containsForbiddenChars = forbiddenChars(ckey) || forbiddenChars(val);
  }

  if (containsForbiddenChars == true)
  {
    std::string details = std::string("found a forbidden character in URI param '") + key + "'";
    OrionError error(SccBadRequest, "invalid character in URI parameter");

    alarmMgr.badInput(clientIp, details);
    ciP->answer = setStatusCodeAndSmartRender(ciP, error);
  }

  return MHD_YES;
}



/* ****************************************************************************
*
* httpHeaderGet - 
*/
static int httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* value)
{
  HttpHeaders*  headerP = (HttpHeaders*) cbDataP;
  std::string   key     = ckey;

  LM_T(LmtHttpHeaders, ("HTTP Header:   %s: %s", key.c_str(), value));

  if      (strcasecmp(key.c_str(), "User-Agent") == 0)        headerP->userAgent      = value;
  else if (strcasecmp(key.c_str(), "Host") == 0)              headerP->host           = value;
  else if (strcasecmp(key.c_str(), "Accept") == 0)            headerP->accept         = value;
  else if (strcasecmp(key.c_str(), "Expect") == 0)            headerP->expect         = value;
  else if (strcasecmp(key.c_str(), "Connection") == 0)        headerP->connection     = value;
  else if (strcasecmp(key.c_str(), "Content-Type") == 0)      headerP->contentType    = value;
  else if (strcasecmp(key.c_str(), "Content-Length") == 0)    headerP->contentLength  = atoi(value);
  else if (strcasecmp(key.c_str(), "Origin") == 0)            headerP->origin         = value;
  else if (strcasecmp(key.c_str(), "Fiware-Service") == 0)    headerP->tenant         = value;
  else if (strcasecmp(key.c_str(), "X-Auth-Token") == 0)      headerP->xauthToken     = value;
  else if (strcasecmp(key.c_str(), "X-Forwarded-For") == 0)   headerP->xforwardedFor  = value;
  else if (strcasecmp(key.c_str(), "Fiware-Correlator") == 0) headerP->correlator     = value;
  else if (strcasecmp(key.c_str(), "Fiware-Servicepath") == 0)
  {
    headerP->servicePath         = value;
    headerP->servicePathReceived = true;
  }
  else
  {
    LM_T(LmtHttpUnsupportedHeader, ("'unsupported' HTTP header: '%s', value '%s'", ckey, value));
  }

  if ((strcasecmp(key.c_str(), "connection") == 0) && (headerP->connection != "") && (headerP->connection != "close"))
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
* wantedOutputSupported - 
*/
static MimeType wantedOutputSupported(const std::string& apiVersion, const std::string& acceptList, std::string* charsetP)
{
  std::vector<std::string>  vec;
  char*                     copy;

  if (acceptList.length() == 0) 
  {
    /* HTTP RFC states that a missing Accept header must be interpreted as if the client is accepting any type */
    copy = strdup("*/*");
  }
  else 
  {
    copy = strdup((char*) acceptList.c_str());
  }
  char*  cP   = copy;

  do
  {
     char* comma;

     comma = strstr(cP, ",");
     if (comma != NULL)
     {
        *comma = 0;
        
        cP = wsStrip(cP);
        vec.push_back(cP);
        cP = comma;
        ++cP;
     }
     else
     {
        cP = wsStrip(cP);
        if (*cP != 0)
        {
           vec.push_back(cP);
        }
        *cP = 0;
     }

  } while (*cP != 0);

  free(copy);

  bool json = false;
  bool text = true;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
     char* s;

     //
     // charset embedded in 'Accept' header?
     // We read it but we don't do anything with it ...
     //
     if ((s = strstr((char*) vec[ix].c_str(), ";")) != NULL)
     {
        *s = 0;
        ++s;
        s = wsStrip(s);
        if (strncmp(s, "charset=", 8) == 0)
        {
           s = &s[8];
           s = wsStrip(s);

           if (charsetP != NULL)
              *charsetP = s;
        }
     }

     std::string mimeType = vec[ix].c_str();
     if (mimeType == "*/*")              { json = true; text=true;}
     if (mimeType == "application/*")    { json = true; }
     if (mimeType == "application/json") { json = true; }
     if (mimeType == "*/json")           { json = true; }
     if (mimeType == "text/plain")       { text = true; }

     //
     // Resetting charset
     //
     if (charsetP != NULL)
     {
       *charsetP = "";
     }
  }

  if (apiVersion == "v2")
  {
    if (json == true)
    {
      return JSON;
    }
    else if (text)
    {
      return TEXT;
    }
  }
  else
  {
    if (json == true)
    {
      return JSON;
    }
  }

  alarmMgr.badInput(clientIp, "no valid 'Accept-format' found");
  return NOMIMETYPE;
}



/* ****************************************************************************
*
* serve - 
*/
static void serve(ConnectionInfo* ciP)
{
  restService(ciP, restServiceV);
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
  ConnectionInfo*  ciP      = (ConnectionInfo*) *con_cls;
  struct timespec  reqEndTime;

  if ((ciP->payload != NULL) && (ciP->payload != static_buffer))
  {
    free(ciP->payload);
  }

  *con_cls = NULL;

  lmTransactionEnd();  // Incoming REST request ends

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &reqEndTime);
    clock_difftime(&reqEndTime, &ciP->reqStartTime, &threadLastTimeStat.reqTime);
  }  

  delete(ciP);

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
}



/* ****************************************************************************
*
* outMimeTypeCheck - 
*/
static int outMimeTypeCheck(ConnectionInfo* ciP)
{
  ciP->outMimeType  = wantedOutputSupported(ciP->apiVersion, ciP->httpHeaders.accept, &ciP->charset);
  if (ciP->outMimeType == NOMIMETYPE)
  {
    /* This is actually an error in the HTTP layer (not exclusively NGSI) so we don't want to use the default 200 */
    ciP->httpStatusCode = SccNotAcceptable;
    ciP->answer = restErrorReplyGet(ciP,
                                    "",
                                    "OrionError",
                                    SccNotAcceptable,
                                    std::string("acceptable MIME types: application/json. Accept header in request: ") + ciP->httpHeaders.accept);

    ciP->outMimeType    = JSON; // We use JSON as default mimeType
    ciP->httpStatusCode = SccNotAcceptable;

    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* servicePathCheck - check vector of service paths
*
* This function is called for ALL requests, when a service-path URI-parameter is found.
* So, '#' is considered a valid character at it is valid for discoveries and queries.
* Later on, if the request is a registration or notification, another function is called
* to make sure there is only ONE service path and that there is no '#' present.
*
* FIXME P5: updates should also call the other servicePathCheck (in common lib)
*
* [ Not static just to let unit tests call this function ]
*/
int servicePathCheck(ConnectionInfo* ciP, const char* servicePath)
{
  //
  // 1. Max 10 paths  - ONLY ONE path allowed at this moment
  // 2. Max 10 levels in each path
  // 3. Max 50 characters in each path component
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
    OrionError e(SccBadRequest, "Only /absolute/ Service Paths allowed [a service path must begin with /]");
    ciP->answer = setStatusCodeAndSmartRender(ciP, e);
    return 1;
  }

  components = stringSplit(servicePath, '/', compV);

  if (components > SERVICE_PATH_MAX_LEVELS)
  {
    OrionError e(SccBadRequest, "too many components in ServicePath");
    ciP->answer = setStatusCodeAndSmartRender(ciP, e);
    return 2;
  }

  for (int ix = 0; ix < components; ++ix)
  {
    if (strlen(compV[ix].c_str()) > SERVICE_PATH_MAX_COMPONENT_LEN)
    {
      OrionError e(SccBadRequest, "component-name too long in ServicePath");
      ciP->answer = setStatusCodeAndSmartRender(ciP, e);
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
        OrionError e(SccBadRequest, "a component of ServicePath contains an illegal character");
        ciP->answer = setStatusCodeAndSmartRender(ciP, e);
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
  if ((ciP->httpHeaders.servicePathReceived == true) && (ciP->servicePath == ""))
  {
    OrionError e(SccBadRequest, "empty service path");
    ciP->answer = e.render(ciP, "");
    alarmMgr.badInput(clientIp, "empty service path");
    return -1;
  }
#endif

  int servicePaths = stringSplit(ciP->servicePath, ',', ciP->servicePathV);

  if (servicePaths == 0)
  {
    /* In this case the result is a vector with an empty string */
    ciP->servicePathV.push_back("");
    return 0;
  }

  if (servicePaths > SERVICE_PATH_MAX_COMPONENTS)
  {
    OrionError e(SccBadRequest, "too many service paths - a maximum of ten service paths is allowed");
    ciP->answer = e.render(ciP, "");
    return -1;
  }

  for (int ix = 0; ix < servicePaths; ++ix)
  {
    std::string stripped = std::string(wsStrip((char*) ciP->servicePathV[ix].c_str()));

    ciP->servicePathV[ix] = removeTrailingSlash(stripped);

    // This was previously a LM_T trace, but we have "promoted" it to INFO due to it is needed to check logs in a .test case (case 0392 service_path_http_header.test)
    LM_I(("Service Path %d: '%s'", ix, ciP->servicePathV[ix].c_str()));
  }

  for (int ix = 0; ix < servicePaths; ++ix)
  {
    int s;

    if ((s = servicePathCheck(ciP, ciP->servicePathV[ix].c_str())) != 0)
    {
      return s;
    }
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
    ciP->answer = restErrorReplyGet(ciP, "", "OrionError", SccUnsupportedMediaType, details);
    ciP->httpStatusCode = SccUnsupportedMediaType;

    return 1;
  }

  // Case 3
  if ((ciP->apiVersion == "v1") && (ciP->httpHeaders.contentType != "application/json"))
  {
    std::string details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;
    ciP->httpStatusCode = SccUnsupportedMediaType;
    ciP->answer = restErrorReplyGet(ciP, "", "OrionError", SccUnsupportedMediaType, details);
    ciP->httpStatusCode = SccUnsupportedMediaType;
    return 1;
  }


  // Case 4
  if ((ciP->apiVersion == "v2") && (ciP->httpHeaders.contentType != "application/json") && (ciP->httpHeaders.contentType != "text/plain"))
  {
    std::string details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;
    ciP->httpStatusCode = SccUnsupportedMediaType;
    ciP->answer = restErrorReplyGet(ciP, "", "OrionError", SccUnsupportedMediaType, details);
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
    OrionError error(SccBadRequest, "invalid character in URI");
    ciP->httpStatusCode = error.code;
    ciP->answer         = setStatusCodeAndSmartRender(ciP, error);
    return false;
  }

  //
  // Remove '/' at end of URL path
  //
  char* s = (char*) url.c_str();
  while (s[strlen(s) - 1] == '/')
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
* based on the URL.
* If the URL starts with "/v2" then the request is considered API version 2.
*
* Otherwise, API version 1.
*
* Except ...
* The new request to change the log level (not trace level), uses the
* URL /admin/log, which DOES NOT start with '/v2', but as some render methods
* depend on the apiVersion and we prefer the 'new render' from v2 for this
* operation (see OrionError::render), we consider internally /admin/log to be part of v2.
*
* FIXME P2: instead of looking at apiVersion for rendering, perhaps we need
*           some other algorithm, considering 'admin' requests as well ...
*/
static std::string apiVersionGet(const char* path)
{
  if ((path[1] == 'v') && (path[2] == '2'))
  {
    return "v2";
  }

  if (strcmp(path, "/admin/log") == 0)
  {
    return "v2";
  }

  return "v1";
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
      return MHD_NO;
    }

    if (timingStatistics)
    {
      clock_gettime(CLOCK_REALTIME, &ciP->reqStartTime);
    }

    LM_T(LmtRequest, (""));
    // WARNING: This log message below is crucial for the correct function of the Behave tests - CANNOT BE REMOVED
    LM_T(LmtRequest, ("--------------------- Serving request %s %s -----------------", method, url));
    *con_cls     = (void*) ciP; // Pointer to ConnectionInfo for subsequent calls
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
    
    MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, &ciP->httpHeaders);

    char correlator[CORRELATOR_ID_SIZE + 1];
    if (ciP->httpHeaders.correlator == "")
    {
      correlatorGenerate(correlator);
      ciP->httpHeaders.correlator = correlator;
    }

    correlatorIdSet(ciP->httpHeaders.correlator.c_str());

    ciP->httpHeader.push_back("Fiware-Correlator");
    ciP->httpHeaderValue.push_back(ciP->httpHeaders.correlator);

    //
    // Transaction starts here
    //
    lmTransactionStart("from", ip, port, url);  // Incoming REST request starts


    /* X-Forwared-For (used by a potential proxy on top of Orion) overrides ip */
    if (ciP->httpHeaders.xforwardedFor == "")
    {
      lmTransactionSetFrom(ip);
    }
    else
    {
      lmTransactionSetFrom(ciP->httpHeaders.xforwardedFor.c_str());
    }

    ciP->apiVersion = apiVersionGet(ciP->url.c_str());

    char tenant[SERVICE_NAME_MAX_LEN + 1];
    ciP->tenantFromHttpHeader = strToLower(tenant, ciP->httpHeaders.tenant.c_str(), sizeof(tenant));
    ciP->outMimeType          = wantedOutputSupported(ciP->apiVersion, ciP->httpHeaders.accept, &ciP->charset);
    if (ciP->outMimeType == NOMIMETYPE)
    {
      ciP->outMimeType = JSON; // JSON is default output mimeType
    }

    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

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
    // If the HTTP header says the request is bigger than our PAYLOAD_MAX_SIZE,
    // just silently "eat" the entire message
    //
    if (ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE)
    {
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
  }

  ciP->servicePath = ciP->httpHeaders.servicePath;
  lmTransactionSetSubservice(ciP->servicePath.c_str());

  if (servicePathSplit(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "error in ServicePath http-header");
    restReply(ciP, ciP->answer);
  }

  if (contentTypeCheck(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "invalid mime-type in Content-Type http-header");
    restReply(ciP, ciP->answer);
  }
  else if (outMimeTypeCheck(ciP) != 0)
  {
    alarmMgr.badInput(clientIp, "invalid mime-type in Accept http-header");
    restReply(ciP, ciP->answer);
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
  if (ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE)
  {
    char details[256];
    snprintf(details, sizeof(details), "payload size: %d, max size supported: %d", ciP->httpHeaders.contentLength, PAYLOAD_MAX_SIZE);

    alarmMgr.badInput(clientIp, details);

    ciP->answer         = restErrorReplyGet(ciP, "", ciP->url, SccRequestEntityTooLarge, details);
    ciP->httpStatusCode = SccRequestEntityTooLarge;
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
    std::string errorMsg = restErrorReplyGet(ciP, "", url, SccContentLengthRequired, "Zero/No Content-Length in PUT/POST/PATCH request");

    ciP->httpStatusCode  = SccContentLengthRequired;
    restReply(ciP, errorMsg);
    alarmMgr.badInput(clientIp, errorMsg);
  }
  else if (ciP->answer != "")
  {
    alarmMgr.badInput(clientIp, ciP->answer);
    restReply(ciP, ciP->answer);
  }
  else
  {
    serveFunction(ciP);
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
*/
static int restStart(IpVersion ipVersion, const char* httpsKey = NULL, const char* httpsCertificate = NULL)
{
  bool      mhdStartError  = true;
  size_t    memoryLimit    = connMemory * 1024; // Connection memory is expressed in kilobytes
  MHD_FLAG  serverMode     = MHD_USE_THREAD_PER_CONNECTION;

  if (port == 0)
  {
    LM_X(1, ("Fatal Error (please call restInit before starting the REST service)"));
  }

  if (threadPoolSize != 0)
  {
    serverMode = MHD_USE_SELECT_INTERNALLY;
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
      LM_T(LmtMhd, ("Starting HTTPS daemon on IPv4 %s port %d", bindIp, port));
      mhdDaemon = MHD_start_daemon(serverMode | MHD_USE_SSL,
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
                                   MHD_OPTION_END);

    }
    else
    {
      LM_T(LmtMhd, ("Starting HTTP daemon on IPv4 %s port %d", bindIp, port));
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

    if ((httpsKey != NULL) && (httpsCertificate != NULL))
    {
      LM_T(LmtMhd, ("Starting HTTPS daemon on IPv6 %s port %d", bindIPv6, port));
      mhdDaemon_v6 = MHD_start_daemon(serverMode | MHD_USE_IPv6 | MHD_USE_SSL,
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
                                      MHD_OPTION_END);
    }
    else
    {
      LM_T(LmtMhd, ("Starting HTTP daemon on IPv6 %s port %d", bindIPv6, port));
      mhdDaemon_v6 = MHD_start_daemon(serverMode | MHD_USE_IPv6,
                                      htons(port),
                                      NULL,
                                      NULL,
                                      connectionTreat,                     NULL,
                                      MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                      MHD_OPTION_CONNECTION_LIMIT,         maxConns,
                                      MHD_OPTION_THREAD_POOL_SIZE,         threadPoolSize,
                                      MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad_v6,
                                      MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
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
  RestService*        _restServiceV,
  IpVersion           _ipVersion,
  const char*         _bindAddress,
  unsigned short      _port,
  bool                _multitenant,
  unsigned int        _connectionMemory,
  unsigned int        _maxConnections,
  unsigned int        _mhdThreadPoolSize,
  const std::string&  _rushHost,
  unsigned short      _rushPort,
  const char*         _allowedOrigin,
  const char*         _httpsKey,
  const char*         _httpsCertificate,
  RestServeFunction   _serveFunction
)
{
  const char* key  = _httpsKey;
  const char* cert = _httpsCertificate;

  port             = _port;
  restServiceV     = _restServiceV;
  ipVersionUsed    = _ipVersion;
  serveFunction    = (_serveFunction != NULL)? _serveFunction : serve;  
  multitenant      = _multitenant;
  connMemory       = _connectionMemory;
  maxConns         = _maxConnections;
  threadPoolSize   = _mhdThreadPoolSize;
  rushHost         = _rushHost;
  rushPort         = _rushPort;

  strncpy(restAllowedOrigin, _allowedOrigin, sizeof(restAllowedOrigin));

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
