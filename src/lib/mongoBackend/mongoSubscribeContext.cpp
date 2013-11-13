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
#include "common/globals.h"
#include "common/sem.h"
#include "common/Format.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubscribeContext.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "ngsi/StatusCode.h"

/* ****************************************************************************
*
* mongoSubscribeContext - 
*/
HttpStatusCode mongoSubscribeContext(SubscribeContextRequest* requestP, SubscribeContextResponse* responseP, Format inFormat)
{

    /* Take semaphore. The LM_S* family of macros combines semaphore release with return */
    semTake();

    LM_T(LmtMongo, ("Subscribe Context Request"));

    DBClientConnection* connection = getMongoConnection();

    /* If expiration is not present, then use a default one */
    if (requestP->duration.isEmpty()) {
        requestP->duration.set(DEFAULT_DURATION);
    }

    /* Calculate expiration (using the current time and the duration field in the request) */
    int expiration = getCurrentTime() + requestP->duration.parse();
    LM_T(LmtMongo, ("Subscription expiration: %d", expiration));

    /* Create the mongoDB subscription document */
    BSONObjBuilder sub;
    OID oid;
    oid.init();
    sub.append("_id", oid);
    sub.append(CSUB_EXPIRATION, expiration);
    sub.append(CSUB_REFERENCE, requestP->reference.get());

    /* Throttling */
    if (!requestP->throttling.isEmpty()) {
        sub.append(CSUB_THROTTLING, requestP->throttling.parse());
    }

    /* Build entities array */
    BSONArrayBuilder entities;
    for (unsigned int ix = 0; ix < requestP->entityIdVector.size(); ++ix) {
        EntityId* en = requestP->entityIdVector.get(ix);
        entities.append(BSON(CSUB_ENTITY_ID << en->id <<
                             CSUB_ENTITY_TYPE << en->type <<
                             CSUB_ENTITY_ISPATTERN << en->isPattern));
    }
    sub.append(CSUB_ENTITIES, entities.arr());

    /* Build attributes array */
    BSONArrayBuilder attrs;
    for (unsigned int ix = 0; ix < requestP->attributeList.size(); ++ix) {
        attrs.append(requestP->attributeList.get(ix));
    }
    sub.append(CSUB_ATTRS, attrs.arr());

    /* Build conditions array (including side-effect notifications and threads creation) */
    bool notificationDone = false;
    BSONArray conds = processConditionVector(&requestP->notifyConditionVector,
                                             requestP->entityIdVector,
                                             requestP->attributeList, oid.str(),
                                             requestP->reference.get(),
                                             &notificationDone,
                                             inFormat);
    sub.append(CSUB_CONDITIONS, conds);
    if (notificationDone) {
        sub.append(CSUB_LASTNOTIFICATION, getCurrentTime());
        sub.append(CSUB_COUNT, 1);
    }

    /* Adding format to use in notifications */
    sub.append(CSUB_FORMAT, std::string(formatToString(inFormat)));

    /* Insert document in database */
    BSONObj subDoc = sub.obj();
    try {
        LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", getSubscribeContextCollectionName(), subDoc.toString().c_str()));
        connection->insert(getSubscribeContextCollectionName(), subDoc);
    }
    catch( const DBException &e ) {
        responseP->subscribeError.errorCode.fill(
            SccReceiverInternalError,
            "Database Error",
            std::string("collection: ") + getSubscribeContextCollectionName() +
                " - insert(): " + subDoc.toString() +
                " - exception: " + e.what()
        );

        LM_SRE(SccOk,("Database error '%s'", responseP->subscribeError.errorCode.reasonPhrase.c_str()));
    }    

    /* Fill the response element */
    responseP->subscribeResponse.duration = requestP->duration;
    responseP->subscribeResponse.subscriptionId.set(oid.str());
    responseP->subscribeResponse.throttling = requestP->throttling;

    LM_SR(SccOk);

}
