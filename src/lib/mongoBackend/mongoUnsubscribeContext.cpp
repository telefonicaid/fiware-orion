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

#include "common/sem.h"
#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoUnsubscribeContext.h"
#include "mongoBackend/safeMongo.h"
#include "cache/subCache.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObj;
using mongo::OID;



/* ****************************************************************************
*
* mongoUnsubscribeContext - 
*/
HttpStatusCode mongoUnsubscribeContext
(
  UnsubscribeContextRequest*   requestP,
  UnsubscribeContextResponse*  responseP,
  const std::string&           tenant
)
{
  bool         reqSemTaken;
  std::string  err;

  reqSemTake(__FUNCTION__, "ngsi10 unsubscribe request", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Unsubscribe Context"));

  /* No matter if success or failure, the subscriptionId in the response is always the one
   * in the request
   */
  responseP->subscriptionId = requestP->subscriptionId;

  if (responseP->subscriptionId.get() == "")
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)", reqSemTaken);
    responseP->statusCode.fill(SccContextElementNotFound);
    responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);
    alarmMgr.badInput(clientIp, "no subscriptionId");

    return SccOk;
  }

  /* Look for document */
  BSONObj sub;
  OID     id;

  if (!safeGetSubId(requestP->subscriptionId, &id, &(responseP->statusCode)))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (safeGetSubId fail)", reqSemTaken);

    if (responseP->statusCode.code == SccContextElementNotFound)
    {
      // FIXME: Doubt - invalid OID format?  Or, just a subscription that was not found?
      std::string details = std::string("invalid OID format: '") + requestP->subscriptionId.get() + "'";

      responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);
      alarmMgr.badInput(clientIp, details);
    }
    else  // SccReceiverInternalError
    {
      responseP->oe.fill(SccReceiverInternalError, responseP->statusCode.details, "InternalError");
      LM_E(("Runtime Error (exception getting OID: %s)", responseP->statusCode.details.c_str()));
    }

    return SccOk;
  }

  if (!collectionFindOne(getSubscribeContextCollectionName(tenant), BSON("_id" << id), &sub, &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);

    responseP->statusCode.fill(SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalError");

    return SccOk;
  }

  if (sub.isEmpty())
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)", reqSemTaken);

    responseP->statusCode.fill(SccContextElementNotFound,
                               std::string("subscriptionId: /") + requestP->subscriptionId.get() + "/");
    responseP->oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);

    return SccOk;
  }

  /* Remove document in MongoDB */

  //
  // FIXME: I would prefer to do the find and remove in a single operation. Is there something similar
  // to findAndModify for this?
  //
  std::string colName = getSubscribeContextCollectionName(tenant);

  if (!collectionRemove(colName, BSON("_id" << OID(requestP->subscriptionId.get())), &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);

    responseP->statusCode.fill(SccReceiverInternalError, err);
    responseP->oe.fill(SccReceiverInternalError, err, "InternalError");

    return SccOk;
  }

  //
  // Removing subscription from mongo subscription cache
  //
  LM_T(LmtSubCache, ("removing subscription '%s' (tenant '%s') from mongo subscription cache",
                     requestP->subscriptionId.get().c_str(),
                     tenant.c_str()));

  cacheSemTake(__FUNCTION__, "Removing subscription from cache");

  CachedSubscription* cSubP = subCacheItemLookup(tenant.c_str(), requestP->subscriptionId.get().c_str());

  if (cSubP != NULL)
  {
    subCacheItemRemove(cSubP);
  }

  cacheSemGive(__FUNCTION__, "Removing subscription from cache");
  reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request", reqSemTaken);
  responseP->statusCode.fill(SccOk);

  return SccOk;
}



#ifdef ORIONLD
/* ****************************************************************************
*
* mongoDeleteLdSubscription -
*/
bool mongoDeleteLdSubscription
(
  const char*     subId,
  const char*     tenant,
  HttpStatusCode* statusCodeP,
  char**          details
)
{
  bool         reqSemTaken;
  std::string  err;

  reqSemTake(__FUNCTION__, "ngsi-ld unsubscribe request", SemWriteOp, &reqSemTaken);

  BSONObj sub;
  if (!collectionFindOne(getSubscribeContextCollectionName(tenant), BSON("_id" << subId), &sub, &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);

    LM_E(("collectionFindOne error: %s", err.c_str()));
    *details     = (char*) "error finding the subscription";
    *statusCodeP = SccReceiverInternalError;

    return false;
  }

  if (sub.isEmpty())
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)", reqSemTaken);

    *details     = (char*) ERROR_DESC_NOT_FOUND_SUBSCRIPTION;
    *statusCodeP = SccContextElementNotFound;

    return false;
  }

  /* Remove document in MongoDB */

  //
  // FIXME: I would prefer to do the find and remove in a single operation. Is there something similar
  // to findAndModify for this?
  //
  std::string colName = getSubscribeContextCollectionName(tenant);

  if (!collectionRemove(colName, BSON("_id" << subId), &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);

    *details     = (char*) "error removing subscription";
    *statusCodeP = SccReceiverInternalError;

    return false;
  }

#if 0
  // FIXME - sub-cache comes later

  //
  // Removing subscription from mongo subscription cache
  //
  LM_T(LmtSubCache, ("removing subscription '%s' (tenant '%s') from mongo subscription cache",
                     requestP->subscriptionId.get().c_str(),
                     tenant.c_str()));

  cacheSemTake(__FUNCTION__, "Removing subscription from cache");

  CachedSubscription* cSubP = subCacheItemLookup(tenant, subId);
  if (cSubP != NULL)
    subCacheItemRemove(cSubP);

  cacheSemGive(__FUNCTION__, "Removing subscription from cache");
#endif

  reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request", reqSemTaken);
  *statusCodeP = SccOk;
  return true;
}
#endif
