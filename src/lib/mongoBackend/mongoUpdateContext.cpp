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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <string.h>

#include "common/globals.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonUpdate.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi/NotifyCondition.h"

#include "common/sem.h"

/* ****************************************************************************
*
* mongoUpdateContext - 
*/
HttpStatusCode mongoUpdateContext(UpdateContextRequest* requestP, UpdateContextResponse* responseP)
{

   /* Take semaphore. The LM_S* family of macros combines semaphore release with return */
   semTake();

   /* FIXME: This check is done already, should be removed. */
    if (strcasecmp(requestP->updateActionType.c_str(), "update") != 0 &&
        strcasecmp(requestP->updateActionType.c_str(), "append") != 0 &&
        strcasecmp(requestP->updateActionType.c_str(), "delete") != 0) {

        responseP->errorCode.fill(SccReceiverInternalError, httpStatusCodeString(SccReceiverInternalError), std::string("offending action: ") + requestP->updateActionType.get());
        LM_SRE(SccOk, ("Unknown updateContext action '%s'", requestP->updateActionType.get().c_str()));
    }

    /* Process each ContextElement */
    for (unsigned int ix= 0; ix < requestP->contextElementVector.size(); ++ix) {        
        processContextElement(requestP->contextElementVector.get(ix), responseP, requestP->updateActionType.get());
    }

    /* Note that although individual processContextElements() invokations returns MsConnectionError, this
       error get "encapsulated" in the StatusCode of the corresponding ContextElementResponse and we
       consider the overall mongoUpdateContext() as MsOk. */

    LM_SR(SccOk);
}
