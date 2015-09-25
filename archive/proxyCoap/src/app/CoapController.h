#ifndef COAP_CONTROLLER_H
#define COAP_CONTROLLER_H
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

#include <netinet/in.h>
#include <boost/thread.hpp>
#include <string>

#include "cantcoap.h"

class CoapController
{

  std::string     host;
  std::string     coapPortStr;
  unsigned short  httpPort;
  unsigned short  coapPort;

  int sendDatagram(int sockfd, boost::scoped_ptr<CoapPDU>& res, sockaddr* recvFrom);
  int sendError(int sockfd, CoapPDU* req, sockaddr* recvFrom, CoapPDU::Code code);

  int callback(CoapPDU* request, int sockfd, struct sockaddr_storage* recvFrom);

public:
  CoapController(const char* _host, unsigned short _httpPort, unsigned short _coapPort);
  void serve();
};

#endif // COAP_CONTROLLER_H
