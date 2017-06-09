/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "apiTypesV2/Entities.h"
#include "ngsi10/QueryContextRequest.h"
#include "alarmMgr/alarmMgr.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutinesV2/postBatchQuery.h"



/* ****************************************************************************
*
* postBatchQuery -
*
* POST /v2/op/query
*
* Payload In:  BatchQueryRequest
* Payload Out: Entities
*
* URI parameters:
*   - limit=NUMBER
*   - offset=NUMBER
*   - options=count,keyValues
*/
std::string postBatchQuery
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  BatchQuery*           bqP  = &parseDataP->bq.res;
  QueryContextRequest*  qcrP = &parseDataP->qcr.res;
  Entities              entities;
  std::string           answer;

  qcrP->fill(bqP);
  bqP->release();  // qcrP just 'took over' the data from bqP, bqP no longer needed

  //
  // This request does not support (it ignores) the URI parameter 'metadata'.
  // Instead the metadata filter comes inside the payload.
  // To pass the metadata filter to the rendering function, the same mechanism as for URI param is used here.
  // This is a little bit ugly as we 'fool' the render method to believe that the filter came via URI param.
  // However, the implementation is cleaner and no more code is needed for this, just to set the
  // URI param with the value of the metadata filter from the payload.
  //
  ciP->uriParam[URI_PARAM_METADATA] = bqP->metadataV.toString();

  answer = postQueryContext(ciP, components, compV, parseDataP);

  if (ciP->httpStatusCode != SccOk)
  {
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

    TIMED_RENDER(answer = entities.render(ciP->uriParamOptions, ciP->uriParam));
  }

  // 04. Cleanup and return result
  entities.release();
  parseDataP->qcr.res.release();

  return answer;
}
