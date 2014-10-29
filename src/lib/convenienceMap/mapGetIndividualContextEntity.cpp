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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"

#include "convenienceMap/mapGetIndividualContextEntity.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi/Scope.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"
#include "rest/HttpStatusCode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* mapGetIndividualContextEntity - 
*/
HttpStatusCode mapGetIndividualContextEntity
(
  const std::string&       entityId,
  const std::string&       entityType,
  EntityTypeInfo           typeInfo,
  ContextElementResponse*  response,
  ConnectionInfo*          ciP
)
{
  HttpStatusCode        ms;
  QueryContextRequest   qcRequest;
  QueryContextResponse  qcResponse;
  EntityId              entity(entityId, entityType, "false");

  // Here I fill in the 'query' entityId for the response
  response->contextElement.entityId.fill(entityId, entityType, "false");
  qcRequest.entityIdVector.push_back(&entity);

  if ((typeInfo == EntityTypeEmpty) || (typeInfo == EntityTypeNotEmpty))
  {
    Scope* scopeP = new Scope(SCOPE_FILTER_EXISTENCE, SCOPE_VALUE_ENTITY_TYPE);

    scopeP->oper  = (typeInfo == EntityTypeEmpty)? SCOPE_OPERATOR_NOT : "";
      
    qcRequest.restriction.scopeVector.push_back(scopeP);
  }

  ms = mongoQueryContext(&qcRequest, &qcResponse, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  qcRequest.restriction.release();

  if ((ms != SccOk) || (qcResponse.contextElementResponseVector.size() == 0))
  {
    response->contextElement.entityId.fill(entityId, entityType, "false");
    response->statusCode.fill(SccContextElementNotFound, std::string("Entity id: '") + entityId + "'");
    LM_W(("Bad Input (entityId '%s' not found)", entityId.c_str()));
    return ms;
  }

  ContextElementResponse*         cerP  = qcResponse.contextElementResponseVector[0];
  std::vector<ContextAttribute*>  attrV = cerP->contextElement.contextAttributeVector.vec;

  response->contextElement.entityId.fill(&cerP->contextElement.entityId);
  for (unsigned int ix = 0; ix < attrV.size() ; ++ix)
  {
    ContextAttribute* ca = new ContextAttribute(attrV[ix]);
    response->contextElement.contextAttributeVector.push_back(ca);
  }

  response->contextElement.entityId.type = qcResponse.contextElementResponseVector.get(0)->contextElement.entityId.type;
  response->statusCode.fill(&qcResponse.contextElementResponseVector.get(0)->statusCode);

  return ms;
}
