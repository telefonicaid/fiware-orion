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
#include "serviceRoutines/postUpdateContextSubscription.h"
#include "serviceRoutines/putSubscriptionConvOp.h"



/* ****************************************************************************
*
* putSubscriptionConvOp - 
*
* PUT /v1/contextSubscriptions/{subscriptionId}
*
* Payload In:  UpdateContextSubscriptionRequest
* Payload Out: UpdateContextSubscriptionResponse
*
* URI parameters
*   x entity::type=TYPE    (NOT TREATED)
*   x !exist=entity::type  (NOT TREATED)
*   x exist=entity::type   (NOT TREATED)
*/
std::string putSubscriptionConvOp
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string                        subscriptionId = compV[2];
  UpdateContextSubscriptionRequest*  ucsrP          = &parseDataP->ucsr.res;

  if (subscriptionId != ucsrP->subscriptionId.get())
  {
    std::string out;
    std::string details = std::string("unmatching subscriptionId URI/payload: /") + subscriptionId + "/ vs /" + ucsrP->subscriptionId.get() + "/";

    restErrorReplyGet(ciP, SccBadRequest, details, &out);

    return out;
  }

  return postUpdateContextSubscription(ciP, components, compV, parseDataP);
}
