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

#include "logMsg/logMsg.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postSubscribeContext.h"
#include "serviceRoutines/postSubscribeContextConvOp.h"



/* ****************************************************************************
*
* postSubscribeContextConvOp - 
*
* POST /v1/contextSubscriptions
* POST /ngsi10/contextSubscriptions
*
* Payload In:  SubscribeContextRequest
* Payload Out: SubscribeContextResponse
*
* URI parameters
*   - !exist=entity::type
*   - exist=entity::type
*   x entity::type=TYPE    (NOT TREATED)
*/
std::string postSubscribeContextConvOp
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string     answer;
  EntityTypeInfo  typeInfo       = EntityTypeEmptyOrNotEmpty;


  // 01. Take care of URI params
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeEmpty;
  }
  else if (ciP->uriParam[URI_PARAM_EXIST] == URI_PARAM_ENTITY_TYPE)
  {
    typeInfo = EntityTypeNotEmpty;
  }


  // 02. Fill in SubscribeContextRequest (actually, modify it)
  parseDataP->scr.res.fill(typeInfo);


  // 03. Call standard operation
  answer = postSubscribeContext(ciP, components, compV, parseDataP);


  // 04. Cleanup and return result
  parseDataP->scr.res.release();

  return answer;
}
