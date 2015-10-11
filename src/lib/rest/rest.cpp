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
#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/wsStrip.h"
#include "common/globals.h"
#include "common/defaultValues.h"
#include "parse/forbiddenChars.h"
#include "rest/RestService.h"
#include "rest/rest.h"
#include "rest/restReply.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netdb.h>


/* ****************************************************************************
*
* IP - 
*/
#define  LOCAL_IP_V6  "::"
#define  LOCAL_IP_V4  "0.0.0.0"



/* ****************************************************************************
*
* PAYLOAD_SIZE - 
*/
#define PAYLOAD_SIZE       (64 * 1024 * 1024)
#define PAYLOAD_MAX_SIZE   (1 * 1024 * 1024)
#define STATIC_BUFFER_SIZE (32 * 1024)



/* ****************************************************************************
*
* Globals
*/
static RestService*              restServiceV          = NULL;
static unsigned short            port                  = 0;
static RestServeFunction         serveFunction         = NULL;
static bool                      acceptTextXml         = false;
static char                      bindIp[MAX_LEN_IP]    = "0.0.0.0";
static char                      bindIPv6[MAX_LEN_IP]  = "::";
IpVersion                        ipVersionUsed         = IPDUAL;
bool                             multitenant           = false;
std::string                      rushHost              = "";
unsigned short                   rushPort              = 0;
char                             restAllowedOrigin[64];
static MHD_Daemon*               mhdDaemon             = NULL;
static MHD_Daemon*               mhdDaemon_v6          = NULL;
static struct sockaddr_in        sad;
static struct sockaddr_in6       sad_v6;
__thread char                    static_buffer[STATIC_BUFFER_SIZE + 1];



/* ****************************************************************************
*
* uriArgumentGet - 
*/
static int uriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* val)
{
  ConnectionInfo*  ciP   = (ConnectionInfo*) cbDataP;
  std::string      key   = ckey;
  std::string      value = (val == NULL)? "" : val;

  if (key == URI_PARAM_NOTIFY_FORMAT)
  {
    if (strcasecmp(val, "xml") == 0)
      value = "XML";
    else if (strcasecmp(val, "json") == 0)
      value = "JSON";
    else
    {
      OrionError error(SccBadRequest, std::string("Bad notification format: /") + value + "/. Valid values: /XML/ and /JSON/");
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = error.render(ciP, "");
      return MHD_YES;
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
        ciP->httpStatusCode = SccBadRequest;
        ciP->answer         = error.render(ciP, "");
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

      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = error.render(ciP, "");
    }
  }
  else
    LM_T(LmtUriParams, ("Received unrecognized URI parameter: '%s'", key.c_str()));

  if (val != NULL)
    ciP->uriParam[key] = value;
  else
    ciP->uriParam[key] = "SET";

  LM_T(LmtUriParams, ("URI parameter:   %s: %s", key.c_str(), ciP->uriParam[key].c_str()));

  //
  // Now check the URI param has no invalid characters
  // Except for the URI params 'q' and 'idPattern' that are not to be checked for invalid characters
  //
  if ((key != "q") && (key != "idPattern"))
  {
    if (forbiddenChars(ckey) || forbiddenChars(val))
    {
      OrionError error(SccBadRequest, "invalid character in URI parameter");

      ciP->httpStatusCode = SccBadRequest;
      ciP->answer         = error.render(ciP, "");
    }
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

  if      (strcasecmp(key.c_str(), "User-Agent") == 0)      headerP->userAgent      = value;
  else if (strcasecmp(key.c_str(), "Host") == 0)            headerP->host           = value;
  else if (strcasecmp(key.c_str(), "Accept") == 0)          headerP->accept         = value;
  else if (strcasecmp(key.c_str(), "Expect") == 0)          headerP->expect         = value;
  else if (strcasecmp(key.c_str(), "Connection") == 0)      headerP->connection     = value;
  else if (strcasecmp(key.c_str(), "Content-Type") == 0)    headerP->contentType    = value;
  else if (strcasecmp(key.c_str(), "Content-Length") == 0)  headerP->contentLength  = atoi(value);
  else if (strcasecmp(key.c_str(), "Origin") == 0)          headerP->origin         = value;
  else if (strcasecmp(key.c_str(), "Fiware-Service") == 0)  headerP->tenant         = value;
  else if (strcasecmp(key.c_str(), "X-Auth-Token") == 0)    headerP->xauthToken     = value;
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
static Format wantedOutputSupported(const std::string& apiVersion, const std::string& acceptList, std::string* charsetP)
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

  bool xml  = false;
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

     std::string format = vec[ix].c_str();
     if (format == "*/*")              { xml  = true; json = true; text=true;}
     if (format == "*/xml")            xml  = true;
     if (format == "application/*")    { xml  = true; json = true; }
     if (format == "application/xml")  xml  = true;
     if (format == "application/json") json = true;
     if (format == "*/json")           json = true;
     if (format == "text/plain")       text = true;
     
     if ((acceptTextXml == true) && (format == "text/xml"))  xml = true;

     //
     // Resetting charset
     //
     if (charsetP != NULL)
        *charsetP = "";
  }

  //
  // API version 1 has XML as default format, v2 has JSON
  //
  if (apiVersion == "v2")
  {
    if (json == true)
    {
      return JSON;
    }
    else if (xml == true)
    {
      return XML;
    }
    else if (text)
    {
      return TEXT;
    }
  }
  else
  {
    if (xml == true)
    {
      return XML;
    }
    else if (json == true)
    {
      return JSON;
    }
  }


  LM_W(("Bad Input (no valid 'Accept-format' found)"));
  return NOFORMAT;
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
  ConnectionInfo* ciP      = (ConnectionInfo*) *con_cls;

  if ((ciP->payload != NULL) && (ciP->payload != static_buffer))
    free(ciP->payload);

  delete(ciP);
  *con_cls = NULL;

  LM_TRANSACTION_END();  // Incoming REST request ends
}


