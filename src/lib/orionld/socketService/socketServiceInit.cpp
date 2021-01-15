/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include <sys/socket.h>                                      // socket, setsockopt, bind, listen
#include <netinet/in.h>                                      // sockaddr_in

#include "logMsg/logMsg.h"                                   // LM_*
#include "logMsg/traceLevels.h"                              // Lmt*

#include "orionld/socketService/socketServiceInit.h"         // Own interface



// -----------------------------------------------------------------------------
//
// socketServiceInit -
//
int socketServiceInit(unsigned short port)
{
  int                 listenFd = -1;
  struct sockaddr_in  sai;

  listenFd = socket(AF_INET,  SOCK_STREAM, 0);

  if (listenFd == -1)
    LM_RP(-1, ("error opening listen socket for socket service"));

  int optval = 1;
  if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1)
    LM_RP(-1, ("error setting options for socket service"));

  sai.sin_family      = AF_INET;
  sai.sin_port        = htons(port);
  sai.sin_addr.s_addr = INADDR_ANY;
  bzero(&sai.sin_zero, 8);

  if (bind(listenFd, (struct sockaddr*) &sai, sizeof(struct sockaddr)) == -1)
    LM_RP(-1, ("error binding socket for socket service"));

  if (listen(listenFd, 10) == -1)
    LM_RP(-1, ("error listening to socket for socket service"));

  return listenFd;
}
