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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/restReply.h"

#include "serviceRoutines/postUpdateContextAvailabilitySubscription.h"
#include "serviceRoutines/putAvailabilitySubscriptionConvOp.h"


/* ****************************************************************************
*
* putAvailabilitySubscriptionConvOp - update an ngsi9 subscription
*
* PUT /v1/registry/contextAvailabilitySubscriptions/{subscriptionId}
* PUT /ngsi9/contextAvailabilitySubscriptions/{subscriptionId}
*
* Payload In:  UpdateContextAvailabilitySubscriptionRequest
* Payload Out: UpdateContextAvailabilitySubscriptionResponse
*
* 1. The payload in is exactly the same as for the corresponding standard operation
*    /v1/registry/updateContextAvailabilitySubscription, so, nothing needs to be done.
* 2. Make sure the subscriptionId in the URL is *exactly the same* as the subscriptionId
*    in the payload - if not, return an error.
* 3. Call the standard service routine 'postUpdateContextAvailabilitySubscription'
*/
std::string putAvailabilitySubscriptionConvOp
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                                    subscriptionId  = (compV.size() == 4)? compV[3] : compV[2];
  UpdateContextAvailabilitySubscriptionRequest*  ucasP           = &parseDataP->ucas.res;

  if (subscriptionId != ucasP->subscriptionId.get())
  {
    std::string out;
    std::string details = std::string("unmatching subscriptionId URI/payload: /") + subscriptionId + "/ vs /" + ucasP->subscriptionId.get() + "/";

    restErrorReplyGet(ciP, SccBadRequest, details, &out);
    return out;
  }

  return postUpdateContextAvailabilitySubscription(ciP, components, compV, parseDataP);
}
