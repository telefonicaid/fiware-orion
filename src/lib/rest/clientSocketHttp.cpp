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
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/ConnectionInfo.h"
#include "rest/clientSocketHttp.h"
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
* See [1] for a discussion on how curl_multi is to be used. Libcurl does not seem
* to provide a way to do asynchronous HTTP transactions in the way we intended
* with the previous version of sendHttpSocket. To enable the old behavior of asynchronous
* HTTP requests uncomment the following #define line.
*
* [1] http://stackoverflow.com/questions/24288513/how-to-do-curl-multi-perform-asynchronously-in-c
*/
//#define USE_OLD_SENDHTTPSOCKET
#ifndef USE_OLD_SENDHTTPSOCKET

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
* sendHttpRequest -
*/
std::string sendHttpSocket
(
   const std::string&     _ip,
   unsigned short         port,
   const std::string&     protocol,
   const std::string&     verb,
   const std::string&     tenant,
   const std::string&     servicePath,
   const std::string&     xauthToken,
   const std::string&     resource,
   const std::string&     content_type,
   const std::string&     content,
   bool                   useRush,
   bool                   waitForResponse
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
  CURL*                      curl;

  ++callNo;

  LM_TRANSACTION_START("to", ip.c_str(), port, resource.c_str());

  // Preconditions check
  if (port == 0)
  {
    LM_E(("Runtime Error (port is ZERO)"));
    LM_TRANSACTION_END();
    return "error";
  }

  if (ip.empty())
  {
    LM_E(("Runtime Error (ip is empty)"));
    LM_TRANSACTION_END();
    return "error";
  }

  if (verb.empty())
  {
    LM_E(("Runtime Error (verb is empty)"));
    LM_TRANSACTION_END();
    return "error";
  }

  if (resource.empty())
  {
    LM_E(("Runtime Error (resource is empty)"));
    LM_TRANSACTION_END();
    return "error";
  }

  if ((content_type.empty()) && (!content.empty()))
  {
    LM_E(("Runtime Error (Content-Type is empty but there is actual content)"));
    LM_TRANSACTION_END();
    return "error";
  }

  if ((!content_type.empty()) && (content.empty()))
  {
    LM_E(("Runtime Error (Content-Type non-empty but there is no content)"));
    LM_TRANSACTION_END();
    return "error";
  }

  if ((curl = curl_easy_init()) == NULL)
  {
    LM_E(("Runtime Error (could not init libcurl)"));
    LM_TRANSACTION_END();
    return "error";
  }

  // Allocate to hold HTTP response
  httpResponse = new MemoryStruct;
  httpResponse->memory = (char*) malloc(1); // will grow as needed
  httpResponse->size = 0; // no data at this point

  //
  // Rush
  // Every call to sendHttpSocket specifies whether RUSH should be used or not.
  // But, this depends also on how the broker was started, so here the 'useRush'
  // parameter is cancelled in case the broker was started without rush.
  //
  // If rush is to be used, the IP/port is stored in rushHeaderIP/rushHeaderPort and
  // after that, the host and port of rush is set as ip/port for the message, that is
  // now sent to rush instead of to its final destination.
  // Also, a few HTTP headers for rush must be setup.
  //
  if ((rushPort == 0) || (rushHost == ""))
    useRush = false;

  if (useRush)
  {
    char         rushHeaderPortAsString[16];
    uint16_t     rushHeaderPort     = 0;
    std::string  rushHeaderIP       = "";
    std::string  headerRushHttp     = "";

    rushHeaderIP   = ip;
    rushHeaderPort = port;
    ip             = rushHost;
    port           = rushPort;

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

  snprintf(portAsString, sizeof(portAsString), "%d", port);

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
  headers = curl_slist_append(headers, "Accept: application/xml, application/json");
  outgoingMsgSize += 41; // from "Accept: application/xml, application/json"

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

    // Cleanup curl environment
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    free(httpResponse->memory);
    delete httpResponse;

    LM_TRANSACTION_END();
    return "error";
  }

  // Contents
  const char* payload = content.c_str();
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (u_int8_t*) payload);

  // Set up URL
  std::string url = "";

  if (protocol == "https:" && !useRush)
  {
    url += protocol + "//";
  }

  if (isIPv6(ip))
  {
    url += "[" + ip + "]";
  }
  else
  {
    url += ip;
  }

  url = url + ":" + portAsString + (resource.at(0) == '/'? "" : "/") + resource;

  // FIXME: I wonder if these next two should be disabled by default.
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // ignore self-signed certificates for SSL end-points
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  // Prepare CURL handle with obtained options
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, verb.c_str()); // Set HTTP verb
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirection (?)
  curl_easy_setopt(curl, CURLOPT_HEADER, 1); // Activate include the header in the body output
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Put headers in place
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeMemoryCallback); // Send data here
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) httpResponse); // Custom data for response handling

  // Synchronous HTTP request
  LM_T(LmtClientOutputPayload, ("Sending message %lu to HTTP server: sending message of %d bytes to HTTP server", callNo, outgoingMsgSize));
  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
  {
    LM_W(("Notification failure for %s:%s (curl_easy_perform failed: %s)", ip.c_str(), portAsString, curl_easy_strerror(res)));
    result = "";
  }
  else
  {
    // The Response is here
    LM_I(("Notification Successfully Sent to %s", url.c_str()));
    result.assign(httpResponse->memory, httpResponse->size);
  }

  // Cleanup curl environment
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  free(httpResponse->memory);
  delete httpResponse;

  LM_TRANSACTION_END();
  return result;
}

