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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUnsubscribeContext.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"

/* ****************************************************************************
*
* mongoUnsubscribeContext - 
*/
HttpStatusCode mongoUnsubscribeContext(UnsubscribeContextRequest* requestP, UnsubscribeContextResponse* responseP, const std::string& tenant)
{
    reqSemTake(__FUNCTION__, "ngsi10 unsubscribe request");

    LM_T(LmtMongo, ("Unsubscribe Context"));

    DBClientConnection* connection = getMongoConnection();

    /* No matter if success or failure, the subscriptionId in the response is always the one
     * in the request */
    responseP->subscriptionId = requestP->subscriptionId;

    /* Look for document */
    BSONObj sub;
    try
    {
        OID id = OID(requestP->subscriptionId.get());
        LM_T(LmtMongo, ("findOne() in '%s' collection _id '%s'}", getSubscribeContextCollectionName(tenant).c_str(),
                           requestP->subscriptionId.get().c_str()));
        mongoSemTake(__FUNCTION__, "findOne in SubscribeContextCollection");
        sub = connection->findOne(getSubscribeContextCollectionName(tenant).c_str(), BSON("_id" << id));
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection");
        LM_I(("Database Operation Successful (findOne _id: %s)", id.toString().c_str()));
    }
    catch (const AssertionException &e)
    {
        /* This happens when OID format is wrong */
        // FIXME: this checking should be done at parsing stage, without progressing to
        // mongoBackend. By the moment we can live this here, but we should remove in the future
        // (old issue #95)
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo assertion exception)");
        reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo assertion exception)");
        responseP->statusCode.fill(SccContextElementNotFound);
        LM_W(("Bad Input (invalid OID format)"));
        return SccOk;
    }
    catch (const DBException &e)
    {
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo db exception)");
        reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)");
        responseP->statusCode.fill(SccReceiverInternalError,
                                   std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                   " - findOne() _id: " + requestP->subscriptionId.get() +
                                   " - exception: " + e.what());
        LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
        return SccOk;
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo generic exception)");
        reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo generic exception)");
        responseP->statusCode.fill(SccReceiverInternalError,
                                   std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                   " - findOne() _id: " + requestP->subscriptionId.get() +
                                   " - exception: " + "generic");
        LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
        return SccOk;
    }

    if (sub.isEmpty()) {
       responseP->statusCode.fill(SccContextElementNotFound, std::string("subscriptionId: '") + requestP->subscriptionId.get() + "'");
       reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)");
       return SccOk;
    }

    /* Remove document in MongoDB */
    // FIXME: I will prefer to do the find and remove in a single operation. Is the some similar
    // to findAndModify for this?
    LM_T(LmtMongo, ("remove() in '%s' collection _id '%s'}", getSubscribeContextCollectionName(tenant).c_str(),
                    requestP->subscriptionId.get().c_str()));
    try
    {
        mongoSemTake(__FUNCTION__, "remove from SubscribeContextCollection");
        connection->remove(getSubscribeContextCollectionName(tenant).c_str(), BSON("_id" << OID(requestP->subscriptionId.get())));
        mongoSemGive(__FUNCTION__, "remove from SubscribeContextCollection");
        LM_I(("Database Operation Successful (remove _id: %s)", requestP->subscriptionId.get().c_str()));
    }
    catch (const DBException &e)
    {
        mongoSemGive(__FUNCTION__, "remove from SubscribeContextCollection (mongo db exception)");
        reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)");
        responseP->statusCode.fill(SccReceiverInternalError,
                                   std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                   " - remove() _id: " + requestP->subscriptionId.get().c_str() +
                                   " - exception: " + e.what());
        LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
        return SccOk;
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "remove from SubscribeContextCollection (mongo generic exception)");
        reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo generic exception)");
        responseP->statusCode.fill(SccReceiverInternalError,
                                   std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                   " - remove() _id: " + requestP->subscriptionId.get().c_str() +
                                   " - exception: " + "generic");

        LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
        return SccOk;
    }

    /* Destroy any previous ONTIMEINTERVAL thread */
    getNotifier()->destroyOntimeIntervalThreads(requestP->subscriptionId.get());

    responseP->statusCode.fill(SccOk);
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request");
    return SccOk;
}
