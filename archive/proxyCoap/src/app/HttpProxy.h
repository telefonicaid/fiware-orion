#ifndef HTTP_PROXY_H
#define HTTP_PROXY_H
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
#include "cantcoap.h"
#include "proxyCoap/HttpMessage.h"

struct MemoryStruct {
  char*   memory;
  size_t  size;
};

static const int  COAP_URI_BUFFER_SIZE = 255;
static const int  COAP_BUFFER_SIZE     = 1024;

extern std::string  sendHttpRequest(const char* host, unsigned short port, CoapPDU* request);

#endif // HTTP_PROXY_H
