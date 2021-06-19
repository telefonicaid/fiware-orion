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
#include <map>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "orionld/types/OrionldTenant.h"             // OrionldTenant
#include "orionld/common/orionldState.h"             // orionldState
#include "orionld/common/tenantList.h"               // tenant0

#include "common/globals.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubscribeContextAvailability.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/dbConstants.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"
#include "rest/uriParamNames.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONArrayBuilder;
using mongo::BSONObjBuilder;
using mongo::OID;



/* ****************************************************************************
*
* mongoSubscribeContextAvailability - 
*/
HttpStatusCode mongoSubscribeContextAvailability
(
  SubscribeContextAvailabilityRequest*   requestP,
  SubscribeContextAvailabilityResponse*  responseP,
  std::map<std::string, std::string>&    uriParam,
  const std::string&                     fiwareCorrelator,
  OrionldTenant*                         tenantP
)
{
  if (tenantP == NULL)
    tenantP = orionldState.tenantP;

  bool           reqSemTaken;

  reqSemTake(__FUNCTION__, "ngsi9 subscribe request", SemWriteOp, &reqSemTaken);

  /* If expiration is not present, then use a default one */
  if (requestP->duration.isEmpty())
  {
    requestP->duration.set(DEFAULT_DURATION);
  }

  /* Calculate expiration (using the current time and the duration field in the request) */
  long long expiration = orionldState.requestTime + requestP->duration.parse();
  LM_T(LmtMongo, ("Subscription expiration: %d", expiration));

  /* Create the mongoDB subscription document */
  BSONObjBuilder sub;
  OID            oid;

  oid.init();
  sub.append("_id", oid);
  sub.append(CASUB_EXPIRATION, expiration);
  sub.append(CASUB_REFERENCE, requestP->reference.get());

  /* Build entities array */
  BSONArrayBuilder entities;

  for (unsigned int ix = 0; ix < requestP->entityIdVector.size(); ++ix)
  {
    EntityId* en = requestP->entityIdVector[ix];

    if (en->type == "")
    {
      entities.append(BSON(CASUB_ENTITY_ID << en->id <<
                           CASUB_ENTITY_ISPATTERN << en->isPattern));
    }
    else
    {
      entities.append(BSON(CASUB_ENTITY_ID << en->id <<
                           CASUB_ENTITY_TYPE << en->type <<
                           CASUB_ENTITY_ISPATTERN << en->isPattern));
    }
  }

  sub.append(CASUB_ENTITIES, entities.arr());

  /* Build attributes array */
  BSONArrayBuilder attrs;
  for (unsigned int ix = 0; ix < requestP->attributeList.size(); ++ix)
  {
    attrs.append(requestP->attributeList[ix]);
  }
  sub.append(CASUB_ATTRS, attrs.arr());

  //
  // FIXME P5: RenderFormat right now hardcoded to "JSON" (NGSI_V1_LEGACY),
  //           in the future the RenderFormat will be taken from the payload
  //

  /* Adding format to use in notifications */
  sub.append(CASUB_FORMAT, renderFormatToString(NGSI_V1_LEGACY));

  /* Insert document in database */
  std::string err;
  if (!collectionInsert(tenantP->avSubscriptions, sub.obj(), &err))
  {
    reqSemGive(__FUNCTION__, "ngsi9 subscribe request (mongo db exception)", reqSemTaken);
    responseP->errorCode.fill(SccReceiverInternalError, err);
    return SccOk;
  }

  //
  // FIXME P5: RenderFormat right now hardcoded to NGSI_V1_LEGACY,
  //           in the future the RenderFormat will be taken from the payload
  //
  /* Send notifications for matching context registrations */
  processAvailabilitySubscription(requestP->entityIdVector,
                                  requestP->attributeList,
                                  oid.toString(),
                                  requestP->reference.get(),
                                  NGSI_V1_LEGACY,
                                  tenantP,
                                  fiwareCorrelator);

  /* Fill the response element */
  responseP->duration = requestP->duration;
  responseP->subscriptionId.set(oid.toString());

  reqSemGive(__FUNCTION__, "ngsi9 subscribe request", reqSemTaken);
  return SccOk;
}
