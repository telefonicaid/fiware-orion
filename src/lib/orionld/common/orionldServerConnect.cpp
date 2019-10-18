/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <sys/types.h>                                           // types
#include <sys/socket.h>                                          // socket
#include <netinet/in.h>                                          // sockaddr_in
#include <netdb.h>                                               // struct hostent
#include <unistd.h>                                              // close

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/common/orionldServerConnect.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// orionldServerConnect -
//
int orionldServerConnect(char* ip, unsigned short portNo)
{
  int                 fd;
  struct hostent*     heP;
  struct sockaddr_in  server;

  heP = gethostbyname(ip);
  if (heP == NULL)
  {
    LM_E(("unable to find host '%s'", ip));
    return -1;
  }

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
  {
    LM_E(("Can't even create a socket: %s", strerror(errno)));
    return -1;
  }

  server.sin_family = AF_INET;
  server.sin_port   = htons(portNo);
  server.sin_addr   = *((struct in_addr*) heP->h_addr);
  bzero(&server.sin_zero, 8);

  if (connect(fd, (struct sockaddr*) &server, sizeof(struct sockaddr)) == -1)
  {
    close(fd);
    LM_E(("Unable to connect to host/port: %s:%d", ip, portNo));
  }

  LM_TMP(("NFY: Connected to server %s:%d", ip, portNo));

  return fd;
}
