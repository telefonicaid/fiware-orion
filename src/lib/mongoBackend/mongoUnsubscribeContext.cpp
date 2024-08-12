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
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/mongoUnsubscribeContext.h"
#include "cache/subCache.h"

#include "mongoDriver/safeMongo.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/BSONObjBuilder.h"



/* ****************************************************************************
*
* mongoUnsubscribeContext - 
*/
HttpStatusCode mongoUnsubscribeContext
(
  const std::string&  subId,
  OrionError*         responseP,
  const std::string&  tenant
)
{
  bool         reqSemTaken;
  std::string  err;

  reqSemTake(__FUNCTION__, "ngsi10 unsubscribe request", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Unsubscribe Context"));

  if (subId.empty())
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)", reqSemTaken);
    responseP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);
    alarmMgr.badInput(clientIp, "no subscriptionId");

    return SccOk;
  }

  /* Look for document */
  orion::BSONObj sub;
  orion::OID     id = orion::OID(subId);

  orion::BSONObjBuilder bobId;
  bobId.append("_id", id);

  if (!orion::collectionFindOne(composeDatabaseName(tenant), COL_CSUBS, bobId.obj(), &sub, &err) && (err != ""))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);

    responseP->fill(SccReceiverInternalError, err, ERROR_INTERNAL_ERROR);
    return SccOk;
  }

  if (sub.isEmpty())
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (no subscriptions found)", reqSemTaken);

    responseP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_SUBSCRIPTION, ERROR_NOT_FOUND);
    return SccOk;
  }

  /* Remove document in MongoDB */

  //
  // FIXME: I would prefer to do the find and remove in a single operation. Is there something similar
  // to findAndModify for this?
  //

  orion::BSONObjBuilder bobId2;
  bobId2.append("_id", orion::OID(subId));

  if (!orion::collectionRemove(composeDatabaseName(tenant), COL_CSUBS, bobId2.obj(), &err))
  {
    reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request (mongo db exception)", reqSemTaken);

    responseP->fill(SccReceiverInternalError, err, ERROR_INTERNAL_ERROR);
    return SccOk;
  }

  //
  // Removing subscription from mongo subscription cache
  //
  if (!noCache)
  {
    LM_T(LmtSubCache, ("removing subscription '%s' (tenant '%s') from mongo subscription cache",
                       subId.c_str(),
                       tenant.c_str()));

    cacheSemTake(__FUNCTION__, "Removing subscription from cache");

    CachedSubscription* cSubP = subCacheItemLookup(tenant.c_str(), subId.c_str());

    if (cSubP != NULL)
    {
      subCacheItemRemove(cSubP);
    }

    cacheSemGive(__FUNCTION__, "Removing subscription from cache");
  }

  reqSemGive(__FUNCTION__, "ngsi10 unsubscribe request", reqSemTaken);

  return SccOk;
}
