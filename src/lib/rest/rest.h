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
extern bool                    multitenant;
extern bool                    corsEnabled;
extern char                    corsOrigin[64];
extern int                     corsMaxAge;
extern RestService*            restBadVerbV;



/* ****************************************************************************
*
* RestServeFunction -
*/
typedef std::string (*RestServeFunction)(ConnectionInfo* ciP);



/* ****************************************************************************
*
* restInit -
*/
extern void restInit
(
   RestService*        _getServiceV,
   RestService*        _putServiceV,
   RestService*        _postServiceV,
   RestService*        _patchServiceV,
   RestService*        _deleteServiceV,
   RestService*        _optionsServiceV,
   RestService*        _noServiceV,
   IpVersion           _ipVersion,
   const char*         _bindAddress,
   unsigned short      _port,
   bool                _multitenant,
   unsigned int        _connectionMemory,
   unsigned int        _maxConnections,
   unsigned int        _mhdThreadPoolSize,
   const char*         _allowedOrigin,
   int                 _corsMaxAge,
   int                 _mhdTimeoutInSeconds,
   const char*         _httpsKey          = NULL,
   const char*         _httpsCert         = NULL
);



/* ****************************************************************************
*
* servicePathCheck -
*/
extern int servicePathCheck(ConnectionInfo* ciP, const char* servicePath);



/* ****************************************************************************
*
* firstServicePath - extract first component of service-path
*/
extern void firstServicePath(const char* servicePath, char* servicePath0, int servicePath0Len);



/* ****************************************************************************
*
* isOriginAllowedForCORS - checks the Origin header of the request and returns
* true if that Origin is allowed to make a CORS request
*/
extern bool isOriginAllowedForCORS(const std::string& requestOrigin);

#endif
