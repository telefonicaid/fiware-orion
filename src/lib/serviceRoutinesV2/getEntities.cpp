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

#include "common/statistics.h"
#include "common/clockFunctions.h"
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
*   - id
*   - idPattern
*   - q
*   - geometry
*   - coords
*   - options=keyValues
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
  LM_M(("In getEntities"));

  Entities     entities;
  std::string  answer;
  std::string  pattern    = ".*"; // all entities, default value
  std::string  id         = ciP->uriParam["id"];
  std::string  idPattern  = ciP->uriParam["idPattern"];
  std::string  q          = ciP->uriParam["q"];
  std::string  geometry   = ciP->uriParam["geometry"];
  std::string  coords     = ciP->uriParam["coords"];
  std::string  out;

  if ((idPattern != "") && (id != ""))
  {
    OrionError oe(SccBadRequest, "Incompatible parameters: id, IdPattern");

    TIMED_RENDER(answer = oe.render(ciP, ""));
    return answer;
  }
  else if (id != "")
  {
    // FIXME: a more efficient query could be possible ...
    std::vector<std::string> idsV;

    stringSplit(id, ',', idsV);

    for (unsigned int ix = 0; ix != idsV.size(); ++ix)
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


  // Making sure geometry and coords are not used individually
  if ((coords != "") && (geometry == ""))
  {
    OrionError   oe(SccBadRequest, "URI param /coords/ used without /geometry/");

    TIMED_RENDER(out = oe.render(ciP, ""));
    return out;
  }
  else if ((geometry != "") && (coords == ""))
  {
    OrionError oe(SccBadRequest, "URI param /geometry/ used without /coords/");

    TIMED_RENDER(out = oe.render(ciP, ""));

    return out;
  }

  // Making sure geometry is valid (if present)
  orion::Geometry           geo;
  std::vector<std::string>  coordsV;

  if (geometry != "")
  {
    std::string  errorString;

    if (geo.parse(geometry.c_str(), &errorString) != 0)
    {
      OrionError oe(SccBadRequest, std::string("error parsing geometry: ") + errorString);

      TIMED_RENDER(out = oe.render(ciP, ""));

      return out;
    }

    if ((geo.areaType != "polygon") && (geo.areaType != "circle"))
    {
      OrionError oe(SccBadRequest, "URI param /geometry/ must be either /polygon/ or /circle/");

      TIMED_RENDER(out = oe.render(ciP, ""));

      return out;
    }

    //
    // As 'geometry' is present, so is 'coords' - checking coords
    //
    int noOfCoords = stringSplit(coords, ';', coordsV);

    if (noOfCoords == 0)
    {
      OrionError oe(SccBadRequest, "URI param /coords/ has no coordinates");

      TIMED_RENDER(out = oe.render(ciP, ""));

      return out;
    }

    if ((geo.areaType == "circle") && (noOfCoords != 1))
    {
      OrionError oe(SccBadRequest, "Too many coordinates for circle");

      TIMED_RENDER(out = oe.render(ciP, ""));

      return out;
    }

    if ((geo.areaType == "polygon") && (noOfCoords < 3))
    {
      OrionError oe(SccBadRequest, "Too few coordinates for polygon");

      TIMED_RENDER(out = oe.render(ciP, ""));

      return out;
    }
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


  // If URI params 'geometry' and 'coords' are given, another scope is to be created for this
  if ((coords != "") && (geometry != ""))
  {
    Scope*       scopeP = new Scope(SCOPE_TYPE_LOCATION, "");
    std::string  errorString;

    if (scopeP->fill(&geo, coordsV, &errorString) != 0)
    {
      OrionError oe(SccBadRequest, errorString);

      TIMED_RENDER(out = oe.render(ciP, ""));

      return out;
    }

    parseDataP->qcr.res.restriction.scopeVector.push_back(scopeP);
  }


  // 02. Call standard op postQueryContext
  answer = postQueryContext(ciP, components, compV, parseDataP);

  if (ciP->httpStatusCode != SccOk)
  {
    // Something went wrong in the query, an invalid pattern for example

    parseDataP->qcr.res.release();

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

    TIMED_RENDER(answer = entities.render(ciP, EntitiesResponse));
  }

  // 04. Cleanup and return result
  entities.release();
  parseDataP->qcr.res.release();

  return answer;
}
