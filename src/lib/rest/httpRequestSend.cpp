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
#include <string.h>                             // strchr()
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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/limits.h"
#include "common/defaultValues.h"
#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "rest/ConnectionInfo.h"
#include "rest/httpRequestSend.h"
#include "rest/HttpHeaders.h"
#include "rest/rest.h"
#include "rest/curlSem.h"
#include "serviceRoutinesV2/versionTreat.h"



/* ****************************************************************************
*
* defaultTimeout -
*/
static long defaultTimeout = DEFAULT_TIMEOUT;



/* ****************************************************************************
*
* setHttpTimeout -
*/
void setHttpTimeout(long defaultTimeoutInMilliseconds)
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
* httpHeaderAdd - add HTTP header to the list of HTTP headers for curl
*
* PARAMETERS
*   o headersP            pointer to the list of HTTP headers, to be used by curl
*   o headerName          The name of the header, e.g. "Content-Type"
*   o headerValue         The value of the header, e.g. "application/json"
*   o headerTotalSizeP    Pointer to a variable holding the total size of the list of headers,
*                         the string length of the list. To this variable, the string length of the added header must be added.
*   o extraHeaders        list of extra headers that were asked for when creating the subscription.
*                         We need this variable here in case the user overloaded any standard header.
*   o usedExtraHeaders    list of headers already used, so that any overloaded standard header are not present twice in the notification.
*
*/
static void httpHeaderAdd
(
  struct curl_slist**                        headersP,
  const std::string&                         headerName,
  const std::string&                         headerValue,
  int*                                       headerTotalSizeP,
  const std::map<std::string, std::string>&  extraHeaders,
  std::map<std::string, bool>&               usedExtraHeaders
)
{

  std::string headerNameLowerCase = headerName;
  std::transform(headerNameLowerCase.begin(), headerNameLowerCase.end(), headerNameLowerCase.begin(), ::tolower);

  std::string  h = headerName + ": " + headerValue;

  // Fiware-Correlator and Ngsiv2-AttrsFormat cannot be overwritten, so we don't
  // search in extraHeaders in these cases
  if ((headerName != HTTP_FIWARE_CORRELATOR) && (headerName != HTTP_NGSIV2_ATTRSFORMAT))
  {
    std::map<std::string, std::string>::const_iterator it;
    it = extraHeaders.find(headerNameLowerCase);
    if (it != extraHeaders.end())  // headerName found in extraHeaders, use matching map value
    {
      h = headerName + ": " + it->second;
      usedExtraHeaders[headerNameLowerCase.c_str()] = true;
    }
  }

  *headersP           = curl_slist_append(*headersP, h.c_str());
  *headerTotalSizeP  += h.size();
}


/* ****************************************************************************
*
* contentLenParse - extract content length fromn HTTP header string
*
* To get the size of the payload, simply search for "Content-Length"
* inside the HTTP headers and extract the number.
*/
static int contentLenParse(char* s)
{
  char* contentLenP  = strstr(s, "Content-Length:");             // Point to beginning of 'Content-Length:'

  if (contentLenP == NULL)
  {
    return 0;
  }

  int offset = strlen("Content-Length:");

  while ((contentLenP[offset] == ' ') || (contentLenP[offset] == '\t'))        // Step over spaces and tabs ...
  {
    ++offset;
  }


  return atoi(&contentLenP[offset]);  // ... and get the number
}



