#include "coap.h"

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

#include "nethelper.h"


//Coap::Coap()
//{
//}

int Coap::gTestCallback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom) {
  socklen_t addrLen = sizeof(struct sockaddr_in);
  if(recvFrom->ss_family==AF_INET6) {
    addrLen = sizeof(struct sockaddr_in6);
  }
  DBG("gTestCallback function called");

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
  if(sent<0) {
    DBG("Error sending packet: %ld.",sent);
    perror(NULL);
    return 1;
  } else {
    DBG("Sent: %ld",sent);
  }

  return 0;
}



int Coap::run(int argc, char **argv) {

  // parse options
  if(argc!=3) {
    printf("USAGE\r\n   %s listenAddress listenPort\r\n",argv[0]);
    return 0;
  }

  char *listenAddressString = argv[1];
  char *listenPortString    = argv[2];

  // setup bind address
  struct addrinfo *bindAddr;
  INFO("Setting up bind address");
  int ret = setupAddress(listenAddressString,listenPortString,&bindAddr,SOCK_DGRAM,AF_INET);
  if(ret!=0) {
    INFO("Error setting up bind address, exiting.");
    return -1;
  }

  // iterate through returned structure to see what we got
  //printAddressStructures(bindAddr);

  // setup socket
  int sockfd = socket(bindAddr->ai_family,bindAddr->ai_socktype,bindAddr->ai_protocol);

  // call bind
  DBG("Binding socket.");
  if(bind(sockfd,bindAddr->ai_addr,bindAddr->ai_addrlen)!=0) {
    DBG("Error binding socket");
    perror(NULL);
    exit(5);
  }
  //printAddress(bindAddr);

  // setup URI callbacks using uthash hash table
  struct URIHashEntry *entry = NULL, *directory = NULL, *hash = NULL;
  for(int i=0; i<gNumResources; i++) {
    // create new hash structure to bind URI and callback
    entry = (struct URIHashEntry*)malloc(sizeof(struct URIHashEntry));
    //entry->uri = gURIList[i];
    entry->uri = "/coaptest";
    //entry->callback = this->gTestCallback;
    // add hash structure to hash table, note that key is the URI
    HASH_ADD_KEYPTR(hh, directory, entry->uri, strlen(entry->uri), entry);
  }

  // buffers for UDP and URIs
  #define BUF_LEN 500
  #define URI_BUF_LEN 32
  char buffer[BUF_LEN];
  char uriBuffer[URI_BUF_LEN];
  int recvURILen = 0;

  // storage for handling receive address
  struct sockaddr_storage recvAddr;
  socklen_t recvAddrLen = sizeof(struct sockaddr_storage);
  struct sockaddr_in *v4Addr;
  struct sockaddr_in6 *v6Addr;
  char straddr[INET6_ADDRSTRLEN];

  // reuse the same PDU
  CoapPDU *recvPDU = new CoapPDU((uint8_t*)buffer,BUF_LEN,BUF_LEN);

  // just block and handle one packet at a time in a single thread
  // you're not going to use this code for a production system are you ;)
  while(1) {
    // receive packet
    ret = recvfrom(sockfd,&buffer,BUF_LEN,0,(sockaddr*)&recvAddr,&recvAddrLen);
    if(ret==-1) {
      INFO("Error receiving data");
      return -1;
    }

    // print src address
    switch(recvAddr.ss_family) {
      case AF_INET:
        v4Addr = (struct sockaddr_in*)&recvAddr;
        INFO("Got packet from %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
      break;

      case AF_INET6:
        v6Addr = (struct sockaddr_in6*)&recvAddr;
        INFO("Got packet from %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
      break;
    }

    // validate packet
    if(ret>BUF_LEN) {
      INFO("PDU too large to fit in pre-allocated buffer");
      continue;
    }
    recvPDU->setPDULength(ret);
    if(recvPDU->validate()!=1) {
      INFO("Malformed CoAP packet");
      continue;
    }
    INFO("Valid CoAP PDU received");
    recvPDU->printHuman();

    // depending on what this is, maybe call callback function
    if(recvPDU->getURI(uriBuffer,URI_BUF_LEN,&recvURILen)!=0) {
      INFO("Error retrieving URI");
      continue;
    }
    if(recvURILen==0) {
      INFO("There is no URI associated with this Coap PDU");
    } else {
      HASH_FIND_STR(directory,uriBuffer,hash);
      if(hash) {
        DBG("Hash id is %d.", hash->id);
        //hash->callback(recvPDU,sockfd,&recvAddr);
        this->gTestCallback(recvPDU,sockfd,&recvAddr);
        continue;
      } else {
        DBG("Hash not found.");
        continue;
      }
    }

    // no URI, handle cases

    // code==0, no payload, this is a ping request, send RST
    if(recvPDU->getPDULength()==0&&recvPDU->getCode()==0) {
      INFO("CoAP ping request");
    }

  }

  return 0;
}
