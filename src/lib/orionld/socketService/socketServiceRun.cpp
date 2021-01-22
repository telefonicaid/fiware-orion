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
// ssRead - read a buffer from a socket
//
static int ssRead(int fd, char* dataP, int dataLen, bool* connectionClosedP)
{
  int nb = 0;

  while (nb < dataLen)
  {
    int sz = read(fd, &dataP[nb], dataLen - nb);
    if (sz == -1)
      LM_RE(-1, ("error reading from Socket Service connection: %s", strerror(errno)));
    else if (sz == 0)
    {
      *connectionClosedP = true;
      return -1;
    }
    nb += sz;
  }

  dataP[nb] = 0;

  return nb;
}



// -----------------------------------------------------------------------------
//
// ssWrite -
//
static void ssWrite(int fd, const char* data, int dataLen)
{
  write(fd, data, dataLen);
}


#if 0
// -----------------------------------------------------------------------------
//
// ssWrite -
//
static void ssGetEntity(char* entityId)
{
  int     responseLen;
  KjNode* responseTree = dbEntityRetrieve(entityId,
                                          NULL,   // attrs
                                          false,  // attrMandatory
                                          false,  // sysAttrs
                                          false,  // keyValues
                                          NULL);  // datasetId

  char response[2048];
  kjRender(orionldState.kjsonP, responseTree, response, sizeof(response));
  responseLen = strlen(response);
  ssWrite(fd, response, responseLen);
}
#endif


// -----------------------------------------------------------------------------
//
// ssTreat -
//
static void ssTreat(int fd, SsHeader* headerP, char* dataP)
{
  switch (headerP->msgCode)
  {
  case SsPing:
    ssWrite(fd, "pong", 4);
    break;

  case SsGetEntity:
#if 0
    ssGetEntity(dataP);
#else
    ssWrite(fd, "GET Entity Is Not Implemented", 30);
#endif
    break;

  default:
    LM_E(("SS: unknown message code 0x%x", headerP->msgCode));
    ssWrite(fd, "unknown message code", 20);
  }
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
  unsigned int   dataLen = 16 * 1024;  // If more is needed, realloc is used
  char*          dataP   = (char*) malloc(dataLen + 1);

  if (dataP == NULL)
    LM_RVE(("error allocating Socket Service buffer of %d bytes: %s", dataLen, strerror(errno)));

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

    if (FD_ISSET(listenFd, &rFds))
    {
      fd = ssAccept(listenFd);
      if (fd == -1)
        LM_RVE(("error accepting incoming connection over Socket Service: %s", strerror(errno)));
    }
    else if (FD_ISSET(fd, &rFds))
    {
      SsHeader  header;
      bool      connectionClosed = false;

      //
      // Read the message header
      //
      if (ssRead(fd, (char*) &header, sizeof(header), &connectionClosed) == -1)
      {
        if (connectionClosed)
        {
          close(fd);
          fd = -1;
          continue;  // back to 'while (1)'
        }

        LM_RVE(("Error reading SS header from Socket Service header: %s", strerror(errno)));
      }

      //
      // Is the data buffer big enough?
      // If not, reallocate
      //
      if (header.dataLen >= dataLen)
      {
        free(dataP);
        dataLen = header.dataLen;
        dataP   = (char*) malloc(dataLen + 1);

        if (dataP == NULL)
          LM_RVE(("error allocating Socket Service buffer of %d bytes: %s", dataLen, strerror(errno)));
      }

      //
      // Read the data
      //
      if (ssRead(fd, dataP, header.dataLen, &connectionClosed) == -1)
      {
        if (connectionClosed)
        {
          close(fd);
          fd = -1;
          continue;  // back to 'while (1)'
        }

        LM_RVE(("Error reading SS body from Socket Service header: %s", strerror(errno)));
      }

      //
      // Treat the request
      //
      ssTreat(fd, &header, dataP);
    }
  }
}
