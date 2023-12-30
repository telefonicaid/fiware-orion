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
#include <unistd.h>                                          // read, write, usleep
#include <errno.h>                                           // errno
#include <string.h>                                          // strerror
#include <poll.h>                                            // poll
#include <netinet/in.h>                                      // sockaddr_in

#include "logMsg/logMsg.h"                                   // LM_*

#include "orionld/common/orionldState.h"                     // troe
#include "orionld/mongoc/mongocServerVersionGet.h"           // mongocServerVersionGet
#include "orionld/troe/pgVersionGet.h"                       // pgVersionGet
#include "orionld/types/SsHeader.h"                          // SsHeader
#include "orionld/types/SsMsgCode.h"                         // SsMsgCode
#include "orionld/socketService/socketServiceRun.h"          // Own interface



// -----------------------------------------------------------------------------
//
// ssAccept -
//
static int ssAccept(int listenFd)
{
  struct sockaddr_in  sa;
  socklen_t           saLen = sizeof(struct sockaddr_in);
  int                 fd    = accept(listenFd, (struct sockaddr*) &sa, &saLen);

  if (fd == -1)
    LM_RE(-1, ("accept socket service connection: %s", strerror(errno)));

  return fd;
}



// -----------------------------------------------------------------------------
//
// healthCheck -
//
static bool healthCheck(void)
{
  char version[64];

  if (mongocServerVersionGet(version) == false)
    return false;

  if (troe)
  {
    if (pgVersionGet(version, sizeof(version)) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// socketServiceRun -
//
// For now, one single connection is served at a time
//
void socketServiceRun(int listenFd)
{
  int connectionFd = -1;

  while (1)
  {
    int            fd = (connectionFd != -1)? connectionFd : listenFd;
    int            rs;
    struct pollfd  pollFd;

    pollFd.fd      = fd;
    pollFd.events  = POLLIN;
    pollFd.revents = 0;

    rs = poll(&pollFd, 1, 5000);

    if (rs == -1)
    {
      if (errno != EINTR)
        LM_RVE(("poll error for socket service: %s", strerror(errno)));
    }
    else if (rs == 0)
    {}
    else if (rs == 1)
    {
      if (fd == listenFd)
      {
        if (healthCheck() == false)
        {
          usleep(500000);
          continue;
        }

        connectionFd = ssAccept(listenFd);
        if (fd == -1)
          LM_RVE(("error accepting incoming connection over Socket Service: %s", strerror(errno)));
      }
      else
      {
        //
        // Make sure it's just a "connection closed"
        //
        int  nb;
        char buf[16];

        nb = read(connectionFd, buf, sizeof(buf));

        if (nb != 0)
          LM_W(("SS: A message was sent over the socket service - the broker is not ready for that - closing connection"));

        close(connectionFd);
        connectionFd = -1;
      }
    }
  }
}
