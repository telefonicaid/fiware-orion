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
#include "common/defaultValues.h"
#include "common/RenderFormat.h"
#include "common/sem.h"
#include "alarmMgr/alarmMgr.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoUpdateContextSubscription.h"
#include "mongoBackend/mongoSubCache.h"
#include "cache/subCache.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"



/* ****************************************************************************
*
* mongoUpdateContextSubscription - 
*/
HttpStatusCode mongoUpdateContextSubscription
(
    UpdateContextSubscriptionRequest*   requestP,
    UpdateContextSubscriptionResponse*  responseP,
    const std::string&                  tenant,
    const std::string&                  xauthToken,
    const std::vector<std::string>&     servicePathV,
    const std::string&                  fiwareCorrelator,
    std::string                         version
)
{ 
  bool  reqSemTaken;

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
      std::string details = std::string("invalid OID mimeType: '") + requestP->subscriptionId.get() + "'";
      alarmMgr.badInput(clientIp, details);
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

  if (version != "v2")
  {
    /* Entities, attribute list and reference are not updatable, so they are appended directly */
    newSub.appendArray(CSUB_ENTITIES, getFieldF(sub, CSUB_ENTITIES).Obj());
    newSub.appendArray(CSUB_ATTRS, getFieldF(sub, CSUB_ATTRS).Obj());
    newSub.append(CSUB_REFERENCE, getStringFieldF(sub, CSUB_REFERENCE));
  }
  else // v2
  {
    // Reference
    std::string ref;
    if (!requestP->reference.isEmpty())
    {
      ref = requestP->reference.get();
    }
    else
    {
      ref = getStringFieldF(sub, CSUB_REFERENCE);
    }
    newSub.append(CSUB_REFERENCE, ref);

    // Entities
    if (requestP->entityIdVector.size() > 0)
    {
      /* Build entities array */
      BSONArrayBuilder entities;
      for (unsigned int ix = 0; ix < requestP->entityIdVector.size(); ++ix)
      {
        EntityId* en = requestP->entityIdVector[ix];

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
      newSub.append(CSUB_ENTITIES, entities.arr());
    }
    else
    {
      newSub.appendArray(CSUB_ENTITIES, getFieldF(sub, CSUB_ENTITIES).Obj());
    }

    // Attributes
    if (requestP->attributeList.size() > 0)
    {
      /* Build attributes array */
      BSONArrayBuilder attrs;
      for (unsigned int ix = 0; ix < requestP->attributeList.size(); ++ix) {
        attrs.append(requestP->attributeList[ix]);
      }
      newSub.append(CSUB_ATTRS, attrs.arr());
    }
    else
    {
      newSub.appendArray(CSUB_ATTRS, getFieldF(sub, CSUB_ATTRS).Obj());
    }
  }

  /* Expiration */
  long long expiration = sub.hasField(CSUB_EXPIRATION)? getIntOrLongFieldAsLongF(sub, CSUB_EXPIRATION) : -1;
  if (version == "v1")
  {
    // Based on duration
    if (!requestP->duration.isEmpty())
    {
      expiration = getCurrentTime() + requestP->duration.parse();
    }
  }
  else // v2
  {
    // Based on expires
    if (requestP->expires > 0)
    {
      expiration = requestP->expires;
    }
  }
  newSub.append(CSUB_EXPIRATION, expiration);
  LM_T(LmtMongo, ("New subscription expiration: %ld", (long) expiration));

  /* ServicePath update */
  newSub.append(CSUB_SERVICE_PATH, servicePathV[0] == "" ? DEFAULT_SERVICE_PATH_QUERIES : servicePathV[0]);

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
  else if (requestP->throttling.seconds > 0) // v2
  {
      newSub.append(CSUB_THROTTLING, (long long) requestP->throttling.seconds);
  }
  else
  {
    //
    // The hasField check is needed as Throttling might not be present in the original doc
    // FIXME P1: However, we could include Throttling in the new doc anyway ...
    //
    if (sub.hasField(CSUB_THROTTLING))
    {
      newSub.append(CSUB_THROTTLING, getIntOrLongFieldAsLongF(sub, CSUB_THROTTLING));
    }
  }


  //
  // StringFilter in Scope?
  //
  // Any Scope of type SCOPE_TYPE_SIMPLE_QUERY in requestP->restriction.scopeVector?
  // If so, set it as string filter to the sub-cache item
  //
  StringFilter*  stringFilterP = NULL;

  for (unsigned int ix = 0; ix < requestP->restriction.scopeVector.size(); ++ix)
  {
    if (requestP->restriction.scopeVector[ix]->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      stringFilterP = requestP->restriction.scopeVector[ix]->stringFilterP;
    }
  }
  

  /* Description */
  if (requestP->descriptionProvided)
  {
    // Note that in the case of description "" the field is deleted
    if (requestP->description != "")
    {
      newSub.append(CSUB_DESCRIPTION, requestP->description);
    }
  }
  else
  {
    // Pass-through of the current subscription
    if (sub.hasField(CSUB_DESCRIPTION))
    {
      newSub.append(CSUB_DESCRIPTION, getStringFieldF(sub, CSUB_DESCRIPTION));
    }
  }

  /* Adding status */
  std::string status;
  if (requestP->status != "")
  {
    status = requestP->status;
  }
  else
  {
    // Note this code will make that subscriptions not using status field (typically, legacy
    // subscriptions from pre-1.1.0 versions) will get status 'active' first time they get
    // updated (either the status field is included in that update or not)
    status = sub.hasField(CSUB_STATUS) ? getStringFieldF(sub, CSUB_STATUS) : STATUS_ACTIVE;
  }
  newSub.append(CSUB_STATUS, status);


  /* Notify conditions */
  bool notificationDone = false;
  if (requestP->notifyConditionVector.size() == 0)
  {
    newSub.appendArray(CSUB_CONDITIONS, getFieldF(sub, CSUB_CONDITIONS).embeddedObject());
  }
  else
  {
      /* Build conditions array (including side-effect notifications and threads creation)
       * In order to do so, we have to create and EntityIdVector and AttributeList from sub
       * document, given the processConditionVector() signature */
       EntityIdVector enV;
       AttributeList attrL;
       if (version == "v1")
       {
         enV   = subToEntityIdVector(sub);
         attrL = subToAttributeList(sub);
       }
       else // v2
       {
         // In v2 entities and attribute are updatable (as part of subject or notification) so
         // we have to check in order to know if we get the attribute from the request or from
         // the subscription
         if (requestP->entityIdVector.size() > 0)
         {
           enV = requestP->entityIdVector;
         }
         else
         {
           enV = subToEntityIdVector(sub);
         }

         if (requestP->attributeList.size() > 0)
         {
           attrL = requestP->attributeList;
         }
         else
         {
           attrL = subToAttributeList(sub);
         }
       }

       BSONArray conds = processConditionVector(&requestP->notifyConditionVector,
                                                enV,
                                                attrL,
                                                requestP->subscriptionId.get(),
                                                getStringFieldF(sub, CSUB_REFERENCE).c_str(),
                                                &notificationDone,
                                                requestP->attrsFormat,
                                                tenant,
                                                xauthToken,
                                                servicePathV,
                                                &requestP->restriction,
                                                status,
                                                fiwareCorrelator,
                                                requestP->attributeList.attributeV);


       newSub.appendArray(CSUB_CONDITIONS, conds);

       /* Remove EntityIdVector and AttributeList dynamic memory */
       enV.release();
       attrL.release();
  }


  // Expression
  if (requestP->expression.isSet)
  {
    /* Build expression */
    BSONObjBuilder expression;

    expression << CSUB_EXPR_Q << requestP->expression.q
             << CSUB_EXPR_GEOM << requestP->expression.geometry
             << CSUB_EXPR_COORDS << requestP->expression.coords
             << CSUB_EXPR_GEOREL << requestP->expression.georel;
    newSub.append(CSUB_EXPR, expression.obj());
  }
  else if (sub.hasField(CSUB_EXPR))
  {
    newSub.append(CSUB_EXPR, getFieldF(sub, CSUB_EXPR).Obj());
  }

  long long count = sub.hasField(CSUB_COUNT) ? getIntOrLongFieldAsLongF(sub, CSUB_COUNT) : 0;

  //
  // Update from cached value, if applicable
  //
  CachedSubscription*  cSubP                = subCacheItemLookup(tenant.c_str(), requestP->subscriptionId.get().c_str());
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
    // FIXME PR: this is safe without sem?
    //
    if (cSubP != NULL)
    {
      cSubP->count                 += 1; // 'count' to be reset later if DB operation OK
      cSubP->lastNotificationTime  = lastNotificationTime;
    }

    newSub.append(CSUB_LASTNOTIFICATION, lastNotificationTime);
    newSub.append(CSUB_COUNT, (long long) count + 1);
    LM_T(LmtSubCache, ("notificationDone => lastNotification set to %lu", lastNotificationTime));
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
      long long lastNotification = getIntOrLongFieldAsLongF(sub, CSUB_LASTNOTIFICATION);

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
      newSub.append(CSUB_COUNT, (long long) count);
    }
  }


  //
  // Adding format to use in notifications
  //
  std::string   renderFormatString  = "";
  RenderFormat  renderFormat        = NO_FORMAT;

  if (requestP->attrsFormat != NO_FORMAT)
  {
    renderFormat       = requestP->attrsFormat;
    renderFormatString = renderFormatToString(renderFormat);
    newSub.append(CSUB_FORMAT, renderFormatString.c_str());
  }
  else
  {
    renderFormatString = sub.hasField(CSUB_FORMAT)? getStringFieldF(sub, CSUB_FORMAT) : renderFormatToString(NGSI_V1_LEGACY);
    renderFormat       = stringToRenderFormat(renderFormatString, false);
    newSub.append(CSUB_FORMAT, renderFormatString);
  }


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
  // This is resolved by two separate functions, one that removes the old one, if found (subCacheItemLookup+subCacheItemRemove), 
  // and the other one that inserts the sub, IF it should be inserted (subCacheItemInsert).
  // If inserted, subCacheUpdateStatisticsIncrement is called to update the statistics counter of insertions.
  //


  // 0. Lookup matching subscription in subscription-cache

  cacheSemTake(__FUNCTION__, "Updating cached subscription");

  cSubP = subCacheItemLookup(tenant.c_str(), requestP->subscriptionId.get().c_str());

  char* subscriptionId   = (char*) requestP->subscriptionId.get().c_str();
  char* servicePath      = (char*) ((cSubP == NULL)? "" : cSubP->servicePath);

  LM_T(LmtSubCache, ("update: %s", newSubObject.toString().c_str()));

  int mscInsert = mongoSubCacheItemInsert(tenant.c_str(),
                                          newSubObject,
                                          subscriptionId,
                                          servicePath,
                                          lastNotificationTime,
                                          expiration,
                                          status,
                                          requestP->expression.q,
                                          requestP->expression.geometry,
                                          requestP->expression.coords,
                                          requestP->expression.georel,
                                          stringFilterP,
                                          renderFormat);

  if (cSubP != NULL)
  {
    LM_T(LmtSubCache, ("Calling subCacheItemRemove"));
    subCacheItemRemove(cSubP);
  }

  if (mscInsert == 0)  // 0: Insertion was really made
  {
    subCacheUpdateStatisticsIncrement();
  }

  cacheSemGive(__FUNCTION__, "Updating cached subscription");
  reqSemGive(__FUNCTION__, "ngsi10 update subscription request", reqSemTaken);

  return SccOk;
}
