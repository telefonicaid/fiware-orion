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
* Author: developer
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "ConnectionInfo.h"
#include "clientSocketHttp.h"
#include "serviceRoutines/versionTreat.h"
#include <iostream>

#include <sys/types.h>                          // system types ...
#include <sys/socket.h>                         // socket, bind, listen
#include <sys/un.h>                             // sockaddr_un
#include <netinet/in.h>                         // struct sockaddr_in
#include <netdb.h>                              // gethostbyname
#include <arpa/inet.h>                          // inet_ntoa
#include <netinet/tcp.h>                        // TCP_NODELAY
#include <string>
#include <unistd.h>                             // close()

#include "common/string.h"
#include "rest/rest.h"


/* ****************************************************************************
*
* socketHttpConnect -
*/
int socketHttpConnect(std::string host, unsigned short port)
{
  int                 fd;
  int                 status;
  struct addrinfo     hints;
  struct addrinfo*    peer;
  char                port_str[10];


 LM_VVV(("Generic Connect to: '%s'  port: '%d'", host.c_str(), port));

 memset(&hints, 0, sizeof(struct addrinfo));
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_protocol = 0;

 if (ipVersionUsed == IPV4) 
 {
   hints.ai_family = AF_INET;
   LM_VVV(("Allow IPv4 only"));
 }
 else if (ipVersionUsed == IPV6)
 {
   hints.ai_family = AF_INET6;
   LM_VVV(("Allow  IPv6 only"));
 }
 else 
 {
   hints.ai_family = AF_UNSPEC;
   LM_VVV(("Allow IPv4 or IPv6"));
 }

 snprintf(port_str, sizeof(port_str), "%d" , port);

 if ((status = getaddrinfo(host.c_str(), port_str, &hints, &peer)) != 0) {
     LM_RE(-1, ("getaddrinfo('%s'): %s", host.c_str(), strerror(errno)));
}

  if ((fd = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol)) == -1) {
     LM_RE(-1, ("socket: %s", strerror(errno)));
}

  if (connect(fd, peer->ai_addr, peer->ai_addrlen) == -1)
  {
    freeaddrinfo(peer);
    close(fd);
    LM_E(("connect(%s, %d): %s", host.c_str(), port, strerror(errno)));
    return -1;
  }
  freeaddrinfo(peer);
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
* arguments or (even better) and exception. To be solved in the future in a hardening
* period.
*
* Note, we are using an hybrid approach, consisting in an static thread-local buffer of
* small size that cope with the most of notifications to avoid expensive
* calloc/free syscalls if the notification payload is not very large.
*
*/
std::string sendHttpSocket
(
   std::string     ip,
   unsigned short  port,
   std::string     verb,
   std::string     resource,
   std::string     content_type,
   std::string     content,
   bool            waitForResponse
)
{  
  char                       buffer[TAM_BUF];
  char                       response[TAM_BUF];
  char                       preContent[TAM_BUF];
  char                       msgStatic[MAX_STA_MSG_SIZE];
  char*                      what       = (char*) "static";
  char*                      msgDynamic = NULL;
  char*                      msg        = msgStatic;   // by default, use the static buffer
  std::string                result;
  static unsigned long long  callNo     = 0;

  ++callNo;

  // Preconditions check
  if (port == 0)        LM_RE("error", ("port is ZERO"));
  if (ip.empty())       LM_RE("error", ("ip is empty"));
  if (verb.empty())     LM_RE("error", ("verb is empty"));
  if (resource.empty()) LM_RE("error", ("resource is empty"));
  if ((content_type.empty()) && (!content.empty())) LM_RE("error", ("Content-Type is empty but there is actual content"));
  if ((!content_type.empty()) && (content.empty())) LM_RE("error", ("there is actual content but Content-Type is empty"));

  // Buffers clear
  memset(buffer, 0, TAM_BUF);
  memset(response, 0, TAM_BUF);
  memset(preContent, 0, TAM_BUF);
  memset(msg, 0, MAX_STA_MSG_SIZE);

  snprintf(preContent, sizeof(preContent),
           "%s %s HTTP/1.1\n"
           "User-Agent: orion/%s\n"
           "Host: %s:%d\n"
           "Accept: application/xml, application/json\n",
           verb.c_str(), resource.c_str(), versionGet(), ip.c_str(), (int) port);

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
    if (neededSize > MAX_DYN_MSG_SIZE) {
        LM_RE("error", ("HTTP request to send is too large: %d bytes", content.length() + strlen(preContent)));
    }
    else if (neededSize > MAX_STA_MSG_SIZE) {
        msgDynamic = (char*) calloc(sizeof(char), neededSize);
        if (msgDynamic == NULL) {
            LM_RE("error", ("dynamic memory allocation failure"));
        }
        msg = msgDynamic;
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

  /* We add a final newline (I guess that the HTTP protocol needs it) */
  strcat(msg, "\n");

  int fd = socketHttpConnect(ip, port); // Connecting to HTTP server

  if (fd == -1)
    LM_RE("error", ("Unable to connect to HTTP server at %s:%d", ip.c_str(), port));

  int nb;
  int sz = strlen(msg);

  LM_T(LmtClientOutputPayload, ("Sending (msg #%lu) %s message of %d bytes to HTTP server", callNo, what, sz));
  LM_T(LmtClientOutputPayloadView, ("Payload to HTTP server:\n%s", msg));
  nb = send(fd, msg, sz, 0);
  if (msgDynamic != NULL) {
      free (msgDynamic);
  }

  if (nb == -1)
    LM_RE("error", ("error sending to HTTP server: %s", strerror(errno)));
  else if (nb != sz)
     LM_E(("error sending to HTTP server. Sent %d bytes of %d", nb, sz));

  if (waitForResponse) {
      nb = recv(fd,&buffer,TAM_BUF-1,0);

      if (nb == -1)
        LM_RE("error", ("error recv from HTTP server: %s", strerror(errno)));
      else if ( nb >= TAM_BUF)
         LM_RE("error", ("recv from HTTP server too long"));
      else
      {
          memcpy(response, buffer, nb);
          LM_T(LmtClientInputPayload, ("Received from HTTP server:\n%s", response));
      }

      if (strlen(response) > 0)
          result = response;
  }
  else {
     LM_T(LmtClientInputPayload, ("not waiting for response"));
     result = "";
  }

  close(fd);
  return result;
}