#else // Old functionality()

/* ****************************************************************************
*
* socketHttpConnect -
*/
int socketHttpConnect(const std::string& host, unsigned short port)
{
  int                 fd;
  struct addrinfo     hints;
  struct addrinfo*    peer;
  char                port_str[10];

  LM_TRANSACTION_START("to", ip.c_str(), port, resource.c_str());

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  if (ipVersionUsed == IPV4)
  {
    hints.ai_family = AF_INET;
    LM_T(LmtIpVersion, ("Allow IPv4 only"));
  }
  else if (ipVersionUsed == IPV6)
  {
    hints.ai_family = AF_INET6;
    LM_T(LmtIpVersion, ("Allow  IPv6 only"));
  }
  else
  {
    hints.ai_family = AF_UNSPEC;
    LM_T(LmtIpVersion, ("Allow IPv4 or IPv6"));
  }

  snprintf(port_str, sizeof(port_str), "%d" , (int) port);

  if (getaddrinfo(host.c_str(), port_str, &hints, &peer) != 0)
  {
    LM_W(("Notification failure for %s:%d (getaddrinfo: %s)", host.c_str(), port, strerror(errno)));
    LM_TRANSACTION_END();
    return -1;
  }

  if ((fd = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol)) == -1)
  {
    LM_W(("Notification failure for %s:%d (socket: %s)", host.c_str(), port, strerror(errno)));
    LM_TRANSACTION_END();
    return -1;
  }

  if (connect(fd, peer->ai_addr, peer->ai_addrlen) == -1)
  {
    freeaddrinfo(peer);
    close(fd);
    LM_W(("Notification failure for %s:%d (connect: %s)", host.c_str(), port, strerror(errno)));
    LM_TRANSACTION_END();
    return -1;
  }

  freeaddrinfo(peer);
  LM_TRANSACTION_END();
  return fd;
}



