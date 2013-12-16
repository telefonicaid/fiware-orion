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
* fermin at tid dot es
*
* Author: Fermín Galán
*/

#include "mongoNotifyContext.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonUpdate.h"
#include "common/sem.h"
#include "ngsi10/UpdateContextResponse.h"

/* ****************************************************************************
*
* mongoNofityContext -
*/
HttpStatusCode mongoNotifyContext(NotifyContextRequest* requestP, NotifyContextResponse* responseP) {

    /* Take semaphore. The LM_S* family of macros combines semaphore release with return */
    semTake();

    /* We ignore "subscriptionId" and "originator" in the request, as we don't have anything interesting
     * to do with them */

    /* Process each ContextElement */
    for (unsigned int ix= 0; ix < requestP->contextElementResponseVector.size(); ++ix) {
        /* We use 'ucr' to conform processContextElement signature but we are not doing anything with that */
        UpdateContextResponse ucr;
        processContextElement(&requestP->contextElementResponseVector.get(ix)->contextElement, &ucr, "append");
    }

    responseP->responseCode.fill(SccOk, "OK");

    LM_SR(SccOk);
}
