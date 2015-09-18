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
#include "rest/OrionError.h"
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

  Entities  entities;
  string    answer;
  string    pattern    = ".*"; // all entities, default value
  string    id         = ciP->uriParam["id"];
  string    idPattern  = ciP->uriParam["idPattern"];
  string    q          = ciP->uriParam["q"];

  if (ciP->uriParam["options"] == "count")
  {
    ciP->uriParam["count"] = "true";
  }

  if ((idPattern != "") && (id != ""))
  {
    OrionError oe(SccBadRequest, "Incompatible parameters: id, IdPattern");
    answer = oe.render(ciP, "");
    return answer;
  }
  else if (id != "")
  {
    // FIXME: a more efficient query could be possible ...
    vector<string> idsV;

    stringSplit(id, ',', idsV);
    vector<string>::size_type sz = idsV.size();
    for (vector<string>::size_type ix = 0; ix != sz; ++ix)
    {
      if (ix != 0)
      {
          pattern += "|";
      }
      pattern += idsV[ix];
    }
  }
  else if (idPattern != "")
  {
    pattern = idPattern;
  }


  //
  // 01. Fill in QueryContextRequest - type "" is valid for all types
  //
  parseDataP->qcr.res.fill(pattern, ciP->uriParam["type"], "true", EntityTypeEmptyOrNotEmpty, "");

  // If URI param 'q' is given, its value must be put in a scope
  if (q != "")
  {
    Scope* scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY, q);

    parseDataP->qcr.res.restriction.scopeVector.push_back(scopeP);
  }


  // 02. Call standard op postQueryContext
  answer = postQueryContext(ciP, components, compV, parseDataP);

  if (ciP->httpStatusCode != SccOk)
  {
    // Something went wrong in the query, an invalid pattern for example
    return answer;
  }

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
