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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "serviceRoutinesV2/getEntityAttribute.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/ContextAttribute.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postQueryContext.h"



/* ****************************************************************************
*
* getEntityAttributeValue -
*
* GET /v2/entities/<id>/attrs/<attrName>
*
* Payload In:  None
* Payload Out: Entity Attribute
*
*/
std::string getEntityAttributeValue
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string            answer;
  std::string            entityId = compV[2];
  std::string            attrName = compV[4];
  QueryContextResponse*  qcrsP    = &parseDataP->qcrs.res;

  // Fill in QueryContextRequest
  parseDataP->qcr.res.fill(compV[2], "", attrName);

  // Call standard op postQueryContext
  postQueryContext(ciP, components, compV, parseDataP);

  // Render entity attribute response
  if (qcrsP->contextElementResponseVector.size() > 1)
  {
    ErrorCode ec("TooManyResults", "There is more than one entity with that id - please refine your query");

    ciP->httpStatusCode = SccConflict;
    answer = ec.toJson(true);
  }
  else if (qcrsP->contextElementResponseVector.size() == 0)
  {
    ErrorCode ec("NotFound", "The requested entity has not been found. Check type and id");
    ciP->httpStatusCode = SccContextElementNotFound;
    answer = ec.toJson(true);
  }
  else if (qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector.size() > 1)
  {
    ErrorCode ec("TooManyResults", "There is more than one attribute with that name. Pleasee refine your query");

    ciP->httpStatusCode = SccConflict;
    answer = ec.toJson(true);
  }
  else if (qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector.size() == 0)
  {
    ErrorCode ec("NotFound", "The requested attribute has not been found. Check its name");
    ciP->httpStatusCode = SccContextElementNotFound;
    answer = ec.toJson(true);
  }
  else
  {
    answer = qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector[0]->toJsonAsValue(ciP);
  }

  // Cleanup and return result
  parseDataP->qcr.res.release();
  parseDataP->qcrs.res.release();

  return answer;
}
