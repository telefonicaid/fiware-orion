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
#include "common/errorMessages.h"

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"
#include "ngsi/QueryContextRequest.h"
#include "alarmMgr/alarmMgr.h"
#include "serviceRoutinesV2/postQueryContext.h"
#include "serviceRoutinesV2/postBatchQuery.h"
#include "serviceRoutinesV2/serviceRoutinesCommon.h"



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
  EntityVector          entities;
  std::string           answer;

  std::string err;
  if (!bqP->expr.fill(&bqP->expr, &err))
  {
    // This is Runtime Error and 500 Internal Server Error as it should not happen:
    // any potential error with the expression should have been caught in the parsing stage
    LM_E(("Runtime Error (filling expression: %s)", err.c_str()));

    ciP->httpStatusCode = SccReceiverInternalError;
    OrionError oe(SccReceiverInternalError, "error filling expression" + err);
    TIMED_RENDER(answer = oe.toJson());

    return answer;
  }

  // To be used later in the render stage
  StringList filterAttrs = bqP->attrsV;

  qcrP->fill(bqP);
  bqP->release();  // qcrP just 'took over' the data from bqP, bqP no longer needed

  postQueryContext(ciP, components, compV, parseDataP);

  // Whichever the case, 200 OK is always returned (in the case of fail with CPr 200 OK [] may be returned)
  // except for some error situations (e.g. duplicated sort tokens in orderBy)
  // (I don't like this code very much, as we are using de description of the error to decide, but
  // I guess that until CPrs functionality gets dropped we cannot do it better...)
  if (parseDataP->qcrs.res.error.description == ERROR_DESC_BAD_REQUEST_DUPLICATED_ORDERBY)
  {
    // FIXME P7: what about of 5xx situationes (e.g. MongoDB errores). They should progress to
    // the client as errors in a similar way

    ciP->httpStatusCode = SccBadRequest;
    OrionError oe(SccBadRequest, parseDataP->qcrs.res.error.description);
    TIMED_RENDER(answer = oe.toJson());
  }
  else
  {
    ciP->httpStatusCode = SccOk;

    // 03. Render Entities response
    if (parseDataP->qcrs.res.contextElementResponseVector.size() == 0)
    {
      answer = "[]";
    }
    else
    {
      OrionError oe;
      entities.fill(parseDataP->qcrs.res, &oe);

      TIMED_RENDER(answer = entities.toJson(getRenderFormat(ciP->uriParamOptions),
                                            filterAttrs.stringV, false, qcrP->metadataList.stringV));
    }
  }

  // 04. Cleanup and return result
  entities.release();
  parseDataP->qcr.res.release();

  return answer;
}
