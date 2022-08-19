#ifndef SRC_APP_CONTEXTBROKER_ORIONRESTSERVICES_H_
#define SRC_APP_CONTEXTBROKER_ORIONRESTSERVICES_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "rest/rest.h"



/* ****************************************************************************
*
* ORION_REST_SERVICE_END - End marker for the Rest Service array
*/
#define ORION_REST_SERVICE_END  { InvalidRequest,  0, {}, NULL }



/* ****************************************************************************
*
* orionRestServicesInit - 
*/
extern void orionRestServicesInit
(
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
   const char*         _httpsKey,
   const char*         _httpsCert
);

#endif  // SRC_APP_CONTEXTBROKER_ORIONRESTSERVICES_H_
