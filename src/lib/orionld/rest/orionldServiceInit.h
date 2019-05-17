#ifndef SRC_LIB_ORIONLD_REST_ORIONLDSERVICEINIT_H_
#define SRC_LIB_ORIONLD_REST_ORIONLDSERVICEINIT_H_

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
#include "orionld/rest/OrionLdRestService.h"       // OrionLdRestServiceSimplifiedVector



// -----------------------------------------------------------------------------
//
// orionldHostName
//
extern char orionldHostName[1024];
extern int  orionldHostNameLen;



// -----------------------------------------------------------------------------
//
// orionldRestServiceV -
//
extern OrionLdRestServiceVector orionldRestServiceV[9];



// -----------------------------------------------------------------------------
//
// orionldServiceInit -
//
// This function converts the RestServiceLd vectors to OrionLdRestService vectors.
//
// PARAMETERS
//   restServiceVV       - vector of REST services
//   vecItems            - number of items in  the vector
//   defContextFromFile  - do not download Core Context nor Default Context if set to true
//
extern void orionldServiceInit(OrionLdRestServiceSimplifiedVector* restServiceVV, int vecItems, char* cachedContextDir);

#endif  // SRC_LIB_ORIONLD_REST_ORIONLDSERVICEINIT_H_
