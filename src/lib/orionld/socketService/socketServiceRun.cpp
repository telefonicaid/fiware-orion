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
#include <unistd.h>                                          // read, write
#include <errno.h>                                           // errno
#include <string.h>                                          // strerror
#include <netinet/in.h>                                      // sockaddr_in

#include "logMsg/logMsg.h"                                   // LM_*
#include "logMsg/traceLevels.h"                              // Lmt*

#include "orionld/db/dbConfiguration.h"                      // dbEntityRetrieve
#include "orionld/socketService/socketService.h"             // SsHeader, SsMsgCode
#include "orionld/socketService/socketServiceRun.h"          // Own interface



// -----------------------------------------------------------------------------
//
// ssAccept -
//
int ssAccept(int listenFd)
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
// socketServiceRun -
//
// For now, one single connection is served at a time
//
void socketServiceRun(int listenFd)
{
  int            fd      = -1;

  while (1)
  {
    int             fds;
    int             fdMax;
    fd_set          rFds;
    struct timeval  tv = { 5, 0 };

    FD_ZERO(&rFds);
    if (fd != -1)
    {
      FD_SET(fd, &rFds);
      fdMax = fd;
    }
    else
    {
      FD_SET(listenFd, &rFds);
      fdMax = listenFd;
    }

    fds = select(fdMax + 1, &rFds, NULL, NULL, &tv);
    if ((fds == -1) && (errno != EINTR))
      LM_RVE(("select error for socket service: %s", strerror(errno)));

    if (fds == 0)
      LM_TMP(("SS: timeout"));
    else if (FD_ISSET(listenFd, &rFds))
    {
      fd = ssAccept(listenFd);
      if (fd == -1)
        LM_RVE(("error accepting incoming connection over Socket Service: %s", strerror(errno)));
      LM_TMP(("SS: Accepted a connection for socket-service"));
    }
    else if (FD_ISSET(fd, &rFds))
    {
      LM_TMP(("SS: Reading from socket-service - obly connection-closed are permitted"));
      //
      // Make sure it's just a "connection closed"
      //
      int  nb;
      char buf[16];

      nb = read(fd, buf, sizeof(buf));

      if (nb != 0)
        LM_X(1, ("A message was sent over the socket service - the broker is not ready for that"));

      LM_TMP(("SS: The socket-service connection was closed"));
      close(fd);
      fd = -1;
    }
  }
}
