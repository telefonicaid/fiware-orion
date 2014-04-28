#include "coap.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include <sys/types.h>
#include <sys/socket.h>
#define __USE_POSIX 1
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <string>
#include <boost/thread.hpp>

int Coap::gTestCallback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom) {
  socklen_t addrLen = sizeof(struct sockaddr_in);
  if (recvFrom->ss_family == AF_INET6) {
    addrLen = sizeof(struct sockaddr_in6);
  }
  LM_V(("gTestCallback function called"));

  //  prepare appropriate response
  CoapPDU *response = new CoapPDU();
  response->setVersion(1);
  response->setMessageID(request->getMessageID());
  response->setToken(request->getTokenPointer(),request->getTokenLength());
  //response->setToken((uint8_t*)"\1\16",2);
  char *payload = (char*)"This is a mundanely worded test payload.";

  // respond differently, depending on method code
  switch(request->getCode()) {
    case CoapPDU::COAP_EMPTY:
      // makes no sense, send RST
    break;
    case CoapPDU::COAP_GET:
      response->setCode(CoapPDU::COAP_CONTENT);
      response->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_TEXT_PLAIN);
      response->setPayload((uint8_t*)payload,strlen(payload));
    break;
    case CoapPDU::COAP_POST:
      response->setCode(CoapPDU::COAP_CREATED);
    break;
    case CoapPDU::COAP_PUT:
      response->setCode(CoapPDU::COAP_CHANGED);
    break;
    case CoapPDU::COAP_DELETE:
      response->setCode(CoapPDU::COAP_DELETED);
      response->setPayload((uint8_t*)"DELETE OK",9);
    break;
    default:
    break;
  }

  // type
  switch(request->getType()) {
    case CoapPDU::COAP_CONFIRMABLE:
      response->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;
    case CoapPDU::COAP_NON_CONFIRMABLE:
      response->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;
    case CoapPDU::COAP_ACKNOWLEDGEMENT:
    break;
    case CoapPDU::COAP_RESET:
    break;
    default:
      return 1;
    break;
  };

  // send the packet
  ssize_t sent = sendto(
    sockfd,
    response->getPDUPointer(),
    response->getPDULength(),
    0,
    (sockaddr*)recvFrom,
    addrLen
  );
  if (sent<0) {
    LM_V(("Error sending packet: %ld.",sent));
    perror(NULL);
    return 1;
  } else {
    LM_V(("Sent: %ld",sent));
  }

  return 0;
}

void Coap::worker()
{

}

