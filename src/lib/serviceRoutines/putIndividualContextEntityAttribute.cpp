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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "ngsi/ParseData.h"
#include "ngsi/StatusCode.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "rest/ConnectionInfo.h"
#include "ngsi10/UpdateContextRequest.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/putIndividualContextEntityAttribute.h"



/* ****************************************************************************
*
* putIndividualContextEntityAttribute -
*
* PUT /v1/contextEntities/{entityId::id}/attributes/{attributeName}
* PUT /ngsi10/contextEntities/{entityId::id}/attributes/{attributeName}
*
* Payload In:  UpdateContextAttributeRequest
* Payload Out: StatusCode
*
* URI parameters:
*   - entity::type=TYPE
*   - note that '!exist=entity::type' and 'exist=entity::type' are not supported by convenience operations
*     that use the standard operation UpdateContext as there is no restriction within UpdateContext.
*   [ attributesFormat=object: makes no sense for this operation as StatusCode is returned ]
*
* 0. Take care of URI params
* 1. Fill in UpdateContextRequest from UpdateContextAttributeRequest and URL-path components
* 2. Call postUpdateContext standard service routine
* 3. Translate UpdateContextResponse to StatusCode
* 4. Cleanup and return result
*/
std::string putIndividualContextEntityAttribute
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  answer;
  std::string  entityId      = compV[2];
  std::string  entityType    = ciP->uriParam[URI_PARAM_ENTITY_TYPE];
  std::string  attributeName = compV[4];
  StatusCode   response;


  // 1. Fill in UpdateContextRequest from UpdateContextAttributeRequest and URL-path components
  parseDataP->upcr.res.fill(&parseDataP->upcar.res, entityId, entityType, attributeName, "UPDATE");


  // 2. Call postUpdateContext standard service routine
  postUpdateContext(ciP, components, compV, parseDataP);


  // 3. Translate UpdateContextResponse to StatusCode
  response.fill(parseDataP->upcrs.res);


  // 4. Cleanup and return result
  TIMED_RENDER(answer = response.render(false, false));

  response.release();
  parseDataP->upcr.res.release();  // This call to release() crashed the functional test
                                   // 647_crash_with_compounds/PUT_v1_contextEntities_E1_attributes_A1.test

  return answer;
}