/* ****************************************************************************
*
* httpRequestSend -
*
* NOTE
* We are using a hybrid approach, consisting of a static thread-local buffer of a
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
*     -8: Unable to initialize libcurl (NOTE: only possible if actual curl is not provided as first argument)
*     -9: Error making HTTP request
*
*/
int httpRequestSend
(
   CURL*                                      _curl,
   const std::string&                         idStringForLogs,
   const std::string&                         from,
   const std::string&                         _ip,
   unsigned short                             port,
   const std::string&                         _protocol,
   const std::string&                         verb,
   const std::string&                         tenant,
   const std::string&                         servicePath,
   const std::string&                         xauthToken,
   const std::string&                         resource,
   const std::string&                         orig_content_type,
   const std::string&                         content,
   const std::string&                         fiwareCorrelation,
   const std::string&                         ngsiv2AttrFormat,
   std::string*                               outP,
   long long*                                 statusCodeP,
   const std::map<std::string, std::string>&  extraHeaders,
   const std::string&                         acceptFormat,
   long                                       timeoutInMilliseconds,
   int                                        providerLimit,
   int                                        providerOffset
)
{
  CURL*                curl;
  struct curl_context  cc;
  char                 servicePath0[SERVICE_PATH_MAX_COMPONENT_LEN + 1];  // +1 for zero termination

  firstServicePath(servicePath.c_str(), servicePath0, sizeof(servicePath0));

  if (_curl == NULL)
  {
    // if curl object has not been provided as paramter, then we create it
    get_curl_context(_ip, &cc);

    if (cc.curl == NULL)
    {
      metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT,        1);
      metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);

      release_curl_context(&cc);
      LM_E(("Runtime Error (could not get curl context)"));

      *outP = "unable to get curl context";
      return -8;
    }

    curl = cc.curl;
  }
  else
  {
    curl = _curl;
  }

  char                            portAsString[STRING_SIZE_FOR_INT];
  static unsigned long long       callNo             = 0;
  std::string                     ip                 = _ip;
  struct curl_slist*              headers            = NULL;
  MemoryStruct*                   httpResponse       = NULL;
  CURLcode                        res;
  int                             outgoingMsgSize    = 0;
  std::string                     content_type(orig_content_type);
  std::map<std::string, bool>     usedExtraHeaders;

  metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT, 1);

  ++callNo;

  // For content-type application/json we add charset=utf-8
  if ((orig_content_type == "application/json") || (orig_content_type == "text/plain"))
  {
    content_type += "; charset=utf-8";
  }

  if (timeoutInMilliseconds <= 0)
  {
    timeoutInMilliseconds = defaultTimeout;
  }

  // Note that the corresponding lmTransactionEnd() is not done in this function, but in the caller, as we
  // need to wait to subNotificationErrorStatus() before ending (or the logs in this operation
  // will show unwanted N/A fields)
  std::string protocol = _protocol + "//";
  correlatorIdSet(fiwareCorrelation.c_str());
  lmTransactionStart("to", protocol.c_str(), + ip.c_str(), port, resource.c_str(),
                     tenant.c_str(), servicePath.c_str(), from.c_str());

  // Preconditions check
  if (port == 0)
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (port is ZERO)"));

    *outP = "invalid port";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -1;
  }

  if (ip.empty())
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (ip is empty)"));

    *outP = "invalid IP";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -2;
  }

  if (verb.empty())
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (verb is empty)"));

    *outP = "invalid verb";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -3;
  }

  if (resource.empty())
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (resource is empty)"));

    *outP = "invalid resource";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -4;
  }

  if ((content_type.empty()) && (!content.empty()))
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (Content-Type is empty but there is actual content)"));

    *outP = "no Content-Type but content present";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -5;
  }

  if ((!content_type.empty()) && (content.empty()))
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (Content-Type non-empty but there is no content)"));

    *outP = "Content-Type present but there is no content";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -6;
  }



  // Allocate to hold HTTP response
  httpResponse         = new MemoryStruct;
  httpResponse->memory = (char*) malloc(1); // will grow as needed
  httpResponse->size   = 0; // no data at this point

  snprintf(portAsString, sizeof(portAsString), "%u", port);

  // ----- User Agent
  char        cvBuf[CURL_VERSION_MAX_LENGTH];
  char        userAgentHeaderValue[HTTP_HEADER_USER_AGENT_MAX_LENGTH];
  std::string userAgentHeaderName = HTTP_USER_AGENT;

  snprintf(userAgentHeaderValue, sizeof(userAgentHeaderValue), "orion/%s libcurl/%s", versionGet(), curlVersionGet(cvBuf, sizeof(cvBuf)));
  LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", (userAgentHeaderName + ": " + userAgentHeaderValue).c_str()));
  httpHeaderAdd(&headers, userAgentHeaderName, userAgentHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // ----- Host
  char        hostHeaderValue[HTTP_HEADER_HOST_MAX_LENGTH];
  std::string hostHeaderName = HTTP_HOST;

  snprintf(hostHeaderValue, sizeof(hostHeaderValue), "%s:%d", ip.c_str(), (int) port);
  LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", (hostHeaderName + ": " + hostHeaderValue).c_str()));
  httpHeaderAdd(&headers, hostHeaderName, hostHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // ----- Tenant
  if (!tenant.empty())
  {
    std::string fiwareServiceHeaderValue = tenant;
    httpHeaderAdd(&headers, HTTP_FIWARE_SERVICE, fiwareServiceHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);
  }

  // ----- Service-Path
  std::string fiwareServicePathHeaderValue =  (servicePath.empty())? "/" : servicePath;
  httpHeaderAdd(&headers, HTTP_FIWARE_SERVICEPATH, fiwareServicePathHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // ----- X-Auth-Token
  if (!xauthToken.empty())
  {
    std::string xauthTokenHeaderValue = xauthToken;
    httpHeaderAdd(&headers, HTTP_X_AUTH_TOKEN, xauthTokenHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);
  }

  // ----- Accept
  std::string acceptedFormats = "application/json";
  if (!acceptFormat.empty())
  {
    acceptedFormats = acceptFormat;
  }

  std::string acceptHeaderValue = acceptedFormats;
  httpHeaderAdd(&headers, HTTP_ACCEPT, acceptHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // ----- Expect
  httpHeaderAdd(&headers, HTTP_EXPECT, " ", &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // ----- Content-length
  std::stringstream contentLengthStringStream;
  contentLengthStringStream << content.size();
  std::string contentLengthHeaderName  = HTTP_CONTENT_LENGTH;
  std::string contentLengthHeaderValue = contentLengthStringStream.str();

  LM_T(LmtHttpHeaders, ("HTTP-HEADERS: '%s'", (contentLengthHeaderName + ": " + contentLengthHeaderValue).c_str()));
  httpHeaderAdd(&headers, contentLengthHeaderName, contentLengthHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  //
  // Add the size of the actual payload
  //
  // Note that 'outgoingMsgSize' is the TOTAL size of the outgoing message,
  // including HTTP headers etc, while 'payloadSize' is the size of just
  // the payload of the message.
  //
  unsigned long long payloadSize = content.size();
  outgoingMsgSize += payloadSize;


  // ----- Content-type
  std::string contentTypeHeaderValue = content_type;

  httpHeaderAdd(&headers, HTTP_CONTENT_TYPE, contentTypeHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // Fiware-Correlator
  std::string correlationHeaderValue = fiwareCorrelation;
  httpHeaderAdd(&headers, HTTP_FIWARE_CORRELATOR, correlationHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);

  // Notify Format
  if ((!ngsiv2AttrFormat.empty()) && (ngsiv2AttrFormat != "JSON") && (ngsiv2AttrFormat != "legacy"))
  {
    std::string nFormatHeaderValue = ngsiv2AttrFormat;

    httpHeaderAdd(&headers, HTTP_NGSIV2_ATTRSFORMAT, nFormatHeaderValue, &outgoingMsgSize, extraHeaders, usedExtraHeaders);
  }

  // Extra headers
  for (std::map<std::string, std::string>::const_iterator it = extraHeaders.begin(); it != extraHeaders.end(); ++it)
  {
    std::string headerNameLowerCase = it->first;
    transform(headerNameLowerCase.begin(), headerNameLowerCase.end(), headerNameLowerCase.begin(), ::tolower);

    // Ngsiv2-AttrsFormat and Fiware-Correlator cannot be overwritten
    if ((headerNameLowerCase == "ngsiv2-attrsformat") || (headerNameLowerCase == "fiware-correlator"))
    {
      continue;
    }

    if (!usedExtraHeaders[headerNameLowerCase])
    {
      std::string header = it->first + ": " + it->second;

      headers            = curl_slist_append(headers, header.c_str());
      outgoingMsgSize   += header.size();
    }
  }


  //
  // FIXME: outgoingMsgSize is a signed int. Max size ~2GB. Might not be enough.
  //        Might be a good idea to change it to 'unsigned long long'
  //

  //
  // Check if total outgoing message size is too big
  //
  if ((unsigned long long) outgoingMsgSize > outReqMsgMaxSize)
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
    LM_E(("Runtime Error (HTTP request to send is too large: %d bytes)", outgoingMsgSize));

    curl_slist_free_all(headers);

    free(httpResponse->memory);
    delete httpResponse;

    *outP = "total outgoing message size is too big";

    if (_curl == NULL)
    {
      release_curl_context(&cc);
    }

    return -7;
  }

  // Contents
  const char* payload = content.c_str();
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (u_int8_t*) payload);

  // Set up URL
  std::string url;
  if (isIPv6(ip))
  {
    url = "[" + ip + "]";
  }
  else
  {
    url = ip;
  }
  url = protocol + url + ":" + portAsString + (resource.at(0) == '/'? "" : "/") + resource;

  if ((providerLimit >= 0) && (providerOffset >=0))
  {
    std::string pLimit = std::to_string(providerLimit);
    std::string pOffset = std::to_string(providerOffset);
    url += "?limit=" + pLimit + "&offset=" + pOffset + "&options=count";
  }

  if (insecureNotif)
  {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // ignore self-signed certificates for SSL end-points
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  }

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
  if (timeoutInMilliseconds > 0)
  {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutInMilliseconds);
  }


  //
  // Synchronous HTTP request
  //
  // This was previously an LM_T trace, but we have "promoted" it to INFO due to it is needed
  // to check logs in a .test case (case 000 notification_different_sizes.test)
  //
  LM_T(LmtOldInfo, ("Sending message %lu to HTTP server: sending message of %d bytes to HTTP server", callNo, outgoingMsgSize));

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    //
    // NOTE: This log line is used by the functional tests in cases/880_timeout_for_forward_and_notifications/
    //       So, this line should not be removed/altered, at least not without also modifying the functests.
    //
    *outP = std::string(curl_easy_strerror(res));

    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_ERRORS, 1);
  }
  else
  {
    //
    // The Response is here
    //
    int   payloadLen  = contentLenParse(httpResponse->memory);

    LM_T(LmtOldInfo, ("Notification Successfully Sent to %s", url.c_str()));
    outP->assign(httpResponse->memory, httpResponse->size);

    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_RESP_SIZE, payloadLen);

    // Get and log response code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, statusCodeP);
    if ((*statusCodeP < 300) && (*statusCodeP > 199))
    {
      LM_T(LmtOldInfo, ("Notification (%s) response OK, http code: %d", idStringForLogs.c_str(), *statusCodeP));
    }
    else
    {
      LM_W(("Notification (%s) response NOT OK, http code: %d", idStringForLogs.c_str(), *statusCodeP));
    }
  }

  if (payloadSize > 0)
  {
    metricsMgr.add(tenant, servicePath0, METRIC_TRANS_OUT_REQ_SIZE, payloadSize);
  }

  // Cleanup curl environment

  curl_slist_free_all(headers);

  free(httpResponse->memory);
  delete httpResponse;

  if (_curl == NULL)
  {
    release_curl_context(&cc);
  }

  return res == CURLE_OK ? 0 : -9;
}
