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
#include "alarmMgr/alarmMgr.h"
#include "common/sem.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"
#include "ngsi9/UnsubscribeContextAvailabilityResponse.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUnsubscribeContextAvailability.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/safeMongo.h"



/* ****************************************************************************
*
* mongoUnsubscribeContextAvailability -
*/
HttpStatusCode mongoUnsubscribeContextAvailability
(
  UnsubscribeContextAvailabilityRequest*   requestP,
  UnsubscribeContextAvailabilityResponse*  responseP,
  const std::string&                       tenant
)
{
  bool        reqSemTaken;
  std::string err;

  reqSemTake(__FUNCTION__, "ngsi9 unsubscribe request", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Unsubscribe Context Availability"));

  /* No matter if success or failure, the subscriptionId in the response is always the one
   * in the request */
  responseP->subscriptionId = requestP->subscriptionId;

  /* Look for document */
  orion::BSONObj sub;
  orion::OID     id;

  if (!safeGetSubId(requestP->subscriptionId, &id, &(responseP->statusCode)))
  {
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (safeGetSubId fail)", reqSemTaken);

    if (responseP->statusCode.code == SccContextElementNotFound)
    {
      // FIXME: doubt: invalid OID format? Or, subscription not found?
      std::string details = std::string("invalid OID format: '") + requestP->subscriptionId.get() + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else  // SccReceiverInternalError
    {
      LM_E(("Runtime Error (exception getting OID: %s)", responseP->statusCode.details.c_str()));
    }

    return SccOk;
  }

  orion::BSONObjBuilder bobId;
  bobId.append("_id", id);

  if (!orion::collectionFindOne(getSubscribeContextAvailabilityCollectionName(tenant), bobId.obj(), &sub, &err))
  {
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo db exception)", reqSemTaken);
    responseP->statusCode.fill(SccReceiverInternalError, err);

    return SccOk;
  }

  alarmMgr.dbErrorReset();

  if (sub.isEmpty())
  {
    responseP->statusCode.fill(SccContextElementNotFound);
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (no subscriptions)", reqSemTaken);

    return SccOk;
  }

  /* Remove document in MongoDB */

  //
  // FIXME: I would prefer to do the find and remove in a single operation. Is the some similar
  // to findAndModify for this?
  //

  orion::BSONObjBuilder bobId2;
  bobId2.append("_id", orion::OID(requestP->subscriptionId.get()));

  std::string colName = getSubscribeContextAvailabilityCollectionName(tenant);
  if (!orion::collectionRemove(colName, bobId2.obj(), &err))
  {
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo db exception)", reqSemTaken);
    responseP->statusCode.fill(SccReceiverInternalError, err);

    return SccOk;
  }

  reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request", reqSemTaken);
  responseP->statusCode.fill(SccOk);

  return SccOk;
}