/* ****************************************************************************
*
* outFormatCheck - 
*/
static int outFormatCheck(ConnectionInfo* ciP)
{
  ciP->outFormat  = wantedOutputSupported(ciP->apiVersion, ciP->httpHeaders.accept, &ciP->charset);
  if (ciP->outFormat == NOFORMAT)
  {
    /* This is actually an error in the HTTP layer (not exclusively NGSI) so we don't want to use the default 200 */
    ciP->httpStatusCode = SccNotAcceptable;
    ciP->answer = restErrorReplyGet(ciP,
                                    XML,
                                    "",
                                    "OrionError",
                                    SccNotAcceptable,
                                    std::string("acceptable MIME types: application/xml, application/json. Accept header in request: ") + ciP->httpHeaders.accept);

    ciP->outFormat      = XML; // We use XML as default format
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
  std::vector<std::string> compV;
  int                      components;


  if (ciP->httpHeaders.servicePathReceived == false)
  {
    return 0;
  }

  if (servicePath[0] != '/')
  {
    OrionError e(SccBadRequest, "Only /absolute/ Service Paths allowed [a service path must begin with /]");
    ciP->answer = e.render(ciP, "");
    return 1;
  }

  components = stringSplit(servicePath, '/', compV);

  if (components > 10)
  {
    OrionError e(SccBadRequest, "too many components in ServicePath");
    ciP->answer = e.render(ciP, "");
    return 2;
  }

  for (int ix = 0; ix < components; ++ix)
  {
    if (strlen(compV[ix].c_str()) > 50)
    {
      OrionError e(SccBadRequest, "component-name too long in ServicePath");
      ciP->answer = e.render(ciP, "");
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
        ciP->answer = e.render(ciP, "");
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
    LM_W(("Bad Input (empty service path)"));
    return -1;
  }
#endif

  int servicePaths = stringSplit(ciP->servicePath, ',', ciP->servicePathV);

  if (servicePaths == 0)
  {
    /* In this case the result is a 0 length vector */
    return 0;
  }

  if (servicePaths > 10)
  {
    OrionError e(SccBadRequest, "too many service paths - a maximum of ten service paths is allowed");
    ciP->answer = e.render(ciP, "");
    return -1;
  }

  for (int ix = 0; ix < servicePaths; ++ix)
  {
    std::string stripped = std::string(wsStrip((char*) ciP->servicePathV[ix].c_str()));

    ciP->servicePathV[ix] = removeTrailingSlash(stripped);

    LM_T(LmtServicePath, ("Service Path %d: '%s'", ix, ciP->servicePathV[ix].c_str()));
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
*   In version 2 of the protocol, we admit ONLY application/json
*/
static int contentTypeCheck(ConnectionInfo* ciP)
{
  //
  // Five cases:
  //   1. If there is no payload, the Content-Type is not interesting
  //   2. Payload present but no Content-Type 
  //   3. text/xml used and acceptTextXml is setto true (iotAgent only)
  //   4. Content-Type present but not supported
  //   5. API version 2 and not 'application/json'
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
    ciP->answer = restErrorReplyGet(ciP, ciP->outFormat, "", "OrionError", SccUnsupportedMediaType, details);
    ciP->httpStatusCode = SccUnsupportedMediaType;

    return 1;
  }


  // Case 3
  if ((acceptTextXml == true) && (ciP->httpHeaders.contentType == "text/xml"))
  {
    return 0;
  }


  // Case 4
  if ((ciP->httpHeaders.contentType != "application/xml") && (ciP->httpHeaders.contentType != "application/json"))
  {
    std::string details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;
    ciP->httpStatusCode = SccUnsupportedMediaType;
    ciP->answer = restErrorReplyGet(ciP, ciP->outFormat, "", "OrionError", SccUnsupportedMediaType, details);
    ciP->httpStatusCode = SccUnsupportedMediaType;
    return 1;
  }


  // Case 5
  if ((ciP->apiVersion == "v2") && (ciP->httpHeaders.contentType != "application/json"))
  {
    std::string details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;
    ciP->httpStatusCode = SccUnsupportedMediaType;
    ciP->answer = restErrorReplyGet(ciP, ciP->outFormat, "", "OrionError", SccUnsupportedMediaType, details);
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
    ciP->httpStatusCode = SccBadRequest;
    ciP->answer         = error.render(ciP, "");

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
* defaultServicePath
*
* Returns a default servicePath (to be used in the case Fiware-servicePath header
* is not found in the request) depending on the RequestCheck
*/
std::string defaultServicePath(const char* url, const char* method)
{
  /* Look for standard operation (based on the URL). Given that some operations are
   * a substring of other, we search first for the longest ones
   *
   *  updateContextAvailabilitySubscription
   *  unsubscribeContextAvailability
   *  subscribeContextAvailability
   *  discoverContextAvailability
   *  updateContextSubscription
   *  unsubscribeContext
   *  subscribeContext
   *  registerContext
   *  updateContext
   *  queryContext
   *
   * Note that in the case of unsubscribeContext and unsubscribeContextAvailability doesn't
   * actually use a servicePath, but we also provide a default in that case for the sake of
   * completeness
   */

  // FIXME P5: this strategy can be improved taking into account full URL. Otherwise, it is
  // ambigous (e.g. entity which id is "queryContext" and are using a conv op). However, it is
  // highly improbable that the user uses entities which id (or type) match the name of a
  // standard operation

  // FIXME P5: unhardwire literals
  if (strcasestr(url, "updateContextAvailabilitySubscription") != NULL) return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "unsubscribeContextAvailability") != NULL)        return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "subscribeContextAvailability") != NULL)          return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "discoverContextAvailability") != NULL)           return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "notifyContextAvailability") != NULL)             return DEFAULT_SERVICE_PATH;
  if (strcasestr(url, "updateContextSubscription") != NULL)             return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "unsubscribeContext") != NULL)                    return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "subscribeContext") != NULL)                      return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "registerContext") != NULL)                       return DEFAULT_SERVICE_PATH;
  if (strcasestr(url, "updateContext") != NULL)                         return DEFAULT_SERVICE_PATH;
  if (strcasestr(url, "notifyContext") != NULL)                         return DEFAULT_SERVICE_PATH;
  if (strcasestr(url, "queryContext") != NULL)                          return DEFAULT_SERVICE_PATH_RECURSIVE;

  /* Look for convenience operation. Subscription-related operations are special, all the other depend on
   * the method
   */
  if (strcasestr(url, "contextAvailabilitySubscriptions") != NULL)      return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasestr(url, "contextSubscriptions") != NULL)                  return DEFAULT_SERVICE_PATH_RECURSIVE;
  if (strcasecmp(method, "POST") == 0)                                  return DEFAULT_SERVICE_PATH;
  if (strcasecmp(method, "PUT") == 0)                                   return DEFAULT_SERVICE_PATH;
  if (strcasecmp(method, "DELETE") == 0)                                return DEFAULT_SERVICE_PATH;
  if (strcasecmp(method, "GET") == 0)                                   return DEFAULT_SERVICE_PATH_RECURSIVE;

  LM_W(("Bad Input (cannot find default service path for: (%s, %s))", method, url));
  return DEFAULT_SERVICE_PATH;
}



