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

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/string.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Entities.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutinesV2/getEntities.h"
#include "serviceRoutines/postQueryContext.h"



/* ****************************************************************************
*
* getEntity -
*
* GET /v2/entities/:id:[?attrs=:list:]
*
* Payload In:  None
* Payload Out: Entity
*
*
* Fill in QueryContextRequest
* Call standard op postQueryContext
* Render Entity response
* Cleanup and return result
*/
std::string getEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  using namespace std;


  // Fill in QueryContextRequest
  parseDataP->qcr.res.fill(compV[2], "", "false", EntityTypeEmptyOrNotEmpty, "");
  // optional parameter for attributes
  string attrs = ciP->uriParam["attrs"];

  if (attrs != "")
  {
    vector<string> attrsV;

    stringSplit(attrs, ',', attrsV);
    for (vector<string>::const_iterator it = attrsV.begin(); it != attrsV.end(); ++it)
    {
      parseDataP->qcr.res.attributeList.push_back_if_absent(*it);
    }
  }

  // Call standard op postQueryContext
  postQueryContext(ciP, components, compV, parseDataP);


  // Render entity response
  Entity       entity;

  entity.fill(&parseDataP->qcrs.res);

  std::string answer;
  TIMED_RENDER(answer = entity.render(ciP, EntityResponse));

  if (parseDataP->qcrs.res.errorCode.code == SccOk && parseDataP->qcrs.res.contextElementResponseVector.size() > 1)
  {
      // No problem found, but we expect only one entity
      ciP->httpStatusCode = SccConflict;
  }
  else
  {
      // the same of the wrapped operation
      ciP->httpStatusCode = parseDataP->qcrs.res.errorCode.code;
  }

  // 04. Cleanup and return result
  entity.release();
  parseDataP->qcr.res.release();

  return answer;
}