/* ****************************************************************************
*
* sendHttpSocket -
*
* The waitForResponse arguments specifies if the method has to wait for response
* before return. If this argument is false, the return string is ""
*
* FIXME: I don't like too much "reusing" natural output to return "error" in the
* case of error. I think it would be smarter to use "std::string* error" in the
* arguments or (even better) an exception. To be solved in the future in a hardening
* period.
*
* Note, we are using a hybrid approach, consisting in an static thread-local buffer of
* small size that copes with most notifications to avoid expensive
* calloc/free syscalls if the notification payload is not very large.
*
*/
std::string sendHttpSocket
(
   const std::string&     _ip,
   unsigned short         port,
   const std::string&     protocol,
   const std::string&     verb,
   const std::string&     tenant,
   const std::string&     resource,
   const std::string&     content_type,
   const std::string&     content,
   bool                   useRush,
   bool                   waitForResponse
)
{  
  char                       buffer[TAM_BUF];
  char                       response[TAM_BUF];
  char                       preContent[TAM_BUF];
  char                       msgStatic[MAX_STA_MSG_SIZE];
  char*                      what               = (char*) "static";
  char*                      msgDynamic         = NULL;
  char*                      msg                = msgStatic;   // by default, use the static buffer
  std::string                rushHeaderIP       = "";
  unsigned short             rushHeaderPort     = 0;
  std::string                rushHttpHeaders    = "";
  static unsigned long long  callNo             = 0;
  std::string                result;
  std::string                ip                 = _ip;

  ++callNo;

  // Preconditions check
  if (port == 0)
  {
    LM_E(("Runtime Error (port is ZERO)"));
    return "error";
  }

  if (ip.empty())
  {
    LM_E(("Runtime Error (ip is empty)"));
    return "error";
  }

  if (verb.empty())
  {
    LM_E(("Runtime Error (verb is empty)"));
    return "error";
  }

  if (resource.empty())
  {
    LM_E(("Runtime Error (resource is empty)"));
    return "error";
  }

  if ((content_type.empty()) && (!content.empty()))
  {
    LM_E(("Runtime Error (Content-Type is empty but there is actual content)"));
    return "error";
  }

  if ((!content_type.empty()) && (content.empty()))
  {
    LM_E(("Runtime Error (Content-Type non-empty but there is no content)"));
    return "error";
  }

  //
  // Rush
  // Every call to sendHttpSocket specifies whether RUSH should be used or not.
  // But, this depends also on how the broker was started, so here the 'useRush'
  // parameter is cancelled in case the broker was started without rush.
  //
  // If rush is to be used, the IP/port is stored in rushHeaderIP/rushHeaderPort and
  // after that, the host and port of rush is set as ip/port for the message, that is
  // now sent to rush instead of to its final destination.
  // Also, a few HTTP headers for rush must be setup.
  //
  if (useRush)
  {
    if ((rushPort == 0) || (rushHost == ""))
      useRush = false;
    else
    {
      char portAsString[16];

      rushHeaderIP   = ip;
      rushHeaderPort = port;
      ip             = rushHost;
      port           = rushPort;

      sprintf(portAsString, "%d", (int) rushHeaderPort);
      rushHttpHeaders = "X-relayer-host: " + rushHeaderIP + ":" + portAsString + "\n";
      if (protocol == "https:")
        rushHttpHeaders += "X-relayer-protocol: https\n";
    }
  }

  // Buffers clear
  memset(buffer, 0, TAM_BUF);
  memset(response, 0, TAM_BUF);
  memset(msg, 0, MAX_STA_MSG_SIZE);

  char cvBuf[128];
  snprintf(preContent, sizeof(preContent),
           "%s %s HTTP/1.1\n"
           "User-Agent: orion/%s libcurl/%s\n"
           "Host: %s:%d\n"
           "Accept: application/xml, application/json\n%s",
           verb.c_str(), resource.c_str(), versionGet(), curlVersionGet(cvBuf, sizeof(cvBuf)), ip.c_str(), (int) port, rushHttpHeaders.c_str());

  LM_T(LmtRush, ("'PRE' HTTP headers:\n--------------\n%s\n-------------", preContent));

  if (tenant != "")
  {
    char tenantHeader[128];

    snprintf(tenantHeader, sizeof(tenantHeader), "fiware-service: %s\n", tenant.c_str());

    strncat(preContent, tenantHeader, sizeof(preContent) - strlen(preContent));
  }

  if (!content.empty())
  {
    char   headers[512];
    sprintf(headers,
            "Content-Type: %s\n"
            "Content-Length: %zu\n",
            content_type.c_str(), content.length() + 1);
    strncat(preContent, headers, sizeof(preContent) - strlen(preContent));

    /* Choose the right buffer (static or dynamic) to use. Note we are using +3 due to:
     *    + 1, for the \n between preContent and content
     *    + 1, for the \n at the end of the message
     *    + 1, for the \0 by the trailing character in C strings
     */
    int neededSize = content.length() + strlen(preContent) + 3;
    if (neededSize > MAX_DYN_MSG_SIZE)
    {
      LM_E(("Runtime Error (HTTP request to send is too large: %d bytes)", content.length() + strlen(preContent)));
      return "error";
    }
    else if (neededSize > MAX_STA_MSG_SIZE)
    {
        msgDynamic = (char*) calloc(sizeof(char), neededSize);
        if (msgDynamic == NULL)
        {
          LM_E(("Runtime Error (dynamic memory allocation failure)"));
          return "error";
        }

        msg  = msgDynamic;
        what = (char*) "dynamic";
    }

    /* The above checking should ensure that the three parts fit, so we are using
     * sprint() instead of snprintf() */
    sprintf(msg, "%s\n%s", preContent, content.c_str());
  }
  else
  {
    /* In the case of no-content we assume that MAX_STA_MSG_SIZE is enough to send the message */
    LM_T(LmtClientOutputPayload, ("Using static buffer to send HTTP request (empty content)"));
    sprintf(msg, "%s\n", preContent);
  }

  /* We add a final newline (I guess that HTTP protocol needs it) */
  strcat(msg, "\n");

  int fd = socketHttpConnect(ip, port); // Connecting to HTTP server

  if (fd == -1)
  {
    return "error";
  }

  int nb;
  int sz = strlen(msg);

  LM_T(LmtClientOutputPayload, ("Sending message %lu to HTTP server: sending %s message of %d bytes to HTTP server", callNo, what, sz));
  LM_T(LmtClientOutputPayloadDump, ("Sending to HTTP server payload:\n%s", msg));
  nb = send(fd, msg, sz, 0);
  if (msgDynamic != NULL)
  {
      free(msgDynamic);
  }

  if (nb == -1)
  {
    LM_W(("Notification failure for %s:%d (send: %s)", _ip.c_str(), port, strerror(errno)));
    return "error";
  }
  else if (nb != sz)
  {
    LM_W(("Notification failure for %s:%d (not entire message sent)", _ip.c_str(), port));
    return "error";
  }

  if (waitForResponse)
  {
      nb = recv(fd, &buffer, TAM_BUF - 1, 0);

      if (nb == -1)
      {
        LM_W(("Notification failure for %s:%d (error receiving ACK from HTTP server: %s)", _ip.c_str(), port, strerror(errno)));
        return "error";
      }
      else if ( nb >= TAM_BUF)
      {
        LM_W(("Notification failure for %s:%d (message size of HTTP server reply is too big: %d (max allowed %d)) ", _ip.c_str(), port, nb, TAM_BUF));
        return "error";
      }
      else
      {
          memcpy(response, buffer, nb);
          LM_I(("Notification Successfully Sent to %s:%d%s", ip.c_str(), port, resource.c_str()));
          LM_T(LmtClientInputPayload, ("Received from HTTP server:\n%s", response));
      }

      if (strlen(response) > 0)
          result = response;
  }
  else
  {
     LM_T(LmtClientInputPayload, ("not waiting for response"));
     LM_I(("Notification Successfully Sent to %s:%d%s", ip.c_str(), port, resource.c_str()));
     result = "";
  }

  close(fd);
  return result;
}

#endif
