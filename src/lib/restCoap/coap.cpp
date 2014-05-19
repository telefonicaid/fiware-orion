#include "coap.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#define __USE_POSIX 1
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <string.h>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "HttpProxy.h"
#include "HttpMessage.h"



/* ****************************************************************************
*
* callback -
*/
int Coap::callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom)
{

  socklen_t addrLen = sizeof(struct sockaddr_in);
  if (recvFrom->ss_family == AF_INET6) {
    addrLen = sizeof(struct sockaddr_in6);
  }

  // Translate request from CoAP to HTTP and send it to MHD
  std::string httpResponse; // Will contain HTTP Response
  httpResponse = sendHttpRequest(host, port, request);
  if (httpResponse == "")
  {
    // Could not get an answer
  }

  HttpMessage* hm = new HttpMessage(httpResponse);


  // Translate response from HTTP to CoAP
  CoapPDU *coapResponse = hm->toCoap();
//  CoapPDU *coapResponse = new CoapPDU();

  if (coapResponse == NULL)
  {
    // Could not translate HTTP into CoAP
  }

  // Prepare appropriate response in CoAP
  coapResponse->setVersion(1);
  coapResponse->setMessageID(request->getMessageID());
  //coapResponse->setToken(request->getTokenPointer(),request->getTokenLength());

  //coapResponse->setToken((uint8_t*)"\1\16",2);

//  char *payload = (char*)"This is a mundanely worded test payload.";

//  // respond differently, depending on method code
//  switch (request->getCode())
//  {
//    case CoapPDU::COAP_EMPTY:
//      // makes no sense, send RST
//    break;
//    case CoapPDU::COAP_GET:
//      coapResponse->setCode(CoapPDU::COAP_CONTENT);
//      coapResponse->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_TEXT_PLAIN);
//      coapResponse->setPayload((uint8_t*)payload,strlen(payload));
//    break;
//    case CoapPDU::COAP_POST:
//      coapResponse->setCode(CoapPDU::COAP_CREATED);
//    break;
//    case CoapPDU::COAP_PUT:
//      coapResponse->setCode(CoapPDU::COAP_CHANGED);
//    break;
//    case CoapPDU::COAP_DELETE:
//      coapResponse->setCode(CoapPDU::COAP_DELETED);
//      coapResponse->setPayload((uint8_t*)"DELETE OK",9);
//    break;
//    default:
//    break;
//  }

  // type
  switch (request->getType())
  {
    case CoapPDU::COAP_CONFIRMABLE:
      coapResponse->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;
    case CoapPDU::COAP_NON_CONFIRMABLE:
      coapResponse->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;
    case CoapPDU::COAP_ACKNOWLEDGEMENT:
    break;
    case CoapPDU::COAP_RESET:
    break;
    default:
      return 1;
    break;
  };

  delete request;

  //coapResponse->printHuman();

  // send the packet
  ssize_t sent = sendto(sockfd, coapResponse->getPDUPointer(), coapResponse->getPDULength(), 0, (sockaddr*)recvFrom, addrLen);
  if (sent < 0)
  {
    LM_V(("Error sending packet: %ld.",sent));
    perror(NULL);
    return 1;
  }
  else
  {
    LM_V(("Sent: %ld",sent));
  }

  delete coapResponse;

  return 0;
}



