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
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include <stdio.h>                           // printf, fprintf, stderr, ...
#include <unistd.h>                          // write
#include <errno.h>                           // errno
#include <string.h>                          // strerror
#include <stdlib.h>                          // exit
#include <stddef.h>                          // NULL
#include <strings.h>                         // bzero
#include <sys/types.h>                       // types
#include <sys/socket.h>                      // socket
#include <netinet/in.h>                      // sockaddr_in
#include <netdb.h>                           // struct hostent



// -----------------------------------------------------------------------------
//
// serverConnect - connect to server
//
int serverConnect(char* host, unsigned short port, char** errorStringP)
{
  int                 fd = socket(AF_INET, SOCK_STREAM, 0);
  struct hostent*     heP;
  struct sockaddr_in  server;

  if (fd == -1)
  {
    *errorStringP = (char*) "unable to create socket";
    return -1;
  }

  heP = gethostbyname(host);
  if (heP == NULL)
  {
    *errorStringP = (char*) "unable to find host";
    return -2;
  }
    
  server.sin_family = AF_INET;
  server.sin_port   = htons(port);
  server.sin_addr   = *((struct in_addr*) heP->h_addr);
  bzero(&server.sin_zero, 8);

  if (connect(fd, (struct sockaddr*) &server, sizeof(struct sockaddr)) == -1)
  {
    *errorStringP = (char*) "unable to connect to host/port of server";
    close(fd);
    return -3;
  }

  return fd;
}



// -----------------------------------------------------------------------------
//
// SsHeader - FIXME: include src/lib/orionld/socketService/socketService.h
//
typedef struct SsHeader
{
  unsigned short msgCode;
  unsigned short options;
  unsigned int   dataLen;
} SsHeader;



// -----------------------------------------------------------------------------
//
// SsMsgCode - FIXME: include src/lib/orionld/socketService/socketService.h
//
typedef enum SsMsgCode
{
  SsPing = 1
} SsMsgCode;



// -----------------------------------------------------------------------------
//
// main -
//
int main(int argC, char* argV[])
{
  char*           eString;
  char*           server  = (char*) "localhost";
  unsigned short  port    = 1027;
  int             fd      = serverConnect(server, port, &eString);
  SsMsgCode       msgCode = SsPing;
  char*           data    = NULL;
  int             dataLen = 0;
  unsigned short  options = 0;
  int             nb;

  if (fd < 0)
  {
    fprintf(stderr, "error connecting to server %s:%d\n", server, port);
    exit(1);
  }

  SsHeader header = { msgCode, options, (unsigned int) dataLen };

  nb = write(fd, &header, sizeof(header));
  if (nb != sizeof(header))
  {
    fprintf(stderr, "error sending Socket Service header to server %s:%d\n", server, port);
    exit(2);
  }

  if (data != NULL)
  {
    nb = write(fd, data, dataLen);
    
    if (nb != dataLen)
    {
      fprintf(stderr, "error sending Socket Service data to server %s:%d\n", server, port);
      exit(3);
    }
  }


  char* response = (char*) malloc(32 * 1024);

  nb = read(fd, response, 32 * 1024);
  if (nb > 0)
  {
    response[nb] = 0;
    printf("Got %d bytes: '%s'\n", nb, response);
  }
  else
  {
    fprintf(stderr, "error reading response from server %s:%d: %s\n", server, port, strerror(errno));
    exit(4);
  }

  return 0;
}
