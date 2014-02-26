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

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUnsubscribeContextAvailability.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"
#include "ngsi9/UnsubscribeContextAvailabilityResponse.h"

#include "common/sem.h"

/* ****************************************************************************
*
* mongoUnsubscribeContextAvailability - 
*/
HttpStatusCode mongoUnsubscribeContextAvailability(UnsubscribeContextAvailabilityRequest* requestP, UnsubscribeContextAvailabilityResponse* responseP)
{
  LM_T(LmtMongo, ("Unsubscribe Context Availability"));

  DBClientConnection* connection = getMongoConnection();

  /* No matter if success or failure, the subscriptionId in the response is always the one
   * in the request */
  responseP->subscriptionId = requestP->subscriptionId;

  /* Look for document */
  BSONObj sub;
  try {
      OID id = OID(requestP->subscriptionId.get());
      LM_T(LmtMongo, ("findOne() in '%s' collection _id '%s'}", getSubscribeContextAvailabilityCollectionName(),
                         requestP->subscriptionId.get().c_str()));
      mongoSemTake(__FUNCTION__, "findOne in SubscribeContextAvailabilityCollection");
      sub = connection->findOne(getSubscribeContextAvailabilityCollectionName(), BSON("_id" << id));
      mongoSemGive(__FUNCTION__, "findOne in SubscribeContextAvailabilityCollection");
      if (sub.isEmpty()) {
          responseP->statusCode.fill(SccContextElementNotFound);
          return SccOk;
      }
  }
  catch( const AssertionException &e ) {
      /* This happens when OID format is wrong */
      // FIXME: this checking should be done at parsing stage, without progressing to
      // mongoBackend. By the moment we can live this here, but we should remove in the future
      // (odl issues #95)
      mongoSemGive(__FUNCTION__, "findOne in SubscribeContextAvailabilityCollection (AssertionException)");
      responseP->statusCode.fill(SccContextElementNotFound);
      mongoSemGive(__FUNCTION__, "");
      return SccOk;
  }
  catch( const DBException &e ) {
      mongoSemGive(__FUNCTION__, "findOne in SubscribeContextAvailabilityCollection (DBException)");
      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName() +
                                 " - findOne() _id: " + requestP->subscriptionId.get() +
                                 " - exception: " + e.what());
      mongoSemGive(__FUNCTION__, "");
      return SccOk;
  }
  catch(...) {
      mongoSemGive(__FUNCTION__, "findOne in SubscribeContextAvailabilityCollection (Generic Exception)");
      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName() +
                                 " - findOne() _id: " + requestP->subscriptionId.get() +
                                 " - exception: " + "generic");
      mongoSemGive(__FUNCTION__, "");
      return SccOk;
  }

  /* Remove document in MongoDB */
  // FIXME: I would prefer to do the find and remove in a single operation. Is the some similar
  // to findAndModify for this?
  try {
      LM_T(LmtMongo, ("remove() in '%s' collection _id '%s'}", getSubscribeContextAvailabilityCollectionName(),
                         requestP->subscriptionId.get().c_str()));
      mongoSemTake(__FUNCTION__, "remove in SubscribeContextAvailabilityCollection");
      connection->remove(getSubscribeContextAvailabilityCollectionName(), BSON("_id" << OID(requestP->subscriptionId.get())));
      mongoSemGive(__FUNCTION__, "remove in SubscribeContextAvailabilityCollection");
  }
  catch( const DBException &e ) {
      mongoSemGive(__FUNCTION__, "remove in SubscribeContextAvailabilityCollection (DBException)");
      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName() +
                                 " - remove() _id: " + requestP->subscriptionId.get().c_str() +
                                 " - exception: " + e.what());
      return SccOk;
  }
  catch(...) {
      mongoSemGive(__FUNCTION__, "remove in SubscribeContextAvailabilityCollection (Generic Exception)");
      responseP->statusCode.fill(SccReceiverInternalError,
                                 std::string("collection: ") + getSubscribeContextAvailabilityCollectionName() +
                                 " - remove() _id: " + requestP->subscriptionId.get().c_str() +
                                 " - exception: " + "generic");
      return SccOk;
  }

  responseP->statusCode.fill(SccOk);
  return SccOk;
}
