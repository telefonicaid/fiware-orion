#ifndef REST_H
#define REST_H

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
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "rest/RestService.h"
#include "rest/StringFilter.h"



/* ****************************************************************************
*
* MAX_LEN_IP - 
*/
#define MAX_LEN_IP  64



/* ****************************************************************************
*
* CONSTANTS RESTINIT - 
*/ 
#define   NO_PORT 0



/* ****************************************************************************
*
*  IpVersion -
*/
typedef enum IpVersion 
{
  IPV4     = 1,
  IPV6     = 2,
  IPDUAL   = 3
} IpVersion;



/* ****************************************************************************
*
* Global vars - 
*/
extern IpVersion               ipVersionUsed;  
extern std::string             rushHost;
extern unsigned short          rushPort;
extern bool                    multitenant;
extern char                    restAllowedOrigin[64];



/* ****************************************************************************
*
* RestServeFunction - 
*/
typedef void (*RestServeFunction)(ConnectionInfo* ciP);



/* ****************************************************************************
*
* restInit - 
*/
extern void restInit
(
   RestService*        _restServiceV,
   IpVersion           _ipVersion,
   const char*         _bindAddress, 
   unsigned short      _port,
   bool                _multitenant,
   unsigned int        _connectionMemory,
   unsigned int        _maxConnections,
   unsigned int        _mhdThreadPoolSize,
   const std::string&  _rushHost,
   unsigned short      _rushPort,
   const char*         _allowedOrigin,
   int                 _mhdTimeoutInSeconds,
   const char*         _httpsKey          = NULL,
   const char*         _httpsCert         = NULL,
   RestServeFunction   _serveFunction     = NULL
);

#endif
