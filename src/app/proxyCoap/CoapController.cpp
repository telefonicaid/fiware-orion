/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: TID Developer
*/

#include "CoapController.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <string.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "proxyCoap/HttpProxy.h"
#include "proxyCoap/HttpMessage.h"

#define PORT_STRING_SIZE 6

CoapController::CoapController(const char *_host, unsigned short _httpPort, unsigned short _coapPort)
{
  // Read host and port values
  httpPort    = _httpPort;
  coapPort    = _coapPort;
  host.assign(_host);

  char* portString = new char[PORT_STRING_SIZE];
  snprintf(portString, PORT_STRING_SIZE, "%d", _coapPort);
  coapPortStr.assign(portString);
}

/* ****************************************************************************
*
* sendDatagram -
*/
int CoapController::sendDatagram(int sockfd, boost::scoped_ptr<CoapPDU>& res, sockaddr* recvFrom)
{
  socklen_t addrLen = sizeof(struct sockaddr_in);
  if (recvFrom->sa_family == AF_INET6)
  {
    addrLen = sizeof(struct sockaddr_in6);
  }

  ssize_t sent = sendto(sockfd, res->getPDUPointer(), res->getPDULength(), 0, recvFrom, addrLen);

  if (sent < 0)
  {
    LM_W(("Error sending packet: %ld.", sent));
    return 1;
  }
  else
  {
    LM_T(LmtCoap, ("Sent: %ld bytes", sent));
  }

  return 0;
}

int CoapController::sendError(int sockfd, CoapPDU* req, sockaddr* recvFrom, CoapPDU::Code code)
{
  boost::scoped_ptr<CoapPDU> res(new CoapPDU());

  res->setVersion(1);
  res->setMessageID(req->getMessageID());
  res->setCode(code);
  res->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
  res->setToken(req->getTokenPointer(), req->getTokenLength());

  return sendDatagram(sockfd, res, recvFrom);
}

/* ****************************************************************************
*
* callback -
*/
int CoapController::callback(CoapPDU* request, int sockfd, struct sockaddr_storage* recvFrom)
{
  // Translate request from CoAP to HTTP and send it to MHD through loopback
  std::string httpResponse;
  httpResponse = sendHttpRequest(host.c_str(), httpPort, request);

  if (httpResponse == "")
  {
    // Could not get an answer from HTTP module
    sendError(sockfd, request, (sockaddr*)recvFrom, CoapPDU::COAP_INTERNAL_SERVER_ERROR);
    return 1;
  }

  // Parse HTTP response
  boost::scoped_ptr<HttpMessage> hm(new HttpMessage(httpResponse));

  // If CoAP message is too big, must send error to requester
  if (hm->contentLength() > COAP_BUFFER_SIZE)
  {
    sendError(sockfd, request, (sockaddr*)recvFrom, CoapPDU::COAP_REQUEST_ENTITY_TOO_LARGE);
    return 1;
  }

  // Translate response from HTTP to CoAP
  boost::scoped_ptr<CoapPDU> coapResponse(hm->toCoap());

  if (!coapResponse)
  {
    // Could not translate HTTP into CoAP
    sendError(sockfd, request, (sockaddr*)recvFrom, CoapPDU::COAP_INTERNAL_SERVER_ERROR);
    return 1;
  }

  // Prepare appropriate response in CoAP
  coapResponse->setVersion(1);
  coapResponse->setMessageID(request->getMessageID());
  coapResponse->setToken(request->getTokenPointer(), request->getTokenLength());

  // Set type
  switch (request->getType())
  {
  case CoapPDU::COAP_CONFIRMABLE:
  case CoapPDU::COAP_NON_CONFIRMABLE:
    coapResponse->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;

  case CoapPDU::COAP_ACKNOWLEDGEMENT:
  case CoapPDU::COAP_RESET:
    break;

  default:
    return 1;
    break;
  };

  // Send the packet
  sendDatagram(sockfd, coapResponse, (sockaddr*) recvFrom);
  return 0;
}



/* ****************************************************************************
*
* serve -
*/
void CoapController::serve()
{
  // Buffers for UDP and URIs
  char buffer[COAP_BUFFER_SIZE];
  char uriBuffer[COAP_URI_BUFFER_SIZE];
  int  recvURILen = 0;
  int  ret = 0;

  // Storage for handling receive address
  struct sockaddr_storage recvAddr;
  socklen_t               recvAddrLen = sizeof(struct sockaddr_storage);

  // Prepare binding address
  struct addrinfo *bindAddr = NULL;
  struct addrinfo hints;

  // Setting up bind address
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype   = SOCK_DGRAM;
  hints.ai_flags     |= AI_NUMERICSERV;
  hints.ai_family     = AF_INET; // ipv4, PF_INET6 for ipv6 or PF_UNSPEC to let OS decide

  int error = getaddrinfo(host.c_str(), coapPortStr.c_str(), &hints, &bindAddr);
  if (error)
  {
    LM_W(("Could not start CoAP server: Error getting address info: %s.", gai_strerror(error)));
    return;
  }

  // Setting up the UDP socket
  int sd = socket(bindAddr->ai_family, bindAddr->ai_socktype, bindAddr->ai_protocol);

  // Binding socket
  if (bind(sd, bindAddr->ai_addr, bindAddr->ai_addrlen) != 0)
  {
    LM_W(("Could not start CoAP server: Error binding socket"));
    return;
  }

  while (1)
  {
    // zero out the buffer
    memset(buffer, 0, COAP_BUFFER_SIZE);

    // receive packet
    ret = recvfrom(sd, &buffer, COAP_BUFFER_SIZE, 0, (sockaddr*) &recvAddr, &recvAddrLen);
    if (ret == -1)
    {
      LM_W(("Error receiving data"));
      continue;
    }

    boost::scoped_ptr<CoapPDU> recvPDU(new CoapPDU((uint8_t*) buffer, COAP_BUFFER_SIZE, COAP_BUFFER_SIZE));

    // validate packet
    if (ret > COAP_BUFFER_SIZE)
    {
      LM_W(("PDU too large to fit in pre-allocated buffer"));
      continue;
    }
    recvPDU->setPDULength(ret);
    if (recvPDU->validate() != 1)
    {
      LM_W(("Malformed CoAP packet"));
      continue;
    }
    LM_T(LmtCoap, ("Valid CoAP PDU received"));

    // Treat URI
    if (recvPDU->getURI(uriBuffer, COAP_URI_BUFFER_SIZE, &recvURILen) != 0)
    {
      LM_W(("Error retrieving URI"));
      continue;
    }

    if (recvURILen == 0)
    {
      LM_T(LmtCoap, ("There is no URI associated with this Coap PDU"));
    }
    else
    {
      // Invoke a callback thread
      boost::thread* workerThread = new boost::thread(boost::bind(&CoapController::callback, this, recvPDU.get(), sd, &recvAddr));

      // Wait for thread to finnish (like using no threads at all) for now
      workerThread->join();

      continue;
    }

    // no URI, handle cases

    // code == 0, no payload, this is a ping request, send RST?
    if ((recvPDU->getPDULength() == 0) && (recvPDU->getCode() == 0))
    {
      LM_T(LmtCoap, ("CoAP ping request"));
    }

  }
}
