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
* Author: developer
*/
#include <unistd.h>                             // close()
#include <sys/types.h>                          // system types ...
#include <sys/socket.h>                         // socket, bind, listen
#include <sys/un.h>                             // sockaddr_un
#include <netinet/in.h>                         // struct sockaddr_in
#include <netdb.h>                              // gethostbyname
#include <arpa/inet.h>                          // inet_ntoa
#include <netinet/tcp.h>                        // TCP_NODELAY
#include <curl/curl.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "common/string.h"
#include "common/sem.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/ConnectionInfo.h"
#include "rest/httpRequestSend.h"
#include "rest/rest.h"
#include "serviceRoutines/versionTreat.h"




/* ****************************************************************************
*
* HTTP header maximum lengths
*/
#define CURL_VERSION_MAX_LENGTH             128
#define HTTP_HEADER_USER_AGENT_MAX_LENGTH   256
#define HTTP_HEADER_HOST_MAX_LENGTH         256



/* ****************************************************************************
*
* Default timeout - 5000 milliseconds
*/
#define DEFAULT_TIMEOUT     5000



/* ****************************************************************************
*
* defaultTimeout - 
*/
static long defaultTimeout = DEFAULT_TIMEOUT;



/* ****************************************************************************
*
* httpRequestInit - 
*/
void httpRequestInit(long defaultTimeoutInMilliseconds)
{
  if (defaultTimeoutInMilliseconds != -1)
  {
    defaultTimeout = defaultTimeoutInMilliseconds;
  }
}



/* **************************************************************************** 
*
* See [1] for a discussion on how curl_multi is to be used. Libcurl does not seem
* to provide a way to do asynchronous HTTP transactions in the way we intended
* with the previous version of httpRequestSend. To enable the old behavior of asynchronous
* HTTP requests uncomment the following #define line.
*
* [1] http://stackoverflow.com/questions/24288513/how-to-do-curl-multi-perform-asynchronously-in-c
*/

struct MemoryStruct
{
  char*   memory;
  size_t  size;
};



/* ****************************************************************************
*
* writeMemoryCallback -
*/
size_t writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  size_t realsize = size * nmemb;
  MemoryStruct* mem = (MemoryStruct *) userp;

  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL)
  {
    LM_E(("Runtime Error (out of memory)"));
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}



/* ****************************************************************************
*
* curlVersionGet - 
*/
static char* curlVersionGet(char* buf, int bufLen)
{
  curl_version_info_data* idP;

  idP = curl_version_info(CURLVERSION_NOW);

  snprintf(buf, bufLen, "%s", idP->version);

  return buf;
}

