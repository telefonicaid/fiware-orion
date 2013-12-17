// RBL nuevo
/* ****************************************************************************
*
* FILE                    clientSocketHttpV6.cpp
*
* CREATION DATE           Dic 2013
*
* AUTHOR                  developer 
*
*
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "ConnectionInfo.h"

#include "clientSocketHttpV6.h"
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
//#include <stdio.h>				// Added by C.Ralli - para tener printf
#include <errno.h>				// Added by C.Ralli - para tener errno


/* ****************************************************************************
*
* socketHttpConnectV6 -
*/
int socketHttpConnectV6 (std::string host, unsigned short port)
{
  int                 fd;
  int status;
  struct addrinfo hints;   
  struct addrinfo *peer; 
  char port_str[10];
 
 memset(&hints, 0, sizeof(hints)); 
 hints.ai_family = AF_INET6;   // AF_UNSPEC;  // Accept IPv4 and IPv6
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_protocol = 0;

 snprintf (port_str, sizeof(port_str), "%d" , port);

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
*/
std::string sendHttpSocketV6( std::string ip,
                           unsigned short port,
                           std::string verb,
                           std::string resource,
                           std::string content_type,
                           std::string content,
                           bool waitForResponse)
{
  char         host[128];
  char         dataLen[32];
  char         buffer[TAM_BUF];
  char         response[TAM_BUF];
  std::string  msg;
  std::string  result;

  // Buffers clear 
  memset (buffer, 0, TAM_BUF);
  memset (response, 0, TAM_BUF);
 
  if ( (port == 0) || (ip.empty()) || (verb.empty()) || (resource.empty()) ) 
  {
/*      LM_RE("error", ("Error in param")); */
  }

  // Param OK
  msg = verb;
  msg += " ";
  msg += resource;
  msg += " ";
  msg += "HTTP/1.1";   
  msg += "\n";        


  snprintf(host, sizeof(host), "Host: %s:%d\n", ip.c_str(), port);

  // FIXME: this should be the name and version of the app using the library. I mean,
  // not always "iotAgent", some case it will be "contextBroker". We need to find a
  // flexible way to fill the User-Agent
  msg += "User-Agent: NGSI Rest Library\n";
  msg += host;
  msg += "Accept: */*\n";

  if ((!content_type.empty()) && (!content.empty()) ) 
  {
    snprintf(dataLen, sizeof(dataLen), "Content-Length: %zu\n", content.length() + 1);

    msg += "Content-Type: ";
    msg += content_type;
    msg += "\n";
    msg += dataLen;
    msg += "\n";
    msg += content;
    msg += "\n";
  }
  else
  {
    // It is mandatory to put \n, othrewise fails
    msg += "\n";
  }
  int fd = socketHttpConnectV6(ip, port); // Connecting to HTTP server
  if (fd == -1)
      LM_RE("error", ("Unable to connect to HTTP server at %s:%d", ip.c_str(), port));


  int sz;
  sz = strlen(msg.c_str());
  int nb;
  nb = send(fd, msg.c_str(), sz, 0);
  if (nb != sz)
  {
      if (nb == -1)
          LM_RE("error", ("error sending to HTTP server: %s", strerror(errno)));
  }

 
   LM_V5(("Sending to HTTP server :\n %s", msg.c_str())); 

   if (waitForResponse) {
      nb = recv(fd,&buffer,TAM_BUF-1,0);

      if (nb == -1)
      {
          LM_RE("error", ("error recv from HTTP server: %s", strerror(errno))); 
      }
      else if ( nb >= TAM_BUF)
      {
          LM_RE("error", ("recv from HTTP server too long")); 
      }
      else
      {
          memcpy (response, buffer, nb);

          LM_V5(("Received from HTTP server :\n %s", response)); 
      }

      if (strlen(response) > 0)
      {
          result = response;
      }

  }
  else
  {
     result = "";
  }

  close(fd);
  return result;
}
