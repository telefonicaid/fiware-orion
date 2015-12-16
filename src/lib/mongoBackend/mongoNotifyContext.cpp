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
* Author: Fermín Galán
*/

#include "common/sem.h"

#include "mongoBackend/mongoNotifyContext.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonUpdate.h"
#include "ngsi10/UpdateContextResponse.h"

/* ****************************************************************************
*
* mongoNotifyContext -
*/
HttpStatusCode mongoNotifyContext
(
  NotifyContextRequest*            requestP,
  NotifyContextResponse*           responseP,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV
)
{
    bool reqSemTaken;

    reqSemTake(__FUNCTION__, "ngsi10 notification", SemWriteOp, &reqSemTaken);

    /* We ignore "subscriptionId" and "originator" in the request, as we don't have anything interesting
     * to do with them */

    /* Process each ContextElement */
    for (unsigned int ix = 0; ix < requestP->contextElementResponseVector.size(); ++ix)
    {
        // We use 'ucr' to conform to processContextElement signature but we are not doing anything with that
        UpdateContextResponse  ucr;
        ContextElement*        ceP = &requestP->contextElementResponseVector.operator[] (ix)->contextElement;
        // FIXME P10: we pass an empty uriParams in order to fulfill the processContextElement signature().
        std::map<std::string, std::string> uriParams;

        processContextElement(ceP, &ucr, "append", tenant, servicePathV, uriParams, xauthToken);
    }

    reqSemGive(__FUNCTION__, "ngsi10 notification", reqSemTaken);
    responseP->responseCode.fill(SccOk);

    return SccOk;
}
