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
* Author: Fermin Galan Marquez
*/
#include <string.h>
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/sem.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi/NotifyCondition.h"
#include "rest/HttpStatusCode.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonUpdate.h"
#include "mongoBackend/mongoUpdateContext.h"



/* ****************************************************************************
*
* mongoUpdateContext - 
*/
HttpStatusCode mongoUpdateContext
(
  UpdateContextRequest*                 requestP,
  UpdateContextResponse*                responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams,    // FIXME P7: we need this to implement "restriction-based" filters
  const std::string&                    xauthToken,
  const std::string&                    fiwareCorrelator,
  const std::string&                    ngsiV2AttrsFormat,
  ApiVersion                            apiVersion,
  Ngsiv2Flavour                         ngsiv2Flavour
)
{
  bool reqSemTaken;

  reqSemTake(__FUNCTION__, "ngsi10 update request", SemWriteOp, &reqSemTaken);

  /* Check that the service path vector has only one element, returning error otherwise */
  if (servicePathV.size() > 1)
  {
    char lenV[STRING_SIZE_FOR_INT];

    snprintf(lenV, sizeof(lenV), "%lu", (unsigned long) servicePathV.size());

    std::string details = std::string("service path length ") + lenV + " is greater than the one in update";
    alarmMgr.badInput(clientIp, details);
    responseP->errorCode.fill(SccBadRequest, "service path length greater than the one in update");
    responseP->oe.fill(SccBadRequest, "service path length greater than the one in update", "BadRequest");
  }
  else
  {
    LM_TMP(("contextElementVector Size: %d", requestP->contextElementVector.size()));
    /* Process each ContextElement */
    for (unsigned int ix = 0; ix < requestP->contextElementVector.size(); ++ix)
    {
      processContextElement(requestP->contextElementVector[ix],
                            responseP,
                            requestP->updateActionType,
                            tenant,
                            servicePathV,
                            uriParams,
                            xauthToken,
                            fiwareCorrelator,
                            ngsiV2AttrsFormat,
                            apiVersion,
                            ngsiv2Flavour);
    }

    /* Note that although individual processContextElements() invocations return ConnectionError, this
       error gets "encapsulated" in the StatusCode of the corresponding ContextElementResponse and we
       consider the overall mongoUpdateContext() as OK.
    */
    responseP->errorCode.fill(SccOk);
  }

  reqSemGive(__FUNCTION__, "ngsi10 update request", reqSemTaken);
  return SccOk;
}