/* ****************************************************************************
*
* httpRequestSendWithCurl -
*
* The waitForResponse arguments specifies if the method has to wait for response
* before return. If this argument is false, the return string is ""
*
* NOTE
* We are using a hybrid approach, consisting in a static thread-local buffer of a
* small size that copes with most notifications to avoid expensive
* calloc/free syscalls if the notification payload is not very large.
*
* RETURN VALUES
*   httpRequestSendWithCurl returns 0 on success and a negative number on failure:
*     -1: Invalid port
*     -2: Invalid IP
*     -3: Invalid verb
*     -4: Invalid resource
*     -5: No Content-Type BUT content present
*     -6: Content-Type present but there is no content
*     -7: Total outgoing message size is too big
*/
int httpRequestSendWithCurl
(
   CURL                   *curl,
   const std::string&     _ip,
   unsigned short         port,
   const std::string&     protocol,
   const std::string&     verb,
   const std::string&     tenant,
   const std::string&     servicePath,
   const std::string&     xauthToken,
   const std::string&     resource,
   const std::string&     orig_content_type,
   const std::string&     content,
   bool                   useRush,
   bool                   waitForResponse,
   std::string*           outP,
   const std::string&     acceptFormat,
   long                   timeoutInMilliseconds
)
{
  char                       portAsString[16];
  static unsigned long long  callNo             = 0;
  std::string                result;
  std::string                ip                 = _ip;
  struct curl_slist*         headers            = NULL;
  MemoryStruct*              httpResponse       = NULL;
  CURLcode                   res;
  int                        outgoingMsgSize       = 0;
  std::string                content_type(orig_content_type);

  ++callNo;

  // For content-type application/json we add charset=utf-8
  if (orig_content_type == "application/json")
  {
    content_type += "; charset=utf-8";
  }

  if (timeoutInMilliseconds == -1)
  {
    timeoutInMilliseconds = defaultTimeout;
  }

  lmTransactionStart("to", ip.c_str(), port, resource.c_str());

  // Preconditions check
  if (port == 0)
  {
    LM_E(("Runtime Error (port is ZERO)"));
    lmTransactionEnd();

    *outP = "error";
    return -1;
  }

  if (ip.empty())
  {
    LM_E(("Runtime Error (ip is empty)"));
    lmTransactionEnd();

    *outP = "error";
    return -2;
  }

  if (verb.empty())
  {
    LM_E(("Runtime Error (verb is empty)"));
    lmTransactionEnd();

    *outP = "error";
    return -3;
  }

  if (resource.empty())
  {
    LM_E(("Runtime Error (resource is empty)"));
    lmTransactionEnd();

    *outP = "error";
    return -4;
  }

  if ((content_type.empty()) && (!content.empty()))
  {
    LM_E(("Runtime Error (Content-Type is empty but there is actual content)"));
    lmTransactionEnd();

    *outP = "error";
    return -5;
  }

  if ((!content_type.empty()) && (content.empty()))
  {
    LM_E(("Runtime Error (Content-Type non-empty but there is no content)"));
    lmTransactionEnd();

    *outP = "error";
    return -6;
  }



  // Allocate to hold HTTP response
  httpResponse = new MemoryStruct;
  httpResponse->memory = (char*) malloc(1); // will grow as needed
  httpResponse->size = 0; // no data at this point

  //
  // Rush
  // Every call to httpRequestSend specifies whether RUSH should be used or not.
  // But, this depends also on how the broker was started, so here the 'useRush'
  // parameter is cancelled in case the broker was started without rush.
  //
  // If rush is to be used, the IP/port is stored in rushHeaderIP/rushHeaderPort and
  // after that, the host and port of rush is set as ip/port for the message, that is
  // now sent to rush instead of to its final destination.
  // Also, a few HTTP headers for rush must be setup.
  //
  if ((rushPort == 0) || (rushHost == ""))
  {
    useRush = false;
  }

  if (useRush)
  {
    char         rushHeaderPortAsString[16];
    uint16_t     rushHeaderPort     = port;
    std::string  rushHeaderIP       = ip;
    std::string  headerRushHttp;

    ip    = rushHost;
    port  = rushPort;

    snprintf(rushHeaderPortAsString, sizeof(rushHeaderPortAsString), "%d", rushHeaderPort);
    headerRushHttp = "X-relayer-host: " + rushHeaderIP + ":" + rushHeaderPortAsString;
    LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", headerRushHttp.c_str()));
    headers = curl_slist_append(headers, headerRushHttp.c_str());
    outgoingMsgSize += headerRushHttp.size();

    if (protocol == "https:")
    {
      headerRushHttp = "X-relayer-protocol: https";
      LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", headerRushHttp.c_str()));
      headers = curl_slist_append(headers, headerRushHttp.c_str());
      outgoingMsgSize += headerRushHttp.size();
    }
  }

  snprintf(portAsString, sizeof(portAsString), "%u", port);

  // ----- User Agent
  char cvBuf[CURL_VERSION_MAX_LENGTH];
  char headerUserAgent[HTTP_HEADER_USER_AGENT_MAX_LENGTH];

  snprintf(headerUserAgent, sizeof(headerUserAgent), "User-Agent: orion/%s libcurl/%s", versionGet(), curlVersionGet(cvBuf, sizeof(cvBuf)));
  LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", headerUserAgent));
  headers = curl_slist_append(headers, headerUserAgent);
  outgoingMsgSize += strlen(headerUserAgent) + 1;

  // ----- Host
  char headerHost[HTTP_HEADER_HOST_MAX_LENGTH];

  snprintf(headerHost, sizeof(headerHost), "Host: %s:%d", ip.c_str(), (int) port);
  LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", headerHost));
  headers = curl_slist_append(headers, headerHost);
  outgoingMsgSize += strlen(headerHost) + 1;

  // ----- Tenant
  if (tenant != "")
  {
    headers = curl_slist_append(headers, ("fiware-service: " + tenant).c_str());
    outgoingMsgSize += tenant.size() + 16; // "fiware-service: "
  }

  // ----- Service-Path
  if (servicePath != "")
  {
    headers = curl_slist_append(headers, ("Fiware-ServicePath: " + servicePath).c_str());
    outgoingMsgSize += servicePath.size() + strlen("Fiware-ServicePath: ");
  }

  // ----- X-Auth-Token
  if (xauthToken != "")
  {
    headers = curl_slist_append(headers, ("X-Auth-Token: " + xauthToken).c_str());
    outgoingMsgSize += xauthToken.size() + strlen("X-Auth-Token: ");
  }

  // ----- Accept
  std::string acceptedFormats = "application/xml, application/json";
  if (acceptFormat != "")
  {
    acceptedFormats = acceptFormat;
  }

  std::string acceptString = "Accept: " + acceptedFormats;
  headers = curl_slist_append(headers, acceptString.c_str());
  outgoingMsgSize += acceptString.size();

  // ----- Expect
  headers = curl_slist_append(headers, "Expect: ");
  outgoingMsgSize += 8; // from "Expect: "

  // ----- Content-length
  std::stringstream contentLengthStringStream;
  contentLengthStringStream << content.size();
  std::string headerContentLength = "Content-length: " + contentLengthStringStream.str();
  LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", headerContentLength.c_str()));
  headers = curl_slist_append(headers, headerContentLength.c_str());
  outgoingMsgSize += contentLengthStringStream.str().size() + 16; // from "Content-length: "
  outgoingMsgSize += content.size();

  // ----- Content-type
  headers = curl_slist_append(headers, ("Content-type: " + content_type).c_str());
  outgoingMsgSize += content_type.size() + 14; // from "Content-type: "


  // Check if total outgoing message size is too big
  if (outgoingMsgSize > MAX_DYN_MSG_SIZE)
  {
    LM_E(("Runtime Error (HTTP request to send is too large: %d bytes)", outgoingMsgSize));

    curl_slist_free_all(headers);

    free(httpResponse->memory);
    delete httpResponse;

    lmTransactionEnd();
    *outP = "error";
    return -7;
  }

  // Contents
  const char* payload = content.c_str();
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (u_int8_t*) payload);

  // Set up URL
  std::string url;
  if (isIPv6(ip))
    url = "[" + ip + "]";
  else
    url = ip;
  url = url + ":" + portAsString + (resource.at(0) == '/'? "" : "/") + resource;

  // Prepare CURL handle with obtained options
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, verb.c_str()); // Set HTTP verb
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirection (?)
  curl_easy_setopt(curl, CURLOPT_HEADER, 1); // Activate include the header in the body output
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Put headers in place
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeMemoryCallback); // Send data here
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) httpResponse); // Custom data for response handling

  //
  // There is a known problem in libcurl (see http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame)
  // which is solved using CURLOPT_NOSIGNAL. If we update some day from libcurl 7.19 (the one that comes with CentOS 6.x) to a newer version
  // (there are some reports about the bug is no longer in libcurl 7.32), using CURLOPT_NOSIGNAL could be not necessary and this be removed).
  // See issue #1016 for more details.
  //
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

  //
  // Timeout 
  //
  // The parameter timeoutInMilliseconds holds the timeout time in milliseconds.
  // If the timeout time requested is 0, then no timeuot is used.
  //
  if (timeoutInMilliseconds != 0) 
  {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutInMilliseconds);
  }


  // Synchronous HTTP request
  LM_T(LmtClientOutputPayload, ("Sending message %lu to HTTP server: sending message of %d bytes to HTTP server", callNo, outgoingMsgSize));
  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
  {
    //
    // NOTE: This log line is used by the functional tests in cases/880_timeout_for_forward_and_notifications/
    //       So, this line should not be removed/altered, at least not without also modifying the functests.
    //
    LM_W(("Notification failure for %s:%s (curl_easy_perform failed: %s)", ip.c_str(), portAsString, curl_easy_strerror(res)));
    *outP = "notification failure";
  }
  else
  {
    // The Response is here
    LM_I(("Notification Successfully Sent to %s", url.c_str()));
    outP->assign(httpResponse->memory, httpResponse->size);
  }

  // Cleanup curl environment

  curl_slist_free_all(headers);

  free(httpResponse->memory);
  delete httpResponse;

  lmTransactionEnd();

  return 0;
}



