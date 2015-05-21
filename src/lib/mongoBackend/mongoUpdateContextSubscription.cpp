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

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContextSubscription.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"

#include "common/Format.h"
#include "common/sem.h"

/* ****************************************************************************
*
* mongoUpdateContextSubscription - 
*/
HttpStatusCode mongoUpdateContextSubscription
(
  UpdateContextSubscriptionRequest*   requestP,
  UpdateContextSubscriptionResponse*  responseP,
  Format                              notifyFormat,
  const std::string&                  tenant,
  const std::string&                  xauthToken,
  const std::vector<std::string>&     servicePathV
)
{
  DBClientBase* connection = NULL;
  bool          reqSemTaken;

  reqSemTake(__FUNCTION__, "ngsi10 update subscription request", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Update Context Subscription, notifyFormat: '%s'", formatToString(notifyFormat)));


  /* Look for document */
  BSONObj  sub;
  try
  {
      OID id = OID(requestP->subscriptionId.get());

      connection = getMongoConnection();
      sub = connection->findOne(getSubscribeContextCollectionName(tenant).c_str(), BSON("_id" << id));
      releaseMongoConnection(connection);
      LM_I(("Database Operation Successful (findOne _id: %s)", id.toString().c_str()));
  }
  catch (const AssertionException &e)
  {
      /* This happens when OID format is wrong */
      // FIXME P4: this checking should be done at the parsing stage, without progressing to
      // mongoBackend. For the moment we can leave this here, but we should remove it in the future
      // (old issue #95)
      //
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo assertion exception)", reqSemTaken);

      responseP->subscribeError.errorCode.fill(SccContextElementNotFound);
      LM_W(("Bad Input (invalid OID format)"));
      return SccOk;
  }
  catch (const DBException &e)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo db exception)", reqSemTaken);

      responseP->subscribeError.errorCode.fill(SccReceiverInternalError,
                                               std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                               " - findOne() _id: " + requestP->subscriptionId.get() +
                                               " - exception: " + e.what());
      LM_E(("Database Error (%s)", responseP->subscribeError.errorCode.details.c_str()));
      return SccOk;
  }
  catch (...)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo generic exception)", reqSemTaken);

      responseP->subscribeError.errorCode.fill(SccReceiverInternalError,
                                               std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                               " - findOne() _id: " + requestP->subscriptionId.get() +
                                               " - exception: " + "generic");
      LM_E(("Database Error (%s)", responseP->subscribeError.errorCode.details.c_str()));
      return SccOk;
  }


  if (sub.isEmpty()) {
      responseP->subscribeError.errorCode.fill(SccContextElementNotFound);
      reqSemGive(__FUNCTION__, "ngsi10 update subscription request (no subscriptions found)", reqSemTaken);
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

  /* Entities, attribute list and reference are not updatable, so they are appended directly */
  newSub.appendArray(CSUB_ENTITIES, sub.getField(CSUB_ENTITIES).Obj());
  newSub.appendArray(CSUB_ATTRS, sub.getField(CSUB_ATTRS).Obj());
  newSub.append(CSUB_REFERENCE, STR_FIELD(sub, CSUB_REFERENCE));

  /* Duration update */
  if (requestP->duration.isEmpty()) {      
      newSub.append(CSUB_EXPIRATION, sub.getField(CSUB_EXPIRATION).numberLong());
  }
  else {
      long long expiration = getCurrentTime() + requestP->duration.parse();
      newSub.append(CSUB_EXPIRATION, expiration);
      LM_T(LmtMongo, ("New subscription expiration: %l", expiration));
  }

  /* Restriction update */
  // FIXME: Restrictions not implemented yet

  /* Throttling update */
  if (!requestP->throttling.isEmpty()) {
      /* Throttling equal to 0 removes throttling */
      long long throttling = requestP->throttling.parse();
      if (throttling != 0) {
          newSub.append(CSUB_THROTTLING, throttling);
      }
  }
  else {
      /* The hasField check is needed due to Throttling could not be present in the original doc */
      if (sub.hasField(CSUB_THROTTLING)) {
          newSub.append(CSUB_THROTTLING, sub.getField(CSUB_THROTTLING).numberLong());
      }
  }

  /* Notify conditions */
  bool notificationDone = false;
  if (requestP->notifyConditionVector.size() == 0) {
    newSub.appendArray(CSUB_CONDITIONS, sub.getField(CSUB_CONDITIONS).embeddedObject());
  }
  else {
      /* Destroy any previous ONTIMEINTERVAL thread */
      getNotifier()->destroyOntimeIntervalThreads(requestP->subscriptionId.get());

      /* Build conditions array (including side-effect notifications and threads creation)
       * In order to do so, we have to create and EntityIdVector and AttributeList from sub
       * document, given the processConditionVector() signature */
       EntityIdVector enV = subToEntityIdVector(sub);
       AttributeList attrL = subToAttributeList(sub);

       BSONArray conds = processConditionVector(&requestP->notifyConditionVector,
                                                enV,
                                                attrL,
                                                requestP->subscriptionId.get(),
                                                C_STR_FIELD(sub, CSUB_REFERENCE),
                                                &notificationDone,
                                                notifyFormat,
                                                tenant,
                                                xauthToken,
                                                servicePathV);

       newSub.appendArray(CSUB_CONDITIONS, conds);

       /* Remove EntityIdVector and AttributeList dynamic memory */
       enV.release();
       attrL.release();
  }

  int count = sub.hasField(CSUB_COUNT) ? sub.getIntField(CSUB_COUNT) : 0;

  /* Last notification */
  if (notificationDone) {
      newSub.append(CSUB_LASTNOTIFICATION, getCurrentTime());
      newSub.append(CSUB_COUNT, count + 1);
  }
  else {
      /* The hasField check is needed due to lastNotification/count could not be present in the original doc */
      if (sub.hasField(CSUB_LASTNOTIFICATION)) {
          newSub.append(CSUB_LASTNOTIFICATION, sub.getIntField(CSUB_LASTNOTIFICATION));
      }
      if (sub.hasField(CSUB_COUNT)) {
          newSub.append(CSUB_COUNT, count);
      }
  }

  /* Adding format to use in notifications */
  newSub.append(CSUB_FORMAT, std::string(formatToString(notifyFormat)));

  /* Update document in MongoDB */
  BSONObj update = newSub.obj();
  try
  {
      LM_T(LmtMongo, ("update() in '%s' collection _id '%s': %s}", getSubscribeContextCollectionName(tenant).c_str(),
                         requestP->subscriptionId.get().c_str(),
                         update.toString().c_str()));

      connection = getMongoConnection();
      connection->update(getSubscribeContextCollectionName(tenant).c_str(), BSON("_id" << OID(requestP->subscriptionId.get())), update);
      releaseMongoConnection(connection);

      LM_I(("Database Operation Successful (update _id: %s, %s)", requestP->subscriptionId.get().c_str(), update.toString().c_str()));
  }
  catch (const DBException &e)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo db exception)", reqSemTaken);

      responseP->subscribeError.errorCode.fill(SccReceiverInternalError,
                                               std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                               " - update() _id: " + requestP->subscriptionId.get().c_str() +
                                               " - update() doc: " + update.toString() +
                                               " - exception: " + e.what());

      LM_E(("Database Error (%s)", responseP->subscribeError.errorCode.details.c_str()));
      return SccOk;
  }
  catch (...)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo generic exception)", reqSemTaken);

      responseP->subscribeError.errorCode.fill(SccReceiverInternalError,
                                               std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
                                               " - update() _id: " + requestP->subscriptionId.get().c_str() +
                                               " - update() doc: " + update.toString() +
                                               " - exception: " + "generic");

      LM_E(("Database Error (%s)", responseP->subscribeError.errorCode.details.c_str()));
      return SccOk;
  }

  /* Duration and throttling are optional parameters, they are only added in the case they
   * was used for update */
  if (!requestP->duration.isEmpty()) {      
      responseP->subscribeResponse.duration = requestP->duration;
  }
  if (!requestP->throttling.isEmpty()) {      
      responseP->subscribeResponse.throttling = requestP->throttling;
  }  
  responseP->subscribeResponse.subscriptionId = requestP->subscriptionId;

  reqSemGive(__FUNCTION__, "ngsi10 update subscription request", reqSemTaken);
  return SccOk;
}
