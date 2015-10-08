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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoUnsubscribeContext.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"
#include "cache/subCache.h"



/* ****************************************************************************
*
* mongoUnsubscribeContext - 
*/
HttpStatusCode mongoUnsubscribeContext(UnsubscribeContextRequest* requestP, UnsubscribeContextResponse* responseP, const std::string& tenant)
{
    bool        reqSemTaken;
    std::string err;

    reqSemTake(__FUNCTION__, "ngsi10 unsubscribe request", SemWriteOp, &reqSemTaken);

    LM_T(LmtMongo, ("Unsubscribe Context"));

    /* No matter if success or failure, the subscriptionId in the response is always the one
     * in the request */
    responseP->subscriptionId = requestP->subscriptionId;

    if (responseP->subscriptionId.get() == "")
    {
        responseP->statusCode.fill(SccContextElementNotFound);
        LM_W(("Bad Input (no subscriptionId)"));
        return SccOk;
    }

    /* Look for document */
    BSONObj sub;
    OID     id;
    try
    {
      id = OID(requestP->subscriptionId.get());
    }
    catch (const AssertionException &e)
    {
      reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo assertion exception)", reqSemTaken);
      //
      // This happens when OID format is wrong
      // FIXME: this checking should be done at parsing stage, without progressing to
      // mongoBackend. By the moment we can live this here, but we should remove in the future
      // (old issue #95)
      //
      responseP->statusCode.fill(SccContextElementNotFound);
      LM_W(("Bad Input (invalid OID format)"));
      return SccOk;
    }

    if (!collectionFindOne(getSubscribeContextCollectionName(tenant), BSON("_id" << id), &sub, &err))
    {
      reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);
      responseP->statusCode.fill(SccReceiverInternalError, err);
      return SccOk;
    }

    if (sub.isEmpty())
    {
       reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)", reqSemTaken);

       responseP->statusCode.fill(SccContextElementNotFound, std::string("subscriptionId: /") + requestP->subscriptionId.get() + "/");
       return SccOk;
    }

    /* Remove document in MongoDB */
    // FIXME: I will prefer to do the find and remove in a single operation. Is the some similar
    // to findAndModify for this?    
    if (!collectionRemove(getSubscribeContextCollectionName(tenant), BSON("_id" << OID(requestP->subscriptionId.get())), &err))
    {
      reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);
      responseP->statusCode.fill(SccReceiverInternalError, err);
      return SccOk;
    }

    /* Destroy any previous ONTIMEINTERVAL thread */
    getNotifier()->destroyOntimeIntervalThreads(requestP->subscriptionId.get());

    responseP->statusCode.fill(SccOk);
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request", reqSemTaken);

    //
    // Removing subscription from cache
    //
    orion::subCache->remove(tenant, "", requestP->subscriptionId.get());

    return SccOk;
}
