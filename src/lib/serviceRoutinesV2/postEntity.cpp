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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "common/errorMessages.h"

#include "apiTypesV2/Entities.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/EntityTypeInfo.h"
#include "rest/OrionError.h"
#include "rest/errorAdaptation.h"
#include "serviceRoutinesV2/postEntity.h"
#include "serviceRoutines/postUpdateContext.h"
#include "parse/forbiddenChars.h"


/* ****************************************************************************
*
* postEntity -
*
* POST /v2/entities/{entityId}
*
* Payload In:  Entity
* Payload Out: None
*
* URI parameters:
*   op:    operation
*/
std::string postEntity
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  Entity*        eP  = &parseDataP->ent.res;
  std::string    op  = ciP->uriParam["op"];
  Ngsiv2Flavour  flavor;

  eP->id   = compV[2];
  eP->type = ciP->uriParam["type"];

  if (forbiddenIdChars(ciP->apiVersion, compV[2].c_str() , NULL))
  {
    OrionError oe(SccBadRequest, INVAL_CHAR_URI, "BadRequest");
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  if (ciP->uriParamOptions["append"] == true) // pure-append
  {
    op     = "APPEND_STRICT";
    flavor = NGSIV2_FLAVOUR_ONUPDATE;
  }
  else
  {
    op     = "APPEND";   // append or update
    flavor = NGSIV2_FLAVOUR_ONAPPEND;
  }
  
  // Fill in UpdateContextRequest
  parseDataP->upcr.res.fill(eP, op);

  // Call standard op postUpdateContext
  postUpdateContext(ciP, components, compV, parseDataP, flavor);

#if 0
  // Any error in the response?
  UpdateContextResponse*  upcrsP = &parseDataP->upcrs.res;
  for (unsigned int ix = 0; ix < upcrsP->contextElementResponseVector.size(); ++ix)
  {
    if ((upcrsP->contextElementResponseVector[ix]->statusCode.code != SccOk) &&
        (upcrsP->contextElementResponseVector[ix]->statusCode.code != SccNone))
    {
      if (upcrsP->contextElementResponseVector[ix]->statusCode.code == SccInvalidParameter)
      {
        OrionError oe;
        if (invalidParameterForNgsiv2(upcrsP->contextElementResponseVector[ix]->statusCode.details, &oe))
        {
          ciP->httpStatusCode = oe.code;
          std::string res;
          TIMED_RENDER(res = oe.render());
          eP->release();
          return res;
        }
      }

      OrionError error(upcrsP->contextElementResponseVector[ix]->statusCode);
      std::string  res;

      ciP->httpStatusCode = error.code;
      TIMED_RENDER(res = error.render());
      eP->release();
      return res;
    }
  }

  // Default value for status code: SccNoContent. This is needed as mongoBackend typically
  // uses SccOk (as SccNoContent doesn't exist for NGSIv1)
  if (ciP->httpStatusCode == SccOk)
  {
    ciP->httpStatusCode = SccNoContent;
  }
#endif

  std::string answer = "";
  if (parseDataP->upcrs.res.oe.code != SccNone )
  {
    TIMED_RENDER(answer = parseDataP->upcrs.res.oe.toJson());
    ciP->httpStatusCode = parseDataP->upcrs.res.oe.code;
  }
  else
  {
    ciP->httpStatusCode = SccNoContent;
  }

  // Cleanup and return result
  eP->release();

  return answer;
}
