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


/* ****************************************************************************
*
* socketHttpConnect -
*/
int socketHttpConnect(std::string host, unsigned short port)
{
  int                 fd;
  struct hostent*     hp;
  struct sockaddr_in  peer;

  if ((hp = gethostbyname(host.c_str())) == NULL)
     LM_RE(-1, ("gethostbyname('%s'): %s", host.c_str(), strerror(errno)));

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
     LM_RE(-1, ("socket: %s", strerror(errno)));

  memset((char*) &peer, 0, sizeof(peer));

  peer.sin_family      = AF_INET;
  peer.sin_addr.s_addr = ((struct in_addr*) (hp->h_addr))->s_addr;
  peer.sin_port        = htons(port);

  if (connect(fd, (struct sockaddr*) &peer, sizeof(peer)) == -1)
  {
    close(fd);
    LM_E(("connect(%s, %d): %s", host.c_str(), port, strerror(errno)));
    return -1;
  }

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
  char         buffer[TAM_BUF];
  char         response[TAM_BUF];
  char         preContent[TAM_BUF];
  char         msgStatic[MAX_STA_MSG_SIZE];

  std::string  result;
  char*        msgDynamic = NULL;
  char*        msg;

  // Buffers clear 
  memset(buffer, 0, TAM_BUF);
  memset(response, 0, TAM_BUF);
  memset(preContent, 0, TAM_BUF);

  if (port == 0)        LM_RE("error", ("port is ZERO"));
  if (ip.empty())       LM_RE("error", ("ip is empty"));
  if (verb.empty())     LM_RE("error", ("verb is empty"));
  if (resource.empty()) LM_RE("error", ("resource is empty"));  

  snprintf(preContent, sizeof(preContent),
           "%s %s HTTP/1.1\n"
           "User-Agent: orion/%s\n"
           "Host: %s:%d\n"
           "Accept: application/xml, application/json\n",
           verb.c_str(), resource.c_str(), versionGet(), ip.c_str(), (int) port);

  if ((!content_type.empty()) && (!content.empty()))
  {

    char   headers[512];
    sprintf(headers,
            "Content-Type: %s\n"
            "Content-Length: %zu\n",
            content_type.c_str(), content.length() + 1);
    strncat(preContent, headers, sizeof(preContent) - strlen(preContent));

    /* Choose the right buffer (static or dynamic) to use */
    int bz;
    if (content.length() + strlen(preContent) > MAX_DYN_MSG_SIZE-1) {
        LM_RE("error", ("content is too large"));
    }
    else if (content.length() + strlen(preContent) > MAX_STA_MSG_SIZE-1) {
        msgDynamic = (char*) calloc(sizeof(char), content.length() + 1);
        msg = msgDynamic;
        bz = content.length() + 1;
    }
    else {
        msg = msgStatic;
        memset(msg, 0, MAX_STA_MSG_SIZE);
        bz = MAX_STA_MSG_SIZE;
    }

    strncat(msg, preContent, bz - strlen(msg));
    strncat(msg, "\n", bz - strlen(msg));
    strncat(msg, content.c_str(), bz - strlen(msg));
  }

  strncat(msg, "\n", sizeof(msg) - strlen(msg));

  int fd = socketHttpConnect(ip, port); // Connecting to HTTP server
  if (msgDynamic != NULL) {
      free (msgDynamic);
  }

  if (fd == -1)
    LM_RE("error", ("Unable to connect to HTTP server at %s:%d", ip.c_str(), port));

  int nb;
  int sz = strlen(msg);

#if 0
  if (sz >= MSG_SIZE)
    LM_RE("Msg too large", ("A message got too large (%d bytes - max size is %d). The contextBroker must be recompiled!!!", sz, sizeof(msg)));
#endif

  LM_T(LmtClientOutputPayload, ("Sending to HTTP server %d bytes:\n%s", sz, msg));
  nb = send(fd, msg, sz, 0);
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
  else
     result = "";

  close(fd);
  return result;
}
