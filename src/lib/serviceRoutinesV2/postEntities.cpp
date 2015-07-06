/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Entities.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutinesV2/postEntities.h"
#include "serviceRoutines/postUpdateContext.h"



/* ****************************************************************************
*
* postEntities - 
*
* POST /v2/entities
*
* Payload In:  Entity
* Payload Out: None
*
* URI parameters:
*   - 
*
* 01. Fill in UpdateContextRequest
* 02. Call standard op postUpdateContext
* 03. Cleanup and return result
*/
std::string postEntities
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity*  eP = &parseDataP->ent.res;


  // 01. Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, "APPEND");
  

  // 02. Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP);


  // 03. Cleanup and return result
  eP->release();

  return "OK";
}
