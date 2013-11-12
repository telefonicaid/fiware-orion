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

#include "mongoOntimeintervalOperations.h"

#include "common/globals.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "MongoGlobal.h"
#include "mongo/client/dbclient.h"

#include "common/sem.h"

using namespace mongo;

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo -
*/
HttpStatusCode mongoGetContextSubscriptionInfo(std::string subId, ContextSubscriptionInfo* csiP, std::string* err) {

    /* Take semaphore. The LM_S* family of macros combines semaphore release with return */
    semTake();

    LM_T(LmtMongo, ("Get Subscription Info operation"));

    DBClientConnection* connection = getMongoConnection();

    /* Search for the document */
    BSONObj sub;
    try {
        LM_T(LmtMongo, ("findOne() in '%s' collection by _id '%s'", getSubscribeContextCollectionName(), subId.c_str()));
        sub = connection->findOne(getSubscribeContextCollectionName(), BSON("_id" << OID(subId)));
    }
    catch( const DBException &e ) {
        *err = e.what();
        LM_SRE(SccOk,("Database error '%s'", err->c_str()));
    }

    LM_T(LmtMongo, ("retrieved subscription: %s", sub.toString().c_str()));

    /* Check if we found anything */
    if (sub.isEmpty()) {
        LM_SR(SccOk);
    }

    /* Build the ContextSubcriptionInfo object */
    std::vector<BSONElement> entities = sub.getField(CSUB_ENTITIES).Array();
    for (unsigned int ix = 0; ix < entities.size(); ++ix) {
        BSONObj entity = entities[ix].embeddedObject();
        EntityId* enP = new EntityId;
        enP->id = STR_FIELD(entity, CSUB_ENTITY_ID);
        enP->type = STR_FIELD(entity, CSUB_ENTITY_TYPE);
        enP->isPattern = STR_FIELD(entity, CSUB_ENTITY_ISPATTERN);
        csiP->entityIdVector.push_back(enP);

    }
    std::vector<BSONElement> attrs = sub.getField(CSUB_ATTRS).Array();
    for (unsigned int ix = 0; ix < attrs.size(); ++ix) {
        csiP->attributeList.push_back(attrs[ix].String());
    }

    csiP->expiration = sub.getIntField(CSUB_EXPIRATION);    
    csiP->url = STR_FIELD(sub, CSUB_REFERENCE);
    if (sub.hasElement(CSUB_LASTNOTIFICATION)) {
        csiP->lastNotification = sub.getIntField(CSUB_LASTNOTIFICATION);
    }
    else {
        csiP->lastNotification = -1;
    }

    csiP->throttling = sub.hasField(CSUB_THROTTLING) ? sub.getIntField(CSUB_THROTTLING) : -1;

    /* Get format. If not found in the csubs document (it could happen in the case of updating Orion using an existing database) we use XML */
    csiP->format = sub.hasField(CSUB_FORMAT) ? stringToFormat(STR_FIELD(sub, CSUB_FORMAT)) : XML;

    LM_SR(SccOk);
}

/* ****************************************************************************
*
* mongoGetContextElementResponses -
*/
HttpStatusCode mongoGetContextElementResponses(EntityIdVector enV, AttributeList attrL, ContextElementResponseVector* cerV, std::string* err) {

    /* Take semaphore. The LM_S* family of macros combines semaphore release with return */
    semTake();

    LM_T(LmtMongo, ("Get Notify Context Request operation"));

    /* This function is basically a wrapper of mongoBackend internal entitiesQuery() function */

    if (!entitiesQuery(enV, attrL, cerV, err, true)) {
        cerV->release();
        LM_SRE(SccOk, ((*err).c_str()));
    }

    LM_SR(SccOk);
}

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification -
*
*/
HttpStatusCode mongoUpdateCsubNewNotification(std::string subId, std::string* err) {

    /* Take semaphore. The LM_S* family of macros combines semaphore release with return */

    LM_T(LmtMongo, ("Update NGI10 Subscription New Notification"));

    DBClientConnection* connection = getMongoConnection();

    /* Update the document */
    try {
        BSONObj query = BSON("_id" << OID(subId));
        BSONObj update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CSUB_COUNT << 1));
        LM_T(LmtMongo, ("update() in '%s' collection: (%s,%s)", getSubscribeContextCollectionName(),
                        query.toString().c_str(),
                        update.toString().c_str()));
        connection->update(getSubscribeContextCollectionName(), query, update);
    }
    catch( const DBException &e ) {
        *err = e.what();
        LM_SRE(SccOk,("Database error '%s'", err->c_str()));
    }

    LM_SR(SccOk);

}


