#ifndef SRC_LIB_REST_ORIONLDMHDCONNECTION_H_
#define SRC_LIB_REST_ORIONLDMHDCONNECTION_H_

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
#include <microhttpd.h>

#include "orionld/rest/RestServiceLd.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* orionldMhdConnectionInit - 
*/
extern int orionldMhdConnectionInit
(
  MHD_Connection*  connection,
  const char*      url,
  const char*      method,
  const char*      version,
  void**           con_cls
);



/* ****************************************************************************
*
* orionldMhdConnectionPayloadRead - 
*/
extern int orionldMhdConnectionPayloadRead
(
  ConnectionInfo*  ciP,
  size_t*          upload_data_size,
  const char*      upload_data
);


/* ****************************************************************************
*
* orionldMhdConnectionTreat - 
*/
extern int orionldMhdConnectionTreat(ConnectionInfo* ciP);
  


// -----------------------------------------------------------------------------
//
// orionLdServiceInit -
//
// This function converts the RestServiceLd vectors to OrionLdRestService vectors
//
extern void orionLdServiceInit(void);



// -----------------------------------------------------------------------------
//
// orionLdServiceInitPresent - 
//
extern void orionLdServiceInitPresent(void);

#endif