/* ****************************************************************************
*
* httpRequestSend - 
*
* RETURN VALUES
*   httpRequestSend returns 0 on success and a negative number on failure:
*     -1: Invalid port
*     -2: Invalid IP
*     -3: Invalid verb
*     -4: Invalid resource
*     -5: No Content-Type BUT content present
*     -6: Content-Type present but there is no content
*     -7: Total outgoing message size is too big
*     -8: Unable to initialize libcurl
*
*   [ error codes -1 to -7 comes from httpRequestSendWithCurl ]
*/
int httpRequestSend
(
   const std::string&     _ip,
   unsigned short         port,
   const std::string&     protocol,
   const std::string&     verb,
   const std::string&     tenant,
   const std::string&     servicePath,
   const std::string&     xauthToken,
   const std::string&     resource,
   const std::string&     orig_content_type,
   const std::string&     content,
   bool                   useRush,
   bool                   waitForResponse,
   std::string*           outP,
   const std::string&     acceptFormat,
   long                   timeoutInMilliseconds
)
{
  struct curl_context  cc;
  int                  response;

  get_curl_context(_ip, &cc);
  if (cc.curl == NULL)
  {
    release_curl_context(&cc);
    LM_E(("Runtime Error (could not init libcurl)"));
    lmTransactionEnd();

    *outP = "error";
    return -8;
  }

  response = httpRequestSendWithCurl(cc.curl,
                                     _ip,
                                     port,
                                     protocol,
                                     verb,
                                     tenant,
                                     servicePath,
                                     xauthToken,
                                     resource,
                                     orig_content_type,
                                     content,
                                     useRush,
                                     waitForResponse,
                                     outP,
                                     acceptFormat,
                                     timeoutInMilliseconds);
  release_curl_context(&cc);
  return response;
}