int Coap::run(const char *host, unsigned short port)
{
  // Buffers for UDP and URIs
  char buffer[BUFFER_SIZE];
  char uriBuffer[URI_BUFFER_SIZE];
  int  recvURILen = 0;
  int  ret = 0;
  char portChar[6];
  snprintf(portChar, 6, "%hd", port);

  // Prepare binding address
  LM_V(("Setting up bind address"));
  struct addrinfo *bindAddr = NULL;
  struct addrinfo hints;

  memset(&hints,0x00,sizeof(struct addrinfo));
  hints.ai_flags      = 0;
  hints.ai_socktype   = SOCK_DGRAM;
  hints.ai_flags     |= AI_NUMERICSERV;
  hints.ai_family     = PF_INET; // ipv4, PF_INET6 for ipv6 or PF_UNSPEC to let OS decide

  int error = getaddrinfo(host,portChar,&hints,&bindAddr);
  if (error) {
    LM_V(("Error getting address info: %s.",gai_strerror(error)));
    return error;
  }
  //**********

  // Setting up the UDP socket
  int sd = socket(bindAddr->ai_family,bindAddr->ai_socktype,bindAddr->ai_protocol);

  // Binding socket
  LM_V(("Binding socket"));
  if (bind(sd,bindAddr->ai_addr,bindAddr->ai_addrlen)!=0) {
    LM_V(("Error binding socket"));
    perror(NULL);
    exit(5);
  }

  // storage for handling receive address
  struct sockaddr_storage recvAddr;
  socklen_t recvAddrLen = sizeof(struct sockaddr_storage);
  struct sockaddr_in *v4Addr;
  struct sockaddr_in6 *v6Addr;
  char straddr[INET6_ADDRSTRLEN];

  // reuse the same PDU
  CoapPDU *recvPDU = new CoapPDU((uint8_t*)buffer, BUFFER_SIZE, BUFFER_SIZE);

  //boost::thread workerThread(&Coap::worker, this);

  // just block and handle one packet at a time in a single thread
  // you're not going to use this code for a production system are you ;)
  LM_V(("Listening for packets..."));
  while(1) {
    // receive packet
    ret = recvfrom(sd, &buffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);
    if (ret == -1) {
      LM_V(("Error receiving data"));
      return -1;
    }

    // print src address
    switch (recvAddr.ss_family) {
      case AF_INET:
        v4Addr = (struct sockaddr_in*)&recvAddr;
        LM_V(("Got packet from %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port)));
      break;

      case AF_INET6:
        v6Addr = (struct sockaddr_in6*)&recvAddr;
        LM_V(("Got packet from %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port)));
      break;
    }

    // validate packet
    if (ret > BUFFER_SIZE) {
      LM_V(("PDU too large to fit in pre-allocated buffer"));
      continue;
    }
    recvPDU->setPDULength(ret);
    if (recvPDU->validate()!=1) {
      LM_V(("Malformed CoAP packet"));
      continue;
    }
    LM_V(("Valid CoAP PDU received"));
    recvPDU->printHuman();

    // depending on what this is, maybe call callback function
    if (recvPDU->getURI(uriBuffer, URI_BUFFER_SIZE, &recvURILen)!=0) {
      LM_V(("Error retrieving URI"));
      continue;
    }
    if (recvURILen == 0) {
      LM_V(("There is no URI associated with this Coap PDU"));
    } else {
      this->gTestCallback(recvPDU,sd,&recvAddr);
      continue;
    }

    // no URI, handle cases

    // code == 0, no payload, this is a ping request, send RST
    if (recvPDU->getPDULength()  ==  0 && recvPDU->getCode()  ==  0) {
      LM_V(("CoAP ping request"));
    }

  }

  //workerThread.join();
  return 0;
}

void Coap::printAddressStructures(struct addrinfo *addr) {
  int count = 0;
  while (addr) {
    printf("Address %d:",count++);
    printf("   ");
    switch(addr->ai_family) {
      case AF_INET:
        printf("IPv4");
      break;

      case AF_INET6:
        printf("IPv6");
      break;

      default:
        printf("Unknown address family");
      break;
    }

    switch (addr->ai_socktype) {
      case SOCK_DGRAM:
        printf(", UDP");
      break;

      case SOCK_STREAM:
        printf(", TCP");
      break;

      case SOCK_RAW:
        printf(", RAW");
      break;

      default:
        printf(", Unknown socket type.");
      break;
    }

    // print out address host and port
    struct sockaddr_in *v4Addr;
    struct sockaddr_in6 *v6Addr;
    char straddr[INET6_ADDRSTRLEN];
    switch (addr->ai_family) {
      case AF_INET:
        v4Addr = (struct sockaddr_in*)addr->ai_addr;
        printf(", %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
      break;

      case AF_INET6:
        v6Addr = (struct sockaddr_in6*)addr->ai_addr;
        printf(", %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
      break;
    }
    printf(" ");

      addr = addr->ai_next;
   }
}

void Coap::printAddress(struct addrinfo *addr) {
  // print out bound address
  if (addr->ai_family == AF_INET) {
    struct sockaddr_in *v4Addr = (struct sockaddr_in*)addr->ai_addr;
    printf("UDP socket: %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
  } else if (addr->ai_family == AF_INET6) {
    char straddr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *v6Addr = (struct sockaddr_in6*)addr->ai_addr;
    printf("UDP socket: %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
  }
}