/* ****************************************************************************
*
* serve -
*/
void Coap::serve()
{
  // Buffers for UDP and URIs
  char buffer[COAP_BUFFER_SIZE];
  char uriBuffer[COAP_URI_BUFFER_SIZE];
  int  recvURILen = 0;
  int  ret = 0;

  // storage for handling receive address
  struct sockaddr_storage recvAddr;
  socklen_t               recvAddrLen = sizeof(struct sockaddr_storage);
  //struct sockaddr_in      *v4Addr;
  //struct sockaddr_in6     *v6Addr;
  //char                    straddr[INET6_ADDRSTRLEN];

  // Prepare binding address
  struct addrinfo *bindAddr = NULL;
  struct addrinfo hints;

  LM_V(("Setting up bind address"));
  memset(&hints, 0x00, sizeof(struct addrinfo));
  hints.ai_flags      = 0;
  hints.ai_socktype   = SOCK_DGRAM;
  hints.ai_flags     |= AI_NUMERICSERV;
  hints.ai_family     = PF_INET; // ipv4, PF_INET6 for ipv6 or PF_UNSPEC to let OS decide

  int error = getaddrinfo(host, portString, &hints, &bindAddr);
  if (error)
  {
    LM_V(("Could not start CoAP server: Error getting address info: %s.", gai_strerror(error)));
    return;
  }

  // Setting up the UDP socket
  int sd = socket(bindAddr->ai_family,bindAddr->ai_socktype,bindAddr->ai_protocol);

  // Binding socket
  if (bind(sd,bindAddr->ai_addr,bindAddr->ai_addrlen)!=0)
  {
    LM_V(("Could not start CoAP server: Error binding socket"));
    perror(NULL);
    return;
  }

  // reuse the same PDU
  //CoapPDU *recvPDU = new CoapPDU((uint8_t*)buffer, COAP_BUFFER_SIZE, COAP_BUFFER_SIZE);
  CoapPDU *recvPDU = NULL;

  LM_V(("Listening for packets..."));
  while (1)
  {
    // zero out the buffer
    memset(buffer, 0, COAP_BUFFER_SIZE);

    // receive packet
    ret = recvfrom(sd, &buffer, COAP_BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);
    if (ret == -1)
    {
      LM_V(("Error receiving data"));
      return;
    }

    std::string bufferString;
    bufferString.assign(buffer);

//    uint8_t* data = (uint8_t*)this->body.c_str();
//    std::size_t length = this->body.length();
//    pdu->setPayload(data, length);

    recvPDU = new CoapPDU((uint8_t*)buffer, COAP_BUFFER_SIZE, COAP_BUFFER_SIZE);

    // validate packet
    if (ret > COAP_BUFFER_SIZE)
    {
      LM_V(("PDU too large to fit in pre-allocated buffer"));
      continue;
    }
    recvPDU->setPDULength(ret);
    if (recvPDU->validate() != 1)
    {
      LM_V(("Malformed CoAP packet"));
      continue;
    }
    LM_V(("Valid CoAP PDU received"));
    //recvPDU->printHuman();

    // Treat URI
    if (recvPDU->getURI(uriBuffer, COAP_URI_BUFFER_SIZE, &recvURILen) != 0)
    {
      LM_V(("Error retrieving URI"));
      continue;
    }

    if (recvURILen == 0)
    {
      LM_V(("There is no URI associated with this Coap PDU"));
    }
    else
    {
      // Invoke a callback thread
      boost::thread *workerThread = new boost::thread(boost::bind(&Coap::callback, this, recvPDU, sd, &recvAddr));
      workerThread->get_id();

      // DEBUG: Wait for thread to finnish (like using no threads at all) for now
      workerThread->join();

      continue;
    }

    // no URI, handle cases

    // code == 0, no payload, this is a ping request, send RST
    if ((recvPDU->getPDULength() == 0) && (recvPDU->getCode() == 0))
    {
      LM_V(("CoAP ping request"));
    }

  }
}



/* ****************************************************************************
*
* run -
*/
int Coap::run(const char *_host, unsigned short _port)
{
  char* portString = new char[6];
  snprintf(portString, 6, "%hd", _port);

  this->port = _port;
  this->host = new char[strlen(_host)];
  strcpy(this->host, _host);
  this->portString = new char[strlen(portString)];
  strcpy(this->portString, portString);

  boost::thread *coapServerThread = new boost::thread(boost::bind(&Coap::serve, this));

  coapServerThread->get_id(); // to prevent 'warning: unused'

  return 0;
}

