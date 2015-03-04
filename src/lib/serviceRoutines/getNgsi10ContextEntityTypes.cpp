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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/getNgsi10ContextEntityTypes.h"



/* ****************************************************************************
*
* getNgsi10ContextEntityTypes - 
*
* GET /v1/contextEntityTypes/{typeName}
* GET /ngsi10/contextEntityTypes/{typeName}
*
* Payload In:  None
* Payload Out: QueryContextResponse
*
* URI parameters:
*   - attributesFormat=object
*   [ since the entityId::type is in the URL (and cannot be empty),
*     the URI parameters dealing with entity-type are rendered meaningless ]
*
* 00. Get values from URL (entityId::type)
* 01. Fill in QueryContextRequest
* 02. Call standard operation postQueryContext (that renders the QueryContextResponse)
* 03. If 404 Not Found - enter request entityId::type into response context element
* 04. Cleanup and return result
*/
std::string getNgsi10ContextEntityTypes
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string answer;
  std::string typeName = compV[2];

  // 01. Fill in QueryContextRequest
  parseDataP->qcr.res.fill(".*", typeName, "");


  // 02. Call standard operation postQueryContext (that renders the QueryContextResponse)
  answer = postQueryContext(ciP, components, compV, parseDataP);


  // 03. If 404 Not Found - enter request entityId::type into response context element
  if (parseDataP->qcrs.res.errorCode.code == 404)
  {
    parseDataP->qcrs.res.errorCode.details = std::string("entityId::type /") + typeName + "/ non-existent";
    answer = parseDataP->qcrs.res.render(ciP, Ngsi10ContextEntityTypes, "");
  }


  // 04. Cleanup and return result
  parseDataP->qcr.res.release();
  return answer;
}
