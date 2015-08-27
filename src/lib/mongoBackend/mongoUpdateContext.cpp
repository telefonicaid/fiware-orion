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
#include <map>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonUpdate.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi/NotifyCondition.h"
#include "rest/HttpStatusCode.h"

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
  const std::string&                    caller
)
{
    bool reqSemTaken;

    reqSemTake(__FUNCTION__, "ngsi10 update request", SemWriteOp, &reqSemTaken);

    /* Check that the service path vector has only one element, returning error otherwise */
    if (servicePathV.size() > 1)
    {
        LM_W(("Bad Input (service path length %d is greater than the one in update)", servicePathV.size()));
        responseP->errorCode.fill(SccBadRequest, "service path length greater than one in update");
    }
    else
    {
        try
        {
            /* Process each ContextElement */
            for (unsigned int ix = 0; ix < requestP->contextElementVector.size(); ++ix)
            {
                processContextElement(requestP->contextElementVector.get(ix),
                        responseP,
                        requestP->updateActionType.get(),
                        tenant,
                        servicePathV,
                        uriParams,
                        xauthToken,
                        caller);
            }
           /* Note that although individual processContextElements() invocations return ConnectionError, this
              error gets "encapsulated" in the StatusCode of the corresponding ContextElementResponse and we
              consider the overall mongoUpdateContext() as OK. */
            responseP->errorCode.fill(SccOk);
        }
        catch (OrionCreateException &e)
        {
            responseP->errorCode.fill(SccUnprocessableEntity);
            reqSemGive(__FUNCTION__, "ngsi10 update request", reqSemTaken);
            return SccUnprocessableEntity;
        }
    }
    reqSemGive(__FUNCTION__, "ngsi10 update request", reqSemTaken);

    return SccOk;
}
