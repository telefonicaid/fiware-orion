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
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeBsonGet.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoOntimeintervalOperations.h"

using namespace mongo;

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo -
*/
HttpStatusCode mongoGetContextSubscriptionInfo
(
  const std::string&       subId,
  ContextSubscriptionInfo* csiP,
  std::string*             err,
  const std::string&       tenant
)
{
    bool          reqSemTaken = false;

    reqSemTake(__FUNCTION__, "get info on subscriptions", SemReadOp, &reqSemTaken);

    LM_T(LmtMongo, ("Get Subscription Info operation"));
    BSONObj sub;
    if (!collectionFindOne(getSubscribeContextCollectionName(tenant), BSON("_id" << OID(subId)), &sub, err))
    {
      reqSemGive(__FUNCTION__, "get info on subscriptions (mongo db exception)", reqSemTaken);
      return SccOk;
    }

    LM_T(LmtMongo, ("retrieved subscription: %s", sub.toString().c_str()));

    /* Check if we found anything */
    if (sub.isEmpty()) {
        reqSemGive(__FUNCTION__, "get info on subscriptions (nothing found)", reqSemTaken);
        return SccOk;
    }

    /* Build the ContextSubcriptionInfo object */
    std::vector<BSONElement> entities = getField(sub, CSUB_ENTITIES).Array();
    for (unsigned int ix = 0; ix < entities.size(); ++ix) {
        BSONObj entity = entities[ix].embeddedObject();
        EntityId* enP = new EntityId;
        enP->id = getStringField(entity, CSUB_ENTITY_ID);
        enP->type = entity.hasField(CSUB_ENTITY_TYPE) ? getStringField(entity, CSUB_ENTITY_TYPE) : "";
        enP->isPattern = getStringField(entity, CSUB_ENTITY_ISPATTERN);
        csiP->entityIdVector.push_back(enP);

    }

    std::vector<BSONElement> attrs = getField(sub, CSUB_ATTRS).Array();
    for (unsigned int ix = 0; ix < attrs.size(); ++ix)
    {
      csiP->attributeList.push_back(attrs[ix].String());
    }

    csiP->url              = getStringField(sub, CSUB_REFERENCE);
    csiP->expiration       = sub.hasField(CSUB_EXPIRATION)?       getIntOrLongFieldAsLong(sub, CSUB_EXPIRATION)       : -1;
    csiP->lastNotification = sub.hasField(CSUB_LASTNOTIFICATION)? getIntOrLongFieldAsLong(sub, CSUB_LASTNOTIFICATION) : -1;
    csiP->throttling       = sub.hasField(CSUB_THROTTLING)?       getIntOrLongFieldAsLong(sub, CSUB_THROTTLING)       : -1;


    /* Get format. If not found in the csubs document (it could happen in the case of updating Orion using an existing database) we use XML */
    std::string fmt = getStringField(sub, CSUB_FORMAT);
    csiP->format = sub.hasField(CSUB_FORMAT)? stringToFormat(fmt) : XML;

    reqSemGive(__FUNCTION__, "get info on subscriptions", reqSemTaken);
    return SccOk;
}

/* ****************************************************************************
*
* mongoGetContextElementResponses -
*
* This function is basically a wrapper of mongoBackend internal entitiesQuery() function
*/
HttpStatusCode mongoGetContextElementResponses(const EntityIdVector& enV, const AttributeList& attrL, ContextElementResponseVector* cerV, std::string* err, const std::string& tenant)
{
    bool reqSemTaken;

    reqSemTake(__FUNCTION__, "get context-element responses", SemReadOp, &reqSemTaken);
    LM_T(LmtMongo, ("Get Notify Context Request operation"));

    // FIXME P10: we are using dummy scope by the moment, until subscription scopes get implemented
    // FIXME P10: we are using an empty service path vector until service paths get implemented for subscriptions
    ContextElementResponseVector rawCerV;
    std::vector<std::string> servicePath;
    Restriction res;
    if (!entitiesQuery(enV, attrL, res, &rawCerV, err, true, tenant, servicePath))
    {
        reqSemGive(__FUNCTION__, "get context-element responses (no entities found)", reqSemTaken);
        rawCerV.release();
        return SccOk;
    }

    /* Prune "not found" CERs */
    pruneContextElements(rawCerV, cerV);
    rawCerV.release();

    reqSemGive(__FUNCTION__, "get context-element responses", reqSemTaken);
    return SccOk;
}

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification -
*
*/
HttpStatusCode mongoUpdateCsubNewNotification(const std::string& subId, std::string* err, const std::string& tenant)
{
    bool           reqSemTaken = false;

    reqSemTake(__FUNCTION__, "update subscription notifications", SemWriteOp, &reqSemTaken);

    LM_T(LmtMongo, ("Update NGI10 Subscription New Notification"));

    /* Update the document */
    BSONObj query  = BSON("_id" << OID(subId));
    BSONObj update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << (long long) getCurrentTime()) << "$inc" << BSON(CSUB_COUNT << 1));
    if (!collectionUpdate(getSubscribeContextCollectionName(tenant), query, update, false, err))
    {
      reqSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo db exception)", reqSemTaken);
      return SccReceiverInternalError;
    }

    reqSemGive(__FUNCTION__, "update subscription notifications", reqSemTaken);
    return SccOk;    
}
