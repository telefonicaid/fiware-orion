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
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "ngsi/ParseData.h"
#include "serviceRoutinesV2/getEntities.h"
#include "serviceRoutinesV2/serviceRoutinesCommon.h"
#include "serviceRoutinesV2/postQueryContext.h"
#include "alarmMgr/alarmMgr.h"



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
*   - mq
*   - geometry
*   - coords
*   - georel
*   - attrs
*   - metadata
*   - options=keyValues
*   - orderBy
*   - type=TYPE
*   - type=TYPE1,TYPE2,...TYPEN
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
  EntityVector entities;
  std::string  answer;
  std::string  pattern     = ".*";  // all entities, default value
  std::string  id          = ciP->uriParam["id"];
  std::string  idPattern   = ciP->uriParam["idPattern"];
  std::string  type        = ciP->uriParam["type"];
  std::string  typePattern = ciP->uriParam["typePattern"];
  std::string  q           = ciP->uriParam[URI_PARAM_Q];
  std::string  mq          = ciP->uriParam[URI_PARAM_MQ];
  std::string  geometry    = ciP->uriParam["geometry"];
  std::string  coords      = ciP->uriParam["coords"];
  std::string  georel      = ciP->uriParam["georel"];
  std::string  out;

  if ((!idPattern.empty()) && (!id.empty()))
  {
    OrionError oe(SccBadRequest, "Incompatible parameters: id, IdPattern", ERROR_BAD_REQUEST);

    TIMED_RENDER(answer = oe.toJson());
    ciP->httpStatusCode = oe.code;
    return answer;
  }
  else if (!id.empty())
  {
    pattern = "";

    // FIXME P5: a more efficient query could be possible (this ends as a regex
    // at MongoDB and regex are *expensive* in performance terms)
    std::vector<std::string> idsV;

    stringSplit(id, ',', idsV);

    for (unsigned int ix = 0; ix != idsV.size(); ++ix)
    {
      if (ix != 0)
      {
        pattern += "|^";
      }
      else
      {
        pattern += "^";
      }

      pattern += idsV[ix] + "$";
    }
  }
  else if (!idPattern.empty())
  {
    pattern   = idPattern;
  }

  if ((!typePattern.empty()) && (!type.empty()))
  {
    OrionError oe(SccBadRequest, "Incompatible parameters: type, typePattern", ERROR_BAD_REQUEST);

    TIMED_RENDER(answer = oe.toJson());
    ciP->httpStatusCode = oe.code;
    return answer;
  }

  //
  // Making sure geometry, georel and coords are not used individually
  //
  if ((!coords.empty()) && (geometry.empty()))
  {
    OrionError oe(SccBadRequest, "Invalid query: URI param /coords/ used without /geometry/", ERROR_BAD_REQUEST);

    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;
    return out;
  }
  else if ((!geometry.empty()) && (coords.empty()))
  {
    OrionError oe(SccBadRequest, "Invalid query: URI param /geometry/ used without /coords/", ERROR_BAD_REQUEST);

    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;
    return out;
  }

  if ((!georel.empty()) && (geometry.empty()))
  {
    OrionError oe(SccBadRequest, "Invalid query: URI param /georel/ used without /geometry/", ERROR_BAD_REQUEST);

    TIMED_RENDER(out = oe.toJson());
    ciP->httpStatusCode = oe.code;
    return out;
  }


  //
  // If URI param 'geometry' is present, create a new geo filter.
  // The fill() method of the geo filter checks the validity of the info in:
  // - geometry
  // - georel
  // - coords
  //
  if (!geometry.empty())
  {
    std::string  errorString;

    if (parseDataP->qcr.res.expr.geoFilter.fill(geometry, coords, georel, &errorString) != 0)
    {
      OrionError oe(SccBadRequest, std::string("Invalid query: ") + errorString, ERROR_BAD_REQUEST);

      TIMED_RENDER(out = oe.toJson());
      ciP->httpStatusCode = oe.code;
      return out;
    }
  }



  //
  // String filter in URI param 'q' ?
  // If so, parse the q-string.

  //
  if (!q.empty())
  {
    std::string  errorString;

    if (parseDataP->qcr.res.expr.stringFilter.parse(q.c_str(), &errorString) == false)
    {
      OrionError oe(SccBadRequest, errorString, ERROR_BAD_REQUEST);

      alarmMgr.badInput(clientIp, errorString, q);

      TIMED_RENDER(out = oe.toJson());
      ciP->httpStatusCode = oe.code;
      return out;
    }
  }


  //
  // Metadata string filter in URI param 'mq' ?
  // If so, parse the mq-string.
  //
  if (!mq.empty())
  {
    std::string  errorString;

    if (parseDataP->qcr.res.expr.mdStringFilter.parse(mq.c_str(), &errorString) == false)
    {
      OrionError oe(SccBadRequest, errorString, ERROR_BAD_REQUEST);

      alarmMgr.badInput(clientIp, errorString, mq);

      TIMED_RENDER(out = oe.toJson());
      ciP->httpStatusCode = oe.code;
      return out;
    }
  }


  //
  // 01. Fill in QueryContextRequest - type "" is valid for all types
  //

  //
  // URI param 'type', three options:
  // 1. Not used, so empty
  // 2. Used with a single type name, so add it to the fill
  // 3. Used and with more than ONE typename

  if (!typePattern.empty())
  {
    EntityId* entityId = new EntityId("", pattern, "", typePattern);
    parseDataP->qcr.res.entityIdVector.push_back(entityId);
  }
  else if (ciP->uriParamTypes.size() == 0)
  {
    parseDataP->qcr.res.fill("", pattern, "");
  }
  else if (ciP->uriParamTypes.size() == 1)
  {
    parseDataP->qcr.res.fill("", pattern, type);
  }
  else
  {
    //
    // More than one type listed in URI param 'type':
    // Add an entity per type to QueryContextRequest::entityIdVector
    //
    for (unsigned int ix = 0; ix < ciP->uriParamTypes.size(); ++ix)
    {
      EntityId* entityId = new EntityId("", pattern, ciP->uriParamTypes[ix], "");

      parseDataP->qcr.res.entityIdVector.push_back(entityId);
    }
  }

  // Get attrs and metadata filters from URL params
  setAttrsFilter(ciP->uriParam, ciP->uriParamOptions, &parseDataP->qcr.res.attrsList);
  setMetadataFilter(ciP->uriParam, &parseDataP->qcr.res.metadataList);

  // 02. Call standard op postQueryContext
  postQueryContext(ciP, components, compV, parseDataP);

  // 03. Check Internal Errors
  // (I don't like this code very much, as for the second case we are using de description of the error to decide, but
  // I guess that until CPrs functionality gets dropped we cannot do it better...)
  if ((parseDataP->qcrs.res.error.code == SccReceiverInternalError) || (parseDataP->qcrs.res.error.description == ERROR_DESC_BAD_REQUEST_DUPLICATED_ORDERBY))

  {
    OrionError oe;
    entities.fill(parseDataP->qcrs.res, &oe);
    TIMED_RENDER(answer = oe.toJson());
    ciP->httpStatusCode = oe.code;
  }
  // 04. Render Entities response
  else if (parseDataP->qcrs.res.contextElementResponseVector.size() == 0)
  {
    ciP->httpStatusCode = SccOk;
    answer = "[]";
  }
  else
  {
    OrionError oe;
    entities.fill(parseDataP->qcrs.res, &oe);

    if (oe.code != SccNone)
    {
      TIMED_RENDER(answer = oe.toJson());
      ciP->httpStatusCode = oe.code;
    }
    else
    {
      // Filtering again attributes may seem redundant, but it will prevent
      // that faulty CPrs inject attributes not requested by client
      TIMED_RENDER(answer = entities.toJson(getRenderFormat(ciP->uriParamOptions),
                                            parseDataP->qcr.res.attrsList.stringV,
                                            false,
                                            parseDataP->qcr.res.metadataList.stringV));
      ciP->httpStatusCode = SccOk;
    }
  }

  // 04. Cleanup and return result
  entities.release();
  parseDataP->qcr.res.release();

  return answer;
}
