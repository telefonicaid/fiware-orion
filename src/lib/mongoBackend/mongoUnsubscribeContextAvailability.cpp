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
  DBClientBase*  connection = NULL;
  bool           reqSemTaken;

  reqSemTake(__FUNCTION__, "ngsi9 unsubscribe request", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Unsubscribe Context Availability"));

  /* No matter if success or failure, the subscriptionId in the response is always the one
   * in the request */
  responseP->subscriptionId = requestP->subscriptionId;

  /* Look for document */
  BSONObj sub;
  try
  {
      OID id = OID(requestP->subscriptionId.get());
      LM_T(LmtMongo, ("findOne() in '%s' collection _id '%s'}", getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
                         requestP->subscriptionId.get().c_str()));


      connection = getMongoConnection();
      sub = connection->findOne(getSubscribeContextAvailabilityCollectionName(tenant).c_str(), BSON("_id" << id));
      releaseMongoConnection(connection);

      LM_I(("Database Operation Successful (findOne _id: %s)", id.toString().c_str()));

      if (sub.isEmpty())
      {
          responseP->statusCode.fill(SccContextElementNotFound);
          reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (no subscriptions)", reqSemTaken);
          return SccOk;
      }
  }
  catch (const AssertionException &e)
  {
      //
      // This happens when OID format is wrong
      // FIXME: this checking should be done at parsing stage, without progressing to
      // mongoBackend. By the moment we can live this here, but we should remove in the future
      // (odl issues #95)
      //
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo assertion exception)", reqSemTaken);

      responseP->statusCode.fill(SccContextElementNotFound);
      LM_W(("Bad Input (invalid OID format)"));
      return SccOk;
  }
  catch (const DBException &e)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo db exception)", reqSemTaken);

      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                 " - findOne() _id: " + requestP->subscriptionId.get() +
                                 " - exception: " + e.what());
      LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
      return SccOk;
  }
  catch (...)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo generic exception)", reqSemTaken);

      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                 " - findOne() _id: " + requestP->subscriptionId.get() +
                                 " - exception: " + "generic");
      LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
      return SccOk;
  }

  /* Remove document in MongoDB */
  // FIXME: I would prefer to do the find and remove in a single operation. Is the some similar
  // to findAndModify for this?

  LM_T(LmtMongo, ("remove() in '%s' collection _id '%s'}", getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
                  requestP->subscriptionId.get().c_str()));

  try
  {
      connection = getMongoConnection();
      connection->remove(getSubscribeContextAvailabilityCollectionName(tenant).c_str(), BSON("_id" << OID(requestP->subscriptionId.get())));
      releaseMongoConnection(connection);

      LM_I(("Database Operation Successful (remove _id: %s)", requestP->subscriptionId.get().c_str()));
  }
  catch (const DBException &e)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo db exception)", reqSemTaken);

      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                 " - remove() _id: " + requestP->subscriptionId.get().c_str() +
                                 " - exception: " + e.what());
      LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
      return SccOk;
  }
  catch (...)
  {
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request (mongo generic exception)", reqSemTaken);

      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName(tenant).c_str() +
                                 " - remove() _id: " + requestP->subscriptionId.get().c_str() +
                                 " - exception: " + "generic");
      LM_E(("Database Error (%s)", responseP->statusCode.details.c_str()));
      return SccOk;
  }

  reqSemGive(__FUNCTION__, "ngsi9 unsubscribe request", reqSemTaken);
  responseP->statusCode.fill(SccOk);
  return SccOk;
}