/* ****************************************************************************
*
* apiVersionGet - 
*
* This function returns the version of the API for the incoming message,
* based on the URL.
* If the URL starts with "/v2" then the request is considered API version 2.
* Otherwise, API version 1.
*/
static std::string apiVersionGet(const char* path)
{
  if ((path[1] == 'v') && (path[2] == '2'))
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
  static int             reqNo       = 1;

  // 1. First call - setup ConnectionInfo and get/check HTTP headers
  if (ciP == NULL)
  {
    //
    // IP Address and port of caller
    //
    char            ip[32];
    unsigned short  port = 0;

    const union MHD_ConnectionInfo* mciP = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (mciP != NULL)
    {
      struct sockaddr* addr = (struct sockaddr*)mciP->client_addr;
      port = (addr->sa_data[0] << 8) + addr->sa_data[1];
      snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
               addr->sa_data[2] & 0xFF,
               addr->sa_data[3] & 0xFF,
               addr->sa_data[4] & 0xFF,
               addr->sa_data[5] & 0xFF);
    }
    else
    {
      port = 0;
      snprintf(ip, sizeof(ip), "IP unknown");
    }


    //
    // ConnectionInfo
    //
    if ((ciP = new ConnectionInfo(url, method, version, connection)) == NULL)
    {
      LM_E(("Runtime Error (error allocating ConnectionInfo)"));
      return MHD_NO;
    }

    *con_cls     = (void*) ciP; // Pointer to ConnectionInfo for subsequent calls
    ciP->port    = port;
    ciP->ip      = ip;
    ciP->callNo  = reqNo;

    ++reqNo;

    //
    // Transaction starts here
    //
    LM_TRANSACTION_START("from", ip, port, url);  // Incoming REST request starts



    //
    // URI parameters
    // 
    ciP->uriParam[URI_PARAM_NOTIFY_FORMAT]      = DEFAULT_PARAM_NOTIFY_FORMAT;
    ciP->uriParam[URI_PARAM_PAGINATION_OFFSET]  = DEFAULT_PAGINATION_OFFSET;
    ciP->uriParam[URI_PARAM_PAGINATION_LIMIT]   = DEFAULT_PAGINATION_LIMIT;
    ciP->uriParam[URI_PARAM_PAGINATION_DETAILS] = DEFAULT_PAGINATION_DETAILS;
    
    MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, &ciP->httpHeaders);
    if (!ciP->httpHeaders.servicePathReceived)
    {
      ciP->httpHeaders.servicePath = defaultServicePath(url, method);
    }

    ciP->apiVersion = apiVersionGet(ciP->url.c_str());

    char tenant[128];
    ciP->tenantFromHttpHeader = strToLower(tenant, ciP->httpHeaders.tenant.c_str(), sizeof(tenant));
    ciP->outFormat            = wantedOutputSupported(ciP->apiVersion, ciP->httpHeaders.accept, &ciP->charset);
    if (ciP->outFormat == NOFORMAT)
      ciP->outFormat = XML; // XML is default output format

    if (urlCheck(ciP, ciP->url) == false)
    {
      LM_W(("Bad Input (error in URI path)"));
      restReply(ciP, ciP->answer);
      return MHD_YES;
    }

    ciP->servicePath = ciP->httpHeaders.servicePath;

    if (servicePathSplit(ciP) != 0)
    {
      LM_W(("Bad Input (error in ServicePath http-header)"));
      restReply(ciP, ciP->answer);
    }

    if (contentTypeCheck(ciP) != 0)
    {
      LM_W(("Bad Input (invalid mime-type in Content-Type http-header)"));
      restReply(ciP, ciP->answer);
    }
    else if (outFormatCheck(ciP) != 0)
    {
      LM_W(("Bad Input (invalid mime-type in Accept http-header)"));
      restReply(ciP, ciP->answer);
    }
    else
      ciP->inFormat = formatParse(ciP->httpHeaders.contentType, NULL);

    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);
    if (ciP->httpStatusCode != SccOk)
    {
      LM_W(("Bad Input (error in URI parameters)"));
      restReply(ciP, ciP->answer);
      return MHD_YES;
    }
    LM_T(LmtUriParams, ("notifyFormat: '%s'", ciP->uriParam[URI_PARAM_NOTIFY_FORMAT].c_str()));

    // Set default mime-type for notifications
    if (ciP->uriParam[URI_PARAM_NOTIFY_FORMAT] == "")
    {
      if (ciP->outFormat == XML)
        ciP->uriParam[URI_PARAM_NOTIFY_FORMAT] = "XML";
      else if (ciP->outFormat == JSON)
        ciP->uriParam[URI_PARAM_NOTIFY_FORMAT] = "JSON";
      else
        ciP->uriParam[URI_PARAM_NOTIFY_FORMAT] = DEFAULT_FORMAT_AS_STRING;

      LM_T(LmtUriParams, ("'default' value for notifyFormat (ciP->outFormat == %d)): '%s'", ciP->outFormat, ciP->uriParam[URI_PARAM_NOTIFY_FORMAT].c_str()));
    }

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
    if (dataLen == ciP->httpHeaders.contentLength)
    {
      if (ciP->httpHeaders.contentLength <= PAYLOAD_MAX_SIZE)
      {
        if (ciP->httpHeaders.contentLength > STATIC_BUFFER_SIZE)
          ciP->payload = (char*) malloc(ciP->httpHeaders.contentLength + 1);
        else
          ciP->payload = static_buffer;

        ciP->payloadSize = dataLen;
        memcpy(ciP->payload, upload_data, dataLen);
        ciP->payload[dataLen] = 0;
      }
      else
      {
        char details[256];
        snprintf(details, sizeof(details), "payload size: %d, max size supported: %d", ciP->httpHeaders.contentLength, PAYLOAD_MAX_SIZE);

        ciP->answer         = restErrorReplyGet(ciP, ciP->outFormat, "", ciP->url, SccRequestEntityTooLarge, details);
        ciP->httpStatusCode = SccRequestEntityTooLarge;
      }

      // All payload received, acknowledge!
      *upload_data_size = 0;
    }
    else
      LM_T(LmtPartialPayload, ("Got %d of payload of %d bytes", dataLen, ciP->httpHeaders.contentLength));

    return MHD_YES;
  }


  // 3. Finally, serve the request (unless an error has occurred)
  if (((ciP->verb == POST) || (ciP->verb == PUT)) && (ciP->httpHeaders.contentLength == 0) && (strncasecmp(ciP->url.c_str(), "/log/", 5) != 0))
  {
    std::string errorMsg = restErrorReplyGet(ciP, ciP->outFormat, "", url, SccLengthRequired, "Zero/No Content-Length in PUT/POST request");
    ciP->httpStatusCode = SccLengthRequired;
    restReply(ciP, errorMsg);
  }
  else if (ciP->answer != "")
  {
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
*/
static int restStart(IpVersion ipVersion, const char* httpsKey = NULL, const char* httpsCertificate = NULL)
{
  bool   mhdStartError = true;
  size_t memoryLimit   = 2 * PAYLOAD_SIZE;

  if (port == 0)
  {
    LM_X(1, ("Fatal Error (please call restInit before starting the REST service)"));
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
      mhdDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL, // MHD_USE_SELECT_INTERNALLY
                                   htons(port),
                                   NULL,
                                   NULL,
                                   connectionTreat,                     NULL,
                                   MHD_OPTION_HTTPS_MEM_KEY,            httpsKey,
                                   MHD_OPTION_HTTPS_MEM_CERT,           httpsCertificate,
                                   MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                   MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad,
                                   MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                   MHD_OPTION_END);

    }
    else
    {
      LM_T(LmtMhd, ("Starting HTTP daemon on IPv4 %s port %d", bindIp, port));
      mhdDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, // MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG
                                   htons(port),
                                   NULL,
                                   NULL,
                                   connectionTreat,                     NULL,
                                   MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
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
      mhdDaemon_v6 = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_IPv6 | MHD_USE_SSL,
                                      htons(port),
                                      NULL,
                                      NULL,
                                      connectionTreat,                     NULL,
                                      MHD_OPTION_HTTPS_MEM_KEY,            httpsKey,
                                      MHD_OPTION_HTTPS_MEM_CERT,           httpsCertificate,
                                      MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
                                      MHD_OPTION_SOCK_ADDR,                (struct sockaddr*) &sad_v6,
                                      MHD_OPTION_NOTIFY_COMPLETED,         requestCompleted, NULL,
                                      MHD_OPTION_END);
    }
    else
    {
      LM_T(LmtMhd, ("Starting HTTP daemon on IPv6 %s port %d", bindIPv6, port));
      mhdDaemon_v6 = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_IPv6,
                                      htons(port),
                                      NULL,
                                      NULL,
                                      connectionTreat,                     NULL,
                                      MHD_OPTION_CONNECTION_MEMORY_LIMIT,  memoryLimit,
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
*           argument _acceptTextXml that was added for iotAgent only.
*           See Issue #256
*/
void restInit
(
  RestService*        _restServiceV,
  IpVersion           _ipVersion,
  const char*         _bindAddress,
  unsigned short      _port,
  bool                _multitenant,
  const std::string&  _rushHost,
  unsigned short      _rushPort,
  const char*         _allowedOrigin,
  const char*         _httpsKey,
  const char*         _httpsCertificate,
  RestServeFunction   _serveFunction,
  bool                _acceptTextXml
)
{
  const char* key  = _httpsKey;
  const char* cert = _httpsCertificate;

  port          = _port;
  restServiceV  = _restServiceV;
  ipVersionUsed = _ipVersion;
  serveFunction = (_serveFunction != NULL)? _serveFunction : serve;
  acceptTextXml = _acceptTextXml;
  multitenant   = _multitenant;
  rushHost      = _rushHost;
  rushPort      = _rushPort;

  strncpy(restAllowedOrigin, _allowedOrigin, sizeof(restAllowedOrigin));

  strncpy(bindIp, LOCAL_IP_V4, MAX_LEN_IP - 1);
  strncpy(bindIPv6, LOCAL_IP_V6, MAX_LEN_IP - 1);

  if (isIPv6(std::string(_bindAddress)))
    strncpy(bindIPv6, _bindAddress, MAX_LEN_IP - 1);
  else
    strncpy(bindIp, _bindAddress, MAX_LEN_IP - 1);

#if 0
  // FIXME P7: string copy from one var to the very same var ...

  if ((_ipVersion == IPV4) || (_ipVersion == IPDUAL))
     strncpy(bindIp, bindIp, MAX_LEN_IP - 1);

  if ((_ipVersion == IPV6) || (_ipVersion == IPDUAL))
     strncpy(bindIPv6, bindIPv6, MAX_LEN_IP - 1);
#endif

  // Starting REST interface
  int r;
  if ((r = restStart(_ipVersion, key, cert)) != 0)
  {
    fprintf(stderr, "restStart: error %d\n", r);
    orionExitFunction(1, "restStart: error");
  }
}
