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
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/getContextEntitiesByEntityId.h"



/* ****************************************************************************
*
* getContextEntitiesByEntityId - 
*
* GET /v1/registry/contextEntities/{entityId::id}
* GET /NGSI9/contextEntities/{entityId::id}
*
* This convenience operation performs an ngsi9 discovery of a fixed
* entityId::id (no wildcards allowed), isPattern == false and type empty,
* which matches ANY type.
*
* However, if the URI parameter '!exist=entity::type', then instead only
* entities with EMPTY types will be discovered.
* 
* Service routine to call: postDiscoverContextAvailability.
*
* As this convop shares output format (DiscoverContextAvailabilityResponse) with its
* corresponding standard operation, there is no need to convert the output.
*/
std::string getContextEntitiesByEntityId
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  entityId = (compV[0] == "v1")? compV[3] : compV[2];
  std::string  answer;

  LM_M(("KZ: CONVENIENCE: got a 'GET' request for entityId '%s'", entityId.c_str()));


  //
  // Fill in parseDataP->dcar.res to pass to postDiscoverContextAvailability
  //
  EntityId                             eId(entityId, "", "");
  std::vector<std::string>             attributeV;
  Restriction                          restriction;

  parseDataP->dcar.res.fill(eId, attributeV, restriction);


  //
  // Debugging
  //
  unsigned int ix;

  LM_M(("KZ: entities:   %d", parseDataP->dcar.res.entityIdVector.size()));
  for (ix = 0; ix < parseDataP->dcar.res.entityIdVector.size(); ++ix)
  {
    LM_M(("KZ: entity %d:   { id='%s', isPattern='%s', type='%s' }",
          ix,
          parseDataP->dcar.res.entityIdVector[ix]->id.c_str(),
          parseDataP->dcar.res.entityIdVector[ix]->isPattern.c_str(),
          parseDataP->dcar.res.entityIdVector[ix]->type.c_str()));
  }

  LM_M(("KZ: attributes: %d", parseDataP->dcar.res.attributeList.size()));
  for (ix = 0; ix < parseDataP->dcar.res.attributeList.size();++ix)
  {
    LM_M(("KZ: attribute %d: '%s'", ix, parseDataP->dcar.res.attributeList[ix].c_str()));
  }

  LM_M(("KZ: scopes:     %d", parseDataP->dcar.res.restriction.scopeVector.size()));
  for (ix = 0; ix < parseDataP->dcar.res.restriction.scopeVector.size();++ix)
  {
    LM_M(("KZ: scope %d: '%s'", 
          ix,
          parseDataP->dcar.res.restriction.scopeVector[ix]->type.c_str(),
          parseDataP->dcar.res.restriction.scopeVector[ix]->value.c_str()));
  }

  //
  // Call the standard operation 
  //
  answer = postDiscoverContextAvailability(ciP, components, compV, parseDataP);


  //
  // Release the data used
  //
  // parseDataP->dcar.res.release();

  return answer;
}
