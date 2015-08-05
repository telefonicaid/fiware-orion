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

#include "common/string.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "apiTypesV2/Entities.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutinesV2/getEntities.h"
#include "serviceRoutines/postQueryContext.h"



/* ****************************************************************************
*
* getEntities - 
*
* GET /v2/entities
*
* Payload In:  None
* Payload Out: Entities
*
* URI parameters:
*   - limit=NUMBER
*   - offset=NUMBER
*   - count=true/false
*
* 01. Fill in QueryContextRequest
* 02. Call standard op postQueryContext
* 03. Render Entities response
* 04. Cleanup and return result
*/
std::string getEntities
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  using namespace std;

  Entities     entities;
  typedef vector<string> vstr;
  typedef vstr::size_type vstr_sz;

  // optional parameter for list of IDs
  string listIDs = ciP->uriParam["id"];
  string pattern;

  if (listIDs != "") {
    vstr idsV;
    stringSplit(listIDs, ',', idsV);
    vstr_sz sz = idsV.size();
    for(vstr_sz ix = 0; ix != sz; ++ix) {
      if (ix != 0)
      {
          pattern += "|";
      }
      pattern += idsV[ix];
    }
  }
  else
  {
    pattern = ".*";
  }


  // 01. Fill in QueryContextRequest
  parseDataP->qcr.res.fill(pattern, "", "true", EntityTypeEmptyOrNotEmpty, "");
  

  // 02. Call standard op postQueryContext
  string answer = postQueryContext(ciP, components, compV, parseDataP);


  // 03. Render Entities response
  if (parseDataP->qcrs.res.contextElementResponseVector.size() == 0)
  {
    ciP->httpStatusCode = SccOk;
    answer = "[]";
  }
  else
  {
    entities.fill(&parseDataP->qcrs.res);
    answer = entities.render(ciP, EntitiesResponse);
  }

  // 04. Cleanup and return result
  entities.release();
  parseDataP->qcr.res.release();

  return answer;
}
