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
HttpStatusCode mongoUpdateContextSubscription(UpdateContextSubscriptionRequest* requestP, UpdateContextSubscriptionResponse* responseP, Format inFormat)
{

  /* Take semaphore. The LM_S* family of macros combines semaphore release with return */
  semTake();

  LM_T(LmtMongo, ("Update Context Subscription"));

  DBClientConnection* connection = getMongoConnection();

  /* Look for document */
  BSONObj sub;
  try {
      OID id = OID(requestP->subscriptionId.get());

      sub = connection->findOne(getSubscribeContextCollectionName(), BSON("_id" << id));
      if (sub.isEmpty()) {
          responseP->subscribeError.errorCode.fill(SccContextElementNotFound, "Subscription Not Found");
          LM_SR(SccOk);
      }
  }
  catch( const AssertionException &e ) {
      /* This happens when OID format is wrong */
      // FIXME: this checking should be done at parsing stage, without progressing to
      // mongoBackend. By the moment we can live this here, but we should remove in the future
      // (old issue #95)
      responseP->subscribeError.errorCode.fill(SccContextElementNotFound, "Subscription Not Found");
      LM_SR(SccOk);
  }
  catch( const DBException &e ) {
      responseP->subscribeError.errorCode.fill(
          SccReceiverInternalError,
         "Database Error",
          std::string("collection: ") + getSubscribeContextCollectionName() +
             " - findOne() _id: " + requestP->subscriptionId.get() +
             " - exception: " + e.what()
      );
      LM_SR(SccOk);
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
      newSub.append(CSUB_EXPIRATION, sub.getIntField(CSUB_EXPIRATION));
  }
  else {
      int expiration = getCurrentTime() + requestP->duration.parse();
      newSub.append(CSUB_EXPIRATION, expiration);
      LM_T(LmtMongo, ("New subscription expiration: %d", expiration));
  }

  /* Restriction update */
  // FIXME: Restrictions not implemented yet

  /* Throttling update */
  if (!requestP->throttling.isEmpty()) {
      /* Throttling equal to 0 removes throttling */
      int throttling = requestP->throttling.parse();
      if (throttling != 0) {
          newSub.append(CSUB_THROTTLING, throttling);
      }
  }
  else {
      /* The hasField check is needed due to Throttling could not be present in the original doc */
      if (sub.hasField(CSUB_THROTTLING)) {
          newSub.append(CSUB_THROTTLING, sub.getIntField(CSUB_THROTTLING));
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
                                                inFormat);
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
  newSub.append(CSUB_FORMAT, std::string(formatToString(inFormat)));

  /* Update document in MongoDB */
  BSONObj update = newSub.obj();
  try {
      LM_T(LmtMongo, ("update() in '%s' collection _id '%s': %s}", getSubscribeContextCollectionName(),
                         requestP->subscriptionId.get().c_str(),
                         update.toString().c_str()));
      connection->update(getSubscribeContextCollectionName(), BSON("_id" << OID(requestP->subscriptionId.get())), update);
  }
  catch( const DBException &e ) {
      responseP->subscribeError.errorCode.fill(
          SccReceiverInternalError,
          "Database Error",
          std::string("collection: ") + getSubscribeContextCollectionName() +
              " - update() _id: " + requestP->subscriptionId.get().c_str() +
              " - update() doc: " + update.toString() +
              " - exception: " + e.what()
       );

      LM_SR(SccOk);
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

  LM_SR(SccOk);
}
