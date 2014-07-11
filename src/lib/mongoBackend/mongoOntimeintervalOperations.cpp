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
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoOntimeintervalOperations.h"

using namespace mongo;

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo -
*/
HttpStatusCode mongoGetContextSubscriptionInfo(const std::string& subId, ContextSubscriptionInfo* csiP, std::string* err, const std::string& tenant) {

    reqSemTake(__FUNCTION__, "get info on subscriptions");

    LM_T(LmtMongo, ("Get Subscription Info operation"));

    DBClientConnection* connection = getMongoConnection();

    /* Search for the document */
    LM_T(LmtMongo, ("findOne() in '%s' collection by _id '%s'", getSubscribeContextCollectionName(tenant).c_str(), subId.c_str()));
    BSONObj sub;
    try {
        mongoSemTake(__FUNCTION__, "findOne in SubscribeContextCollection");
        sub = connection->findOne(getSubscribeContextCollectionName(tenant).c_str(), BSON("_id" << OID(subId)));
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection");
    }
    catch( const DBException &e ) {
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo db exception)");
        reqSemGive(__FUNCTION__, "get info on subscriptions (mongo db exception)");
        *err = e.what();
        LM_RE(SccOk, ("Database error '%s'", err->c_str()));
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo generic exception)");
        reqSemGive(__FUNCTION__, "get info on subscriptions (mongo generic exception)");
        *err = "Database error: received generic exception";
        LM_RE(SccOk, ("Database error '%s'", "received generic exception"));
    }

    LM_T(LmtMongo, ("retrieved subscription: %s", sub.toString().c_str()));

    /* Check if we found anything */
    if (sub.isEmpty()) {
        reqSemGive(__FUNCTION__, "get info on subscriptions (nothing found)");
        return SccOk;
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

    BSONElement be = sub.getField(CSUB_EXPIRATION);
    csiP->expiration = be.numberLong();

    csiP->url = STR_FIELD(sub, CSUB_REFERENCE);
    if (sub.hasElement(CSUB_LASTNOTIFICATION)) {
        csiP->lastNotification = sub.getIntField(CSUB_LASTNOTIFICATION);
    }
    else {
        csiP->lastNotification = -1;
    }

    csiP->throttling = sub.hasField(CSUB_THROTTLING) ? sub.getField(CSUB_THROTTLING).numberLong() : -1;

    /* Get format. If not found in the csubs document (it could happen in the case of updating Orion using an existing database) we use XML */
    std::string fmt = STR_FIELD(sub, CSUB_FORMAT);
    csiP->format = sub.hasField(CSUB_FORMAT)? stringToFormat(fmt) : XML;

    reqSemGive(__FUNCTION__, "get info on subscriptions");
    return SccOk;
}

/* ****************************************************************************
*
* mongoGetContextElementResponses -
*/
HttpStatusCode mongoGetContextElementResponses(const EntityIdVector& enV, const AttributeList& attrL, ContextElementResponseVector* cerV, std::string* err, const std::string& tenant) {

    /* This function is basically a wrapper of mongoBackend internal entitiesQuery() function */

    reqSemTake(__FUNCTION__, "get context-element responses");
    LM_T(LmtMongo, ("Get Notify Context Request operation"));

    // FIXME P10: we are using dummy scope by the moment, until subscription scopes get implemented
    // FIXME P10: we are using an empty service path vector until service paths get implemented for subscriptions
    std::vector<std::string> servicePath;
    Restriction res;
    if (!entitiesQuery(enV, attrL, res, cerV, err, true, tenant, servicePath)) {
        reqSemGive(__FUNCTION__, "get context-element responses (no entities found)");
        cerV->release();
        LM_RE(SccOk, ((*err).c_str()));
    }

    reqSemGive(__FUNCTION__, "get context-element responses");
    return SccOk;
}

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification -
*
*/
HttpStatusCode mongoUpdateCsubNewNotification(const std::string& subId, std::string* err, const std::string& tenant) {

    reqSemTake(__FUNCTION__, "update subscription notifications");

    LM_T(LmtMongo, ("Update NGI10 Subscription New Notification"));

    DBClientConnection* connection = getMongoConnection();

    /* Update the document */
    BSONObj query  = BSON("_id" << OID(subId));
    BSONObj update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CSUB_COUNT << 1));

    LM_T(LmtMongo, ("update() in '%s' collection: (%s,%s)", getSubscribeContextCollectionName(tenant).c_str(),
                    query.toString().c_str(),
                    update.toString().c_str()));

    mongoSemTake(__FUNCTION__, "update in SubscribeContextCollection");

    try
    {
        connection->update(getSubscribeContextCollectionName(tenant).c_str(), query, update);
        mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection");
    }
    catch (const DBException &e)
    {
        mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo db exception)");
        *err = e.what();
        LM_W(("Database Error ('%s', '%s')", query.toString().c_str(), err->c_str()));
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo generic exception)");
        *err = "Generic Exception";
        LM_W(("Database Error ('%s', '%s')", query.toString().c_str(), "Generic Exception"));
    }

    reqSemGive(__FUNCTION__, "update subscription notifications");
    return SccOk;
}
