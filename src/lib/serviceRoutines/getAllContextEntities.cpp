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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "ngsi/ParseData.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/getAllContextEntities.h"
#include "serviceRoutines/postQueryContext.h"



/* ****************************************************************************
*
* getAllContextEntities -
*
* GET /v1/contextEntities
*
* Payload In:  None
* Payload Out: QueryContextResponse
*
* URI parameters:
*   - attributesFormat=object
*   - entity::type=XXX
*   - !exist=entity::type
*   - exist=entity::type
*   - offset=XXX
*   - limit=XXX
*   - details=on
*
* 01. Get values from URL (entityId::type, exist, !exist
* 02. Fill in QueryContextRequest
* 03. Call standard op postQueryContext
* 04. Cleanup and return result
*/
std::string getAllContextEntities
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string     typeName  = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  EntityTypeInfo  typeInfo  = EntityTypeEmptyOrNotEmpty;
  std::string     answer;


  // 01. Get values from URL (entityId::type, exist, !exist)
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }


  // 02. Fill in QueryContextRequest
  parseDataP->qcr.res.fill(".*", typeName, "true", typeInfo, "");


  // 03. Call standard op postQueryContext
  answer = postQueryContext(ciP, components, compV, parseDataP);


  // 04. Cleanup and return result
  parseDataP->qcr.res.release();
  return answer;
}
