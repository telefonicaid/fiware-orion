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

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoUnsubscribeContextAvailability.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"
#include "ngsi9/UnsubscribeContextAvailabilityResponse.h"

#include "common/sem.h"

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
  BSONObj sub;
  OID     id;
  try
  {
    id = OID(requestP->subscriptionId.get());
  }
  catch (const AssertionException &e)
  {
      //
      // This happens when OID format is wrong
      // FIXME: this checking should be done at parsing stage, without progressing to
      // mongoBackend. By the moment we can live this here, but we should remove in the future
      // (odl issues #95)
      //
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo assertion exception)", reqSemTaken);
      responseP->statusCode.fill(SccContextElementNotFound);
      LM_W(("Bad Input (invalid OID format)"));
      return SccOk;
  }
  catch (...)
  {
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo generic exception)", reqSemTaken);
      responseP->statusCode.fill(SccReceiverInternalError);
      LM_E(("Runtime Error (generic exception getting OID)"));
      return SccOk;
  }

  if (!collectionFindOne(getSubscribeContextAvailabilityCollectionName(tenant), BSON("_id" << id), &sub, &err))
  {
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo db exception)", reqSemTaken);
    responseP->statusCode.fill(SccReceiverInternalError, err);
    LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
    return SccOk;
  }

  if (sub.isEmpty())
  {
    responseP->statusCode.fill(SccContextElementNotFound);
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (no subscriptions)", reqSemTaken);
    return SccOk;
  }

  /* Remove document in MongoDB */
  // FIXME: I would prefer to do the find and remove in a single operation. Is the some similar
  // to findAndModify for this?  
  if (!collectionRemove(getSubscribeContextAvailabilityCollectionName(tenant), BSON("_id" << OID(requestP->subscriptionId.get())), &err))
  {
    reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo db exception)", reqSemTaken);
    responseP->statusCode.fill(SccReceiverInternalError, err);
    return SccOk;
  }

  reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request", reqSemTaken);
  responseP->statusCode.fill(SccOk);
  return SccOk;
}
