#ifndef CLIENT_SOCKET_HTTP_H
#define CLIENT_SOCKET_HTTP_H

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* fermin at tid dot es
*
* Author: developer
*/
#include <string>
#include <vector>

#include "ConnectionInfo.h"


#define TAM_BUF          (8 * 1024)            // 8 KB  (for HTTP responses and pre-payload bytes in request, which will be very small)
#define MAX_STA_MSG_SIZE (20 * 1024)           // 20 KB (HTTP request static buffer)
#define MAX_DYN_MSG_SIZE (8 * 1024 * 1024)     // 8 MB  (maximum length of the HTTP request dynamic buffer)


/***************************************************************************
*
* socketHttpConnect - 
*/
extern int socketHttpConnect(std::string host, unsigned short port);



/* ****************************************************************************
*
* sendHttpSocket - 
*/
extern std::string sendHttpSocket( std::string ip,
                                   unsigned short port, 
                                   std::string verb,
                                   std::string resource, 
                                   std::string content_type, 
                                   std::string content,
                                   bool waitForResponse = true
                                   );

#endif
