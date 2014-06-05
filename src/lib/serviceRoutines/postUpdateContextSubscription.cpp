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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "mongoBackend/mongoUpdateContextSubscription.h"
#include "ngsi/ParseData.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"
#include "rest/ConnectionInfo.h"
#include "serviceRoutines/postUpdateContextSubscription.h"



/* ****************************************************************************
*
* postUpdateContextSubscription - 
*/
std::string postUpdateContextSubscription(ConnectionInfo* ciP, int components, std::vector<std::string>& compV, ParseData* parseDataP)
{
  UpdateContextSubscriptionResponse  ucsr;
  std::string                        answer;

  ucsr.subscribeError.subscriptionId = parseDataP->ucsr.res.subscriptionId;

  // FIXME P6: by the moment, we are assuming that notification will be sent in the same format than the one
  // used to do the subscription, so we are passing ciP->inFomat. This is just an heuristic, the client could want
  // for example to use XML in the subscription message but wants notifications in JSON. We need a more
  // flexible approach, to be implemented
  ciP->httpStatusCode = mongoUpdateContextSubscription(&parseDataP->ucsr.res, &ucsr, ciP->inFormat, ciP->tenant);
  answer = ucsr.render(UpdateContextSubscription, ciP->outFormat, "");
  return answer;
}
