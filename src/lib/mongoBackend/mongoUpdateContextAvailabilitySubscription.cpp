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
#include "mongoBackend/mongoUpdateContextAvailabilitySubscription.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionRequest.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionResponse.h"

/* ****************************************************************************
*
* mongoUpdateContextAvailabilitySubscription - 
*/
HttpStatusCode mongoUpdateContextAvailabilitySubscription
(
  UpdateContextAvailabilitySubscriptionRequest*   requestP,
  UpdateContextAvailabilitySubscriptionResponse*  responseP,
  Format                                          notifyFormat,
  const std::string&                              tenant
)
{
  bool reqSemTaken;

  LM_T(LmtMongo, ("Update Context Subscription, notifyFormat: '%s'", formatToString(notifyFormat)));
  reqSemTake(__FUNCTION__, "ngsi9 update subscription request", SemWriteOp, &reqSemTaken);

  DBClientBase* connection = NULL;

  /* Look for document */
  BSONObj  sub;
  try {
      OID id = OID(requestP->subscriptionId.get());

      connection = getMongoConnection();
      sub = connection->findOne(getSubscribeContextAvailabilityCollectionName(tenant).c_str(), BSON("_id" << id));
      releaseMongoConnection(connection);
      
      LM_I(("Database Operation Successful (findOne _id: %s)", id.toString().c_str()));
  }
  catch (const AssertionException &e)
  {
      //
      // This happens when OID format is wrong
      // FIXME: this checking should be done at parsing stage, without progressing to
      // mongoBackend. By the moment we can live this here, but we should remove in the future
      // (old issue #95)
      //
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 update subscription request (mongo assertion exception)", reqSemTaken);

      responseP->errorCode.fill(SccContextElementNotFound);
      LM_W(("Bad Input (invalid OID format)"));
      return SccOk;
  }
  catch (const DBException &e)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 update subscription request (mongo db exception)", reqSemTaken);

      responseP->errorCode.fill(SccReceiverInternalError,
                                std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                " - findOne() _id: " + requestP->subscriptionId.get() +
                                " - exception: " + e.what());
      LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
      return SccOk;
  }
  catch (...)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 update subscription request (mongo generic exception)", reqSemTaken);

      responseP->errorCode.fill(SccReceiverInternalError,
                                std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                " - findOne() _id: " + requestP->subscriptionId.get() +
                                " - exception: " + "generic");
      LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
      return SccOk;
  }

  if (sub.isEmpty())
  {
     responseP->errorCode.fill(SccContextElementNotFound);
     reqSemGive(__FUNCTION__, "ngsi9 update subscription request (no subscriptions found)", reqSemTaken);
     return SccOk;
  }

  /* We start with an empty BSONObjBuilder and process requestP for all the fields that can
   * be updated. I don't like too much this strategy (I would have preferred to start with
   * a copy of the original document, then modify as neded, but this doesn't seem to be easy
   * using the API provide by the Mongo C++ driver)
   *
   * FIXME: a better implementation strategy could be doing an findAndModify() query to do the
   * update, so detecting if the document was not found, instead of using findOne() + update()
   * with $set operation. One operations to MongoDb. vs two operations.
   */
  BSONObjBuilder newSub;

  /* Entities (mandatory) */
  BSONArrayBuilder entities;
  for (unsigned int ix = 0; ix < requestP->entityIdVector.size(); ++ix) {
      EntityId* en = requestP->entityIdVector.get(ix);
      if (en->type == "") {
          entities.append(BSON(CASUB_ENTITY_ID << en->id <<
                               CASUB_ENTITY_ISPATTERN << en->isPattern));
      }
      else {
          entities.append(BSON(CASUB_ENTITY_ID << en->id <<
                               CASUB_ENTITY_TYPE << en->type <<
                               CASUB_ENTITY_ISPATTERN << en->isPattern));
      }

  }
  newSub.append(CASUB_ENTITIES, entities.arr());

  /* Attributes (always taken into account) */
  BSONArrayBuilder attrs;
  for (unsigned int ix = 0; ix < requestP->attributeList.size(); ++ix) {
      attrs.append(requestP->attributeList.get(ix));
  }
  newSub.append(CASUB_ATTRS, attrs.arr());

  /* Duration (optional) */
  if (requestP->duration.isEmpty()) {
      newSub.append(CASUB_EXPIRATION, sub.getField(CASUB_EXPIRATION).numberLong());
  }
  else {
      long long expiration = getCurrentTime() + requestP->duration.parse();
      newSub.append(CASUB_EXPIRATION, expiration);
      LM_T(LmtMongo, ("New subscription expiration: %l", expiration));
  }

  /* Reference is not updatable, so it is appended directly */
  newSub.append(CASUB_REFERENCE, STR_FIELD(sub, CASUB_REFERENCE));

  int count = sub.hasField(CASUB_COUNT) ? sub.getIntField(CASUB_COUNT) : 0;

  /* The hasField check is needed due to lastNotification/count could not be present in the original doc */
  if (sub.hasField(CASUB_LASTNOTIFICATION)) {
      newSub.append(CASUB_LASTNOTIFICATION, sub.getIntField(CASUB_LASTNOTIFICATION));
  }
  if (sub.hasField(CASUB_COUNT)) {
      newSub.append(CASUB_COUNT, count);
  }

  /* Adding format to use in notifications */
  newSub.append(CASUB_FORMAT, std::string(formatToString(notifyFormat)));

  /* Update document in MongoDB */
  BSONObj update = newSub.obj();
  LM_T(LmtMongo, ("update() in '%s' collection _id '%s': %s}", getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
                  requestP->subscriptionId.get().c_str(),
                  update.toString().c_str()));
  try
  {
      connection = getMongoConnection();
      connection->update(getSubscribeContextAvailabilityCollectionName(tenant).c_str(), BSON("_id" << OID(requestP->subscriptionId.get())), update);
      releaseMongoConnection(connection);

      LM_I(("Database Operation Successful (update _id: %s)", requestP->subscriptionId.get().c_str()));
  }
  catch (const DBException &e)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 update subscription request (mongo db exception)", reqSemTaken);

      responseP->errorCode.fill(SccReceiverInternalError,
                                std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                " - update() _id: " + requestP->subscriptionId.get().c_str() +
                                " - update() doc: " + update.toString() +
                                " - exception: " + e.what());

      LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
      return SccOk;
  }
  catch (...)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 update subscription request (mongo generic exception)", reqSemTaken);

      responseP->errorCode.fill(SccReceiverInternalError,
                                std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                " - update() _id: " + requestP->subscriptionId.get().c_str() +
                                " - update() doc: " + update.toString() +
                                " - exception: " + "generic");
      LM_E(("Database Error (%s)", responseP->errorCode.details.c_str()));
      return SccOk;
  }

  /* Send notifications for matching context registrations */
  processAvailabilitySubscription(requestP->entityIdVector, requestP->attributeList, requestP->subscriptionId.get(), STR_FIELD(sub, CASUB_REFERENCE), notifyFormat, tenant);

  /* Duration is an optional parameter, it is only added in the case they
   * was used for update */
  if (!requestP->duration.isEmpty())
  {
      responseP->duration = requestP->duration;
  }

  responseP->subscriptionId = requestP->subscriptionId;

  reqSemGive(__FUNCTION__, "ngsi9 update subscription request", reqSemTaken);

  return SccOk;
}
