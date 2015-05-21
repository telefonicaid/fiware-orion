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
#include "common/globals.h"
#include "common/Format.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubscribeContext.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/SubscribeContextResponse.h"
#include "ngsi/StatusCode.h"
#include "rest/uriParamNames.h"

/* ****************************************************************************
*
* mongoSubscribeContext - 
*/
HttpStatusCode mongoSubscribeContext
(
  SubscribeContextRequest*             requestP,
  SubscribeContextResponse*            responseP,
  const std::string&                   tenant,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   xauthToken,
  const std::vector<std::string>&      servicePathV
)
{
    const std::string  notifyFormatAsString  = uriParam[URI_PARAM_NOTIFY_FORMAT];
    Format             notifyFormat          = stringToFormat(notifyFormatAsString);
    std::string        servicePath           = (servicePathV.size() == 0)? "" : servicePathV[0];
    DBClientBase*      connection            = NULL;
    bool               reqSemTaken           = false;

    LM_T(LmtMongo, ("Subscribe Context Request: notifications sent in '%s' format", notifyFormatAsString.c_str()));

    reqSemTake(__FUNCTION__, "ngsi10 subscribe request", SemWriteOp, &reqSemTaken);

    /* If expiration is not present, then use a default one */
    if (requestP->duration.isEmpty()) {
        requestP->duration.set(DEFAULT_DURATION);
    }

    /* Calculate expiration (using the current time and the duration field in the request) */
    long long expiration = getCurrentTime() + requestP->duration.parse();
    LM_T(LmtMongo, ("Subscription expiration: %lu", expiration));

    /* Create the mongoDB subscription document */
    BSONObjBuilder sub;
    OID oid;
    oid.init();
    sub.append("_id", oid);
    sub.append(CSUB_EXPIRATION, expiration);
    sub.append(CSUB_REFERENCE, requestP->reference.get());

    /* Throttling */
    if (!requestP->throttling.isEmpty()) {
      sub.append(CSUB_THROTTLING, (long long) requestP->throttling.parse());
    }

    if (servicePath != "")
    {
      sub.append(CSUB_SERVICE_PATH, servicePath);
    }

    
    /* Build entities array */
    BSONArrayBuilder entities;
    for (unsigned int ix = 0; ix < requestP->entityIdVector.size(); ++ix)
    {
        EntityId* en = requestP->entityIdVector.get(ix);

        if (en->type == "")
        {
          entities.append(BSON(CSUB_ENTITY_ID << en->id <<
                               CSUB_ENTITY_ISPATTERN << en->isPattern));
        }
        else
        {
          entities.append(BSON(CSUB_ENTITY_ID << en->id <<
                               CSUB_ENTITY_TYPE << en->type <<
                               CSUB_ENTITY_ISPATTERN << en->isPattern));
        }
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
                                             requestP->attributeList, oid.toString(),
                                             requestP->reference.get(),
                                             &notificationDone,
                                             notifyFormat,
                                             tenant,
                                             xauthToken,
                                             servicePathV);
    sub.append(CSUB_CONDITIONS, conds);
    if (notificationDone) {
        sub.append(CSUB_LASTNOTIFICATION, getCurrentTime());
        sub.append(CSUB_COUNT, 1);
    }

    /* Adding format to use in notifications */
    sub.append(CSUB_FORMAT, notifyFormatAsString);

    /* Insert document in database */
    BSONObj subDoc = sub.obj();
    LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", getSubscribeContextCollectionName(tenant).c_str(), subDoc.toString().c_str()));

    try
    {
        connection = getMongoConnection();
        connection->insert(getSubscribeContextCollectionName(tenant).c_str(), subDoc);
        releaseMongoConnection(connection);
        LM_I(("Database Operation Successful (insert %s)", subDoc.toString().c_str()));
    }
    catch (const DBException &e)
    {
        releaseMongoConnection(connection);
        reqSemGive(__FUNCTION__, "ngsi10 subscribe request (mongo db exception)", reqSemTaken);

        responseP->subscribeError.errorCode.fill(SccReceiverInternalError,
                                                 std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                                 " - insert(): " + subDoc.toString() +
                                                 " - exception: " + e.what());

        LM_E(("Database Error ('insert %s into %s', '%s'", subDoc.toString().c_str(), getSubscribeContextCollectionName(tenant).c_str(), e.what()));
        return SccOk;
    }    
    catch (...)
    {
        releaseMongoConnection(connection);
        reqSemGive(__FUNCTION__, "ngsi10 subscribe request (mongo generic exception)", reqSemTaken);

        responseP->subscribeError.errorCode.fill(SccReceiverInternalError,
                                                 std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                                 " - insert(): " + subDoc.toString() +
                                                 " - exception: " + "generic");

        LM_E(("Database Error ('insert %s into %s', '%s'", subDoc.toString().c_str(), getSubscribeContextCollectionName(tenant).c_str(), "generic exception"));
        return SccOk;
    }    

    reqSemGive(__FUNCTION__, "ngsi10 subscribe request", reqSemTaken);

    /* Fill the response element */
    responseP->subscribeResponse.duration = requestP->duration;
    responseP->subscribeResponse.subscriptionId.set(oid.toString());
    responseP->subscribeResponse.throttling = requestP->throttling;

    return SccOk;
}
