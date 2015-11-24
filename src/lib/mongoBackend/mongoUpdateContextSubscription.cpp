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
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoUpdateContextSubscription.h"
#include "mongoBackend/mongoSubCache.h"
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
  bool          reqSemTaken;

  reqSemTake(__FUNCTION__, "ngsi10 update subscription request", SemWriteOp, &reqSemTaken);

  /* Look for document */
  BSONObj     sub;
  std::string err;
  OID         id;

  if (!safeGetSubId(requestP->subscriptionId, &id, &(responseP->subscribeError.errorCode)))
  {
    reqSemGive(__FUNCTION__, "ngsi10 update subscription request (safeGetSubId fail)", reqSemTaken);
    if (responseP->subscribeError.errorCode.code == SccContextElementNotFound)
    {
      LM_W(("Bad Input (invalid OID format: %s)", requestP->subscriptionId.get().c_str()));
    }
    else // SccReceiverInternalError
    {
      LM_E(("Runtime Error (exception getting OID: %s)", responseP->subscribeError.errorCode.details.c_str()));
    }
    return SccOk;
  }

  if (!collectionFindOne(getSubscribeContextCollectionName(tenant), BSON("_id" << id), &sub, &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo db exception)", reqSemTaken);
    responseP->subscribeError.errorCode.fill(SccReceiverInternalError, err);
    return SccOk;
  }

  if (sub.isEmpty())
  {
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
  newSub.appendArray(CSUB_ENTITIES, getField(sub, CSUB_ENTITIES).Obj());
  newSub.appendArray(CSUB_ATTRS, getField(sub, CSUB_ATTRS).Obj());
  newSub.append(CSUB_REFERENCE, getStringField(sub, CSUB_REFERENCE));

  /* Duration update */
  long long expiration = getCurrentTime() + requestP->duration.parse();
  if (requestP->duration.isEmpty())
  {
    //
    // No duration in incoming request => "inherit" expirationDate from 'old' subscription
    //
    long long expirationTime = sub.hasField(CSUB_EXPIRATION)? getIntOrLongFieldAsLong(sub, CSUB_EXPIRATION) : -1;

    newSub.append(CSUB_EXPIRATION, expirationTime);
  }
  else
  {
    newSub.append(CSUB_EXPIRATION, expiration);
    LM_T(LmtMongo, ("New subscription expiration: %l", expiration));
  }

  /* Restriction update */
  // FIXME: Restrictions not implemented yet

  /* Throttling update */
  if (!requestP->throttling.isEmpty())
  {
    /* Throttling equal to 0 removes throttling */
    long long throttling = requestP->throttling.parse();

    if (throttling != 0)
    {
      newSub.append(CSUB_THROTTLING, throttling);
    }
  }
  else
  {
    //
    // The hasField check is needed as Throttling might not be present in the original doc
    // FIXME P1: However, we could include Throttling in the new doc anyway ...
    //
    if (sub.hasField(CSUB_THROTTLING))
    {
      newSub.append(CSUB_THROTTLING, getIntOrLongFieldAsLong(sub, CSUB_THROTTLING));
    }
  }

  /* Notify conditions */
  bool notificationDone = false;
  if (requestP->notifyConditionVector.size() == 0) {
    newSub.appendArray(CSUB_CONDITIONS, getField(sub, CSUB_CONDITIONS).embeddedObject());
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
                                                getStringField(sub, CSUB_REFERENCE).c_str(),
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


  
  int count = sub.hasField(CSUB_COUNT) ? getIntField(sub, CSUB_COUNT) : 0;

  //
  // Update from cached value, is applicable
  //
  CachedSubscription*  cSubP                = mongoSubCacheItemLookup(tenant.c_str(), requestP->subscriptionId.get().c_str());
  long long            lastNotificationTime = 0;

  if (cSubP != NULL)
  {
    count                += cSubP->count;
    lastNotificationTime  = cSubP->lastNotificationTime;
  }

  /* Last notification */
  if (notificationDone)
  {
    lastNotificationTime = getCurrentTime();

    //
    // Update sub-cache
    //
    if (cSubP != NULL)
    {
      cSubP->count                 += 1; // 'count' to be reset later if DB operation OK
      cSubP->lastNotificationTime  = lastNotificationTime;
    }

    newSub.append(CSUB_LASTNOTIFICATION, lastNotificationTime);
    newSub.append(CSUB_COUNT, count + 1);
    LM_T(LmtMongoSubCache, ("notificationDone => lastNotification set to %lu", lastNotificationTime));
  }
  else
  {
    //
    // FIXME P1: if CSUB_LASTNOTIFICATION is not in the original doc, it will also not be in the new doc.
    //           Is this necessary?
    //           The implementation would get a lot simpler if we ALWAYS add CSUB_LASTNOTIFICATION and CSUB_COUNT
    //           to 'newSub'
    //

    /* The hasField checks are needed as lastNotification/count might not be present in the original doc */
    if (sub.hasField(CSUB_LASTNOTIFICATION))
    {
      long long lastNotification = getIntOrLongFieldAsLong(sub, CSUB_LASTNOTIFICATION);

      //
      // Compare with 'lastNotificationTime', that might come from the sub-cache.
      // If the cached value of lastNotificationTime is higher, then use it.
      //
      if (lastNotificationTime > lastNotification)
      {
        lastNotification = lastNotificationTime;
      }
      newSub.append(CSUB_LASTNOTIFICATION, lastNotification);
    }

    if (sub.hasField(CSUB_COUNT))
    {
      newSub.append(CSUB_COUNT, count);
    }
  }

  /* Adding format to use in notifications */
  newSub.append(CSUB_FORMAT, std::string(formatToString(notifyFormat)));

  /* Update document in MongoDB */
  BSONObj  newSubObject = newSub.obj();
  if (!collectionUpdate(getSubscribeContextCollectionName(tenant), BSON("_id" << OID(requestP->subscriptionId.get())), newSubObject, false, &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 update subscription request (mongo db exception)", reqSemTaken);
    responseP->subscribeError.errorCode.fill(SccReceiverInternalError, err);
    return SccOk;
  }

  if (cSubP != NULL)
  {
    cSubP->count = 0; // New 'count' has been sent to DB - must be reset here
  }

  // Duration and throttling are optional parameters, they are only added in the case they were used for update
  if (!requestP->duration.isEmpty())
  {
    responseP->subscribeResponse.duration = requestP->duration;
  }

  if (!requestP->throttling.isEmpty())
  {
    responseP->subscribeResponse.throttling = requestP->throttling;
  }
  responseP->subscribeResponse.subscriptionId = requestP->subscriptionId;



  //
  // Modification of the subscription cache
  //
  // The subscription "before this update" is looked up in cache and referenced by 'cSubP'.
  // The "updated subscription information" is in 'newSubObject' (mongo BSON object format).
  // 
  // All we need to do now for the cache is to:
  //   1. Remove 'cSubP' from sub-cache (if present)
  //   2. Create 'newSubObject' in sub-cache (if applicable)
  //
  // The subscription is already updated in mongo.
  //
  //
  // There are four different scenarios here:
  //   1. Old sub was in cache, new sub enters cache
  //   2. Old sub was NOT in cache, new sub enters cache
  //   3. Old subwas in cache, new sub DOES NOT enter cache
  //   4. Old sub was NOT in cache, new sub DOES NOT enter cache
  //
  // This is resolved by two separate functions, one that removes the old one, if found (mongoSubCacheItemLookup+mongoSubCacheItemRemove), 
  // and the other one that inserts the sub, IF it should be inserted (mongoSubCacheItemInsert).
  // If inserted, mongoSubCacheUpdateStatisticsIncrement is called to update the statistics counter of insertions.
  //


  // 0. Lookup matching subscription in subscription-cache

  cacheSemTake(__FUNCTION__, "Updating cached subscription");

  cSubP = mongoSubCacheItemLookup(tenant.c_str(), requestP->subscriptionId.get().c_str());

  char* subscriptionId   = (char*) requestP->subscriptionId.get().c_str();
  char* servicePath      = (char*) ((cSubP == NULL)? "" : cSubP->servicePath);

  LM_T(LmtMongoSubCache, ("update: %s", newSubObject.toString().c_str()));

  int mscInsert = mongoSubCacheItemInsert(tenant.c_str(), newSubObject, subscriptionId, servicePath, lastNotificationTime, expiration);

  if (cSubP != NULL)
  {
    LM_T(LmtMongoSubCache, ("Calling mongoSubCacheItemRemove"));
    mongoSubCacheItemRemove(cSubP);
  }

  if (mscInsert == 0)  // 0: Insertion was really made
  {
    mongoSubCacheUpdateStatisticsIncrement();
  }

  cacheSemGive(__FUNCTION__, "Updating cached subscription");
  reqSemGive(__FUNCTION__, "ngsi10 update subscription request", reqSemTaken);

  return SccOk;
}
